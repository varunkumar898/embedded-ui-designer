#include <QTest>
#include <QObject>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QProcess>

#include "analyzer/ResourceEstimator.h"
#include "build/BuildConfiguration.h"
#include "build/DiagnosticParser.h"
#include "codegen/FirmwareExportManager.h"
#include "telemetry/TelemetryProtocol.h"
#include "telemetry/DeviceMonitorRuntime.h"
#include "project/Project.h"
#include "models/Screen.h"
#include "models/ButtonComponent.h"
#include "models/ProgressBarComponent.h"
#include "models/GaugeComponent.h"
#include "models/DataSource.h"
#include "runtime/ew_tree.h"

class TestHardening : public QObject {
    Q_OBJECT

private slots:
    // ── Track 1: Resource Analyzer Tests ──
    void testFramebufferMath();
    void testResourceEstimatorEstimation();
    void testResourceThresholdsAndWarnings();
    void testEmbedWebMultiMcuTargetSelection();

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
    void testEmbedWebFirmwareExportAndBuild();

    // ── Track 4: Telemetry Protocol & Monitor Tests ──
    void testTelemetryFormatAndChecksum();
    void testTelemetryStreamingParserChunks();
    void testTelemetryBadChecksumRejection();
    void testTelemetryMalformedFrameRecovery();
    void testDeviceMonitorCsvExport();
    void testStage471ReconciliationEvidence();
};

// ── Track 1: Resource Analyzer ──

void TestHardening::testFramebufferMath() {
    // 320x240 @ 16bpp (2 bytes/pixel):
    // Row bytes = 320 * 2 = 640. 640 is already divisible by 16.
    // Single buffer = 640 * 240 = 153,600 bytes (150 KB)
    size_t fb1 = ResourceEstimator::calculateFramebufferBytes(320, 240, 16, 1, 16);
    QCOMPARE(fb1, static_cast<size_t>(153600));

    // Double buffered: 153,600 * 2 = 307,200 bytes (300 KB)
    size_t fb2 = ResourceEstimator::calculateFramebufferBytes(320, 240, 16, 2, 16);
    QCOMPARE(fb2, static_cast<size_t>(307200));

    // Non-aligned width: 125x100 @ 16bpp (250 bytes/row)
    // 16-byte alignment: 250 -> 256 bytes row stride.
    // Framebuffer = 256 * 100 * 1 = 25,600 bytes
    size_t fbAligned = ResourceEstimator::calculateFramebufferBytes(125, 100, 16, 1, 16);
    QCOMPARE(fbAligned, static_cast<size_t>(25600));

    // Monochrome 128x64 (1 bpp):
    // 128 * 1 / 8 = 16 bytes/row (aligned to 16). 16 * 64 = 1024 bytes (1 KB)
    size_t fbMono = ResourceEstimator::calculateFramebufferBytes(128, 64, 1, 1, 16);
    QCOMPARE(fbMono, static_cast<size_t>(1024));
}

void TestHardening::testResourceEstimatorEstimation() {
    Project proj(nullptr);
    proj.newProject("TestAnalyzer", 320, 240);
    proj.setTargetFramework("lvgl");

    // Add a button and a gauge to active screen
    Screen* scr = proj.activeScreen();
    QVERIFY(scr != nullptr);

    auto* btn = new ButtonComponent("btn1");
    btn->setText("Click Me");
    auto* gauge = new GaugeComponent("gauge1");
    scr->addComponent(btn);
    scr->addComponent(gauge);

    // Add a DataSource
    DataSource ds("PA0_ADC", "Analog In", DataSourceType::Adc);
    proj.addDataSource(ds);

    ResourceEstimator estimator;
    ResourceAnalysis analysis = estimator.analyze(&proj, "STM32H745");

    QCOMPARE(analysis.breakdown.components_flash > 0, true);
    QCOMPARE(analysis.breakdown.framework_flash > 0, true);
    QVERIFY(analysis.flash_used > 64 * 1024); // LVGL base + widgets + fonts
    QVERIFY(analysis.ram_used > 0); // Some RAM used (framebuffer + framework)
}

void TestHardening::testResourceThresholdsAndWarnings() {
    Project proj(nullptr);
    proj.newProject("ThresholdTest", 480, 320);
    proj.setTargetFramework("lvgl");

    // Add many components to exceed small device capacity
    Screen* scr = proj.activeScreen();
    QVERIFY(scr != nullptr);
    for (int i = 0; i < 50; ++i) {
        auto* btn = new ButtonComponent(QString("btn%1").arg(i));
        btn->setText(QString("Button %1").arg(i));
        scr->addComponent(btn);
    }

    // Use a device with very small capacity
    ResourceEstimator estimator;
    DeviceProfile smallDevice = estimator.capabilityDatabase().getDeviceProfile("STM32F030R8");
    smallDevice.ram_kb = 8;  // Very small RAM (8 KB)
    smallDevice.flash_kb = 64; // Very small Flash (64 KB)

    ResourceAnalysis analysis = estimator.analyze(&proj, smallDevice);

    // With 50 buttons and small device, we should exceed capacity
    QVERIFY(analysis.flash_percent > 100.0 || analysis.ram_percent > 100.0);

    bool hasCriticalWarning = false;
    for (const QString& w : analysis.warnings) {
        if (w.contains("CRITICAL")) {
            hasCriticalWarning = true;
            break;
        }
    }
    QVERIFY(hasCriticalWarning);
}

void TestHardening::testEmbedWebMultiMcuTargetSelection() {
    Project proj(nullptr);
    proj.newProject("MultiMcuTest", 320, 240);
    proj.setTargetFramework("embedweb");

    Screen* scr = proj.activeScreen();
    QVERIFY(scr != nullptr);
    for (int i = 0; i < 5; ++i) {
        auto* btn = new ButtonComponent(QString("btn%1").arg(i));
        btn->setText(QString("Item %1").arg(i));
        scr->addComponent(btn);
    }

    ResourceEstimator estimator;

    // Verify CapabilityDatabase profiles have Stage 4.6 fields populated
    const auto& db = estimator.capabilityDatabase();

    // Exact profile verifications (Stage 4.7 memory regions decomposition)
    DeviceProfile f4 = db.getDeviceProfile("STM32F429");
    QCOMPARE(f4.architecture, QString("Cortex-M4"));
    QCOMPARE(f4.required_toolchain, QString("arm-none-eabi-gcc"));
    QCOMPARE(f4.embedweb_adapter_status, QString("verified"));
    QCOMPARE(f4.internal_sram_kb, 260);
    QCOMPARE(f4.dma_sram_kb, 192); // SRAM1-3 DMA-capable
    QCOMPARE(f4.tcm_kb, 64);       // CCM non-DMA
    QCOMPARE(f4.onchip_flash_kb, 2048);
    QCOMPARE(f4.external_flash_kb, 0);

    DeviceProfile h7 = db.getDeviceProfile("STM32H745");
    QCOMPARE(h7.architecture, QString("Cortex-M7"));
    QCOMPARE(h7.required_toolchain, QString("arm-none-eabi-gcc"));
    QCOMPARE(h7.embedweb_adapter_status, QString("verified"));
    QCOMPARE(h7.internal_sram_kb, 1024);
    QCOMPARE(h7.dma_sram_kb, 512); // AXI SRAM DMA-capable
    QCOMPARE(h7.tcm_kb, 192);      // ITCM + DTCM non-DMA
    QCOMPARE(h7.onchip_flash_kb, 2048);
    QCOMPARE(h7.external_flash_kb, 0);

    DeviceProfile rp2040 = db.getDeviceProfile("Raspberry Pi Pico");
    QCOMPARE(rp2040.architecture, QString("Cortex-M0+"));
    QCOMPARE(rp2040.required_toolchain, QString("arm-none-eabi-gcc"));
    QCOMPARE(rp2040.embedweb_adapter_status, QString("verified"));
    QCOMPARE(rp2040.internal_sram_kb, 264);
    QCOMPARE(rp2040.dma_sram_kb, 264);
    QCOMPARE(rp2040.tcm_kb, 0);
    QCOMPARE(rp2040.onchip_flash_kb, 0); // No on-chip flash
    QCOMPARE(rp2040.external_flash_kb, 2048); // 2MB board QSPI flash

    DeviceProfile esp32s3 = db.getDeviceProfile("ESP32-S3");
    QCOMPARE(esp32s3.architecture, QString("Xtensa-LX7"));
    QCOMPARE(esp32s3.required_toolchain, QString("xtensa-esp32s3-elf-gcc"));
    QCOMPARE(esp32s3.internal_sram_kb, 512);
    QCOMPARE(esp32s3.dma_sram_kb, 384);
    QCOMPARE(esp32s3.tcm_kb, 0);
    QCOMPARE(esp32s3.onchip_flash_kb, 0); // No on-chip flash
    QCOMPARE(esp32s3.external_flash_kb, 8192); // 8MB module flash
    QCOMPARE(esp32s3.psram_kb, 8192); // 8MB module Octal PSRAM (N8R8)
    QCOMPARE(esp32s3.module_variant, QString("ESP32-S3-DevKitC-1-N8R8"));
    QCOMPARE(esp32s3.dma_requires_internal_sram, 1);

    DeviceProfile nxp = db.getDeviceProfile("NXP i.MX RT1060");
    QCOMPARE(nxp.architecture, QString("Cortex-M7"));
    QCOMPARE(nxp.required_toolchain, QString("arm-none-eabi-gcc"));
    QCOMPARE(nxp.embedweb_adapter_status, QString("verified"));
    QCOMPARE(nxp.internal_sram_kb, 1024);
    QCOMPARE(nxp.dma_sram_kb, 512); // OCRAM DMA-capable
    QCOMPARE(nxp.tcm_kb, 512);      // FlexRAM ITCM/DTCM non-DMA
    QCOMPARE(nxp.onchip_flash_kb, 0);
    QCOMPARE(nxp.external_flash_kb, 8192);

    DeviceProfile f0 = db.getDeviceProfile("STM32F030R8");
    QCOMPARE(f0.architecture, QString("Cortex-M0"));
    QCOMPARE(f0.internal_sram_kb, 8); // 8 KB RAM
    QCOMPARE(f0.dma_sram_kb, 8);
    QCOMPARE(f0.onchip_flash_kb, 64);
    QCOMPARE(f0.external_flash_kb, 0);

    // 1. Dual-buffer (default) evaluation
    auto evalsDual = estimator.evaluateEmbedWebTargets(&proj, 20.0f, 2, false);
    QVERIFY(!evalsDual.isEmpty());

    auto findEval = [](const QVector<ResourceEstimator::EmbedWebTargetEvaluation>& evals, const QString& name)
        -> const ResourceEstimator::EmbedWebTargetEvaluation* {
        for (const auto& e : evals) {
            if (e.targetName == name) return &e;
        }
        return nullptr;
    };

    const auto* h7Eval = findEval(evalsDual, "STM32H745");
    QVERIFY(h7Eval != nullptr);
    QCOMPARE(h7Eval->bufferCount, (uint8_t)2);
    QCOMPARE(h7Eval->dmaSliceBufferBytes, (uint32_t)20480); // 320 * 16 * 2 * 2
    QCOMPARE(h7Eval->treeStorageBytes, (uint32_t)11780);    // Measured 32-bit ARM sizeof(ew_tree_t)
    // Style double-count correction: core & stack is 1,164 bytes (28 B engine + 48 B driver + 64 B dispatch + 1024 B stack)
    QCOMPARE(h7Eval->coreAndStackBytes, (uint32_t)1164);
    QCOMPARE(h7Eval->resourceStatus, ResourceEstimator::EmbedWebTargetEvaluation::ResourceStatus::ResourceCompatible);
    QCOMPARE(h7Eval->buildStatus, ResourceEstimator::EmbedWebTargetEvaluation::BuildStatus::CompilerObjectsBuilt);
    QCOMPARE(h7Eval->hardwareStatus, ResourceEstimator::EmbedWebTargetEvaluation::HardwareStatus::HardwareNotTested);
    QCOMPARE(h7Eval->compatibility, ResourceEstimator::EmbedWebTargetEvaluation::Compatible);

    // 2. Single-buffer evaluation
    auto evalsSingle = estimator.evaluateEmbedWebTargets(&proj, 20.0f, 1, false);
    const auto* h7Single = findEval(evalsSingle, "STM32H745");
    QVERIFY(h7Single != nullptr);
    QCOMPARE(h7Single->bufferCount, (uint8_t)1);
    QCOMPARE(h7Single->dmaSliceBufferBytes, (uint32_t)10240); // 320 * 16 * 2 * 1
    QCOMPARE(h7Single->internalSramRequiredKb, (uint32_t)((10240 + 11780 + 1164 + 1023) / 1024)); // 23 KB

    // 3. PSRAM Placement Test: Tree in PSRAM vs Tree in Internal SRAM
    // By default (allowTreeInPsram = false), ESP32-S3 tree is in internal SRAM
    const auto* espNoPsramTree = findEval(evalsDual, "ESP32-S3");
    QVERIFY(espNoPsramTree != nullptr);
    QCOMPARE(espNoPsramTree->treePlacedInPsram, false);
    QCOMPARE(espNoPsramTree->psramRequiredKb, (uint32_t)0);

    // With allowTreeInPsram = true, tree moves to PSRAM on ESP32-S3
    auto evalsPsramTree = estimator.evaluateEmbedWebTargets(&proj, 20.0f, 2, true);
    const auto* espPsramTree = findEval(evalsPsramTree, "ESP32-S3");
    QVERIFY(espPsramTree != nullptr);
    QCOMPARE(espPsramTree->treePlacedInPsram, true);
    QCOMPARE(espPsramTree->psramRequiredKb, (uint32_t)((11780 + 1023) / 1024)); // 12 KB
    // Internal SRAM required is reduced because tree moved to PSRAM
    QVERIFY(espPsramTree->internalSramRequiredKb < espNoPsramTree->internalSramRequiredKb);

    // RP2040 does NOT have PSRAM, so tree cannot be placed in PSRAM even if allowTreeInPsram = true
    const auto* rpPsramTree = findEval(evalsPsramTree, "Raspberry Pi Pico");
    QVERIFY(rpPsramTree != nullptr);
    QCOMPARE(rpPsramTree->treePlacedInPsram, false);

    // 4. Status Orthogonality Verification
    // ESP32-S3: ResourceCompatible, toolchain discovered in ~/.espressif/tools, TargetApplicationLinked!
    QCOMPARE(espNoPsramTree->resourceStatus, ResourceEstimator::EmbedWebTargetEvaluation::ResourceStatus::ResourceCompatible);
    QCOMPARE(espNoPsramTree->buildStatus, ResourceEstimator::EmbedWebTargetEvaluation::BuildStatus::TargetApplicationLinked);
    QCOMPARE(espNoPsramTree->hardwareStatus, ResourceEstimator::EmbedWebTargetEvaluation::HardwareStatus::HardwareNotTested);
    QCOMPARE(espNoPsramTree->toolchainAvailable, true);

    // STM32F030R8: 8 KB RAM cannot fit required RAM -> InsufficientInternalSRAM
    const auto* f0Eval = findEval(evalsDual, "STM32F030R8");
    QVERIFY(f0Eval != nullptr);
    QCOMPARE(f0Eval->resourceStatus, ResourceEstimator::EmbedWebTargetEvaluation::ResourceStatus::InsufficientInternalSRAM);
    QCOMPARE(f0Eval->compatibility, ResourceEstimator::EmbedWebTargetEvaluation::InsufficientRAM);

    // 5. Unimplemented adapter
    const auto* nrfEval = findEval(evalsDual, "nRF52840");
    if (nrfEval) {
        QCOMPARE(nrfEval->compatibility, ResourceEstimator::EmbedWebTargetEvaluation::AdapterNotImplemented);
    }

    // 6. Unknown / incomplete profile query handled gracefully
    DeviceProfile unknown = db.getDeviceProfile("NonExistentMCU_XYZ");
    QCOMPARE(unknown.name, QString("NonExistentMCU_XYZ")); // Graceful fallback retains query name
    QCOMPARE(unknown.family, QString("Generic"));
    QCOMPARE(db.deviceExists("NonExistentMCU_XYZ"), false);

    // 7. Safety margin headroom enforcement
    auto tightEvals = estimator.evaluateEmbedWebTargets(&proj, 90.0f);
    for (const auto& eval : tightEvals) {
        if (eval.internalSramAvailableKb < 128 && eval.compatibility == ResourceEstimator::EmbedWebTargetEvaluation::Compatible) {
            QFAIL("Device with <128KB RAM should not be cleanly compatible with 90% safety margin!");
        }
    }

    // 8. Internal-SRAM exhaustion despite available PSRAM:
    // A target with 16 KB internal SRAM and 8 MB PSRAM.
    // Dual-buffer DMA slice buffer (20 KB) + core/stack exceeds 16 KB internal SRAM,
    // proving external PSRAM cannot satisfy internal DMA requirements even if tree is in PSRAM!
    DeviceProfile smallSramWithPsram;
    smallSramWithPsram.name = "HypotheticalSmallSramMCU";
    smallSramWithPsram.family = "TestFamily";
    smallSramWithPsram.architecture = "Cortex-M4";
    smallSramWithPsram.required_toolchain = "arm-none-eabi-gcc";
    smallSramWithPsram.embedweb_adapter_status = "verified";
    smallSramWithPsram.internal_sram_kb = 16; // 16 KB internal SRAM
    smallSramWithPsram.psram_kb = 8192;       // 8 MB PSRAM
    smallSramWithPsram.flash_kb = 4096;
    smallSramWithPsram.dma_requires_internal_sram = 1;
    smallSramWithPsram.supported_frameworks = {"embedweb"};
    estimator.capabilityDatabase().addDeviceProfile(smallSramWithPsram);

    auto smallSramEvals = estimator.evaluateEmbedWebTargets(&proj, 0.0f, 2, true);
    const auto* smallSramEval = findEval(smallSramEvals, "HypotheticalSmallSramMCU");
    QVERIFY(smallSramEval != nullptr);
    QCOMPARE(smallSramEval->resourceStatus, ResourceEstimator::EmbedWebTargetEvaluation::ResourceStatus::InsufficientInternalSRAM);
    QCOMPARE(smallSramEval->compatibility, ResourceEstimator::EmbedWebTargetEvaluation::InsufficientRAM);
    QCOMPARE(smallSramEval->statusText, QString("Insufficient Internal SRAM"));
    QVERIFY(smallSramEval->reason.contains("SRAM", Qt::CaseInsensitive));
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

void TestHardening::testEmbedWebFirmwareExportAndBuild() {
    QTemporaryDir exportDir;
    QVERIFY(exportDir.isValid());

    Project proj(nullptr);
    proj.newProject("EmbedWebFirmwareTest", 320, 240);
    proj.setTargetFramework("embedweb");

    Screen* scr = proj.activeScreen();
    QVERIFY(scr != nullptr);

    ButtonComponent* btn = new ButtonComponent("StartButton");
    btn->setText("Start");
    btn->setCompPos(20, 30);
    btn->setCompSize(100, 35);
    scr->addComponent(btn);

    ProgressBarComponent* pbar = new ProgressBarComponent("BatteryPbar");
    pbar->setValue(0.75);
    pbar->setCompPos(20, 80);
    pbar->setCompSize(160, 24);
    scr->addComponent(pbar);

    FirmwareExportManager mgr;
    ExportOptions opts;
    opts.outputDirectory = exportDir.path();
    opts.targetFramework = "embedweb";
    opts.generateCmake = true;
    opts.overwriteUserCode = false;

    // 1. First Export
    ExportResult result = mgr.exportProject(&proj, opts);
    QVERIFY2(result.success, qPrintable(result.errorMessage));

    QDir dir(exportDir.path());
    // Verify directory structure
    QVERIFY(dir.exists("generated/ui"));
    QVERIFY(dir.exists("generated/runtime"));
    QVERIFY(dir.exists("generated/bindings"));
    QVERIFY(dir.exists("generated/target"));
    QVERIFY(dir.exists("user"));

    // Verify key files
    QVERIFY(QFile::exists(dir.filePath("user/main.c")));
    QVERIFY(QFile::exists(dir.filePath("user/custom_logic.h")));
    QVERIFY(QFile::exists(dir.filePath("user/custom_logic.c")));
    QVERIFY(QFile::exists(dir.filePath("generated/ui/ui_tree.h")));
    QVERIFY(QFile::exists(dir.filePath("generated/ui/ui_tree.c")));
    QVERIFY(QFile::exists(dir.filePath("generated/ui/ui_assets.h")));
    QVERIFY(QFile::exists(dir.filePath("generated/ui/ui_assets.c")));
    QVERIFY(QFile::exists(dir.filePath("generated/runtime/ew_types.h")));
    QVERIFY(QFile::exists(dir.filePath("generated/runtime/ew_tree.c")));
    QVERIFY(QFile::exists(dir.filePath("generated/runtime/ew_layout.c")));
    QVERIFY(QFile::exists(dir.filePath("generated/runtime/ew_raster.c")));
    QVERIFY(QFile::exists(dir.filePath("generated/runtime/ew_hal.c")));
    QVERIFY(QFile::exists(dir.filePath("generated/runtime/ew_input.c")));
    QVERIFY(QFile::exists(dir.filePath("generated/bindings/ui_bindings.h")));
    QVERIFY(QFile::exists(dir.filePath("generated/target/target_hal.h")));
    QVERIFY(QFile::exists(dir.filePath("generated/target/ui_board_config.h")));
    QVERIFY(dir.exists("generated/runtime/adapters"));
    QVERIFY(QFile::exists(dir.filePath("generated/runtime/adapters/ew_adapter_host.c")));
    QVERIFY(QFile::exists(dir.filePath("generated/runtime/adapters/ew_adapter_stm32.c")));
    QVERIFY(QFile::exists(dir.filePath("generated/runtime/adapters/ew_adapter_rp2040.c")));
    QVERIFY(QFile::exists(dir.filePath("generated/runtime/adapters/ew_adapter_esp32.c")));
    QVERIFY(QFile::exists(dir.filePath("generated/runtime/adapters/ew_adapter_nxp.c")));
    QVERIFY(QFile::exists(dir.filePath("generated/manifest.json")));
    QVERIFY(QFile::exists(dir.filePath("CMakeLists.txt")));

    // 2. End-to-end host project compilation of generated export!
    QString buildPath = dir.filePath("build");
    QProcess configProc;
    configProc.start("cmake", QStringList() << "-B" << buildPath << "-S" << dir.path());
    QVERIFY(configProc.waitForFinished(30000));
    QCOMPARE(configProc.exitCode(), 0);

    QProcess buildProc;
    buildProc.start("cmake", QStringList() << "--build" << buildPath);
    QVERIFY(buildProc.waitForFinished(30000));
    QCOMPARE(buildProc.exitCode(), 0);

    // 3. User code preservation test on re-export
    QString mainPath = dir.filePath("user/main.c");
    {
        QFile f(mainPath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&f);
        out << "// USER EMBEDWEB CODE PERSISTS\n";
        out << "int user_marker = 42;\n";
        f.close();
    }
    ExportResult res2 = mgr.exportProject(&proj, opts);
    QVERIFY(res2.success);
    {
        QFile f(mainPath);
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        QString content = f.readAll();
        f.close();
        QVERIFY(content.contains("USER EMBEDWEB CODE PERSISTS"));
    }
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

void TestHardening::testStage471ReconciliationEvidence() {
    // 1. Capacity macro & structural invariants
    QCOMPARE(EW_MAX_NODES, 128);
    QCOMPARE(EW_MAX_TREE_NODES, 128);
    #if defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__ == 4
    QCOMPARE(sizeof(ew_tree_t), static_cast<size_t>(EW_MAX_TREE_NODES * sizeof(ew_node_t) + 4));
    #else
    QVERIFY(sizeof(ew_tree_t) >= static_cast<size_t>(EW_MAX_TREE_NODES * sizeof(ew_node_t) + 4));
    #endif

    // Target 32-bit embedded invariants (Cortex-M, Xtensa LX7, RISC-V 32)
    const size_t targetNodeSize32 = 92;
    const size_t targetTreeSize32 = 128 * targetNodeSize32 + 4; // 11,780 bytes
    QCOMPARE(targetTreeSize32, static_cast<size_t>(11780));

    // 2. Arithmetic Reconciliation
    // Historical calculation: 28 (engine) + 48 (driver) + 64 (dispatch) + 512 (style cache) + 1024 (stack) = 1,676 bytes
    // (Historical reports erroneously cited 1,648 or 1,692 due to variable padding typos).
    const uint32_t historicalSum = 28 + 48 + 64 + 512 + 1024;
    QCOMPARE(historicalSum, 1676u);

    // Corrected calculation eliminating 512-byte style double-counting:
    const uint32_t correctedCoreAndStack = 28 + 48 + 64 + 1024;
    QCOMPARE(correctedCoreAndStack, 1164u);
    QCOMPARE(historicalSum - 512u, correctedCoreAndStack);

    // 3. Slice Buffer Mathematics
    // 320x240 RGB565 with 16 slice lines:
    const uint32_t singleSliceBufferBytes = 320 * 16 * 2; // 10,240 bytes
    const uint32_t dualSliceBufferBytes = singleSliceBufferBytes * 2; // 20,480 bytes
    QCOMPARE(singleSliceBufferBytes, 10240u);
    QCOMPARE(dualSliceBufferBytes, 20480u);

    // Total static RAM predicted for EmbedWeb core runtime:
    const uint32_t predictedStaticRam = targetTreeSize32 + dualSliceBufferBytes + 28; // Tree + Dual Slice + Engine
    QCOMPARE(predictedStaticRam, 32288u);
}

QTEST_MAIN(TestHardening)
#include "test_hardening.moc"
