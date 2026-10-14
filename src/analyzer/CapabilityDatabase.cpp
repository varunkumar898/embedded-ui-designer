#include "CapabilityDatabase.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>

CapabilityDatabase::CapabilityDatabase(QObject* parent)
    : QObject(parent)
{
    initBuiltinDefaults();
}

void CapabilityDatabase::initBuiltinDefaults() {
    // STM32H7 Series
    addDeviceProfile({
        "STM32H745",
        "STM32H7",
        512,
        2048,
        480,
        {"UART1", "UART2", "UART3", "SPI1", "SPI2", "SPI3", "I2C1", "I2C2", "CAN1"},
        {"qt-for-mcus", "ugfx", "lvgl", "embedweb"},
        "Dual-core Cortex-M7 (480MHz) + Cortex-M4 (240MHz)",
        1024, 0, 0, 1, "Cortex-M7", "arm-none-eabi-gcc", "verified", 20.0f,
        512, 192, 2048, 0, "Nucleo-H745ZI-Q"
    });

    addDeviceProfile({
        "STM32H743",
        "STM32H7",
        512,
        2048,
        400,
        {"UART1", "UART2", "SPI1", "SPI2", "I2C1", "I2C2"},
        {"qt-for-mcus", "ugfx", "lvgl", "embedweb"},
        "High-performance single-core Cortex-M7",
        1024, 0, 0, 1, "Cortex-M7", "arm-none-eabi-gcc", "verified", 20.0f,
        512, 192, 2048, 0, "Nucleo-H743ZI"
    });

    // STM32F7 / F4 Series
    addDeviceProfile({
        "STM32F746",
        "STM32F7",
        320,
        1024,
        216,
        {"UART1", "UART2", "SPI1", "SPI2", "I2C1"},
        {"qt-for-mcus", "ugfx", "lvgl", "embedweb"},
        "Discovery kit Cortex-M7 with Chrom-ART",
        320, 0, 0, 1, "Cortex-M7", "arm-none-eabi-gcc", "verified", 20.0f,
        256, 64, 1024, 0, "STM32F746G-DISCO"
    });

    addDeviceProfile({
        "STM32F429",
        "STM32F4",
        256,
        2048,
        180,
        {"UART1", "UART2", "SPI1", "SPI2", "I2C1"},
        {"ugfx", "lvgl", "embedweb"},
        "Cortex-M4 with LTDC LCD controller",
        260, 0, 0, 1, "Cortex-M4", "arm-none-eabi-gcc", "verified", 20.0f,
        192, 64, 2048, 0, "STM32F429I-DISC1"
    });

    addDeviceProfile({
        "STM32F401",
        "STM32F4",
        96,
        512,
        84,
        {"UART1", "UART2", "SPI1", "SPI2", "I2C1"},
        {"ugfx", "lvgl", "embedweb"},
        "Entry-level Cortex-M4",
        96, 0, 0, 1, "Cortex-M4", "arm-none-eabi-gcc", "verified", 20.0f,
        96, 0, 512, 0, "Nucleo-F401RE"
    });

    addDeviceProfile({
        "STM32F030R8",
        "STM32F0",
        8,
        64,
        48,
        {"UART1", "SPI1", "I2C1"},
        {"ugfx", "embedweb"},
        "Ultra-low-cost Cortex-M0",
        8, 0, 0, 1, "Cortex-M0", "arm-none-eabi-gcc", "verified", 10.0f,
        8, 0, 64, 0, "NUCLEO-F030R8"
    });

    addDeviceProfile({
        "STM32L476",
        "STM32L4",
        128,
        1024,
        80,
        {"UART1", "UART2", "SPI1", "I2C1"},
        {"qt-for-mcus", "ugfx", "lvgl", "embedweb"},
        "Ultra-low-power Cortex-M4",
        128, 0, 0, 1, "Cortex-M4", "arm-none-eabi-gcc", "verified", 20.0f,
        96, 32, 1024, 0, "NUCLEO-L476RG"
    });

    // ESP32 Series
    addDeviceProfile({
        "ESP32",
        "ESP32",
        520,
        4096,
        240,
        {"UART0", "UART1", "SPI2", "SPI3", "I2C0", "WiFi", "BLE"},
        {"lvgl", "qt-for-mcus", "ugfx", "embedweb"},
        "Dual-core Xtensa LX6 with 4MB Flash",
        520, 0, 64, 1, "Xtensa-LX6", "xtensa-esp32-elf-gcc", "supported_untested", 20.0f,
        320, 0, 0, 4096, "ESP32-WROOM-32"
    });

    addDeviceProfile({
        "ESP32-S3",
        "ESP32",
        512,
        8192,
        240,
        {"UART0", "UART1", "SPI2", "I2C0", "USB", "WiFi", "BLE"},
        {"lvgl", "qt-for-mcus", "ugfx", "embedweb"},
        "Dual-core Xtensa LX7 with vector instructions (ESP32-S3-DevKitC-1-N8R8 / N8R2)",
        512, 8192, 64, 1, "Xtensa-LX7", "xtensa-esp32s3-elf-gcc", "supported_untested", 20.0f,
        384, 0, 0, 8192, "ESP32-S3-DevKitC-1-N8R8"
    });

    addDeviceProfile({
        "ESP32-C3",
        "ESP32",
        400,
        4096,
        160,
        {"UART0", "UART1", "SPI2", "I2C0", "WiFi", "BLE"},
        {"lvgl", "ugfx", "embedweb"},
        "Single-core RISC-V with WiFi and BLE",
        400, 0, 64, 1, "RISC-V", "riscv32-esp-elf-gcc", "supported_untested", 20.0f,
        384, 0, 0, 4096, "ESP32-C3-DevKitM-1"
    });

    addDeviceProfile({
        "ESP32-C6",
        "ESP32",
        512,
        4096,
        160,
        {"UART0", "UART1", "SPI2", "I2C0", "WiFi", "BLE", "Zigbee"},
        {"lvgl", "ugfx", "embedweb"},
        "Single-core RISC-V with WiFi 6, BLE 5, and 802.15.4",
        512, 0, 64, 1, "RISC-V", "riscv32-esp-elf-gcc", "supported_untested", 20.0f,
        384, 0, 0, 4096, "ESP32-C6-DevKitC-1"
    });

    // NXP i.MX RT Series
    addDeviceProfile({
        "NXP i.MX RT1060",
        "i.MX RT",
        1024,
        8192,
        600,
        {"UART1", "UART2", "SPI1", "SPI2", "I2C1", "CAN1"},
        {"qt-for-mcus", "lvgl", "ugfx", "embedweb"},
        "Cortex-M7 Crossover MCU @ 600MHz",
        1024, 0, 0, 1, "Cortex-M7", "arm-none-eabi-gcc", "verified", 20.0f,
        512, 512, 0, 8192, "MIMXRT1060-EVK"
    });

    addDeviceProfile({
        "NXP i.MX RT1050",
        "i.MX RT",
        512,
        4096,
        600,
        {"UART1", "UART2", "SPI1", "I2C1"},
        {"qt-for-mcus", "lvgl", "ugfx", "embedweb"},
        "Cortex-M7 Crossover MCU @ 600MHz",
        512, 0, 0, 1, "Cortex-M7", "arm-none-eabi-gcc", "verified", 20.0f,
        256, 256, 0, 4096, "MIMXRT1050-EVK"
    });

    // Raspberry Pi Pico Series (Microcontrollers)
    addDeviceProfile({
        "Raspberry Pi Pico",
        "RP2040",
        264,
        2048,
        133,
        {"UART0", "UART1", "SPI0", "SPI1", "I2C0", "I2C1", "PIO"},
        {"lvgl", "ugfx", "embedweb"},
        "Dual-core Cortex-M0+ with programmable I/O",
        264, 0, 16, 1, "Cortex-M0+", "arm-none-eabi-gcc", "verified", 20.0f,
        264, 0, 0, 2048, "Raspberry Pi Pico (RP2040)"
    });

    addDeviceProfile({
        "Raspberry Pi Pico 2",
        "RP2350",
        520,
        4096,
        150,
        {"UART0", "UART1", "SPI0", "SPI1", "I2C0", "I2C1", "PIO"},
        {"lvgl", "ugfx", "qt-for-mcus", "embedweb"},
        "Dual Hazard3 RISC-V / Cortex-M33 @ 150MHz",
        520, 0, 16, 1, "Cortex-M33", "arm-none-eabi-gcc", "verified", 20.0f,
        520, 0, 0, 4096, "Raspberry Pi Pico 2 (RP2350)"
    });


    // Nordic & Microchip
    addDeviceProfile({
        "nRF52840",
        "nRF52",
        256,
        1024,
        64,
        {"UART0", "SPI0", "I2C0", "BLE", "USB"},
        {"lvgl", "ugfx"},
        "Cortex-M4 with Bluetooth 5.0",
        256, 0, 32, 0, "Cortex-M4", "arm-none-eabi-gcc", "unsupported", 20.0f,
        256, 0, 1024, 0, "nRF52840-DK"
    });

    addDeviceProfile({
        "ATSAMD51",
        "SAMD51",
        192,
        512,
        120,
        {"UART1", "SPI1", "I2C1", "USB"},
        {"lvgl", "ugfx"},
        "Cortex-M4F @ 120MHz",
        192, 0, 16, 0, "Cortex-M4", "arm-none-eabi-gcc", "unsupported", 20.0f,
        192, 0, 512, 0, "SAMD51-Thing-Plus"
    });
}

bool CapabilityDatabase::loadFromFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return false;

    QJsonObject root = doc.object();
    QString family = root.value("family").toString();

    if (root.contains("devices") && root.value("devices").isArray()) {
        QJsonArray devArr = root.value("devices").toArray();
        for (const QJsonValue& v : devArr) {
            if (v.isObject()) {
                QJsonObject d = v.toObject();
                DeviceProfile p;
                p.name = d.value("name").toString();
                p.family = d.value("family").toString(family);
                p.ram_kb = d.value("ram_kb").toInt(128);
                p.flash_kb = d.value("flash_kb").toInt(512);
                p.cpu_speed_mhz = d.value("cpu_speed_mhz").toInt(100);
                p.notes = d.value("notes").toString();

                if (d.contains("peripherals") && d.value("peripherals").isArray()) {
                    for (const QJsonValue& pv : d.value("peripherals").toArray()) {
                        p.peripherals.append(pv.toString());
                    }
                }

                if (d.contains("supported_frameworks") && d.value("supported_frameworks").isArray()) {
                    for (const QJsonValue& fv : d.value("supported_frameworks").toArray()) {
                        p.supported_frameworks.append(fv.toString().toLower());
                    }
                }

                if (!p.name.isEmpty()) {
                    addDeviceProfile(p);
                }
            }
        }
        return true;
    }

    return false;
}

bool CapabilityDatabase::loadFromDirectory(const QString& dirPath) {
    QDir dir(dirPath);
    if (!dir.exists()) return false;

    QStringList filters;
    filters << "*.json";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    bool anyLoaded = false;
    for (const QFileInfo& fi : files) {
        if (loadFromFile(fi.absoluteFilePath())) {
            anyLoaded = true;
        }
    }
    return anyLoaded;
}

bool CapabilityDatabase::loadBuiltinDatabase() {
    initBuiltinDefaults();
    return true;
}

DeviceProfile CapabilityDatabase::getDeviceProfile(const QString& device_name) const {
    QString key = device_name.trimmed().toLower();
    for (auto it = m_devices.constBegin(); it != m_devices.constEnd(); ++it) {
        if (it.key().toLower() == key || it.value().name.toLower() == key) {
            return it.value();
        }
    }

    // Default fallback profile if not found
    DeviceProfile fallback;
    fallback.name = device_name.isEmpty() ? "Generic Cortex-M4" : device_name;
    fallback.family = "Generic";
    fallback.ram_kb = 256;
    fallback.flash_kb = 1024;
    fallback.cpu_speed_mhz = 120;
    fallback.supported_frameworks = {"qt-for-mcus", "ugfx", "lvgl"};
    return fallback;
}

QStringList CapabilityDatabase::listAllDevices() const {
    QStringList names;
    for (const DeviceProfile& p : m_devices) {
        names.append(p.name);
    }
    return names;
}

QStringList CapabilityDatabase::listFamilies() const {
    QSet<QString> families;
    for (const DeviceProfile& p : m_devices) {
        if (!p.family.isEmpty()) families.insert(p.family);
    }
    return families.values();
}

QStringList CapabilityDatabase::getDevicesByFamily(const QString& family) const {
    QStringList names;
    for (const DeviceProfile& p : m_devices) {
        if (p.family.compare(family, Qt::CaseInsensitive) == 0) {
            names.append(p.name);
        }
    }
    return names;
}

bool CapabilityDatabase::deviceExists(const QString& name) const {
    QString key = name.trimmed().toLower();
    for (auto it = m_devices.constBegin(); it != m_devices.constEnd(); ++it) {
        if (it.key().toLower() == key || it.value().name.toLower() == key) {
            return true;
        }
    }
    return false;
}

bool CapabilityDatabase::supportsFramework(const QString& device, const QString& framework) const {
    DeviceProfile p = getDeviceProfile(device);
    QString normFw = framework.trimmed().toLower();
    for (const QString& f : p.supported_frameworks) {
        if (f.toLower() == normFw) return true;
    }
    return false;
}

void CapabilityDatabase::addDeviceProfile(const DeviceProfile& profile) {
    if (profile.name.isEmpty()) return;
    m_devices[profile.name] = profile;
}

void CapabilityDatabase::clear() {
    m_devices.clear();
}
