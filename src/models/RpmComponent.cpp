#include "RpmComponent.h"
#include <QPen>
#include <QBrush>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

RpmComponent::RpmComponent(const QString& id, QGraphicsItem* parent)
    : ValueVisualizationComponent(id, "RPM", parent)
{
    m_width = 170;
    m_height = 170;
    m_value = 3500.0;
    m_minimum = 0.0;
    m_maximum = 8000.0;
    m_unit = "RPM";
    m_warningThreshold = 6000.0;
    m_criticalThreshold = 7000.0;
    m_valueColor = QColor(245, 158, 11); // Amber-500 (#f59e0b)
    m_trackColor = QColor(15, 23, 42);   // Slate-900 (#0f172a)
    m_needleColor = QColor(239, 68, 68); // Red-500
}

void RpmComponent::setMajorTicks(int ticks) {
    ticks = std::clamp(ticks, 2, 20);
    if (m_majorTicks != ticks) {
        m_majorTicks = ticks;
        update();
        emit propertyChanged(this);
    }
}

void RpmComponent::setMinorTicks(int ticks) {
    ticks = std::clamp(ticks, 0, 10);
    if (m_minorTicks != ticks) {
        m_minorTicks = ticks;
        update();
        emit propertyChanged(this);
    }
}

void RpmComponent::setShowNeedle(bool show) {
    if (m_showNeedle != show) {
        m_showNeedle = show;
        update();
        emit propertyChanged(this);
    }
}

void RpmComponent::setNeedleColor(const QColor& color) {
    if (m_needleColor != color) {
        m_needleColor = color;
        update();
        emit propertyChanged(this);
    }
}

void RpmComponent::setPresentation(const QString& pres) {
    if (m_presentation != pres) {
        m_presentation = pres;
        update();
        emit propertyChanged(this);
    }
}

void RpmComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    qreal side = std::min(m_width, m_height);
    qreal cx = m_width / 2.0;
    qreal cy = m_height / 2.0;

    qreal outerRadius = side / 2.0 - 4.0;
    qreal trackThick = std::clamp(side * 0.08, 6.0, 16.0);
    qreal trackRadius = outerRadius - trackThick / 2.0;

    QRectF trackArcRect(cx - trackRadius, cy - trackRadius, trackRadius * 2.0, trackRadius * 2.0);

    // 1. Dial Face
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(QColor(15, 23, 42, 220)));
    painter->drawEllipse(QPointF(cx, cy), outerRadius, outerRadius);

    painter->setPen(QPen(QColor(51, 65, 85), 1.5));
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(QPointF(cx, cy), outerRadius, outerRadius);

    // 2. Track Arc
    QPen trackPen(m_trackColor, trackThick, Qt::SolidLine, Qt::FlatCap);
    painter->setPen(trackPen);
    painter->drawArc(trackArcRect, m_startAngle * 16, -m_sweepAngle * 16);

    // Redline & Warning zones
    double warnNorm = std::clamp((m_warningThreshold - m_minimum) / (m_maximum - m_minimum), 0.0, 1.0);
    double critNorm = std::clamp((m_criticalThreshold - m_minimum) / (m_maximum - m_minimum), 0.0, 1.0);

    if (critNorm > warnNorm) {
        int warnStartAngle = static_cast<int>(std::round((m_startAngle - warnNorm * m_sweepAngle) * 16));
        int warnSpan = static_cast<int>(std::round(-(critNorm - warnNorm) * m_sweepAngle * 16));
        QPen warnPen(m_warningColor, trackThick * 0.5, Qt::SolidLine, Qt::FlatCap);
        painter->setPen(warnPen);
        painter->drawArc(trackArcRect, warnStartAngle, warnSpan);
    }
    if (critNorm < 1.0) {
        int critStartAngle = static_cast<int>(std::round((m_startAngle - critNorm * m_sweepAngle) * 16));
        int critSpan = static_cast<int>(std::round(-(1.0 - critNorm) * m_sweepAngle * 16));
        QPen critPen(m_criticalColor, trackThick * 0.5, Qt::SolidLine, Qt::FlatCap);
        painter->setPen(critPen);
        painter->drawArc(trackArcRect, critStartAngle, critSpan);
    }

    // 3. Active RPM Arc
    double norm = normalizedValue();
    int rpmSpan = static_cast<int>(std::round(-m_sweepAngle * norm * 16));
    if (std::abs(rpmSpan) > 0) {
        QPen valPen(effectiveValueColor(), trackThick, Qt::SolidLine, Qt::RoundCap);
        painter->setPen(valPen);
        painter->drawArc(trackArcRect, m_startAngle * 16, rpmSpan);
    }

    // 4. Ticks (0..8)
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

            if (isMajor && m_showLabels && side >= 120) {
                // If maximum is around 8000, label as 0, 1, 2, ... 8
                double tickVal = m_minimum + fraction * (m_maximum - m_minimum);
                QString lblText = (m_maximum >= 1000.0) ? QString::number(tickVal / 1000.0, 'f', 0) : QString::number(tickVal, 'f', 0);

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

    // 5. Needle
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

    // 6. Digital RPM Text & Subtitle
    if (m_showValueText) {
        painter->setPen(m_textColor);
        QFont f = painter->font();
        f.setPixelSize(std::clamp(static_cast<int>(side * 0.14), 11, 24));
        f.setBold(true);
        painter->setFont(f);

        QRectF valRect(cx - side * 0.35, cy + side * 0.1, side * 0.7, side * 0.2);
        painter->drawText(valRect, Qt::AlignCenter, QString::number(static_cast<int>(std::round(m_value))));

        if (m_showUnit && !m_unit.isEmpty()) {
            QFont uf = painter->font();
            uf.setPixelSize(std::clamp(static_cast<int>(side * 0.06), 7, 10));
            uf.setBold(false);
            painter->setFont(uf);
            painter->setPen(QColor(148, 163, 184));
            QRectF unitRect(cx - side * 0.35, cy + side * 0.27, side * 0.7, side * 0.14);
            painter->drawText(unitRect, Qt::AlignCenter, m_unit);
        }
    }
}

QJsonObject RpmComponent::toJson() const {
    QJsonObject obj = ValueVisualizationComponent::toJson();
    obj["majorTicks"] = m_majorTicks;
    obj["minorTicks"] = m_minorTicks;
    obj["showNeedle"] = m_showNeedle;
    obj["presentation"] = m_presentation;
    obj["needleColor"] = serializeColor(m_needleColor, colorStyleRef("needleColor"));
    return obj;
}

void RpmComponent::fromJson(const QJsonObject& json) {
    ValueVisualizationComponent::fromJson(json);

    if (json.contains("majorTicks")) m_majorTicks = json.value("majorTicks").toInt(9);
    if (json.contains("minorTicks")) m_minorTicks = json.value("minorTicks").toInt(3);
    if (json.contains("showNeedle")) m_showNeedle = json.value("showNeedle").toBool(true);
    if (json.contains("presentation")) m_presentation = json.value("presentation").toString("Gauge");

    if (json.contains("needleColor")) {
        QString ref;
        deserializeColor(json.value("needleColor"), m_needleColor, ref);
        setColorStyleRef("needleColor", ref);
    }
    update();
}

QString RpmComponent::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString qml;
    qml += QString("%1// RPM: %2\n").arg(indent, m_id);
    qml += QString("%1Item {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    width: %2; height: %3\n").arg(indent).arg(m_width).arg(m_height);
    qml += QString("%1    property real rpm: %2\n").arg(indent).arg(m_value, 0, 'f', 0);
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString RpmComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// RPM: %2 (%3 RPM)\n").arg(indent, m_id).arg(m_value);
    return code;
}
