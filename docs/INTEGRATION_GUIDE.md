# Hardware Integration Guide for Embedded Device Manufacturers

## Overview

This guide provides step-by-step instructions for microcontroller manufacturers, OEM hardware developers, and firmware engineers to integrate the **Embedded UI Designer** platform onto target hardware platforms.

---

## Supported Hardware Architectures

| Target SoC / Board | Architecture | Flash / SRAM | Toolchain / Programmer | Supported HAL Drivers |
| :--- | :--- | :--- | :--- | :--- |
| **STM32F030R8** | ARM Cortex-M0 | 64KB / 8KB | `arm-none-eabi-gcc` + OpenOCD / ST-Link | GPIO, ADC, PWM, UART, SPI |
| **ESP32 / ESP32-S3** | Xtensa / RISC-V | 4MB-16MB / 512KB | ESP-IDF (`idf.py`) + esptool | GPIO, ADC, PWM, I2C, SPI, Wi-Fi |
| **RP2040** | Dual Cortex-M0+ | 2MB / 264KB | Pico SDK + picotool / CMSIS-DAP | GPIO, ADC, PWM, PIO, I2C, SPI |
| **NXP i.MX RT1060** | ARM Cortex-M7 | 8MB / 1MB | MCUXpresso + J-Link | GPIO, ADC, FlexPWM, Ethernet |

---

## 1. Pin Layout Configuration

Define the hardware pin capabilities in your project schema or board descriptor. For example, in [device-config.json](file:///home/cherry/embedded-ui-designer/device-config.json):

```json
{
  "board": "STM32F030R8",
  "clockFrequency": 48000000,
  "peripherals": {
    "gpio": [
      { "pin": 5, "label": "LED_STATUS", "mode": "OUTPUT_PP" },
      { "pin": 13, "label": "USER_BUTTON", "mode": "INPUT_PULLUP" }
    ],
    "adc": [
      { "channel": 0, "pin": 0, "resolution": 12, "samplingRate": 1000 }
    ],
    "pwm": [
      { "channel": 1, "pin": 3, "timer": "TIM3", "frequency": 10000 }
    ]
  }
}
```

---

## 2. Firmware Compilation & Flashing

### C Bare-Metal (STM32 & RP2040)
```bash
cd firmware
mkdir -p build && cd build
cmake -DBOARD=STM32F030R8 -DCMAKE_TOOLCHAIN_FILE=../toolchain/arm-none-eabi.cmake ..
make -j4
openocd -f interface/stlink.cfg -f target/stm32f0x.cfg -c "program firmware.elf verify reset exit"
```

### Modern C++ (ESP32 / ESP-IDF)
```bash
cd firmware-cpp
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### Memory-Safe Rust (embedded-hal)
```bash
cd firmware-rust
cargo build --release --target thumbv7em-none-eabihf
probe-run --chip STM32F030R8 target/thumbv7em-none-eabihf/release/firmware
```

---

## 3. Web Asset Bundling

Embed the static frontend assets from `src/frontend/dist/` into device read-only flash (SPIFFS / LittleFS or compiled C byte arrays):

```bash
./deploy.sh --target STM32F030R8 --port /dev/ttyUSB0
```

---

## 4. REST API Verification

Test communication with the onboard embedded web server:

```bash
# Verify device health
curl -s http://192.168.1.100:8080/health

# Read GPIO pin
curl -X GET "http://192.168.1.100:8080/api/v1/readGpio?pin=5" \
  -H "Authorization: ApiKey test-api-key-123" \
  -H "X-API-Secret: secret-value-456"

# Set PWM duty cycle
curl -X POST "http://192.168.1.100:8080/api/v1/setPwm" \
  -H "Authorization: ApiKey test-api-key-123" \
  -H "X-API-Secret: secret-value-456" \
  -H "Content-Type: application/json" \
  -d '{"pin": 3, "dutyCycle": 75}'
```
