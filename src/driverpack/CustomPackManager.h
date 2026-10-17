#pragma once

#include "CustomPackModel.h"
#include "analyzer/CapabilityDatabase.h"
#include <QObject>
#include <QList>
#include <QMap>

namespace driverpack {

class CustomPackManager : public QObject {
    Q_OBJECT

public:
    static CustomPackManager& instance();

    QString customPacksDirectory() const;
    void setCustomPacksDirectory(const QString& path); // For testing

    bool installPack(const CustomPackDef& pack, CapabilityDatabase* capDb = nullptr, QString* error = nullptr);
    bool uninstallPack(const QString& deviceName, CapabilityDatabase* capDb = nullptr);

    void loadInstalledPacks(CapabilityDatabase* capDb = nullptr);
    QList<CustomPackDef> installedPacks() const;
    bool isCustomDevice(const QString& deviceName) const;
    CustomPackDef getPack(const QString& deviceName) const;

    // Helper to build a DeviceProfile from CustomPackDef
    static DeviceProfile createDeviceProfile(const CustomPackDef& pack);

signals:
    void packInstalled(const QString& deviceName);
    void packUninstalled(const QString& deviceName);
    void packsReloaded();

private:
    explicit CustomPackManager(QObject* parent = nullptr);
    ~CustomPackManager() override = default;

    QString m_customPacksDir;
    QMap<QString, CustomPackDef> m_packs;
};

} // namespace driverpack
