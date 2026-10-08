#include "ValueVisualizationComponent.h"
#include <algorithm>
#include <cmath>

ValueVisualizationComponent::ValueVisualizationComponent(const QString& id, const QString& compType, QGraphicsItem* parent)
    : UIComponent(id, compType, parent)
{
}

void ValueVisualizationComponent::setValue(double val) {
    val = std::clamp(val, m_minimum, m_maximum);
    if (m_value != val) {
        m_value = val;
        if (m_useThresholdStates) {
            evaluateThresholdState();
        }
        update();
        emit propertyChanged(this);
        emit interactionTriggered("On Value Changed");
    }
}

void ValueVisualizationComponent::setMinimum(double min) {
    if (m_minimum != min) {
        m_minimum = min;
        if (m_value < m_minimum) m_value = m_minimum;
        update();
        emit propertyChanged(this);
    }
}

void ValueVisualizationComponent::setMaximum(double max) {
    if (m_maximum != max) {
        m_maximum = max;
        if (m_value > m_maximum) m_value = m_maximum;
        update();
        emit propertyChanged(this);
    }
}

void ValueVisualizationComponent::setStep(double step) {
    if (step > 0 && m_step != step) {
        m_step = step;
        update();
        emit propertyChanged(this);
    }
}

void ValueVisualizationComponent::setUnit(const QString& unit) {
    if (m_unit != unit) {
        m_unit = unit;
        update();
        emit propertyChanged(this);
    }
}

void ValueVisualizationComponent::setPrecision(int prec) {
    prec = std::clamp(prec, 0, 6);
    if (m_precision != prec) {
        m_precision = prec;
        update();
        emit propertyChanged(this);
    }
}

double ValueVisualizationComponent::normalizedValue() const {
    if (m_maximum <= m_minimum) return 0.0;
    return std::clamp((m_value - m_minimum) / (m_maximum - m_minimum), 0.0, 1.0);
}

QString ValueVisualizationComponent::formattedValue() const {
    return QString::number(m_value, 'f', m_precision);
}

QString ValueVisualizationComponent::formattedValueWithUnit() const {
    if (m_showUnit && !m_unit.isEmpty()) {
        return QString("%1 %2").arg(formattedValue(), m_unit);
    }
    return formattedValue();
}

void ValueVisualizationComponent::setWarningThreshold(double thresh) {
    if (m_warningThreshold != thresh) {
        m_warningThreshold = thresh;
        if (m_useThresholdStates) evaluateThresholdState();
        update();
        emit propertyChanged(this);
    }
}

void ValueVisualizationComponent::setCriticalThreshold(double thresh) {
    if (m_criticalThreshold != thresh) {
        m_criticalThreshold = thresh;
        if (m_useThresholdStates) evaluateThresholdState();
        update();
        emit propertyChanged(this);
    }
}

void ValueVisualizationComponent::setUseThresholdStates(bool enable) {
    if (m_useThresholdStates != enable) {
        m_useThresholdStates = enable;
        if (m_useThresholdStates) evaluateThresholdState();
        update();
        emit propertyChanged(this);
    }
}

void ValueVisualizationComponent::setShowValueText(bool show) {
    if (m_showValueText != show) {
        m_showValueText = show;
        update();
        emit propertyChanged(this);
    }
}

void ValueVisualizationComponent::setShowUnit(bool show) {
    if (m_showUnit != show) {
        m_showUnit = show;
        update();
        emit propertyChanged(this);
    }
}

void ValueVisualizationComponent::setShowTicks(bool show) {
    if (m_showTicks != show) {
        m_showTicks = show;
        update();
        emit propertyChanged(this);
    }
}

void ValueVisualizationComponent::setShowLabels(bool show) {
    if (m_showLabels != show) {
        m_showLabels = show;
        update();
        emit propertyChanged(this);
    }
}

void ValueVisualizationComponent::setValueColor(const QColor& color) {
    if (m_valueColor != color) {
        m_valueColor = color;
        update();
        emit propertyChanged(this);
    }
}

void ValueVisualizationComponent::setTrackColor(const QColor& color) {
    if (m_trackColor != color) {
        m_trackColor = color;
        update();
        emit propertyChanged(this);
    }
}

void ValueVisualizationComponent::setBorderColor(const QColor& color) {
    if (m_borderColor != color) {
        m_borderColor = color;
        update();
        emit propertyChanged(this);
    }
}

void ValueVisualizationComponent::setBorderWidth(int width) {
    width = std::max(0, width);
    if (m_borderWidth != width) {
        m_borderWidth = width;
        update();
        emit propertyChanged(this);
    }
}

void ValueVisualizationComponent::setWarningColor(const QColor& color) {
    if (m_warningColor != color) {
        m_warningColor = color;
        update();
        emit propertyChanged(this);
    }
}

void ValueVisualizationComponent::setCriticalColor(const QColor& color) {
    if (m_criticalColor != color) {
        m_criticalColor = color;
        update();
        emit propertyChanged(this);
    }
}

void ValueVisualizationComponent::setTextColor(const QColor& color) {
    if (m_textColor != color) {
        m_textColor = color;
        update();
        emit propertyChanged(this);
    }
}

QColor ValueVisualizationComponent::effectiveValueColor() const {
    QString state = m_currentState.toLower();
    if (state == "error" || state == "critical") {
        return m_criticalColor;
    }
    if (state == "warning") {
        return m_warningColor;
    }
    if (state == "disabled") {
        return QColor(100, 116, 139);
    }

    // If threshold evaluation is active or values exceed thresholds
    if (m_value >= m_criticalThreshold && m_criticalThreshold > m_minimum) {
        return m_criticalColor;
    }
    if (m_value >= m_warningThreshold && m_warningThreshold > m_minimum) {
        return m_warningColor;
    }

    return effectiveBackgroundColor(m_valueColor);
}

void ValueVisualizationComponent::evaluateThresholdState() {
    if (m_value >= m_criticalThreshold && m_criticalThreshold > m_minimum) {
        setCurrentState("critical");
    } else if (m_value >= m_warningThreshold && m_warningThreshold > m_minimum) {
        setCurrentState("warning");
    } else {
        setCurrentState("normal");
    }
}

void ValueVisualizationComponent::applyBoundValue(const QVariant& val) {
    bool ok = false;
    double d = val.toDouble(&ok);
    if (ok) {
        setValue(d);
    }
}

QJsonObject ValueVisualizationComponent::toJson() const {
    QJsonObject obj = UIComponent::toJson();
    obj["value"] = m_value;
    obj["minimum"] = m_minimum;
    obj["maximum"] = m_maximum;
    obj["step"] = m_step;
    obj["unit"] = m_unit;
    obj["precision"] = m_precision;
    obj["warningThreshold"] = m_warningThreshold;
    obj["criticalThreshold"] = m_criticalThreshold;
    obj["useThresholdStates"] = m_useThresholdStates;
    obj["showValueText"] = m_showValueText;
    obj["showUnit"] = m_showUnit;
    obj["showTicks"] = m_showTicks;
    obj["showLabels"] = m_showLabels;

    obj["valueColor"] = serializeColor(m_valueColor, colorStyleRef("valueColor"));
    obj["trackColor"] = serializeColor(m_trackColor, colorStyleRef("trackColor"));
    obj["borderColor"] = serializeColor(m_borderColor, colorStyleRef("borderColor"));
    obj["warningColor"] = serializeColor(m_warningColor, colorStyleRef("warningColor"));
    obj["criticalColor"] = serializeColor(m_criticalColor, colorStyleRef("criticalColor"));
    obj["textColor"] = serializeColor(m_textColor, colorStyleRef("textColor"));
    obj["borderWidth"] = m_borderWidth;

    return obj;
}

void ValueVisualizationComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);

    if (json.contains("minimum")) m_minimum = json.value("minimum").toDouble(0.0);
    if (json.contains("maximum")) m_maximum = json.value("maximum").toDouble(100.0);
    if (json.contains("value")) m_value = json.value("value").toDouble(50.0);
    if (json.contains("displayValue")) m_value = json.value("displayValue").toDouble(m_value);
    if (json.contains("step")) m_step = json.value("step").toDouble(1.0);
    if (json.contains("unit")) m_unit = json.value("unit").toString("");
    if (json.contains("precision")) m_precision = json.value("precision").toInt(0);

    if (json.contains("warningThreshold")) m_warningThreshold = json.value("warningThreshold").toDouble(80.0);
    if (json.contains("criticalThreshold")) m_criticalThreshold = json.value("criticalThreshold").toDouble(95.0);
    if (json.contains("useThresholdStates")) m_useThresholdStates = json.value("useThresholdStates").toBool(false);

    if (json.contains("showValueText")) m_showValueText = json.value("showValueText").toBool(true);
    if (json.contains("showUnit")) m_showUnit = json.value("showUnit").toBool(true);
    if (json.contains("showTicks")) m_showTicks = json.value("showTicks").toBool(true);
    if (json.contains("showLabels")) m_showLabels = json.value("showLabels").toBool(true);

    if (json.contains("valueColor")) {
        QString ref;
        deserializeColor(json.value("valueColor"), m_valueColor, ref);
        setColorStyleRef("valueColor", ref);
    } else if (json.contains("barColor")) {
        QString ref;
        deserializeColor(json.value("barColor"), m_valueColor, ref);
        setColorStyleRef("valueColor", ref);
    }
    if (json.contains("trackColor")) {
        QString ref;
        deserializeColor(json.value("trackColor"), m_trackColor, ref);
        setColorStyleRef("trackColor", ref);
    }
    if (json.contains("borderColor")) {
        QString ref;
        deserializeColor(json.value("borderColor"), m_borderColor, ref);
        setColorStyleRef("borderColor", ref);
    }
    if (json.contains("warningColor")) {
        QString ref;
        deserializeColor(json.value("warningColor"), m_warningColor, ref);
        setColorStyleRef("warningColor", ref);
    }
    if (json.contains("criticalColor")) {
        QString ref;
        deserializeColor(json.value("criticalColor"), m_criticalColor, ref);
        setColorStyleRef("criticalColor", ref);
    }
    if (json.contains("textColor")) {
        QString ref;
        deserializeColor(json.value("textColor"), m_textColor, ref);
        setColorStyleRef("textColor", ref);
    }

    m_borderWidth = json.value("borderWidth").toInt(m_borderWidth);
    update();
}

void ValueVisualizationComponent::applyColorStyle(const QString& styleName, const QColor& color) {
    bool changed = false;
    if (colorStyleRef("valueColor") == styleName) {
        m_valueColor = color;
        changed = true;
    }
    if (colorStyleRef("trackColor") == styleName) {
        m_trackColor = color;
        changed = true;
    }
    if (colorStyleRef("borderColor") == styleName) {
        m_borderColor = color;
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
