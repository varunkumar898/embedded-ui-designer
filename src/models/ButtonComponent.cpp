#include "ButtonComponent.h"
#include <QPen>
#include <QBrush>
#include <QFontMetrics>
#include <algorithm>
#include <cmath>

ButtonComponent::ButtonComponent(const QString& id, QGraphicsItem* parent)
    : UIComponent(id, "Button", parent)
{
    m_width = 120;
    m_height = 40;
}

void ButtonComponent::setText(const QString& text) {
    if (m_text != text) {
        m_text = text;
        update();
        emit propertyChanged(this);
    }
}

void ButtonComponent::setBackgroundColor(const QColor& color) {
    if (m_backgroundColor != color) {
        m_backgroundColor = color;
        update();
        emit propertyChanged(this);
    }
}

void ButtonComponent::setTextColor(const QColor& color) {
    if (m_textColor != color) {
        m_textColor = color;
        update();
        emit propertyChanged(this);
    }
}

void ButtonComponent::setBorderColor(const QColor& color) {
    if (m_borderColor != color) {
        m_borderColor = color;
        update();
        emit propertyChanged(this);
    }
}

void ButtonComponent::setBorderWidth(int width) {
    width = std::max(0, width);
    if (m_borderWidth != width) {
        m_borderWidth = width;
        update();
        emit propertyChanged(this);
    }
}

void ButtonComponent::setFontFamily(const QString& family) {
    if (m_fontFamily != family) {
        m_fontFamily = family;
        update();
        emit propertyChanged(this);
    }
}

void ButtonComponent::setPixelSize(int size) {
    size = std::clamp(size, 6, 72);
    if (m_pixelSize != size) {
        m_pixelSize = size;
        update();
        emit propertyChanged(this);
    }
}

void ButtonComponent::setBold(bool b) {
    if (m_bold != b) {
        m_bold = b;
        update();
        emit propertyChanged(this);
    }
}

void ButtonComponent::setEnabled(bool enabled) {
    if (m_enabled != enabled) {
        m_enabled = enabled;
        update();
        emit propertyChanged(this);
    }
}

void ButtonComponent::setCornerRadius(int r) {
    int maxR = static_cast<int>(std::floor(std::min(m_width, m_height) / 2.0));
    r = std::clamp(r, 0, std::max(0, maxR));
    if (m_cornerRadius != r) {
        m_cornerRadius = r;
        update();
        emit propertyChanged(this);
    }
}

void ButtonComponent::setOnClickedHandler(const QString& handler) {
    if (m_onClickedHandler != handler) {
        m_onClickedHandler = handler;
        emit propertyChanged(this);
    }
}

void ButtonComponent::setTargetScreenId(const QString& targetScreenId) {
    if (m_targetScreenId != targetScreenId) {
        m_targetScreenId = targetScreenId;
        emit targetScreenIdChanged(m_targetScreenId);
        emit propertyChanged(this);
    }
}

void ButtonComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF rect(0, 0, m_width, m_height);

    // Compute effective background and text colors based on enabled state
    QColor bg = effectiveBackgroundColor(m_backgroundColor);
    QColor fg = effectiveTextColor(m_textColor);
    QColor bd = effectiveBorderColor(m_borderColor);
    int bw = effectiveBorderWidth(m_borderWidth);
    if (!m_enabled || (hasStateStyle(currentState()) && stateStyle(currentState()).hasEnabled && !stateStyle(currentState()).enabled)) {
        bg.setAlpha(120);
        fg.setAlpha(120);
        bd.setAlpha(80);
    }

    // 1. Button body
    painter->setBrush(QBrush(bg));
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(rect, m_cornerRadius, m_cornerRadius);

    // 2. Border
    if (bw > 0 && bd.alpha() > 0) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(bd, bw));
        qreal inset = bw / 2.0;
        QRectF borderRect(inset, inset, m_width - bw, m_height - bw);
        qreal r = std::max<qreal>(0, m_cornerRadius - inset);
        painter->drawRoundedRect(borderRect, r, r);
    }


    // 3. Button text
    painter->setPen(QPen(fg));
    QFont font(m_fontFamily);
    font.setPixelSize(m_pixelSize);
    font.setBold(m_bold);
    painter->setFont(font);
    painter->drawText(rect, Qt::AlignCenter, m_text);
}

QJsonObject ButtonComponent::toJson() const {
    QJsonObject obj = UIComponent::toJson();
    obj["text"] = m_text;
    obj["backgroundColor"] = serializeColor(m_backgroundColor, colorStyleRef("backgroundColor"));
    obj["textColor"] = serializeColor(m_textColor, colorStyleRef("textColor"));
    obj["borderColor"] = serializeColor(m_borderColor, colorStyleRef("borderColor"));
    obj["borderWidth"] = m_borderWidth;
    obj["fontFamily"] = m_fontFamily;
    obj["pixelSize"] = m_pixelSize;
    obj["bold"] = m_bold;
    obj["enabled"] = m_enabled;
    obj["cornerRadius"] = m_cornerRadius;
    obj["onClicked"] = m_onClickedHandler;
    if (!m_targetScreenId.isEmpty()) {
        obj["targetScreenId"] = m_targetScreenId;
    }
    return obj;
}

void ButtonComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);
    m_text = json.value("text").toString(m_text);
    if (json.contains("backgroundColor")) {
        QString ref;
        deserializeColor(json.value("backgroundColor"), m_backgroundColor, ref);
        setColorStyleRef("backgroundColor", ref);
    }
    if (json.contains("textColor")) {
        QString ref;
        deserializeColor(json.value("textColor"), m_textColor, ref);
        setColorStyleRef("textColor", ref);
    }
    if (json.contains("borderColor")) {
        QString ref;
        deserializeColor(json.value("borderColor"), m_borderColor, ref);
        setColorStyleRef("borderColor", ref);
    }
    m_borderWidth = json.value("borderWidth").toInt(m_borderWidth);
    m_fontFamily = json.value("fontFamily").toString(m_fontFamily);
    m_pixelSize = json.value("pixelSize").toInt(m_pixelSize);
    m_bold = json.value("bold").toBool(m_bold);
    m_enabled = json.value("enabled").toBool(m_enabled);
    m_cornerRadius = json.value("cornerRadius").toInt(m_cornerRadius);
    m_onClickedHandler = json.value("onClicked").toString(m_onClickedHandler);
    m_targetScreenId = json.value("targetScreenId").toString();
    update();
}

void ButtonComponent::applyColorStyle(const QString& styleName, const QColor& color) {
    bool changed = false;
    if (colorStyleRef("backgroundColor") == styleName) {
        m_backgroundColor = color;
        changed = true;
    }
    if (colorStyleRef("textColor") == styleName) {
        m_textColor = color;
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

QString ButtonComponent::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString qml;
    qml += QString("%1Button {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    x: %2\n").arg(indent).arg(static_cast<int>(pos().x()));
    qml += QString("%1    y: %2\n").arg(indent).arg(static_cast<int>(pos().y()));
    qml += QString("%1    width: %2\n").arg(indent).arg(static_cast<int>(m_width));
    qml += QString("%1    height: %2\n").arg(indent).arg(static_cast<int>(m_height));
    qml += QString("%1    text: \"%2\"\n").arg(indent, m_text);
    qml += QString("%1    enabled: %2\n").arg(indent, m_enabled ? "true" : "false");
    if (!m_onClickedHandler.isEmpty()) {
        qml += QString("%1    onClicked: root.%2()\n").arg(indent, m_onClickedHandler);
    }
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString ButtonComponent::toCppSignalSlotStub() const {
    if (m_onClickedHandler.isEmpty()) return QString();
    return QString("// Signal connection for %1\n// connect(root, SIGNAL(%2()), this, SLOT(on_%2()));\nvoid on_%2();\n")
        .arg(m_id, m_onClickedHandler);
}

QString ButtonComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// Button: %2\n").arg(indent, m_id);
    code += QString("%1wi.g.x = %2; wi.g.y = %3;\n").arg(indent).arg(static_cast<int>(pos().x())).arg(static_cast<int>(pos().y()));
    code += QString("%1wi.g.width = %2; wi.g.height = %3;\n").arg(indent).arg(static_cast<int>(m_width)).arg(static_cast<int>(m_height));
    code += QString("%1wi.text = \"%2\";\n").arg(indent, m_text);
    code += QString("%1wi.customDraw = NULL;\n").arg(indent);
    code += QString("%1GHandle btn_%2 = gwinButtonCreate(NULL, &wi);\n").arg(indent, m_id);
    code += QString("%1gwinSetVisible(btn_%2, gTrue);\n").arg(indent, m_id);
    return code;
}
