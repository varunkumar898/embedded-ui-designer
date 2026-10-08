#include "ComponentState.h"

QString componentStateToString(ComponentStateEnum state) {
    switch (state) {
        case ComponentStateEnum::Normal:   return "normal";
        case ComponentStateEnum::Warning:  return "warning";
        case ComponentStateEnum::Error:    return "error";
        case ComponentStateEnum::Disabled: return "disabled";
        case ComponentStateEnum::Active:   return "active";
        case ComponentStateEnum::Focused:  return "focused";
        case ComponentStateEnum::Pressed:  return "pressed";
        case ComponentStateEnum::Selected: return "selected";
        case ComponentStateEnum::Loading:  return "loading";
        case ComponentStateEnum::Custom:   return "custom";
        default:                           return "normal";
    }
}

ComponentStateEnum stringToComponentState(const QString& str) {
    QString lower = str.trimmed().toLower();
    if (lower == "normal" || lower == "default") return ComponentStateEnum::Normal;
    if (lower == "warning" || lower == "warn") return ComponentStateEnum::Warning;
    if (lower == "error" || lower == "danger" || lower == "critical") return ComponentStateEnum::Error;
    if (lower == "disabled" || lower == "disable") return ComponentStateEnum::Disabled;
    if (lower == "active") return ComponentStateEnum::Active;
    if (lower == "focused" || lower == "focus") return ComponentStateEnum::Focused;
    if (lower == "pressed" || lower == "down") return ComponentStateEnum::Pressed;
    if (lower == "selected") return ComponentStateEnum::Selected;
    if (lower == "loading") return ComponentStateEnum::Loading;
    return ComponentStateEnum::Custom;
}

QJsonObject ComponentStateStyle::toJson() const {
    QJsonObject obj;
    if (hasBackgroundColor) obj["backgroundColor"] = backgroundColor.name(QColor::HexArgb);
    if (hasTextColor) obj["textColor"] = textColor.name(QColor::HexArgb);
    if (hasBorderColor) obj["borderColor"] = borderColor.name(QColor::HexArgb);
    if (hasBorderWidth) obj["borderWidth"] = borderWidth;
    if (hasOpacity) obj["opacity"] = opacity;
    if (hasVisible) obj["visible"] = visible;
    if (hasEnabled) obj["enabled"] = enabled;
    if (!customProperties.isEmpty()) {
        obj["custom"] = customProperties;
    }
    return obj;
}

ComponentStateStyle ComponentStateStyle::fromJson(const QJsonObject& json) {
    ComponentStateStyle style;
    if (json.contains("backgroundColor") || json.contains("fillColor") || json.contains("barColor")) {
        style.hasBackgroundColor = true;
        QString key = json.contains("backgroundColor") ? "backgroundColor" : (json.contains("fillColor") ? "fillColor" : "barColor");
        style.backgroundColor = QColor(json.value(key).toString());
    }
    if (json.contains("textColor") || json.contains("color")) {
        style.hasTextColor = true;
        QString key = json.contains("textColor") ? "textColor" : "color";
        style.textColor = QColor(json.value(key).toString());
    }
    if (json.contains("borderColor") || json.contains("strokeColor")) {
        style.hasBorderColor = true;
        QString key = json.contains("borderColor") ? "borderColor" : "strokeColor";
        style.borderColor = QColor(json.value(key).toString());
    }
    if (json.contains("borderWidth") || json.contains("strokeWidth")) {
        style.hasBorderWidth = true;
        QString key = json.contains("borderWidth") ? "borderWidth" : "strokeWidth";
        style.borderWidth = json.value(key).toInt();
    }
    if (json.contains("opacity")) {
        style.hasOpacity = true;
        style.opacity = json.value("opacity").toDouble();
    }
    if (json.contains("visible")) {
        style.hasVisible = true;
        style.visible = json.value("visible").toBool();
    }
    if (json.contains("enabled")) {
        style.hasEnabled = true;
        style.enabled = json.value("enabled").toBool();
    }
    if (json.contains("custom") && json.value("custom").isObject()) {
        style.customProperties = json.value("custom").toObject();
    }
    return style;
}

bool ComponentStateStyle::isEmpty() const {
    return !hasBackgroundColor && !hasTextColor && !hasBorderColor &&
           !hasBorderWidth && !hasOpacity && !hasVisible && !hasEnabled && customProperties.isEmpty();
}

bool ComponentStateStyle::operator==(const ComponentStateStyle& other) const {
    return hasBackgroundColor == other.hasBackgroundColor &&
           backgroundColor == other.backgroundColor &&
           hasTextColor == other.hasTextColor &&
           textColor == other.textColor &&
           hasBorderColor == other.hasBorderColor &&
           borderColor == other.borderColor &&
           hasBorderWidth == other.hasBorderWidth &&
           borderWidth == other.borderWidth &&
           hasOpacity == other.hasOpacity &&
           qFuzzyCompare(opacity, other.opacity) &&
           hasVisible == other.hasVisible &&
           visible == other.visible &&
           hasEnabled == other.hasEnabled &&
           enabled == other.enabled &&
           customProperties == other.customProperties;
}
