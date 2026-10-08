#include "AIProviderRegistry.h"
#include "providers/MockAIProvider.h"
#include "providers/OpenAIProvider.h"
#include "providers/AnthropicProvider.h"
#include "providers/GeminiProvider.h"
#include "providers/CustomOpenAICompatibleProvider.h"
#include <QSettings>

namespace AI {

QString taskTypeToString(TaskType type) {
    switch (type) {
        case TaskType::Design: return "Design";
        case TaskType::Diagnostics: return "Diagnostics";
        case TaskType::Code: return "Code";
        case TaskType::Documentation: return "Documentation";
        default: return "General";
    }
}

TaskType stringToTaskType(const QString& str) {
    QString lower = str.toLower();
    if (lower == "design") return TaskType::Design;
    if (lower == "diagnostics") return TaskType::Diagnostics;
    if (lower == "code") return TaskType::Code;
    if (lower == "documentation") return TaskType::Documentation;
    return TaskType::General;
}

AIProviderRegistry& AIProviderRegistry::instance() {
    static AIProviderRegistry s_instance;
    return s_instance;
}

AIProviderRegistry::AIProviderRegistry(QObject* parent)
    : QObject(parent)
{
    registerDefaultProviders();
    loadSettings();
}

void AIProviderRegistry::registerDefaultProviders() {
    registerProvider(std::make_shared<MockAIProvider>());
    registerProvider(std::make_shared<OpenAIProvider>());
    registerProvider(std::make_shared<AnthropicProvider>());
    registerProvider(std::make_shared<GeminiProvider>());
    registerProvider(std::make_shared<CustomOpenAICompatibleProvider>());
}

void AIProviderRegistry::registerProvider(std::shared_ptr<AIProvider> provider) {
    if (!provider) return;
    QString id = provider->providerId().toLower();
    m_providers[id] = provider;
}

std::shared_ptr<AIProvider> AIProviderRegistry::getProvider(const QString& id) const {
    QString key = id.toLower();
    if (m_providers.contains(key)) {
        return m_providers[key];
    }
    return nullptr;
}

QList<std::shared_ptr<AIProvider>> AIProviderRegistry::availableProviders() const {
    return m_providers.values();
}

QStringList AIProviderRegistry::providerIds() const {
    return m_providers.keys();
}

void AIProviderRegistry::setDefaultProviderId(const QString& id) {
    QString key = id.toLower();
    if (m_providers.contains(key) && m_defaultProviderId != key) {
        m_defaultProviderId = key;
        emit activeProviderChanged(m_defaultProviderId);
    }
}

void AIProviderRegistry::setFallbackProviderId(const QString& id) {
    m_fallbackProviderId = id.toLower();
}

std::shared_ptr<AIProvider> AIProviderRegistry::activeProvider() const {
    auto p = getProvider(m_defaultProviderId);
    if (!p && !m_providers.isEmpty()) {
        return m_providers.first();
    }
    return p;
}

std::shared_ptr<AIProvider> AIProviderRegistry::fallbackProvider() const {
    if (m_fallbackProviderId.isEmpty()) return nullptr;
    return getProvider(m_fallbackProviderId);
}

TaskProfile AIProviderRegistry::taskProfile(TaskType type) const {
    if (m_taskProfiles.contains(type)) {
        return m_taskProfiles[type];
    }
    // Default task profile points to default provider
    TaskProfile defaultProf;
    defaultProf.preferredProviderId = m_defaultProviderId;
    if (auto p = activeProvider()) {
        defaultProf.preferredModel = p->currentModel();
    }
    return defaultProf;
}

void AIProviderRegistry::setTaskProfile(TaskType type, const TaskProfile& profile) {
    m_taskProfiles[type] = profile;
}

std::shared_ptr<AIProvider> AIProviderRegistry::providerForTask(TaskType type) const {
    if (m_taskProfiles.contains(type)) {
        const auto& prof = m_taskProfiles[type];
        if (!prof.preferredProviderId.isEmpty()) {
            auto p = getProvider(prof.preferredProviderId);
            if (p && p->isConfigured()) return p;
        }
    }
    return activeProvider();
}

void AIProviderRegistry::saveSettings() {
    // SECURITY: Stored in user-specific local app settings (QSettings)
    // NEVER in project JSON, .euiproj, git repo, or logs!
    QSettings settings("EmbeddedUIDesigner", "AISettings");
    settings.beginGroup("General");
    settings.setValue("defaultProviderId", m_defaultProviderId);
    settings.setValue("fallbackProviderId", m_fallbackProviderId);
    settings.endGroup();

    settings.beginGroup("Providers");
    for (auto it = m_providers.begin(); it != m_providers.end(); ++it) {
        auto p = it.value();
        settings.beginGroup(p->providerId());
        // Simple obfuscation so keys aren't in plain ascii if file is opened
        QByteArray keyBytes = p->apiKey().toUtf8();
        settings.setValue("apiKey", QString::fromLatin1(keyBytes.toBase64()));
        settings.setValue("model", p->currentModel());
        settings.setValue("baseUrl", p->baseUrl());
        settings.setValue("cachedModels", p->cachedModels());
        settings.endGroup();
    }
    settings.endGroup();

    settings.beginGroup("TaskProfiles");
    for (auto it = m_taskProfiles.begin(); it != m_taskProfiles.end(); ++it) {
        settings.beginGroup(taskTypeToString(it.key()));
        settings.setValue("providerId", it.value().preferredProviderId);
        settings.setValue("model", it.value().preferredModel);
        settings.endGroup();
    }
    settings.endGroup();
}

void AIProviderRegistry::loadSettings() {
    QSettings settings("EmbeddedUIDesigner", "AISettings");
    settings.beginGroup("General");
    if (settings.contains("defaultProviderId")) {
        m_defaultProviderId = settings.value("defaultProviderId").toString();
    }
    if (settings.contains("fallbackProviderId")) {
        m_fallbackProviderId = settings.value("fallbackProviderId").toString();
    }
    settings.endGroup();

    settings.beginGroup("Providers");
    for (auto it = m_providers.begin(); it != m_providers.end(); ++it) {
        auto p = it.value();
        settings.beginGroup(p->providerId());
        if (settings.contains("apiKey")) {
            QByteArray decodedKey = QByteArray::fromBase64(settings.value("apiKey").toString().toLatin1());
            QString key = QString::fromUtf8(decodedKey);
            QString model = settings.value("model").toString();
            QString baseUrl = settings.value("baseUrl").toString();
            p->configure(key, model, baseUrl);
        }
        if (settings.contains("cachedModels")) {
            p->setCachedModels(settings.value("cachedModels").toStringList());
        }
        settings.endGroup();
    }
    settings.endGroup();

    settings.beginGroup("TaskProfiles");
    const QStringList childGroups = settings.childGroups();
    for (const QString& group : childGroups) {
        settings.beginGroup(group);
        TaskProfile prof;
        prof.preferredProviderId = settings.value("providerId").toString();
        prof.preferredModel = settings.value("model").toString();
        m_taskProfiles[stringToTaskType(group)] = prof;
        settings.endGroup();
    }
    settings.endGroup();
}

void AIProviderRegistry::clearAllCredentials() {
    for (auto& p : m_providers) {
        p->configure("", p->currentModel(), p->baseUrl());
    }
    saveSettings();
}

} // namespace AI
