# Embedded UI Designer – User Guide

Welcome to **Embedded UI Designer**! This guide walks you through designing embedded user interfaces visually and generating ready-to-compile code for microcontrollers using **µGFX** and **Qt Quick Ultralite (Qt for MCUs)**.

---

## 1. Creating a New Project

1. Click **File → New Project** (or press `Ctrl+N`).
2. Select your target display resolution from the toolbar dropdown:
   - `320 × 240 (QVGA)`: Standard for STM32 Discovery and ESP32 LCD modules.
   - `480 × 320 (HVGA)`: Common for 3.5" touch panels.
   - `800 × 480 (WVGA)`: 7" industrial displays and automotive clusters.
   - `128 × 64`: Monochromatic I2C/SPI OLED displays.

---

## 2. Designing the Interface

1. **Add Components**:
   - Find the component you want in the **Components** palette on the left (`Button`, `Text / Label`, `Rectangle`, `Progress Bar`).
   - Drag it onto the canvas, or double-click to insert it in the center.
2. **Move and Snap**:
   - Drag elements across the display surface.
   - Grid snapping aligns items to clean 10px intervals. Toggle snap on/off in the **View** menu.
3. **Resize**:
   - Click an element to select it. Interactive resize handles will appear on corners and edges.
   - Drag any handle to resize.

---

## 3. Customizing Properties

With any element selected, the **Properties Panel** on the right allows live adjustment of:
- **Identifier (`id`)**: The variable name used in code generation (e.g. `btn_start`, `label_temp`).
- **Transform**: Coordinates (`X`, `Y`) and Dimensions (`W`, `H`).
- **Styling**: Background colors, text colors, font sizes, corner radius, borders.
- **Signals**: Set the `OnClicked` stub name (e.g. `toggleFan`).

---

## 4. Exporting Code

### Option A: µGFX C Project (Recommended to start)
- **Menu**: `File → Export µGFX C Project...` (or `Ctrl+E`).
- Generates:
  - `main.c` (event loop with `GListener`)
  - `gfxconf.h` (enables exact flags needed by widgets)
  - `ui.c` and `ui.h` (`GWidgetInit` push-button pattern)
  - `CMakeLists.txt` (compiles via `src/gfx_mk.c`)
- **Compilation on PC**:
  ```bash
  cd exported_ugfx_folder
  mkdir build && cd build
  cmake .. -DUGFX_PATH=/path/to/ugfx
  cmake --build .
  ```

### Option B: Qt for MCUs (QUL) Project
- **Menu**: `File → Export Qt for MCUs (QUL) Project...` (or `Ctrl+Shift+E`).
- Generates:
  - `CMakeLists.txt` (using `find_package(Qul)`, `qul_add_target`, `app_target_setup_os`)
  - `<Target>.qmlproject`
  - `MainScreen.qml` (using strictly QUL supported types: `Rectangle`, `Text`, `MouseArea`)
- **Compilation**: Requires Qt Quick Ultralite SDK, ARM GCC toolchain, and `-DQUL_PLATFORM=<board>`.

---

## 5. Automated Headless Export (CLI Mode)

You can run headless exports without launching the graphical UI:

```bash
./EmbeddedUIDesigner --export ugfx --project examples/simple.euiproj --out /path/to/output
./EmbeddedUIDesigner --export qul --project examples/simple.euiproj --out /path/to/output
```
This is ideal for continuous integration and automated compile tests.
