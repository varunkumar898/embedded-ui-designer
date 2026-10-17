#pragma once

#include <QWidget>
#include <QString>
#include <QList>
#include "analyzer/ai/AIProvider.h"

class QComboBox;
class QTextBrowser;
class QTextEdit;
class QPushButton;
class QLabel;
class DesignerController;
class Project;
class QUndoStack;

namespace AI {

class AIAssistantPanel : public QWidget {
    Q_OBJECT

public:
    explicit AIAssistantPanel(DesignerController* controller, Project* project, QUndoStack* undoStack, QWidget* parent = nullptr);
    ~AIAssistantPanel() override = default;

    void setProject(Project* project);
    void setUndoStack(QUndoStack* undoStack);

private slots:
    void onProviderChanged(int index);
    void onModelChanged(int index);
    void onSendMessage();
    void onStopGeneration();
    void onClearChat();
    void onOpenSettings();

private:
    void setupUI();
    void updateModelList();
    void appendChatMessage(AIRole role, const QString& text);
    void updateStreamingText(const QString& delta);
    void executeToolCalls(const QList<ToolCall>& toolCalls);
    bool confirmHardwareAction(const ToolCall& call);
    void streamWithProvider(std::shared_ptr<AIProvider> provider, const QJsonArray& tools, bool isFallbackAttempt);

    DesignerController* m_controller = nullptr;
    Project* m_project = nullptr;
    QUndoStack* m_undoStack = nullptr;

    QComboBox* m_providerCombo = nullptr;
    QComboBox* m_modelCombo = nullptr;
    QPushButton* m_settingsButton = nullptr;
    QPushButton* m_clearButton = nullptr;

    QTextBrowser* m_chatHistory = nullptr;
    QTextEdit* m_inputEdit = nullptr;
    QPushButton* m_sendButton = nullptr;
    QPushButton* m_stopButton = nullptr;

    AIConversation m_conversation;
    bool m_isStreaming = false;
    QString m_currentAssistantStream;
};

} // namespace AI
