#include "CustomComponentInstance.h"
#include "Project.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <algorithm>
#include <cmath>

CustomComponentInstance::CustomComponentInstance(const QString& id, const QString& defId, Project* project, QGraphicsItem* parent)
    : UIComponent(id, "CustomComponent", parent)
    , m_definitionId(defId)
    , m_project(project)
{
    CustomComponentDefinition def = definition();
    if (!def.id.isEmpty()) {
        m_width = def.width;
        m_height = def.height;
        m_minValue = def.minValue;
        m_maxValue = def.maxValue;
        m_value = def.defaultValue;
    } else {
        m_width = 160;
        m_height = 40;
    }
}

void CustomComponentInstance::setDefinitionId(const QString& defId) {
    if (m_definitionId != defId) {
        m_definitionId = defId;
        CustomComponentDefinition def = definition();
        if (!def.id.isEmpty()) {
            m_width = def.width;
            m_height = def.height;
            m_minValue = def.minValue;
            m_maxValue = def.maxValue;
            m_value = def.defaultValue;
        }
        update();
        emit propertyChanged(this);
    }
}

CustomComponentDefinition CustomComponentInstance::definition() const {
    if (m_project) {
        return m_project->findCustomComponentDefinition(m_definitionId);
    }
    return CustomComponentDefinition();
}

QString CustomComponentInstance::behaviorRole() const {
    return definition().behaviorRole;
}

void CustomComponentInstance::setValue(double v) {
    v = std::clamp(v, m_minValue, m_maxValue);
    if (std::abs(m_value - v) > 1e-6) {
        m_value = v;
        update();
        emit propertyChanged(this);
    }
}

void CustomComponentInstance::setMinValue(double min) {
    if (m_minValue != min) {
        m_minValue = min;
        if (m_value < m_minValue) m_value = m_minValue;
        update();
        emit propertyChanged(this);
    }
}

void CustomComponentInstance::setMaxValue(double max) {
    if (m_maxValue != max) {
        m_maxValue = max;
        if (m_value > m_maxValue) m_value = m_maxValue;
        update();
        emit propertyChanged(this);
    }
}

QRectF CustomComponentInstance::computeThumbRect(const PrimitiveShapeData& thumbPrim, const PrimitiveShapeData& trackPrim) const {
    qreal scaleX = m_width / std::max(1.0, definition().width);
    qreal scaleY = m_height / std::max(1.0, definition().height);

    QRectF trackRect(trackPrim.relativeRect.x() * scaleX, trackPrim.relativeRect.y() * scaleY,
                     trackPrim.relativeRect.width() * scaleX, trackPrim.relativeRect.height() * scaleY);
    qreal thumbW = thumbPrim.relativeRect.width() * scaleX;
    qreal thumbH = thumbPrim.relativeRect.height() * scaleY;

    qreal ratio = (m_maxValue > m_minValue) ? (m_value - m_minValue) / (m_maxValue - m_minValue) : 0.0;
    ratio = std::clamp(ratio, 0.0, 1.0);

    qreal travel = std::max(0.0, trackRect.width() - thumbW);
    qreal thumbX = trackRect.left() + ratio * travel;
    qreal thumbY = trackRect.top() + (trackRect.height() - thumbH) / 2.0;

    return QRectF(thumbX, thumbY, thumbW, thumbH);
}

void CustomComponentInstance::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    CustomComponentDefinition def = definition();
    if (def.primitives.isEmpty()) {
        // Fallback placeholder outline
        painter->setPen(QPen(QColor("#717C8F"), 1, Qt::DashLine));
        painter->setBrush(QColor(37, 40, 48, 180));
        painter->drawRoundedRect(0, 0, m_width, m_height, 6, 6);
        painter->setPen(QColor("#FFFFFF"));
        painter->drawText(QRectF(0, 0, m_width, m_height), Qt::AlignCenter, def.name.isEmpty() ? m_id : def.name);
        return;
    }

    qreal scaleX = m_width / std::max(1.0, def.width);
    qreal scaleY = m_height / std::max(1.0, def.height);

    const PrimitiveShapeData* trackPrim = nullptr;
    const PrimitiveShapeData* thumbPrim = nullptr;

    for (const auto& prim : def.primitives) {
        if (prim.isTrack) trackPrim = &prim;
        if (prim.isThumb) thumbPrim = &prim;
    }

    QString role = def.behaviorRole;

    for (const auto& prim : def.primitives) {
        QRectF destRect(prim.relativeRect.x() * scaleX, prim.relativeRect.y() * scaleY,
                        prim.relativeRect.width() * scaleX, prim.relativeRect.height() * scaleY);

        if (role == "Slider" && prim.isThumb && trackPrim) {
            destRect = computeThumbRect(prim, *trackPrim);
        }

        QColor fill = QColor(prim.properties.value("fillColor").toString("#2196F3"));
        QColor stroke = QColor(prim.properties.value("strokeColor").toString("#3B404E"));
        int strokeW = prim.properties.value("strokeWidth").toInt(1);
        int radius = static_cast<int>(prim.properties.value("cornerRadius").toDouble(0.0) * ((scaleX + scaleY) / 2.0));

        painter->setPen(strokeW > 0 ? QPen(stroke, strokeW) : Qt::NoPen);
        painter->setBrush(fill.isValid() ? QBrush(fill) : Qt::NoBrush);

        if (prim.type == "Circle" || prim.type == "Ellipse") {
            painter->drawEllipse(destRect);
        } else if (prim.type == "Text") {
            QString txt = prim.properties.value("text").toString();
            QFont font("Roboto", prim.properties.value("pixelSize").toInt(14));
            font.setBold(prim.properties.value("bold").toBool(false));
            painter->setFont(font);
            painter->setPen(QColor(prim.properties.value("textColor").toString("#FFFFFF")));
            painter->drawText(destRect, Qt::AlignCenter, txt);
        } else {
            // Rectangle / rounded rect
            painter->drawRoundedRect(destRect, radius, radius);
        }
    }
}

void CustomComponentInstance::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (behaviorRole() == "Slider" && event->button() == Qt::LeftButton) {
        CustomComponentDefinition def = definition();
        const PrimitiveShapeData* trackPrim = nullptr;
        const PrimitiveShapeData* thumbPrim = nullptr;
        for (const auto& prim : def.primitives) {
            if (prim.isTrack) trackPrim = &prim;
            if (prim.isThumb) thumbPrim = &prim;
        }
        if (trackPrim && thumbPrim) {
            QRectF thumbR = computeThumbRect(*thumbPrim, *trackPrim);
            qreal clickX = event->pos().x();
            qreal scaleX = m_width / std::max(1.0, def.width);
            QRectF trackRect(trackPrim->relativeRect.x() * scaleX, trackPrim->relativeRect.y() * scaleX,
                             trackPrim->relativeRect.width() * scaleX, trackPrim->relativeRect.height() * scaleX);

            if (trackRect.adjusted(-10, -10, 10, 10).contains(event->pos()) || thumbR.adjusted(-5, -5, 5, 5).contains(event->pos())) {
                m_isDraggingThumb = true;
                qreal travel = std::max(1.0, trackRect.width() - thumbR.width());
                qreal ratio = (clickX - trackRect.left() - thumbR.width() / 2.0) / travel;
                setValue(m_minValue + std::clamp(ratio, 0.0, 1.0) * (m_maxValue - m_minValue));
                event->accept();
                return;
            }
        }
    }
    UIComponent::mousePressEvent(event);
}

void CustomComponentInstance::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_isDraggingThumb && behaviorRole() == "Slider") {
        CustomComponentDefinition def = definition();
        const PrimitiveShapeData* trackPrim = nullptr;
        const PrimitiveShapeData* thumbPrim = nullptr;
        for (const auto& prim : def.primitives) {
            if (prim.isTrack) trackPrim = &prim;
            if (prim.isThumb) thumbPrim = &prim;
        }
        if (trackPrim && thumbPrim) {
            qreal scaleX = m_width / std::max(1.0, def.width);
            QRectF trackRect(trackPrim->relativeRect.x() * scaleX, trackPrim->relativeRect.y() * scaleX,
                             trackPrim->relativeRect.width() * scaleX, trackPrim->relativeRect.height() * scaleX);
            qreal thumbW = thumbPrim->relativeRect.width() * scaleX;
            qreal travel = std::max(1.0, trackRect.width() - thumbW);
            qreal ratio = (event->pos().x() - trackRect.left() - thumbW / 2.0) / travel;
            setValue(m_minValue + std::clamp(ratio, 0.0, 1.0) * (m_maxValue - m_minValue));
            event->accept();
            return;
        }
    }
    UIComponent::mouseMoveEvent(event);
}

void CustomComponentInstance::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (m_isDraggingThumb) {
        m_isDraggingThumb = false;
        event->accept();
        return;
    }
    UIComponent::mouseReleaseEvent(event);
}

QJsonObject CustomComponentInstance::toJson() const {
    QJsonObject obj = UIComponent::toJson();
    obj["type"] = "CustomComponent";
    obj["definitionId"] = m_definitionId;
    obj["value"] = m_value;
    obj["minValue"] = m_minValue;
    obj["maxValue"] = m_maxValue;
    return obj;
}

void CustomComponentInstance::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);
    m_definitionId = json.value("definitionId").toString();
    m_value = json.value("value").toDouble(m_value);
    m_minValue = json.value("minValue").toDouble(m_minValue);
    m_maxValue = json.value("maxValue").toDouble(m_maxValue);
    update();
}

QString CustomComponentInstance::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    CustomComponentDefinition def = definition();
    QString qml;
    qml += QString("%1// Custom Component Instance: %2 (%3)\n").arg(indent, m_id, def.name);
    qml += QString("%1Item {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    x: %2; y: %3; width: %4; height: %5\n")
        .arg(indent)
        .arg(static_cast<int>(pos().x()))
        .arg(static_cast<int>(pos().y()))
        .arg(static_cast<int>(m_width))
        .arg(static_cast<int>(m_height));
    if (def.behaviorRole == "Slider" || def.behaviorRole == "ProgressBar") {
        qml += QString("%1    property real value: %2\n").arg(indent).arg(m_value);
    }
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString CustomComponentInstance::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    CustomComponentDefinition def = definition();
    QString funcName = QString("create_%1").arg(def.name.isEmpty() ? "Custom" : def.name);
    QString code;
    code += QString("%1// Reusable custom component call: %2\n").arg(indent, m_id);
    code += QString("%1GHandle %2 = %3(NULL, %4, %5, %6, %7, %8f);\n")
        .arg(indent, m_id, funcName)
        .arg(static_cast<int>(pos().x()))
        .arg(static_cast<int>(pos().y()))
        .arg(static_cast<int>(m_width))
        .arg(static_cast<int>(m_height))
        .arg(m_value, 0, 'f', 1);
    return code;
}
