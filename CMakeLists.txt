cmake_minimum_required(VERSION 3.20)
project(EmbeddedUIDesigner 
    VERSION 1.0.0 
    DESCRIPTION "Desktop UI Designer & Code Generator for Embedded Microcontrollers"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Qt-specific CMake automatic tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Required Qt6 packages
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Test
    Network
)
find_package(Qt6 6.5 OPTIONAL_COMPONENTS
    SerialPort
)

# ── Clipper2 (Boolean path operations) ────────────────────────────────────
# Fetched at configure time; provides Clipper2Lib::Clipper2 target.
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
# Suppress compiler warnings on third-party Clipper2 targets
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

# Common Source files (excluding main.cpp)
set(COMMON_SOURCES
    src/MainWindow.cpp
    src/MainWindow.h

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

    # Display Configuration
    src/models/DisplayConfig.h

    # Asset Compilation Pipeline
    src/assets/ImageAssetProcessor.cpp
    src/assets/ImageAssetProcessor.h

    # Canvas UIComponent Hierarchy
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
    src/commands/MoveComponentCommand.cpp
    src/commands/MoveComponentCommand.h
    src/commands/ResizeComponentCommand.cpp
    src/commands/ResizeComponentCommand.h
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
    src/hardware/BoardConfigParser.cpp
    src/hardware/BoardConfigParser.h
    src/hardware/DeviceManager.cpp
    src/hardware/DeviceManager.h
    src/hardware/FlashController.cpp
    src/hardware/FlashController.h
    src/hardware/OpenOcdManager.cpp
    src/hardware/OpenOcdManager.h

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
    src/models/ColorStyle.h

    src/project/Project.cpp
    src/project/Project.h
    src/project/QmlImporter.cpp
    src/project/QmlImporter.h

    # Component Schema System
    src/models/ComponentSchema.cpp
    src/models/ComponentSchema.h

    # Live MCP & Controller API
    src/api/DesignerController.cpp
    src/api/DesignerController.h
    src/api/DesignerLocalServer.cpp
    src/api/DesignerLocalServer.h

    # Multi-Provider AI Platform Subsystem
    src/ai/AIProvider.cpp
    src/ai/AIProvider.h
    src/ai/AIProviderRegistry.cpp
    src/ai/AIProviderRegistry.h
    src/ai/AIProjectContext.cpp
    src/ai/AIProjectContext.h
    src/ai/AIToolRegistry.cpp
    src/ai/AIToolRegistry.h
    src/ai/providers/MockAIProvider.cpp
    src/ai/providers/MockAIProvider.h
    src/ai/providers/OpenAIProvider.cpp
    src/ai/providers/OpenAIProvider.h
    src/ai/providers/AnthropicProvider.cpp
    src/ai/providers/AnthropicProvider.h
    src/ai/providers/GeminiProvider.cpp
    src/ai/providers/GeminiProvider.h
    src/ai/providers/CustomOpenAICompatibleProvider.cpp
    src/ai/providers/CustomOpenAICompatibleProvider.h
    src/dialogs/AISettingsDialog.cpp
    src/dialogs/AISettingsDialog.h
)

set(SOURCES
    src/main.cpp
    ${COMMON_SOURCES}
)

# Qt Resource manifests (.qrc)
set(RESOURCES
    resources/app.qrc
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ai/providers
)

# Target Link Libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Clipper2
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

# ============================================================================
# Automated Testing Suite
# ============================================================================
option(BUILD_TESTING "Build the testing tree." ON)
if(BUILD_TESTING)
    enable_testing()

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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ai/providers
)

target_link_libraries(test_all_cases PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    Qt6::Network
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ai/providers
)

target_link_libraries(test_functional_runner PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ai
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ai/providers
)

target_link_libraries(test_live_nucleo PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Clipper2
)

if(Qt6SerialPort_FOUND)
    target_link_libraries(test_live_nucleo PRIVATE Qt6::SerialPort)
    target_compile_definitions(test_live_nucleo PRIVATE HAVE_QT_SERIALPORT)
endif()

set_target_properties(test_live_nucleo PROPERTIES WIN32_EXECUTABLE FALSE)
endif() # BUILD_TESTING


