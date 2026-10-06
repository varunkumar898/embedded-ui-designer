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
