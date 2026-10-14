#ifndef STM32_HAL_H
#define STM32_HAL_H

#include "BoardHAL.h"

/**
 * @class STM32HAL
 * @brief Hardware abstraction for STM32F030R8 (and compatible STM32F0 series).
 * 
 * GPIO Register Layout (STM32F030R8):
 * - GPIOA base: 0x48000000
 * - GPIOB base: 0x48000400
 * - etc.
 * 
 * Register Offsets from GPIO base:
 * - MODER (Mode Register):       0x00
 * - OTYPER (Output Type):        0x04
 * - OSPEEDR (Output Speed):      0x08
 * - PUPDR (Pull-up/Pull-down):   0x0C
 * - IDR (Input Data Register):   0x10
 * - ODR (Output Data Register):  0x14
 * - BSRR (Bit Set/Reset):        0x18  ← Use for SET HIGH
 * - BRR (Bit Reset):             0x1C  ← Use for SET LOW
 * - LCKR (Lock Register):        0x1C
 * 
 * GPIO Ports available: PA, PB, PC, PD, PE, PF (6 ports, A=0 to F=5)
 * Pins per port: 0-15 (16 pins)
 * 
 * Pin string format: "PA4", "PB12", etc.
 */
class STM32HAL : public BoardHAL {
public:
    uint32_t getGPIOBaseAddress(uint8_t port) const override;
    uint32_t getRegisterOffset_BSRR() const override { return 0x18; }
    uint32_t getRegisterOffset_BRR() const override { return 0x1C; }
    uint32_t getRegisterOffset_IDR() const override { return 0x10; }
    
    bool isValidPort(uint8_t port) const override;
    bool isValidPin(uint8_t port, uint8_t pin) const override;
    
    QString getBoardName() const override { return "STM32F030R8"; }
    QString getManufacturer() const override { return "STMicroelectronics"; }
    
    uint32_t getBitMaskForPin(uint8_t pin) const override;
    bool parsePinString(const QString& pinString, uint8_t& outPort, uint8_t& outPin) const override;
    QString pinToString(uint8_t port, uint8_t pin) const override;
    
    QString getQuirksAndLimitations() const override {
        return "PA13-PA14 reserved for debugging (SWD). "
               "LD2 (user LED) on PA5. "
               "LD1 (power LED) on PA9.";
    }
};

#endif // STM32_HAL_H
