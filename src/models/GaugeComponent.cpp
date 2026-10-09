#include "GaugeComponent.h"
#include <QPen>
#include <QBrush>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

GaugeComponent::GaugeComponent(const QString& id, QGraphicsItem* parent)
    : ValueVisualizationComponent(id, "Gauge", parent)
{
    m_width = 160;
    m_height = 160;
    m_value = 50.0;
    m_minimum = 0.0;
    m_maximum = 100.0;
    m_unit = "°C";
    m_valueColor = QColor(14, 165, 233); // Sky-500 (#0ea5e9)
    m_trackColor = QColor(30, 41, 59);   // Slate-800 (#1e293b)
    m_needleColor = QColor(239, 68, 68); // Red-500 (#ef4444)
}

void GaugeComponent::setStartAngle(int angle) {
    if (m_startAngle != angle) {
        m_startAngle = angle;
        update();
        emit propertyChanged(this);
    }
}

void GaugeComponent::setSweepAngle(int angle) {
    angle = std::clamp(angle, 30, 360);
    if (m_sweepAngle != angle) {
        m_sweepAngle = angle;
        update();
        emit propertyChanged(this);
    }
}

void GaugeComponent::setThickness(int t) {
    t = std::clamp(t, 2, 40);
    if (m_thickness != t) {
        m_thickness = t;
        update();
        emit propertyChanged(this);
    }
}

void GaugeComponent::setMajorTicks(int ticks) {
    ticks = std::clamp(ticks, 2, 20);
    if (m_majorTicks != ticks) {
        m_majorTicks = ticks;
        update();
        emit propertyChanged(this);
    }
}

void GaugeComponent::setMinorTicks(int ticks) {
    ticks = std::clamp(ticks, 0, 10);
    if (m_minorTicks != ticks) {
        m_minorTicks = ticks;
        update();
        emit propertyChanged(this);
    }
}

void GaugeComponent::setShowNeedle(bool show) {
    if (m_showNeedle != show) {
        m_showNeedle = show;
        update();
        emit propertyChanged(this);
    }
}

void GaugeComponent::setNeedleColor(const QColor& color) {
    if (m_needleColor != color) {
        m_needleColor = color;
        update();
        emit propertyChanged(this);
    }
}

void GaugeComponent::setGaugeStyle(const QString& style) {
    if (m_gaugeStyle != style) {
        m_gaugeStyle = style;
        if (style.compare("SemiCircle", Qt::CaseInsensitive) == 0) {
            m_startAngle = 180;
            m_sweepAngle = 180;
        } else if (style.compare("FullCircle", Qt::CaseInsensitive) == 0) {
            m_startAngle = 90;
            m_sweepAngle = 360;
        } else { // "Arc"
            m_startAngle = 225;
            m_sweepAngle = 270;
        }
        update();
        emit propertyChanged(this);
    }
}

void GaugeComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    qreal side = std::min(m_width, m_height);
    qreal cx = m_width / 2.0;
    qreal cy = m_height / 2.0;

    qreal outerRadius = side / 2.0 - 4.0;
    qreal innerRadius = outerRadius - m_thickness;
    if (innerRadius <= 0) return;

    QRectF trackArcRect(cx - (outerRadius - m_thickness/2.0),
                        cy - (outerRadius - m_thickness/2.0),
                        (outerRadius - m_thickness/2.0) * 2.0,
                        (outerRadius - m_thickness/2.0) * 2.0);

    // 1. Draw Background Track Arc
    QPen trackPen(m_trackColor, m_thickness, Qt::SolidLine, Qt::RoundCap);
    painter->setPen(trackPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawArc(trackArcRect, m_startAngle * 16, -m_sweepAngle * 16);

    // 2. Draw Active Value Arc
    double norm = normalizedValue();
    int progressSpan = static_cast<int>(std::round(-m_sweepAngle * norm * 16));
    if (std::abs(progressSpan) > 0) {
        QPen valPen(effectiveValueColor(), m_thickness, Qt::SolidLine, Qt::RoundCap);
        painter->setPen(valPen);
        painter->drawArc(trackArcRect, m_startAngle * 16, progressSpan);
    }

    // 3. Draw Ticks & Numeric Labels
    if (m_showTicks && m_majorTicks >= 2) {
        qreal tickOuterR = innerRadius - 2.0;
        qreal majorTickInnerR = tickOuterR - 8.0;
        qreal minorTickInnerR = tickOuterR - 4.0;

        int totalIntervals = (m_majorTicks - 1);
        int totalMinorSteps = totalIntervals * (m_minorTicks + 1);

        for (int i = 0; i <= totalMinorSteps; ++i) {
            bool isMajor = (i % (m_minorTicks + 1) == 0);
            double fraction = static_cast<double>(i) / totalMinorSteps;
            // Angle in degrees clockwise from startAngle
            double angleDeg = m_startAngle - fraction * m_sweepAngle;
            double angleRad = angleDeg * M_PI / 180.0;

            qreal tickInner = isMajor ? majorTickInnerR : minorTickInnerR;
            QPointF pOuter(cx + tickOuterR * std::cos(angleRad),
                           cy - tickOuterR * std::sin(angleRad));
            QPointF pInner(cx + tickInner * std::cos(angleRad),
                           cy - tickInner * std::sin(angleRad));

            QPen tickPen(isMajor ? QColor(203, 213, 225) : QColor(100, 116, 139),
                         isMajor ? 1.5 : 1.0);
            painter->setPen(tickPen);
            painter->drawLine(pInner, pOuter);

            // Draw label at major tick
            if (isMajor && m_showLabels && side >= 100) {
                double tickVal = m_minimum + fraction * (m_maximum - m_minimum);
                QString lblText = QString::number(tickVal, 'f', 0);

                qreal labelR = majorTickInnerR - 9.0;
                QPointF pLabel(cx + labelR * std::cos(angleRad),
                               cy - labelR * std::sin(angleRad));

                painter->setPen(QColor(148, 163, 184));
                QFont f = painter->font();
                f.setPixelSize(std::clamp(static_cast<int>(side * 0.065), 7, 11));
                painter->setFont(f);
                QRectF textBound(pLabel.x() - 15, pLabel.y() - 8, 30, 16);
                painter->drawText(textBound, Qt::AlignCenter, lblText);
            }
        }
    }

    // 4. Draw Needle
    if (m_showNeedle) {
        double currentAngleDeg = m_startAngle - norm * m_sweepAngle;
        double currentAngleRad = currentAngleDeg * M_PI / 180.0;
        double needleLength = innerRadius - 6.0;

        QPointF tip(cx + needleLength * std::cos(currentAngleRad),
                    cy - needleLength * std::sin(currentAngleRad));

        // Perpendicular for base of needle triangle
        double perpRad = currentAngleRad + M_PI / 2.0;
        qreal baseWidth = 3.5;
        QPointF b1(cx + baseWidth * std::cos(perpRad), cy - baseWidth * std::sin(perpRad));
        QPointF b2(cx - baseWidth * std::cos(perpRad), cy + baseWidth * std::sin(perpRad));
        QPointF tail(cx - (baseWidth * 2) * std::cos(currentAngleRad),
                     cy + (baseWidth * 2) * std::sin(currentAngleRad));

        QPainterPath needlePath;
        needlePath.moveTo(b1);
        needlePath.lineTo(tip);
        needlePath.lineTo(b2);
        needlePath.lineTo(tail);
        needlePath.closeSubpath();

        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(m_needleColor));
        painter->drawPath(needlePath);

        // Center hub / cap
        qreal capRadius = 6.0;
        painter->setBrush(QColor(30, 41, 59));
        painter->setPen(QPen(QColor(148, 163, 184), 1.5));
        painter->drawEllipse(QPointF(cx, cy), capRadius, capRadius);
    }

    // 5. Digital Value Readout at bottom
    if (m_showValueText) {
        painter->setPen(m_textColor);
        QFont f = painter->font();
        f.setPixelSize(std::clamp(static_cast<int>(side * 0.12), 10, 22));
        f.setBold(true);
        painter->setFont(f);

        QRectF valRect(cx - side * 0.35, cy + side * 0.12, side * 0.7, side * 0.3);
        painter->drawText(valRect, Qt::AlignCenter, formattedValueWithUnit());
    }
}

QJsonObject GaugeComponent::toJson() const {
    QJsonObject obj = ValueVisualizationComponent::toJson();
    obj["startAngle"] = m_startAngle;
    obj["sweepAngle"] = m_sweepAngle;
    obj["thickness"] = m_thickness;
    obj["majorTicks"] = m_majorTicks;
    obj["minorTicks"] = m_minorTicks;
    obj["showNeedle"] = m_showNeedle;
    obj["gaugeStyle"] = m_gaugeStyle;
    obj["needleColor"] = serializeColor(m_needleColor, colorStyleRef("needleColor"));
    return obj;
}

void GaugeComponent::fromJson(const QJsonObject& json) {
    ValueVisualizationComponent::fromJson(json);

    if (json.contains("startAngle")) m_startAngle = json.value("startAngle").toInt(225);
    if (json.contains("sweepAngle")) m_sweepAngle = json.value("sweepAngle").toInt(270);
    if (json.contains("thickness")) m_thickness = json.value("thickness").toInt(12);
    if (json.contains("majorTicks")) m_majorTicks = json.value("majorTicks").toInt(5);
    if (json.contains("minorTicks")) m_minorTicks = json.value("minorTicks").toInt(4);
    if (json.contains("showNeedle")) m_showNeedle = json.value("showNeedle").toBool(true);
    if (json.contains("gaugeStyle")) m_gaugeStyle = json.value("gaugeStyle").toString("Arc");

    if (json.contains("needleColor")) {
        QString ref;
        deserializeColor(json.value("needleColor"), m_needleColor, ref);
        setColorStyleRef("needleColor", ref);
    }
    update();
}

QString GaugeComponent::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString qml;
    qml += QString("%1// Gauge: %2\n").arg(indent, m_id);
    qml += QString("%1Item {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    width: %2; height: %3\n").arg(indent).arg(m_width).arg(m_height);
    qml += QString("%1    property real value: %2\n").arg(indent).arg(m_value, 0, 'f', 1);
    qml += QString("%1    property real min: %2\n").arg(indent).arg(m_minimum, 0, 'f', 1);
    qml += QString("%1    property real max: %2\n").arg(indent).arg(m_maximum, 0, 'f', 1);
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString GaugeComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// Gauge: %2\n").arg(indent, m_id);
    code += QString("%1// Value: %2 %3\n").arg(indent).arg(m_value).arg(m_unit);
    return code;
}
