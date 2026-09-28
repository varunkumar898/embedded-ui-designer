# Prompt & Technical Requirements: Embedded UI Designer

## Target Tech Stack
- Language: C++17
- Framework: Qt 6.5+ LTS (QtWidgets, QtGui, QtCore, QtQml/Quick)
- Build System: CMake 3.20+
- Serialization: Qt JSON (`QJsonDocument`) / `nlohmann/json`
- CI/CD: GitHub Actions (Windows MSVC, Linux GCC, macOS Clang)

## Phase 1 MVP Scope
1. **Interactive Canvas (`CanvasView` & `CanvasScene`)**:
   - Canvas surface with customizable target display resolution (320x240, 480x320, 800x480, custom).
   - Component drag-and-drop, selection handles, bounding-box resizing, interactive moving.
   - Grid overlay with configurable snap-to-grid (e.g., 5px, 10px).
   - Canvas zoom (25% - 400%) and smooth panning.
2. **Component Model (`UIComponent`)**:
   - `Button`: text, background color, text color, corner radius, onClicked signal stub.
   - `Label` (`Text`): text content, font family, pixel size, bold/italic, color, alignment.
   - `Rectangle`: fill color, stroke color, stroke width, border radius.
   - `ProgressBar`: range (0.0 to 1.0 or custom), fill color, background color.
   - `TextInput`: placeholder, border styling, font settings.
   - `Image`: source path, scaling mode, opacity.
3. **Panels**:
   - `ComponentPalette`: Drag source or click-to-add for components.
   - `PropertiesPanel`: Real-time bi-directional synchronization with selected component.
   - `LayerPanel`: Tree view showing z-order, visibility toggling, rename, delete.
4. **Project Management (`Project`)**:
   - Save/Load `.euiproj` JSON.
   - Target display configuration (resolution, color depth, target type).
   - Auto-save timer and recent files history.
5. **Code Generator (`QtMcuGenerator`)**:
   - Generates CMakeLists.txt, main.cpp, MainWindow.h/cpp, MainScreen.qml, and README.md.
