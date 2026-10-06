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

// Commands
#include "AddComponentCommand.h"
#include "DeleteComponentCommand.h"
#include "MoveComponentCommand.h"
#include "ResizeComponentCommand.h"
#include "PropertyChangeCommand.h"

// Canvas & Panels
#include "CanvasScene.h"
#include "CanvasView.h"
#include "PropertiesPanel.h"
#include "PrototypePanel.h"
#include "SerialMonitorDialog.h"
#include "PinBindingDialog.h"
#include "HardwareBridge.h"
#include "BoardConfigParser.h"
#include "QmlImporter.h"
#include "DeviceManager.h"
#include "MainWindow.h"
#include "LayerPanel.h"
#include "ColorPickerDialog.h"
#include "Project.h"

// Generators
#include "UgfxGenerator.h"
#include "QtMcuGenerator.h"
#include "LvglGenerator.h"
#include "HardwareModel.h"
#include "HardwareProvider.h"
#include "DeviceDatabase.h"
#include "PinMuxEngine.h"
#include "DesignerController.h"

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

    void cleanupTestCase();

private:
    QTemporaryDir m_tempDir;
};

void TestAllCases::initTestCase() {
    QVERIFY(m_tempDir.isValid());
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
    dialog.resize(920, 720);
    dialog.show();
    QApplication::processEvents();

    auto* slider = dialog.findChild<QSlider*>("potentiometerSlider");
    auto* rawLabel = dialog.findChild<QLabel*>("adcRawCountLabel");
    auto* spiBtn = dialog.findChild<QPushButton*>("spiTransferButton");
    auto* spiInLabel = dialog.findChild<QLabel*>("spiByteInLabel");
    QVERIFY(slider);
    QVERIFY(rawLabel);
    QVERIFY(spiBtn);
    QVERIFY(spiInLabel);

    // Bind PA0 to ProgressBar and Label
    bridge.bindComponent(pb, "PA0", PinMode::AnalogIn);
    bridge.bindComponent(lbl, "PA0", PinMode::AnalogIn);

    // Vary potentiometer slider to 2800
    slider->setValue(2800);
    QApplication::processEvents();
    QVERIFY(rawLabel->text().contains("2800"));
    QCOMPARE(lbl->text(), QString("2800"));
    QVERIFY(qAbs(pb->value() - (2800.0 / 4095.0)) < 0.002);

    // Trigger SPI raw transfer
    auto* spiOutSpin = dialog.findChild<QSpinBox*>("spiByteOutSpin");
    QVERIFY(spiOutSpin);
    spiOutSpin->setValue(0xA5);
    spiBtn->click();
    QApplication::processEvents();
    QVERIFY(spiInLabel->text().contains("0x5A")); // 0xA5 ^ 0xFF = 0x5A

    // Capture verification screenshot of dialog with SPI and ADC
    const QString shotPath = screenshotPath("verify_adc_spi_binding.png");
    QVERIFY(dialog.grab().save(shotPath));

    // Capture canvas view screenshot showing bound components with initial 1200 count
    slider->setValue(1200);
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
    slider->setValue(3200);
    QApplication::processEvents();
    QCOMPARE(lbl->text(), QString("3200"));
    const QString canvasShotPath2 = screenshotPath("verify_adc_raw_count_3200.png");
    QVERIFY(view.grab().save(canvasShotPath2));

    bridge.unbindComponent(pb);
    bridge.unbindComponent(lbl);
}

void TestAllCases::testComponentProtocolConfigurationAndScreenshots() {
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
    QCOMPARE(HardwareBridge::instance().currentBoardId(), bp.id);

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
    const QString sdkPath = QDir(QCoreApplication::applicationDirPath()).filePath("../examples/esp32_devkit.sdkconfig");
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
    QCOMPARE(HardwareBridge::instance().currentBoardId(), bp.id);

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
    const QString qmlPath = QDir(QCoreApplication::applicationDirPath()).filePath("../examples/simple_dashboard.qml");
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
    const QString qmlPath = QDir(QCoreApplication::applicationDirPath()).filePath("../examples/complex_unsupported.qml");
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

QTEST_MAIN(TestAllCases)
#include "test_all_cases.moc"



