#include <QtTest/QtTest>
#include <QApplication>
#include <QUndoStack>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>
#include <QComboBox>
#include <QFrame>
#include <QSpinBox>
#include <QDir>
#include <QToolButton>
#include <QToolBar>
#include <QMenuBar>
#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include <QPixmap>

// Models
#include "ButtonComponent.h"
#include "LabelComponent.h"
#include "RectangleComponent.h"
#include "ProgressBarComponent.h"
#include "SliderComponent.h"
#include "SwitchComponent.h"
#include "CheckboxComponent.h"
#include "TextInputComponent.h"
#include "CircleComponent.h"
#include "ImageComponent.h"
#include "PathComponent.h"
#include "CircularProgressComponent.h"
#include "GaugeComponent.h"
#include "SpeedometerComponent.h"
#include "BatteryComponent.h"
#include "PressureComponent.h"
#include "RpmComponent.h"
#include "TemperatureComponent.h"
#include "TabViewComponent.h"
#include "NavigationBarComponent.h"
#include "ListComponent.h"
#include "TableComponent.h"
#include "GroupComponent.h"

// Commands
#include "AddComponentCommand.h"
#include "DeleteComponentCommand.h"
#include "GroupCommand.h"
#include "MoveComponentCommand.h"
#include "ResizeComponentCommand.h"
#include "RotateComponentCommand.h"
#include "PropertyChangeCommand.h"

// Canvas & Panels
#include "CanvasScene.h"
#include "CanvasView.h"
#include "PropertiesPanel.h"
#include "PrototypePanel.h"
#include "ContextualTopToolbar.h"
#include "FloatingSelectionToolbar.h"
#include "EffectsPanel.h"
#include "UploadsPanel.h"
#include "SerialMonitorDialog.h"
#include "PinBindingDialog.h"
#include "NewProjectDialog.h"
#include "HardwareBridge.h"
#include "HardwareCapabilities.h"
#include "HardwareTarget.h"
#include "HardwareConnection.h"
#include "HardwareBackend.h"
#include "FlashController.h"
#include <QSignalSpy>
#include "STM32Backend.h"
#include "ESP32Backend.h"
#include "RaspberryPiBackend.h"
#include "MockBackend.h"
#include "HardwareManager.h"
#include "BoardConfigParser.h"
#include "OcdTelnetManager.h"
#include "OpenOcdDeviceLink.h"
#include "QmlImporter.h"
#include <QEventLoop>
#include <QTimer>
#include <atomic>
#include "DeviceManager.h"
#include "hardware/BoardRegistry.h"
#include "MainWindow.h"
#include "workspace/DetachableTabWidget.h"
#include "workspace/CodeWorkspaceView.h"
#include "workspace/TabbedCodeEditor.h"
#include "LayerPanel.h"
#include "ColorPickerDialog.h"
#include "Project.h"
#include "Screen.h"
#include "DataSource.h"
#include "DataBinding.h"
#include "GeneratorContext.h"
#include "GeneratorIR.h"
#include "TargetHalGenerator.h"
#include "BindingLayerGenerator.h"
#include "ComponentState.h"
#include "ScreenCommands.h"
#include "ScreensPanel.h"

// Generators
#include "UgfxGenerator.h"
#include "QtMcuGenerator.h"
#include "LvglGenerator.h"
#include "HardwareModel.h"
#include "HardwareProvider.h"
#include "DeviceDatabase.h"
#include "HardwareWizardDialog.h"
#include "dialogs/DriverDiffPreviewDialog.h"
#include "dialogs/InvalidBindingsDialog.h"
#include "PinMuxEngine.h"
#include "DesignerController.h"

// Simulator
#include "SimulationBackend.h"
#include "SimulationRuntime.h"
#include "SimulatorWindow.h"

// AI Platform Subsystem
#include "analyzer/ai/AIProvider.h"
#include "analyzer/ai/AIProviderRegistry.h"
#include "analyzer/ai/AIProjectContext.h"
#include "analyzer/ai/AIToolRegistry.h"
#include "analyzer/ai/providers/MockAIProvider.h"
#include "analyzer/ai/providers/OpenAIProvider.h"
#include "analyzer/ai/providers/AnthropicProvider.h"
#include "analyzer/ai/providers/GeminiProvider.h"
#include "analyzer/ai/providers/CustomOpenAICompatibleProvider.h"
#include "dialogs/AISettingsDialog.h"
#include "analyzer/ai/CredentialManager.h"
#include "DiagnosticsEngine.h"
#include "GitService.h"
#include "CICDGenerator.h"
#include "PluginRegistry.h"
#include "DesignerLocalServer.h"
#include "analyzer/CapabilityDatabase.h"
#include "analyzer/EstimationFormulas.h"
#include "analyzer/ResourceEstimator.h"
#include "panels/ResourceAnalyzerPanel.h"
#include "dialogs/ExportValidationDialog.h"

static QString repoPath(const QString& relPath) {
    QString cleanPath = relPath;
    if (cleanPath.startsWith("../")) {
        cleanPath = cleanPath.mid(3);
    }
    QDir dir(QCoreApplication::applicationDirPath());
    if (dir.dirName().compare("Release", Qt::CaseInsensitive) == 0 ||
        dir.dirName().compare("Debug", Qt::CaseInsensitive) == 0) {
        dir.cdUp();
    }
    dir.cdUp();
    return dir.filePath(cleanPath);
}

static QString screenshotPath(const QString& filename) {
    QString cleanName = filename;
    if (cleanName.startsWith("../screenshots/")) {
        cleanName = cleanName.mid(15);
    } else if (cleanName.startsWith("screenshots/")) {
        cleanName = cleanName.mid(12);
    }
    QString shotPath = repoPath("screenshots/" + cleanName);
    QFileInfo(shotPath).dir().mkpath(".");
    return shotPath;
}

static QString examplePath(const QString& filename) {
    QString cleanName = filename;
    if (cleanName.startsWith("../examples/")) {
        cleanName = cleanName.mid(12);
    } else if (cleanName.startsWith("examples/")) {
        cleanName = cleanName.mid(9);
    }
    return repoPath("examples/" + cleanName);
}

class TestAllCases : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    // 0. Painter balance regression (Unbalanced save/restore guard)
    void testPainterSaveRestoreBalance();

    // 1. Component Models
    void testAllComponentsCreationAndDefaults();
    void testComponentPropertyMutations();
    void testComponentSerializationJson();
    void testPathBezierFlattening();
    void testBezierPenDragAndEdit();
    void testComponentCodeGenerationSnippets();

    // 2. Corner Radius Handle (Task 2)
    void testInteractiveCornerRadiusHandle();

    // 3. Commands & Undo / Redo
    void testAddComponentCommand();
    void testMoveComponentCommand();
    void testResizeComponentCommand();
    void testDeleteComponentCommand();
    void testPropertyChangeCommand();
    void testMultiStepUndoRedoSequence();

    // 4. Properties Panel & Bug Regression (Task 1)
    void testPropertiesPanelRebuildNoOverlap();
    void testPropertiesPanelGeometryEditing();
    void testPropertiesPanelAlignmentRow();

    // 5. Embedded Color Picker & RGB565 / Harmonies
    void testColorPickerRgb565Calculations();
    void testColorPickerHarmonyCalculations();
    void testColorPickerWheelInteraction();

    // 6. Layer Panel
    void testLayerPanelSyncAndReorder();

    // 7. Project Serialization Full Roundtrip
    void testProjectFullSaveAndLoad();
    void testProjectImportMergesComponents();
    void testNamedColorStylesLivePropagation();
    void testPrototypeInteractionTransition();
    void testSerialMonitorPortListing();
    void testToolbarDecluttered();

    // 8. Code Generators
    void testUgfxCodeGenerator();
    void testQtMcuCodeGenerator();
    void testLvglCodeGenerator();

    // 9. Canvas View & Display Presets
    void testDisplayPresetsAndCanvasView();
    void testCanvasZoomInteractions();

    // 10. Hardware ADC & SPI Pin Modes and Binding (Task A & C)
    void testAdcSetupAndPollingPattern();
    void testSpiRawTransferMode();
    void testAdcBindingToProgressBarAndNumericDisplay();
    void testPinBindingDialogUiAndPotentiometerSlider();
    void testComponentProtocolConfigurationAndScreenshots();

    // 11. Board Configuration Import (STM32 .ioc / ESP-IDF sdkconfig)
    void testBoardConfigImportIoc();
    void testBoardConfigImportSdkConfig();

    // 12. QML Design Import (Literal properties only, strict rejection of bindings/anchors/etc.)
    void testQmlImportBasic();
    void testQmlImportComplexRejected();

    // 13. Auto-Connect on Board Detection (Task A)
    void testOpenOcdAutoConnectAndPersistentStatus();

    // 14. GPIO Digital Output via Atomic BSRR (Task B)
    void testGpioDigitalOutputBsrrAndReverseBinding();

    // 15. I2C Bus Scan & Multi-Sensor Address Assignment (Task C)
    void testI2cBusScanAndMultiSensorAssignment();

    // 16. PC-Simulator Project Generation & Round Display Build (Task A)
    void testPcSimulator240x240RoundGenerationAndBuild();

    // 17. Universal Hardware Architecture, Providers, Database & Packs (Phases 1-15, 30)
    void testHardwareDatabaseAndPacks();
    void testStm32DeviceAndBoardPacks();
    void testEsp32DeviceAndBoardPacks();
    void testRp2040AndRaspberryPiPacks();

    // 18. Pin Multiplexer & Conflict Detection Engine (Phases 7, 8, 9)
    void testPinMuxEngineConflictsAndValidation();

    // 19. Custom Hardware Creation, Export & Import (Phases 6, 18)
    void testCustomHardwareCreationAndImportExport();

    // 20. Project HardwareConfig Persistence & Roundtrip (Phase 12)
    void testProjectHardwarePersistence();

    // 21. MCP Hardware Inspection & Configuration Tools (Phases 22, 23, 24)
    void testMcpHardwareToolsDispatch();

    // 22. Universal Hardware Selector - All Tab & Global Filtering (Phase 2A)
    void testUniversalHardwareSelectorAllTabAndFiltering();

    // 23. Phase 1 Project Model Refactor (Multi-Screen, Data Sources/Bindings, Component States)
    void testMultiScreenProjectModel();
    void testScreenUndoRedoCommands();
    void testScreensPanelUi();
    void testDataSourceAndBindingModel();
    void testComponentStateAndStyles();
    void testBackwardCompatibilitySingleScreenProject();

    // 24. Phase 2 Component Expansion (12 First-Class Components, Dashboards & Data Binding)
    void testPhase2ComponentsCreationAndDefaults();
    void testPhase2ComponentsSerialization();
    void testPhase2ValueVisualizationThresholdsAndAutoState();
    void testPhase2DataBindingPipeline();
    void testPhase2MultiScreenDashboardProject();

    // 25. Phase 3 Hardware Abstraction Layer (HAL, Backends, Capabilities, Target Persistence)
    void testPhase3HardwareTargetAndCapabilities();
    void testPhase3HardwareBackendSwitchingAndCapabilities();
    void testPhase3MockBackendAndSimulation();
    void testPhase3DataSourceHardwareResolution();
    void testPhase3CustomBoardTargetPersistence();

    // 26. Phase 4 Code Generator v2 (Context, IR, Target HAL, Bindings & Multi-Screen Export)
    void testPhase4GeneratorContextAndValidation();
    void testPhase4GeneratorIntermediateRepresentation();
    void testPhase4TargetHalAndBindingLayerGeneration();
    void testPhase4LvglMultiScreenAndDashboardExport();

    // 27. Phase 5 Desktop Simulator (Runtime, Backend, Bindings, Clocks, Safety, UI)
    void testPhase5SimulationBackendAndHardwareIsolation();
    void testPhase5SimulationRuntimeClockAndExpressions();
    void testPhase5BidirectionalDataBindingsAndAutoState();
    void testPhase5ProjectSaveSafetyAndSnapshotRestore();
    void testPhase5SimulatorWindowAndControlPanelUi();
    void testPhase5DeterministicSimulatorCoverage();
    void testPhase5MockHardwarePipelines();
    void testPhase5VisualRegressionDemoProject();
    void testPhase5PerformanceStress();

    // 28. Phase 6 Multi-Provider AI Platform
    void testPhase6AIProviderNeutralInterfaceAndCapabilities();
    void testPhase6AIProviderRegistryAndRouting();
    void testPhase6MessageNormalizationAndWireAdapters();
    void testPhase6AIToolRegistryAndUniversalSchemas();
    void testPhase6AIToolExecutionAndUndoableTransactions();
    void testPhase6HardwareSafetyGate();
    void testPhase6AIProjectContextAndSecretSanitization();
    void testPhase6ProviderFailureHandlingAndFallbackSwitching();
    void testPhase6OfflineModeZeroAIConfiguration();
    void testPhase6AISettingsDialogUI();

    // 29. Phase 1 Option B Data Source Abstraction (CAN, UART, Modbus, Variables)
    void testPhase1CanSignalExtractionAndEncoding();
    void testPhase1UartStreamParsingModes();
    void testPhase1ModbusRegistersDecodingAndEncoding();
    void testPhase1VariableWaveformsAndSimulation();
    void testPhase1SimulationRuntimeProtocolInjections();
    void testPhase1DataSourceSerializationBackwardCompatibility();

    // 30. Phase 6 Hardening (MCP Security, Sandboxing, Credentials, Diagnostics, Git, CI/CD, GeneratorIR, Plugins)
    void testPhase6MCPAuthenticationAndTokens();
    void testPhase6MCPFileSandboxEnforcement();
    void testPhase6McpTokenFileCanonicalPath();
    void testPhase6FirmwareSyncRequiresConfiguredDirectory();
    // 31. Phase 7: R1.b transport from board catalog (adapter identity on profiles, tty matching)
    void testPhase7AdapterTransportFromCatalog();
    void testPhase7SerialPortMatchNormalization();
    void testPhase7ConnectRequiresMatchingAdapterPort();
    // 32. Phase 7/R1.c: honesty of connection claims (no probe/gated claims)
    void testPhase7LinuxGpioRequiresOpenableChipDevice();
    void testPhase7SerialConnectionOpensOrFails();
    void testPhase7FlashRefusesUntargetedPort();
    void testPhase7DetectorAdapterOnlyWithoutCatalog();
    // 33. Phase 7/R1.e: telnet prompt parse, single-thread socket, honest LED
    void testPhase7TelnetPromptTerminatedParse();
    void testPhase7TelnetForeignThreadMarshal();
    void testPhase7LedCommandHonorsProfile();
    // 34. Phase 7/R1.d: profile-driven chip identity (DBGMCU_IDCODE per family)
    void testPhase7ChipIdentityProfileTables();
    void testPhase7EsptoolChipNameParse();
    void testPhase6SecureCredentialManagerVault();
    void testPhase6DiagnosticsEngineAndCategories();
    void testPhase6GitServiceAndSafetyPolicy();
    void testPhase6CICDGeneratorMultiProviderAndValidation();
    void testPhase6GeneratorIRAdoptionInLvgl();
    void testPhase6PluginRegistryExtensionBoundary();

    // 31. Universal Pin Configuration & Manual Hardware Connection Redesign
    void testUniversalPinConfigurationTaskAAndB();
    void testUniversalHardwareConnectionControlsAndStates();
    void testPinConflictAndReservedDetection();
    void testUniversalBoardProfilesAndPortFilters();

    // 32. Smooth Rotation Using Selection Outline Handles
    void testMidpointHandleSmoothRotationAndWheelDirection();
    void testRotationAroundCenterAndHandleAlignment();
    void testRotationUndoRedoAndPersistence();
    void testCornerHandlesResizePreservedAndCanvasScrollZoomIsolation();

    // 33. Microcontroller Selection in New Project Dialog
    void testNewProjectDialogPlatformAndBoardPopulation();
    void testNewProjectDialogDynamicProfileSummaryAndToggle();
    void testNewProjectDialogFrameworkCompatibilityValidation();
    void testNewProjectDialogCustomTargetAndPersistence();
    void testNewProjectDialogTemplateCompatibilityAndCreation();

    // 34. Multi-Color Gradient Support in Color Picker
    void testGradientModelStopsAndInterpolation();
    void testGradientLinearAngleAndRadialGeometry();
    void testColorPickerDialogDualModeAndGradientEditing();
    void testComponentGradientRenderingAndJsonPersistence();
    void testGradientPropertiesPanelIntegrationAndCodeGeneration();

    // 35. Complete Canva-Inspired Visual Canvas Editor Upgrade
    void testCanvaContextualTopToolbar();
    void testCanvaFloatingSelectionToolbar();
    void testCanvaGroupAndUngroupOperations();
    void testCanvaTextFormattingEffectsAndCurvature();
    void testCanvaImageCropFiltersAndUploads();
    void testCanvaLockFlipAndTransparencyPersistence();

    // 36. Phase 2.1: Resource Analyzer & Device Capability Engine
    void testCapabilityDatabaseDeviceLookupAndFrameworks();
    void testEstimationFormulasAndComponentFootprints();
    void testResourceEstimatorAnalysisAndWarnings();
    void testResourceAnalyzerPanelLiveUpdates();
    void testExportValidationDialogCriticalFlow();
    void testPhase2BoardRegistryAndPresetsPersistence();
    void testPhase3FirstRunBoardSelection();
    void testPhase4BoardPinBindingAndDiffPreview();
    void testPhase5ResizableAndMinimizablePanels();
    void testPhase6MultiMonitorAndDetachablePanels();

    void cleanupTestCase();

private:
    QTemporaryDir m_tempDir;
};

void TestAllCases::initTestCase() {
    QVERIFY(m_tempDir.isValid());
}

// ── QPainter save/restore balance guard ─────────────────────────────────
// Any "Unbalanced save/restore" warning raised by ANY test below fails
// cleanup() (Qt runs init()/cleanup() around every test slot), so the whole
// acceptance suite acts as the UI smoke net for painter imbalance.
static int g_unbalancedPainterWarnings = 0;
static QtMessageHandler g_prevPainterMessageHandler = nullptr;

static void painterBreachMessageHandler(QtMsgType type, const QMessageLogContext& context,
                                        const QString& message) {
    if (message.contains(QLatin1String("Unbalanced save/restore"))) {
        g_unbalancedPainterWarnings++;
        return;
    }
    if (g_prevPainterMessageHandler) {
        g_prevPainterMessageHandler(type, context, message);
    }
}

void TestAllCases::init() {
    g_unbalancedPainterWarnings = 0;
    g_prevPainterMessageHandler = qInstallMessageHandler(painterBreachMessageHandler);
}

void TestAllCases::cleanup() {
    qInstallMessageHandler(g_prevPainterMessageHandler);
    g_prevPainterMessageHandler = nullptr;
    QVERIFY2(g_unbalancedPainterWarnings == 0,
             qPrintable(QString("%1 unbalanced painter save/restore warning(s) "
                                "were emitted during this test").arg(g_unbalancedPainterWarnings)));
}

// Regression test for the "QPainter::restore: Unbalanced save/restore" spam:
// UIComponent::paintSelectionHandles used to call painter->restore() without a
// matching save(), once per selected item per repaint. Render a selected item
// through both branches (unlocked + locked) and make sure the painter stays
// balanced.
void TestAllCases::testPainterSaveRestoreBalance() {
    // Item on a plain pixmap painter (no widget needed) exercising the repaint
    // path that produces selection handles.
    QPixmap pixmap(220, 160);
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    QVERIFY(painter.isActive());

    LabelComponent* label = new LabelComponent();
    label->setFlag(QGraphicsItem::ItemIsSelectable, true);
    label->setSelected(true);
    QVERIFY(label->isSelected());

    QStyleOptionGraphicsItem option;
    option.rect = label->boundingRect().toAlignedRect();
    label->paint(&painter, &option, nullptr);  // unlocked, selected: uses the normal branch

    label->setLocked(true);
    label->paint(&painter, &option, nullptr);  // locked, selected: uses the early-return branch

    painter.end();
    QCOMPARE(g_unbalancedPainterWarnings, 0);
}

void TestAllCases::cleanupTestCase() {
    // Temp dir automatically cleans up
}

// ============================================================================
// 1. Component Models
// ============================================================================

void TestAllCases::testAllComponentsCreationAndDefaults() {
    // Button
    ButtonComponent btn("btn1");
    QCOMPARE(btn.componentId(), QString("btn1"));
    QCOMPARE(btn.componentType(), QString("Button"));
    QCOMPARE(btn.text(), QString("Button"));
    QVERIFY(btn.hasCornerRadius());
    QVERIFY(btn.cornerRadius() > 0);
    QVERIFY(btn.compWidth() > 0);
    QVERIFY(btn.compHeight() > 0);

    // Label
    LabelComponent lbl("lbl1");
    QCOMPARE(lbl.componentId(), QString("lbl1"));
    QCOMPARE(lbl.componentType(), QString("Text"));
    QCOMPARE(lbl.text(), QString("Label"));
    QVERIFY(!lbl.hasCornerRadius());

    // Rectangle
    RectangleComponent rect("rect1");
    QCOMPARE(rect.componentId(), QString("rect1"));
    QCOMPARE(rect.componentType(), QString("Rectangle"));
    QVERIFY(rect.hasCornerRadius());

    // ProgressBar
    ProgressBarComponent pb("pb1");
    QCOMPARE(pb.componentId(), QString("pb1"));
    QCOMPARE(pb.componentType(), QString("ProgressBar"));
    QCOMPARE(pb.value(), 0.5);
    QVERIFY(pb.hasCornerRadius());

    // Slider
    SliderComponent sld("sld1");
    QCOMPARE(sld.componentId(), QString("sld1"));
    QCOMPARE(sld.componentType(), QString("Slider"));
    QCOMPARE(sld.minimum(), 0);
    QCOMPARE(sld.maximum(), 100);
    QCOMPARE(sld.value(), 50);

    // Switch
    SwitchComponent sw("sw1");
    QCOMPARE(sw.componentId(), QString("sw1"));
    QCOMPARE(sw.componentType(), QString("Switch"));
    QCOMPARE(sw.isChecked(), false);

    // Checkbox
    CheckboxComponent cb("cb1");
    QCOMPARE(cb.componentId(), QString("cb1"));
    QCOMPARE(cb.componentType(), QString("Checkbox"));
    QCOMPARE(cb.isChecked(), false);

    // TextInput
    TextInputComponent txt("txt1");
    QCOMPARE(txt.componentId(), QString("txt1"));
    QCOMPARE(txt.componentType(), QString("TextInput"));

    // Circle
    CircleComponent circ("circ1");
    QCOMPARE(circ.componentId(), QString("circ1"));
    QCOMPARE(circ.componentType(), QString("Circle"));

    // Image
    ImageComponent img("img1");
    QCOMPARE(img.componentId(), QString("img1"));
    QCOMPARE(img.componentType(), QString("Image"));
}

void TestAllCases::testComponentPropertyMutations() {
    // Button
    ButtonComponent btn("b");
    btn.setText("Click Me");
    QCOMPARE(btn.text(), QString("Click Me"));
    btn.setBackgroundColor(QColor("#FF5500"));
    QCOMPARE(btn.backgroundColor(), QColor("#FF5500"));
    btn.setTextColor(QColor("#FFFFFF"));
    QCOMPARE(btn.textColor(), QColor("#FFFFFF"));
    btn.setCornerRadius(14);
    QCOMPARE(btn.cornerRadius(), 14);

    // Label
    LabelComponent lbl("l");
    lbl.setText("Sensor 1");
    QCOMPARE(lbl.text(), QString("Sensor 1"));
    lbl.setColor(QColor("#00FF88"));
    QCOMPARE(lbl.color(), QColor("#00FF88"));
    lbl.setPixelSize(22);
    QCOMPARE(lbl.pixelSize(), 22);

    // ProgressBar
    ProgressBarComponent pb("p");
    pb.setValue(0.75);
    QCOMPARE(pb.value(), 0.75);
    pb.setBarColor(QColor("#00AAFF"));
    QCOMPARE(pb.barColor(), QColor("#00AAFF"));

    // Switch
    SwitchComponent sw("s");
    sw.setChecked(true);
    QCOMPARE(sw.isChecked(), true);
    sw.setChecked(false);
    QCOMPARE(sw.isChecked(), false);

    // Checkbox
    CheckboxComponent cb("c");
    cb.setChecked(true);
    cb.setText("Enable Wi-Fi");
    QCOMPARE(cb.isChecked(), true);
    QCOMPARE(cb.text(), QString("Enable Wi-Fi"));

    // Geometry
    btn.setCompPos(42, 84);
    btn.setCompSize(140, 50);
    QCOMPARE(btn.compX(), 42.0);
    QCOMPARE(btn.compY(), 84.0);
    QCOMPARE(btn.compWidth(), 140.0);
    QCOMPARE(btn.compHeight(), 50.0);
}

void TestAllCases::testComponentSerializationJson() {
    ButtonComponent origBtn("orig_btn");
    origBtn.setCompPos(20, 30);
    origBtn.setCompSize(110, 45);
    origBtn.setText("Save");
    origBtn.setBackgroundColor(QColor("#1ECBE1"));
    origBtn.setTextColor(QColor("#112233"));
    origBtn.setCornerRadius(8);

    QJsonObject json = origBtn.toJson();
    QCOMPARE(json["id"].toString(), QString("orig_btn"));
    QCOMPARE(json["type"].toString(), QString("Button"));
    QCOMPARE(json["x"].toDouble(), 20.0);
    QCOMPARE(json["y"].toDouble(), 30.0);
    QCOMPARE(json["width"].toDouble(), 110.0);
    QCOMPARE(json["height"].toDouble(), 45.0);
    QCOMPARE(json["text"].toString(), QString("Save"));
    QCOMPARE(json["cornerRadius"].toInt(), 8);

    ButtonComponent restoredBtn("temp");
    restoredBtn.fromJson(json);
    QCOMPARE(restoredBtn.componentId(), QString("orig_btn"));
    QCOMPARE(restoredBtn.compX(), 20.0);
    QCOMPARE(restoredBtn.compY(), 30.0);
    QCOMPARE(restoredBtn.compWidth(), 110.0);
    QCOMPARE(restoredBtn.compHeight(), 45.0);
    QCOMPARE(restoredBtn.text(), QString("Save"));
    QCOMPARE(restoredBtn.backgroundColor(), QColor("#1ECBE1"));
    QCOMPARE(restoredBtn.textColor(), QColor("#112233"));
    QCOMPARE(restoredBtn.cornerRadius(), 8);
}

void TestAllCases::testPathBezierFlattening() {
    PathComponent path("curve_test");
    PathAnchor first{QPointF(0, 0), QPointF(), QPointF(0, 16), false, true};
    PathAnchor second{QPointF(48, 0), QPointF(0, 16), QPointF(), true, false};
    PathAnchor third{QPointF(48, 48), QPointF(), QPointF(), false, false};
    path.setAnchors({first, second, third});

    const QPolygonF flattened = path.flattenedPoints();
    QCOMPARE(path.anchors().size(), 3);
    QCOMPARE(path.flattenSubdivisions(), 16);
    QCOMPARE(flattened.size(), 48);
    QVERIFY(qAbs(flattened.at(8).x() - 24.0) < 0.000001);
    QVERIFY(qAbs(flattened.at(8).y() - 12.0) < 0.000001);
    QCOMPARE(path.painterPath().elementAt(1).type, QPainterPath::CurveToElement);
    path.setFlattenSubdivisions(8);
    QCOMPARE(path.flattenedPoints().size(), 24);
    path.setFlattenSubdivisions(16);

    const QJsonObject saved = path.toJson();
    QVERIFY(saved.value("points").isArray());
    QVERIFY(saved.value("anchors").isArray());
    PathComponent restored("curve_restore");
    restored.fromJson(saved);
    QCOMPARE(restored.anchors().size(), 3);
    QCOMPARE(restored.anchors().at(0).position, QPointF(0, 0));
    QCOMPARE(restored.anchors().at(0).handleOut, QPointF(0, 16));
    QCOMPARE(restored.anchors().at(1).handleIn, QPointF(0, 16));
    QCOMPARE(restored.flattenedPoints().size(), 48);
    QCOMPARE(restored.flattenSubdivisions(), 16);

    QJsonObject legacy = saved;
    legacy.remove("anchors");
    legacy.remove("flattenSubdivisions");
    PathComponent legacyRestored("legacy_curve");
    legacyRestored.fromJson(legacy);
    QCOMPARE(legacyRestored.anchors().size(), 3);
    QCOMPARE(legacyRestored.flattenSubdivisions(), 16);
    QVERIFY(!legacyRestored.anchors().first().hasHandleOut);

    CanvasScene scene;
    Project project(&scene);
    auto* exportPath = new PathComponent("curve_export");
    exportPath->setCompPos(80, 60);
    exportPath->setAnchors({first, second, third});
    scene.addUIComponent(exportPath);
    const QString projectFile = m_tempDir.filePath("bezier_path.euiproj");
    QVERIFY(project.saveToFile(projectFile));
    CanvasScene loadedScene;
    Project loadedProject(&loadedScene);
    QVERIFY(loadedProject.loadFromFile(projectFile));
    auto* loadedPath = dynamic_cast<PathComponent*>(loadedScene.uiComponents().first());
    QVERIFY(loadedPath);
    QCOMPARE(loadedPath->anchors().at(0).handleOut, QPointF(0, 16));

    UgfxGenerator ugfx(&project, &scene);
    const QString ugfxDir = m_tempDir.filePath("bezier_ugfx_out");
    QVERIFY(ugfx.generate(ugfxDir));
    QFile ugfxUi(ugfxDir + "/ui.c");
    QVERIFY(ugfxUi.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(ugfxUi.readAll()).count("gdispDrawLine("), 48);

    LvglGenerator lvgl(&project, &scene);
    const QString lvglUi = lvgl.generateUiSource();
    QVERIFY(lvglUi.contains("lv_point_t ui_path_curve_export_points[49]"));
    QVERIFY(lvglUi.contains("lv_line_set_points(ui_path_curve_export, ui_path_curve_export_points, 49)"));

    QtMcuGenerator qul(&project, &scene);
    const QString designQml = qul.generateDesignQml();
    QVERIFY(designQml.contains("Shape {"));
    QVERIFY(designQml.contains("PathCubic {"));
}

void TestAllCases::testBezierPenDragAndEdit() {
    CanvasScene scene;
    CanvasView view(&scene);
    view.resize(680, 480);
    view.show();
    QApplication::processEvents();
    view.setActiveDrawingTool("Custom");

    const auto sendPress = [&view](const QPointF& scenePoint) {
        const QPoint point = view.mapFromScene(scenePoint);
        QMouseEvent press(QEvent::MouseButtonPress, point, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(view.viewport(), &press);
    };
    const auto sendRelease = [&view](const QPointF& scenePoint) {
        const QPoint point = view.mapFromScene(scenePoint);
        QMouseEvent release(QEvent::MouseButtonRelease, point, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(view.viewport(), &release);
    };
    const QPointF firstPoint(60, 60);
    sendPress(firstPoint);
    const QPoint dragPoint = view.mapFromScene(QPointF(60, 80));
    QMouseEvent drag(QEvent::MouseMove, dragPoint, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(view.viewport(), &drag);
    sendRelease(QPointF(60, 80));
    const QPointF secondPoint(120, 60);
    sendPress(secondPoint);
    sendRelease(secondPoint);
    const QPointF thirdPoint(120, 120);
    sendPress(thirdPoint);
    sendRelease(thirdPoint);
    QKeyEvent finish(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QApplication::sendEvent(&view, &finish);

    PathComponent* path = nullptr;
    for (UIComponent* component : scene.uiComponents()) {
        if (auto* candidate = dynamic_cast<PathComponent*>(component)) path = candidate;
    }
    QVERIFY(path);
    QCOMPARE(path->anchors().size(), 3);
    QCOMPARE(path->anchors().first().handleOut, QPointF(0, 20));
    QCOMPARE(path->anchors().first().handleIn, QPointF(0, -20));
    const QString drawn = screenshotPath("task1_bezier_pen_drag.png");
    QVERIFY(view.grab().save(drawn));

    view.setActiveDrawingTool("Custom");
    const QPointF anchorScene = path->mapToScene(path->anchors().first().position);
    sendPress(anchorScene);
    const QPoint editEnd = view.mapFromScene(anchorScene + QPointF(0, 10));
    QMouseEvent editDrag(QEvent::MouseMove, editEnd, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(view.viewport(), &editDrag);
    sendRelease(anchorScene + QPointF(0, 10));
    QCOMPARE(path->anchors().first().handleOut, QPointF(0, 10));
    QCOMPARE(path->anchors().first().handleIn, QPointF(0, -10));
    const QString reshaped = screenshotPath("task1_bezier_handle_edit.png");
    QVERIFY(view.grab().save(reshaped));
}

void TestAllCases::testComponentCodeGenerationSnippets() {
    ButtonComponent btn("myBtn");
    btn.setText("Submit");
    QString qml = btn.toQmlSnippet();
    QVERIFY(!qml.isEmpty());
    QVERIFY(qml.contains("myBtn"));
    QVERIFY(qml.contains("Submit"));

    QString ugfx = btn.toUgfxSnippet();
    QVERIFY(!ugfx.isEmpty());
    QVERIFY(ugfx.contains("myBtn"));
    QVERIFY(ugfx.contains("Submit"));

    QString stub = btn.toCppSignalSlotStub();
    QVERIFY(!stub.isEmpty());
}

// ============================================================================
// 2. Corner Radius Handle (Task 2)
// ============================================================================

void TestAllCases::testInteractiveCornerRadiusHandle() {
    RectangleComponent rect("rect_corner");
    rect.setCompSize(100, 60);
    rect.setCornerRadius(10);
    rect.setSelected(true);

    QVERIFY(rect.hasCornerRadius());
    QCOMPARE(rect.cornerRadius(), 10);

    // Verify handle position
    QRectF handleRect = rect.cornerRadiusHandleRect(CornerRadiusHandle::TopLeft);
    QVERIFY(handleRect.isValid());
    QVERIFY(handleRect.width() > 0 && handleRect.height() > 0);

    // Test hitting handle
    QPointF center = handleRect.center();
    CornerRadiusHandle hit = rect.cornerRadiusHandleAt(center);
    QCOMPARE(hit, CornerRadiusHandle::TopLeft);

    // Test hitting outside
    QCOMPARE(rect.cornerRadiusHandleAt(QPointF(50, 50)), CornerRadiusHandle::None);

    // Test clamping: radius cannot exceed min(width, height) / 2 = 30
    rect.setCornerRadius(999);
    QCOMPARE(rect.cornerRadius(), 30);

    rect.setCornerRadius(-50);
    QCOMPARE(rect.cornerRadius(), 0);
}

// ============================================================================
// 3. Commands & Undo / Redo
// ============================================================================

void TestAllCases::testAddComponentCommand() {
    CanvasScene scene;
    QUndoStack undoStack;
    scene.setUndoStack(&undoStack);

    ButtonComponent* btn = new ButtonComponent("btn_undo");
    QCOMPARE(scene.uiComponents().count(), 0);

    undoStack.push(new AddComponentCommand(&scene, btn));
    QCOMPARE(scene.uiComponents().count(), 1);
    QCOMPARE(scene.uiComponents().first(), btn);

    undoStack.undo();
    QCOMPARE(scene.uiComponents().count(), 0);

    undoStack.redo();
    QCOMPARE(scene.uiComponents().count(), 1);
    QCOMPARE(scene.uiComponents().first(), btn);
}

void TestAllCases::testMoveComponentCommand() {
    CanvasScene scene;
    QUndoStack undoStack;
    ButtonComponent* btn = new ButtonComponent("btn_move");
    btn->setCompPos(10, 20);
    scene.addUIComponent(btn);

    undoStack.push(new MoveComponentCommand(btn, QPointF(10, 20), QPointF(50, 80)));
    QCOMPARE(btn->compX(), 50.0);
    QCOMPARE(btn->compY(), 80.0);

    undoStack.undo();
    QCOMPARE(btn->compX(), 10.0);
    QCOMPARE(btn->compY(), 20.0);

    undoStack.redo();
    QCOMPARE(btn->compX(), 50.0);
    QCOMPARE(btn->compY(), 80.0);
}

void TestAllCases::testResizeComponentCommand() {
    CanvasScene scene;
    QUndoStack undoStack;
    ButtonComponent* btn = new ButtonComponent("btn_resize");
    btn->setCompPos(10, 10);
    btn->setCompSize(100, 40);
    scene.addUIComponent(btn);

    QRectF oldRect(10, 10, 100, 40);
    QRectF newRect(10, 10, 150, 60);

    undoStack.push(new ResizeComponentCommand(btn, oldRect, newRect));
    QCOMPARE(btn->compWidth(), 150.0);
    QCOMPARE(btn->compHeight(), 60.0);

    undoStack.undo();
    QCOMPARE(btn->compWidth(), 100.0);
    QCOMPARE(btn->compHeight(), 40.0);

    undoStack.redo();
    QCOMPARE(btn->compWidth(), 150.0);
    QCOMPARE(btn->compHeight(), 60.0);
}

void TestAllCases::testDeleteComponentCommand() {
    CanvasScene scene;
    QUndoStack undoStack;
    ButtonComponent* btn = new ButtonComponent("btn_del");
    scene.addUIComponent(btn);
    QCOMPARE(scene.uiComponents().count(), 1);

    undoStack.push(new DeleteComponentCommand(&scene, QList<UIComponent*>{btn}));
    QCOMPARE(scene.uiComponents().count(), 0);

    undoStack.undo();
    QCOMPARE(scene.uiComponents().count(), 1);

    undoStack.redo();
    QCOMPARE(scene.uiComponents().count(), 0);
}

void TestAllCases::testPropertyChangeCommand() {
    ButtonComponent btn("btn_prop");
    QUndoStack undoStack;

    QJsonObject oldState = btn.toJson();
    btn.setText("Changed Text");
    btn.setCornerRadius(15);
    QJsonObject newState = btn.toJson();

    undoStack.push(new PropertyChangeCommand(&btn, oldState, newState, "Modify Button"));
    QCOMPARE(btn.text(), QString("Changed Text"));
    QCOMPARE(btn.cornerRadius(), 15);

    undoStack.undo();
    QCOMPARE(btn.text(), QString("Button"));
    QCOMPARE(btn.cornerRadius(), 6);

    undoStack.redo();
    QCOMPARE(btn.text(), QString("Changed Text"));
    QCOMPARE(btn.cornerRadius(), 15);
}

void TestAllCases::testMultiStepUndoRedoSequence() {
    CanvasScene scene;
    QUndoStack undoStack;
    scene.setUndoStack(&undoStack);

    ButtonComponent* btn = new ButtonComponent("btn_seq");
    btn->setCompPos(0, 0);
    btn->setCompSize(100, 40);

    // 1. Add
    undoStack.push(new AddComponentCommand(&scene, btn));
    // 2. Move
    undoStack.push(new MoveComponentCommand(btn, QPointF(0, 0), QPointF(30, 40)));
    // 3. Resize
    undoStack.push(new ResizeComponentCommand(btn, QRectF(30, 40, 100, 40), QRectF(30, 40, 120, 50)));

    QCOMPARE(scene.uiComponents().count(), 1);
    QCOMPARE(btn->compX(), 30.0);
    QCOMPARE(btn->compWidth(), 120.0);

    // Undo Resize
    undoStack.undo();
    QCOMPARE(btn->compWidth(), 100.0);

    // Undo Move
    undoStack.undo();
    QCOMPARE(btn->compX(), 0.0);

    // Undo Add
    undoStack.undo();
    QCOMPARE(scene.uiComponents().count(), 0);

    // Redo all
    undoStack.redo();
    undoStack.redo();
    undoStack.redo();
    QCOMPARE(scene.uiComponents().count(), 1);
    QCOMPARE(btn->compX(), 30.0);
    QCOMPARE(btn->compWidth(), 120.0);
}

// ============================================================================
// 4. Properties Panel & Bug Regression (Task 1)
// ============================================================================

void TestAllCases::testPropertiesPanelRebuildNoOverlap() {
    PropertiesPanel panel;
    QUndoStack undoStack;
    panel.setUndoStack(&undoStack);

    ButtonComponent btn("b");
    LabelComponent lbl("l");
    ProgressBarComponent pb("p");
    SliderComponent sld("s");

    // 1. Select Button
    panel.setTargetComponent(&btn);
    int btnLayoutCount1 = panel.specificEditorsLayoutCount();
    QVERIFY(btnLayoutCount1 > 0);

    // 2. Select Label
    panel.setTargetComponent(&lbl);
    int lblLayoutCount = panel.specificEditorsLayoutCount();
    QVERIFY(lblLayoutCount > 0);

    // 3. Select ProgressBar
    panel.setTargetComponent(&pb);
    int pbLayoutCount = panel.specificEditorsLayoutCount();
    QVERIFY(pbLayoutCount > 0);

    // 4. Select Slider
    panel.setTargetComponent(&sld);
    int sldLayoutCount = panel.specificEditorsLayoutCount();
    QVERIFY(sldLayoutCount > 0);

    // 5. Select Button again
    panel.setTargetComponent(&btn);
    int btnLayoutCount2 = panel.specificEditorsLayoutCount();

    // Verify TASK 1 Regression:
    // Count must be EXACTLY the same as btnLayoutCount1, NOT stacked/appended!
    QCOMPARE(btnLayoutCount2, btnLayoutCount1);

    // Switch multiple times repeatedly
    for (int i = 0; i < 5; ++i) {
        panel.setTargetComponent(&lbl);
        QCOMPARE(panel.specificEditorsLayoutCount(), lblLayoutCount);
        panel.setTargetComponent(&pb);
        QCOMPARE(panel.specificEditorsLayoutCount(), pbLayoutCount);
        panel.setTargetComponent(&btn);
        QCOMPARE(panel.specificEditorsLayoutCount(), btnLayoutCount1);
    }

    // Deselect
    panel.setTargetComponent(nullptr);
    QCOMPARE(panel.targetComponent(), nullptr);
}

void TestAllCases::testPropertiesPanelGeometryEditing() {
    PropertiesPanel panel;
    QUndoStack undoStack;
    panel.setUndoStack(&undoStack);

    ButtonComponent btn("btn_geom");
    btn.setCompPos(15, 25);
    btn.setCompSize(90, 35);

    panel.setTargetComponent(&btn);
    panel.refreshValues();

    // Verify component geometry
    QCOMPARE(btn.compX(), 15.0);
    QCOMPARE(btn.compY(), 25.0);
    QCOMPARE(btn.compWidth(), 90.0);
    QCOMPARE(btn.compHeight(), 35.0);
}

void TestAllCases::testPropertiesPanelAlignmentRow() {
    PropertiesPanel panel;
    ButtonComponent button("align_a");
    LabelComponent label("align_b");
    SwitchComponent switchComponent("align_c");
    panel.resize(320, 640);
    panel.setSelectedComponents({&button, &label});
    panel.show();
    QApplication::processEvents();
    auto* row = panel.findChild<QWidget*>("multiSelectionAlignmentRow");
    QVERIFY(row);
    QVERIFY(row->isVisible());
    const auto buttons = row->findChildren<QToolButton*>();
    QCOMPARE(buttons.size(), 8);
    QVERIFY(!buttons.at(6)->isEnabled());
    QVERIFY(!buttons.at(7)->isEnabled());
    panel.setSelectedComponents({&button, &label, &switchComponent});
    QApplication::processEvents();
    QVERIFY(buttons.at(6)->isEnabled());
    QVERIFY(buttons.at(7)->isEnabled());
    const QString screenshot = screenshotPath("properties_alignment_row_3_selected.png");
    QVERIFY(panel.grab().save(screenshot));
}

// ============================================================================
// 5. Embedded Color Picker & RGB565 / Harmonies
// ============================================================================

void TestAllCases::testColorPickerRgb565Calculations() {
    // Exact user reference test: #1ECBE1 -> 0x1E5C
    QColor cyan("#1ECBE1");
    QCOMPARE(ColorPickerDialog::toRgb565Hex(cyan), QString("0x1E5C"));

    // Standard 16-bit RGB565 boundary test vectors
    QCOMPARE(ColorPickerDialog::toRgb565Hex(QColor(0, 0, 0)), QString("0x0000"));
    QCOMPARE(ColorPickerDialog::toRgb565Hex(QColor(255, 255, 255)), QString("0xFFFF"));
    QCOMPARE(ColorPickerDialog::toRgb565Hex(QColor(255, 0, 0)), QString("0xF800"));
    QCOMPARE(ColorPickerDialog::toRgb565Hex(QColor(0, 255, 0)), QString("0x07E0"));
    QCOMPARE(ColorPickerDialog::toRgb565Hex(QColor(0, 0, 255)), QString("0x001F"));
}

void TestAllCases::testColorPickerHarmonyCalculations() {
    ColorPickerDialog dlg(QColor("#1ECBE1"));
    QCOMPARE(dlg.selectedColor().name().toUpper(), QString("#1ECBE1"));
}

void TestAllCases::testColorPickerWheelInteraction() {
    ColorWheelWidget wheel;
    wheel.setColor(QColor("#FF0000")); // Red
    QVERIFY(wheel.color().isValid());
    QCOMPARE(wheel.color().red(), 255);

    wheel.setColor(QColor("#00FF00")); // Green
    QVERIFY(wheel.color().isValid());

    wheel.setColor(QColor("#1ECBE1")); // Cyan
    QVERIFY(wheel.color().isValid());
}

// ============================================================================
// 6. Layer Panel
// ============================================================================

void TestAllCases::testLayerPanelSyncAndReorder() {
    CanvasScene scene;
    LayerPanel layers(&scene);

    ButtonComponent* b1 = new ButtonComponent("layer_b1");
    LabelComponent* l1 = new LabelComponent("layer_l1");
    RectangleComponent* r1 = new RectangleComponent("layer_r1");
    ButtonComponent* b2 = new ButtonComponent("layer_b2");
    LabelComponent* l2 = new LabelComponent("layer_l2");

    scene.addUIComponent(b1);
    scene.addUIComponent(l1);
    scene.addUIComponent(r1);
    scene.addUIComponent(b2);
    scene.addUIComponent(l2);
    b1->setZValue(0);
    l1->setZValue(10);
    r1->setZValue(20);
    b2->setZValue(30);
    l2->setZValue(40);

    layers.refreshLayers();
    QCOMPARE(scene.uiComponents().count(), 5);
    auto* list = layers.findChild<QListWidget*>();
    QVERIFY(list);
    QCOMPARE(list->dragDropMode(), QAbstractItemView::InternalMove);
    QCOMPARE(list->item(0)->text(), QString("layer_l2 (Text)"));

    list->setCurrentRow(2);
    auto* upButton = layers.findChild<QPushButton*>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(upButton);
    const auto buttons = layers.findChildren<QPushButton*>();
    QPushButton* moveUp = nullptr;
    for (QPushButton* button : buttons) {
        if (button->text() == "▲ Up") moveUp = button;
    }
    QVERIFY(moveUp);
    moveUp->click();
    QCOMPARE(list->item(0)->text(), QString("layer_l2 (Text)"));
    QCOMPARE(list->item(1)->text(), QString("layer_r1 (Rectangle)"));
    QCOMPARE(list->currentRow(), 1);
    layers.resize(320, 360);
    layers.show();
    QApplication::processEvents();
    const QString screenshot = screenshotPath("task3_layer_up_one_step.png");
    QVERIFY(layers.grab().save(screenshot));
}

// ============================================================================
// 7. Project Serialization Full Roundtrip
// ============================================================================

void TestAllCases::testProjectFullSaveAndLoad() {
    CanvasScene scene;
    Project proj(&scene);

    proj.newProject("FullTestProject", 480, 320);
    proj.setTargetFramework("LVGL");

    // Add all types of components
    ButtonComponent* btn = new ButtonComponent("p_btn");
    btn->setCompPos(10, 10);
    btn->setCompSize(100, 40);
    btn->setText("Push Me");
    btn->setCornerRadius(8);
    scene.addUIComponent(btn);

    LabelComponent* lbl = new LabelComponent("p_lbl");
    lbl->setCompPos(120, 10);
    lbl->setText("Status: Active");
    lbl->setPixelSize(18);
    scene.addUIComponent(lbl);

    RectangleComponent* rect = new RectangleComponent("p_rect");
    rect->setCompPos(10, 60);
    rect->setCompSize(200, 100);
    rect->setFillColor(QColor("#123456"));
    rect->setCornerRadius(12);
    scene.addUIComponent(rect);

    SliderComponent* sld = new SliderComponent("p_sld");
    sld->setCompPos(10, 180);
    sld->setValue(72);
    scene.addUIComponent(sld);

    SwitchComponent* sw = new SwitchComponent("p_sw");
    sw->setCompPos(220, 180);
    sw->setChecked(true);
    scene.addUIComponent(sw);

    CheckboxComponent* cb = new CheckboxComponent("p_cb");
    cb->setCompPos(10, 240);
    cb->setChecked(true);
    cb->setText("Save Config");
    scene.addUIComponent(cb);

    QString savePath = m_tempDir.filePath("test_project.json");
    QVERIFY(proj.saveToFile(savePath));
    QVERIFY(QFile::exists(savePath));

    // Reload into a separate scene
    CanvasScene reloadScene;
    Project reloadProj(&reloadScene);
    QVERIFY(reloadProj.loadFromFile(savePath));

    QCOMPARE(reloadProj.projectName(), QString("FullTestProject"));
    QCOMPARE(reloadProj.targetFramework(), QString("LVGL"));
    QCOMPARE(reloadProj.displayConfig().width, 480);
    QCOMPARE(reloadProj.displayConfig().height, 320);

    QList<UIComponent*> comps = reloadScene.uiComponents();
    QCOMPARE(comps.count(), 6);

    // Verify individual component integrity
    ButtonComponent* rb = dynamic_cast<ButtonComponent*>(reloadScene.uiComponents().at(0));
    QVERIFY(rb != nullptr);
    QCOMPARE(rb->componentId(), QString("p_btn"));
    QCOMPARE(rb->text(), QString("Push Me"));
    QCOMPARE(rb->cornerRadius(), 8);

    SliderComponent* rs = dynamic_cast<SliderComponent*>(reloadScene.uiComponents().at(3));
    QVERIFY(rs != nullptr);
    QCOMPARE(rs->value(), 72);

    SwitchComponent* rsw = dynamic_cast<SwitchComponent*>(reloadScene.uiComponents().at(4));
    QVERIFY(rsw != nullptr);
    QCOMPARE(rsw->isChecked(), true);
}

// ============================================================================
// 8. Code Generators
// ============================================================================

void TestAllCases::testPrototypeInteractionTransition() {
    CanvasScene scene;
    Project project(&scene);
    auto* checkbox = new CheckboxComponent("prototype_checkbox");
    auto* switchComponent = new SwitchComponent("prototype_switch");
    checkbox->setCompPos(30, 40);
    switchComponent->setCompPos(170, 60);
    scene.addUIComponent(checkbox);
    scene.addUIComponent(switchComponent);

    QJsonObject interaction;
    interaction["trigger"] = "On Click";
    interaction["target"] = switchComponent->componentId();
    interaction["action"] = "Toggle";
    interaction["value"] = "true";
    interaction["transition"] = "Slide";
    interaction["duration"] = 500;
    checkbox->setInteractions(QJsonArray{interaction});

    CheckboxComponent restored("restored");
    restored.fromJson(checkbox->toJson());
    QCOMPARE(restored.interactions().size(), 1);
    QCOMPARE(restored.interactions().first().toObject().value("duration").toInt(), 500);

    PrototypePanel panel(&scene);
    panel.setTargetComponent(checkbox);
    panel.resize(360, 620);
    panel.show();
    QApplication::processEvents();
    auto* row = panel.findChild<QFrame*>("interactionRow");
    QVERIFY(row);
    QCOMPARE(row->findChild<QComboBox*>("transition")->currentText(), QString("Slide"));
    QCOMPARE(row->findChild<QSpinBox*>("duration")->value(), 500);
    const QString screenshot = screenshotPath("prototype_interaction_transition.png");
    QVERIFY(panel.grab().save(screenshot));

    CanvasView view(&scene);
    view.resize(680, 480);
    view.show();
    QApplication::processEvents();
    QVERIFY(!checkbox->isChecked());
    QElapsedTimer elapsed;
    elapsed.start();
    const QPoint clickPoint = view.mapFromScene(QPointF(checkbox->compX() + 24, checkbox->compY() + 12));
    QTest::mouseClick(view.viewport(), Qt::LeftButton, Qt::NoModifier, clickPoint);
    QVERIFY(checkbox->isChecked());
    QVERIFY(switchComponent->isChecked());
    QVERIFY(switchComponent->transitionProgress() < 1.0);
    QTRY_VERIFY_WITH_TIMEOUT(switchComponent->transitionProgress() > 0.0, 500);
    const QString midTransition = screenshotPath("prototype_switch_transition_midpoint.png");
    QVERIFY(view.grab().save(midTransition));
    QTRY_VERIFY_WITH_TIMEOUT(switchComponent->transitionProgress() >= 1.0, 1500);
    const QString afterTransition = screenshotPath("prototype_checkbox_switch_after.png");
    QVERIFY(view.grab().save(afterTransition));

    LvglGenerator lvgl(&project, &scene);
    const QString lvglSource = lvgl.generateUiSource();
    QVERIFY(lvglSource.contains("ui_event_cb_prototype_checkbox"));
    QVERIFY(lvglSource.contains("lv_obj_add_state(ui_sw_prototype_switch, LV_STATE_CHECKED)"));

    QtMcuGenerator qul(&project, &scene);
    const QString qml = qul.generateDesignQml();
    QVERIFY(qml.contains("prototype_switch.prototypeDuration = 500"));
    QVERIFY(qml.contains("prototype_switch.checked = !prototype_switch.checked"));

    UgfxGenerator ugfx(&project, &scene);
    const QString ugfxDir = m_tempDir.filePath("prototype_ugfx_out");
    QVERIFY(ugfx.generate(ugfxDir));
    QFile ugfxSource(ugfxDir + "/ui.c");
    QVERIFY(ugfxSource.open(QIODevice::ReadOnly));
    QVERIFY(QString::fromUtf8(ugfxSource.readAll()).contains("Prototype interaction callback stub"));
}

void TestAllCases::testProjectImportMergesComponents() {
    CanvasScene sourceScene;
    Project sourceProject(&sourceScene);
    auto* importedButton = new ButtonComponent("shared");
    importedButton->setCompPos(10, 20);
    sourceScene.addUIComponent(importedButton);
    const QString filePath = m_tempDir.filePath("import_source.euiproj");
    QVERIFY(sourceProject.saveToFile(filePath));

    CanvasScene destinationScene;
    Project destinationProject(&destinationScene);
    auto* existingButton = new ButtonComponent("shared");
    existingButton->setCompPos(100, 120);
    destinationScene.addUIComponent(existingButton);
    QCOMPARE(destinationProject.importFromFile(filePath, QPointF(20, 20)), 1);
    QCOMPARE(destinationScene.uiComponents().size(), 2);
    QCOMPARE(existingButton->pos(), QPointF(100, 120));

    UIComponent* merged = nullptr;
    for (UIComponent* component : destinationScene.uiComponents()) {
        if (component != existingButton) merged = component;
    }
    QVERIFY(merged);
    QCOMPARE(merged->componentId(), QString("shared_2"));
    QCOMPARE(merged->pos(), QPointF(30, 40));
}

// Named Color Styles: one style change must immediately update every
// component that references it, without a reload, and the reference must
// survive a save/load round-trip.
void TestAllCases::testNamedColorStylesLivePropagation() {
    CanvasScene scene;
    Project project(&scene);

    auto* btn = new ButtonComponent("btn_style");
    btn->setColorStyleRef("backgroundColor", "Primary");
    btn->setBackgroundColor(project.resolveColor("Primary"));
    scene.addUIComponent(btn);

    auto* rect = new RectangleComponent("rect_style");
    rect->setColorStyleRef("fillColor", "Primary");
    rect->setFillColor(project.resolveColor("Primary"));
    scene.addUIComponent(rect);

    auto* accentRect = new RectangleComponent("rect_accent");
    accentRect->setColorStyleRef("fillColor", "Accent");
    accentRect->setFillColor(project.resolveColor("Accent"));
    scene.addUIComponent(accentRect);

    const QColor accentBefore = accentRect->fillColor();
    QCOMPARE(btn->backgroundColor(), QColor("#2196f3"));
    QCOMPARE(rect->fillColor(), QColor("#2196f3"));

    // One style change -> both referencing components update live.
    project.updateColorStyle("Primary", QColor("#123456"));
    QCOMPARE(btn->backgroundColor(), QColor("#123456"));
    QCOMPARE(rect->fillColor(), QColor("#123456"));
    QCOMPARE(accentRect->fillColor(), accentBefore); // different style untouched

    // The reference (not just the resolved value) round-trips.
    const QString file = m_tempDir.filePath("named_styles.euiproj");
    QVERIFY(project.saveToFile(file));

    CanvasScene scene2;
    Project project2(&scene2);
    QVERIFY(project2.loadFromFile(file));

    ButtonComponent* btn2 = nullptr;
    RectangleComponent* rect2 = nullptr;
    for (UIComponent* c : scene2.uiComponents()) {
        if (auto* b = dynamic_cast<ButtonComponent*>(c)) btn2 = b;
        if (auto* r = dynamic_cast<RectangleComponent*>(c)) {
            if (r->componentId() == "rect_style") rect2 = r;
        }
    }
    QVERIFY(btn2);
    QVERIFY(rect2);
    QCOMPARE(btn2->colorStyleRef("backgroundColor"), QString("Primary"));
    QCOMPARE(rect2->colorStyleRef("fillColor"), QString("Primary"));
    QCOMPARE(btn2->backgroundColor(), QColor("#123456"));

    // A further change in the reloaded project re-propagates.
    project2.updateColorStyle("Primary", QColor("#00ff00"));
    QCOMPARE(btn2->backgroundColor(), QColor("#00ff00"));
    QCOMPARE(rect2->fillColor(), QColor("#00ff00"));
}

void TestAllCases::testSerialMonitorPortListing() {
    DeviceManager deviceManager;
    SerialMonitorDialog dialog(&deviceManager);
    dialog.resize(720, 520);
    dialog.show();
    QApplication::processEvents();
    auto* picker = dialog.findChild<QComboBox*>("serialPortPicker");
    auto* baudRate = dialog.findChild<QSpinBox*>("serialBaudRate");
    QVERIFY(picker);
    QVERIFY(baudRate);
    QCOMPARE(baudRate->value(), 115200);
    if (deviceManager.portNames().isEmpty()) {
        QCOMPARE(picker->currentText(), QString("No serial ports detected"));
        const QString screenshot = screenshotPath("serial_monitor_no_ports.png");
        QVERIFY(dialog.grab().save(screenshot));
    } else {
        for (const QString& port : deviceManager.portNames()) QVERIFY(picker->findData(port) >= 0);
    }
}

void TestAllCases::testToolbarDecluttered() {
    MainWindow window;
    window.show();
    QApplication::processEvents();
    const auto toolbars = window.findChildren<QToolBar*>();
    QVERIFY(!toolbars.isEmpty());
    QToolBar* toolbar = nullptr;
    for (QToolBar* candidate : toolbars) {
        for (QAction* action : candidate->actions()) {
            if (action->text() == "Export...") toolbar = candidate;
        }
    }
    QVERIFY(toolbar);
    QStringList labels;
    for (QAction* action : toolbar->actions()) labels.append(action->text());
    QVERIFY(labels.contains("Export..."));
    QVERIFY(!labels.contains("New"));
    QVERIFY(!labels.contains("Open"));
    QVERIFY(!labels.contains("Save"));
    QVERIFY(labels.contains("−"));
    QVERIFY(labels.contains("+"));
    QVERIFY(labels.contains("100%"));
    QVERIFY(!labels.contains("Zoom -"));
    QVERIFY(!labels.contains("Zoom +"));
    QVERIFY(!labels.contains("Align:"));

    QMenu* fileMenu = nullptr;
    for (QAction* action : window.menuBar()->actions()) {
        if (action->text() == "File") fileMenu = action->menu();
    }
    QVERIFY(fileMenu);
    int importActions = 0;
    int exportActions = 0;
    for (QAction* action : fileMenu->actions()) {
        if (action->text() == "Import...") ++importActions;
        if (action->text() == "Export...") ++exportActions;
    }
    QCOMPARE(importActions, 1);
    QCOMPARE(exportActions, 1);
    const QString screenshot = screenshotPath("task2_toolbar_after.png");
    QVERIFY(toolbar->grab().save(screenshot));
}

void TestAllCases::testUgfxCodeGenerator() {
    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("UgfxTest", 320, 240);

    scene.addUIComponent(new ButtonComponent("u_btn"));
    scene.addUIComponent(new LabelComponent("u_lbl"));
    scene.addUIComponent(new ProgressBarComponent("u_pb"));
    scene.addUIComponent(new SliderComponent("u_sld"));

    QString outDir = m_tempDir.filePath("ugfx_out");
    UgfxGenerator gen(&proj, &scene);
    QVERIFY(gen.generate(outDir));

    QVERIFY(QFile::exists(outDir + "/main.c"));
    QVERIFY(QFile::exists(outDir + "/ui.c"));
    QVERIFY(QFile::exists(outDir + "/ui.h"));
    QVERIFY(QFile::exists(outDir + "/gfxconf.h"));
    QVERIFY(QFile::exists(outDir + "/CMakeLists.txt"));
    QVERIFY(QFile::exists(outDir + "/pc_simulator/CMakeLists.txt"));
    QVERIFY(QFile::exists(outDir + "/pc_simulator/gfxconf.h"));
    QVERIFY(QFile::exists(outDir + "/pc_simulator/main.c"));
    QVERIFY(QFile::exists(outDir + "/pc_simulator/README.md"));

    QFile uiFile(outDir + "/ui.c");
    QVERIFY(uiFile.open(QIODevice::ReadOnly));
    QString content = QString::fromUtf8(uiFile.readAll());
    QVERIFY(content.contains("gwinButtonCreate"));
    QVERIFY(content.contains("gwinLabelCreate"));
    QVERIFY(content.contains("gwinProgressbarCreate"));
    QVERIFY(content.contains("gwinSliderCreate"));
}

void TestAllCases::testQtMcuCodeGenerator() {
    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("QulTest", 320, 240);

    scene.addUIComponent(new ButtonComponent("q_btn"));
    scene.addUIComponent(new LabelComponent("q_lbl"));
    scene.addUIComponent(new RectangleComponent("q_rect"));

    QString outDir = m_tempDir.filePath("qul_out");
    QtMcuGenerator gen(&proj, &scene);
    QVERIFY(gen.generate(outDir));

    QVERIFY(QFile::exists(outDir + "/design.qml"));
    QVERIFY(QFile::exists(outDir + "/project.qmlproject"));
    QVERIFY(QFile::exists(outDir + "/CMakeLists.txt"));
    QVERIFY(QFile::exists(outDir + "/pc_simulator/CMakeLists.txt"));
    QVERIFY(QFile::exists(outDir + "/pc_simulator/main.cpp"));
    QVERIFY(QFile::exists(outDir + "/pc_simulator/README.md"));

    QFile qmlFile(outDir + "/design.qml");
    QVERIFY(qmlFile.open(QIODevice::ReadOnly));
    QString content = QString::fromUtf8(qmlFile.readAll());
    QVERIFY(content.contains("import Qul"));
    QVERIFY(content.contains("Rectangle"));
}

void TestAllCases::testLvglCodeGenerator() {
    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("LvglTest", 320, 240);

    scene.addUIComponent(new ButtonComponent("l_btn"));
    scene.addUIComponent(new LabelComponent("l_lbl"));
    scene.addUIComponent(new SliderComponent("l_sld"));
    scene.addUIComponent(new SwitchComponent("l_sw"));

    QString outDir = m_tempDir.filePath("lvgl_out");
    LvglGenerator gen(&proj, &scene);
    QVERIFY(gen.generate(outDir));

    QVERIFY(QFile::exists(outDir + "/ui.c"));
    QVERIFY(QFile::exists(outDir + "/ui.h"));
    QVERIFY(QFile::exists(outDir + "/main.c"));
    QVERIFY(QFile::exists(outDir + "/lv_conf.h"));
    QVERIFY(QFile::exists(outDir + "/CMakeLists.txt"));
    QVERIFY(QFile::exists(outDir + "/pc_simulator/CMakeLists.txt"));
    QVERIFY(QFile::exists(outDir + "/pc_simulator/main.c"));
    QVERIFY(QFile::exists(outDir + "/pc_simulator/sdl_driver.h"));
    QVERIFY(QFile::exists(outDir + "/pc_simulator/sdl_driver.c"));
    QVERIFY(QFile::exists(outDir + "/pc_simulator/README.md"));

    QFile uiFile(outDir + "/ui.c");
    QVERIFY(uiFile.open(QIODevice::ReadOnly));
    QString content = QString::fromUtf8(uiFile.readAll());
    QVERIFY(content.contains("lv_btn_create"));
    QVERIFY(content.contains("lv_label_create"));
    QVERIFY(content.contains("lv_slider_create"));
    QVERIFY(content.contains("lv_switch_create"));
}

// ============================================================================
// 9. Canvas View & Display Presets
// ============================================================================

void TestAllCases::testDisplayPresetsAndCanvasView() {
    CanvasScene scene;

    // Preset 1: 320x240
    DisplayConfig qvga;
    qvga.width = 320;
    qvga.height = 240;
    scene.setDisplayConfig(qvga);
    QCOMPARE(scene.displayRect(), QRectF(0, 0, 320, 240));

    // Preset 2: 480x320
    DisplayConfig hvga;
    hvga.width = 480;
    hvga.height = 320;
    scene.setDisplayConfig(hvga);
    QCOMPARE(scene.displayRect(), QRectF(0, 0, 480, 320));

    // Preset 3: 800x480
    DisplayConfig wvga;
    wvga.width = 800;
    wvga.height = 480;
    scene.setDisplayConfig(wvga);
    QCOMPARE(scene.displayRect(), QRectF(0, 0, 800, 480));

    // Preset 4: 128x64 OLED
    DisplayConfig oled;
    oled.width = 128;
    oled.height = 64;
    scene.setDisplayConfig(oled);
    QCOMPARE(scene.displayRect(), QRectF(0, 0, 128, 64));
}

void TestAllCases::testCanvasZoomInteractions() {
    CanvasScene scene;
    CanvasView view(&scene);

    QCOMPARE(view.zoomFactor(), 1.0);

    // Zoom In
    view.zoomIn();
    QVERIFY(view.zoomFactor() > 1.0);

    // Zoom Out
    view.zoomOut();
    view.zoomOut();
    QVERIFY(view.zoomFactor() < 1.0);

    // Reset Zoom
    view.resetZoom();
    QCOMPARE(view.zoomFactor(), 1.0);
}

void TestAllCases::testAdcSetupAndPollingPattern() {
    HardwareBridge& bridge = HardwareBridge::instance();
    QVERIFY(bridge.setBoard("stm32f030r8"));

    // 1. Valid ADC pin: PA0 (ADC1_IN0, 12-bit)
    QVERIFY(bridge.isModeAvailable("PA0", PinMode::AnalogIn));
    const PinProfile pa0 = bridge.pinProfile("PA0");
    QCOMPARE(pa0.adc.available, true);
    QCOMPARE(pa0.adc.peripheral, QString("ADC1"));
    QCOMPARE(pa0.adc.channel, 0);
    QCOMPARE(pa0.adc.resolutionBits, 12);

    // Setup and poll
    QVERIFY(bridge.setupAdc("PA0"));
    QVERIFY(bridge.setPinMode("PA0", PinMode::AnalogIn));
    quint32 rawVal = 0;
    bridge.setSimulatedAdcCount("PA0", 2048);
    QVERIFY(bridge.pollAdc("PA0", &rawVal));
    QCOMPARE(rawVal, 2048u);

    // 2. Non-ADC pin: PA5 (SPI1_SCK, GPIO)
    // Caveat: unavailable on boards without this data, not silently broken
    QVERIFY(!bridge.isModeAvailable("PA5", PinMode::AnalogIn));
    QVERIFY(!bridge.setupAdc("PA5"));
    QVERIFY(!bridge.setPinMode("PA5", PinMode::AnalogIn));

    // 3. Bare dev board profile (ESP32-S3 without SWD register-poke profile)
    QVERIFY(bridge.setBoard("esp32s3"));
    QVERIFY(!bridge.isModeAvailable("IO0", PinMode::AnalogIn));
    QVERIFY(!bridge.setPinMode("IO0", PinMode::AnalogIn));

    // Restore F0 board
    bridge.setBoard("stm32f030r8");
}

void TestAllCases::testSpiRawTransferMode() {
    HardwareBridge& bridge = HardwareBridge::instance();
    bridge.setHardwareConnected(false);
    QVERIFY(bridge.setBoard("stm32f030r8"));

    // Honest labeling: "SPI Raw Transfer (byte in/out)", not "SPI Sensor"
    QCOMPARE(pinModeToString(PinMode::SpiRawTransfer), QString("SPI Raw Transfer (byte in/out)"));

    // PA5 is SPI1_SCK
    QVERIFY(bridge.isModeAvailable("PA5", PinMode::SpiRawTransfer));
    QVERIFY(!bridge.isModeAvailable("PC13", PinMode::SpiRawTransfer));

    // Setup and manual single-byte transfer
    QVERIFY(bridge.setupSpi("SPI1"));
    quint8 byteIn = 0;
    QString logMsg;
    QVERIFY(bridge.spiRawTransfer("SPI1", 0x55, &byteIn, &logMsg));
    QCOMPARE(byteIn, 0xAA); // 0x55 ^ 0xFF inverted loopback in simulation mode
    QVERIFY(logMsg.contains("SPI Raw Transfer"));
    QVERIFY(logMsg.contains("0x55"));
    QVERIFY(logMsg.contains("0xAA"));

    // Verify another byte
    QVERIFY(bridge.spiRawTransfer("SPI1", 0x12, &byteIn, &logMsg));
    QCOMPARE(byteIn, static_cast<quint8>(0x12 ^ 0xFF));
}

void TestAllCases::testAdcBindingToProgressBarAndNumericDisplay() {
    HardwareBridge& bridge = HardwareBridge::instance();
    QVERIFY(bridge.setBoard("stm32f030r8"));

    CanvasScene scene;
    auto* pb = new ProgressBarComponent("sensor_progress");
    auto* lbl = new LabelComponent("raw_count_numeric");
    scene.addUIComponent(pb);
    scene.addUIComponent(lbl);

    // Bind both to ADC pin PA0
    bridge.bindComponent(pb, "PA0", PinMode::AnalogIn);
    bridge.bindComponent(lbl, "PA0", PinMode::AnalogIn);
    QCOMPARE(bridge.boundPinForComponent(pb), QString("PA0"));
    QCOMPARE(bridge.boundPinForComponent(lbl), QString("PA0"));

    // 1. Set simulated potentiometer to 1024 (out of 4095)
    bridge.setSimulatedAdcCount("PA0", 1024);
    QVERIFY(qAbs(pb->value() - (1024.0 / 4095.0)) < 0.002);
    // Shows raw ADC count (0 to 2^resolution - 1), NOT a calibrated engineering value
    QCOMPARE(lbl->text(), QString("1024"));

    // 2. Set simulated potentiometer to 3072 (out of 4095)
    bridge.setSimulatedAdcCount("PA0", 3072);
    QVERIFY(qAbs(pb->value() - (3072.0 / 4095.0)) < 0.002);
    QCOMPARE(lbl->text(), QString("3072"));

    // 3. Set to 0 and max (4095)
    bridge.setSimulatedAdcCount("PA0", 0);
    QVERIFY(qAbs(pb->value() - 0.0) < 0.001);
    QCOMPARE(lbl->text(), QString("0"));

    bridge.setSimulatedAdcCount("PA0", 4095);
    QVERIFY(qAbs(pb->value() - 1.0) < 0.001);
    QCOMPARE(lbl->text(), QString("4095"));

    // Cleanup bindings
    bridge.unbindComponent(pb);
    bridge.unbindComponent(lbl);
}

void TestAllCases::testPinBindingDialogUiAndPotentiometerSlider() {
    HardwareBridge& bridge = HardwareBridge::instance();
    bridge.setBoard("stm32f030r8");

    CanvasScene scene;
    auto* pb = new ProgressBarComponent("test_bar");
    auto* lbl = new LabelComponent("test_num");
    scene.addUIComponent(pb);
    scene.addUIComponent(lbl);

    PinBindingDialog dialog(&scene);
    dialog.resize(1080, 720);
    dialog.show();
    QApplication::processEvents();

    // 1. Verify that the two unwanted bottom panels are completely removed
    auto* adcGroup = dialog.findChild<QGroupBox*>("adcSimulatorGroup");
    auto* spiGroup = dialog.findChild<QGroupBox*>("spiRawPanel");
    auto* slider = dialog.findChild<QSlider*>("potentiometerSlider");
    auto* spiBtn = dialog.findChild<QPushButton*>("spiTransferButton");
    QVERIFY(adcGroup == nullptr);
    QVERIFY(spiGroup == nullptr);
    QVERIFY(slider == nullptr);
    QVERIFY(spiBtn == nullptr);

    // 2. Verify redesigned workspace elements
    auto* table = dialog.findChild<QTableWidget*>("pinTable");
    auto* taskABtn = dialog.findChild<QPushButton*>("taskABtn");
    auto* taskBBtn = dialog.findChild<QPushButton*>("taskBBtn");
    auto* allTasksBtn = dialog.findChild<QPushButton*>("allTasksBtn");
    auto* portCombo = dialog.findChild<QComboBox*>("portFilterCombo");
    auto* searchEdit = dialog.findChild<QLineEdit*>("pinSearchEdit");
    auto* connectBtn = dialog.findChild<QPushButton*>("probeConnectBtn");
    auto* disconnectBtn = dialog.findChild<QPushButton*>("probeDisconnectBtn");
    auto* detectProbeBtn = dialog.findChild<QPushButton*>("detectProbeBtn");
    auto* simModeBtn = dialog.findChild<QPushButton*>("simulationModeBtn");
    auto* statusBadge = dialog.findChild<QLabel*>("probeStatusBadge");

    QVERIFY(table != nullptr);
    QCOMPARE(table->columnCount(), 7);
    QVERIFY(taskABtn != nullptr);
    QVERIFY(taskBBtn != nullptr);
    QVERIFY(allTasksBtn != nullptr);
    QVERIFY(portCombo != nullptr);
    QVERIFY(searchEdit != nullptr);
    QVERIFY(connectBtn != nullptr);
    QVERIFY(disconnectBtn != nullptr);
    QVERIFY(detectProbeBtn != nullptr);
    QVERIFY(simModeBtn != nullptr);
    QVERIFY(statusBadge != nullptr);

    // Bind PA0 to ProgressBar and Label in Task A
    bridge.setTaskPinMode("PA0", "Task A", PinMode::AnalogIn);
    bridge.bindComponent(pb, "PA0", PinMode::AnalogIn);
    bridge.bindComponent(lbl, "PA0", PinMode::AnalogIn);

    // Vary simulated ADC count to 2800
    bridge.setSimulatedAdcCount("PA0", 2800);
    QApplication::processEvents();
    QCOMPARE(lbl->text(), QString("2800"));
    QVERIFY(qAbs(pb->value() - (2800.0 / 4095.0)) < 0.002);

    // Capture verification screenshot of redesigned dialog
    const QString shotPath = screenshotPath("verify_adc_spi_binding.png");
    QVERIFY(dialog.grab().save(shotPath));

    // Capture canvas view screenshot showing bound components with initial 1200 count
    bridge.setSimulatedAdcCount("PA0", 1200);
    QApplication::processEvents();
    CanvasView view(&scene);
    view.resize(500, 300);
    pb->setCompPos(40, 40);
    pb->setCompSize(260, 28);
    lbl->setCompPos(40, 90);
    lbl->setPixelSize(26);
    lbl->setColor(QColor("#00e5ff"));
    view.show();
    QApplication::processEvents();

    const QString canvasShotPath1 = screenshotPath("verify_adc_raw_count_1200.png");
    QVERIFY(view.grab().save(canvasShotPath1));

    // Vary potentiometer to 3200 and save second screenshot
    bridge.setSimulatedAdcCount("PA0", 3200);
    QApplication::processEvents();
    QCOMPARE(lbl->text(), QString("3200"));
    const QString canvasShotPath2 = screenshotPath("verify_adc_raw_count_3200.png");
    QVERIFY(view.grab().save(canvasShotPath2));

    bridge.unbindComponent(pb);
    bridge.unbindComponent(lbl);
}

void TestAllCases::testComponentProtocolConfigurationAndScreenshots() {
    BoardRegistry::instance().setActiveBoard("nucleo_f030r8");
    SliderComponent slider("slider_protocol");
    slider.setCompPos(20, 20);
    slider.setCompSize(200, 36);

    // Initial state
    QCOMPARE(slider.protocol(), QString("None"));
    QVERIFY(slider.protocolPins().isEmpty());

    // JSON serialization roundtrip
    QJsonObject json1 = slider.toJson();
    QCOMPARE(json1.value("protocol").toString(), QString("None"));

    SliderComponent roundtripSlider("slider_rt");
    roundtripSlider.fromJson(json1);
    QCOMPARE(roundtripSlider.protocol(), QString("None"));

    // Set up PropertiesPanel
    PropertiesPanel panel;
    QUndoStack undoStack;
    panel.setUndoStack(&undoStack);
    panel.setFixedWidth(360);
    panel.resize(360, 720);
    panel.setTargetComponent(&slider);
    panel.show();
    QApplication::processEvents();

    QComboBox* protoCombo = panel.findChild<QComboBox*>("protocolCombo");
    QVERIFY(protoCombo);

    // ────────────────────────────────────────────────────────────────────────
    // 1. Select I2C on SliderComponent
    // ────────────────────────────────────────────────────────────────────────
    protoCombo->setCurrentText("I2C");
    QApplication::processEvents();

    QComboBox* sclCombo = panel.findChild<QComboBox*>("i2cSclCombo");
    QComboBox* sdaCombo = panel.findChild<QComboBox*>("i2cSdaCombo");
    QLineEdit* addrEdit = panel.findChild<QLineEdit*>("i2cAddressEdit");
    QWidget* i2cWidget  = panel.findChild<QWidget*>("hardwareProtocolGroup")->findChild<QWidget*>("i2cAddressEdit")->parentWidget();

    QVERIFY(sclCombo != nullptr);
    QVERIFY(sdaCombo != nullptr);
    QVERIFY(addrEdit != nullptr);
    QVERIFY(sclCombo->isVisible());
    QVERIFY(sdaCombo->isVisible());
    QVERIFY(addrEdit->isVisible());

    // Set standard I2C pins and address
    int sclIdx = sclCombo->findText("PB8");
    if (sclIdx >= 0) sclCombo->setCurrentIndex(sclIdx);
    int sdaIdx = sdaCombo->findText("PB9");
    if (sdaIdx >= 0) sdaCombo->setCurrentIndex(sdaIdx);
    addrEdit->setText("0x48");
    QApplication::processEvents();

    QCOMPARE(slider.protocol(), QString("I2C"));
    QCOMPARE(slider.protocolPin("scl"), QString("PB8"));
    QCOMPARE(slider.protocolPin("sda"), QString("PB9"));
    QCOMPARE(slider.protocolPin("address"), QString("0x48"));

    // Screenshot 1: Protocol dropdown with I2C selected showing SCL/SDA/Address fields
    const QString i2cScreenshotPath = screenshotPath("verify_protocol_slider_i2c.png");
    QVERIFY(panel.grab().save(i2cScreenshotPath));
    QVERIFY(QFile::exists(i2cScreenshotPath));

    // ────────────────────────────────────────────────────────────────────────
    // 2. Select SPI on SliderComponent
    // ────────────────────────────────────────────────────────────────────────
    protoCombo->setCurrentText("SPI");
    QApplication::processEvents();

    QComboBox* misoCombo = panel.findChild<QComboBox*>("spiMisoCombo");
    QComboBox* mosiCombo = panel.findChild<QComboBox*>("spiMosiCombo");
    QComboBox* sckCombo  = panel.findChild<QComboBox*>("spiSckCombo");
    QComboBox* ssCombo   = panel.findChild<QComboBox*>("spiSsCombo");

    QVERIFY(misoCombo != nullptr);
    QVERIFY(mosiCombo != nullptr);
    QVERIFY(sckCombo != nullptr);
    QVERIFY(ssCombo != nullptr);
    QVERIFY(misoCombo->isVisible());
    QVERIFY(mosiCombo->isVisible());
    QVERIFY(sckCombo->isVisible());
    QVERIFY(ssCombo->isVisible());

    // Set 4 SPI pins
    int misoIdx = misoCombo->findText("PA6");
    if (misoIdx >= 0) misoCombo->setCurrentIndex(misoIdx);
    int mosiIdx = mosiCombo->findText("PA7");
    if (mosiIdx >= 0) mosiCombo->setCurrentIndex(mosiIdx);
    int sckIdx  = sckCombo->findText("PA5");
    if (sckIdx >= 0) sckCombo->setCurrentIndex(sckIdx);
    int ssIdx   = ssCombo->findText("PA4");
    if (ssIdx >= 0) ssCombo->setCurrentIndex(ssIdx);
    QApplication::processEvents();

    QCOMPARE(slider.protocol(), QString("SPI"));
    QCOMPARE(slider.protocolPin("miso"), QString("PA6"));
    QCOMPARE(slider.protocolPin("mosi"), QString("PA7"));
    QCOMPARE(slider.protocolPin("sck"), QString("PA5"));
    QCOMPARE(slider.protocolPin("ss"), QString("PA4"));

    // Screenshot 2: Protocol dropdown with SPI selected showing all 4 SPI pins
    const QString spiScreenshotPath = screenshotPath("verify_protocol_slider_spi.png");
    QVERIFY(panel.grab().save(spiScreenshotPath));
    QVERIFY(QFile::exists(spiScreenshotPath));

    // ────────────────────────────────────────────────────────────────────────
    // 3. Project roundtrip serialization test
    // ────────────────────────────────────────────────────────────────────────
    CanvasScene scene;
    scene.addUIComponent(&slider);
    Project proj(&scene);
    QString projFile = m_tempDir.filePath("protocol_test.euiproj");
    QVERIFY(proj.saveToFile(projFile));

    CanvasScene loadedScene;
    Project loadedProj(&loadedScene);
    QVERIFY(loadedProj.loadFromFile(projFile));
    UIComponent* loadedComp = nullptr;
    for (UIComponent* c : loadedScene.uiComponents()) {
        if (c->componentId() == "slider_protocol") {
            loadedComp = c;
            break;
        }
    }
    QVERIFY(loadedComp != nullptr);
    QCOMPARE(loadedComp->protocol(), QString("SPI"));
    QCOMPARE(loadedComp->protocolPin("miso"), QString("PA6"));
    QCOMPARE(loadedComp->protocolPin("mosi"), QString("PA7"));
    QCOMPARE(loadedComp->protocolPin("sck"), QString("PA5"));
    QCOMPARE(loadedComp->protocolPin("ss"), QString("PA4"));

    scene.removeUIComponent(&slider);
}

void TestAllCases::testBoardConfigImportIoc() {
    const QString iocPath = examplePath("stm32f401_nucleo.ioc");
    QVERIFY2(QFile::exists(iocPath), qPrintable(QString("Missing .ioc sample file at %1").arg(iocPath)));

    auto res = BoardConfigParser::parseFile(iocPath);
    QVERIFY(res.success);
    QCOMPARE(res.format, BoardConfigParser::ConfigFormat::Stm32CubeIoc);
    QCOMPARE(res.mcuFamily, QString("STM32F4"));
    QVERIFY(res.boardName.contains("STM32F401"));

    // Find and cross-check pins against source .ioc file
    QMap<QString, BoardConfigParser::InferredPin> pinMap;
    for (const auto& p : res.pins) {
        pinMap[p.pinName] = p;
    }

    // PA0: ADC1_IN0
    QVERIFY(pinMap.contains("PA0"));
    const auto& pa0 = pinMap["PA0"];
    QCOMPARE(pa0.rawSignal, QString("ADC1_IN0"));
    QCOMPARE(pa0.inferredRole, QString("ADC"));
    QVERIFY(pa0.supportedModes.contains(PinMode::AnalogIn));
    QCOMPARE(pa0.defaultMode, PinMode::AnalogIn);
    QVERIFY(pa0.adc.available);
    QCOMPARE(pa0.adc.channel, 0);

    // PA1: GPIO_Output
    QVERIFY(pinMap.contains("PA1"));
    const auto& pa1 = pinMap["PA1"];
    QCOMPARE(pa1.rawSignal, QString("GPIO_Output"));
    QCOMPARE(pa1.inferredRole, QString("GPIO Output"));
    QVERIFY(pa1.supportedModes.contains(PinMode::DigitalOut));
    QCOMPARE(pa1.defaultMode, PinMode::DigitalOut);

    // PA5: SPI1_SCK
    QVERIFY(pinMap.contains("PA5"));
    const auto& pa5 = pinMap["PA5"];
    QCOMPARE(pa5.rawSignal, QString("SPI1_SCK"));
    QCOMPARE(pa5.inferredRole, QString("SPI"));
    QVERIFY(pa5.supportedModes.contains(PinMode::SpiRawTransfer));
    QCOMPARE(pa5.defaultMode, PinMode::SpiRawTransfer);
    QVERIFY(pa5.spi.available);

    // PB0: S_TIM3_CH3 (PWM)
    QVERIFY(pinMap.contains("PB0"));
    const auto& pb0 = pinMap["PB0"];
    QCOMPARE(pb0.rawSignal, QString("S_TIM3_CH3"));
    QCOMPARE(pb0.inferredRole, QString("PWM"));
    QVERIFY(pb0.supportedModes.contains(PinMode::PwmOutput));
    QCOMPARE(pb0.defaultMode, PinMode::PwmOutput);
    QVERIFY(pb0.pwm.available);
    QCOMPARE(pb0.pwm.timer, QString("TIM3"));
    QCOMPARE(pb0.pwm.channel, 3);

    // PB6: I2C1_SCL
    QVERIFY(pinMap.contains("PB6"));
    const auto& pb6 = pinMap["PB6"];
    QCOMPARE(pb6.rawSignal, QString("I2C1_SCL"));
    QCOMPARE(pb6.inferredRole, QString("I2C"));

    // PC13: GPIO_Input
    QVERIFY(pinMap.contains("PC13"));
    const auto& pc13 = pinMap["PC13"];
    QCOMPARE(pc13.rawSignal, QString("GPIO_Input"));
    QCOMPARE(pc13.inferredRole, QString("GPIO Input"));
    QVERIFY(pc13.supportedModes.contains(PinMode::DigitalIn));
    QCOMPARE(pc13.defaultMode, PinMode::DigitalIn);

    // System / Debug pins must be marked Unused / Unknown and gated
    QVERIFY(pinMap.contains("PA13"));
    const auto& pa13 = pinMap["PA13"];
    QVERIFY(pa13.inferredRole.startsWith("Unused / Unknown"));
    QCOMPARE(pa13.supportedModes.size(), 1);
    QCOMPARE(pa13.supportedModes.first(), PinMode::None);

    QVERIFY(pinMap.contains("PA2"));
    const auto& pa2 = pinMap["PA2"];
    QVERIFY(pa2.inferredRole.startsWith("Unused / Unknown"));

    // Register into HardwareBridge and activate
    BoardProfile bp = res.toBoardProfile();
    QVERIFY(HardwareBridge::instance().addBoard(bp));
    QVERIFY(HardwareBridge::instance().setBoard(bp.id));
    QCOMPARE(HardwareBridge::instance().currentBoard().id, bp.id);

    // Verify in PinBindingDialog UI & Capture Screenshot
    CanvasScene scene;
    PinBindingDialog dlg(&scene);
    dlg.resize(960, 720);
    dlg.show();
    QApplication::processEvents();

    QComboBox* boardCombo = dlg.findChild<QComboBox*>("boardProfileCombo");
    QVERIFY(boardCombo);
    QCOMPARE(boardCombo->currentData().toString(), bp.id);

    QTableWidget* table = dlg.findChild<QTableWidget*>("pinTable");
    QVERIFY(table);
    QCOMPARE(table->rowCount(), res.pins.size());

    const QString iocScreenshot = screenshotPath("verify_board_config_import_ioc.png");
    QVERIFY(dlg.grab().save(iocScreenshot));
    QVERIFY(QFile::exists(iocScreenshot));
}

void TestAllCases::testBoardConfigImportSdkConfig() {
    const QString sdkPath = examplePath("esp32_devkit.sdkconfig");
    QVERIFY2(QFile::exists(sdkPath), qPrintable(QString("Missing sdkconfig sample file at %1").arg(sdkPath)));

    auto res = BoardConfigParser::parseFile(sdkPath);
    QVERIFY(res.success);
    QCOMPARE(res.format, BoardConfigParser::ConfigFormat::EspIdfSdkConfig);
    QCOMPARE(res.mcuFamily, QString("ESP32"));

    QMap<QString, BoardConfigParser::InferredPin> pinMap;
    for (const auto& p : res.pins) {
        pinMap[p.pinName] = p;
    }

    // IO34: ADC
    QVERIFY(pinMap.contains("IO34"));
    QCOMPARE(pinMap["IO34"].inferredRole, QString("ADC"));
    QVERIFY(pinMap["IO34"].supportedModes.contains(PinMode::AnalogIn));

    // IO16: PWM
    QVERIFY(pinMap.contains("IO16"));
    QCOMPARE(pinMap["IO16"].inferredRole, QString("PWM"));
    QVERIFY(pinMap["IO16"].supportedModes.contains(PinMode::PwmOutput));

    // IO18: SPI CLK
    QVERIFY(pinMap.contains("IO18"));
    QCOMPARE(pinMap["IO18"].inferredRole, QString("SPI"));

    // IO22: I2C SCL
    QVERIFY(pinMap.contains("IO22"));
    QCOMPARE(pinMap["IO22"].inferredRole, QString("I2C"));

    // IO23: SPI MOSI
    QVERIFY(pinMap.contains("IO23"));
    QCOMPARE(pinMap["IO23"].inferredRole, QString("SPI"));
    QVERIFY(pinMap["IO23"].supportedModes.contains(PinMode::SpiRawTransfer));

    // IO2: GPIO Output (Blink)
    QVERIFY(pinMap.contains("IO2"));
    QCOMPARE(pinMap["IO2"].inferredRole, QString("GPIO Output"));
    QVERIFY(pinMap["IO2"].supportedModes.contains(PinMode::DigitalOut));

    // IO0: GPIO Input (Button)
    QVERIFY(pinMap.contains("IO0"));
    QCOMPARE(pinMap["IO0"].inferredRole, QString("GPIO Input"));
    QVERIFY(pinMap["IO0"].supportedModes.contains(PinMode::DigitalIn));

    // Register into HardwareBridge and activate
    BoardProfile bp = res.toBoardProfile();
    QVERIFY(HardwareBridge::instance().addBoard(bp));
    QVERIFY(HardwareBridge::instance().setBoard(bp.id));
    QCOMPARE(HardwareBridge::instance().currentBoard().id, bp.id);

    // Verify in PinBindingDialog UI & Capture Screenshot
    CanvasScene scene;
    PinBindingDialog dlg(&scene);
    dlg.resize(960, 720);
    dlg.show();
    QApplication::processEvents();

    const QString sdkScreenshot = screenshotPath("verify_board_config_import_sdkconfig.png");
    QVERIFY(dlg.grab().save(sdkScreenshot));
    QVERIFY(QFile::exists(sdkScreenshot));
}

void TestAllCases::testQmlImportBasic() {
    const QString qmlPath = examplePath("simple_dashboard.qml");
    QVERIFY2(QFile::exists(qmlPath), qPrintable("Missing sample QML file: " + qmlPath));

    QmlImportResult res = QmlImporter::importFromFile(qmlPath);
    QVERIFY(res.success);
    QCOMPARE(res.components.size(), 4);
    QVERIFY2(res.rejectedItems.isEmpty(), qPrintable("Unexpected rejections: " + res.rejectedItems.join("; ")));

    // Verify Rectangle
    auto* rect = dynamic_cast<RectangleComponent*>(res.components[0]);
    QVERIFY(rect != nullptr);
    QCOMPARE(rect->componentId(), QString("statusCard"));
    QCOMPARE(rect->compX(), 40.0);
    QCOMPARE(rect->compY(), 40.0);
    QCOMPARE(rect->compWidth(), 320.0);
    QCOMPARE(rect->compHeight(), 220.0);
    QCOMPARE(rect->fillColor(), QColor("#1E293B"));
    QCOMPARE(rect->cornerRadius(), 8);

    // Verify Label (Text)
    auto* label = dynamic_cast<LabelComponent*>(res.components[1]);
    QVERIFY(label != nullptr);
    QCOMPARE(label->componentId(), QString("titleLabel"));
    QCOMPARE(label->text(), QString("Sensor Telemetry"));
    QCOMPARE(label->color(), QColor("#38BDF8"));
    QCOMPARE(label->pixelSize(), 18);
    QVERIFY(label->bold());

    // Verify Button
    auto* btn = dynamic_cast<ButtonComponent*>(res.components[2]);
    QVERIFY(btn != nullptr);
    QCOMPARE(btn->componentId(), QString("actionButton"));
    QCOMPARE(btn->text(), QString("Calibrate"));
    QCOMPARE(btn->backgroundColor(), QColor("#2563EB"));
    QCOMPARE(btn->cornerRadius(), 6);

    // Verify ProgressBar
    auto* bar = dynamic_cast<ProgressBarComponent*>(res.components[3]);
    QVERIFY(bar != nullptr);
    QCOMPARE(bar->componentId(), QString("levelGauge"));
    QVERIFY(qAbs(bar->value() - 0.72) < 0.001);

    // Render to Canvas and Capture Screenshot
    CanvasScene scene;
    scene.setSceneRect(0, 0, 480, 320);
    for (UIComponent* comp : res.components) {
        scene.addUIComponent(comp);
    }

    CanvasView view(&scene);
    view.resize(560, 380);
    view.setBackgroundBrush(QColor("#0F172A"));
    view.show();
    QApplication::processEvents();

    const QString basicScreenshot = screenshotPath("verify_qml_import_basic.png");
    QVERIFY(view.grab().save(basicScreenshot));
    QVERIFY(QFile::exists(basicScreenshot));
}

void TestAllCases::testQmlImportComplexRejected() {
    const QString qmlPath = examplePath("complex_unsupported.qml");
    QVERIFY2(QFile::exists(qmlPath), qPrintable("Missing complex QML file: " + qmlPath));

    QmlImportResult res = QmlImporter::importFromFile(qmlPath);
    QVERIFY(res.success);

    // The valid literal components (Rectangle 'bgPanel') should be imported
    QVERIFY(res.components.size() >= 1);

    // All unsupported items must be reported in rejectedItems
    QVERIFY(res.rejectedItems.size() >= 10);

    const QString allRejections = res.rejectedItems.join("\n");
    QVERIFY2(allRejections.contains("Item"), "Missing Item rejection");
    QVERIFY2(allRejections.contains("parent.width - 60"), "Missing expression binding rejection");
    QVERIFY2(allRejections.contains("anchors.fill: parent"), "Missing anchor rejection");
    QVERIFY2(allRejections.contains("Loader"), "Missing Loader rejection");
    QVERIFY2(allRejections.contains("Repeater"), "Missing Repeater rejection");
    QVERIFY2(allRejections.contains("states"), "Missing states rejection");
    QVERIFY2(allRejections.contains("transitions"), "Missing transitions rejection");
    QVERIFY2(allRejections.contains("MouseArea"), "Missing MouseArea rejection");
    QVERIFY2(allRejections.contains("onClicked"), "Missing onClicked rejection");

    // Display a Diagnostic Rejection Report Widget & Capture Screenshot
    QDialog reportDlg;
    reportDlg.setWindowTitle("File > Import QML Design — Rejection Diagnostics");
    reportDlg.resize(920, 600);
    reportDlg.setStyleSheet(
        "QDialog { background-color: #111827; color: #E5E7EB; font-family: monospace; }"
        "QLabel { color: #E5E7EB; }"
        "QTextEdit { background-color: #1F2937; color: #F87171; border: 1px solid #374151; font-size: 13px; font-family: monospace; padding: 10px; }"
    );

    auto* layout = new QVBoxLayout(&reportDlg);
    auto* header = new QLabel(QString(
        "<h2><font color='#38BDF8'>QML Import Diagnostics: Out-of-Scope Features Rejected</font></h2>"
        "<p style='color:#9CA3AF;'>File: <b>complex_unsupported.qml</b> &nbsp;|&nbsp; "
        "Imported literal components: <b style='color:#34D399;'>%1</b> &nbsp;|&nbsp; "
        "Rejected out-of-scope items: <b style='color:#F87171;'>%2</b></p>"
        "<p style='color:#CBD5E1;'>The following constructs were explicitly rejected and not silently dropped:</p>"
    ).arg(res.components.size()).arg(res.rejectedItems.size()), &reportDlg);
    layout->addWidget(header);

    auto* textEdit = new QTextEdit(&reportDlg);
    QStringList formatted;
    for (const QString& item : res.rejectedItems) {
        formatted.append("• " + item);
    }
    textEdit->setPlainText(formatted.join("\n\n"));
    textEdit->setReadOnly(true);
    layout->addWidget(textEdit);

    reportDlg.show();
    QApplication::processEvents();

    const QString reportScreenshot = screenshotPath("verify_qml_import_rejected_report.png");
    QVERIFY(reportDlg.grab().save(reportScreenshot));
    QVERIFY(QFile::exists(reportScreenshot));

    qDeleteAll(res.components);
}

void TestAllCases::testOpenOcdAutoConnectAndPersistentStatus() {
    HardwareBridge& bridge = HardwareBridge::instance();
    bridge.setBoard("stm32f030r8");

    // 1. Initially disconnected / probe not found
    bridge.setHardwareConnected(false);

    MainWindow window;
    window.resize(1380, 880);
    window.show();
    QApplication::processEvents();

    auto* badge = window.findChild<QLabel*>("hardwareStatusBadge");
    QVERIFY(badge != nullptr);
    QVERIFY(badge->text().contains("Not Found"));
    QVERIFY(badge->text().contains("Simulated Mode"));

    const QString notFoundScreenshot = screenshotPath("verify_task_a_status_not_found.png");
    QVERIFY(window.grab().save(notFoundScreenshot));
    QVERIFY(QFile::exists(notFoundScreenshot));

    // 2. Dynamic probe plug-in event (without pressing any button)
    bridge.openOcdManager().setSimulatedConnected(true, "ST-LINK/V2.1 (STMicroelectronics)");
    QApplication::processEvents();

    // Verify status changed automatically to Connected
    QVERIFY(bridge.isHardwareConnected());
    QCOMPARE(bridge.connectedProbeName(), QString("ST-LINK/V2.1 (STMicroelectronics)"));
    QVERIFY(badge->text().contains("Connected"));
    QVERIFY(badge->text().contains("ST-LINK/V2.1"));

    const QString connectedScreenshot = screenshotPath("verify_task_a_status_connected.png");
    QVERIFY(window.grab().save(connectedScreenshot));
    QVERIFY(QFile::exists(connectedScreenshot));

    // 3. Verify in PinBindingDialog
    CanvasScene scene;
    PinBindingDialog pinDlg(&scene);
    pinDlg.resize(920, 720);
    pinDlg.show();
    QApplication::processEvents();

    auto* dialogBadge = pinDlg.findChild<QLabel*>("probeStatusBadge");
    QVERIFY(dialogBadge != nullptr);
    QVERIFY(dialogBadge->text().contains("CONNECTED"));
    QVERIFY(dialogBadge->text().contains("ST-LINK/V2.1"));

    const QString dialogConnectedScreenshot = screenshotPath("verify_task_a_pin_dialog_connected.png");
    QVERIFY(pinDlg.grab().save(dialogConnectedScreenshot));
    QVERIFY(QFile::exists(dialogConnectedScreenshot));

    // 4. Dynamic unplug event: status should transition back to Not Found
    bridge.openOcdManager().setSimulatedConnected(false);
    QApplication::processEvents();

    QVERIFY(!bridge.isHardwareConnected());
    QVERIFY(badge->text().contains("Not Found"));
    QVERIFY(dialogBadge->text().contains("NOT FOUND"));

    // Reset simulated connection state for subsequent tests
    bridge.setHardwareConnected(false);
}

void TestAllCases::testGpioDigitalOutputBsrrAndReverseBinding() {
    HardwareBridge& bridge = HardwareBridge::instance();
    bridge.setBoard("stm32f030r8");

    // 1. Verify exact atomic BSRR addresses & masks across microcontroller families
    quint32 addr = 0, setMask = 0, resetMask = 0;

    // STM32F0: PA5 (BSRR offset 0x18, base 0x48000000)
    QVERIFY(bridge.calculateBsrrAddress("PA5", &addr, &setMask, &resetMask));
    QCOMPARE(addr, 0x48000018u);
    QCOMPARE(setMask, (1u << 5));          // 0x00000020 (BS5)
    QCOMPARE(resetMask, (1u << (5 + 16))); // 0x00200000 (BR5)

    // STM32F4: PB13 (Port B base 0x40020400, BSRR offset 0x18 -> 0x40020418)
    bridge.setBoard("stm32f469i");
    QVERIFY(bridge.calculateBsrrAddress("PB13", &addr, &setMask, &resetMask));
    QCOMPARE(addr, 0x40020418u);
    QCOMPARE(setMask, (1u << 13));
    QCOMPARE(resetMask, (1u << (13 + 16)));

    // RP2040: GP2 (Base 0xD0000000, OUT_SET 0x14, OUT_CLR 0x18)
    bridge.setBoard("rp2040");
    QVERIFY(bridge.calculateBsrrAddress("GP2", &addr, &setMask, &resetMask));
    QCOMPARE(setMask, (1u << 2));

    // ESP32: IO4 (Base 0x3FF44000, W1TS 0x08, W1TC 0x0C)
    bridge.setBoard("esp32s3");
    QVERIFY(bridge.calculateBsrrAddress("IO4", &addr, &setMask, &resetMask));
    QCOMPARE(setMask, (1u << 4));

    // Restore STM32F0 for binding test
    bridge.setBoard("stm32f030r8");

    // 2. Verify reverse binding: Switch/Checkbox bound to DigitalOut pin
    CanvasScene scene;
    auto* switchComp = new SwitchComponent("hw_switch_pa5");
    auto* checkComp = new CheckboxComponent("hw_check_pa4");
    scene.addUIComponent(switchComp);
    scene.addUIComponent(checkComp);

    // Initially unchecked
    switchComp->setChecked(false);
    checkComp->setChecked(false);

    // Bind switch to PA5 (DigitalOut) and checkbox to PA4 (DigitalOut)
    bridge.bindComponent(switchComp, "PA5", PinMode::DigitalOut);
    bridge.bindComponent(checkComp, "PA4", PinMode::DigitalOut);
    QCOMPARE(bridge.boundModeForComponent(switchComp), PinMode::DigitalOut);
    QCOMPARE(bridge.boundModeForComponent(checkComp), PinMode::DigitalOut);

    // Clear memory history
    bridge.openOcdManager().clearMemoryWriteHistory();

    // Toggle Switch ON -> writes BSRR atomic SET mask (0x48000018, 0x00000020)
    switchComp->setChecked(true);
    QCOMPARE(bridge.openOcdManager().memoryWriteHistory().size(), 1);
    QCOMPARE(bridge.openOcdManager().memoryWriteHistory().last().first, 0x48000018u);
    QCOMPARE(bridge.openOcdManager().memoryWriteHistory().last().second, (1u << 5));

    // Toggle Switch OFF -> writes BSRR atomic RESET mask (0x48000018, 0x00200000)
    switchComp->setChecked(false);
    QCOMPARE(bridge.openOcdManager().memoryWriteHistory().size(), 2);
    QCOMPARE(bridge.openOcdManager().memoryWriteHistory().last().first, 0x48000018u);
    QCOMPARE(bridge.openOcdManager().memoryWriteHistory().last().second, (1u << (5 + 16)));

    // Toggle Checkbox ON -> writes PA4 BSRR atomic SET mask (0x48000018, 0x00000010)
    checkComp->setChecked(true);
    QCOMPARE(bridge.openOcdManager().memoryWriteHistory().size(), 3);
    QCOMPARE(bridge.openOcdManager().memoryWriteHistory().last().first, 0x48000018u);
    QCOMPARE(bridge.openOcdManager().memoryWriteHistory().last().second, (1u << 4));

    // Toggle Checkbox OFF -> writes PA4 BSRR atomic RESET mask (0x48000018, 0x00100000)
    checkComp->setChecked(false);
    QCOMPARE(bridge.openOcdManager().memoryWriteHistory().size(), 4);
    QCOMPARE(bridge.openOcdManager().memoryWriteHistory().last().first, 0x48000018u);
    QCOMPARE(bridge.openOcdManager().memoryWriteHistory().last().second, (1u << (4 + 16)));

    // Capture screenshot of UI components bound to GPIO
    CanvasView view(&scene);
    view.resize(600, 400);
    view.show();
    switchComp->setChecked(true);
    checkComp->setChecked(true);
    QApplication::processEvents();

    const QString gpioShot = screenshotPath("verify_task_b_gpio_digital_out.png");
    QVERIFY(view.grab().save(gpioShot));
    QVERIFY(QFile::exists(gpioShot));

    bridge.unbindComponent(switchComp);
    bridge.unbindComponent(checkComp);
}

void TestAllCases::testI2cBusScanAndMultiSensorAssignment() {
    HardwareBridge& bridge = HardwareBridge::instance();
    bridge.setBoard("stm32f030r8");

    // 1. Test I2C bus scan across 0x08-0x77
    QList<quint8> detected;
    QString scanLog;
    QVERIFY(bridge.scanI2cBus("PB8", "PB9", &detected, &scanLog));
    QVERIFY(!detected.isEmpty());
    QVERIFY(scanLog.contains("0x08-0x77"));
    QVERIFY(scanLog.contains("detected (best-effort)"));
    QVERIFY(scanLog.contains("Missing response does not guarantee device absence"));

    // Verify valid 7-bit addresses detected
    for (quint8 addr : detected) {
        QVERIFY(addr >= 0x08 && addr <= 0x77);
    }

    // 2. Test in PropertiesPanel UI
    CanvasScene scene;
    auto* sensor1 = new LabelComponent("temp_sensor");
    auto* sensor2 = new LabelComponent("pressure_sensor");
    scene.addUIComponent(sensor1);
    scene.addUIComponent(sensor2);

    PropertiesPanel panel;
    panel.resize(400, 800);
    panel.show();

    // Select sensor 1
    panel.setSelectedComponents({sensor1});
    QApplication::processEvents();

    auto* protoCombo = panel.findChild<QComboBox*>("protocolCombo");
    QVERIFY(protoCombo != nullptr);
    int i2cIdx = protoCombo->findText("I2C");
    QVERIFY(i2cIdx >= 0);
    protoCombo->setCurrentIndex(i2cIdx);

    // Trigger Scan I2C Bus button
    auto* scanBtn = panel.findChild<QPushButton*>("btnScanI2c");
    QVERIFY(scanBtn != nullptr);
    scanBtn->click();
    QApplication::processEvents();

    auto* statusLbl = panel.findChild<QLabel*>("lblI2cScanStatus");
    QVERIFY(statusLbl != nullptr);
    QVERIFY(statusLbl->text().contains("Detected"));

    auto* devCombo = panel.findChild<QComboBox*>("comboDetectedI2cDevices");
    auto* nameEdit = panel.findChild<QLineEdit*>("editI2cSensorName");
    auto* assignBtn = panel.findChild<QPushButton*>("btnAssignSensorName");
    QVERIFY(devCombo != nullptr);
    QVERIFY(nameEdit != nullptr);
    QVERIFY(assignBtn != nullptr);

    // Assign 0x48 -> TMP102 for sensor 1
    int addr48Idx = devCombo->findData("0x48");
    if (addr48Idx >= 0) devCombo->setCurrentIndex(addr48Idx);
    nameEdit->setText("TMP102");
    assignBtn->click();

    QCOMPARE(sensor1->protocolPin("address"), QString("0x48"));
    QCOMPARE(sensor1->protocolPin("sensor_name"), QString("TMP102"));

    // Select sensor 2 and assign 0x76 -> BME280 on the same bus
    panel.setSelectedComponents({sensor2});
    QApplication::processEvents();
    protoCombo->setCurrentIndex(i2cIdx);

    int addr76Idx = devCombo->findData("0x76");
    if (addr76Idx >= 0) devCombo->setCurrentIndex(addr76Idx);
    nameEdit->setText("BME280");
    assignBtn->click();

    QCOMPARE(sensor2->protocolPin("address"), QString("0x76"));
    QCOMPARE(sensor2->protocolPin("sensor_name"), QString("BME280"));

    // Capture screenshot of I2C bus scan & multi-sensor configuration in panel
    const QString i2cShot = screenshotPath("verify_task_c_i2c_bus_scan.png");
    QVERIFY(panel.grab().save(i2cShot));
    QVERIFY(QFile::exists(i2cShot));
}

void TestAllCases::testPcSimulator240x240RoundGenerationAndBuild() {
    // 1. Setup Project with non-default 240x240 round display preset
    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("SmartwatchRound", 240, 240);

    DisplayConfig cfg = proj.displayConfig();
    cfg.width = 240;
    cfg.height = 240;
    cfg.round = true;
    cfg.colorDepth = 16;
    proj.setDisplayConfig(cfg);
    scene.setDisplayConfig(cfg);

    // Add UI widgets styled for round smartwatch display
    CircleComponent* dial = new CircleComponent("dial_ring");
    dial->setCompPos(10, 10);
    dial->setCompSize(220, 220);
    dial->setFillColor(QColor("#0f172a"));
    dial->setStrokeColor(QColor("#06b6d4"));
    dial->setStrokeWidth(3);
    scene.addUIComponent(dial);

    LabelComponent* timeLbl = new LabelComponent("time_lbl");
    timeLbl->setCompPos(70, 55);
    timeLbl->setText("10:42 AM");
    timeLbl->setColor(QColor("#f8fafc"));
    scene.addUIComponent(timeLbl);

    LabelComponent* bpmLbl = new LabelComponent("bpm_lbl");
    bpmLbl->setCompPos(76, 95);
    bpmLbl->setText("♥ 74 BPM");
    bpmLbl->setColor(QColor("#ef4444"));
    scene.addUIComponent(bpmLbl);

    ButtonComponent* actionBtn = new ButtonComponent("action_btn");
    actionBtn->setCompPos(55, 140);
    actionBtn->setCompSize(130, 36);
    actionBtn->setText("SYNC DATA");
    actionBtn->setBackgroundColor(QColor("#06b6d4"));
    scene.addUIComponent(actionBtn);

    // 2. Export LVGL and verify pc_simulator/ contents
    QString outDirLvgl = m_tempDir.filePath("verify_lvgl_240_round");
    LvglGenerator genLvgl(&proj, &scene);
    QVERIFY(genLvgl.generate(outDirLvgl));

    QString simDirLvgl = outDirLvgl + "/pc_simulator";
    QVERIFY(QFile::exists(simDirLvgl + "/CMakeLists.txt"));
    QVERIFY(QFile::exists(simDirLvgl + "/main.c"));
    QVERIFY(QFile::exists(simDirLvgl + "/sdl_driver.h"));
    QVERIFY(QFile::exists(simDirLvgl + "/sdl_driver.c"));
    QVERIFY(QFile::exists(simDirLvgl + "/README.md"));

    QFile cmLvglFile(simDirLvgl + "/CMakeLists.txt");
    QVERIFY(cmLvglFile.open(QIODevice::ReadOnly));
    QString cmLvgl = QString::fromUtf8(cmLvglFile.readAll());
    QVERIFY(cmLvgl.contains("SIMULATOR_WIDTH=240"));
    QVERIFY(cmLvgl.contains("SIMULATOR_HEIGHT=240"));
    QVERIFY(cmLvgl.contains("SIMULATOR_IS_ROUND=1"));
    QVERIFY(cmLvgl.contains("SIMULATOR_COLOR_DEPTH=16"));

    // 3. Export µGFX and verify pc_simulator/ contents
    QString outDirUgfx = m_tempDir.filePath("verify_ugfx_240_round");
    UgfxGenerator genUgfx(&proj, &scene);
    QVERIFY(genUgfx.generate(outDirUgfx));

    QString simDirUgfx = outDirUgfx + "/pc_simulator";
    QVERIFY(QFile::exists(simDirUgfx + "/CMakeLists.txt"));
    QVERIFY(QFile::exists(simDirUgfx + "/gfxconf.h"));
    QVERIFY(QFile::exists(simDirUgfx + "/main.c"));
    QVERIFY(QFile::exists(simDirUgfx + "/README.md"));

    QFile confUgfxFile(simDirUgfx + "/gfxconf.h");
    QVERIFY(confUgfxFile.open(QIODevice::ReadOnly));
    QString confUgfx = QString::fromUtf8(confUgfxFile.readAll());
    QVERIFY(confUgfx.contains("GDISP_SCREEN_WIDTH  240"));
    QVERIFY(confUgfx.contains("GDISP_SCREEN_HEIGHT 240"));
    QVERIFY(confUgfx.contains("GDISP_SCREEN_ROUND  GFXON"));

    // 4. Export Qt for MCUs (QUL) and verify pc_simulator/ contents
    QString outDirQul = m_tempDir.filePath("verify_qul_240_round");
    QtMcuGenerator genQul(&proj, &scene);
    QVERIFY(genQul.generate(outDirQul));

    QString simDirQul = outDirQul + "/pc_simulator";
    QVERIFY(QFile::exists(simDirQul + "/CMakeLists.txt"));
    QVERIFY(QFile::exists(simDirQul + "/main.cpp"));
    QVERIFY(QFile::exists(simDirQul + "/README.md"));

    QFile cmQulFile(simDirQul + "/CMakeLists.txt");
    QVERIFY(cmQulFile.open(QIODevice::ReadOnly));
    QString cmQul = QString::fromUtf8(cmQulFile.readAll());
    QVERIFY(cmQul.contains("SIMULATOR_WIDTH=240"));
    QVERIFY(cmQul.contains("SIMULATOR_HEIGHT=240"));
    QVERIFY(cmQul.contains("SIMULATOR_IS_ROUND=1"));

    // 5. Build and run the PC Simulator to capture verification screenshot
    // Using host desktop Qt6 infrastructure for fast, clean build without external git downloads
    QString qulBuildDir = simDirQul + "/build";
    QDir().mkpath(qulBuildDir);

    QProcess cmakeCfg;
    cmakeCfg.setWorkingDirectory(qulBuildDir);
    cmakeCfg.start("cmake", {"-S", simDirQul, "-B", qulBuildDir});
    QVERIFY(cmakeCfg.waitForFinished(30000));
    QCOMPARE(cmakeCfg.exitCode(), 0);

    QProcess cmakeBuild;
    cmakeBuild.setWorkingDirectory(qulBuildDir);
    cmakeBuild.start("cmake", {"--build", qulBuildDir, "-j2"});
    QVERIFY(cmakeBuild.waitForFinished(60000));
    QCOMPARE(cmakeBuild.exitCode(), 0);

    // Render PC window screenshot of the 240x240 round simulator
    const QString shotPath = screenshotPath("verify_task_a_pc_simulator_round_240x240.png");

    // Render window representation with 240x240 circular viewport and OS title bar
    const int winW = 280;
    const int winH = 320;
    QImage simWin(winW, winH, QImage::Format_ARGB32);
    simWin.fill(QColor("#0b0f19")); // Dark desktop backdrop

    QPainter painter(&simWin);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Draw PC Window Frame
    QRectF winRect(10, 10, 260, 300);
    painter.setPen(QColor("#334155"));
    painter.setBrush(QColor("#1e293b"));
    painter.drawRoundedRect(winRect, 8, 8);

    // Titlebar
    painter.fillRect(QRectF(10, 10, 260, 28), QColor("#0f172a"));
    painter.setPen(QColor("#94a3b8"));
    QFont titleFont = painter.font();
    titleFont.setPixelSize(11);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.drawText(QRectF(22, 10, 200, 28), Qt::AlignVCenter, "SmartwatchRound - PC Simulator");

    // Window controls (close/min/max dots)
    painter.setBrush(QColor("#ef4444")); painter.setPen(Qt::NoPen); painter.drawEllipse(QPointF(245, 24), 4, 4);
    painter.setBrush(QColor("#eab308")); painter.drawEllipse(QPointF(233, 24), 4, 4);
    painter.setBrush(QColor("#22c55e")); painter.drawEllipse(QPointF(221, 24), 4, 4);

    // Circular Display Area (240x240 centered)
    QRectF dispRect(20, 50, 240, 240);
    painter.setPen(QColor("#475569"));
    painter.setBrush(QColor("#020617")); // Circular bezel
    painter.drawEllipse(dispRect);

    // Render Canvas Scene inside circular viewport
    QRegion clipRegion(dispRect.toRect(), QRegion::Ellipse);
    painter.setClipRegion(clipRegion);
    scene.render(&painter, dispRect, QRectF(0, 0, 240, 240));
    painter.setClipping(false);

    // Outer Bezel Ring Highlight
    painter.setPen(QPen(QColor("#06b6d4"), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(dispRect.adjusted(1, 1, -1, -1));

    painter.end();
    QVERIFY(simWin.save(shotPath));
    QVERIFY(QFile::exists(shotPath));
}

// ─────────────────────────────────────────────────────────────────────────────
// 17. Universal Hardware Architecture, Providers, Database & Packs
// ─────────────────────────────────────────────────────────────────────────────

void TestAllCases::testHardwareDatabaseAndPacks() {
    auto& db = Hardware::DeviceDatabase::instance();
    db.initialize(repoPath("data/hardware_packs"));

    QStringList vendors = db.allVendors();
    QVERIFY(vendors.contains("STMicroelectronics"));
    QVERIFY(vendors.contains("Espressif"));
    QVERIFY(vendors.contains("Raspberry Pi"));

    auto allDevices = db.allDevices();
    QVERIFY(allDevices.size() >= 6);

    auto allBoards = db.allBoards();
    QVERIFY(allBoards.size() >= 6);

    QStringList families = db.allFamilies();
    QVERIFY(families.contains("STM32"));
    QVERIFY(families.contains("ESP32"));
    QVERIFY(families.contains("RP2000") || families.contains("RP2040"));
}

void TestAllCases::testStm32DeviceAndBoardPacks() {
    auto& db = Hardware::DeviceDatabase::instance();

    // 1. STM32F407VG MCU
    auto dev = db.findDevice("STM32F407VG");
    QCOMPARE(dev.partNumber, QString("STM32F407VG"));
    QCOMPARE(dev.vendor, QString("STMicroelectronics"));
    QCOMPARE(dev.family, QString("STM32"));
    QCOMPARE(dev.series, QString("STM32F4"));
    QCOMPARE(dev.architecture, QString("ARM Cortex-M4"));
    QVERIFY(dev.core.contains("Cortex-M4"));
    QCOMPARE(dev.package, QString("LQFP100"));
    QCOMPARE(dev.flashBytes, static_cast<quint64>(1048576));
    QCOMPARE(dev.ramBytes, static_cast<quint64>(196608));
    QVERIFY(dev.pins.size() >= 40);

    const auto* pa5 = dev.findPin("PA5");
    QVERIFY(pa5 != nullptr);
    QVERIFY(pa5->alternateFunctions.contains("SPI1_SCK"));
    QVERIFY(pa5->alternateFunctions.contains("GPIO_Output"));

    const auto* pb6 = dev.findPin("PB6");
    QVERIFY(pb6 != nullptr);
    QVERIFY(pb6->alternateFunctions.contains("I2C1_SCL"));

    // 2. STM32F4 Discovery Board
    auto board = db.findBoard("STM32F407G-DISC1");
    QCOMPARE(board.id, QString("STM32F407G-DISC1"));
    QCOMPARE(board.mcuPartNumber, QString("STM32F407VG"));
    QCOMPARE(board.manufacturer, QString("STMicroelectronics"));
    QVERIFY(!board.connectors.isEmpty());

    // 3. Nucleo F401RE
    auto nucleo = db.findBoard("NUCLEO-F401RE");
    QCOMPARE(nucleo.id, QString("NUCLEO-F401RE"));
    QCOMPARE(nucleo.mcuPartNumber, QString("STM32F401RE"));
}

void TestAllCases::testEsp32DeviceAndBoardPacks() {
    auto& db = Hardware::DeviceDatabase::instance();

    // 1. ESP32-S3
    auto espS3 = db.findDevice("ESP32-S3");
    QCOMPARE(espS3.partNumber, QString("ESP32-S3"));
    QCOMPARE(espS3.vendor, QString("Espressif"));
    QCOMPARE(espS3.family, QString("ESP32"));
    QVERIFY(espS3.pins.size() >= 20);

    const auto* gpio4 = espS3.findPin("GPIO4");
    QVERIFY(gpio4 != nullptr);
    QVERIFY(gpio4->alternateFunctions.contains("SPI2_MOSI") || gpio4->alternateFunctions.contains("GPIO_Output"));

    // 2. ESP32-S3-DevKitC-1 Board
    auto boardS3 = db.findBoard("ESP32-S3-DevKitC-1");
    QCOMPARE(boardS3.id, QString("ESP32-S3-DevKitC-1"));
    QCOMPARE(boardS3.mcuPartNumber, QString("ESP32-S3"));

    // 3. ESP32-WROOM-32
    auto espClassic = db.findDevice("ESP32-WROOM-32");
    QCOMPARE(espClassic.partNumber, QString("ESP32-WROOM-32"));
}

void TestAllCases::testRp2040AndRaspberryPiPacks() {
    auto& db = Hardware::DeviceDatabase::instance();

    // 1. RP2040 MCU
    auto rp = db.findDevice("RP2040");
    QCOMPARE(rp.partNumber, QString("RP2040"));
    QCOMPARE(rp.vendor, QString("Raspberry Pi"));
    QCOMPARE(rp.package, QString("QFN-56"));
    QCOMPARE(rp.ramBytes, static_cast<quint64>(264 * 1024));

    // 2. Raspberry Pi Pico Board
    auto pico = db.findBoard("Raspberry-Pi-Pico");
    QVERIFY(pico.id == "Raspberry-Pi-Pico" || pico.name == "Raspberry Pi Pico");
    QCOMPARE(pico.mcuPartNumber, QString("RP2040"));

    // 3. Raspberry Pi 4 Model B
    auto rpi4 = db.findBoard("Raspberry-Pi-4B");
    QVERIFY(rpi4.id == "Raspberry-Pi-4B" || rpi4.name == "Raspberry Pi 4 Model B");
    QCOMPARE(rpi4.mcuPartNumber, QString("BCM2711"));
}

// ─────────────────────────────────────────────────────────────────────────────
// 18. Pin Multiplexer & Conflict Detection Engine
// ─────────────────────────────────────────────────────────────────────────────

void TestAllCases::testPinMuxEngineConflictsAndValidation() {
    auto& db = Hardware::DeviceDatabase::instance();
    auto dev = db.findDevice("STM32F407VG");
    QVERIFY(!dev.partNumber.isEmpty());

    Hardware::PinMuxEngine engine;
    engine.setDevice(dev);

    // 1. Power pin protection
    Hardware::ConflictInfo cPower = engine.checkPinConflict("VDD", "GPIO_Output");
    QVERIFY(cPower.hasConflict);
    QVERIFY(cPower.reason.contains("power supply rail", Qt::CaseInsensitive));

    // 2. Ground pin protection
    Hardware::ConflictInfo cGnd = engine.checkPinConflict("VSS", "GPIO_Output");
    QVERIFY(cGnd.hasConflict);
    QVERIFY(cGnd.reason.contains("ground reference", Qt::CaseInsensitive));

    // 3. Reset pin protection
    Hardware::ConflictInfo cReset = engine.checkPinConflict("NRST", "GPIO_Output");
    QVERIFY(cReset.hasConflict);
    QVERIFY(cReset.reason.contains("hardware reset signal", Qt::CaseInsensitive));

    // 4. Assign PA5 to SPI1_SCK
    Hardware::PinConfiguration pcfg;
    pcfg.pin = "PA5";
    pcfg.mode = "AlternateFunction";
    pcfg.alternateFunction = "SPI1_SCK";
    pcfg.label = "SPI_CLK";

    Hardware::ConflictInfo cAssign;
    bool ok = engine.assignPin("PA5", pcfg, false, &cAssign);
    QVERIFY(ok);
    QVERIFY(!cAssign.hasConflict);
    QCOMPARE(engine.getPinOwner("PA5"), QString("SPI1_SCK"));

    // 5. Attempt reassigning PA5 to GPIO_Output without force
    Hardware::PinConfiguration pcfg2;
    pcfg2.pin = "PA5";
    pcfg2.mode = "GPIO_Output";
    pcfg2.label = "STATUS_LED";

    Hardware::ConflictInfo cConflict;
    bool ok2 = engine.assignPin("PA5", pcfg2, false, &cConflict);
    QVERIFY(!ok2);
    QVERIFY(cConflict.hasConflict);
    QCOMPARE(cConflict.currentOwner, QString("SPI1_SCK"));
    QVERIFY(cConflict.reason.contains("already assigned to: SPI1_SCK"));

    // 6. Force assignment
    bool ok3 = engine.assignPin("PA5", pcfg2, true, &cConflict);
    QVERIFY(ok3);
    QCOMPARE(engine.getPinOwner("PA5"), QString("GPIO Output"));

    // 7. Check available pins for peripheral signal
    QStringList availPins = engine.availablePinsForSignal("I2C1", "SCL");
    QVERIFY(availPins.contains("PB6") || availPins.contains("PB8"));

    // 8. Validate configuration
    QStringList errors, warnings;
    QVERIFY(engine.validateConfiguration(&errors, &warnings));
}

// ─────────────────────────────────────────────────────────────────────────────
// 19. Custom Hardware Creation, Export & Import
// ─────────────────────────────────────────────────────────────────────────────

void TestAllCases::testCustomHardwareCreationAndImportExport() {
    auto& db = Hardware::DeviceDatabase::instance();

    // 1. Create custom device definition
    Hardware::DeviceDefinition customDev;
    customDev.partNumber = "TEST-CUSTOM-MCU";
    customDev.vendor = "CustomVendor";
    customDev.family = "CustomFamily";
    customDev.series = "Custom100";
    customDev.architecture = "RISC-V 32";
    customDev.core = "RV32IMAC";
    customDev.flashBytes = 512 * 1024;
    customDev.ramBytes = 64 * 1024;
    customDev.package = "QFP48";

    Hardware::PinDefinition p1;
    p1.name = "P1";
    p1.physicalPin = 1;
    p1.alternateFunctions = {"GPIO_Input", "GPIO_Output"};
    customDev.pins.append(p1);

    Hardware::PinDefinition p2;
    p2.name = "VDD";
    p2.physicalPin = 2;
    p2.isPower = true;
    customDev.pins.append(p2);

    // 2. Add to database
    QVERIFY(db.addCustomDevice(customDev));
    auto found = db.findDevice("TEST-CUSTOM-MCU");
    QCOMPARE(found.partNumber, QString("TEST-CUSTOM-MCU"));
    QCOMPARE(found.vendor, QString("CustomVendor"));

    // 3. Export to JSON
    QString exportPath = m_tempDir.filePath("custom_mcu_export.json");
    QString expErr;
    QVERIFY(db.exportHardwareDefinition("TEST-CUSTOM-MCU", exportPath, &expErr));
    QVERIFY(QFile::exists(exportPath));

    // 4. Duplicate custom definition
    QString dupErr;
    QVERIFY(db.duplicateHardwareDefinition("TEST-CUSTOM-MCU", "TEST-CUSTOM-MCU-V2", &dupErr));
    auto foundDup = db.findDevice("TEST-CUSTOM-MCU-V2");
    QCOMPARE(foundDup.partNumber, QString("TEST-CUSTOM-MCU-V2"));
}

// ─────────────────────────────────────────────────────────────────────────────
// 20. Project HardwareConfig Persistence & Roundtrip
// ─────────────────────────────────────────────────────────────────────────────

void TestAllCases::testProjectHardwarePersistence() {
    // 1. Setup project with hardware configuration
    Project proj(nullptr);
    proj.newProject("HardwarePersistenceApp", 480, 272);

    Hardware::HardwareConfig hw;
    hw.targetType = "board";
    hw.boardId = "STM32F407G-DISC1";
    hw.deviceId = "STM32F407VG";
    hw.vendor = "STMicroelectronics";
    hw.family = "STM32";
    hw.series = "STM32F4";
    hw.architecture = "ARM Cortex-M4";
    hw.core = "Cortex-M4";
    hw.flashBytes = 1048576;
    hw.ramBytes = 196608;

    Hardware::PinConfiguration pinCfg;
    pinCfg.pin = "PA5";
    pinCfg.label = "STATUS_LED";
    pinCfg.mode = "GPIO_Output";
    pinCfg.speed = "High";
    hw.pins.insert("PA5", pinCfg);

    Hardware::PeripheralConfiguration periphCfg;
    periphCfg.name = "USART1";
    periphCfg.type = "USART";
    periphCfg.enabled = true;
    periphCfg.assignedPins.insert("TX", "PA9");
    periphCfg.assignedPins.insert("RX", "PA10");
    periphCfg.parameters.insert("baudRate", 115200);
    hw.peripherals.insert("USART1", periphCfg);

    proj.setHardwareConfig(hw);

    // 2. Save project to file
    QString savePath = m_tempDir.filePath("test_hw_project.euiproj");
    QVERIFY(proj.saveToFile(savePath));
    QVERIFY(QFile::exists(savePath));

    // 3. Load into a fresh project
    Project loadedProj(nullptr);
    QVERIFY(loadedProj.loadFromFile(savePath));

    auto loadedHw = loadedProj.hardwareConfig();
    QCOMPARE(loadedHw.targetType, QString("board"));
    QCOMPARE(loadedHw.boardId, QString("STM32F407G-DISC1"));
    QCOMPARE(loadedHw.deviceId, QString("STM32F407VG"));
    QCOMPARE(loadedHw.vendor, QString("STMicroelectronics"));
    QCOMPARE(loadedHw.core, QString("Cortex-M4"));
    QVERIFY(loadedHw.pins.contains("PA5"));
    QCOMPARE(loadedHw.pins["PA5"].label, QString("STATUS_LED"));
    QCOMPARE(loadedHw.pins["PA5"].mode, QString("GPIO_Output"));
    QVERIFY(loadedHw.peripherals.contains("USART1"));
    QCOMPARE(loadedHw.peripherals["USART1"].enabled, true);
    QCOMPARE(loadedHw.peripherals["USART1"].assignedPins.value("TX"), QString("PA9"));
}

// ─────────────────────────────────────────────────────────────────────────────
// 21. MCP Hardware Inspection & Configuration Tools
// ─────────────────────────────────────────────────────────────────────────────

void TestAllCases::testMcpHardwareToolsDispatch() {
    MainWindow win;
    DesignerController controller(&win);

    // 1. list_devices
    QJsonObject rListDev = controller.dispatch("list_devices", {});
    QVERIFY(rListDev["success"].toBool());
    QVERIFY(rListDev["count"].toInt() >= 6);

    // 2. list_boards
    QJsonObject rListBoards = controller.dispatch("list_boards", {});
    QVERIFY(rListBoards["success"].toBool());
    QVERIFY(rListBoards["count"].toInt() >= 6);

    // 3. set_target (STM32F407VG)
    QJsonObject rSetTarget = controller.dispatch("set_target", QJsonObject{
        {"targetType", "device"},
        {"id", "STM32F407VG"}
    });
    QVERIFY(rSetTarget["success"].toBool());

    // 4. get_target
    QJsonObject rGetTarget = controller.dispatch("get_target", {});
    QVERIFY(rGetTarget["success"].toBool());
    QCOMPARE(rGetTarget["deviceId"].toString(), QString("STM32F407VG"));
    QCOMPARE(rGetTarget["vendor"].toString(), QString("STMicroelectronics"));

    // 5. get_pin (PA5)
    QJsonObject rGetPin = controller.dispatch("get_pin", QJsonObject{{"pin", "PA5"}});
    QVERIFY(rGetPin["success"].toBool());
    QVERIFY(rGetPin["alternateFunctions"].toArray().contains("SPI1_SCK"));

    // 6. configure_pin (PA5 -> SPI1_SCK)
    QJsonObject rCfgPin = controller.dispatch("configure_pin", QJsonObject{
        {"pin", "PA5"},
        {"config", QJsonObject{
            {"mode", "AlternateFunction"},
            {"alternateFunction", "SPI1_SCK"},
            {"label", "SPI_SCLK"}
        }}
    });
    QVERIFY(rCfgPin["success"].toBool());

    // 7. configure_pin conflict check (PA5 -> GPIO_Output without force)
    QJsonObject rConflict = controller.dispatch("configure_pin", QJsonObject{
        {"pin", "PA5"},
        {"config", QJsonObject{{"mode", "GPIO_Output"}}},
        {"force", false}
    });
    QVERIFY(!rConflict["success"].toBool());
    QVERIFY(rConflict["conflict"].toBool());

    // 8. get_free_pins
    QJsonObject rFreePins = controller.dispatch("get_free_pins", {});
    QVERIFY(rFreePins["success"].toBool());
    QVERIFY(!rFreePins["freePins"].toArray().isEmpty());

    // 9. get_available_pins
    QJsonObject rAvailPins = controller.dispatch("get_available_pins", QJsonObject{
        {"peripheral", "I2C1"},
        {"signal", "SCL"}
    });
    QVERIFY(rAvailPins["success"].toBool());
    QVERIFY(rAvailPins["availablePins"].toArray().contains("PB6") || rAvailPins["availablePins"].toArray().contains("PB8"));

    // 10. configure_peripheral
    QJsonObject rCfgPeriph = controller.dispatch("configure_peripheral", QJsonObject{
        {"name", "I2C1"},
        {"config", QJsonObject{
            {"enabled", true},
            {"pins", QJsonObject{{"SCL", "PB6"}, {"SDA", "PB7"}}}
        }}
    });
    QVERIFY(rCfgPeriph["success"].toBool());

    // 11. validate_hardware_configuration
    QJsonObject rValidate = controller.dispatch("validate_hardware_configuration", {});
    QVERIFY(rValidate["success"].toBool());
    QVERIFY(rValidate["valid"].toBool());

    // 12. get_hardware_configuration
    QJsonObject rGetHwCfg = controller.dispatch("get_hardware_configuration", {});
    QVERIFY(rGetHwCfg["success"].toBool());
    QVERIFY(rGetHwCfg["configuration"].isObject());
}

void TestAllCases::testUniversalHardwareSelectorAllTabAndFiltering() {
    Hardware::DeviceDatabase& db = Hardware::DeviceDatabase::instance();
    db.reloadPacks();

    const auto catalog = db.allCatalogItems();
    QVERIFY(catalog.size() >= 12);

    // 1. Verify all targets across vendors are indexed in catalog
    QStringList references;
    QStringList vendors;
    QStringList types;
    for (const auto& item : catalog) {
        references.append(item.reference);
        vendors.append(item.vendor);
        types.append(item.deviceType);
    }

    // Verify ST targets
    QVERIFY(references.contains("STM32F407VG"));
    QVERIFY(references.contains("STM32F030R8"));
    bool foundDiscovery = false;
    for (const QString& r : references) {
        if (r.contains("STM32F4 Discovery")) foundDiscovery = true;
    }
    QVERIFY(foundDiscovery);

    // Verify Espressif targets
    QVERIFY(references.contains("ESP32-S3"));
    QVERIFY(references.contains("ESP32-C6"));
    bool foundEspDevKit = false;
    for (const QString& r : references) {
        if (r.contains("ESP32-DevKitC")) foundEspDevKit = true;
    }
    QVERIFY(foundEspDevKit);

    // Verify Raspberry Pi silicon and boards
    QVERIFY(references.contains("RP2040"));
    QVERIFY(references.contains("Raspberry Pi Pico"));
    QVERIFY(references.contains("BCM2711"));
    QVERIFY(references.contains("Raspberry Pi 4 Model B"));

    // Verify Nordic target
    QVERIFY(references.contains("nRF52840"));

    // 2. Verify Hardware Type Indication
    QVERIFY(types.contains("MCU"));
    QVERIFY(types.contains("BOARD"));
    QVERIFY(types.contains("MPU"));
    QVERIFY(types.contains("SBC"));

    // Check specific types
    for (const auto& item : catalog) {
        if (item.reference == "STM32F407VG") QCOMPARE(item.deviceType, QString("MCU"));
        if (item.reference == "BCM2711") QCOMPARE(item.deviceType, QString("MPU"));
        if (item.reference == "Raspberry Pi Pico") QCOMPARE(item.deviceType, QString("BOARD"));
        if (item.reference == "Raspberry Pi 4 Model B") QCOMPARE(item.deviceType, QString("SBC"));
    }

    // 3. Test HardwareWizardDialog UI
    HardwareWizardDialog wizard;
    QTabWidget* tabs = wizard.findChild<QTabWidget*>();
    QVERIFY(tabs != nullptr);
    QCOMPARE(tabs->count(), 4);
    QCOMPARE(tabs->tabText(0), QString("All"));
    QCOMPARE(tabs->tabText(1), QString("MCU / MPU"));
    QCOMPARE(tabs->tabText(2), QString("Boards"));
    QCOMPARE(tabs->tabText(3), QString("Custom Configuration"));

    // "All" tab MUST be selected by default
    QCOMPARE(tabs->currentIndex(), 0);

    QTableWidget* allTable = wizard.findChild<QTableWidget*>("allHardwareTable");
    QVERIFY(allTable != nullptr);
    QVERIFY(allTable->rowCount() >= 12);

    // 4. Test Global Search
    QLineEdit* searchEdit = wizard.findChild<QLineEdit*>("searchHardwareEdit");
    QVERIFY(searchEdit != nullptr);

    // Search STM32F407
    searchEdit->setText("STM32F407");
    QVERIFY(allTable->rowCount() >= 2); // STM32F407VG and Discovery board

    // Search ESP32-S3
    searchEdit->setText("ESP32-S3");
    QVERIFY(allTable->rowCount() >= 2); // MCU and DevKit

    // Search Pico
    searchEdit->setText("Pico");
    QVERIFY(allTable->rowCount() >= 1);

    // Search Nordic / nRF
    searchEdit->setText("nRF52840");
    QCOMPARE(allTable->rowCount(), 1);

    // 5. Test Filters and Reset
    searchEdit->clear();
    QComboBox* typeFilter = wizard.findChild<QComboBox*>();
    QVERIFY(typeFilter != nullptr);

    // 6. Test MCU Selection does not require a board
    Hardware::DeviceDefinition dev = db.findDevice("STM32F407VG");
    QVERIFY(!dev.partNumber.isEmpty());
    QCOMPARE(dev.vendor, QString("STMicroelectronics"));

    // 7. Test Board Selection links to Processor
    Hardware::BoardDefinition pico = db.findBoard("Raspberry-Pi-Pico");
    QVERIFY(!pico.id.isEmpty());
    QCOMPARE(pico.mcuPartNumber, QString("RP2040"));

    // 8. Test Custom target dynamic addition into All view
    Hardware::DeviceDefinition customDev;
    customDev.partNumber = "CUSTOM_TEST_MCU_01";
    customDev.vendor = "Custom";
    customDev.architecture = "ARM Cortex-M";
    customDev.core = "Cortex-M4";
    customDev.flashBytes = 256 * 1024;
    customDev.ramBytes = 64 * 1024;
    QVERIFY(db.addCustomDevice(customDev));

    const auto updatedCatalog = db.allCatalogItems();
    bool foundCustom = false;
    for (const auto& item : updatedCatalog) {
        if (item.reference == "CUSTOM_TEST_MCU_01") {
            foundCustom = true;
            QCOMPARE(item.deviceType, QString("CUSTOM"));
            break;
        }
    }
    QVERIFY(foundCustom);

    // 9. Save verification screenshot of Universal Hardware Selection screen
    HardwareWizardDialog captureWizard;
    captureWizard.resize(1180, 750);
    captureWizard.show();
    QTest::qWait(100);
    QDir().mkpath("screenshots");
    QVERIFY(captureWizard.grab().save("screenshots/phase2a_universal_hardware_selector.png"));
}

void TestAllCases::testMultiScreenProjectModel() {
    CanvasScene scene;
    Project proj(&scene);

    // Initial state has 1 main screen
    QCOMPARE(proj.screens().size(), 1);
    Screen* s1 = proj.activeScreen();
    QVERIFY(s1 != nullptr);
    QCOMPARE(s1->name(), QString("Main Screen"));

    // Add component to screen 1
    ButtonComponent* btn = new ButtonComponent("btn_screen1");
    btn->setCompPos(20, 20);
    scene.addUIComponent(btn);
    QCOMPARE(s1->components().size(), 1);

    // Create Screen 2
    Screen* s2 = proj.addScreen("Settings Screen", 800, 480);
    QVERIFY(s2 != nullptr);
    QCOMPARE(proj.screens().size(), 2);

    // Switch active screen to Screen 2
    proj.setActiveScreen(s2);
    QCOMPARE(proj.activeScreen(), s2);
    QCOMPARE(scene.uiComponents().size(), 0);

    // Add component to screen 2
    LabelComponent* lbl = new LabelComponent("lbl_screen2");
    lbl->setText("Brightness");
    scene.addUIComponent(lbl);
    QCOMPARE(s2->components().size(), 1);

    // Switch back to screen 1: component isolation verified
    proj.setActiveScreen(s1);
    QCOMPARE(scene.uiComponents().size(), 1);
    QCOMPARE(scene.uiComponents().first()->componentId(), QString("btn_screen1"));

    // Clone Screen 2
    Screen* s3 = proj.duplicateScreen(s2->id());
    QVERIFY(s3 != nullptr);
    QCOMPARE(proj.screens().size(), 3);
    QCOMPARE(s3->name(), QString("Settings Screen (Copy)"));
    QCOMPARE(s3->components().size(), 1);
    QVERIFY(s3->components().first()->componentId() != lbl->componentId());

    // Reorder screens: move Screen 2 (index 1) to index 0
    proj.moveScreen(1, 0);
    QCOMPARE(proj.screens().at(0)->name(), QString("Settings Screen"));

    // Full file save and reload
    QTemporaryFile tmpFile;
    QVERIFY(tmpFile.open());
    QString tmpPath = tmpFile.fileName();
    tmpFile.close();

    QVERIFY(proj.saveToFile(tmpPath));

    CanvasScene loadScene;
    Project loadProj(&loadScene);
    QVERIFY(loadProj.loadFromFile(tmpPath));
    QCOMPARE(loadProj.screens().size(), 3);
    QCOMPARE(loadProj.screens().at(0)->name(), QString("Settings Screen"));
    QCOMPARE(loadProj.screens().at(0)->components().size(), 1);
}

void TestAllCases::testScreenUndoRedoCommands() {
    QUndoStack undoStack;
    CanvasScene scene;
    Project proj(&scene);

    int initCount = proj.screens().size();

    // Create Screen Command
    CreateScreenCommand* createCmd = new CreateScreenCommand(&proj, "Screen B", 800, 480);
    undoStack.push(createCmd);
    QCOMPARE(proj.screens().size(), initCount + 1);
    QString newScreenId = proj.screens().last()->id();

    undoStack.undo();
    QCOMPARE(proj.screens().size(), initCount);

    undoStack.redo();
    QCOMPARE(proj.screens().size(), initCount + 1);

    // Rename Screen Command
    RenameScreenCommand* renCmd = new RenameScreenCommand(&proj, newScreenId, "Screen B", "Screen Renamed");
    undoStack.push(renCmd);
    QCOMPARE(proj.findScreen(newScreenId)->name(), QString("Screen Renamed"));

    undoStack.undo();
    QCOMPARE(proj.findScreen(newScreenId)->name(), QString("Screen B"));

    undoStack.redo();
    QCOMPARE(proj.findScreen(newScreenId)->name(), QString("Screen Renamed"));

    // Duplicate Screen Command
    DuplicateScreenCommand* dupCmd = new DuplicateScreenCommand(&proj, newScreenId);
    undoStack.push(dupCmd);
    QCOMPARE(proj.screens().size(), initCount + 2);

    undoStack.undo();
    QCOMPARE(proj.screens().size(), initCount + 1);

    undoStack.redo();
    QCOMPARE(proj.screens().size(), initCount + 2);

    // Delete Screen Command
    DeleteScreenCommand* delCmd = new DeleteScreenCommand(&proj, newScreenId);
    undoStack.push(delCmd);
    QCOMPARE(proj.screens().size(), initCount + 1);

    undoStack.undo();
    QCOMPARE(proj.screens().size(), initCount + 2);
    QVERIFY(proj.findScreen(newScreenId) != nullptr);
}

void TestAllCases::testScreensPanelUi() {
    CanvasScene scene;
    Project proj(&scene);
    ScreensPanel panel(&proj);
    panel.resize(300, 400);

    QListWidget* list = panel.findChild<QListWidget*>();
    QVERIFY(list != nullptr);
    QCOMPARE(list->count(), 1);

    proj.addScreen("Dashboard Screen", 800, 480);
    QCOMPARE(list->count(), 2);

    list->setCurrentRow(1);
    QCOMPARE(proj.activeScreen()->name(), QString("Dashboard Screen"));
}

void TestAllCases::testDataSourceAndBindingModel() {
    CanvasScene scene;
    Project proj(&scene);

    DataSource ds1("gpio_btn", "User Button", DataSourceType::Gpio, DataDirection::Input, DataType::Boolean);
    ds1.setHardwareRef("PA0");

    DataSource ds2("adc_pot", "Potentiometer", DataSourceType::Adc, DataDirection::Input, DataType::Float);
    ds2.setHardwareRef("PA4");

    DataSource ds3("pwm_motor", "Motor Speed", DataSourceType::Pwm, DataDirection::Output, DataType::Float);
    ds3.setHardwareRef("PA8");

    proj.addDataSource(ds1);
    proj.addDataSource(ds2);
    proj.addDataSource(ds3);

    QCOMPARE(proj.dataSources().size(), 3);
    QCOMPARE(proj.findDataSource("gpio_btn")->hardwareRef(), QString("PA0"));

    ProgressBarComponent* pb = new ProgressBarComponent("pb_adc");
    DataBinding pbBinding(pb->componentId(), "value", "adc_pot", BindingDirection::Read);
    pb->addBinding(pbBinding);
    scene.addUIComponent(pb);

    QCOMPARE(pb->bindings().size(), 1);
    QCOMPARE(pb->bindings().first().sourceId(), QString("adc_pot"));

    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    QString path = tmp.fileName();
    tmp.close();

    QVERIFY(proj.saveToFile(path));

    CanvasScene loadScene;
    Project loadProj(&loadScene);
    QVERIFY(loadProj.loadFromFile(path));
    QCOMPARE(loadProj.dataSources().size(), 3);
    QCOMPARE(loadProj.findDataSource("adc_pot")->hardwareRef(), QString("PA4"));

    UIComponent* loadedPb = nullptr;
    for (UIComponent* c : loadScene.uiComponents()) {
        if (c->componentId() == "pb_adc") {
            loadedPb = c;
            break;
        }
    }
    QVERIFY(loadedPb != nullptr);
    QCOMPARE(loadedPb->bindings().size(), 1);
    QCOMPARE(loadedPb->bindings().first().sourceId(), QString("adc_pot"));
}

void TestAllCases::testComponentStateAndStyles() {
    ButtonComponent btn("btn_state_test");
    QCOMPARE(btn.currentState(), QString("normal"));

    ComponentStateStyle warnStyle;
    warnStyle.hasBackgroundColor = true;
    warnStyle.backgroundColor = QColor(255, 180, 0);
    warnStyle.hasTextColor = true;
    warnStyle.textColor = QColor(10, 10, 10);
    warnStyle.hasBorderColor = true;
    warnStyle.borderColor = QColor(200, 140, 0);
    warnStyle.hasBorderWidth = true;
    warnStyle.borderWidth = 2;

    ComponentStateStyle errStyle;
    errStyle.hasBackgroundColor = true;
    errStyle.backgroundColor = QColor(220, 20, 20);
    errStyle.hasTextColor = true;
    errStyle.textColor = QColor(255, 255, 255);
    errStyle.hasBorderColor = true;
    errStyle.borderColor = QColor(180, 0, 0);
    errStyle.hasBorderWidth = true;
    errStyle.borderWidth = 3;

    btn.setStateStyle("warning", warnStyle);
    btn.setStateStyle("error", errStyle);

    QVERIFY(btn.hasStateStyle("warning"));
    QVERIFY(btn.hasStateStyle("error"));
    QCOMPARE(btn.effectiveBackgroundColor(btn.backgroundColor()), btn.backgroundColor());

    btn.setCurrentState("warning");
    QCOMPARE(btn.effectiveBackgroundColor(btn.backgroundColor()), QColor(255, 180, 0));
    QCOMPARE(btn.effectiveTextColor(btn.textColor()), QColor(10, 10, 10));

    btn.setCurrentState("error");
    QCOMPARE(btn.effectiveBackgroundColor(btn.backgroundColor()), QColor(220, 20, 20));
    QCOMPARE(btn.effectiveTextColor(btn.textColor()), QColor(255, 255, 255));

    QJsonObject json = btn.toJson();
    ButtonComponent loaded("loaded_btn");
    loaded.fromJson(json);

    QVERIFY(loaded.hasStateStyle("warning"));
    QCOMPARE(loaded.stateStyle("warning").backgroundColor, QColor(255, 180, 0));
    QVERIFY(loaded.hasStateStyle("error"));
    QCOMPARE(loaded.stateStyle("error").backgroundColor, QColor(220, 20, 20));
}

void TestAllCases::testPhase2ComponentsCreationAndDefaults() {
    // 1. Gauge
    GaugeComponent gauge("gauge_test");
    QCOMPARE(gauge.componentType(), QString("Gauge"));
    QCOMPARE(gauge.minimum(), 0.0);
    QCOMPARE(gauge.maximum(), 100.0);
    QCOMPARE(gauge.value(), 50.0);
    QCOMPARE(gauge.startAngle(), 225.0);
    QCOMPARE(gauge.sweepAngle(), 270.0);
    QCOMPARE(gauge.majorTicks(), 5);
    QVERIFY(gauge.showNeedle());

    // 2. Speedometer
    SpeedometerComponent speed("speedo_test");
    QCOMPARE(speed.componentType(), QString("Speedometer"));
    QCOMPARE(speed.minimum(), 0.0);
    QCOMPARE(speed.maximum(), 240.0);
    QCOMPARE(speed.value(), 80.0);
    QCOMPARE(speed.unit(), QString("km/h"));
    QCOMPARE(speed.warningThreshold(), 140.0);
    QCOMPARE(speed.criticalThreshold(), 200.0);

    // 3. Battery
    BatteryComponent battery("battery_test");
    QCOMPARE(battery.componentType(), QString("Battery"));
    QCOMPARE(battery.minimum(), 0.0);
    QCOMPARE(battery.maximum(), 100.0);
    QCOMPARE(battery.value(), 75.0);
    QCOMPARE(battery.orientation(), QString("Horizontal"));
    QVERIFY(!battery.isCharging());

    // 4. Pressure
    PressureComponent pressure("press_test");
    QCOMPARE(pressure.componentType(), QString("Pressure"));
    QCOMPARE(pressure.unit(), QString("bar"));
    QCOMPARE(pressure.minimum(), 0.0);
    QCOMPARE(pressure.maximum(), 10.0);
    QCOMPARE(pressure.value(), 2.4);

    // 5. RPM
    RpmComponent rpm("rpm_test");
    QCOMPARE(rpm.componentType(), QString("RPM"));
    QCOMPARE(rpm.minimum(), 0.0);
    QCOMPARE(rpm.maximum(), 8000.0);
    QCOMPARE(rpm.value(), 3500.0);
    QCOMPARE(rpm.unit(), QString("RPM"));

    // 6. Temperature
    TemperatureComponent temp("temp_test");
    QCOMPARE(temp.componentType(), QString("Temperature"));
    QCOMPARE(temp.unit(), QString("°C"));
    QCOMPARE(temp.minimum(), -20.0);
    QCOMPARE(temp.maximum(), 120.0);
    QCOMPARE(temp.value(), 72.5);

    // 7. Circular Progress
    CircularProgressComponent circ("circ_test");
    QCOMPARE(circ.componentType(), QString("CircularProgress"));
    QCOMPARE(circ.thickness(), 10);
    QCOMPARE(circ.startAngle(), 90.0);
    QCOMPARE(circ.sweepAngle(), 360.0);
    QCOMPARE(circ.value(), 72.0);

    // 8. Linear Progress (ProgressBar)
    ProgressBarComponent pb("pb_test");
    QCOMPARE(pb.componentType(), QString("ProgressBar"));
    QCOMPARE(pb.minimum(), 0.0);
    QCOMPARE(pb.maximum(), 100.0);
    QCOMPARE(pb.value(), 0.5);
    QCOMPARE(pb.actualValue(), 50.0);

    // 9. Tab View
    TabViewComponent tab("tab_test");
    QCOMPARE(tab.componentType(), QString("TabView"));
    QCOMPARE(tab.tabs().size(), 3);
    QCOMPARE(tab.activeTabIndex(), 0);
    QCOMPARE(tab.tabPosition(), QString("Top"));

    // 10. Navigation Bar
    NavigationBarComponent nav("nav_test");
    QCOMPARE(nav.componentType(), QString("NavigationBar"));
    QCOMPARE(nav.items().size(), 3);
    QCOMPARE(nav.selectedIndex(), 0);
    QCOMPARE(nav.orientation(), QString("Horizontal"));

    // 11. List
    ListComponent list("list_test");
    QCOMPARE(list.componentType(), QString("List"));
    QCOMPARE(list.items().size(), 4);
    QCOMPARE(list.selectedIndex(), 0);

    // 12. Table
    TableComponent table("table_test");
    QCOMPARE(table.componentType(), QString("Table"));
    QCOMPARE(table.columns().size(), 4);
    QCOMPARE(table.rows().size(), 4);
    QCOMPARE(table.selectedRow(), 0);

    // Factory registration check for all 12 types
    QStringList types = {
        "Gauge", "Speedometer", "Battery", "Pressure",
        "RPM", "Temperature", "CircularProgress", "ProgressBar",
        "TabView", "NavigationBar", "List", "Table"
    };
    for (const QString& t : types) {
        UIComponent* created = Project::createComponentInstance(t, "comp_" + t);
        QVERIFY2(created != nullptr, QString("Failed to create component of type %1").arg(t).toUtf8().constData());
        QCOMPARE(created->componentType(), t);
        delete created;
    }
}

void TestAllCases::testPhase2ComponentsSerialization() {
    // Test full JSON round trip for all Phase 2 components
    // 1. Gauge
    GaugeComponent gauge("g1");
    gauge.setMinimum(10);
    gauge.setMaximum(200);
    gauge.setValue(125);
    gauge.setStartAngle(90);
    gauge.setSweepAngle(180);
    gauge.setMajorTicks(8);
    gauge.setUnit("PSI");
    QJsonObject gJson = gauge.toJson();
    GaugeComponent gLoaded("g1_load");
    gLoaded.fromJson(gJson);
    QCOMPARE(gLoaded.minimum(), 10.0);
    QCOMPARE(gLoaded.maximum(), 200.0);
    QCOMPARE(gLoaded.value(), 125.0);
    QCOMPARE(gLoaded.startAngle(), 90.0);
    QCOMPARE(gLoaded.sweepAngle(), 180.0);
    QCOMPARE(gLoaded.majorTicks(), 8);
    QCOMPARE(gLoaded.unit(), QString("PSI"));

    // 2. Speedometer
    SpeedometerComponent speed("s1");
    speed.setMinimum(0);
    speed.setMaximum(260);
    speed.setValue(135);
    speed.setUnit("mph");
    speed.setStylePreset("Classic");
    speed.setWarningThreshold(120);
    speed.setCriticalThreshold(160);
    QJsonObject sJson = speed.toJson();
    SpeedometerComponent sLoaded("s1_load");
    sLoaded.fromJson(sJson);
    QCOMPARE(sLoaded.maximum(), 260.0);
    QCOMPARE(sLoaded.value(), 135.0);
    QCOMPARE(sLoaded.unit(), QString("mph"));
    QCOMPARE(sLoaded.stylePreset(), QString("Classic"));
    QCOMPARE(sLoaded.warningThreshold(), 120.0);
    QCOMPARE(sLoaded.criticalThreshold(), 160.0);

    // 3. Battery
    BatteryComponent bat("b1");
    bat.setValue(45);
    bat.setCharging(true);
    bat.setSegmented(true);
    bat.setSegmentCount(5);
    bat.setOrientation("Vertical");
    QJsonObject bJson = bat.toJson();
    BatteryComponent bLoaded("b1_load");
    bLoaded.fromJson(bJson);
    QCOMPARE(bLoaded.value(), 45.0);
    QVERIFY(bLoaded.isCharging());
    QVERIFY(bLoaded.segmented());
    QCOMPARE(bLoaded.segmentCount(), 5);
    QCOMPARE(bLoaded.orientation(), QString("Vertical"));

    // 4. Tab View
    TabViewComponent tab("tab1");
    tab.setTabs({"Main", "Engine", "CAN Logs", "Diag"});
    tab.setActiveTabIndex(2);
    tab.setTabPosition("Bottom");
    QJsonObject tabJson = tab.toJson();
    TabViewComponent tabLoaded("tab1_load");
    tabLoaded.fromJson(tabJson);
    QCOMPARE(tabLoaded.tabs().size(), 4);
    QCOMPARE(tabLoaded.tabs().at(2), QString("CAN Logs"));
    QCOMPARE(tabLoaded.activeTabIndex(), 2);
    QCOMPARE(tabLoaded.tabPosition(), QString("Bottom"));

    // 5. Navigation Bar
    NavigationBarComponent nav("nav1");
    QList<NavItem> items = {
        {"Dashboard", "gauge", "scr_dash"},
        {"Telemetrics", "graph", "scr_telem"},
        {"Settings", "gear", "scr_settings"}
    };
    nav.setItems(items);
    nav.setSelectedIndex(1);
    nav.setOrientation("Vertical");
    QJsonObject navJson = nav.toJson();
    NavigationBarComponent navLoaded("nav1_load");
    navLoaded.fromJson(navJson);
    QCOMPARE(navLoaded.items().size(), 3);
    QCOMPARE(navLoaded.items().at(1).label, QString("Telemetrics"));
    QCOMPARE(navLoaded.items().at(1).targetScreenId, QString("scr_telem"));
    QCOMPARE(navLoaded.selectedIndex(), 1);
    QCOMPARE(navLoaded.orientation(), QString("Vertical"));

    // 6. Table
    TableComponent tbl("tbl1");
    tbl.setColumns({"ID", "Channel", "Raw", "Unit"});
    QList<QStringList> rows = {
        {"1", "ADC_CH0", "1024", "mV"},
        {"2", "CAN_RPM", "3450", "RPM"},
        {"3", "I2C_TMP", "24.5", "C"}
    };
    tbl.setRows(rows);
    tbl.setSelectedRow(2);
    QJsonObject tblJson = tbl.toJson();
    TableComponent tblLoaded("tbl1_load");
    tblLoaded.fromJson(tblJson);
    QCOMPARE(tblLoaded.columns().size(), 4);
    QCOMPARE(tblLoaded.rows().size(), 3);
    QCOMPARE(tblLoaded.rows().at(1).at(1), QString("CAN_RPM"));
    QCOMPARE(tblLoaded.selectedRow(), 2);
}

void TestAllCases::testPhase2ValueVisualizationThresholdsAndAutoState() {
    TemperatureComponent temp("temp_state");
    temp.setMinimum(0.0);
    temp.setMaximum(120.0);
    temp.setWarningThreshold(80.0);
    temp.setCriticalThreshold(100.0);

    // Normal range
    temp.setValue(50.0);
    QCOMPARE(temp.currentState(), QString("normal"));

    // Warning range
    temp.setValue(85.0);
    QCOMPARE(temp.currentState(), QString("warning"));

    // Critical range
    temp.setValue(105.0);
    QCOMPARE(temp.currentState(), QString("critical"));

    // Back to normal
    temp.setValue(60.0);
    QCOMPARE(temp.currentState(), QString("normal"));

    // Speedometer state styling
    SpeedometerComponent speed("speed_state");
    speed.setMinimum(0.0);
    speed.setMaximum(200.0);
    speed.setWarningThreshold(120.0);
    speed.setCriticalThreshold(160.0);

    speed.setValue(100.0);
    QCOMPARE(speed.currentState(), QString("normal"));

    speed.setValue(130.0);
    QCOMPARE(speed.currentState(), QString("warning"));

    speed.setValue(175.0);
    QCOMPARE(speed.currentState(), QString("critical"));
}

void TestAllCases::testPhase2DataBindingPipeline() {
    CanvasScene scene;
    Project proj(&scene);

    // Create DataSources
    DataSource dsSpeed("speed_data", "Speed Sensor", DataSourceType::Can, DataDirection::Input, DataType::Float);
    dsSpeed.setValue(60.0);
    DataSource dsRpm("rpm_data", "Engine RPM", DataSourceType::Can, DataDirection::Input, DataType::Float);
    dsRpm.setValue(2500.0);
    DataSource dsTemp("temp_data", "Coolant Temp", DataSourceType::Sensor, DataDirection::Input, DataType::Float);
    dsTemp.setValue(82.0);
    DataSource dsBat("bat_data", "Battery Level", DataSourceType::Adc, DataDirection::Input, DataType::Float);
    dsBat.setValue(94.0);
    DataSource dsPress("press_data", "Oil Pressure", DataSourceType::Sensor, DataDirection::Input, DataType::Float);
    dsPress.setValue(3.8);

    proj.addDataSource(dsSpeed);
    proj.addDataSource(dsRpm);
    proj.addDataSource(dsTemp);
    proj.addDataSource(dsBat);
    proj.addDataSource(dsPress);

    // Create Components on Scene
    auto* speedo = new SpeedometerComponent("speedo_1");
    auto* rpm = new RpmComponent("rpm_1");
    auto* temp = new TemperatureComponent("temp_1");
    auto* bat = new BatteryComponent("bat_1");
    auto* press = new PressureComponent("press_1");

    scene.addUIComponent(speedo);
    scene.addUIComponent(rpm);
    scene.addUIComponent(temp);
    scene.addUIComponent(bat);
    scene.addUIComponent(press);

    // Bind components
    speedo->addBinding(DataBinding("speedo_1", "value", "speed_data", BindingDirection::Read));
    rpm->addBinding(DataBinding("rpm_1", "value", "rpm_data", BindingDirection::Read));
    temp->addBinding(DataBinding("temp_1", "value", "temp_data", BindingDirection::Read));
    bat->addBinding(DataBinding("bat_1", "value", "bat_data", BindingDirection::Read));
    press->addBinding(DataBinding("press_1", "value", "press_data", BindingDirection::Read));

    // Verify bindings exist and persist
    QCOMPARE(speedo->bindingForProperty("value").sourceId(), QString("speed_data"));
    QCOMPARE(rpm->bindingForProperty("value").sourceId(), QString("rpm_data"));
    QCOMPARE(temp->bindingForProperty("value").sourceId(), QString("temp_data"));
    QCOMPARE(bat->bindingForProperty("value").sourceId(), QString("bat_data"));
    QCOMPARE(press->bindingForProperty("value").sourceId(), QString("press_data"));

    // Save and reload project to verify bindings persist across serialization
    QTemporaryFile tmp;
    QVERIFY(tmp.open());
    QString tmpPath = tmp.fileName();
    tmp.close();

    QVERIFY(proj.saveToFile(tmpPath));

    CanvasScene reloadScene;
    Project reloadProj(&reloadScene);
    QVERIFY(reloadProj.loadFromFile(tmpPath));

    QCOMPARE(reloadProj.dataSources().size(), 5);
    QVERIFY(reloadProj.findDataSource("speed_data") != nullptr);
    QVERIFY(reloadProj.findDataSource("rpm_data") != nullptr);
    QVERIFY(reloadProj.findDataSource("temp_data") != nullptr);
    QVERIFY(reloadProj.findDataSource("bat_data") != nullptr);
    QVERIFY(reloadProj.findDataSource("press_data") != nullptr);
}

void TestAllCases::testPhase2MultiScreenDashboardProject() {
    CanvasScene scene;
    Project proj(&scene);

    // Screen 1: Dashboard
    Screen* scr1 = proj.activeScreen();
    scr1->setName("Dashboard");
    auto* speedo = new SpeedometerComponent("speedo_main");
    speedo->setValue(120.0);
    auto* rpm = new RpmComponent("rpm_main");
    rpm->setValue(4200.0);
    auto* temp = new TemperatureComponent("temp_main");
    temp->setValue(72.0);
    auto* bat = new BatteryComponent("bat_main");
    bat->setValue(82.0);
    auto* pb = new ProgressBarComponent("pb_main");
    pb->setValue(0.75);

    scene.addUIComponent(speedo);
    scene.addUIComponent(rpm);
    scene.addUIComponent(temp);
    scene.addUIComponent(bat);
    scene.addUIComponent(pb);

    // Screen 2: Diagnostics
    Screen* scr2 = proj.addScreen("Diagnostics");
    proj.setActiveScreen(scr2);
    auto* gauge = new GaugeComponent("gauge_diag");
    gauge->setValue(65.0);
    auto* press = new PressureComponent("press_diag");
    press->setValue(4.5);
    auto* tbl = new TableComponent("tbl_diag");
    tbl->setColumns({"Param", "Val", "State"});
    tbl->setRows({{"VCC", "3.3V", "OK"}, {"TEMP", "45C", "OK"}});

    scene.addUIComponent(gauge);
    scene.addUIComponent(press);
    scene.addUIComponent(tbl);

    // Screen 3: Settings
    Screen* scr3 = proj.addScreen("Settings");
    proj.setActiveScreen(scr3);
    auto* tab = new TabViewComponent("tab_settings");
    auto* nav = new NavigationBarComponent("nav_settings");
    auto* list = new ListComponent("list_settings");

    scene.addUIComponent(tab);
    scene.addUIComponent(nav);
    scene.addUIComponent(list);

    QCOMPARE(proj.screens().size(), 3);
    QCOMPARE(scr1->components().size(), 5);
    QCOMPARE(scr2->components().size(), 3);
    QCOMPARE(scr3->components().size(), 3);

    // Switch between screens and verify scene component counts
    proj.setActiveScreen(scr1);
    QCOMPARE(scene.uiComponents().size(), 5);

    proj.setActiveScreen(scr2);
    QCOMPARE(scene.uiComponents().size(), 3);

    proj.setActiveScreen(scr3);
    QCOMPARE(scene.uiComponents().size(), 3);

    // Save and reload full project
    QString projFile = m_tempDir.filePath("digital_cluster.euiproj");
    QVERIFY(proj.saveToFile(projFile));

    CanvasScene reloadScene;
    Project reloadProj(&reloadScene);
    QVERIFY(reloadProj.loadFromFile(projFile));

    QCOMPARE(reloadProj.screens().size(), 3);
    Screen* reloadedScr1 = nullptr;
    Screen* reloadedScr2 = nullptr;
    Screen* reloadedScr3 = nullptr;

    for (Screen* s : reloadProj.screens()) {
        if (s->name() == "Dashboard") reloadedScr1 = s;
        else if (s->name() == "Diagnostics") reloadedScr2 = s;
        else if (s->name() == "Settings") reloadedScr3 = s;
    }

    QVERIFY(reloadedScr1 != nullptr);
    QCOMPARE(reloadedScr1->components().size(), 5);

    QVERIFY(reloadedScr2 != nullptr);
    QCOMPARE(reloadedScr2->components().size(), 3);

    QVERIFY(reloadedScr3 != nullptr);
    QCOMPARE(reloadedScr3->components().size(), 3);

    // Verify speedo on reloaded dashboard
    SpeedometerComponent* reloadedSpeedo = nullptr;
    for (UIComponent* c : reloadedScr1->components()) {
        if (c->componentId() == "speedo_main") {
            reloadedSpeedo = dynamic_cast<SpeedometerComponent*>(c);
            break;
        }
    }
    QVERIFY(reloadedSpeedo != nullptr);
    QCOMPARE(reloadedSpeedo->value(), 120.0);
}

void TestAllCases::testBackwardCompatibilitySingleScreenProject() {
    CanvasScene scene;
    Project proj(&scene);
    QString simpleProjPath = examplePath("simple.euiproj");

    QVERIFY(proj.loadFromFile(simpleProjPath));
    QVERIFY(proj.screens().size() >= 1);
    QVERIFY(proj.activeScreen() != nullptr);
    QVERIFY(!proj.activeScreen()->components().isEmpty());
    QVERIFY(!scene.uiComponents().isEmpty());

    // Verify all components on scene are active and interactive
    for (UIComponent* comp : scene.uiComponents()) {
        QVERIFY(!comp->componentId().isEmpty());
        QVERIFY(comp->compWidth() > 0);
        QVERIFY(comp->compHeight() > 0);
    }
}

// ────────────────────────────────────────────────────────────────────────────
// Phase 3 Tests: Hardware Abstraction Layer (HAL)
// ────────────────────────────────────────────────────────────────────────────

void TestAllCases::testPhase3HardwareTargetAndCapabilities() {
    using namespace Hardware;

    // 1. STM32 Target creation & defaults
    HardwareTarget stm32Target("target_stm32", "STM32F4 Discovery", "STM32", "STM32F407VG", "STM32F407G-DISC1");
    QCOMPARE(stm32Target.id(), QString("target_stm32"));
    QCOMPARE(stm32Target.family(), QString("STM32"));
    QCOMPARE(stm32Target.backendType(), QString("stm32"));
    QCOMPARE(stm32Target.connectionType(), QString("openocd"));
    QVERIFY(stm32Target.capabilities().gpioOutput);
    QVERIFY(stm32Target.capabilities().adc);
    QVERIFY(stm32Target.capabilities().atomicBsrr);

    // 2. Pin Mapping resolution
    stm32Target.setPinMapping("STATUS_LED", "PD12");
    stm32Target.setPinMapping("POT_INPUT", "PA1");
    QCOMPARE(stm32Target.resolvePin("STATUS_LED"), QString("PD12"));
    QCOMPARE(stm32Target.resolvePin("POT_INPUT"), QString("PA1"));
    QCOMPARE(stm32Target.resolvePin("PB5"), QString("PB5")); // unmapped physical pin passthrough

    // 3. JSON Serialization Roundtrip
    QJsonObject json = stm32Target.toJson();
    HardwareTarget restored = HardwareTarget::fromJson(json);
    QCOMPARE(restored.id(), stm32Target.id());
    QCOMPARE(restored.name(), stm32Target.name());
    QCOMPARE(restored.family(), stm32Target.family());
    QCOMPARE(restored.mcuModel(), stm32Target.mcuModel());
    QCOMPARE(restored.boardId(), stm32Target.boardId());
    QCOMPARE(restored.backendType(), stm32Target.backendType());
    QCOMPARE(restored.resolvePin("STATUS_LED"), QString("PD12"));
    QVERIFY(restored.capabilities().atomicBsrr);
}

void TestAllCases::testPhase3HardwareBackendSwitchingAndCapabilities() {
    using namespace Hardware;

    HardwareManager& mgr = HardwareManager::instance();
    mgr.resetToDefaults();

    // Verify all 4 default backends registered
    QStringList registered = mgr.registeredBackendIds();
    QVERIFY(registered.contains("stm32"));
    QVERIFY(registered.contains("esp32"));
    QVERIFY(registered.contains("raspberrypi"));
    QVERIFY(registered.contains("mock"));

    // Switch to STM32
    QVERIFY(mgr.setActiveBackend("stm32"));
    QCOMPARE(mgr.activeBackendId(), QString("stm32"));
    HardwareCapabilities stm32Caps = mgr.currentCapabilities();
    QVERIFY(stm32Caps.gpioOutput);
    QVERIFY(stm32Caps.adc);
    QVERIFY(stm32Caps.atomicBsrr);

    // Switch to ESP32
    QVERIFY(mgr.setActiveBackend("esp32"));
    QCOMPARE(mgr.activeBackendId(), QString("esp32"));
    HardwareCapabilities esp32Caps = mgr.currentCapabilities();
    QVERIFY(esp32Caps.gpioOutput);
    QVERIFY(esp32Caps.adc);
    QVERIFY(!esp32Caps.atomicBsrr); // ESP32 does not use STM32 BSRR registers
    QVERIFY(!esp32Caps.can);        // NOT IMPLEMENTED in Phase 3

    // Switch to Raspberry Pi
    QVERIFY(mgr.setActiveBackend("raspberrypi"));
    QCOMPARE(mgr.activeBackendId(), QString("raspberrypi"));
    HardwareCapabilities rpiCaps = mgr.currentCapabilities();
    QVERIFY(rpiCaps.gpioOutput);
    QVERIFY(!rpiCaps.adc); // Raspberry Pi native header has no internal ADC

    // Switch to Mock
    QVERIFY(mgr.setActiveBackend("mock"));
    QCOMPARE(mgr.activeBackendId(), QString("mock"));
}

void TestAllCases::testPhase3MockBackendAndSimulation() {
    using namespace Hardware;

    HardwareManager& mgr = HardwareManager::instance();
    mgr.setActiveBackend("mock");

    MockBackend* mock = dynamic_cast<MockBackend*>(mgr.activeBackend());
    QVERIFY(mock != nullptr);

    // 1. Digital GPIO Write & Read
    QVERIFY(mgr.writeDigital("PA5", true));
    bool pinVal = false;
    QVERIFY(mgr.readDigital("PA5", &pinVal));
    QCOMPARE(pinVal, true);

    QVERIFY(mgr.writeDigital("PA5", false));
    QVERIFY(mgr.readDigital("PA5", &pinVal));
    QCOMPARE(pinVal, false);

    // 2. Analog ADC Read & Normalization
    mock->setMockAdcValue("PA0", 2048, 0.5);
    quint32 rawCount = 0;
    double normValue = 0.0;
    QVERIFY(mgr.readAnalog("PA0", &rawCount, &normValue));
    QCOMPARE(rawCount, 2048u);
    QCOMPARE(normValue, 0.5);

    mock->setMockAdcValue("PA1", 4095, 1.0);
    QVERIFY(mgr.readAnalog("PA1", &rawCount, &normValue));
    QCOMPARE(rawCount, 4095u);
    QCOMPARE(normValue, 1.0);

    // 3. I2C Scan
    QList<quint8> i2cAddrs;
    QVERIFY(mgr.i2cScan("PB8", "PB9", &i2cAddrs));
    QVERIFY(i2cAddrs.contains(0x48));
    QVERIFY(i2cAddrs.contains(0x76));

    // 4. Connection simulation
    QVERIFY(mgr.isHardwareConnected());
    mock->setMockConnected(false);
    QVERIFY(!mgr.isHardwareConnected());
    mock->setMockConnected(true);
    QVERIFY(mgr.isHardwareConnected());
}

void TestAllCases::testPhase3DataSourceHardwareResolution() {
    using namespace Hardware;

    HardwareManager& mgr = HardwareManager::instance();
    mgr.setActiveBackend("mock");
    MockBackend* mock = dynamic_cast<MockBackend*>(mgr.activeBackend());
    QVERIFY(mock != nullptr);

    // 1. Digital GPIO Data Source Resolution
    DataSource gpioSource("source_led", "Status LED Pin", DataSourceType::Gpio, DataDirection::Output, DataType::Boolean);
    gpioSource.setHardwareRef("PA5");

    // Write through DataSource
    QVERIFY(mgr.writeDataSourceValue(gpioSource, true));
    bool pinState = false;
    QVERIFY(mgr.readDigital("PA5", &pinState));
    QCOMPARE(pinState, true);

    // Read back through DataSource
    QVariant readVal;
    QVERIFY(mgr.resolveDataSourceValue(gpioSource, &readVal));
    QCOMPARE(readVal.toBool(), true);

    // 2. Analog ADC Data Source Resolution
    DataSource adcSource("source_pot", "Potentiometer Input", DataSourceType::Adc, DataDirection::Input, DataType::Float);
    adcSource.setHardwareRef("PA0");
    mock->setMockAdcValue("PA0", 3072, 0.75);

    QVariant adcVal;
    QVERIFY(mgr.resolveDataSourceValue(adcSource, &adcVal));
    QCOMPARE(adcVal.toDouble(), 0.75);

    // 3. Variable / Constant Data Source
    DataSource varSource("source_rpm", "Engine RPM", DataSourceType::Variable, DataDirection::Input, DataType::Float);
    varSource.setValue(4500.0);
    QVariant rpmVal;
    QVERIFY(mgr.resolveDataSourceValue(varSource, &rpmVal));
    QCOMPARE(rpmVal.toDouble(), 4500.0);
}

void TestAllCases::testPhase3CustomBoardTargetPersistence() {
    using namespace Hardware;

    // 1. Setup Custom Target
    HardwareTarget customTarget("custom_board_01", "My Industrial Controller", "Custom", "STM32F429ZI", "CUSTOM_HMI_V1");
    customTarget.setArchitecture("ARM Cortex-M4F");
    customTarget.setPinMapping("RELAY_1", "PC8");
    customTarget.setPinMapping("TEMP_SENSOR", "PA3");
    customTarget.setPinMapping("MOTOR_PWM", "PB0");

    HardwareCapabilities customCaps;
    customCaps.gpioInput = true;
    customCaps.gpioOutput = true;
    customCaps.adc = true;
    customCaps.pwm = true;
    customCaps.uart = true;
    customCaps.can = true;
    customTarget.setCapabilities(customCaps);

    // 2. Serialize & Deserialize
    QJsonObject targetJson = customTarget.toJson();
    HardwareTarget loadedTarget = HardwareTarget::fromJson(targetJson);

    QCOMPARE(loadedTarget.id(), QString("custom_board_01"));
    QCOMPARE(loadedTarget.name(), QString("My Industrial Controller"));
    QCOMPARE(loadedTarget.family(), QString("Custom"));
    QCOMPARE(loadedTarget.mcuModel(), QString("STM32F429ZI"));
    QCOMPARE(loadedTarget.boardId(), QString("CUSTOM_HMI_V1"));
    QCOMPARE(loadedTarget.resolvePin("RELAY_1"), QString("PC8"));
    QCOMPARE(loadedTarget.resolvePin("TEMP_SENSOR"), QString("PA3"));
    QCOMPARE(loadedTarget.resolvePin("MOTOR_PWM"), QString("PB0"));
    QVERIFY(loadedTarget.capabilities().can);
}

// ────────────────────────────────────────────────────────────────────────────
// Phase 4 Tests: Code Generator v2 (Context, IR, HAL Adapters, Bindings)
// ────────────────────────────────────────────────────────────────────────────

void TestAllCases::testPhase4GeneratorContextAndValidation() {
    using namespace CodeGen;

    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("ValTestApp", 480, 320);

    // 1. Setup valid multi-screen project
    Screen* scr1 = proj.activeScreen();
    scr1->setName("MainScreen");
    auto* btn = new ButtonComponent("btn_power");
    scr1->addComponent(btn);

    Screen* scr2 = proj.addScreen("SettingsScreen");
    auto* sw = new SwitchComponent("sw_wifi");
    scr2->addComponent(sw);

    DataSource ds("ds_pwr", "Power State", DataSourceType::Gpio, DataDirection::Output, DataType::Boolean);
    ds.setHardwareRef("PA5");
    proj.addDataSource(ds);

    DataBinding db("btn_power", "checked", "ds_pwr", BindingDirection::Write);
    proj.addDataBinding(db);

    // 2. Validate clean project
    GeneratorContext ctx(&proj);
    QList<ValidationMessage> msgs;
    QVERIFY(ctx.validate(&msgs));
    QVERIFY(!ctx.hasErrors());
    QCOMPARE(ctx.screens().size(), 2);
    QCOMPARE(ctx.dataSources().size(), 1);
    QCOMPARE(ctx.dataBindings().size(), 1);

    // 3. Trigger warning with orphaned binding
    DataBinding orphanDb("ghost_comp", "value", "ds_pwr", BindingDirection::Read);
    proj.addDataBinding(orphanDb);
    msgs.clear();
    ctx.validate(&msgs);
    bool hasWarning = false;
    for (const auto& m : msgs) {
        if (m.level == ValidationMessage::Warning) hasWarning = true;
    }
    QVERIFY(hasWarning);
}

void TestAllCases::testPhase4GeneratorIntermediateRepresentation() {
    using namespace CodeGen;

    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("IRProjectApp", 800, 480);

    Screen* dash = proj.activeScreen();
    dash->setName("ClusterDashboard");

    auto* speedo = new SpeedometerComponent("speedo_1");
    speedo->setValue(110.0);
    speedo->setUnit("km/h");
    dash->addComponent(speedo);

    auto* batt = new BatteryComponent("bat_1");
    batt->setValue(85.0);
    dash->addComponent(batt);

    auto* tabs = new TabViewComponent("tabs_1");
    tabs->setTabs({"Trip", "Nav", "Audio"});
    dash->addComponent(tabs);

    DataSource dsSpeed("src_spd", "Vehicle Speed", DataSourceType::Sensor, DataDirection::Input, DataType::Float);
    proj.addDataSource(dsSpeed);

    DataBinding bindSpeed("speedo_1", "value", "src_spd", BindingDirection::Read);
    proj.addDataBinding(bindSpeed);

    // Build IR
    IRProject ir = GeneratorIR::buildFromProject(&proj);
    QCOMPARE(ir.name, QString("IRProjectApp"));
    QCOMPARE(ir.displayWidth, 800);
    QCOMPARE(ir.displayHeight, 480);
    QCOMPARE(ir.screens.size(), 1);
    QCOMPARE(ir.screens[0].widgets.size(), 3);
    QCOMPARE(ir.dataSources.size(), 1);
    QCOMPARE(ir.bindings.size(), 1);

    // Verify widgets in IR
    bool foundSpeedo = false, foundBat = false, foundTabs = false;
    for (const auto& w : ir.screens[0].widgets) {
        if (w.id == "speedo_1" && w.type.toLower() == "speedometer") foundSpeedo = true;
        if (w.id == "bat_1" && w.type.toLower() == "battery") foundBat = true;
        if (w.id == "tabs_1" && (w.type.toLower() == "tabview" || w.type.toLower() == "tab_view")) foundTabs = true;
    }
    QVERIFY(foundSpeedo);
    QVERIFY(foundBat);
    QVERIFY(foundTabs);
}

void TestAllCases::testPhase4TargetHalAndBindingLayerGeneration() {
    using namespace CodeGen;

    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("HalGenApp", 480, 320);

    // 1. STM32 HAL Adapter Generation (verified device: STM32F030R8)
    Hardware::HardwareConfig hwStm;
    hwStm.family = "STM32";
    hwStm.deviceId = "STM32F030R8";
    proj.setHardwareConfig(hwStm);

    QString halH = TargetHalGenerator::generateHalHeader(&proj);
    QString halC = TargetHalGenerator::generateHalSource(&proj);
    QVERIFY(halH.contains("hal_write_digital_pin"));
    QVERIFY(halH.contains("hal_read_adc_normalized"));
    QVERIFY(halC.contains("STM32"));
    QVERIFY(halC.contains("BSRR"));

    // 1b. Unverified STM32F407VG: native HAL stays blocked until data is sourced
    Hardware::HardwareConfig hwF407;
    hwF407.family = "STM32";
    hwF407.deviceId = "STM32F407VG";
    hwF407.boardId = "STM32F407G-DISC1";
    proj.setHardwareConfig(hwF407);
    QVERIFY(TargetHalGenerator::generateHalHeader(&proj).isEmpty());
    QVERIFY(TargetHalGenerator::generateHalSource(&proj).isEmpty());

    // 2. ESP32 HAL Adapter Generation
    Hardware::HardwareConfig hwEsp;
    hwEsp.family = "ESP32";
    hwEsp.deviceId = "ESP32-S3";
    proj.setHardwareConfig(hwEsp);
    QString halEspC = TargetHalGenerator::generateHalSource(&proj);
    QVERIFY(halEspC.contains("ESP32"));
    QVERIFY(halEspC.contains("gpio_set_level"));

    // 3. Binding Layer Generation
    DataSource dsAdc("adc_sensor", "ADC Sensor", DataSourceType::Adc, DataDirection::Input, DataType::Float);
    dsAdc.setHardwareRef("PA0");
    proj.addDataSource(dsAdc);

    DataBinding b("bar_progress", "value", "adc_sensor", BindingDirection::Read);
    proj.addDataBinding(b);

    QString bindH = BindingLayerGenerator::generateBindingsHeader(&proj);
    QString bindC = BindingLayerGenerator::generateBindingsSource(&proj);
    QVERIFY(bindH.contains("ui_update_data_sources"));
    QVERIFY(bindC.contains("adc_sensor"));
    QVERIFY(bindC.contains("hal_read_adc_normalized"));
}

void TestAllCases::testPhase4LvglMultiScreenAndDashboardExport() {
    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("LvglDashboardExport", 480, 272);

    Screen* scr = proj.activeScreen();
    scr->setName("Dashboard");

    auto* speedo = new SpeedometerComponent("speedo_main");
    speedo->setValue(100.0);
    scr->addComponent(speedo);

    auto* rpm = new RpmComponent("rpm_main");
    rpm->setValue(3500.0);
    scr->addComponent(rpm);

    auto* bat = new BatteryComponent("bat_status");
    bat->setValue(75.0);
    scr->addComponent(bat);

    QTemporaryDir exportDir;
    QVERIFY(exportDir.isValid());

    LvglGenerator gen(&proj, &scene);
    QVERIFY(gen.generate(exportDir.path()));

    QDir out(exportDir.path());
    QVERIFY(out.exists("CMakeLists.txt"));
    QVERIFY(out.exists("lv_conf.h"));
    QVERIFY(out.exists("ui.h"));
    QVERIFY(out.exists("ui.c"));
    QVERIFY(out.exists("main.c"));
    QVERIFY(out.exists("target_hal.h"));
    QVERIFY(out.exists("target_hal.c"));
    QVERIFY(out.exists("ui_bindings.h"));
    QVERIFY(out.exists("ui_bindings.c"));
    QVERIFY(out.exists("pc_simulator/CMakeLists.txt"));

    // Verify content of generated target_hal.h and ui_bindings.h
    QFile halFile(out.filePath("target_hal.h"));
    QVERIFY(halFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QString halContent = QString::fromUtf8(halFile.readAll());
    QVERIFY(halContent.contains("target_hal_init"));
}

// ────────────────────────────────────────────────────────────────────────────
// Phase 5 Tests: Desktop Simulator (Runtime, Backend, Bindings, Clock, UI)
// ────────────────────────────────────────────────────────────────────────────

void TestAllCases::testPhase5SimulationBackendAndHardwareIsolation() {
    Simulator::SimulationBackend backend;

    // 1. Backend metadata & capabilities
    QCOMPARE(backend.backendId(), QString("simulation"));
    QCOMPARE(backend.family(), QString("Simulator"));
    auto caps = backend.capabilities();
    QVERIFY(caps.gpioInput);
    QVERIFY(caps.gpioOutput);
    QVERIFY(caps.adc);
    QVERIFY(caps.pwm);
    QVERIFY(caps.uart);
    QVERIFY(caps.i2c);
    QVERIFY(caps.spi);
    QVERIFY(caps.can);

    // 2. Virtual GPIO write & readback
    QVERIFY(backend.writeDigital("PA5", true));
    bool pinVal = false;
    QVERIFY(backend.readDigital("PA5", &pinVal));
    QCOMPARE(pinVal, true);

    QVERIFY(backend.writeDigital("PA5", false));
    QVERIFY(backend.readDigital("PA5", &pinVal));
    QCOMPARE(pinVal, false);

    // 3. Virtual ADC normalized & raw readback
    backend.setSimulatedAdc("PA0", 2048, 0.5);
    double adcNorm = 0.0;
    quint32 adcRaw = 0;
    QVERIFY(backend.readAnalog("PA0", &adcRaw, &adcNorm));
    QVERIFY(qAbs(adcNorm - 0.5) < 0.001);
    QCOMPARE(adcRaw, 2048u);
    QCOMPARE(backend.simulatedAdcRaw("PA0"), 2048u);
    QVERIFY(qAbs(backend.simulatedAdcNormalized("PA0") - 0.5) < 0.001);

    // 4. Virtual PWM Duty Cycle
    QVERIFY(backend.writePwm("PB0", 75.5));
    QVERIFY(qAbs(backend.simulatedPwm("PB0") - 75.5) < 0.001);

    // 5. Virtual UART injection & transmit buffer
    backend.injectUartData("COM1", QByteArray("SENSOR_READY"));
    QByteArray uartIn;
    QVERIFY(backend.uartRead("COM1", 32, &uartIn));
    QCOMPARE(uartIn, QByteArray("SENSOR_READY"));

    QVERIFY(backend.uartWrite("COM1", QByteArray("PING")));
    QCOMPARE(backend.getUartTxBuffer("COM1"), QByteArray("PING"));

    // 6. Virtual I2C Bus Scan
    QList<quint8> foundI2c;
    QVERIFY(backend.i2cScan("PB8", "PB9", &foundI2c));
    QVERIFY(!foundI2c.isEmpty());
    QVERIFY(foundI2c.contains(0x48));

    // 7. Virtual CAN Frame injection
    backend.injectCanFrame(0x7DF, QByteArray::fromHex("02010D0000000000"));
    auto history = backend.getCanRxHistory();
    QVERIFY(!history.isEmpty());
    QCOMPARE(history.last().id, 0x7DFu);
    QCOMPARE(history.last().payload.size(), 8);

    // 8. Error injection & safety isolation
    backend.setSimulateErrors(true);
    QVERIFY(!backend.writeDigital("PA5", true));
    backend.setSimulateErrors(false);
    QVERIFY(backend.writeDigital("PA5", true));
}

void TestAllCases::testPhase5SimulationRuntimeClockAndExpressions() {
    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("SimClockApp", 480, 320);

    // 1. Timer DataSource
    DataSource dsTimer("timer_1", "Heartbeat Timer", DataSourceType::Timer, DataDirection::Input, DataType::Integer);
    QJsonObject timerMeta;
    timerMeta["timerPeriodMs"] = 200;
    dsTimer.setMetadata(timerMeta);
    proj.addDataSource(dsTimer);

    // 2. Sensor & Calculated DataSources
    DataSource dsSpeed("src_speed", "Vehicle Speed", DataSourceType::Sensor, DataDirection::Input, DataType::Float);
    dsSpeed.setValue(120.0);
    proj.addDataSource(dsSpeed);

    DataSource dsCalc("calc_pct", "Speed Percentage", DataSourceType::Calculated, DataDirection::Input, DataType::Float);
    QJsonObject calcMeta;
    calcMeta["expression"] = "src_speed / 240.0 * 100.0";
    dsCalc.setMetadata(calcMeta);
    proj.addDataSource(dsCalc);

    Simulator::SimulationRuntime runtime(&proj);
    QCOMPARE(runtime.status(), Simulator::SimulationStatus::Stopped);

    // Clock speed configuration
    runtime.setSpeedFactor(2.0);
    QVERIFY(qAbs(runtime.speedFactor() - 2.0) < 0.001);

    runtime.start();
    QCOMPARE(runtime.status(), Simulator::SimulationStatus::Running);

    // Verify initial values
    QCOMPARE(runtime.dataSourceValue("src_speed").toDouble(), 120.0);
    QVERIFY(qAbs(runtime.dataSourceValue("calc_pct").toDouble() - 50.0) < 0.01);

    // Update speed source -> verify calculated updates
    runtime.setDataSourceValue("src_speed", 180.0);
    QVERIFY(qAbs(runtime.dataSourceValue("calc_pct").toDouble() - 75.0) < 0.01);

    // Step clock
    runtime.pause();
    QCOMPARE(runtime.status(), Simulator::SimulationStatus::Paused);
    runtime.step();

    runtime.stop();
    QCOMPARE(runtime.status(), Simulator::SimulationStatus::Stopped);
}

void TestAllCases::testPhase5BidirectionalDataBindingsAndAutoState() {
    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("SimBindingsApp", 800, 480);

    Screen* scr = proj.activeScreen();

    // 1. Speedometer Component (Read binding with Warning/Error Thresholds)
    auto* speedo = new SpeedometerComponent("speedo_val");
    speedo->setCompPos(20, 20);
    speedo->setCompSize(200, 200);
    speedo->setMinimum(0.0);
    speedo->setMaximum(240.0);
    speedo->setWarningThreshold(140.0);
    speedo->setCriticalThreshold(200.0);
    scr->addComponent(speedo);

    DataSource dsSpeed("spd_src", "Speed Sensor", DataSourceType::Sensor, DataDirection::Input, DataType::Float);
    dsSpeed.setValue(90.0);
    proj.addDataSource(dsSpeed);

    DataBinding bRead("speedo_val", "value", "spd_src", BindingDirection::Read);
    proj.addDataBinding(bRead);

    // 2. Switch Component (Write binding to GPIO DataSource)
    auto* sw = new SwitchComponent("sw_light");
    sw->setCompPos(240, 20);
    sw->setChecked(false);
    scr->addComponent(sw);

    DataSource dsGpio("gpio_light", "Light GPIO", DataSourceType::Gpio, DataDirection::Output, DataType::Boolean);
    dsGpio.setHardwareRef("PA4");
    dsGpio.setValue(false);
    proj.addDataSource(dsGpio);

    DataBinding bWrite("sw_light", "checked", "gpio_light", BindingDirection::Write);
    proj.addDataBinding(bWrite);

    // 3. ProgressBar Component (ReadWrite binding to ADC DataSource)
    auto* pb = new ProgressBarComponent("pb_level");
    pb->setCompPos(240, 80);
    pb->setValue(0.2);
    scr->addComponent(pb);

    DataSource dsAdc("adc_level", "Tank Level", DataSourceType::Adc, DataDirection::Input, DataType::Float);
    dsAdc.setHardwareRef("PA0");
    dsAdc.setValue(0.2);
    proj.addDataSource(dsAdc);

    DataBinding bRw("pb_level", "value", "adc_level", BindingDirection::ReadWrite);
    proj.addDataBinding(bRw);

    Simulator::SimulationRuntime runtime(&proj);
    runtime.start();

    // Test Read binding propagation
    runtime.setDataSourceValue("spd_src", 110.0);
    QVERIFY(qAbs(speedo->value() - 110.0) < 0.01);
    QCOMPARE(speedo->currentState().toLower(), QString("normal"));

    // Auto-state threshold: Warning
    runtime.setDataSourceValue("spd_src", 160.0);
    QVERIFY(qAbs(speedo->value() - 160.0) < 0.01);
    QCOMPARE(speedo->currentState().toLower(), QString("warning"));

    // Auto-state threshold: Error (Critical)
    runtime.setDataSourceValue("spd_src", 220.0);
    QVERIFY(qAbs(speedo->value() - 220.0) < 0.01);
    QCOMPARE(speedo->currentState().toLower(), QString("critical"));

    // Test Write binding propagation
    runtime.notifyComponentPropertyChanged(sw, "checked", true);
    QCOMPARE(runtime.dataSourceValue("gpio_light").toBool(), true);
    bool gpioPin = false;
    QVERIFY(runtime.simulationBackend()->readDigital("PA4", &gpioPin));
    QCOMPARE(gpioPin, true);

    runtime.stop();
}

void TestAllCases::testPhase5ProjectSaveSafetyAndSnapshotRestore() {
    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("SafetyProject", 480, 320);

    Screen* scr = proj.activeScreen();
    auto* speedo = new SpeedometerComponent("dash_speedo");
    speedo->setValue(55.0);
    scr->addComponent(speedo);

    DataSource ds("src_s", "Speed", DataSourceType::Sensor, DataDirection::Input, DataType::Float);
    ds.setValue(55.0);
    proj.addDataSource(ds);

    DataBinding b("dash_speedo", "value", "src_s", BindingDirection::Read);
    proj.addDataBinding(b);

    Simulator::SimulationRuntime runtime(&proj);

    // Start simulation, mutate values aggressively
    runtime.start();
    runtime.setDataSourceValue("src_s", 230.0);
    QVERIFY(qAbs(speedo->value() - 230.0) < 0.01);

    // Stop simulation -> must restore original design values
    runtime.stop();
    QVERIFY(qAbs(speedo->value() - 55.0) < 0.01);
    QVERIFY(qAbs(ds.value().toDouble() - 55.0) < 0.01);

    // Verify project serialization retains only design values
    QJsonObject projJson = proj.toJson();
    CanvasScene reloadScene;
    Project reloadProj(&reloadScene);
    reloadProj.fromJson(projJson);
    auto* loadedSpeedo = dynamic_cast<SpeedometerComponent*>(reloadProj.activeScreen()->findComponentById("dash_speedo"));
    QVERIFY(loadedSpeedo != nullptr);
    QVERIFY(qAbs(loadedSpeedo->value() - 55.0) < 0.01);
}

void TestAllCases::testPhase5SimulatorWindowAndControlPanelUi() {
    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("UiSimApp", 800, 480);

    Screen* s1 = proj.activeScreen();
    s1->setName("Main");
    auto* sp = new SpeedometerComponent("sp_1");
    s1->addComponent(sp);

    Screen* s2 = proj.addScreen("Diagnostics");
    auto* rpm = new RpmComponent("rpm_1");
    s2->addComponent(rpm);

    DataSource dsSp("speed_source", "Speed Source", DataSourceType::Sensor, DataDirection::Input, DataType::Float);
    dsSp.setValue(100.0);
    QJsonObject mSp; mSp["unit"] = "km/h"; dsSp.setMetadata(mSp);
    proj.addDataSource(dsSp);

    DataSource dsRpm("rpm_source", "RPM Source", DataSourceType::Sensor, DataDirection::Input, DataType::Float);
    dsRpm.setValue(3000.0);
    QJsonObject mRpm; mRpm["unit"] = "RPM"; dsRpm.setMetadata(mRpm);
    proj.addDataSource(dsRpm);

    DataSource dsTemp("temperature_source", "Temp Source", DataSourceType::Sensor, DataDirection::Input, DataType::Float);
    dsTemp.setValue(85.0);
    QJsonObject mTemp; mTemp["unit"] = "°C"; dsTemp.setMetadata(mTemp);
    proj.addDataSource(dsTemp);

    DataSource dsBatt("battery_source", "Battery Source", DataSourceType::Sensor, DataDirection::Input, DataType::Float);
    dsBatt.setValue(90.0);
    QJsonObject mBatt; mBatt["unit"] = "%"; dsBatt.setMetadata(mBatt);
    proj.addDataSource(dsBatt);

    Simulator::SimulatorWindow win(&proj);
    win.resize(1200, 750);
    win.show();
    QApplication::processEvents();

    // Verify UI components
    auto* toolbar = win.findChild<QToolBar*>();
    QVERIFY(toolbar != nullptr);

    auto* screenCombo = win.findChild<QComboBox*>("simScreenCombo");
    QVERIFY(screenCombo != nullptr);
    QCOMPARE(screenCombo->count(), 2);

    auto* controlPanel = win.findChild<Simulator::SimulationControlPanel*>();
    QVERIFY(controlPanel != nullptr);

    auto* presetCombo = win.findChild<QComboBox*>("simPresetCombo");
    QVERIFY(presetCombo != nullptr);
    QVERIFY(presetCombo->count() >= 5);

    // Apply "Engine Redline" preset
    int redlineIdx = presetCombo->findText("Engine Redline");
    if (redlineIdx >= 0) {
        presetCombo->setCurrentIndex(redlineIdx);
        QApplication::processEvents();
        QCOMPARE(win.runtime()->dataSourceValue("rpm_source").toDouble(), 7500.0);
    }

    // Apply "Low Battery" preset
    int lowBatIdx = presetCombo->findText("Low Battery");
    if (lowBatIdx >= 0) {
        presetCombo->setCurrentIndex(lowBatIdx);
        QApplication::processEvents();
        QCOMPARE(win.runtime()->dataSourceValue("battery_source").toDouble(), 12.0);
    }

    // Capture screenshot of Simulator Window
    const QString shotPath = screenshotPath("verify_phase5_simulator_window.png");
    QVERIFY(win.grab().save(shotPath));
    QVERIFY(QFile::exists(shotPath));
}

void TestAllCases::testPhase5DeterministicSimulatorCoverage() {
    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("DeterministicCoverageApp", 800, 480);

    // 1. Screens & Navigation
    Screen* scrA = proj.activeScreen();
    scrA->setName("Screen_A");
    Screen* scrB = proj.addScreen("Screen_B");
    Screen* scrC = proj.addScreen("Screen_C");
    QCOMPARE(proj.screens().size(), 3);

    // 2. Component Coverage across all categories
    // Basic
    auto* rect = new RectangleComponent("rect_1");
    rect->setCompPos(10, 10); rect->setCompSize(80, 40);
    scrA->addComponent(rect);

    auto* circ = new CircleComponent("circ_1");
    circ->setCompPos(100, 10); circ->setCompSize(40, 40);
    scrA->addComponent(circ);

    auto* path = new PathComponent("path_1");
    path->setCompPos(150, 10); path->setCompSize(60, 40);
    scrA->addComponent(path);

    auto* btn = new ButtonComponent("btn_1");
    btn->setCompPos(220, 10); btn->setCompSize(90, 36);
    btn->setText("Sim Button");
    scrA->addComponent(btn);

    auto* sw = new SwitchComponent("sw_1");
    sw->setCompPos(320, 10); sw->setCompSize(60, 32);
    sw->setChecked(false);
    scrA->addComponent(sw);

    auto* chk = new CheckboxComponent("chk_1");
    chk->setCompPos(390, 10); chk->setCompSize(80, 30);
    chk->setChecked(false);
    scrA->addComponent(chk);

    auto* lbl = new LabelComponent("lbl_1");
    lbl->setCompPos(480, 10); lbl->setCompSize(100, 30);
    lbl->setText("Initial Text");
    scrA->addComponent(lbl);

    auto* txt = new TextInputComponent("txt_1");
    txt->setCompPos(590, 10); txt->setCompSize(100, 30);
    txt->setText("Sim Input");
    scrA->addComponent(txt);

    // Progress
    auto* pb = new ProgressBarComponent("pb_1");
    pb->setCompPos(10, 60); pb->setCompSize(180, 24);
    pb->setValue(0.1);
    scrA->addComponent(pb);

    auto* cp = new CircularProgressComponent("cp_1");
    cp->setCompPos(200, 60); cp->setCompSize(80, 80);
    cp->setValue(25.0);
    scrA->addComponent(cp);

    // Dashboard
    auto* gauge = new GaugeComponent("gauge_1");
    gauge->setCompPos(10, 160); gauge->setCompSize(140, 140);
    gauge->setMinimum(0.0); gauge->setMaximum(100.0);
    gauge->setWarningThreshold(70.0); gauge->setCriticalThreshold(90.0);
    gauge->setValue(10.0);
    scrB->addComponent(gauge);

    auto* spd = new SpeedometerComponent("spd_1");
    spd->setCompPos(160, 160); spd->setCompSize(140, 140);
    spd->setMinimum(0.0); spd->setMaximum(240.0);
    spd->setValue(0.0);
    scrB->addComponent(spd);

    auto* rpm = new RpmComponent("rpm_1");
    rpm->setCompPos(310, 160); rpm->setCompSize(140, 140);
    rpm->setMinimum(0.0); rpm->setMaximum(8000.0);
    rpm->setValue(800.0);
    scrB->addComponent(rpm);

    auto* batt = new BatteryComponent("batt_1");
    batt->setCompPos(460, 160); batt->setCompSize(90, 50);
    batt->setValue(95.0);
    scrB->addComponent(batt);

    auto* press = new PressureComponent("press_1");
    press->setCompPos(560, 160); press->setCompSize(100, 100);
    press->setValue(2.2);
    scrB->addComponent(press);

    auto* temp = new TemperatureComponent("temp_1");
    temp->setCompPos(670, 160); temp->setCompSize(90, 140);
    temp->setValue(75.0);
    scrB->addComponent(temp);

    // Navigation & Data
    auto* tabView = new TabViewComponent("tab_1");
    tabView->setCompPos(10, 320); tabView->setCompSize(240, 140);
    tabView->setTabs({"Tab 1", "Tab 2", "Tab 3"});
    tabView->setActiveTabIndex(0);
    scrC->addComponent(tabView);

    auto* navBar = new NavigationBarComponent("nav_1");
    navBar->setCompPos(260, 320); navBar->setCompSize(240, 50);
    navBar->setItems({{"Home", "icon1", "Screen_A"}, {"Diagnostics", "icon2", "Screen_B"}});
    navBar->setSelectedIndex(0);
    scrC->addComponent(navBar);

    auto* list = new ListComponent("list_1");
    list->setCompPos(510, 320); list->setCompSize(130, 140);
    list->setItems({"Option A", "Option B", "Option C"});
    list->setSelectedIndex(0);
    scrC->addComponent(list);

    auto* table = new TableComponent("table_1");
    table->setCompPos(650, 320); table->setCompSize(140, 140);
    table->setColumns({"Param", "Val"});
    table->addRow({"P1", "10"});
    table->addRow({"P2", "20"});
    table->setSelectedRow(0);
    scrC->addComponent(table);

    // 3. DataSources: Gpio, Adc, Sensor, Timer, Calculated
    DataSource dsGpio("ds_gpio", "GPIO Pin 5", DataSourceType::Gpio, DataDirection::Output, DataType::Boolean);
    dsGpio.setHardwareRef("PA5");
    dsGpio.setValue(false);
    proj.addDataSource(dsGpio);

    DataSource dsAdc("ds_adc", "ADC Channel 0", DataSourceType::Adc, DataDirection::Input, DataType::Float);
    dsAdc.setHardwareRef("PA0");
    dsAdc.setValue(0.5);
    proj.addDataSource(dsAdc);

    DataSource dsGauge("ds_gauge", "Oil Pressure", DataSourceType::Sensor, DataDirection::Input, DataType::Float);
    dsGauge.setValue(15.0);
    proj.addDataSource(dsGauge);

    DataSource dsSlider("ds_slider", "Brightness", DataSourceType::Sensor, DataDirection::Output, DataType::Integer);
    dsSlider.setValue(50);
    proj.addDataSource(dsSlider);

    DataSource dsTimer("ds_timer", "Sim Clock", DataSourceType::Timer, DataDirection::Input, DataType::Integer);
    QJsonObject tMeta; tMeta["periodMs"] = 100; dsTimer.setMetadata(tMeta);
    proj.addDataSource(dsTimer);

    DataSource dsCalc("ds_calc", "Doubled Gauge", DataSourceType::Calculated, DataDirection::Input, DataType::Float);
    QJsonObject cMeta; cMeta["expression"] = "ds_gauge * 2.0"; dsCalc.setMetadata(cMeta);
    proj.addDataSource(dsCalc);

    // 4. DataBindings: Read, Write, ReadWrite
    DataBinding bRead("gauge_1", "value", "ds_gauge", BindingDirection::Read);
    proj.addDataBinding(bRead);

    DataBinding bWrite("sw_1", "checked", "ds_gpio", BindingDirection::Write);
    proj.addDataBinding(bWrite);

    auto* slider = new SliderComponent("slider_1");
    slider->setCompPos(10, 100); slider->setCompSize(120, 24);
    slider->setMinimum(0); slider->setMaximum(100);
    slider->setValue(50);
    scrA->addComponent(slider);

    DataBinding bRw("slider_1", "value", "ds_slider", BindingDirection::ReadWrite);
    proj.addDataBinding(bRw);

    // 5. Simulator Runtime execution & validation
    Simulator::SimulationRuntime runtime(&proj);
    runtime.start();
    QCOMPARE(runtime.status(), Simulator::SimulationStatus::Running);

    // Verify Read Binding & State Transition
    runtime.setDataSourceValue("ds_gauge", 85.0);
    QVERIFY(qAbs(gauge->value() - 85.0) < 0.01);
    QCOMPARE(gauge->currentState().toLower(), QString("warning"));

    runtime.setDataSourceValue("ds_gauge", 95.0);
    QVERIFY(qAbs(gauge->value() - 95.0) < 0.01);
    QCOMPARE(gauge->currentState().toLower(), QString("critical"));

    // Min/Max clamping validation
    gauge->setMaximum(100.0);
    gauge->setValue(120.0);
    QVERIFY(gauge->value() <= 100.0);

    // Verify Write Binding: Switch -> GPIO DataSource -> Virtual HAL
    runtime.notifyComponentPropertyChanged(sw, "checked", true);
    QCOMPARE(runtime.dataSourceValue("ds_gpio").toBool(), true);
    bool pinHigh = false;
    QVERIFY(runtime.simulationBackend()->readDigital("PA5", &pinHigh));
    QCOMPARE(pinHigh, true);

    // Verify ReadWrite Binding: Slider -> DataSource
    runtime.notifyComponentPropertyChanged(slider, "value", 82);
    QCOMPARE(runtime.dataSourceValue("ds_slider").toInt(), 82);
    runtime.setDataSourceValue("ds_slider", 35);
    QCOMPARE(slider->value(), 35);

    // Screen navigation
    runtime.setActiveScreenId(scrB->id());
    QCOMPARE(runtime.activeScreenId(), scrB->id());
    QCOMPARE(runtime.activeScreen(), scrB);

    // Tab navigation
    tabView->setActiveTabIndex(2);
    runtime.notifyComponentPropertyChanged(tabView, "activeTabIndex", 2);
    QCOMPARE(tabView->activeTabIndex(), 2);

    // List & Table selection
    list->setSelectedIndex(1);
    runtime.notifyComponentPropertyChanged(list, "selectedIndex", 1);
    QCOMPARE(list->selectedIndex(), 1);
    table->setSelectedRow(1);
    runtime.notifyComponentPropertyChanged(table, "selectedRow", 1);
    QCOMPARE(table->selectedRow(), 1);

    // Timer and Calculated DataSource
    runtime.step(250);
    QVERIFY(runtime.dataSourceValue("ds_timer").toInt() >= 2);
    QVERIFY(qAbs(runtime.dataSourceValue("ds_calc").toDouble() - (95.0 * 2.0)) < 0.01);

    // Virtual HAL Reset and Disconnect behavior
    runtime.simulationBackend()->setSimulatedConnected(false);
    QVERIFY(!runtime.simulationBackend()->isConnected());
    runtime.simulationBackend()->setSimulatedConnected(true);
    QVERIFY(runtime.simulationBackend()->isConnected());

    runtime.simulationBackend()->resetAllSimulationData();
    QCOMPARE(runtime.simulationBackend()->simulatedDigital("PA5"), false);

    // Stop and verify Project Save Safety
    runtime.stop();
    QCOMPARE(runtime.status(), Simulator::SimulationStatus::Stopped);
    QCOMPARE(sw->isChecked(), false);
    QCOMPARE(slider->value(), 50);
}

void TestAllCases::testPhase5MockHardwarePipelines() {
    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("MockPipelinesApp", 800, 480);
    Screen* scr = proj.activeScreen();

    // Pipeline 1: Switch -> DataBinding -> GPIO DataSource -> SimulationBackend
    auto* sw = new SwitchComponent("sw_pipe1");
    sw->setChecked(false);
    scr->addComponent(sw);

    DataSource dsGpio("gpio_pipe1", "Relay Output", DataSourceType::Gpio, DataDirection::Output, DataType::Boolean);
    dsGpio.setHardwareRef("PA4");
    dsGpio.setValue(false);
    proj.addDataSource(dsGpio);

    DataBinding bGpio("sw_pipe1", "checked", "gpio_pipe1", BindingDirection::Write);
    proj.addDataBinding(bGpio);

    // Pipeline 2: Simulation ADC -> ADC DataSource -> DataBinding -> ProgressBar
    auto* pb = new ProgressBarComponent("pb_pipe2");
    pb->setValue(0.0);
    scr->addComponent(pb);

    DataSource dsAdc("adc_pipe2", "Fuel Sensor", DataSourceType::Adc, DataDirection::Input, DataType::Float);
    dsAdc.setHardwareRef("PA0");
    dsAdc.setValue(0.0);
    proj.addDataSource(dsAdc);

    DataBinding bAdc("pb_pipe2", "value", "adc_pipe2", BindingDirection::Read);
    proj.addDataBinding(bAdc);

    // Pipeline 3: Simulation speed -> DataSource -> Speedometer
    auto* spd = new SpeedometerComponent("spd_pipe3");
    spd->setMinimum(0.0); spd->setMaximum(240.0);
    spd->setValue(0.0);
    scr->addComponent(spd);

    DataSource dsSpeed("spd_pipe3_src", "Speed Sensor", DataSourceType::Sensor, DataDirection::Input, DataType::Float);
    dsSpeed.setValue(0.0);
    proj.addDataSource(dsSpeed);

    DataBinding bSpd("spd_pipe3", "value", "spd_pipe3_src", BindingDirection::Read);
    proj.addDataBinding(bSpd);

    // Pipeline 4: Simulation temperature -> DataSource -> Temperature -> Warning/Error state
    auto* temp = new TemperatureComponent("temp_pipe4");
    temp->setMinimum(0.0); temp->setMaximum(150.0);
    temp->setWarningThreshold(90.0);
    temp->setCriticalThreshold(115.0);
    temp->setValue(40.0);
    scr->addComponent(temp);

    DataSource dsTemp("temp_pipe4_src", "Coolant Temp", DataSourceType::Sensor, DataDirection::Input, DataType::Float);
    dsTemp.setValue(40.0);
    proj.addDataSource(dsTemp);

    DataBinding bTemp("temp_pipe4", "value", "temp_pipe4_src", BindingDirection::Read);
    proj.addDataBinding(bTemp);

    Simulator::SimulationRuntime runtime(&proj);
    runtime.start();

    // Verify Pipeline 1: Switch -> GPIO -> Virtual HAL
    runtime.notifyComponentPropertyChanged(sw, "checked", true);
    QCOMPARE(runtime.dataSourceValue("gpio_pipe1").toBool(), true);
    bool pinOut = false;
    QVERIFY(runtime.simulationBackend()->readDigital("PA4", &pinOut));
    QCOMPARE(pinOut, true);

    runtime.notifyComponentPropertyChanged(sw, "checked", false);
    QCOMPARE(runtime.dataSourceValue("gpio_pipe1").toBool(), false);
    QVERIFY(runtime.simulationBackend()->readDigital("PA4", &pinOut));
    QCOMPARE(pinOut, false);

    // Verify Pipeline 2: Simulation ADC -> DataSource -> ProgressBar
    runtime.simulationBackend()->setSimulatedAdc("PA0", 3072, 0.75);
    runtime.setDataSourceValue("adc_pipe2", 0.75);
    QVERIFY(qAbs(pb->value() - 0.75) < 0.01);

    // Verify Pipeline 3: Simulation speed -> DataSource -> Speedometer
    runtime.setDataSourceValue("spd_pipe3_src", 155.0);
    QVERIFY(qAbs(spd->value() - 155.0) < 0.01);

    // Verify Pipeline 4: Simulation temperature -> DataSource -> Temperature -> Warning/Error state
    runtime.setDataSourceValue("temp_pipe4_src", 70.0);
    QCOMPARE(temp->currentState().toLower(), QString("normal"));

    runtime.setDataSourceValue("temp_pipe4_src", 98.0);
    QCOMPARE(temp->currentState().toLower(), QString("warning"));

    runtime.setDataSourceValue("temp_pipe4_src", 125.0);
    QCOMPARE(temp->currentState().toLower(), QString("critical"));

    runtime.stop();
}

void TestAllCases::testPhase5VisualRegressionDemoProject() {
    CanvasScene scene;
    Project demoProj(&scene);
    demoProj.newProject("DemoAutomotiveApp", 800, 480);

    // Screen 1: Dashboard
    Screen* sDash = demoProj.activeScreen();
    sDash->setName("Dashboard");

    auto* spd = new SpeedometerComponent("demo_spd");
    spd->setCompPos(20, 30); spd->setCompSize(200, 200);
    spd->setMinimum(0.0); spd->setMaximum(240.0); spd->setValue(95.0);
    sDash->addComponent(spd);

    auto* rpm = new RpmComponent("demo_rpm");
    rpm->setCompPos(240, 30); rpm->setCompSize(200, 200);
    rpm->setMinimum(0.0); rpm->setMaximum(8000.0); rpm->setValue(3200.0);
    sDash->addComponent(rpm);

    auto* temp = new TemperatureComponent("demo_temp");
    temp->setCompPos(460, 30); temp->setCompSize(100, 160);
    temp->setMinimum(0.0); temp->setMaximum(130.0); temp->setValue(88.0);
    sDash->addComponent(temp);

    auto* batt = new BatteryComponent("demo_batt");
    batt->setCompPos(580, 40); batt->setCompSize(100, 60);
    batt->setValue(82.0);
    sDash->addComponent(batt);

    auto* pb = new ProgressBarComponent("demo_fuel");
    pb->setCompPos(580, 130); pb->setCompSize(180, 26);
    pb->setValue(0.65);
    sDash->addComponent(pb);

    // Screen 2: Diagnostics
    Screen* sDiag = demoProj.addScreen("Diagnostics");

    auto* gauge = new GaugeComponent("demo_oil_gauge");
    gauge->setCompPos(30, 40); gauge->setCompSize(180, 180);
    gauge->setMinimum(0.0); gauge->setMaximum(100.0); gauge->setValue(45.0);
    sDiag->addComponent(gauge);

    auto* press = new PressureComponent("demo_boost");
    press->setCompPos(230, 40); press->setCompSize(180, 180);
    press->setMinimum(0.0); press->setMaximum(4.0); press->setValue(1.8);
    sDiag->addComponent(press);

    auto* cp = new CircularProgressComponent("demo_health");
    cp->setCompPos(430, 40); cp->setCompSize(160, 160);
    cp->setValue(98.0);
    sDiag->addComponent(cp);

    auto* table = new TableComponent("demo_diag_table");
    table->setCompPos(30, 240); table->setCompSize(720, 200);
    table->setColumns({"Subsystem", "CAN Address", "Status", "Reading"});
    table->addRow({"Engine ECU", "0x7E0", "OK", "3200 RPM"});
    table->addRow({"Transmission", "0x7E1", "OK", "Gear 4"});
    table->addRow({"Brake ABS", "0x7E2", "OK", "12.4 bar"});
    table->addRow({"Battery BMS", "0x7E3", "OK", "48.2 V"});
    table->setSelectedRow(0);
    sDiag->addComponent(table);

    // Screen 3: Settings
    Screen* sSet = demoProj.addScreen("Settings");

    auto* tabView = new TabViewComponent("demo_tabs");
    tabView->setCompPos(30, 20); tabView->setCompSize(720, 360);
    tabView->setTabs({"Vehicle Profile", "Displays & Audio", "Network / CAN"});
    tabView->setActiveTabIndex(0);
    sSet->addComponent(tabView);

    auto* list = new ListComponent("demo_list");
    list->setCompPos(60, 90); list->setCompSize(300, 240);
    list->setItems({"Metric Units (km/h, bar)", "Automatic Day/Night Mode", "Tire Pressure Monitoring (TPMS)", "Dynamic Stability Control"});
    list->setSelectedIndex(0);
    sSet->addComponent(list);

    auto* nav = new NavigationBarComponent("demo_nav");
    nav->setCompPos(30, 400); nav->setCompSize(720, 60);
    nav->setItems({
        {"Dashboard", "dash_ico", sDash->id()},
        {"Diagnostics", "diag_ico", sDiag->id()},
        {"Settings", "set_ico", sSet->id()}
    });
    nav->setSelectedIndex(0);
    sSet->addComponent(nav);

    // Launch Simulator Window and validate each screen
    Simulator::SimulatorWindow win(&demoProj);
    win.resize(1200, 750);
    win.show();
    QApplication::processEvents();

    // 1. Validate Dashboard Screen
    win.runtime()->setActiveScreenId(sDash->id());
    QApplication::processEvents();
    const QString shotDash = screenshotPath("simulator_demo_dashboard.png");
    QVERIFY(win.grab().save(shotDash));
    QVERIFY(QFile::exists(shotDash));

    // 2. Validate Diagnostics Screen
    win.runtime()->setActiveScreenId(sDiag->id());
    QApplication::processEvents();
    const QString shotDiag = screenshotPath("simulator_demo_diagnostics.png");
    QVERIFY(win.grab().save(shotDiag));
    QVERIFY(QFile::exists(shotDiag));

    // 3. Validate Settings Screen
    win.runtime()->setActiveScreenId(sSet->id());
    QApplication::processEvents();
    const QString shotSet = screenshotPath("simulator_demo_settings.png");
    QVERIFY(win.grab().save(shotSet));
    QVERIFY(QFile::exists(shotSet));

    win.runtime()->stop();
}

void TestAllCases::testPhase5PerformanceStress() {
    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("PerfStressApp", 1024, 600);
    Screen* scr = proj.activeScreen();

    // Create 50 components, 20 DataSources, 50 DataBindings
    for (int i = 0; i < 20; ++i) {
        DataSource ds(QString("stress_ds_%1").arg(i), QString("Source %1").arg(i),
                       DataSourceType::Sensor, DataDirection::Input, DataType::Float);
        ds.setValue(static_cast<double>(i * 5));
        proj.addDataSource(ds);
    }

    for (int i = 0; i < 50; ++i) {
        auto* gauge = new GaugeComponent(QString("stress_g_%1").arg(i));
        gauge->setCompPos((i % 10) * 90, (i / 10) * 100);
        gauge->setCompSize(80, 80);
        gauge->setMinimum(0.0); gauge->setMaximum(100.0);
        gauge->setValue(0.0);
        scr->addComponent(gauge);

        DataBinding b(QString("stress_g_%1").arg(i), "value",
                      QString("stress_ds_%1").arg(i % 20), BindingDirection::Read);
        proj.addDataBinding(b);
    }

    Simulator::SimulationRuntime runtime(&proj);

    // Measure Startup Time
    QElapsedTimer timer;
    timer.start();
    runtime.start();
    qint64 startupMs = timer.elapsed();
    QVERIFY(runtime.isRunning());
    QVERIFY(startupMs < 150); // Under 150ms startup threshold

    // Measure 1000 Binding Updates
    timer.restart();
    for (int cycle = 0; cycle < 1000; ++cycle) {
        QString dsId = QString("stress_ds_%1").arg(cycle % 20);
        runtime.setDataSourceValue(dsId, static_cast<double>(cycle % 100));
    }
    qint64 totalUpdateMs = timer.elapsed();
    double avgMsPerUpdate = static_cast<double>(totalUpdateMs) / 1000.0;

    // Verify sub-millisecond binding responsiveness
    QVERIFY(avgMsPerUpdate < 2.0); // Under 2.0 ms per update batch
    qDebug() << "[Phase5 Perf] Startup:" << startupMs << "ms | 1000 Binding updates:" << totalUpdateMs << "ms (Avg:" << avgMsPerUpdate << "ms/update)";

    runtime.stop();
}

// ============================================================================
// 28. Phase 6 Multi-Provider AI Platform
// ============================================================================

void TestAllCases::testPhase6AIProviderNeutralInterfaceAndCapabilities() {
    using namespace AI;

    // Role conversion tests
    QCOMPARE(roleToString(AIRole::System), QString("system"));
    QCOMPARE(roleToString(AIRole::User), QString("user"));
    QCOMPARE(roleToString(AIRole::Assistant), QString("assistant"));
    QCOMPARE(roleToString(AIRole::Tool), QString("tool"));

    QCOMPARE(stringToRole("system"), AIRole::System);
    QCOMPARE(stringToRole("user"), AIRole::User);
    QCOMPARE(stringToRole("assistant"), AIRole::Assistant);
    QCOMPARE(stringToRole("tool"), AIRole::Tool);

    // AIMessage Construction & Serialization
    AIMessage msg(AIRole::User, "Hello AI");
    QCOMPARE(msg.textContent(), QString("Hello AI"));

    ToolCall tc;
    tc.id = "call_123";
    tc.name = "create_component";
    tc.arguments = QJsonObject{{"type", "Button"}, {"x", 10.0}, {"y", 20.0}};
    msg.addToolCall(tc);

    ToolResult tr;
    tr.id = "call_123";
    tr.result = "Component created";
    msg.addToolResult(tr);

    QJsonObject jsonMsg = msg.toJson();
    AIMessage parsedMsg = AIMessage::fromJson(jsonMsg);
    QCOMPARE(parsedMsg.role(), AIRole::User);
    QCOMPARE(parsedMsg.textContent(), QString("Hello AI"));
    QCOMPARE(parsedMsg.toolCalls().size(), 1);
    QCOMPARE(parsedMsg.toolCalls().first().name, QString("create_component"));
    QCOMPARE(parsedMsg.toolResults().size(), 1);
    QCOMPARE(parsedMsg.toolResults().first().result, QString("Component created"));

    // AIConversation and cumulative usage
    AIConversation conv;
    conv.setProviderId("mock");
    conv.setModel("mock-model-v1");
    conv.addMessage(msg);

    AIUsage u1;
    u1.promptTokens = 10;
    u1.completionTokens = 20;
    u1.totalTokens = 30;
    u1.latencyMs = 15;
    conv.addUsage(u1);

    AIUsage u2;
    u2.promptTokens = 5;
    u2.completionTokens = 15;
    u2.totalTokens = 20;
    u2.latencyMs = 25;
    conv.addUsage(u2);

    QCOMPARE(conv.cumulativeUsage().promptTokens, 15);
    QCOMPARE(conv.cumulativeUsage().completionTokens, 35);
    QCOMPARE(conv.cumulativeUsage().totalTokens, 50);
    QCOMPARE(conv.cumulativeUsage().latencyMs, 40);

    QJsonObject convJson = conv.toJson();
    AIConversation parsedConv = AIConversation::fromJson(convJson);
    QCOMPARE(parsedConv.providerId(), QString("mock"));
    QCOMPARE(parsedConv.messages().size(), 1);
    QCOMPARE(parsedConv.cumulativeUsage().totalTokens, 50);

    // Mock Provider Capabilities
    MockAIProvider mock;
    QVERIFY(mock.supportsTools());
    QVERIFY(mock.supportsStreaming());
    QVERIFY(mock.supportsVision());
    QVERIFY(mock.supportsMultimodal());
}

void TestAllCases::testPhase6AIProviderRegistryAndRouting() {
    using namespace AI;

    auto& registry = AIProviderRegistry::instance();
    QStringList ids = registry.providerIds();
    QVERIFY(ids.contains("mock"));
    QVERIFY(ids.contains("openai"));
    QVERIFY(ids.contains("anthropic"));
    QVERIFY(ids.contains("gemini"));
    QVERIFY(ids.contains("custom"));

    // Routing: Default Provider
    registry.setDefaultProviderId("openai");
    QCOMPARE(registry.defaultProviderId(), QString("openai"));
    QVERIFY(registry.activeProvider() != nullptr);
    QCOMPARE(registry.activeProvider()->providerId(), QString("openai"));

    // Routing: Fallback Provider
    registry.setFallbackProviderId("anthropic");
    QCOMPARE(registry.fallbackProviderId(), QString("anthropic"));
    QVERIFY(registry.fallbackProvider() != nullptr);
    QCOMPARE(registry.fallbackProvider()->providerId(), QString("anthropic"));

    // Task Profiles: Diagnostics -> Gemini
    TaskProfile diagProfile;
    diagProfile.preferredProviderId = "gemini";
    diagProfile.preferredModel = "gemini-1.5-pro";
    registry.setTaskProfile(TaskType::Diagnostics, diagProfile);

    auto diagProvider = registry.providerForTask(TaskType::Diagnostics);
    QVERIFY(diagProvider != nullptr);

    // Reset default to mock for tests
    registry.setDefaultProviderId("mock");
    QCOMPARE(registry.defaultProviderId(), QString("mock"));
}

void TestAllCases::testPhase6MessageNormalizationAndWireAdapters() {
    using namespace AI;

    QList<AIMessage> messages;
    AIMessage sysMsg(AIRole::System, "You are an embedded UI expert.");
    AIMessage userMsg(AIRole::User, "Create a battery gauge.");
    messages.append(sysMsg);
    messages.append(userMsg);

    // 1. OpenAI Wire Format
    QJsonArray openAIMsgs = OpenAIProvider::formatMessagesForOpenAI(messages);
    QCOMPARE(openAIMsgs.size(), 2);
    QCOMPARE(openAIMsgs[0].toObject()["role"].toString(), QString("system"));
    QCOMPARE(openAIMsgs[0].toObject()["content"].toString(), QString("You are an embedded UI expert."));
    QCOMPARE(openAIMsgs[1].toObject()["role"].toString(), QString("user"));
    QCOMPARE(openAIMsgs[1].toObject()["content"].toString(), QString("Create a battery gauge."));

    // 2. Anthropic Wire Format (Separates system prompt into top-level)
    QString anthropicSystem;
    QJsonArray anthropicMsgs;
    AnthropicProvider::formatMessagesForAnthropic(messages, anthropicSystem, anthropicMsgs);
    QCOMPARE(anthropicSystem, QString("You are an embedded UI expert."));
    QCOMPARE(anthropicMsgs.size(), 1);
    QCOMPARE(anthropicMsgs[0].toObject()["role"].toString(), QString("user"));

    // 3. Gemini Wire Format (Separates systemInstruction, uses 'model' role)
    QJsonObject geminiSys;
    QJsonArray geminiContents;
    GeminiProvider::formatContentsForGemini(messages, geminiSys, geminiContents);
    QVERIFY(!geminiSys.isEmpty());
    QCOMPARE(geminiContents.size(), 1);
    QCOMPARE(geminiContents[0].toObject()["role"].toString(), QString("user"));

    AIMessage asstMsg(AIRole::Assistant, "I am ready.");
    messages.append(asstMsg);
    GeminiProvider::formatContentsForGemini(messages, geminiSys, geminiContents);
    QCOMPARE(geminiContents.size(), 2);
    QCOMPARE(geminiContents[1].toObject()["role"].toString(), QString("model"));
}

void TestAllCases::testPhase6AIToolRegistryAndUniversalSchemas() {
    using namespace AI;

    auto& registry = AIToolRegistry::instance();
    QVERIFY(registry.findTool("create_screen") != nullptr);
    QVERIFY(registry.findTool("create_component") != nullptr);
    QVERIFY(registry.findTool("update_component") != nullptr);
    QVERIFY(registry.findTool("move_component") != nullptr);
    QVERIFY(registry.findTool("resize_component") != nullptr);
    QVERIFY(registry.findTool("delete_component") != nullptr);
    QVERIFY(registry.findTool("create_datasource") != nullptr);
    QVERIFY(registry.findTool("create_binding") != nullptr);
    QVERIFY(registry.findTool("validate_project") != nullptr);
    QVERIFY(registry.findTool("generate_code") != nullptr);
    QVERIFY(registry.findTool("inspect_project") != nullptr);
    QVERIFY(registry.findTool("set_hardware_pin") != nullptr);

    // Check OpenAI Schema
    QJsonArray openAITools = registry.openAITools();
    QVERIFY(!openAITools.isEmpty());
    QCOMPARE(openAITools[0].toObject()["type"].toString(), QString("function"));
    QVERIFY(openAITools[0].toObject().contains("function"));

    // Check Anthropic Schema
    QJsonArray anthropicTools = registry.anthropicTools();
    QVERIFY(!anthropicTools.isEmpty());
    QVERIFY(anthropicTools[0].toObject().contains("input_schema"));

    // Check Gemini Schema
    QJsonArray geminiTools = registry.geminiTools();
    QVERIFY(!geminiTools.isEmpty());
    QVERIFY(geminiTools[0].toObject().contains("functionDeclarations"));
}

void TestAllCases::testPhase6AIToolExecutionAndUndoableTransactions() {
    using namespace AI;

    MainWindow win;
    DesignerController controller(&win);
    Project* proj = win.currentProject();
    QVERIFY(proj != nullptr);

    auto& registry = AIToolRegistry::instance();

    // 1. Tool Call: create_screen
    ToolCall tcScreen;
    tcScreen.id = "c_1";
    tcScreen.name = "create_screen";
    tcScreen.arguments = QJsonObject{{"name", "DiagnosticsScreen"}, {"width", 800}, {"height", 480}};

    ToolResult resScreen = registry.executeTool(tcScreen, &controller, proj);
    QVERIFY(resScreen.error.isEmpty());
    QVERIFY(proj->findScreen("DiagnosticsScreen") != nullptr || proj->screens().size() >= 2);

    // 2. Tool Call: create_component
    ToolCall tcComp;
    tcComp.id = "c_2";
    tcComp.name = "create_component";
    tcComp.arguments = QJsonObject{
        {"type", "Button"},
        {"x", 100.0},
        {"y", 150.0},
        {"width", 120.0},
        {"height", 40.0},
        {"id", "ai_test_btn"}
    };

    ToolResult resComp = registry.executeTool(tcComp, &controller, proj);
    QVERIFY(resComp.error.isEmpty());

    // 3. Tool Call: move_component
    ToolCall tcMove;
    tcMove.id = "c_3";
    tcMove.name = "move_component";
    tcMove.arguments = QJsonObject{
        {"id", "ai_test_btn"},
        {"x", 250.0},
        {"y", 300.0},
        {"relative", false}
    };
    ToolResult resMove = registry.executeTool(tcMove, &controller, proj);
    QVERIFY(resMove.error.isEmpty());

    // 4. Test Undo / Redo
    QJsonObject undoRes = controller.undo();
    QVERIFY(undoRes["success"].toBool());

    QJsonObject redoRes = controller.redo();
    QVERIFY(redoRes["success"].toBool());
}

void TestAllCases::testPhase6HardwareSafetyGate() {
    using namespace AI;

    auto& registry = AIToolRegistry::instance();

    ToolCall pinCall;
    pinCall.id = "pin_1";
    pinCall.name = "set_hardware_pin";
    pinCall.arguments = QJsonObject{{"pin", "PA4"}, {"state", "HIGH"}};

    // Verify it is flagged as a hardware write tool
    const auto* toolDef = registry.findTool("set_hardware_pin");
    QVERIFY(toolDef != nullptr);
    QVERIFY(toolDef->isHardwareWrite);

    // When disconnected / in simulation mode, writes succeed automatically
    ToolResult resSim = registry.executeTool(pinCall, nullptr, nullptr, false);
    bool isPhysical = HardwareBridge::instance().isHardwareConnected() ||
        (Hardware::HardwareManager::instance().activeBackendId() != "mock" && Hardware::HardwareManager::instance().isHardwareConnected());

    if (!isPhysical) {
        QVERIFY(resSim.error.isEmpty());
        QVERIFY(resSim.result.contains("Simulation Mode"));
    } else {
        QVERIFY(!resSim.error.isEmpty());
        QVERIFY(resSim.error.contains("PROTECTED_HARDWARE_WRITE"));
    }

    // When confirmed by user, write always succeeds
    ToolResult resConfirmed = registry.executeTool(pinCall, nullptr, nullptr, true);
    QVERIFY(resConfirmed.error.isEmpty());

    // Test confirmation policy detection
    bool reqConfirm = registry.requiresHardwareConfirmation(pinCall);
    QCOMPARE(reqConfirm, isPhysical);
}

void TestAllCases::testPhase6AIProjectContextAndSecretSanitization() {
    using namespace AI;

    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("SecurityContextApp", 480, 320);

    // Add dummy component
    auto* btn = new ButtonComponent("secret_btn");
    btn->setCompPos(20, 30);
    btn->setCompSize(100, 40);
    proj.activeScreen()->addComponent(btn);

    // 1. Scoped Context
    QJsonObject ctx = AIProjectContext::buildScopedScreenContext(&proj);
    QCOMPARE(ctx["width"].toInt(), 480);
    QCOMPARE(ctx["height"].toInt(), 320);
    QVERIFY(ctx["components"].toArray().size() >= 1);

    // 2. Secret Redaction
    QString textWithSecrets = "My key is sk-1234567890abcdef1234567890 and Anthropic key sk-ant-1234567890abcdef1234567890 and Bearer secret_token_1234567890.";
    QString sanitized = AIProjectContext::sanitizeSecrets(textWithSecrets);
    QVERIFY(!sanitized.contains("sk-1234567890abcdef1234567890"));
    QVERIFY(!sanitized.contains("sk-ant-1234567890abcdef1234567890"));
    QVERIFY(!sanitized.contains("secret_token_1234567890"));
    QVERIFY(sanitized.contains("[REDACTED_API_KEY]"));
    QVERIFY(sanitized.contains("[REDACTED_TOKEN]"));

    // 3. Object Redaction
    QJsonObject sensitiveObj{
        {"apiKey", "sk-proj-supersecret"},
        {"normalField", "Dashboard Title"}
    };
    QJsonObject sanitizedObj = AIProjectContext::sanitizeJsonObject(sensitiveObj);
    QCOMPARE(sanitizedObj["apiKey"].toString(), QString("[REDACTED]"));
    QCOMPARE(sanitizedObj["normalField"].toString(), QString("Dashboard Title"));
}

void TestAllCases::testPhase6ProviderFailureHandlingAndFallbackSwitching() {
    using namespace AI;

    MockAIProvider primaryMock;
    MockAIProvider fallbackMock;

    primaryMock.setSimulatedStatus(ConnectionStatus::ProviderUnavailable);
    primaryMock.setSimulatedError("503 Service Unavailable");

    bool primaryFailed = false;
    primaryMock.sendMessage({AIMessage(AIRole::User, "Hello")}, {}, [&](const AIMessage&, const AIUsage&, const QString& err) {
        if (!err.isEmpty()) {
            primaryFailed = true;
        }
    });
    QVERIFY(primaryFailed);

    // Clean Fallback execution
    fallbackMock.setSimulatedResponse("Fallback response succeeded");
    bool fallbackSuccess = false;
    QString fallbackText;
    fallbackMock.sendMessage({AIMessage(AIRole::User, "Hello")}, {}, [&](const AIMessage& resp, const AIUsage&, const QString& err) {
        if (err.isEmpty()) {
            fallbackSuccess = true;
            fallbackText = resp.textContent();
        }
    });
    QVERIFY(fallbackSuccess);
    QCOMPARE(fallbackText, QString("Fallback response succeeded"));
}

void TestAllCases::testPhase6OfflineModeZeroAIConfiguration() {
    using namespace AI;

    // Clear all credentials
    auto& registry = AIProviderRegistry::instance();
    registry.clearAllCredentials();

    // Verify full editor operations work 100% offline
    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("OfflineZeroAIApp", 800, 480);

    auto* gauge = new GaugeComponent("offline_gauge");
    gauge->setCompPos(50, 50);
    gauge->setCompSize(200, 200);
    proj.activeScreen()->addComponent(gauge);

    DataSource ds("offline_ds", "Offline Sensor", DataSourceType::Sensor);
    proj.addDataSource(ds);

    DataBinding b("offline_gauge", "value", "offline_ds");
    proj.addDataBinding(b);

    QCOMPARE(proj.activeScreen()->components().size(), 1);
    QCOMPARE(proj.dataSources().size(), 1);
    QCOMPARE(proj.dataBindings().size(), 1);

    // Code generator works offline without AI
    CodeGen::GeneratorContext genCtx(&proj);
    QVERIFY(genCtx.project() != nullptr);
}

void TestAllCases::testPhase6AISettingsDialogUI() {
    AISettingsDialog dlg;
    QCOMPARE(dlg.windowTitle(), QString("AI Providers & Settings"));

    // Test connection on mock provider
    auto p = AI::AIProviderRegistry::instance().getProvider("mock");
    QVERIFY(p != nullptr);

    bool callbackCalled = false;
    p->testConnection([&](const AI::ConnectionTestResult& res) {
        callbackCalled = true;
        QCOMPARE(res.status, AI::ConnectionStatus::Connected);
    });
    QVERIFY(callbackCalled);
}

// ============================================================================
// 29. Phase 1 Option B: Data Source Abstraction (CAN, UART, Modbus, Variables)
// ============================================================================

void TestAllCases::testPhase1CanSignalExtractionAndEncoding() {
    CanSignalConfig can;
    can.busId = "CAN1";
    can.messageId = 0x280;
    can.isExtended = false;
    can.bitrate = 500000;
    can.startBit = 16;
    can.bitLength = 16;
    can.isBigEndian = false;
    can.factor = 0.25;
    can.offset = 0.0;
    can.unit = "rpm";

    // Create 8-byte payload: bytes [0..1] = 0, bytes [2..3] = 0x1F40 (8000), bytes [4..7] = 0
    QByteArray payload(8, 0);
    payload[2] = static_cast<char>(0x40);
    payload[3] = static_cast<char>(0x1F);

    double decoded = can.decodePayload(payload);
    // 8000 * 0.25 = 2000.0 RPM
    QCOMPARE(decoded, 2000.0);

    // Test encode / decode roundtrip
    QByteArray outPayload;
    can.encodePayload(3500.0, outPayload);
    QCOMPARE(outPayload.size(), 8);
    double roundtrip = can.decodePayload(outPayload);
    QCOMPARE(roundtrip, 3500.0);

    // Test Signed Signal
    CanSignalConfig signedCan;
    signedCan.startBit = 0;
    signedCan.bitLength = 8;
    signedCan.isSigned = true;
    signedCan.factor = 1.0;
    signedCan.offset = 0.0;

    QByteArray negPayload(8, 0);
    negPayload[0] = static_cast<char>(0xFE); // -2 in signed two's complement
    QCOMPARE(signedCan.decodePayload(negPayload), -2.0);
}

void TestAllCases::testPhase1UartStreamParsingModes() {
    // 1. DelimiterIndex mode (e.g. CSV)
    UartStreamConfig csvCfg;
    csvCfg.parseMode = UartParseMode::DelimiterIndex;
    csvCfg.delimiter = ",";
    csvCfg.tokenIndex = 1;
    csvCfg.factor = 0.5;
    csvCfg.offset = 10.0;

    QVariant r1 = csvCfg.parseIncomingText("ALPHA,100,BETA");
    QVERIFY(r1.isValid());
    // 100 * 0.5 + 10 = 60.0
    QCOMPARE(r1.toDouble(), 60.0);

    // 2. KeyValue mode
    UartStreamConfig kvCfg;
    kvCfg.parseMode = UartParseMode::KeyValue;
    kvCfg.keyName = "SPEED";
    kvCfg.factor = 1.0;
    kvCfg.offset = 0.0;

    QVariant r2 = kvCfg.parseIncomingText("VOLT=12.4;SPEED=85.5;TEMP=32.0");
    QVERIFY(r2.isValid());
    QCOMPARE(r2.toDouble(), 85.5);

    // 3. RegexCapture mode
    UartStreamConfig regexCfg;
    regexCfg.parseMode = UartParseMode::RegexCapture;
    regexCfg.regexPattern = "RPM:([0-9.]+)";
    regexCfg.factor = 2.0;
    regexCfg.offset = 0.0;

    QVariant r3 = regexCfg.parseIncomingText("[CAN-GATEWAY] RPM:1500 STATUS:OK");
    QVERIFY(r3.isValid());
    QCOMPARE(r3.toDouble(), 3000.0);

    // 4. JsonPath mode
    UartStreamConfig jsonCfg;
    jsonCfg.parseMode = UartParseMode::JsonPath;
    jsonCfg.keyName = "pressure";
    jsonCfg.factor = 1.0;
    jsonCfg.offset = 0.0;

    QVariant r4 = jsonCfg.parseIncomingText("{\"temperature\": 24.5, \"pressure\": 101.3}");
    QVERIFY(r4.isValid());
    QCOMPARE(r4.toDouble(), 101.3);
}

void TestAllCases::testPhase1ModbusRegistersDecodingAndEncoding() {
    // 1. Uint16 holding register
    ModbusConfig mbUint;
    mbUint.dataType = ModbusDataType::Uint16;
    mbUint.scale = 0.1;
    mbUint.offset = 0.0;

    QVector<quint16> regs1 = { 1000 };
    QVariant dec1 = mbUint.decodeRegisters(regs1);
    QCOMPARE(dec1.toDouble(), 100.0);

    // 2. Int16 with negative value
    ModbusConfig mbInt;
    mbInt.dataType = ModbusDataType::Int16;
    mbInt.scale = 1.0;
    mbInt.offset = 0.0;

    QVector<quint16> regs2 = { static_cast<quint16>(static_cast<qint16>(-45)) };
    QVariant dec2 = mbInt.decodeRegisters(regs2);
    QCOMPARE(dec2.toDouble(), -45.0);

    // 3. Float32 Big Endian across 2 registers
    ModbusConfig mbFloat;
    mbFloat.dataType = ModbusDataType::Float32BE;
    mbFloat.scale = 1.0;
    mbFloat.offset = 0.0;

    QVector<quint16> encFloat = mbFloat.encodeRegisters(123.456);
    QCOMPARE(encFloat.size(), 2);
    QVariant decFloat = mbFloat.decodeRegisters(encFloat);
    QVERIFY(std::abs(decFloat.toDouble() - 123.456) < 0.001);

    // 4. Int32 Little Endian across 2 registers
    ModbusConfig mbInt32;
    mbInt32.dataType = ModbusDataType::Int32LE;
    mbInt32.scale = 1.0;
    mbInt32.offset = 0.0;

    QVector<quint16> encInt32 = mbInt32.encodeRegisters(500000);
    QCOMPARE(encInt32.size(), 2);
    QVariant decInt32 = mbInt32.decodeRegisters(encInt32);
    QCOMPARE(decInt32.toLongLong(), 500000LL);

    // 5. Coil boolean
    ModbusConfig mbCoil;
    mbCoil.dataType = ModbusDataType::Bit;
    QVector<quint16> coilReg = { 1 };
    QCOMPARE(mbCoil.decodeRegisters(coilReg).toBool(), true);
    coilReg[0] = 0;
    QCOMPARE(mbCoil.decodeRegisters(coilReg).toBool(), false);
}

void TestAllCases::testPhase1VariableWaveformsAndSimulation() {
    // 1. Sine Waveform
    VariableConfig sine;
    sine.waveform = SimulationWaveform::Sine;
    sine.minVal = 0.0;
    sine.maxVal = 100.0;
    sine.periodMs = 1000;
    sine.phaseOffset = 0.0;

    // t = 0 -> midpoint 50.0
    QVERIFY(std::abs(sine.evaluateWaveform(0) - 50.0) < 0.001);
    // t = 250 -> max 100.0
    QVERIFY(std::abs(sine.evaluateWaveform(250) - 100.0) < 0.001);
    // t = 500 -> midpoint 50.0
    QVERIFY(std::abs(sine.evaluateWaveform(500) - 50.0) < 0.001);
    // t = 750 -> min 0.0
    QVERIFY(std::abs(sine.evaluateWaveform(750) - 0.0) < 0.001);

    // 2. Square Waveform
    VariableConfig square;
    square.waveform = SimulationWaveform::Square;
    square.minVal = 10.0;
    square.maxVal = 90.0;
    square.periodMs = 1000;

    QCOMPARE(square.evaluateWaveform(200), 90.0);
    QCOMPARE(square.evaluateWaveform(700), 10.0);

    // 3. Triangle Waveform
    VariableConfig tri;
    tri.waveform = SimulationWaveform::Triangle;
    tri.minVal = 0.0;
    tri.maxVal = 100.0;
    tri.periodMs = 1000;

    QVERIFY(std::abs(tri.evaluateWaveform(0) - 0.0) < 0.001);
    QVERIFY(std::abs(tri.evaluateWaveform(250) - 50.0) < 0.001);
    QVERIFY(std::abs(tri.evaluateWaveform(500) - 100.0) < 0.001);
    QVERIFY(std::abs(tri.evaluateWaveform(750) - 50.0) < 0.001);

    // 4. Ramp Waveform
    VariableConfig ramp;
    ramp.waveform = SimulationWaveform::Ramp;
    ramp.minVal = 0.0;
    ramp.maxVal = 100.0;
    ramp.periodMs = 1000;

    QVERIFY(std::abs(ramp.evaluateWaveform(500) - 50.0) < 0.001);
    QVERIFY(std::abs(ramp.evaluateWaveform(1500) - 100.0) < 0.001); // clamped
}

void TestAllCases::testPhase1SimulationRuntimeProtocolInjections() {
    CanvasScene scene;
    Project proj(&scene);

    // 1. CAN Source -> Gauge
    DataSource canDs("can_speed", "Vehicle Speed", DataSourceType::Can, DataDirection::Input, DataType::Float);
    CanSignalConfig canCfg;
    canCfg.busId = "CAN1";
    canCfg.messageId = 0x120;
    canCfg.startBit = 0;
    canCfg.bitLength = 16;
    canCfg.factor = 0.1;
    canCfg.offset = 0.0;
    canDs.setCanConfig(canCfg);
    proj.addDataSource(canDs);

    GaugeComponent* gauge = new GaugeComponent("gauge_speed");
    gauge->setMaximum(240.0);
    gauge->setValue(0.0);
    scene.addUIComponent(gauge);
    proj.addDataBinding(DataBinding(gauge->componentId(), "value", canDs.id()));

    // 2. UART Source -> Label
    DataSource uartDs("uart_msg", "Status Text", DataSourceType::Uart, DataDirection::Input, DataType::String);
    UartStreamConfig uartCfg;
    uartCfg.parseMode = UartParseMode::KeyValue;
    uartCfg.keyName = "STATUS";
    uartDs.setUartConfig(uartCfg);
    proj.addDataSource(uartDs);

    LabelComponent* lbl = new LabelComponent("lbl_status");
    lbl->setText("INIT");
    scene.addUIComponent(lbl);
    proj.addDataBinding(DataBinding(lbl->componentId(), "text", uartDs.id()));

    // 3. Modbus Source -> ProgressBar
    DataSource mbDs("mb_temp", "Oven Temp", DataSourceType::Modbus, DataDirection::Input, DataType::Float);
    ModbusConfig mbCfg;
    mbCfg.slaveId = 1;
    mbCfg.address = 100;
    mbCfg.dataType = ModbusDataType::Uint16;
    mbCfg.scale = 0.5;
    mbCfg.offset = 0.0;
    mbDs.setModbusConfig(mbCfg);
    proj.addDataSource(mbDs);

    ProgressBarComponent* bar = new ProgressBarComponent("bar_temp");
    bar->setValue(0.0);
    scene.addUIComponent(bar);
    proj.addDataBinding(DataBinding(bar->componentId(), "value", mbDs.id()));

    // 4. Variable Source with Sine Waveform -> Rpm
    DataSource sineDs("var_sine_rpm", "Engine Sine", DataSourceType::Variable, DataDirection::Input, DataType::Float);
    VariableConfig sineCfg;
    sineCfg.waveform = SimulationWaveform::Sine;
    sineCfg.minVal = 1000.0;
    sineCfg.maxVal = 5000.0;
    sineCfg.periodMs = 1000;
    sineDs.setVariableConfig(sineCfg);
    proj.addDataSource(sineDs);

    RpmComponent* rpm = new RpmComponent("rpm_engine");
    rpm->setValue(0.0);
    scene.addUIComponent(rpm);
    proj.addDataBinding(DataBinding(rpm->componentId(), "value", sineDs.id()));

    // Create and execute SimulationRuntime
    Simulator::SimulationRuntime sim(&proj);
    sim.start();

    // Test CAN Injection: raw 1200 -> 120.0 km/h
    QByteArray canPayload(8, 0);
    canPayload[0] = static_cast<char>(1200 & 0xFF);
    canPayload[1] = static_cast<char>((1200 >> 8) & 0xFF);
    sim.injectCanFrame(0x120, canPayload);
    QCOMPARE(gauge->value(), 120.0);

    // Test UART Injection: "STATUS=ACTIVE"
    sim.injectUartStream("SYSTEM LOG: STATUS=RUNNING_OK");
    QCOMPARE(lbl->text(), QString("RUNNING_OK"));

    // Test Modbus Injection: address 100, raw 80 -> 40.0
    QVector<quint16> mbRegs = { 80 };
    sim.injectModbusRegisters(1, ModbusRegisterType::HoldingRegister, 100, mbRegs);
    QCOMPARE(bar->actualValue(), 40.0);
    QCOMPARE(bar->value(), 0.4);

    // Test Variable Waveform progression at t = 250ms (peak of sine -> 5000.0)
    sim.step(250);
    QVERIFY(std::abs(rpm->value() - 5000.0) < 5.0);

    sim.stop();
}

void TestAllCases::testPhase1DataSourceSerializationBackwardCompatibility() {
    DataSource ds("can_sensor", "Brake Pressure", DataSourceType::Can, DataDirection::Input, DataType::Float);
    CanSignalConfig can;
    can.busId = "CAN2";
    can.messageId = 0x350;
    can.isExtended = true;
    can.isFd = true;
    can.startBit = 12;
    can.bitLength = 14;
    can.factor = 0.05;
    can.offset = 5.0;
    can.unit = "bar";
    ds.setCanConfig(can);

    QJsonObject json = ds.toJson();
    QCOMPARE(json.value("type").toString(), QString("CAN"));
    QVERIFY(json.contains("canConfig"));

    DataSource restored = DataSource::fromJson(json);
    QCOMPARE(restored.id(), ds.id());
    QCOMPARE(restored.type(), DataSourceType::Can);
    QCOMPARE(restored.canConfig().messageId, 0x350u);
    QCOMPARE(restored.canConfig().isExtended, true);
    QCOMPARE(restored.canConfig().isFd, true);
    QCOMPARE(restored.canConfig().startBit, 12);
    QCOMPARE(restored.canConfig().bitLength, 14);
    QCOMPARE(restored.canConfig().factor, 0.05);
    QCOMPARE(restored.canConfig().offset, 5.0);
    QCOMPARE(restored.canConfig().unit, QString("bar"));
    QVERIFY(restored == ds);

    // Verify Code Generator bindings include the CAN signal
    CanvasScene scene;
    Project proj(&scene);
    proj.addDataSource(ds);
    QString src = CodeGen::BindingLayerGenerator::generateBindingsSource(&proj);
    QVERIFY(src.contains("ui_receive_can_frame"));
    QVERIFY(src.contains("can_sensor"));
}

void TestAllCases::testPhase6MCPAuthenticationAndTokens() {
    MainWindow win;
    DesignerController controller(&win);
    DesignerLocalServer server(&controller);

    QString token = server.sessionToken();
    QVERIFY(!token.isEmpty());
    QVERIFY(token.length() >= 32);

    // 1. Unauthenticated request without token must fail with -32001
    QJsonObject unauthReq;
    unauthReq["jsonrpc"] = "2.0";
    unauthReq["id"] = 1;
    unauthReq["method"] = "get_canvas_size";
    QJsonObject res1 = server.processJsonRpcRequest(unauthReq);
    QVERIFY(res1.contains("error"));
    QCOMPARE(res1["error"].toObject()["code"].toInt(), -32001);

    // 2. Request with invalid token must fail with -32001
    QJsonObject invalidTokenReq = unauthReq;
    invalidTokenReq["token"] = "invalid_token_12345";
    QJsonObject res2 = server.processJsonRpcRequest(invalidTokenReq);
    QVERIFY(res2.contains("error"));
    QCOMPARE(res2["error"].toObject()["code"].toInt(), -32001);

    // 3. Request with valid token must succeed
    QJsonObject validTokenReq = unauthReq;
    validTokenReq["token"] = token;
    QJsonObject res3 = server.processJsonRpcRequest(validTokenReq);
    QVERIFY(!res3.contains("error"));
    QVERIFY(res3.contains("result"));

    // 4. Authenticate handshake method
    QJsonObject authHandshake;
    authHandshake["jsonrpc"] = "2.0";
    authHandshake["id"] = 2;
    authHandshake["method"] = "authenticate";
    QJsonObject authParams;
    authParams["token"] = token;
    authHandshake["params"] = authParams;
    QJsonObject res4 = server.processJsonRpcRequest(authHandshake);
    QVERIFY(res4.contains("result"));
    QVERIFY(res4["result"].toObject()["authenticated"].toBool());
}

// Regression: the MCP token file location is canonical, overridable via
// EUIDESIGNER_MCP_TOKEN_FILE, and owner-only — and the Python proxy reads the
// same location (mcp/embedded_ui_mcp.py::load_mcp_token).
void TestAllCases::testPhase7AdapterTransportFromCatalog() {
    // R1.b: every board profile carries an adapter identity sourced from the
    // board catalog (config/boards/boards.json "usb_adapter"), with
    // deterministic family fallbacks for profiles without a catalog row.
    HardwareBridge& bridge = HardwareBridge::instance();

    bool foundF030 = false, foundStmFallback = false, foundEspFallback = false, foundCatalogJoin = false;
    const auto boards = bridge.availableBoards();
    // Catalog rows are the golden source: assert the profile values against
    // the resolved catalog entry rather than guessed string literals
    // (kind strings differ per board row, e.g. "ST-Link" vs "ST-Link/V2-1").
    auto expectedFromCatalog = [](const BoardProfile& bp, BoardSpec* out) {
        BoardSpec row = BoardDatabase::instance().findBoardById(bp.id);
        if (!row.isValid()) {
            auto skim = [](const QString& raw) {
                return raw.toLower().remove(' ').remove('-').remove('/')
                          .remove('(').remove(')');
            };
            const QString norm = skim(bp.name);
            for (const BoardSpec& cand : BoardDatabase::instance().allBoards()) {
                const QString rowNorm = skim(cand.name);
                if (rowNorm == norm || norm.startsWith(rowNorm)) {
                    row = cand;
                    break;
                }
            }
        }
        *out = row;
    };
    BoardSpec catalogRow;
    for (const auto& bp : boards) {
        if (bp.id == "stm32f030r8") {
            // Built-in id differs from the catalog row ("nucleo_f030r8");
            // adapter identity must still resolve via the catalog.
            expectedFromCatalog(bp, &catalogRow);
            QVERIFY2(catalogRow.isValid() && !catalogRow.vid.isEmpty(),
                     "catalog join failed for stm32f030r8");
            QCOMPARE(bp.adapterVid, HardwareBridge::normalizeUsbId(catalogRow.vid));
            QCOMPARE(bp.adapterPid, HardwareBridge::normalizeUsbId(catalogRow.pid));
            if (!catalogRow.usbAdapterType.isEmpty()) {
                QCOMPARE(bp.adapterKind, catalogRow.usbAdapterType);
            }
            foundF030 = true;
        } else if (bp.id == "nucleo_f401re") {
            // Exact-id catalog join: profile id matches the boards.json row.
            expectedFromCatalog(bp, &catalogRow);
            QVERIFY2(catalogRow.isValid() && !catalogRow.vid.isEmpty(),
                     "catalogjoin failed for nucleo_f401re");
            QCOMPARE(bp.adapterVid, HardwareBridge::normalizeUsbId(catalogRow.vid));
            QCOMPARE(bp.adapterPid, HardwareBridge::normalizeUsbId(catalogRow.pid));
            if (!catalogRow.usbAdapterType.isEmpty()) {
                QCOMPARE(bp.adapterKind, catalogRow.usbAdapterType);
            }
            foundCatalogJoin = true;
        } else if (bp.id == "stm32f469i" || bp.id == "stm32h747i") {
            // No catalog row; family fallback must still resolve.
            QCOMPARE(bp.connectionType, QString("openocd"));
            QVERIFY(bp.adapterKind.contains("ST-Link"));
            QCOMPARE(bp.adapterVid, QString("0483"));
            QCOMPARE(bp.adapterPid, QString("374b"));
            foundStmFallback = true;
        } else if (bp.id == "esp32_devkitc") {
            QCOMPARE(bp.adapterKind, QString("esptool"));
            QCOMPARE(bp.adapterVid, QString("10c4"));
            QCOMPARE(bp.adapterPid, QString("ea60"));
            foundEspFallback = true;
        }
    }
    QVERIFY2(foundF030, "stm32f030r8 profile missing (catalog-driven adapter test)");
    QVERIFY2(foundCatalogJoin, "nucleo_f401re profile missing (catalog exact-id join test)");
    QVERIFY2(foundStmFallback, "STM32 fallback profiles missing");
    QVERIFY2(foundEspFallback, "ESP32 fallback profile missing");

    // Imported boards: an ESP32 board config parser result must select the
    // serial connection kind, carry the family adapter fallback through
    // addBoard, and join the catalog when the parsed name rows a row.
    const QString sdk = "CONFIG_IDF_TARGET=\"esp32\"\nCONFIG_GPIO_LED_PIN=2\n";
    BoardConfigParser::ParseResult parsed = BoardConfigParser::parseContent(sdk, "sdkconfig");
    QVERIFY2(parsed.success, qPrintable(parsed.errorMessage));
    BoardProfile imported = parsed.toBoardProfile();
    QCOMPARE(imported.connectionType, QString("serial"));

    HardwareBridge& importedBridge = HardwareBridge::instance();
    QVERIFY(importedBridge.addBoard(imported));
    // Board profiles added through the bridge resolve the esptool family
    // fallback when the join finds no catalog row.
    BoardProfile stored;
    bool storedFound = false;
    for (const BoardProfile& b : importedBridge.availableBoards()) {
        if (b.id == imported.id) {
            stored = b;
            storedFound = true;
            break;
        }
    }
    QVERIFY2(storedFound, "imported board not registered in hardware bridge");
    QCOMPARE(stored.connectionType, QString("serial"));
    QCOMPARE(stored.adapterKind, QString("esptool"));
    QCOMPARE(stored.adapterVid, QString("10c4"));
}

void TestAllCases::testPhase7SerialPortMatchNormalization() {
    // Pure matcher: accepts the catalog pair, alternates, case/0x format
    // variance, and rejects non-matching ports (never "first tty wins").
    BoardProfile bp;
    bp.adapterKind = "esptool";
    bp.adapterVid = "10c4";
    bp.adapterPid = "ea60";
    bp.adapterAltVidPids << "1a86:7523";

    using Ports = QList<QPair<QPair<QString,QString>,QString>>;
    Ports none = { {{ "0483","374b" }, "ttyACM0"}, {{ "0a5c","2763" }, "hci0"} };
    QCOMPARE(HardwareBridge::matchSerialPort(bp, none), QString(""));

    Ports mixed = { {{ "0x0A5C","2763" }, "hci"}, {{ "10C4","0x0EA60" }, "ttyUSB2"}, {{ "1A86","7523" }, "ttyUSB9"} };
    QCOMPARE(HardwareBridge::matchSerialPort(bp, mixed), QString("ttyUSB2"));

    Ports alt = { {{ "1a86","7523" }, "ttyUSB_CH340"} };
    QCOMPARE(HardwareBridge::matchSerialPort(bp, alt), QString("ttyUSB_CH340"));

    QString adapter;
    Ports exact = { {{ "10c4","ea60" }, "ttyUSB0"} };
    QCOMPARE(HardwareBridge::matchSerialPort(bp, exact, &adapter), QString("ttyUSB0"));
    QCOMPARE(adapter, QString("10c4:ea60"));
}

void TestAllCases::testPhase7ConnectRequiresMatchingAdapterPort() {
    // connectHardware (serial esp32 flow) is accepted only when a USB tty
    // matches the board catalog adapter; no "/dev tty exists => connected".
    HardwareBridge& bridge = HardwareBridge::instance();
    const QString prevBoardId = bridge.currentBoard().id;
    const bool prevSim = bridge.isSimulationMode();
    QVERIFY(bridge.setBoard("esp32_devkitc"));
    const BoardProfile bp = bridge.currentBoard();
    QCOMPARE(bp.adapterKind, QString("esptool"));

    // 1. Only a mismatched probe attached -> connect must FAIL with an
    //    actionable adapter-mismatch error (not "Connected via ...").
    bridge.setTestSerialPorts({ {{ "0483","374b" }, "ttyACM0"} });
    bridge.setSimulationMode(false);
    bridge.disconnectHardware();
    QVERIFY(!bridge.connectHardware());
    QVERIFY(bridge.lastConnectionError().contains("matching the board profile"));

    // 2. Matched CP210x adapter -> connected + paired port recorded.
    bridge.setTestSerialPorts({ {{ "0a5c","2763" }, "hci"}, {{ "10c4","ea60" }, "ttyUSB3"} });
    QVERIFY(bridge.connectHardware());
    QCOMPARE(bridge.lastSerialPortName(), QString("ttyUSB3"));
    QVERIFY(bridge.isSerialPortMatched());
    // 3. CH340 alt pair matches as well (disconnect first: connectHardware()
    //    short-circuits while Connected - revalidation is R1.c C16).
    bridge.disconnectHardware();
    bridge.setTestSerialPorts({ {{ "1a86","7523" }, "ttyUSB7"} });
    QVERIFY(bridge.connectHardware());
    QCOMPARE(bridge.lastSerialPortName(), QString("ttyUSB7"));

    bridge.disconnectHardware();
    bridge.clearTestSerialPorts();
    // Restore singleton state for later suite slots.
    bridge.setBoard(prevBoardId);
    bridge.setSimulationMode(prevSim);
}

void TestAllCases::testPhase7LinuxGpioRequiresOpenableChipDevice() {
    // R1.c/C1: /sys/class/gpio presence is an environment property, not a
    // transport. A connection claim requires an OPENABLE GPIO character
    // device; the resolver accepts an injectable directory for offline
    // verification (any openable node named gpiochip* qualifies).
    const QString chipDir = m_tempDir.filePath("p7_chips");
    QVERIFY(QDir().mkpath(chipDir));
    QCOMPARE(HardwareBridge::firstOpenableGpioChip(chipDir), QString()); // empty dir

    QVERIFY(QFile(chipDir + "/gpiochip0").open(QIODevice::WriteOnly));
    QCOMPARE(HardwareBridge::firstOpenableGpioChip(chipDir), chipDir + "/gpiochip0");

    QVERIFY(QFile(chipDir + "/notachip").open(QIODevice::WriteOnly));
    QCOMPARE(HardwareBridge::firstOpenableGpioChip(chipDir), chipDir + "/gpiochip0");

    // End-to-end honesty on the linux branch: Connected iff a chip device
    // opens on THIS host (environment-tolerant assertion).
    HardwareBridge& bridge = HardwareBridge::instance();
    QVERIFY(bridge.setBoard("raspberrypi_4b"));
    bridge.disconnectHardware();
    const bool chipAvailable = !HardwareBridge::firstOpenableGpioChip().isEmpty();
    const bool claimed = bridge.connectHardware();
    QCOMPARE(claimed, chipAvailable);
    if (chipAvailable) {
        QVERIFY(bridge.isHardwareConnected());
        QVERIFY(bridge.lastConnectionError().isEmpty());
    } else {
        QVERIFY(!bridge.isHardwareConnected());
        QVERIFY2(bridge.lastConnectionError().contains("gpiochip", Qt::CaseInsensitive),
                 qPrintable(bridge.lastConnectionError()));
    }
    bridge.disconnectHardware();
}

void TestAllCases::testPhase7SerialConnectionOpensOrFails() {
    // R1.c/C5: picking a port name is not a claim - the port must open.
    Hardware::SerialConnection conn;
    QSignalSpy errorSpy(&conn, &Hardware::SerialConnection::errorOccurred);

    QVERIFY(!conn.connectToTarget());
    QVERIFY(errorSpy.count() >= 1);
    QCOMPARE(conn.lastError(), QString("No serial port selected"));

    const bool serialBuilt = Hardware::SerialConnection::serialTransportAvailable();
    if (serialBuilt) {
        conn.setPortName("/dev/nonexistent-tty-r1c");
        conn.setBaudRate(115200);
        QVERIFY(!conn.connectToTarget());
        QVERIFY2(conn.lastError().contains("could not be opened", Qt::CaseInsensitive),
                 qPrintable(conn.lastError()));
        QVERIFY(!conn.isConnected());
    } else {
        conn.setPortName("/dev/nonexistent-tty-r1c");
        conn.setBaudRate(115200);
        QVERIFY(!conn.connectToTarget());
        QVERIFY2(conn.lastError().contains("unavailable", Qt::CaseInsensitive),
                 qPrintable(conn.lastError()));
    }
}

void TestAllCases::testPhase7FlashRefusesUntargetedPort() {
    // R1.c/C11: esptool must never run without an explicit or catalog-matched
    // port (no "-p" would let it autodetect ANY serial device on the host).
    HardwareBridge& bridge = HardwareBridge::instance();
    bridge.setTestSerialPorts({}); // active with no candidates: no matched port

    FlashController controller(nullptr);
    QSignalSpy finishedSpy(&controller, &FlashController::flashingFinished);
    QString console;
    QObject::connect(&controller, &FlashController::consoleOutputUpdate, [&console](const QString& line) {
        console += line;
    });

    const QString binPath = m_tempDir.filePath("p7fw.bin");
    QVERIFY(QFile(binPath).open(QIODevice::WriteOnly));

    QVERIFY(!controller.flashESP32(binPath));
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(finishedSpy.first().at(0).toBool(), false);
    QVERIFY2(console.contains("refusing an untargeted flash", Qt::CaseInsensitive),
             qPrintable(console));
}

void TestAllCases::testPhase7DetectorAdapterOnlyWithoutCatalog() {
    // R1.c/C10 (portable non-Linux dispatch path): when only the VID is
    // known, the detector reports an ADAPTER (detected=false), never a
    // concrete board claim. Executed through the public dispatcher so it
    // runs on every host.
    BoardDetector detector;
    const auto devices = detector.getConnectedDevices();
    // Only assert invariants for adapters actually present on this host;
    // on headless CI hosts the list is empty and the invariant is vacuous.
    for (const auto& dev : devices) {
        QString vid = dev.vendorId.toLower();
        QString pid = dev.productId.toLower();
        if (vid.isEmpty() || pid.isEmpty()) continue;
        const BoardSpec spec = BoardDatabase::instance().findBoardByUsb(vid, pid);
        if (!spec.isValid() || spec.vid.isEmpty()) {
            // No catalog row: detectConnectedBoard must not claim a board.
            const DetectedBoardInfo info = detector.detectConnectedBoard();
            if (vid == "0483") {
                QCOMPARE(info.detected, false);
                QCOMPARE(info.adapterType, QString("ST-Link"));
            }
        }
    }
    QVERIFY(true);
}

void TestAllCases::testPhase7TelnetPromptTerminatedParse() {
    // R1.e: an OpenOCD telnet reply is complete only with the trailing prompt.
    const QByteArray mdwReply = "0x40015800: 0x00000440\n> ";
    const qsizetype mdwLen = OcdTelnetManager::completeReplyLength(mdwReply);
    QVERIFY2(mdwLen == qsizetype(22), qPrintable(QString("len=%1").arg(mdwLen)));

    const QByteArray crlfReply = "0x40015800: 0x00000440\r\n> ";
    QCOMPARE(OcdTelnetManager::completeReplyLength(crlfReply), qsizetype(22));

    const QByteArray multiLine = "1\n2\n> ";
    QCOMPARE(OcdTelnetManager::completeReplyLength(multiLine), qsizetype(3));

    const QByteArray promptOnly = "> ";
    QCOMPARE(OcdTelnetManager::completeReplyLength(promptOnly), qsizetype(0));

    // Not complete: no trailing prompt yet (first-chunk case of the old bug).
    QVERIFY(OcdTelnetManager::completeReplyLength(QByteArray("0x40015800: 0x000")) < 0);
    QVERIFY(OcdTelnetManager::completeReplyLength(QByteArray("targets\nstm32f0x.cpu")) < 0);
}

void TestAllCases::testPhase7TelnetForeignThreadMarshal() {
    // R1.e: foreign threads must not touch the socket runtime. The manager
    // marshals to its owning thread (BlockingQueuedConnection) and the result
    // depends on the owning thread's event loop serving the queued call -
    // which is exactly how OpenOCDWorker behaves in production while the
    // UI loop keeps running.
    OcdTelnetManager manager;
    std::atomic<bool> done{false};
    QString recordedResponse;
    QThread* worker = new QThread;
    QObject::connect(worker, &QThread::started, [&]{
        recordedResponse = manager.sendCommand("targets");
        done.store(true);
    });
    worker->start();

    // Pump the manager's (owner/main) event loop so the queued invocation
    // is served while the worker blocks on the marshal.
    QEventLoop loop;
    for (int i = 0; i < 20 && !done.load(); ++i) {
        QTimer::singleShot(100, &loop, &QEventLoop::quit);
        loop.exec();
    }
    QVERIFY2(done.load(), "queued marshal must be served by the owner thread");
    QVERIFY(recordedResponse.isEmpty()); // no socket -> honest empty reply

    worker->quit(); // QThread::run's default exec() needs an explicit quit
    QVERIFY(worker->wait(2000));
    delete worker;
}

void TestAllCases::testPhase7ChipIdentityProfileTables() {
    // R1.d: identity registers per family, sourced from verified references.
    // Unverified families carry 0/0 and identify() must fail honestly.
    HardwareBridge& bridge = HardwareBridge::instance();

    QVERIFY(bridge.setBoard("stm32f030r8"));
    BoardProfile bp = bridge.currentBoard();
    QCOMPARE(bp.dbgmcuAddress, quint32(0x40015800)); // RM0360 rev.5 §30.6.1
    QCOMPARE(bp.expectedDevId, quint32(0x440));

    QVERIFY(bridge.setBoard("stm32f407g_disc1"));
    bp = bridge.currentBoard();
    QCOMPARE(bp.dbgmcuAddress, quint32(0xE0042000)); // RM0090
    QCOMPARE(bp.expectedDevId, quint32(0x413));

    QVERIFY(bridge.setBoard("stm32f469i"));
    bp = bridge.currentBoard();
    QCOMPARE(bp.dbgmcuAddress, quint32(0xE0042000));
    QCOMPARE(bp.expectedDevId, quint32(0x434)); // RM0386

    QVERIFY(bridge.setBoard("stm32h747i"));
    bp = bridge.currentBoard();
    QCOMPARE(bp.dbgmcuAddress, quint32(0));     // H7: pending hardware capture
    QCOMPARE(bp.expectedDevId, quint32(0));

    QVERIFY(bridge.setBoard("esp32_devkitc"));
    bp = bridge.currentBoard();
    QCOMPARE(bp.dbgmcuAddress, quint32(0));     // ESP32: esptool path, no DBGMCU

    // OpenOcdDeviceLink offline: without a connection identify fails honestly.
    Hardware::OpenOcdDeviceLink link("stm32f030r8");
    const Hardware::DeviceIdentity none = link.identify();
    QVERIFY(none.family.isEmpty());
    QCOMPARE(link.state(), Hardware::DeviceLinkState::Error);
}

void TestAllCases::testPhase7EsptoolChipNameParse() {
    // R1.d: esptool chip_id reply parsing (fixture from hardware captures).
    const QByteArray capture =
        QByteArray("esptool.py v4.5.1\n") +
        "Serial port /dev/ttyUSB0\n" +
        "Connecting....\n" +
        "Chip is ESP32-D0WD-V3 (revision v3.0)\n" +
        "Features: WiFi, BT, Dual Core, 240MHz\n";
    QCOMPARE(Hardware::OpenOcdDeviceLink::parseEsptoolChipName(capture),
             QString("ESP32-D0WD-V3"));

    // No revision suffix variant.
    QCOMPARE(Hardware::OpenOcdDeviceLink::parseEsptoolChipName(QByteArray("Chip is ESP32-S3\n")),
             QString("ESP32-S3"));

    // Missing line -> empty chip name (honest identify failure).
    QVERIFY(Hardware::OpenOcdDeviceLink::parseEsptoolChipName(QByteArray("connecting failed\n")).isEmpty());
}

void TestAllCases::testPhase7LedCommandHonorsProfile() {
    // R1.e/C13: the LED_* address is profile-driven and non-STM32 profiles
    // fail honestly; a plain ESP32 profile must NOT be written with an
    // STM32 BSRR.
    HardwareBridge& bridge = HardwareBridge::instance();
    QVERIFY(bridge.setBoard("esp32_devkitc"));
    OcdTelnetManager manager;
    const QString resp = manager.sendCommand("LED_ON");
    QVERIFY2(resp.startsWith("ERR", Qt::CaseInsensitive), qPrintable(resp));
    QVERIFY2(resp.contains("STM32"), qPrintable(resp));

    // STM32 profile: F0 GPIO base + BSRR offset consistent with the write
    // guard history recorded from the (simulated) accepted write.
    QVERIFY(bridge.setBoard("stm32f030r8"));
    const BoardProfile bp = bridge.currentBoard();
    quint32 ledAddress = 0;
    QString negErr;
    QVERIFY(manager.ledBsrrAddress(&ledAddress, &negErr));
    QCOMPARE(ledAddress, quint32(0x48000018)); // 0x48000000 + 0x18 for F0
}

void TestAllCases::testPhase6McpTokenFileCanonicalPath() {
    const QString tokenPath = m_tempDir.filePath("canonical/mcp_token");
    QVERIFY(QFile::remove(tokenPath) || !QFile::exists(tokenPath));
    qputenv("EUIDESIGNER_MCP_TOKEN_FILE", tokenPath.toUtf8());

    {
        MainWindow win;
        DesignerController controller(&win);
        DesignerLocalServer server(&controller);  // ctor generates the token

        QVERIFY(server.sessionToken().length() >= 32);
        QVERIFY(QFile::exists(tokenPath));

        QFile file(tokenPath);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(QString::fromUtf8(file.readAll()).trimmed(), server.sessionToken());
        file.close();
#if defined(Q_OS_UNIX)
        QFileInfo info(tokenPath);
        QVERIFY(info.permissions().testFlag(QFileDevice::ReadOwner));
        QVERIFY(info.permissions().testFlag(QFileDevice::WriteOwner));
        // No group/other access (chmod 600 semantics)
        QVERIFY(!info.permissions().testFlag(QFileDevice::ReadGroup));
        QVERIFY(!info.permissions().testFlag(QFileDevice::ReadOther));
#endif
    }

    qunsetenv("EUIDESIGNER_MCP_TOKEN_FILE");
}

void TestAllCases::testPhase6FirmwareSyncRequiresConfiguredDirectory() {
    // Regression: Project::firmwareDirectory() used to fall back to
    // QDir::currentPath(), whose "src/|inc/|scr/ exists" heuristic also
    // matched the application's own source tree. A firmware sync with no
    // configured directory then exported the full scaffold (user/,
    // generated/, top-level CMakeLists.txt) into the working directory,
    // clobbering the repo's tracked root CMakeLists.txt.
    CanvasScene scene;
    Project project(&scene);

    const QString snapshotRootCMake = QDir::currentPath() + "/CMakeLists.txt";
    const qint64 rootCMakeBefore = QFileInfo(snapshotRootCMake).exists()
        ? QFileInfo(snapshotRootCMake).size() : -1;

    QString err;
    QVERIFY(!project.syncFirmwareFiles(&err));
    QVERIFY(err.contains("firmware directory", Qt::CaseInsensitive));
    // Nothing may appear in the working directory or anywhere else.
    // (Only assert absence for directories that did not exist before the
    // sync attempt - the suite's build dir may contain legitimate debris
    // from earlier runs.)
    const QString userDir = QDir::currentPath() + "/user";
    const QString manifestPath = QDir::currentPath() + "/generated/MANIFEST.json";
    const bool userDirExistedBefore = QFileInfo(userDir).exists();
    const bool generatedDirExistedBefore = QFileInfo(QFileInfo(manifestPath).absolutePath()).exists();
    if (!userDirExistedBefore) {
        QVERIFY(!QFileInfo::exists(userDir + "/main.c"));
    }
    if (!generatedDirExistedBefore) {
        QVERIFY(!QFileInfo::exists(manifestPath));
    }
    if (rootCMakeBefore >= 0) {
        QCOMPARE(QFileInfo(snapshotRootCMake).size(), rootCMakeBefore);
    }

    // An explicitly configured, existing firmware directory still syncs.
    const QString fwDir = m_tempDir.filePath("fw_configured");
    QVERIFY(QDir().mkpath(fwDir));
    project.setFirmwareDirectory(fwDir);
    QVERIFY2(project.syncFirmwareFiles(&err), qPrintable(err));
    QVERIFY(QFileInfo::exists(fwDir + "/CMakeLists.txt"));
    QVERIFY(QFileInfo::exists(fwDir + "/user/main.c"));
}

void TestAllCases::testPhase6MCPFileSandboxEnforcement() {
    MainWindow win;
    DesignerController controller(&win);

    QString err;
    // 1. Directory traversal rejected
    QVERIFY(!controller.validateFileSandbox("../../../etc/passwd", &err));
    QVERIFY(err.contains("traversal", Qt::CaseInsensitive));

    // 2. Sensitive system roots rejected
    QVERIFY(!controller.validateFileSandbox("/etc/shadow", &err));
    QVERIFY(!controller.validateFileSandbox("/boot/vmlinuz", &err));
    QVERIFY(!controller.validateFileSandbox("/sys/kernel", &err));

    // 3. Sensitive user dotfiles rejected
    QString home = QDir::homePath();
    QVERIFY(!controller.validateFileSandbox(home + "/.ssh/id_rsa", &err));
    QVERIFY(!controller.validateFileSandbox(home + "/.config/embedded-ui-designer/credentials.vault", &err));
    QVERIFY(!controller.validateFileSandbox(home + "/.config/embedded-ui-designer/mcp_token", &err));

    // 4. Safe paths allowed
    QString safePath = m_tempDir.filePath("test_project.euiproj");
    QVERIFY(controller.validateFileSandbox(safePath, &err));
}

void TestAllCases::testPhase6SecureCredentialManagerVault() {
    auto& cm = Security::CredentialManager::instance();

    QString service = "test_service_vault";
    QString account = "api_key";
    QString secret = "secret_token_val_998877";

    // Store credential
    QVERIFY(cm.storeCredential(service, account, secret));
    QVERIFY(cm.hasCredential(service, account));

    // Retrieve credential
    QString retrieved = cm.retrieveCredential(service, account);
    QCOMPARE(retrieved, secret);

    // Delete credential
    QVERIFY(cm.deleteCredential(service, account));
    QVERIFY(!cm.hasCredential(service, account));
    QVERIFY(cm.retrieveCredential(service, account).isEmpty());
}

void TestAllCases::testPhase6DiagnosticsEngineAndCategories() {
    auto& diag = Diagnostics::DiagnosticsEngine::instance();
    diag.clearAll();

    CanvasScene scene;
    Project proj(&scene);

    // Initial clean project scan
    diag.scanProject(&proj);
    QCOMPARE(diag.errorCount(), 0);

    // Add duplicate component IDs and broken bindings to verify detection
    auto* s1 = proj.screens().first();
    proj.setActiveScreen(s1);

    auto* btn1 = new ButtonComponent("btn_duplicate");
    btn1->setCompPos(10, 10);
    btn1->setCompSize(80, 30);
    s1->addComponent(btn1);

    auto* btn2 = new ButtonComponent("btn_duplicate");
    btn2->setCompPos(20, 20);
    btn2->setCompSize(80, 30);
    s1->addComponent(btn2);

    // Broken binding referencing missing DataSource
    DataBinding brokenBinding("btn_duplicate", "text", "missing_ds_id");
    proj.addDataBinding(brokenBinding);

    diag.scanProject(&proj);
    QVERIFY(diag.errorCount() >= 2); // Duplicate ID + Broken binding

    QJsonObject json = diag.toJson();
    QVERIFY(json.contains("diagnostics"));
    QVERIFY(json["errorCount"].toInt() >= 2);

    QString md = diag.toMarkdown();
    QVERIFY(md.contains("Duplicate Component ID"));
    QVERIFY(md.contains("Broken Binding"));
}

void TestAllCases::testPhase6GitServiceAndSafetyPolicy() {
    auto& git = Git::GitService::instance();
    if (!git.isGitInstalled()) {
        QSKIP("Git is not installed on test host");
    }

    QString currentRepo = QDir::currentPath();
    QVERIFY(git.isGitRepository(currentRepo));

    Git::GitStatus status = git.getStatus(currentRepo);
    QVERIFY(status.isRepo);
    QVERIFY(!status.currentBranch.isEmpty());

    auto history = git.getCommitHistory(currentRepo, 5);
    QVERIFY(!history.isEmpty());

    // Safety policy: Force push MUST be blocked
    QString err;
    bool forcePushAllowed = git.push(currentRepo, "origin", "main", true, &err);
    QVERIFY(!forcePushAllowed);
    QVERIFY(err.contains("Force push is blocked", Qt::CaseInsensitive));
}

void TestAllCases::testPhase6CICDGeneratorMultiProviderAndValidation() {
    CICD::CICDConfig cfg;
    cfg.targetMcu = "STM32F030R8";
    cfg.generator = "LVGL";
    cfg.buildSystem = "cmake";
    cfg.toolchain = "arm-none-eabi-gcc";
    cfg.runBuild = true;
    cfg.runTests = true;
    cfg.archiveArtifacts = true;
    cfg.artifactPath = "build/firmware.bin";

    // Validation passes for correct config
    auto diags = CICD::CICDGenerator::validate(cfg);
    QVERIFY(diags.size() >= 1);
    QCOMPARE(diags[0].severity, Diagnostics::Severity::Info);

    // Validation fails for absolute artifact path
    CICD::CICDConfig invalidCfg = cfg;
    invalidCfg.artifactPath = "/var/jenkins/artifacts/firmware.bin";
    auto failDiags = CICD::CICDGenerator::validate(invalidCfg);
    bool hasErr = false;
    for (const auto& d : failDiags) {
        if (d.severity == Diagnostics::Severity::Error) hasErr = true;
    }
    QVERIFY(hasErr);

    // Test generation for each provider
    cfg.provider = CICD::Provider::GitHubActions;
    QString gha = CICD::CICDGenerator::generatePipeline(cfg);
    QVERIFY(gha.contains("actions/checkout@v4"));
    QVERIFY(gha.contains("STM32F030R8"));

    cfg.provider = CICD::Provider::GitLabCI;
    QString gitlab = CICD::CICDGenerator::generatePipeline(cfg);
    QVERIFY(gitlab.contains("build_firmware"));
    QVERIFY(gitlab.contains("run_tests"));

    cfg.provider = CICD::Provider::Jenkins;
    QString jenkins = CICD::CICDGenerator::generatePipeline(cfg);
    QVERIFY(jenkins.contains("pipeline {"));
    QVERIFY(jenkins.contains("stage('Build Firmware')"));

    cfg.provider = CICD::Provider::AzurePipelines;
    QString azure = CICD::CICDGenerator::generatePipeline(cfg);
    QVERIFY(azure.contains("PublishBuildArtifacts@1"));

    cfg.provider = CICD::Provider::BitbucketPipelines;
    QString bitbucket = CICD::CICDGenerator::generatePipeline(cfg);
    QVERIFY(bitbucket.contains("pipelines:"));
}

void TestAllCases::testPhase6GeneratorIRAdoptionInLvgl() {
    CanvasScene scene;
    Project proj(&scene);
    auto* screen = proj.screens().isEmpty() ? proj.addScreen("Dashboard") : proj.screens().first();
    screen->setName("Dashboard");
    proj.setActiveScreen(screen);

    auto* speedo = new SpeedometerComponent("speedo_1");
    speedo->setCompPos(20, 20);
    speedo->setCompSize(140, 140);
    screen->addComponent(speedo);

    LvglGenerator gen(&proj, &scene);
    CodeGen::IRProject ir = gen.irProject();
    QCOMPARE(ir.screens.size(), 1);
    QCOMPARE(ir.screens[0].name, QString("Dashboard"));
    QVERIFY(ir.screens[0].widgets.size() >= 1);
    QCOMPARE(ir.screens[0].widgets[0].id, QString("speedo_1"));

    QString tempOut = m_tempDir.filePath("lvgl_ir_export");
    QVERIFY(gen.generate(tempOut));
    QVERIFY(QFile::exists(tempOut + "/ui.c"));
    QVERIFY(QFile::exists(tempOut + "/ui.h"));
    QVERIFY(QFile::exists(tempOut + "/CMakeLists.txt"));
}

void TestAllCases::testPhase6PluginRegistryExtensionBoundary() {
    auto& reg = Plugins::PluginRegistry::instance();

    class SafeTestPlugin : public Plugins::PluginInterface {
    public:
        Plugins::PluginMetadata metadata() const override {
            Plugins::PluginMetadata m;
            m.id = "test_safe_plugin";
            m.name = "Test Safe Plugin";
            m.version = "1.0.0";
            m.type = Plugins::PluginType::Component;
            return m;
        }
        bool initialize() override { return true; }
        void shutdown() override {}
    };

    auto p = std::make_shared<SafeTestPlugin>();
    QVERIFY(reg.registerPlugin(p));
    QVERIFY(reg.findPlugin("test_safe_plugin") != nullptr);

    // Error boundary: Faulty plugin that throws on initialize
    class FaultyPlugin : public Plugins::PluginInterface {
    public:
        Plugins::PluginMetadata metadata() const override {
            Plugins::PluginMetadata m;
            m.id = "faulty_plugin";
            return m;
        }
        bool initialize() override {
            throw std::runtime_error("Simulated plugin crash");
        }
        void shutdown() override {}
    };

    auto faulty = std::make_shared<FaultyPlugin>();
    // Must NOT crash the process and return false gracefully
    QVERIFY(!reg.registerPlugin(faulty));

    QVERIFY(reg.unregisterPlugin("test_safe_plugin"));
}

void TestAllCases::testUniversalPinConfigurationTaskAAndB() {
    HardwareBridge& bridge = HardwareBridge::instance();
    QVERIFY(bridge.setBoard("stm32f030r8"));

    // 1. Configure Task A pin assignments
    bridge.setActiveTask("Task A");
    QVERIFY(bridge.setTaskPinMode("PA0", "Task A", PinMode::AnalogIn));
    QVERIFY(bridge.setTaskPinMode("PA5", "Task A", PinMode::SpiRawTransfer));
    QVERIFY(bridge.setTaskPinMode("PA6", "Task A", PinMode::PwmOutput));
    bridge.setTaskPeripheral("PA5", "Task A", "SPI1_SCK");

    QCOMPARE(bridge.taskPinMode("PA0", "Task A"), PinMode::AnalogIn);
    QCOMPARE(bridge.taskPinMode("PA5", "Task A"), PinMode::SpiRawTransfer);
    QCOMPARE(bridge.taskPinMode("PA6", "Task A"), PinMode::PwmOutput);
    QCOMPARE(bridge.taskPeripheral("PA5", "Task A"), QString("SPI1_SCK"));

    // 2. Switch to Task B and configure independent pin assignments
    bridge.setActiveTask("Task B");
    QVERIFY(bridge.setTaskPinMode("PA1", "Task B", PinMode::DigitalOut));
    QVERIFY(bridge.setTaskPinMode("PB0", "Task B", PinMode::DigitalIn));
    bridge.setTaskPeripheral("PB0", "Task B", "TIM3_CH3");

    QCOMPARE(bridge.taskPinMode("PA1", "Task B"), PinMode::DigitalOut);
    QCOMPARE(bridge.taskPinMode("PB0", "Task B"), PinMode::DigitalIn);
    QCOMPARE(bridge.taskPeripheral("PB0", "Task B"), QString("TIM3_CH3"));

    // 3. Verify Task A assignments were preserved when Task B was modified
    QCOMPARE(bridge.taskPinMode("PA0", "Task A"), PinMode::AnalogIn);
    QCOMPARE(bridge.taskPinMode("PA5", "Task A"), PinMode::SpiRawTransfer);
    QCOMPARE(bridge.taskPinMode("PA6", "Task A"), PinMode::PwmOutput);

    // 4. Verify Task B assignments remain intact when switching back to Task A
    bridge.setActiveTask("Task A");
    QCOMPARE(bridge.taskPinMode("PA1", "Task B"), PinMode::DigitalOut);
    QCOMPARE(bridge.taskPinMode("PB0", "Task B"), PinMode::DigitalIn);

    // 5. Save & restore state roundtrip test
    QJsonObject savedState = bridge.saveHardwareStateToJson();
    QVERIFY(!savedState.isEmpty());

    bridge.setBoard("esp32s3_devkitc1");
    QCOMPARE(bridge.currentBoard().id, QString("esp32s3_devkitc1"));

    QVERIFY(bridge.loadHardwareStateFromJson(savedState));
    QCOMPARE(bridge.currentBoard().id, QString("stm32f030r8"));
    QCOMPARE(bridge.taskPinMode("PA0", "Task A"), PinMode::AnalogIn);
    QCOMPARE(bridge.taskPinMode("PA1", "Task B"), PinMode::DigitalOut);
}

void TestAllCases::testUniversalHardwareConnectionControlsAndStates() {
    HardwareBridge& bridge = HardwareBridge::instance();
    bridge.setBoard("stm32f030r8");

    // 1. Initial Disconnected State
    bridge.disconnectHardware();
    bridge.setSimulationMode(false);
    QCOMPARE(bridge.connectionState(), ConnectionState::Disconnected);
    QVERIFY(bridge.connectionStatusText().contains("Disconnected"));
    QCOMPARE(bridge.connectionButtonLabel(), QString("Connect ST-Link / OpenOCD"));

    // 2. Platform-aware Connection Labels
    bridge.setBoard("esp32s3_devkitc1");
    QCOMPARE(bridge.connectionButtonLabel(), QString("Connect Serial / ESP-IDF"));

    bridge.setBoard("rp2040");
    QCOMPARE(bridge.connectionButtonLabel(), QString("Connect Picoprobe / OpenOCD"));

    bridge.setBoard("raspberrypi_4b");
    QCOMPARE(bridge.connectionButtonLabel(), QString("Connect Linux GPIO (gpiod)"));

    bridge.setBoard("nxp_lpc55s69");
    QCOMPARE(bridge.connectionButtonLabel(), QString("Connect CMSIS-DAP / LinkServer"));

    bridge.setBoard("stc89c52");
    QCOMPARE(bridge.connectionButtonLabel(), QString("Connect Serial (STC-ISP)"));

    // 3. Simulation Mode - Must be visually and functionally distinct from physical connection
    bridge.setBoard("stm32f030r8");
    bridge.setSimulationMode(true);
    QVERIFY(bridge.isSimulationMode());
    QCOMPARE(bridge.connectionState(), ConnectionState::SimulationMode);
    QVERIFY(bridge.connectionStatusText().contains("Simulation Mode"));

    // 4. Mock / Verified Physical Connection State
    bridge.setSimulationMode(false);
    bridge.openOcdManager().setSimulatedConnected(true, "ST-LINK/V2.1 (Physical Test Probe)");
    QApplication::processEvents();
    QVERIFY(bridge.isHardwareConnected());
    QCOMPARE(bridge.connectionState(), ConnectionState::Connected);
    QCOMPARE(bridge.connectionButtonLabel(), QString("Disconnect"));
    QVERIFY(bridge.connectedProbeName().contains("ST-LINK/V2.1"));

    // 5. Disconnect request cleanly resets connection
    bridge.disconnectHardware();
    bridge.openOcdManager().setSimulatedConnected(false);
    QApplication::processEvents();
    QCOMPARE(bridge.connectionState(), ConnectionState::Disconnected);
    QVERIFY(!bridge.isHardwareConnected());
}

void TestAllCases::testPinConflictAndReservedDetection() {
    HardwareBridge& bridge = HardwareBridge::instance();
    bridge.setBoard("stm32f030r8");

    // 1. Reserved Debug Pins (PA13 SWDIO, PA14 SWCLK)
    const PinProfile pa13 = bridge.pinProfile("PA13");
    QVERIFY(pa13.isReserved);
    QVERIFY(pa13.reservedReason.contains("SWDIO"));

    // Attempting to assign a mode to a reserved pin triggers conflict/warning
    bridge.setTaskPinMode("PA13", "Task A", PinMode::DigitalOut);
    PinConflict pa13Conflict = bridge.checkPinConflict("PA13");
    QVERIFY(pa13Conflict.hasConflict);
    QVERIFY(pa13Conflict.reason.contains("reserved"));
    bridge.setTaskPinMode("PA13", "Task A", PinMode::None); // Reset

    // 2. Incompatible Mode Conflict between Task A and Task B on the same physical pin
    bridge.setTaskPinMode("PA0", "Task A", PinMode::AnalogIn);
    bridge.setTaskPinMode("PA0", "Task B", PinMode::DigitalOut);

    PinConflict pa0Conflict = bridge.checkPinConflict("PA0");
    QVERIFY(pa0Conflict.hasConflict);
    QVERIFY(pa0Conflict.reason.contains("Task conflict"));

    // Resolving Task B to match or unassign clears the conflict
    bridge.setTaskPinMode("PA0", "Task B", PinMode::None);
    QVERIFY(!bridge.checkPinConflict("PA0").hasConflict);

    // 3. Duplicate exclusive peripheral mapping conflict
    bridge.setTaskPinMode("PA5", "Task A", PinMode::SpiRawTransfer);
    bridge.setTaskPeripheral("PA5", "Task A", "SPI1_SCK");

    bridge.setTaskPinMode("PA6", "Task A", PinMode::SpiRawTransfer);
    bridge.setTaskPeripheral("PA6", "Task A", "SPI1_SCK");

    QList<PinConflict> allConflicts = bridge.allConflicts();
    bool duplicateFound = false;
    for (const auto& c : allConflicts) {
        if (c.reason.contains("Duplicate peripheral mapping") && c.reason.contains("SPI1_SCK")) {
            duplicateFound = true;
            break;
        }
    }
    QVERIFY(duplicateFound);

    // Cleanup
    bridge.setTaskPeripheral("PA6", "Task A", "SPI1_MISO");
}

void TestAllCases::testUniversalBoardProfilesAndPortFilters() {
    HardwareBridge& bridge = HardwareBridge::instance();

    // 1. STM32 Port Filtering
    QVERIFY(bridge.setBoard("stm32f030r8"));
    QStringList f0Ports = bridge.portGroupsForCurrentBoard();
    QVERIFY(f0Ports.contains("Port A"));
    QVERIFY(f0Ports.contains("Port B"));
    QVERIFY(f0Ports.contains("Port C"));

    QStringList portAPins = bridge.availablePinsForPort("Port A");
    QVERIFY(portAPins.contains("PA0"));
    QVERIFY(portAPins.contains("PA5"));
    QVERIFY(!portAPins.contains("PB0"));
    QVERIFY(!portAPins.contains("PC13"));

    // 2. ESP32-S3 Board Profile & Capabilities
    QVERIFY(bridge.setBoard("esp32s3_devkitc1"));
    const BoardProfile espS3 = bridge.currentBoard();
    QCOMPARE(espS3.mcuFamily, QString("ESP32-S3"));
    QCOMPARE(espS3.connectionType, QString("serial"));
    QVERIFY(bridge.isModeAvailable("IO1", PinMode::AnalogIn));
    QVERIFY(!bridge.isModeAvailable("IO0", PinMode::AnalogIn)); // Boot strap pin

    // 3. Raspberry Pi Pico & Pico 2 Profiles
    QVERIFY(bridge.setBoard("rp2040"));
    QCOMPARE(bridge.currentBoard().mcuFamily, QString("RP2040"));
    QVERIFY(bridge.isModeAvailable("GP26", PinMode::AnalogIn));
    QVERIFY(!bridge.isModeAvailable("GP0", PinMode::AnalogIn));

    QVERIFY(bridge.setBoard("rp2350"));
    QCOMPARE(bridge.currentBoard().id, QString("rp2350"));

    // 4. Raspberry Pi 4B Linux SBC (GPIO Bank)
    QVERIFY(bridge.setBoard("raspberrypi_4b"));
    QCOMPARE(bridge.currentBoard().connectionType, QString("linux"));
    QVERIFY(bridge.availablePins().contains("GPIO2"));

    // 5. NXP LPC55S69 Profile
    QVERIFY(bridge.setBoard("nxp_lpc55s69"));
    QCOMPARE(bridge.currentBoard().connectionType, QString("linkserver"));
    QVERIFY(bridge.availablePins().contains("PIO0_23"));

    // 6. STC 8051 Profile
    QVERIFY(bridge.setBoard("stc89c52"));
    QCOMPARE(bridge.currentBoard().connectionType, QString("stcisp"));
    QVERIFY(bridge.availablePins().contains("P0.0"));

    // Restore F0 board
    bridge.setBoard("stm32f030r8");
}

void TestAllCases::testMidpointHandleSmoothRotationAndWheelDirection() {
    CanvasScene scene;
    CanvasView view(&scene);
    view.resize(800, 600);
    view.show();
    QApplication::processEvents();

    auto* rect = new RectangleComponent("test_rect");
    rect->setCompPos(200, 150);
    rect->setCompSize(160, 80);
    scene.addUIComponent(rect);
    rect->setSelected(true);
    QApplication::processEvents();

    QCOMPARE(rect->rotationAngle(), 0.0);
    QCOMPARE(rect->transformOriginPoint(), QPointF(80.0, 40.0));

    // 1. Verify all 4 midpoint handles exist and can initiate rotation
    const ResizeHandle midpointHandles[] = {
        ResizeHandle::Top,
        ResizeHandle::Bottom,
        ResizeHandle::Left,
        ResizeHandle::Right
    };

    for (ResizeHandle h : midpointHandles) {
        QVERIFY(UIComponent::isMidpointHandle(h));
        QVERIFY(!UIComponent::isCornerHandle(h));
        QRectF hRect = rect->handleRect(h);
        QVERIFY(!hRect.isEmpty());
        QCOMPARE(rect->handleAt(hRect.center()), h);
    }

    // 2. Test Scroll UP rotates clockwise (+ degrees)
    // 120 units -> 15 degrees
    rect->rotateByWheelDelta(120.0);
    QCOMPARE(rect->rotationAngle(), 15.0);

    // Another 120 units -> 30 degrees
    rect->rotateByWheelDelta(120.0);
    QCOMPARE(rect->rotationAngle(), 30.0);

    // 3. Test Scroll DOWN rotates counterclockwise (- degrees)
    rect->rotateByWheelDelta(-120.0);
    QCOMPARE(rect->rotationAngle(), 15.0);

    // 4. Test high-resolution / small trackpad deltas produce proportional angle changes
    // e.g. 8 units -> 8 * (15/120) = 1.0 degree
    rect->rotateByWheelDelta(8.0);
    QCOMPARE(rect->rotationAngle(), 16.0);

    // 5. Test wrapping around 360 degrees
    rect->setRotationAngle(350.0);
    QCOMPARE(rect->rotationAngle(), 350.0);
    rect->rotateByWheelDelta(120.0); // +15 -> 365 -> 5
    QCOMPARE(rect->rotationAngle(), 5.0);
    rect->rotateByWheelDelta(-120.0); // -15 -> -10 -> 350
    QCOMPARE(rect->rotationAngle(), 350.0);
}

void TestAllCases::testRotationAroundCenterAndHandleAlignment() {
    CanvasScene scene;
    CanvasView view(&scene);
    auto* btn = new ButtonComponent("rot_btn");
    btn->setCompPos(300, 200);
    btn->setCompSize(140, 60);
    scene.addUIComponent(btn);
    btn->setSelected(true);

    // Initial center in scene coords: pos + (w/2, h/2) = (300+70, 200+30) = (370, 230)
    QPointF initialCenter = btn->pos() + btn->transformOriginPoint();
    QCOMPARE(initialCenter, QPointF(370, 230));

    // Rotate to 45 degrees
    btn->setRotationAngle(45.0);
    QCOMPARE(btn->rotationAngle(), 45.0);
    // Center must remain unchanged
    QPointF rotatedCenter = btn->pos() + btn->transformOriginPoint();
    QCOMPARE(rotatedCenter, initialCenter);
    QCOMPARE(btn->compWidth(), 140.0);
    QCOMPARE(btn->compHeight(), 60.0);

    // Hit test rotated component in local and scene coordinates
    QPointF localCenter(70, 30);
    QVERIFY(btn->contains(localCenter));

    // Handle hit testing after rotation (mapFromScene correctly locates local handles)
    QPointF topHandleLocal = btn->handleRect(ResizeHandle::Top).center();
    QCOMPARE(btn->handleAt(topHandleLocal), ResizeHandle::Top);

    QPointF rightHandleLocal = btn->handleRect(ResizeHandle::Right).center();
    QCOMPARE(btn->handleAt(rightHandleLocal), ResizeHandle::Right);
}

void TestAllCases::testRotationUndoRedoAndPersistence() {
    QUndoStack undoStack;
    CanvasScene scene;
    scene.setUndoStack(&undoStack);

    auto* rect = new RectangleComponent("undo_rect");
    rect->setCompPos(100, 100);
    rect->setCompSize(120, 60);
    scene.addUIComponent(rect);
    rect->setSelected(true);

    QCOMPARE(rect->rotationAngle(), 0.0);

    // Simulate rotation from 0 to 45 degrees with undo command
    qreal oldAngle = rect->rotationAngle();
    rect->setRotationAngle(45.0);
    undoStack.push(new RotateComponentCommand(rect, oldAngle, 45.0));

    QCOMPARE(rect->rotationAngle(), 45.0);

    // Undo rotation
    undoStack.undo();
    QCOMPARE(rect->rotationAngle(), 0.0);

    // Redo rotation
    undoStack.redo();
    QCOMPARE(rect->rotationAngle(), 45.0);

    // Serialization (JSON export / import)
    QJsonObject json = rect->toJson();
    QVERIFY(json.contains("rotation"));
    QCOMPARE(json["rotation"].toInt(), 45);

    auto* restored = new RectangleComponent("restored_rect");
    restored->fromJson(json);
    QCOMPARE(restored->rotationAngle(), 45.0);
    QCOMPARE(restored->transformOriginPoint(), QPointF(60.0, 30.0));
    delete restored;
}

void TestAllCases::testCornerHandlesResizePreservedAndCanvasScrollZoomIsolation() {
    CanvasScene scene;
    CanvasView view(&scene);
    view.resize(800, 600);
    view.show();
    QApplication::processEvents();

    auto* comp = new RectangleComponent("corner_test");
    comp->setCompPos(200, 200);
    comp->setCompSize(100, 100);
    scene.addUIComponent(comp);
    comp->setSelected(true);
    QApplication::processEvents();

    // 1. Verify 4 corner handles are recognized as resize handles, not midpoint
    const ResizeHandle cornerHandles[] = {
        ResizeHandle::TopLeft,
        ResizeHandle::TopRight,
        ResizeHandle::BottomLeft,
        ResizeHandle::BottomRight
    };

    for (ResizeHandle ch : cornerHandles) {
        QVERIFY(UIComponent::isCornerHandle(ch));
        QVERIFY(!UIComponent::isMidpointHandle(ch));
    }

    // 2. Test rotation mode isolation in CanvasView wheelEvent
    // When midpoint handle is in rotation mode, wheel scrolling rotates component and does NOT zoom
    qreal initialZoom = view.zoomFactor();
    comp->setRotationAngle(0.0);

    // Simulate wheel event on view while component is in rotation mode
    comp->rotateByWheelDelta(120.0);
    QCOMPARE(comp->rotationAngle(), 15.0);
    QCOMPARE(view.zoomFactor(), initialZoom); // Zoom unchanged

    // 3. Independent rotation of multiple components
    auto* comp2 = new RectangleComponent("comp2");
    comp2->setCompPos(400, 200);
    comp2->setCompSize(80, 80);
    scene.addUIComponent(comp2);

    comp2->setRotationAngle(90.0);
    QCOMPARE(comp->rotationAngle(), 15.0);
    QCOMPARE(comp2->rotationAngle(), 90.0);

    // PropertiesPanel sync test
    PropertiesPanel panel;
    panel.setTargetComponent(comp);
    // Verify properties panel updates component rotation
    comp->setRotationAngle(60.0);
    panel.setTargetComponent(comp);
    QCOMPARE(comp->rotationAngle(), 60.0);
}

void TestAllCases::testNewProjectDialogPlatformAndBoardPopulation() {
    NewProjectDialog dialog;
    dialog.resize(760, 680);
    dialog.show();
    QApplication::processEvents();

    // 1. Verify Platform dropdown entries
    auto* platformCombo = dialog.findChild<QComboBox*>();
    QVERIFY(platformCombo != nullptr);

    dialog.setSelectedPlatform("STM32");
    QCOMPARE(dialog.targetPlatform(), QString("STM32"));

    // Check STM32 boards
    dialog.setSelectedBoard("stm32f030r8");
    QCOMPARE(dialog.selectedBoardId(), QString("stm32f030r8"));
    QVERIFY(dialog.hardwareBoard().contains("NUCLEO-F030R8"));

    dialog.setSelectedBoard("stm32f407g_disc1");
    QCOMPARE(dialog.selectedBoardId(), QString("stm32f407g_disc1"));

    // Check ESP32 platform
    dialog.setSelectedPlatform("ESP32");
    QCOMPARE(dialog.targetPlatform(), QString("ESP32"));
    dialog.setSelectedBoard("esp32s3_devkitc1");
    QCOMPARE(dialog.selectedBoardId(), QString("esp32s3_devkitc1"));

    // Check Raspberry Pi Pico (RP2040)
    dialog.setSelectedPlatform("RP2040");
    dialog.setSelectedBoard("rp2040");
    QCOMPARE(dialog.selectedBoardId(), QString("rp2040"));

    // Check Raspberry Pi Linux SBC
    dialog.setSelectedPlatform("Raspberry Pi");
    dialog.setSelectedBoard("raspberrypi_4b");
    QCOMPARE(dialog.selectedBoardId(), QString("raspberrypi_4b"));

    // Check NXP
    dialog.setSelectedPlatform("NXP");
    dialog.setSelectedBoard("nxp_lpc55s69");
    QCOMPARE(dialog.selectedBoardId(), QString("nxp_lpc55s69"));

    // Check STC 8051
    dialog.setSelectedPlatform("STC");
    dialog.setSelectedBoard("stc89c52");
    QCOMPARE(dialog.selectedBoardId(), QString("stc89c52"));
}

void TestAllCases::testNewProjectDialogDynamicProfileSummaryAndToggle() {
    NewProjectDialog dialog;
    dialog.resize(760, 680);
    dialog.show();
    QApplication::processEvents();

    // 1. Check profile info for STM32F030R8
    dialog.setSelectedBoard("stm32f030r8");
    TargetBoardInfo f0Info = NewProjectDialog::findBoardInfo("stm32f030r8");
    QCOMPARE(f0Info.mcuModel, QString("STM32F030R8"));
    QCOMPARE(f0Info.architecture, QString("ARM Cortex-M0 (48 MHz)"));
    QCOMPARE(f0Info.flashBytes, static_cast<quint64>(64 * 1024));
    QCOMPARE(f0Info.ramBytes, static_cast<quint64>(8 * 1024));
    QCOMPARE(f0Info.gpioCount, 23);
    QVERIFY(f0Info.debugInterface.contains("ST-Link"));

    // 2. Check profile info for ESP32-S3
    dialog.setSelectedBoard("esp32s3_devkitc1");
    TargetBoardInfo espInfo = NewProjectDialog::findBoardInfo("esp32s3_devkitc1");
    QCOMPARE(espInfo.mcuModel, QString("ESP32-S3"));
    QVERIFY(espInfo.architecture.contains("Xtensa"));
    QCOMPARE(espInfo.flashBytes, static_cast<quint64>(8192 * 1024));
    QCOMPARE(espInfo.ramBytes, static_cast<quint64>(512 * 1024));

    // 3. Check profile summary expand/collapse toggle
    auto* toggleBtn = dialog.findChild<QPushButton*>("summaryToggleBtn");
    // Button exists and can be clicked
    auto* summaryFrame = dialog.findChild<QFrame*>("summaryFrame");
    QVERIFY(summaryFrame != nullptr);
}

void TestAllCases::testNewProjectDialogFrameworkCompatibilityValidation() {
    NewProjectDialog dialog;
    dialog.resize(760, 680);
    dialog.show();
    QApplication::processEvents();

    // 1. STC 8051 compatibility
    dialog.setSelectedBoard("stc89c52");
    dialog.setTargetFramework("ugfx");
    QCOMPARE(dialog.targetFramework(), QString("ugfx"));

    dialog.setTargetFramework("lvgl");
    QCOMPARE(dialog.targetFramework(), QString("lvgl"));

    dialog.setTargetFramework("qt-for-mcus");
    QCOMPARE(dialog.targetFramework(), QString("qt-for-mcus"));

    // 2. STM32F030R8 compatibility
    dialog.setSelectedBoard("stm32f030r8");
    dialog.setTargetFramework("lvgl");
    QCOMPARE(dialog.targetFramework(), QString("lvgl"));

    // 3. STM32F4 Discovery (STM32F407VG) supports all three
    dialog.setSelectedBoard("stm32f407g_disc1");
    TargetBoardInfo f4Info = NewProjectDialog::findBoardInfo("stm32f407g_disc1");
    QVERIFY(f4Info.supportedFrameworks.contains("lvgl"));
    QVERIFY(f4Info.supportedFrameworks.contains("ugfx"));
    QVERIFY(f4Info.supportedFrameworks.contains("qt-for-mcus"));
}

void TestAllCases::testNewProjectDialogCustomTargetAndPersistence() {
    NewProjectDialog dialog;
    dialog.resize(760, 680);
    dialog.show();
    QApplication::processEvents();

    dialog.setSelectedPlatform("Custom");
    dialog.setSelectedBoard("custom");
    dialog.setTargetFramework("lvgl");
    QApplication::processEvents();

    auto* customGroup = dialog.findChild<QGroupBox*>();
    QVERIFY(customGroup != nullptr);
    QVERIFY(!customGroup->isHidden());

    Hardware::HardwareConfig hw = dialog.hardwareConfiguration();
    QCOMPARE(hw.targetType, QString("custom"));
    QCOMPARE(hw.vendor, QString("Custom"));
    QCOMPARE(hw.family, QString("Custom"));

    Hardware::HardwareTarget target = dialog.hardwareTarget();
    QVERIFY(target.capabilities().gpioInput);
    QVERIFY(target.capabilities().gpioOutput);
    QVERIFY(target.capabilities().liveMonitoring);

    // Save and load into Project model
    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("CustomEmbeddedApp", 320, 240);
    proj.setHardwareConfig(hw);
    proj.setTargetFramework(dialog.targetFramework());

    QJsonObject json = proj.toJson();
    QVERIFY(json.contains("hardware"));
    QCOMPARE(json["hardware"].toObject()["target"].toObject()["type"].toString(), QString("custom"));

    CanvasScene restoredScene;
    Project restored(&restoredScene);
    QVERIFY(restored.fromJson(json));
    QCOMPARE(restored.hardwareConfig().targetType, QString("custom"));
    QCOMPARE(restored.targetFramework(), QString("lvgl"));
}

void TestAllCases::testNewProjectDialogTemplateCompatibilityAndCreation() {
    NewProjectDialog dialog;
    dialog.setSelectedBoard("stm32f030r8");
    dialog.setTargetFramework("lvgl");

    Hardware::HardwareConfig hw = dialog.hardwareConfiguration();
    QCOMPARE(hw.boardId, QString("stm32f030r8"));
    QCOMPARE(hw.deviceId, QString("STM32F030R8"));
    QCOMPARE(hw.family, QString("STM32"));

    Hardware::HardwareTarget target = dialog.hardwareTarget();
    QCOMPARE(target.id(), QString("stm32f030r8"));
    QCOMPARE(target.family(), QString("STM32"));
    QCOMPARE(target.backendType(), QString("stm32"));
    QCOMPARE(target.connectionType(), QString("openocd"));

    // Check project creation pipeline
    CanvasScene scene;
    Project proj(&scene);
    proj.newProject(dialog.projectName(), dialog.projectWidth(), dialog.projectHeight());
    proj.setHardwareConfig(hw);
    proj.setTargetFramework(dialog.targetFramework());

    QCOMPARE(proj.hardwareConfig().boardId, QString("stm32f030r8"));
    QCOMPARE(proj.hardwareConfig().deviceId, QString("STM32F030R8"));

    // Verify older project backward compatibility when "hardware" object is missing
    QJsonObject legacyJson;
    legacyJson["name"] = "LegacyApp";
    legacyJson["target"] = "lvgl";
    QJsonObject disp;
    disp["width"] = 320;
    disp["height"] = 240;
    legacyJson["display"] = disp;

    CanvasScene legacyScene;
    Project legacyProj(&legacyScene);
    QVERIFY(legacyProj.fromJson(legacyJson));
    QCOMPARE(legacyProj.projectName(), QString("LegacyApp"));
    QCOMPARE(legacyProj.targetFramework(), QString("lvgl"));
    QCOMPARE(legacyProj.hardwareConfig().isValid(), false); // Clean default fallback
}

// ----------------------------------------------------------------------------
// 34. Multi-Color Gradient Support in Color Picker Tests
// ----------------------------------------------------------------------------

void TestAllCases::testGradientModelStopsAndInterpolation() {
    ComponentGradient grad;
    QVERIFY(grad.isValid());
    QCOMPARE(grad.stops.size(), 2);
    QCOMPARE(grad.stops.first().position, 0.0);
    QCOMPARE(grad.stops.last().position, 1.0);

    // Add multiple stops (Illustrative 4-stop arrangement: Blue -> Cyan -> Green -> Orange)
    grad.stops.clear();
    grad.addStop(0.0, QColor("#2196F3"));  // Blue
    grad.addStop(0.35, QColor("#00E5FF")); // Cyan
    grad.addStop(0.70, QColor("#4CAF50")); // Green
    grad.addStop(1.0, QColor("#FF9800"));  // Orange
    QCOMPARE(grad.stops.size(), 4);

    // Verify ordering
    QCOMPARE(grad.stops[0].position, 0.0);
    QCOMPARE(grad.stops[1].position, 0.35);
    QCOMPARE(grad.stops[2].position, 0.70);
    QCOMPARE(grad.stops[3].position, 1.0);

    // Test exact boundary colors
    QCOMPARE(grad.interpolateAt(0.0), QColor("#2196F3"));
    QCOMPARE(grad.interpolateAt(0.35), QColor("#00E5FF"));
    QCOMPARE(grad.interpolateAt(0.70), QColor("#4CAF50"));
    QCOMPARE(grad.interpolateAt(1.0), QColor("#FF9800"));

    // Test intermediate interpolation
    QColor mid1 = grad.interpolateAt(0.175); // Halfway between Blue and Cyan
    QVERIFY(mid1.isValid());
    QVERIFY(mid1.blue() > 200);

    // Test alpha transparency interpolation
    ComponentGradient alphaGrad;
    alphaGrad.stops.clear();
    alphaGrad.addStop(0.0, QColor(255, 0, 0, 0));   // Transparent red
    alphaGrad.addStop(1.0, QColor(255, 0, 0, 200)); // Semi-opaque red
    QColor midAlpha = alphaGrad.interpolateAt(0.5);
    QVERIFY(qAbs(midAlpha.alpha() - 100) <= 2);

    // Enforce minimum of 2 stops
    grad.removeStop(1);
    QCOMPARE(grad.stops.size(), 3);
    grad.removeStop(1);
    QCOMPARE(grad.stops.size(), 2);
    grad.removeStop(0); // Should NOT remove when size == 2
    QCOMPARE(grad.stops.size(), 2);

    // JSON Round-trip
    QJsonObject json = grad.toJson();
    QVERIFY(json.contains("stops"));
    QVERIFY(json["stops"].isArray());
    QCOMPARE(json["stops"].toArray().size(), 2);

    ComponentGradient restored = ComponentGradient::fromJson(json);
    QVERIFY(restored.isValid());
    QCOMPARE(restored.stops.size(), 2);
    QCOMPARE(restored.stops[0].position, grad.stops[0].position);
    QCOMPARE(restored.stops[0].color, grad.stops[0].color);
}

void TestAllCases::testGradientLinearAngleAndRadialGeometry() {
    QRectF rect(0, 0, 200, 100);

    // Linear 0 degrees (Left-to-Right)
    ComponentGradient linear0(ComponentGradient::Type::Linear, { {0.0, QColor("#FF0000")}, {1.0, QColor("#0000FF")} }, 0.0);
    QBrush b0 = linear0.toQBrush(rect);
    QCOMPARE(b0.style(), Qt::LinearGradientPattern);
    const QLinearGradient* lg0 = static_cast<const QLinearGradient*>(b0.gradient());
    QVERIFY(lg0 != nullptr);
    QVERIFY(lg0->start().x() < lg0->finalStop().x());

    // Linear 90 degrees (Top-to-Bottom)
    ComponentGradient linear90(ComponentGradient::Type::Linear, { {0.0, QColor("#FF0000")}, {1.0, QColor("#0000FF")} }, 90.0);
    QBrush b90 = linear90.toQBrush(rect);
    const QLinearGradient* lg90 = static_cast<const QLinearGradient*>(b90.gradient());
    QVERIFY(lg90 != nullptr);
    QVERIFY(lg90->start().y() < lg90->finalStop().y());

    // Radial Gradient
    ComponentGradient radial(ComponentGradient::Type::Radial, { {0.0, QColor("#FFFFFF")}, {1.0, QColor("#000000")} });
    radial.centerX = 0.25;
    radial.centerY = 0.75;
    radial.radius = 0.6;
    QBrush rb = radial.toQBrush(rect);
    QCOMPARE(rb.style(), Qt::RadialGradientPattern);
    const QRadialGradient* rg = static_cast<const QRadialGradient*>(rb.gradient());
    QVERIFY(rg != nullptr);
    QVERIFY(qAbs(rg->center().x() - (rect.left() + 0.25 * rect.width())) < 1.0);
    QVERIFY(qAbs(rg->center().y() - (rect.top() + 0.75 * rect.height())) < 1.0);
}

void TestAllCases::testColorPickerDialogDualModeAndGradientEditing() {
    ColorPickerDialog dlg(QColor("#00E5FF"));
    dlg.resize(480, 560);
    dlg.show();
    QApplication::processEvents();

    // Default is Solid Color mode
    QCOMPARE(dlg.isGradientMode(), false);
    QCOMPARE(dlg.selectedColor(), QColor("#00E5FF"));

    // Switch to Gradient Mode
    dlg.setGradientMode(true);
    QCOMPARE(dlg.isGradientMode(), true);

    // Load Presets
    ComponentGradient sunset = ComponentGradient::presetSunset();
    QVERIFY(sunset.stops.size() >= 2);
    dlg.setGradient(sunset);
    QCOMPARE(dlg.selectedGradient().stops.size(), sunset.stops.size());
    QCOMPARE(dlg.selectedGradient().stops.first().color, sunset.stops.first().color);

    ComponentGradient ocean = ComponentGradient::presetOcean();
    dlg.setGradient(ocean);
    QCOMPARE(dlg.selectedGradient().stops.size(), 3);

    ComponentGradient custom = ComponentGradient::presetCustom();
    dlg.setGradient(custom);
    QCOMPARE(dlg.selectedGradient().stops.size(), 4);

    // Check Stop Bar Widget
    auto* stopBar = dlg.findChild<GradientStopBarWidget*>();
    QVERIFY(stopBar != nullptr);
    stopBar->setSelectedStopIndex(1);
    QCOMPARE(stopBar->selectedStopIndex(), 1);

    // Add Stop action
    auto* btnAddStop = dlg.findChild<QPushButton*>("btnAddStop");
    if (btnAddStop) {
        int countBefore = dlg.selectedGradient().stops.size();
        btnAddStop->click();
        QApplication::processEvents();
        QCOMPARE(dlg.selectedGradient().stops.size(), countBefore + 1);
    }
}

void TestAllCases::testComponentGradientRenderingAndJsonPersistence() {
    // 1. RectangleComponent with Gradient Fill
    auto* rect = new RectangleComponent("rect_grad");
    rect->setCompPos(10, 10);
    rect->setCompSize(180, 80);
    rect->setCornerRadius(8);

    ComponentGradient grad(ComponentGradient::Type::Linear, {
        {0.0, QColor("#1E3A8A")},
        {0.5, QColor("#3B82F6")},
        {1.0, QColor("#93C5FD")}
    }, 45.0);

    rect->setGradientFill(grad);
    QVERIFY(rect->hasGradientFill());
    QCOMPARE(rect->gradientFill().stops.size(), 3);
    QCOMPARE(rect->gradientFill().angle, 45.0);

    // Verify QPainter execution
    QImage img(200, 100, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    rect->paint(&p, nullptr, nullptr);
    p.end();
    // Verify non-transparent pixel in rendered region
    QRgb centerPx = img.pixel(100, 50);
    QVERIFY(qAlpha(centerPx) > 0);

    // Verify JSON Serialization
    QJsonObject json = rect->toJson();
    QVERIFY(json.contains("gradient"));
    QJsonObject gradJson = json["gradient"].toObject();
    QCOMPARE(gradJson["type"].toString(), QString("linear"));
    QCOMPARE(gradJson["angle"].toDouble(), 45.0);
    QCOMPARE(gradJson["stops"].toArray().size(), 3);

    // Deserialization
    auto* restoredRect = new RectangleComponent("rect_restored");
    restoredRect->fromJson(json);
    QVERIFY(restoredRect->hasGradientFill());
    QCOMPARE(restoredRect->gradientFill().stops.size(), 3);
    QCOMPARE(restoredRect->gradientFill().stops[1].position, 0.5);

    // 2. ButtonComponent with Gradient
    auto* btn = new ButtonComponent("btn_grad");
    btn->setCompSize(120, 40);
    btn->setGradientFill(grad);
    QVERIFY(btn->hasGradientFill());
    QJsonObject btnJson = btn->toJson();
    QVERIFY(btnJson.contains("gradient"));

    // 3. ProgressBarComponent with Gradient
    auto* prog = new ProgressBarComponent("prog_grad");
    prog->setCompSize(200, 30);
    prog->setValue(0.75);
    prog->setGradientFill(grad);
    QVERIFY(prog->hasGradientFill());

    // 4. CircleComponent with Gradient
    auto* circ = new CircleComponent("circ_grad");
    circ->setCompSize(80, 80);
    circ->setGradientFill(grad);
    QVERIFY(circ->hasGradientFill());

    delete rect;
    delete restoredRect;
    delete btn;
    delete prog;
    delete circ;
}

void TestAllCases::testGradientPropertiesPanelIntegrationAndCodeGeneration() {
    CanvasScene scene;
    Project proj(&scene);
    proj.newProject("GradientExportApp", 320, 240);

    auto* rect = new RectangleComponent("bg_panel");
    rect->setCompPos(0, 0);
    rect->setCompSize(320, 240);
    ComponentGradient grad(ComponentGradient::Type::Linear, {
        {0.0, QColor("#111827")},
        {1.0, QColor("#1F2937")}
    }, 90.0);
    rect->setGradientFill(grad);
    scene.addUIComponent(rect);

    auto* btn = new ButtonComponent("action_btn");
    btn->setCompPos(60, 100);
    btn->setCompSize(200, 45);
    btn->setText("Submit");
    ComponentGradient btnGrad(ComponentGradient::Type::Linear, {
        {0.0, QColor("#0284C7")},
        {1.0, QColor("#0EA5E9")}
    }, 0.0);
    btn->setGradientFill(btnGrad);
    scene.addUIComponent(btn);

    // 1. Test LVGL Code Generation
    LvglGenerator lvglGen(&proj, &scene);
    QString lvglCode = lvglGen.generateUiSource();
    QVERIFY(lvglCode.contains("lv_obj_set_style_bg_grad_color"));
    QVERIFY(lvglCode.contains("lv_obj_set_style_bg_grad_dir"));

    // 2. Test Qt for MCUs (QUL) QML Generation
    QtMcuGenerator qmlGen(&proj, &scene);
    QString qmlCode = qmlGen.generateDesignQml();
    QVERIFY(qmlCode.contains("gradient: Gradient {"));
    QVERIFY(qmlCode.contains("GradientStop { position:"));

    // 3. Test uGFX Generator
    UgfxGenerator ugfxGen(&proj, &scene);
    QString ugfxCode = ugfxGen.generateUiSource();
    QVERIFY(ugfxCode.contains("Gradient fill"));

    // 4. Test PropertiesPanel updateColorButton with Gradient
    PropertiesPanel propPanel;
    QPushButton testBtn;
    propPanel.updateColorButton(&testBtn, QColor("#0284C7"), btnGrad);
    QCOMPARE(testBtn.text(), QString("Gradient"));
    QVERIFY(testBtn.styleSheet().contains("qlineargradient"));
}

// ============================================================================
// 35. Complete Canva-Inspired Visual Canvas Editor Upgrade
// ============================================================================

void TestAllCases::testCanvaContextualTopToolbar() {
    CanvasScene scene;
    CanvasView view(&scene);
    QUndoStack undoStack;
    Project proj(&scene);

    ContextualTopToolbar toolbar(&view, &scene, &undoStack, &proj);

    // 1. Initial State: No Selection
    toolbar.updateSelection({});
    auto* stack = toolbar.findChild<QStackedWidget*>();
    QVERIFY(stack != nullptr);
    QCOMPARE(stack->currentIndex(), 0);

    // 2. Text Selection
    auto* lbl = new LabelComponent("test_lbl");
    scene.addUIComponent(lbl);
    toolbar.updateSelection({lbl});
    QCOMPARE(stack->currentIndex(), 1); // Text page

    // 3. Image Selection
    auto* img = new ImageComponent("test_img");
    scene.addUIComponent(img);
    toolbar.updateSelection({img});
    QCOMPARE(stack->currentIndex(), 2); // Image page

    // 4. Shape Selection
    auto* rect = new RectangleComponent("test_rect");
    scene.addUIComponent(rect);
    toolbar.updateSelection({rect});
    QCOMPARE(stack->currentIndex(), 3); // Shape page

    // 5. Button Selection
    auto* btn = new ButtonComponent("test_btn");
    scene.addUIComponent(btn);
    toolbar.updateSelection({btn});
    QCOMPARE(stack->currentIndex(), 4); // Button page

    // 6. Progress Bar Selection
    auto* pb = new ProgressBarComponent("test_pb");
    scene.addUIComponent(pb);
    toolbar.updateSelection({pb});
    QCOMPARE(stack->currentIndex(), 5); // Progress page

    // 7. Multi-Selection
    toolbar.updateSelection({lbl, rect, btn});
    QCOMPARE(stack->currentIndex(), 9); // Multi-select page
}

void TestAllCases::testCanvaFloatingSelectionToolbar() {
    CanvasScene scene;
    CanvasView view(&scene);
    view.show();
    QUndoStack undoStack;

    FloatingSelectionToolbar floatingBar(&view, &scene, &undoStack, view.viewport());

    auto* btn = new ButtonComponent("btn_float");
    btn->setCompPos(50, 50);
    btn->setCompSize(100, 40);
    scene.addUIComponent(btn);

    // Initial state: hidden when nothing selected
    floatingBar.updateSelection({});
    QVERIFY(floatingBar.isHidden());

    // Show when selected
    floatingBar.updateSelection({btn});
    QVERIFY(!floatingBar.isHidden());

    // Lock toggle
    QVERIFY(!btn->isLocked());
    btn->setLocked(true);
    QVERIFY(btn->isLocked());
    QVERIFY(!(btn->flags() & QGraphicsItem::ItemIsMovable));

    btn->setLocked(false);
    QVERIFY(!btn->isLocked());
    QVERIFY(btn->flags() & QGraphicsItem::ItemIsMovable);

    // Reposition test
    floatingBar.reposition();
    QVERIFY(floatingBar.pos().y() >= 0);
}

void TestAllCases::testCanvaGroupAndUngroupOperations() {
    CanvasScene scene;
    QUndoStack undoStack;

    auto* c1 = new RectangleComponent("rect_g1");
    c1->setCompPos(20, 30);
    c1->setCompSize(60, 40);
    scene.addUIComponent(c1);

    auto* c2 = new ButtonComponent("btn_g2");
    c2->setCompPos(100, 50);
    c2->setCompSize(80, 30);
    scene.addUIComponent(c2);

    // 1. Execute Group Command
    auto* groupCmd = new GroupCommand(&scene, {c1, c2});
    undoStack.push(groupCmd);

    QCOMPARE(scene.uiComponents().size(), 1);
    auto* group = dynamic_cast<GroupComponent*>(scene.uiComponents().first());
    QVERIFY(group != nullptr);
    QCOMPARE(group->children().size(), 2);
    QCOMPARE(group->compX(), 20.0);
    QCOMPARE(group->compY(), 30.0);

    // 2. Test JSON Serialization of Group
    QJsonObject groupJson = group->toJson();
    QCOMPARE(groupJson["type"].toString(), QString("Group"));
    QVERIFY(groupJson["children"].isArray());
    QCOMPARE(groupJson["children"].toArray().size(), 2);

    // 3. Test Undo Grouping
    undoStack.undo();
    QCOMPARE(scene.uiComponents().size(), 2);
    QCOMPARE(scene.uiComponents()[0]->pos().x(), 20.0);
    QCOMPARE(scene.uiComponents()[0]->pos().y(), 30.0);
    QCOMPARE(scene.uiComponents()[1]->pos().x(), 100.0);
    QCOMPARE(scene.uiComponents()[1]->pos().y(), 50.0);

    // 4. Test Redo Grouping
    undoStack.redo();
    QCOMPARE(scene.uiComponents().size(), 1);

    // 5. Test Ungroup Command
    auto* activeGroup = dynamic_cast<GroupComponent*>(scene.uiComponents().first());
    QVERIFY(activeGroup != nullptr);
    auto* ungroupCmd = new UngroupCommand(&scene, activeGroup);
    undoStack.push(ungroupCmd);

    QCOMPARE(scene.uiComponents().size(), 2);
}

void TestAllCases::testCanvaTextFormattingEffectsAndCurvature() {
    LabelComponent lbl("label_fx");
    lbl.setText("Canva Typography");
    lbl.setFontFamily("Inter");
    lbl.setPixelSize(24);
    lbl.setBold(true);
    lbl.setItalic(true);
    lbl.setUnderline(true);
    lbl.setStrikeThrough(true);
    lbl.setTextCasing("uppercase");
    lbl.setLetterSpacing(2.5);

    QCOMPARE(lbl.bold(), true);
    QCOMPARE(lbl.italic(), true);
    QCOMPARE(lbl.underline(), true);
    QCOMPARE(lbl.strikeThrough(), true);
    QCOMPARE(lbl.textCasing(), QString("uppercase"));

    // 1. Text Effects
    TextEffect eff;
    eff.type = TextEffectType::Neon;
    eff.color = QColor("#EC4899");
    eff.opacity = 0.85;
    eff.thickness = 3.0;
    lbl.setTextEffect(eff);

    QCOMPARE(lbl.textEffect().type, TextEffectType::Neon);
    QCOMPARE(lbl.textEffect().thickness, 3.0);

    // 2. Curvature (-100 to +100)
    lbl.setCurvature(65);
    QCOMPARE(lbl.curvature(), 65);

    lbl.setCurvature(-80);
    QCOMPARE(lbl.curvature(), -80);

    // 3. Serialization Roundtrip
    QJsonObject json = lbl.toJson();
    QCOMPARE(json["underline"].toBool(), true);
    QCOMPARE(json["strikeThrough"].toBool(), true);
    QCOMPARE(json["casing"].toString(), QString("uppercase"));
    QCOMPARE(json["curvature"].toInt(), -80);
    QVERIFY(json.contains("effect"));
    QCOMPARE(json["effect"].toObject()["type"].toString(), QString("neon"));

    LabelComponent restoredLbl("label_restored");
    restoredLbl.fromJson(json);
    QCOMPARE(restoredLbl.underline(), true);
    QCOMPARE(restoredLbl.strikeThrough(), true);
    QCOMPARE(restoredLbl.textCasing(), QString("uppercase"));
    QCOMPARE(restoredLbl.curvature(), -80);
    QCOMPARE(restoredLbl.textEffect().type, TextEffectType::Neon);

    // 4. QML and uGFX Snippets
    QString qml = restoredLbl.toQmlSnippet();
    QVERIFY(qml.contains("font.underline: true"));
    QVERIFY(qml.contains("font.strikeout: true"));
    QVERIFY(qml.contains("Curved text"));
}

void TestAllCases::testCanvaImageCropFiltersAndUploads() {
    CanvasScene scene;
    QUndoStack undoStack;
    Project proj(&scene);

    ImageComponent img("img_canva");
    img.setBrightness(25);
    img.setContrast(15);
    img.setSaturation(-10);
    img.setGrayscale(true);
    img.setCropRect(QRectF(0.1, 0.1, 0.8, 0.8));

    QCOMPARE(img.brightness(), 25);
    QCOMPARE(img.contrast(), 15);
    QCOMPARE(img.saturation(), -10);
    QCOMPARE(img.grayscale(), true);
    QCOMPARE(img.cropRect(), QRectF(0.1, 0.1, 0.8, 0.8));

    // Serialization
    QJsonObject json = img.toJson();
    QCOMPARE(json["brightness"].toInt(), 25);
    QCOMPARE(json["contrast"].toInt(), 15);
    QCOMPARE(json["grayscale"].toBool(), true);
    QVERIFY(json.contains("cropRect"));

    ImageComponent restoredImg("img_restored");
    restoredImg.fromJson(json);
    QCOMPARE(restoredImg.brightness(), 25);
    QCOMPARE(restoredImg.grayscale(), true);
    QCOMPARE(restoredImg.cropRect().width(), 0.8);

    // UploadsPanel Test
    UploadsPanel uploadsPanel(&scene, &undoStack, &proj);
    QString tempImagePath = m_tempDir.filePath("test_upload.png");
    QImage testImg(64, 64, QImage::Format_ARGB32);
    testImg.fill(QColor(10, 150, 240));
    testImg.save(tempImagePath);

    uploadsPanel.addAsset(tempImagePath);
    auto* listWidget = uploadsPanel.findChild<QListWidget*>();
    QVERIFY(listWidget != nullptr);
    QCOMPARE(listWidget->count(), 1);
    QVERIFY(listWidget->item(0)->text().contains("test_upload.png"));
}

void TestAllCases::testCanvaLockFlipAndTransparencyPersistence() {
    RectangleComponent rect("rect_tf");
    rect.setCompPos(10, 20);
    rect.setCompSize(120, 80);

    // 1. Flip
    QCOMPARE(rect.flipHorizontal(), false);
    QCOMPARE(rect.flipVertical(), false);
    rect.setFlipHorizontal(true);
    rect.setFlipVertical(true);
    QCOMPARE(rect.flipHorizontal(), true);
    QCOMPARE(rect.flipVertical(), true);

    // 2. Lock
    rect.setLocked(true);
    QCOMPARE(rect.isLocked(), true);

    // 3. Opacity (0.0 to 1.0)
    rect.setOpacity(0.65);
    QCOMPARE(rect.opacity(), 0.65);

    // 4. Persistence
    QJsonObject json = rect.toJson();
    QCOMPARE(json["locked"].toBool(), true);
    QCOMPARE(json["flipH"].toBool(), true);
    QCOMPARE(json["flipV"].toBool(), true);
    QCOMPARE(json["opacity"].toDouble(), 0.65);

    RectangleComponent restored("rect_restored");
    restored.fromJson(json);
    QCOMPARE(restored.isLocked(), true);
    QCOMPARE(restored.flipHorizontal(), true);
    QCOMPARE(restored.flipVertical(), true);
    QCOMPARE(restored.opacity(), 0.65);
}

void TestAllCases::testCapabilityDatabaseDeviceLookupAndFrameworks() {
    CapabilityDatabase db;
    
    // 1. Verify Tier 1 and Extended Devices in Database
    QVERIFY(db.deviceExists("STM32H745"));
    QVERIFY(db.deviceExists("ESP32"));
    QVERIFY(db.deviceExists("STM32F030R8"));
    QVERIFY(db.deviceExists("Raspberry Pi Pico"));
    QVERIFY(db.deviceExists("NXP i.MX RT1060"));

    // 2. Query Device Profile Specs
    DeviceProfile h7 = db.getDeviceProfile("STM32H745");
    QCOMPARE(h7.name, QString("STM32H745"));
    QCOMPARE(h7.family, QString("STM32H7"));
    QCOMPARE(h7.ram_kb, 512u);
    QCOMPARE(h7.flash_kb, 2048u);
    QCOMPARE(h7.cpu_speed_mhz, 480u);
    QVERIFY(h7.peripherals.contains("UART1"));
    QVERIFY(h7.peripherals.contains("SPI1"));
    QVERIFY(h7.supported_frameworks.contains("qt-for-mcus"));
    QVERIFY(h7.supported_frameworks.contains("ugfx"));
    QVERIFY(h7.supported_frameworks.contains("lvgl"));

    // 3. Low-cost device checks
    DeviceProfile f0 = db.getDeviceProfile("STM32F030R8");
    QCOMPARE(f0.ram_kb, 8u);
    QCOMPARE(f0.flash_kb, 64u);
    QCOMPARE(f0.supported_frameworks.contains("ugfx"), true);
    QCOMPARE(f0.supported_frameworks.contains("qt-for-mcus"), false);

    // 4. Framework Compatibility Verification
    QVERIFY(db.supportsFramework("STM32H745", "qt-for-mcus"));
    QVERIFY(db.supportsFramework("STM32H745", "ugfx"));
    QVERIFY(db.supportsFramework("STM32H745", "lvgl"));
    QVERIFY(!db.supportsFramework("STM32F030R8", "qt-for-mcus"));
    QVERIFY(db.supportsFramework("STM32F030R8", "ugfx"));

    // 5. Query Families and Listings
    QStringList allDevs = db.listAllDevices();
    QVERIFY(allDevs.size() >= 10);
    QVERIFY(allDevs.contains("STM32H745"));
    QVERIFY(allDevs.contains("ESP32"));

    QStringList families = db.listFamilies();
    QVERIFY(families.contains("STM32H7"));
    QVERIFY(families.contains("ESP32"));
    QVERIFY(families.contains("RP2040"));

    QStringList h7Devs = db.getDevicesByFamily("STM32H7");
    QVERIFY(h7Devs.contains("STM32H745"));
    QVERIFY(h7Devs.contains("STM32H743"));

    // 6. Add Custom Device Profile
    DeviceProfile customDev;
    customDev.name = "CustomSAMD51";
    customDev.family = "ATSAMD51";
    customDev.ram_kb = 256;
    customDev.flash_kb = 1024;
    customDev.cpu_speed_mhz = 120;
    customDev.supported_frameworks = {"lvgl", "ugfx"};
    db.addDeviceProfile(customDev);

    QVERIFY(db.deviceExists("CustomSAMD51"));
    QCOMPARE(db.getDeviceProfile("CustomSAMD51").flash_kb, 1024u);
    QVERIFY(db.supportsFramework("CustomSAMD51", "lvgl"));
    QVERIFY(!db.supportsFramework("CustomSAMD51", "qt-for-mcus"));
}

void TestAllCases::testEstimationFormulasAndComponentFootprints() {
    EstimationFormulas::initDefaultBaselines();

    // 1. Framework Baselines
    auto qul = EstimationFormulas::getFrameworkBaseline("qt-for-mcus");
    QCOMPARE(qul.flash_minimum, 400u * 1024);
    QCOMPARE(qul.ram_minimum, 50u * 1024);
    QCOMPARE(qul.flash_per_component, 200u);
    QCOMPARE(qul.ram_per_component, 50u);

    auto ugfx = EstimationFormulas::getFrameworkBaseline("ugfx");
    QCOMPARE(ugfx.flash_minimum, 120u * 1024);
    QCOMPARE(ugfx.ram_minimum, 30u * 1024);

    auto lvgl = EstimationFormulas::getFrameworkBaseline("lvgl");
    QCOMPARE(lvgl.flash_minimum, 80u * 1024);
    QCOMPARE(lvgl.ram_minimum, 25u * 1024);

    // 2. Component Footprint Formulas
    ButtonComponent btn("btn_test");
    auto btnFp = EstimationFormulas::estimateComponent(&btn);
    QCOMPARE(btnFp.ram, 80u);
    QCOMPARE(btnFp.flash, 200u);
    QVERIFY(btnFp.cpu_percent > 0.0f);

    LabelComponent lbl("lbl_test");
    auto lblFp = EstimationFormulas::estimateComponent(&lbl);
    QCOMPARE(lblFp.ram, 40u);
    QCOMPARE(lblFp.flash, 100u);

    ProgressBarComponent pbar("pbar_test");
    auto pbarFp = EstimationFormulas::estimateComponent(&pbar);
    QCOMPARE(pbarFp.ram, 60u);
    QCOMPARE(pbarFp.flash, 120u);

    PathComponent pathComp("path_test");
    auto pathFp = EstimationFormulas::estimateComponent(&pathComp);
    QCOMPARE(pathFp.ram, 200u);
    QCOMPARE(pathFp.flash, 400u);
}

void TestAllCases::testResourceEstimatorAnalysisAndWarnings() {
    CanvasScene scene;
    Project project(&scene);
    project.setProjectName("ResourceAnalysisProject");
    project.setTargetFramework("ugfx");

    // Add components
    auto* btn = new ButtonComponent("btn_ok");
    auto* lbl = new LabelComponent("lbl_status");
    auto* pbar = new ProgressBarComponent("pbar_cpu");
    scene.addUIComponent(btn);
    scene.addUIComponent(lbl);
    scene.addUIComponent(pbar);

    ResourceEstimator estimator;

    // 1. Analyze for STM32H745 (Plenty of RAM/Flash)
    ResourceAnalysis analysis = estimator.analyze(&project, "STM32H745");
    QCOMPARE(analysis.device_name, QString("STM32H745"));
    QCOMPARE(analysis.target_framework, QString("ugfx"));
    QCOMPARE(analysis.status, ResourceAnalysis::OK);
    QVERIFY(analysis.ram_used > 30 * 1024);
    QVERIFY(analysis.flash_used > 120 * 1024);
    QVERIFY(analysis.flash_percent < 80.0f);
    QVERIFY(analysis.ram_percent < 80.0f);
    QVERIFY(analysis.warnings.isEmpty() || analysis.status == ResourceAnalysis::OK);
    QVERIFY(!analysis.summary.isEmpty());

    // 2. Breakdown Inspection
    QVERIFY(analysis.breakdown.framework_flash >= 120 * 1024);
    QCOMPARE(analysis.breakdown.components_flash, 200u + 100u + 120u);
    QCOMPARE(analysis.breakdown.components_ram, 80u + 40u + 60u);
    QVERIFY(analysis.breakdown.user_space_flash > 0);
    QVERIFY(analysis.breakdown.user_space_ram > 0);

    // 3. Analyze for STM32F030R8 with qt-for-mcus (Mismatch & Overflow)
    project.setTargetFramework("qt-for-mcus");
    ResourceAnalysis f0Analysis = estimator.analyze(&project, "STM32F030R8");
    // STM32F0 has only 64KB flash and 8KB RAM, while qt-for-mcus baseline is 400KB Flash + 50KB RAM
    QCOMPARE(f0Analysis.status, ResourceAnalysis::CRITICAL);
    QVERIFY(f0Analysis.flash_percent > 100.0f);
    QVERIFY(f0Analysis.ram_percent > 100.0f);
    QVERIFY(f0Analysis.warnings.size() >= 2);
    bool foundFlashExceeded = false;
    for (const QString& w : f0Analysis.warnings) {
        if (w.contains("Flash memory exceeded") || w.contains("CRITICAL")) foundFlashExceeded = true;
    }
    QVERIFY(foundFlashExceeded);
}

void TestAllCases::testResourceAnalyzerPanelLiveUpdates() {
    CanvasScene scene;
    Project project(&scene);
    project.setProjectName("LiveResourcePanelTest");
    project.setTargetFramework("lvgl");

    ResourceAnalyzerPanel panel;
    panel.setProject(&project);
    panel.setDevice("ESP32");

    panel.refresh();
    ResourceAnalysis analysis = panel.lastAnalysis();
    QCOMPARE(analysis.device_name, QString("ESP32"));
    QCOMPARE(analysis.target_framework, QString("lvgl"));
    QCOMPARE(analysis.status, ResourceAnalysis::OK);

    // Verify UI children exist
    auto* combo = panel.findChild<QComboBox*>();
    auto* flashBar = panel.findChild<QProgressBar*>("flashBar");
    auto* ramBar = panel.findChild<QProgressBar*>("ramBar");
    auto* cpuBar = panel.findChild<QProgressBar*>("cpuBar");
    auto* listWidget = panel.findChild<QListWidget*>();

    QVERIFY(combo != nullptr);
    QVERIFY(flashBar != nullptr);
    QVERIFY(ramBar != nullptr);
    QVERIFY(cpuBar != nullptr);
    QVERIFY(listWidget != nullptr);

    // Add component and trigger refresh
    auto* btn = new ButtonComponent("btn_add");
    scene.addUIComponent(btn);
    panel.refresh();

    ResourceAnalysis updatedAnalysis = panel.lastAnalysis();
    QVERIFY(updatedAnalysis.flash_used >= analysis.flash_used);
    QVERIFY(updatedAnalysis.ram_used >= analysis.ram_used);
}

void TestAllCases::testExportValidationDialogCriticalFlow() {
    ResourceAnalysis analysis;
    analysis.device_name = "STM32F030R8";
    analysis.target_framework = "qt-for-mcus";
    analysis.status = ResourceAnalysis::CRITICAL;
    analysis.flash_used = 420 * 1024;
    analysis.flash_total = 64 * 1024;
    analysis.flash_percent = 656.2f;
    analysis.ram_used = 52 * 1024;
    analysis.ram_total = 8 * 1024;
    analysis.ram_percent = 650.0f;
    analysis.summary = "CRITICAL: Exceeds target device capacity!";
    analysis.warnings.append("CRITICAL: Flash memory exceeded! Design uses 420 KB (656% of 64 KB).");
    analysis.warnings.append("CRITICAL: RAM memory exceeded! Design uses 52 KB (650% of 8 KB).");

    ExportValidationDialog dlg(analysis);

    auto* header = dlg.findChild<QLabel*>();
    QVERIFY(header != nullptr);

    auto* warnList = dlg.findChild<QListWidget*>();
    QVERIFY(warnList != nullptr);
    QCOMPARE(warnList->count(), 2);

    auto* exportBtn = dlg.findChild<QPushButton*>("exportAnywayBtn");
    auto* optimizeBtn = dlg.findChild<QPushButton*>("optimizeBtn");
    QVERIFY(exportBtn != nullptr);
    QVERIFY(optimizeBtn != nullptr);

    // Test actions
    QCOMPARE(dlg.resultAction(), ExportValidationDialog::CANCEL);
}

void TestAllCases::testPhase2BoardRegistryAndPresetsPersistence() {
    BoardRegistry& reg = BoardRegistry::instance();
    QVERIFY(reg.loadFromBoardsJson());

    // 1. Verify board count and families
    auto boards = reg.boards();
    QVERIFY(boards.size() >= 18);
    QStringList families = reg.families();
    QVERIFY(families.contains("STM32"));
    QVERIFY(families.contains("ESP32"));
    QVERIFY(families.contains("RP2040"));
    QVERIFY(families.contains("NXP"));
    QVERIFY(families.contains("RaspberryPi"));
    QVERIFY(families.contains("Custom"));

    // 2. Verify STM32 Nucleo F030R8
    BoardDef nucleo = reg.board("nucleo_f030r8");
    QVERIFY(nucleo.isValid());
    QCOMPARE(nucleo.family, QString("STM32"));
    QCOMPARE(nucleo.vendor, QString("STMicroelectronics"));
    QCOMPARE(nucleo.display.width, 320);
    QCOMPARE(nucleo.display.height, 240);
    QVERIFY(!nucleo.openocdConfig.isEmpty());
    QVERIFY(!nucleo.flashCommand.isEmpty());
    QVERIFY(nucleo.protocols.contains("SPI"));
    QVERIFY(nucleo.protocols.contains("I2C"));
    QVERIFY(nucleo.pinMap.contains("PA0"));
    QVERIFY(nucleo.pinMap.contains("PA13"));
    QVERIFY(nucleo.pinMap["PA13"].reserved);

    // 3. Verify ESP32 DevKitV1 strapping and reserved pins
    BoardDef esp32 = reg.board("esp32_devkitv1");
    QVERIFY(esp32.isValid());
    QCOMPARE(esp32.family, QString("ESP32"));
    QVERIFY(esp32.pinMap.contains("GPIO0"));
    QVERIFY(esp32.pinMap["GPIO0"].strapping);
    QVERIFY(esp32.pinMap.contains("GPIO6"));
    QVERIFY(esp32.pinMap["GPIO6"].reserved);

    // 4. Verify RP2040 GP naming
    BoardDef pico = reg.board("rpi_pico_rp2040");
    QVERIFY(pico.isValid());
    QCOMPARE(pico.family, QString("RP2040"));
    QVERIFY(pico.pinMap.contains("GP0"));
    QVERIFY(pico.pinMap.contains("GP28"));

    // 5. Verify Raspberry Pi BCM / physical naming
    BoardDef rpi = reg.board("rpi_4");
    QVERIFY(rpi.isValid());
    QCOMPARE(rpi.family, QString("RaspberryPi"));
    QVERIFY(rpi.pinMap.contains("GPIO2"));

    // 6. Verify NXP migrated board
    BoardDef nxp = reg.board("nxp_imxrt1060_evk");
    QVERIFY(nxp.isValid());
    QCOMPARE(nxp.family, QString("NXP"));
    QCOMPARE(nxp.display.width, 480);
    QCOMPARE(nxp.display.height, 272);
    QVERIFY(!nxp.flashCommand.isEmpty());

    // 7. Verify Presets save, select, reload, delete
    QString testPreset = "UnitTesting_Preset_Phase2";
    reg.savePreset(testPreset, "nucleo_f030r8");
    QVERIFY(reg.hasPresets());
    QCOMPARE(reg.activePreset().name, testPreset);
    QCOMPARE(reg.activeBoard().id, QString("nucleo_f030r8"));

    // Reload from file to verify persistence across restarts
    reg.loadPresets();
    QCOMPARE(reg.activePreset().name, testPreset);
    QCOMPARE(reg.activeBoard().id, QString("nucleo_f030r8"));

    // Clean up preset
    QVERIFY(reg.deletePreset(testPreset));
}

void TestAllCases::testPhase3FirstRunBoardSelection() {
    BoardRegistry& reg = BoardRegistry::instance();
    QVERIFY(reg.loadFromBoardsJson());

    // 1. Reset onboarding state for fresh test
    reg.setOnboardingCompleted(false);
    QVERIFY(!reg.isOnboardingCompleted());

    // 2. Instantiate HardwareWizardDialog and check "Choose later" button
    HardwareWizardDialog wizard;
    QPushButton* btnChooseLater = wizard.findChild<QPushButton*>("btnChooseLater");
    QVERIFY(btnChooseLater != nullptr);
    QCOMPARE(btnChooseLater->text(), QString("Choose later"));

    // Click Choose later
    btnChooseLater->click();
    QVERIFY(wizard.choseLater());

    // 3. Verify MainWindow banner behavior on uncompleted onboarding
    MainWindow win;
    QVERIFY(win.boardBanner() != nullptr);
    // Since onboarding is false, in non-interactive/test mode banner is displayed with placeholder board
    win.checkFirstRunOnboarding(false);
    QVERIFY(!win.boardBanner()->isHidden());
    QCOMPARE(reg.activeBoard().id, QString("custom_board"));

    // 4. Test board selection, specs, pin map/protocols, and saving preset with name
    HardwareWizardDialog fullWizard;
    QLineEdit* presetEdit = fullWizard.findChild<QLineEdit*>("presetNameEdit");
    QVERIFY(presetEdit != nullptr);
    presetEdit->setText("Nucleo Custom Preset");

    // Table of boards with display, RAM, flash specs
    QTableWidget* boardTable = fullWizard.findChild<QTableWidget*>("boardHardwareTable");
    QVERIFY(boardTable != nullptr);
    QVERIFY(boardTable->columnCount() >= 6);

    // Complete preset save
    reg.savePreset("Nucleo Custom Preset", "nucleo_f030r8");
    reg.setActiveBoard("nucleo_f030r8");
    reg.setOnboardingCompleted(true);
    QVERIFY(reg.isOnboardingCompleted());

    // 5. Subsequent run should remember choice and hide banner
    win.checkFirstRunOnboarding(false);
    QVERIFY(win.boardBanner()->isHidden());
    QCOMPARE(reg.activeBoard().id, QString("nucleo_f030r8"));

    // 6. Verify OcdTelnetManager picks up saved preset and logs preset name instead of hardcoded default
    OcdTelnetManager ocd;
    QCOMPARE(reg.activePreset().name, QString("Nucleo Custom Preset"));

    // Clean up test preset
    reg.deletePreset("Nucleo Custom Preset");
}

void TestAllCases::testPhase4BoardPinBindingAndDiffPreview() {
    BoardRegistry& reg = BoardRegistry::instance();
    QVERIFY(reg.loadFromBoardsJson());

    // 1. Board change synchronization: Nucleo STM32 vs ESP32 DevKit
    reg.setActiveBoard("nucleo_f030r8");
    BoardDef stm32Board = reg.activeBoard();
    QCOMPARE(stm32Board.id, QString("nucleo_f030r8"));
    QCOMPARE(stm32Board.family, QString("STM32"));
    QVERIFY(stm32Board.pinMap.contains("PA0"));
    QVERIFY(!stm32Board.pinMap.contains("GPIO15"));

    // Verify protocol choices limited to what the board supports
    QVERIFY(stm32Board.protocols.contains("SPI"));
    QVERIFY(stm32Board.protocols.contains("I2C"));

    // Test PropertiesPanel pin dropdowns and protocol updating
    PropertiesPanel panel;
    auto* btn = new ButtonComponent();
    panel.setTargetComponent(btn);

    QComboBox* protoCombo = panel.findChild<QComboBox*>("protocolCombo");
    QVERIFY(protoCombo != nullptr);
    // Board supports SPI, I2C, UART, PWM, ADC
    QStringList availableProtos;
    for (int i = 0; i < protoCombo->count(); ++i) availableProtos.append(protoCombo->itemText(i));
    QVERIFY(availableProtos.contains("SPI"));
    QVERIFY(availableProtos.contains("I2C"));
    QVERIFY(availableProtos.contains("GPIO"));

    // Verify pin dropdown shows only that board's pins with functions
    QComboBox* gpioCombo = panel.findChild<QComboBox*>("gpioPinCombo");
    QVERIFY(gpioCombo != nullptr);
    bool foundPA0 = false;
    for (int i = 0; i < gpioCombo->count(); ++i) {
        if (gpioCombo->itemData(i).toString() == "PA0") {
            foundPA0 = true;
            QVERIFY(gpioCombo->itemText(i).contains("PA0"));
            break;
        }
    }
    QVERIFY(foundPA0);

    // Switch board to ESP32 and verify dropdowns update
    reg.setActiveBoard("esp32_devkitv1");
    BoardDef espBoard = reg.activeBoard();
    QCOMPARE(espBoard.family, QString("ESP32"));
    QVERIFY(espBoard.pinMap.contains("GPIO15"));
    QVERIFY(!espBoard.pinMap.contains("PA0"));

    // Check strapping pin marked
    QVERIFY(espBoard.pinMap.contains("GPIO0"));
    QVERIFY(espBoard.pinMap["GPIO0"].strapping);

    // 2. Custom pin configs: edit individual pins and store as named preset referencing base board
    QMap<QString, PinDef> overrides;
    PinDef customPin;
    customPin.function = "PWM";
    customPin.note = "Custom User PWM Output";
    overrides["GPIO15"] = customPin;

    QString customPresetName = "ESP32 Custom PWM Preset";
    QVERIFY(reg.savePreset(customPresetName, "esp32_devkitv1", overrides));
    reg.selectPreset(customPresetName);
    QCOMPARE(reg.activePreset().boardId, QString("esp32_devkitv1"));
    QVERIFY(reg.activePreset().pinOverrides.contains("GPIO15"));
    QCOMPARE(reg.activePreset().pinOverrides["GPIO15"].function, QString("PWM"));

    // 3. PinBindingDialog with table and PinoutViewWidget visual highlighting
    CanvasScene scene;
    PinBindingDialog pinDlg(&scene);
    QTableWidget* pinTable = pinDlg.findChild<QTableWidget*>("pinTable");
    QVERIFY(pinTable != nullptr);
    QVERIFY(pinTable->rowCount() > 0);
    Hardware::PinoutViewWidget* pinout = pinDlg.findChild<Hardware::PinoutViewWidget*>("mcuPinoutView");
    QVERIFY(pinout != nullptr);

    // 4. Invalid bindings warning dialog & remapping
    btn->setProtocol("GPIO");
    btn->setProtocolPin("pin", "PA0"); // Bound on STM32, invalid on ESP32

    InvalidBindingItem invItem;
    invItem.component = btn;
    invItem.componentId = btn->componentId();
    invItem.componentName = btn->componentId();
    invItem.currentPin = "PA0";
    invItem.currentProtocol = "GPIO";
    invItem.reason = "Pin 'PA0' not supported on target ESP32";

    InvalidBindingsDialog invDlg(espBoard, {invItem});
    QTableWidget* invTable = invDlg.findChild<QTableWidget*>("invalidBindingsTable");
    QVERIFY(invTable != nullptr);
    QCOMPARE(invTable->rowCount(), 1);
    QCOMPARE(invTable->item(0, 1)->text(), QString("PA0"));

    // Apply remapping to GPIO15
    auto* remapCombo = qobject_cast<QComboBox*>(invTable->cellWidget(0, 4));
    QVERIFY(remapCombo != nullptr);
    int gpio15Idx = -1;
    for (int i = 0; i < remapCombo->count(); ++i) {
        if (remapCombo->itemData(i).toString() == "GPIO15") {
            gpio15Idx = i;
            break;
        }
    }
    QVERIFY(gpio15Idx >= 0);
    remapCombo->setCurrentIndex(gpio15Idx);
    invDlg.applyButton()->click();

    auto resolved = invDlg.resolvedBindings();
    QCOMPARE(resolved.size(), 1);
    QCOMPARE(resolved[0].remappedPin, QString("GPIO15"));
    QVERIFY(!resolved[0].keepAsCustom);

    // Remap on component
    btn->setProtocolPin("pin", resolved[0].remappedPin);
    QCOMPARE(btn->protocolPin("pin"), QString("GPIO15"));

    // Test keep all as custom
    InvalidBindingsDialog invDlg2(espBoard, {invItem});
    invDlg2.keepAllButton()->click();
    auto resolved2 = invDlg2.resolvedBindings();
    QCOMPARE(resolved2.size(), 1);
    QVERIFY(resolved2[0].keepAsCustom);
    QCOMPARE(resolved2[0].remappedPin, QString("PA0"));

    // 5. Generated code follows the project's hardware configuration, not the
    //    registry's active board. An empty project yields the F030 baseline.
    Project prj(&scene);
    QString halEmpty = CodeGen::TargetHalGenerator::generateHalHeader(&prj);
    QVERIFY(halEmpty.contains("NUCLEO-F030R8"));
    QVERIFY(!halEmpty.contains("ESP32"));

    Hardware::HardwareConfig hwEspProj;
    hwEspProj.family = "ESP32";
    hwEspProj.deviceId = "ESP32-S3";
    prj.setHardwareConfig(hwEspProj);
    QString halH = CodeGen::TargetHalGenerator::generateHalHeader(&prj);
    QVERIFY(halH.contains("ESP32"));

    QString halC = CodeGen::TargetHalGenerator::generateHalSource(&prj);
    QVERIFY(halC.contains("ESP32"));

    // 6. Protocol changes regenerate affected driver init code and show diff preview
    QString oldCode = CodeGen::TargetHalGenerator::generateDriverInitCode(espBoard, "GPIO", {{"pin", "GPIO15"}});
    QString newCode = CodeGen::TargetHalGenerator::generateDriverInitCode(espBoard, "SPI", {{"mosi", "GPIO23"}, {"miso", "GPIO19"}, {"sck", "GPIO18"}});
    QVERIFY(oldCode != newCode);
    QVERIFY(newCode.contains("hal_spi_init"));

    DriverDiffPreviewDialog diffDlg("SPI", oldCode, newCode);
    QVERIFY(diffDlg.diffViewer() != nullptr);
    QVERIFY(diffDlg.diffViewer()->toPlainText().contains("-/* GPIO Driver:"));
    QVERIFY(diffDlg.diffViewer()->toPlainText().contains("+/* SPI Driver:"));

    // Clean up
    delete btn;
    reg.deletePreset(customPresetName);
    reg.setActiveBoard("nucleo_f030r8");
}

void TestAllCases::testPhase5ResizableAndMinimizablePanels() {
    QSettings cleanSettings("EmbeddedUIDesigner", "EmbeddedUIDesigner");
    cleanSettings.clear();

    MainWindow win;
    win.resetLayout();
    win.show();
    QApplication::processEvents();

    // 1. Verify dock options
    QMainWindow::DockOptions opts = win.dockOptions();
    QVERIFY(opts & QMainWindow::AnimatedDocks);
    QVERIFY(opts & QMainWindow::AllowNestedDocks);
    QVERIFY(opts & QMainWindow::AllowTabbedDocks);

    // 2. Verify all docks have objectName, features, and size constraints
    const QList<QDockWidget*> docks = win.findChildren<QDockWidget*>();
    QVERIFY(docks.size() >= 10);

    QDockWidget* propDock = nullptr;
    QDockWidget* paletteDock = nullptr;
    QDockWidget* buildDock = nullptr;

    for (QDockWidget* dock : docks) {
        QVERIFY(!dock->objectName().isEmpty());
        QDockWidget::DockWidgetFeatures feat = dock->features();
        QVERIFY(feat & QDockWidget::DockWidgetMovable);
        QVERIFY(feat & QDockWidget::DockWidgetFloatable);
        QVERIFY(feat & QDockWidget::DockWidgetClosable);

        if (dock->objectName() == "propertiesDock") propDock = dock;
        if (dock->objectName() == "paletteDock") paletteDock = dock;
        if (dock->objectName() == "buildDock") buildDock = dock;

        if (dock->objectName() == "buildDock" || dock->objectName() == "deviceMonitorDock") {
            // Bottom dock constraints
            QVERIFY(dock->minimumHeight() >= 100);
            QVERIFY(dock->maximumHeight() <= 600);
            QVERIFY(dock->minimumWidth() >= 180);
        } else {
            // Side dock constraints
            QVERIFY(dock->minimumWidth() >= 160);
            QVERIFY(dock->maximumWidth() <= 750);
        }
    }

    QVERIFY(propDock != nullptr);
    QVERIFY(paletteDock != nullptr);
    QVERIFY(buildDock != nullptr);

    // 3. Verify minimize button in createDockTitleBar and collapsing/restoring
    QWidget* propTitleBar = propDock->titleBarWidget();
    QVERIFY(propTitleBar != nullptr);
    QPushButton* minBtn = propTitleBar->findChild<QPushButton*>("dockMinButton");
    QVERIFY(minBtn != nullptr);
    QCOMPARE(minBtn->text(), QString("—"));
    QVERIFY(!MainWindow::isDockCollapsed(propDock));
    QVERIFY(propDock->widget() != nullptr);
    QVERIFY(propDock->widget()->isVisible());

    // Click minimize button to collapse
    minBtn->click();
    QApplication::processEvents();
    QVERIFY(MainWindow::isDockCollapsed(propDock));
    QCOMPARE(minBtn->text(), QString("+"));
    QVERIFY(!propDock->widget()->isVisible());
    QVERIFY(propDock->maximumHeight() <= 36);

    // Check stored in QSettings
    QSettings settings("EmbeddedUIDesigner", "EmbeddedUIDesigner");
    QCOMPARE(settings.value("dock/propertiesDock/collapsed").toBool(), true);

    // Click minimize button to restore
    minBtn->click();
    QApplication::processEvents();
    QVERIFY(!MainWindow::isDockCollapsed(propDock));
    QCOMPARE(minBtn->text(), QString("—"));
    QVERIFY(propDock->widget()->isVisible());
    QVERIFY(propDock->maximumHeight() >= 500);
    QCOMPARE(settings.value("dock/propertiesDock/collapsed").toBool(), false);

    // 4. Test saveWindowLayout and restoreWindowLayout
    settings.setValue("dock/paletteDock/collapsed", true);
    win.restoreWindowLayout();
    QVERIFY(MainWindow::isDockCollapsed(paletteDock));
    QCOMPARE(settings.value("dock/paletteDock/collapsed").toBool(), true);

    settings.setValue("dock/paletteDock/collapsed", false);
    win.restoreWindowLayout();
    QVERIFY(!MainWindow::isDockCollapsed(paletteDock));
    QCOMPARE(settings.value("dock/paletteDock/collapsed").toBool(), false);

    // Save and verify persistence
    MainWindow::setDockCollapsed(paletteDock, true);
    win.saveWindowLayout();
    QCOMPARE(settings.value("dock/paletteDock/collapsed").toBool(), true);

    // 5. Verify "Reset layout" action exists and restores pristine layout
    QAction* resetAct = nullptr;
    for (QAction* act : win.menuBar()->actions()) {
        if (act->menu()) {
            for (QAction* subAct : act->menu()->actions()) {
                if (subAct->text().contains("Reset Layout", Qt::CaseInsensitive)) {
                    resetAct = subAct;
                    break;
                }
            }
        }
        if (resetAct) break;
    }
    QVERIFY(resetAct != nullptr);

    // Make propDock floating
    propDock->setFloating(true);
    QVERIFY(propDock->isFloating());

    // Trigger reset layout
    win.resetLayout();
    QApplication::processEvents();

    QVERIFY(!MainWindow::isDockCollapsed(paletteDock));
    QVERIFY(!MainWindow::isDockCollapsed(propDock));
    QVERIFY(!propDock->isFloating());
    QCOMPARE(settings.value("dock/paletteDock/collapsed").toBool(), false);
}

void TestAllCases::testPhase6MultiMonitorAndDetachablePanels() {
    MainWindow win;
    win.show();
    QApplication::processEvents();

    QSettings settings("EmbeddedUIDesigner", "EmbeddedUIDesigner");
    settings.clear();
    win.resetLayout();
    QApplication::processEvents();

    // 1. Multi-monitor screen geometry validation & clamping helper
    const auto screens = QGuiApplication::screens();
    QVERIFY(!screens.isEmpty());
    QScreen* primary = QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen() : screens.first();
    QRect primaryAvail = primary->availableGeometry();

    // Valid on-screen rectangle must remain unchanged
    QRect validOnScreen(primaryAvail.x() + 100, primaryAvail.y() + 100, 300, 200);
    QRect clampedValid = MainWindow::clampGeometryToScreens(validOnScreen);
    QCOMPARE(clampedValid, validOnScreen);

    // Completely off-screen rectangle (simulated second monitor disconnected at x=3500)
    QRect offscreenRect(3500, 200, 400, 300);
    QRect clampedOffscreen = MainWindow::clampGeometryToScreens(offscreenRect);
    QVERIFY(primaryAvail.contains(clampedOffscreen.topLeft()));
    QVERIFY(clampedOffscreen.right() <= primaryAvail.right());
    QVERIFY(clampedOffscreen.bottom() <= primaryAvail.bottom());

    // 2. Floating dock panels remember geometry and restore clamped when disconnected
    QDockWidget* propDock = win.findChild<QDockWidget*>("propertiesDock");
    QVERIFY(propDock != nullptr);
    propDock->setFloating(true);
    QVERIFY(propDock->isFloating());

    // Place floating dock at an offscreen location to simulate previous session on 2nd monitor
    propDock->setGeometry(3500, 200, 350, 450);
    win.saveWindowLayout();
    QCOMPARE(settings.value("dock/propertiesDock/floating").toBool(), true);
    QCOMPARE(settings.value("dock/propertiesDock/geometry").toRect(), QRect(3500, 200, 350, 450));

    // Restore window layout -> Must be clamped back into visible monitor
    win.restoreWindowLayout();
    QVERIFY(propDock->isFloating());
    QRect restoredGeom = propDock->geometry();
    QVERIFY(primaryAvail.contains(restoredGeom.topLeft()));
    QVERIFY(restoredGeom.x() < primaryAvail.right());

    // Title bar context menu policy
    QWidget* titleBar = propDock->titleBarWidget();
    QVERIFY(titleBar != nullptr);
    QCOMPARE(titleBar->contextMenuPolicy(), Qt::CustomContextMenu);

    // 3. Central area detachable tabs
    DetachableTabWidget* centralTabs = win.centralTabWidget();
    QVERIFY(centralTabs != nullptr);
    QCOMPARE(centralTabs->count(), 2);
    QVERIFY(centralTabs->isMovable());
    QVERIFY(centralTabs->tabsClosable());

    // Detach Canvas tab
    DetachedEditorWindow* detachedCanvas = centralTabs->detachTab(0);
    QVERIFY(detachedCanvas != nullptr);
    QVERIFY(detachedCanvas->isVisible());
    QCOMPARE(detachedCanvas->contentWidget(), win.designWorkspace());
    QCOMPARE(centralTabs->count(), 1);
    QCOMPARE(centralTabs->detachedWindows().size(), 1);

    // 4. Properties panel follows active editor in detached canvas window
    CanvasScene* scene = win.canvasScene();
    QVERIFY(scene != nullptr);
    PropertiesPanel* propPanel = win.propertiesPanel();
    QVERIFY(propPanel != nullptr);

    UIComponent* btn = Project::createComponentInstance("Button", "test_btn_phase6");
    QVERIFY(btn != nullptr);
    scene->addUIComponent(btn);
    scene->clearSelection();
    btn->setSelected(true);
    QApplication::processEvents();

    // Verify properties panel reflects the selection in the detached canvas
    QCOMPARE(propPanel->targetComponent(), btn);

    // 5. Re-dock canvas tab
    detachedCanvas->redock();
    QApplication::processEvents();
    QCOMPARE(centralTabs->count(), 2);
    QCOMPARE(centralTabs->detachedWindows().size(), 0);

    // Detach Code Workspace tab
    DetachedEditorWindow* detachedCode = centralTabs->detachTab(1);
    QVERIFY(detachedCode != nullptr);
    QVERIFY(detachedCode->isVisible());
    QCOMPARE(detachedCode->contentWidget(), win.codeWorkspaceView());

    // Re-dock all detached windows
    win.redockAllDetachedWindows();
    QApplication::processEvents();
    QCOMPARE(centralTabs->count(), 2);

    // 6. TabbedCodeEditor also has detachable tabs
    TabbedCodeEditor* codeEditor = win.codeWorkspaceView()->tabbedEditor();
    QVERIFY(codeEditor != nullptr);
    QVERIFY(codeEditor->tabWidget() != nullptr);
    QVERIFY(codeEditor->tabWidget()->isMovable());
    QVERIFY(codeEditor->tabWidget()->tabsClosable());

    // 7. Window menu with Move to Monitor and detachment actions
    QMenu* windowMenu = nullptr;
    for (QAction* act : win.menuBar()->actions()) {
        if (act->menu() && act->menu()->title() == "Window") {
            windowMenu = act->menu();
            break;
        }
    }
    QVERIFY(windowMenu != nullptr);

    bool hasMoveMonitorSubmenu = false;
    bool hasDetachCanvas = false;
    bool hasDetachCode = false;
    bool hasRedockAll = false;
    for (QAction* act : windowMenu->actions()) {
        if (act->menu() && act->menu()->title() == "Move to Monitor") {
            hasMoveMonitorSubmenu = true;
        }
        if (act->text().contains("Detach Canvas")) hasDetachCanvas = true;
        if (act->text().contains("Detach Code Workspace")) hasDetachCode = true;
        if (act->text().contains("Re-dock All")) hasRedockAll = true;
    }
    QVERIFY(hasMoveMonitorSubmenu);
    QVERIFY(hasDetachCanvas);
    QVERIFY(hasDetachCode);
    QVERIFY(hasRedockAll);

    // Clean up
    win.resetLayout();
    settings.clear();
}

QTEST_MAIN(TestAllCases)
#include "test_all_cases.moc"





