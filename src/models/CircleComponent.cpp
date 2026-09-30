#include "CircleComponent.h"
#include <QPainterPath>

CircleComponent::CircleComponent(const QString& id, QGraphicsItem* parent)
    : UIComponent(id, "Circle", parent)
{
    m_width = 40.0;
    m_height = 40.0;
}

void CircleComponent::setFillColor(const QColor& color) {
    if (m_fillColor != color) {
        m_fillColor = color;
        update();
        emit propertyChanged(this);
    }
}

void CircleComponent::setStrokeColor(const QColor& color) {
    if (m_strokeColor != color) {
        m_strokeColor = color;
        update();
        emit propertyChanged(this);
    }
}

void CircleComponent::setStrokeWidth(int width) {
    if (m_strokeWidth != width) {
        m_strokeWidth = width;
        update();
        emit propertyChanged(this);
    }
}

void CircleComponent::setFilled(bool filled) {
    if (m_isFilled != filled) {
        m_isFilled = filled;
        update();
        emit propertyChanged(this);
    }
}

void CircleComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    qreal diameter = qMin(m_width, m_height);
    qreal offset = m_strokeWidth > 0 ? (m_strokeWidth / 2.0) : 0.0;
    qreal effDim = diameter - (offset * 2.0);
    qreal x = (m_width - diameter) / 2.0 + offset;
    qreal y = (m_height - diameter) / 2.0 + offset;

    if (m_strokeWidth > 0) {
        painter->setPen(QPen(m_strokeColor, m_strokeWidth));
    } else {
        painter->setPen(Qt::NoPen);
    }

    if (m_isFilled) {
        painter->setBrush(m_fillColor);
    } else {
        painter->setBrush(Qt::NoBrush);
    }

    painter->drawEllipse(QRectF(x, y, effDim, effDim));
}

void CircleComponent::applyColorStyle(const QString& styleName, const QColor& color) {
    bool changed = false;
    if (colorStyleRef("fillColor") == styleName) {
        m_fillColor = color;
        changed = true;
    }
    if (colorStyleRef("strokeColor") == styleName) {
        m_strokeColor = color;
        changed = true;
    }
    if (changed) {
        update();
        emit propertyChanged(this);
    }
}

QJsonObject CircleComponent::toJson() const {
    QJsonObject json = UIComponent::toJson();
    json["fillColor"] = serializeColor(m_fillColor, colorStyleRef("fillColor"));
    json["strokeColor"] = serializeColor(m_strokeColor, colorStyleRef("strokeColor"));
    json["strokeWidth"] = m_strokeWidth;
    json["isFilled"] = m_isFilled;
    return json;
}

void CircleComponent::fromJson(const QJsonObject& json) {
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
    }
    m_strokeWidth = json.value("strokeWidth").toInt(1);
    m_isFilled = json.value("isFilled").toBool(true);
}

QString CircleComponent::toQmlSnippet(int indentSpaces) const {
    QString ind(indentSpaces, ' ');
    int dim = static_cast<int>(qMin(m_width, m_height));
    QString qml;
    qml += ind + "Rectangle {\n";
    qml += ind + QString("    id: %1\n").arg(m_id);
    qml += ind + QString("    x: %1; y: %2; width: %3; height: %4\n")
        .arg(static_cast<int>(compX())).arg(static_cast<int>(compY()))
        .arg(dim).arg(dim);
    qml += ind + "    radius: width / 2\n";
    qml += ind + QString("    color: \"%1\"\n").arg(m_isFilled ? m_fillColor.name() : "transparent");
    if (m_strokeWidth > 0) {
        qml += ind + QString("    border.color: \"%1\"\n").arg(m_strokeColor.name());
        qml += ind + QString("    border.width: %1\n").arg(m_strokeWidth);
    }
    qml += ind + "}\n";
    return qml;
}

QString CircleComponent::toUgfxSnippet(int indentSpaces) const {
    QString ind(indentSpaces, ' ');
    int dim = static_cast<int>(qMin(m_width, m_height));
    int radius = dim / 2;
    int cx = static_cast<int>(compX()) + radius;
    int cy = static_cast<int>(compY()) + radius;

    QString code;
    code += ind + QString("// Circle Shape: %1\n").arg(m_id);
    if (m_isFilled) {
        code += ind + QString("gdispFillCircle(%1, %2, %3, HTML2COLOR(0x%4));\n")
            .arg(cx).arg(cy).arg(radius).arg(m_fillColor.name().mid(1).toUpper());
    }
    if (m_strokeWidth > 0) {
        code += ind + QString("gdispDrawCircle(%1, %2, %3, HTML2COLOR(0x%4));\n")
            .arg(cx).arg(cy).arg(radius).arg(m_strokeColor.name().mid(1).toUpper());
    }
    return code;
}
