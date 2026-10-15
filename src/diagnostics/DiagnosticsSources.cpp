#include "DiagnosticsSources.h"
#include "DiagnosticsService.h"
#include "Project.h"
#include "Screen.h"
#include "UIComponent.h"
#include "ImageComponent.h"
#include "analyzer/ResourceAnalysis.h"
#include "analyzer/ResourceEstimator.h"
#include "codegen/GeneratorContext.h"
#include "models/DiagnosticsEngine.h"

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

} // namespace

void registerBuiltinSources(DiagnosticsService& service) {
    service.registerSource(QStringLiteral("Resources"), resourcesSource);
    service.registerSource(QStringLiteral("Asset"), assetSource);
    service.registerSource(QStringLiteral("Binding"), bindingSource);
    service.registerSource(QStringLiteral("Layout"), layoutSource);
}

} // namespace DiagnosticsSources
} // namespace Diagnostics
