#include "PressureComponent.h"
#include <QPen>
#include <QBrush>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PressureComponent::PressureComponent(const QString& id, QGraphicsItem* parent)
    : ValueVisualizationComponent(id, "Pressure", parent)
{
    m_width = 140;
    m_height = 140;
    m_value = 2.4;
    m_minimum = 0.0;
    m_maximum = 10.0;
    m_precision = 1;
    m_unit = "bar";
    m_warningThreshold = 6.0;
    m_criticalThreshold = 8.0;
    m_valueColor = QColor(14, 165, 233); // Sky-500 (#0ea5e9)
    m_trackColor = QColor(30, 41, 59);   // Slate-800 (#1e293b)
}

void PressureComponent::setDisplayStyle(const QString& style) {
    if (m_displayStyle != style) {
        m_displayStyle = style;
        update();
        emit propertyChanged(this);
    }
}

void PressureComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    qreal side = std::min(m_width, m_height);
    qreal cx = m_width / 2.0;
    qreal cy = m_height / 2.0;

    int startAngle = 225;
    int sweepAngle = 270;
    qreal trackThick = std::clamp(side * 0.09, 6.0, 16.0);
    qreal trackRadius = side / 2.0 - trackThick / 2.0 - 4.0;
    if (trackRadius <= 0) return;

    QRectF trackArcRect(cx - trackRadius, cy - trackRadius, trackRadius * 2.0, trackRadius * 2.0);

    // 1. Background Arc Track
    QPen trackPen(m_trackColor, trackThick, Qt::SolidLine, Qt::RoundCap);
    painter->setPen(trackPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawArc(trackArcRect, startAngle * 16, -sweepAngle * 16);

    // 2. Warning / Critical Threshold Arcs
    double warnNorm = std::clamp((m_warningThreshold - m_minimum) / (m_maximum - m_minimum), 0.0, 1.0);
    double critNorm = std::clamp((m_criticalThreshold - m_minimum) / (m_maximum - m_minimum), 0.0, 1.0);

    if (critNorm > warnNorm) {
        int warnStartAngle = static_cast<int>(std::round((startAngle - warnNorm * sweepAngle) * 16));
        int warnSpan = static_cast<int>(std::round(-(critNorm - warnNorm) * sweepAngle * 16));
        QPen warnPen(m_warningColor, trackThick * 0.4, Qt::SolidLine, Qt::FlatCap);
        painter->setPen(warnPen);
        painter->drawArc(trackArcRect, warnStartAngle, warnSpan);
    }
    if (critNorm < 1.0) {
        int critStartAngle = static_cast<int>(std::round((startAngle - critNorm * sweepAngle) * 16));
        int critSpan = static_cast<int>(std::round(-(1.0 - critNorm) * sweepAngle * 16));
        QPen critPen(m_criticalColor, trackThick * 0.4, Qt::SolidLine, Qt::FlatCap);
        painter->setPen(critPen);
        painter->drawArc(trackArcRect, critStartAngle, critSpan);
    }

    // 3. Active Pressure Progress Arc
    double norm = normalizedValue();
    int progressSpan = static_cast<int>(std::round(-sweepAngle * norm * 16));
    if (std::abs(progressSpan) > 0) {
        QPen valPen(effectiveValueColor(), trackThick, Qt::SolidLine, Qt::RoundCap);
        painter->setPen(valPen);
        painter->drawArc(trackArcRect, startAngle * 16, progressSpan);
    }

    // 4. Center Label & Readout
    // Sub-title "PRESSURE"
    painter->setPen(QColor(148, 163, 184)); // Slate-400
    QFont labelFont = painter->font();
    labelFont.setPixelSize(std::clamp(static_cast<int>(side * 0.07), 8, 11));
    labelFont.setBold(true);
    labelFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    painter->setFont(labelFont);
    QRectF titleRect(cx - side * 0.4, cy - side * 0.22, side * 0.8, side * 0.16);
    painter->drawText(titleRect, Qt::AlignCenter, "PRESSURE");

    // Main Value Number
    painter->setPen(m_textColor);
    QFont valFont = painter->font();
    valFont.setPixelSize(std::clamp(static_cast<int>(side * 0.18), 12, 26));
    valFont.setBold(true);
    painter->setFont(valFont);
    QRectF valRect(cx - side * 0.4, cy - side * 0.06, side * 0.8, side * 0.26);
    painter->drawText(valRect, Qt::AlignCenter, formattedValue());

    // Unit "BAR" / "PSI"
    if (m_showUnit && !m_unit.isEmpty()) {
        QFont unitFont = painter->font();
        unitFont.setPixelSize(std::clamp(static_cast<int>(side * 0.08), 8, 12));
        unitFont.setBold(false);
        painter->setFont(unitFont);
        painter->setPen(QColor(148, 163, 184));
        QRectF unitRect(cx - side * 0.4, cy + side * 0.2, side * 0.8, side * 0.16);
        painter->drawText(unitRect, Qt::AlignCenter, m_unit.toUpper());
    }
}

QJsonObject PressureComponent::toJson() const {
    QJsonObject obj = ValueVisualizationComponent::toJson();
    obj["displayStyle"] = m_displayStyle;
    return obj;
}

void PressureComponent::fromJson(const QJsonObject& json) {
    ValueVisualizationComponent::fromJson(json);

    if (json.contains("displayStyle")) m_displayStyle = json.value("displayStyle").toString("Dial");
    update();
}

QString PressureComponent::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString qml;
    qml += QString("%1// Pressure: %2\n").arg(indent, m_id);
    qml += QString("%1Item {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    width: %2; height: %3\n").arg(indent).arg(m_width).arg(m_height);
    qml += QString("%1    property real pressure: %2\n").arg(indent).arg(m_value, 0, 'f', m_precision);
    qml += QString("%1    property string unit: \"%2\"\n").arg(indent, m_unit);
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString PressureComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// Pressure: %2 (%3 %4)\n").arg(indent, m_id).arg(m_value).arg(m_unit);
    return code;
}
