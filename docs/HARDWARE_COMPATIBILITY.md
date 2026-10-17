# Hardware Compatibility Matrix

This document provides the hardware compatibility specification for the Embedded UI Designer Platform across supported microcontroller families and development evaluation boards.

## Supported Target Platforms

| Family | MCU / Target Part | CPU Architecture | Frequency | RAM | Flash | Peripherals Supported | HAL Driver Backend | Verification Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **STM32** | STM32F030R8 | ARM Cortex-M0 | 48 MHz | 8 KB | 64 KB | GPIO, ADC (12-bit), PWM, SPI, UART | `Stm32DriverPack` / OpenOCD ST-Link | Compile-checked (`arm-none-eabi-gcc -fsyntax-only`), golden regression tested |
| **STM32** | STM32F401RE / F407VG | ARM Cortex-M4F | 84-168 MHz | 96-192 KB | 512-1024 KB | GPIO, ADC (12-bit), PWM, SPI, I2C, CAN | `Stm32DriverPack` / OpenOCD ST-Link | Compile-checked (`arm-none-eabi-gcc -fsyntax-only`) |
| **STM32** | STM32H745 / H747XI | Dual Cortex-M7/M4 | 480 MHz | 1 MB | 2 MB | High-res Display, LTDC, FMC, Dual Core | No HAL pack; UI export (LVGL/Qt for MCUs) | UI export tested; HAL blocked by error diagnostic |
| **ESP32** | ESP32-D0WDQ6 | Xtensa Dual LX6 | 240 MHz | 520 KB | 4-16 MB | GPIO, ADC, PWM (LEDC), I2C, SPI, WiFi, BLE | `Esp32DriverPack` / ESP-IDF CDC | Output-tested only (project tree & ESP-IDF code verified by unit tests; not tested on physical hardware) |
| **ESP32** | ESP32-S3 | Xtensa Dual LX7 | 240 MHz | 512 KB | 8-32 MB | GPIO, Octal SPI PSRAM, USB OTG, AI Vector | `Esp32DriverPack` / ESP-IDF CDC | Output-tested only (project tree & ESP-IDF code verified by unit tests; not tested on physical hardware) |
| **RP2040** | Raspberry Pi Pico | Dual Cortex-M0+ | 133 MHz | 264 KB | 2 MB | GPIO, ADC, PWM, PIO State Machines, SPI | `Rp2040DriverPack` / Pico SDK Picoprobe | Output-tested only (project tree & Pico SDK code verified by unit tests; not tested on physical hardware) |
| **NXP** | i.MX RT1060 | ARM Cortex-M7 | 600 MHz | 1 MB | External | LCDIF, 2D GPU, High-speed USB, CAN-FD | `NxpDriverPack` / MCUXpresso J-Link | Output-tested only (project tree & MCUXpresso CMSIS code verified by unit tests; not tested on physical hardware) |
| **Nordic** | nRF52840 | ARM Cortex-M4F | 64 MHz | 256 KB | 1 MB | GPIO, ADC, PWM, BLE 5.0, USB, QSPI | `NordicDriverPack` / Segger J-Link | Output-tested only (project tree & Zephyr/nRF Connect code verified by unit tests; not tested on physical hardware) |

## Hardware Capabilities

1. **GPIO Operations**:
   - Atomic BSRR / BRR port register writes for sub-microsecond pulse timing.
   - Debounced digital input sampling with configurable interrupt triggers (rising, falling, change).

2. **Analog Inputs (ADC)**:
   - 12-bit / 16-bit successive approximation register conversion.
   - Calibrated voltage scaling and oversampling filters.

3. **Pulse Width Modulation (PWM)**:
   - Configurable period and duty cycle (0-100%).
   - Timer hardware channels for motor control and LED backlight dimming.

4. **Communication Buses**:
   - Hardware SPI master up to 40 MHz.
   - Standard (100 kHz) and Fast (400 kHz) I2C bus masters.
