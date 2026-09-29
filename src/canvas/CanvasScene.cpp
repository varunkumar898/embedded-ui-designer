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
    // 1. Dark canvas background
    painter->fillRect(rect, QColor(19, 21, 25));

    // Subtle fine dot matrix grid across canvas
    painter->save();
    QPen dotPen(QColor(36, 40, 50), 1.4);
    painter->setPen(dotPen);
    const qreal step = 28.0;
    qreal startX = std::floor(rect.left() / step) * step;
    qreal startY = std::floor(rect.top() / step) * step;
    for (qreal x = startX; x <= rect.right(); x += step) {
        for (qreal y = startY; y <= rect.bottom(); y += step) {
            painter->drawPoint(QPointF(x, y));
        }
    }
    painter->restore();

    QRectF sRect = displayRect();

    // 2. Soft realistic drop shadow under artboard
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    for (int i = 1; i <= 8; ++i) {
        painter->setBrush(QColor(0, 0, 0, 30 - i * 3));
        painter->drawRoundedRect(sRect.adjusted(-i * 2.5, -i * 1.5 + i * 2.5, i * 2.5, i * 4.0 + i * 2.5), 8, 8);
    }
    painter->restore();

    // 3. White Target Screen Artboard Surface with rounded corners
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(m_screenBackgroundColor);
    painter->setPen(QPen(QColor(215, 222, 232), 1.0));
    painter->drawRoundedRect(sRect, 8, 8);
    painter->restore();

    // 4. Subtle grid rendering inside display bounds
    if (m_gridVisible && m_gridSize > 0) {
        painter->save();
        QPainterPath clip;
        clip.addRoundedRect(sRect, 8, 8);
        painter->setClipPath(clip);

        QPen gridPen(QColor(228, 233, 240), 1, Qt::DotLine);
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

    // Nameplate Badge floating and overlapping top edge of target display
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    QFont font = painter->font();
    font.setPixelSize(11);
    font.setBold(true);
    painter->setFont(font);

    QString label = QString("Target MCU Display: %1 × %2 (%3-bit %4)")
        .arg(m_displayConfig.width)
        .arg(m_displayConfig.height)
        .arg(m_displayConfig.colorDepth)
        .arg(m_displayConfig.type);

    QFontMetrics fm(font);
    qreal bw = std::max(290.0, static_cast<double>(fm.horizontalAdvance(label) + 28));
    qreal bh = 24.0;
    QRectF badgeRect(sRect.center().x() - bw / 2.0, sRect.top() - bh + 4.0, bw, bh);

    // Subtle drop shadow under badge
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 70));
    painter->drawRoundedRect(badgeRect.translated(0, 2), 8, 8);

    // Badge pill body
    painter->setBrush(QColor(26, 29, 36));
    painter->setPen(QPen(QColor(42, 47, 58), 1.0));
    painter->drawRoundedRect(badgeRect, 8, 8);

    // Crisp text
    painter->setPen(QColor(220, 228, 238));
    painter->drawText(badgeRect.adjusted(0, -1, 0, -1), Qt::AlignCenter, label);
    painter->restore();
}
