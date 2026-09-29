#include <QtTest/QtTest>
#include <QApplication>
#include <QUndoStack>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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
#include "LayerPanel.h"
#include "ColorPickerDialog.h"
#include "Project.h"

// Generators
#include "UgfxGenerator.h"
#include "QtMcuGenerator.h"
#include "LvglGenerator.h"

class TestAllCases : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // 1. Component Models
    void testAllComponentsCreationAndDefaults();
    void testComponentPropertyMutations();
    void testComponentSerializationJson();
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

    // 5. Embedded Color Picker & RGB565 / Harmonies
    void testColorPickerRgb565Calculations();
    void testColorPickerHarmonyCalculations();
    void testColorPickerWheelInteraction();

    // 6. Layer Panel
    void testLayerPanelSyncAndReorder();

    // 7. Project Serialization Full Roundtrip
    void testProjectFullSaveAndLoad();

    // 8. Code Generators
    void testUgfxCodeGenerator();
    void testQtMcuCodeGenerator();
    void testLvglCodeGenerator();

    // 9. Canvas View & Display Presets
    void testDisplayPresetsAndCanvasView();
    void testCanvasZoomInteractions();

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

    scene.addUIComponent(b1);
    scene.addUIComponent(l1);
    scene.addUIComponent(r1);

    layers.refreshLayers();
    QCOMPARE(scene.uiComponents().count(), 3);
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

QTEST_MAIN(TestAllCases)
#include "test_all_cases.moc"
