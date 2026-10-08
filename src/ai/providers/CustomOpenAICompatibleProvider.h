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

    bool isConfigured() const override;
};

} // namespace AI
