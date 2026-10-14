#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QColor>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QUuid>
#include <QDateTime>
#include <QPointF>

namespace IR {

// ============================================================================
// Versioning & Schema Migration
// ============================================================================

constexpr int IR_VERSION_MAJOR = 1;
constexpr int IR_VERSION_MINOR = 0;
constexpr int IR_VERSION_PATCH = 0;
constexpr const char* IR_SCHEMA_VERSION = "1.0.0";

struct IRVersion {
    int major = IR_VERSION_MAJOR;
    int minor = IR_VERSION_MINOR;
    int patch = IR_VERSION_PATCH;

    QJsonObject toJson() const;
    static IRVersion fromJson(const QJsonObject& obj);
    bool operator==(const IRVersion& other) const;
    bool operator<(const IRVersion& other) const;
    bool isCompatibleWith(const IRVersion& other) const; // same major version
    QString toString() const;
};

struct IRSchemaMigration {
    IRVersion fromVersion;
    IRVersion toVersion;
    QString description;
    // Migration function would be registered separately
};

// ============================================================================
// Core Identity & Provenance
// ============================================================================

struct IRNodeId {
    QString value;

    IRNodeId() = default;
    explicit IRNodeId(const QString& v) : value(v) {}
    static IRNodeId generate() { return IRNodeId(QUuid::createUuid().toString(QUuid::WithoutBraces)); }
    static IRNodeId fromString(const QString& s) { return IRNodeId(s); }
    bool isValid() const { return !value.isEmpty(); }
    QString toString() const { return value; }

    bool operator==(const IRNodeId& other) const { return value == other.value; }
    bool operator!=(const IRNodeId& other) const { return value != other.value; }
    bool operator<(const IRNodeId& other) const { return value < other.value; }
};

inline uint qHash(const IRNodeId& id, uint seed = 0) { return qHash(id.value, seed); }

struct IRSourceProvenance {
    IRNodeId sourceNodeId;          // Original node ID from source format (Figma, HTML, etc.)
    QString sourceFormat;           // "figma", "html", "qml", "legacy", "manual"
    QString sourceFilePath;         // Optional: path to source file
    QDateTime importedAt;
    QJsonObject originalProperties; // Raw properties from source for round-trip fidelity

    QJsonObject toJson() const;
    static IRSourceProvenance fromJson(const QJsonObject& obj);
};

// ============================================================================
// Diagnostics & Validation
// ============================================================================

struct IRDiagnostic {
    enum class Severity { Info, Warning, Error };
    enum class Code {
        // Capability-related
        UnsupportedProperty,
        ApproximateProperty,
        UnknownProperty,
        CapabilityMismatch,

        // Structure-related
        MissingRequiredProperty,
        InvalidPropertyValue,
        DuplicateNodeId,
        OrphanedNode,
        CircularReference,

        // Migration-related
        SchemaMigrationApplied,
        SchemaMigrationFailed,
        DeprecatedProperty,

        // Import-related
        ImportApproximated,
        ImportSkipped,
        ImportFailed,

        // General
        ValidationError,
        InternalError
    };

    Severity severity = Severity::Info;
    Code code = Code::ValidationError;
    QString message;
    IRNodeId nodeId;
    QString propertyPath;           // e.g., "screens[0].widgets[2].properties.borderRadius"
    QString sourceFormat;           // "figma", "html", "legacy", etc.
    QString suggestedFix;           // Human-readable suggestion
    QJsonObject context;            // Additional structured context

    QJsonObject toJson() const;
    static IRDiagnostic fromJson(const QJsonObject& obj);
};

using IRDiagnostics = QList<IRDiagnostic>;

// ============================================================================
// Layout System
// ============================================================================

enum class IRLayoutType {
    Absolute,       // Fixed x,y,width,height
    Flex,           // Flexbox-like
    Grid,           // CSS Grid-like
    Stack,          // Stack/overlay
    Flow            // Flow layout
};

enum class IRFlexDirection { Row, RowReverse, Column, ColumnReverse };
enum class IRFlexWrap { NoWrap, Wrap, WrapReverse };
enum class IRAlignItems { Stretch, FlexStart, Center, FlexEnd, Baseline };
enum class IRJustifyContent { FlexStart, Center, FlexEnd, SpaceBetween, SpaceAround, SpaceEvenly };

struct IRLayout {
    IRLayoutType type = IRLayoutType::Absolute;

    // Flex properties
    IRFlexDirection flexDirection = IRFlexDirection::Row;
    IRFlexWrap flexWrap = IRFlexWrap::NoWrap;
    IRAlignItems alignItems = IRAlignItems::Stretch;
    IRJustifyContent justifyContent = IRJustifyContent::FlexStart;
    qreal gap = 0;
    qreal rowGap = 0;
    qreal columnGap = 0;

    // Grid properties
    QStringList gridTemplateColumns;
    QStringList gridTemplateRows;
    QStringList gridTemplateAreas;

    // Child layout properties (for items within a flex/grid container)
    qreal flexGrow = 0;
    qreal flexShrink = 1;
    qreal flexBasis = -1; // -1 = auto
    int alignSelf = -1;   // -1 = auto, otherwise IRAlignItems
    int gridColumnStart = 0;
    int gridColumnEnd = 0;
    int gridRowStart = 0;
    int gridRowEnd = 0;
    QString gridArea;

    // Constraints (for absolute positioning relative to parent)
    bool useConstraints = false;
    qreal left = 0, right = 0, top = 0, bottom = 0;
    qreal centerX = 0, centerY = 0;
    bool hasLeft = false, hasRight = false, hasTop = false, hasBottom = false;
    bool hasCenterX = false, hasCenterY = false;

    QJsonObject toJson() const;
    static IRLayout fromJson(const QJsonObject& obj);
};

// ============================================================================
// Style System
// ============================================================================

struct IRColor {
    // Supports solid, gradient, or style reference
    enum class Type { Solid, LinearGradient, RadialGradient, StyleRef, None };
    Type type = Type::None;

    // Solid
    QColor solidColor;

    // Gradient
    struct GradientStop {
        qreal offset;      // 0.0 - 1.0
        QColor color;
        QJsonObject toJson() const;
        static GradientStop fromJson(const QJsonObject& obj);
    };
    QList<GradientStop> gradientStops;
    qreal gradientAngle = 0;           // Linear gradient angle in degrees
    QPointF gradientCenter = QPointF(0.5, 0.5); // Radial gradient center
    qreal gradientRadius = 0.5;        // Radial gradient radius

    // Style reference
    QString styleRef;

    QJsonObject toJson() const;
    static IRColor fromJson(const QJsonObject& obj);
    bool isValid() const { return type != Type::None; }
    bool isTransparent() const { return type == Type::Solid && solidColor.alpha() == 0; }
};

enum class IRBorderStyle { None, Solid, Dashed, Dotted, Double };

struct IRBorder {
    IRColor color;
    qreal width = 0;
    IRBorderStyle style = IRBorderStyle::Solid;
    QJsonObject toJson() const;
    static IRBorder fromJson(const QJsonObject& obj);
};

struct IRShadow {
    qreal offsetX = 0;
    qreal offsetY = 0;
    qreal blurRadius = 0;
    qreal spreadRadius = 0;
    IRColor color;
    bool inset = false;

    QJsonObject toJson() const;
    static IRShadow fromJson(const QJsonObject& obj);
};

enum class IRTextAlign { Left, Center, Right, Justify };
enum class IRVerticalAlign { Top, Middle, Bottom };
enum class IRTextOverflow { Clip, Ellipsis, Visible };
enum class IRWhiteSpace { Normal, NoWrap, Pre, PreWrap, PreLine };

struct IRTextStyle {
    QString fontFamily;
    int fontSize = 14;              // In pixels
    qreal fontWeight = 400;         // 100-900
    bool italic = false;
    bool underline = false;
    bool strikethrough = false;
    IRColor color;
    IRTextAlign align = IRTextAlign::Left;
    IRVerticalAlign verticalAlign = IRVerticalAlign::Middle;
    qreal lineHeight = 1.2;         // Multiplier
    qreal letterSpacing = 0;
    IRTextOverflow overflow = IRTextOverflow::Clip;
    IRWhiteSpace whiteSpace = IRWhiteSpace::Normal;
    bool wordWrap = true;

    QJsonObject toJson() const;
    static IRTextStyle fromJson(const QJsonObject& obj);
};

// ============================================================================
// Effects & Transforms
// ============================================================================

struct IRTransform {
    qreal translateX = 0;
    qreal translateY = 0;
    qreal scaleX = 1;
    qreal scaleY = 1;
    qreal rotation = 0;             // Degrees
    qreal skewX = 0;
    qreal skewY = 0;
    QPointF transformOrigin = QPointF(0.5, 0.5); // Relative to bounds

    QJsonObject toJson() const;
    static IRTransform fromJson(const QJsonObject& obj);
    bool isIdentity() const;
};

struct IRFilter {
    enum class Type { Blur, Brightness, Contrast, Grayscale, HueRotate, Invert, Opacity, Saturate, Sepia, DropShadow, Custom };
    Type type;
    QVariant value; // Type-dependent value

    QJsonObject toJson() const;
    static IRFilter fromJson(const QJsonObject& obj);
};

struct IREffects {
    qreal opacity = 1.0;
    QList<IRShadow> shadows;
    QList<IRFilter> filters;
    IRColor backgroundColor;
    IRBorder border;
    int borderRadius = 0;           // Uniform radius
    QList<int> borderRadii;         // [tl, tr, br, bl] for individual corners
    IRTransform transform;
    QString clipPath;               // SVG path or basic shape
    QString mask;                   // Mask reference

    QJsonObject toJson() const;
    static IREffects fromJson(const QJsonObject& obj);
};

// ============================================================================
// Assets
// ============================================================================

enum class IRAssetType { Image, Font, Video, Audio, Data, Other };

struct IRAsset {
    IRNodeId id;
    QString name;
    IRAssetType type = IRAssetType::Image;
    QString mimeType;
    QString filePath;               // Relative to project assets folder
    QString originalPath;           // Original import path
    qint64 fileSize = 0;
    int width = 0;                  // For images/video
    int height = 0;
    QString colorFormat;            // e.g., "RGBA8888", "RGB565"
    QMap<QString, QString> metadata; // Format-specific metadata
    IRSourceProvenance provenance;

    QJsonObject toJson() const;
    static IRAsset fromJson(const QJsonObject& obj);
};

// ============================================================================
// Animations
// ============================================================================

enum class IREasingType {
    Linear,
    EaseIn, EaseOut, EaseInOut,
    EaseInQuad, EaseOutQuad, EaseInOutQuad,
    EaseInCubic, EaseOutCubic, EaseInOutCubic,
    EaseInQuart, EaseOutQuart, EaseInOutQuart,
    EaseInQuint, EaseOutQuint, EaseInOutQuint,
    EaseInSine, EaseOutSine, EaseInOutSine,
    EaseInExpo, EaseOutExpo, EaseInOutExpo,
    EaseInCirc, EaseOutCirc, EaseInOutCirc,
    EaseInBack, EaseOutBack, EaseInOutBack,
    EaseInElastic, EaseOutElastic, EaseInOutElastic,
    EaseInBounce, EaseOutBounce, EaseInOutBounce,
    CustomBezier
};

struct IREasing {
    IREasingType type = IREasingType::EaseInOut;
    // For CustomBezier: x1, y1, x2, y2
    QList<qreal> bezierPoints;

    QJsonObject toJson() const;
    static IREasing fromJson(const QJsonObject& obj);
};

enum class IRAnimationDirection { Normal, Reverse, Alternate, AlternateReverse };
enum class IRAnimationFillMode { None, Forwards, Backwards, Both };

struct IRKeyframe {
    qreal offset;                   // 0.0 - 1.0
    QJsonObject properties;         // Animated properties at this keyframe
    IREasing easing;                // Easing to next keyframe

    QJsonObject toJson() const;
    static IRKeyframe fromJson(const QJsonObject& obj);
};

struct IRAnimation {
    IRNodeId id;
    QString name;
    IRNodeId targetNodeId;          // Component/element being animated
    QString propertyPath;           // e.g., "properties.opacity", "layout.x"
    QList<IRKeyframe> keyframes;
    int duration = 300;             // Milliseconds
    int delay = 0;
    int iterationCount = 1;         // -1 = infinite
    IRAnimationDirection direction = IRAnimationDirection::Normal;
    IRAnimationFillMode fillMode = IRAnimationFillMode::Forwards;
    IREasing easing;                // Default easing between keyframes
    bool isPlaying = false;

    QJsonObject toJson() const;
    static IRAnimation fromJson(const QJsonObject& obj);
};

// ============================================================================
// Events & Interactions
// ============================================================================

enum class IREventType {
    Click, DoubleClick, LongPress,
    Press, Release,
    HoverEnter, HoverLeave,
    Focus, Blur,
    ValueChange,
    SwipeLeft, SwipeRight, SwipeUp, SwipeDown,
    DragStart, DragMove, DragEnd,
    KeyPress, KeyRelease,
    ScreenEnter, ScreenLeave,
    AnimationStart, AnimationEnd, AnimationIteration,
    Custom
};

struct IRInteraction {
    IRNodeId id;
    IRNodeId sourceNodeId;          // Component that triggers
    IREventType trigger;
    QString targetNodeId;           // Component affected (empty = self)
    QString action;                 // "show", "hide", "toggle", "navigate", "setValue", "callFunction", "custom"
    QJsonObject actionParams;       // Action-specific parameters
    QJsonObject conditions;         // Optional conditions
    bool enabled = true;

    QJsonObject toJson() const;
    static IRInteraction fromJson(const QJsonObject& obj);
};

// ============================================================================
// Data Binding
// ============================================================================

enum class IRBindingDirection { Read, Write, ReadWrite };

struct IRBinding {
    IRNodeId id;
    IRNodeId componentId;
    QString propertyPath;           // e.g., "properties.text", "style.opacity"
    QString sourceId;               // Data source ID
    IRBindingDirection direction = IRBindingDirection::Read;
    QString transformExpression;    // Optional expression to transform value
    QString validatorExpression;    // Optional validation expression

    QJsonObject toJson() const;
    static IRBinding fromJson(const QJsonObject& obj);
};

enum class IRDataSourceType { Variable, Sensor, Network, Hardware, System, Custom };
enum class IRDataDirection { Input, Output, Bidirectional };
enum class IRDataType { String, Number, Boolean, Object, Array, Color, Image };

struct IRDataSource {
    IRNodeId id;
    QString name;
    IRDataSourceType type = IRDataSourceType::Variable;
    IRDataDirection direction = IRDataDirection::Input;
    IRDataType dataType = IRDataType::String;
    QString hardwareRef;            // e.g., "ADC1_IN0", "GPIO_PA5"
    QVariant initialValue;
    QJsonObject config;             // Type-specific configuration
    int updateInterval = 0;         // 0 = event-driven, >0 = polling ms

    QJsonObject toJson() const;
    static IRDataSource fromJson(const QJsonObject& obj);
};

// ============================================================================
// Component Definitions (Widget Types)
// ============================================================================

struct IRComponentProperty {
    QString name;
    QString displayName;
    QString type;                   // "string", "number", "boolean", "color", "font", "asset", "enum", "array", "object"
    QVariant defaultValue;
    bool required = false;
    bool readOnly = false;
    QStringList enumValues;         // For enum type
    QString description;
    QString group;                  // Property group for UI organization
    int order = 0;

    QJsonObject toJson() const;
    static IRComponentProperty fromJson(const QJsonObject& obj);
};

struct IRComponentDefinition {
    IRNodeId id;
    QString name;                   // Display name
    QString category;               // "basic", "input", "display", "container", "chart", "custom"
    QString description;
    QString icon;                   // Icon identifier
    QList<IRComponentProperty> properties;
    QList<QString> supportedStates; // e.g., "normal", "hover", "pressed", "disabled", "focused"
    QList<QString> supportedEvents; // e.g., "click", "valueChange"
    bool isContainer = false;       // Can contain child components
    QJsonObject defaultProperties;
    IRSourceProvenance provenance;

    QJsonObject toJson() const;
    static IRComponentDefinition fromJson(const QJsonObject& obj);
};

// ============================================================================
// Component Instance (Widget in a Screen)
// ============================================================================

struct IRWidget {
    IRNodeId id;
    QString componentType;          // References IRComponentDefinition.id
    QString name;                   // Human-readable name

    // Layout
    IRLayout layout;

    // Style & Appearance
    IREffects effects;
    IRTextStyle textStyle;

    // Properties (component-specific, dynamic)
    QJsonObject properties;

    // States (per-component state overrides)
    QMap<QString, QJsonObject> stateOverrides; // stateName -> property overrides

    // Data bindings
    QList<IRBinding> bindings;

    // Interactions
    QList<IRInteraction> interactions;

    // Animations targeting this widget
    QList<IRAnimation> animations;

    // Children (for container components)
    QList<IRWidget> children;

    // Component label / text (optional, for buttons, labels, etc.)
    QString label;
    QJsonObject customProperties;

    // Provenance
    IRSourceProvenance provenance;

    QJsonObject toJson() const;
    QJsonObject toJsonObject() const { return toJson(); }
    static IRWidget fromJson(const QJsonObject& obj);
    static IRWidget fromJsonObject(const QJsonObject& json, QString* outError = nullptr);
};

// ============================================================================
// Screen
// ============================================================================

struct IRScreen {
    IRNodeId id;
    QString name;
    int width = 320;
    int height = 240;
    IRColor backgroundColor;
    IRLayout rootLayout;            // Layout for root container
    QList<IRWidget> widgets;
    QList<qreal> hGuides;
    QList<qreal> vGuides;
    IRSourceProvenance provenance;

    QJsonObject toJson() const;
    QJsonObject toJsonObject() const { return toJson(); }
    static IRScreen fromJson(const QJsonObject& obj);
    static IRScreen fromJsonObject(const QJsonObject& json, QString* outError = nullptr);
};

// ============================================================================
// Project (Root IR Document)
// ============================================================================

struct IRProject {
    IRVersion version;
    IRNodeId id;
    QString name;
    QString displayName;
    QString description;
    QString targetFramework;        // "qt-for-mcus", "lvgl", "ugfx", "flutter", etc.
    QString targetFamily;           // "STM32", "ESP32", "RP2040", "Desktop"
    QString targetBoard;            // Specific board ID

    // Target profile / hardware limits
    struct TargetProfile {
        QString mcu;
        QString framework;
        qint64 maxFlash = 0;
        qint64 maxRam = 0;
        QJsonObject toJson() const;
        static TargetProfile fromJson(const QJsonObject& obj);
    };
    TargetProfile targetProfile;

    // Display configuration
    int displayWidth = 320;
    int displayHeight = 240;
    bool isRound = false;
    int colorDepth = 16;
    QString colorFormat = "RGB565";

    // Color styles (project-wide)
    struct IRColorStyle {
        QString name;
        IRColor color;
        QString description;
        QJsonObject toJson() const;
        static IRColorStyle fromJson(const QJsonObject& obj);
    };
    QList<IRColorStyle> colorStyles;

    // Component library (custom components)
    QList<IRComponentDefinition> componentLibrary;

    // Screens
    QList<IRScreen> screens;
    IRNodeId activeScreenId;

    // Data sources & bindings
    QList<IRDataSource> dataSources;
    QList<IRBinding> bindings;

    // Assets
    QList<IRAsset> assets;

    // Global animations
    QList<IRAnimation> globalAnimations;

    // Metadata
    QDateTime createdAt;
    QDateTime modifiedAt;
    QString author;
    QMap<QString, QString> metadata; // Free-form metadata

    QString toJson() const;
    QJsonObject toJsonObject() const;
    static IRProject fromJson(const QString& json, QString* outError = nullptr);
    static IRProject fromJsonObject(const QJsonObject& json, QString* outError = nullptr);
    static IRProject fromJson(const QJsonObject& obj);

    // Validation
    IRDiagnostics validate() const;
    IRDiagnostics validateAgainstCapabilities(const class IRTargetCapabilities& caps) const;
};

// ============================================================================
// Capability System
// ============================================================================

enum class IRCapabilityLevel {
    Supported,          // Fully supported natively
    Lowered,            // Supported via controlled lowering (with known transformations)
    Approximate,        // Supported with visible difference (warning emitted)
    Unsupported,        // Not supported at all (error emitted)
    Unknown             // Capability not yet evaluated
};

struct IRCapabilityEntry {
    QString propertyPath;           // e.g., "widgets[].properties.borderRadius", "layout.flexDirection"
    QString componentType;          // Empty = applies to all components
    IRCapabilityLevel level = IRCapabilityLevel::Unknown;
    QString reason;                 // Human-readable explanation
    QString alternative;            // Suggested alternative if unsupported/approximate
    QJsonObject constraints;        // Additional constraints (e.g., max value, supported enums)

    QJsonObject toJson() const;
    static IRCapabilityEntry fromJson(const QJsonObject& obj);
};

class IRTargetCapabilities {
public:
    QString targetId;               // e.g., "lvgl-v9", "ugfx", "qt-for-mcus-2.5"
    QString targetName;
    QString targetVersion;
    QString vendor;
    QStringList supportedFrameworks; // Frameworks this capability profile applies to

    // Display capabilities
    int maxWidth = 0;               // 0 = unlimited
    int maxHeight = 0;
    QStringList supportedColorFormats; // "RGB565", "RGB888", "ARGB8888", "Mono"
    int maxColorDepth = 32;
    bool supportsRoundDisplay = false;

    // Font & text shaping
    bool supportsCustomFonts = true;
    bool supportsFontFallback = false;
    bool supportsComplexTextShaping = false; // HarfBuzz, etc.
    int maxFontSize = 0;            // 0 = unlimited
    QStringList supportedFontFormats; // "TTF", "OTF", "BDF", "LVGL_FONT"

    // Asset constraints
    int maxImageWidth = 0;
    int maxImageHeight = 0;
    int maxImageSizeBytes = 0;
    QStringList supportedImageFormats; // "PNG", "JPEG", "BMP", "LVGL_RAW"
    bool supportsVectorGraphics = false;

    // Layout features
    bool supportsFlexLayout = false;
    bool supportsGridLayout = false;
    bool supportsAbsoluteLayout = true;
    bool supportsStackLayout = false;
    bool supportsConstraints = false;

    // Style features
    bool supportsGradients = false;
    bool supportsLinearGradients = false;
    bool supportsRadialGradients = false;
    bool supportsOpacity = true;
    bool supportsBorderRadius = true;
    int maxBorderRadius = 0;
    bool supportsBoxShadow = false;
    bool supportsFilters = false;
    bool supportsBlendModes = false;
    bool supportsClipPath = false;
    bool supportsMask = false;
    bool supportsTransforms = false;
    bool supportsTransform3D = false;

    // Animation features
    bool supportsAnimations = false;
    bool supportsKeyframeAnimations = false;
    bool supportsSpringAnimations = false;
    int maxConcurrentAnimations = 0;
    QStringList supportedEasingFunctions;

    // Input/Interaction
    bool supportsTouch = false;
    bool supportsMouse = false;
    bool supportsKeyboard = false;
    bool supportsEncoder = false;
    bool supportsGesture = false;

    // Runtime requirements
    QString minRuntimeVersion;
    QStringList requiredModules;    // e.g., "lvgl_freetype", "ugdx_gwin"
    QStringList optionalModules;
    qint64 minFlashBytes = 0;
    qint64 minRamBytes = 0;

    // Licensing
    QString license;                // "MIT", "Apache-2.0", "GPL-3.0", "Commercial", etc.
    QString licenseUrl;
    bool allowsStaticLinking = true;
    bool allowsCommercialUse = true;

    // Detailed capability matrix
    QList<IRCapabilityEntry> capabilities;

    // Lookup
    IRCapabilityLevel getCapabilityLevel(const QString& componentType, const QString& propertyPath) const;
    void setCapability(const QString& componentType, const QString& propertyPath, IRCapabilityLevel level,
                       const QString& reason = QString(), const QString& alternative = QString(),
                       const QJsonObject& constraints = QJsonObject());

    QJsonObject toJson() const;
    static IRTargetCapabilities fromJson(const QJsonObject& obj);

    // Built-in profiles
    static IRTargetCapabilities lvglProfile();
    static IRTargetCapabilities ugfxProfile();
    static IRTargetCapabilities qtForMCUProfile();
    static IRTargetCapabilities flutterProfile();
    static IRTargetCapabilities htmlCanvasProfile();
};

} // namespace IR

// Q_DECLARE_METATYPE for Qt container support
Q_DECLARE_METATYPE(IR::IRNodeId)
Q_DECLARE_METATYPE(IR::IRVersion)
Q_DECLARE_METATYPE(IR::IRSourceProvenance)
Q_DECLARE_METATYPE(IR::IRDiagnostic)
Q_DECLARE_METATYPE(IR::IRLayout)
Q_DECLARE_METATYPE(IR::IRColor)
Q_DECLARE_METATYPE(IR::IRBorder)
Q_DECLARE_METATYPE(IR::IRShadow)
Q_DECLARE_METATYPE(IR::IRTextStyle)
Q_DECLARE_METATYPE(IR::IRTransform)
Q_DECLARE_METATYPE(IR::IRFilter)
Q_DECLARE_METATYPE(IR::IREffects)
Q_DECLARE_METATYPE(IR::IRAsset)
Q_DECLARE_METATYPE(IR::IREasing)
Q_DECLARE_METATYPE(IR::IRKeyframe)
Q_DECLARE_METATYPE(IR::IRAnimation)
Q_DECLARE_METATYPE(IR::IRInteraction)
Q_DECLARE_METATYPE(IR::IRBinding)
Q_DECLARE_METATYPE(IR::IRDataSource)
Q_DECLARE_METATYPE(IR::IRComponentProperty)
Q_DECLARE_METATYPE(IR::IRComponentDefinition)
Q_DECLARE_METATYPE(IR::IRWidget)
Q_DECLARE_METATYPE(IR::IRScreen)
Q_DECLARE_METATYPE(IR::IRProject)
Q_DECLARE_METATYPE(IR::IRTargetCapabilities)
Q_DECLARE_METATYPE(IR::IRCapabilityEntry)