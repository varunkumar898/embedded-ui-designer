# Desktop Simulator Architecture & User Guide

## 1. Overview & Architectural Goals

The **Desktop Simulator** is the interactive runtime execution and verification environment for Embedded UI Designer. It allows embedded developers, firmware engineers, and UI designers to test multi-screen UI projects, live DataBindings, state transition machines, and virtual hardware interactions directly on desktop operating systems (Linux, macOS, Windows) without requiring target microcontrollers or physical debug probes (ST-Link, J-Link, OpenOCD).

The simulator directly executes the authoritative `Project` model without converting or duplicating project schemas:

```
                         Embedded UI Designer
                                  │
                               Project
                                  │
          ┌───────────────────────┼───────────────────────┐
          │                       │                       │
       Designer               Simulator             Code Generator
          │                       │                       │
     CanvasScene            SimulationRuntime        GeneratorIR
                                  │                       │
                           SimulationBackend       LVGL / QUL / µGFX
                                  │
                           DataSource / Binding
                                  │
                           Component Runtime
```

---

## 2. Simulation Architecture & Core Subsystems

### 2.1 SimulationRuntime (`src/simulator/SimulationRuntime.h`, `.cpp`)
- **Execution Lifecycle**: Manages discrete simulation states (`Stopped`, `Running`, `Paused`), tick steps (`step(ms)`), and restart.
- **Clock Speed Scaling**: Configurable time multiplier (`0.25x`, `0.5x`, `1.0x Realtime`, `2.0x`, `5.0x`, `10.0x`).
- **Timer DataSources**: Automatically advances periodic counter sources based on simulated elapsed time.
- **Calculated DataSources & Safe Expression Parser**: Evaluates mathematical expressions (e.g. `src_speed / 240.0 * 100.0`) using a recursive-descent parser with division-by-zero protection.
- **Project Save Safety (Snapshot Engine)**: Captures a complete JSON snapshot of all component properties, DataSources, and active screen before execution starts. When simulation is stopped or the window is closed, all original design values are restored, ensuring runtime mutations never taint `.euiproj` project files.

### 2.2 SimulationBackend (`src/simulator/SimulationBackend.h`, `.cpp`)
- **Virtual HAL**: Implements `Hardware::HardwareBackend` under the ID `"simulation"`.
- **Digital GPIO**: Virtual pin state dictionary with atomic high/low toggling and readback.
- **Analog ADC**: Simulated 12-bit ADC channels supporting raw counts (`0-4095`) and normalized floats (`0.0-1.0`).
- **PWM Output**: Virtual duty cycle registry (`0.0% - 100.0%`).
- **Serial / CAN Bus**: Virtual UART circular RX/TX queues and CAN frame injection/history.
- **Hardware Fault Injection**: User-controllable toggles for simulated hardware timeouts, communication faults, and ADC noise.
- **Hardware Isolation Boundary**: Complete physical hardware lockout. No physical USB endpoints, OpenOCD processes, or serial ports are touched in simulation mode.

### 2.3 SimulationCanvasView (`src/simulator/SimulationCanvasView.h`, `.cpp`)
- Dedicated non-editing `QGraphicsView` displaying the active screen without design-time adornments (no selection borders, resize handles, or alignment guides).
- Handles interactive desktop user input:
  - **Button**: Pressed/released state transition, "On Click" interaction.
  - **Switch / Checkbox**: Toggles boolean state, pushes value to bound DataSources via Write bindings.
  - **Slider**: Dragging adjusts normalized/raw value and bound DataSources.
  - **TabView**: Click tab headers to switch active tabs.
  - **NavigationBar**: Segment selection transitions active screens.
  - **List & Table**: Row selection updates `selectedIndex` and `selectedRow`.
- Viewport scaling: 25% to 400% zoom and "Fit Viewport".

### 2.4 SimulationControlPanel (`src/simulator/SimulationControlPanel.h`, `.cpp`)
- **Data Sources Tab**: Live inspectors and manual override widgets (spinboxes, sliders, toggles) for every project DataSource.
- **Virtual HAL Tab**: Inspects virtual hardware states, resets virtual registers, and toggles hardware error injection.
- **Data Monitor Tab**: Real-time telemetry log table with timestamps, source IDs, previous values, and new values.
- **Automotive / Dashboard Presets**: Instant configuration of typical vehicle conditions:
  - `Normal Cruise`: 80 km/h, 2800 RPM, 85°C, 90% Battery, 3.2 bar.
  - `Engine Redline`: 215 km/h, 7500 RPM.
  - `Low Battery Warning`: 35 km/h, 1200 RPM, 12% Battery.
  - `High Temperature Alert`: 105 km/h, 3800 RPM, 118°C.
  - `Cold Start`: 0 km/h, 1100 RPM, 15°C, 99% Battery.

---

## 3. DataBinding & State Engine Runtime

The runtime continuously coordinates bidirectional data bindings across components and DataSources:

1. **Read Bindings**:
   - Simulated DataSource updates (via preset, timer, or manual slider).
   - `SimulationRuntime::propagateDataSourceBindings()` updates target component value.
   - For `ValueVisualizationComponent` instances (Speedometer, RPM, Temperature, Gauge, Battery, Pressure, ProgressBar), `evaluateThresholdState()` automatically transitions component states between `normal`, `warning`, and `critical` based on configured thresholds.
2. **Write Bindings**:
   - User interacts with an input component (e.g. toggles Switch or drags Slider).
   - `SimulationCanvasView` calls `SimulationRuntime::notifyComponentPropertyChanged()`.
   - Runtime updates the bound DataSource.
   - If the DataSource maps to a virtual hardware pin (`hardwareRef`), `SimulationBackend` updates virtual GPIO or ADC registers.
3. **ReadWrite Bindings**:
   - Maintains two-way synchronization between component and DataSource with internal recursion guards.

---

## 4. Multi-Screen Navigation & Interaction Rules

- **NavigationBar**: Items configured with a `targetScreenId` trigger screen switches when clicked.
- **Prototype Interactions**: Components with configured prototype interactions (`Trigger`: On Click / On Change, `Action`: Show / Hide / Toggle / Set Value) execute against the target component or screen without modifying design-time state.
- **Toolbar Screen Selector**: A dropdown in `SimulatorWindow` allows direct switching between any project screen at any time.

---

## 5. Physical Hardware vs. Simulation Mode

| Aspect | Simulation Mode | Real Hardware Mode |
| :--- | :--- | :--- |
| **Backend** | `SimulationBackend` (`"simulation"`) | `HardwareBridge` / `STM32Backend` / OpenOCD |
| **Pins & BSRR** | Memory dictionary in host RAM | Physical MCU register writes (`0x48000018`) |
| **ADC Readings** | User-controlled slider or mock injector | Real 12-bit ADC SAR conversion (`PA0`, etc.) |
| **Safety** | Isolated — zero risk to MCU pins or loads | Drives physical voltages (3.3V / 5V / PWM) |
| **Debug Probes** | No probe or USB connection needed | Requires ST-Link / J-Link via USB |

> [!IMPORTANT]
> Simulation results prove project model consistency, binding validity, and UI aesthetics. They do **not** substitute for physical hardware testing or electrical verification.

---

## 6. Supported Component Coverage

Every component type in the Embedded UI Designer repository is supported in the Desktop Simulator:

- **Basic**: `RectangleComponent`, `CircleComponent`, `PathComponent`, `ButtonComponent`, `SwitchComponent`, `CheckboxComponent`, `LabelComponent`, `TextInputComponent`, `ImageComponent`.
- **Progress**: `ProgressBarComponent`, `CircularProgressComponent`.
- **Dashboard**: `GaugeComponent`, `SpeedometerComponent`, `RpmComponent`, `BatteryComponent`, `PressureComponent`, `TemperatureComponent`.
- **Navigation**: `TabViewComponent`, `NavigationBarComponent`.
- **Data**: `ListComponent`, `TableComponent`.

---

## 7. Known Limitations

1. **Host Display vs. Embedded Framebuffer**: The simulator uses Qt’s desktop rendering pipeline (QPainter / QGraphicsView), which produces crisp desktop previews but does not simulate color depth quantization (e.g. RGB565 banding) or physical display refresh rates (e.g. SPI display bandwidth limits).
2. **Interrupt Latency & RTOS Scheduling**: Simulation assumes instant non-blocking updates and does not model FreeRTOS task starvation or interrupt preemption.
