#include "PluginRegistry.h"
#include <QDebug>

namespace Plugins {

QString pluginTypeToString(PluginType type) {
    switch (type) {
        case PluginType::Component: return "Component";
        case PluginType::Hardware: return "Hardware";
        case PluginType::Board: return "Board";
        case PluginType::Generator: return "Generator";
        case PluginType::AIProvider: return "AI Provider";
        case PluginType::Other: return "Other";
    }
    return "Other";
}

PluginRegistry& PluginRegistry::instance() {
    static PluginRegistry s_instance;
    return s_instance;
}

PluginRegistry::PluginRegistry(QObject* parent)
    : QObject(parent)
{
}

bool PluginRegistry::registerPlugin(std::shared_ptr<PluginInterface> plugin) {
    if (!plugin) return false;
    PluginMetadata meta = plugin->metadata();
    if (meta.id.isEmpty()) return false;

    if (m_plugins.contains(meta.id)) {
        return false; // Already registered
    }

    // Try-catch / boundary protection so a plugin init failure never crashes editor
    bool ok = false;
    try {
        ok = plugin->initialize();
    } catch (const std::exception& e) {
        emit pluginError(meta.id, QString("Exception initializing plugin: %1").arg(e.what()));
        return false;
    } catch (...) {
        emit pluginError(meta.id, "Unknown exception initializing plugin.");
        return false;
    }

    if (!ok) {
        emit pluginError(meta.id, "Plugin initialization returned false.");
        return false;
    }

    m_plugins[meta.id] = plugin;
    emit pluginRegistered(meta.id);
    return true;
}

bool PluginRegistry::unregisterPlugin(const QString& pluginId) {
    if (!m_plugins.contains(pluginId)) return false;

    auto plugin = m_plugins.take(pluginId);
    if (plugin) {
        try {
            plugin->shutdown();
        } catch (...) {
            // Safe teardown
        }
    }
    emit pluginUnregistered(pluginId);
    return true;
}

QList<std::shared_ptr<PluginInterface>> PluginRegistry::registeredPlugins() const {
    return m_plugins.values();
}

std::shared_ptr<PluginInterface> PluginRegistry::findPlugin(const QString& pluginId) const {
    return m_plugins.value(pluginId, nullptr);
}

int PluginRegistry::pluginCount() const {
    return m_plugins.size();
}

QStringList PluginRegistry::pluginIds() const {
    return m_plugins.keys();
}

void PluginRegistry::initializeAll() {
    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
        try {
            it.value()->initialize();
        } catch (const std::exception& e) {
            emit pluginError(it.key(), QString("Exception in initializeAll: %1").arg(e.what()));
        }
    }
}

void PluginRegistry::shutdownAll() {
    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
        try {
            it.value()->shutdown();
        } catch (...) {
        }
    }
    m_plugins.clear();
}

} // namespace Plugins
