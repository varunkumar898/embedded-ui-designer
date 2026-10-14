#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>

class Project;

namespace Diagnostics {

enum class Category {
    Project,
    Component,
    Binding,
    Hardware,
    Generator,
    CICD,
    AI,
    MCP,
    Plugin
};

enum class Severity {
    Info,
    Warning,
    Error
};

QString categoryToString(Category cat);
Category stringToCategory(const QString& str);

QString severityToString(Severity sev);
Severity stringToSeverity(const QString& str);

struct DiagnosticItem {
    QString id;
    Category category = Category::Project;
    Severity severity = Severity::Info;
    QString title;
    QString message;
    QString sourceId;       // e.g. component ID, screen ID, pin name
    QString fixSuggestion;  // Actionable suggestion
    QDateTime timestamp = QDateTime::currentDateTime();

    QJsonObject toJson() const;
    static DiagnosticItem fromJson(const QJsonObject& obj);
};

class DiagnosticsEngine : public QObject {
    Q_OBJECT

public:
    static DiagnosticsEngine& instance();

    void addDiagnostic(const DiagnosticItem& item);
    void addDiagnostic(Category cat, Severity sev, const QString& title, const QString& msg,
                       const QString& sourceId = QString(), const QString& fix = QString());

    void clearCategory(Category cat);
    void clearAll();

    QList<DiagnosticItem> allDiagnostics() const;
    QList<DiagnosticItem> diagnosticsForCategory(Category cat) const;
    QList<DiagnosticItem> diagnosticsForSeverity(Severity sev) const;

    int errorCount() const;
    int warningCount() const;

    // Full Project Scan
    void scanProject(const Project* project);

    // Serialization for AI and UI
    QJsonObject toJson() const;
    QString toMarkdown() const;

signals:
    void diagnosticsChanged();
    void diagnosticAdded(const DiagnosticItem& item);

private:
    explicit DiagnosticsEngine(QObject* parent = nullptr);
    ~DiagnosticsEngine() override = default;
    DiagnosticsEngine(const DiagnosticsEngine&) = delete;
    DiagnosticsEngine& operator=(const DiagnosticsEngine&) = delete;

    QList<DiagnosticItem> m_items;
};

} // namespace Diagnostics
