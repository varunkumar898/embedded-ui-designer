#include <QTest>
#include <QSignalSpy>
#include "ir/IRAdapter.h"
#include "ir/IRSerialization.h"
#include "project/Project.h"
#include "models/Screen.h"
#include "models/ButtonComponent.h"
#include "models/LabelComponent.h"
#include "models/ProgressBarComponent.h"
#include "models/GaugeComponent.h"
#include "models/DataSource.h"
#include "models/DataBinding.h"
#include "models/ImageComponent.h"
#include "models/GroupComponent.h"

using namespace IR;

class TestIRAdapter : public QObject {
    Q_OBJECT

private slots:
    void testProjectToIRConversion();
    void testIRToProjectConversion();
    void testRoundTripProjectToIRToProject();
    void testRoundTripIRToProjectToIR();
    void testDataBindingFidelity();
    void testCustomPropertiesFidelity();
    void testAssetExtractionFidelity();
    void testContainerHierarchyFidelity();
    void testDisplayNameFidelity();
};

void TestIRAdapter::testProjectToIRConversion() {
    Project project(nullptr);
    project.newProject("AdapterTestApp", 480, 272);
    project.setTargetFramework("lvgl");

    Hardware::HardwareConfig hw;
    hw.deviceId = "ESP32-S3";
    hw.flashBytes = 8 * 1024 * 1024;
    hw.ramBytes = 512 * 1024;
    project.setHardwareConfig(hw);

    Screen* screen = project.activeScreen();
    QVERIFY(screen != nullptr);

    auto* btn = new ButtonComponent("btn_start");
    btn->setText("Start Motor");
    btn->setCompPos(40, 50);
    btn->setCompSize(140, 45);
    screen->addComponent(btn);

    auto* lbl = new LabelComponent("lbl_speed");
    lbl->setText("RPM: 1200");
    lbl->setCompPos(200, 50);
    screen->addComponent(lbl);

    DataSource ds("RPM_SOURCE", "Motor RPM", DataSourceType::Variable);
    ds.setHardwareRef("GPIO_18");
    QJsonObject meta;
    meta["pollingIntervalMs"] = 100;
    ds.setMetadata(meta);
    project.addDataSource(ds);

    DataBinding binding("lbl_speed", "text", "RPM_SOURCE", BindingDirection::Read);
    binding.setTransformExpression("RPM: " + QString("value"));
    project.addDataBinding(binding);

    // Convert Project -> IRProject
    IRProject ir = ProjectToIRAdapter::convert(project);

    QCOMPARE(ir.name, QString("AdapterTestApp"));
    QCOMPARE(ir.targetFramework, QString("lvgl"));
    QCOMPARE(ir.displayWidth, 480);
    QCOMPARE(ir.displayHeight, 272);
    QCOMPARE(ir.targetProfile.mcu, QString("ESP32-S3"));
    QCOMPARE(ir.screens.size(), 1);
    QCOMPARE(ir.screens[0].widgets.size(), 2);
    QCOMPARE(ir.dataSources.size(), 1);
    QCOMPARE(ir.bindings.size(), 1);

    // Verify component conversion
    QCOMPARE(ir.screens[0].widgets[0].id.toString(), QString("btn_start"));
    QCOMPARE(ir.screens[0].widgets[0].label, QString("Start Motor"));
    QCOMPARE(ir.screens[0].widgets[0].layout.left, 40.0);
    QCOMPARE(ir.screens[0].widgets[0].layout.top, 50.0);
}

void TestIRAdapter::testIRToProjectConversion() {
    IRProject ir;
    ir.name = "FromIRApp";
    ir.displayName = "FromIRApp";
    ir.targetFramework = "qt-for-mcus";
    ir.targetBoard = "STM32H745";
    ir.targetProfile.mcu = "STM32H745";
    ir.targetProfile.maxFlash = 2097152;
    ir.targetProfile.maxRam = 1048576;
    ir.displayWidth = 800;
    ir.displayHeight = 480;

    // 3 Screens
    for (int s = 1; s <= 3; ++s) {
        IRScreen screen;
        screen.id = IRNodeId::fromString(QString("screen_%1").arg(s));
        screen.name = QString("Screen_%1").arg(s);
        screen.width = 800;
        screen.height = 480;
        screen.backgroundColor.solidColor = QColor(20, 20, 20);

        IRWidget btn;
        btn.id = IRNodeId::fromString(QString("btn_screen_%1").arg(s));
        btn.componentType = "Button";
        btn.label = QString("Button %1").arg(s);
        btn.layout.left = 100 * s;
        btn.layout.hasLeft = true;
        btn.layout.top = 80;
        btn.layout.hasTop = true;
        screen.widgets.append(btn);

        ir.screens.append(screen);
    }

    IRDataSource ds;
    ds.id = IRNodeId::fromString("ds_temp");
    ds.name = "TemperatureSensor";
    ds.hardwareRef = "ADC1_IN5";
    ir.dataSources.append(ds);

    std::unique_ptr<Project> project = IRToProjectAdapter::convert(ir);
    QVERIFY(project != nullptr);

    QCOMPARE(project->projectName(), QString("FromIRApp"));
    QCOMPARE(project->targetFramework(), QString("qt-for-mcus"));
    QCOMPARE(project->displayConfig().width, 800);
    QCOMPARE(project->displayConfig().height, 480);
    QCOMPARE(project->hardwareConfig().deviceId, QString("STM32H745"));
    QCOMPARE(project->screenCount(), 3);
    QCOMPARE(project->screens()[0]->components().size(), 1);
    QCOMPARE(project->dataSources().size(), 1);
    QCOMPARE(project->dataSources()[0].id(), QString("ds_temp"));

    // Verify UIComponent class type
    UIComponent* comp = project->screens()[0]->components()[0];
    QVERIFY(comp != nullptr);
    auto* btnComp = dynamic_cast<ButtonComponent*>(comp);
    QVERIFY(btnComp != nullptr);
    QCOMPARE(btnComp->text(), QString("Button 1"));
    QCOMPARE(btnComp->compX(), 100.0);
}

void TestIRAdapter::testRoundTripProjectToIRToProject() {
    Project original(nullptr);
    original.newProject("RoundtripProject", 320, 240);
    original.setTargetFramework("lvgl");

    Screen* screen = original.activeScreen();
    QVERIFY(screen != nullptr);

    auto* btn = new ButtonComponent("btn_unique_1");
    btn->setText("Submit");
    btn->setCompPos(25, 35);
    btn->setCompSize(110, 40);
    screen->addComponent(btn);

    auto* prog = new ProgressBarComponent("prog_unique_1");
    prog->setValue(65);
    prog->setCompPos(50, 100);
    prog->setCompSize(200, 20);
    screen->addComponent(prog);

    DataSource ds("DS_BATTERY", "Battery Level", DataSourceType::Variable);
    ds.setHardwareRef("ADC_BATT");
    original.addDataSource(ds);

    DataBinding b("prog_unique_1", "value", "DS_BATTERY", BindingDirection::Read);
    original.addDataBinding(b);

    // Project -> IR -> Project
    IRProject ir = ProjectToIRAdapter::convert(original);
    std::unique_ptr<Project> restored = IRToProjectAdapter::convert(ir);
    QVERIFY(restored != nullptr);

    QCOMPARE(restored->projectName(), original.projectName());
    QCOMPARE(restored->targetFramework(), original.targetFramework());
    QCOMPARE(restored->displayConfig().width, original.displayConfig().width);
    QCOMPARE(restored->displayConfig().height, original.displayConfig().height);
    QCOMPARE(restored->screenCount(), original.screenCount());

    Screen* restoredScreen = restored->screens().first();
    QCOMPARE(restoredScreen->components().size(), 2);

    UIComponent* rBtn = restoredScreen->findComponentById("btn_unique_1");
    QVERIFY(rBtn != nullptr);
    QCOMPARE(rBtn->compX(), 25.0);
    QCOMPARE(rBtn->compY(), 35.0);
    auto* rBtnReal = dynamic_cast<ButtonComponent*>(rBtn);
    QVERIFY(rBtnReal != nullptr);
    QCOMPARE(rBtnReal->text(), QString("Submit"));

    UIComponent* rProg = restoredScreen->findComponentById("prog_unique_1");
    QVERIFY(rProg != nullptr);
    QCOMPARE(rProg->compX(), 50.0);
    QCOMPARE(rProg->compY(), 100.0);
}

void TestIRAdapter::testRoundTripIRToProjectToIR() {
    IRProject irOriginal;
    irOriginal.name = "IRRoundtrip";
    irOriginal.displayName = "IRRoundtrip";
    irOriginal.targetFramework = "lvgl";
    irOriginal.targetBoard = "STM32F030R8";
    irOriginal.targetProfile.mcu = "STM32F030R8";
    irOriginal.displayWidth = 320;
    irOriginal.displayHeight = 240;

    IRScreen screen;
    screen.id = IRNodeId::fromString("screen_main");
    screen.name = "MainScreen";
    screen.width = 320;
    screen.height = 240;

    IRWidget widget;
    widget.id = IRNodeId::fromString("widget_btn");
    widget.componentType = "Button";
    widget.label = "Click Here";
    widget.layout.left = 45;
    widget.layout.hasLeft = true;
    widget.layout.top = 55;
    widget.layout.hasTop = true;
    screen.widgets.append(widget);
    irOriginal.screens.append(screen);

    // IR -> Project -> IR
    std::unique_ptr<Project> proj = IRToProjectAdapter::convert(irOriginal);
    QVERIFY(proj != nullptr);
    IRProject irRestored = ProjectToIRAdapter::convert(*proj);

    // Verify key fields
    QCOMPARE(irRestored.name, irOriginal.name);
    QCOMPARE(irRestored.displayWidth, irOriginal.displayWidth);
    QCOMPARE(irRestored.displayHeight, irOriginal.displayHeight);
    QCOMPARE(irRestored.screens.size(), 1);
    QCOMPARE(irRestored.screens[0].widgets.size(), 1);
    QCOMPARE(irRestored.screens[0].widgets[0].id.toString(), QString("widget_btn"));
    QCOMPARE(irRestored.screens[0].widgets[0].label, QString("Click Here"));
    QCOMPARE(irRestored.screens[0].widgets[0].layout.left, 45.0);
    QCOMPARE(irRestored.screens[0].widgets[0].layout.top, 55.0);
}

void TestIRAdapter::testDataBindingFidelity() {
    Project project(nullptr);
    project.newProject("BindingTest", 320, 240);

    Screen* screen = project.activeScreen();
    auto* lbl = new LabelComponent("lbl_val");
    screen->addComponent(lbl);

    DataSource ds("ADC_IN", "Sensor Reading", DataSourceType::Variable);
    project.addDataSource(ds);

    DataBinding binding("lbl_val", "text", "ADC_IN", BindingDirection::Read);
    binding.setTransformExpression("Math.round(value * 3.3 / 4095.0, 2) + ' V'");
    project.addDataBinding(binding);

    // Round trip
    IRProject ir = ProjectToIRAdapter::convert(project);
    std::unique_ptr<Project> restored = IRToProjectAdapter::convert(ir);
    QVERIFY(restored != nullptr);

    QCOMPARE(restored->dataBindings().size(), 1);
    const DataBinding& rb = restored->dataBindings()[0];
    QCOMPARE(rb.componentId(), QString("lbl_val"));
    QCOMPARE(rb.propertyName(), QString("text"));
    QCOMPARE(rb.sourceId(), QString("ADC_IN"));
    QCOMPARE(rb.transformExpression(), QString("Math.round(value * 3.3 / 4095.0, 2) + ' V'"));
}

void TestIRAdapter::testCustomPropertiesFidelity() {
    Project project(nullptr);
    project.newProject("CustomPropsTest", 320, 240);

    Screen* screen = project.activeScreen();
    auto* gauge = new GaugeComponent("gauge_1");
    gauge->setMinimum(10.0);
    gauge->setMaximum(180.0);
    gauge->setValue(75.5);
    gauge->setUnit("km/h");
    screen->addComponent(gauge);

    // Round trip
    IRProject ir = ProjectToIRAdapter::convert(project);
    std::unique_ptr<Project> restored = IRToProjectAdapter::convert(ir);
    QVERIFY(restored != nullptr);

    Screen* rScreen = restored->activeScreen();
    UIComponent* comp = rScreen->findComponentById("gauge_1");
    QVERIFY(comp != nullptr);

    auto* rGauge = dynamic_cast<GaugeComponent*>(comp);
    QVERIFY(rGauge != nullptr);
    QCOMPARE(rGauge->minimum(), 10.0);
    QCOMPARE(rGauge->maximum(), 180.0);
    QCOMPARE(rGauge->value(), 75.5);
    QCOMPARE(rGauge->unit(), QString("km/h"));
}

void TestIRAdapter::testAssetExtractionFidelity() {
    Project project(nullptr);
    project.newProject("AssetTestApp", 480, 320);

    Screen* screen = project.activeScreen();
    QVERIFY(screen != nullptr);

    auto* img = new ImageComponent("logo_img");
    img->setImagePath("assets/embedded_logo.png");
    img->setFormat("RGB565");
    img->setOpacityPercent(85);
    img->setCompPos(20, 30);
    img->setCompSize(120, 60);
    screen->addComponent(img);

    // Convert Project -> IR
    IRProject ir = ProjectToIRAdapter::convert(project);

    // Verify Asset extraction in IR
    QCOMPARE(ir.assets.size(), 1);
    const IRAsset& a = ir.assets.first();
    QCOMPARE(a.filePath, QString("assets/embedded_logo.png"));
    QCOMPARE(a.name, QString("embedded_logo.png"));
    QCOMPARE(a.colorFormat, QString("RGB565"));

    // Verify Widget properties
    QCOMPARE(ir.screens[0].widgets.size(), 1);
    const IRWidget& w = ir.screens[0].widgets.first();
    QCOMPARE(w.properties.value("imagePath").toString(), QString("assets/embedded_logo.png"));

    // Round-trip IR -> Project
    std::unique_ptr<Project> restored = IRToProjectAdapter::convert(ir);
    QVERIFY(restored != nullptr);

    Screen* rScreen = restored->activeScreen();
    UIComponent* rComp = rScreen->findComponentById("logo_img");
    QVERIFY(rComp != nullptr);
    auto* rImg = dynamic_cast<ImageComponent*>(rComp);
    QVERIFY(rImg != nullptr);
    QCOMPARE(rImg->imagePath(), QString("assets/embedded_logo.png"));
    QCOMPARE(rImg->format(), QString("RGB565"));
    QCOMPARE(rImg->opacityPercent(), 85);
}

void TestIRAdapter::testContainerHierarchyFidelity() {
    Project project(nullptr);
    project.newProject("HierarchyTestApp", 320, 240);

    Screen* screen = project.activeScreen();
    QVERIFY(screen != nullptr);

    auto* group = new GroupComponent("grp_header");
    group->setCompPos(0, 0);
    group->setCompSize(320, 50);

    auto* btn = new ButtonComponent("header_back");
    btn->setText("Back");
    btn->setCompPos(10, 10);
    group->addChild(btn);

    auto* lbl = new LabelComponent("header_title");
    lbl->setText("Settings");
    lbl->setCompPos(70, 15);
    group->addChild(lbl);

    screen->addComponent(group);

    // Convert Project -> IR
    IRProject ir = ProjectToIRAdapter::convert(project);
    QCOMPARE(ir.screens.size(), 1);
    QCOMPARE(ir.screens[0].widgets.size(), 1);

    const IRWidget& irGrp = ir.screens[0].widgets.first();
    QCOMPARE(irGrp.id.toString(), QString("grp_header"));
    QCOMPARE(irGrp.children.size(), 2);
    QCOMPARE(irGrp.children[0].id.toString(), QString("header_back"));
    QCOMPARE(irGrp.children[1].id.toString(), QString("header_title"));

    // Round-trip IR -> Project
    std::unique_ptr<Project> restored = IRToProjectAdapter::convert(ir);
    QVERIFY(restored != nullptr);

    Screen* rScreen = restored->activeScreen();
    UIComponent* rComp = rScreen->findComponentById("grp_header");
    QVERIFY(rComp != nullptr);
    auto* rGrp = dynamic_cast<GroupComponent*>(rComp);
    QVERIFY(rGrp != nullptr);
    QCOMPARE(rGrp->children().size(), 2);

    UIComponent* rBtn = rGrp->children()[0];
    UIComponent* rLbl = rGrp->children()[1];
    QCOMPARE(rBtn->componentId(), QString("header_back"));
    QCOMPARE(rLbl->componentId(), QString("header_title"));
}

void TestIRAdapter::testDisplayNameFidelity() {
    Project project(nullptr);
    project.newProject("system_dashboard_v1", 480, 320);
    project.setDisplayName("Vehicle Telemetry Cluster");

    IRProject ir = ProjectToIRAdapter::convert(project);
    QCOMPARE(ir.name, QString("system_dashboard_v1"));
    QCOMPARE(ir.displayName, QString("Vehicle Telemetry Cluster"));

    std::unique_ptr<Project> restored = IRToProjectAdapter::convert(ir);
    QVERIFY(restored != nullptr);
    QCOMPARE(restored->projectName(), QString("system_dashboard_v1"));
    QCOMPARE(restored->displayName(), QString("Vehicle Telemetry Cluster"));
}

QTEST_MAIN(TestIRAdapter)
#include "test_ir_adapter.moc"
