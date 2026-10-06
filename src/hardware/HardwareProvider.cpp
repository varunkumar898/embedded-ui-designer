#include "HardwareProvider.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace Hardware {

// ── GenericPackProvider ──

GenericPackProvider::GenericPackProvider(const QString& id, const QString& name, const QString& vendor)
    : m_id(id)
    , m_name(name)
    , m_vendor(vendor)
{
}

DeviceDefinition GenericPackProvider::findDevice(const QString& partNumber) const {
    if (m_devices.contains(partNumber)) {
        return m_devices.value(partNumber);
    }
    for (const auto& dev : m_devices) {
        if (dev.partNumber.compare(partNumber, Qt::CaseInsensitive) == 0) {
            return dev;
        }
    }
    return DeviceDefinition();
}

BoardDefinition GenericPackProvider::findBoard(const QString& boardId) const {
    if (m_boards.contains(boardId)) {
        return m_boards.value(boardId);
    }
    for (const auto& b : m_boards) {
        if (b.id.compare(boardId, Qt::CaseInsensitive) == 0) {
            return b;
        }
    }
    return BoardDefinition();
}

void GenericPackProvider::addDevice(const DeviceDefinition& device) {
    if (!device.partNumber.isEmpty()) {
        m_devices[device.partNumber] = device;
    }
}

void GenericPackProvider::addBoard(const BoardDefinition& board) {
    if (!board.id.isEmpty()) {
        m_boards[board.id] = board;
    }
}

bool GenericPackProvider::loadFromDirectory(const QString& dirPath) {
    QDir dir(dirPath);
    if (!dir.exists()) return false;

    QStringList jsonFiles = dir.entryList(QStringList() << "*.json", QDir::Files);
    for (const QString& file : jsonFiles) {
        QFile f(dir.filePath(file));
        if (!f.open(QIODevice::ReadOnly)) continue;
        QByteArray data = f.readAll();
        f.close();

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(data, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) continue;

        QJsonObject root = doc.object();
        if (root.contains("identity")) {
            DeviceDefinition dev = DeviceDefinition::fromJson(root);
            if (!dev.partNumber.isEmpty()) {
                addDevice(dev);
            }
        } else if (root.contains("metadata") && root.value("metadata").toObject().contains("mcuPartNumber")) {
            BoardDefinition b = BoardDefinition::fromJson(root);
            if (!b.id.isEmpty()) {
                addBoard(b);
            }
        }
    }
    return true;
}

// ── Stm32Provider ──

Stm32Provider::Stm32Provider()
    : GenericPackProvider("stm32", "STM32 Provider", "STMicroelectronics")
{
    ToolchainConfiguration tc1;
    tc1.framework = "STM32Cube";
    tc1.toolchain = "arm-none-eabi-gcc";
    tc1.ide = "STM32CubeIDE";
    tc1.library = "HAL";

    ToolchainConfiguration tc2;
    tc2.framework = "CMSIS";
    tc2.toolchain = "arm-none-eabi-gcc";
    tc2.ide = "VS Code";
    tc2.library = "LL";

    m_toolchains = { tc1, tc2 };
}

// ── Esp32Provider ──

Esp32Provider::Esp32Provider()
    : GenericPackProvider("esp32", "ESP32 Provider", "Espressif")
{
    ToolchainConfiguration tc1;
    tc1.framework = "ESP-IDF";
    tc1.toolchain = "xtensa-esp32-elf-gcc";
    tc1.ide = "VS Code / ESP-IDF Extension";
    tc1.library = "ESP-IDF v5.x";

    ToolchainConfiguration tc2;
    tc2.framework = "Arduino-ESP32";
    tc2.toolchain = "xtensa-esp32-elf-gcc";
    tc2.ide = "Arduino IDE / PlatformIO";
    tc2.library = "Arduino Core";

    m_toolchains = { tc1, tc2 };
}

// ── Rp2040Provider ──

Rp2040Provider::Rp2040Provider()
    : GenericPackProvider("rp2040", "RP2040 Provider", "Raspberry Pi")
{
    ToolchainConfiguration tc1;
    tc1.framework = "Pico SDK";
    tc1.toolchain = "arm-none-eabi-gcc";
    tc1.ide = "VS Code / CMake";
    tc1.library = "pico-sdk (C/C++)";

    ToolchainConfiguration tc2;
    tc2.framework = "Arduino-Pico";
    tc2.toolchain = "arm-none-eabi-gcc";
    tc2.ide = "Arduino IDE";
    tc2.library = "Earle F. Philhower Core";

    m_toolchains = { tc1, tc2 };
}

// ── RaspberryPiProvider ──

RaspberryPiProvider::RaspberryPiProvider()
    : GenericPackProvider("raspberrypi", "Raspberry Pi SBC Provider", "Raspberry Pi")
{
    ToolchainConfiguration tc1;
    tc1.framework = "Linux (Raspberry Pi OS)";
    tc1.toolchain = "aarch64-linux-gnu-gcc";
    tc1.ide = "VS Code / CMake / Linux Native";
    tc1.library = "libgpiod / pigpio / C++20";

    m_toolchains = { tc1 };
}

// ── CustomHardwareProvider ──

CustomHardwareProvider::CustomHardwareProvider()
    : GenericPackProvider("custom", "Custom Target Provider", "Custom")
{
    ToolchainConfiguration tc;
    tc.framework = "Custom / Embedded C++";
    tc.toolchain = "gcc";
    tc.ide = "Custom";
    tc.library = "Custom";
    m_toolchains = { tc };
}

void CustomHardwareProvider::removeDevice(const QString& partNumber) {
    m_devices.remove(partNumber);
}

void CustomHardwareProvider::removeBoard(const QString& boardId) {
    m_boards.remove(boardId);
}

} // namespace Hardware
