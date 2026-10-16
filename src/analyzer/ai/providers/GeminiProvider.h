#pragma once

#include "AIProvider.h"
#include <QPointer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace AI {

class GeminiProvider : public AIProvider {
    Q_OBJECT

public:
    explicit GeminiProvider(QObject* parent = nullptr);
    ~GeminiProvider() override = default;

    QString providerId() const override { return "gemini"; }
    QString displayName() const override { return "Google Gemini"; }
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
    static void formatContentsForGemini(const QList<AIMessage>& messages, QJsonObject& outSystemInstruction, QJsonArray& outContents);
    static QJsonArray formatToolsForGemini(const QJsonArray& universalTools);
    static QJsonObject parseGeminiResponse(const QByteArray& data, AIMessage& outMsg, AIUsage& outUsage);

private:
    QString effectiveEndpoint(const QString& action) const;

    QNetworkAccessManager m_net;
    QPointer<QNetworkReply> m_activeReply;
};

} // namespace AI
