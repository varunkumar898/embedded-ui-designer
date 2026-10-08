#include "TemperatureComponent.h"
#include <QPen>
#include <QBrush>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

TemperatureComponent::TemperatureComponent(const QString& id, QGraphicsItem* parent)
    : ValueVisualizationComponent(id, "Temperature", parent)
{
    m_width = 110;
    m_height = 140;
    m_value = 72.5;
    m_minimum = -20.0;
    m_maximum = 120.0;
    m_precision = 1;
    m_unit = "°C";
    m_warningThreshold = 90.0;
    m_criticalThreshold = 105.0;
    m_valueColor = QColor(16, 185, 129); // Emerald-500 (#10b981)
    m_trackColor = QColor(30, 41, 59);   // Slate-800 (#1e293b)
    m_borderColor = QColor(51, 65, 85);  // Slate-700 (#334155)
}

void TemperatureComponent::setDisplayStyle(const QString& style) {
    if (m_displayStyle != style) {
        m_displayStyle = style;
        update();
        emit propertyChanged(this);
    }
}

void TemperatureComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    if (m_displayStyle.compare("Dial", Qt::CaseInsensitive) == 0) {
        // Dial display style
        qreal side = std::min(m_width, m_height);
        qreal cx = m_width / 2.0;
        qreal cy = m_height / 2.0;
        qreal trackThick = std::clamp(side * 0.1, 6.0, 16.0);
        qreal trackRadius = side / 2.0 - trackThick / 2.0 - 4.0;
        QRectF arcRect(cx - trackRadius, cy - trackRadius, trackRadius * 2.0, trackRadius * 2.0);

        QPen trackPen(m_trackColor, trackThick, Qt::SolidLine, Qt::RoundCap);
        painter->setPen(trackPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawArc(arcRect, 225 * 16, -270 * 16);

        double norm = normalizedValue();
        int span = static_cast<int>(std::round(-270 * norm * 16));
        if (std::abs(span) > 0) {
            QPen valPen(effectiveValueColor(), trackThick, Qt::SolidLine, Qt::RoundCap);
            painter->setPen(valPen);
            painter->drawArc(arcRect, 225 * 16, span);
        }

        painter->setPen(m_textColor);
        QFont f = painter->font();
        f.setPixelSize(std::clamp(static_cast<int>(side * 0.16), 11, 22));
        f.setBold(true);
        painter->setFont(f);
        QRectF valRect(cx - side * 0.4, cy - side * 0.15, side * 0.8, side * 0.3);
        painter->drawText(valRect, Qt::AlignCenter, formattedValueWithUnit());
        return;
    }

    // Thermometer (Default) Style:
    // Left: vertical stem and bottom bulb
    // Right: numerical readout and thresholds
    qreal bulbRadius = std::clamp(m_width * 0.16, 10.0, 22.0);
    qreal stemWidth = bulbRadius * 0.8;
    qreal cx = bulbRadius + 8.0;
    qreal bottomBulbCy = m_height - bulbRadius - 6.0;
    qreal topStemCy = 14.0;
    qreal stemHeight = bottomBulbCy - topStemCy;

    QRectF stemTrackRect(cx - stemWidth/2.0, topStemCy, stemWidth, stemHeight);

    // 1. Thermometer Track Outline & Bulb
    QPainterPath thermoPath;
    thermoPath.addRoundedRect(stemTrackRect, stemWidth/2.0, stemWidth/2.0);
    thermoPath.addEllipse(QPointF(cx, bottomBulbCy), bulbRadius, bulbRadius);

    painter->setPen(QPen(m_borderColor, m_borderWidth));
    painter->setBrush(QBrush(m_trackColor));
    painter->drawPath(thermoPath);

    // 2. Liquid Fill (Bulb + Stem Column)
    double norm = normalizedValue();
    QColor fillCol = effectiveValueColor();

    painter->save();
    painter->setClipPath(thermoPath);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(fillCol));

    // Bulb is always filled
    painter->drawEllipse(QPointF(cx, bottomBulbCy), bulbRadius + 2.0, bulbRadius + 2.0);

    // Stem column fill up to height
    qreal fillH = stemHeight * norm;
    if (fillH > 0.0) {
        QRectF fillRect(cx - stemWidth/2.0, bottomBulbCy - fillH, stemWidth, fillH + bulbRadius);
        painter->drawRect(fillRect);
    }
    painter->restore();

    // 3. Right-side Readouts & Ticks
    // Title "TEMP"
    qreal rightLeft = cx + bulbRadius + 8.0;
    qreal rightWidth = m_width - rightLeft - 4.0;
    if (rightWidth > 20.0) {
        painter->setPen(QColor(148, 163, 184)); // Slate-400
        QFont titleFont = painter->font();
        titleFont.setPixelSize(std::clamp(static_cast<int>(m_height * 0.09), 8, 12));
        titleFont.setBold(true);
        titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.5);
        painter->setFont(titleFont);
        QRectF titleRect(rightLeft, topStemCy, rightWidth, 18.0);
        painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, "TEMP");

        // Large Numerical Value
        painter->setPen(m_textColor);
        QFont valFont = painter->font();
        valFont.setPixelSize(std::clamp(static_cast<int>(m_height * 0.16), 13, 24));
        valFont.setBold(true);
        painter->setFont(valFont);
        QRectF valRect(rightLeft, topStemCy + 20.0, rightWidth, 28.0);
        painter->drawText(valRect, Qt::AlignLeft | Qt::AlignVCenter, formattedValue());

        // Unit
        if (m_showUnit && !m_unit.isEmpty()) {
            QFont unitFont = painter->font();
            unitFont.setPixelSize(std::clamp(static_cast<int>(m_height * 0.11), 9, 14));
            unitFont.setBold(false);
            painter->setFont(unitFont);
            painter->setPen(QColor(148, 163, 184));
            QRectF unitRect(rightLeft, topStemCy + 48.0, rightWidth, 18.0);
            painter->drawText(unitRect, Qt::AlignLeft | Qt::AlignVCenter, m_unit);
        }

        // Min/Max indicator small labels
        QFont minMaxFont = painter->font();
        minMaxFont.setPixelSize(8);
        painter->setFont(minMaxFont);
        painter->setPen(QColor(100, 116, 139));
        QRectF maxRect(rightLeft, topStemCy - 2.0, rightWidth, 12.0);
        QRectF minRect(rightLeft, bottomBulbCy - 6.0, rightWidth, 12.0);
        // Ticks along the stem
        painter->drawLine(QPointF(cx + stemWidth/2.0 + 1.0, topStemCy + 2.0),
                         QPointF(cx + stemWidth/2.0 + 4.0, topStemCy + 2.0));
        painter->drawLine(QPointF(cx + stemWidth/2.0 + 1.0, bottomBulbCy - bulbRadius/2.0),
                         QPointF(cx + stemWidth/2.0 + 4.0, bottomBulbCy - bulbRadius/2.0));
    }
}

QJsonObject TemperatureComponent::toJson() const {
    QJsonObject obj = ValueVisualizationComponent::toJson();
    obj["displayStyle"] = m_displayStyle;
    return obj;
}

void TemperatureComponent::fromJson(const QJsonObject& json) {
    ValueVisualizationComponent::fromJson(json);

    if (json.contains("displayStyle")) m_displayStyle = json.value("displayStyle").toString("Thermometer");
    update();
}

QString TemperatureComponent::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString qml;
    qml += QString("%1// Temperature: %2\n").arg(indent, m_id);
    qml += QString("%1Item {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    width: %2; height: %3\n").arg(indent).arg(m_width).arg(m_height);
    qml += QString("%1    property real temperature: %2\n").arg(indent).arg(m_value, 0, 'f', m_precision);
    qml += QString("%1    property string unit: \"%2\"\n").arg(indent, m_unit);
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString TemperatureComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// Temperature: %2 (%3 %4)\n").arg(indent, m_id).arg(m_value).arg(m_unit);
    return code;
}
