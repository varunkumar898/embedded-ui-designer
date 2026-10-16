#pragma once

#include "AIProvider.h"
#include <QPointer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace AI {

class OpenAIProvider : public AIProvider {
    Q_OBJECT

public:
    explicit OpenAIProvider(QObject* parent = nullptr);
    ~OpenAIProvider() override = default;

    QString providerId() const override { return "openai"; }
    QString displayName() const override { return "OpenAI"; }
    AIProviderCapabilities capabilities() const override;

    void listModels(std::function<void(const QStringList& models, const QString& error)> callback) override;

    void sendMessage(const QList<AIMessage>& messages,
                    const QJsonArray& tools,
                    std::function<void(const AIMessage& response, const AIUsage& usage, const QString& error)> callback) override;

    void streamMessage(const QList<AIMessage>& messages,
                      const QJsonArray& tools,
                      std::function<void(const QString& deltaText, const QList<ToolCall>& toolCalls, bool finished, const AIUsage& usage, const QString& error)> callback) override;

    void testConnection(std::function<void(const ConnectionTestResult& result)> callback) override;

    void cancelRequest() override;

    // Helper to format messages for OpenAI wire format
    static QJsonArray formatMessagesForOpenAI(const QList<AIMessage>& messages);
    static QJsonObject parseOpenAIResponse(const QByteArray& data, AIMessage& outMsg, AIUsage& outUsage);

protected:
    virtual QString effectiveBaseUrl() const;
    virtual void setupAuthHeaders(QNetworkRequest& req) const;

private:
    QNetworkAccessManager m_net;
    QPointer<QNetworkReply> m_activeReply;
};

} // namespace AI
