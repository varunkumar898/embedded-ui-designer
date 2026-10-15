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

    if (!root.contains("devices") || !root.value("devices").isArray()) {
        return false;
    }

    for (const QJsonValue& v : root.value("devices").toArray()) {
        if (!v.isObject()) continue;
        QJsonObject d = v.toObject();
        const QString name = d.value("name").toString();
        if (name.isEmpty()) continue;

        // Task 4: merge onto an existing profile (built-in or previously loaded),
        // so fields that are absent from the JSON keep their fallback values.
        DeviceProfile p;
        if (deviceExists(name)) {
            p = getDeviceProfile(name);
        }
        p.name = name;
        if (d.contains("family")) p.family = d.value("family").toString(family);
        else if (p.family.isEmpty()) p.family = family;
        if (d.contains("vendor")) p.vendor = d.value("vendor").toString();
        if (d.contains("notes")) p.notes = d.value("notes").toString();
        if (d.contains("cpu_speed_mhz")) {
            p.cpu_speed_mhz = static_cast<uint32_t>(d.value("cpu_speed_mhz").toInt(static_cast<int>(p.cpu_speed_mhz)));
        }

        if (d.contains("cores") && d.value("cores").isArray()) {
            p.cores.clear();
            for (const QJsonValue& cv : d.value("cores").toArray()) {
                p.cores.append(cv.toString());
            }
        }

        // RAM: nested block (Task 4 schema) with legacy flat fallback.
        if (d.contains("ram") && d.value("ram").isObject()) {
            QJsonObject ram = d.value("ram").toObject();
            if (ram.contains("total_kb")) {
                p.ram_kb = static_cast<uint32_t>(ram.value("total_kb").toInt(static_cast<int>(p.ram_kb)));
            }
            if (ram.contains("regions") && ram.value("regions").isArray()) {
                p.ram_regions.clear();
                for (const QJsonValue& rv : ram.value("regions").toArray()) {
                    if (!rv.isObject()) continue;
                    QJsonObject ro = rv.toObject();
                    DeviceProfile::RamRegion region;
                    region.name = ro.value("name").toString();
                    region.kb = static_cast<uint32_t>(ro.value("kb").toInt(0));
                    const QJsonValue uf = ro.value("usable_for_framebuffer");
                    region.usable_for_framebuffer = uf.isBool() ? (uf.toBool() ? 1 : 0) : -1;
                    if (!region.name.isEmpty()) p.ram_regions.append(region);
                }
            }
        }
        if (d.contains("ram_kb")) {
            p.ram_kb = static_cast<uint32_t>(d.value("ram_kb").toInt(static_cast<int>(p.ram_kb)));
        }

        // Flash: nested block (Task 4 schema) with legacy flat fallback.
        // internal_kb == null => no on-chip flash (external parts).
        // typical_external_kb is informational and must not be treated as a hard limit.
        if (d.contains("flash") && d.value("flash").isObject()) {
            QJsonObject flash = d.value("flash").toObject();
            if (flash.contains("external")) {
                p.flash_is_external = flash.value("external").toBool(p.flash_is_external);
            }
            if (flash.contains("internal_kb")) {
                p.onchip_flash_kb = flash.value("internal_kb").isNull()
                    ? 0u
                    : static_cast<uint32_t>(flash.value("internal_kb").toInt(0));
            }
            if (flash.contains("typical_external_kb")) {
                p.external_flash_kb = flash.value("typical_external_kb").isNull()
                    ? 0u
                    : static_cast<uint32_t>(flash.value("typical_external_kb").toInt(0));
            }
        }
        if (d.contains("flash_kb")) {
            p.flash_kb = static_cast<uint32_t>(d.value("flash_kb").toInt(static_cast<int>(p.flash_kb)));
        } else if (d.contains("flash") && d.value("flash").isObject()) {
            // Derive the advertised flash size from the nested block when the flat
            // legacy value is absent.
            QJsonObject flash = d.value("flash").toObject();
            if (flash.contains("internal_kb") && !flash.value("internal_kb").isNull()) {
                p.flash_kb = static_cast<uint32_t>(flash.value("internal_kb").toInt(static_cast<int>(p.flash_kb)));
            } else if (flash.contains("typical_external_kb") && !flash.value("typical_external_kb").isNull()) {
                p.flash_kb = static_cast<uint32_t>(flash.value("typical_external_kb").toInt(static_cast<int>(p.flash_kb)));
            }
        }

        if (d.contains("has_lcd_controller")) {
            const QJsonValue h = d.value("has_lcd_controller");
            p.has_lcd_controller = h.isBool() ? (h.toBool() ? 1 : 0) : -1;
        }

        if (d.contains("peripherals") && d.value("peripherals").isArray()) {
            p.peripherals.clear();
            for (const QJsonValue& pv : d.value("peripherals").toArray()) {
                p.peripherals.append(pv.toString());
            }
        }

        if (d.contains("supported_frameworks") && d.value("supported_frameworks").isArray()) {
            p.supported_frameworks.clear();
            for (const QJsonValue& fv : d.value("supported_frameworks").toArray()) {
                p.supported_frameworks.append(fv.toString().toLower());
            }
        }

        // Stage 4.x extended fields: only overridden when present in the JSON.
        auto readU32 = [&d](const char* key, uint32_t& target) {
            if (d.contains(key)) {
                target = static_cast<uint32_t>(d.value(key).toInt(static_cast<int>(target)));
            }
        };
        readU32("internal_sram_kb", p.internal_sram_kb);
        readU32("psram_kb", p.psram_kb);
        readU32("bootloader_flash_kb", p.bootloader_flash_kb);
        readU32("dma_sram_kb", p.dma_sram_kb);
        readU32("tcm_kb", p.tcm_kb);
        readU32("onchip_flash_kb", p.onchip_flash_kb);
        readU32("external_flash_kb", p.external_flash_kb);
        if (d.contains("dma_requires_internal_sram")) {
            p.dma_requires_internal_sram =
                static_cast<uint8_t>(d.value("dma_requires_internal_sram").toInt(p.dma_requires_internal_sram));
        }
        if (d.contains("architecture")) p.architecture = d.value("architecture").toString();
        if (d.contains("required_toolchain")) p.required_toolchain = d.value("required_toolchain").toString();
        if (d.contains("embedweb_adapter_status")) p.embedweb_adapter_status = d.value("embedweb_adapter_status").toString();
        if (d.contains("safety_margin_percent")) {
            p.safety_margin_percent = static_cast<float>(d.value("safety_margin_percent").toDouble(p.safety_margin_percent));
        }
        if (d.contains("module_variant")) p.module_variant = d.value("module_variant").toString();

        // Provenance (Task 4): vendor sources and verification flag.
        if (d.contains("sources") && d.value("sources").isArray()) {
            p.sources.clear();
            for (const QJsonValue& sv : d.value("sources").toArray()) {
                p.sources.append(sv.toString());
            }
        }
        if (d.contains("verified")) p.verified = d.value("verified").toBool();

        addDeviceProfile(p);
    }
    return true;
}

bool CapabilityDatabase::loadFromDirectory(const QString& dirPath) {
    QDir dir(dirPath);
    if (!dir.exists()) return false;

    QStringList filters;
    filters << "*.json";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    bool anyLoaded = false;
    for (const QFileInfo& fi : files) {
        // Task 4: files starting with '_' are schemas/metadata, not device data.
        if (fi.fileName().startsWith('_')) continue;
        if (loadFromFile(fi.absoluteFilePath())) {
            anyLoaded = true;
        }
    }
    return anyLoaded;
}

bool CapabilityDatabase::loadBuiltinDatabase() {
    initBuiltinDefaults();
    loadShippedDeviceDatabase();
    return true;
}

void CapabilityDatabase::loadShippedDeviceDatabase() {
    // Task 4: overlay the vendor-verified device database shipped in
    // resources/devices/*.json. The Qt resource copy is used in installed builds
    // and in tests; the filesystem fallbacks cover raw build trees. JSON files are
    // merged onto the compiled-in defaults, which remain the safety net for fields
    // that a file does not specify.
    const QStringList candidates = {
        QStringLiteral(":/devices"),
        QStringLiteral("resources/devices"),
        QStringLiteral("../resources/devices")
    };
    for (const QString& dir : candidates) {
        if (loadFromDirectory(dir)) {
            return;
        }
    }
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
