#include "MockAIProvider.h"
#include <QTimer>
#include <QElapsedTimer>

namespace AI {

MockAIProvider::MockAIProvider(QObject* parent)
    : AIProvider(parent)
{
    m_apiKey = "mock-api-key";
    m_currentModel = "mock-model-v1";
    m_cachedModels = {"mock-model-v1", "mock-pro-fast", "mock-ultra-vision"};
}

AIProviderCapabilities MockAIProvider::capabilities() const {
    AIProviderCapabilities caps;
    caps.textInput = true;
    caps.textOutput = true;
    caps.imageInput = true;
    caps.imageOutput = false;
    caps.streaming = true;
    caps.toolCalling = true;
    caps.structuredOutput = true;
    caps.longContext = true;
    caps.conversationState = true;
    return caps;
}

void MockAIProvider::listModels(std::function<void(const QStringList& models, const QString& error)> callback) {
    if (!m_simulatedError.isEmpty()) {
        callback({}, m_simulatedError);
        return;
    }
    callback(m_cachedModels, QString());
}

void MockAIProvider::sendMessage(const QList<AIMessage>& messages,
                                const QJsonArray& tools,
                                std::function<void(const AIMessage& response, const AIUsage& usage, const QString& error)> callback)
{
    Q_UNUSED(tools);
    m_isCancelled = false;

    if (!m_simulatedError.isEmpty()) {
        callback(AIMessage(), AIUsage(), m_simulatedError);
        return;
    }

    AIUsage usage;
    usage.promptTokens = 25;
    usage.completionTokens = 45;
    usage.totalTokens = 70;
    usage.latencyMs = 12;
    usage.costEstimate = "$0.000";

    AIMessage resp(AIRole::Assistant);

    if (!m_simulatedToolCalls.isEmpty()) {
        resp.setToolCalls(m_simulatedToolCalls);
        resp.setTextContent(m_simulatedResponseText.isEmpty() ? "Executing requested tool action." : m_simulatedResponseText);
    } else if (!m_simulatedResponseText.isEmpty()) {
        resp.setTextContent(m_simulatedResponseText);
    } else {
        // Auto-reply based on last message
        QString lastText = messages.isEmpty() ? "" : messages.last().textContent();
        resp.setTextContent(QString("Mock AI response to: %1").arg(lastText));
    }

    callback(resp, usage, QString());
}

void MockAIProvider::streamMessage(const QList<AIMessage>& messages,
                                  const QJsonArray& tools,
                                  std::function<void(const QString& deltaText, const QList<ToolCall>& toolCalls, bool finished, const AIUsage& usage, const QString& error)> callback)
{
    Q_UNUSED(messages);
    Q_UNUSED(tools);
    m_isCancelled = false;

    if (!m_simulatedError.isEmpty()) {
        callback("", {}, true, AIUsage(), m_simulatedError);
        return;
    }

    QString text = m_simulatedResponseText.isEmpty() ? "Mock AI streaming completion." : m_simulatedResponseText;
    QStringList chunks = text.split(" ", Qt::SkipEmptyParts);

    AIUsage usage;
    usage.promptTokens = 20;
    usage.completionTokens = text.length() / 4;
    usage.totalTokens = usage.promptTokens + usage.completionTokens;
    usage.latencyMs = 25;

    for (int i = 0; i < chunks.size(); ++i) {
        if (m_isCancelled) {
            callback("", {}, true, usage, "Request cancelled");
            return;
        }
        QString chunk = chunks[i] + (i + 1 < chunks.size() ? " " : "");
        callback(chunk, {}, false, usage, QString());
    }

    // Send final completion with any tool calls
    callback("", m_simulatedToolCalls, true, usage, QString());
}

void MockAIProvider::testConnection(std::function<void(const ConnectionTestResult& result)> callback) {
    ConnectionTestResult res;
    res.status = m_simulatedStatus;
    res.detectedModels = m_cachedModels;

    switch (m_simulatedStatus) {
        case ConnectionStatus::Connected:
            res.message = "Successfully connected to Mock AI Provider.";
            break;
        case ConnectionStatus::AuthenticationFailed:
            res.message = "Mock authentication failed (invalid key).";
            break;
        case ConnectionStatus::InvalidModel:
            res.message = "Mock model not found.";
            break;
        case ConnectionStatus::RateLimited:
            res.message = "Mock provider rate limit exceeded.";
            break;
        case ConnectionStatus::NetworkError:
            res.message = "Mock provider network timeout.";
            break;
        case ConnectionStatus::ProviderUnavailable:
            res.message = "Mock provider service unavailable.";
            break;
        case ConnectionStatus::NotConfigured:
            res.message = "Mock provider not configured.";
            break;
    }

    callback(res);
}

} // namespace AI
