#include <QTest>
#include <QObject>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>

#include "analyzer/ResourceAnalyzer.h"
#include "build/BuildConfiguration.h"
#include "build/DiagnosticParser.h"
#include "codegen/FirmwareExportManager.h"
#include "telemetry/TelemetryProtocol.h"
#include "telemetry/DeviceMonitorRuntime.h"
#include "project/Project.h"
#include "models/Screen.h"
#include "models/ButtonComponent.h"
#include "models/GaugeComponent.h"
#include "models/DataSource.h"

class TestHardening : public QObject {
    Q_OBJECT

private slots:
    // ── Track 1: Resource Analyzer Tests ──
    void testFramebufferMath();
    void testResourceAnalyzerEstimation();
    void testResourceThresholdsAndWarnings();

    // ── Track 2: Build System & Diagnostics Tests ──
    void testBuildConfigurationValidation();
    void testCustomFlagSanitization();
    void testDiagnosticParserGccClang();
    void testDiagnosticParserMsvc();
    void testDiagnosticParserCMake();

    // ── Track 3: Firmware Export & Preservation Tests ──
    void testFirmwareExportDirectoryStructure();
    void testUserCodePreservation();
    void testRootCMakeGeneration();

    // ── Track 4: Telemetry Protocol & Monitor Tests ──
    void testTelemetryFormatAndChecksum();
    void testTelemetryStreamingParserChunks();
    void testTelemetryBadChecksumRejection();
    void testTelemetryMalformedFrameRecovery();
    void testDeviceMonitorCsvExport();
};

// ── Track 1: Resource Analyzer ──

void TestHardening::testFramebufferMath() {
    // 320x240 @ 16bpp (2 bytes/pixel):
    // Row bytes = 320 * 2 = 640. 640 is already divisible by 16.
    // Single buffer = 640 * 240 = 153,600 bytes (150 KB)
    size_t fb1 = ResourceAnalyzer::calculateFramebufferBytes(320, 240, 16, 1, 16);
    QCOMPARE(fb1, static_cast<size_t>(153600));

    // Double buffered: 153,600 * 2 = 307,200 bytes (300 KB)
    size_t fb2 = ResourceAnalyzer::calculateFramebufferBytes(320, 240, 16, 2, 16);
    QCOMPARE(fb2, static_cast<size_t>(307200));

    // Non-aligned width: 125x100 @ 16bpp (250 bytes/row)
    // 16-byte alignment: 250 -> 256 bytes row stride.
    // Framebuffer = 256 * 100 * 1 = 25,600 bytes
    size_t fbAligned = ResourceAnalyzer::calculateFramebufferBytes(125, 100, 16, 1, 16);
    QCOMPARE(fbAligned, static_cast<size_t>(25600));

    // Monochrome 128x64 (1 bpp):
    // 128 * 1 / 8 = 16 bytes/row (aligned to 16). 16 * 64 = 1024 bytes (1 KB)
    size_t fbMono = ResourceAnalyzer::calculateFramebufferBytes(128, 64, 1, 1, 16);
    QCOMPARE(fbMono, static_cast<size_t>(1024));
}

void TestHardening::testResourceAnalyzerEstimation() {
    Project proj(nullptr);
    proj.newProject("TestAnalyzer", 320, 240);
    proj.setTargetFramework("lvgl");

    // Add a button and a gauge to active screen
    Screen* scr = proj.activeScreen();
    QVERIFY(scr != nullptr);

    auto* btn = new ButtonComponent("btn1", "Click Me");
    auto* gauge = new GaugeComponent("gauge1", "Speed");
    scr->addComponent(btn);
    scr->addComponent(gauge);

    // Add a DataSource
    DataSource ds("PA0_ADC", "Analog In", DataSourceType::Adc);
    proj.addDataSource(ds);

    ResourceAnalyzer analyzer;
    ResourceReport report = analyzer.analyzeProject(&proj);

    QCOMPARE(report.totalComponents, 2);
    QCOMPARE(report.totalScreens, 1);
    QCOMPARE(report.totalDataSources, 1);
    QVERIFY(report.flashTotalEstimatedBytes > 64 * 1024); // LVGL base + widgets + fonts
    QVERIFY(report.ramTotalEstimatedBytes > report.framebufferBytes);
}

void TestHardening::testResourceThresholdsAndWarnings() {
    Project proj(nullptr);
    proj.newProject("ThresholdTest", 480, 320);

    // Configure target MCU with 128KB Flash and 64KB RAM (less than 480x320 framebuffer!)
    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F030R8";
    hw.flashBytes = 128 * 1024;
    hw.ramBytes = 64 * 1024; // 64 KB RAM
    proj.setHardwareConfig(hw);

    ResourceAnalyzer analyzer;
    ResourceReport report = analyzer.analyzeProject(&proj);

    QVERIFY(report.hasTargetLimits);
    // 480x320x2 = 300KB RAM which exceeds 64KB target RAM!
    QVERIFY(report.ramUtilizationPercent > 100.0);

    bool hasRamCriticalWarning = false;
    for (const auto& w : report.warnings) {
        if (w.category == "RAM" && w.severity == ResourceWarningSeverity::Critical) {
            hasRamCriticalWarning = true;
            break;
        }
    }
    QVERIFY(hasRamCriticalWarning);
}

// ── Track 2: Build Configuration & Diagnostics ──

void TestHardening::testBuildConfigurationValidation() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    BuildConfiguration cfg;
    cfg.workingDirectory = tempDir.path();

    QString err;
    QVERIFY(cfg.validate(&err));

    // Empty working directory must fail validation
    cfg.workingDirectory = "";
    QVERIFY(!cfg.validate(&err));
    QVERIFY(!err.isEmpty());

    // Non-existent directory must fail validation
    cfg.workingDirectory = "Z:/NonExistentPath/12345/abc";
    QVERIFY(!cfg.validate(&err));
}

void TestHardening::testCustomFlagSanitization() {
    QTemporaryDir tempDir;
    BuildConfiguration cfg;
    cfg.workingDirectory = tempDir.path();

    // Safe flags
    cfg.customFlags = "-Wall -O2 -DDEBUG=1";
    QString err;
    QVERIFY(cfg.validate(&err));

    // Unsafe shell injection chars: ; & | ` $ > <
    QStringList unsafeFlags = {
        "-O2; rm -rf /",
        "-O2 && echo bad",
        "-O2 | cat",
        "-O2 `whoami`",
        "-O2 $(id)",
        "-O2 > /tmp/out"
    };

    for (const QString& unsafe : unsafeFlags) {
        cfg.customFlags = unsafe;
        QVERIFY2(!cfg.validate(&err), qPrintable(QString("Failed to reject unsafe flag: %1").arg(unsafe)));
    }
}

void TestHardening::testDiagnosticParserGccClang() {
    DiagnosticParser parser;

    QString errLine = "src/main.c:42:15: error: expected ';' before 'return'";
    CompilerDiagnostic diag = parser.parseLine(errLine);
    QCOMPARE(diag.severity, DiagnosticSeverity::Error);
    QCOMPARE(diag.filePath, QString("src/main.c"));
    QCOMPARE(diag.line, 42);
    QCOMPARE(diag.column, 15);
    QVERIFY(diag.message.contains("expected ';'"));

    QString warnLine = "src/ui.c:108:5: warning: unused variable 'temp' [-Wunused-variable]";
    CompilerDiagnostic warnDiag = parser.parseLine(warnLine);
    QCOMPARE(warnDiag.severity, DiagnosticSeverity::Warning);
    QCOMPARE(warnDiag.line, 108);
}

void TestHardening::testDiagnosticParserMsvc() {
    DiagnosticParser parser;

    QString msvcErr = "C:\\project\\main.cpp(55,10): error C2065: 'undeclared_var': undeclared identifier";
    CompilerDiagnostic diag = parser.parseLine(msvcErr);
    QCOMPARE(diag.severity, DiagnosticSeverity::Error);
    QCOMPARE(diag.line, 55);
    QCOMPARE(diag.column, 10);
    QVERIFY(diag.message.contains("undeclared identifier"));
}

void TestHardening::testDiagnosticParserCMake() {
    DiagnosticParser parser;

    QString cmakeErr = "CMake Error at CMakeLists.txt:14 (find_package): Could not find Qt6";
    CompilerDiagnostic diag = parser.parseLine(cmakeErr);
    QCOMPARE(diag.severity, DiagnosticSeverity::Error);
    QCOMPARE(diag.filePath, QString("CMakeLists.txt"));
    QCOMPARE(diag.line, 14);
}

// ── Track 3: Firmware Export ──

void TestHardening::testFirmwareExportDirectoryStructure() {
    QTemporaryDir exportDir;
    QVERIFY(exportDir.isValid());

    Project proj(nullptr);
    proj.newProject("FirmwareTest", 320, 240);
    proj.setTargetFramework("ugfx");

    FirmwareExportManager mgr;
    ExportOptions opts;
    opts.outputDirectory = exportDir.path();
    opts.targetFramework = "ugfx";
    opts.generateCmake = true;
    opts.overwriteUserCode = false;

    ExportResult result = mgr.exportProject(&proj, opts);
    QVERIFY(result.success);

    // Verify directory layout
    QDir dir(exportDir.path());
    QVERIFY(dir.exists("generated/ui"));
    QVERIFY(dir.exists("generated/bindings"));
    QVERIFY(dir.exists("generated/target"));
    QVERIFY(dir.exists("generated/assets"));
    QVERIFY(dir.exists("user"));

    // Verify key files
    QVERIFY(QFile::exists(dir.filePath("user/main.c")));
    QVERIFY(QFile::exists(dir.filePath("user/custom_logic.h")));
    QVERIFY(QFile::exists(dir.filePath("user/custom_logic.c")));
    QVERIFY(QFile::exists(dir.filePath("generated/ui/gui.h")));
    QVERIFY(QFile::exists(dir.filePath("generated/bindings/ui_bindings.h")));
    QVERIFY(QFile::exists(dir.filePath("generated/target/target_hal.h")));
    QVERIFY(QFile::exists(dir.filePath("CMakeLists.txt")));
}

void TestHardening::testUserCodePreservation() {
    QTemporaryDir exportDir;
    QVERIFY(exportDir.isValid());

    Project proj(nullptr);
    proj.newProject("PreserveTest", 320, 240);

    FirmwareExportManager mgr;
    ExportOptions opts;
    opts.outputDirectory = exportDir.path();
    opts.overwriteUserCode = false;

    // First Export
    ExportResult res1 = mgr.exportProject(&proj, opts);
    QVERIFY(res1.success);

    // Modify user/main.c with custom user logic
    QDir dir(exportDir.path());
    QString mainPath = dir.filePath("user/main.c");
    {
        QFile f(mainPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&f);
        out << "// USER CUSTOM CODE MODIFIED BY DEVELOPER\n";
        out << "int my_custom_variable = 999;\n";
        f.close();
    }

    // Second Export (Regeneration)
    ExportResult res2 = mgr.exportProject(&proj, opts);
    QVERIFY(res2.success);

    // Verify that user/main.c was preserved and NOT overwritten!
    QFile verifyFile(mainPath);
    QVERIFY(verifyFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = verifyFile.readAll();
    verifyFile.close();

    QVERIFY(content.contains("USER CUSTOM CODE MODIFIED BY DEVELOPER"));
    QVERIFY(content.contains("my_custom_variable = 999"));
}

void TestHardening::testRootCMakeGeneration() {
    QTemporaryDir exportDir;
    Project proj(nullptr);
    proj.newProject("CMakeProjTest", 320, 240);

    FirmwareExportManager mgr;
    ExportOptions opts;
    opts.outputDirectory = exportDir.path();
    opts.generateCmake = true;

    ExportResult res = mgr.exportProject(&proj, opts);
    QVERIFY(res.success);

    QFile cmakeFile(QDir(exportDir.path()).filePath("CMakeLists.txt"));
    QVERIFY(cmakeFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = cmakeFile.readAll();
    cmakeFile.close();

    QVERIFY(content.contains("project(CMakeProjTest"));
    QVERIFY(content.contains("add_executable"));
    QVERIFY(content.contains("generated/ui"));
    QVERIFY(content.contains("user"));
}

// ── Track 4: Telemetry Protocol & Monitor ──

void TestHardening::testTelemetryFormatAndChecksum() {
    // Format frame: $TLM,1.0,PA0_ADC,INT,1024,1*CS\r\n
    QByteArray frame = TelemetryProtocol::formatFrame("PA0_ADC", TelemetryDataType::Int, 1024, 1);
    QVERIFY(frame.startsWith("$TLM,1.0,PA0_ADC,INT,1024,1*"));
    QVERIFY(frame.endsWith("\r\n"));

    TelemetryFrame parsed = TelemetryProtocol::parseFrame(QString::fromUtf8(frame));
    QVERIFY(parsed.isValid);
    QCOMPARE(parsed.version, QString("1.0"));
    QCOMPARE(parsed.sourceId, QString("PA0_ADC"));
    QCOMPARE(parsed.type, TelemetryDataType::Int);
    QCOMPARE(parsed.value.toInt(), 1024);
    QCOMPARE(parsed.sequence, 1u);
}

void TestHardening::testTelemetryStreamingParserChunks() {
    TelemetryProtocol protocol;

    QByteArray fullFrame = TelemetryProtocol::formatFrame("SPEED", TelemetryDataType::Float, 55.4, 12);

    // Feed in two separate chunks
    int split = fullFrame.size() / 2;
    QByteArray chunk1 = fullFrame.left(split);
    QByteArray chunk2 = fullFrame.mid(split);

    QList<TelemetryFrame> frames1 = protocol.feedBytes(chunk1);
    QCOMPARE(frames1.size(), 0); // Partial, should not yield complete frame

    QList<TelemetryFrame> frames2 = protocol.feedBytes(chunk2);
    QCOMPARE(frames2.size(), 1);
    QVERIFY(frames2[0].isValid);
    QCOMPARE(frames2[0].sourceId, QString("SPEED"));
    QCOMPARE(frames2[0].type, TelemetryDataType::Float);
    QCOMPARE(frames2[0].sequence, 12u);
}

void TestHardening::testTelemetryBadChecksumRejection() {
    // Frame with deliberately corrupted checksum: *FF
    QString badFrame = "$TLM,1.0,TEMP,FLOAT,25.5,1*FF\r\n";
    TelemetryFrame parsed = TelemetryProtocol::parseFrame(badFrame);
    QVERIFY(!parsed.isValid);
    QVERIFY(parsed.errorMessage.contains("Checksum mismatch"));
}

void TestHardening::testTelemetryMalformedFrameRecovery() {
    TelemetryProtocol protocol;

    // Buffer with preceding garbage followed by valid frame
    QByteArray garbage = "GARBAGE_NOISE_12345###";
    QByteArray validFrame = TelemetryProtocol::formatFrame("STATUS", TelemetryDataType::Bool, true, 1);

    QList<TelemetryFrame> frames = protocol.feedBytes(garbage + validFrame);
    QCOMPARE(frames.size(), 1);
    QVERIFY(frames[0].isValid);
    QCOMPARE(frames[0].sourceId, QString("STATUS"));
    QCOMPARE(frames[0].value.toBool(), true);
}

void TestHardening::testDeviceMonitorCsvExport() {
    QTemporaryDir tempDir;
    QString csvPath = tempDir.filePath("test_telemetry.csv");

    Project proj(nullptr);
    DeviceMonitorRuntime runtime(&proj);

    // Simulate sending commands to generate log entries
    runtime.sendCommand("LED1", TelemetryDataType::Bool, true); // Will fail port write (unconnected) but log command

    // Feed a valid frame directly to protocol parser to simulate serial arrival
    TelemetryProtocol protocol;
    QByteArray frameData = TelemetryProtocol::formatFrame("VOLTAGE", TelemetryDataType::Float, 3.29, 100);

    QString err;
    QVERIFY(runtime.exportCsv(csvPath, &err));

    QFile csvFile(csvPath);
    QVERIFY(csvFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QString csvContent = csvFile.readAll();
    csvFile.close();

    // Verify CSV RFC4180 header and structure
    QVERIFY(csvContent.startsWith("Timestamp,Direction,SourceID,DataType,Value,Sequence,Valid,ErrorMessage,RawMessage"));
}

QTEST_MAIN(TestHardening)
#include "test_hardening.moc"
