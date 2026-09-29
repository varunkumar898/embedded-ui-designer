# Embedded UI Designer

> Visual drag-and-drop UI designer and code generator for embedded microcontrollers (µGFX C & Qt Quick Ultralite).

![Language](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Framework](https://img.shields.io/badge/Qt-6.5%20LTS%20(LGPLv3)-green.svg)
![Platforms](https://img.shields.io/badge/Platforms-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)
![Build](https://img.shields.io/badge/Build-CMake-orange.svg)
[![Embedded UI Designer CI/CD](https://github.com/varunkumar898/embedded-ui-designer/actions/workflows/build.yml/badge.svg)](https://github.com/varunkumar898/embedded-ui-designer/actions/workflows/build.yml)

---

## ⚡ Overview

Embedded UI Designer bridges the gap between hardware engineering and modern graphical interfaces. Rather than manually hardcoding coordinates or complex display driver calls, developers can:
1. **Design visually** at the exact resolution of the target hardware (320×240, 480×320, 800×480, etc.).
2. **Configure properties** (colors, typography, corner radii, signals/slots, progress bounds).
3. **Export ready-to-compile projects**:
   - **µGFX C project**: Royalty-free, single-file compilation (`src/gfx_mk.c`), tested directly on PC (Win32/Linux) or baremetal microcontrollers.
   - **Qt for MCUs project**: Targets Qt Quick Ultralite (QUL 2.x+) using `qul_add_target(...)` and supported primitives (`Rectangle`, `Text`, `MouseArea`).
4. **Automate in CI/CD**: Headless CLI export mode guarantees your generated projects compile before touching hardware.

---

## 📂 Repository Directory Layout

```
embedded-ui-designer/
├── CMakeLists.txt                       # CMake build config (qt_add_executable WIN32 MACOSX_BUNDLE)
├── README.md                            # Project documentation and build guide
├── .github/
│   └── workflows/
│       ├── build.yml                    # CI workflow with automated export-compile verification
│       └── release.yml                  # Release workflow (Windows .zip, Linux AppImage, macOS DMG)
├── packaging/
│   ├── app.rc                           # Windows executable icon resource script
│   ├── embedded-ui-designer.desktop     # Linux desktop entry specification for AppImage
│   ├── embedded-ui-designer.png         # 256x256 application icon
│   ├── embedded-ui-designer.ico         # Windows icon asset
│   └── embedded-ui-designer.icns        # macOS icon bundle asset
├── src/
│   ├── main.cpp                         # Desktop bootstrap & Headless CLI export engine
│   ├── MainWindow.h/cpp                 # Window chrome, menus, toolbars, docks, autosave, theme
│   ├── assets/
│   │   └── ImageAssetProcessor.h/cpp    # Image converter (RGB565, 1-bit monochrome, RGB888 C arrays)
│   ├── canvas/
│   │   ├── CanvasScene.h/cpp            # Target display boundary, grid, snap, component tracking
│   │   └── CanvasView.h/cpp             # Interactive canvas with zoom (25%-400%), pan, drag & drop
│   ├── codegen/
│   │   ├── CodeGenerator.h/cpp          # Code generator abstract base class
│   │   ├── IExporter.h                  # Exporter base interface
│   │   ├── UgfxGenerator.h/cpp          # Production µGFX C generator (gfxconf.h, ui.c/h, main.c, CMakeLists.txt)
│   │   ├── QtMcuGenerator.h/cpp         # Production Qt Quick Ultralite generator (qul_add_target, .qmlproject)
│   │   ├── UgfxExporter.h/cpp           # µGFX export implementation for DocumentModel
│   │   └── QtMcuExporter.h/cpp          # QUL export implementation for DocumentModel
│   ├── commands/
│   │   ├── AddComponentCommand.h/cpp    # Undo/redo: canvas component addition
│   │   ├── DeleteComponentCommand.h/cpp # Undo/redo: canvas component deletion
│   │   ├── MoveComponentCommand.h/cpp   # Undo/redo: canvas component translation & batch move
│   │   ├── ResizeComponentCommand.h/cpp # Undo/redo: interactive component handle resizing
│   │   ├── PropertyChangeCommand.h/cpp  # Undo/redo: property inspector JSON state mutation
│   │   ├── AddWidgetCommand.h/cpp       # Undo/redo: DocumentModel widget addition
│   │   └── MoveWidgetCommand.h/cpp      # Undo/redo: DocumentModel widget positioning
│   ├── hardware/
│   │   ├── DeviceManager.h/cpp          # QSerialPortInfo auto-detection & board profile mapping
│   │   └── FlashController.h/cpp        # Asynchronous flashing toolchain bridge (OpenOCD, ST-Link, esptool)
│   ├── models/
│   │   ├── DisplayConfig.h              # Target display resolution, color depth, type model
│   │   ├── UIComponent.h/cpp            # Base QGraphicsObject with selection handles & serialization
│   │   ├── ButtonComponent.h/cpp        # Button (text, background, radius, onClicked stubs)
│   │   ├── LabelComponent.h/cpp         # Text label (font family, pixel size, bold/italic, color)
│   │   ├── RectangleComponent.h/cpp     # Shape panel (fill color, stroke color, stroke width, radius)
│   │   ├── ProgressBarComponent.h/cpp   # Progress indicator (0.0–1.0 value, bar/track colors)
│   │   ├── ImageComponent.h/cpp         # Bitmap image component
│   │   ├── SliderComponent.h/cpp        # Linear slider (orientation, min/max, value, track/thumb colors)
│   │   ├── SwitchComponent.h/cpp        # Toggle switch (checked state, thumb/track styling)
│   │   ├── CheckboxComponent.h/cpp      # Checkbox (checked state, box and label rendering)
│   │   ├── TextInputComponent.h/cpp     # Single-line text input (placeholder, keyboard support)
│   │   ├── CircleComponent.h/cpp        # Ellipse/circle shape (fill, stroke, radius)
│   │   ├── DocumentModel.h/cpp          # Multi-screen DOM document model
│   │   ├── ScreenModel.h/cpp            # Screen model container
│   │   └── WidgetModel.h/cpp            # Declarative widget model
│   ├── panels/
│   │   ├── ComponentPalette.h/cpp       # Left dock toolbox with draggable UI elements
│   │   ├── PropertiesPanel.h/cpp        # Right dock live bi-directional property inspector
│   │   └── LayerPanel.h/cpp             # Visual z-order layer tree and management
│   ├── project/
│   │   └── Project.h/cpp                # .euiproj JSON serialization & AppDataLocation storage
│   └── qml/
│       ├── Main.qml                     # Declarative QML designer interface
│       └── qml.qrc                      # QML resource bundle definition
├── resources/
│   ├── app.qrc                          # Embedded Qt resource file (embedded icons, configs, samples)
│   └── config/
│       └── devices.json                 # Pre-configured MCU profiles (STM32H7, ESP32-S3, NXP RT1060)
├── examples/
│   ├── simple.euiproj                   # Minimal test project (Button, Label, Rect, ProgressBar)
│   └── sample_dashboard.euiproj         # Thermostat dashboard project
└── docs/
    ├── USER_GUIDE.md                    # Visual designer usage, export, and flashing guide
    ├── DEVELOPER.md                     # Architecture guide & how to add components / generators
    └── specs/                           # Specifications and execution roadmap
```

---

## 🛠️ Building Locally

### Prerequisites
- **Qt 6.5+** (Core, Gui, Widgets, Qml, Quick, SerialPort)
- **CMake 3.20+**
- **C++17 Compiler** (MSVC 2019/2022 on Windows, GCC 9+ on Linux, Apple Clang on macOS)

### Build Commands

```bash
# Clone the repository
git clone https://github.com/your-org/embedded-ui-designer.git
cd embedded-ui-designer

# Configure CMake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build the executable
cmake --build build --config Release --parallel

# Launch the Designer
./build/EmbeddedUIDesigner              # Linux / macOS
.\build\Release\EmbeddedUIDesigner.exe   # Windows
```

---

## 💻 Headless CLI Mode (Automated Testing)

The designer executable includes a command-line interface for automated builds and testing without launching a GUI:

```bash
# Export µGFX C project
./EmbeddedUIDesigner --export ugfx --project examples/simple.euiproj --out build/ugfx_out

# Compile exported µGFX project on your PC
cmake -S build/ugfx_out -B build/ugfx_out/build -DUGFX_PATH=/path/to/ugfx
cmake --build build/ugfx_out/build

# Export Qt for MCUs (QUL) project
./EmbeddedUIDesigner --export qul --project examples/simple.euiproj --out build/qul_out
```

---

## 📦 Packaging as a Double-Click App

- **Windows**: Built with `WIN32` flag to suppress the console window. Run `windeployqt --release dist/EmbeddedUIDesigner.exe`.
- **macOS**: Built as a native `.app` bundle with `MACOSX_BUNDLE` and embedded `.icns` icon.
- **Linux**: Bundled into a standalone `.AppImage` using `linuxdeploy` and `packaging/embedded-ui-designer.desktop`.
- **GitHub Release**: Tagging any commit (`git tag v0.1.0 && git push origin v0.1.0`) triggers `.github/workflows/release.yml` to automatically build and attach Windows `.zip`, Linux `.AppImage`, and macOS `.dmg` downloads to GitHub Releases.

---

## 🗺️ Development Roadmap

- [x] **Stage 1 (Current)**:
  - Drag-and-drop canvas with resolution boundaries, grid, and snap.
  - Component hierarchy: Button, Label, Rectangle, ProgressBar.
  - Properties panel & Layer tree synchronization.
  - `.euiproj` JSON format & `QStandardPaths::AppDataLocation` storage.
  - **µGFX Exporter**: `gfxconf.h` feature flags, `GWidgetInit` pattern, and single-file `src/gfx_mk.c` build.
  - **Qt for MCUs Exporter**: Qt Quick Ultralite (`find_package(Qul)`, `qul_add_target`, `QML_PROJECT`).
  - Headless CLI export mode & automated CI compilation test.
- [ ] **Stage 2**: Expanded components (Image, Slider, Switch, Checkbox, Text input, Circle, Line, Multi-screen).
- [ ] **Stage 3**: Advanced editing (Undo/redo stack, copy/paste, multi-select, alignment tools).
- [ ] **Stage 4**: Embedded hardware toolchain (Asset pipeline PNG->C array, QSerialPortInfo port detection, vendor flash command preview for STM32/ESP32).
- [ ] **Stage 5**: Template gallery (Thermostat, automotive cluster, calculator, smartwatch).

---

## 📄 License & Notices

- **Application Code**: Licensed under the **MIT License**.
- **Qt Libraries**: This application is built using Qt 6.5 LTS under the **GNU Lesser General Public License (LGPLv3)**. Qt is a registered trademark of The Qt Company Ltd. and its subsidiaries. This application dynamically links against unmodified Qt libraries in compliance with LGPLv3. See [qt.io/licensing](https://www.qt.io/licensing/) for details.
