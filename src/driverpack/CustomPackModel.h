#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <cstdint>

namespace driverpack {

struct CustomRegisterDef {
    QString name;
    QString offset;            // e.g. "0x00"
    QString description;
    uint32_t resetValue = 0;
};

struct CustomPeripheralDef {
    QString name;              // e.g. "GPIOA", "USART1", "SPI1"
    QString baseAddress;       // e.g. "0x40020000"
    QString type;              // e.g. "GPIO", "USART", "SPI", "I2C"
    QString description;
    QVector<CustomRegisterDef> registers;
};

struct CustomMemoryRegion {
    QString name;              // e.g. "FLASH", "SRAM", "PSRAM"
    QString type;              // "flash", "sram", "psram", "tcm", "rom", "other"
    QString startAddress;      // e.g. "0x08000000"
    uint32_t sizeKb = 0;
    int usableForFramebuffer = -1; // -1 unknown, 0 no, 1 yes
};

struct CustomPinDef {
    QString pin;               // e.g. "PA0", "PB5"
    QString function;          // e.g. "GPIO_Output", "USART1_TX"
    QString peripheral;        // e.g. "GPIOA", "USART1"
};

struct CustomBoardWiring {
    QString displayInterface;  // "SPI", "I2C", "Parallel8080", "RGB", "DSI", "None"
    QJsonObject displayPins;   // pin mappings e.g. {"CS": "PB0", "DC": "PB1"}
    QJsonObject touchPins;     // pin mappings e.g. {"SCL": "PB6", "SDA": "PB7"}
    QStringList powerRails;    // e.g. ["3.3V", "5V"]
};

struct CustomSdkReference {
    QString path;
    QStringList requiredFiles;
    bool detected = false;
};

struct CustomProvenance {
    QStringList sourceFiles;
    QString importerVersion = "1.0.0";
    QString generatedBy = "importer"; // "user" | "importer" | "ai"
    bool verified = false;            // MUST be false by default
};

struct CustomPackDef {
    // Device identity
    QString name;              // e.g. "STM32G071RB"
    QString vendor;            // e.g. "STMicroelectronics"
    QString partNumber;        // e.g. "STM32G071RBT6"

    QString family;            // e.g. "STM32", "ESP32", "Custom"
    QString core;              // e.g. "Cortex-M0+", "Cortex-M4"
    uint32_t clockSpeedMhz = 64;

    QVector<CustomMemoryRegion> memoryRegions;
    QVector<CustomPeripheralDef> peripherals;
    QVector<CustomPinDef> pinTable;
    CustomBoardWiring boardWiring;
    CustomSdkReference sdkReference;
    QString toolchain = "arm-none-eabi-gcc";
    QStringList templatesUsed;
    CustomProvenance provenance;

    // Serialization & Validation
    QJsonObject toJson() const;
    static CustomPackDef fromJson(const QJsonObject& obj, QStringList* errors = nullptr);
    bool isValid(QStringList* errors = nullptr) const;

    // Queries
    const CustomPeripheralDef* findPeripheral(const QString& periphName) const;
    bool hasPeripheral(const QString& periphName) const;
    uint32_t totalRamKb() const;
    uint32_t totalFlashKb() const;
};

class CustomPackValidator {
public:
    static bool validateJson(const QJsonObject& json, QStringList& errors);
    static bool validatePack(const CustomPackDef& pack, QStringList& errors);
    static bool isValidHexAddress(const QString& hex);
};

} // namespace driverpack
