#include "ProgressBarComponent.h"
#include <QPen>
#include <QBrush>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

ProgressBarComponent::ProgressBarComponent(const QString& id, QGraphicsItem* parent)
    : UIComponent(id, "ProgressBar", parent)
{
    m_width = 180;
    m_height = 20;
}

double ProgressBarComponent::actualValue() const {
    if (m_maximum <= m_minimum) return m_minimum;
    return m_minimum + m_value * (m_maximum - m_minimum);
}

void ProgressBarComponent::setActualValue(double val) {
    if (m_maximum <= m_minimum) return;
    val = std::clamp(val, m_minimum, m_maximum);
    double normalized = (val - m_minimum) / (m_maximum - m_minimum);
    setValue(normalized);
}

void ProgressBarComponent::setValue(double val) {
    // If a raw engineering value is passed (e.g. 65 when max is 100), map it
    if (val > 1.0 && m_maximum > 1.0) {
        setActualValue(val);
        return;
    }
    val = std::clamp(val, 0.0, 1.0);
    if (m_value != val) {
        m_value = val;
        update();
        emit propertyChanged(this);
        emit interactionTriggered("On Change");
        emit interactionTriggered("On Value Changed");
    }
}

void ProgressBarComponent::setMinimum(double min) {
    if (m_minimum != min) {
        m_minimum = min;
        update();
        emit propertyChanged(this);
    }
}

void ProgressBarComponent::setMaximum(double max) {
    if (m_maximum != max) {
        m_maximum = max;
        update();
        emit propertyChanged(this);
    }
}

void ProgressBarComponent::setOrientation(const QString& orient) {
    Qt::Orientation o = (orient.compare("Vertical", Qt::CaseInsensitive) == 0) ? Qt::Vertical : Qt::Horizontal;
    if (m_orientation != o) {
        m_orientation = o;
        update();
        emit propertyChanged(this);
    }
}

void ProgressBarComponent::setBarColor(const QColor& color) {
    if (m_barColor != color) {
        m_barColor = color;
        update();
        emit propertyChanged(this);
    }
}

void ProgressBarComponent::setTrackColor(const QColor& color) {
    if (m_trackColor != color) {
        m_trackColor = color;
        update();
        emit propertyChanged(this);
    }
}

void ProgressBarComponent::setBorderColor(const QColor& color) {
    if (m_borderColor != color) {
        m_borderColor = color;
        update();
        emit propertyChanged(this);
    }
}

void ProgressBarComponent::setBorderWidth(int width) {
    width = std::max(0, width);
    if (m_borderWidth != width) {
        m_borderWidth = width;
        update();
        emit propertyChanged(this);
    }
}

void ProgressBarComponent::setCornerRadius(int r) {
    int maxR = static_cast<int>(std::floor(std::min(m_width, m_height) / 2.0));
    r = std::clamp(r, 0, std::max(0, maxR));
    if (m_cornerRadius != r) {
        m_cornerRadius = r;
        update();
        emit propertyChanged(this);
    }
}

void ProgressBarComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF trackRect(0, 0, m_width, m_height);

    // 1. Draw modern dark outer track
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(m_trackColor));
    painter->drawRoundedRect(trackRect, m_cornerRadius, m_cornerRadius);

    // 2. Draw smooth progress fill clipped to outer rounded track
    QPainterPath trackPath;
    trackPath.addRoundedRect(trackRect, m_cornerRadius, m_cornerRadius);

    painter->save();
    painter->setClipPath(trackPath);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(m_barColor));

    if (m_orientation == Qt::Horizontal) {
        qreal fillW = m_width * std::clamp(m_value, 0.0, 1.0);
        if (fillW > 0.0) {
            painter->drawRect(QRectF(0, 0, fillW, m_height));
        }
    } else {
        qreal fillH = m_height * std::clamp(m_value, 0.0, 1.0);
        if (fillH > 0.0) {
            painter->drawRect(QRectF(0, m_height - fillH, m_width, fillH));
        }
    }
    painter->restore();

    // 3. Draw subtle contrast border
    if (m_borderWidth > 0 && m_borderColor.alpha() > 0) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(m_borderColor, m_borderWidth));
        qreal inset = m_borderWidth / 2.0;
        QRectF borderRect(inset, inset, m_width - m_borderWidth, m_height - m_borderWidth);
        qreal borderR = std::max<qreal>(0, m_cornerRadius - inset);
        painter->drawRoundedRect(borderRect, borderR, borderR);
    }
}

QJsonObject ProgressBarComponent::toJson() const {
    QJsonObject obj = UIComponent::toJson();
    obj["value"] = m_value;
    obj["displayValue"] = actualValue();
    obj["minimum"] = m_minimum;
    obj["maximum"] = m_maximum;
    obj["orientation"] = orientation();

    QString barRef = colorStyleRef("barColor").isEmpty() ? colorStyleRef("fillColor") : colorStyleRef("barColor");
    obj["barColor"] = serializeColor(m_barColor, barRef);
    obj["trackColor"] = serializeColor(m_trackColor, colorStyleRef("trackColor"));
    obj["borderColor"] = serializeColor(m_borderColor, colorStyleRef("borderColor"));
    obj["borderWidth"] = m_borderWidth;
    obj["cornerRadius"] = m_cornerRadius;
    return obj;
}

void ProgressBarComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);

    if (json.contains("minimum")) m_minimum = json.value("minimum").toDouble(0.0);
    if (json.contains("maximum")) m_maximum = json.value("maximum").toDouble(100.0);

    if (json.contains("displayValue")) {
        setActualValue(json.value("displayValue").toDouble());
    } else if (json.contains("value")) {
        double v = json.value("value").toDouble(m_value);
        if (v > 1.0 && m_maximum > 1.0) {
            setActualValue(v);
        } else {
            setValue(v);
        }
    }

    if (json.contains("orientation")) {
        setOrientation(json.value("orientation").toString("Horizontal"));
    }

    if (json.contains("barColor")) {
        QString ref;
        deserializeColor(json.value("barColor"), m_barColor, ref);
        setColorStyleRef("barColor", ref);
    }
    if (json.contains("trackColor")) {
        QString ref;
        deserializeColor(json.value("trackColor"), m_trackColor, ref);
        setColorStyleRef("trackColor", ref);
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

void ProgressBarComponent::applyColorStyle(const QString& styleName, const QColor& color) {
    bool changed = false;
    if (colorStyleRef("barColor") == styleName || colorStyleRef("fillColor") == styleName) {
        m_barColor = color;
        changed = true;
    }
    if (colorStyleRef("trackColor") == styleName) {
        m_trackColor = color;
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

QString ProgressBarComponent::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString qml;
    qml += QString("%1ProgressBar {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    x: %2\n").arg(indent).arg(static_cast<int>(pos().x()));
    qml += QString("%1    y: %2\n").arg(indent).arg(static_cast<int>(pos().y()));
    qml += QString("%1    width: %2\n").arg(indent).arg(static_cast<int>(m_width));
    qml += QString("%1    height: %2\n").arg(indent).arg(static_cast<int>(m_height));
    qml += QString("%1    from: %2\n").arg(indent).arg(m_minimum, 0, 'f', 1);
    qml += QString("%1    to: %2\n").arg(indent).arg(m_maximum, 0, 'f', 1);
    qml += QString("%1    value: %2\n").arg(indent).arg(actualValue(), 0, 'f', 1);
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString ProgressBarComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// ProgressBar: %2\n").arg(indent, m_id);
    code += QString("%1wi.g.x = %2; wi.g.y = %3;\n").arg(indent).arg(static_cast<int>(pos().x())).arg(static_cast<int>(pos().y()));
    code += QString("%1wi.g.width = %2; wi.g.height = %3;\n").arg(indent).arg(static_cast<int>(m_width)).arg(static_cast<int>(m_height));
    code += QString("%1wi.text = \"\";\n").arg(indent);
    code += QString("%1GHandle prog_%2 = gwinProgressbarCreate(NULL, &wi);\n").arg(indent, m_id);
    code += QString("%1gwinProgressbarSetRange(prog_%2, %3, %4);\n").arg(indent, m_id).arg(static_cast<int>(m_minimum)).arg(static_cast<int>(m_maximum));
    code += QString("%1gwinProgressbarSetPosition(prog_%2, %3);\n").arg(indent, m_id).arg(static_cast<int>(actualValue()));
    code += QString("%1gwinSetVisible(prog_%2, gTrue);\n").arg(indent, m_id);
    return code;
}
