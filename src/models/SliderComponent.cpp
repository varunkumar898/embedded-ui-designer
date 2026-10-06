#include "SliderComponent.h"
#include <QPainterPath>
#include <algorithm>
#include <cmath>

SliderComponent::SliderComponent(const QString& id, QGraphicsItem* parent)
    : UIComponent(id, "Slider", parent)
{
    m_width = 160.0;
    m_height = 24.0;
}

void SliderComponent::setValue(int val) {
    int clamped = std::clamp(val, m_minimum, m_maximum);
    if (m_value != clamped) {
        m_value = clamped;
        update();
        emit propertyChanged(this);
        emit interactionTriggered("On Change");
        emit interactionTriggered("On Value Changed");
    }
}

void SliderComponent::setMinimum(int min) {
    if (m_minimum != min) {
        m_minimum = min;
        setValue(m_value);
        emit propertyChanged(this);
    }
}

void SliderComponent::setMaximum(int max) {
    if (m_maximum != max) {
        m_maximum = max;
        setValue(m_value);
        emit propertyChanged(this);
    }
}

void SliderComponent::setOrientation(const QString& orient) {
    Qt::Orientation o = (orient.compare("Vertical", Qt::CaseInsensitive) == 0) ? Qt::Vertical : Qt::Horizontal;
    if (m_orientation != o) {
        m_orientation = o;
        update();
        emit propertyChanged(this);
    }
}

void SliderComponent::setTrackColor(const QColor& color) {
    if (m_trackColor != color) {
        m_trackColor = color;
        update();
        emit propertyChanged(this);
    }
}

void SliderComponent::setFillColor(const QColor& color) {
    if (m_fillColor != color) {
        m_fillColor = color;
        update();
        emit propertyChanged(this);
    }
}

void SliderComponent::setHandleColor(const QColor& color) {
    if (m_handleColor != color) {
        m_handleColor = color;
        update();
        emit propertyChanged(this);
    }
}

void SliderComponent::setBorderColor(const QColor& color) {
    if (m_borderColor != color) {
        m_borderColor = color;
        update();
        emit propertyChanged(this);
    }
}

void SliderComponent::setBorderWidth(int width) {
    width = std::max(0, width);
    if (m_borderWidth != width) {
        m_borderWidth = width;
        update();
        emit propertyChanged(this);
    }
}

void SliderComponent::setCornerRadius(int r) {
    r = std::max(0, r);
    if (m_cornerRadius != r) {
        m_cornerRadius = r;
        update();
        emit propertyChanged(this);
    }
}

void SliderComponent::updateValueFromPos(const QPointF& p) {
    if (m_orientation == Qt::Horizontal) {
        qreal trackMargin = 10.0;
        qreal trackWidth = m_width - (2.0 * trackMargin);
        if (trackWidth <= 0.0) return;
        qreal relX = std::clamp(p.x() - trackMargin, 0.0, trackWidth);
        qreal ratio = relX / trackWidth;
        int newVal = m_minimum + static_cast<int>(ratio * (m_maximum - m_minimum) + 0.5);
        setValue(newVal);
    } else {
        qreal trackMargin = 10.0;
        qreal trackHeight = m_height - (2.0 * trackMargin);
        if (trackHeight <= 0.0) return;
        qreal relY = std::clamp(m_height - trackMargin - p.y(), 0.0, trackHeight);
        qreal ratio = relY / trackHeight;
        int newVal = m_minimum + static_cast<int>(ratio * (m_maximum - m_minimum) + 0.5);
        setValue(newVal);
    }
}

void SliderComponent::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (m_activeHandle == ResizeHandle::None && event->button() == Qt::LeftButton) {
        updateValueFromPos(event->pos());
    }
    UIComponent::mousePressEvent(event);
}

void SliderComponent::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (!m_resizing && (event->buttons() & Qt::LeftButton) && m_activeHandle == ResizeHandle::None) {
        updateValueFromPos(event->pos());
    }
    UIComponent::mouseMoveEvent(event);
}

void SliderComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    qreal range = static_cast<qreal>(m_maximum - m_minimum);
    qreal ratio = range > 0 ? (m_value - m_minimum) / range : 0.0;
    ratio = std::clamp(ratio, 0.0, 1.0);

    if (m_orientation == Qt::Horizontal) {
        qreal trackMargin = 10.0;
        qreal trackH = 6.0;
        qreal trackY = (m_height - trackH) / 2.0;
        qreal trackW = m_width - (2.0 * trackMargin);

        // 1. Inactive Track background
        if (m_borderWidth > 0 && m_borderColor.alpha() > 0) {
            painter->setPen(QPen(m_borderColor, m_borderWidth));
        } else {
            painter->setPen(Qt::NoPen);
        }
        painter->setBrush(m_trackColor);
        painter->drawRoundedRect(QRectF(trackMargin, trackY, trackW, trackH), m_cornerRadius, m_cornerRadius);

        // 2. Active Fill
        qreal fillW = trackW * ratio;
        if (fillW > 0) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(m_fillColor);
            painter->drawRoundedRect(QRectF(trackMargin, trackY, fillW, trackH), m_cornerRadius, m_cornerRadius);
        }

        // 3. Thumb Handle
        qreal thumbRadius = 7.0;
        qreal thumbX = trackMargin + fillW;
        qreal thumbY = m_height / 2.0;

        painter->setBrush(m_handleColor);
        painter->setPen(QPen(m_fillColor, 1.5));
        painter->drawEllipse(QPointF(thumbX, thumbY), thumbRadius, thumbRadius);
    } else {
        qreal trackMargin = 10.0;
        qreal trackW = 6.0;
        qreal trackX = (m_width - trackW) / 2.0;
        qreal trackH = m_height - (2.0 * trackMargin);

        if (m_borderWidth > 0 && m_borderColor.alpha() > 0) {
            painter->setPen(QPen(m_borderColor, m_borderWidth));
        } else {
            painter->setPen(Qt::NoPen);
        }
        painter->setBrush(m_trackColor);
        painter->drawRoundedRect(QRectF(trackX, trackMargin, trackW, trackH), m_cornerRadius, m_cornerRadius);

        qreal fillH = trackH * ratio;
        if (fillH > 0) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(m_fillColor);
            painter->drawRoundedRect(QRectF(trackX, trackMargin + (trackH - fillH), trackW, fillH), m_cornerRadius, m_cornerRadius);
        }

        qreal thumbRadius = 7.0;
        qreal thumbX = m_width / 2.0;
        qreal thumbY = trackMargin + (trackH - fillH);

        painter->setBrush(m_handleColor);
        painter->setPen(QPen(m_fillColor, 1.5));
        painter->drawEllipse(QPointF(thumbX, thumbY), thumbRadius, thumbRadius);
    }
}

QJsonObject SliderComponent::toJson() const {
    QJsonObject json = UIComponent::toJson();
    json["value"] = m_value;
    json["minimum"] = m_minimum;
    json["maximum"] = m_maximum;
    json["orientation"] = orientation();
    json["trackColor"] = serializeColor(m_trackColor, colorStyleRef("trackColor"));
    json["fillColor"] = serializeColor(m_fillColor, colorStyleRef("fillColor"));
    json["handleColor"] = serializeColor(m_handleColor, colorStyleRef("handleColor"));
    json["borderColor"] = serializeColor(m_borderColor, colorStyleRef("borderColor"));
    json["borderWidth"] = m_borderWidth;
    json["cornerRadius"] = m_cornerRadius;
    return json;
}

void SliderComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);
    m_minimum = json.value("minimum").toInt(0);
    m_maximum = json.value("maximum").toInt(100);
    m_value = json.value("value").toInt(50);
    if (json.contains("orientation")) {
        setOrientation(json.value("orientation").toString("Horizontal"));
    }
    if (json.contains("trackColor")) {
        QString ref;
        deserializeColor(json.value("trackColor"), m_trackColor, ref);
        setColorStyleRef("trackColor", ref);
    }
    if (json.contains("fillColor")) {
        QString ref;
        deserializeColor(json.value("fillColor"), m_fillColor, ref);
        setColorStyleRef("fillColor", ref);
    }
    if (json.contains("handleColor")) {
        QString ref;
        deserializeColor(json.value("handleColor"), m_handleColor, ref);
        setColorStyleRef("handleColor", ref);
    } else if (json.contains("thumbColor")) {
        QString ref;
        deserializeColor(json.value("thumbColor"), m_handleColor, ref);
        setColorStyleRef("handleColor", ref);
    }
    if (json.contains("borderColor")) {
        QString ref;
        deserializeColor(json.value("borderColor"), m_borderColor, ref);
        setColorStyleRef("borderColor", ref);
    }
    m_borderWidth = json.value("borderWidth").toInt(m_borderWidth);
    m_cornerRadius = json.value("cornerRadius").toInt(m_cornerRadius);
    update();
}

void SliderComponent::applyColorStyle(const QString& styleName, const QColor& color) {
    bool changed = false;
    if (colorStyleRef("trackColor") == styleName) {
        m_trackColor = color;
        changed = true;
    }
    if (colorStyleRef("fillColor") == styleName) {
        m_fillColor = color;
        changed = true;
    }
    if (colorStyleRef("handleColor") == styleName || colorStyleRef("thumbColor") == styleName) {
        m_handleColor = color;
        changed = true;
    }
    if (colorStyleRef("borderColor") == styleName) {
        m_borderColor = color;
        changed = true;
    }
    if (changed) {
        update();
        emit propertyChanged(this);
    }
}

QString SliderComponent::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString qml;
    qml += QString("%1Slider {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    x: %2\n").arg(indent).arg(static_cast<int>(pos().x()));
    qml += QString("%1    y: %2\n").arg(indent).arg(static_cast<int>(pos().y()));
    qml += QString("%1    width: %2\n").arg(indent).arg(static_cast<int>(m_width));
    qml += QString("%1    height: %2\n").arg(indent).arg(static_cast<int>(m_height));
    qml += QString("%1    from: %2\n").arg(indent).arg(m_minimum);
    qml += QString("%1    to: %2\n").arg(indent).arg(m_maximum);
    qml += QString("%1    value: %2\n").arg(indent).arg(m_value);
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString SliderComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// Slider: %2\n").arg(indent, m_id);
    code += QString("%1wi.g.x = %2; wi.g.y = %3;\n").arg(indent).arg(static_cast<int>(pos().x())).arg(static_cast<int>(pos().y()));
    code += QString("%1wi.g.width = %2; wi.g.height = %3;\n").arg(indent).arg(static_cast<int>(m_width)).arg(static_cast<int>(m_height));
    code += QString("%1wi.text = \"\";\n").arg(indent);
    code += QString("%1GHandle slider_%2 = gwinSliderCreate(NULL, &wi);\n").arg(indent, m_id);
    code += QString("%1gwinSliderSetRange(slider_%2, %3, %4);\n").arg(indent, m_id).arg(m_minimum).arg(m_maximum);
    code += QString("%1gwinSliderSetPosition(slider_%2, %3);\n").arg(indent, m_id).arg(m_value);
    code += QString("%1gwinSetVisible(slider_%2, gTrue);\n").arg(indent, m_id);
    return code;
}
