#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QMap>
#include <optional>
#include "CapabilityDatabase.h"
#include "hardware/DeviceDatabase.h"
#include "hardware/HardwareModel.h"

/// @brief Unified adapter providing a single query interface for device information
/// Bridges CapabilityDatabase (ResourceAnalyzer needs) and Hardware::DeviceDatabase (rich hardware data)
/// Does NOT create a third database - only provides unified access to existing sources
class UnifiedDeviceAdapter : public QObject {
    Q_OBJECT

public:
    struct UnifiedDeviceInfo {
        // Core identity
        QString name;                 // e.g. "STM32H745"
        QString displayName;          // Human-readable name
        QString family;               // e.g. "STM32H7"
        QString vendor;               // e.g. "STMicroelectronics"
        QString partNumber;           // Exact part number
        QString architecture;         // e.g. "Cortex-M7"
        QString core;                 // e.g. "Cortex-M7"
        QString package;              // e.g. "LQFP144"

        // Memory
        uint32_t ram_kb = 0;
        uint32_t flash_kb = 0;

        // Performance
        uint32_t cpu_speed_mhz = 0;

        // Peripherals & connectivity
        QStringList peripherals;              // ["UART", "SPI", "I2C", "CAN", "USB", "ETH", "WiFi", "BLE"]
        QMap<QString, int> peripheralCounts;  // Detailed counts: "usart"->3, "spi"->2, etc.
        QStringList supported_frameworks;     // ["lvgl", "ugfx", "qt-for-mcus"]

        // Display capabilities
        QString displayInterface;     // "LTDC", "SPI", "RGB", "MIPI-DSI", "HDMI"
        int maxDisplayWidth = 0;
        int maxDisplayHeight = 0;
        QStringList supportedColorFormats;  // ["RGB565", "RGB888", "ARGB8888"]

        // Source tracking
        QString sourceProvenance;     // "builtin", "STM32Pack", "ESP32Pack", "Custom", etc.
        bool isCustom = false;
        bool isBoard = false;         // true if this is a board (not raw MCU)
        QString associatedMcu;        // For boards: the MCU part number

        // Validation
        bool isValid() const { return !name.isEmpty() && ram_kb > 0 && flash_kb > 0; }
    };

    explicit UnifiedDeviceAdapter(QObject* parent = nullptr);
    ~UnifiedDeviceAdapter() override = default;

    // Initialize both databases
    void initialize(const QString& customPacksDir = QString());

    // Primary query: get unified info by device/board name
    std::optional<UnifiedDeviceInfo> getDeviceInfo(const QString& deviceOrBoardName) const;

    // Query by project hardware config
    std::optional<UnifiedDeviceInfo> getDeviceInfoForProject(const Hardware::HardwareConfig& hwConfig) const;

    // List all available devices/boards
    QStringList listAllDevices() const;
    QStringList listAllBoards() const;
    QStringList listAllTargets() const;  // Both devices and boards

    // Framework compatibility
    bool supportsFramework(const QString& deviceOrBoardName, const QString& framework) const;
    QStringList getSupportedFrameworks(const QString& deviceOrBoardName) const;

    // Peripheral queries
    bool hasPeripheral(const QString& deviceOrBoardName, const QString& peripheralType) const;
    int getPeripheralCount(const QString& deviceOrBoardName, const QString& peripheralType) const;

    // Display capability queries
    bool supportsDisplayInterface(const QString& deviceOrBoardName, const QString& interfaceType) const;
    QPair<int, int> getMaxDisplayResolution(const QString& deviceOrBoardName) const;

    // CapabilityDatabase integration (for ResourceAnalyzer)
    DeviceProfile getCapabilityProfile(const QString& deviceName) const;
    bool capabilityDatabaseHasDevice(const QString& deviceName) const;

    // Hardware::DeviceDatabase integration (for pin config, codegen, etc.)
    Hardware::DeviceDefinition getHardwareDefinition(const QString& partNumber) const;
    Hardware::BoardDefinition getBoardDefinition(const QString& boardId) const;
    bool hardwareDatabaseHasDevice(const QString& partNumber) const;
    bool hardwareDatabaseHasBoard(const QString& boardId) const;

    // Refresh from hardware database
    void refreshFromHardwareDatabase();

signals:
    void deviceInfoChanged(const QString& deviceName);
    void databasesRefreshed();

private:
    // Convert Hardware::DeviceDefinition -> UnifiedDeviceInfo
    UnifiedDeviceInfo convertDeviceDefinition(const Hardware::DeviceDefinition& dev) const;
    // Convert Hardware::BoardDefinition -> UnifiedDeviceInfo
    UnifiedDeviceInfo convertBoardDefinition(const Hardware::BoardDefinition& board) const;
    // Convert DeviceProfile -> UnifiedDeviceInfo
    UnifiedDeviceInfo convertCapabilityProfile(const DeviceProfile& profile) const;
    // Merge hardware definition with capability profile (hardware takes precedence for facts)
    UnifiedDeviceInfo mergeWithCapabilityProfile(const UnifiedDeviceInfo& hwInfo, const QString& deviceName) const;

    CapabilityDatabase* m_capabilityDb = nullptr;
    bool m_initialized = false;
};

/// @brief Convenience singleton accessor
UnifiedDeviceAdapter& unifiedDeviceAdapter();
