#ifndef BOARD_HAL_H
#define BOARD_HAL_H

#include <QString>
#include <cstdint>

/**
 * @class BoardHAL
 * @brief Abstract interface for board-specific GPIO operations.
 * 
 * Each MCU has different:
 * - GPIO register base addresses
 * - Register offsets (BSRR, BRR, IDR, GPIO_OUT_W1TS, etc.)
 * - Register bit layouts
 * 
 * This interface abstracts those differences so OcdTelnetManager
 * can work with any board via OpenOCD Telnet.
 */
class BoardHAL {
public:
    virtual ~BoardHAL() = default;

    /**
     * @brief Get the GPIO port base address for a given port (A, B, C, etc.)
     * @param port Port letter ('A' = 0, 'B' = 1, etc.)
     * @return Base memory address for that port's GPIO block
     */
    virtual uint32_t getGPIOBaseAddress(uint8_t port) const = 0;

    /**
     * @brief Get register offset for GPIO_BSRR (Bit Set-Reset Register).
     * Used to SET a pin HIGH (or TOGGLE via two operations).
     * @return Register offset from GPIO base (e.g., 0x18 for STM32, varies for ESP32)
     */
    virtual uint32_t getRegisterOffset_BSRR() const = 0;

    /**
     * @brief Get register offset for GPIO_BRR (Bit Reset Register).
     * Used to SET a pin LOW.
     * @return Register offset from GPIO base (e.g., 0x1C for STM32, varies for ESP32)
     */
    virtual uint32_t getRegisterOffset_BRR() const = 0;

    /**
     * @brief Get register offset for GPIO_IDR (Input Data Register).
     * Used to READ the current state of a pin.
     * @return Register offset from GPIO base (e.g., 0x10 for STM32)
     */
    virtual uint32_t getRegisterOffset_IDR() const = 0;

    /**
     * @brief Check if this HAL supports the given port.
     * Some boards have limited GPIO ports (e.g., STM32F030R8 has PA-PF).
     * @param port Port index (0=A, 1=B, etc.)
     * @return true if valid, false otherwise
     */
    virtual bool isValidPort(uint8_t port) const = 0;

    /**
     * @brief Check if this HAL supports the given pin.
     * Some MCUs have fewer GPIO pins per port.
     * @param port Port index
     * @param pin Pin index (0-15 typical)
     * @return true if valid, false otherwise
     */
    virtual bool isValidPin(uint8_t port, uint8_t pin) const = 0;

    /**
     * @brief Get human-readable board name.
     * @return e.g., "STM32F030R8", "ESP32", etc.
     */
    virtual QString getBoardName() const = 0;

    /**
     * @brief Get board manufacturer.
     * @return e.g., "STMicroelectronics", "Espressif"
     */
    virtual QString getManufacturer() const = 0;

    /**
     * @brief Calculate the bit mask for setting pin(s) HIGH.
     * For STM32: pin number (each pin = 1 bit)
     * For ESP32: same (but verify register layout)
     * @param pin Pin number (0-15)
     * @return Bitmask value
     */
    virtual uint32_t getBitMaskForPin(uint8_t pin) const = 0;

    /**
     * @brief Parse GPIO pin string to port/pin indices.
     * Example: "PA4" → port=0, pin=4
     * @param pinString Pin notation (e.g., "PA4", "GPIO18")
     * @param outPort Filled with port index
     * @param outPin Filled with pin index
     * @return true if parsing succeeded
     */
    virtual bool parsePinString(const QString& pinString, uint8_t& outPort, uint8_t& outPin) const = 0;

    /**
     * @brief Convert port/pin indices back to string.
     * Example: port=0, pin=4 → "PA4"
     * @param port Port index
     * @param pin Pin index
     * @return Pin string notation
     */
    virtual QString pinToString(uint8_t port, uint8_t pin) const = 0;

    /**
     * @brief (Optional) Get board-specific quirks or limitations.
     * e.g., "No pull-up on PA0", "PWM on pins 1-4 only"
     * @return Description string
     */
    virtual QString getQuirksAndLimitations() const { return "None"; }
};

#endif // BOARD_HAL_H
