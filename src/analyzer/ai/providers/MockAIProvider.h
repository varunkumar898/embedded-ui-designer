#pragma once

#include "AIProvider.h"

namespace AI {

class MockAIProvider : public AIProvider {
    Q_OBJECT

public:
    explicit MockAIProvider(QObject* parent = nullptr);
    ~MockAIProvider() override = default;

    QString providerId() const override { return "mock"; }
    QString displayName() const override { return "Mock AI Provider (Deterministic / Offline)"; }
    AIProviderCapabilities capabilities() const override;

    void listModels(std::function<void(const QStringList& models, const QString& error)> callback) override;

    void sendMessage(const QList<AIMessage>& messages,
                    const QJsonArray& tools,
                    std::function<void(const AIMessage& response, const AIUsage& usage, const QString& error)> callback) override;

    void streamMessage(const QList<AIMessage>& messages,
                      const QJsonArray& tools,
                      std::function<void(const QString& deltaText, const QList<ToolCall>& toolCalls, bool finished, const AIUsage& usage, const QString& error)> callback) override;

    void testConnection(std::function<void(const ConnectionTestResult& result)> callback) override;

    // Simulation hooks for deterministic testing
    void setSimulatedStatus(ConnectionStatus status) { m_simulatedStatus = status; }
    void setSimulatedError(const QString& err) { m_simulatedError = err; }
    void setSimulatedResponse(const QString& text) { m_simulatedResponseText = text; }
    void setSimulatedToolCalls(const QList<ToolCall>& calls) { m_simulatedToolCalls = calls; }
    void setSimulateSlowNetwork(bool slow) { m_simulateSlowNetwork = slow; }

private:
    ConnectionStatus m_simulatedStatus = ConnectionStatus::Connected;
    QString m_simulatedError;
    QString m_simulatedResponseText;
    QList<ToolCall> m_simulatedToolCalls;
    bool m_simulateSlowNetwork = false;
};

} // namespace AI
