#pragma once

#include "AIProvider.h"
#include <QObject>
#include <QString>
#include <QMap>
#include <memory>

namespace AI {

enum class TaskType {
    General,
    Design,
    Diagnostics,
    Code,
    Documentation
};

QString taskTypeToString(TaskType type);
TaskType stringToTaskType(const QString& str);

struct TaskProfile {
    QString preferredProviderId;
    QString preferredModel;
};

class AIProviderRegistry : public QObject {
    Q_OBJECT

public:
    static AIProviderRegistry& instance();

    void registerProvider(std::shared_ptr<AIProvider> provider);
    std::shared_ptr<AIProvider> getProvider(const QString& id) const;
    QList<std::shared_ptr<AIProvider>> availableProviders() const;
    QStringList providerIds() const;

    QString defaultProviderId() const { return m_defaultProviderId; }
    void setDefaultProviderId(const QString& id);

    QString fallbackProviderId() const { return m_fallbackProviderId; }
    void setFallbackProviderId(const QString& id);

    std::shared_ptr<AIProvider> activeProvider() const;
    std::shared_ptr<AIProvider> fallbackProvider() const;

    // Task Profiles
    TaskProfile taskProfile(TaskType type) const;
    void setTaskProfile(TaskType type, const TaskProfile& profile);
    std::shared_ptr<AIProvider> providerForTask(TaskType type) const;

    // Persistence (Secure Local Settings - NEVER in Project Files)
    void saveSettings();
    void loadSettings();

    // Clear all credentials
    void clearAllCredentials();

signals:
    void activeProviderChanged(const QString& providerId);
    void providerConfigured(const QString& providerId);

private:
    explicit AIProviderRegistry(QObject* parent = nullptr);
    ~AIProviderRegistry() override = default;
    AIProviderRegistry(const AIProviderRegistry&) = delete;
    AIProviderRegistry& operator=(const AIProviderRegistry&) = delete;

    void registerDefaultProviders();

    QMap<QString, std::shared_ptr<AIProvider>> m_providers;
    QString m_defaultProviderId = "mock";
    QString m_fallbackProviderId;
    QMap<TaskType, TaskProfile> m_taskProfiles;
};

} // namespace AI
