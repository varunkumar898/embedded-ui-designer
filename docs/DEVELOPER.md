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
                                  │ contains UIComponent*
                     ┌────────────┴────────────┐
                     ▼                         ▼
             ┌───────────────┐         ┌───────────────┐
             │    Project    │         │ CodeGenerator │
             │  (.euiproj)   │         │ (µGFX & QUL)  │
             └───────────────┘         └───────────────┘
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
