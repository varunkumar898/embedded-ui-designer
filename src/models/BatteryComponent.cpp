#include "BatteryComponent.h"
#include <QPen>
#include <QBrush>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

BatteryComponent::BatteryComponent(const QString& id, QGraphicsItem* parent)
    : ValueVisualizationComponent(id, "Battery", parent)
{
    m_width = 120;
    m_height = 48;
    m_value = 75.0;
    m_minimum = 0.0;
    m_maximum = 100.0;
    m_unit = "%";
    m_warningThreshold = 20.0;
    m_criticalThreshold = 10.0;
    m_valueColor = QColor(16, 185, 129); // Emerald-500 (#10b981)
    m_trackColor = QColor(15, 23, 42);   // Slate-900 (#0f172a)
    m_borderColor = QColor(51, 65, 85);  // Slate-700 (#334155)
    m_borderWidth = 2;
}

void BatteryComponent::setCharging(bool charging) {
    if (m_charging != charging) {
        m_charging = charging;
        if (m_charging) {
            setCurrentState("charging");
        } else {
            setCurrentState("normal");
        }
        update();
        emit propertyChanged(this);
    }
}

void BatteryComponent::setOrientation(const QString& orient) {
    Qt::Orientation o = (orient.compare("Vertical", Qt::CaseInsensitive) == 0) ? Qt::Vertical : Qt::Horizontal;
    if (m_orientation != o) {
        m_orientation = o;
        update();
        emit propertyChanged(this);
    }
}

void BatteryComponent::setShowPercentage(bool show) {
    if (m_showPercentage != show) {
        m_showPercentage = show;
        update();
        emit propertyChanged(this);
    }
}

void BatteryComponent::setSegmented(bool seg) {
    if (m_segmented != seg) {
        m_segmented = seg;
        update();
        emit propertyChanged(this);
    }
}

void BatteryComponent::setSegmentCount(int count) {
    count = std::clamp(count, 1, 20);
    if (m_segmentCount != count) {
        m_segmentCount = count;
        update();
        emit propertyChanged(this);
    }
}

QColor BatteryComponent::effectiveValueColor() const {
    QString state = m_currentState.toLower();
    if (state == "charging" || m_charging) {
        return m_chargingColor;
    }
    if (state == "error" || state == "critical") {
        return m_criticalColor;
    }
    if (state == "warning") {
        return m_warningColor;
    }
    if (state == "disabled") {
        return QColor(100, 116, 139);
    }

    // Battery thresholds are low-level (<= critical is red, <= warning is amber)
    if (m_value <= m_criticalThreshold) {
        return m_criticalColor;
    }
    if (m_value <= m_warningThreshold) {
        return m_warningColor;
    }

    return effectiveBackgroundColor(m_valueColor);
}

void BatteryComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    bool isH = (m_orientation == Qt::Horizontal);
    qreal terminalSize = isH ? std::clamp(m_width * 0.05, 4.0, 8.0) : std::clamp(m_height * 0.05, 4.0, 8.0);
    
    QRectF bodyRect;
    QRectF terminalRect;

    if (isH) {
        bodyRect = QRectF(m_borderWidth/2.0, m_borderWidth/2.0,
                          m_width - terminalSize - m_borderWidth, m_height - m_borderWidth);
        qreal termH = bodyRect.height() * 0.45;
        terminalRect = QRectF(bodyRect.right(), bodyRect.top() + (bodyRect.height() - termH)/2.0,
                              terminalSize - 1.0, termH);
    } else {
        qreal termW = (m_width - m_borderWidth) * 0.45;
        terminalRect = QRectF((m_width - termW)/2.0, m_borderWidth/2.0,
                              termW, terminalSize - 1.0);
        bodyRect = QRectF(m_borderWidth/2.0, terminalRect.bottom(),
                          m_width - m_borderWidth, m_height - terminalSize - m_borderWidth);
    }

    // 1. Draw Battery Body Track & Border
    painter->setPen(QPen(effectiveBorderColor(m_borderColor), m_borderWidth));
    painter->setBrush(QBrush(m_trackColor));
    painter->drawRoundedRect(bodyRect, 6.0, 6.0);

    // Terminal Nub
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(effectiveBorderColor(m_borderColor)));
    painter->drawRoundedRect(terminalRect, 2.0, 2.0);

    // 2. Draw Battery Fill / Segments
    QRectF innerRect = bodyRect.adjusted(3.0, 3.0, -3.0, -3.0);
    double norm = normalizedValue();
    QColor fillCol = effectiveValueColor();

    if (innerRect.width() > 0 && innerRect.height() > 0 && norm > 0.0) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(fillCol));

        if (m_segmented && m_segmentCount > 1) {
            int activeSegments = static_cast<int>(std::round(norm * m_segmentCount));
            qreal gap = 3.0;
            if (isH) {
                qreal segW = (innerRect.width() - (m_segmentCount - 1) * gap) / m_segmentCount;
                for (int i = 0; i < activeSegments; ++i) {
                    QRectF segRect(innerRect.left() + i * (segW + gap), innerRect.top(), segW, innerRect.height());
                    painter->drawRoundedRect(segRect, 2.0, 2.0);
                }
            } else {
                qreal segH = (innerRect.height() - (m_segmentCount - 1) * gap) / m_segmentCount;
                for (int i = 0; i < activeSegments; ++i) {
                    QRectF segRect(innerRect.left(), innerRect.bottom() - (i + 1) * segH - i * gap, innerRect.width(), segH);
                    painter->drawRoundedRect(segRect, 2.0, 2.0);
                }
            }
        } else {
            // Smooth continuous fill
            if (isH) {
                qreal fillW = innerRect.width() * norm;
                painter->drawRoundedRect(QRectF(innerRect.left(), innerRect.top(), fillW, innerRect.height()), 3.0, 3.0);
            } else {
                qreal fillH = innerRect.height() * norm;
                painter->drawRoundedRect(QRectF(innerRect.left(), innerRect.bottom() - fillH, innerRect.width(), fillH), 3.0, 3.0);
            }
        }
    }

    // 3. Charging Bolt Icon Overlay
    if (m_charging || m_currentState.toLower() == "charging") {
        QPointF center = bodyRect.center();
        qreal boltH = std::min(bodyRect.width(), bodyRect.height()) * 0.55;
        qreal boltW = boltH * 0.5;

        QPainterPath boltPath;
        boltPath.moveTo(center.x() + boltW * 0.1, center.y() - boltH * 0.5);
        boltPath.lineTo(center.x() - boltW * 0.5, center.y() + boltH * 0.05);
        boltPath.lineTo(center.x() - boltW * 0.05, center.y() + boltH * 0.05);
        boltPath.lineTo(center.x() - boltW * 0.2, center.y() + boltH * 0.5);
        boltPath.lineTo(center.x() + boltW * 0.5, center.y() - boltH * 0.05);
        boltPath.lineTo(center.x() + boltW * 0.05, center.y() - boltH * 0.05);
        boltPath.closeSubpath();

        painter->setPen(QPen(QColor(15, 23, 42), 1.0));
        painter->setBrush(QBrush(QColor(255, 255, 255, 230)));
        painter->drawPath(boltPath);
    }

    // 4. Percentage / Value Text
    if (m_showPercentage && bodyRect.height() >= 18 && !m_charging && m_currentState.toLower() != "charging") {
        painter->setPen(m_textColor);
        QFont f = painter->font();
        f.setPixelSize(std::clamp(static_cast<int>(bodyRect.height() * 0.42), 9, 16));
        f.setBold(true);
        painter->setFont(f);
        painter->drawText(bodyRect, Qt::AlignCenter, QString("%1%").arg(static_cast<int>(std::round(m_value))));
    }
}

QJsonObject BatteryComponent::toJson() const {
    QJsonObject obj = ValueVisualizationComponent::toJson();
    obj["charging"] = m_charging;
    obj["orientation"] = orientation();
    obj["showPercentage"] = m_showPercentage;
    obj["segmented"] = m_segmented;
    obj["segmentCount"] = m_segmentCount;
    return obj;
}

void BatteryComponent::fromJson(const QJsonObject& json) {
    ValueVisualizationComponent::fromJson(json);

    if (json.contains("charging")) m_charging = json.value("charging").toBool(false);
    if (json.contains("orientation")) setOrientation(json.value("orientation").toString("Horizontal"));
    if (json.contains("showPercentage")) m_showPercentage = json.value("showPercentage").toBool(true);
    if (json.contains("segmented")) m_segmented = json.value("segmented").toBool(true);
    if (json.contains("segmentCount")) m_segmentCount = json.value("segmentCount").toInt(5);
    update();
}

QString BatteryComponent::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString qml;
    qml += QString("%1// Battery: %2\n").arg(indent, m_id);
    qml += QString("%1Item {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    width: %2; height: %3\n").arg(indent).arg(m_width).arg(m_height);
    qml += QString("%1    property real batteryLevel: %2\n").arg(indent).arg(m_value, 0, 'f', 1);
    qml += QString("%1    property bool charging: %2\n").arg(indent, m_charging ? "true" : "false");
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString BatteryComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// Battery: %2 (%3%)\n").arg(indent, m_id).arg(m_value);
    return code;
}
