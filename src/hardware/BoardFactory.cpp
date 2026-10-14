#include "BoardFactory.h"
#include "STM32HAL.h"
#include "ESP32HAL.h"
#include <QDebug>

QString BoardFactory::detectBoardFromOpenOCD(const QString& openocdOutput) {
    // OpenOCD "targets" output contains chip name
    // Examples:
    // "stm32f0x.cpu"
    // "esp32.cpu"
    
    if (openocdOutput.contains("stm32f0", Qt::CaseInsensitive)) {
        return "STM32F030R8";
    }
    if (openocdOutput.contains("esp32", Qt::CaseInsensitive)) {
        // Could be "esp32", "esp32s3", etc.
        if (openocdOutput.contains("s3", Qt::CaseInsensitive)) {
            return "ESP32-S3";
        }
        return "ESP32";
    }
    if (openocdOutput.contains("rp2040", Qt::CaseInsensitive)) {
        return "RP2040";  // Future support
    }
    
    qWarning() << "Unknown board detected. Output:" << openocdOutput;
    return "UNKNOWN";
}

std::unique_ptr<BoardHAL> BoardFactory::createHAL(const QString& boardName) {
    if (boardName.startsWith("STM32", Qt::CaseInsensitive)) {
        return std::make_unique<STM32HAL>();
    }
    if (boardName.startsWith("ESP32", Qt::CaseInsensitive)) {
        return std::make_unique<ESP32HAL>();
    }
    
    qWarning() << "Unsupported board:" << boardName;
    return nullptr;  // Caller must handle null
}

QStringList BoardFactory::getSupportedBoards() {
    return {
        "STM32F030R8",
        "ESP32",
        "ESP32-S3",
        "ESP32-DEVKITV1",
        "ESP32-DEVKITV1-USB"
        // Future: "RP2040", "NXP i.MX RT1062", etc.
    };
}

bool BoardFactory::isSupported(const QString& boardName) {
    const QStringList supported = getSupportedBoards();
    for (const QString& item : supported) {
        if (item.compare(boardName, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return boardName.startsWith("STM32", Qt::CaseInsensitive) || boardName.startsWith("ESP32", Qt::CaseInsensitive);
}

QString BoardFactory::getOpenOCDConfigPath(const QString& boardName) {
    if (boardName.contains("CP210", Qt::CaseInsensitive)) {
        return ":/config/openocd/esp32-devkitv1-cp210x.cfg";
    }
    if (boardName == "ESP32-DEVKITV1" || boardName == "ESP32") {
        return ":/config/openocd/esp32-devkitv1.cfg";
    }
    if (boardName == "ESP32-DEVKITV1-USB") {
        return ":/config/openocd/esp32-devkitv1-usb.cfg";
    }
    if (boardName == "STM32F030R8" || boardName == "STM32") {
        return ":/config/openocd/stm32f030r8.cfg";
    }
    
    return "";
}

