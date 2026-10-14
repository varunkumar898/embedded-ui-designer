#ifndef ESP32_HAL_H
#define ESP32_HAL_H

#include "BoardHAL.h"

/**
 * @class ESP32HAL
 * @brief Hardware abstraction for ESP32 / ESP32-S3.
 * 
 * GPIO Register Layout (ESP32):
 * - GPIO base: 0x3FF44000 (different from STM32!)
 * 
 * Register Offsets:
 * - GPIO_OUT (Output Data):      0x04
 * - GPIO_OUT_W1TS (Set output):  0x08  ← Use for SET HIGH
 * - GPIO_OUT_W1TC (Clear output):0x0C  ← Use for SET LOW
 * - GPIO_IN (Input Data):        0x3C  ← Use for READ
 * 
 * GPIO Pins: 0-39 (40 pins, but not all usable)
 * Usable pins: 0-21, 25-27, 32-39 (internal use on some)
 * 
 * Pin string format: "GPIO0", "GPIO18", etc. (or "0", "18")
 */
class ESP32HAL : public BoardHAL {
public:
    uint32_t getGPIOBaseAddress(uint8_t port) const override;
    uint32_t getRegisterOffset_BSRR() const override { return 0x08; }  // GPIO_OUT_W1TS (SET)
    uint32_t getRegisterOffset_BRR() const override { return 0x0C; }   // GPIO_OUT_W1TC (CLEAR)
    uint32_t getRegisterOffset_IDR() const override { return 0x3C; }   // GPIO_IN (READ)
    
    bool isValidPort(uint8_t port) const override;
    bool isValidPin(uint8_t port, uint8_t pin) const override;
    
    QString getBoardName() const override { return "ESP32"; }
    QString getManufacturer() const override { return "Espressif"; }
    
    uint32_t getBitMaskForPin(uint8_t pin) const override;
    bool parsePinString(const QString& pinString, uint8_t& outPort, uint8_t& outPin) const override;
    QString pinToString(uint8_t port, uint8_t pin) const override;
    
    QString getQuirksAndLimitations() const override {
        return "GPIO0 used for boot mode. "
               "GPIO6-11 reserved for SPI Flash. "
               "GPIO12-15 need boot strapping. "
               "Pins 28-31 reserved. "
               "GPIO0 at startup must be HIGH for normal boot.";
    }
};

#endif // ESP32_HAL_H
