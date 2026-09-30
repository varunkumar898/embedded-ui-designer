#include "SliderComponent.h"
#include <QPainterPath>
#include <algorithm>

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

void SliderComponent::updateValueFromPos(const QPointF& p) {
    qreal trackMargin = 10.0;
    qreal trackWidth = m_width - (2.0 * trackMargin);
    if (trackWidth <= 0.0) return;

    qreal relX = std::clamp(p.x() - trackMargin, 0.0, trackWidth);
    qreal ratio = relX / trackWidth;
    int newVal = m_minimum + static_cast<int>(ratio * (m_maximum - m_minimum) + 0.5);
    setValue(newVal);
}

void SliderComponent::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (m_activeHandle == ResizeHandle::None && event->button() == Qt::LeftButton) {
        // Check if click is near track
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
    qreal trackMargin = 10.0;
    qreal trackH = 6.0;
    qreal trackY = (m_height - trackH) / 2.0;
    qreal trackW = m_width - (2.0 * trackMargin);

    // 1. Inactive Track background
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_trackColor);
    painter->drawRoundedRect(QRectF(trackMargin, trackY, trackW, trackH), 3.0, 3.0);

    // 2. Active Fill
    qreal range = static_cast<qreal>(m_maximum - m_minimum);
    qreal ratio = range > 0 ? (m_value - m_minimum) / range : 0.0;
    ratio = std::clamp(ratio, 0.0, 1.0);
    qreal fillW = trackW * ratio;

    if (fillW > 0) {
        painter->setBrush(m_fillColor);
        painter->drawRoundedRect(QRectF(trackMargin, trackY, fillW, trackH), 3.0, 3.0);
    }

    // 3. Thumb Handle
    qreal thumbRadius = 7.0;
    qreal thumbX = trackMargin + fillW;
    qreal thumbY = m_height / 2.0;

    painter->setBrush(m_handleColor);
    painter->setPen(QPen(m_fillColor, 1.5));
    painter->drawEllipse(QPointF(thumbX, thumbY), thumbRadius, thumbRadius);
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
    if (colorStyleRef("handleColor") == styleName) {
        m_handleColor = color;
        changed = true;
    }
    if (changed) {
        update();
        emit propertyChanged(this);
    }
}

QJsonObject SliderComponent::toJson() const {
    QJsonObject json = UIComponent::toJson();
    json["value"] = m_value;
    json["minimum"] = m_minimum;
    json["maximum"] = m_maximum;
    json["trackColor"] = serializeColor(m_trackColor, colorStyleRef("trackColor"));
    json["fillColor"] = serializeColor(m_fillColor, colorStyleRef("fillColor"));
    json["handleColor"] = serializeColor(m_handleColor, colorStyleRef("handleColor"));
    return json;
}

void SliderComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);
    m_minimum = json.value("minimum").toInt(0);
    m_maximum = json.value("maximum").toInt(100);
    m_value = json.value("value").toInt(50);
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
    }
}

QString SliderComponent::toQmlSnippet(int indentSpaces) const {
    QString ind(indentSpaces, ' ');
    QString qml;
    qml += ind + "Item {\n";
    qml += ind + QString("    id: %1\n").arg(m_id);
    qml += ind + QString("    x: %1; y: %2; width: %3; height: %4\n")
        .arg(static_cast<int>(compX())).arg(static_cast<int>(compY()))
        .arg(static_cast<int>(m_width)).arg(static_cast<int>(m_height));
    qml += ind + QString("    property int value: %1\n").arg(m_value);
    qml += ind + QString("    property int minimum: %1\n").arg(m_minimum);
    qml += ind + QString("    property int maximum: %1\n").arg(m_maximum);
    qml += ind + "    Rectangle {\n";
    qml += ind + "        anchors.verticalCenter: parent.verticalCenter\n";
    qml += ind + "        x: 10; width: parent.width - 20; height: 6; radius: 3\n";
    qml += ind + QString("        color: \"%1\"\n").arg(m_trackColor.name());
    qml += ind + "        Rectangle {\n";
    qml += ind + "            height: parent.height; radius: 3\n";
    qml += ind + QString("            width: (parent.width * (%1 - %2)) / (%3 - %2)\n")
        .arg(m_value).arg(m_minimum).arg(m_maximum);
    qml += ind + QString("            color: \"%1\"\n").arg(m_fillColor.name());
    qml += ind + "        }\n";
    qml += ind + "    }\n";
    qml += ind + "    Rectangle {\n";
    qml += ind + "        width: 14; height: 14; radius: 7\n";
    qml += ind + QString("        x: 10 + ((parent.width - 20) * (%1 - %2)) / (%3 - %2) - 7\n")
        .arg(m_value).arg(m_minimum).arg(m_maximum);
    qml += ind + "        anchors.verticalCenter: parent.verticalCenter\n";
    qml += ind + QString("        color: \"%1\"\n").arg(m_handleColor.name());
    qml += ind + QString("        border.color: \"%1\"; border.width: 1\n").arg(m_fillColor.name());
    qml += ind + "    }\n";
    qml += ind + "}\n";
    return qml;
}

QString SliderComponent::toUgfxSnippet(int indentSpaces) const {
    QString ind(indentSpaces, ' ');
    QString code;
    code += ind + QString("// Slider: %1\n").arg(m_id);
    code += ind + "gwinWidgetClearInit(&wi);\n";
    code += ind + "wi.g.show = gTrue;\n";
    code += ind + QString("wi.g.x = %1; wi.g.y = %2;\n").arg(static_cast<int>(compX())).arg(static_cast<int>(compY()));
    code += ind + QString("wi.g.width = %1; wi.g.height = %2;\n").arg(static_cast<int>(m_width)).arg(static_cast<int>(m_height));
    code += ind + QString("ghSlider_%1 = gwinSliderCreate(0, &wi);\n").arg(m_id);
    code += ind + QString("gwinSliderSetRange(ghSlider_%1, %2, %3);\n").arg(m_id).arg(m_minimum).arg(m_maximum);
    code += ind + QString("gwinSliderSetPosition(ghSlider_%1, %2);\n").arg(m_id).arg(m_value);
    return code;
}
