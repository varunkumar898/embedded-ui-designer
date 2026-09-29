# Embedded UI Designer – Developer Guide

This guide describes the core architecture, the CLI headless pipeline, and how to extend both component types and target code generators (**µGFX** and **Qt Quick Ultralite / QUL**).

---

## Architecture Overview

```
                      ┌────────────────────────┐
                      │   main.cpp / CLI Mode  │
                      └───────────┬────────────┘
         ┌────────────────────────┼────────────────────────┐
         ▼                        ▼                        ▼
 ┌───────────────┐        ┌───────────────┐        ┌───────────────┐
 │ ComponentPal. │        │  CanvasView   │        │PropertiesPan. │
 └───────┬───────┘        └───────┬───────┘        └───────┬───────┘
         │ (Drag & Drop)          │                        │
         └───────────────► ┌──────┴───────┐ ◄──────────────┘
                           │ CanvasScene  │ (Bi-directional sync)
                           └──────┬───────┘
                                  │
         ┌────────────────────────┼────────────────────────┐
         ▼                        ▼                        ▼
 ┌───────────────┐        ┌───────────────┐        ┌───────────────┐
 │  QUndoStack   │        │    Project    │        │ CodeGenerator │
 │ (Commands)    │        │  (.euiproj)   │        │ (µGFX & QUL)  │
 └───────────────┘        └───────┬───────┘        └───────┬───────┘
                                  │                        │
                          ┌───────┴───────┐        ┌───────┴───────┐
                          │ HardwareBridge│        │ AssetPipeline │
                          │(Device/Flash) │        │ (RGB565/Mono) │
                          └───────────────┘        └───────────────┘
```

---

## 1. CLI Headless Mode & Automated Testing

The designer executable supports a headless export mode designed for CI/CD and automated test validation:

```bash
./build/EmbeddedUIDesigner --export <ugfx|qul> --project <path/to/project.euiproj> --out <path/to/output_dir>
```

When `--export` is present, the app initializes headlessly (`QT_QPA_PLATFORM=offscreen`), loads the canvas state from the JSON project, generates all target source and build files, and exits with status `0` on success (or `1` on error).

This enables automated "does it compile" verification:
```bash
# 1. Export µGFX project
./build/EmbeddedUIDesigner --export ugfx --project examples/simple.euiproj --out /tmp/ugfx_out

# 2. Compile exported code against µGFX library
cmake -S /tmp/ugfx_out -B /tmp/ugfx_out/build -DUGFX_PATH=/path/to/ugfx
cmake --build /tmp/ugfx_out/build
```

---

## 2. µGFX Code Generation Architecture

µGFX projects do not require a Qt license and can be compiled on desktop (Windows Win32, Linux X11/framebuffer) or flashed directly to microcontrollers.

### Compilation Architecture (`src/gfx_mk.c`)
The generator uses µGFX's single-file compilation approach:
- Sources compiled: `main.c`, `ui.c`, and `${UGFX_DIR}/src/gfx_mk.c`.
- Include directories: Current directory (where `gfxconf.h` and `ui.h` reside) and `${UGFX_DIR}`.

### `gfxconf.h` Requirements
1. **Operating System (GOS)**: Exactly ONE OS option must be defined:
   ```c
   #if defined(_WIN32) || defined(__WIN32__)
       #define GFX_USE_OS_WIN32    GFXON
   #elif defined(__linux__) || defined(__unix__)
       #define GFX_USE_OS_LINUX    GFXON
   #else
       #define GFX_USE_OS_RAW32    GFXON
   #endif
   ```
2. **Subsystem and Widget Feature Flags**:
   Every widget type used by the design must have its respective flag enabled:
   - `GFX_USE_GDISP GFXON`, `GDISP_NEED_TEXT GFXON`, `GDISP_NEED_CLIP GFXON`
   - `GFX_USE_GWIN GFXON`, `GWIN_NEED_WINDOWMANAGER GFXON`, `GWIN_NEED_WIDGET GFXON`
   - `GWIN_NEED_BUTTON GFXON` (when buttons are in design)
   - `GWIN_NEED_LABEL GFXON` (when text labels are in design)
   - `GWIN_NEED_PROGRESSBAR GFXON` (when progress bars are in design)
   - `GFX_USE_GEVENT GFXON`, `GFX_USE_GINPUT GFXON`, `GINPUT_NEED_MOUSE GFXON`

### Widget Initialization Pattern
Widget code in `ui.c` adheres strictly to the µGFX PushButton pattern:
```c
GWidgetInit wi;
gwinWidgetClearInit(&wi);
wi.g.show = gTrue;
wi.g.x = 20; wi.g.y = 80;
wi.g.width = 140; wi.g.height = 40;
wi.text = "Click Me";
wi.customDraw = NULL;
ghBtn_1 = gwinButtonCreate(NULL, &wi);
```

### Event Handling Loop (`main.c`)
```c
gfxInit();
ui_init();

GListener gl;
geventListenerInit(&gl);
gwinAttachListener(&gl);

while (gTrue) {
    GEvent *pe = geventEventWait(&gl, 100);
    if (pe && pe->type == GEVENT_GWIN_BUTTON) {
        GEventGWinButton *peb = (GEventGWinButton *)pe;
        // Dispatch button click
    }
}
```

---

## 3. Qt for MCUs (Qt Quick Ultralite / QUL) Architecture

> [!IMPORTANT]
> **Qt for MCUs is NOT standard desktop Qt 6.** It does not use `QApplication`, `QMainWindow`, or `QtQuick.Controls`. It is Qt Quick Ultralite (QUL), which requires a licensed QUL SDK and targets select hardware platforms (STM32, NXP, Renesas, Infineon, ESP32).

### Target CMake Structure
```cmake
cmake_minimum_required(VERSION 3.21)
project(MyUI LANGUAGES C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qul REQUIRED)
qul_add_target(MyUI QML_PROJECT MyUI.qmlproject)
app_target_setup_os(MyUI)
```

### Supported QML Types
QUL supports a constrained subset of declarative elements:
- `Rectangle`
- `Text`
- `Image`
- `MouseArea`
- `Item`

Interactive elements like Buttons and Progress Bars are constructed using these fundamental primitives:
```qml
// Button implementation in Qt Quick Ultralite:
Rectangle {
    id: btn
    x: 20; y: 80; width: 140; height: 40
    color: btn_mouse.pressed ? "#1976d2" : "#2196f3"
    radius: 4

    Text {
        anchors.centerIn: parent
        text: "Press Me"
        color: "#ffffff"
        font.bold: true
    }

    MouseArea {
        id: btn_mouse
        anchors.fill: parent
        onClicked: root.buttonPressed()
    }
}
```

### Building for Hardware Targets
```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=$QUL_ROOT/lib/cmake/Qul/toolchain/armgcc.cmake \
  -DQUL_PLATFORM=<board-name> \
  -DQUL_TARGET_TOOLCHAIN_DIR=/path/to/arm-none-eabi \
  -DQUL_BOARD_SDK_DIR=/path/to/board-sdk

cmake --build build
```

---

## 4. How to Add a New UI Component Type

To add a new component (e.g. `CustomSwitchComponent`):

1. **Subclass `UIComponent`** in `src/models/`:
   - Override `paintComponent(QPainter* painter)`.
   - Override `toJson()` and `fromJson()`.
   - Implement `toQmlSnippet()` (using only QUL primitives: `Rectangle`, `Text`, `MouseArea`).
   - Implement `toUgfxSnippet()` (using `GWidgetInit` / `gwinWidgetClearInit(&wi)`).

2. **Register in Factory & UI Panels**:
   - In [ComponentPalette.cpp](file:///c:/Users/varun/Desktop/embedded%20UI%20development/src/panels/ComponentPalette.cpp): add entry to `m_listWidget`.
   - In [CanvasView.cpp](file:///c:/Users/varun/Desktop/embedded%20UI%20development/src/canvas/CanvasView.cpp) (`createComponentByType`): instantiate class.
   - In [Project.cpp](file:///c:/Users/varun/Desktop/embedded%20UI%20development/src/project/Project.cpp) (`createComponentInstance`): instantiate class on JSON deserialization.
   - In [PropertiesPanel.cpp](file:///c:/Users/varun/Desktop/embedded%20UI%20development/src/panels/PropertiesPanel.cpp): add inspector widgets.

---

## 5. Command Pattern (Undo / Redo System)

All user mutations on the visual canvas and property inspectors are tracked via Qt's `QUndoStack` and encapsulated inside `QUndoCommand` subclasses located in `src/commands/`:

| Command Class | Target Subsystem | Description |
| :--- | :--- | :--- |
| `AddComponentCommand` | Canvas & Project | Adds a newly dropped component to `CanvasScene` and registers it in `Project`. Undoing cleanly detaches and hides the component; redoing re-adds it. |
| `DeleteComponentCommand` | Canvas & Project | Supports multi-component selection deletion. Preserves references and restores full component states upon undo. |
| `MoveComponentCommand` | CanvasScene | Records initial and final `(x, y)` positions for single or multi-selected items. Features `mergeWith()` support (`Id = 1002`) to coalesce rapid drag updates into a single undo step. |
| `ResizeComponentCommand` | CanvasScene | Captures bounding rect transformations (`oldGeom` vs `newGeom`) from interactive resize handles. |
| `PropertyChangeCommand` | PropertiesPanel | Captures pre- and post-mutation JSON state snapshots (`QJsonObject`) for any edited property (color, font, text, min/max, checked state). |

### Integration with MainWindow & Panels
`MainWindow` owns the central `QUndoStack`:
```cpp
m_undoStack = new QUndoStack(this);
m_scene->setUndoStack(m_undoStack);
m_propertiesPanel->setUndoStack(m_undoStack);
```
Standard keyboard shortcuts `Ctrl+Z` (Undo) and `Ctrl+Y` / `Ctrl+Shift+Z` (Redo) are connected to `m_undoStack->createUndoAction()` and `m_undoStack->createRedoAction()`.

---

## 6. Hardware Bridge & Flashing Subsystem (`src/hardware/`)

The hardware subsystem facilitates testing directly on physical microcontrollers through automated port detection and external programmer toolchain execution.

### Device Manager (`DeviceManager.h/cpp`)
- **Port Auto-Detection**: Uses `QSerialPortInfo::availablePorts()` coupled with an active polling timer (`QTimer`) to detect plug-and-play USB device insertions and removals.
- **Hardware Profile Matching**: Compares detected `vendorId` and `productId` against `resources/config/devices.json` to identify target architectures (STM32, ESP32, Raspberry Pi RP2040, NXP i.MX RT).
- **Command Synthesis**: Automatically generates platform-appropriate flashing invocations (e.g. `esptool.py --port ... write_flash 0x10000 firmware.bin`).

### Flash Controller (`FlashController.h/cpp`)
- **Asynchronous Execution**: Wraps external vendor programming CLI tools using `QProcess` so the UI remains responsive during long erase/flash cycles.
- **Supported Toolchain Backends**:
  - **STM32**: `STM32_Programmer_CLI` via SWD or USB DFU interfaces (`-c port=SWD mode=UR -w <bin> 0x08000000 -v -rst`).
  - **ESP32**: `esptool.py` over serial UART (`--chip esp32 --port <port> --baud 921600 write_flash -z 0x10000 <bin>`).
  - **OpenOCD / RISC-V**: OpenOCD script execution for GDB/SWD probes (J-Link, CMSIS-DAP).
  - **UF2 Mass Storage**: Direct binary copy to mounted USB mass storage volumes (RP2040 bootloader, SAMD21).
- **Diagnostics & Signals**: Streams stdout/stderr via `consoleOutputUpdate(QString)`, emits `flashingStarted()`, and delivers final status with `flashingFinished(bool success, int exitCode)`.

---

## 7. Asset Compilation Pipeline (`src/assets/`)

Microcontroller framebuffers require specific memory layouts and color packings that differ from desktop RGBA32 formats. `ImageAssetProcessor` converts standard images (`QImage`) into flash-ready C byte arrays:

### Supported Formats (`ImageFormat`)
- **`ImageFormat::RGB565`**: 16-bit high color (5-bit red, 6-bit green, 5-bit blue). Packed into little-endian or big-endian 16-bit integers suitable for ST7789, ILI9341, and SSD1351 display controllers.
- **`ImageFormat::Monochrome`**: 1-bit per pixel thresholded bitmap. 8 horizontal pixels are packed into a single `uint8_t` byte, designed for monochrome OLEDs (SSD1306) and e-Paper displays (UC8151).
- **`ImageFormat::RGB888`**: 24-bit true color (8-bit red, 8-bit green, 8-bit blue) for 24-bit parallel RGB displays.

### Output Methods
- **`generateCArray(const QImage& image, ImageFormat format, const QString& arrayName)`**:
  Generates a formatted C source string ready to be saved into generated projects:
  ```c
  // Formatted C array output
  const uint8_t logo_data[] = {
      0x1f, 0x00, 0xf8, 0x00, 0x07, 0xe0, ...
  };
  ```
- **`processToBytes(const QImage& image, ImageFormat format)`**:
  Returns raw binary `QByteArray` for streaming or embedded filesystem packing.
