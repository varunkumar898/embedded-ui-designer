#include <QTest>
#include <QDir>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "driverpack/CustomPackModel.h"
#include "driverpack/CmsisSvdImporter.h"
#include "driverpack/SdkReferenceChecker.h"
#include "driverpack/PinTableParser.h"
#include "driverpack/CustomDriverPack.h"
#include "driverpack/CustomPackManager.h"
#include "driverpack/DriverPackRegistry.h"
#include "analyzer/CapabilityDatabase.h"
#include "codegen/FirmwareExportManager.h"
#include "project/Project.h"
#include "canvas/CanvasScene.h"

using namespace driverpack;

class TestCustomPacks : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // 1. Pack model & schema validation
    void testValidPackCreationAndValidation();
    void testInvalidOrMissingFields();

    // 2. CMSIS-SVD importer
    void testCmsisSvdImporterValid();
    void testMaliciousSvdXxe();
    void testMaliciousSvdHugeFile();
    void testMaliciousSvdDeepNesting();

    // 3. Pin table
    void testPinTableValid();
    void testPinTableUnknownPeripheralRejected();

    // 4. SDK reference
    void testSdkReferenceChecking();

    // 5. Pack installation under user data directory & CapabilityDatabase injection
    void testCustomPackInstallation();

    // 6. Firmware export with custom pack: clean family output, SVD register headers
    void testExportFromCustomPackCleanOutput();

    // 7. Unsupported device export behavior
    void testExportUnsupportedDeviceUnchanged();

private:
    QTemporaryDir m_tempDir;
};

void TestCustomPacks::initTestCase() {
    QVERIFY(m_tempDir.isValid());
}

void TestCustomPacks::cleanupTestCase() {
}

void TestCustomPacks::testValidPackCreationAndValidation() {
    CustomPackDef pack;
    pack.name = "TestMCU400";
    pack.vendor = "CustomSemi";
    pack.partNumber = "TS400VG";
    pack.family = "Custom";
    pack.core = "Cortex-M4";
    pack.clockSpeedMhz = 120;

    CustomMemoryRegion flash;
    flash.name = "FLASH";
    flash.type = "flash";
    flash.startAddress = "0x08000000";
    flash.sizeKb = 1024;
    pack.memoryRegions.append(flash);

    CustomMemoryRegion sram;
    sram.name = "SRAM";
    sram.type = "sram";
    sram.startAddress = "0x20000000";
    sram.sizeKb = 256;
    sram.usableForFramebuffer = 1;
    pack.memoryRegions.append(sram);

    CustomPeripheralDef gpioa;
    gpioa.name = "GPIOA";
    gpioa.baseAddress = "0x40020000";
    gpioa.type = "GPIO";
    gpioa.description = "General Purpose I/O A";

    CustomRegisterDef moder;
    moder.name = "MODER";
    moder.offset = "0x00";
    gpioa.registers.append(moder);
    pack.peripherals.append(gpioa);

    CustomPinDef pin0;
    pin0.pin = "PA0";
    pin0.function = "GPIO_Output";
    pin0.peripheral = "GPIOA";
    pack.pinTable.append(pin0);

    pack.provenance.sourceFiles = {"test_svd.xml"};
    pack.provenance.importerVersion = "1.0.0";
    pack.provenance.generatedBy = "importer";
    pack.provenance.verified = false; // verified: false by default

    QStringList errors;
    QVERIFY2(pack.isValid(&errors), qPrintable(errors.join(", ")));

    // Test round-trip JSON serialization
    QJsonObject json = pack.toJson();
    CustomPackDef deserialized = CustomPackDef::fromJson(json, &errors);
    QVERIFY2(errors.isEmpty(), qPrintable(errors.join(", ")));
    QCOMPARE(deserialized.name, pack.name);
    QCOMPARE(deserialized.vendor, pack.vendor);
    QCOMPARE(deserialized.partNumber, pack.partNumber);
    QCOMPARE(deserialized.totalRamKb(), 256u);
    QCOMPARE(deserialized.totalFlashKb(), 1024u);
    QCOMPARE(deserialized.provenance.verified, false);
}

void TestCustomPacks::testInvalidOrMissingFields() {
    CustomPackDef badPack;
    QStringList errors;
    QVERIFY(!badPack.isValid(&errors));
    QVERIFY(!errors.isEmpty());

    // Test specific field rejections via JSON validator
    QJsonObject root;
    QStringList jsonErrors;
    QVERIFY(!CustomPackValidator::validateJson(root, jsonErrors));
    QVERIFY(jsonErrors.join(" ").contains("device_identity"));

    // Invalid hex address
    QJsonObject id;
    id["name"] = "BadMCU";
    id["vendor"] = "Acme";
    id["part_number"] = "BM100";
    root["device_identity"] = id;
    root["family"] = "Custom";
    root["core"] = "Cortex-M0";

    QJsonArray mems;
    QJsonObject mem;
    mem["name"] = "FLASH";
    mem["type"] = "flash";
    mem["start_address"] = "INVALID_HEX";
    mem["size_kb"] = 64;
    mems.append(mem);
    root["memory_regions"] = mems;

    jsonErrors.clear();
    QVERIFY(!CustomPackValidator::validateJson(root, jsonErrors));
    QVERIFY(jsonErrors.join(" ").contains("not a valid hex address"));
}

void TestCustomPacks::testCmsisSvdImporterValid() {
    QString fixturePath = QDir::current().filePath("tests/fixtures/tiny_test.svd");
    if (!QFile::exists(fixturePath)) {
        fixturePath = "../tests/fixtures/tiny_test.svd";
    }
    QVERIFY2(QFile::exists(fixturePath), qPrintable(fixturePath));

    SvdImportResult res = CmsisSvdImporter::importFromFile(fixturePath);
    QVERIFY2(res.success, qPrintable(res.errorMessage));
    QCOMPARE(res.deviceName, QString("MyCustomMCU"));
    QCOMPARE(res.vendor, QString("AcmeSemi"));
    QCOMPARE(res.cpuName, QString("CM4"));
    QCOMPARE(res.clockFrequencyHz, 80000000u);
    QCOMPARE(res.peripherals.size(), 2);

    // Verify GPIOA
    const CustomPeripheralDef* gpioa = nullptr;
    const CustomPeripheralDef* usart1 = nullptr;
    for (const auto& p : res.peripherals) {
        if (p.name == "GPIOA") gpioa = &p;
        if (p.name == "USART1") usart1 = &p;
    }
    QVERIFY(gpioa != nullptr);
    QVERIFY(usart1 != nullptr);

    QCOMPARE(gpioa->baseAddress, QString("0x40020000"));
    QCOMPARE(gpioa->registers.size(), 3);
    QCOMPARE(gpioa->registers[0].name, QString("MODER"));
    QCOMPARE(gpioa->registers[0].offset, QString("0x0"));

    QCOMPARE(usart1->baseAddress, QString("0x40013800"));
    QCOMPARE(usart1->registers.size(), 2);
}

void TestCustomPacks::testMaliciousSvdXxe() {
    QString fixturePath = QDir::current().filePath("tests/fixtures/xxe_test.svd");
    if (!QFile::exists(fixturePath)) {
        fixturePath = "../tests/fixtures/xxe_test.svd";
    }
    QVERIFY2(QFile::exists(fixturePath), qPrintable(fixturePath));

    SvdImportResult res = CmsisSvdImporter::importFromFile(fixturePath);
    // Should safely reject the external entity or fail without reading /etc/passwd
    QVERIFY(!res.success || !res.deviceName.contains("root:"));
    if (!res.success) {
        QVERIFY(res.errorMessage.contains("XXE") || res.errorMessage.contains("Disallowed") || res.errorMessage.contains("entities"));
    }
}

void TestCustomPacks::testMaliciousSvdHugeFile() {
    // Construct string larger than 10MB
    QByteArray huge(11 * 1024 * 1024, '<');
    SvdImportResult res = CmsisSvdImporter::importFromString(QString::fromUtf8(huge));
    QVERIFY(!res.success);
    QVERIFY(res.errorMessage.contains("exceeds maximum"));
}

void TestCustomPacks::testMaliciousSvdDeepNesting() {
    // Construct XML with 70 nested tags (exceeds MAX_XML_DEPTH = 64)
    QString deepXml = "<?xml version=\"1.0\"?>";
    for (int i = 0; i < 70; ++i) {
        deepXml += QString("<tag%1>").arg(i);
    }
    deepXml += "payload";
    for (int i = 69; i >= 0; --i) {
        deepXml += QString("</tag%1>").arg(i);
    }

    SvdImportResult res = CmsisSvdImporter::importFromString(deepXml);
    QVERIFY(!res.success);
    QVERIFY(res.errorMessage.contains("depth limit") || res.errorMessage.contains("nesting"));
}

void TestCustomPacks::testPinTableValid() {
    QString fixturePath = QDir::current().filePath("tests/fixtures/valid_pins.csv");
    if (!QFile::exists(fixturePath)) {
        fixturePath = "../tests/fixtures/valid_pins.csv";
    }
    QVERIFY2(QFile::exists(fixturePath), qPrintable(fixturePath));

    QSet<QString> allowed = {"GPIOA", "USART1"};
    PinTableParseResult res = PinTableParser::parseCsvFile(fixturePath, allowed);
    QVERIFY2(res.success, qPrintable(res.errors.join("; ")));
    QCOMPARE(res.pins.size(), 3);
    QCOMPARE(res.pins[0].pin, QString("PA0"));
    QCOMPARE(res.pins[0].peripheral, QString("GPIOA"));
    QCOMPARE(res.pins[1].pin, QString("PA9"));
    QCOMPARE(res.pins[1].peripheral, QString("USART1"));
}

void TestCustomPacks::testPinTableUnknownPeripheralRejected() {
    QString fixturePath = QDir::current().filePath("tests/fixtures/invalid_pins.csv");
    if (!QFile::exists(fixturePath)) {
        fixturePath = "../tests/fixtures/invalid_pins.csv";
    }
    QVERIFY2(QFile::exists(fixturePath), qPrintable(fixturePath));

    QSet<QString> allowed = {"GPIOA", "USART1"};
    PinTableParseResult res = PinTableParser::parseCsvFile(fixturePath, allowed);
    QVERIFY(!res.success);
    QVERIFY(!res.errors.isEmpty());
    QVERIFY(res.errors.first().contains("unknown peripheral 'UNKNOWN_PERIPHERAL_XYZ'"));
}

void TestCustomPacks::testSdkReferenceChecking() {
    // 1. Non-existent path
    SdkCheckResult res1 = SdkReferenceChecker::verifySdkDirectory("/non/existent/path");
    QVERIFY(!res1.valid);
    QVERIFY(res1.message.contains("does not exist"));

    // 2. Temp directory with required files
    QDir dir(m_tempDir.path());
    dir.mkpath("Drivers/CMSIS");
    QFile f(dir.filePath("Drivers/CMSIS/stm32g0xx.h"));
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("/* CMSIS header */");
    f.close();

    SdkCheckResult res2 = SdkReferenceChecker::verifySdkDirectory(dir.path(), {"Drivers/CMSIS/stm32g0xx.h"});
    QVERIFY(res2.valid);
    QVERIFY(res2.detected);
    QCOMPARE(res2.detectedSdkType, QString("STM32Cube"));
}

void TestCustomPacks::testCustomPackInstallation() {
    QString testPacksDir = m_tempDir.filePath("test_custom_packs");
    CustomPackManager::instance().setCustomPacksDirectory(testPacksDir);

    CustomPackDef pack;
    pack.name = "STM32G071RB";
    pack.vendor = "STMicroelectronics";
    pack.partNumber = "STM32G071RBT6";
    pack.family = "STM32G0";
    pack.core = "Cortex-M0+";
    pack.clockSpeedMhz = 64;

    CustomMemoryRegion flash;
    flash.name = "FLASH";
    flash.type = "flash";
    flash.startAddress = "0x08000000";
    flash.sizeKb = 128;
    pack.memoryRegions.append(flash);

    CustomMemoryRegion sram;
    sram.name = "SRAM";
    sram.type = "sram";
    sram.startAddress = "0x20000000";
    sram.sizeKb = 36;
    sram.usableForFramebuffer = 1;
    pack.memoryRegions.append(sram);

    CustomPeripheralDef gpioa;
    gpioa.name = "GPIOA";
    gpioa.baseAddress = "0x50000000";
    gpioa.type = "GPIO";
    CustomRegisterDef moder;
    moder.name = "MODER";
    moder.offset = "0x00";
    gpioa.registers.append(moder);
    pack.peripherals.append(gpioa);

    CustomPinDef pin;
    pin.pin = "PA5";
    pin.function = "GPIO_Output";
    pin.peripheral = "GPIOA";
    pack.pinTable.append(pin);

    pack.provenance.sourceFiles = {"stm32g071.svd"};
    pack.provenance.importerVersion = "1.0.0";
    pack.provenance.generatedBy = "importer";
    pack.provenance.verified = false;

    CapabilityDatabase capDb;
    QString err;
    bool ok = CustomPackManager::instance().installPack(pack, &capDb, &err);
    QVERIFY2(ok, qPrintable(err));

    // Verify file written to directory
    QVERIFY(QFile::exists(QDir(testPacksDir).filePath("STM32G071RB.json")));

    // Verify CapabilityDatabase has profile with verified: false
    DeviceProfile prof = capDb.getDeviceProfile("STM32G071RB");
    QCOMPARE(prof.name, QString("STM32G071RB"));
    QCOMPARE(prof.verified, false);
    QVERIFY(prof.sources.first().contains("imported pack"));
    QCOMPARE(prof.ram_kb, 36u);
    QCOMPARE(prof.flash_kb, 128u);

    // Verify pack registered in DriverPackRegistry
    IMcuDriverPack* registeredPack = DriverPackRegistry::instance().findPackForDevice("STM32G071RB");
    QVERIFY(registeredPack != nullptr);
}

void TestCustomPacks::testExportFromCustomPackCleanOutput() {
    QString outDir = m_tempDir.filePath("custom_export");
    QDir().mkpath(outDir);

    CanvasScene scene;
    Project project(&scene);
    project.setProjectName("CustomPackExport");
    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32G071RB";
    hw.family = "STM32G0";
    project.setHardwareConfig(hw);

    FirmwareExportManager exportMgr;
    ExportOptions opts;
    opts.outputDirectory = outDir;
    opts.targetFramework = "lvgl";
    opts.targetMcu = "STM32G071RB";

    ExportResult res = exportMgr.exportProject(&project, opts);
    QVERIFY2(res.success, qPrintable(res.errorMessage));

    // Verify generated/target/target_hal.h exists and includes the register header
    QFile halH(outDir + "/generated/target/target_hal.h");
    QVERIFY(halH.open(QIODevice::ReadOnly | QIODevice::Text));
    QString halHContent = QString::fromUtf8(halH.readAll());
    QVERIFY(halHContent.contains("stm32g071rb_registers.h"));

    // Verify generated/target/stm32g071rb_registers.h exists and has SVD base address
    QFile regH(outDir + "/generated/target/stm32g071rb_registers.h");
    QVERIFY(regH.open(QIODevice::ReadOnly | QIODevice::Text));
    QString regHContent = QString::fromUtf8(regH.readAll());
    QVERIFY(regHContent.contains("#define GPIOA_BASE (0x50000000UL)"));
    QVERIFY(regHContent.contains("#define GPIOA_MODER"));

    // Verify MANIFEST.json tracks the custom register file
    QFile mf(outDir + "/generated/MANIFEST.json");
    QVERIFY(mf.open(QIODevice::ReadOnly | QIODevice::Text));
    QString mfContent = QString::fromUtf8(mf.readAll());
    QVERIFY(mfContent.contains("stm32g071rb_registers.h"));
}

void TestCustomPacks::testExportUnsupportedDeviceUnchanged() {
    QString outDir = m_tempDir.filePath("unsupported_export");
    QDir().mkpath(outDir);

    CanvasScene scene;
    Project project(&scene);
    project.setProjectName("H745Export");
    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32H745";
    hw.family = "STM32H7";
    project.setHardwareConfig(hw);

    FirmwareExportManager exportMgr;
    ExportOptions opts;
    opts.outputDirectory = outDir;
    opts.targetFramework = "lvgl";
    opts.targetMcu = "STM32H745";

    ExportResult res = exportMgr.exportProject(&project, opts);
    QVERIFY2(res.success, qPrintable(res.errorMessage));

    // UI files MUST be exported unchanged
    QVERIFY(QFile::exists(outDir + "/generated/ui/ui.h"));
    QVERIFY(QFile::exists(outDir + "/generated/ui/ui.c"));

    // Native HAL is omitted and warned
    QVERIFY(!QFile::exists(outDir + "/generated/target/target_hal.c"));
    QVERIFY(!res.warnings.isEmpty());
}

QTEST_MAIN(TestCustomPacks)
#include "test_custom_packs.moc"
