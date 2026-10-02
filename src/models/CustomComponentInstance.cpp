#include "CustomComponentInstance.h"
#include "Project.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QGraphicsSceneMouseEvent>
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

void CustomComponentInstance::setDefinition(const CustomComponentDefinition& def) {
    m_definitionId = def.id;
    m_cachedDefinition = def;
    m_width = def.width;
    m_height = def.height;
    m_minValue = def.minValue;
    m_maxValue = def.maxValue;
    m_value = def.defaultValue;
    update();
    emit propertyChanged(this);
}

CustomComponentDefinition CustomComponentInstance::definition() const {
    if (m_project) {
        CustomComponentDefinition def = m_project->findCustomComponentDefinition(m_definitionId);
        if (!def.id.isEmpty()) return def;
    }
    return m_cachedDefinition;
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
    qreal origW = std::max(1.0, definition().width);
    qreal origH = std::max(1.0, definition().height);
    qreal scaleX = m_width / origW;
    qreal scaleY = m_height / origH;

    qreal trackX = trackPrim.relX * scaleX;
    qreal trackY = trackPrim.relY * scaleY;
    qreal trackW = trackPrim.relWidth * scaleX;
    qreal trackH = trackPrim.relHeight * scaleY;

    QRectF trackRect(trackX, trackY, trackW, trackH);
    qreal thumbW = thumbPrim.relWidth * scaleX;
    qreal thumbH = thumbPrim.relHeight * scaleY;

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
        painter->setPen(QPen(QColor("#717C8F"), 1, Qt::DashLine));
        painter->setBrush(QColor(37, 40, 48, 180));
        painter->drawRoundedRect(0, 0, m_width, m_height, 6, 6);
        painter->setPen(QColor("#FFFFFF"));
        painter->drawText(QRectF(0, 0, m_width, m_height), Qt::AlignCenter, def.name.isEmpty() ? m_id : def.name);
        return;
    }

    qreal origW = std::max(1.0, def.width);
    qreal origH = std::max(1.0, def.height);
    qreal scaleX = m_width / origW;
    qreal scaleY = m_height / origH;

    const PrimitiveShapeData* trackPrim = nullptr;
    const PrimitiveShapeData* thumbPrim = nullptr;

    for (const auto& prim : def.primitives) {
        if (prim.role == "track" || prim.isTrack) trackPrim = &prim;
        if (prim.role == "thumb" || prim.isThumb) thumbPrim = &prim;
    }

    QString role = def.behaviorRole;

    for (const auto& prim : def.primitives) {
        QRectF destRect(prim.relX * scaleX, prim.relY * scaleY,
                        prim.relWidth * scaleX, prim.relHeight * scaleY);

        if ((role == "Slider" || role == "ProgressBar") && (prim.role == "thumb" || prim.isThumb) && trackPrim) {
            destRect = computeThumbRect(prim, *trackPrim);
        }

        QColor fill = prim.fillColor;
        QColor stroke = prim.strokeColor;
        int strokeW = prim.strokeWidth;
        int radius = static_cast<int>(prim.cornerRadius * ((scaleX + scaleY) / 2.0));

        painter->setPen(strokeW > 0 ? QPen(stroke, strokeW) : Qt::NoPen);
        painter->setBrush(fill.isValid() ? QBrush(fill) : Qt::NoBrush);

        if (prim.shapeType == "Circle" || prim.type == "Circle" || prim.shapeType == "Ellipse") {
            painter->drawEllipse(destRect);
        } else if (prim.shapeType == "Text" || prim.type == "Text") {
            QFont font("Roboto", 13);
            font.setBold(true);
            painter->setFont(font);
            painter->setPen(stroke.isValid() ? stroke : QColor("#FFFFFF"));
            painter->drawText(destRect, Qt::AlignCenter, def.name);
        } else {
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
            if (prim.role == "track" || prim.isTrack) trackPrim = &prim;
            if (prim.role == "thumb" || prim.isThumb) thumbPrim = &prim;
        }
        if (trackPrim && thumbPrim) {
            QRectF thumbR = computeThumbRect(*thumbPrim, *trackPrim);
            qreal clickX = event->pos().x();
            qreal origW = std::max(1.0, def.width);
            qreal scaleX = m_width / origW;
            QRectF trackRect(trackPrim->relX * scaleX, trackPrim->relY * scaleX,
                             trackPrim->relWidth * scaleX, trackPrim->relHeight * scaleX);

            if (trackRect.adjusted(-10, -10, 10, 10).contains(event->pos()) || thumbR.adjusted(-8, -8, 8, 8).contains(event->pos())) {
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
            if (prim.role == "track" || prim.isTrack) trackPrim = &prim;
            if (prim.role == "thumb" || prim.isThumb) thumbPrim = &prim;
        }
        if (trackPrim && thumbPrim) {
            qreal origW = std::max(1.0, def.width);
            qreal scaleX = m_width / origW;
            QRectF trackRect(trackPrim->relX * scaleX, trackPrim->relY * scaleX,
                             trackPrim->relWidth * scaleX, trackPrim->relHeight * scaleX);
            qreal thumbW = thumbPrim->relWidth * scaleX;
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
