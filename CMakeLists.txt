cmake_minimum_required(VERSION 3.20)
project(EmbeddedUIDesigner 
    VERSION 1.1.0
    DESCRIPTION "Desktop UI Designer & Code Generator for Embedded Microcontrollers"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform and Architecture Auto-Detection
if(NOT DEFINED PLATFORM OR PLATFORM STREQUAL "auto")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(PLATFORM "Linux")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(PLATFORM "macOS")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(PLATFORM "Windows")
    else()
        set(PLATFORM "Embedded")
    endif()
endif()

if(NOT DEFINED ARCH OR ARCH STREQUAL "auto")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        set(ARCH "x86_64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i[3-6]86|x86")
        set(ARCH "x86")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
        set(ARCH "ARM64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
        set(ARCH "ARM")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "riscv")
        set(ARCH "RISC-V")
    else()
        set(ARCH "${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()

message(STATUS "Embedded UI Designer: Platform=${PLATFORM}, Architecture=${ARCH}")

# Build Configuration Options
option(ENABLE_TESTS "Build acceptance test suite (102 tests)" ON)
option(ENABLE_SECURITY_HARDENING "Enable security hardening" ON)
option(ENABLE_OBSERVABILITY "Enable Prometheus/ELK observability" ON)
option(BUILD_FOR_EMBEDDED "Cross-compile for embedded target" OFF)

# Global model and core include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
)
# JsonCpp library detection with FetchContent fallback
if(FORCE_FETCH_JSONCPP OR (APPLE AND CMAKE_OSX_ARCHITECTURES MATCHES "x86_64" AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm|ARM"))
    set(SKIP_SYSTEM_JSONCPP TRUE)
else()
    set(SKIP_SYSTEM_JSONCPP FALSE)
endif()

if(NOT SKIP_SYSTEM_JSONCPP)
    find_package(jsoncpp QUIET)
    if(NOT jsoncpp_FOUND)
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(JSONCPP QUIET jsoncpp)
        endif()
    endif()
endif()

if(TARGET jsoncpp)
    set(JSONCPP_TARGET jsoncpp)
elseif(TARGET jsoncpp_lib)
    set(JSONCPP_TARGET jsoncpp_lib)
elseif(jsoncpp_FOUND)
    set(JSONCPP_TARGET jsoncpp)
elseif(JSONCPP_FOUND)
    include_directories(${JSONCPP_INCLUDE_DIRS})
    link_directories(${JSONCPP_LIBRARY_DIRS})
    set(JSONCPP_TARGET ${JSONCPP_LIBRARIES})
else()
    include(FetchContent)
    FetchContent_Declare(
        jsoncpp
        GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
        GIT_TAG        1.9.5
    )
    set(JSONCPP_WITH_TESTS OFF CACHE BOOL "" FORCE)
    set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(jsoncpp)
    set(JSONCPP_TARGET jsoncpp_lib)
endif()

link_libraries(${JSONCPP_TARGET})

# Optional Qt6 detection for Desktop GUI and Qt unit tests
find_package(Qt6 6.5 QUIET COMPONENTS
    Core
    Gui
    Widgets
    Test
    Network
    Xml
)

if(Qt6_FOUND)
    message(STATUS "Qt6 6.5 found - Desktop GUI application and widget tests enabled")
    set(HAVE_QT6 TRUE)
    find_package(Qt6 6.5 OPTIONAL_COMPONENTS SerialPort)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    # ── Clipper2 (Boolean path operations) ────────────────────────────────────
    include(FetchContent)
    FetchContent_Declare(
        Clipper2
        GIT_REPOSITORY https://github.com/AngusJohnson/Clipper2.git
        GIT_TAG        Clipper2_1.3.0
        SOURCE_SUBDIR  CPP
    )
    set(CLIPPER2_UTILS OFF CACHE BOOL "" FORCE)
    set(CLIPPER2_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(CLIPPER2_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(Clipper2)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        foreach(_clipper_tgt IN ITEMS Clipper2 Clipper2Z)
            if(TARGET ${_clipper_tgt})
                target_compile_options(${_clipper_tgt} PRIVATE
                    -Wno-error -Wno-pedantic -Wno-template-id-cdtor)
            endif()
        endforeach()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        foreach(_clipper_tgt IN ITEMS Clipper2 Clipper2Z)
            if(TARGET ${_clipper_tgt})
                target_compile_options(${_clipper_tgt} PRIVATE
                    -Wno-error -Wno-pedantic)
            endif()
        endforeach()
    endif()
else()
    message(STATUS "Qt6 6.5 NOT found - Desktop GUI disabled. Building headless acceptance test suite.")
    set(HAVE_QT6 FALSE)
endif()


# Common Source files (excluding main.cpp)
set(COMMON_SOURCES
    src/MainWindow.cpp
    src/MainWindow.h

    # Workspace & Code IDE Subsystem
    src/workspace/WorkspaceMode.h
    src/workspace/WorkspaceManager.cpp
    src/workspace/WorkspaceManager.h
    src/workspace/DesignWorkspace.cpp
    src/workspace/DesignWorkspace.h
    src/workspace/CodeWorkspace.cpp
    src/workspace/CodeWorkspace.h
    src/workspace/CodeEditor.cpp
    src/workspace/CodeEditor.h
    src/workspace/CppSyntaxHighlighter.cpp
    src/workspace/CppSyntaxHighlighter.h
    src/workspace/TabbedCodeEditor.cpp
    src/workspace/TabbedCodeEditor.h
    src/workspace/ProjectExplorerWidget.cpp
    src/workspace/ProjectExplorerWidget.h
    src/workspace/ProblemsWidget.cpp
    src/workspace/ProblemsWidget.h
    src/workspace/TerminalWidget.cpp
    src/workspace/TerminalWidget.h
    src/workspace/SerialMonitorWidget.cpp
    src/workspace/SerialMonitorWidget.h
    src/workspace/CodeWorkspaceView.cpp
    src/workspace/CodeWorkspaceView.h
    src/workspace/DetachableTabWidget.cpp
    src/workspace/DetachableTabWidget.h

    # Universal Board Detection & Specifications (espboards.dev)
    src/hardware/BoardDatabase.cpp
    src/hardware/BoardDatabase.h
    src/hardware/BoardDetector.cpp
    src/hardware/BoardDetector.h
    src/ui/dialogs/BoardSelectorDialog.cpp
    src/ui/dialogs/BoardSelectorDialog.h

    # LVGL Component Library & Code Generation
    src/components/ComponentLibrary.cpp
    src/components/ComponentLibrary.h
    src/components/codegen/LVGLCodeGenerator.cpp
    src/components/codegen/LVGLCodeGenerator.h
    src/components/lvgl/LVGLWidgets.h
    src/components/lvgl/LVGLButton.h
    src/components/lvgl/LVGLLabel.h
    src/components/lvgl/LVGLSlider.h
    src/components/lvgl/LVGLSwitch.h
    src/components/lvgl/LVGLContainer.h
    src/components/lvgl/LVGLGauge.h
    src/components/lvgl/LVGLChart.h
    src/components/lvgl/LVGLSpinbox.h
    src/components/lvgl/LVGLKeyboard.h

    src/dialogs/FlashDialog.cpp
    src/dialogs/FlashDialog.h
    src/dialogs/SerialMonitorDialog.cpp
    src/dialogs/SerialMonitorDialog.h
    src/dialogs/PinBindingDialog.cpp
    src/dialogs/PinBindingDialog.h
    src/dialogs/NewProjectDialog.cpp
    src/dialogs/NewProjectDialog.h
    src/dialogs/HardwareWizardDialog.cpp
    src/dialogs/HardwareWizardDialog.h
    src/dialogs/DriverDiffPreviewDialog.cpp
    src/dialogs/DriverDiffPreviewDialog.h
    src/dialogs/InvalidBindingsDialog.cpp
    src/dialogs/InvalidBindingsDialog.h
    src/dialogs/ExportDialog.cpp
    src/dialogs/ExportDialog.h

    # Resource Analyzer
    src/analyzer/ResourceAnalyzer.cpp
    src/analyzer/ResourceAnalyzer.h

    # Track 2: Build System Integration
    src/build/BuildConfiguration.cpp
    src/build/BuildConfiguration.h
    src/build/BuildRunner.cpp
    src/build/BuildRunner.h
    src/build/DiagnosticParser.cpp
    src/build/DiagnosticParser.h
    src/panels/BuildOutputPanel.cpp
    src/panels/BuildOutputPanel.h

    # Track 3: Firmware Export
    src/codegen/FirmwareExportManager.cpp
    src/codegen/FirmwareExportManager.h

    # Track 4: Hardware Device Monitor with Live Telemetry
    src/telemetry/TelemetryProtocol.cpp
    src/telemetry/TelemetryProtocol.h
    src/telemetry/DeviceMonitorRuntime.cpp
    src/telemetry/DeviceMonitorRuntime.h
    src/panels/DeviceMonitorPanel.cpp
    src/panels/DeviceMonitorPanel.h

    # Display Configuration
    src/models/DisplayConfig.h

    # Asset Compilation Pipeline
    src/assets/ImageAssetProcessor.cpp
    src/assets/ImageAssetProcessor.h

    # Canvas UIComponent Hierarchy
    src/models/ComponentGradient.cpp
    src/models/ComponentGradient.h
    src/models/UIComponent.cpp
    src/models/UIComponent.h
    src/models/ValueVisualizationComponent.cpp
    src/models/ValueVisualizationComponent.h
    src/models/ButtonComponent.cpp
    src/models/ButtonComponent.h
    src/models/LabelComponent.cpp
    src/models/LabelComponent.h
    src/models/RectangleComponent.cpp
    src/models/RectangleComponent.h
    src/models/ProgressBarComponent.cpp
    src/models/ProgressBarComponent.h
    src/models/CircularProgressComponent.cpp
    src/models/CircularProgressComponent.h
    src/models/GaugeComponent.cpp
    src/models/GaugeComponent.h
    src/models/SpeedometerComponent.cpp
    src/models/SpeedometerComponent.h
    src/models/BatteryComponent.cpp
    src/models/BatteryComponent.h
    src/models/PressureComponent.cpp
    src/models/PressureComponent.h
    src/models/RpmComponent.cpp
    src/models/RpmComponent.h
    src/models/TemperatureComponent.cpp
    src/models/TemperatureComponent.h
    src/models/TabViewComponent.cpp
    src/models/TabViewComponent.h
    src/models/NavigationBarComponent.cpp
    src/models/NavigationBarComponent.h
    src/models/ListComponent.cpp
    src/models/ListComponent.h
    src/models/TableComponent.cpp
    src/models/TableComponent.h
    src/models/ImageComponent.cpp
    src/models/ImageComponent.h
    src/models/SliderComponent.cpp
    src/models/SliderComponent.h
    src/models/SwitchComponent.cpp
    src/models/SwitchComponent.h
    src/models/CheckboxComponent.cpp
    src/models/CheckboxComponent.h
    src/models/TextInputComponent.cpp
    src/models/TextInputComponent.h
    src/models/CircleComponent.cpp
    src/models/CircleComponent.h
    src/models/GroupComponent.cpp
    src/models/GroupComponent.h
    src/models/PathComponent.cpp
    src/models/PathComponent.h
    src/models/ComponentDefinition.cpp
    src/models/ComponentDefinition.h
    src/models/CustomComponentInstance.cpp
    src/models/CustomComponentInstance.h
    src/models/Screen.cpp
    src/models/Screen.h
    src/models/DataSource.cpp
    src/models/DataSource.h
    src/models/DataBinding.cpp
    src/models/DataBinding.h
    src/models/ComponentState.cpp
    src/models/ComponentState.h

    # Command Pattern (Undo/Redo System)
    src/commands/AddComponentCommand.cpp
    src/commands/AddComponentCommand.h
    src/commands/DeleteComponentCommand.cpp
    src/commands/DeleteComponentCommand.h
    src/commands/GroupCommand.cpp
    src/commands/GroupCommand.h
    src/commands/MoveComponentCommand.cpp
    src/commands/MoveComponentCommand.h
    src/commands/ResizeComponentCommand.cpp
    src/commands/ResizeComponentCommand.h
    src/commands/RotateComponentCommand.cpp
    src/commands/RotateComponentCommand.h
    src/commands/PropertyChangeCommand.cpp
    src/commands/PropertyChangeCommand.h
    src/commands/AlignDistributeCommand.cpp
    src/commands/AlignDistributeCommand.h
    src/commands/BooleanPathCommand.cpp
    src/commands/BooleanPathCommand.h
    src/commands/ScreenCommands.cpp
    src/commands/ScreenCommands.h


    # Code Generators (Phase 4 Code Generator v2)
    src/codegen/CodeGenerator.cpp
    src/codegen/CodeGenerator.h
    src/codegen/GeneratorContext.cpp
    src/codegen/GeneratorContext.h
    src/codegen/GeneratorIR.cpp
    src/codegen/GeneratorIR.h
    src/codegen/TargetHalGenerator.cpp
    src/codegen/TargetHalGenerator.h
    src/codegen/BindingLayerGenerator.cpp
    src/codegen/BindingLayerGenerator.h
    src/codegen/QtMcuGenerator.cpp
    src/codegen/QtMcuGenerator.h
    src/codegen/UgfxGenerator.cpp
    src/codegen/UgfxGenerator.h
    src/codegen/LvglGenerator.cpp
    src/codegen/LvglGenerator.h
    src/codegen/EmbedWebGenerator.cpp
    src/codegen/EmbedWebGenerator.h
    src/codegen/BackendRegistry.cpp
    src/codegen/BackendRegistry.h

    # EmbedWeb Runtime
    src/runtime/ew_tree.c
    src/runtime/ew_tree.h
    src/runtime/ew_layout.c
    src/runtime/ew_layout.h
    src/runtime/ew_raster.c
    src/runtime/ew_raster.h
    src/runtime/ew_hal.c
    src/runtime/ew_hal.h
    src/runtime/ew_resource.c
    src/runtime/ew_resource.h
    src/runtime/ew_input.c
    src/runtime/ew_input.h
    src/runtime/ew_hydrate.cpp
    src/runtime/ew_hydrate.h

    # Desktop Simulator Subsystem (Phase 5)
    src/simulator/SimulationBackend.cpp
    src/simulator/SimulationBackend.h
    src/simulator/SimulationRuntime.cpp
    src/simulator/SimulationRuntime.h
    src/simulator/SimulationCanvasView.cpp
    src/simulator/SimulationCanvasView.h
    src/simulator/SimulationControlPanel.cpp
    src/simulator/SimulationControlPanel.h
    src/simulator/SimulatorWindow.cpp
    src/simulator/SimulatorWindow.h

    # Hardware Bridge & Flashing Subsystem
    src/hardware/HardwareBridge.cpp
    src/hardware/HardwareBridge.h
    src/hardware/BoardRegistry.cpp
    src/hardware/BoardRegistry.h
    src/hardware/BoardConfigParser.cpp
    src/hardware/BoardConfigParser.h
    src/hardware/DeviceManager.cpp
    src/hardware/DeviceManager.h
    src/hardware/FlashController.cpp
    src/hardware/FlashController.h
    src/hardware/OpenOcdManager.cpp
    src/hardware/OpenOcdManager.h
    src/hardware/OpenOCDManager.cpp
    src/hardware/OpenOCDManager.h
    src/hardware/OpenOCDWorker.cpp
    src/hardware/OpenOCDWorker.h
    src/hardware/BoardHAL.h
    src/hardware/BoardFactory.cpp
    src/hardware/BoardFactory.h
    src/hardware/STM32HAL.cpp
    src/hardware/STM32HAL.h
    src/hardware/ESP32HAL.cpp
    src/hardware/ESP32HAL.h

    # Universal Hardware Target & Database Subsystem
    src/hardware/HardwareCapabilities.h
    src/hardware/HardwareTarget.cpp
    src/hardware/HardwareTarget.h
    src/hardware/HardwareConnection.cpp
    src/hardware/HardwareConnection.h
    src/hardware/HardwareBackend.h
    src/hardware/STM32Backend.cpp
    src/hardware/STM32Backend.h
    src/hardware/ESP32Backend.cpp
    src/hardware/ESP32Backend.h
    src/hardware/RaspberryPiBackend.cpp
    src/hardware/RaspberryPiBackend.h
    src/hardware/MockBackend.cpp
    src/hardware/MockBackend.h
    src/hardware/HardwareManager.cpp
    src/hardware/HardwareManager.h
    src/hardware/HardwareModel.cpp
    src/hardware/HardwareModel.h
    src/hardware/PinMuxEngine.cpp
    src/hardware/PinMuxEngine.h
    src/hardware/HardwareProvider.cpp
    src/hardware/HardwareProvider.h
    src/hardware/DeviceDatabase.cpp
    src/hardware/DeviceDatabase.h
    src/hardware/PinoutViewWidget.cpp
    src/hardware/PinoutViewWidget.h

    # Panels & Visual Views
    src/canvas/CanvasView.cpp
    src/canvas/CanvasView.h
    src/canvas/CanvasScene.cpp
    src/canvas/CanvasScene.h
    src/panels/ComponentPalette.cpp
    src/panels/ComponentPalette.h
    src/panels/PropertiesPanel.cpp
    src/panels/PropertiesPanel.h
    src/panels/PrototypePanel.cpp
    src/panels/PrototypePanel.h
    src/panels/ColorPickerDialog.cpp
    src/panels/ColorPickerDialog.h
    src/panels/LayerPanel.cpp
    src/panels/LayerPanel.h
    src/panels/StylesPanel.cpp
    src/panels/StylesPanel.h
    src/panels/ScreensPanel.cpp
    src/panels/ScreensPanel.h
    src/panels/ContextualTopToolbar.cpp
    src/panels/ContextualTopToolbar.h
    src/panels/FloatingSelectionToolbar.cpp
    src/panels/FloatingSelectionToolbar.h
    src/panels/EffectsPanel.cpp
    src/panels/EffectsPanel.h
    src/panels/UploadsPanel.cpp
    src/panels/UploadsPanel.h
    src/models/ColorStyle.h

    # Resource Analyzer & Device Capability Engine (Phase 2.1)
    src/analyzer/ResourceAnalysis.h
    src/analyzer/EstimationFormulas.cpp
    src/analyzer/EstimationFormulas.h
    src/analyzer/EstimationCache.h
    src/analyzer/CapabilityDatabase.cpp
    src/analyzer/CapabilityDatabase.h
    src/analyzer/ResourceEstimator.cpp
    src/analyzer/ResourceEstimator.h
    src/panels/ResourceAnalyzerPanel.cpp
    src/panels/ResourceAnalyzerPanel.h
    src/dialogs/ExportValidationDialog.cpp
    src/dialogs/ExportValidationDialog.h

    # Diagnostics Service & Problems Panel (Task 3)
    src/diagnostics/DiagnosticsService.cpp
    src/diagnostics/DiagnosticsService.h
    src/diagnostics/DiagnosticsSources.cpp
    src/diagnostics/DiagnosticsSources.h
    src/panels/ProblemsPanel.cpp
    src/panels/ProblemsPanel.h

    # Board & Display dock (Task 5)
    src/models/BoardDisplayConfig.h
    src/analyzer/FramebufferFit.h
    src/panels/BoardDisplayPanel.cpp
    src/panels/BoardDisplayPanel.h

    # Unified Device Adapter (Phase 0)
    src/analyzer/UnifiedDeviceAdapter.cpp
    src/analyzer/UnifiedDeviceAdapter.h

    # Phase 3: Universal IR & Importers
    src/ir/IRTypes.cpp
    src/ir/IRTypes.h
    src/ir/IRSerialization.cpp
    src/ir/IRSerialization.h
    src/ir/IRAdapter.cpp
    src/ir/IRAdapter.h
    src/importers/HtmlCssImporter.cpp
    src/importers/HtmlCssImporter.h
    src/importers/HtmlCssParser.cpp
    src/importers/HtmlCssParser.h

    src/project/Project.cpp
    src/project/Project.h
    src/project/QmlImporter.cpp
    src/project/QmlImporter.h

    # EmbeddedUI Unified Project Model & API Schema
    src/model/ProjectModel.cpp
    src/model/ProjectModel.h
    src/model/TypeSystem.cpp
    src/model/TypeSystem.h
    src/model/ProjectSerializer.cpp
    src/model/ProjectSerializer.h
    src/codegen/CodegenEngine.cpp
    src/codegen/CodegenEngine.h

    # Security Hardening & Performance Infrastructure (Weeks 9-10)
    src/security/SecurityConfig.cpp
    src/security/SecurityConfig.h
    src/security/AuthenticationMiddleware.cpp
    src/security/AuthenticationMiddleware.h
    src/security/RateLimiter.cpp
    src/security/RateLimiter.h
    src/security/AuditLogger.cpp
    src/security/AuditLogger.h
    src/security/FirmwareUpdateValidator.cpp
    src/security/FirmwareUpdateValidator.h
    src/performance/ConnectionPool.cpp
    src/performance/ConnectionPool.h
    src/performance/RequestBatcher.cpp
    src/performance/RequestBatcher.h
    src/performance/ResponseCache.cpp
    src/performance/ResponseCache.h
    src/performance/PerformanceMetrics.cpp
    src/performance/PerformanceMetrics.h

    # Component Schema System
    src/models/ComponentSchema.cpp
    src/models/ComponentSchema.h

    # Live MCP & Controller API
    src/api/DesignerController.cpp
    src/api/DesignerController.h
    src/api/DesignerLocalServer.cpp
    src/api/DesignerLocalServer.h

    # Multi-Provider AI Platform Subsystem
    src/analyzer/ai/AIProvider.cpp
    src/analyzer/ai/AIProvider.h
    src/analyzer/ai/AIProviderRegistry.cpp
    src/analyzer/ai/AIProviderRegistry.h
    src/analyzer/ai/AIProjectContext.cpp
    src/analyzer/ai/AIProjectContext.h
    src/analyzer/ai/AIToolRegistry.cpp
    src/analyzer/ai/AIToolRegistry.h
    src/analyzer/ai/providers/MockAIProvider.cpp
    src/analyzer/ai/providers/MockAIProvider.h
    src/analyzer/ai/providers/OpenAIProvider.cpp
    src/analyzer/ai/providers/OpenAIProvider.h
    src/analyzer/ai/providers/AnthropicProvider.cpp
    src/analyzer/ai/providers/AnthropicProvider.h
    src/analyzer/ai/providers/GeminiProvider.cpp
    src/analyzer/ai/providers/GeminiProvider.h
    src/analyzer/ai/providers/CustomOpenAICompatibleProvider.cpp
    src/analyzer/ai/providers/CustomOpenAICompatibleProvider.h
    src/dialogs/AISettingsDialog.cpp
    src/dialogs/AISettingsDialog.h
    src/dialogs/AIProposedChangesDialog.cpp
    src/dialogs/AIProposedChangesDialog.h
    src/panels/AIAssistantPanel.cpp
    src/panels/AIAssistantPanel.h
    src/analyzer/ai/CredentialManager.cpp
    src/analyzer/ai/CredentialManager.h
    src/models/DiagnosticsEngine.cpp
    src/models/DiagnosticsEngine.h
    src/git/GitService.cpp
    src/git/GitService.h
    src/dialogs/GitDialog.cpp
    src/dialogs/GitDialog.h
    src/cicd/CICDGenerator.cpp
    src/cicd/CICDGenerator.h
    src/dialogs/CICDDialog.cpp
    src/dialogs/CICDDialog.h
    src/plugins/PluginRegistry.cpp
    src/plugins/PluginRegistry.h
    src/styles/StyleManager.cpp
    src/styles/StyleManager.h
    src/theme/Theme.cpp
    src/theme/Theme.h
)

set(SOURCES
    src/main.cpp
    ${COMMON_SOURCES}
)

if(HAVE_QT6)
# Qt Resource manifests (.qrc)
set(RESOURCES
    resources/app.qrc
    resources/styles.qrc
    resources.qrc
)

# Platform-specific icon resources
if(WIN32)
    set(PLATFORM_SOURCES packaging/app.rc)
elseif(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE embedded-ui-designer.icns)
    set(PLATFORM_SOURCES packaging/embedded-ui-designer.icns)
    set_source_files_properties(packaging/embedded-ui-designer.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
endif()

# Define double-click desktop app (WIN32 hides console on Windows, MACOSX_BUNDLE builds .app on macOS)
if(COMMAND qt_add_executable)
    qt_add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
        ${SOURCES}
        ${RESOURCES}
        ${PLATFORM_SOURCES}
    )
else()
    add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE
        ${SOURCES}
        ${RESOURCES}
        ${PLATFORM_SOURCES}
    )
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "EmbeddedUIDesigner"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.embeddeddev.embeddeduidesigner"
    MACOSX_BUNDLE_BUNDLE_NAME "EmbeddedUIDesigner"
    MACOSX_BUNDLE_DISPLAY_NAME "Embedded UI Designer"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
)

# Target Include Directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/src/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/project
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/git
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cicd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ir
    ${CMAKE_CURRENT_SOURCE_DIR}/src/importers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/styles
    ${CMAKE_CURRENT_SOURCE_DIR}/src/workspace
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/lvgl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
)

# Target Link Libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Xml
    Clipper2
    jsoncpp
)

if(Qt6SerialPort_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::SerialPort)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_QT_SERIALPORT)
endif()

# Compiler warnings and options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Installation targets for packaging
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# CPack Packaging Configuration
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "EmbeddedUIDesigner")
set(CPACK_PACKAGE_VENDOR "Embedded UI Development Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Visual UI Designer for Microcontrollers (µGFX & Qt for MCUs)")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "ZIP;TGZ")
else()
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Embedded UI Team <support@example.com>")
endif()

include(CPack)
endif() # HAVE_QT6

# ============================================================================
# Automated Testing Suite
# ============================================================================
option(BUILD_TESTING "Build the testing tree." ON)
if(BUILD_TESTING)
    enable_testing()

if(HAVE_QT6)

if(COMMAND qt_add_executable)
    qt_add_executable(test_all_cases
        tests/test_all_cases.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
else()
    add_executable(test_all_cases
        tests/test_all_cases.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
endif()

target_include_directories(test_all_cases PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/src/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/project
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai/providers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/git
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cicd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry
    ${CMAKE_CURRENT_SOURCE_DIR}/src/workspace
)

target_link_libraries(test_all_cases PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    Qt6::Network
    Qt6::Xml
    Clipper2
)

if(Qt6SerialPort_FOUND)
    target_link_libraries(test_all_cases PRIVATE Qt6::SerialPort)
    target_compile_definitions(test_all_cases PRIVATE HAVE_QT_SERIALPORT)
endif()

set_target_properties(test_all_cases PROPERTIES WIN32_EXECUTABLE FALSE)

add_test(NAME AllCases COMMAND test_all_cases)

# Functional Validation Runner
if(COMMAND qt_add_executable)
    qt_add_executable(test_functional_runner
        tests/test_functional_runner.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
else()
    add_executable(test_functional_runner
        tests/test_functional_runner.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
endif()

target_include_directories(test_functional_runner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/src/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/project
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai/providers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/git
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cicd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/workspace
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/lvgl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dialogs
)

target_link_libraries(test_functional_runner PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Xml
    Clipper2
)

if(Qt6SerialPort_FOUND)
    target_link_libraries(test_functional_runner PRIVATE Qt6::SerialPort)
    target_compile_definitions(test_functional_runner PRIVATE HAVE_QT_SERIALPORT)
endif()

set_target_properties(test_functional_runner PROPERTIES WIN32_EXECUTABLE FALSE)

# Live Hardware & Nucleo Detection Test Runner
if(COMMAND qt_add_executable)
    qt_add_executable(test_live_nucleo
        tests/test_live_nucleo.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
else()
    add_executable(test_live_nucleo
        tests/test_live_nucleo.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
endif()

target_include_directories(test_live_nucleo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/src/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/project
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai/providers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/git
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cicd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer
)

target_link_libraries(test_live_nucleo PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Xml
    Clipper2
)

if(Qt6SerialPort_FOUND)
    target_link_libraries(test_live_nucleo PRIVATE Qt6::SerialPort)
    target_compile_definitions(test_live_nucleo PRIVATE HAVE_QT_SERIALPORT)
endif()

set_target_properties(test_live_nucleo PROPERTIES WIN32_EXECUTABLE FALSE)

# Multi-Board Hardware Abstraction Layer Test Target
if(COMMAND qt_add_executable)
    qt_add_executable(test_board_hal
        tests/test_board_hal.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
else()
    add_executable(test_board_hal
        tests/test_board_hal.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
endif()

target_include_directories(test_board_hal PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/src/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/project
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/git
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cicd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer
)

target_link_libraries(test_board_hal PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Xml
    Clipper2
)

if(Qt6SerialPort_FOUND)
    target_link_libraries(test_board_hal PRIVATE Qt6::SerialPort)
    target_compile_definitions(test_board_hal PRIVATE HAVE_QT_SERIALPORT)
endif()

set_target_properties(test_board_hal PROPERTIES WIN32_EXECUTABLE FALSE)
add_test(NAME test_board_hal COMMAND test_board_hal)


# Hardening Milestone Unit & Integration Test Suite
if(COMMAND qt_add_executable)
    qt_add_executable(test_hardening
        tests/test_hardening.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
else()
    add_executable(test_hardening
        tests/test_hardening.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
endif()

target_include_directories(test_hardening PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/src/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/project
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai/providers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/build
    ${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry
)

target_link_libraries(test_hardening PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    Qt6::Network
    Qt6::Xml
    Clipper2
)

if(Qt6SerialPort_FOUND)
    target_link_libraries(test_hardening PRIVATE Qt6::SerialPort)
    target_compile_definitions(test_hardening PRIVATE HAVE_QT_SERIALPORT)
endif()

set_target_properties(test_hardening PROPERTIES WIN32_EXECUTABLE FALSE)

add_test(NAME Hardening COMMAND test_hardening)

# Phase 3 Unit Tests: IR Serialization
if(COMMAND qt_add_executable)
    qt_add_executable(test_ir_serialization
        tests/test_ir_serialization.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
else()
    add_executable(test_ir_serialization
        tests/test_ir_serialization.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
endif()

target_include_directories(test_ir_serialization PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ir
    ${CMAKE_CURRENT_SOURCE_DIR}/src/importers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/src/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/project
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai/providers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/git
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cicd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry
)

target_link_libraries(test_ir_serialization PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    Qt6::Network
    Qt6::Xml
    Clipper2
)
if(Qt6SerialPort_FOUND)
    target_link_libraries(test_ir_serialization PRIVATE Qt6::SerialPort)
endif()
set_target_properties(test_ir_serialization PROPERTIES WIN32_EXECUTABLE FALSE)
add_test(NAME IRSerialization COMMAND test_ir_serialization)

# Phase 3 Unit Tests: IR Adapter
if(COMMAND qt_add_executable)
    qt_add_executable(test_ir_adapter
        tests/test_ir_adapter.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
else()
    add_executable(test_ir_adapter
        tests/test_ir_adapter.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
endif()

target_include_directories(test_ir_adapter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ir
    ${CMAKE_CURRENT_SOURCE_DIR}/src/importers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/src/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/project
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai/providers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/git
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cicd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry
)

target_link_libraries(test_ir_adapter PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    Qt6::Network
    Qt6::Xml
    Clipper2
)
if(Qt6SerialPort_FOUND)
    target_link_libraries(test_ir_adapter PRIVATE Qt6::SerialPort)
endif()
set_target_properties(test_ir_adapter PROPERTIES WIN32_EXECUTABLE FALSE)
add_test(NAME IRAdapter COMMAND test_ir_adapter)

# Phase 3 Unit Tests: HTML/CSS Importer
if(COMMAND qt_add_executable)
    qt_add_executable(test_html_css_importer
        tests/test_html_css_importer.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
else()
    add_executable(test_html_css_importer
        tests/test_html_css_importer.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
endif()

target_include_directories(test_html_css_importer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ir
    ${CMAKE_CURRENT_SOURCE_DIR}/src/importers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/src/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/project
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai/providers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/git
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cicd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry
)

target_link_libraries(test_html_css_importer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    Qt6::Network
    Qt6::Xml
    Clipper2
)
if(Qt6SerialPort_FOUND)
    target_link_libraries(test_html_css_importer PRIVATE Qt6::SerialPort)
endif()
set_target_properties(test_html_css_importer PROPERTIES WIN32_EXECUTABLE FALSE)
add_test(NAME HtmlCssImporter COMMAND test_html_css_importer)

# Stage 4.1-4.5 Unit Tests: Isolated EmbedWeb Runtime Prototype & Hydration & Multi-MCU Adapters
if(COMMAND qt_add_executable)
    qt_add_executable(test_embedweb_runtime
        tests/test_embedweb_runtime.cpp
        src/runtime/ew_tree.c
        src/runtime/ew_layout.c
        src/runtime/ew_raster.c
        src/runtime/ew_hal.c
        src/runtime/ew_resource.c
        src/runtime/ew_input.c
        src/runtime/ew_hydrate.cpp
        src/runtime/adapters/ew_adapter_host.c
        src/runtime/adapters/ew_adapter_stm32.c
        src/runtime/adapters/ew_adapter_rp2040.c
        src/runtime/adapters/ew_adapter_esp32.c
        src/runtime/adapters/ew_adapter_nxp.c
        src/ir/IRTypes.cpp
        src/ir/IRSerialization.cpp
    )
else()
    add_executable(test_embedweb_runtime
        tests/test_embedweb_runtime.cpp
        src/runtime/ew_tree.c
        src/runtime/ew_layout.c
        src/runtime/ew_raster.c
        src/runtime/ew_hal.c
        src/runtime/ew_resource.c
        src/runtime/ew_input.c
        src/runtime/ew_hydrate.cpp
        src/runtime/adapters/ew_adapter_host.c
        src/runtime/adapters/ew_adapter_stm32.c
        src/runtime/adapters/ew_adapter_rp2040.c
        src/runtime/adapters/ew_adapter_esp32.c
        src/runtime/adapters/ew_adapter_nxp.c
        src/ir/IRTypes.cpp
        src/ir/IRSerialization.cpp
    )
endif()

target_include_directories(test_embedweb_runtime PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/adapters
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ir
)

target_link_libraries(test_embedweb_runtime PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)
set_target_properties(test_embedweb_runtime PROPERTIES WIN32_EXECUTABLE FALSE)
add_test(NAME EmbedWebRuntime COMMAND test_embedweb_runtime)
endif() # HAVE_QT6

# Standalone EmbedWeb Desktop Simulator Harness (Stage 4.4)
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(SDL2 QUIET sdl2)
endif()
if(NOT SDL2_FOUND)
    find_package(SDL2 QUIET)
endif()

add_executable(embedweb_simulator
    src/runtime/simulator/embedweb_simulator.c
    src/runtime/ew_tree.c
    src/runtime/ew_layout.c
    src/runtime/ew_raster.c
    src/runtime/ew_hal.c
    src/runtime/ew_resource.c
    src/runtime/ew_input.c
)

target_include_directories(embedweb_simulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime
)

if(SDL2_FOUND)
    target_compile_definitions(embedweb_simulator PRIVATE HAS_SDL2)
    if(TARGET SDL2::SDL2)
        target_link_libraries(embedweb_simulator PRIVATE SDL2::SDL2)
    elseif(SDL2_LIBRARIES)
        target_link_libraries(embedweb_simulator PRIVATE ${SDL2_LIBRARIES})
        target_include_directories(embedweb_simulator PRIVATE ${SDL2_INCLUDE_DIRS})
    endif()
    message(STATUS "EmbedWeb Simulator: SDL2 support enabled")
else()
    message(STATUS "EmbedWeb Simulator: SDL2 not found, building in headless mode")
endif()

set_target_properties(embedweb_simulator PROPERTIES WIN32_EXECUTABLE FALSE)
add_test(NAME EmbedWebSimulatorHarness COMMAND embedweb_simulator --test-interaction)

# Work Stream 1 & 2: Project Model & Frontend API Integration Tests
add_executable(test_project_model
    tests/test_project_model.cpp
    src/model/ProjectModel.cpp
    src/model/ProjectModel.h
    src/model/TypeSystem.cpp
    src/model/TypeSystem.h
    src/model/ProjectSerializer.cpp
    src/model/ProjectSerializer.h
)
target_include_directories(test_project_model PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
)
target_link_libraries(test_project_model PRIVATE
    ${JSONCPP_TARGET}
)
set_target_properties(test_project_model PROPERTIES WIN32_EXECUTABLE FALSE)
add_test(NAME ProjectModelAcceptanceTests COMMAND test_project_model)

# Work Stream 1, 2 & 3: Multi-Language Code Generators Tests (C / C++ / Rust)
add_executable(test_codegen_multi_language
    tests/test_codegen_multi_language.cpp
    src/codegen/CodegenEngine.cpp
    src/codegen/CodegenEngine.h
    src/model/ProjectModel.cpp
    src/model/ProjectModel.h
    src/model/TypeSystem.cpp
    src/model/TypeSystem.h
    src/model/ProjectSerializer.cpp
    src/model/ProjectSerializer.h
)
target_include_directories(test_codegen_multi_language PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen
)
target_link_libraries(test_codegen_multi_language PRIVATE
    ${JSONCPP_TARGET}
)
set_target_properties(test_codegen_multi_language PROPERTIES WIN32_EXECUTABLE FALSE)
add_test(NAME MultiLanguageCodegenAcceptanceTests COMMAND test_codegen_multi_language)

# Work Stream 1, 2 & 3: Frontend Integration & Verification Tests (Weeks 7-8)
find_program(NODE_EXECUTABLE node)
if(NODE_EXECUTABLE)
    add_test(NAME FrontendIntegrationAcceptanceTests COMMAND ${NODE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/src/frontend/tests/integration_runner.js)
endif()

# Work Stream 1, 2 & 3: Security Hardening & Performance Optimization Tests (Weeks 9-10)
add_executable(test_security_performance
    tests/test_security_performance.cpp
    src/security/SecurityConfig.cpp
    src/security/SecurityConfig.h
    src/security/AuthenticationMiddleware.cpp
    src/security/AuthenticationMiddleware.h
    src/security/RateLimiter.cpp
    src/security/RateLimiter.h
    src/security/AuditLogger.cpp
    src/security/AuditLogger.h
    src/security/FirmwareUpdateValidator.cpp
    src/security/FirmwareUpdateValidator.h
    src/performance/ConnectionPool.cpp
    src/performance/ConnectionPool.h
    src/performance/RequestBatcher.cpp
    src/performance/RequestBatcher.h
    src/performance/ResponseCache.cpp
    src/performance/ResponseCache.h
    src/performance/PerformanceMetrics.cpp
    src/performance/PerformanceMetrics.h
)
target_include_directories(test_security_performance PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_target_properties(test_security_performance PROPERTIES WIN32_EXECUTABLE FALSE)
add_test(NAME SecurityPerformanceAcceptanceTests COMMAND test_security_performance)

# Work Stream 1, 2 & 3: Documentation, CI/CD, Compliance & Observability Tests (Weeks 11-12)
add_executable(test_compliance_observability
    tests/test_compliance_observability.cpp
    src/documentation/DocumentationGenerator.cpp
    src/documentation/DocumentationGenerator.h
    src/documentation/OpenAPISpecBuilder.cpp
    src/documentation/OpenAPISpecBuilder.h
    src/compliance/ComplianceValidator.cpp
    src/compliance/ComplianceValidator.h
    src/observability/ObservabilityCollector.cpp
    src/observability/ObservabilityCollector.h
)
target_include_directories(test_compliance_observability PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_target_properties(test_compliance_observability PROPERTIES WIN32_EXECUTABLE FALSE)
add_test(NAME DocumentationComplianceAcceptanceTests COMMAND test_compliance_observability)

# MCU-Specific Driver Library and Capability-Aware Project Structure Tests
add_executable(test_mcu_driver_packs
    tests/test_mcu_driver_packs.cpp
    src/driverpack/DriverDependency.cpp
    src/driverpack/DriverDependency.h
    src/driverpack/Stm32DriverPack.cpp
    src/driverpack/Stm32DriverPack.h
    src/driverpack/Esp32DriverPack.cpp
    src/driverpack/Esp32DriverPack.h
    src/driverpack/Rp2040DriverPack.cpp
    src/driverpack/Rp2040DriverPack.h
    src/driverpack/NxpDriverPack.cpp
    src/driverpack/NxpDriverPack.h
    src/driverpack/NordicDriverPack.cpp
    src/driverpack/NordicDriverPack.h
    src/driverpack/DriverPackRegistry.cpp
    src/driverpack/DriverPackRegistry.h
    src/driverpack/McuProjectGenerator.cpp
    src/driverpack/McuProjectGenerator.h
)
target_include_directories(test_mcu_driver_packs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_target_properties(test_mcu_driver_packs PROPERTIES WIN32_EXECUTABLE FALSE)
add_test(NAME McuDriverPackAcceptanceTests COMMAND test_mcu_driver_packs)

set_tests_properties(
    EmbedWebSimulatorHarness
    ProjectModelAcceptanceTests
    MultiLanguageCodegenAcceptanceTests
    FrontendIntegrationAcceptanceTests
    SecurityPerformanceAcceptanceTests
    DocumentationComplianceAcceptanceTests
    McuDriverPackAcceptanceTests
    PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Workspace Widgets (Embedded Terminal, Serial Monitor, Code Workspace View) Tests
if(HAVE_QT6)
if(COMMAND qt_add_executable)
    qt_add_executable(test_workspace_widgets
        tests/test_workspace_widgets.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
else()
    add_executable(test_workspace_widgets
        tests/test_workspace_widgets.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
endif()

target_include_directories(test_workspace_widgets PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/workspace
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/src/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/project
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/git
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cicd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ir
    ${CMAKE_CURRENT_SOURCE_DIR}/src/importers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/styles
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/lvgl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
)

target_link_libraries(test_workspace_widgets PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    Qt6::Network
    Qt6::Xml
    Clipper2
    ${JSONCPP_TARGET}
)

if(Qt6SerialPort_FOUND)
    target_link_libraries(test_workspace_widgets PRIVATE Qt6::SerialPort)
    target_compile_definitions(test_workspace_widgets PRIVATE HAVE_QT_SERIALPORT)
endif()

set_target_properties(test_workspace_widgets PROPERTIES WIN32_EXECUTABLE FALSE)
add_test(NAME WorkspaceWidgetsAcceptanceTests COMMAND test_workspace_widgets)

# Task 1 Export Validation Integration Acceptance Tests
if(COMMAND qt_add_executable)
    qt_add_executable(test_export_validation
        tests/test_export_validation.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
else()
    add_executable(test_export_validation
        tests/test_export_validation.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
endif()

target_include_directories(test_export_validation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/workspace
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/src/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/project
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/git
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cicd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ir
    ${CMAKE_CURRENT_SOURCE_DIR}/src/importers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/styles
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/lvgl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
)

target_link_libraries(test_export_validation PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    Qt6::Network
    Qt6::Xml
    Clipper2
    ${JSONCPP_TARGET}
)

if(Qt6SerialPort_FOUND)
    target_link_libraries(test_export_validation PRIVATE Qt6::SerialPort)
    target_compile_definitions(test_export_validation PRIVATE HAVE_QT_SERIALPORT)
endif()

set_target_properties(test_export_validation PROPERTIES WIN32_EXECUTABLE FALSE)
add_test(NAME ExportValidationAcceptanceTests COMMAND test_export_validation)

# Task 2 Resource Analyzer + Live Test Integration Acceptance Tests
if(COMMAND qt_add_executable)
    qt_add_executable(test_live_test_resources
        tests/test_live_test_resources.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
else()
    add_executable(test_live_test_resources
        tests/test_live_test_resources.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
endif()

target_include_directories(test_live_test_resources PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/workspace
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/src/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/project
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/git
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cicd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ir
    ${CMAKE_CURRENT_SOURCE_DIR}/src/importers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/styles
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/lvgl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
)

target_link_libraries(test_live_test_resources PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    Qt6::Network
    Qt6::Xml
    Clipper2
    ${JSONCPP_TARGET}
)

if(Qt6SerialPort_FOUND)
    target_link_libraries(test_live_test_resources PRIVATE Qt6::SerialPort)
    target_compile_definitions(test_live_test_resources PRIVATE HAVE_QT_SERIALPORT)
endif()

set_target_properties(test_live_test_resources PROPERTIES WIN32_EXECUTABLE FALSE)
add_test(NAME LiveTestResourceAnalyzerTests COMMAND test_live_test_resources)

# Task 3 Diagnostics Service + Problems Panel Acceptance Tests
if(COMMAND qt_add_executable)
    qt_add_executable(test_problems_diagnostics
        tests/test_problems_diagnostics.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
else()
    add_executable(test_problems_diagnostics
        tests/test_problems_diagnostics.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
endif()

target_include_directories(test_problems_diagnostics PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/workspace
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/src/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/project
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/git
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cicd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ir
    ${CMAKE_CURRENT_SOURCE_DIR}/src/importers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/styles
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/lvgl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
)

target_link_libraries(test_problems_diagnostics PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    Qt6::Network
    Qt6::Xml
    Clipper2
    ${JSONCPP_TARGET}
)

if(Qt6SerialPort_FOUND)
    target_link_libraries(test_problems_diagnostics PRIVATE Qt6::SerialPort)
    target_compile_definitions(test_problems_diagnostics PRIVATE HAVE_QT_SERIALPORT)
endif()

set_target_properties(test_problems_diagnostics PROPERTIES WIN32_EXECUTABLE FALSE)
add_test(NAME ProblemsDiagnosticsAcceptanceTests COMMAND test_problems_diagnostics)

# Task 4 MCU Capability Database Acceptance Tests
if(COMMAND qt_add_executable)
    qt_add_executable(test_mcu_capabilities
        tests/test_mcu_capabilities.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
else()
    add_executable(test_mcu_capabilities
        tests/test_mcu_capabilities.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
endif()

target_include_directories(test_mcu_capabilities PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/workspace
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/src/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/project
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/git
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cicd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ir
    ${CMAKE_CURRENT_SOURCE_DIR}/src/importers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/styles
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/lvgl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
)

target_link_libraries(test_mcu_capabilities PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    Qt6::Network
    Qt6::Xml
    Clipper2
    ${JSONCPP_TARGET}
)

if(Qt6SerialPort_FOUND)
    target_link_libraries(test_mcu_capabilities PRIVATE Qt6::SerialPort)
    target_compile_definitions(test_mcu_capabilities PRIVATE HAVE_QT_SERIALPORT)
endif()

set_target_properties(test_mcu_capabilities PROPERTIES WIN32_EXECUTABLE FALSE)
add_test(NAME McuCapabilityDatabaseTests COMMAND test_mcu_capabilities)

# Task 5 Board & Display Dock Acceptance Tests
if(COMMAND qt_add_executable)
    qt_add_executable(test_board_display_dock
        tests/test_board_display_dock.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
else()
    add_executable(test_board_display_dock
        tests/test_board_display_dock.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
endif()

target_include_directories(test_board_display_dock PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/workspace
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/src/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/project
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/git
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cicd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ir
    ${CMAKE_CURRENT_SOURCE_DIR}/src/importers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/styles
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/lvgl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
)

target_link_libraries(test_board_display_dock PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    Qt6::Network
    Qt6::Xml
    Clipper2
    ${JSONCPP_TARGET}
)

if(Qt6SerialPort_FOUND)
    target_link_libraries(test_board_display_dock PRIVATE Qt6::SerialPort)
    target_compile_definitions(test_board_display_dock PRIVATE HAVE_QT_SERIALPORT)
endif()

set_target_properties(test_board_display_dock PROPERTIES WIN32_EXECUTABLE FALSE)
add_test(NAME BoardDisplayDockAcceptanceTests COMMAND test_board_display_dock)

# Task 6 Target Backend selector + widget support matrix acceptance tests
if(COMMAND qt_add_executable)
    qt_add_executable(test_target_backend
        tests/test_target_backend.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
else()
    add_executable(test_target_backend
        tests/test_target_backend.cpp
        ${COMMON_SOURCES}
        ${RESOURCES}
    )
endif()

target_include_directories(test_target_backend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/workspace
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canvas
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hardware
    ${CMAKE_CURRENT_SOURCE_DIR}/src/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/project
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/simulator
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/git
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cicd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins
    ${CMAKE_CURRENT_SOURCE_DIR}/src/analyzer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/telemetry
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ir
    ${CMAKE_CURRENT_SOURCE_DIR}/src/importers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/styles
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/codegen
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/lvgl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
)

target_link_libraries(test_target_backend PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    Qt6::Network
    Qt6::Xml
    Clipper2
    ${JSONCPP_TARGET}
)

if(Qt6SerialPort_FOUND)
    target_link_libraries(test_target_backend PRIVATE Qt6::SerialPort)
    target_compile_definitions(test_target_backend PRIVATE HAVE_QT_SERIALPORT)
endif()

set_target_properties(test_target_backend PROPERTIES WIN32_EXECUTABLE FALSE)
add_test(NAME TargetBackendAcceptanceTests COMMAND test_target_backend)
endif() # HAVE_QT6


endif() # BUILD_TESTING
