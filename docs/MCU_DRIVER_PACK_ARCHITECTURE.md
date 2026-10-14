# Complete MCU-Specific Driver Library and Capability-Aware Project Architecture

## Executive Summary

Embedded UI Designer enforces a **device-specific, capability-aware driver architecture** that rejects the concept of a single "generic" driver pretending to support all microcontrollers. Every microcontroller family (STM32, ESP32, RP2040, NXP i.MX RT, Nordic nRF52, etc.) possesses unique register layouts, clock topologies, interrupt controllers, memory mappings, and toolchain conventions.

---

## 1. Capability-Aware Project Generation Workflow

```text
┌───────────────────────────────────────┐
│     Select Target MCU & Board         │ (e.g. STM32F030R8, ESP32, RP2040)
└──────────────────┬────────────────────┘
                   │
                   ▼
┌───────────────────────────────────────┐
│   Query Authoritative Device Spec     │ (Memory maps, register bases, vectors)
└──────────────────┬────────────────────┘
                   │
                   ▼
┌───────────────────────────────────────┐
│   Resolve Driver Dependencies Graph   │ (USART -> RCC + GPIO + NVIC)
└──────────────────┬────────────────────┘
                   │
                   ▼
┌───────────────────────────────────────┐
│  Filter Unsupported Hardware Modules  │ (Rejects CAN/DMA if peripheral absent)
└──────────────────┬────────────────────┘
                   │
                   ▼
┌───────────────────────────────────────┐
│  Generate Family-Specific Hierarchy   │ (STM32 Bare-Metal, ESP-IDF, Pico SDK)
└──────────────────┬────────────────────┘
                   │
                   ▼
┌───────────────────────────────────────┐
│  Startup, Linker & Toolchain Config   │ (Real memory origin/lengths, flags)
└──────────────────┬────────────────────┘
                   │
                   ▼
┌───────────────────────────────────────┐
│    Compile, Validate & Deploy         │ (Native arm-none-eabi-gcc / idf.py)
└───────────────────────────────────────┘
```

---

## 2. Supported MCU Families & Generated Project Trees

### A. Bare-Metal STM32 (e.g., STM32F030R8, STM32F401RE)

For a bare-metal STM32 target, the generated project produces an exact, standard embedded layout:

```text
MyProject/
├── Core/
│   ├── Inc/
│   │   ├── main.h             # System initialization declarations
│   │   └── app_config.h       # Clock frequency and target device defines
│   └── Src/
│       ├── main.c             # System clock setup, GPIO config, main loop
│       └── app.c              # Application logic
│
├── Drivers/
│   ├── CMSIS/                 # CMSIS Core support
│   ├── Device/
│   │   └── STM32F030x8.h      # Real register offsets, base addresses (GPIOA, RCC, etc.)
│   ├── Inc/
│   │   ├── RCC_Driver.h       # Clock and reset configuration
│   │   ├── NVIC_Driver.h      # Nested Vectored Interrupt Controller
│   │   ├── SYSCFG_Driver.h    # System configuration & EXTI routing
│   │   ├── EXTI_Driver.h      # External interrupt lines
│   │   ├── GPIO_Driver.h      # Pin modes, pull-up/down, atomic BSRR/BRR
│   │   ├── USART_Driver.h     # Serial communication
│   │   ├── I2C_Driver.h       # Two-wire interface
│   │   ├── SPI_Driver.h       # Serial peripheral interface
│   │   ├── ADC_Driver.h       # Analog-to-digital converter
│   │   ├── Timer_Driver.h     # Timers and PWM generation
│   │   └── DMA_Driver.h       # Direct Memory Access (included only if MCU has DMA)
│   └── Src/
│       ├── RCC_Driver.c
│       ├── NVIC_Driver.c
│       ├── SYSCFG_Driver.c
│       ├── EXTI_Driver.c
│       ├── GPIO_Driver.c
│       ├── USART_Driver.c
│       ├── I2C_Driver.c
│       ├── SPI_Driver.c
│       ├── ADC_Driver.c
│       ├── Timer_Driver.c
│       └── DMA_Driver.c
│
├── Startup/
│   └── startup.s              # Real interrupt vector table and Reset_Handler
├── Linker/
│   └── linker.ld              # FLASH (0x08000000, 64K), RAM (0x20000000, 8K)
├── Middleware/
│   └── WiFi_AT/               # External AT-command module driver (when Wi-Fi requested)
├── generated/                 # UI-bound telemetry and serialization code
├── Build/                     # Output directory for ELF and BIN
├── Makefile                   # arm-none-eabi-gcc toolchain build script
├── README.md                  # Project documentation
└── .gitignore                 # Build artifacts exclusion
```

### B. Espressif ESP32 / ESP32-S3 (ESP-IDF Structure)

Preserves native ESP-IDF conventions:

```text
MyProject/
├── CMakeLists.txt             # include($ENV{IDF_PATH}/tools/cmake/project.cmake)
├── main/
│   ├── CMakeLists.txt         # idf_component_register(...)
│   ├── main.c                 # app_main(void), nvs_flash_init, Wi-Fi initialization
│   ├── app.h / app.c
│   └── app_config.h
├── components/
│   └── drivers/
│       ├── CMakeLists.txt
│       ├── include/           # gpio_driver.h, adc_driver.h, wifi_driver.h, ble_driver.h
│       └── src/               # Native ESP-IDF driver implementations
├── partitions.csv             # Custom partition layout (NVS, OTA, Factory)
├── sdkconfig.defaults         # CPU frequency (240MHz), Flash size (4MB)
├── README.md
└── .gitignore
```

### C. Raspberry Pi RP2040 (Pico SDK Structure)

Preserves Pico SDK conventions:

```text
MyProject/
├── CMakeLists.txt             # pico_sdk_init(), target_link_libraries(pico_stdlib ...)
├── pico_sdk_import.cmake      # Official SDK import script
├── include/
│   └── app_config.h
├── src/
│   ├── main.c                 # stdio_init_all(), hardware/gpio.h, hardware/adc.h
│   └── app.c
├── README.md
└── .gitignore
```

### D. NXP i.MX RT Series (MCUXpresso SDK)

Preserves NXP SDK conventions:

```text
MyProject/
├── source/main.c              # BOARD_InitBootPins(), BOARD_InitBootClocks()
├── board/board.h / board.c    # Pin routing and clock tree configuration
├── drivers/fsl_gpio.h / .c    # NXP FSL peripheral drivers
├── README.md
└── .gitignore
```

### E. Nordic Semiconductor nRF52 Series (nRF Connect / Zephyr)

Preserves Zephyr / nRF Connect SDK conventions:

```text
MyProject/
├── CMakeLists.txt             # find_package(Zephyr REQUIRED)
├── prj.conf                   # CONFIG_BT=y, CONFIG_BT_PERIPHERAL=y
├── src/main.c                 # bt_enable(NULL), zephyr/drivers/gpio.h
├── README.md
└── .gitignore
```

---

## 3. Dependency Graph Resolution

When peripherals are requested, required prerequisites are resolved automatically:

```text
USART Driver ──────────► Requires RCC + GPIO + NVIC
I2C Driver ────────────► Requires RCC + GPIO
SPI Driver ────────────► Requires RCC + GPIO
Timer / PWM Driver ────► Requires RCC + GPIO + TIM
EXTI Driver ───────────► Requires SYSCFG + NVIC + GPIO
External Wi-Fi Module ─► Requires USART + GPIO + Timer
Internal Wi-Fi ────────► Requires Onboard Radio + Network Stack (ESP32 only)
Internal Bluetooth ────► Requires Onboard BLE Controller (ESP32, nRF52 only)
```

---

## 4. Connectivity Integrity (No "Fake" Features)

* **Integrated Wireless**: Only MCUs with on-die radios (e.g. ESP32, nRF52840) generate native wireless stacks (`esp_wifi`, Zephyr Bluetooth).
* **External Modules**: For MCUs without integrated radios (e.g. STM32, RP2040, NXP i.MX RT), Embedded UI Designer **never claims integrated wireless**. Instead, it generates a host-interface driver (e.g. `Middleware/WiFi_AT/esp_at_driver.h/.c`) operating over UART or SPI with command parsing and status monitoring.

---

## 5. Automated Verification & Test Results

The dedicated test suite [tests/test_mcu_driver_packs.cpp](file:///home/cherry/embedded-ui-designer/tests/test_mcu_driver_packs.cpp) verifies all 8 criteria with 100% pass rate:

```
============================================================
  MCU-SPECIFIC DRIVER LIBRARY & PROJECT TREE TESTS (8 TESTS)
============================================================

  [PASS] STM32 Project Tree: Complete Core, Drivers, Startup, and Linker files generated
  [PASS] Capability Validation: Unsupported peripheral (CAN) rejected with descriptive error
  [PASS] Driver Dependencies: USART automatically pulled RCC, GPIO, and NVIC prerequisites
  [PASS] External Wi-Fi: STM32 cleanly integrates external AT module driver over UART
  [PASS] ESP32 Project Tree: ESP-IDF component structure with native Wi-Fi & BLE generated
  [PASS] RP2040 Project Tree: Pico SDK CMake structure with hardware headers generated
  [PASS] NXP & Nordic Project Trees: Vendor-accurate SDK conventions generated
  [PASS] Filesystem Export & Compilation: Generated C drivers compile without error

============================================================
RESULTS: 8/8 MCU driver library tests passed (100%)
============================================================
```
