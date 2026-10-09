#include "CircularProgressComponent.h"
#include <QPen>
#include <QBrush>
#include <algorithm>
#include <cmath>

CircularProgressComponent::CircularProgressComponent(const QString& id, QGraphicsItem* parent)
    : ValueVisualizationComponent(id, "CircularProgress", parent)
{
    m_width = 120;
    m_height = 120;
    m_value = 72.0;
    m_minimum = 0.0;
    m_maximum = 100.0;
    m_unit = "%";
    m_valueColor = QColor(16, 185, 129); // Modern Emerald (#10b981)
    m_trackColor = QColor(28, 32, 40);   // Dark Slate (#1c2028)
}

void CircularProgressComponent::setThickness(int t) {
    t = std::clamp(t, 2, 40);
    if (m_thickness != t) {
        m_thickness = t;
        update();
        emit propertyChanged(this);
    }
}

void CircularProgressComponent::setStartAngle(int angle) {
    if (m_startAngle != angle) {
        m_startAngle = angle;
        update();
        emit propertyChanged(this);
    }
}

void CircularProgressComponent::setSweepAngle(int angle) {
    angle = std::clamp(angle, 10, 360);
    if (m_sweepAngle != angle) {
        m_sweepAngle = angle;
        update();
        emit propertyChanged(this);
    }
}

void CircularProgressComponent::setShowPercentage(bool show) {
    if (m_showPercentage != show) {
        m_showPercentage = show;
        update();
        emit propertyChanged(this);
    }
}

void CircularProgressComponent::setCenterLabel(const QString& label) {
    if (m_centerLabel != label) {
        m_centerLabel = label;
        update();
        emit propertyChanged(this);
    }
}

void CircularProgressComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    qreal side = std::min(m_width, m_height);
    qreal inset = m_thickness / 2.0 + 2.0;
    qreal diameter = side - inset * 2.0;
    if (diameter <= 0) return;

    qreal cx = m_width / 2.0;
    qreal cy = m_height / 2.0;
    QRectF arcRect(cx - diameter / 2.0, cy - diameter / 2.0, diameter, diameter);

    // 1. Outer / Background Track Arc
    QPen trackPen(m_trackColor, m_thickness, Qt::SolidLine, Qt::RoundCap);
    painter->setPen(trackPen);
    painter->setBrush(Qt::NoBrush);

    // Qt drawArc uses 1/16th of a degree and counter-clockwise direction
    int qtStartAngle = m_startAngle * 16;
    int qtTotalSpan = -m_sweepAngle * 16; // negative for clockwise
    painter->drawArc(arcRect, qtStartAngle, qtTotalSpan);

    // 2. Value Arc
    double norm = normalizedValue();
    int qtProgressSpan = static_cast<int>(std::round(-m_sweepAngle * norm * 16));

    if (std::abs(qtProgressSpan) > 0) {
        QPen progressPen(effectiveValueColor(), m_thickness, Qt::SolidLine, Qt::RoundCap);
        painter->setPen(progressPen);
        painter->drawArc(arcRect, qtStartAngle, qtProgressSpan);
    }

    // 3. Center Text Readout
    if (m_showValueText || m_showPercentage) {
        painter->setPen(m_textColor);
        QFont valFont = painter->font();
        int fontSize = std::clamp(static_cast<int>(side * 0.18), 10, 32);
        valFont.setPixelSize(fontSize);
        valFont.setBold(true);
        painter->setFont(valFont);

        QString mainText;
        if (m_showPercentage) {
            mainText = QString("%1%").arg(static_cast<int>(std::round(norm * 100.0)));
        } else {
            mainText = formattedValueWithUnit();
        }

        QRectF textRect(cx - diameter / 2.0 + m_thickness, cy - diameter / 2.0 + m_thickness,
                       diameter - m_thickness * 2, diameter - m_thickness * 2);

        if (!m_centerLabel.isEmpty()) {
            QRectF topHalf = textRect.adjusted(0, 0, 0, -textRect.height() * 0.25);
            painter->drawText(topHalf, Qt::AlignCenter, mainText);

            QFont lblFont = painter->font();
            lblFont.setPixelSize(std::clamp(static_cast<int>(fontSize * 0.55), 8, 14));
            lblFont.setBold(false);
            painter->setFont(lblFont);
            painter->setPen(QColor(148, 163, 184)); // Slate 400
            QRectF botHalf = textRect.adjusted(0, textRect.height() * 0.35, 0, 0);
            painter->drawText(botHalf, Qt::AlignCenter, m_centerLabel);
        } else {
            painter->drawText(textRect, Qt::AlignCenter, mainText);
        }
    }
}

QJsonObject CircularProgressComponent::toJson() const {
    QJsonObject obj = ValueVisualizationComponent::toJson();
    obj["thickness"] = m_thickness;
    obj["startAngle"] = m_startAngle;
    obj["sweepAngle"] = m_sweepAngle;
    obj["showPercentage"] = m_showPercentage;
    obj["centerLabel"] = m_centerLabel;
    return obj;
}

void CircularProgressComponent::fromJson(const QJsonObject& json) {
    ValueVisualizationComponent::fromJson(json);

    if (json.contains("thickness")) m_thickness = json.value("thickness").toInt(10);
    if (json.contains("startAngle")) m_startAngle = json.value("startAngle").toInt(90);
    if (json.contains("sweepAngle")) m_sweepAngle = json.value("sweepAngle").toInt(360);
    if (json.contains("showPercentage")) m_showPercentage = json.value("showPercentage").toBool(true);
    if (json.contains("centerLabel")) m_centerLabel = json.value("centerLabel").toString("");
    update();
}

QString CircularProgressComponent::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString qml;
    qml += QString("%1// CircularProgress: %2\n").arg(indent, m_id);
    qml += QString("%1Item {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    width: %2; height: %3\n").arg(indent).arg(m_width).arg(m_height);
    qml += QString("%1    property real value: %2\n").arg(indent).arg(m_value, 0, 'f', 1);
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString CircularProgressComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// CircularProgress: %2\n").arg(indent, m_id);
    code += QString("%1gdispDrawArc(%2, %3, %4, %5, %6, %7);\n")
        .arg(indent)
        .arg(static_cast<int>(pos().x() + m_width/2))
        .arg(static_cast<int>(pos().y() + m_height/2))
        .arg(static_cast<int>(std::min(m_width, m_height)/2 - m_thickness))
        .arg(m_startAngle)
        .arg(m_sweepAngle)
        .arg("HTML2COLOR(0x10B981)");
    return code;
}
