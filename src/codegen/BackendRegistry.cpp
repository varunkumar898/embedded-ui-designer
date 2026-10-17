#include "BackendRegistry.h"

#include "Project.h"
#include "Screen.h"
#include "UIComponent.h"

#include <QHash>

namespace {

struct ComponentInfo {
    const char* type;
    const char* displayName;
};

// Canonical component types as persisted by the component constructors, with
// the human readable name used in messages.
const QList<ComponentInfo>& componentCatalog() {
    static const QList<ComponentInfo> catalog = {
        { "Button", "Button" },
        { "Text", "Label" },
        { "Rectangle", "Rectangle" },
        { "Circle", "Circle" },
        { "Path", "Path" },
        { "Image", "Image" },
        { "ProgressBar", "Progress Bar" },
        { "Slider", "Slider" },
        { "Switch", "Switch" },
        { "Checkbox", "Checkbox" },
        { "TextInput", "Text Input" },
        { "CircularProgress", "Circular Progress" },
        { "Gauge", "Gauge" },
        { "Speedometer", "Speedometer" },
        { "Battery", "Battery" },
        { "Pressure", "Pressure" },
        { "RPM", "RPM" },
        { "Temperature", "Temperature" },
        { "TabView", "Tab View" },
        { "NavigationBar", "Navigation Bar" },
        { "List", "List" },
        { "Table", "Table" },
        { "Group", "Group" },
        { "CustomInstance", "Custom component" },
    };
    return catalog;
}

// Lower-case aliases accepted for robustness with legacy/hand-edited files.
QString canonicalAlias(const QString& lower) {
    static const QHash<QString, QString> aliases = {
        { "label", "Text" },
        { "text", "Text" },
        { "progress", "ProgressBar" },
        { "linearprogress", "ProgressBar" },
        { "circular_progress", "CircularProgress" },
        { "speedo", "Speedometer" },
        { "rpmgauge", "RPM" },
        { "navigationbar", "NavigationBar" },
        { "tabview", "TabView" },
        { "custom", "CustomInstance" },
        { "customcomponent", "CustomInstance" },
        { "custom_instance", "CustomInstance" },
    };
    return aliases.value(lower);
}

QString canonicalType(const QString& rawType) {
    const QString trimmed = rawType.trimmed();
    if (trimmed.isEmpty()) return QString();
    const QString lower = trimmed.toLower();
    for (const ComponentInfo& info : componentCatalog()) {
        if (QString::fromLatin1(info.type).toLower() == lower) {
            return QString::fromLatin1(info.type);
        }
    }
    return canonicalAlias(lower);
}

BackendComponentSupport makeSupport(const QString& type, BackendSupportLevel level, const QString& note = QString()) {
    BackendComponentSupport support;
    support.componentType = type;
    support.displayName = BackendRegistry::componentDisplayName(type);
    support.level = level;
    support.note = note;
    return support;
}

BackendComponentSupport makeSupported(const QString& type) {
    return makeSupport(type, BackendSupportLevel::Supported);
}

BackendComponentSupport makePartial(const QString& type, const QString& note) {
    return makeSupport(type, BackendSupportLevel::Partial, note);
}

BackendComponentSupport makeUnsupported(const QString& type) {
    return makeSupport(type, BackendSupportLevel::Unsupported);
}

// Components the three C/QML generators emit through their dynamic_cast
// chains (Button, Label, Rectangle, Circle, Path, ProgressBar, Slider,
// Switch, Checkbox, TextInput).
const QStringList& classicSupportedTypes() {
    static const QStringList types = {
        "Button", "Text", "Rectangle", "Circle", "Path",
        "ProgressBar", "Slider", "Switch", "Checkbox", "TextInput",
    };
    return types;
}

const QStringList& valueAndContainerTypes() {
    static const QStringList types = {
        "CircularProgress", "Gauge", "Speedometer", "Battery", "Pressure",
        "RPM", "Temperature", "TabView", "NavigationBar", "List", "Table", "Group",
    };
    return types;
}

QList<BackendComponentSupport> classicComponentMatrix(bool imageSupported) {
    QList<BackendComponentSupport> components;
    for (const QString& type : classicSupportedTypes()) {
        components.append(makeSupported(type));
    }
    if (imageSupported) {
        components.append(makeSupported("Image"));
    } else {
        components.append(makePartial(
            "Image",
            "a handle is declared but the widget is never created in the generated UI."));
    }
    for (const QString& type : valueAndContainerTypes()) {
        components.append(makeUnsupported(type));
    }
    return components;
}

QList<BackendComponentSupport> qtMcuComponentMatrix() {
    QList<BackendComponentSupport> components;
    for (const QString& type : classicSupportedTypes()) {
        components.append(makeSupported(type));
    }
    components.append(makeSupported("Image"));
    for (const QString& type : valueAndContainerTypes()) {
        components.append(makePartial(
            type,
            "component is not natively supported in Qt Quick Ultralite core; "
            "a placeholder rectangle is exported instead."));
    }
    return components;
}

QList<BackendComponentSupport> embedWebComponentMatrix() {
    QList<BackendComponentSupport> components;
    // The IR hydrator strictly accepts container/rectangle/label/button/
    // progressbar/image and aborts generation for anything else.
    static const QStringList supported = { "Rectangle", "Text", "Button", "ProgressBar", "Image", "Group" };
    for (const QString& type : supported) {
        components.append(makeSupported(type));
    }
    static const QStringList unsupported = {
        "Slider", "Switch", "Checkbox", "TextInput", "Circle", "Path",
        "CircularProgress", "Gauge", "Speedometer", "Battery", "Pressure",
        "RPM", "Temperature", "TabView", "NavigationBar", "List", "Table" };
    for (const QString& type : unsupported) {
        components.append(makeUnsupported(type));
    }
    return components;
}

QList<BackendDescriptor> builtinBackends() {
    QList<BackendDescriptor> list;

    BackendDescriptor lvgl;
    lvgl.id = "lvgl";
    lvgl.displayName = "LVGL";
    lvgl.status = "Stable";
    lvgl.description = "LVGL C library generator (ui.h/ui.c, lv_conf.h, CMake, PC simulator).";
    lvgl.frameworkId = "lvgl";
    lvgl.pixelFormats = { "RGB565", "RGB888" };
    lvgl.supportsAnimations = true;
    lvgl.animationsNote = "Value widgets use LV_ANIM_ON; prototype slide/dissolve transitions "
                          "remain theme-controlled (emitted as a comment only).";
    lvgl.unsupportedPolicy = BackendUnsupportedPolicy::OmitWarning;
    lvgl.components = classicComponentMatrix(/*imageSupported=*/false);
    list.append(lvgl);

    BackendDescriptor ugfx;
    ugfx.id = "ugfx";
    ugfx.displayName = "µGFX";
    ugfx.status = "Stable";
    ugfx.description = "µGFX C generator (gui.h/gui.c, gfxconf.h, CMake, PC simulator).";
    ugfx.frameworkId = "ugfx";
    ugfx.pixelFormats = { "RGB565", "RGB888" };
    ugfx.supportsAnimations = false;
    ugfx.animationsNote = "The generated code contains no animation calls.";
    ugfx.unsupportedPolicy = BackendUnsupportedPolicy::OmitWarning;
    ugfx.components = classicComponentMatrix(/*imageSupported=*/false);
    list.append(ugfx);

    BackendDescriptor qtmcu;
    qtmcu.id = "qt-for-mcus";
    qtmcu.displayName = "Qt for MCUs (QUL)";
    qtmcu.status = "Stable";
    qtmcu.description = "Qt for MCUs (QUL) QML generator (screen .qml, qmlproject, CMake, PC simulator).";
    qtmcu.frameworkId = "qt-for-mcus";
    qtmcu.pixelFormats = { "RGB565", "RGB888" };
    qtmcu.supportsAnimations = true;
    qtmcu.animationsNote = "Prototype transitions are emitted as Behavior/NumberAnimation.";
    qtmcu.unsupportedPolicy = BackendUnsupportedPolicy::PlaceholderWarning;
    qtmcu.components = qtMcuComponentMatrix();
    list.append(qtmcu);

    BackendDescriptor embedweb;
    embedweb.id = "embedweb";
    embedweb.displayName = "EmbedWeb";
    embedweb.status = "Experimental";
    embedweb.description = "Bounded-memory native C runtime with its own IR-based generator and "
                           "HAL adapters (STM32, RP2040, ESP32, NXP, host).";
    embedweb.frameworkId = "embedweb";
    embedweb.pixelFormats = { "RGB565" };
    embedweb.supportsAnimations = false;
    embedweb.animationsNote = "Runtime policy renders statically; animations are unsupported.";
    embedweb.unsupportedPolicy = BackendUnsupportedPolicy::HardError;
    embedweb.components = embedWebComponentMatrix();
    list.append(embedweb);

    BackendDescriptor native;
    native.id = "native";
    native.displayName = "Native Runtime (planned)";
    native.status = "Planned";
    native.description = "First-party native C runtime backend. Not implemented yet.";
    native.frameworkId = QString();
    native.selectable = false;
    native.disabledTooltip = "Not implemented yet.";
    list.append(native);

    return list;
}

QList<BackendDescriptor>& storage() {
    static QList<BackendDescriptor> backends = builtinBackends();
    return backends;
}

int indexOfId(const QString& id) {
    const QList<BackendDescriptor>& list = storage();
    for (int i = 0; i < list.size(); ++i) {
        if (list.at(i).id == id) return i;
    }
    return -1;
}

} // namespace

QList<BackendDescriptor> BackendRegistry::backends() {
    return storage();
}

bool BackendRegistry::registerBackend(const BackendDescriptor& backend) {
    if (backend.id.trimmed().isEmpty()) return false;
    if (indexOfId(backend.id) >= 0) return false;
    storage().append(backend);
    return true;
}

bool BackendRegistry::unregisterBackend(const QString& id) {
    const int index = indexOfId(id);
    if (index < 0) return false;
    storage().removeAt(index);
    return true;
}

bool BackendRegistry::contains(const QString& id) {
    return indexOfId(id) >= 0;
}

BackendDescriptor BackendRegistry::backend(const QString& id) {
    const int index = indexOfId(id);
    if (index < 0) return BackendDescriptor();
    return storage().at(index);
}

BackendDescriptor BackendRegistry::backendForFramework(const QString& targetFramework) {
    // Must mirror FirmwareExportManager's generator dispatch so that the
    // selected backend is exactly the generator that would run.
    const QString fw = targetFramework.toLower();
    if (fw.contains("ugfx")) return backend(QStringLiteral("ugfx"));
    if (fw.contains("qtmcu") || fw.contains("qul") || fw.contains("qt")) {
        return backend(QStringLiteral("qt-for-mcus"));
    }
    if (fw.contains("embedweb")) return backend(QStringLiteral("embedweb"));
    return backend(QStringLiteral("lvgl"));
}

QString BackendRegistry::componentDisplayName(const QString& componentType) {
    const QString canonical = canonicalType(componentType);
    if (canonical.isEmpty()) {
        const QString trimmed = componentType.trimmed();
        return trimmed.isEmpty() ? QStringLiteral("Unknown component") : trimmed;
    }
    for (const ComponentInfo& info : componentCatalog()) {
        if (QString::fromLatin1(info.type) == canonical) {
            return QString::fromLatin1(info.displayName);
        }
    }
    return canonical;
}

BackendComponentSupport BackendRegistry::supportFor(const QString& backendId, const QString& componentType) {
    BackendComponentSupport result;
    const QString canonical = canonicalType(componentType);
    result.componentType = canonical.isEmpty() ? componentType.trimmed() : canonical;
    result.displayName = componentDisplayName(result.componentType);
    result.level = BackendSupportLevel::Unknown;

    const BackendDescriptor descriptor = backend(backendId);
    if (descriptor.id.isEmpty()) return result;
    for (const BackendComponentSupport& support : descriptor.components) {
        if (support.componentType == result.componentType) {
            return support;
        }
    }
    // Undocumented component types stay Unknown on purpose.
    return result;
}

QList<BackendFinding> BackendRegistry::validate(const Project* project, const QString& backendId) {
    QList<BackendFinding> findings;
    if (!project) return findings;

    const BackendDescriptor descriptor = backendId.isEmpty()
        ? backendForFramework(project->targetFramework())
        : backend(backendId);
    if (descriptor.id.isEmpty() || !descriptor.selectable) return findings;

    for (Screen* screen : project->screens()) {
        if (!screen) continue;
        for (UIComponent* component : screen->components()) {
            if (!component) continue;

            const BackendComponentSupport support = supportFor(descriptor.id, component->componentType());
            if (support.level == BackendSupportLevel::Supported) continue;

            BackendFinding finding;
            finding.backendId = descriptor.id;
            finding.componentType = support.componentType;
            finding.componentDisplayName = support.displayName;
            finding.screenId = screen->id();
            finding.componentId = component->componentId();
            finding.level = support.level;

            switch (support.level) {
            case BackendSupportLevel::Supported:
                continue;
            case BackendSupportLevel::Partial:
                finding.severity = BackendFindingSeverity::Warning;
                finding.message = QString("%1 is not fully supported by %2 backend: %3")
                                      .arg(support.displayName, descriptor.displayName,
                                           support.note.isEmpty()
                                               ? QStringLiteral("only part of the widget is generated.")
                                               : support.note);
                break;
            case BackendSupportLevel::Unsupported:
                switch (descriptor.unsupportedPolicy) {
                case BackendUnsupportedPolicy::HardError:
                    finding.severity = BackendFindingSeverity::Error;
                    finding.message = QString("%1 is not supported by %2 backend; export cannot be generated.")
                                          .arg(support.displayName, descriptor.displayName);
                    break;
                case BackendUnsupportedPolicy::PlaceholderWarning:
                    finding.severity = BackendFindingSeverity::Warning;
                    finding.message = QString("%1 is not supported by %2 backend; a placeholder rectangle is exported instead.")
                                          .arg(support.displayName, descriptor.displayName);
                    break;
                case BackendUnsupportedPolicy::OmitWarning:
                    finding.severity = BackendFindingSeverity::Warning;
                    finding.message = QString("%1 is not supported by %2 backend and will be omitted from the generated UI.")
                                          .arg(support.displayName, descriptor.displayName);
                    break;
                }
                break;
            case BackendSupportLevel::Unknown:
                finding.severity = BackendFindingSeverity::Info;
                finding.message = QString("Support for component type '%1' is not documented for %2 backend; support cannot be confirmed.")
                                      .arg(finding.componentType, descriptor.displayName);
                break;
            }

            findings.append(finding);
        }
    }

    return findings;
}
