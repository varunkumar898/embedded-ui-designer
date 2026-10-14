#include "FloatingSelectionToolbar.h"
#include "theme/Theme.h"
#include "CanvasView.h"
#include "CanvasScene.h"
#include "GroupComponent.h"
#include "AddComponentCommand.h"
#include "DeleteComponentCommand.h"
#include "GroupCommand.h"
#include "PropertyChangeCommand.h"
#include "Project.h"
#include <QUndoStack>
#include <QGraphicsItem>
#include <QApplication>
#include <QClipboard>
#include <algorithm>

FloatingSelectionToolbar::FloatingSelectionToolbar(CanvasView* view, CanvasScene* scene, QUndoStack* undoStack, QWidget* parent)
    : QWidget(parent ? parent : view->viewport())
    , m_view(view)
    , m_scene(scene)
    , m_undoStack(undoStack)
{
    setupUi();
    hide();
}

void FloatingSelectionToolbar::setupUi() {
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setObjectName("FloatingSelectionToolbar");

    setStyleSheet(QString(
        "#FloatingSelectionToolbar {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: %3px;"
        "  padding: 2px;"
        "}"
        "QToolButton {"
        "  background: transparent;"
        "  color: %4;"
        "  border: none;"
        "  border-radius: %5px;"
        "  padding: 4px 7px;"
        "  font-size: 11px;"
        "  font-weight: 600;"
        "}"
        "QToolButton:hover {"
        "  background: %6;"
        "  color: %4;"
        "}"
        "QToolButton:pressed {"
        "  background: %7;"
        "}")
        .arg(Theme::BgRaised, Theme::BorderSubtle)
        .arg(Theme::RadiusMd)
        .arg(Theme::TextPrimary)
        .arg(Theme::RadiusSm)
        .arg(Theme::BgOverlay, Theme::BgBase)
    );

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 3, 4, 3);
    layout->setSpacing(3);

    m_lockBtn = new QToolButton(this);
    m_lockBtn->setText("🔒");
    m_lockBtn->setToolTip("Lock / Unlock Component");
    connect(m_lockBtn, &QToolButton::clicked, this, &FloatingSelectionToolbar::onLockUnlockClicked);
    layout->addWidget(m_lockBtn);

    m_duplicateBtn = new QToolButton(this);
    m_duplicateBtn->setText("📄+");
    m_duplicateBtn->setToolTip("Duplicate (Ctrl+D)");
    connect(m_duplicateBtn, &QToolButton::clicked, this, &FloatingSelectionToolbar::onDuplicateClicked);
    layout->addWidget(m_duplicateBtn);

    m_deleteBtn = new QToolButton(this);
    m_deleteBtn->setText("🗑");
    m_deleteBtn->setToolTip("Delete (Del)");
    connect(m_deleteBtn, &QToolButton::clicked, this, &FloatingSelectionToolbar::onDeleteClicked);
    layout->addWidget(m_deleteBtn);

    m_groupBtn = new QToolButton(this);
    m_groupBtn->setText("⊞ Group");
    m_groupBtn->setToolTip("Group Components (Ctrl+G)");
    connect(m_groupBtn, &QToolButton::clicked, this, &FloatingSelectionToolbar::onGroupClicked);
    layout->addWidget(m_groupBtn);

    m_ungroupBtn = new QToolButton(this);
    m_ungroupBtn->setText("⊟ Ungroup");
    m_ungroupBtn->setToolTip("Ungroup (Ctrl+Shift+G)");
    connect(m_ungroupBtn, &QToolButton::clicked, this, &FloatingSelectionToolbar::onUngroupClicked);
    layout->addWidget(m_ungroupBtn);

    m_moreBtn = new QToolButton(this);
    m_moreBtn->setText("•••");
    m_moreBtn->setToolTip("More actions");
    m_moreBtn->setPopupMode(QToolButton::InstantPopup);

    m_moreMenu = new QMenu(this);
    m_flipHAction = m_moreMenu->addAction("Flip Horizontal ⇄", this, &FloatingSelectionToolbar::onFlipHorizontal);
    m_flipVAction = m_moreMenu->addAction("Flip Vertical ⇅", this, &FloatingSelectionToolbar::onFlipVertical);
    m_moreMenu->addSeparator();
    m_bringFrontAction = m_moreMenu->addAction("Bring to Front ⇧", this, &FloatingSelectionToolbar::onBringToFront);
    m_bringForwardAction = m_moreMenu->addAction("Bring Forward ↑", this, &FloatingSelectionToolbar::onBringForward);
    m_sendBackwardAction = m_moreMenu->addAction("Send Backward ↓", this, &FloatingSelectionToolbar::onSendBackward);
    m_sendBackAction = m_moreMenu->addAction("Send to Back ⇩", this, &FloatingSelectionToolbar::onSendToBack);

    m_moreBtn->setMenu(m_moreMenu);
    layout->addWidget(m_moreBtn);

    adjustSize();
}

void FloatingSelectionToolbar::updateSelection(const QList<UIComponent*>& selected) {
    m_selectedComps = selected;
    if (m_selectedComps.isEmpty()) {
        hide();
        return;
    }

    updateButtons();
    show();
    raise();
    reposition();
}

void FloatingSelectionToolbar::updateButtons() {
    if (m_selectedComps.isEmpty()) return;

    bool anyLocked = false;
    for (UIComponent* c : m_selectedComps) {
        if (c && c->isLocked()) {
            anyLocked = true;
            break;
        }
    }

    m_lockBtn->setText(anyLocked ? "🔓 Unlock" : "🔒 Lock");
    m_lockBtn->setToolTip(anyLocked ? "Unlock Component" : "Lock Component");

    m_groupBtn->setVisible(m_selectedComps.size() > 1);

    bool hasGroup = false;
    for (UIComponent* c : m_selectedComps) {
        if (dynamic_cast<GroupComponent*>(c)) {
            hasGroup = true;
            break;
        }
    }
    m_ungroupBtn->setVisible(hasGroup);

    adjustSize();
}

void FloatingSelectionToolbar::reposition() {
    if (m_selectedComps.isEmpty() || !m_view || !m_view->viewport()) return;

    QRectF sceneBounding;
    bool first = true;
    for (UIComponent* c : m_selectedComps) {
        if (!c) continue;
        QRectF r = c->sceneBoundingRect();
        if (first) {
            sceneBounding = r;
            first = false;
        } else {
            sceneBounding = sceneBounding.united(r);
        }
    }

    if (sceneBounding.isNull()) return;

    QPoint topLeft = m_view->mapFromScene(sceneBounding.topLeft());
    QPoint topRight = m_view->mapFromScene(sceneBounding.topRight());
    int centerX = (topLeft.x() + topRight.x()) / 2;

    int toolbarW = width();
    int toolbarH = height();

    int targetX = centerX - toolbarW / 2;
    int targetY = topLeft.y() - toolbarH - 12; // 12px above selection

    // Clamping to viewport bounds
    QRect vpRect = m_view->viewport()->rect();
    if (targetY < 8) {
        // If too close to top, place below selection
        QPoint bottomLeft = m_view->mapFromScene(sceneBounding.bottomLeft());
        targetY = bottomLeft.y() + 12;
    }
    targetX = std::clamp(targetX, 8, std::max(8, vpRect.width() - toolbarW - 8));
    targetY = std::clamp(targetY, 8, std::max(8, vpRect.height() - toolbarH - 8));

    move(targetX, targetY);
}

void FloatingSelectionToolbar::onLockUnlockClicked() {
    if (m_selectedComps.isEmpty()) return;

    bool anyLocked = false;
    for (UIComponent* c : m_selectedComps) {
        if (c && c->isLocked()) {
            anyLocked = true;
            break;
        }
    }

    bool newLock = !anyLocked;
    for (UIComponent* c : m_selectedComps) {
        if (c) {
            c->setLocked(newLock);
        }
    }
    updateButtons();
}

void FloatingSelectionToolbar::onDuplicateClicked() {
    if (m_selectedComps.isEmpty() || !m_scene) return;

    m_scene->clearSelection();
    for (UIComponent* comp : m_selectedComps) {
        if (!comp) continue;
        UIComponent* dup = Project::createComponentInstance(comp->componentType(), comp->componentId() + "_copy");
        if (dup) {
            dup->fromJson(comp->toJson());
            dup->setComponentId(comp->componentId() + "_copy");
            dup->setCompPos(comp->pos().x() + 15, comp->pos().y() + 15);
            if (m_undoStack) {
                m_undoStack->push(new AddComponentCommand(m_scene, dup));
            } else {
                m_scene->addUIComponent(dup);
                dup->setSelected(true);
            }
        }
    }
}

void FloatingSelectionToolbar::onDeleteClicked() {
    if (m_selectedComps.isEmpty() || !m_scene) return;

    if (m_undoStack) {
        m_undoStack->push(new DeleteComponentCommand(m_scene, m_selectedComps));
    } else {
        for (UIComponent* c : m_selectedComps) {
            if (c) {
                m_scene->removeUIComponent(c);
                delete c;
            }
        }
    }
}

void FloatingSelectionToolbar::onGroupClicked() {
    if (m_selectedComps.size() <= 1 || !m_scene) return;

    if (m_undoStack) {
        m_undoStack->push(new GroupCommand(m_scene, m_selectedComps));
    } else {
        GroupCommand cmd(m_scene, m_selectedComps);
        cmd.redo();
    }
}

void FloatingSelectionToolbar::onUngroupClicked() {
    if (m_selectedComps.isEmpty() || !m_scene) return;

    for (UIComponent* c : m_selectedComps) {
        if (auto group = dynamic_cast<GroupComponent*>(c)) {
            if (m_undoStack) {
                m_undoStack->push(new UngroupCommand(m_scene, group));
            } else {
                UngroupCommand cmd(m_scene, group);
                cmd.redo();
            }
        }
    }
}

void FloatingSelectionToolbar::onFlipHorizontal() {
    for (UIComponent* c : m_selectedComps) {
        if (c) c->setFlipHorizontal(!c->flipHorizontal());
    }
}

void FloatingSelectionToolbar::onFlipVertical() {
    for (UIComponent* c : m_selectedComps) {
        if (c) c->setFlipVertical(!c->flipVertical());
    }
}

void FloatingSelectionToolbar::onBringToFront() {
    if (!m_scene || m_selectedComps.isEmpty()) return;
    qreal maxZ = 0.0;
    for (QGraphicsItem* item : m_scene->items()) {
        maxZ = std::max(maxZ, item->zValue());
    }
    for (UIComponent* c : m_selectedComps) {
        if (c) c->setZValue(maxZ + 1.0);
    }
}

void FloatingSelectionToolbar::onSendToBack() {
    if (!m_scene || m_selectedComps.isEmpty()) return;
    qreal minZ = 0.0;
    for (QGraphicsItem* item : m_scene->items()) {
        minZ = std::min(minZ, item->zValue());
    }
    for (UIComponent* c : m_selectedComps) {
        if (c) c->setZValue(minZ - 1.0);
    }
}

void FloatingSelectionToolbar::onBringForward() {
    for (UIComponent* c : m_selectedComps) {
        if (c) c->setZValue(c->zValue() + 1.0);
    }
}

void FloatingSelectionToolbar::onSendBackward() {
    for (UIComponent* c : m_selectedComps) {
        if (c) c->setZValue(c->zValue() - 1.0);
    }
}
