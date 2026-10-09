#include "SpeedometerComponent.h"
#include <QPen>
#include <QBrush>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SpeedometerComponent::SpeedometerComponent(const QString& id, QGraphicsItem* parent)
    : ValueVisualizationComponent(id, "Speedometer", parent)
{
    m_width = 180;
    m_height = 180;
    m_value = 80.0;
    m_minimum = 0.0;
    m_maximum = 240.0;
    m_unit = "km/h";
    m_warningThreshold = 140.0;
    m_criticalThreshold = 200.0;
    m_valueColor = QColor(56, 189, 248);  // Cyan-400 (#38bdf8)
    m_trackColor = QColor(15, 23, 42);    // Slate-900 (#0f172a)
    m_needleColor = QColor(239, 68, 68);  // Red-500
}

void SpeedometerComponent::setMajorTicks(int ticks) {
    ticks = std::clamp(ticks, 2, 20);
    if (m_majorTicks != ticks) {
        m_majorTicks = ticks;
        update();
        emit propertyChanged(this);
    }
}

void SpeedometerComponent::setMinorTicks(int ticks) {
    ticks = std::clamp(ticks, 0, 10);
    if (m_minorTicks != ticks) {
        m_minorTicks = ticks;
        update();
        emit propertyChanged(this);
    }
}

void SpeedometerComponent::setShowNeedle(bool show) {
    if (m_showNeedle != show) {
        m_showNeedle = show;
        update();
        emit propertyChanged(this);
    }
}

void SpeedometerComponent::setNeedleColor(const QColor& color) {
    if (m_needleColor != color) {
        m_needleColor = color;
        update();
        emit propertyChanged(this);
    }
}

void SpeedometerComponent::setStylePreset(const QString& preset) {
    if (m_stylePreset != preset) {
        m_stylePreset = preset;
        if (preset.compare("Classic", Qt::CaseInsensitive) == 0) {
            m_valueColor = QColor(248, 250, 252);
            m_trackColor = QColor(30, 41, 59);
        } else if (preset.compare("Compact", Qt::CaseInsensitive) == 0) {
            m_showNeedle = false;
        } else {
            m_valueColor = QColor(56, 189, 248);
            m_trackColor = QColor(15, 23, 42);
            m_showNeedle = true;
        }
        update();
        emit propertyChanged(this);
    }
}

void SpeedometerComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    qreal side = std::min(m_width, m_height);
    qreal cx = m_width / 2.0;
    qreal cy = m_height / 2.0;

    qreal outerRadius = side / 2.0 - 4.0;
    qreal trackThick = std::clamp(side * 0.08, 6.0, 18.0);
    qreal trackRadius = outerRadius - trackThick / 2.0;

    QRectF trackArcRect(cx - trackRadius, cy - trackRadius, trackRadius * 2.0, trackRadius * 2.0);

    // 1. Dark Gauge Dial Background
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(QColor(15, 23, 42, 220))); // Slate 900
    painter->drawEllipse(QPointF(cx, cy), outerRadius, outerRadius);

    // Subtle Outer Bezel
    painter->setPen(QPen(QColor(51, 65, 85), 1.5));
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(QPointF(cx, cy), outerRadius, outerRadius);

    // 2. Base Track Arc
    QPen trackPen(m_trackColor, trackThick, Qt::SolidLine, Qt::FlatCap);
    painter->setPen(trackPen);
    painter->drawArc(trackArcRect, m_startAngle * 16, -m_sweepAngle * 16);

    // 3. Colored Threshold Zones (Normal, Warning, Redline/Critical)
    double warnNorm = std::clamp((m_warningThreshold - m_minimum) / (m_maximum - m_minimum), 0.0, 1.0);
    double critNorm = std::clamp((m_criticalThreshold - m_minimum) / (m_maximum - m_minimum), 0.0, 1.0);

    // Warning Arc section (e.g. from warnNorm to critNorm)
    if (critNorm > warnNorm) {
        int warnStartAngle = static_cast<int>(std::round((m_startAngle - warnNorm * m_sweepAngle) * 16));
        int warnSpan = static_cast<int>(std::round(-(critNorm - warnNorm) * m_sweepAngle * 16));
        QPen warnPen(m_warningColor, trackThick * 0.5, Qt::SolidLine, Qt::FlatCap);
        painter->setPen(warnPen);
        painter->drawArc(trackArcRect, warnStartAngle, warnSpan);
    }
    // Redline Arc section (from critNorm to 1.0)
    if (critNorm < 1.0) {
        int critStartAngle = static_cast<int>(std::round((m_startAngle - critNorm * m_sweepAngle) * 16));
        int critSpan = static_cast<int>(std::round(-(1.0 - critNorm) * m_sweepAngle * 16));
        QPen critPen(m_criticalColor, trackThick * 0.5, Qt::SolidLine, Qt::FlatCap);
        painter->setPen(critPen);
        painter->drawArc(trackArcRect, critStartAngle, critSpan);
    }

    // 4. Active Speed Progress Arc
    double norm = normalizedValue();
    int speedSpan = static_cast<int>(std::round(-m_sweepAngle * norm * 16));
    if (std::abs(speedSpan) > 0) {
        QPen valPen(effectiveValueColor(), trackThick, Qt::SolidLine, Qt::RoundCap);
        painter->setPen(valPen);
        painter->drawArc(trackArcRect, m_startAngle * 16, speedSpan);
    }

    // 5. Ticks & Speed Numbers
    if (m_showTicks && m_majorTicks >= 2) {
        qreal tickOuterR = trackRadius - trackThick / 2.0 - 2.0;
        qreal majorTickInnerR = tickOuterR - 7.0;
        qreal minorTickInnerR = tickOuterR - 3.5;

        int totalIntervals = (m_majorTicks - 1);
        int totalMinorSteps = totalIntervals * (m_minorTicks + 1);

        for (int i = 0; i <= totalMinorSteps; ++i) {
            bool isMajor = (i % (m_minorTicks + 1) == 0);
            double fraction = static_cast<double>(i) / totalMinorSteps;
            double angleDeg = m_startAngle - fraction * m_sweepAngle;
            double angleRad = angleDeg * M_PI / 180.0;

            qreal tickInner = isMajor ? majorTickInnerR : minorTickInnerR;
            QPointF pOuter(cx + tickOuterR * std::cos(angleRad),
                           cy - tickOuterR * std::sin(angleRad));
            QPointF pInner(cx + tickInner * std::cos(angleRad),
                           cy - tickInner * std::sin(angleRad));

            QPen tickPen(isMajor ? QColor(226, 232, 240) : QColor(100, 116, 139),
                         isMajor ? 1.5 : 1.0);
            painter->setPen(tickPen);
            painter->drawLine(pInner, pOuter);

            // Major Tick Number (e.g. 0, 30, 60...)
            if (isMajor && m_showLabels && side >= 120) {
                double tickVal = m_minimum + fraction * (m_maximum - m_minimum);
                QString lblText = QString::number(tickVal, 'f', 0);

                qreal labelR = majorTickInnerR - 10.0;
                QPointF pLabel(cx + labelR * std::cos(angleRad),
                               cy - labelR * std::sin(angleRad));

                painter->setPen(fraction >= critNorm ? m_criticalColor : QColor(203, 213, 225));
                QFont f = painter->font();
                f.setPixelSize(std::clamp(static_cast<int>(side * 0.055), 7, 10));
                painter->setFont(f);
                QRectF textBound(pLabel.x() - 14, pLabel.y() - 7, 28, 14);
                painter->drawText(textBound, Qt::AlignCenter, lblText);
            }
        }
    }

    // 6. Needle
    if (m_showNeedle) {
        double currentAngleDeg = m_startAngle - norm * m_sweepAngle;
        double currentAngleRad = currentAngleDeg * M_PI / 180.0;
        double needleLength = trackRadius - trackThick / 2.0 - 10.0;

        QPointF tip(cx + needleLength * std::cos(currentAngleRad),
                    cy - needleLength * std::sin(currentAngleRad));

        double perpRad = currentAngleRad + M_PI / 2.0;
        qreal baseWidth = 3.5;
        QPointF b1(cx + baseWidth * std::cos(perpRad), cy - baseWidth * std::sin(perpRad));
        QPointF b2(cx - baseWidth * std::cos(perpRad), cy + baseWidth * std::sin(perpRad));
        QPointF tail(cx - 7.0 * std::cos(currentAngleRad), cy + 7.0 * std::sin(currentAngleRad));

        QPainterPath needlePath;
        needlePath.moveTo(b1);
        needlePath.lineTo(tip);
        needlePath.lineTo(b2);
        needlePath.lineTo(tail);
        needlePath.closeSubpath();

        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(m_needleColor));
        painter->drawPath(needlePath);

        // Center Cap
        qreal capRadius = 7.0;
        painter->setBrush(QColor(15, 23, 42));
        painter->setPen(QPen(QColor(148, 163, 184), 1.5));
        painter->drawEllipse(QPointF(cx, cy), capRadius, capRadius);
    }

    // 7. Center Digital Speed Readout + Unit
    if (m_showValueText) {
        painter->setPen(m_textColor);
        QFont f = painter->font();
        f.setPixelSize(std::clamp(static_cast<int>(side * 0.16), 12, 28));
        f.setBold(true);
        painter->setFont(f);

        QRectF valRect(cx - side * 0.35, cy + side * 0.1, side * 0.7, side * 0.22);
        painter->drawText(valRect, Qt::AlignCenter, formattedValue());

        if (m_showUnit && !m_unit.isEmpty()) {
            QFont uf = painter->font();
            uf.setPixelSize(std::clamp(static_cast<int>(side * 0.065), 7, 12));
            uf.setBold(false);
            painter->setFont(uf);
            painter->setPen(QColor(148, 163, 184)); // Slate-400
            QRectF unitRect(cx - side * 0.35, cy + side * 0.28, side * 0.7, side * 0.14);
            painter->drawText(unitRect, Qt::AlignCenter, m_unit.toUpper());
        }
    }
}

QJsonObject SpeedometerComponent::toJson() const {
    QJsonObject obj = ValueVisualizationComponent::toJson();
    obj["majorTicks"] = m_majorTicks;
    obj["minorTicks"] = m_minorTicks;
    obj["showNeedle"] = m_showNeedle;
    obj["stylePreset"] = m_stylePreset;
    obj["needleColor"] = serializeColor(m_needleColor, colorStyleRef("needleColor"));
    return obj;
}

void SpeedometerComponent::fromJson(const QJsonObject& json) {
    ValueVisualizationComponent::fromJson(json);

    if (json.contains("majorTicks")) m_majorTicks = json.value("majorTicks").toInt(9);
    if (json.contains("minorTicks")) m_minorTicks = json.value("minorTicks").toInt(2);
    if (json.contains("showNeedle")) m_showNeedle = json.value("showNeedle").toBool(true);
    if (json.contains("stylePreset")) m_stylePreset = json.value("stylePreset").toString("Modern");

    if (json.contains("needleColor")) {
        QString ref;
        deserializeColor(json.value("needleColor"), m_needleColor, ref);
        setColorStyleRef("needleColor", ref);
    }
    update();
}

QString SpeedometerComponent::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString qml;
    qml += QString("%1// Speedometer: %2\n").arg(indent, m_id);
    qml += QString("%1Item {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    width: %2; height: %3\n").arg(indent).arg(m_width).arg(m_height);
    qml += QString("%1    property real speed: %2\n").arg(indent).arg(m_value, 0, 'f', 1);
    qml += QString("%1    property string unit: \"%2\"\n").arg(indent, m_unit);
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString SpeedometerComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// Speedometer: %2\n").arg(indent, m_id);
    code += QString("%1// Speed: %2 %3\n").arg(indent).arg(m_value).arg(m_unit);
    return code;
}
