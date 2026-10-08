# Supported Boards & Profiles — Phase 3

This document lists the representative boards, MCU families, and custom board definition formats supported by the **Hardware Abstraction Layer (HAL)**.

---

## 1. Built-in Board Profiles

| Family | Board / Target Name | MCU Part Number | Architecture | Connection | Supported Features |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **STM32** | NUCLEO-F030R8 | STM32F030R8 | ARM Cortex-M0 | OpenOCD (ST-Link) | GPIO (Atomic BSRR), ADC (12-bit), PWM, SPI, I2C, Reset |
| **STM32** | STM32F4-Discovery | STM32F407VG | ARM Cortex-M4F | OpenOCD (ST-Link) | GPIO (Atomic BSRR), ADC (12-bit), PWM, SPI, I2C, Reset |
| **STM32** | STM32H747I-DISCO | STM32H747XI | Dual Cortex-M7/M4 | OpenOCD (ST-Link) | GPIO (Atomic BSRR), ADC (16-bit), PWM, SPI, I2C, Reset |
| **ESP32** | ESP32-S3-DevKitC-1 | ESP32-S3 | Xtensa Dual-Core LX7 | Serial / USB CDC | GPIO (IO0-IO48), ADC (12-bit), LEDC PWM, UART, Reset |
| **ESP32** | ESP32-WROOM-32 | ESP32-D0WDQ6 | Xtensa Dual-Core LX6 | Serial / USB CDC | GPIO (IO0-IO39), ADC (12-bit), LEDC PWM, UART, Reset |
| **Raspberry Pi** | 40-Pin Header Profile | BCM2835 / BCM2711 | ARM Cortex-A | Linux Sysfs / Drivers | GPIO (GPIO2-GPIO27), PWM, SPI (`/dev/spidev0.0`), I2C (`/dev/i2c-1`) |
| **Raspberry Pi** | Raspberry Pi Pico | RP2040 | Dual Cortex-M0+ | OpenOCD / Serial | GPIO (GP0-GP29), ADC (12-bit), PWM, SPI, I2C |
| **Mock** | Virtual Simulation Target | Virtual-MCU | In-Memory | Mock Loopback | Full Simulation (GPIO In/Out, ADC, PWM, SPI, I2C) |

---

## 2. Custom Board Configuration

Custom boards can be defined dynamically without modifying C++ code using the standard JSON format:

```json
{
  "id": "custom_hmi_controller",
  "name": "Custom Industrial Display HMI",
  "family": "Custom",
  "mcuModel": "STM32F429ZIT6",
  "boardId": "HMI_REV_B",
  "architecture": "ARM Cortex-M4F",
  "backendType": "stm32",
  "connectionType": "openocd",
  "capabilities": {
    "gpioInput": true,
    "gpioOutput": true,
    "adc": true,
    "pwm": true,
    "uart": true,
    "spi": true,
    "i2c": true,
    "can": true,
    "reset": true,
    "liveMonitoring": true,
    "atomicBsrr": true
  },
  "pinMappings": {
    "STATUS_LED": "PD12",
    "RELAY_OUTPUT": "PC8",
    "ANALOG_POT": "PA1",
    "MOTOR_PWM": "PB0"
  },
  "metadata": {
    "voltage": "3.3V",
    "creator": "Embedded Engineering Team"
  }
}
```
