#include "UIComponent.h"
#include <QPen>
#include <QBrush>
#include <QGraphicsScene>
#include <cmath>

UIComponent::UIComponent(const QString& id, const QString& compType, QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_id(id)
    , m_componentType(compType)
{
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
}

void UIComponent::setComponentId(const QString& id) {
    if (m_id != id) {
        m_id = id;
        emit propertyChanged(this);
    }
}

void UIComponent::setCompPos(qreal x, qreal y) {
    if (pos().x() != x || pos().y() != y) {
        prepareGeometryChange();
        setPos(x, y);
        emit geometryChangedSignal(this);
    }
}

void UIComponent::setCompSize(qreal w, qreal h) {
    w = std::max(10.0, w);
    h = std::max(10.0, h);
    if (m_width != w || m_height != h) {
        prepareGeometryChange();
        m_width = w;
        m_height = h;
        update();
        emit geometryChangedSignal(this);
    }
}

QRectF UIComponent::boundingRect() const {
    qreal margin = HANDLE_SIZE / 2.0 + 2.0;
    return QRectF(-margin, -margin, m_width + margin * 2, m_height + margin * 2);
}

void UIComponent::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();
    paintComponent(painter);
    painter->restore();

    if (isSelected()) {
        paintSelectionHandles(painter);
    }
}

void UIComponent::paintSelectionHandles(QPainter* painter) {
    painter->save();
    
    // Outline bounding box
    QPen outlinePen(QColor(0, 120, 255), 1.5, Qt::DashLine);
    painter->setPen(outlinePen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(QRectF(0, 0, m_width, m_height));

    // Draw handles
    painter->setPen(QPen(QColor(0, 120, 255), 1.0));
    painter->setBrush(QBrush(Qt::white));

    auto drawH = [&](ResizeHandle h) {
        painter->drawRect(handleRect(h));
    };

    drawH(ResizeHandle::TopLeft);
    drawH(ResizeHandle::TopRight);
    drawH(ResizeHandle::BottomLeft);
    drawH(ResizeHandle::BottomRight);
    drawH(ResizeHandle::Top);
    drawH(ResizeHandle::Bottom);
    drawH(ResizeHandle::Left);
    drawH(ResizeHandle::Right);

    painter->restore();
}

QRectF UIComponent::handleRect(ResizeHandle handle) const {
    qreal s = HANDLE_SIZE;
    qreal hs = s / 2.0;
    switch (handle) {
        case ResizeHandle::TopLeft:     return QRectF(-hs, -hs, s, s);
        case ResizeHandle::TopRight:    return QRectF(m_width - hs, -hs, s, s);
        case ResizeHandle::BottomLeft:  return QRectF(-hs, m_height - hs, s, s);
        case ResizeHandle::BottomRight: return QRectF(m_width - hs, m_height - hs, s, s);
        case ResizeHandle::Top:         return QRectF(m_width / 2.0 - hs, -hs, s, s);
        case ResizeHandle::Bottom:      return QRectF(m_width / 2.0 - hs, m_height - hs, s, s);
        case ResizeHandle::Left:        return QRectF(-hs, m_height / 2.0 - hs, s, s);
        case ResizeHandle::Right:       return QRectF(m_width - hs, m_height / 2.0 - hs, s, s);
        default:                        return QRectF();
    }
}

ResizeHandle UIComponent::handleAt(const QPointF& p) const {
    if (!isSelected()) return ResizeHandle::None;

    const ResizeHandle handles[] = {
        ResizeHandle::TopLeft, ResizeHandle::TopRight,
        ResizeHandle::BottomLeft, ResizeHandle::BottomRight,
        ResizeHandle::Top, ResizeHandle::Bottom,
        ResizeHandle::Left, ResizeHandle::Right
    };

    for (auto h : handles) {
        if (handleRect(h).contains(p)) {
            return h;
        }
    }
    return ResizeHandle::None;
}

void UIComponent::updateCursorForHandle(ResizeHandle handle) {
    switch (handle) {
        case ResizeHandle::TopLeft:
        case ResizeHandle::BottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case ResizeHandle::TopRight:
        case ResizeHandle::BottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        case ResizeHandle::Top:
        case ResizeHandle::Bottom:
            setCursor(Qt::SizeVerCursor);
            break;
        case ResizeHandle::Left:
        case ResizeHandle::Right:
            setCursor(Qt::SizeHorCursor);
            break;
        default:
            setCursor(Qt::ArrowCursor);
            break;
    }
}

void UIComponent::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    if (isSelected()) {
        ResizeHandle h = handleAt(event->pos());
        updateCursorForHandle(h);
    } else {
        setCursor(Qt::ArrowCursor);
    }
    QGraphicsObject::hoverMoveEvent(event);
}

void UIComponent::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton && isSelected()) {
        m_activeHandle = handleAt(event->pos());
        if (m_activeHandle != ResizeHandle::None) {
            m_resizing = true;
            m_dragStartPos = event->scenePos();
            m_initialGeom = QRectF(pos().x(), pos().y(), m_width, m_height);
            event->accept();
            return;
        }
    }
    m_resizing = false;
    QGraphicsObject::mousePressEvent(event);
    emit geometryChangedSignal(this);
}

void UIComponent::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_resizing) {
        QPointF delta = event->scenePos() - m_dragStartPos;
        qreal nx = m_initialGeom.x();
        qreal ny = m_initialGeom.y();
        qreal nw = m_initialGeom.width();
        qreal nh = m_initialGeom.height();

        switch (m_activeHandle) {
            case ResizeHandle::Right:
                nw += delta.x();
                break;
            case ResizeHandle::Bottom:
                nh += delta.y();
                break;
            case ResizeHandle::BottomRight:
                nw += delta.x();
                nh += delta.y();
                break;
            case ResizeHandle::Left:
                nx += delta.x();
                nw -= delta.x();
                break;
            case ResizeHandle::Top:
                ny += delta.y();
                nh -= delta.y();
                break;
            case ResizeHandle::TopLeft:
                nx += delta.x();
                nw -= delta.x();
                ny += delta.y();
                nh -= delta.y();
                break;
            case ResizeHandle::TopRight:
                nw += delta.x();
                ny += delta.y();
                nh -= delta.y();
                break;
            case ResizeHandle::BottomLeft:
                nx += delta.x();
                nw -= delta.x();
                nh += delta.y();
                break;
            default:
                break;
        }

        if (nw < 15.0) {
            if (m_activeHandle == ResizeHandle::Left || m_activeHandle == ResizeHandle::TopLeft || m_activeHandle == ResizeHandle::BottomLeft) {
                nx = m_initialGeom.right() - 15.0;
            }
            nw = 15.0;
        }
        if (nh < 15.0) {
            if (m_activeHandle == ResizeHandle::Top || m_activeHandle == ResizeHandle::TopLeft || m_activeHandle == ResizeHandle::TopRight) {
                ny = m_initialGeom.bottom() - 15.0;
            }
            nh = 15.0;
        }

        prepareGeometryChange();
        setPos(nx, ny);
        m_width = nw;
        m_height = nh;
        update();
        emit geometryChangedSignal(this);
        event->accept();
        return;
    }

    QGraphicsObject::mouseMoveEvent(event);
    emit geometryChangedSignal(this);
}

void UIComponent::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (m_resizing) {
        m_resizing = false;
        m_activeHandle = ResizeHandle::None;
        setCursor(Qt::ArrowCursor);
        event->accept();
        emit geometryChangedSignal(this);
        return;
    }
    QGraphicsObject::mouseReleaseEvent(event);
    emit geometryChangedSignal(this);
}

QJsonObject UIComponent::toJson() const {
    QJsonObject obj;
    obj["id"] = m_id;
    obj["type"] = m_componentType;
    obj["x"] = static_cast<int>(std::round(pos().x()));
    obj["y"] = static_cast<int>(std::round(pos().y()));
    obj["width"] = static_cast<int>(std::round(m_width));
    obj["height"] = static_cast<int>(std::round(m_height));
    return obj;
}

void UIComponent::fromJson(const QJsonObject& json) {
    m_id = json.value("id").toString(m_id);
    m_componentType = json.value("type").toString(m_componentType);
    qreal x = json.value("x").toDouble(pos().x());
    qreal y = json.value("y").toDouble(pos().y());
    setCompPos(x, y);
    qreal w = json.value("width").toDouble(m_width);
    qreal h = json.value("height").toDouble(m_height);
    setCompSize(w, h);
}
