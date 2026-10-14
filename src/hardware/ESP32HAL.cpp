#include "ESP32HAL.h"
#include <QRegularExpression>

/**
 * ESP32 GPIO Base Address
 * GPIO base: 0x3FF44000
 * NOTE: ESP32 doesn't have traditional "ports" like STM32.
 * It has 40 GPIO pins (GPIO0-GPIO39) in a flat namespace.
 * We simulate "ports" by dividing into groups:
 * - Port 0: GPIO0-GPIO15
 * - Port 1: GPIO16-GPIO31
 * - Port 2: GPIO32-GPIO39
 */
uint32_t ESP32HAL::getGPIOBaseAddress(uint8_t /*port*/) const {
    // For ESP32, port index doesn't matter - all pins use the same base address
    // Port parameter is ignored; we use pin number directly
    return 0x3FF44000;
}

bool ESP32HAL::isValidPort(uint8_t port) const {
    // ESP32 doesn't have traditional ports, but we simulate them
    // Port 0: GPIO0-15, Port 1: GPIO16-31, Port 2: GPIO32-39
    return port <= 2;
}

bool ESP32HAL::isValidPin(uint8_t port, uint8_t pin) const {
    // Virtual port layout:
    // Port 0: pins 0-15 → GPIO0-GPIO15
    // Port 1: pins 0-15 → GPIO16-GPIO31
    // Port 2: pins 0-7  → GPIO32-GPIO39
    
    if (port == 0) {
        // GPIO6-11 reserved for SPI Flash
        if (pin >= 6 && pin <= 11) {
            return false;
        }
        return pin <= 15;
    }
    if (port == 1) {
        // GPIO28-31 reserved (pins 12-15 on port 1)
        if (pin >= 12 && pin <= 15) {
            return false;
        }
        return pin <= 15;
    }
    if (port == 2) {
        return pin <= 7;
    }
    return false;
}

uint32_t ESP32HAL::getBitMaskForPin(uint8_t pin) const {
    // ESP32 GPIO uses simple bit shift (same as STM32)
    return (1U << pin);
}

bool ESP32HAL::parsePinString(const QString& pinString, uint8_t& outPort, uint8_t& outPin) const {
    // Expected formats:
    // "GPIO18" or "18"
    QRegularExpression regex("^(?:GPIO)?(\\d{1,2})$", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = regex.match(pinString.trimmed());
    
    if (!match.hasMatch()) {
        return false;
    }
    
    uint16_t gpioNumber = match.captured(1).toUShort();
    
    // Map GPIO number to virtual port/pin
    if (gpioNumber <= 15) {
        outPort = 0;
        outPin = static_cast<uint8_t>(gpioNumber);
    } else if (gpioNumber <= 31) {
        outPort = 1;
        outPin = static_cast<uint8_t>(gpioNumber - 16);
    } else if (gpioNumber <= 39) {
        outPort = 2;
        outPin = static_cast<uint8_t>(gpioNumber - 32);
    } else {
        return false;
    }
    
    return isValidPin(outPort, outPin);
}

QString ESP32HAL::pinToString(uint8_t port, uint8_t pin) const {
    if (!isValidPin(port, pin)) {
        return "INVALID";
    }

    uint8_t gpioNumber = 0;
    if (port == 0) {
        gpioNumber = pin;
    } else if (port == 1) {
        gpioNumber = 16 + pin;
    } else if (port == 2) {
        gpioNumber = 32 + pin;
    } else {
        return "INVALID";
    }
    
    return QString("GPIO%1").arg(gpioNumber);
}
