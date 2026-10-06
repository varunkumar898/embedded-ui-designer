#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QVariant>

namespace Hardware {

struct PinDefinition {
    QString name;               // e.g. "PA5", "GPIO4"
    int physicalPin = 0;        // Physical pin number on package / header
    QString description;
    QStringList alternateFunctions; // e.g. ["GPIO_Input", "GPIO_Output", "SPI1_SCK", "ADC1_IN5"]
    QString defaultFunction;
    bool isPower = false;
    bool isGround = false;
    bool isReset = false;
    bool isReserved = false;
    bool isBoot = false;

    QJsonObject toJson() const;
    static PinDefinition fromJson(const QJsonObject& obj);
};

struct PeripheralDescriptor {
    QString name;               // e.g. "SPI1", "USART1", "I2C1"
    QString type;               // e.g. "SPI", "USART", "UART", "I2C", "CAN", "ADC", "PWM", "USB"
    QMap<QString, QStringList> signalOptions; // e.g. "SCK" -> ["PA5", "PB3"], "MISO" -> ["PA6", "PB4"]
    QJsonObject defaults;

    QJsonObject toJson() const;
    static PeripheralDescriptor fromJson(const QJsonObject& obj);
};

struct DeviceDefinition {
    QString vendor;             // e.g. "STMicroelectronics", "Espressif", "Raspberry Pi"
    QString family;             // e.g. "STM32", "ESP32", "RP2000"
    QString series;             // e.g. "STM32F4", "ESP32-S3", "RP2040"
    QString partNumber;         // e.g. "STM32F407VG", "ESP32-S3", "RP2040"
    QString architecture;       // e.g. "ARM Cortex-M4", "Xtensa LX7", "ARM Cortex-M0+"
    QString core;               // e.g. "Cortex-M4F", "Dual Cortex-M0+"
    QString package;            // e.g. "LQFP100", "QFN-56"
    quint64 flashBytes = 0;
    quint64 ramBytes = 0;
    quint64 eepromBytes = 0;
    double minVoltage = 0.0;
    double maxVoltage = 0.0;
    int maxClockMhz = 0;
    int gpioCount = 0;
    QMap<QString, int> peripheralsCount;
    QList<PeripheralDescriptor> peripheralDescriptors;
    QList<PinDefinition> pins;
    QJsonObject clockTree;
    QString sourceProvenance;
    QString version = "1.0.0";

    const PinDefinition* findPin(const QString& pinName) const;
    const PeripheralDescriptor* findPeripheral(const QString& periName) const;
    QStringList availablePinsForSignal(const QString& periName, const QString& signalName) const;

    QJsonObject toJson() const;
    static DeviceDefinition fromJson(const QJsonObject& obj);
};

struct BoardConnector {
    QString name;
    int pinCount = 0;
    QMap<int, QString> pinMapping; // Pin number (1-based) -> MCU Pin or power/gnd label

    QJsonObject toJson() const;
    static BoardConnector fromJson(const QJsonObject& obj);
};

struct BoardDefinition {
    QString id;                 // e.g. "STM32F407G-DISC1"
    QString name;               // e.g. "STM32F4 Discovery"
    QString manufacturer;       // e.g. "STMicroelectronics"
    QString boardFamily;        // e.g. "Discovery", "DevKitC", "Pico"
    QString mcuPartNumber;      // e.g. "STM32F407VG"
    QString architecture;
    QString debugInterface;     // e.g. "ST-LINK/V2", "SWD", "USB-JTAG"
    QString supplyVoltage;
    QMap<QString, QString> physicalPinLabels; // e.g. "LED_GREEN" -> "PD12", "D13" -> "PA5"
    QList<BoardConnector> connectors;
    QJsonObject onboardPeripherals;
    QString sourceProvenance;
    QString version = "1.0.0";

    QJsonObject toJson() const;
    static BoardDefinition fromJson(const QJsonObject& obj);
};

// ── Live Project Hardware Configuration Structures ──

struct PinConfiguration {
    QString pin;                // Pin name, e.g. "PA5"
    QString label;              // Custom user label, e.g. "STATUS_LED"
    QString gpio;               // GPIO identifier, e.g. "PA5"
    QString mode = "None";      // "GPIO_Input", "GPIO_Output", "Analog", "AlternateFunction", "Reset", "Disabled"
    QString alternateFunction;  // e.g. "SPI1_SCK"
    QString pull = "NoPull";    // "NoPull", "PullUp", "PullDown"
    QString speed = "Medium";   // "Low", "Medium", "High", "VeryHigh"
    QString outputType = "PushPull"; // "PushPull", "OpenDrain"
    QString initialOutput = "Low";   // "Low", "High"
    QString interrupt = "None"; // "None", "RisingEdge", "FallingEdge", "BothEdges"
    bool locked = false;

    QJsonObject toJson() const;
    static PinConfiguration fromJson(const QJsonObject& obj);
};

struct PeripheralConfiguration {
    QString name;               // e.g. "SPI1", "USART1", "I2C1"
    QString type;               // e.g. "SPI", "USART", "I2C"
    bool enabled = false;
    QMap<QString, QString> assignedPins; // Signal name -> MCU Pin name, e.g. "SCK" -> "PA5"
    QMap<QString, QVariant> parameters;  // e.g. "baudRate": 115200, "clockSpeedHz": 10000000

    QJsonObject toJson() const;
    static PeripheralConfiguration fromJson(const QJsonObject& obj);
};

struct ClockConfiguration {
    int sysClockMhz = 0;
    int hclkMhz = 0;
    int apb1Mhz = 0;
    int apb2Mhz = 0;
    QString oscSource = "Internal";
    int extOscMhz = 0;

    QJsonObject toJson() const;
    static ClockConfiguration fromJson(const QJsonObject& obj);
};

struct ToolchainConfiguration {
    QString framework = "stm32cube"; // "stm32cube", "esp-idf", "pico-sdk", "linux", "arduino"
    QString toolchain = "arm-none-eabi-gcc";
    QString ide = "STM32CubeIDE";
    QString library = "HAL";

    QJsonObject toJson() const;
    static ToolchainConfiguration fromJson(const QJsonObject& obj);
};

struct HardwareConfig {
    QString targetType = "device"; // "device", "board", "custom"
    QString vendor;
    QString family;
    QString series;
    QString deviceId;              // MCU part number, e.g. "STM32F407VG"
    QString boardId;               // Board ID if selected, e.g. "STM32F407G-DISC1"
    QString architecture;
    QString core;
    QString package;
    quint64 flashBytes = 0;
    quint64 ramBytes = 0;

    QMap<QString, PinConfiguration> pins;
    QMap<QString, PeripheralConfiguration> peripherals;
    ClockConfiguration clock;
    ToolchainConfiguration toolchain;

    bool isValid() const { return !deviceId.isEmpty() || !vendor.isEmpty(); }
    void clear();

    QJsonObject toJson() const;
    static HardwareConfig fromJson(const QJsonObject& obj);
};

} // namespace Hardware
