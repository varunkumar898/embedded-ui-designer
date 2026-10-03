#include "TextInputComponent.h"
#include <QPainterPath>
#include <QFontMetrics>

TextInputComponent::TextInputComponent(const QString& id, QGraphicsItem* parent)
    : UIComponent(id, "TextInput", parent)
{
    m_width = 140.0;
    m_height = 30.0;
}

void TextInputComponent::setText(const QString& text) {
    if (m_text != text) {
        m_text = text;
        update();
        emit propertyChanged(this);
        emit interactionTriggered("On Change");
        emit interactionTriggered("On Value Changed");
    }
}

void TextInputComponent::setPlaceholder(const QString& placeholder) {
    if (m_placeholder != placeholder) {
        m_placeholder = placeholder;
        update();
        emit propertyChanged(this);
    }
}

void TextInputComponent::setTextColor(const QColor& color) {
    if (m_textColor != color) {
        m_textColor = color;
        update();
        emit propertyChanged(this);
    }
}

void TextInputComponent::setPlaceholderColor(const QColor& color) {
    if (m_placeholderColor != color) {
        m_placeholderColor = color;
        update();
        emit propertyChanged(this);
    }
}

void TextInputComponent::setBackgroundColor(const QColor& color) {
    if (m_backgroundColor != color) {
        m_backgroundColor = color;
        update();
        emit propertyChanged(this);
    }
}

void TextInputComponent::setBorderColor(const QColor& color) {
    if (m_borderColor != color) {
        m_borderColor = color;
        update();
        emit propertyChanged(this);
    }
}

void TextInputComponent::setBorderWidth(int width) {
    if (m_borderWidth != width) {
        m_borderWidth = width;
        update();
        emit propertyChanged(this);
    }
}

void TextInputComponent::setCornerRadius(int radius) {
    int maxR = static_cast<int>(std::floor(std::min(m_width, m_height) / 2.0));
    radius = std::clamp(radius, 0, std::max(0, maxR));
    if (m_cornerRadius != radius) {
        m_cornerRadius = radius;
        update();
        emit propertyChanged(this);
    }
}

void TextInputComponent::setPixelSize(int size) {
    if (m_pixelSize != size) {
        m_pixelSize = size;
        update();
        emit propertyChanged(this);
    }
}

void TextInputComponent::setReadOnly(bool ro) {
    if (m_readOnly != ro) {
        m_readOnly = ro;
        update();
        emit propertyChanged(this);
    }
}

void TextInputComponent::setOnTextChangedHandler(const QString& handler) {
    if (m_onTextChangedHandler != handler) {
        m_onTextChangedHandler = handler;
        emit propertyChanged(this);
    }
}

void TextInputComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    // 1. Draw input field background and border
    if (m_borderWidth > 0) {
        painter->setPen(QPen(m_borderColor, m_borderWidth));
    } else {
        painter->setPen(Qt::NoPen);
    }
    painter->setBrush(m_backgroundColor);
    painter->drawRoundedRect(QRectF(0, 0, m_width, m_height), m_cornerRadius, m_cornerRadius);

    // 2. Draw text or placeholder
    QFont font = painter->font();
    font.setPointSize(qMax(8, m_pixelSize - 3));
    painter->setFont(font);

    bool showPlaceholder = m_text.isEmpty();
    painter->setPen(showPlaceholder ? m_placeholderColor : m_textColor);

    QRectF textRect(6.0, 0, m_width - 12.0, m_height);
    QString displayStr = showPlaceholder ? m_placeholder : m_text;
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, displayStr);

    // 3. Optional blinking cursor when selected on canvas
    if (isSelected() && !m_readOnly) {
        QFontMetrics fm(font);
        int textW = showPlaceholder ? 0 : fm.horizontalAdvance(m_text);
        qreal cursorX = qMin(6.0 + textW, m_width - 8.0);
        qreal cursorY1 = m_height * 0.22;
        qreal cursorY2 = m_height * 0.78;

        painter->setPen(QPen(m_textColor, 1.5));
        painter->drawLine(QPointF(cursorX, cursorY1), QPointF(cursorX, cursorY2));
    }
}

QJsonObject TextInputComponent::toJson() const {
    QJsonObject json = UIComponent::toJson();
    json["text"] = m_text;
    json["placeholder"] = m_placeholder;
    json["textColor"] = m_textColor.name();
    json["placeholderColor"] = m_placeholderColor.name();
    json["backgroundColor"] = m_backgroundColor.name();
    json["borderColor"] = m_borderColor.name();
    json["borderWidth"] = m_borderWidth;
    json["cornerRadius"] = m_cornerRadius;
    json["pixelSize"] = m_pixelSize;
    json["readOnly"] = m_readOnly;
    json["onTextChanged"] = m_onTextChangedHandler;
    return json;
}

void TextInputComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);
    m_text = json.value("text").toString();
    m_placeholder = json.value("placeholder").toString("Enter text...");
    if (json.contains("textColor")) m_textColor = QColor(json.value("textColor").toString());
    if (json.contains("placeholderColor")) m_placeholderColor = QColor(json.value("placeholderColor").toString());
    if (json.contains("backgroundColor")) m_backgroundColor = QColor(json.value("backgroundColor").toString());
    if (json.contains("borderColor")) m_borderColor = QColor(json.value("borderColor").toString());
    m_borderWidth = json.value("borderWidth").toInt(1);
    m_cornerRadius = json.value("cornerRadius").toInt(4);
    m_pixelSize = json.value("pixelSize").toInt(13);
    m_readOnly = json.value("readOnly").toBool(false);
    m_onTextChangedHandler = json.value("onTextChanged").toString();
}

QString TextInputComponent::toQmlSnippet(int indentSpaces) const {
    QString ind(indentSpaces, ' ');
    QString qml;
    qml += ind + "Rectangle {\n";
    qml += ind + QString("    id: %1_box\n").arg(m_id);
    qml += ind + QString("    x: %1; y: %2; width: %3; height: %4\n")
        .arg(static_cast<int>(compX())).arg(static_cast<int>(compY()))
        .arg(static_cast<int>(m_width)).arg(static_cast<int>(m_height));
    qml += ind + QString("    color: \"%1\"\n").arg(m_backgroundColor.name());
    if (m_borderWidth > 0) {
        qml += ind + QString("    border.color: \"%1\"\n").arg(m_borderColor.name());
        qml += ind + QString("    border.width: %1\n").arg(m_borderWidth);
    }
    if (m_cornerRadius > 0) {
        qml += ind + QString("    radius: %1\n").arg(m_cornerRadius);
    }
    qml += ind + "    TextInput {\n";
    qml += ind + QString("        id: %1\n").arg(m_id);
    qml += ind + "        anchors.fill: parent\n";
    qml += ind + "        anchors.leftMargin: 6; anchors.rightMargin: 6\n";
    qml += ind + "        anchors.verticalCenter: parent.verticalCenter\n";
    qml += ind + QString("        text: \"%1\"\n").arg(m_text);
    qml += ind + QString("        color: \"%1\"\n").arg(m_textColor.name());
    qml += ind + QString("        font.pixelSize: %1\n").arg(m_pixelSize);
    if (m_readOnly) {
        qml += ind + "        readOnly: true\n";
    }
    if (!m_onTextChangedHandler.isEmpty()) {
        qml += ind + QString("        onTextChanged: root.%1(text)\n").arg(m_onTextChangedHandler);
    }
    qml += ind + "    }\n";
    qml += ind + "}\n";
    return qml;
}

QString TextInputComponent::toUgfxSnippet(int indentSpaces) const {
    QString ind(indentSpaces, ' ');
    QString code;
    code += ind + QString("// TextInput: %1\n").arg(m_id);
    code += ind + "gwinWidgetClearInit(&wi);\n";
    code += ind + "wi.g.show = gTrue;\n";
    code += ind + QString("wi.g.x = %1; wi.g.y = %2;\n").arg(static_cast<int>(compX())).arg(static_cast<int>(compY()));
    code += ind + QString("wi.g.width = %1; wi.g.height = %2;\n").arg(static_cast<int>(m_width)).arg(static_cast<int>(m_height));
    code += ind + QString("wi.text = \"%1\";\n").arg(m_text);
    code += ind + QString("ghEdit_%1 = gwinTexteditCreate(0, &wi, 100);\n").arg(m_id);
    code += ind + QString("gwinSetColor(ghEdit_%1, HTML2COLOR(0x%2));\n")
        .arg(m_id, m_textColor.name().mid(1).toUpper());
    code += ind + QString("gwinSetBgColor(ghEdit_%1, HTML2COLOR(0x%2));\n")
        .arg(m_id, m_backgroundColor.name().mid(1).toUpper());
    return code;
}
