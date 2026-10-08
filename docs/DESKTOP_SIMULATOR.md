# Phase 5: Desktop Simulator — Architecture & Specification

## 1. Overview & Architectural Goals

The **Desktop Simulator** enables interactive execution and testing of embedded UI projects directly on host desktop workstations (Linux, macOS, Windows) without physical target microcontrollers (MCU) or debug probes (ST-Link, J-Link, OpenOCD).

The simulator directly executes the authoritative [Project](file:///home/cherry/embedded-ui-designer/src/project/Project.h) model. It does **not** create a separate project format or duplicate core data structures.

```
                    Project
                       │
        ┌──────────────┼──────────────┐
        │              │              │
     Screens      DataSources    DataBindings
        │              │              │
        └──────────────┼──────────────┘
                       │
                 Simulation Runtime
                       │
          ┌────────────┼────────────┐
          │            │            │
      Components   Simulated HAL  State Engine
          │            │            │
          └────────────┼────────────┘
                       │
             Simulation Canvas View
                       │
               Desktop User Input
```

---

## 2. Core Subsystems & Components

### 2.1 Simulation Runtime (`SimulationRuntime`)
- **Location**: [src/simulator/SimulationRuntime.h](file:///home/cherry/embedded-ui-designer/src/simulator/SimulationRuntime.h), [src/simulator/SimulationRuntime.cpp](file:///home/cherry/embedded-ui-designer/src/simulator/SimulationRuntime.cpp)
- **Roles**:
  - Manages execution lifecycle: `start()`, `pause()`, `resume()`, `stop()`, `restart()`, `step(stepMs)`.
  - Configurable simulation clock with variable speed scaling (`0.25x`, `0.5x`, `1.0x`, `2.0x`, `5.0x`, `10.0x`).
  - Safe expression evaluator for `Calculated` DataSources (e.g. `speed_src / 240.0 * 100.0`) with recursive-descent tokenized parser and division-by-zero protection.
  - Automatic timer increment for `Timer` DataSources.
  - **Project Save Safety**: Captures a full design snapshot before running and restores all design values on stop/window close, ensuring runtime mutations are never saved to `.euiproj` unintentionally.

### 2.2 Simulated Hardware HAL (`SimulationBackend`)
- **Location**: [src/simulator/SimulationBackend.h](file:///home/cherry/embedded-ui-designer/src/simulator/SimulationBackend.h), [src/simulator/SimulationBackend.cpp](file:///home/cherry/embedded-ui-designer/src/simulator/SimulationBackend.cpp)
- **Roles**:
  - Implements [HardwareBackend](file:///home/cherry/embedded-ui-designer/src/hardware/HardwareBackend.h) under ID `"simulation"`.
  - Virtual digital GPIO input/output registers with instant pin toggling.
  - Virtual ADC analog registers supporting raw 12-bit counts (`0-4095`) and normalized floats (`0.0-1.0`).
  - Virtual PWM duty cycle generator (`0.0 - 100.0%`).
  - Virtual UART circular transmit/receive buffers with packet injection.
  - Virtual I2C peripheral scan (`0x48`, `0x68`, `0x76`) and register memory map.
  - Virtual SPI loopback and virtual CAN frame injection / inspection.
  - **Hardware Safety Boundary**: Guaranteed complete isolation. When in Simulation mode, no physical serial ports, OpenOCD sessions, or ST-Link USB endpoints are accessed.

### 2.3 Interactive Simulation Canvas (`SimulationCanvasView`)
- **Location**: [src/simulator/SimulationCanvasView.h](file:///home/cherry/embedded-ui-designer/src/simulator/SimulationCanvasView.h), [src/simulator/SimulationCanvasView.cpp](file:///home/cherry/embedded-ui-designer/src/simulator/SimulationCanvasView.cpp)
- **Roles**:
  - Renders the exact UI components created in the designer without selection borders or resize handles.
  - Translates desktop mouse and touch interactions into component interactions:
    - **Buttons**: Press, release, click triggers.
    - **Switches / Checkboxes**: State toggling, triggering write bindings to virtual GPIO.
    - **Sliders**: Dragging updates value and bound analog sensors.
    - **TabViews**: Tab switching updates visible sub-containers.
    - **Navigation Bars**: Screen navigation switches project screens live.
    - **Lists / Tables**: Interactive row and cell selection.
  - Aspect-ratio preserving viewport zooming (`100%`, `150%`, `200%`, `Fit Viewport`).

### 2.4 Simulation Inspector & Control Panel (`SimulationControlPanel`)
- **Location**: [src/simulator/SimulationControlPanel.h](file:///home/cherry/embedded-ui-designer/src/simulator/SimulationControlPanel.h), [src/simulator/SimulationControlPanel.cpp](file:///home/cherry/embedded-ui-designer/src/simulator/SimulationControlPanel.cpp)
- **Roles**:
  - **Data Sources Tab**: Live type-specific value editors:
    - Boolean / GPIO: Interactive toggles with green active indicator LEDs.
    - Analog / Sensors / PWM: Synchronized numeric spinboxes and range sliders.
    - Strings / Enums: Direct text inputs and dropdowns.
  - **Virtual HAL Tab**: Inspect simulated digital pins, analog ADC channels, PWM outputs, UART queues, I2C scan, and test hardware error injection.
  - **Data Monitor Tab**: Real-time chronological telemetry log with source ID, previous value, new value, and millisecond timestamps.
  - **Presets**: Instant application of automotive conditions:
    - `Normal Operation` (80 km/h, 2800 RPM, 85°C, 90% Batt, 3.2 bar)
    - `Low Battery Warning` (35 km/h, 1200 RPM, 12% Batt)
    - `High Temperature Alert` (105 km/h, 3800 RPM, 118°C)
    - `Engine Redline` (215 km/h, 7500 RPM)
    - `Pressure Warning` (0.7 bar)

### 2.5 Standalone Simulator Window (`SimulatorWindow`)
- **Location**: [src/simulator/SimulatorWindow.h](file:///home/cherry/embedded-ui-designer/src/simulator/SimulatorWindow.h), [src/simulator/SimulatorWindow.cpp](file:///home/cherry/embedded-ui-designer/src/simulator/SimulatorWindow.cpp)
- **Roles**:
  - Standalone top-level tool window accessible via `Simulation -> Run Desktop Simulator...` (`Ctrl+R`) or toolbar action.
  - Toolbar controls: Run, Pause, Stop, Step, Restart, Screen Selector, Speed Multiplier, Zoom, and Simulation Mode Safety Badge.

---

## 3. Data Binding & Auto-State Pipeline

```
┌──────────────────────────────────────────────────────────┐
│                   Data Binding Runtime                   │
└──────────────────────────────────────────────────────────┘
  [Read Binding Flow]
    Simulated DataSource (e.g. speed_src = 160)
         │
         ▼
    SimulationRuntime::propagateDataSourceBindings
         │
         ▼
    SpeedometerComponent::setValue(160)
         │
         ▼
    ValueVisualizationComponent::evaluateThresholdState
         │  (160 >= warningThreshold 140)
         ▼
    ComponentState: "warning"  -->  Amber Warning Needle & Glow

  [Write Binding Flow]
    User clicks SwitchComponent ("sw_light" -> true)
         │
         ▼
    SimulationRuntime::notifyComponentPropertyChanged
         │
         ▼
    DataSource ("gpio_light" -> true)
         │
         ▼
    SimulationBackend::writeDigital("PA4", true)
         │
         ▼
    Virtual GPIO PA4 Pin = HIGH (Recursion guard prevents cyclic feedback)
```

---

## 4. Verification & Testing

The Phase 5 test suite is integrated into the automated test harnesses:
- **Unit Test Suite**: [tests/test_all_cases.cpp](file:///home/cherry/embedded-ui-designer/tests/test_all_cases.cpp) (`testPhase5*` slots).
  - `testPhase5SimulationBackendAndHardwareIsolation`
  - `testPhase5SimulationRuntimeClockAndExpressions`
  - `testPhase5BidirectionalDataBindingsAndAutoState`
  - `testPhase5ProjectSaveSafetyAndSnapshotRestore`
  - `testPhase5SimulatorWindowAndControlPanelUi`
- **Functional Validation Runner**: [tests/test_functional_runner.cpp](file:///home/cherry/embedded-ui-designer/tests/test_functional_runner.cpp) (Section 18).
  - 80/80 Unit Tests Passing (100%).
  - 79/79 Functional Validation Checks Passing (100%).
