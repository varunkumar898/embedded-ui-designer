#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <memory>

namespace Plugins {

enum class PluginType {
    Component,
    Hardware,
    Board,
    Generator,
    AIProvider,
    Other
};

QString pluginTypeToString(PluginType type);

struct PluginMetadata {
    QString id;
    QString name;
    QString version;
    QString author;
    PluginType type = PluginType::Other;
    QString description;
    bool enabled = true;
};

class PluginInterface {
public:
    virtual ~PluginInterface() = default;
    virtual PluginMetadata metadata() const = 0;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
};

class PluginRegistry : public QObject {
    Q_OBJECT

public:
    static PluginRegistry& instance();

    bool registerPlugin(std::shared_ptr<PluginInterface> plugin);
    bool unregisterPlugin(const QString& pluginId);

    QList<std::shared_ptr<PluginInterface>> registeredPlugins() const;
    std::shared_ptr<PluginInterface> findPlugin(const QString& pluginId) const;

    int pluginCount() const;
    QStringList pluginIds() const;

    // Safe error boundary initialization
    void initializeAll();
    void shutdownAll();

signals:
    void pluginRegistered(const QString& pluginId);
    void pluginUnregistered(const QString& pluginId);
    void pluginError(const QString& pluginId, const QString& error);

private:
    explicit PluginRegistry(QObject* parent = nullptr);
    ~PluginRegistry() override = default;
    PluginRegistry(const PluginRegistry&) = delete;
    PluginRegistry& operator=(const PluginRegistry&) = delete;

    QMap<QString, std::shared_ptr<PluginInterface>> m_plugins;
};

} // namespace Plugins
