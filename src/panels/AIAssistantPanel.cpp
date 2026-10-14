#include "AIAssistantPanel.h"
#include "AIProviderRegistry.h"
#include "AIToolRegistry.h"
#include "AIProjectContext.h"
#include "DiagnosticsEngine.h"
#include "AIProposedChangesDialog.h"
#include "AISettingsDialog.h"
#include "DesignerController.h"
#include "Project.h"
#include "HardwareManager.h"
#include "HardwareBridge.h"
#include "theme/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QTextBrowser>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QUndoStack>
#include <QJsonDocument>

namespace AI {

AIAssistantPanel::AIAssistantPanel(DesignerController* controller, Project* project, QUndoStack* undoStack, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_project(project)
    , m_undoStack(undoStack)
{
    setupUI();
}

void AIAssistantPanel::setProject(Project* project) {
    m_project = project;
}

void AIAssistantPanel::setUndoStack(QUndoStack* undoStack) {
    m_undoStack = undoStack;
}

void AIAssistantPanel::setupUI() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);

    // Top control bar: Provider & Model selector + Settings
    auto topBar = new QHBoxLayout();
    topBar->addWidget(new QLabel("Provider:", this));

    m_providerCombo = new QComboBox(this);
    auto pids = AIProviderRegistry::instance().providerIds();
    for (const auto& pid : pids) {
        auto p = AIProviderRegistry::instance().getProvider(pid);
        m_providerCombo->addItem(p ? p->displayName() : pid, pid);
    }
    topBar->addWidget(m_providerCombo);

    topBar->addWidget(new QLabel("Model:", this));
    m_modelCombo = new QComboBox(this);
    m_modelCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    topBar->addWidget(m_modelCombo);

    m_settingsButton = new QPushButton("⚙️", this);
    m_settingsButton->setToolTip("Configure AI Provider Keys & Models");
    m_settingsButton->setFixedWidth(30);
    connect(m_settingsButton, &QPushButton::clicked, this, &AIAssistantPanel::onOpenSettings);
    topBar->addWidget(m_settingsButton);

    m_clearButton = new QPushButton("🗑️", this);
    m_clearButton->setToolTip("Clear Conversation History");
    m_clearButton->setFixedWidth(30);
    connect(m_clearButton, &QPushButton::clicked, this, &AIAssistantPanel::onClearChat);
    topBar->addWidget(m_clearButton);

    mainLayout->addLayout(topBar);

    // Chat History Viewer
    m_chatHistory = new QTextBrowser(this);
    m_chatHistory->setOpenExternalLinks(true);
    m_chatHistory->document()->setDefaultStyleSheet(QString(
        "body { font-family: sans-serif; font-size: 13px; line-height: 1.4; }"
        ".user { background-color: %1; color: %2; padding: 8px 12px; border-radius: %3px; margin: 4px 0; }"
        ".assistant { background-color: %4; color: %5; padding: 8px 12px; border-radius: %3px; margin: 4px 0; }"
        ".tool { background-color: %6; color: %7; font-family: monospace; font-size: 11px; padding: 6px; border-left: 3px solid %8; margin: 3px 0; }"
        ".error { background-color: %9; color: %10; padding: 6px; border-radius: 4px; margin: 3px 0; }"
        "code { background-color: %11; padding: 2px 4px; border-radius: 3px; font-family: monospace; }"
    ).arg(Theme::BgRaised, Theme::TextPrimary)
     .arg(Theme::RadiusLg)
     .arg(Theme::BgSurface, Theme::TextSecondary, Theme::BgBase, Theme::Info, Theme::Accent)
     .arg(Theme::DangerBg, Theme::Danger, Theme::BgOverlay));
    mainLayout->addWidget(m_chatHistory, 1);

    // Input area + Send/Stop buttons
    auto inputLayout = new QHBoxLayout();
    m_inputEdit = new QTextEdit(this);
    m_inputEdit->setMaximumHeight(70);
    m_inputEdit->setPlaceholderText("Ask AI Assistant (e.g., 'Add a speedometer for car speed', 'Why is binding broken?', 'Set temperature to 75')...");
    inputLayout->addWidget(m_inputEdit, 1);

    auto btnLayout = new QVBoxLayout();
    m_sendButton = new QPushButton("Send", this);
    m_sendButton->setStyleSheet(QString("QPushButton { background-color: %1; color: %2; font-weight: 600; padding: 6px 14px; border-radius: %3px; }").arg(Theme::Accent, Theme::BgBase).arg(Theme::RadiusSm));
    connect(m_sendButton, &QPushButton::clicked, this, &AIAssistantPanel::onSendMessage);
    btnLayout->addWidget(m_sendButton);

    m_stopButton = new QPushButton("Stop", this);
    m_stopButton->setEnabled(false);
    connect(m_stopButton, &QPushButton::clicked, this, &AIAssistantPanel::onStopGeneration);
    btnLayout->addWidget(m_stopButton);

    inputLayout->addLayout(btnLayout);
    mainLayout->addLayout(inputLayout);

    connect(m_providerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AIAssistantPanel::onProviderChanged);
    connect(m_modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AIAssistantPanel::onModelChanged);

    // Initial state
    QString defId = AIProviderRegistry::instance().defaultProviderId();
    int idx = m_providerCombo->findData(defId);
    if (idx != -1) {
        m_providerCombo->setCurrentIndex(idx);
    }
    updateModelList();
}

void AIAssistantPanel::onProviderChanged(int index) {
    if (index < 0) return;
    QString pid = m_providerCombo->itemData(index).toString();
    AIProviderRegistry::instance().setDefaultProviderId(pid);
    updateModelList();
}

void AIAssistantPanel::updateModelList() {
    m_modelCombo->blockSignals(true);
    m_modelCombo->clear();

    auto provider = AIProviderRegistry::instance().activeProvider();
    if (provider) {
        QStringList models = provider->cachedModels();
        if (models.isEmpty()) {
            models = QStringList() << provider->currentModel();
        }
        m_modelCombo->addItems(models);
        int curIdx = m_modelCombo->findText(provider->currentModel());
        if (curIdx != -1) {
            m_modelCombo->setCurrentIndex(curIdx);
        }
    }
    m_modelCombo->blockSignals(false);
}

void AIAssistantPanel::onModelChanged(int index) {
    if (index < 0) return;
    auto provider = AIProviderRegistry::instance().activeProvider();
    if (provider) {
        provider->setModel(m_modelCombo->currentText());
    }
}

void AIAssistantPanel::onOpenSettings() {
    AISettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        updateModelList();
    }
}

void AIAssistantPanel::onClearChat() {
    m_conversation.clear();
    m_chatHistory->clear();
    appendChatMessage(AIRole::System, "Conversation cleared. Ready for instructions.");
}

void AIAssistantPanel::appendChatMessage(AIRole role, const QString& text) {
    QString html;
    if (role == AIRole::User) {
        html = QString("<div class='user'><b>You:</b><br>%1</div>").arg(text.toHtmlEscaped().replace("\n", "<br>"));
    } else if (role == AIRole::Assistant) {
        html = QString("<div class='assistant'><b>AI Assistant:</b><br>%1</div>").arg(text);
    } else if (role == AIRole::System) {
        html = QString("<div style='color: %1; font-size: 11px; margin: 4px 0;'><i>%2</i></div>").arg(Theme::TextMuted, text.toHtmlEscaped());
    } else if (role == AIRole::Tool) {
        html = QString("<div class='tool'>%1</div>").arg(text);
    }
    m_chatHistory->append(html);
}

void AIAssistantPanel::updateStreamingText(const QString& delta) {
    m_currentAssistantStream += delta;
    // Replace current streaming view
    m_chatHistory->undo(); // Undo previous stream frame if desired, or append
    m_chatHistory->append(QString("<div class='assistant'><b>AI Assistant:</b><br>%1</div>").arg(m_currentAssistantStream.toHtmlEscaped().replace("\n", "<br>")));
}

void AIAssistantPanel::onStopGeneration() {
    auto provider = AIProviderRegistry::instance().activeProvider();
    if (provider) {
        provider->cancelRequest();
    }
    m_isStreaming = false;
    m_sendButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    appendChatMessage(AIRole::System, "Generation stopped by user.");
}

void AIAssistantPanel::onSendMessage() {
    QString userText = m_inputEdit->toPlainText().trimmed();
    if (userText.isEmpty()) return;

    auto provider = AIProviderRegistry::instance().activeProvider();
    if (!provider) {
        QMessageBox::warning(this, "AI Provider Error", "No active AI provider selected.");
        return;
    }

    if (!provider->isConfigured()) {
        QMessageBox::warning(this, "AI Provider Not Configured",
            QString("Please configure the API Key for %1 in Settings (⚙️).").arg(provider->displayName()));
        onOpenSettings();
        return;
    }

    m_inputEdit->clear();
    appendChatMessage(AIRole::User, userText);

    // Build scoped context for initial message
    if (m_conversation.messages().isEmpty()) {
        QString sysPrompt = "You are an intelligent embedded UI co-pilot in Embedded UI Designer.\n";
        sysPrompt += "You can inspect project state, generate components, manage DataSources, create bindings, and run simulations.\n";
        if (m_project) {
            sysPrompt += "\n" + AIProjectContext::formatScreenContextAsMarkdown(m_project);
            sysPrompt += "\n" + Diagnostics::DiagnosticsEngine::instance().toMarkdown();
        }
        m_conversation.addMessage(AIMessage(AIRole::System, sysPrompt));
    }

    m_conversation.addMessage(AIMessage(AIRole::User, userText));

    m_isStreaming = true;
    m_sendButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_currentAssistantStream.clear();

    // Prepare tools schema
    QJsonArray tools = AIToolRegistry::instance().universalTools();

    // Streaming call
    provider->streamMessage(
        m_conversation.messages(),
        tools,
        [this](const QString& deltaText, const QList<ToolCall>& toolCalls, bool finished, const AIUsage& usage, const QString& error) {
            if (!error.isEmpty()) {
                m_isStreaming = false;
                m_sendButton->setEnabled(true);
                m_stopButton->setEnabled(false);
                m_chatHistory->append(QString("<div class='error'><b>Error:</b> %1</div>").arg(error.toHtmlEscaped()));
                return;
            }

            if (!deltaText.isEmpty()) {
                m_currentAssistantStream += deltaText;
            }

            if (finished) {
                m_isStreaming = false;
                m_sendButton->setEnabled(true);
                m_stopButton->setEnabled(false);

                if (!m_currentAssistantStream.isEmpty()) {
                    appendChatMessage(AIRole::Assistant, m_currentAssistantStream.toHtmlEscaped().replace("\n", "<br>"));
                    AIMessage assistMsg(AIRole::Assistant, m_currentAssistantStream);
                    assistMsg.setToolCalls(toolCalls);
                    m_conversation.addMessage(assistMsg);
                }

                if (!toolCalls.isEmpty()) {
                    executeToolCalls(toolCalls);
                }
            }
        }
    );
}

void AIAssistantPanel::executeToolCalls(const QList<ToolCall>& toolCalls) {
    if (toolCalls.isEmpty()) return;

    // Phase 6H: AI Change Preview before executing modifications
    bool hasModifications = false;
    for (const auto& call : toolCalls) {
        if (call.name == "create_component" || call.name == "create_data_source" ||
            call.name == "create_binding" || call.name == "move_component" ||
            call.name == "delete_component") {
            hasModifications = true;
            break;
        }
    }

    if (hasModifications || toolCalls.size() > 1) {
        AIProposedChangesDialog previewDlg(toolCalls, this);
        if (previewDlg.exec() != QDialog::Accepted) {
            appendChatMessage(AIRole::System, "AI Proposed Changes were rejected by user.");
            return;
        }
    }

    // Phase 6I: Group related AI operations into single undoable transaction
    if (m_undoStack) {
        m_undoStack->beginMacro("AI Assistant Changes");
    }

    bool allSucceeded = true;
    QString lastError;

    for (const auto& call : toolCalls) {
        appendChatMessage(AIRole::Tool, QString("🔧 Executing Tool: <b>%1</b>(%2)")
            .arg(call.name, QString::fromUtf8(QJsonDocument(call.arguments).toJson(QJsonDocument::Compact))));

        // Phase 6J: Real Hardware write confirmation gate
        bool confirmed = true;
        if (AIToolRegistry::instance().requiresHardwareConfirmation(call)) {
            confirmed = confirmHardwareAction(call);
            if (!confirmed) {
                appendChatMessage(AIRole::Tool, QString("🚫 Operation cancelled: Hardware write permission denied."));
                allSucceeded = false;
                lastError = "User rejected hardware write";
                break;
            }
        }

        ToolResult res = AIToolRegistry::instance().executeTool(call, m_controller, m_project, confirmed);
        if (!res.error.isEmpty()) {
            allSucceeded = false;
            lastError = res.error;
            appendChatMessage(AIRole::Tool, QString("<font color='%1'>❌ Failed: %2</font>").arg(Theme::Danger, res.error.toHtmlEscaped()));
            break;
        } else {
            appendChatMessage(AIRole::Tool, QString("<font color='%1'>✔️ Success: %2</font>").arg(Theme::Success, res.result.toHtmlEscaped()));
        }
    }

    // Phase 6I: Rollback if any sub-operation fails
    if (m_undoStack) {
        m_undoStack->endMacro();
        if (!allSucceeded) {
            m_undoStack->undo();
            appendChatMessage(AIRole::System, "Transaction failed: State rolled back to avoid partial modifications.");
        }
    }
}

bool AIAssistantPanel::confirmHardwareAction(const ToolCall& call) {
    QString board = m_project ? m_project->hardwareConfig().boardId : "Connected MCU";
    if (board.isEmpty()) board = "Physical MCU";

    QString opText = QString("%1: %2").arg(call.name, QString::fromUtf8(QJsonDocument(call.arguments).toJson(QJsonDocument::Compact)));

    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle("REAL HARDWARE WRITE CONFIRMATION");
    box.setText(QString("<b>REAL HARDWARE WRITE DETECTED</b><br><br>"
                        "<b>Target:</b> %1<br>"
                        "<b>Operation:</b> %2<br><br>"
                        "Allow AI Assistant to execute this physical write?").arg(board, opText));
    auto* allowBtn = box.addButton("ALLOW ONCE", QMessageBox::AcceptRole);
    auto* cancelBtn = box.addButton("CANCEL", QMessageBox::RejectRole);
    box.setDefaultButton(cancelBtn);

    box.exec();
    return box.clickedButton() == allowBtn;
}

} // namespace AI
