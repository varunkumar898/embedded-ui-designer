#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <memory>
#include "HardwareModel.h"
#include "HardwareProvider.h"

namespace Hardware {

class DeviceDatabase : public QObject {
    Q_OBJECT

public:
    static DeviceDatabase& instance();

    void initialize(const QString& customPacksDir = QString());
    void reloadPacks();

    // Provider management
    void registerProvider(std::shared_ptr<IHardwareProvider> provider);
    QList<IHardwareProvider*> providers() const;
    IHardwareProvider* findProviderForVendor(const QString& vendor) const;

    // Queries
    QList<DeviceDefinition> allDevices() const;
    QList<BoardDefinition> allBoards() const;
    QList<HardwareItem> allCatalogItems() const;
    DeviceDefinition findDevice(const QString& partNumber) const;
    BoardDefinition findBoard(const QString& boardId) const;

    QStringList allVendors() const;
    QStringList allFamilies(const QString& vendorFilter = QString()) const;
    QStringList allArchitectures() const;
    QStringList allCores() const;
    QStringList allPackages() const;
    QStringList allDeviceTypes() const;

    // Custom Hardware Management (Phase 6, 18)
    bool addCustomDevice(const DeviceDefinition& device);
    bool addCustomBoard(const BoardDefinition& board);
    bool exportHardwareDefinition(const QString& id, const QString& filePath, QString* outError = nullptr) const;
    bool importHardwareDefinition(const QString& filePath, QString* outId = nullptr, QString* outError = nullptr);
    bool duplicateHardwareDefinition(const QString& srcId, const QString& newId, QString* outError = nullptr);

    CustomHardwareProvider* customProvider() const { return m_customProvider.get(); }

signals:
    void databaseReloaded();
    void hardwareDefinitionAdded(const QString& id);

private:
    explicit DeviceDatabase(QObject* parent = nullptr);
    ~DeviceDatabase() override = default;

    void discoverPacksInDirectory(const QString& rootPath);

    QList<std::shared_ptr<IHardwareProvider>> m_providers;
    std::shared_ptr<CustomHardwareProvider> m_customProvider;
    QString m_packsRootPath;
    bool m_initialized = false;
};

} // namespace Hardware
