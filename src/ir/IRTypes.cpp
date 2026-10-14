#include "IRTypes.h"
#include <QDateTime>
#include <QUuid>
#include <QDebug>
#include <QSet>
#include <functional>

namespace IR {

// ============================================================================
// Versioning & Schema Migration
// ============================================================================

QJsonObject IRVersion::toJson() const {
    QJsonObject obj;
    obj["major"] = major;
    obj["minor"] = minor;
    obj["patch"] = patch;
    return obj;
}

IRVersion IRVersion::fromJson(const QJsonObject& obj) {
    IRVersion version;
    version.major = obj["major"].toInt(1);
    version.minor = obj["minor"].toInt(0);
    version.patch = obj["patch"].toInt(0);
    return version;
}

bool IRVersion::operator==(const IRVersion& other) const {
    return major == other.major && minor == other.minor && patch == other.patch;
}

bool IRVersion::operator<(const IRVersion& other) const {
    if (major != other.major) return major < other.major;
    if (minor != other.minor) return minor < other.minor;
    return patch < other.patch;
}

bool IRVersion::isCompatibleWith(const IRVersion& other) const {
    return major == other.major; // Compatible if same major version
}

QString IRVersion::toString() const {
    return QString("%1.%2.%3").arg(major).arg(minor).arg(patch);
}

// ============================================================================
// Core Identity & Provenance
// ============================================================================

QJsonObject IRSourceProvenance::toJson() const {
    QJsonObject obj;
    obj["sourceNodeId"] = sourceNodeId.toString();
    obj["sourceFormat"] = sourceFormat;
    obj["sourceFilePath"] = sourceFilePath;
    obj["importedAt"] = importedAt.toString(Qt::ISODate);
    obj["originalProperties"] = originalProperties;
    return obj;
}

IRSourceProvenance IRSourceProvenance::fromJson(const QJsonObject& obj) {
    IRSourceProvenance provenance;
    provenance.sourceNodeId = IRNodeId::fromString(obj["sourceNodeId"].toString());
    provenance.sourceFormat = obj["sourceFormat"].toString();
    provenance.sourceFilePath = obj["sourceFilePath"].toString();
    provenance.importedAt = QDateTime::fromString(obj["importedAt"].toString(), Qt::ISODate);
    provenance.originalProperties = obj["originalProperties"].toObject();
    return provenance;
}

// ============================================================================
// Diagnostics & Validation
// ============================================================================

QJsonObject IRDiagnostic::toJson() const {
    QJsonObject obj;
    obj["severity"] = static_cast<int>(severity);
    obj["code"] = static_cast<int>(code);
    obj["message"] = message;
    obj["nodeId"] = nodeId.toString();
    obj["propertyPath"] = propertyPath;
    obj["sourceFormat"] = sourceFormat;
    obj["suggestedFix"] = suggestedFix;
    obj["context"] = context;
    return obj;
}

IRDiagnostic IRDiagnostic::fromJson(const QJsonObject& obj) {
    IRDiagnostic diagnostic;
    diagnostic.severity = static_cast<Severity>(obj["severity"].toInt());
    diagnostic.code = static_cast<Code>(obj["code"].toInt());
    diagnostic.message = obj["message"].toString();
    diagnostic.nodeId = IRNodeId::fromString(obj["nodeId"].toString());
    diagnostic.propertyPath = obj["propertyPath"].toString();
    diagnostic.sourceFormat = obj["sourceFormat"].toString();
    diagnostic.suggestedFix = obj["suggestedFix"].toString();
    diagnostic.context = obj["context"].toObject();
    return diagnostic;
}

// ============================================================================
// Layout System
// ============================================================================

QJsonObject IRLayout::toJson() const {
    QJsonObject obj;
    obj["type"] = static_cast<int>(type);

    // Flex properties
    obj["flexDirection"] = static_cast<int>(flexDirection);
    obj["flexWrap"] = static_cast<int>(flexWrap);
    obj["alignItems"] = static_cast<int>(alignItems);
    obj["justifyContent"] = static_cast<int>(justifyContent);
    obj["gap"] = gap;
    obj["rowGap"] = rowGap;
    obj["columnGap"] = columnGap;

    // Grid properties
    if (!gridTemplateColumns.isEmpty()) {
        QJsonArray cols;
        for (const QString& col : gridTemplateColumns) cols.append(col);
        obj["gridTemplateColumns"] = cols;
    }
    if (!gridTemplateRows.isEmpty()) {
        QJsonArray rows;
        for (const QString& row : gridTemplateRows) rows.append(row);
        obj["gridTemplateRows"] = rows;
    }
    if (!gridTemplateAreas.isEmpty()) {
        QJsonArray areas;
        for (const QString& area : gridTemplateAreas) areas.append(area);
        obj["gridTemplateAreas"] = areas;
    }

    // Child layout properties
    obj["flexGrow"] = flexGrow;
    obj["flexShrink"] = flexShrink;
    obj["flexBasis"] = flexBasis;
    obj["alignSelf"] = alignSelf;
    obj["gridColumnStart"] = gridColumnStart;
    obj["gridColumnEnd"] = gridColumnEnd;
    obj["gridRowStart"] = gridRowStart;
    obj["gridRowEnd"] = gridRowEnd;
    obj["gridArea"] = gridArea;

    // Constraints
    obj["useConstraints"] = useConstraints;
    if (hasLeft) obj["left"] = left;
    if (hasRight) obj["right"] = right;
    if (hasTop) obj["top"] = top;
    if (hasBottom) obj["bottom"] = bottom;
    if (hasCenterX) obj["centerX"] = centerX;
    if (hasCenterY) obj["centerY"] = centerY;

    return obj;
}

IRLayout IRLayout::fromJson(const QJsonObject& obj) {
    IRLayout layout;
    layout.type = static_cast<IRLayoutType>(obj["type"].toInt());

    // Flex properties
    layout.flexDirection = static_cast<IRFlexDirection>(obj["flexDirection"].toInt());
    layout.flexWrap = static_cast<IRFlexWrap>(obj["flexWrap"].toInt());
    layout.alignItems = static_cast<IRAlignItems>(obj["alignItems"].toInt());
    layout.justifyContent = static_cast<IRJustifyContent>(obj["justifyContent"].toInt());
    layout.gap = obj["gap"].toDouble();
    layout.rowGap = obj["rowGap"].toDouble();
    layout.columnGap = obj["columnGap"].toDouble();

    // Grid properties
    if (obj.contains("gridTemplateColumns")) {
        QJsonArray cols = obj["gridTemplateColumns"].toArray();
        for (const QJsonValue& col : cols) layout.gridTemplateColumns.append(col.toString());
    }
    if (obj.contains("gridTemplateRows")) {
        QJsonArray rows = obj["gridTemplateRows"].toArray();
        for (const QJsonValue& row : rows) layout.gridTemplateRows.append(row.toString());
    }
    if (obj.contains("gridTemplateAreas")) {
        QJsonArray areas = obj["gridTemplateAreas"].toArray();
        for (const QJsonValue& area : areas) layout.gridTemplateAreas.append(area.toString());
    }

    // Child layout properties
    layout.flexGrow = obj["flexGrow"].toDouble();
    layout.flexShrink = obj["flexShrink"].toDouble();
    layout.flexBasis = obj["flexBasis"].toDouble();
    layout.alignSelf = obj["alignSelf"].toInt();
    layout.gridColumnStart = obj["gridColumnStart"].toInt();
    layout.gridColumnEnd = obj["gridColumnEnd"].toInt();
    layout.gridRowStart = obj["gridRowStart"].toInt();
    layout.gridRowEnd = obj["gridRowEnd"].toInt();
    layout.gridArea = obj["gridArea"].toString();

    // Constraints
    layout.useConstraints = obj["useConstraints"].toBool();
    layout.hasLeft = obj.contains("left");
    layout.left = obj["left"].toDouble();
    layout.hasRight = obj.contains("right");
    layout.right = obj["right"].toDouble();
    layout.hasTop = obj.contains("top");
    layout.top = obj["top"].toDouble();
    layout.hasBottom = obj.contains("bottom");
    layout.bottom = obj["bottom"].toDouble();
    layout.hasCenterX = obj.contains("centerX");
    layout.centerX = obj["centerX"].toDouble();
    layout.hasCenterY = obj.contains("centerY");
    layout.centerY = obj["centerY"].toDouble();

    return layout;
}

// ============================================================================
// Style System
// ============================================================================

QJsonObject IRColor::GradientStop::toJson() const {
    QJsonObject obj;
    obj["offset"] = offset;
    obj["color"] = color.name(QColor::HexArgb);
    return obj;
}

IRColor::GradientStop IRColor::GradientStop::fromJson(const QJsonObject& obj) {
    GradientStop stop;
    stop.offset = obj["offset"].toDouble();
    stop.color = QColor(obj["color"].toString());
    return stop;
}

QJsonObject IRColor::toJson() const {
    QJsonObject obj;
    obj["type"] = static_cast<int>(type);

    switch (type) {
        case Type::Solid:
            obj["solidColor"] = solidColor.name(QColor::HexArgb);
            break;
        case Type::LinearGradient:
            obj["gradientAngle"] = gradientAngle;
            break;
        case Type::RadialGradient:
            obj["gradientCenter"] = QString("%1,%2").arg(gradientCenter.x()).arg(gradientCenter.y());
            obj["gradientRadius"] = gradientRadius;
            break;
        case Type::StyleRef:
            obj["styleRef"] = styleRef;
            break;
    }

    if (!gradientStops.isEmpty()) {
        QJsonArray stops;
        for (const GradientStop& stop : gradientStops) {
            stops.append(stop.toJson());
        }
        obj["gradientStops"] = stops;
    }

    return obj;
}

IRColor IRColor::fromJson(const QJsonObject& obj) {
    IRColor color;
    color.type = static_cast<Type>(obj["type"].toInt());

    switch (color.type) {
        case Type::Solid:
            color.solidColor = QColor(obj["solidColor"].toString());
            break;
        case Type::LinearGradient:
            color.gradientAngle = obj["gradientAngle"].toDouble();
            break;
        case Type::RadialGradient: {
            QString centerStr = obj["gradientCenter"].toString();
            QStringList parts = centerStr.split(',');
            if (parts.size() == 2) {
                color.gradientCenter = QPointF(parts[0].toDouble(), parts[1].toDouble());
            }
            color.gradientRadius = obj["gradientRadius"].toDouble();
            break;
        }
        case Type::StyleRef:
            color.styleRef = obj["styleRef"].toString();
            break;
    }

    if (obj.contains("gradientStops")) {
        QJsonArray stops = obj["gradientStops"].toArray();
        for (const QJsonValue& stopVal : stops) {
            color.gradientStops.append(GradientStop::fromJson(stopVal.toObject()));
        }
    }

    return color;
}

// ============================================================================
// Effects & Transforms
// ============================================================================

QJsonObject IRBorder::toJson() const {
    QJsonObject obj;
    obj["color"] = color.toJson();
    obj["width"] = width;
    obj["style"] = static_cast<int>(style);
    return obj;
}

IRBorder IRBorder::fromJson(const QJsonObject& obj) {
    IRBorder border;
    border.color = IRColor::fromJson(obj["color"].toObject());
    border.width = obj["width"].toDouble();
    border.style = static_cast<IRBorderStyle>(obj["style"].toInt());
    return border;
}

QJsonObject IRShadow::toJson() const {
    QJsonObject obj;
    obj["offsetX"] = offsetX;
    obj["offsetY"] = offsetY;
    obj["blurRadius"] = blurRadius;
    obj["spreadRadius"] = spreadRadius;
    obj["color"] = color.toJson();
    obj["inset"] = inset;
    return obj;
}

IRShadow IRShadow::fromJson(const QJsonObject& obj) {
    IRShadow shadow;
    shadow.offsetX = obj["offsetX"].toDouble();
    shadow.offsetY = obj["offsetY"].toDouble();
    shadow.blurRadius = obj["blurRadius"].toDouble();
    shadow.spreadRadius = obj["spreadRadius"].toDouble();
    shadow.color = IRColor::fromJson(obj["color"].toObject());
    shadow.inset = obj["inset"].toBool();
    return shadow;
}

QJsonObject IRTransform::toJson() const {
    QJsonObject obj;
    obj["translateX"] = translateX;
    obj["translateY"] = translateY;
    obj["scaleX"] = scaleX;
    obj["scaleY"] = scaleY;
    obj["rotation"] = rotation;
    obj["skewX"] = skewX;
    obj["skewY"] = skewY;
    obj["transformOrigin"] = QString("%1,%2").arg(transformOrigin.x()).arg(transformOrigin.y());
    return obj;
}

IRTransform IRTransform::fromJson(const QJsonObject& obj) {
    IRTransform transform;
    transform.translateX = obj["translateX"].toDouble();
    transform.translateY = obj["translateY"].toDouble();
    transform.scaleX = obj["scaleX"].toDouble();
    transform.scaleY = obj["scaleY"].toDouble();
    transform.rotation = obj["rotation"].toDouble();
    transform.skewX = obj["skewX"].toDouble();
    transform.skewY = obj["skewY"].toDouble();

    QString originStr = obj["transformOrigin"].toString();
    QStringList parts = originStr.split(',');
    if (parts.size() == 2) {
        transform.transformOrigin = QPointF(parts[0].toDouble(), parts[1].toDouble());
    }

    return transform;
}

bool IRTransform::isIdentity() const {
    return translateX == 0 && translateY == 0 &&
           scaleX == 1 && scaleY == 1 &&
           rotation == 0 && skewX == 0 && skewY == 0;
}

// ============================================================================
// IRAsset
// ============================================================================

QJsonObject IRAsset::toJson() const {
    QJsonObject obj;
    obj["id"] = id.toString();
    obj["name"] = name;
    obj["type"] = static_cast<int>(type);
    obj["mimeType"] = mimeType;
    obj["filePath"] = filePath;
    obj["originalPath"] = originalPath;
    obj["fileSize"] = fileSize;
    obj["width"] = width;
    obj["height"] = height;
    obj["colorFormat"] = colorFormat;

    if (!metadata.isEmpty()) {
        QJsonObject metaObj;
        for (auto it = metadata.constBegin(); it != metadata.constEnd(); ++it) {
            metaObj[it.key()] = it.value();
        }
        obj["metadata"] = metaObj;
    }

    obj["provenance"] = provenance.toJson();
    return obj;
}

IRAsset IRAsset::fromJson(const QJsonObject& obj) {
    IRAsset asset;
    asset.id = IRNodeId::fromString(obj["id"].toString());
    asset.name = obj["name"].toString();
    asset.type = static_cast<IRAssetType>(obj["type"].toInt());
    asset.mimeType = obj["mimeType"].toString();
    asset.filePath = obj["filePath"].toString();
    asset.originalPath = obj["originalPath"].toString();
    asset.fileSize = obj["fileSize"].toInt();
    asset.width = obj["width"].toInt();
    asset.height = obj["height"].toInt();
    asset.colorFormat = obj["colorFormat"].toString();

    if (obj.contains("metadata")) {
        QJsonObject metaObj = obj["metadata"].toObject();
        for (auto it = metaObj.begin(); it != metaObj.end(); ++it) {
            asset.metadata[it.key()] = it.value().toString();
        }
    }

    asset.provenance = IRSourceProvenance::fromJson(obj["provenance"].toObject());
    return asset;
}

// ============================================================================
// Animations
// ============================================================================

QJsonObject IREasing::toJson() const {
    QJsonObject obj;
    obj["type"] = static_cast<int>(type);
    if (!bezierPoints.isEmpty()) {
        QJsonArray points;
        for (qreal point : bezierPoints) points.append(point);
        obj["bezierPoints"] = points;
    }
    return obj;
}

IREasing IREasing::fromJson(const QJsonObject& obj) {
    IREasing easing;
    easing.type = static_cast<IREasingType>(obj["type"].toInt());

    if (obj.contains("bezierPoints")) {
        QJsonArray points = obj["bezierPoints"].toArray();
        for (const QJsonValue& pointVal : points) {
            easing.bezierPoints.append(pointVal.toDouble());
        }
    }

    return easing;
}

QJsonObject IRKeyframe::toJson() const {
    QJsonObject obj;
    obj["offset"] = offset;
    obj["properties"] = properties;
    obj["easing"] = easing.toJson();
    return obj;
}

IRKeyframe IRKeyframe::fromJson(const QJsonObject& obj) {
    IRKeyframe keyframe;
    keyframe.offset = obj["offset"].toDouble();
    keyframe.properties = obj["properties"].toObject();
    keyframe.easing = IREasing::fromJson(obj["easing"].toObject());
    return keyframe;
}

QJsonObject IRAnimation::toJson() const {
    QJsonObject obj;
    obj["id"] = id.toString();
    obj["name"] = name;
    obj["targetNodeId"] = targetNodeId.toString();
    obj["propertyPath"] = propertyPath;

    QJsonArray kfArray;
    for (const IRKeyframe& kf : keyframes) {
        kfArray.append(kf.toJson());
    }
    obj["keyframes"] = kfArray;

    obj["duration"] = duration;
    obj["delay"] = delay;
    obj["iterationCount"] = iterationCount;
    obj["direction"] = static_cast<int>(direction);
    obj["fillMode"] = static_cast<int>(fillMode);
    obj["easing"] = easing.toJson();
    obj["isPlaying"] = isPlaying;

    return obj;
}

IRAnimation IRAnimation::fromJson(const QJsonObject& obj) {
    IRAnimation animation;
    animation.id = IRNodeId::fromString(obj["id"].toString());
    animation.name = obj["name"].toString();
    animation.targetNodeId = IRNodeId::fromString(obj["targetNodeId"].toString());
    animation.propertyPath = obj["propertyPath"].toString();

    QJsonArray keyframes = obj["keyframes"].toArray();
    for (const QJsonValue& kfVal : keyframes) {
        animation.keyframes.append(IRKeyframe::fromJson(kfVal.toObject()));
    }

    animation.duration = obj["duration"].toInt();
    animation.delay = obj["delay"].toInt();
    animation.iterationCount = obj["iterationCount"].toInt();
    animation.direction = static_cast<IRAnimationDirection>(obj["direction"].toInt());
    animation.fillMode = static_cast<IRAnimationFillMode>(obj["fillMode"].toInt());
    animation.easing = IREasing::fromJson(obj["easing"].toObject());
    animation.isPlaying = obj["isPlaying"].toBool();

    return animation;
}

// ============================================================================
// IRProject Validation
// ============================================================================

IRDiagnostics IRProject::validate() const {
    IRDiagnostics diags;

    if (name.trimmed().isEmpty()) {
        IRDiagnostic d;
        d.severity = IRDiagnostic::Severity::Error;
        d.code = IRDiagnostic::Code::MissingRequiredProperty;
        d.message = "Project name cannot be empty.";
        d.propertyPath = "name";
        diags.append(d);
    }

    if (displayWidth <= 0 || displayHeight <= 0) {
        IRDiagnostic d;
        d.severity = IRDiagnostic::Severity::Error;
        d.code = IRDiagnostic::Code::InvalidPropertyValue;
        d.message = QString("Invalid display dimensions: %1x%2.").arg(displayWidth).arg(displayHeight);
        d.propertyPath = "displayWidth";
        diags.append(d);
    }

    if (screens.isEmpty()) {
        IRDiagnostic d;
        d.severity = IRDiagnostic::Severity::Warning;
        d.code = IRDiagnostic::Code::MissingRequiredProperty;
        d.message = "Project contains no screens.";
        d.propertyPath = "screens";
        diags.append(d);
    }

    QSet<QString> seenNodeIds;
    QSet<QString> existingWidgetIds;
    QSet<QString> existingSourceIds;

    for (const auto& ds : dataSources) {
        QString sid = ds.id.toString();
        if (seenNodeIds.contains(sid)) {
            IRDiagnostic d;
            d.severity = IRDiagnostic::Severity::Error;
            d.code = IRDiagnostic::Code::DuplicateNodeId;
            d.message = QString("Duplicate data source ID: %1").arg(sid);
            d.nodeId = ds.id;
            diags.append(d);
        }
        seenNodeIds.insert(sid);
        existingSourceIds.insert(sid);
    }

    for (int sIdx = 0; sIdx < screens.size(); ++sIdx) {
        const IRScreen& s = screens[sIdx];
        QString sid = s.id.toString();
        if (seenNodeIds.contains(sid)) {
            IRDiagnostic d;
            d.severity = IRDiagnostic::Severity::Error;
            d.code = IRDiagnostic::Code::DuplicateNodeId;
            d.message = QString("Duplicate screen ID: %1").arg(sid);
            d.nodeId = s.id;
            diags.append(d);
        }
        seenNodeIds.insert(sid);

        if (s.width <= 0 || s.height <= 0) {
            IRDiagnostic d;
            d.severity = IRDiagnostic::Severity::Warning;
            d.code = IRDiagnostic::Code::InvalidPropertyValue;
            d.message = QString("Screen '%1' has non-positive dimensions: %2x%3").arg(s.name).arg(s.width).arg(s.height);
            d.nodeId = s.id;
            diags.append(d);
        }

        std::function<void(const IRWidget&)> validateWidgetTree = [&](const IRWidget& w) {
            QString wid = w.id.toString();
            if (seenNodeIds.contains(wid)) {
                IRDiagnostic d;
                d.severity = IRDiagnostic::Severity::Error;
                d.code = IRDiagnostic::Code::DuplicateNodeId;
                d.message = QString("Duplicate widget ID: %1").arg(wid);
                d.nodeId = w.id;
                diags.append(d);
            }
            seenNodeIds.insert(wid);
            existingWidgetIds.insert(wid);

            if (w.componentType.isEmpty()) {
                IRDiagnostic d;
                d.severity = IRDiagnostic::Severity::Error;
                d.code = IRDiagnostic::Code::MissingRequiredProperty;
                d.message = QString("Widget '%1' has empty componentType.").arg(wid);
                d.nodeId = w.id;
                diags.append(d);
            }

            for (const auto& child : w.children) {
                validateWidgetTree(child);
            }
        };

        for (const auto& w : s.widgets) {
            validateWidgetTree(w);
        }
    }

    for (const auto& b : bindings) {
        if (!existingSourceIds.contains(b.sourceId)) {
            IRDiagnostic d;
            d.severity = IRDiagnostic::Severity::Warning;
            d.code = IRDiagnostic::Code::OrphanedNode;
            d.message = QString("Binding references non-existent data source: %1").arg(b.sourceId);
            d.nodeId = b.id;
            diags.append(d);
        }
        if (!existingWidgetIds.contains(b.componentId.toString())) {
            IRDiagnostic d;
            d.severity = IRDiagnostic::Severity::Warning;
            d.code = IRDiagnostic::Code::OrphanedNode;
            d.message = QString("Binding references non-existent widget: %1").arg(b.componentId.toString());
            d.nodeId = b.id;
            diags.append(d);
        }
    }

    return diags;
}

IRDiagnostics IRProject::validateAgainstCapabilities(const IRTargetCapabilities& caps) const {
    IRDiagnostics diags;

    if (caps.maxWidth > 0 && displayWidth > caps.maxWidth) {
        IRDiagnostic d;
        d.severity = IRDiagnostic::Severity::Error;
        d.code = IRDiagnostic::Code::CapabilityMismatch;
        d.message = QString("Display width %1 exceeds target maximum %2").arg(displayWidth).arg(caps.maxWidth);
        d.propertyPath = "displayWidth";
        diags.append(d);
    }
    if (caps.maxHeight > 0 && displayHeight > caps.maxHeight) {
        IRDiagnostic d;
        d.severity = IRDiagnostic::Severity::Error;
        d.code = IRDiagnostic::Code::CapabilityMismatch;
        d.message = QString("Display height %1 exceeds target maximum %2").arg(displayHeight).arg(caps.maxHeight);
        d.propertyPath = "displayHeight";
        diags.append(d);
    }

    if (isRound && !caps.supportsRoundDisplay) {
        IRDiagnostic d;
        d.severity = IRDiagnostic::Severity::Warning;
        d.code = IRDiagnostic::Code::CapabilityMismatch;
        d.message = "Target does not support round displays.";
        d.propertyPath = "isRound";
        diags.append(d);
    }

    if (!caps.supportedColorFormats.isEmpty() && !caps.supportedColorFormats.contains(colorFormat)) {
        IRDiagnostic d;
        d.severity = IRDiagnostic::Severity::Warning;
        d.code = IRDiagnostic::Code::CapabilityMismatch;
        d.message = QString("Color format '%1' not in target supported formats [%2]")
                        .arg(colorFormat, caps.supportedColorFormats.join(", "));
        d.propertyPath = "colorFormat";
        diags.append(d);
    }

    std::function<void(const IRWidget&)> checkWidget = [&](const IRWidget& w) {
        if (w.layout.type == IRLayoutType::Flex && !caps.supportsFlexLayout) {
            IRDiagnostic d;
            d.severity = IRDiagnostic::Severity::Warning;
            d.code = IRDiagnostic::Code::UnsupportedProperty;
            d.message = QString("Widget '%1' uses Flex layout unsupported by target.").arg(w.id.toString());
            d.nodeId = w.id;
            d.suggestedFix = "Lower layout to Absolute positioning.";
            diags.append(d);
        }
        if (w.layout.type == IRLayoutType::Grid && !caps.supportsGridLayout) {
            IRDiagnostic d;
            d.severity = IRDiagnostic::Severity::Warning;
            d.code = IRDiagnostic::Code::UnsupportedProperty;
            d.message = QString("Widget '%1' uses Grid layout unsupported by target.").arg(w.id.toString());
            d.nodeId = w.id;
            diags.append(d);
        }

        if (w.effects.backgroundColor.type == IRColor::Type::LinearGradient && !caps.supportsLinearGradients && !caps.supportsGradients) {
            IRDiagnostic d;
            d.severity = IRDiagnostic::Severity::Warning;
            d.code = IRDiagnostic::Code::UnsupportedProperty;
            d.message = QString("Widget '%1' uses linear gradient unsupported by target.").arg(w.id.toString());
            d.nodeId = w.id;
            d.suggestedFix = "Use solid color background.";
            diags.append(d);
        }
        if (w.effects.backgroundColor.type == IRColor::Type::RadialGradient && !caps.supportsRadialGradients && !caps.supportsGradients) {
            IRDiagnostic d;
            d.severity = IRDiagnostic::Severity::Warning;
            d.code = IRDiagnostic::Code::UnsupportedProperty;
            d.message = QString("Widget '%1' uses radial gradient unsupported by target.").arg(w.id.toString());
            d.nodeId = w.id;
            diags.append(d);
        }

        if (w.effects.borderRadius > 0 && !caps.supportsBorderRadius) {
            IRDiagnostic d;
            d.severity = IRDiagnostic::Severity::Warning;
            d.code = IRDiagnostic::Code::UnsupportedProperty;
            d.message = QString("Widget '%1' has borderRadius but target does not support rounded corners.").arg(w.id.toString());
            d.nodeId = w.id;
            diags.append(d);
        } else if (caps.maxBorderRadius > 0 && w.effects.borderRadius > caps.maxBorderRadius) {
            IRDiagnostic d;
            d.severity = IRDiagnostic::Severity::Info;
            d.code = IRDiagnostic::Code::ApproximateProperty;
            d.message = QString("Widget '%1' borderRadius (%2) exceeds target max (%3); will be clamped.")
                            .arg(w.id.toString()).arg(w.effects.borderRadius).arg(caps.maxBorderRadius);
            d.nodeId = w.id;
            diags.append(d);
        }

        if (w.effects.opacity < 1.0 && !caps.supportsOpacity) {
            IRDiagnostic d;
            d.severity = IRDiagnostic::Severity::Warning;
            d.code = IRDiagnostic::Code::UnsupportedProperty;
            d.message = QString("Widget '%1' uses opacity unsupported by target.").arg(w.id.toString());
            d.nodeId = w.id;
            diags.append(d);
        }

        if (!w.effects.shadows.isEmpty() && !caps.supportsBoxShadow) {
            IRDiagnostic d;
            d.severity = IRDiagnostic::Severity::Warning;
            d.code = IRDiagnostic::Code::UnsupportedProperty;
            d.message = QString("Widget '%1' uses box shadow unsupported by target.").arg(w.id.toString());
            d.nodeId = w.id;
            diags.append(d);
        }

        if (!w.animations.isEmpty() && !caps.supportsAnimations) {
            IRDiagnostic d;
            d.severity = IRDiagnostic::Severity::Warning;
            d.code = IRDiagnostic::Code::UnsupportedProperty;
            d.message = QString("Widget '%1' contains animations unsupported by target.").arg(w.id.toString());
            d.nodeId = w.id;
            diags.append(d);
        }

        for (const auto& child : w.children) {
            checkWidget(child);
        }
    };

    for (const auto& s : screens) {
        for (const auto& w : s.widgets) {
            checkWidget(w);
        }
    }

    return diags;
}

} // namespace IR