# Adding Hardware Targets, Device Packs & Boards Guide

This guide documents the Universal Hardware Configuration Architecture implemented in the Embedded UI Designer (Phases 1–34). It explains how to add new MCUs/MPUs, development boards, vendors, custom targets, and how to configure them via the UI Wizard and MCP (Model Context Protocol).

---

## 1. Hardware Architecture Overview

The Embedded UI Designer employs a pluggable, data-driven hardware abstraction layer:

```
                      CLAUDE / ANTIGRAVITY (AI)
                                 │
                                 │ MCP (JSON-RPC)
                                 ▼
                     ┌───────────────────────┐
                     │       MCP LAYER       │
                     │ (embedded_ui_mcp.py)  │
                     └───────────┬───────────┘
                                 │ Localhost Socket
                                 ▼
                     ┌───────────────────────┐
                     │  DESIGNER CONTROLLER  │
                     │  (DesignerController) │
                     └───────────┬───────────┘
                                 │
             ┌───────────────────┴───────────────────┐
             ▼                                       ▼
     UI DESIGN MODEL                         HARDWARE ENGINE
  (CanvasScene, Components)                          │
             │                         ┌─────────────┴─────────────┐
             │                         ▼                           ▼
             │                DeviceDatabase (Packs)         PinMuxEngine
             │                - GenericPackProvider          - Conflict Detection
             │                - Stm32Provider                - Alternate Function (AF)
             │                - Esp32Provider                - Pin Ownership Tracking
             │                - Rp2040Provider               - Peripheral Routing
             │                - RaspberryPiProvider          - Validation Rules
             │                - CustomHardwareProvider
             │                         │                           │
             └─────────────────────────┼───────────────────────────┘
                                       ▼
                                 PROJECT MODEL
                                   (.euiproj)
                                       │
                                       ▼
                                CODE GENERATORS
                           (LVGL / µGFX / Qt Quick UL)
```

The hardware subsystem consists of:
1. **Device Packs (`data/hardware_packs/`)**: Standalone JSON files describing authoritative vendor datasheet specifications.
2. **Device Database (`DeviceDatabase`)**: Singleton catalog indexing all microcontrollers, microprocessors, development boards, vendors, and packages.
3. **Pin Multiplexing Engine (`PinMuxEngine`)**: Real-time pinmux conflict detector validating alternate functions, power/ground rails, debug lines, and peripheral signal allocations.
4. **Hardware Wizard (`HardwareWizardDialog`)**: 6-step desktop dialog styled after technical engineering tools (STM32CubeIDE / CubeMX) with dark theme, pinout view, peripheral inspector, and review summary.
5. **Pinout Visualization (`PinoutViewWidget`)**: High-performance QPainter widget rendering physical packages (LQFP, QFN) and dual-row board headers with interactive pin state colors.
6. **Project Persistence (`Project`)**: Saves `HardwareConfig` (MCU, board, pins, peripherals, clock, toolchain) into the `.euiproj` project format.
7. **MCP Hardware Server (`DesignerController` + `embedded_ui_mcp.py`)**: 20+ specialized tools enabling AI agents to inspect, configure, and bind pins and peripherals live.

---

## 2. Device Database Format (MCU / MPU Definition)

Each microcontroller/microprocessor definition is a JSON file placed in `data/hardware_packs/<vendor_slug>/<part_number_slug>.json`.

### Schema Specification
```json
{
  "schemaVersion": "1.0",
  "provenance": {
    "source": "Authoritative Vendor Datasheet / Reference Manual",
    "vendor": "STMicroelectronics",
    "partNumber": "STM32F407VG",
    "version": "1.0.0"
  },
  "identity": {
    "vendor": "STMicroelectronics",
    "family": "STM32",
    "series": "STM32F4",
    "partNumber": "STM32F407VG",
    "architecture": "ARM Cortex-M4",
    "core": "Cortex-M4F",
    "package": "LQFP100",
    "flash": 1048576,
    "ram": 196608,
    "eeprom": 0,
    "minVoltage": 1.8,
    "maxVoltage": 3.6,
    "maxClockMhz": 168,
    "gpioCount": 82
  },
  "peripherals": {
    "gpio": 82,
    "uart": 4,
    "usart": 2,
    "spi": 3,
    "i2c": 3,
    "can": 2,
    "adc": 3,
    "dac": 2,
    "timers": 14,
    "usb_otg": 2,
    "ethernet": 1
  },
  "peripheralDescriptors": [
    {
      "name": "SPI1",
      "type": "SPI",
      "signalOptions": {
        "SCK": ["PA5", "PB3"],
        "MISO": ["PA6", "PB4"],
        "MOSI": ["PA7", "PB5"],
        "NSS": ["PA4", "PA15"]
      },
      "defaults": {
        "mode": "Master",
        "clockSpeedHz": 10000000,
        "cpol": 0,
        "cpha": 0
      }
    }
  ],
  "clockTree": {
    "defaultSysClockMhz": 168,
    "maxSysClockMhz": 168,
    "hclkMhz": 168,
    "apb1MaxMhz": 42,
    "apb2MaxMhz": 84,
    "oscillatorSources": ["HSI (16 MHz)", "HSE Crystal (8 MHz)", "HSE Bypass"]
  },
  "pins": [
    {
      "name": "PA5",
      "physicalPin": 30,
      "alternateFunctions": ["GPIO_Input", "GPIO_Output", "Analog", "SPI1_SCK", "TIM2_CH1", "DAC_OUT2", "ADC1_IN5"]
    },
    {
      "name": "VDD",
      "physicalPin": 11,
      "alternateFunctions": [],
      "isPower": true
    },
    {
      "name": "VSS",
      "physicalPin": 10,
      "alternateFunctions": [],
      "isGround": true
    },
    {
      "name": "NRST",
      "physicalPin": 14,
      "alternateFunctions": [],
      "isReset": true
    },
    {
      "name": "PA13",
      "physicalPin": 72,
      "alternateFunctions": ["GPIO_Input", "GPIO_Output", "JTMS-SWDIO"],
      "isReserved": true
    }
  ]
}
```

---

## 3. Board Database Format

Development boards are separate from raw chips. A board defines physical connectors (Arduino Uno headers, Raspberry Pi 40-pin GPIO, Morpho, PMOD), pin labels (e.g. `LED_GREEN -> PD12`), power switches, buttons, and onboard peripheral ICs.

### Schema Specification
```json
{
  "schemaVersion": "1.0",
  "provenance": {
    "source": "STMicroelectronics Discovery Kit with STM32F407VG MCU User Manual (UM1472)",
    "vendor": "STMicroelectronics",
    "boardId": "STM32F407G-DISC1",
    "version": "1.0.0"
  },
  "metadata": {
    "id": "STM32F407G-DISC1",
    "name": "STM32F4 Discovery",
    "manufacturer": "STMicroelectronics",
    "boardFamily": "Discovery",
    "mcuPartNumber": "STM32F407VG",
    "architecture": "ARM Cortex-M4",
    "debugInterface": "ST-LINK/V2-A (SWD)",
    "supplyVoltage": "5V (USB) or 3.3V/5V external"
  },
  "physicalPinLabels": {
    "LED_GREEN": "PD12",
    "LED_ORANGE": "PD13",
    "LED_RED": "PD14",
    "LED_BLUE": "PD15",
    "USER_BUTTON": "PA0",
    "AUDIO_DAC_SCL": "PB6",
    "AUDIO_DAC_SDA": "PB9"
  },
  "connectors": [
    {
      "name": "Header P1 (Extension Header)",
      "pinCount": 50,
      "pinMapping": {
        "1": "GND",
        "2": "GND",
        "3": "VDD",
        "4": "VDD",
        "7": "PC1",
        "9": "PC3",
        "11": "PA1"
      }
    }
  ],
  "onboardPeripherals": {
    "audio": "Cirrus Logic CS43L22 Audio DAC with integrated class D speaker driver",
    "motionSensor": "ST MEMS LIS3DSH 3-axis accelerometer",
    "microphone": "ST MEMS MP45DT02 omnidirectional audio sensor",
    "usb": "USB OTG FS Micro-AB connector"
  }
}
```

---

## 4. Custom Hardware Format

Users can create, export, and import custom hardware targets without editing application source code. Custom definitions are stored in `data/hardware_packs/custom/` or user AppData directory.

A custom hardware JSON file must have either:
- `"identity"` with `"partNumber"` (defines a custom microcontroller)
- `"metadata"` with `"id"` and `"mcuPartNumber"` (defines a custom development board)

---

## 5. Pin Multiplexing & Conflict Engine

The `PinMuxEngine` protects physical hardware integrity:

### Rules Enforced:
1. **Power Supply Protection**: Pins flagged `"isPower": true` (e.g. VDD, 3V3, 5V) can never be assigned as GPIO outputs or alternate functions. Attempting to assign produces an error:
   `"Pin 'VDD' is a dedicated power supply rail and cannot be reconfigured."`
2. **Ground Protection**: Pins flagged `"isGround": true` (VSS, GND) are strictly locked.
3. **Hardware Reset**: Pin flagged `"isReset": true` (NRST, CHIP_PU, RUN) is strictly protected.
4. **Reserved/Debug Pins**: Pins flagged `"isReserved": true` (SWDIO, SWCLK, JTAG) alert the user of debugger conflict.
5. **Alternate Function Checking**: Verifies that the requested peripheral function (e.g. `SPI1_SCK`) exists in that pin's datasheet `alternateFunctions` list.
6. **Conflict Resolution**:
   - If a pin is already allocated (e.g. PA5 to `SPI1_SCK`), attempting to assign `GPIO_Output` triggers a conflict:
     `"Pin PA5 is already assigned to: SPI1_SCK. Do you want to replace the existing assignment?"`
   - Passing `force=true` unbinds the previous peripheral signal and reassigns the pin.

---

## 6. How to Add a New MCU Target

To add a new microcontroller (e.g. NXP LPC55S69, Nordic nRF52840, Microchip SAMD21):

1. Create a pack folder: `data/hardware_packs/<vendor_name>/` (e.g. `data/hardware_packs/nordic/`).
2. Create `<part_number>.json` (e.g. `nrf52840.json`).
3. Fill in:
   - `provenance` with exact datasheet and version reference.
   - `identity` with vendor, family, partNumber, core, package, flash, RAM.
   - `peripheralDescriptors` for UART, SPI, I2C, ADC, PWM.
   - `pins` listing pin names and alternate functions.
4. Restart the Embedded UI Designer or call `DeviceDatabase::instance().reloadPacks()` (or MCP `load_hardware_definition`).
5. The MCU immediately appears in the Hardware Wizard MCU selector with full search and filtering.

---

## 7. How to Add a New Board Target

1. Place `<board_id>.json` in `data/hardware_packs/<vendor_name>/` (e.g. `data/hardware_packs/nordic/nrf52840_dk.json`).
2. In `metadata`, specify `mcuPartNumber` pointing to the corresponding MCU partNumber.
3. Define `physicalPinLabels` (e.g. `"LED1": "P0.13"`, `"BUTTON1": "P0.11"`).
4. Define `connectors` for the board's pin headers.
5. Reload packs. The board appears in Tab 2 of the Hardware Project Wizard.

---

## 8. How to Add a New Vendor Architecture

The architecture uses `IHardwareProvider`. You can either:
1. **Drop JSON packs into `data/hardware_packs/<vendor_folder>/`**: The generic pack provider automatically creates a vendor category with full indexing.
2. **Implement a C++ Provider**: Subclass `Hardware::GenericPackProvider` or `Hardware::IHardwareProvider`, implement custom pinmux rules or toolchains, and register via:
   ```cpp
   Hardware::DeviceDatabase::instance().registerProvider(std::make_shared<MyVendorProvider>());
   ```

---

## 9. Model Context Protocol (MCP) Hardware Tools

Claude and Antigravity can query and configure hardware via 20+ specialized tools:

| MCP Tool Name | Description | Key Arguments |
|---|---|---|
| `get_target` | Get active project hardware target, memory & pin counts | none |
| `set_target` | Set active MCU or board target in project | `targetType`, `id` |
| `list_devices` | Search microcontrollers in database | `vendor`, `family`, `search` |
| `list_boards` | Search development boards in database | `vendor`, `mcu`, `search` |
| `get_device_info` | Get full datasheet specs for MCU | `deviceId` |
| `get_board_info` | Get board connectors, LEDs & debug specs | `boardId` |
| `get_pinout` | Retrieve physical package pinout & AF options | `deviceId` (optional) |
| `list_pins` | List current project pins with modes and states | none |
| `get_pin` | Inspect specific pin configuration & owner | `pin` |
| `configure_pin` | Configure mode, label, pull, speed, interrupt | `pin`, `config`, `force` |
| `configure_pins` | Batch configure multiple pins with conflict detection | `assignments`, `force` |
| `list_peripherals` | List on-chip peripherals with enabled state | none |
| `get_peripheral` | Inspect peripheral signals and baud/clock parameters | `name` |
| `configure_peripheral` | Enable peripheral, set parameters, map pins | `name`, `config`, `force` |
| `get_available_pins` | Find pins capable of serving a peripheral signal | `peripheral`, `signal` |
| `get_free_pins` | Get list of unassigned general-purpose I/O pins | none |
| `validate_hardware_configuration` | Run pinmux validation engine for errors/warnings | none |
| `get_hardware_configuration` | Get complete project hardware JSON | none |
| `create_custom_hardware` | Register new custom device/board definition | `definition` |
| `save_hardware_definition` | Export hardware definition to external file | `id`, `filePath` |
| `load_hardware_definition` | Import hardware definition from external file | `filePath` |

---

## 10. Example Natural Language Claude / Antigravity Commands

### Example 1: Select hardware target
**User Prompt**: *"Create an ESP32-S3 project on the DevKitC-1 board."*
**AI Action**:
Calls MCP tool:
```json
{
  "name": "set_target",
  "arguments": {
    "targetType": "board",
    "id": "ESP32-S3-DevKitC-1"
  }
}
```

### Example 2: Check available pins and configure SPI
**User Prompt**: *"Which pins can I use for SPI clock on this board? Set GPIO6 as SPI clock and GPIO4 as MOSI."*
**AI Action**:
Calls:
```json
{
  "name": "get_available_pins",
  "arguments": {
    "peripheral": "SPI2",
    "signal": "SCK"
  }
}
```
Then configures pins:
```json
{
  "name": "configure_pin",
  "arguments": {
    "pin": "GPIO6",
    "config": {
      "mode": "AlternateFunction",
      "alternateFunction": "SPI2_SCK",
      "label": "DISPLAY_SCK"
    }
  }
}
```

### Example 3: Configure communication peripheral
**User Prompt**: *"Configure USART1 at 115200 baud with PA9 as TX and PA10 as RX."*
**AI Action**:
Calls:
```json
{
  "name": "configure_peripheral",
  "arguments": {
    "name": "USART1",
    "config": {
      "enabled": true,
      "pins": { "TX": "PA9", "RX": "PA10" },
      "parameters": { "baudRate": 115200, "dataBits": 8, "parity": "None", "stopBits": 1 }
    }
  }
}
```

### Example 4: Pin conflict detection
**User Prompt**: *"Set PA5 as a Status LED output."* (PA5 is already assigned to SPI1_SCK)
**AI Action**:
Calls `configure_pin`. Engine returns:
```json
{
  "success": false,
  "conflict": true,
  "error": "Pin PA5 is already assigned to: SPI1_SCK. Do you want to replace the existing assignment?",
  "currentOwner": "SPI1_SCK",
  "proposedOwner": "GPIO_Output"
}
```
AI politely reports to the user:
*"PA5 is currently allocated to SPI1_SCK. Would you like me to reassign PA5 and choose an alternate pin for SPI1 clock, or reassign it with force?"*
