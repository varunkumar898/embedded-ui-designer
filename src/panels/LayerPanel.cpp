#include "LayerPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QAbstractItemModel>
#include <QSignalBlocker>
#include "theme/Theme.h"

LayerPanel::LayerPanel(CanvasScene* scene, QWidget* parent)
    : QWidget(parent)
    , m_scene(scene)
{
    setupUi();

    connect(m_scene, &CanvasScene::componentAdded, this, &LayerPanel::refreshLayers);
    connect(m_scene, &CanvasScene::componentRemoved, this, &LayerPanel::refreshLayers);
    connect(m_scene, &CanvasScene::componentChanged, this, &LayerPanel::refreshLayers);
    connect(m_scene, &CanvasScene::componentSelected, this, [this](UIComponent* comp) {
        if (m_syncing) return;
        m_syncing = true;
        if (!comp) {
            m_listWidget->clearSelection();
        } else {
            for (int i = 0; i < m_listWidget->count(); ++i) {
                QListWidgetItem* item = m_listWidget->item(i);
                if (reinterpret_cast<UIComponent*>(item->data(Qt::UserRole).value<quintptr>()) == comp) {
                    m_listWidget->setCurrentItem(item);
                    break;
                }
            }
        }
        m_syncing = false;
    });
}

void LayerPanel::setupUi() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    QLabel* title = new QLabel("COMPONENT LAYERS", this);
    title->setStyleSheet(QString("color: %1; font-size: 10px; font-weight: 600; letter-spacing: 0.5px; padding-left: 8px; padding-top: 4px; padding-bottom: 2px;").arg(Theme::TextMuted));
    layout->addWidget(title);

    m_listWidget = new QListWidget(this);
    m_listWidget->setDragDropMode(QAbstractItemView::InternalMove);
    m_listWidget->setDefaultDropAction(Qt::MoveAction);
    m_listWidget->setDragEnabled(true);
    m_listWidget->setAcceptDrops(true);
    m_listWidget->setDropIndicatorShown(true);
    m_listWidget->setStyleSheet(QString(
        "QListWidget { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; font-size: 12px; outline: none; padding: 6px; }"
        "QListWidget::item { "
        "  background: %5; "
        "  color: %2; "
        "  border: 1px solid %3; "
        "  border-radius: %6px; "
        "  padding: 6px 10px; "
        "  margin: 2px 2px; "
        "  font-weight: 500; "
        "}"
        "QListWidget::item:hover { "
        "  background: %7; "
        "  border-color: %8; "
        "  color: %2; "
        "}"
        "QListWidget::item:selected { "
        "  background: %9; "
        "  border-color: %10; "
        "  color: %2; "
        "  font-weight: 600; "
        "}")
        .arg(Theme::BgSurface, Theme::TextPrimary, Theme::BorderSubtle)
        .arg(Theme::RadiusMd)
        .arg(Theme::BgRaised)
        .arg(Theme::RadiusSm)
        .arg(Theme::BgOverlay, Theme::BorderStrong, Theme::AccentSoft, Theme::Accent)
    );
    layout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::itemSelectionChanged, this, &LayerPanel::onSelectionChanged);
    connect(m_listWidget->model(), &QAbstractItemModel::rowsMoved, this,
            [this](const QModelIndex&, int, int, const QModelIndex&, int) {
        applyListOrder();
        refreshLayers();
    });

    QHBoxLayout* actions = new QHBoxLayout();
    actions->setSpacing(8);

    m_btnUp = new QPushButton("▲ Up", this);
    m_btnDown = new QPushButton("▼ Down", this);
    m_btnDelete = new QPushButton("Delete", this);

    QString btnStyle = QString(
        "QPushButton { "
        "  background: %1; "
        "  color: %2; "
        "  border: 1px solid %3; "
        "  border-radius: %4px; "
        "  padding: 7px 12px; "
        "  font-size: 11.5px; "
        "  font-weight: 600; "
        "} "
        "QPushButton:hover { "
        "  background: %5; "
        "  border-color: %6; "
        "  color: %2; "
        "} "
        "QPushButton:pressed { "
        "  background: %7; "
        "  border-color: %3; "
        "  color: %8; "
        "}")
        .arg(Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle)
        .arg(Theme::RadiusSm)
        .arg(Theme::BgOverlay, Theme::BorderStrong, Theme::BgBase, Theme::TextSecondary);
    m_btnUp->setStyleSheet(btnStyle);
    m_btnDown->setStyleSheet(btnStyle);
    m_btnDelete->setStyleSheet(btnStyle);

    actions->addWidget(m_btnUp, 1);
    actions->addWidget(m_btnDown, 1);
    actions->addWidget(m_btnDelete, 1);
    layout->addLayout(actions);

    connect(m_btnUp, &QPushButton::clicked, this, &LayerPanel::onMoveUp);
    connect(m_btnDown, &QPushButton::clicked, this, &LayerPanel::onMoveDown);
    connect(m_btnDelete, &QPushButton::clicked, this, &LayerPanel::onDelete);
}

void LayerPanel::refreshLayers() {
    if (m_syncing) return;
    m_syncing = true;

    UIComponent* currentComp = nullptr;
    if (m_listWidget->currentItem()) {
        currentComp = reinterpret_cast<UIComponent*>(m_listWidget->currentItem()->data(Qt::UserRole).value<quintptr>());
    }

    m_listWidget->clear();
    QList<UIComponent*> comps = m_scene->uiComponents();

    // Reverse order so top visual items appear at top of layer list
    for (int i = comps.size() - 1; i >= 0; --i) {
        UIComponent* comp = comps[i];
        QString label = QString("%1 (%2)").arg(comp->componentId(), comp->componentType());
        QListWidgetItem* item = new QListWidgetItem(label, m_listWidget);
        item->setData(Qt::UserRole, QVariant::fromValue(reinterpret_cast<quintptr>(comp)));
        if (comp == currentComp) {
            m_listWidget->setCurrentItem(item);
        }
    }

    m_syncing = false;
}

void LayerPanel::onSelectionChanged() {
    if (m_syncing) return;
    m_syncing = true;

    QListWidgetItem* item = m_listWidget->currentItem();
    if (item) {
        UIComponent* comp = reinterpret_cast<UIComponent*>(item->data(Qt::UserRole).value<quintptr>());
        if (comp) {
            m_scene->clearSelection();
            comp->setSelected(true);
            emit componentSelected(comp);
        }
    } else {
        m_scene->clearSelection();
        emit componentSelected(nullptr);
    }

    m_syncing = false;
}

void LayerPanel::onMoveUp() {
    const int row = m_listWidget->currentRow();
    if (row <= 0) return;
    m_syncing = true;
    QListWidgetItem* item = m_listWidget->takeItem(row);
    m_listWidget->insertItem(row - 1, item);
    m_listWidget->setCurrentItem(item);
    applyListOrder();
    m_syncing = false;
    refreshLayers();
}

void LayerPanel::onMoveDown() {
    const int row = m_listWidget->currentRow();
    if (row < 0 || row >= m_listWidget->count() - 1) return;
    m_syncing = true;
    QListWidgetItem* item = m_listWidget->takeItem(row);
    m_listWidget->insertItem(row + 1, item);
    m_listWidget->setCurrentItem(item);
    applyListOrder();
    m_syncing = false;
    refreshLayers();
}

void LayerPanel::applyListOrder() {
    const int count = m_listWidget->count();
    for (int row = 0; row < count; ++row) {
        UIComponent* component = reinterpret_cast<UIComponent*>(
            m_listWidget->item(row)->data(Qt::UserRole).value<quintptr>());
        if (component) component->setZValue(count - row);
    }
    m_scene->update();
}

void LayerPanel::onDelete() {
    QListWidgetItem* item = m_listWidget->currentItem();
    if (!item) return;
    UIComponent* comp = reinterpret_cast<UIComponent*>(item->data(Qt::UserRole).value<quintptr>());
    if (comp) {
        m_scene->removeUIComponent(comp);
        delete comp;
        refreshLayers();
    }
}
