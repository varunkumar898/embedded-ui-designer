#include "DeviceDatabase.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QSet>
#include <QDebug>

namespace Hardware {

DeviceDatabase& DeviceDatabase::instance() {
    static DeviceDatabase s_instance;
    return s_instance;
}

DeviceDatabase::DeviceDatabase(QObject* parent)
    : QObject(parent)
{
}

void DeviceDatabase::initialize(const QString& customPacksDir) {
    if (m_initialized && customPacksDir.isEmpty()) return;

    m_providers.clear();

    auto stm32 = std::make_shared<Stm32Provider>();
    auto esp32 = std::make_shared<Esp32Provider>();
    auto rp2040 = std::make_shared<Rp2040Provider>();
    auto rpi = std::make_shared<RaspberryPiProvider>();
    m_customProvider = std::make_shared<CustomHardwareProvider>();

    m_providers.append(stm32);
    m_providers.append(esp32);
    m_providers.append(rp2040);
    m_providers.append(rpi);
    m_providers.append(m_customProvider);

    // Candidate directories for packs
    QStringList searchPaths;
    if (!customPacksDir.isEmpty()) {
        searchPaths.append(customPacksDir);
    }

    // Repo and executable relative paths (priority)
    searchPaths.append(QDir::current().filePath("data/hardware_packs"));
    searchPaths.append(QDir::current().filePath("../data/hardware_packs"));
    searchPaths.append(QDir::current().filePath("../../data/hardware_packs"));
    searchPaths.append(QCoreApplication::applicationDirPath() + "/data/hardware_packs");
    searchPaths.append(QCoreApplication::applicationDirPath() + "/../data/hardware_packs");
    searchPaths.append(QCoreApplication::applicationDirPath() + "/../../data/hardware_packs");

    // AppData packs directory
    QString appDataPacks = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/hardware_packs";
    searchPaths.append(appDataPacks);

    for (const QString& path : searchPaths) {
        QDir d(path);
        if (d.exists()) {
            QStringList subdirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            QStringList jsonFiles = d.entryList(QStringList() << "*.json", QDir::Files);
            if (!subdirs.isEmpty() || !jsonFiles.isEmpty()) {
                discoverPacksInDirectory(path);
                m_packsRootPath = path;
                break;
            }
        }
    }

    m_initialized = true;
    emit databaseReloaded();
}

void DeviceDatabase::reloadPacks() {
    m_initialized = false;
    initialize(m_packsRootPath);
}

void DeviceDatabase::discoverPacksInDirectory(const QString& rootPath) {
    QDir rootDir(rootPath);
    if (!rootDir.exists()) return;

    // Check specific known directories first
    QString stm32Dir = rootDir.filePath("stmicroelectronics");
    if (QDir(stm32Dir).exists()) {
        for (auto& p : m_providers) {
            if (p->id() == "stm32") p->loadFromDirectory(stm32Dir);
        }
    }

    QString espDir = rootDir.filePath("espressif");
    if (QDir(espDir).exists()) {
        for (auto& p : m_providers) {
            if (p->id() == "esp32") p->loadFromDirectory(espDir);
        }
    }

    QString rpiDir = rootDir.filePath("raspberrypi");
    if (QDir(rpiDir).exists()) {
        for (auto& p : m_providers) {
            if (p->id() == "rp2040" || p->id() == "raspberrypi") {
                p->loadFromDirectory(rpiDir);
            }
        }
    }

    QString customDir = rootDir.filePath("custom");
    if (QDir(customDir).exists()) {
        m_customProvider->loadFromDirectory(customDir);
    }

    // Discover any other vendor directories dynamically (e.g. nordic, nxp, microchip, etc.)
    QStringList subdirs = rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& sub : subdirs) {
        if (sub == "stmicroelectronics" || sub == "espressif" || sub == "raspberrypi" || sub == "custom") {
            continue;
        }
        QString vendorPath = rootDir.filePath(sub);
        QString vendorName = sub;
        vendorName[0] = vendorName[0].toUpper();

        auto genericProv = std::make_shared<GenericPackProvider>(sub, QString("%1 Provider").arg(vendorName), vendorName);
        if (genericProv->loadFromDirectory(vendorPath)) {
            m_providers.append(genericProv);
        }
    }
}

void DeviceDatabase::registerProvider(std::shared_ptr<IHardwareProvider> provider) {
    if (provider) {
        m_providers.append(provider);
        emit databaseReloaded();
    }
}

QList<IHardwareProvider*> DeviceDatabase::providers() const {
    QList<IHardwareProvider*> list;
    for (const auto& p : m_providers) list.append(p.get());
    return list;
}

IHardwareProvider* DeviceDatabase::findProviderForVendor(const QString& vendor) const {
    for (const auto& p : m_providers) {
        if (p->vendor().compare(vendor, Qt::CaseInsensitive) == 0) {
            return p.get();
        }
    }
    return nullptr;
}

QList<DeviceDefinition> DeviceDatabase::allDevices() const {
    QList<DeviceDefinition> list;
    for (const auto& p : m_providers) {
        list.append(p->devices());
    }
    return list;
}

QList<BoardDefinition> DeviceDatabase::allBoards() const {
    QList<BoardDefinition> list;
    for (const auto& p : m_providers) {
        list.append(p->boards());
    }
    return list;
}

DeviceDefinition DeviceDatabase::findDevice(const QString& partNumber) const {
    for (const auto& p : m_providers) {
        DeviceDefinition dev = p->findDevice(partNumber);
        if (!dev.partNumber.isEmpty()) return dev;
    }
    for (const auto& d : allDevices()) {
        if (d.partNumber.compare(partNumber, Qt::CaseInsensitive) == 0) {
            return d;
        }
    }
    return DeviceDefinition();
}

BoardDefinition DeviceDatabase::findBoard(const QString& boardId) const {
    for (const auto& p : m_providers) {
        BoardDefinition b = p->findBoard(boardId);
        if (!b.id.isEmpty()) return b;
    }
    for (const auto& b : allBoards()) {
        if (b.id.compare(boardId, Qt::CaseInsensitive) == 0 ||
            b.name.compare(boardId, Qt::CaseInsensitive) == 0) {
            return b;
        }
    }
    return BoardDefinition();
}

QStringList DeviceDatabase::allVendors() const {
    QSet<QString> vendors;
    for (const auto& dev : allDevices()) {
        if (!dev.vendor.isEmpty()) vendors.insert(dev.vendor);
    }
    for (const auto& b : allBoards()) {
        if (!b.manufacturer.isEmpty()) vendors.insert(b.manufacturer);
    }
    QStringList list = vendors.values();
    list.sort();
    return list;
}

QStringList DeviceDatabase::allFamilies(const QString& vendorFilter) const {
    QSet<QString> families;
    for (const auto& dev : allDevices()) {
        if (vendorFilter.isEmpty() || dev.vendor.compare(vendorFilter, Qt::CaseInsensitive) == 0) {
            if (!dev.family.isEmpty()) families.insert(dev.family);
        }
    }
    for (const auto& b : allBoards()) {
        if (vendorFilter.isEmpty() || b.manufacturer.compare(vendorFilter, Qt::CaseInsensitive) == 0) {
            if (!b.boardFamily.isEmpty()) families.insert(b.boardFamily);
        }
    }
    QStringList list = families.values();
    list.sort();
    return list;
}

QStringList DeviceDatabase::allArchitectures() const {
    QSet<QString> archs;
    for (const auto& dev : allDevices()) {
        if (!dev.architecture.isEmpty()) archs.insert(dev.architecture);
    }
    for (const auto& b : allBoards()) {
        if (!b.architecture.isEmpty()) archs.insert(b.architecture);
    }
    QStringList list = archs.values();
    list.sort();
    return list;
}

QStringList DeviceDatabase::allPackages() const {
    QSet<QString> pkgs;
    for (const auto& dev : allDevices()) {
        if (!dev.package.isEmpty()) pkgs.insert(dev.package);
    }
    QStringList list = pkgs.values();
    list.sort();
    return list;
}

QStringList DeviceDatabase::allCores() const {
    QSet<QString> cores;
    for (const auto& dev : allDevices()) {
        if (!dev.core.isEmpty()) cores.insert(dev.core);
    }
    QStringList list = cores.values();
    list.sort();
    return list;
}

QStringList DeviceDatabase::allDeviceTypes() const {
    return QStringList() << "All" << "MCU" << "MPU" << "SBC" << "BOARD" << "CUSTOM";
}

QList<HardwareItem> DeviceDatabase::allCatalogItems() const {
    QList<HardwareItem> items;
    QSet<QString> seenIds;

    // 1. Devices (MCU / MPU / Custom)
    for (const auto& dev : allDevices()) {
        if (dev.partNumber.isEmpty() || seenIds.contains(dev.partNumber)) continue;
        seenIds.insert(dev.partNumber);

        HardwareItem item;
        item.id = dev.partNumber;
        item.reference = dev.partNumber;
        item.vendor = dev.vendor;
        item.family = dev.family;
        item.series = dev.series;
        item.architecture = dev.architecture;
        item.core = dev.core;
        item.package = dev.package;
        item.flashBytes = dev.flashBytes;
        item.ramBytes = dev.ramBytes;
        item.maxClockMhz = dev.maxClockMhz;
        item.isBoard = false;
        item.isCustom = (dev.vendor == "Custom" || dev.sourceProvenance.contains("Custom", Qt::CaseInsensitive));
        if (item.isCustom) {
            item.deviceType = "CUSTOM";
        } else if (dev.architecture.contains("Cortex-A", Qt::CaseInsensitive) || dev.architecture.contains("ARMv8", Qt::CaseInsensitive) || dev.partNumber.contains("BCM", Qt::CaseInsensitive)) {
            item.deviceType = "MPU";
        } else {
            item.deviceType = "MCU";
        }
        item.associatedMcu = dev.partNumber;
        item.sourceProvenance = dev.sourceProvenance;

        // Connectivity summary
        QStringList comms;
        if (dev.peripheralsCount.value("usart", 0) > 0 || dev.peripheralsCount.value("uart", 0) > 0)
            comms.append(QString("UART(%1)").arg(dev.peripheralsCount.value("usart", 0) + dev.peripheralsCount.value("uart", 0)));
        if (dev.peripheralsCount.value("spi", 0) > 0)
            comms.append(QString("SPI(%1)").arg(dev.peripheralsCount.value("spi", 0)));
        if (dev.peripheralsCount.value("i2c", 0) > 0)
            comms.append(QString("I2C(%1)").arg(dev.peripheralsCount.value("i2c", 0)));
        if (dev.peripheralsCount.value("can", 0) > 0)
            comms.append(QString("CAN(%1)").arg(dev.peripheralsCount.value("can", 0)));
        if (dev.peripheralsCount.value("usb", 0) > 0)
            comms.append("USB");
        if (dev.peripheralsCount.value("ethernet", 0) > 0)
            comms.append("ETH");
        if (dev.peripheralsCount.value("wifi", 0) > 0)
            comms.append("Wi-Fi");
        if (dev.peripheralsCount.value("ble", 0) > 0 || dev.peripheralsCount.value("bluetooth", 0) > 0)
            comms.append("BLE");
        if (dev.peripheralsCount.value("adc", 0) > 0)
            comms.append(QString("ADC(%1)").arg(dev.peripheralsCount.value("adc", 0)));
        if (comms.isEmpty()) {
            for (const auto& d : dev.peripheralDescriptors) {
                if (!comms.contains(d.type)) comms.append(d.type);
            }
        }
        item.connectivity = comms.join(", ");
        items.append(item);
    }

    // 2. Boards (BOARD / SBC)
    for (const auto& b : allBoards()) {
        if (b.id.isEmpty() || seenIds.contains(b.id)) continue;
        seenIds.insert(b.id);

        HardwareItem item;
        item.id = b.id;
        item.reference = b.name.isEmpty() ? b.id : b.name;
        item.vendor = b.manufacturer;
        item.family = b.boardFamily;
        item.series = b.boardFamily;
        item.architecture = b.architecture;
        item.isBoard = true;
        item.associatedMcu = b.mcuPartNumber;
        item.isCustom = (b.manufacturer == "Custom" || b.sourceProvenance.contains("Custom", Qt::CaseInsensitive));

        if (item.isCustom) {
            item.deviceType = "CUSTOM";
        } else if (b.mcuPartNumber.contains("BCM2711", Qt::CaseInsensitive) || b.id.contains("Pi-4", Qt::CaseInsensitive)) {
            item.deviceType = "SBC";
        } else {
            item.deviceType = "BOARD";
        }

        DeviceDefinition mcu = findDevice(b.mcuPartNumber);
        if (!mcu.partNumber.isEmpty()) {
            item.core = mcu.core;
            if (item.architecture.isEmpty()) item.architecture = mcu.architecture;
            item.flashBytes = mcu.flashBytes;
            item.ramBytes = mcu.ramBytes;
            item.maxClockMhz = mcu.maxClockMhz;
        }

        if (!b.connectors.isEmpty()) {
            item.package = QString("%1 (%2-pin)").arg(b.connectors.first().name).arg(b.connectors.first().pinCount);
        } else {
            item.package = "Header / PCB";
        }

        QStringList comms;
        if (!b.debugInterface.isEmpty()) comms.append(b.debugInterface);
        if (b.id.contains("Pico", Qt::CaseInsensitive)) comms.append("USB 1.1, SWD");
        else if (b.id.contains("4B", Qt::CaseInsensitive)) comms.append("Gigabit ETH, Wi-Fi 5, BT 5.0, 2x USB3");
        else if (b.id.contains("DevKit", Qt::CaseInsensitive)) comms.append("Wi-Fi, Bluetooth, USB-UART");
        else if (b.id.contains("DISC", Qt::CaseInsensitive)) comms.append("ST-LINK/V2, USB FS, Audio DAC");
        else if (b.id.contains("Nucleo", Qt::CaseInsensitive)) comms.append("ST-LINK/V2-1, Morpho, Arduino V3");
        item.connectivity = comms.join(", ");
        item.sourceProvenance = b.sourceProvenance;

        items.append(item);
    }

    return items;
}

bool DeviceDatabase::addCustomDevice(const DeviceDefinition& device) {
    if (device.partNumber.isEmpty()) return false;
    m_customProvider->addDevice(device);
    emit hardwareDefinitionAdded(device.partNumber);
    emit databaseReloaded();
    return true;
}

bool DeviceDatabase::addCustomBoard(const BoardDefinition& board) {
    if (board.id.isEmpty()) return false;
    m_customProvider->addBoard(board);
    emit hardwareDefinitionAdded(board.id);
    emit databaseReloaded();
    return true;
}

bool DeviceDatabase::exportHardwareDefinition(const QString& id, const QString& filePath, QString* outError) const {
    DeviceDefinition dev = findDevice(id);
    if (!dev.partNumber.isEmpty()) {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            if (outError) *outError = file.errorString();
            return false;
        }
        file.write(QJsonDocument(dev.toJson()).toJson(QJsonDocument::Indented));
        file.close();
        return true;
    }

    BoardDefinition b = findBoard(id);
    if (!b.id.isEmpty()) {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            if (outError) *outError = file.errorString();
            return false;
        }
        file.write(QJsonDocument(b.toJson()).toJson(QJsonDocument::Indented));
        file.close();
        return true;
    }

    if (outError) *outError = QString("Hardware definition '%1' not found.").arg(id);
    return false;
}

bool DeviceDatabase::importHardwareDefinition(const QString& filePath, QString* outId, QString* outError) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (outError) *outError = file.errorString();
        return false;
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        if (outError) *outError = QString("JSON Parse error: %1").arg(parseErr.errorString());
        return false;
    }

    QJsonObject root = doc.object();
    if (root.contains("identity")) {
        DeviceDefinition dev = DeviceDefinition::fromJson(root);
        if (dev.partNumber.isEmpty()) {
            if (outError) *outError = "Invalid device definition: missing partNumber.";
            return false;
        }
        addCustomDevice(dev);
        if (outId) *outId = dev.partNumber;
        return true;
    } else if (root.contains("metadata")) {
        BoardDefinition b = BoardDefinition::fromJson(root);
        if (b.id.isEmpty()) {
            if (outError) *outError = "Invalid board definition: missing board id.";
            return false;
        }
        addCustomBoard(b);
        if (outId) *outId = b.id;
        return true;
    }

    if (outError) *outError = "Unrecognized hardware definition format.";
    return false;
}

bool DeviceDatabase::duplicateHardwareDefinition(const QString& srcId, const QString& newId, QString* outError) {
    DeviceDefinition dev = findDevice(srcId);
    if (!dev.partNumber.isEmpty()) {
        dev.partNumber = newId;
        dev.vendor = "Custom (" + dev.vendor + ")";
        addCustomDevice(dev);
        return true;
    }

    BoardDefinition b = findBoard(srcId);
    if (!b.id.isEmpty()) {
        b.id = newId;
        b.name = b.name + " (Copy)";
        addCustomBoard(b);
        return true;
    }

    if (outError) *outError = QString("Source definition '%1' not found.").arg(srcId);
    return false;
}

} // namespace Hardware
