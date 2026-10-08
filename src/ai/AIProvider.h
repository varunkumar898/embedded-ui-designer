#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QList>
#include <QDateTime>
#include <functional>

namespace AI {

enum class AIRole {
    System,
    User,
    Assistant,
    Tool
};

QString roleToString(AIRole role);
AIRole stringToRole(const QString& str);

struct AIProviderCapabilities {
    bool textInput = true;
    bool textOutput = true;
    bool imageInput = false;
    bool imageOutput = false;
    bool streaming = true;
    bool toolCalling = true;
    bool structuredOutput = true;
    bool longContext = true;
    bool conversationState = true;
};

struct ToolCall {
    QString id;
    QString name;
    QJsonObject arguments;
};

struct ToolResult {
    QString id;
    QString result;
    QString error;
};

struct AIMessageContent {
    QString type = "text"; // "text" or "image"
    QString text;
    QByteArray imageData;
    QString imageMimeType = "image/png";
};

class AIMessage {
public:
    AIMessage() = default;
    explicit AIMessage(AIRole role, const QString& textContent = QString());

    AIRole role() const { return m_role; }
    void setRole(AIRole role) { m_role = role; }

    QString textContent() const;
    void setTextContent(const QString& text);

    const QList<AIMessageContent>& contents() const { return m_contents; }
    void addContent(const AIMessageContent& content) { m_contents.append(content); }
    void addImageContent(const QByteArray& data, const QString& mimeType = "image/png");

    const QList<ToolCall>& toolCalls() const { return m_toolCalls; }
    void setToolCalls(const QList<ToolCall>& calls) { m_toolCalls = calls; }
    void addToolCall(const ToolCall& call) { m_toolCalls.append(call); }

    const QList<ToolResult>& toolResults() const { return m_toolResults; }
    void setToolResults(const QList<ToolResult>& results) { m_toolResults = results; }
    void addToolResult(const ToolResult& result) { m_toolResults.append(result); }

    QJsonObject metadata() const { return m_metadata; }
    void setMetadata(const QJsonObject& meta) { m_metadata = meta; }

    QJsonObject toJson() const;
    static AIMessage fromJson(const QJsonObject& obj);

private:
    AIRole m_role = AIRole::User;
    QList<AIMessageContent> m_contents;
    QList<ToolCall> m_toolCalls;
    QList<ToolResult> m_toolResults;
    QJsonObject m_metadata;
};

struct AIUsage {
    int promptTokens = 0;
    int completionTokens = 0;
    int totalTokens = 0;
    qint64 latencyMs = 0;
    QString costEstimate;
};

enum class ConnectionStatus {
    Connected,
    AuthenticationFailed,
    InvalidModel,
    RateLimited,
    NetworkError,
    ProviderUnavailable,
    NotConfigured
};

QString connectionStatusToString(ConnectionStatus status);

struct ConnectionTestResult {
    ConnectionStatus status = ConnectionStatus::NotConfigured;
    QString message;
    QStringList detectedModels;
};

class AIConversation {
public:
    AIConversation() = default;

    QString providerId() const { return m_providerId; }
    void setProviderId(const QString& pid) { m_providerId = pid; }

    QString model() const { return m_model; }
    void setModel(const QString& model) { m_model = model; }

    const QList<AIMessage>& messages() const { return m_messages; }
    void addMessage(const AIMessage& msg) { m_messages.append(msg); }
    void clear() { m_messages.clear(); }

    AIUsage cumulativeUsage() const { return m_cumulativeUsage; }
    void addUsage(const AIUsage& usage);

    QJsonObject toJson() const;
    static AIConversation fromJson(const QJsonObject& obj);

private:
    QString m_providerId;
    QString m_model;
    QList<AIMessage> m_messages;
    AIUsage m_cumulativeUsage;
};

/**
 * @brief Common provider-neutral interface for all AI backends.
 */
class AIProvider : public QObject {
    Q_OBJECT

public:
    explicit AIProvider(QObject* parent = nullptr);
    ~AIProvider() override = default;

    virtual QString providerId() const = 0;
    virtual QString displayName() const = 0;
    virtual AIProviderCapabilities capabilities() const = 0;

    virtual bool isConfigured() const;
    virtual void configure(const QString& apiKey, const QString& model = QString(), const QString& baseUrl = QString());

    virtual QString apiKey() const { return m_apiKey; }
    virtual QString currentModel() const { return m_currentModel; }
    virtual QString baseUrl() const { return m_baseUrl; }
    virtual void setModel(const QString& model) { m_currentModel = model; }

    virtual QStringList cachedModels() const { return m_cachedModels; }
    virtual void setCachedModels(const QStringList& models) { m_cachedModels = models; }

    // Capabilities convenience queries
    bool supportsTools() const { return capabilities().toolCalling; }
    bool supportsStructuredOutput() const { return capabilities().structuredOutput; }
    bool supportsVision() const { return capabilities().imageInput; }
    bool supportsStreaming() const { return capabilities().streaming; }
    bool supportsMultimodal() const { return capabilities().imageInput || capabilities().imageOutput; }

    // Async Operations
    virtual void listModels(std::function<void(const QStringList& models, const QString& error)> callback) = 0;

    virtual void sendMessage(const QList<AIMessage>& messages,
                            const QJsonArray& tools,
                            std::function<void(const AIMessage& response, const AIUsage& usage, const QString& error)> callback) = 0;

    virtual void streamMessage(const QList<AIMessage>& messages,
                              const QJsonArray& tools,
                              std::function<void(const QString& deltaText, const QList<ToolCall>& toolCalls, bool finished, const AIUsage& usage, const QString& error)> callback) = 0;

    virtual void testConnection(std::function<void(const ConnectionTestResult& result)> callback) = 0;

    virtual void cancelRequest();

signals:
    void configurationChanged();
    void requestCancelled();

protected:
    QString m_apiKey;
    QString m_currentModel;
    QString m_baseUrl;
    QStringList m_cachedModels;
    bool m_isCancelled = false;
};

} // namespace AI
