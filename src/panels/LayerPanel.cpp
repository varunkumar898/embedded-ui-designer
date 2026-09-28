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
    layout->setContentsMargins(4, 8, 4, 4);
    layout->setSpacing(6);

    QLabel* title = new QLabel("Component Layers", this);
    title->setStyleSheet("color: #9AA5B8; font-size: 11px; font-weight: bold; text-transform: uppercase; padding-left: 8px;");
    layout->addWidget(title);

    m_listWidget = new QListWidget(this);
    m_listWidget->setStyleSheet(
        "QListWidget { background-color: #252830; color: #E0E5EE; border: none; font-size: 12px; outline: none; }"
        "QListWidget::item { padding: 6px 10px; border-radius: 4px; margin: 2px 4px; background-color: #2F333E; }"
        "QListWidget::item:hover { background-color: #3B404E; }"
        "QListWidget::item:selected { background-color: #2196F3; color: #FFFFFF; font-weight: bold; }"
    );
    layout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::itemSelectionChanged, this, &LayerPanel::onSelectionChanged);

    QHBoxLayout* actions = new QHBoxLayout();
    actions->setSpacing(4);

    m_btnUp = new QPushButton("▲ Up", this);
    m_btnDown = new QPushButton("▼ Down", this);
    m_btnDelete = new QPushButton("Delete", this);

    QString btnStyle = "QPushButton { background-color: #2F333E; color: #D0D5DD; border: 1px solid #3B404E; border-radius: 4px; padding: 4px 8px; font-size: 11px; }"
                       "QPushButton:hover { background-color: #3B404E; color: #FFFFFF; }";
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
