#include "LayerPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

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
    layout->setContentsMargins(6, 8, 6, 6);
    layout->setSpacing(6);

    QLabel* title = new QLabel("COMPONENT LAYERS", this);
    title->setStyleSheet("color: #8fa0b8; font-size: 11px; font-weight: bold; text-transform: uppercase; padding-left: 6px; padding-top: 2px;");
    layout->addWidget(title);

    m_listWidget = new QListWidget(this);
    m_listWidget->setStyleSheet(
        "QListWidget { background-color: #1a1c23; color: #E0E5EE; border: 1px solid #101217; border-top: 1px solid #0d0f14; border-bottom: 1px solid #303746; border-radius: 6px; font-size: 12px; outline: none; padding: 4px; }"
        "QListWidget::item { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #303542, stop:0.05 #383f4e, stop:0.5 #282d38, stop:0.96 #20242d, stop:1 #181b22); "
        "  color: #e0e6f0; "
        "  border-top: 1px solid #4a5468; "
        "  border-left: 1px solid #343a47; "
        "  border-right: 1px solid #1e222b; "
        "  border-bottom: 2px solid #101217; "
        "  border-radius: 5px; "
        "  padding: 6px 10px; "
        "  margin: 2px 3px; "
        "  font-weight: 500; "
        "}"
        "QListWidget::item:hover { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3c4454, stop:0.05 #465063, stop:0.5 #313745, stop:0.96 #262b36, stop:1 #1d212a); "
        "  color: #ffffff; "
        "  border-top: 1px solid #606f87; "
        "}"
        "QListWidget::item:selected { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1a96ff, stop:0.05 #0a84ed, stop:0.5 #006ecb, stop:0.96 #0054a0, stop:1 #003d78); "
        "  color: #ffffff; "
        "  border-top: 1px solid #82c6ff; "
        "  border-left: 1px solid #369cff; "
        "  border-right: 1px solid #004b91; "
        "  border-bottom: 2px solid #00264d; "
        "  font-weight: bold; "
        "}"
    );
    layout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::itemSelectionChanged, this, &LayerPanel::onSelectionChanged);

    QHBoxLayout* actions = new QHBoxLayout();
    actions->setSpacing(6);

    m_btnUp = new QPushButton("▲ Up", this);
    m_btnDown = new QPushButton("▼ Down", this);
    m_btnDelete = new QPushButton("Delete", this);

    QString btnStyle = 
        "QPushButton { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3c4352, stop:0.04 #454d5d, stop:0.48 #2f3542, stop:0.52 #252a35, stop:0.96 #1f232d, stop:1 #181b23); "
        "  color: #e2e8f2; "
        "  border-top: 1px solid #586378; "
        "  border-left: 1px solid #3b4252; "
        "  border-right: 1px solid #232731; "
        "  border-bottom: 2px solid #111318; "
        "  border-radius: 5px; "
        "  padding: 5px 12px; "
        "  font-size: 11px; "
        "  font-weight: bold; "
        "} "
        "QPushButton:hover { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4c5567, stop:0.04 #576175, stop:0.48 #3b4353, stop:0.52 #303745, stop:0.96 #282e3b, stop:1 #202530); "
        "  color: #ffffff; "
        "  border-top: 1px solid #73829c; "
        "} "
        "QPushButton:pressed { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #15181f, stop:0.08 #1c2029, stop:0.5 #232833, stop:1 #2a2f3c); "
        "  color: #b0bac9; "
        "  border-top: 2px solid #0d0f13; "
        "  border-left: 1px solid #15181f; "
        "  border-right: 1px solid #363d4c; "
        "  border-bottom: 1px solid #485264; "
        "  padding-top: 6px; "
        "  padding-bottom: 4px; "
        "}";
    m_btnUp->setStyleSheet(btnStyle);
    m_btnDown->setStyleSheet(btnStyle);
    m_btnDelete->setStyleSheet(btnStyle);

    actions->addWidget(m_btnUp);
    actions->addWidget(m_btnDown);
    actions->addWidget(m_btnDelete);
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
    QListWidgetItem* item = m_listWidget->currentItem();
    if (!item) return;
    UIComponent* comp = reinterpret_cast<UIComponent*>(item->data(Qt::UserRole).value<quintptr>());
    if (comp) {
        comp->setZValue(comp->zValue() + 1.0);
        refreshLayers();
    }
}

void LayerPanel::onMoveDown() {
    QListWidgetItem* item = m_listWidget->currentItem();
    if (!item) return;
    UIComponent* comp = reinterpret_cast<UIComponent*>(item->data(Qt::UserRole).value<quintptr>());
    if (comp) {
        comp->setZValue(comp->zValue() - 1.0);
        refreshLayers();
    }
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
