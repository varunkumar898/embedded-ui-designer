#pragma once

#include "AIProvider.h"
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <functional>

class DesignerController;
class Project;

namespace AI {

struct ToolDefinition {
    QString name;
    QString description;
    QJsonObject parameters; // JSON schema
    std::function<ToolResult(const QJsonObject& args, DesignerController* controller, Project* project)> handler;
    bool isHardwareWrite = false;
};

class AIToolRegistry : public QObject {
    Q_OBJECT

public:
    static AIToolRegistry& instance();

    void registerTool(const ToolDefinition& tool);
    const ToolDefinition* findTool(const QString& name) const;
    QList<ToolDefinition> allTools() const;

    // Universal schemas for providers
    QJsonArray universalTools() const;
    QJsonArray openAITools() const;
    QJsonArray anthropicTools() const;
    QJsonArray geminiTools() const;

    // Execution via DesignerController / Project (atomic, undoable)
    ToolResult executeTool(const ToolCall& call, DesignerController* controller, Project* project, bool userConfirmedHardwareWrite = false);

    // Hardware write confirmation policy
    bool requiresHardwareConfirmation(const ToolCall& call) const;

signals:
    void toolExecuted(const QString& name, bool success);

private:
    explicit AIToolRegistry(QObject* parent = nullptr);
    ~AIToolRegistry() override = default;
    AIToolRegistry(const AIToolRegistry&) = delete;
    AIToolRegistry& operator=(const AIToolRegistry&) = delete;

    void registerStandardTools();

    QMap<QString, ToolDefinition> m_tools;
};

} // namespace AI
