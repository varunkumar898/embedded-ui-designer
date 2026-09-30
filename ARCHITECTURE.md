# Architecture Guide

This document maps all features of the Embedded UI Designer to their exact implementation file paths and purposes.

> **Standing Rule:** Before touching any code, consult this table to identify the specific file(s) needed. Read and edit **only** those files. If this table does not clearly point to the right file for a feature, ask explicitly rather than scanning broadly. If a task adds, renames, or removes a file, update this table in the same commit.

## Feature Map

| Feature | Exact File Path(s) | One-line Purpose |
| :--- | :--- | :--- |
| **Application Entry & Shell** | `src/main.cpp`<br>`src/MainWindow.h`<br>`src/MainWindow.cpp` | Initializes Qt application, sets up main window layout, menus, docks, and connects canvas/panel actions. |
| **Canvas & Selection** | `src/canvas/CanvasScene.h`<br>`src/canvas/CanvasScene.cpp`<br>`src/canvas/CanvasView.h`<br>`src/canvas/CanvasView.cpp` | Handles canvas viewport rendering, grid snap, zoom/pan, rubber-band multi-select, and item selection state synchronization. |
| **Properties Panel** | `src/panels/PropertiesPanel.h`<br>`src/panels/PropertiesPanel.cpp` | Displays single-component inspector or multi-component selection summary ("N components selected") without widget overlap. |
| **Layers Panel** | `src/panels/LayerPanel.h`<br>`src/panels/LayerPanel.cpp` | Manages visual hierarchy, z-ordering, component visibility, and layer tree selection sync with the canvas. |
| **Styles Panel / Color Picking** | `src/panels/PropertiesPanel.h`<br>`src/panels/PropertiesPanel.cpp`<br>`src/panels/ColorPickerDialog.h`<br>`src/panels/ColorPickerDialog.cpp` | Provides visual styling controls (colors, borders, corner radius, fonts, opacity) and color picker dialog with presets. |
| **Component Palette** | `src/panels/ComponentPalette.h`<br>`src/panels/ComponentPalette.cpp` | Provides draggable palette of embedded UI widgets and controls for dropping onto the canvas. |
| **Custom Components (Base)** | `src/models/UIComponent.h`<br>`src/models/UIComponent.cpp` | Base QGraphicsItem class defining geometry, selection/resize handles, JSON serialization, and rendering hooks. |
| **Custom Components (Models)** | `src/models/ButtonComponent.h` / `.cpp`<br>`src/models/CheckboxComponent.h` / `.cpp`<br>`src/models/CircleComponent.h` / `.cpp`<br>`src/models/ImageComponent.h` / `.cpp`<br>`src/models/LabelComponent.h` / `.cpp`<br>`src/models/ProgressBarComponent.h` / `.cpp`<br>`src/models/RectangleComponent.h` / `.cpp`<br>`src/models/SliderComponent.h` / `.cpp`<br>`src/models/SwitchComponent.h` / `.cpp`<br>`src/models/TextInputComponent.h` / `.cpp` | Concrete component implementations providing model properties, custom canvas rendering, and default dimensions. |
| **Display Configuration** | `src/models/DisplayConfig.h` | Defines target embedded display resolution, color format/depth, and screen orientation. |
| **Commands & Undo/Redo** | `src/commands/AddComponentCommand.h` / `.cpp`<br>`src/commands/DeleteComponentCommand.h` / `.cpp`<br>`src/commands/MoveComponentCommand.h` / `.cpp`<br>`src/commands/ResizeComponentCommand.h` / `.cpp`<br>`src/commands/PropertyChangeCommand.h` / `.cpp`<br>`src/commands/AlignDistributeCommand.h` / `.cpp` | QUndoCommand implementations providing reversible actions, including compound atomic undo for multi-item align and distribute. |
| **Align & Distribute Actions** | `src/MainWindow.h`<br>`src/MainWindow.cpp`<br>`src/commands/AlignDistributeCommand.h`<br>`src/commands/AlignDistributeCommand.cpp` | Computes selection bounding box alignment and equal-gap distribution for 2+ and 3+ selected components. |
| **Project Save / Load** | `src/project/Project.h`<br>`src/project/Project.cpp` | Serializes and deserializes project metadata, display configuration, and component hierarchy to/from `.euiproj` JSON. |
| **Code Generator (Base)** | `src/codegen/CodeGenerator.h`<br>`src/codegen/CodeGenerator.cpp` | Abstract base class defining code generator interface, source formatting, and file export workflow. |
| **Code Generator (LVGL)** | `src/codegen/LvglGenerator.h`<br>`src/codegen/LvglGenerator.cpp` | Generates C source and header files using the LVGL (Light and Versatile Graphics Library) API. |
| **Code Generator (Qt for MCUs / QUL)** | `src/codegen/QtMcuGenerator.h`<br>`src/codegen/QtMcuGenerator.cpp` | Generates QML/C++ projects targeting Qt for MCUs (QUL) runtime. |
| **Code Generator (uGFX)** | `src/codegen/UgfxGenerator.h`<br>`src/codegen/UgfxGenerator.cpp` | Generates C initialization and layout routines targeting the uGFX embedded library. |
| **Device Manager & Hardware** | `src/hardware/DeviceManager.h`<br>`src/hardware/DeviceManager.cpp`<br>`src/hardware/FlashController.h`<br>`src/hardware/FlashController.cpp` | Manages target microcontroller profiles (ESP32, STM32, RP2040) and coordinates binary firmware flashing via CLI tools. |
| **Image Asset Processing** | `src/assets/ImageAssetProcessor.h`<br>`src/assets/ImageAssetProcessor.cpp` | Converts graphical assets (PNG/JPEG) into C array pixel buffers and palette-indexed formats for embedded displays. |
| **Docker Dev Setup** | `Dockerfile`<br>`docker-compose.yml`<br>`docs/DOCKER_DEV.md` | Provides containerized build and execution environment with Qt6, Ninja, CMake, and X11 forwarding support. |
