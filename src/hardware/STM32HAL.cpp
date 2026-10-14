#include "STM32HAL.h"
#include <QRegularExpression>

/**
 * STM32F030 GPIO Base Addresses
 * Each port occupies 0x400 bytes in memory.
 */
uint32_t STM32HAL::getGPIOBaseAddress(uint8_t port) const {
    const uint32_t GPIO_BASE = 0x48000000;
    const uint32_t PORT_OFFSET = 0x400;
    return GPIO_BASE + (port * PORT_OFFSET);
}

bool STM32HAL::isValidPort(uint8_t port) const {
    // STM32F030R8 supports ports A-F (0-5)
    return port <= 5;  // PA=0, PB=1, ..., PF=5
}

bool STM32HAL::isValidPin(uint8_t port, uint8_t pin) const {
    // STM32F030R8 supports ports A-F and pins 0-15
    if (port > 5 || pin > 15) {
        return false;
    }
    // PA13-PA14 reserved for debugging (SWD)
    if (port == 0 && (pin == 13 || pin == 14)) {
        return false;
    }
    return true;
}

uint32_t STM32HAL::getBitMaskForPin(uint8_t pin) const {
    // STM32 uses simple bit shift: pin 4 → 1 << 4 = 0x10
    return (1U << pin);
}

bool STM32HAL::parsePinString(const QString& pinString, uint8_t& outPort, uint8_t& outPin) const {
    // Expected format: "PA4", "PB12", "PF15", etc.
    QRegularExpression regex("^P([A-F])(\\d{1,2})$", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = regex.match(pinString.trimmed());
    
    if (!match.hasMatch()) {
        return false;
    }
    
    QString portChar = match.captured(1).toUpper();  // "A", "B", ..., "F"
    QString pinNum = match.captured(2);              // "4", "12", etc.
    
    outPort = portChar.at(0).toLatin1() - 'A';       // Convert 'A' → 0, 'B' → 1, etc.
    outPin = static_cast<uint8_t>(pinNum.toUShort());
    
    return isValidPin(outPort, outPin);
}

QString STM32HAL::pinToString(uint8_t port, uint8_t pin) const {
    if (!isValidPin(port, pin)) {
        return "INVALID";
    }
    char portChar = 'A' + port;
    return QString("P%1%2").arg(portChar).arg(pin);
}
