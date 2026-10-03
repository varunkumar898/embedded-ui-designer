# Architecture

> **Standing rule (effective immediately):** Before touching any code, consult this file to
> identify the exact file(s) needed. Read and edit **only those files**. Do not grep or scan
> the entire `src/` tree "to be safe." If this document doesn't clearly point to the right
> file, say so explicitly and ask rather than broad-scanning.
>
> **Maintenance rule:** After any task that adds, renames, or removes a file, update the
> table below in the same commit so it never goes stale.

---

## Directory Map

```
embedded-ui-designer/
├── src/
│   ├── main.cpp                    Application entry point
│   ├── MainWindow.{h,cpp}          Top-level shell (menus, toolbars, dock wiring)
│   ├── canvas/                     Rendering surface
│   ├── panels/                     Dock panels & color picker
│   ├── models/                     Component data model & display config
│   ├── commands/                   Qt undo/redo command objects
│   ├── codegen/                    Code generators (abstract base + 3 targets)
│   ├── project/                    Project serialization (save/load/autosave)
│   ├── hardware/                   Device detection & firmware flashing
│   ├── dialogs/                    Modal dialogs (flash, serial monitor, new-project)
│   └── assets/                     Image-to-C-array conversion utility
├── Dockerfile                      Docker dev environment image definition
├── docker-compose.yml              Docker Compose service (X11 forwarding for GUI)
├── CMakeLists.txt                  CMake build definition
└── .gitignore                      Excludes build/, build-*/, output_*/ (see below)
```

---

## Feature → File(s) Table

| Feature | File path(s) | One-line purpose |
|---------|-------------|-----------------|
| **Canvas – rendering surface** | `src/canvas/CanvasScene.{h,cpp}` | `QGraphicsScene` subclass; owns the component list, grid, snap-to-grid, display boundary, and emits selection/change signals |
| **Canvas – interaction & zoom** | `src/canvas/CanvasView.{h,cpp}` | `QGraphicsView` subclass; handles mouse events, rubber-band drag-select, zoom, panning, drag-drop from palette, pen path drawing mode, and context menu |
| **Selection (single & multi)** | `src/canvas/CanvasScene.{h,cpp}` + `src/canvas/CanvasView.{h,cpp}` | Scene emits `selectionListChanged`; View drives rubber-band multi-select and propagates it; handled together |
| **Properties panel** | `src/panels/PropertiesPanel.{h,cpp}` | Dock panel showing geometry, ID, and component-specific properties (text, colors, radii, font, handler name, etc.) for the selected component; also exposes alignment/distribute buttons |
| **Styles / color picking** | `src/panels/ColorPickerDialog.{h,cpp}` | Custom color-wheel dialog with HSV wheel, hex input, RGB-565 preview, and harmony swatches; invoked by color buttons inside `PropertiesPanel` — there is no separate "Styles panel" |
| **Layers panel** | `src/panels/LayerPanel.{h,cpp}` | Dock panel listing all components in z-order with move-up / move-down / delete controls; syncs selection with `CanvasScene` |
| **Component palette** | `src/panels/ComponentPalette.{h,cpp}` | Left-dock widget listing available component types; emits `componentDoubleClicked` and `shapeToolSelected` signals consumed by `MainWindow` / `CanvasView` |
| **Prototype / interaction panel** | `src/panels/PrototypePanel.{h,cpp}` | Right-dock panel for defining on-component event→target interactions (e.g., "on click → show component X"); stores interactions as JSON on the component |
| **Component data model (base)** | `src/models/UIComponent.{h,cpp}` | Abstract `QGraphicsObject` base for all widgets; owns id, type, geometry, resize/corner-radius handles, interaction array, and the `toJson`/`fromJson`/`toQmlSnippet`/`toUgfxSnippet` virtual interface |
| **Display configuration** | `src/models/DisplayConfig.h` | Plain struct holding screen width, height, color depth, type (LCD/OLED/E-Ink), DPI, round flag; serializes to/from JSON |
| **Custom components – Button** | `src/models/ButtonComponent.{h,cpp}` | Clickable button with fill/border/text colors and corner radius |
| **Custom components – Label** | `src/models/LabelComponent.{h,cpp}` | Text label with font size, bold/italic, color, and horizontal alignment |
| **Custom components – Checkbox** | `src/models/CheckboxComponent.{h,cpp}` | Checkbox widget with checked-state, fill, and accent colors |
| **Custom components – Switch** | `src/models/SwitchComponent.{h,cpp}` | Toggle switch with on/off state and track/thumb colors |
| **Custom components – Slider** | `src/models/SliderComponent.{h,cpp}` | Horizontal slider with min/max/value and track/thumb colors |
| **Custom components – ProgressBar** | `src/models/ProgressBarComponent.{h,cpp}` | Progress bar (0.0–1.0 value) with fill and background colors |
| **Custom components – TextInput** | `src/models/TextInputComponent.{h,cpp}` | Text input field with placeholder, read-only flag, border, and text colors |
| **Custom components – Rectangle** | `src/models/RectangleComponent.{h,cpp}` | Filled rectangle with fill/border colors and corner radius |
| **Custom components – Circle** | `src/models/CircleComponent.{h,cpp}` | Filled ellipse/circle with fill and border colors |
| **Custom components – Image** | `src/models/ImageComponent.{h,cpp}` | Image widget referencing an external file path and pixel format (RGB565/Mono/RGB888) |
| **Custom components – Path** | `src/models/PathComponent.{h,cpp}` | Bézier-path shape with anchor points, control handles, stroke/fill, and flattening for export |
| **Image-to-C-array conversion** | `src/assets/ImageAssetProcessor.{h,cpp}` | Static utility that converts a `QImage` to a packed C `uint8_t` array string (RGB565, Monochrome, or RGB888) for embedding in firmware |
| **Code generator – abstract base** | `src/codegen/CodeGenerator.{h,cpp}` | Pure-virtual `generate(outputDir)` interface plus a `writeFile` helper; all concrete generators inherit from this |
| **Code generator – µGFX** | `src/codegen/UgfxGenerator.{h,cpp}` | Emits a complete µGFX C project: `CMakeLists.txt`, `gfxconf.h`, `ui.h`, `ui.c`, `main.c`, `README.md` |
| **Code generator – Qt for MCU (QUL)** | `src/codegen/QtMcuGenerator.{h,cpp}` | Emits a Qt for MCU QML project: `CMakeLists.txt`, `.qmlproject`, `Design.qml`, `README.md` |
| **Code generator – LVGL** | `src/codegen/LvglGenerator.{h,cpp}` | Emits a full LVGL C project: `CMakeLists.txt`, `lv_conf.h`, `ui.h`, `ui.c`, `main.c`, `idf_component.yml`, `platformio.ini`, `README.md` |
| **Project save / load** | `src/project/Project.{h,cpp}` | Owns project name, file path, target framework, `DisplayConfig`, and dirty flag; serializes the entire scene to JSON; handles autosave to app-data directory and sample-project loading |
| **Device manager** | `src/hardware/DeviceManager.{h,cpp}` | Polls serial/COM/tty ports on a timer; exposes port list, vendor details, board-type detection, and flash-command lookup; emits connect/disconnect signals |
| **Firmware flashing** | `src/hardware/FlashController.{h,cpp}` | Launches OpenOCD, ST-Link, or esptool as a `QProcess`; streams real-time console output; supports STM32, RISC-V, and ESP32 targets |
| **Flash dialog** | `src/dialogs/FlashDialog.{h,cpp}` | Modal dialog for selecting port, binary path, and triggering `FlashController`; shows live console log |
| **Serial monitor dialog** | `src/dialogs/SerialMonitorDialog.{h,cpp}` | Modal dialog for opening a serial port and displaying received data |
| **New project dialog** | `src/dialogs/NewProjectDialog.{h,cpp}` | Modal dialog to configure project name, resolution preset, and target framework before creating a new project |
| **Undo / redo commands** | `src/commands/AddComponentCommand.{h,cpp}`<br>`src/commands/DeleteComponentCommand.{h,cpp}`<br>`src/commands/MoveComponentCommand.{h,cpp}`<br>`src/commands/ResizeComponentCommand.{h,cpp}`<br>`src/commands/PropertyChangeCommand.{h,cpp}`<br>`src/commands/AlignDistributeCommand.{h,cpp}`<br>`src/commands/BooleanPathCommand.{h,cpp}` | Seven `QUndoCommand` subclasses for reversible add, delete, move, resize, property, align/distribute, and Boolean-path operations |
| **Application shell** | `src/MainWindow.{h,cpp}` | Top-level `QMainWindow`; creates and wires all subsystems, menus, toolbars, and dock panels; dispatches File / Edit / View / Export / Hardware menu actions |
| **Entry point** | `src/main.cpp` | Creates `QApplication` and `MainWindow`; applies global dark theme |
| **Docker dev environment** | `Dockerfile` + `docker-compose.yml` | Ubuntu 22.04 image installing Qt 6.5.3 via `aqtinstall` with X11 forwarding so the GUI can be developed inside Docker |

---

## `.gitignore` – Build & Export Output Coverage

The following patterns are explicitly excluded and **confirmed not tracked** (see verification below):

| Pattern | What it excludes |
|---------|-----------------|
| `build/` | CMake build directory |
| `build-*/` | Any variant build directories (e.g., `build-release/`) |
| `cmake-build-*/` | CLion-style build directories |
| `out/` | Generic output folder |
| `output_ugfx/` | µGFX code generator output |
| `output_qul/` | Qt for MCU code generator output |
| `output_lvgl/` | LVGL code generator output |
| `output_*/` | Any other generator output |

> **Verification result:** `git ls-files | grep -E "^build/"` → **empty output (exit code 1 = no matches)**.
> No files under `build/` have ever been committed to the repository. ✅

