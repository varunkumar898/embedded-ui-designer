#include "IRSerialization.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>

namespace IR {

// ============================================================================
// Color Helpers
// ============================================================================

QString IRSerialization::colorToHex(const QColor& color) {
    if (!color.isValid()) return "#00000000";
    if (color.alpha() == 255) {
        return color.name(QColor::HexRgb).toLower();
    }
    return color.name(QColor::HexArgb).toLower();
}

QColor IRSerialization::colorFromHex(const QString& hex) {
    if (hex.isEmpty()) return QColor();
    QString trimmed = hex.trimmed().toLower();
    if (trimmed == "transparent") return QColor(0, 0, 0, 0);
    if (trimmed == "black") return QColor(0, 0, 0);
    if (trimmed == "white") return QColor(255, 255, 255);

    QColor c = QColor::fromString(trimmed);
    if (c.isValid()) return c;
    return QColor(trimmed);
}

// ============================================================================
// Enum String Conversions
// ============================================================================

QString IRSerialization::layoutTypeToString(IRLayoutType type) {
    switch (type) {
        case IRLayoutType::Absolute: return "absolute";
        case IRLayoutType::Flex:     return "flexbox";
        case IRLayoutType::Grid:     return "grid";
        case IRLayoutType::Stack:    return "stack";
        case IRLayoutType::Flow:     return "flow";
        default:                     return "none";
    }
}

IRLayoutType IRSerialization::stringToLayoutType(const QString& str) {
    QString s = str.trimmed().toLower();
    if (s == "flex" || s == "flexbox") return IRLayoutType::Flex;
    if (s == "grid") return IRLayoutType::Grid;
    if (s == "stack") return IRLayoutType::Stack;
    if (s == "flow") return IRLayoutType::Flow;
    return IRLayoutType::Absolute;
}

QString IRSerialization::flexDirectionToString(IRFlexDirection dir) {
    switch (dir) {
        case IRFlexDirection::Row:           return "row";
        case IRFlexDirection::RowReverse:    return "row-reverse";
        case IRFlexDirection::Column:        return "column";
        case IRFlexDirection::ColumnReverse: return "column-reverse";
    }
    return "row";
}

IRFlexDirection IRSerialization::stringToFlexDirection(const QString& str) {
    QString s = str.trimmed().toLower();
    if (s == "row-reverse") return IRFlexDirection::RowReverse;
    if (s == "column") return IRFlexDirection::Column;
    if (s == "column-reverse") return IRFlexDirection::ColumnReverse;
    return IRFlexDirection::Row;
}

QString IRSerialization::justifyContentToString(IRJustifyContent jc) {
    switch (jc) {
        case IRJustifyContent::FlexStart:    return "flex-start";
        case IRJustifyContent::Center:       return "center";
        case IRJustifyContent::FlexEnd:      return "flex-end";
        case IRJustifyContent::SpaceBetween: return "space-between";
        case IRJustifyContent::SpaceAround:  return "space-around";
        case IRJustifyContent::SpaceEvenly:  return "space-evenly";
    }
    return "flex-start";
}

IRJustifyContent IRSerialization::stringToJustifyContent(const QString& str) {
    QString s = str.trimmed().toLower();
    if (s == "center") return IRJustifyContent::Center;
    if (s == "flex-end") return IRJustifyContent::FlexEnd;
    if (s == "space-between") return IRJustifyContent::SpaceBetween;
    if (s == "space-around") return IRJustifyContent::SpaceAround;
    if (s == "space-evenly") return IRJustifyContent::SpaceEvenly;
    return IRJustifyContent::FlexStart;
}

QString IRSerialization::alignItemsToString(IRAlignItems ai) {
    switch (ai) {
        case IRAlignItems::Stretch:   return "stretch";
        case IRAlignItems::FlexStart: return "flex-start";
        case IRAlignItems::Center:    return "center";
        case IRAlignItems::FlexEnd:   return "flex-end";
        case IRAlignItems::Baseline:  return "baseline";
    }
    return "stretch";
}

IRAlignItems IRSerialization::stringToAlignItems(const QString& str) {
    QString s = str.trimmed().toLower();
    if (s == "flex-start") return IRAlignItems::FlexStart;
    if (s == "center") return IRAlignItems::Center;
    if (s == "flex-end") return IRAlignItems::FlexEnd;
    if (s == "baseline") return IRAlignItems::Baseline;
    return IRAlignItems::Stretch;
}

QString IRSerialization::easingTypeToString(IREasingType type) {
    switch (type) {
        case IREasingType::Linear: return "linear";
        case IREasingType::EaseIn: return "ease-in";
        case IREasingType::EaseOut: return "ease-out";
        case IREasingType::EaseInOut: return "ease-in-out";
        case IREasingType::EaseInQuad: return "ease-in-quad";
        case IREasingType::EaseOutQuad: return "ease-out-quad";
        case IREasingType::EaseInOutQuad: return "ease-in-out-quad";
        case IREasingType::EaseInCubic: return "ease-in-cubic";
        case IREasingType::EaseOutCubic: return "ease-out-cubic";
        case IREasingType::EaseInOutCubic: return "ease-in-out-cubic";
        case IREasingType::EaseInSine: return "ease-in-sine";
        case IREasingType::EaseOutSine: return "ease-out-sine";
        case IREasingType::EaseInOutSine: return "ease-in-out-sine";
        case IREasingType::EaseInExpo: return "ease-in-expo";
        case IREasingType::EaseOutExpo: return "ease-out-expo";
        case IREasingType::EaseInOutExpo: return "ease-in-out-expo";
        case IREasingType::EaseInBounce: return "ease-in-bounce";
        case IREasingType::EaseOutBounce: return "ease-out-bounce";
        case IREasingType::EaseInOutBounce: return "ease-in-out-bounce";
        case IREasingType::CustomBezier: return "custom-bezier";
        default: return "ease-in-out";
    }
}

IREasingType IRSerialization::stringToEasingType(const QString& str) {
    QString s = str.trimmed().toLower();
    if (s == "linear") return IREasingType::Linear;
    if (s == "ease-in") return IREasingType::EaseIn;
    if (s == "ease-out") return IREasingType::EaseOut;
    if (s == "ease-in-out") return IREasingType::EaseInOut;
    if (s == "ease-in-quad") return IREasingType::EaseInQuad;
    if (s == "ease-out-quad") return IREasingType::EaseOutQuad;
    if (s == "ease-in-out-quad") return IREasingType::EaseInOutQuad;
    if (s == "ease-in-cubic") return IREasingType::EaseInCubic;
    if (s == "ease-out-cubic") return IREasingType::EaseOutCubic;
    if (s == "ease-in-out-cubic") return IREasingType::EaseInOutCubic;
    if (s == "ease-in-sine") return IREasingType::EaseInSine;
    if (s == "ease-out-sine") return IREasingType::EaseOutSine;
    if (s == "ease-in-out-sine") return IREasingType::EaseInOutSine;
    if (s == "ease-in-expo") return IREasingType::EaseInExpo;
    if (s == "ease-out-expo") return IREasingType::EaseOutExpo;
    if (s == "ease-in-out-expo") return IREasingType::EaseInOutExpo;
    if (s == "ease-in-bounce") return IREasingType::EaseInBounce;
    if (s == "ease-out-bounce") return IREasingType::EaseOutBounce;
    if (s == "ease-in-out-bounce") return IREasingType::EaseInOutBounce;
    if (s == "custom-bezier") return IREasingType::CustomBezier;
    return IREasingType::EaseInOut;
}

QString IRSerialization::bindingDirectionToString(IRBindingDirection dir) {
    switch (dir) {
        case IRBindingDirection::Read:      return "read";
        case IRBindingDirection::Write:     return "write";
        case IRBindingDirection::ReadWrite: return "readwrite";
    }
    return "read";
}

IRBindingDirection IRSerialization::stringToBindingDirection(const QString& str) {
    QString s = str.trimmed().toLower();
    if (s == "write") return IRBindingDirection::Write;
    if (s == "readwrite" || s == "bidirectional") return IRBindingDirection::ReadWrite;
    return IRBindingDirection::Read;
}

QString IRSerialization::dataSourceTypeToString(IRDataSourceType type) {
    switch (type) {
        case IRDataSourceType::Variable: return "variable";
        case IRDataSourceType::Sensor:   return "sensor";
        case IRDataSourceType::Network:  return "rest";
        case IRDataSourceType::Hardware: return "hardware";
        case IRDataSourceType::System:   return "system";
        case IRDataSourceType::Custom:   return "custom";
    }
    return "variable";
}

IRDataSourceType IRSerialization::stringToDataSourceType(const QString& str) {
    QString s = str.trimmed().toLower();
    if (s == "sensor") return IRDataSourceType::Sensor;
    if (s == "network" || s == "rest") return IRDataSourceType::Network;
    if (s == "hardware" || s == "gpio" || s == "adc" || s == "can") return IRDataSourceType::Hardware;
    if (s == "system") return IRDataSourceType::System;
    if (s == "custom") return IRDataSourceType::Custom;
    return IRDataSourceType::Variable;
}

// ============================================================================
// Data Binding & Data Source
// ============================================================================

QJsonObject IRSerialization::dataSourceToJsonObject(const IRDataSource& ds) {
    QJsonObject obj;
    obj["id"] = ds.id.toString();
    obj["name"] = ds.name;
    obj["type"] = dataSourceTypeToString(ds.type);
    if (!ds.hardwareRef.isEmpty()) obj["url"] = ds.hardwareRef;
    if (ds.updateInterval > 0) obj["refreshInterval"] = ds.updateInterval;
    if (ds.initialValue.isValid()) obj["initialValue"] = QJsonValue::fromVariant(ds.initialValue);
    if (!ds.config.isEmpty()) obj["config"] = ds.config;
    return obj;
}

IRDataSource IRSerialization::dataSourceFromJsonObject(const QJsonObject& json, QString* outError) {
    Q_UNUSED(outError);
    IRDataSource ds;
    ds.id = IRNodeId::fromString(json["id"].toString());
    ds.name = json["name"].toString();
    ds.type = stringToDataSourceType(json["type"].toString());
    if (json.contains("url")) ds.hardwareRef = json["url"].toString();
    else if (json.contains("hardwareRef")) ds.hardwareRef = json["hardwareRef"].toString();
    ds.updateInterval = json["refreshInterval"].toInt(json["updateInterval"].toInt(0));
    if (json.contains("initialValue")) ds.initialValue = json["initialValue"].toVariant();
    if (json.contains("config")) ds.config = json["config"].toObject();
    return ds;
}

QJsonObject IRSerialization::bindingToJsonObject(const IRBinding& binding) {
    QJsonObject obj;
    obj["id"] = binding.id.toString();
    obj["sourceId"] = binding.sourceId;
    obj["componentId"] = binding.componentId.toString();
    obj["propertyPath"] = binding.propertyPath;
    obj["direction"] = bindingDirectionToString(binding.direction);
    if (!binding.transformExpression.isEmpty()) obj["transformScript"] = binding.transformExpression;
    if (!binding.validatorExpression.isEmpty()) obj["validatorScript"] = binding.validatorExpression;
    obj["bidirectional"] = (binding.direction == IRBindingDirection::ReadWrite);
    return obj;
}

IRBinding IRSerialization::bindingFromJsonObject(const QJsonObject& json, QString* outError) {
    Q_UNUSED(outError);
    IRBinding binding;
    binding.id = IRNodeId::fromString(json["id"].toString());
    binding.sourceId = json["sourceId"].toString();
    binding.componentId = IRNodeId::fromString(json["componentId"].toString());
    binding.propertyPath = json["propertyPath"].toString();
    
    if (json.contains("bidirectional") && json["bidirectional"].toBool()) {
        binding.direction = IRBindingDirection::ReadWrite;
    } else if (json.contains("direction")) {
        binding.direction = stringToBindingDirection(json["direction"].toString());
    }
    
    if (json.contains("transformScript")) {
        binding.transformExpression = json["transformScript"].toString();
    } else if (json.contains("transformExpression")) {
        binding.transformExpression = json["transformExpression"].toString();
    }
    
    if (json.contains("validatorScript")) {
        binding.validatorExpression = json["validatorScript"].toString();
    } else if (json.contains("validatorExpression")) {
        binding.validatorExpression = json["validatorExpression"].toString();
    }
    return binding;
}

// ============================================================================
// Animation Serialization
// ============================================================================

QJsonObject IRSerialization::animationToJsonObject(const IRAnimation& anim) {
    QJsonObject obj;
    obj["id"] = anim.id.toString();
    obj["name"] = anim.name;
    obj["targetNodeId"] = anim.targetNodeId.toString();
    obj["propertyPath"] = anim.propertyPath;
    obj["duration"] = anim.duration;
    obj["delay"] = anim.delay;
    obj["iterationCount"] = anim.iterationCount;
    obj["easing"] = easingTypeToString(anim.easing.type);

    QJsonArray kfArray;
    for (const IRKeyframe& kf : anim.keyframes) {
        QJsonObject kfObj;
        kfObj["time"] = kf.offset;
        kfObj["value"] = kf.properties;
        kfObj["easing"] = easingTypeToString(kf.easing.type);
        kfArray.append(kfObj);
    }
    obj["keyframes"] = kfArray;
    return obj;
}

IRAnimation IRSerialization::animationFromJsonObject(const QJsonObject& json, QString* outError) {
    Q_UNUSED(outError);
    IRAnimation anim;
    anim.id = IRNodeId::fromString(json["id"].toString());
    anim.name = json["name"].toString();
    anim.targetNodeId = IRNodeId::fromString(json["targetNodeId"].toString());
    anim.propertyPath = json["propertyPath"].toString();
    anim.duration = json["duration"].toInt(300);
    anim.delay = json["delay"].toInt(0);
    anim.iterationCount = json["iterationCount"].toInt(1);
    anim.easing.type = stringToEasingType(json["easing"].toString());

    QJsonArray kfArray = json["keyframes"].toArray();
    for (const QJsonValue& val : kfArray) {
        QJsonObject kfObj = val.toObject();
        IRKeyframe kf;
        kf.offset = kfObj.contains("time") ? kfObj["time"].toDouble() : kfObj["offset"].toDouble();
        kf.properties = kfObj.contains("value") ? kfObj["value"].toObject() : kfObj["properties"].toObject();
        kf.easing.type = stringToEasingType(kfObj["easing"].toString());
        anim.keyframes.append(kf);
    }
    return anim;
}

// ============================================================================
// Widget Serialization
// ============================================================================

QJsonObject IRSerialization::widgetToJsonObject(const IRWidget& widget) {
    QJsonObject obj;
    obj["id"] = widget.id.toString();
    obj["type"] = widget.componentType.toLower();
    if (!widget.name.isEmpty()) obj["name"] = widget.name;
    if (!widget.label.isEmpty()) obj["label"] = widget.label;

    // Layout
    QJsonObject layoutObj;
    layoutObj["type"] = layoutTypeToString(widget.layout.type);
    if (widget.layout.type == IRLayoutType::Flex) {
        layoutObj["direction"] = flexDirectionToString(widget.layout.flexDirection);
        layoutObj["justifyContent"] = justifyContentToString(widget.layout.justifyContent);
        layoutObj["alignItems"] = alignItemsToString(widget.layout.alignItems);
        layoutObj["gap"] = widget.layout.gap;
    }
    if (widget.layout.hasLeft) layoutObj["x"] = widget.layout.left;
    if (widget.layout.hasTop) layoutObj["y"] = widget.layout.top;
    obj["layout"] = layoutObj;

    // Style
    QJsonObject styleObj;
    if (widget.layout.hasLeft) styleObj["x"] = widget.layout.left;
    if (widget.layout.hasTop) styleObj["y"] = widget.layout.top;
    if (widget.properties.contains("width")) styleObj["width"] = widget.properties["width"].toDouble();
    if (widget.properties.contains("height")) styleObj["height"] = widget.properties["height"].toDouble();
    if (widget.effects.backgroundColor.isValid()) {
        styleObj["backgroundColor"] = colorToHex(widget.effects.backgroundColor.solidColor);
    }
    if (widget.textStyle.color.isValid()) {
        styleObj["textColor"] = colorToHex(widget.textStyle.color.solidColor);
    }
    if (widget.textStyle.fontSize > 0) {
        styleObj["fontSize"] = widget.textStyle.fontSize;
    }
    if (!widget.textStyle.fontFamily.isEmpty()) {
        styleObj["fontFamily"] = widget.textStyle.fontFamily;
    }
    if (widget.effects.borderRadius > 0) {
        styleObj["borderRadius"] = widget.effects.borderRadius;
    }
    if (widget.effects.border.width > 0) {
        styleObj["borderWidth"] = widget.effects.border.width;
        if (widget.effects.border.color.isValid()) {
            styleObj["borderColor"] = colorToHex(widget.effects.border.color.solidColor);
        }
    }
    if (widget.effects.opacity < 1.0) {
        styleObj["opacity"] = widget.effects.opacity;
    }
    obj["style"] = styleObj;

    // Properties & Custom Properties
    if (!widget.properties.isEmpty()) {
        obj["properties"] = widget.properties;
    }
    if (!widget.customProperties.isEmpty()) {
        obj["customProperties"] = widget.customProperties;
    }

    // Children
    QJsonArray childrenArray;
    for (const IRWidget& child : widget.children) {
        childrenArray.append(widgetToJsonObject(child));
    }
    obj["children"] = childrenArray;

    // Bindings
    QJsonArray bindingsArray;
    for (const IRBinding& b : widget.bindings) {
        bindingsArray.append(bindingToJsonObject(b));
    }
    obj["bindings"] = bindingsArray;

    return obj;
}

IRWidget IRSerialization::widgetFromJsonObject(const QJsonObject& json, QString* outError) {
    Q_UNUSED(outError);
    IRWidget w;
    w.id = IRNodeId::fromString(json["id"].toString());
    w.componentType = json["type"].toString();
    w.name = json.contains("name") ? json["name"].toString() : w.id.toString();
    w.label = json["label"].toString();

    // Layout
    if (json.contains("layout")) {
        QJsonObject lObj = json["layout"].toObject();
        w.layout.type = stringToLayoutType(lObj["type"].toString());
        if (lObj.contains("direction")) w.layout.flexDirection = stringToFlexDirection(lObj["direction"].toString());
        if (lObj.contains("justifyContent")) w.layout.justifyContent = stringToJustifyContent(lObj["justifyContent"].toString());
        if (lObj.contains("alignItems")) w.layout.alignItems = stringToAlignItems(lObj["alignItems"].toString());
        if (lObj.contains("gap")) w.layout.gap = lObj["gap"].toDouble();
        if (lObj.contains("x")) { w.layout.left = lObj["x"].toDouble(); w.layout.hasLeft = true; }
        if (lObj.contains("y")) { w.layout.top = lObj["y"].toDouble(); w.layout.hasTop = true; }
    }

    // Style
    if (json.contains("style")) {
        QJsonObject sObj = json["style"].toObject();
        if (sObj.contains("x")) { w.layout.left = sObj["x"].toDouble(); w.layout.hasLeft = true; }
        if (sObj.contains("y")) { w.layout.top = sObj["y"].toDouble(); w.layout.hasTop = true; }
        if (sObj.contains("backgroundColor")) {
            w.effects.backgroundColor.type = IRColor::Type::Solid;
            w.effects.backgroundColor.solidColor = colorFromHex(sObj["backgroundColor"].toString());
        }
        if (sObj.contains("textColor")) {
            w.textStyle.color.type = IRColor::Type::Solid;
            w.textStyle.color.solidColor = colorFromHex(sObj["textColor"].toString());
        }
        if (sObj.contains("fontSize")) w.textStyle.fontSize = sObj["fontSize"].toInt();
        if (sObj.contains("fontFamily")) w.textStyle.fontFamily = sObj["fontFamily"].toString();
        if (sObj.contains("borderRadius")) w.effects.borderRadius = sObj["borderRadius"].toInt();
        if (sObj.contains("borderWidth")) w.effects.border.width = sObj["borderWidth"].toDouble();
        if (sObj.contains("borderColor")) {
            w.effects.border.color.type = IRColor::Type::Solid;
            w.effects.border.color.solidColor = colorFromHex(sObj["borderColor"].toString());
        }
        if (sObj.contains("opacity")) w.effects.opacity = sObj["opacity"].toDouble();
    }

    // Properties
    if (json.contains("properties")) {
        w.properties = json["properties"].toObject();
    }
    if (json.contains("customProperties")) {
        w.customProperties = json["customProperties"].toObject();
    }

    // Children
    QJsonArray chArray = json["children"].toArray();
    for (const QJsonValue& chVal : chArray) {
        w.children.append(widgetFromJsonObject(chVal.toObject()));
    }

    // Bindings
    QJsonArray bArray = json["bindings"].toArray();
    for (const QJsonValue& bVal : bArray) {
        w.bindings.append(bindingFromJsonObject(bVal.toObject()));
    }

    return w;
}

// ============================================================================
// Screen Serialization
// ============================================================================

QJsonObject IRSerialization::screenToJsonObject(const IRScreen& screen) {
    QJsonObject obj;
    obj["id"] = screen.id.toString();
    obj["name"] = screen.name;
    obj["width"] = screen.width;
    obj["height"] = screen.height;
    if (screen.backgroundColor.isValid()) {
        obj["backgroundColor"] = colorToHex(screen.backgroundColor.solidColor);
    }

    QJsonObject layoutObj;
    layoutObj["type"] = layoutTypeToString(screen.rootLayout.type);
    if (screen.rootLayout.type == IRLayoutType::Flex) {
        layoutObj["direction"] = flexDirectionToString(screen.rootLayout.flexDirection);
        layoutObj["justifyContent"] = justifyContentToString(screen.rootLayout.justifyContent);
        layoutObj["alignItems"] = alignItemsToString(screen.rootLayout.alignItems);
        layoutObj["gap"] = screen.rootLayout.gap;
    }
    obj["layout"] = layoutObj;

    QJsonArray compArray;
    for (const IRWidget& w : screen.widgets) {
        compArray.append(widgetToJsonObject(w));
    }
    obj["components"] = compArray;
    return obj;
}

IRScreen IRSerialization::screenFromJsonObject(const QJsonObject& json, QString* outError) {
    Q_UNUSED(outError);
    IRScreen screen;
    screen.id = IRNodeId::fromString(json["id"].toString());
    screen.name = json["name"].toString();
    screen.width = json["width"].toInt(320);
    screen.height = json["height"].toInt(240);
    if (json.contains("backgroundColor")) {
        screen.backgroundColor.type = IRColor::Type::Solid;
        screen.backgroundColor.solidColor = colorFromHex(json["backgroundColor"].toString());
    }

    if (json.contains("layout")) {
        QJsonObject lObj = json["layout"].toObject();
        screen.rootLayout.type = stringToLayoutType(lObj["type"].toString());
        if (lObj.contains("direction")) screen.rootLayout.flexDirection = stringToFlexDirection(lObj["direction"].toString());
        if (lObj.contains("justifyContent")) screen.rootLayout.justifyContent = stringToJustifyContent(lObj["justifyContent"].toString());
        if (lObj.contains("alignItems")) screen.rootLayout.alignItems = stringToAlignItems(lObj["alignItems"].toString());
        if (lObj.contains("gap")) screen.rootLayout.gap = lObj["gap"].toDouble();
    }

    QJsonArray compArray = json.contains("components") ? json["components"].toArray() : json["widgets"].toArray();
    for (const QJsonValue& val : compArray) {
        screen.widgets.append(widgetFromJsonObject(val.toObject()));
    }
    return screen;
}

// ============================================================================
// Project Serialization
// ============================================================================

QJsonObject IRSerialization::toJsonObject(const IRProject& project) {
    QJsonObject root;
    root["irVersion"] = "1.0";

    QJsonObject projObj;
    projObj["id"] = project.id.toString();
    projObj["name"] = project.name;
    projObj["displayName"] = !project.displayName.isEmpty() ? project.displayName : project.name;

    // Target Profile
    QJsonObject targetProf;
    targetProf["mcu"] = !project.targetProfile.mcu.isEmpty() ? project.targetProfile.mcu : project.targetFamily;
    targetProf["framework"] = !project.targetProfile.framework.isEmpty() ? project.targetProfile.framework : project.targetFramework;
    targetProf["maxFlash"] = project.targetProfile.maxFlash;
    targetProf["maxRam"] = project.targetProfile.maxRam;
    projObj["targetProfile"] = targetProf;

    // Display
    QJsonObject displayObj;
    displayObj["width"] = project.displayWidth;
    displayObj["height"] = project.displayHeight;
    displayObj["colorDepth"] = project.colorDepth;
    displayObj["colorFormat"] = project.colorFormat;
    displayObj["isRound"] = project.isRound;
    projObj["display"] = displayObj;

    // Screens
    QJsonArray screensArray;
    for (const IRScreen& s : project.screens) {
        screensArray.append(screenToJsonObject(s));
    }
    projObj["screens"] = screensArray;

    // Data Sources
    QJsonArray dsArray;
    for (const IRDataSource& ds : project.dataSources) {
        dsArray.append(dataSourceToJsonObject(ds));
    }
    projObj["dataSources"] = dsArray;

    // Data Bindings
    QJsonArray bArray;
    for (const IRBinding& b : project.bindings) {
        bArray.append(bindingToJsonObject(b));
    }
    projObj["dataBindings"] = bArray;

    // Global animations
    QJsonArray animArray;
    for (const IRAnimation& a : project.globalAnimations) {
        animArray.append(animationToJsonObject(a));
    }
    projObj["animations"] = animArray;

    root["project"] = projObj;
    return root;
}

QString IRSerialization::toJson(const IRProject& project) {
    QJsonDocument doc(toJsonObject(project));
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

IRProject IRSerialization::fromJsonObject(const QJsonObject& root, QString* outError) {
    if (root.isEmpty()) {
        if (outError) *outError = "Empty JSON object";
        return IRProject();
    }

    // Version validation
    QString versionStr = root["irVersion"].toString();
    if (versionStr.isEmpty() && root.contains("project")) {
        versionStr = root["project"].toObject()["irVersion"].toString();
    }

    if (!versionStr.isEmpty()) {
        if (!versionStr.startsWith("1.")) {
            if (outError) *outError = QString("Unsupported IR version: %1 (expected 1.x)").arg(versionStr);
            return IRProject();
        }
    }

    // Project container resolution
    QJsonObject projObj = root.contains("project") ? root["project"].toObject() : root;

    IRProject proj;
    proj.version = IRVersion();
    proj.id = IRNodeId::fromString(projObj["id"].toString());
    proj.name = projObj["name"].toString();
    proj.displayName = projObj.contains("displayName") ? projObj["displayName"].toString() : proj.name;

    // Target Profile
    if (projObj.contains("targetProfile")) {
        QJsonObject tp = projObj["targetProfile"].toObject();
        proj.targetProfile.mcu = tp["mcu"].toString();
        proj.targetProfile.framework = tp["framework"].toString();
        proj.targetProfile.maxFlash = tp["maxFlash"].toInteger(0);
        proj.targetProfile.maxRam = tp["maxRam"].toInteger(0);
        proj.targetFamily = proj.targetProfile.mcu;
        proj.targetFramework = proj.targetProfile.framework;
    }

    // Display
    if (projObj.contains("display")) {
        QJsonObject disp = projObj["display"].toObject();
        proj.displayWidth = disp["width"].toInt(320);
        proj.displayHeight = disp["height"].toInt(240);
        proj.colorDepth = disp["colorDepth"].toInt(16);
        proj.colorFormat = disp["colorFormat"].toString("RGB565");
        proj.isRound = disp["isRound"].toBool(false);
    }

    // Screens
    QJsonArray screensArray = projObj["screens"].toArray();
    for (const QJsonValue& sVal : screensArray) {
        proj.screens.append(screenFromJsonObject(sVal.toObject()));
    }
    if (!proj.screens.isEmpty()) {
        proj.activeScreenId = proj.screens.first().id;
    }

    // Data Sources
    QJsonArray dsArray = projObj["dataSources"].toArray();
    for (const QJsonValue& dsVal : dsArray) {
        proj.dataSources.append(dataSourceFromJsonObject(dsVal.toObject()));
    }

    // Data Bindings
    QJsonArray bArray = projObj.contains("dataBindings") ? projObj["dataBindings"].toArray() : projObj["bindings"].toArray();
    for (const QJsonValue& bVal : bArray) {
        proj.bindings.append(bindingFromJsonObject(bVal.toObject()));
    }

    // Animations
    QJsonArray aArray = projObj["animations"].toArray();
    for (const QJsonValue& aVal : aArray) {
        proj.globalAnimations.append(animationFromJsonObject(aVal.toObject()));
    }

    return proj;
}

IRProject IRSerialization::fromJson(const QString& json, QString* outError) {
    if (json.trimmed().isEmpty()) {
        if (outError) *outError = "Empty JSON string";
        return IRProject();
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (outError) *outError = QString("Invalid JSON: %1 at offset %2").arg(parseError.errorString()).arg(parseError.offset);
        return IRProject();
    }

    if (!doc.isObject()) {
        if (outError) *outError = "Root JSON must be an object";
        return IRProject();
    }

    return fromJsonObject(doc.object(), outError);
}

} // namespace IR

// ============================================================================
// Direct IRProject member function delegators
// ============================================================================

namespace IR {

QString IRProject::toJson() const {
    return IRSerialization::toJson(*this);
}

QJsonObject IRProject::toJsonObject() const {
    return IRSerialization::toJsonObject(*this);
}

IRProject IRProject::fromJson(const QString& json, QString* outError) {
    return IRSerialization::fromJson(json, outError);
}

IRProject IRProject::fromJsonObject(const QJsonObject& json, QString* outError) {
    return IRSerialization::fromJsonObject(json, outError);
}

IRProject IRProject::fromJson(const QJsonObject& obj) {
    return IRSerialization::fromJsonObject(obj);
}

QJsonObject IRWidget::toJson() const {
    return IRSerialization::widgetToJsonObject(*this);
}

IRWidget IRWidget::fromJson(const QJsonObject& obj) {
    return IRSerialization::widgetFromJsonObject(obj);
}

IRWidget IRWidget::fromJsonObject(const QJsonObject& json, QString* outError) {
    return IRSerialization::widgetFromJsonObject(json, outError);
}

QJsonObject IRScreen::toJson() const {
    return IRSerialization::screenToJsonObject(*this);
}

IRScreen IRScreen::fromJson(const QJsonObject& obj) {
    return IRSerialization::screenFromJsonObject(obj);
}

IRScreen IRScreen::fromJsonObject(const QJsonObject& json, QString* outError) {
    return IRSerialization::screenFromJsonObject(json, outError);
}

} // namespace IR
