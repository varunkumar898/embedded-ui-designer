#include "CustomOpenAICompatibleProvider.h"

namespace AI {

CustomOpenAICompatibleProvider::CustomOpenAICompatibleProvider(QObject* parent)
    : OpenAIProvider(parent)
{
    m_baseUrl = "http://localhost:11434/v1"; // Common Ollama endpoint default
    m_currentModel = "llama3";
    m_cachedModels = {"llama3", "mistral", "qwen2.5-coder", "phi3"};
}

bool CustomOpenAICompatibleProvider::isConfigured() const {
    // Local providers might not require an API key, but do require a base URL
    return !m_baseUrl.trimmed().isEmpty();
}

} // namespace AI
