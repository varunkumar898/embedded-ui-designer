#include "UIComponent.h"
#include "CanvasScene.h"
#include "MoveComponentCommand.h"
#include "ResizeComponentCommand.h"
#include "PropertyChangeCommand.h"
#include <QUndoStack>
#include <QPen>
#include <QBrush>
#include <QGraphicsScene>
#include <cmath>
#include <algorithm>

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

void UIComponent::setComponentVisible(bool visible) {
    if (isVisible() != visible) {
        setVisible(visible);
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
        if (hasCornerRadius()) {
            int maxR = static_cast<int>(std::floor(std::min(m_width, m_height) / 2.0));
            if (cornerRadius() > maxR) {
                setCornerRadius(maxR);
            }
        }
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

    // Draw Corner Radius handles if component supports corner radius
    if (hasCornerRadius()) {
        painter->setRenderHint(QPainter::Antialiasing, true);
        const CornerRadiusHandle crHandles[] = {
            CornerRadiusHandle::TopLeft,
            CornerRadiusHandle::TopRight,
            CornerRadiusHandle::BottomLeft,
            CornerRadiusHandle::BottomRight
        };

        for (auto crh : crHandles) {
            QRectF r = cornerRadiusHandleRect(crh);
            painter->setPen(QPen(QColor(0, 120, 255), 1.2));
            if (m_draggingRadius && m_activeRadiusHandle == crh) {
                painter->setBrush(QBrush(QColor(0, 120, 255)));
            } else {
                painter->setBrush(QBrush(Qt::white));
            }
            painter->drawEllipse(r);
        }
    }

    painter->restore();
}

qreal UIComponent::cornerRadiusHandleOffset() const {
    qreal maxR = std::floor(std::min(m_width, m_height) / 2.0);
    if (maxR <= 0.0) return 0.0;
    qreal h0 = std::min(8.0, std::max(4.0, maxR * 0.4));
    qreal h1 = std::max(h0, maxR - 3.0);
    qreal r = std::clamp(static_cast<qreal>(cornerRadius()), 0.0, maxR);
    return h0 + (h1 - h0) * (r / maxR);
}

QRectF UIComponent::cornerRadiusHandleRect(CornerRadiusHandle handle) const {
    qreal offset = cornerRadiusHandleOffset();
    qreal size = 6.0;
    qreal hs = size / 2.0;

    switch (handle) {
        case CornerRadiusHandle::TopLeft:
            return QRectF(offset - hs, offset - hs, size, size);
        case CornerRadiusHandle::TopRight:
            return QRectF(m_width - offset - hs, offset - hs, size, size);
        case CornerRadiusHandle::BottomLeft:
            return QRectF(offset - hs, m_height - offset - hs, size, size);
        case CornerRadiusHandle::BottomRight:
            return QRectF(m_width - offset - hs, m_height - offset - hs, size, size);
        default:
            return QRectF();
    }
}

CornerRadiusHandle UIComponent::cornerRadiusHandleAt(const QPointF& pos) const {
    if (!isSelected() || !hasCornerRadius()) return CornerRadiusHandle::None;

    const CornerRadiusHandle handles[] = {
        CornerRadiusHandle::TopLeft,
        CornerRadiusHandle::TopRight,
        CornerRadiusHandle::BottomLeft,
        CornerRadiusHandle::BottomRight
    };

    for (auto h : handles) {
        QRectF r = cornerRadiusHandleRect(h);
        QPointF center = r.center();
        qreal dx = pos.x() - center.x();
        qreal dy = pos.y() - center.y();
        if (dx * dx + dy * dy <= 5.0 * 5.0) {
            return h;
        }
    }
    return CornerRadiusHandle::None;
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
        if (hasCornerRadius() && cornerRadiusHandleAt(event->pos()) != CornerRadiusHandle::None) {
            setCursor(Qt::PointingHandCursor);
        } else {
            ResizeHandle h = handleAt(event->pos());
            updateCursorForHandle(h);
        }
    } else {
        setCursor(Qt::ArrowCursor);
    }
    QGraphicsObject::hoverMoveEvent(event);
}

void UIComponent::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    m_initialGeom = QRectF(pos().x(), pos().y(), m_width, m_height);
    if (event->button() == Qt::LeftButton && isSelected()) {
        if (hasCornerRadius()) {
            m_activeRadiusHandle = cornerRadiusHandleAt(event->pos());
            if (m_activeRadiusHandle != CornerRadiusHandle::None) {
                m_draggingRadius = true;
                m_dragStartRadius = cornerRadius();
                m_dragStartPos = event->scenePos();
                event->accept();
                return;
            }
        }
        m_activeHandle = handleAt(event->pos());
        if (m_activeHandle != ResizeHandle::None) {
            m_resizing = true;
            m_dragStartPos = event->scenePos();
            event->accept();
            return;
        }
    }
    m_resizing = false;
    m_draggingRadius = false;
    if (scene()) {
        for (QGraphicsItem* it : scene()->selectedItems()) {
            if (auto comp = dynamic_cast<UIComponent*>(it)) {
                comp->recordInitialPosition();
            }
        }
    }
    QGraphicsObject::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) emit interactionTriggered("On Click");
    emit geometryChangedSignal(this);
}

void UIComponent::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_draggingRadius) {
        qreal maxR = std::floor(std::min(m_width, m_height) / 2.0);
        qreal h0 = std::min(8.0, std::max(4.0, maxR * 0.4));
        qreal h1 = std::max(h0, maxR - 3.0);
        QPointF p = event->pos();
        qreal d = h0;
        switch (m_activeRadiusHandle) {
            case CornerRadiusHandle::TopLeft:
                d = (p.x() + p.y()) / 2.0;
                break;
            case CornerRadiusHandle::TopRight:
                d = ((m_width - p.x()) + p.y()) / 2.0;
                break;
            case CornerRadiusHandle::BottomLeft:
                d = (p.x() + (m_height - p.y())) / 2.0;
                break;
            case CornerRadiusHandle::BottomRight:
                d = ((m_width - p.x()) + (m_height - p.y())) / 2.0;
                break;
            default:
                break;
        }

        qreal ratio = 0.0;
        if (h1 > h0) {
            ratio = (d - h0) / (h1 - h0);
        }
        int newR = static_cast<int>(std::round(ratio * maxR));
        int clampedR = std::clamp(newR, 0, static_cast<int>(maxR));
        if (clampedR != cornerRadius()) {
            setCornerRadius(clampedR);
        }
        setCursor(Qt::PointingHandCursor);
        event->accept();
        return;
    }

    if (m_resizing) {
        QPointF delta = event->scenePos() - m_dragStartPos;
        qreal nx = m_initialGeom.x();
        qreal ny = m_initialGeom.y();
        qreal nw = m_initialGeom.width();
        qreal nh = m_initialGeom.height();

        const bool isCornerHandle = m_activeHandle == ResizeHandle::TopLeft ||
                                    m_activeHandle == ResizeHandle::TopRight ||
                                    m_activeHandle == ResizeHandle::BottomLeft ||
                                    m_activeHandle == ResizeHandle::BottomRight;
        if (isCornerHandle) {
            const bool leftHandle = m_activeHandle == ResizeHandle::TopLeft ||
                                    m_activeHandle == ResizeHandle::BottomLeft;
            const bool topHandle = m_activeHandle == ResizeHandle::TopLeft ||
                                   m_activeHandle == ResizeHandle::TopRight;
            const qreal horizontalSign = leftHandle ? -1.0 : 1.0;
            const qreal verticalSign = topHandle ? -1.0 : 1.0;
            const qreal minScale = std::max(15.0 / m_initialGeom.width(),
                                            15.0 / m_initialGeom.height());

            if (event->modifiers() & Qt::ShiftModifier) {
                const QPointF initialVector(horizontalSign * m_initialGeom.width(),
                                             verticalSign * m_initialGeom.height());
                const QPointF dragVector = initialVector + delta;
                const qreal denominator = QPointF::dotProduct(initialVector, initialVector);
                const qreal scale = QPointF::dotProduct(dragVector, initialVector) / denominator;
                nw = m_initialGeom.width() * std::max(scale, minScale);
                nh = m_initialGeom.height() * std::max(scale, minScale);
            } else {
                nw = std::max(15.0, m_initialGeom.width() + horizontalSign * delta.x());
                nh = std::max(15.0, m_initialGeom.height() + verticalSign * delta.y());
            }
            nx = leftHandle ? m_initialGeom.right() - nw : m_initialGeom.left();
            ny = topHandle ? m_initialGeom.bottom() - nh : m_initialGeom.top();
        } else {

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
        }

        prepareGeometryChange();
        setPos(nx, ny);
        m_width = nw;
        m_height = nh;
        if (hasCornerRadius()) {
            int maxR = static_cast<int>(std::floor(std::min(m_width, m_height) / 2.0));
            if (cornerRadius() > maxR) {
                setCornerRadius(maxR);
            }
        }
        update();
        emit geometryChangedSignal(this);
        event->accept();
        return;
    }

    QGraphicsObject::mouseMoveEvent(event);
    emit geometryChangedSignal(this);
}

void UIComponent::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (m_draggingRadius) {
        m_draggingRadius = false;
        m_activeRadiusHandle = CornerRadiusHandle::None;
        setCursor(Qt::ArrowCursor);
        if (cornerRadius() != m_dragStartRadius) {
            if (auto cs = dynamic_cast<CanvasScene*>(scene())) {
                if (cs->undoStack()) {
                    QJsonObject oldState = toJson();
                    oldState["cornerRadius"] = m_dragStartRadius;
                    QJsonObject newState = toJson();
                    cs->undoStack()->push(new PropertyChangeCommand(this, oldState, newState, "Change Corner Radius"));
                }
            }
        }
        event->accept();
        return;
    }

    if (m_resizing) {
        m_activeHandle = ResizeHandle::None;
        setCursor(Qt::ArrowCursor);
        QRectF finalGeom(pos().x(), pos().y(), m_width, m_height);
        if (finalGeom != m_initialGeom) {
            if (auto cs = dynamic_cast<CanvasScene*>(scene())) {
                if (cs->undoStack()) {
                    cs->undoStack()->push(new ResizeComponentCommand(this, m_initialGeom, finalGeom));
                }
            }
        }
        event->accept();
        emit geometryChangedSignal(this);
        m_resizing = false;
        return;
    }
    QGraphicsObject::mouseReleaseEvent(event);
    if (auto cs = dynamic_cast<CanvasScene*>(scene())) {
        if (cs->undoStack()) {
            QList<MoveComponentCommand::MoveEntry> moves;
            if (scene()) {
                for (QGraphicsItem* it : scene()->selectedItems()) {
                    if (auto comp = dynamic_cast<UIComponent*>(it)) {
                        if (comp->pos() != comp->initialPos()) {
                            moves.append({comp, comp->initialPos(), comp->pos()});
                        }
                    }
                }
            }
            if (moves.isEmpty() && pos() != m_initialGeom.topLeft()) {
                moves.append({this, m_initialGeom.topLeft(), pos()});
            }
            if (!moves.isEmpty()) {
                cs->undoStack()->push(new MoveComponentCommand(moves));
            }
        }
    }
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
    obj["visible"] = isVisible();
    obj["interactions"] = m_interactions;
    obj["protocol"] = m_protocol.isEmpty() ? QString("None") : m_protocol;
    QJsonObject pinObj;
    for (auto it = m_protocolPins.constBegin(); it != m_protocolPins.constEnd(); ++it) {
        pinObj[it.key()] = it.value();
    }
    obj["protocolPins"] = pinObj;
    return obj;
}

void UIComponent::fromJson(const QJsonObject& json) {
    m_id = json.value("id").toString(m_id);
    m_componentType = json.value("type").toString(m_componentType);
    if (json.contains("visible")) {
        setVisible(json.value("visible").toBool(true));
    }
    m_interactions = json.value("interactions").toArray();
    m_protocol = json.value("protocol").toString("None");
    m_protocolPins.clear();
    if (json.contains("protocolPins") && json.value("protocolPins").isObject()) {
        QJsonObject pinObj = json.value("protocolPins").toObject();
        for (auto it = pinObj.constBegin(); it != pinObj.constEnd(); ++it) {
            m_protocolPins[it.key()] = it.value().toString();
        }
    }
    qreal x = json.value("x").toDouble(pos().x());
    qreal y = json.value("y").toDouble(pos().y());
    setCompPos(x, y);
    qreal w = json.value("width").toDouble(m_width);
    qreal h = json.value("height").toDouble(m_height);
    setCompSize(w, h);
}

void UIComponent::setInteractions(const QJsonArray& interactions) {
    if (m_interactions == interactions) return;
    m_interactions = interactions;
    emit propertyChanged(this);
}

// ── Named Color Style References ──────────────────────────────────────────

void UIComponent::setColorStyleRef(const QString& propertyKey, const QString& styleName) {
    if (styleName.isEmpty()) {
        m_colorStyleRefs.remove(propertyKey);
    } else {
        m_colorStyleRefs[propertyKey] = styleName;
    }
}

QString UIComponent::colorStyleRef(const QString& propertyKey) const {
    return m_colorStyleRefs.value(propertyKey);
}

bool UIComponent::hasColorStyleRef(const QString& propertyKey) const {
    return m_colorStyleRefs.contains(propertyKey) && !m_colorStyleRefs.value(propertyKey).isEmpty();
}

void UIComponent::clearColorStyleRef(const QString& propertyKey) {
    m_colorStyleRefs.remove(propertyKey);
}

void UIComponent::applyColorStyle(const QString& styleName, const QColor& color) {
    Q_UNUSED(styleName);
    Q_UNUSED(color);
}

QJsonValue UIComponent::serializeColor(const QColor& color, const QString& styleRef) {
    if (!styleRef.isEmpty()) {
        QJsonObject refObj;
        refObj["styleRef"] = styleRef;
        return refObj;
    }
    return color.name(QColor::HexRgb);
}

void UIComponent::deserializeColor(const QJsonValue& val, QColor& colorOut, QString& styleRefOut) {
    if (val.isObject()) {
        styleRefOut = val.toObject().value("styleRef").toString();
    } else if (val.isString()) {
        colorOut = QColor(val.toString());
        styleRefOut.clear();
    }
}

void UIComponent::setProtocol(const QString& proto) {
    if (m_protocol != proto) {
        m_protocol = proto;
        emit propertyChanged(this);
    }
}

void UIComponent::setProtocolPin(const QString& role, const QString& pin) {
    if (m_protocolPins.value(role) != pin) {
        m_protocolPins[role] = pin;
        emit propertyChanged(this);
    }
}

void UIComponent::setProtocolPins(const QMap<QString, QString>& pins) {
    if (m_protocolPins != pins) {
        m_protocolPins = pins;
        emit propertyChanged(this);
    }
}

void UIComponent::clearProtocol() {
    if (m_protocol != "None" || !m_protocolPins.isEmpty()) {
        m_protocol = "None";
        m_protocolPins.clear();
        emit propertyChanged(this);
    }
}
