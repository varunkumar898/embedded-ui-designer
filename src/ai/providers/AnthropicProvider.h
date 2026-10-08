#pragma once

#include "AIProvider.h"
#include <QPointer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace AI {

class AnthropicProvider : public AIProvider {
    Q_OBJECT

public:
    explicit AnthropicProvider(QObject* parent = nullptr);
    ~AnthropicProvider() override = default;

    QString providerId() const override { return "anthropic"; }
    QString displayName() const override { return "Anthropic Claude"; }
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

    // Format helpers
    static void formatMessagesForAnthropic(const QList<AIMessage>& messages, QString& outSystemPrompt, QJsonArray& outMessages);
    static QJsonArray formatToolsForAnthropic(const QJsonArray& universalTools);
    static QJsonObject parseAnthropicResponse(const QByteArray& data, AIMessage& outMsg, AIUsage& outUsage);

private:
    void setupHeaders(QNetworkRequest& req) const;

    QNetworkAccessManager m_net;
    QPointer<QNetworkReply> m_activeReply;
};

} // namespace AI
