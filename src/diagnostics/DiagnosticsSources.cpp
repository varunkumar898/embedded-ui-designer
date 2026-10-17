#include "DiagnosticsSources.h"
#include "DiagnosticsService.h"
#include "Project.h"
#include "Screen.h"
#include "UIComponent.h"
#include "ImageComponent.h"
#include "analyzer/ResourceAnalysis.h"
#include "analyzer/ResourceEstimator.h"
#include "analyzer/FramebufferFit.h"
#include "models/BoardDisplayConfig.h"
#include "codegen/BackendRegistry.h"
#include "codegen/GeneratorContext.h"
#include "models/DiagnosticsEngine.h"
#include "driverpack/DriverPackRegistry.h"
#include "hardware/BoardRegistry.h"

#include <QFileInfo>

namespace Diagnostics {
namespace DiagnosticsSources {

namespace {

Diagnostic::Severity mapWarningSeverity(ResourceAnalysis::WarningSeverity severity) {
    switch (severity) {
        case ResourceAnalysis::WarningSeverity::Critical: return Diagnostic::Severity::Error;
        case ResourceAnalysis::WarningSeverity::Warning:  return Diagnostic::Severity::Warning;
        case ResourceAnalysis::WarningSeverity::Info:     return Diagnostic::Severity::Info;
    }
    return Diagnostic::Severity::Info;
}

Diagnostic::Severity mapValidationLevel(CodeGen::ValidationMessage::Level level) {
    switch (level) {
        case CodeGen::ValidationMessage::Error:   return Diagnostic::Severity::Error;
        case CodeGen::ValidationMessage::Warning: return Diagnostic::Severity::Warning;
        case CodeGen::ValidationMessage::Info:    return Diagnostic::Severity::Info;
    }
    return Diagnostic::Severity::Info;
}

Diagnostic::Severity mapEngineSeverity(Diagnostics::Severity severity) {
    switch (severity) {
        case Diagnostics::Severity::Error:   return Diagnostic::Severity::Error;
        case Diagnostics::Severity::Warning: return Diagnostic::Severity::Warning;
        case Diagnostics::Severity::Info:    return Diagnostic::Severity::Info;
    }
    return Diagnostic::Severity::Info;
}

QString effectiveFramework(const Project* project) {
    QString framework = project ? project->targetFramework().toLower() : QString();
    if (framework.isEmpty()) framework = QStringLiteral("lvgl");
    return framework;
}

// One estimator instance for all refreshes so its estimation cache stays warm.
ResourceEstimator& sharedEstimator() {
    static ResourceEstimator estimator;
    return estimator;
}

QString screenContaining(const Project* project, const QString& componentId) {
    if (!project || componentId.isEmpty()) return QString();
    for (Screen* screen : project->screens()) {
        if (screen && screen->findComponentById(componentId)) {
            return screen->id();
        }
    }
    return QString();
}

QVector<Diagnostic> resourcesSource(const Project* project) {
    QVector<Diagnostic> out;
    if (!project) return out;

    ResourceEstimator& estimator = sharedEstimator();
    const ResourceAnalysis analysis =
        estimator.analyzeProject(project, project->getDeviceProfile(), effectiveFramework(project));

    for (const auto& warning : analysis.structuredWarnings) {
        Diagnostic diagnostic;
        diagnostic.severity = mapWarningSeverity(warning.severity);
        diagnostic.message = warning.message;
        diagnostic.source = QStringLiteral("Resources");
        out.append(diagnostic);
    }
    return out;
}

QVector<Diagnostic> assetSource(const Project* project) {
    QVector<Diagnostic> out;
    if (!project) return out;

    for (Screen* screen : project->screens()) {
        if (!screen) continue;
        for (UIComponent* component : screen->components()) {
            if (!component || component->componentType() != QStringLiteral("Image")) continue;
            auto* image = dynamic_cast<ImageComponent*>(component);
            if (!image) continue;

            const QString path = image->imagePath();
            if (path.isEmpty() || QFileInfo::exists(path)) continue;

            Diagnostic diagnostic;
            diagnostic.severity = Diagnostic::Severity::Error;
            diagnostic.message = QString("Missing asset file: %1").arg(path);
            diagnostic.source = QStringLiteral("Asset");
            diagnostic.screenId = screen->id();
            diagnostic.componentId = component->componentId();
            out.append(diagnostic);
        }
    }
    return out;
}

QVector<Diagnostic> bindingSource(const Project* project) {
    QVector<Diagnostic> out;
    if (!project) return out;

    CodeGen::GeneratorContext context(project);
    QList<CodeGen::ValidationMessage> messages;
    context.validate(&messages);

    for (const auto& message : messages) {
        if (!message.message.startsWith(QStringLiteral("DataBinding"))) continue;

        Diagnostic diagnostic;
        diagnostic.severity = mapValidationLevel(message.level);
        diagnostic.message = message.message;
        diagnostic.source = QStringLiteral("Binding");
        diagnostic.screenId = message.screenId;
        diagnostic.componentId = message.componentId;
        if (diagnostic.screenId.isEmpty()) {
            diagnostic.screenId = screenContaining(project, diagnostic.componentId);
        }
        out.append(diagnostic);
    }
    return out;
}

QVector<Diagnostic> layoutSource(const Project* project) {
    QVector<Diagnostic> out;
    if (!project) return out;

    auto& engine = DiagnosticsEngine::instance();
    engine.scanProject(project);

    for (const auto& item : engine.diagnosticsForCategory(Category::Component)) {
        if (item.title != QStringLiteral("Out of Screen Bounds")) continue;

        Diagnostic diagnostic;
        diagnostic.severity = mapEngineSeverity(item.severity);
        diagnostic.message = item.message;
        diagnostic.source = QStringLiteral("Layout");
        diagnostic.componentId = item.sourceId;
        diagnostic.screenId = screenContaining(project, item.sourceId);
        out.append(diagnostic);
    }
    return out;
}

// Task 5: buffering / wiring diagnostics.
//  * Error   - the chosen interface needs an on-chip LCD controller the MCU lacks.
//  * Warning - the estimated framebuffer does not fit framebuffer-capable RAM.
//  * Info    - RAM region suitability is not documented (never a hard error).
// The "flash depends on board" Info stays in the Resources source; it is not
// duplicated here.
QVector<Diagnostic> boardDisplaySource(const Project* project) {
    QVector<Diagnostic> out;
    if (!project) return out;

    const DeviceProfile device = project->getDeviceProfile();
    if (device.name.isEmpty()) return out;

    const BoardDisplayConfig board = project->boardDisplayConfig();
    const DisplayConfig display = project->displayConfig();

    if (!board.interfaceId.isEmpty() &&
        BoardDisplayConfig::interfaceRequiresLcdController(board.interfaceId) &&
        device.has_lcd_controller == 0) {
        Diagnostic diagnostic;
        diagnostic.severity = Diagnostic::Severity::Error;
        diagnostic.message = QString("%1 requires an on-chip LCD controller, but %2 does not provide one.")
                                 .arg(board.interfaceId, device.name);
        diagnostic.source = QStringLiteral("BoardDisplay");
        out.append(diagnostic);
    }

    if (device.ram_kb == 0) return out;

    const FramebufferFit::Result fit = FramebufferFit::evaluate(
        device, display.width, display.height, display.colorDepth, board);

    if (fit.status == FramebufferFit::Result::Status::DoesNotFit) {
        Diagnostic diagnostic;
        diagnostic.severity = Diagnostic::Severity::Warning;
        diagnostic.message = QString("Framebuffer: %1 %2").arg(fit.message, fit.hint);
        diagnostic.source = QStringLiteral("BoardDisplay");
        out.append(diagnostic);
    } else if (fit.regionSuitabilityUnknown) {
        Diagnostic diagnostic;
        diagnostic.severity = Diagnostic::Severity::Info;
        diagnostic.message = QString("Framebuffer (%1): %2")
                                 .arg(device.name, fit.hint);
        diagnostic.source = QStringLiteral("BoardDisplay");
        out.append(diagnostic);
    }
    return out;
}

QVector<Diagnostic> backendSource(const Project* project) {
    QVector<Diagnostic> out;
    if (!project) return out;

    const QList<BackendFinding> findings = BackendRegistry::validate(project);
    for (const BackendFinding& finding : findings) {
        Diagnostic diagnostic;
        switch (finding.severity) {
        case BackendFindingSeverity::Error:
            diagnostic.severity = Diagnostic::Severity::Error;
            break;
        case BackendFindingSeverity::Warning:
            diagnostic.severity = Diagnostic::Severity::Warning;
            break;
        case BackendFindingSeverity::Info:
            diagnostic.severity = Diagnostic::Severity::Info;
            break;
        }
        diagnostic.message = finding.message;
        diagnostic.source = QStringLiteral("Backend");
        diagnostic.screenId = finding.screenId;
        diagnostic.componentId = finding.componentId;
        out.append(diagnostic);
    }
    return out;
}

QVector<Diagnostic> driverPackSource(const Project* project) {
    QVector<Diagnostic> out;
    if (!project) return out;

    QString boardId = project->hardwareConfig().boardId.trimmed();
    QString deviceId = project->hardwareConfig().deviceId.trimmed();
    QString devName;
    if (!boardId.isEmpty()) {
        BoardDef bd = BoardRegistry::instance().board(boardId);
        if (bd.isValid()) {
            devName = !bd.mcu.isEmpty() ? bd.mcu : boardId;
        } else {
            devName = boardId;
        }
    } else if (!deviceId.isEmpty()) {
        devName = deviceId;
    } else {
        devName = project->getDeviceProfile().name.trimmed();
    }

    if (devName.isEmpty()) {
        Diagnostic diagnostic;
        diagnostic.severity = Diagnostic::Severity::Info;
        diagnostic.message = QStringLiteral("No device selected; using default STM32F030R8 baseline.");
        diagnostic.source = QStringLiteral("DriverPack");
        out.append(diagnostic);
        return out;
    }

    auto* pack = driverpack::DriverPackRegistry::instance().findPackForDevice(devName.toStdString());
    if (!pack) {
        Diagnostic diagnostic;
        diagnostic.severity = Diagnostic::Severity::Error;
        diagnostic.message = QString("No driver pack for %1; import one").arg(devName);
        diagnostic.source = QStringLiteral("DriverPack");
        out.append(diagnostic);
    }
    return out;
}

} // namespace

void registerBuiltinSources(DiagnosticsService& service) {
    service.registerSource(QStringLiteral("Resources"), resourcesSource);
    service.registerSource(QStringLiteral("Asset"), assetSource);
    service.registerSource(QStringLiteral("Binding"), bindingSource);
    service.registerSource(QStringLiteral("Layout"), layoutSource);
    service.registerSource(QStringLiteral("BoardDisplay"), boardDisplaySource);
    service.registerSource(QStringLiteral("Backend"), backendSource);
    service.registerSource(QStringLiteral("DriverPack"), driverPackSource);
}

} // namespace DiagnosticsSources
} // namespace Diagnostics
