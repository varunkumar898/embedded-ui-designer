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
| **Canvas – rendering surface** | `src/canvas/CanvasScene.{h,cpp}` | `QGraphicsScene` subclass; owns the component list, grid, snap-to-grid, ruler guides (h/v), snap-to-guides+edges, snap-highlight, display boundary, and emits selection/change signals |
| **Canvas – interaction & zoom** | `src/canvas/CanvasView.{h,cpp}` | `QGraphicsView` subclass; handles mouse events, rubber-band drag-select, zoom, panning, drag-drop from palette, pen path drawing mode, ruler strips (20 px, tick-marked), guide drag-from-ruler, guide move/delete, smart-snap highlight overlay, and context menu |
| **Selection (single & multi)** | `src/canvas/CanvasScene.{h,cpp}` + `src/canvas/CanvasView.{h,cpp}` | Scene emits `selectionListChanged`; View drives rubber-band multi-select and propagates it; handled together |
| **Properties panel** | `src/panels/PropertiesPanel.{h,cpp}` | Dock panel showing geometry, ID, and component-specific properties (text, colors, radii, font, handler name, etc.) for the selected component; exposes alignment/distribute buttons and a hardware "Protocol" dropdown (None / GPIO / PWM / ADC / I2C / SPI) revealing per-role board pin dropdowns |
| **Styles / color picking** | `src/panels/ColorPickerDialog.{h,cpp}` | Custom color-wheel dialog with HSV wheel, hex input, RGB-565 preview, and harmony swatches; invoked by color buttons inside `PropertiesPanel` |
| **Named color styles** | `src/panels/StylesPanel.{h,cpp}`, `src/models/ColorStyle.h`, `src/project/Project.cpp` | "THEME STYLES" dock listing project-scoped named colors; recoloring a style calls `Project::updateColorStyle`, which live-propagates the new color to every canvas component referencing it (via `UIComponent::applyColorStyle`) with no reload. Component color properties serialize as a `{"styleRef":"Name"}` object when bound, plain hex otherwise. `PropertiesPanel` color rows expose a Hex ⇄ Named-Style toggle. |
| **Layers panel** | `src/panels/LayerPanel.{h,cpp}` | Dock panel listing all components in z-order with move-up / move-down / delete controls; syncs selection with `CanvasScene` |
| **Component palette** | `src/panels/ComponentPalette.{h,cpp}` | Left-dock widget listing built-in component types + "MY COMPONENTS" section; "+ Save" button emits `saveAsComponentRequested`; `refreshCustomComponents(QStringList)` slot rebuilds the custom list; drag and double-click handled by `MainWindow` |
| **Prototype / interaction panel** | `src/panels/PrototypePanel.{h,cpp}` | Right-dock panel for defining on-component event→target interactions (e.g., "on click → show component X"); stores interactions as JSON on the component |
| **Component data model (base)** | `src/models/UIComponent.{h,cpp}` | Abstract `QGraphicsObject` base for all widgets; owns id, type, geometry, resize/corner-radius handles, interaction array, named color-style references (`setColorStyleRef`/`colorStyleRef`/`applyColorStyle`), color serialization helpers (`serializeColor`/`deserializeColor`), hardware protocol binding (`protocol`/`protocolPins`), and the `toJson`/`fromJson`/`toQmlSnippet`/`toUgfxSnippet` virtual interface |
| **Display configuration** | `src/models/DisplayConfig.h` | Plain struct holding screen width, height, color depth, type (LCD/OLED/E-Ink), DPI, round flag; serializes to/from JSON |
| **Custom components – Button** | `src/models/ButtonComponent.{h,cpp}` | Clickable button with fill/border/text colors and corner radius |
| **Custom components – Label** | `src/models/LabelComponent.{h,cpp}` | Text label with font family (dropdown, 18 options), size, bold/italic, color, alignment, letter-spacing, and line-height; codegen maps to QML font.letterSpacing/lineHeight and LVGL text_letter_space/text_line_space |
| **Custom components – Checkbox** | `src/models/CheckboxComponent.{h,cpp}` | Checkbox widget with checked-state, fill, and accent colors |
| **Custom components – Switch** | `src/models/SwitchComponent.{h,cpp}` | Toggle switch with on/off state and track/thumb colors |
| **Custom components – Slider** | `src/models/SliderComponent.{h,cpp}` | Horizontal slider with min/max/value and track/thumb colors |
| **Custom components – ProgressBar** | `src/models/ProgressBarComponent.{h,cpp}` | Progress bar (0.0–1.0 value) with fill and background colors |
| **Custom components – TextInput** | `src/models/TextInputComponent.{h,cpp}` | Text input field with placeholder, read-only flag, border, and text colors |
| **Custom components – Rectangle** | `src/models/RectangleComponent.{h,cpp}` | Filled rectangle with fill/border colors and corner radius |
| **Custom components – Circle** | `src/models/CircleComponent.{h,cpp}` | Filled ellipse/circle with fill and border colors |
| **Custom components – Image** | `src/models/ImageComponent.{h,cpp}` | Image widget referencing an external file path and pixel format (RGB565/Mono/RGB888) |
| **Custom components – Path** | `src/models/PathComponent.{h,cpp}` | Bézier-path shape with anchor points, control handles, stroke/fill, and flattening for export |
| **Custom component library – Definition** | `src/models/ComponentDefinition.{h,cpp}` | Stores a reusable component template: unique ID, display name, base-shape JSON, and a list of named `ComponentVariant`s (fill/stroke/accent color overrides); `defaultVariants()` provides Primary/Secondary/Danger presets; serialises into the project's `componentLibrary` JSON array |
| **Custom component library – Instance** | `src/models/CustomComponentInstance.{h,cpp}` | `UIComponent` subclass that references a `ComponentDefinition` by ID; `applyVariant()` swaps fill/stroke/accent colors; renders a rounded-rect with variant label; persists definitionId + variant name in `.euiproj` |
| **Image-to-C-array conversion** | `src/assets/ImageAssetProcessor.{h,cpp}` | Static utility that converts a `QImage` to a packed C `uint8_t` array string (RGB565, Monochrome, or RGB888) for embedding in firmware |
| **Code generator – abstract base** | `src/codegen/CodeGenerator.{h,cpp}` | Pure-virtual `generate(outputDir)` interface plus a `writeFile` helper; all concrete generators inherit from this |
| **Code generator – µGFX** | `src/codegen/UgfxGenerator.{h,cpp}` | Emits a complete µGFX C project: `CMakeLists.txt`, `gfxconf.h`, `ui.h`, `ui.c`, `main.c`, `README.md`, and desktop simulator subfolder `pc_simulator/` (X11/Win32 driver matched to DisplayConfig) |
| **Code generator – Qt for MCU (QUL)** | `src/codegen/QtMcuGenerator.{h,cpp}` | Emits a Qt for MCU QML project: `CMakeLists.txt`, `.qmlproject`, `Design.qml`, `README.md`, and desktop simulator subfolder `pc_simulator/` (QUL desktop platform or host Qt6 Quick fallback) |
| **Code generator – LVGL** | `src/codegen/LvglGenerator.{h,cpp}` | Emits a full LVGL C project: `CMakeLists.txt`, `lv_conf.h`, `ui.h`, `ui.c`, `main.c`, `idf_component.yml`, `platformio.ini`, `README.md`, and desktop simulator subfolder `pc_simulator/` with SDL2 driver matching DisplayConfig |
| **Project save / load** | `src/project/Project.{h,cpp}` | Owns project name, file path, target framework, `DisplayConfig`, component library (`QList<ComponentDefinition>`), named color styles (`QList<ColorStyle>`), and dirty flag; serializes entire scene + library + `colorStyles` array to JSON; `updateColorStyle` live-propagates; autosave and sample-project loading |
| **QML design import** | `src/project/QmlImporter.{h,cpp}` | Imports literal-only QML designs (Rectangle, Text, Button, Image, Slider, ProgressBar) while strictly rejecting and reporting bindings, expressions, anchors, scripts, and unsupported elements |
| **Device manager** | `src/hardware/DeviceManager.{h,cpp}` | Polls serial/COM/tty ports on a timer; exposes port list, vendor details, board-type detection, and flash-command lookup; emits connect/disconnect signals |
| **OpenOCD probe manager** | `src/hardware/OpenOcdManager.{h,cpp}` | Background daemon & probe scanner detecting ST-Link, CMSIS-DAP, J-Link, ESP32, and RP2040; searches multiple binary locations (`/usr/bin/openocd`, `/usr/local/bin/openocd`, `$PATH`), interface scripts (`stlink.cfg`, `stlink-v2.cfg`), and target scripts (`stm32f0x.cfg`, etc.); manages TCP TCL/Telnet register poke and memory read/write |
| **Hardware bridge (ADC, SPI, PWM, GPIO)** | `src/hardware/HardwareBridge.{h,cpp}` | Board profiles with per-pin ADC channel, PWM timer/channel, and SPI mapping; formal ADC setup-then-poll pattern; SPI raw 8-bit master transfer; throttled live PWM writes; atomic BSRR GPIO digital output; component binding engine with physical OpenOCD/ST-Link detection, USB (`lsusb` VID:PID) probe, `/dev/ttyACM*` discovery, live hardware verification on STM32 Nucleo-F030R8, and simulation fallback |
| **Hardware backend abstraction & targets** | `src/hardware/HardwareManager.{h,cpp}`, `src/hardware/HardwareBackend.h`, `src/hardware/HardwareTarget.{h,cpp}`, `src/hardware/STM32Backend.{h,cpp}`, `src/hardware/ESP32Backend.{h,cpp}`, `src/hardware/RaspberryPiBackend.{h,cpp}`, `src/hardware/MockBackend.{h,cpp}` | Pluggable backend architecture decoupling physical probes, board pinout routing, and peripheral registers from the UI; supports hot-swapping between physical hardware and mock/simulation targets |
| **Multi-screen project architecture** | `src/models/Screen.{h,cpp}`, `src/panels/ScreensPanel.{h,cpp}`, `src/commands/ScreenCommands.{h,cpp}` | Project-wide multi-screen management; supports creating, duplicating, reordering, deleting, and activating individual screens with dedicated component hierarchies and undo/redo |
| **Data sources & bindings** | `src/models/DataSource.{h,cpp}`, `src/models/DataBinding.{h,cpp}`, `src/models/ComponentState.{h,cpp}` | Data source abstraction supporting GPIO, ADC, Timer, and custom variables; bidirectional property binding with live evaluation, format expressions, and threshold-triggered component visual states |
| **Code generator v2 (IR & HAL)** | `src/codegen/GeneratorContext.{h,cpp}`, `src/codegen/GeneratorIR.{h,cpp}`, `src/codegen/TargetHalGenerator.{h,cpp}`, `src/codegen/BindingLayerGenerator.{h,cpp}` | Framework-agnostic intermediate representation (IR) compiler and target HAL generator emitting clean multi-screen firmware code and peripheral binding boilerplate |
| **Desktop simulator** | `src/simulator/SimulationRuntime.{h,cpp}`, `src/simulator/SimulationBackend.{h,cpp}`, `src/simulator/SimulatorWindow.{h,cpp}`, `src/simulator/SimulationControlPanel.{h,cpp}`, `src/simulator/SimulationCanvasView.{h,cpp}` | Standalone desktop simulation engine providing interactive virtual hardware pins, simulated sensor wave generators, step/pause/run clock controls, bidirectional data bindings, and visual runtime validation |
| **Board config parser (.ioc, sdkconfig)** | `src/hardware/BoardConfigParser.{h,cpp}` | Parser for STM32CubeMX `.ioc` files and ESP-IDF `sdkconfig` files; maps configured pins to GPIO, ADC, PWM, SPI, and I2C modes while gating unknown/system pins |
| **Firmware flashing** | `src/hardware/FlashController.{h,cpp}` | Launches OpenOCD, ST-Link, or esptool as a `QProcess`; streams real-time console output; supports STM32, RISC-V, and ESP32 targets |
| **Pin configuration & binding dialog** | `src/dialogs/PinBindingDialog.{h,cpp}` | Dialog for board selection, per-pin mode configuration (Digital In/Out, PWM Output, Analog In, SPI Raw Transfer (byte in/out)), unavailable mode gating, component binding, interactive ADC potentiometer simulation, and manual SPI raw send/receive panel |
| **Flash dialog** | `src/dialogs/FlashDialog.{h,cpp}` | Modal dialog for selecting port, binary path, and triggering `FlashController`; shows live console log |
| **Serial monitor dialog** | `src/dialogs/SerialMonitorDialog.{h,cpp}` | Modal dialog for opening a serial port and displaying received data |
| **New project dialog** | `src/dialogs/NewProjectDialog.{h,cpp}` | Modal dialog to configure project name, resolution preset, and target framework before creating a new project |
| **Undo / redo commands** | `src/commands/AddComponentCommand.{h,cpp}`<br>`src/commands/DeleteComponentCommand.{h,cpp}`<br>`src/commands/MoveComponentCommand.{h,cpp}`<br>`src/commands/ResizeComponentCommand.{h,cpp}`<br>`src/commands/PropertyChangeCommand.{h,cpp}`<br>`src/commands/AlignDistributeCommand.{h,cpp}`<br>`src/commands/BooleanPathCommand.{h,cpp}` | Seven `QUndoCommand` subclasses for reversible add, delete, move, resize, property, align/distribute, and Boolean-path operations |
| **Component schema & introspection** | `src/models/ComponentSchema.{h,cpp}` | Machine-readable schema registry, property reflection, validation, and JSON metadata exporter for all component types |
| **Universal hardware model** | `src/hardware/HardwareModel.{h,cpp}` | Data structures and JSON serialization for `PinDefinition`, `PeripheralDescriptor`, `DeviceDefinition`, `BoardDefinition`, `PinConfiguration`, `PeripheralConfiguration`, `ClockConfiguration`, `ToolchainConfiguration`, `HardwareConfig` |
| **Pin multiplexing & conflict engine** | `src/hardware/PinMuxEngine.{h,cpp}` | Conflict detection engine enforcing power/ground/reset/reserved pin protection, alternate function validation, and peripheral pin routing |
| **Device & board database / packs** | `src/hardware/DeviceDatabase.{h,cpp}`, `src/hardware/HardwareProvider.{h,cpp}`, `data/hardware_packs/` | Pluggable device provider architecture indexing STM32, ESP32, RP2040, Raspberry Pi, and Custom hardware packs with import/export |
| **Hardware project wizard** | `src/dialogs/HardwareWizardDialog.{h,cpp}` | 6-step desktop project creation wizard (Hardware Selector, Pinout & Conflict Dialog, Peripherals, Clocks/Memory, Toolchains, Review) |
| **Visual pinout viewer** | `src/hardware/PinoutViewWidget.{h,cpp}` | High-performance QPainter widget rendering physical packages (LQFP/QFN) and dual-row board headers with pin status color coding |
| **Designer controller (API)** | `src/api/DesignerController.{h,cpp}` | Application-level controller exposing canvas, component, selection, hardware, pinmux, and export APIs directly dispatching via QUndoStack commands |
| **Local control server (TCP)** | `src/api/DesignerLocalServer.{h,cpp}` | Embedded Qt TCP server (port 8765) receiving JSON-RPC commands and dispatching them to DesignerController on the main thread |
| **MCP server (AI interface)** | `mcp/embedded_ui_mcp.py` | Stdio JSON-RPC Model Context Protocol server exposing 48 tools and 8 resources to Claude, Antigravity, and AI agents |
| **Application shell** | `src/MainWindow.{h,cpp}` | Top-level `QMainWindow`; creates and wires all subsystems, menus, toolbars, and dock panels; dispatches File / Edit / View / Export / Hardware menu actions |
| **Entry point** | `src/main.cpp` | Creates `QApplication` and `MainWindow`; applies global dark theme |
| **Docker dev environment** | `Dockerfile` + `docker-compose.yml` | Ubuntu 22.04 image installing Qt 6.5.3 via `aqtinstall` with X11 forwarding so the GUI can be developed inside Docker |

---

## Third-Party Dependencies

| Dependency | Purpose | Integration Method | Note |
|------------|---------|-------------------|------|
| **Qt 6** (6.5+) | Application framework (Core, Gui, Widgets, Network, Test, optional SerialPort, Quick/Qml) | System package / `find_package` | Primary GUI and canvas rendering engine |
| **Clipper2** | Boolean 2D polygon operations (Union, Difference, Intersection, XOR) | CMake `FetchContent` | Statically linked into `EmbeddedUIDesigner` and test runners |
| **SDL2** (Simple DirectMedia Layer 2) | Desktop windowing, event dispatch, and accelerated rendering driver for exported LVGL PC simulators | System package (`find_package` / `pkg_check_modules`) with automatic `FetchContent` fallback | Required for compiling and running the exported `pc_simulator/` target for LVGL projects |

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

