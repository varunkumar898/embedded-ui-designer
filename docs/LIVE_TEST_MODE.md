# Live Test Mode

## Overview

Live Test Mode is a toggleable editor state that allows interactive testing of GPIO-bound components (currently Switch components) against a connected physical board via OpenOCD. When enabled, the design runs on the PC (CanvasView simulation) and can drive real GPIO pins on the target hardware.

## Activation

- **Toolbar button**: "● Live Test" (checkable `QToolButton`, objectName `liveTestButton`)
- **Shortcut**: `Ctrl+L`
- **Default state**: OFF

## Behavior When ON

1. **Button appearance**: Background colored with `Theme::LiveTestActive`, bold text
2. **Status bar**: Shows "✓ Live Test: ON (Click switches to control GPIO)"
3. **Resource Analyzer panel**: Stays visible (dock shown + raised). A banner label (objectName `liveTestBannerLabel`) appears at the top of the panel with text:
   > "Live Test: the design runs on your PC. Figures show what the exported firmware would need on `<device>`."
   The device name comes from the currently selected target in the Resource Analyzer (defaults to STM32H745 if none selected).
4. **CanvasView**: Enters live test mode (`CanvasView::setLiveTestMode(true)`). Mouse clicks on Switch components trigger GPIO toggles.
5. **Signal emitted**: `liveTestModeChanged(true)` from MainWindow.

## Behavior When OFF

1. **Button appearance**: Default style (no custom stylesheet)
2. **Status bar**: Shows "Design Mode (Resource Analyzer enabled)"
3. **Resource Analyzer panel**: Remains visible. Banner hidden.
4. **CanvasView**: Exits live test mode (`CanvasView::setLiveTestMode(false)`). Switch clicks no longer drive GPIO.
5. **Signal emitted**: `liveTestModeChanged(false)` from MainWindow.

## GPIO Interaction Path

1. User clicks a Switch component on the canvas while Live Test is ON.
2. `CanvasView::mousePressEvent` routes the click to `onComponentClicked()`, which calls `handleSwitchToggle(component)` for Switch components.
3. `handleSwitchToggle` reads the Switch's `gpio_pin` property (string, e.g., "PA4" or "GPIO18").
4. BoardHAL parses the pin string to (port, pin).
5. `OpenOCDManager::gpioToggle(port, pin)` is called to drive the physical pin, followed by `gpioRead(port, pin)` to read the state back.
6. Latency is measured (high-resolution clock, milliseconds) and emitted via `CanvasView::gpioStateChanged(pin, newState, latencyMs)`.

## OpenOCD Integration

- `MainWindow` owns an `OpenOCDManager` instance (`m_openocd`).
- On startup, `MainWindow` passes `&m_openocd` to `CanvasView::setOpenOCDManager()`.
- Connection to the board is managed separately via the MCU Selection panel (Connect to Board button, OpenOCD config browse).
- If OpenOCD is not connected when a Switch is toggled, no hardware write happens: `sendCommand` logs "Not connected to OpenOCD", and `OpenOCDManager` keeps a simulated pin state (`simulatedStates`) which `gpioRead` returns for the read-back.

## Resource Analyzer in Live Test

- The Resource Analyzer dock (`resourceDock`) is **never hidden** by Live Test toggle (supersedes earlier behavior where it was hidden).
- While Live Test is ON, the panel refreshes its analysis (`ResourceAnalyzerPanel::refresh()`) so flash/RAM figures stay current.
- The banner uses `Theme::InfoBg` / `Theme::Info` styling and is pinned at the top of the panel's layout.

## What Is NOT Hardware-Verified

- Only Switch components with a valid `gpio_pin` property drive hardware.
- Other component types (Button, Label, Image, etc.) do not interact with GPIO.
- No firmware is flashed; the design runs purely in the PC-side CanvasView simulation.
- No input components read GPIO; `gpioRead` is used only to confirm the state after a `gpioToggle`.
- Latency measurement is PC-side only (time to call OpenOCD write + read-back), not round-trip.
- Multi-board, multi-core, or RTOS contexts are not handled.

## Code References

- `MainWindow::toggleLiveTestMode(bool)` — src/MainWindow.cpp:2280
- `MainWindow::createGpioControls()` — src/MainWindow.cpp:2234 (toolbar GPIO widget)
- `CanvasView::setLiveTestMode(bool)` — src/canvas/CanvasView.cpp:959
- `CanvasView::handleSwitchToggle(UIComponent*)` — src/canvas/CanvasView.cpp:969
- `CanvasView::setOpenOCDManager(OpenOCDManager*)` — src/canvas/CanvasView.cpp:964
- `ResourceAnalyzerPanel::setLiveTestMode(bool)` — src/panels/ResourceAnalyzerPanel.cpp:212
- `ResourceAnalyzerPanel::liveTestBannerText()` — src/panels/ResourceAnalyzerPanel.cpp:221
- `ResourceAnalyzerPanel::updateLiveTestBanner()` — src/panels/ResourceAnalyzerPanel.cpp:229