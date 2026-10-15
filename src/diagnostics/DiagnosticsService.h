#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVector>
#include <functional>

class Project;

namespace Diagnostics {

/**
 * @brief A single user-facing problem surfaced in the Problems panel.
 */
struct Diagnostic {
    enum class Severity { Error = 0, Warning = 1, Info = 2 };

    Severity severity = Severity::Info;
    QString message;
    QString source;       // e.g. "Resources", "Export", "Binding", "Layout", "Asset"
    QString screenId;     // screen this diagnostic belongs to (may be empty)
    QString componentId;  // component this diagnostic belongs to (may be empty)
};

/**
 * @brief Aggregates diagnostics from any number of registered sources.
 *
 * Sources are plain callbacks evaluated against the current project, so new
 * features (backend compatibility, IR validation, build errors, ...) can add
 * diagnostics without changing this class or the Problems panel.
 */
class DiagnosticsService : public QObject {
    Q_OBJECT

public:
    using SourceCallback = std::function<QVector<Diagnostic>(const Project*)>;

    explicit DiagnosticsService(QObject* parent = nullptr);

    void setProject(const Project* project);
    const Project* project() const { return m_project; }

    void registerSource(const QString& name, SourceCallback callback);
    void clearSources();
    QStringList sourceNames() const;

    QVector<Diagnostic> runNow();
    const QVector<Diagnostic>& diagnostics() const { return m_diagnostics; }

    int errorCount() const;
    int warningCount() const;
    int infoCount() const;
    int totalCount() const { return m_diagnostics.size(); }

    static QString severityLabel(Diagnostic::Severity severity);

public slots:
    void scheduleRefresh(int delayMs = 250);

signals:
    void diagnosticsChanged(const QVector<Diagnostic>& diagnostics);

private:
    struct RegisteredSource {
        QString name;
        SourceCallback callback;
    };

    QVector<RegisteredSource> m_sources;
    QVector<Diagnostic> m_diagnostics;
    const Project* m_project = nullptr;
    QTimer* m_refreshTimer = nullptr;
};

} // namespace Diagnostics
