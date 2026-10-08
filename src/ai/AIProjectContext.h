#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>

class Project;
class Screen;
class UIComponent;

namespace AI {

class AIProjectContext {
public:
    AIProjectContext() = default;

    // Scoped context extraction
    static QJsonObject buildScopedScreenContext(const Project* project, const QString& screenName = QString());
    static QString formatScreenContextAsMarkdown(const Project* project, const QString& screenName = QString());

    // Hardware & Diagnostics context
    static QJsonObject buildHardwareContext(const Project* project);
    static QJsonObject buildDiagnosticsContext(const Project* project);

    // Security & Sanitization
    static QString sanitizeSecrets(const QString& text);
    static QJsonObject sanitizeJsonObject(const QJsonObject& obj);
    static QJsonArray sanitizeJsonArray(const QJsonArray& arr);
};

} // namespace AI
