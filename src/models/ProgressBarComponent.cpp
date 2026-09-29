#include "ProgressBarComponent.h"
#include <QPen>
#include <QBrush>
#include <algorithm>

ProgressBarComponent::ProgressBarComponent(const QString& id, QGraphicsItem* parent)
    : UIComponent(id, "ProgressBar", parent)
{
    m_width = 160;
    m_height = 20;
}

void ProgressBarComponent::setValue(double val) {
    val = std::clamp(val, 0.0, 1.0);
    if (m_value != val) {
        m_value = val;
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

    // Track
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(m_trackColor));
    painter->drawRoundedRect(QRectF(0, 0, m_width, m_height), m_cornerRadius, m_cornerRadius);

    // Filled bar
    qreal fillWidth = m_width * m_value;
    if (fillWidth > 0.0) {
        painter->setBrush(QBrush(m_barColor));
        painter->drawRoundedRect(QRectF(0, 0, fillWidth, m_height), m_cornerRadius, m_cornerRadius);
    }
}

QJsonObject ProgressBarComponent::toJson() const {
    QJsonObject obj = UIComponent::toJson();
    obj["value"] = m_value;
    obj["barColor"] = m_barColor.name();
    obj["trackColor"] = m_trackColor.name();
    obj["cornerRadius"] = m_cornerRadius;
    return obj;
}

void ProgressBarComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);
    m_value = json.value("value").toDouble(m_value);
    if (json.contains("barColor")) {
        m_barColor = QColor(json.value("barColor").toString());
    }
    if (json.contains("trackColor")) {
        m_trackColor = QColor(json.value("trackColor").toString());
    }
    m_cornerRadius = json.value("cornerRadius").toInt(m_cornerRadius);
    update();
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
    qml += QString("%1    value: %2\n").arg(indent).arg(m_value, 0, 'f', 2);
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
    code += QString("%1gwinProgressbarSetPosition(prog_%2, %3);\n").arg(indent, m_id).arg(static_cast<int>(m_value * 100));
    code += QString("%1gwinSetVisible(prog_%2, gTrue);\n").arg(indent, m_id);
    return code;
}
