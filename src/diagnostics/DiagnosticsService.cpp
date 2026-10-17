#include "DiagnosticsService.h"
#include "Project.h"

#include <algorithm>

namespace Diagnostics {

DiagnosticsService::DiagnosticsService(QObject* parent)
    : QObject(parent) {
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    m_refreshTimer->setInterval(250);
    connect(m_refreshTimer, &QTimer::timeout, this, [this]() {
        runNow();
    });
}

void DiagnosticsService::setProject(const Project* project) {
    m_project = project;
    scheduleRefresh(0);
}

void DiagnosticsService::registerSource(const QString& name, SourceCallback callback) {
    m_sources.append({ name, std::move(callback) });
}

void DiagnosticsService::clearSources() {
    m_sources.clear();
}

QStringList DiagnosticsService::sourceNames() const {
    QStringList names;
    for (const auto& source : m_sources) {
        names << source.name;
    }
    return names;
}

QVector<Diagnostic> DiagnosticsService::runNow() {
    m_refreshTimer->stop();

    QVector<Diagnostic> collected;
    for (const auto& source : m_sources) {
        if (!source.callback) continue;
        collected += source.callback(m_project);
    }

    std::stable_sort(collected.begin(), collected.end(),
                     [](const Diagnostic& a, const Diagnostic& b) {
                         return static_cast<int>(a.severity) < static_cast<int>(b.severity);
                     });

    m_diagnostics = collected;
    emit diagnosticsChanged(m_diagnostics);
    return m_diagnostics;
}

int DiagnosticsService::errorCount() const {
    return std::count_if(m_diagnostics.begin(), m_diagnostics.end(),
                         [](const Diagnostic& d) { return d.severity == Diagnostic::Severity::Error; });
}

int DiagnosticsService::warningCount() const {
    return std::count_if(m_diagnostics.begin(), m_diagnostics.end(),
                         [](const Diagnostic& d) { return d.severity == Diagnostic::Severity::Warning; });
}

int DiagnosticsService::infoCount() const {
    return std::count_if(m_diagnostics.begin(), m_diagnostics.end(),
                         [](const Diagnostic& d) { return d.severity == Diagnostic::Severity::Info; });
}

QString DiagnosticsService::severityLabel(Diagnostic::Severity severity) {
    switch (severity) {
        case Diagnostic::Severity::Error:   return QStringLiteral("Error");
        case Diagnostic::Severity::Warning: return QStringLiteral("Warning");
        case Diagnostic::Severity::Info:    return QStringLiteral("Info");
    }
    return QStringLiteral("Info");
}

void DiagnosticsService::scheduleRefresh(int delayMs) {
    m_refreshTimer->start(delayMs);
}

} // namespace Diagnostics
