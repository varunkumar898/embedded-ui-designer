#include "CanvasScene.h"
#include <QPainter>
#include <cmath>

CanvasScene::CanvasScene(QObject* parent)
    : QGraphicsScene(parent)
{
    setDisplayConfig(DisplayConfig());
    connect(this, &QGraphicsScene::selectionChanged, this, &CanvasScene::onSelectionChanged);
}

void CanvasScene::setDisplayConfig(const DisplayConfig& config) {
    m_displayConfig = config;
    qreal margin = 100.0;
    setSceneRect(-margin, -margin, config.width + margin * 2, config.height + margin * 2);
    update();
}

QRectF CanvasScene::displayRect() const {
    return QRectF(0, 0, m_displayConfig.width, m_displayConfig.height);
}

void CanvasScene::setGridVisible(bool visible) {
    if (m_gridVisible != visible) {
        m_gridVisible = visible;
        update();
    }
}

void CanvasScene::setSnapToGrid(bool snap) {
    m_snapToGrid = snap;
}

void CanvasScene::setGridSize(int size) {
    if (size > 0 && m_gridSize != size) {
        m_gridSize = size;
        update();
    }
}

void CanvasScene::setScreenBackgroundColor(const QColor& color) {
    if (m_screenBackgroundColor != color) {
        m_screenBackgroundColor = color;
        update();
    }
}

void CanvasScene::addUIComponent(UIComponent* comp) {
    if (!comp) return;
    addItem(comp);
    connect(comp, &UIComponent::propertyChanged, this, [this, comp]() {
        emit componentChanged(comp);
    });
    connect(comp, &UIComponent::geometryChangedSignal, this, &CanvasScene::onComponentGeometryChanged);
    emit componentAdded(comp);
}

void CanvasScene::removeUIComponent(UIComponent* comp) {
    if (!comp) return;
    removeItem(comp);
    emit componentRemoved(comp);
}

QList<UIComponent*> CanvasScene::uiComponents() const {
    QList<UIComponent*> list;
    for (QGraphicsItem* item : items(Qt::AscendingOrder)) {
        if (auto comp = dynamic_cast<UIComponent*>(item)) {
            list.append(comp);
        }
    }
    return list;
}

void CanvasScene::clearComponents() {
    for (UIComponent* comp : uiComponents()) {
        removeUIComponent(comp);
        delete comp;
    }
}

QPointF CanvasScene::snapPoint(const QPointF& pt) const {
    if (!m_snapToGrid || m_gridSize <= 1) return pt;
    qreal sx = std::round(pt.x() / m_gridSize) * m_gridSize;
    qreal sy = std::round(pt.y() / m_gridSize) * m_gridSize;
    return QPointF(sx, sy);
}

void CanvasScene::onSelectionChanged() {
    QList<QGraphicsItem*> sel = selectedItems();
    if (sel.isEmpty()) {
        emit componentSelected(nullptr);
    } else {
        if (auto comp = dynamic_cast<UIComponent*>(sel.first())) {
            emit componentSelected(comp);
        }
    }
}

void CanvasScene::onComponentGeometryChanged(UIComponent* comp) {
    if (m_snapToGrid && comp) {
        QPointF snapped = snapPoint(comp->pos());
        if (snapped != comp->pos()) {
            comp->setPos(snapped);
        }
    }
    emit componentChanged(comp);
}

void CanvasScene::drawBackground(QPainter* painter, const QRectF& rect) {
    // Fill outer background with dark workspace canvas color
    painter->fillRect(rect, QColor(30, 32, 38));

    QRectF sRect = displayRect();

    // Screen Drop Shadow
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 70));
    painter->drawRoundedRect(sRect.translated(5, 5), 4, 4);
    painter->restore();

    // Target Screen Surface
    painter->fillRect(sRect, m_screenBackgroundColor);

    // Screen Border
    painter->save();
    painter->setPen(QPen(QColor(100, 110, 130), 1.5));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(sRect);
    painter->restore();

    // Grid rendering (inside display screen bounds)
    if (m_gridVisible && m_gridSize > 0) {
        painter->save();
        painter->setClipRect(sRect);
        
        QPen gridPen(QColor(215, 220, 230), 1, Qt::DotLine);
        painter->setPen(gridPen);

        for (qreal x = 0; x <= sRect.width(); x += m_gridSize) {
            painter->drawLine(QPointF(x, 0), QPointF(x, sRect.height()));
        }
        for (qreal y = 0; y <= sRect.height(); y += m_gridSize) {
            painter->drawLine(QPointF(0, y), QPointF(sRect.width(), y));
        }
        painter->restore();
    }
}

void CanvasScene::drawForeground(QPainter* painter, const QRectF& rect) {
    Q_UNUSED(rect);
    QRectF sRect = displayRect();

    // Badge showing screen info above target display
    painter->save();
    QFont font = painter->font();
    font.setPixelSize(11);
    font.setBold(true);
    painter->setFont(font);

    QString label = QString("Target MCU Display: %1 × %2 (%3-bit %4)")
        .arg(m_displayConfig.width)
        .arg(m_displayConfig.height)
        .arg(m_displayConfig.colorDepth)
        .arg(m_displayConfig.type);

    QRectF badgeRect(0, -26, 280, 20);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(45, 52, 65));
    painter->drawRoundedRect(badgeRect, 4, 4);

    painter->setPen(QColor(220, 230, 245));
    painter->drawText(badgeRect, Qt::AlignCenter, label);
    painter->restore();
}
