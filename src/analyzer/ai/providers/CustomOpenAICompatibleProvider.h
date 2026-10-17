#pragma once

#include "OpenAIProvider.h"

namespace AI {

class CustomOpenAICompatibleProvider : public OpenAIProvider {
    Q_OBJECT

public:
    explicit CustomOpenAICompatibleProvider(QObject* parent = nullptr);
    ~CustomOpenAICompatibleProvider() override = default;

    QString providerId() const override { return "custom"; }
    QString displayName() const override { return "Custom / Local (OpenAI-compatible)"; }
    static const QString PresetLMStudio;
    static const QString PresetOllama;

    bool isConfigured() const override;

    void testConnection(std::function<void(const ConnectionTestResult& result)> callback) override;

protected:
    QString effectiveBaseUrl() const override;
    bool isSuitableModel(const QString& modelId) const override;
};

} // namespace AI
