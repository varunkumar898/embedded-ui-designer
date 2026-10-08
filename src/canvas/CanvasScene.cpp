#include "CanvasScene.h"
#include <QPainter>
#include <cmath>
#include <algorithm>

CanvasScene::CanvasScene(QObject* parent)
    : QGraphicsScene(parent)
{
    setDisplayConfig(DisplayConfig());
    connect(this, &QGraphicsScene::selectionChanged, this, &CanvasScene::onSelectionChanged);
}

CanvasScene::~CanvasScene() {
    disconnect(this, nullptr, this, nullptr);
    detachAllComponents();
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

void CanvasScene::detachAllComponents() {
    clearSelection();
    for (UIComponent* comp : uiComponents()) {
        removeItem(comp);
    }
}


static constexpr qreal SNAP_TOLERANCE = 6.0;  ///< Scene-pixel radius for smart snapping

QPointF CanvasScene::snapPoint(const QPointF& pt, UIComponent* ignore) const {
    qreal rx = pt.x();
    qreal ry = pt.y();

    // 1. Grid snap (existing behaviour)
    if (m_snapToGrid && m_gridSize > 1) {
        rx = std::round(rx / m_gridSize) * m_gridSize;
        ry = std::round(ry / m_gridSize) * m_gridSize;
    }

    // 2. Guide snap — overrides grid if closer
    for (qreal gx : m_vGuides) {
        if (std::abs(pt.x() - gx) < SNAP_TOLERANCE)
            rx = gx;
    }
    for (qreal gy : m_hGuides) {
        if (std::abs(pt.y() - gy) < SNAP_TOLERANCE)
            ry = gy;
    }

    // 3. Component edge/centre snap
    qreal bestDx = SNAP_TOLERANCE, bestDy = SNAP_TOLERANCE;
    for (UIComponent* comp : uiComponents()) {
        if (comp == ignore) continue;
        const qreal left   = comp->pos().x();
        const qreal right  = left + comp->compWidth();
        const qreal top    = comp->pos().y();
        const qreal bottom = top  + comp->compHeight();
        const qreal cx     = (left + right)  / 2.0;
        const qreal cy     = (top  + bottom) / 2.0;

        for (qreal ex : {left, right, cx}) {
            if (std::abs(pt.x() - ex) < bestDx) {
                bestDx = std::abs(pt.x() - ex);
                rx = ex;
            }
        }
        for (qreal ey : {top, bottom, cy}) {
            if (std::abs(pt.y() - ey) < bestDy) {
                bestDy = std::abs(pt.y() - ey);
                ry = ey;
            }
        }
    }

    return QPointF(rx, ry);
}

// ── Guide management ──────────────────────────────────────────────────

void CanvasScene::addHGuide(qreal y) {
    m_hGuides.append(y);
    update();
}
void CanvasScene::addVGuide(qreal x) {
    m_vGuides.append(x);
    update();
}
void CanvasScene::removeHGuide(qreal y) {
    auto it = std::min_element(m_hGuides.begin(), m_hGuides.end(),
        [y](qreal a, qreal b){ return std::abs(a-y) < std::abs(b-y); });
    if (it != m_hGuides.end() && std::abs(*it - y) < 12.0)
        m_hGuides.erase(it);
    update();
}
void CanvasScene::removeVGuide(qreal x) {
    auto it = std::min_element(m_vGuides.begin(), m_vGuides.end(),
        [x](qreal a, qreal b){ return std::abs(a-x) < std::abs(b-x); });
    if (it != m_vGuides.end() && std::abs(*it - x) < 12.0)
        m_vGuides.erase(it);
    update();
}
void CanvasScene::clearGuides() {
    m_hGuides.clear();
    m_vGuides.clear();
    update();
}
void CanvasScene::setGuides(const QList<qreal>& hg, const QList<qreal>& vg) {
    m_hGuides = hg;
    m_vGuides = vg;
    update();
}

// ── Snap-highlight ────────────────────────────────────────────────────────

void CanvasScene::showSnapHighlight(const QLineF& line) {
    m_snapHighlightLine   = line;
    m_snapHighlightActive = true;
    update();
}
void CanvasScene::clearSnapHighlight() {
    m_snapHighlightActive = false;
    update();
}

void CanvasScene::setPathPreview(const QList<QPointF>& points, const QPointF& cursor, bool visible) {
    m_pathPreviewPoints = points;
    m_pathPreviewCursor = cursor;
    m_pathPreviewVisible = visible;
    update();
}

void CanvasScene::onSelectionChanged() {
    QList<QGraphicsItem*> sel = selectedItems();

    QList<UIComponent*> uiSel;
    for (QGraphicsItem* item : sel) {
        if (auto comp = dynamic_cast<UIComponent*>(item)) {
            uiSel.append(comp);
        }
    }
    emit selectionListChanged(uiSel);

    if (uiSel.isEmpty()) {
        emit componentSelected(nullptr);
    } else if (uiSel.size() == 1) {
        emit componentSelected(uiSel.first());
    } else {
        emit componentSelected(nullptr);
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

    // ── Ruler guides (drawn behind the nameplate badge) ──────────────────
    if (!m_hGuides.isEmpty() || !m_vGuides.isEmpty()) {
        painter->save();
        QPen guidePen(QColor(0, 200, 180, 220), 1.0, Qt::DashLine);
        guidePen.setDashPattern({6, 4});
        painter->setPen(guidePen);
        for (qreal gy : m_hGuides) {
            painter->drawLine(QPointF(sceneRect().left(), gy),
                              QPointF(sceneRect().right(), gy));
        }
        for (qreal gx : m_vGuides) {
            painter->drawLine(QPointF(gx, sceneRect().top()),
                              QPointF(gx, sceneRect().bottom()));
        }
        painter->restore();
    }

    // ── Snap-highlight (one-shot pink line while dragging) ──────────────
    if (m_snapHighlightActive) {
        painter->save();
        QPen hlPen(QColor(255, 65, 130, 210), 1.5);
        painter->setPen(hlPen);
        painter->drawLine(m_snapHighlightLine);
        painter->restore();
        // Clear the one-shot flag so it doesn't persist
        m_snapHighlightActive = false;
    }

    // ── Nameplate badge (existing code) ───────────────────────────────
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

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 70));
    painter->drawRoundedRect(badgeRect.translated(0, 2), 8, 8);

    painter->setBrush(QColor(26, 29, 36));
    painter->setPen(QPen(QColor(42, 47, 58), 1.0));
    painter->drawRoundedRect(badgeRect, 8, 8);

    painter->setPen(QColor(220, 228, 238));
    painter->drawText(badgeRect.adjusted(0, -1, 0, -1), Qt::AlignCenter, label);
    painter->restore();

    if (m_pathPreviewVisible) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QPen(QColor(26, 150, 255), 2.0, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(QColor(26, 150, 255, 70));

        for (int index = 1; index < m_pathPreviewPoints.size(); ++index) {
            painter->drawLine(m_pathPreviewPoints.at(index - 1), m_pathPreviewPoints.at(index));
        }
        if (!m_pathPreviewPoints.isEmpty()) {
            painter->drawLine(m_pathPreviewPoints.last(), m_pathPreviewCursor);
        }
        for (const QPointF& point : m_pathPreviewPoints) {
            painter->drawEllipse(point, 3.5, 3.5);
        }
        painter->drawEllipse(m_pathPreviewCursor, 4.0, 4.0);
        painter->restore();
    }
}
