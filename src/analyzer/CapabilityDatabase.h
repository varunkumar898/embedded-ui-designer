#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <cstdint>

struct DeviceProfile {
    QString name;                       // e.g. "STM32H745"
    QString family;                     // e.g. "STM32H7"
    uint32_t ram_kb = 128;              // Total advertised RAM
    uint32_t flash_kb = 512;            // Total advertised Flash
    uint32_t cpu_speed_mhz = 100;
    QStringList peripherals;            // ["UART", "SPI", "I2C"]
    QStringList supported_frameworks;   // ["qt-for-mcus", "ugfx", "lvgl", "embedweb"]
    QString notes;

    // Stage 4.5 Multi-MCU Hardware & Resource Metrics
    uint32_t internal_sram_kb = 0;      // Physical on-chip SRAM
    uint32_t psram_kb = 0;              // External PSRAM if equipped
    uint32_t bootloader_flash_kb = 0;   // Reserved for bootloader/partition table
    uint8_t dma_requires_internal_sram = 1;
    QString architecture;               // "Cortex-M4", "Cortex-M7", "Cortex-M0+", "Xtensa-LX7", etc.
    QString required_toolchain;         // "arm-none-eabi-gcc", "xtensa-esp32s3-elf-gcc", etc.
    QString embedweb_adapter_status;    // "verified", "supported_untested", "not_implemented"
    float safety_margin_percent = 20.0f; // Configurable headroom margin

    // Stage 4.7 Detailed Memory Region Decomposition
    uint32_t dma_sram_kb = 0;           // DMA-accessible internal SRAM (AXI SRAM, SRAM1-3, OCRAM)
    uint32_t tcm_kb = 0;                // Tightly-Coupled Memory (CPU-only, non-DMA: ITCM/DTCM, CCM)
    uint32_t onchip_flash_kb = 0;       // Physical on-chip Flash ROM
    uint32_t external_flash_kb = 0;     // Board/Module external Flash ROM (QSPI/OSPI)
    QString module_variant;             // Exact module variant (e.g. "ESP32-S3-WROOM-1-N8R8")
};

class CapabilityDatabase : public QObject {
    Q_OBJECT

public:
    explicit CapabilityDatabase(QObject* parent = nullptr);
    ~CapabilityDatabase() override = default;

    // Load from JSON files / directories
    bool loadFromFile(const QString& path);
    bool loadFromDirectory(const QString& dirPath);
    bool loadBuiltinDatabase();

    // Queries
    DeviceProfile getDeviceProfile(const QString& device_name) const;
    QStringList listAllDevices() const;
    QStringList listFamilies() const;
    QStringList getDevicesByFamily(const QString& family) const;

    // Validation
    bool deviceExists(const QString& name) const;
    bool supportsFramework(const QString& device, const QString& framework) const;

    // Mutation
    void addDeviceProfile(const DeviceProfile& profile);
    void clear();

private:
    void initBuiltinDefaults();

    QMap<QString, DeviceProfile> m_devices;
};
