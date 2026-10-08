# Code Generator v2 Architecture & Guide

Embedded UI Designer v2 introduces a target-aware, architecture-driven code generation subsystem (`CodeGen::*`). It cleanly decouples visual UI components, target hardware architectures (STM32, ESP32, Raspberry Pi, Generic), data bindings, and runtime UI frameworks (LVGL, Qt for MCUs / QUL, µGFX).

---

## 1. Architectural Overview

```
                   ┌────────────────────────────────────────┐
                   │                Project                 │
                   │  ├── Screens                           │
                   │  ├── Components (Phase 2 Expanded)     │
                   │  ├── DataSources                       │
                   │  ├── DataBindings                      │
                   │  ├── ComponentStates                   │
                   │  └── HardwareTarget (Phase 3 HAL)      │
                   └───────────────────┬────────────────────┘
                                       │
                                       ▼
                   ┌────────────────────────────────────────┐
                   │      CodeGen::GeneratorContext         │
                   │  - Read-only Project Model Access      │
                   │  - Comprehensive Project Validation    │
                   │  - Warnings vs Errors Engine           │
                   └───────────────────┬────────────────────┘
                                       │
                                       ▼
                   ┌────────────────────────────────────────┐
                   │         CodeGen::GeneratorIR           │
                   │  - Target-Agnostic IR Compilation      │
                   │  - IRProject, IRScreen, IRWidget       │
                   │  - IRDataSource, IRBinding             │
                   └───────┬───────────┬───────────┬────────┘
                           │           │           │
            ┌──────────────┘           │           └──────────────┐
            ▼                          ▼                          ▼
┌──────────────────────┐   ┌──────────────────────┐   ┌──────────────────────┐
│  TargetHalGenerator  │   │BindingLayerGenerator │   │  Framework Backend   │
│  - target_hal.h/.c   │   │  - ui_bindings.h/.c  │   │  - LvglGenerator     │
│  - STM32 (BSRR/ADC)  │   │  - Data Update Loop  │   │  - QtMcuGenerator    │
│  - ESP32 (ESP-IDF)   │   │  - Read/Write/RW     │   │  - UgfxGenerator     │
│  - Linux / Generic   │   │  - Direct HW Bridge  │   │  - Future Generator  │
└──────────────────────┘   └──────────────────────┘   └──────────────────────┘
            │                          │                          │
            └──────────────────┬──────────────────────────────────┘
                               ▼
            ┌──────────────────────────────────────┐
            │   Complete Target-Ready Project      │
            │   - Clean User Code Boundary         │
            │   - Standalone CMakeLists.txt        │
            │   - Deterministic Compilation        │
            └──────────────────────────────────────┘
```

---

## 2. Core Modules

### 2.1 `CodeGen::GeneratorContext` (`src/codegen/GeneratorContext.h`)
- **Immutability**: Read-only wrapper around `Project`, `Screen`, `UIComponent`, `DataSource`, `DataBinding`, and `HardwareTarget`.
- **Pre-Flight Validation**:
  - Duplicate Component & Screen IDs detection.
  - Missing `DataSource` references in `DataBinding`.
  - Unmapped hardware pins or unsupported peripherals.
  - Returns `bool isValid` and populates detailed `QList<ValidationMessage>` (`Info`, `Warning`, `Error`).

### 2.2 `CodeGen::GeneratorIR` (`src/codegen/GeneratorIR.h`)
- Compiles rich desktop Qt graphics models into target-agnostic intermediate structures:
  - `IRProject`: Target family, board, color depth, resolution, display geometry (rectangle/round).
  - `IRScreen`: Multi-screen hierarchy, screen background color, dimensions.
  - `IRWidget`: Position, dimensions, Z-order, JSON serialized properties, state styles, bindings, interactions.
  - `IRDataSource` & `IRBinding`: Direction (`Read`, `Write`, `ReadWrite`), hardware references (`PA0`, `GPIO4`), transform expressions.

### 2.3 `CodeGen::TargetHalGenerator` (`src/codegen/TargetHalGenerator.h`)
- Emits target-specific Hardware Abstraction Layer adapters without hardcoding MCU registers into UI components:
  - **STM32**: Generates atomic `GPIOx->BSRR` fast writes, `ADC1->DR` conversion routines, and SysTick millisecond timing.
  - **ESP32**: Generates ESP-IDF `gpio_set_level()`, `gpio_get_level()`, and `adc1_get_raw()` drivers.
  - **Raspberry Pi / Linux**: Generates standard sysfs / libgpiod wrappers.
  - **Generic / PC Simulator**: Emits high-precision clock simulation for native PC preview and automated CI validation.

### 2.4 `CodeGen::BindingLayerGenerator` (`src/codegen/BindingLayerGenerator.h`)
- Emits `ui_bindings.h` and `ui_bindings.c` connecting hardware peripheral signals to LVGL/QUL/µGFX widgets:
  - **Read Direction**: Periodic polling reads `hal_read_adc_normalized()` and invokes widget setters (e.g., `lv_bar_set_value()`, `lv_arc_set_value()`).
  - **Write Direction**: Event callbacks forward user interactions (switches, buttons, sliders) to `hal_write_digital_pin()` or PWM outputs.
  - **ReadWrite Direction**: Bi-directional synchronization with state caching.

---

## 3. Supported Framework Backends

| Framework | Target Language | Multi-Screen | Data Binding | HAL Drivers | Supported Dashboard Widgets |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **LVGL v8/v9** | C / C++ | Full | Native C | Native C | Speedometer, RPM, Gauge, Battery, Temp, Pressure, CircularProgress, TabView, NavigationBar, List, Table |
| **Qt for MCUs (QUL)** | QML / C++ | Full | QML Binding | C++ Backend | Gauges, Progress, Buttons, Sliders, Switches, Labels, Shapes |
| **µGFX** | C | Full | C Callback | GDISP / GADC | Gauges, Progress, Buttons, Sliders, Labels, Shapes |

---

## 4. User Code Boundary & Project Layout

When exporting to LVGL or embedded targets, the output structure clearly isolates generated UI and HAL from user logic:

```
exported_project/
├── CMakeLists.txt             # Top-level build file
├── lv_conf.h                  # LVGL configuration (optimized for target RAM)
├── main.c                     # Target entry point and main polling loop
├── target_hal.h               # Target-specific HAL interface
├── target_hal.c               # Target-specific HAL implementation (STM32/ESP32/RPI)
├── ui.h                       # UI object declarations and screen handles
├── ui.c                       # Multi-screen widget hierarchy and styling
├── ui_bindings.h              # Data binding dispatcher interface
├── ui_bindings.c              # Binding event callbacks and peripheral sampling
└── pc_simulator/              # PC simulation testbench for desktop verification
    └── CMakeLists.txt
```

---

## 5. Verification & Testing

Phase 4 code generation is continuously validated by:
- **Unit Test Suite** (`tests/test_all_cases.cpp`): Tests validation diagnostics, IR compilation, HAL generation for STM32/ESP32, and LVGL multi-screen dashboard export.
- **Functional Validation Runner** (`tests/test_functional_runner.cpp`): Section 17 executes end-to-end multi-screen project compilation, verifying file creation, deterministic code output, and HAL integration with zero compiler errors.
