#include <QTest>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QFile>
#include <QTextStream>
#include "ir/IRSerialization.h"

using namespace IR;

class TestIRSerialization : public QObject {
    Q_OBJECT

private slots:
    void testIRJsonRoundtrip();
    void testIRJsonPersistence();
    void testIRJsonVersionHandling();
    void testIRJsonValidation();
    void testIRColorSerialization();
    void testIREnumSerialization();
    void testIRStructuralValidation();
    void testIRCapabilityValidation();
};

void TestIRSerialization::testIRJsonRoundtrip() {
    IRProject project;
    project.id = IRNodeId::fromString("proj-uuid-1");
    project.name = "SmartThermostat";
    project.displayName = "Smart Thermostat UI";
    project.targetFramework = "lvgl";
    project.targetFamily = "STM32";
    project.targetBoard = "STM32H745";
    project.targetProfile.mcu = "STM32H745";
    project.targetProfile.framework = "lvgl";
    project.targetProfile.maxFlash = 1048576;
    project.targetProfile.maxRam = 262144;
    project.displayWidth = 480;
    project.displayHeight = 320;
    project.colorDepth = 16;
    project.colorFormat = "RGB565";

    // 3 screens
    for (int s = 1; s <= 3; ++s) {
        IRScreen screen;
        screen.id = IRNodeId::fromString(QString("screen-%1").arg(s));
        screen.name = QString("Screen %1").arg(s);
        screen.width = 480;
        screen.height = 320;
        screen.backgroundColor.type = IRColor::Type::Solid;
        screen.backgroundColor.solidColor = QColor("#1e2025");
        screen.rootLayout.type = IRLayoutType::Flex;
        screen.rootLayout.flexDirection = IRFlexDirection::Column;
        screen.rootLayout.justifyContent = IRJustifyContent::Center;
        screen.rootLayout.alignItems = IRAlignItems::Center;
        screen.rootLayout.gap = 12.0;

        // Add components (total 10 across screens)
        int compCount = (s == 1) ? 4 : 3;
        for (int c = 1; c <= compCount; ++c) {
            IRWidget w;
            w.id = IRNodeId::fromString(QString("w-%1-%2").arg(s).arg(c));
            w.componentType = (c % 2 == 0) ? "button" : "label";
            w.name = QString("Widget_%1_%2").arg(s).arg(c);
            w.label = QString("Label_%1_%2").arg(s).arg(c);
            w.layout.type = IRLayoutType::Absolute;
            w.layout.left = c * 20.0;
            w.layout.hasLeft = true;
            w.layout.top = c * 30.0;
            w.layout.hasTop = true;
            w.effects.backgroundColor.type = IRColor::Type::Solid;
            w.effects.backgroundColor.solidColor = QColor("#007acc");
            w.textStyle.color.type = IRColor::Type::Solid;
            w.textStyle.color.solidColor = QColor("#ffffff");
            w.textStyle.fontSize = 16;
            w.effects.borderRadius = 8;
            screen.widgets.append(w);
        }
        project.screens.append(screen);
    }

    // 5 data sources
    for (int ds = 1; ds <= 5; ++ds) {
        IRDataSource source;
        source.id = IRNodeId::fromString(QString("ds-%1").arg(ds));
        source.name = QString("Sensor_%1").arg(ds);
        source.type = (ds % 2 == 0) ? IRDataSourceType::Hardware : IRDataSourceType::Sensor;
        source.hardwareRef = QString("ADC1_CH%1").arg(ds);
        source.updateInterval = 500 * ds;
        project.dataSources.append(source);
    }

    // 8 bindings
    for (int b = 1; b <= 8; ++b) {
        IRBinding binding;
        binding.id = IRNodeId::fromString(QString("bind-%1").arg(b));
        binding.sourceId = QString("ds-%1").arg((b % 5) + 1);
        binding.componentId = IRNodeId::fromString(QString("w-1-%1").arg((b % 4) + 1));
        binding.propertyPath = "value";
        binding.direction = (b % 3 == 0) ? IRBindingDirection::ReadWrite : IRBindingDirection::Read;
        binding.transformExpression = QString("value * %1").arg(b);
        project.bindings.append(binding);
    }

    // Serialize to JSON
    QString jsonStr = IRSerialization::toJson(project);
    QVERIFY(!jsonStr.isEmpty());

    // Deserialize back
    QString error;
    IRProject restored = IRSerialization::fromJson(jsonStr, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));

    // Verify properties
    QCOMPARE(restored.name, project.name);
    QCOMPARE(restored.displayName, project.displayName);
    QCOMPARE(restored.targetProfile.mcu, project.targetProfile.mcu);
    QCOMPARE(restored.targetProfile.maxFlash, project.targetProfile.maxFlash);
    QCOMPARE(restored.displayWidth, 480);
    QCOMPARE(restored.displayHeight, 320);

    QCOMPARE(restored.screens.size(), 3);
    QCOMPARE(restored.screens[0].widgets.size(), 4);
    QCOMPARE(restored.screens[1].widgets.size(), 3);
    QCOMPARE(restored.screens[2].widgets.size(), 3);

    QCOMPARE(restored.dataSources.size(), 5);
    QCOMPARE(restored.bindings.size(), 8);

    // Verify first binding transformation
    QCOMPARE(restored.bindings[0].transformExpression, QString("value * 1"));
    QCOMPARE(restored.bindings[0].sourceId, QString("ds-2"));
}

void TestIRSerialization::testIRJsonPersistence() {
    IRProject project;
    project.id = IRNodeId::fromString("persistent-proj-id");
    project.name = "DiskPersistenceApp";
    project.displayWidth = 800;
    project.displayHeight = 480;

    IRScreen screen;
    screen.id = IRNodeId::fromString("screen-main");
    screen.name = "Main";
    screen.width = 800;
    screen.height = 480;

    IRWidget widget;
    widget.id = IRNodeId::fromString("btn-power");
    widget.componentType = "button";
    widget.label = "Power Off";
    screen.widgets.append(widget);
    project.screens.append(screen);

    QString jsonStr = project.toJson();
    QVERIFY(!jsonStr.isEmpty());

    // Write to temporary file
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString filePath = tempFile.fileName();
    tempFile.write(jsonStr.toUtf8());
    tempFile.close();

    // Read back
    QFile readFile(filePath);
    QVERIFY(readFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QString fileContent = QString::fromUtf8(readFile.readAll());
    readFile.close();

    QString error;
    IRProject restored = IRProject::fromJson(fileContent, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(restored.name, QString("DiskPersistenceApp"));
    QCOMPARE(restored.screens.size(), 1);
    QCOMPARE(restored.screens[0].widgets.size(), 1);
    QCOMPARE(restored.screens[0].widgets[0].label, QString("Power Off"));
}

void TestIRSerialization::testIRJsonVersionHandling() {
    // Valid 1.0 JSON
    QString validJson = R"({
        "irVersion": "1.0",
        "project": {
            "name": "ValidV1Project"
        }
    })";

    QString error;
    IRProject validProj = IRSerialization::fromJson(validJson, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(validProj.name, QString("ValidV1Project"));

    // Unsupported 2.0 JSON
    QString futureJson = R"({
        "irVersion": "2.0",
        "project": {
            "name": "FutureV2Project"
        }
    })";

    QString futureError;
    IRProject invalidProj = IRSerialization::fromJson(futureJson, &futureError);
    QVERIFY(!futureError.isEmpty());
    QVERIFY(futureError.contains("Unsupported IR version"));
    QCOMPARE(invalidProj.name, QString());
}

void TestIRSerialization::testIRJsonValidation() {
    // Malformed JSON (syntax error)
    QString brokenJson = "{ \"irVersion\": \"1.0\", \"project\": { incomplete json ...";
    QString error;
    IRProject proj = IRSerialization::fromJson(brokenJson, &error);
    QVERIFY(!error.isEmpty());
    QVERIFY(error.contains("Invalid JSON"));
    QCOMPARE(proj.name, QString());

    // Empty string
    QString emptyError;
    IRProject emptyProj = IRSerialization::fromJson("", &emptyError);
    QVERIFY(!emptyError.isEmpty());
    QVERIFY(emptyError.contains("Empty JSON string"));
}

void TestIRSerialization::testIRColorSerialization() {
    // Black
    QColor black(0, 0, 0);
    QString blackHex = IRSerialization::colorToHex(black);
    QCOMPARE(blackHex, QString("#000000"));
    QCOMPARE(IRSerialization::colorFromHex(blackHex), black);

    // White
    QColor white(255, 255, 255);
    QString whiteHex = IRSerialization::colorToHex(white);
    QCOMPARE(whiteHex, QString("#ffffff"));
    QCOMPARE(IRSerialization::colorFromHex(whiteHex), white);

    // Semi-transparent
    QColor semiTrans(255, 128, 64, 120);
    QString semiHex = IRSerialization::colorToHex(semiTrans);
    QVERIFY(semiHex.startsWith("#"));
    QCOMPARE(IRSerialization::colorFromHex(semiHex).alpha(), 120);

    // Fully transparent
    QColor trans(0, 0, 0, 0);
    QString transHex = IRSerialization::colorToHex(trans);
    QCOMPARE(IRSerialization::colorFromHex(transHex).alpha(), 0);
}

void TestIRSerialization::testIREnumSerialization() {
    // Layout Types
    QCOMPARE(IRSerialization::layoutTypeToString(IRLayoutType::Absolute), QString("absolute"));
    QCOMPARE(IRSerialization::layoutTypeToString(IRLayoutType::Flex), QString("flexbox"));
    QCOMPARE(IRSerialization::layoutTypeToString(IRLayoutType::Grid), QString("grid"));
    QCOMPARE(IRSerialization::stringToLayoutType("flexbox"), IRLayoutType::Flex);
    QCOMPARE(IRSerialization::stringToLayoutType("flex"), IRLayoutType::Flex);
    QCOMPARE(IRSerialization::stringToLayoutType("grid"), IRLayoutType::Grid);
    QCOMPARE(IRSerialization::stringToLayoutType("absolute"), IRLayoutType::Absolute);

    // Easing Types
    QCOMPARE(IRSerialization::easingTypeToString(IREasingType::Linear), QString("linear"));
    QCOMPARE(IRSerialization::easingTypeToString(IREasingType::EaseInOut), QString("ease-in-out"));
    QCOMPARE(IRSerialization::easingTypeToString(IREasingType::EaseInBounce), QString("ease-in-bounce"));
    QCOMPARE(IRSerialization::stringToEasingType("linear"), IREasingType::Linear);
    QCOMPARE(IRSerialization::stringToEasingType("ease-in-out"), IREasingType::EaseInOut);
    QCOMPARE(IRSerialization::stringToEasingType("ease-in-bounce"), IREasingType::EaseInBounce);

    // Directions
    QCOMPARE(IRSerialization::flexDirectionToString(IRFlexDirection::Column), QString("column"));
    QCOMPARE(IRSerialization::stringToFlexDirection("column"), IRFlexDirection::Column);
    QCOMPARE(IRSerialization::justifyContentToString(IRJustifyContent::SpaceBetween), QString("space-between"));
    QCOMPARE(IRSerialization::stringToJustifyContent("space-between"), IRJustifyContent::SpaceBetween);
}

void TestIRSerialization::testIRStructuralValidation() {
    // 1. Valid project passes with 0 error diagnostics
    IRProject validProj;
    validProj.name = "HealthyProject";
    validProj.displayWidth = 480;
    validProj.displayHeight = 320;
    IRScreen screen;
    screen.id = IRNodeId::fromString("screen_1");
    screen.name = "Screen 1";
    screen.width = 480;
    screen.height = 320;
    IRWidget btn;
    btn.id = IRNodeId::fromString("btn_1");
    btn.componentType = "Button";
    screen.widgets.append(btn);
    validProj.screens.append(screen);

    IRDiagnostics diags = validProj.validate();
    for (const auto& d : diags) {
        QVERIFY(d.severity != IRDiagnostic::Severity::Error);
    }

    // 2. Project with empty name triggers Error
    IRProject badNameProj = validProj;
    badNameProj.name = "";
    IRDiagnostics badNameDiags = badNameProj.validate();
    bool foundNameErr = false;
    for (const auto& d : badNameDiags) {
        if (d.severity == IRDiagnostic::Severity::Error && d.code == IRDiagnostic::Code::MissingRequiredProperty) {
            foundNameErr = true;
        }
    }
    QVERIFY(foundNameErr);

    // 3. Project with duplicate widget IDs triggers Error
    IRProject dupProj = validProj;
    IRWidget dupBtn;
    dupBtn.id = IRNodeId::fromString("btn_1"); // duplicate ID
    dupBtn.componentType = "Button";
    dupProj.screens[0].widgets.append(dupBtn);
    IRDiagnostics dupDiags = dupProj.validate();
    bool foundDupErr = false;
    for (const auto& d : dupDiags) {
        if (d.severity == IRDiagnostic::Severity::Error && d.code == IRDiagnostic::Code::DuplicateNodeId) {
            foundDupErr = true;
        }
    }
    QVERIFY(foundDupErr);

    // 4. Binding pointing to non-existent data source triggers Warning
    IRProject badBindingProj = validProj;
    IRBinding b;
    b.id = IRNodeId::generate();
    b.componentId = IRNodeId::fromString("btn_1");
    b.sourceId = "GHOST_SOURCE";
    badBindingProj.bindings.append(b);
    IRDiagnostics bindingDiags = badBindingProj.validate();
    bool foundOrphanWarn = false;
    for (const auto& d : bindingDiags) {
        if (d.severity == IRDiagnostic::Severity::Warning && d.code == IRDiagnostic::Code::OrphanedNode) {
            foundOrphanWarn = true;
        }
    }
    QVERIFY(foundOrphanWarn);
}

void TestIRSerialization::testIRCapabilityValidation() {
    IRProject proj;
    proj.name = "CapabilityTest";
    proj.displayWidth = 800;
    proj.displayHeight = 480;
    proj.colorFormat = "ARGB8888";

    IRScreen screen;
    screen.id = IRNodeId::fromString("screen_cap");
    screen.name = "CapScreen";
    screen.width = 800;
    screen.height = 480;

    IRWidget flexWidget;
    flexWidget.id = IRNodeId::fromString("widget_flex");
    flexWidget.componentType = "Container";
    flexWidget.layout.type = IRLayoutType::Flex;
    flexWidget.effects.backgroundColor.type = IRColor::Type::LinearGradient;
    flexWidget.effects.borderRadius = 15;
    flexWidget.effects.opacity = 0.5;
    screen.widgets.append(flexWidget);
    proj.screens.append(screen);

    // Constrained MCU capability profile
    IRTargetCapabilities caps;
    caps.targetId = "mcu-entry";
    caps.maxWidth = 480;  // Proj width 800 exceeds this
    caps.maxHeight = 320;
    caps.supportedColorFormats = {"RGB565"}; // Proj uses ARGB8888
    caps.supportsFlexLayout = false;
    caps.supportsGradients = false;
    caps.supportsBorderRadius = false;
    caps.supportsOpacity = false;

    IRDiagnostics capDiags = proj.validateAgainstCapabilities(caps);
    QVERIFY(!capDiags.isEmpty());

    bool widthMismatch = false;
    bool colorMismatch = false;
    bool flexUnsupported = false;
    bool gradUnsupported = false;
    bool radiusUnsupported = false;
    bool opacityUnsupported = false;

    for (const auto& d : capDiags) {
        if (d.code == IRDiagnostic::Code::CapabilityMismatch && d.propertyPath == "displayWidth") widthMismatch = true;
        if (d.code == IRDiagnostic::Code::CapabilityMismatch && d.propertyPath == "colorFormat") colorMismatch = true;
        if (d.code == IRDiagnostic::Code::UnsupportedProperty && d.message.contains("Flex")) flexUnsupported = true;
        if (d.code == IRDiagnostic::Code::UnsupportedProperty && d.message.contains("linear gradient")) gradUnsupported = true;
        if (d.code == IRDiagnostic::Code::UnsupportedProperty && d.message.contains("borderRadius")) radiusUnsupported = true;
        if (d.code == IRDiagnostic::Code::UnsupportedProperty && d.message.contains("opacity")) opacityUnsupported = true;
    }

    QVERIFY(widthMismatch);
    QVERIFY(colorMismatch);
    QVERIFY(flexUnsupported);
    QVERIFY(gradUnsupported);
    QVERIFY(radiusUnsupported);
    QVERIFY(opacityUnsupported);
}

QTEST_MAIN(TestIRSerialization)
#include "test_ir_serialization.moc"
