#include "ButtonComponent.h"
#include <QPen>
#include <QBrush>
#include <QFontMetrics>

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

    // Button body
    painter->setBrush(QBrush(m_backgroundColor));
    painter->setPen(QPen(m_backgroundColor.darker(115), 1.0));
    painter->drawRoundedRect(QRectF(0, 0, m_width, m_height), m_cornerRadius, m_cornerRadius);

    // Button text
    painter->setPen(QPen(m_textColor));
    QFont font = painter->font();
    font.setPixelSize(14);
    font.setBold(true);
    painter->setFont(font);
    painter->drawText(QRectF(0, 0, m_width, m_height), Qt::AlignCenter, m_text);
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
    if (changed) {
        update();
        emit propertyChanged(this);
    }
}

QJsonObject ButtonComponent::toJson() const {
    QJsonObject obj = UIComponent::toJson();
    obj["text"] = m_text;
    obj["backgroundColor"] = serializeColor(m_backgroundColor, colorStyleRef("backgroundColor"));
    obj["textColor"] = serializeColor(m_textColor, colorStyleRef("textColor"));
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
    m_cornerRadius = json.value("cornerRadius").toInt(m_cornerRadius);
    m_onClickedHandler = json.value("onClicked").toString(m_onClickedHandler);
    m_targetScreenId = json.value("targetScreenId").toString();
    update();
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
