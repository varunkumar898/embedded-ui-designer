#include "RectangleComponent.h"
#include <QPen>
#include <QBrush>
#include <cmath>
#include <algorithm>

RectangleComponent::RectangleComponent(const QString& id, QGraphicsItem* parent)
    : UIComponent(id, "Rectangle", parent)
{
    m_width = 160;
    m_height = 80;
}

void RectangleComponent::setFillColor(const QColor& color) {
    if (m_fillColor != color) {
        m_fillColor = color;
        update();
        emit propertyChanged(this);
    }
}

void RectangleComponent::setStrokeColor(const QColor& color) {
    if (m_strokeColor != color) {
        m_strokeColor = color;
        update();
        emit propertyChanged(this);
    }
}

void RectangleComponent::setStrokeWidth(int w) {
    w = std::max(0, w);
    if (m_strokeWidth != w) {
        m_strokeWidth = w;
        update();
        emit propertyChanged(this);
    }
}

void RectangleComponent::setCornerRadius(int r) {
    int maxR = static_cast<int>(std::floor(std::min(m_width, m_height) / 2.0));
    r = std::clamp(r, 0, std::max(0, maxR));
    if (m_cornerRadius != r) {
        m_cornerRadius = r;
        update();
        emit propertyChanged(this);
    }
}

void RectangleComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF rect(0, 0, m_width, m_height);

    // 1. Draw rounded fill
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(m_fillColor));
    painter->drawRoundedRect(rect, m_cornerRadius, m_cornerRadius);

    // 2. Draw border stroke with half-width inset for crispness
    if (m_strokeWidth > 0 && m_strokeColor.alpha() > 0) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(m_strokeColor, m_strokeWidth));
        qreal inset = m_strokeWidth / 2.0;
        QRectF strokeRect(inset, inset, m_width - m_strokeWidth, m_height - m_strokeWidth);
        qreal r = std::max<qreal>(0, m_cornerRadius - inset);
        painter->drawRoundedRect(strokeRect, r, r);
    }
}

QJsonObject RectangleComponent::toJson() const {
    QJsonObject obj = UIComponent::toJson();
    obj["fillColor"] = serializeColor(m_fillColor, colorStyleRef("fillColor"));
    obj["strokeColor"] = serializeColor(m_strokeColor, colorStyleRef("strokeColor"));
    obj["strokeWidth"] = m_strokeWidth;
    obj["cornerRadius"] = m_cornerRadius;
    return obj;
}

void RectangleComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);
    if (json.contains("fillColor")) {
        QString ref;
        deserializeColor(json.value("fillColor"), m_fillColor, ref);
        setColorStyleRef("fillColor", ref);
    }
    if (json.contains("strokeColor")) {
        QString ref;
        deserializeColor(json.value("strokeColor"), m_strokeColor, ref);
        setColorStyleRef("strokeColor", ref);
    } else if (json.contains("borderColor")) {
        QString ref;
        deserializeColor(json.value("borderColor"), m_strokeColor, ref);
        setColorStyleRef("strokeColor", ref);
    }
    if (json.contains("strokeWidth")) {
        m_strokeWidth = json.value("strokeWidth").toInt(m_strokeWidth);
    } else if (json.contains("borderWidth")) {
        m_strokeWidth = json.value("borderWidth").toInt(m_strokeWidth);
    }
    m_cornerRadius = json.value("cornerRadius").toInt(m_cornerRadius);
    update();
}

void RectangleComponent::applyColorStyle(const QString& styleName, const QColor& color) {
    bool changed = false;
    if (colorStyleRef("fillColor") == styleName) {
        m_fillColor = color;
        changed = true;
    }
    if (colorStyleRef("strokeColor") == styleName || colorStyleRef("borderColor") == styleName) {
        m_strokeColor = color;
        changed = true;
    }
    if (changed) {
        update();
        emit propertyChanged(this);
    }
}

QString RectangleComponent::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString qml;
    qml += QString("%1Rectangle {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    x: %2\n").arg(indent).arg(static_cast<int>(pos().x()));
    qml += QString("%1    y: %2\n").arg(indent).arg(static_cast<int>(pos().y()));
    qml += QString("%1    width: %2\n").arg(indent).arg(static_cast<int>(m_width));
    qml += QString("%1    height: %2\n").arg(indent).arg(static_cast<int>(m_height));
    qml += QString("%1    color: \"%2\"\n").arg(indent, m_fillColor.name());
    if (m_cornerRadius > 0) {
        qml += QString("%1    radius: %2\n").arg(indent).arg(m_cornerRadius);
    }
    if (m_strokeWidth > 0) {
        qml += QString("%1    border.color: \"%2\"\n").arg(indent, m_strokeColor.name());
        qml += QString("%1    border.width: %2\n").arg(indent).arg(m_strokeWidth);
    }
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString RectangleComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// Rectangle: %2\n").arg(indent, m_id);
    code += QString("%1gdispFillArea(%2, %3, %4, %5, HTML2COLOR(%6));\n")
        .arg(indent)
        .arg(static_cast<int>(pos().x()))
        .arg(static_cast<int>(pos().y()))
        .arg(static_cast<int>(m_width))
        .arg(static_cast<int>(m_height))
        .arg(m_fillColor.name().toUpper());
    return code;
}
