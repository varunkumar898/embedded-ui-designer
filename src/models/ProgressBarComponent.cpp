#include "ProgressBarComponent.h"
#include <QPen>
#include <QBrush>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

ProgressBarComponent::ProgressBarComponent(const QString& id, QGraphicsItem* parent)
    : ValueVisualizationComponent(id, "ProgressBar", parent)
{
    m_width = 180;
    m_height = 20;
    m_value = 50.0;
    m_minimum = 0.0;
    m_maximum = 100.0;
    m_valueColor = QColor(16, 185, 129); // Modern Emerald Green (#10b981)
    m_trackColor = QColor(28, 32, 40);  // Dark rounded track (#1c2028)
    m_borderColor = QColor(45, 52, 65); // Subtle contrast border (#2d3441)
    m_borderWidth = 1;
    m_cornerRadius = 4;
}

void ProgressBarComponent::setValue(double val) {
    // If a raw ratio <= 1.0 is passed when maximum is > 1.0 (e.g. 0.75 when max is 100)
    // or direct engineering value (e.g. 75.0)
    if (val <= 1.0 && val >= 0.0 && m_maximum > 1.0 && val != m_minimum) {
        val = m_minimum + val * (m_maximum - m_minimum);
    }
    ValueVisualizationComponent::setValue(val);
}

void ProgressBarComponent::setOrientation(const QString& orient) {
    Qt::Orientation o = (orient.compare("Vertical", Qt::CaseInsensitive) == 0) ? Qt::Vertical : Qt::Horizontal;
    if (m_orientation != o) {
        m_orientation = o;
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

    QRectF trackRect(0, 0, m_width, m_height);

    // 1. Draw modern dark outer track
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(m_trackColor));
    painter->drawRoundedRect(trackRect, m_cornerRadius, m_cornerRadius);

    // 2. Draw smooth progress fill clipped to outer rounded track
    QPainterPath trackPath;
    trackPath.addRoundedRect(trackRect, m_cornerRadius, m_cornerRadius);

    painter->save();
    painter->setClipPath(trackPath);
    painter->setPen(Qt::NoPen);
    QColor effectiveBar = effectiveValueColor();
    painter->setBrush(QBrush(effectiveBar));

    double norm = normalizedValue();
    if (m_orientation == Qt::Horizontal) {
        qreal fillW = m_width * norm;
        if (fillW > 0.0) {
            painter->drawRect(QRectF(0, 0, fillW, m_height));
        }
    } else {
        qreal fillH = m_height * norm;
        if (fillH > 0.0) {
            painter->drawRect(QRectF(0, m_height - fillH, m_width, fillH));
        }
    }
    painter->restore();

    // 3. Draw subtle contrast border
    QColor effectiveBd = effectiveBorderColor(m_borderColor);
    int effectiveBw = effectiveBorderWidth(m_borderWidth);
    if (effectiveBw > 0 && effectiveBd.alpha() > 0) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(effectiveBd, effectiveBw));
        qreal inset = effectiveBw / 2.0;
        QRectF borderRect(inset, inset, m_width - effectiveBw, m_height - effectiveBw);
        qreal borderR = std::max<qreal>(0, m_cornerRadius - inset);
        painter->drawRoundedRect(borderRect, borderR, borderR);
    }

    // 4. Optional Text Overlay if enabled
    if (m_showValueText && m_height >= 14) {
        painter->setPen(m_textColor);
        QFont f = painter->font();
        f.setPixelSize(std::clamp(static_cast<int>(m_height * 0.6), 9, 14));
        f.setBold(true);
        painter->setFont(f);
        painter->drawText(trackRect, Qt::AlignCenter, formattedValueWithUnit());
    }
}

QJsonObject ProgressBarComponent::toJson() const {
    QJsonObject obj = ValueVisualizationComponent::toJson();
    obj["orientation"] = orientation();
    obj["cornerRadius"] = m_cornerRadius;
    // Backward compatibility keys
    obj["barColor"] = obj["valueColor"];
    obj["displayValue"] = m_value;
    return obj;
}

void ProgressBarComponent::fromJson(const QJsonObject& json) {
    ValueVisualizationComponent::fromJson(json);

    if (json.contains("orientation")) {
        setOrientation(json.value("orientation").toString("Horizontal"));
    }
    m_cornerRadius = json.value("cornerRadius").toInt(m_cornerRadius);
    update();
}

void ProgressBarComponent::applyColorStyle(const QString& styleName, const QColor& color) {
    ValueVisualizationComponent::applyColorStyle(styleName, color);
    if (colorStyleRef("barColor") == styleName || colorStyleRef("fillColor") == styleName) {
        setValueColor(color);
    }
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
    qml += QString("%1    from: %2\n").arg(indent).arg(m_minimum, 0, 'f', 1);
    qml += QString("%1    to: %2\n").arg(indent).arg(m_maximum, 0, 'f', 1);
    qml += QString("%1    value: %2\n").arg(indent).arg(m_value, 0, 'f', 1);
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
    code += QString("%1gwinProgressbarSetRange(prog_%2, %3, %4);\n").arg(indent, m_id).arg(static_cast<int>(m_minimum)).arg(static_cast<int>(m_maximum));
    code += QString("%1gwinProgressbarSetPosition(prog_%2, %3);\n").arg(indent, m_id).arg(static_cast<int>(m_value));
    code += QString("%1gwinSetVisible(prog_%2, gTrue);\n").arg(indent, m_id);
    return code;
}
