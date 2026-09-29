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
    // 1. Outer Workbench / Design Mat Background
    painter->fillRect(rect, QColor(24, 26, 33));

    // Subtle workbench grid dots/lines
    painter->save();
    QPen benchPen(QColor(36, 40, 50, 140), 1);
    painter->setPen(benchPen);
    qreal startX = std::floor(rect.left() / 20.0) * 20.0;
    qreal startY = std::floor(rect.top() / 20.0) * 20.0;
    for (qreal x = startX; x <= rect.right(); x += 40.0) {
        for (qreal y = startY; y <= rect.bottom(); y += 40.0) {
            painter->drawPoint(QPointF(x, y));
        }
    }
    painter->restore();

    QRectF sRect = displayRect();

    // 2. Multi-layer realistic contact & ambient drop shadow
    painter->save();
    painter->setPen(Qt::NoPen);
    for (int i = 1; i <= 6; ++i) {
        painter->setBrush(QColor(0, 0, 0, 24 - i * 3));
        painter->drawRoundedRect(sRect.adjusted(-i * 2, -i * 1.5 + i * 2.5, i * 2, i * 3.5 + i * 2.5), 6, 6);
    }
    painter->restore();

    // 3. Hardware Display Bezel (Chamfered dark metal frame)
    painter->save();
    QRectF bezelRect = sRect.adjusted(-4, -4, 4, 4);
    QLinearGradient bezelGrad(0, bezelRect.top(), 0, bezelRect.bottom());
    bezelGrad.setColorAt(0.0, QColor(58, 66, 82));
    bezelGrad.setColorAt(0.08, QColor(44, 50, 62));
    bezelGrad.setColorAt(0.92, QColor(28, 32, 40));
    bezelGrad.setColorAt(1.0, QColor(16, 18, 24));
    painter->setBrush(bezelGrad);
    painter->setPen(QPen(QColor(72, 82, 102), 1.0));
    painter->drawRoundedRect(bezelRect, 4, 4);

    // Inner gasket / debossed LCD seal
    painter->setPen(QPen(QColor(10, 12, 16), 1.2));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(sRect.adjusted(-0.6, -0.6, 0.6, 0.6));
    painter->restore();

    // 4. Target Screen Surface (Active LCD Panel)
    painter->fillRect(sRect, m_screenBackgroundColor);

    // 5. Grid rendering (inside display screen bounds)
    if (m_gridVisible && m_gridSize > 0) {
        painter->save();
        painter->setClipRect(sRect);

        QPen gridPen(QColor(215, 222, 232), 1, Qt::DotLine);
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

    // Tactile Hardware Nameplate Badge centered above target display
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    QFont font = painter->font();
    font.setPixelSize(12);
    font.setBold(true);
    painter->setFont(font);

    QString label = QString("Target MCU Display: %1 × %2 (%3-bit %4)")
        .arg(m_displayConfig.width)
        .arg(m_displayConfig.height)
        .arg(m_displayConfig.colorDepth)
        .arg(m_displayConfig.type);

    QFontMetrics fm(font);
    qreal bw = std::max(330.0, static_cast<double>(fm.horizontalAdvance(label) + 36));
    qreal bh = 24.0;
    QRectF badgeRect(sRect.center().x() - bw / 2.0, sRect.top() - bh - 8.0, bw, bh);

    // Badge drop shadow
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 75));
    painter->drawRoundedRect(badgeRect.translated(0, 2), 5, 5);

    // Badge 3D metallic gradient body
    QLinearGradient badgeGrad(0, badgeRect.top(), 0, badgeRect.bottom());
    badgeGrad.setColorAt(0.0, QColor(56, 64, 78));
    badgeGrad.setColorAt(0.08, QColor(46, 52, 64));
    badgeGrad.setColorAt(0.5, QColor(34, 38, 48));
    badgeGrad.setColorAt(0.95, QColor(24, 27, 34));
    badgeGrad.setColorAt(1.0, QColor(16, 18, 23));
    painter->setBrush(badgeGrad);

    // Bevel rim
    QLinearGradient rimGrad(0, badgeRect.top(), 0, badgeRect.bottom());
    rimGrad.setColorAt(0.0, QColor(90, 102, 124));
    rimGrad.setColorAt(0.5, QColor(50, 58, 72));
    rimGrad.setColorAt(1.0, QColor(12, 14, 18));
    painter->setPen(QPen(QBrush(rimGrad), 1.2));
    painter->drawRoundedRect(badgeRect, 5, 5);

    // Specular top highlight line
    painter->setPen(QPen(QColor(255, 255, 255, 60), 1.0));
    painter->drawLine(QPointF(badgeRect.left() + 6, badgeRect.top() + 1.2),
                      QPointF(badgeRect.right() - 6, badgeRect.top() + 1.2));

    // Embossed text: subtle shadow then crisp front text
    painter->setPen(QColor(0, 0, 0, 180));
    painter->drawText(badgeRect.translated(0, 1), Qt::AlignCenter, label);

    painter->setPen(QColor(228, 236, 246));
    painter->drawText(badgeRect, Qt::AlignCenter, label);
    painter->restore();
}
