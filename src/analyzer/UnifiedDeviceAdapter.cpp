#include "UnifiedDeviceAdapter.h"
#include "CapabilityDatabase.h"
#include "hardware/DeviceDatabase.h"
#include "hardware/HardwareManager.h"
#include <QCoreApplication>
#include <QDebug>

UnifiedDeviceAdapter::UnifiedDeviceAdapter(QObject* parent)
    : QObject(parent)
{
}


void UnifiedDeviceAdapter::initialize(const QString& customPacksDir) {
    // Initialize Hardware::DeviceDatabase
    Hardware::DeviceDatabase::instance().initialize(customPacksDir);
    
    // Connect to database reload signal
    connect(&Hardware::DeviceDatabase::instance(), &Hardware::DeviceDatabase::databaseReloaded,
            this, &UnifiedDeviceAdapter::databasesRefreshed);
    
    m_initialized = true;
}

std::optional<UnifiedDeviceAdapter::UnifiedDeviceInfo> UnifiedDeviceAdapter::getDeviceInfo(const QString& deviceOrBoardName) const {
    if (deviceOrBoardName.isEmpty()) return std::nullopt;
    
    // Try as board first (boards have more complete info including display interface)
    Hardware::BoardDefinition board = Hardware::DeviceDatabase::instance().findBoard(deviceOrBoardName);
    if (!board.id.isEmpty()) {
        UnifiedDeviceInfo info = convertBoardDefinition(board);
        // Merge with capability profile for framework support
        info = mergeWithCapabilityProfile(info, board.mcuPartNumber);
        return info;
    }
    
    // Try as device
    Hardware::DeviceDefinition dev = Hardware::DeviceDatabase::instance().findDevice(deviceOrBoardName);
    if (!dev.partNumber.isEmpty()) {
        UnifiedDeviceInfo info = convertDeviceDefinition(dev);
        info = mergeWithCapabilityProfile(info, dev.partNumber);
        return info;
    }
    
    return std::nullopt;
}

std::optional<UnifiedDeviceAdapter::UnifiedDeviceInfo> UnifiedDeviceAdapter::getDeviceInfoForProject(const Hardware::HardwareConfig& hwConfig) const {
    // Priority: boardId > deviceId > default
    if (!hwConfig.boardId.isEmpty()) {
        return getDeviceInfo(hwConfig.boardId);
    }
    if (!hwConfig.deviceId.isEmpty()) {
        return getDeviceInfo(hwConfig.deviceId);
    }
    // Default fallback
    return getDeviceInfo("STM32H745");
}

QStringList UnifiedDeviceAdapter::listAllDevices() const {
    QStringList devices;
    for (const auto& dev : Hardware::DeviceDatabase::instance().allDevices()) {
        if (!dev.partNumber.isEmpty()) devices << dev.partNumber;
    }
    return devices;
}

QStringList UnifiedDeviceAdapter::listAllBoards() const {
    QStringList boards;
    for (const auto& board : Hardware::DeviceDatabase::instance().allBoards()) {
        if (!board.id.isEmpty()) boards << board.id;
    }
    return boards;
}

QStringList UnifiedDeviceAdapter::listAllTargets() const {
    QStringList targets = listAllBoards();
    targets << listAllDevices();
    targets.removeDuplicates();
    return targets;
}

bool UnifiedDeviceAdapter::supportsFramework(const QString& deviceOrBoardName, const QString& framework) const {
    auto info = getDeviceInfo(deviceOrBoardName);
    if (!info) return false;
    
    QString normFw = framework.trimmed().toLower();
    for (const QString& f : info->supported_frameworks) {
        if (f.toLower() == normFw) return true;
    }
    return false;
}

QStringList UnifiedDeviceAdapter::getSupportedFrameworks(const QString& deviceOrBoardName) const {
    auto info = getDeviceInfo(deviceOrBoardName);
    return info ? info->supported_frameworks : QStringList();
}

bool UnifiedDeviceAdapter::hasPeripheral(const QString& deviceOrBoardName, const QString& peripheralType) const {
    return getPeripheralCount(deviceOrBoardName, peripheralType) > 0;
}

int UnifiedDeviceAdapter::getPeripheralCount(const QString& deviceOrBoardName, const QString& peripheralType) const {
    auto info = getDeviceInfo(deviceOrBoardName);
    if (!info) return 0;
    
    QString key = peripheralType.toLower();
    // Normalize common peripheral names
    if (key == "uart" || key == "usart") key = "usart";
    if (key == "spi") key = "spi";
    if (key == "i2c") key = "i2c";
    if (key == "can") key = "can";
    if (key == "usb") key = "usb";
    if (key == "ethernet" || key == "eth") key = "ethernet";
    if (key == "wifi") key = "wifi";
    if (key == "ble" || key == "bluetooth") key = "ble";
    if (key == "adc") key = "adc";
    
    return info->peripheralCounts.value(key, 0);
}

bool UnifiedDeviceAdapter::supportsDisplayInterface(const QString& deviceOrBoardName, const QString& interfaceType) const {
    auto info = getDeviceInfo(deviceOrBoardName);
    if (!info) return false;
    return info->displayInterface.compare(interfaceType, Qt::CaseInsensitive) == 0;
}

QPair<int, int> UnifiedDeviceAdapter::getMaxDisplayResolution(const QString& deviceOrBoardName) const {
    auto info = getDeviceInfo(deviceOrBoardName);
    if (!info) return qMakePair(0, 0);
    return qMakePair(info->maxDisplayWidth, info->maxDisplayHeight);
}

DeviceProfile UnifiedDeviceAdapter::getCapabilityProfile(const QString& deviceName) const {
    // Create temporary CapabilityDatabase for lookup
    CapabilityDatabase db;
    db.loadBuiltinDatabase();
    return db.getDeviceProfile(deviceName);
}

bool UnifiedDeviceAdapter::capabilityDatabaseHasDevice(const QString& deviceName) const {
    CapabilityDatabase db;
    db.loadBuiltinDatabase();
    return db.deviceExists(deviceName);
}

Hardware::DeviceDefinition UnifiedDeviceAdapter::getHardwareDefinition(const QString& partNumber) const {
    return Hardware::DeviceDatabase::instance().findDevice(partNumber);
}

Hardware::BoardDefinition UnifiedDeviceAdapter::getBoardDefinition(const QString& boardId) const {
    return Hardware::DeviceDatabase::instance().findBoard(boardId);
}

bool UnifiedDeviceAdapter::hardwareDatabaseHasDevice(const QString& partNumber) const {
    auto dev = Hardware::DeviceDatabase::instance().findDevice(partNumber);
    return !dev.partNumber.isEmpty();
}

bool UnifiedDeviceAdapter::hardwareDatabaseHasBoard(const QString& boardId) const {
    auto board = Hardware::DeviceDatabase::instance().findBoard(boardId);
    return !board.id.isEmpty();
}

void UnifiedDeviceAdapter::refreshFromHardwareDatabase() {
    Hardware::DeviceDatabase::instance().reloadPacks();
    emit databasesRefreshed();
}

UnifiedDeviceAdapter::UnifiedDeviceInfo UnifiedDeviceAdapter::convertDeviceDefinition(const Hardware::DeviceDefinition& dev) const {
    UnifiedDeviceInfo info;
    info.name = dev.partNumber;
    info.displayName = dev.partNumber;
    info.family = dev.family;
    info.vendor = dev.vendor;
    info.partNumber = dev.partNumber;
    info.architecture = dev.architecture;
    info.core = dev.core;
    info.package = dev.package;
    info.ram_kb = static_cast<uint32_t>(dev.ramBytes / 1024);
    info.flash_kb = static_cast<uint32_t>(dev.flashBytes / 1024);
    info.cpu_speed_mhz = static_cast<uint32_t>(dev.maxClockMhz);
    info.sourceProvenance = dev.sourceProvenance;
    info.isCustom = (dev.vendor == "Custom" || dev.sourceProvenance.contains("Custom", Qt::CaseInsensitive));
    info.isBoard = false;
    
    // Peripherals from peripheralsCount map
    if (dev.peripheralsCount.value("usart", 0) > 0 || dev.peripheralsCount.value("uart", 0) > 0) {
        info.peripherals << "UART";
        info.peripheralCounts["usart"] = dev.peripheralsCount.value("usart", 0) + dev.peripheralsCount.value("uart", 0);
    }
    if (dev.peripheralsCount.value("spi", 0) > 0) {
        info.peripherals << "SPI";
        info.peripheralCounts["spi"] = dev.peripheralsCount.value("spi", 0);
    }
    if (dev.peripheralsCount.value("i2c", 0) > 0) {
        info.peripherals << "I2C";
        info.peripheralCounts["i2c"] = dev.peripheralsCount.value("i2c", 0);
    }
    if (dev.peripheralsCount.value("can", 0) > 0) {
        info.peripherals << "CAN";
        info.peripheralCounts["can"] = dev.peripheralsCount.value("can", 0);
    }
    if (dev.peripheralsCount.value("usb", 0) > 0) {
        info.peripherals << "USB";
        info.peripheralCounts["usb"] = dev.peripheralsCount.value("usb", 0);
    }
    if (dev.peripheralsCount.value("ethernet", 0) > 0) {
        info.peripherals << "ETH";
        info.peripheralCounts["ethernet"] = dev.peripheralsCount.value("ethernet", 0);
    }
    if (dev.peripheralsCount.value("wifi", 0) > 0) {
        info.peripherals << "WiFi";
        info.peripheralCounts["wifi"] = dev.peripheralsCount.value("wifi", 0);
    }
    if (dev.peripheralsCount.value("ble", 0) > 0 || dev.peripheralsCount.value("bluetooth", 0) > 0) {
        info.peripherals << "BLE";
        info.peripheralCounts["ble"] = dev.peripheralsCount.value("ble", 0) + dev.peripheralsCount.value("bluetooth", 0);
    }
    if (dev.peripheralsCount.value("adc", 0) > 0) {
        info.peripherals << "ADC";
        info.peripheralCounts["adc"] = dev.peripheralsCount.value("adc", 0);
    }
    
    // Default supported frameworks based on architecture
    if (dev.architecture.contains("Cortex-M", Qt::CaseInsensitive) || dev.architecture.contains("RISC-V", Qt::CaseInsensitive)) {
        info.supported_frameworks = {"lvgl", "ugfx", "qt-for-mcus"};
    } else if (dev.architecture.contains("Cortex-A", Qt::CaseInsensitive) || dev.architecture.contains("ARMv8", Qt::CaseInsensitive)) {
        info.supported_frameworks = {"qt-for-mcus", "lvgl"};
    } else {
        info.supported_frameworks = {"lvgl", "ugfx"};
    }
    
    // Display interface detection from peripherals
    if (dev.peripheralsCount.value("ltdc", 0) > 0 || dev.peripheralsCount.value("dsi", 0) > 0) {
        info.displayInterface = "LTDC";
    } else if (dev.peripheralsCount.value("spi", 0) > 0) {
        info.displayInterface = "SPI";
    } else if (dev.peripheralsCount.value("rgb", 0) > 0 || dev.peripheralsCount.value("mipi-dsi", 0) > 0) {
        info.displayInterface = "RGB";
    }
    
    return info;
}

UnifiedDeviceAdapter::UnifiedDeviceInfo UnifiedDeviceAdapter::convertBoardDefinition(const Hardware::BoardDefinition& board) const {
    UnifiedDeviceInfo info;
    info.name = board.id;
    info.displayName = board.name.isEmpty() ? board.id : board.name;
    info.family = board.boardFamily;
    info.vendor = board.manufacturer;
    info.partNumber = board.mcuPartNumber;
    info.isBoard = true;
    info.associatedMcu = board.mcuPartNumber;
    info.sourceProvenance = board.sourceProvenance;
    info.isCustom = (board.manufacturer == "Custom" || board.sourceProvenance.contains("Custom", Qt::CaseInsensitive));
    
    // Get MCU details for memory/performance
    Hardware::DeviceDefinition mcu = Hardware::DeviceDatabase::instance().findDevice(board.mcuPartNumber);
    if (!mcu.partNumber.isEmpty()) {
        info.architecture = mcu.architecture;
        info.core = mcu.core;
        info.ram_kb = static_cast<uint32_t>(mcu.ramBytes / 1024);
        info.flash_kb = static_cast<uint32_t>(mcu.flashBytes / 1024);
        info.cpu_speed_mhz = static_cast<uint32_t>(mcu.maxClockMhz);
        info.package = mcu.package;
        
        // Copy peripheral info from MCU
        auto mcuInfo = convertDeviceDefinition(mcu);
        info.peripherals = mcuInfo.peripherals;
        info.peripheralCounts = mcuInfo.peripheralCounts;
        info.supported_frameworks = mcuInfo.supported_frameworks;
    }
    
    // Board-specific display interface from connectors
    if (!board.connectors.isEmpty()) {
        for (const auto& conn : board.connectors) {
            if (conn.name.contains("LCD", Qt::CaseInsensitive) || conn.name.contains("Display", Qt::CaseInsensitive)) {
                info.displayInterface = "RGB"; // Typical for board-level LCD connectors
                break;
            }
            if (conn.name.contains("MIPI", Qt::CaseInsensitive) || conn.name.contains("DSI", Qt::CaseInsensitive)) {
                info.displayInterface = "MIPI-DSI";
                break;
            }
            if (conn.name.contains("SPI", Qt::CaseInsensitive)) {
                info.displayInterface = "SPI";
            }
        }
    }
    
    // Board-level max display resolution hints
    if (board.id.contains("480", Qt::CaseInsensitive) || board.name.contains("480", Qt::CaseInsensitive)) {
        info.maxDisplayWidth = 480;
        info.maxDisplayHeight = 320;
    } else if (board.id.contains("800", Qt::CaseInsensitive) || board.name.contains("800", Qt::CaseInsensitive)) {
        info.maxDisplayWidth = 800;
        info.maxDisplayHeight = 480;
    } else if (board.id.contains("320", Qt::CaseInsensitive) || board.name.contains("320", Qt::CaseInsensitive)) {
        info.maxDisplayWidth = 320;
        info.maxDisplayHeight = 240;
    }
    
    info.supportedColorFormats = {"RGB565", "RGB888", "ARGB8888"};
    
    return info;
}

UnifiedDeviceAdapter::UnifiedDeviceInfo UnifiedDeviceAdapter::convertCapabilityProfile(const DeviceProfile& profile) const {
    UnifiedDeviceInfo info;
    info.name = profile.name;
    info.displayName = profile.name;
    info.family = profile.family;
    info.ram_kb = profile.ram_kb;
    info.flash_kb = profile.flash_kb;
    info.cpu_speed_mhz = profile.cpu_speed_mhz;
    info.peripherals = profile.peripherals;
    info.supported_frameworks = profile.supported_frameworks;
    info.sourceProvenance = "builtin-capability";
    
    // Fill peripheral counts from list (approximate)
    for (const QString& p : profile.peripherals) {
        QString key = p.toLower();
        if (key.contains("uart") || key.contains("usart")) info.peripheralCounts["usart"]++;
        else if (key.contains("spi")) info.peripheralCounts["spi"]++;
        else if (key.contains("i2c")) info.peripheralCounts["i2c"]++;
        else if (key.contains("can")) info.peripheralCounts["can"]++;
        else if (key.contains("usb")) info.peripheralCounts["usb"]++;
        else if (key.contains("eth")) info.peripheralCounts["ethernet"]++;
        else if (key.contains("wifi")) info.peripheralCounts["wifi"]++;
        else if (key.contains("ble") || key.contains("bluetooth")) info.peripheralCounts["ble"]++;
        else if (key.contains("adc")) info.peripheralCounts["adc"]++;
    }
    
    return info;
}

UnifiedDeviceAdapter::UnifiedDeviceInfo UnifiedDeviceAdapter::mergeWithCapabilityProfile(const UnifiedDeviceInfo& hwInfo, const QString& deviceName) const {
    UnifiedDeviceInfo merged = hwInfo;
    
    // Get capability profile for framework validation
    CapabilityDatabase db;
    db.loadBuiltinDatabase();
    DeviceProfile capProfile = db.getDeviceProfile(deviceName);
    
    if (!capProfile.name.isEmpty() && capProfile.name != "Generic Cortex-M4") {
        // Capability database has specific framework support - use it
        merged.supported_frameworks = capProfile.supported_frameworks;
        
        // If hardware DB didn't have frameworks, use capability DB
        if (merged.supported_frameworks.isEmpty()) {
            merged.supported_frameworks = capProfile.supported_frameworks;
        }
        
        // Merge peripherals (capability DB may have simpler names)
        for (const QString& p : capProfile.peripherals) {
            if (!merged.peripherals.contains(p, Qt::CaseInsensitive)) {
                merged.peripherals << p;
            }
        }
    }
    
    return merged;
}

// Singleton accessor
UnifiedDeviceAdapter& unifiedDeviceAdapter() {
    static UnifiedDeviceAdapter instance;
    return instance;
}
