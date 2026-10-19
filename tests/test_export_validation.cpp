#include <QTest>
#include <QObject>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QPushButton>
#include <QProgressBar>
#include <QListWidget>
#include <QLabel>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QStandardPaths>

#include "analyzer/ResourceAnalysis.h"
#include "analyzer/ResourceEstimator.h"
#include "analyzer/CapabilityDatabase.h"
#include "analyzer/EstimationFormulas.h"
#include "hardware/DeviceDatabase.h"
#include "hardware/BoardRegistry.h"
#include "hardware/HardwareModel.h"
#include "project/Project.h"
#include "canvas/CanvasScene.h"
#include "models/ButtonComponent.h"
#include "models/LabelComponent.h"
#include "models/SliderComponent.h"
#include "models/ImageComponent.h"
#include "codegen/FirmwareExportManager.h"
#include "codegen/LvglGenerator.h"
#include "codegen/UgfxGenerator.h"
#include "codegen/QtMcuGenerator.h"
#include "codegen/TargetHalGenerator.h"
#include "driverpack/DriverPackRegistry.h"
#include "dialogs/ExportDialog.h"
#include "diagnostics/DiagnosticsService.h"
#include "diagnostics/DiagnosticsSources.h"

namespace {

// ---- Phase C1 (plan-driven HAL) test helpers -------------------------------

Hardware::PinConfiguration makePinConfig(const QString& pin, const QString& mode,
                                         const QString& alternateFunction = QString()) {
    Hardware::PinConfiguration p;
    p.pin = pin;
    p.gpio = pin;
    p.mode = mode;
    p.alternateFunction = alternateFunction;
    p.pull = mode == "AlternateFunction" ? "PullUp" : "NoPull";
    p.speed = mode == "AlternateFunction" ? "VeryHigh" : "Medium";
    p.outputType = "PushPull";
    p.initialOutput = "Low";
    return p;
}

Hardware::PeripheralConfiguration makePeripheralConfig(const QString& name, const QString& type,
                                                       const QMap<QString, QString>& pins,
                                                       const QMap<QString, QVariant>& params) {
    Hardware::PeripheralConfiguration per;
    per.name = name;
    per.type = type;
    per.enabled = true;
    per.assignedPins = pins;
    per.parameters = params;
    return per;
}

void setFullClock(Hardware::HardwareConfig& hw, int mhz) {
    hw.clock.sysClockMhz = mhz;
    hw.clock.hclkMhz = mhz;
    hw.clock.apb1Mhz = mhz;
    hw.clock.apb2Mhz = mhz;
    hw.clock.oscSource = "Internal";
}

// Probe A: PA5 output (high) + USART1 TX/RX on PA9/PA10 @ 230400.
Hardware::HardwareConfig probeAConfig() {
    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F030R8";
    hw.family = "STM32";
    setFullClock(hw, 48);

    Hardware::PinConfiguration led = makePinConfig("PA5", "GPIO_Output");
    led.initialOutput = "High";
    hw.pins.insert("PA5", led);
    hw.pins.insert("PA9", makePinConfig("PA9", "AlternateFunction", "USART1_TX"));
    hw.pins.insert("PA10", makePinConfig("PA10", "AlternateFunction", "USART1_RX"));
    hw.peripherals.insert("USART1", makePeripheralConfig(
        "USART1", "USART",
        QMap<QString, QString>{{"TX", "PA9"}, {"RX", "PA10"}},
        QMap<QString, QVariant>{{"baudRate", 230400}}));
    return hw;
}

// Probe B: PC13 output (low) + USART1 on PB6/PB7 @ 9600 + I2C1 on PB10/PB11
// (an invalid placement on this device: I2C1 lives on PB6-PB9).
Hardware::HardwareConfig probeBConfig() {
    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F030R8";
    hw.family = "STM32";
    setFullClock(hw, 48);

    Hardware::PinConfiguration led = makePinConfig("PC13", "GPIO_Output");
    led.initialOutput = "Low";
    led.speed = "Low";
    hw.pins.insert("PC13", led);
    hw.pins.insert("PB6", makePinConfig("PB6", "AlternateFunction", "USART1_TX"));
    hw.pins.insert("PB7", makePinConfig("PB7", "AlternateFunction", "USART1_RX"));
    Hardware::PinConfiguration scl = makePinConfig("PB10", "AlternateFunction", "I2C1_SCL");
    scl.pull = "PullUp";
    scl.speed = "High";
    scl.outputType = "OpenDrain";
    hw.pins.insert("PB10", scl);
    Hardware::PinConfiguration sda = makePinConfig("PB11", "AlternateFunction", "I2C1_SDA");
    sda.pull = "PullUp";
    sda.speed = "High";
    sda.outputType = "OpenDrain";
    hw.pins.insert("PB11", sda);

    hw.peripherals.insert("USART1", makePeripheralConfig(
        "USART1", "USART",
        QMap<QString, QString>{{"TX", "PB6"}, {"RX", "PB7"}},
        QMap<QString, QVariant>{{"baudRate", 9600}}));
    hw.peripherals.insert("I2C1", makePeripheralConfig(
        "I2C1", "I2C",
        QMap<QString, QString>{{"SCL", "PB10"}, {"SDA", "PB11"}},
        QMap<QString, QVariant>{{"clockSpeedHz", 400000}}));
    return hw;
}

bool exportConfigTo(FirmwareExportManager& mgr, CanvasScene& scene, Project& project,
                    const Hardware::HardwareConfig& hw, const QString& dir, ExportResult* out) {
    project.setHardwareConfig(hw);
    ExportOptions opts;
    opts.outputDirectory = dir;
    opts.targetFramework = "lvgl";
    opts.targetMcu = "STM32F030R8";
    opts.forceExport = true;
    *out = mgr.exportProject(&project, opts);
    return out->success;
}

QString readTextFile(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return QString();
    return QString::fromUtf8(f.readAll());
}

QString sha256OfFile(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return QString();
    return QString::fromLatin1(
        QCryptographicHash::hash(f.readAll(), QCryptographicHash::Sha256).toHex());
}

bool warningsContain(const QStringList& warnings, const QString& needleA,
                     const QString& needleB = QString()) {
    for (const QString& w : warnings) {
        if (w.contains(needleA) && (needleB.isEmpty() || w.contains(needleB))) return true;
    }
    return false;
}

} // namespace

class TestExportValidation : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testDeviceProfileResolutionValid();
    void testDeviceProfileResolutionFallbackEmpty();
    void testResourceAnalysisNoDeviceSelected();
    void testResourceAnalysisCriticalBlocksExport();
    void testResourceAnalysisWarningAllowsExport();
    void testResourceAnalysisOkPasses();
    void testFrameworkSwitchingBaseline();
    void testAnalysisPerformanceCachingUnder100ms();
    void testOptimizeHeaviestComponents();
    void testRegressionGeneratorOutputUnchanged();
    void testEstimationBaselinesDefaultsAuthoritative();
    void testEmptyFolderExportForAllDatabaseDevices();
    void testGoldenF030OutputUnchanged();
    void testManifestDeterministic();
    void testOrphanRemoval();
    void testEditDetectionWithBackup();
    void testUserFilesUntouched();
    void testUiExportWorksForUnsupportedDeviceWithoutHal();
    void testManifestSafetyRejectsPathTraversalAndExternalSymlinks();
    void testCompileCheckArmGccSyntax();

    // Phase C1: plan-driven HAL generation (configured STM32F030R8 projects)
    void testPlanHalGoldenEmptyConfigHashes();
    void testPlanHalProbeAMatchesConfig();
    void testPlanHalProbeBMatchesConfigAndWarnsI2c();
    void testPlanHalProbeAAndBDiffer();
    void testPlanHalValidSpiI2cUartAndCompileCheck();
    void testPlanHalConflictedPinAndPeripheralSkippedWithErrors();
    void testPlanHalUnknownClockAssumptionInfo();
    void testPlanHalNoInterruptCode();
};

void TestExportValidation::initTestCase() {
    Hardware::DeviceDatabase::instance().initialize();
    BoardRegistry::instance().loadFromBoardsJson();
}

void TestExportValidation::testDeviceProfileResolutionValid() {
    CanvasScene scene;
    Project project(&scene);

    // Test with standard STM32 part number
    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F407VG";
    project.setHardwareConfig(hw);

    DeviceProfile prof = project.getDeviceProfile();
    QVERIFY(!prof.name.isEmpty());
    QCOMPARE(prof.name, QString("STM32F407VG"));
    QVERIFY(prof.flash_kb > 0);
    QVERIFY(prof.ram_kb > 0);

    // Test with Nucleo board ID
    Hardware::HardwareConfig hwBoard;
    hwBoard.boardId = "nucleo_f030r8";
    project.setHardwareConfig(hwBoard);

    DeviceProfile profBoard = project.getDeviceProfile();
    QVERIFY(!profBoard.name.isEmpty());
    QVERIFY(profBoard.flash_kb > 0);
    QVERIFY(profBoard.ram_kb > 0);
}

void TestExportValidation::testDeviceProfileResolutionFallbackEmpty() {
    CanvasScene scene;
    Project project(&scene);

    // Unknown device/board ID
    Hardware::HardwareConfig hw;
    hw.deviceId = "NonExistentMcu_XYZ999";
    hw.boardId = "non_existent_board_abc";
    project.setHardwareConfig(hw);

    DeviceProfile prof = project.getDeviceProfile();
    // Must return an empty/invalid profile, NOT a made-up default MCU
    QVERIFY(prof.name.isEmpty());
}

void TestExportValidation::testResourceAnalysisNoDeviceSelected() {
    CanvasScene scene;
    Project project(&scene);

    Hardware::HardwareConfig hw;
    hw.deviceId = "";
    hw.boardId = "";
    project.setHardwareConfig(hw);

    ResourceEstimator estimator;
    ResourceAnalysis analysis = estimator.analyzeProject(&project, "lvgl");

    QCOMPARE(analysis.has_device, false);
    QCOMPARE(analysis.status, ResourceAnalysis::OK);
    QCOMPARE(analysis.summary, QString("No device selected"));
    QVERIFY(!analysis.structuredWarnings.isEmpty());
    QCOMPARE(analysis.structuredWarnings.first().severity, ResourceAnalysis::WarningSeverity::Info);

    // Verify FirmwareExportManager does not block export when no device is selected
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    FirmwareExportManager exportMgr;
    ExportOptions opts;
    opts.outputDirectory = tempDir.path();
    opts.targetFramework = "lvgl";

    ExportResult result = exportMgr.exportProject(&project, opts);
    QVERIFY(result.success);
}

void TestExportValidation::testResourceAnalysisCriticalBlocksExport() {
    CanvasScene scene;
    Project project(&scene);

    // Configure a small device (STM32F030R8: 64 KB flash, 8 KB RAM)
    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F030R8";
    project.setHardwareConfig(hw);

    // Add multiple components and screens to exceed 8 KB RAM
    for (int s = 0; s < 5; ++s) {
        Screen* scr = new Screen(QString("Screen_%1").arg(s));
        for (int c = 0; c < 20; ++c) {
            scr->addComponent(new ButtonComponent(QString("btn_%1_%2").arg(s).arg(c)));
            scr->addComponent(new LabelComponent(QString("lbl_%1_%2").arg(s).arg(c)));
        }
        project.addScreen(scr);
    }

    ResourceEstimator estimator;
    ResourceAnalysis analysis = estimator.analyzeProject(&project, "qt-for-mcus");

    QCOMPARE(analysis.status, ResourceAnalysis::CRITICAL);
    bool hasCriticalWarning = false;
    for (const auto& w : analysis.structuredWarnings) {
        if (w.severity == ResourceAnalysis::WarningSeverity::Critical) {
            hasCriticalWarning = true;
            break;
        }
    }
    QVERIFY(hasCriticalWarning);

    // Test that FirmwareExportManager BLOCKS export without forceExport
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    FirmwareExportManager exportMgr;
    ExportOptions opts;
    opts.outputDirectory = tempDir.path();
    opts.targetFramework = "qt-for-mcus";
    opts.forceExport = false;

    ExportResult resBlocked = exportMgr.exportProject(&project, opts);
    QVERIFY(!resBlocked.success);
    QVERIFY(resBlocked.errorMessage.contains("Export blocked"));

    // Test that ExportDialog disables the Export button on CRITICAL
    ExportDialog dlg(&project, nullptr);
    QPushButton* exportBtn = dlg.findChild<QPushButton*>("exportButton");
    QVERIFY(exportBtn != nullptr);
    QVERIFY(!exportBtn->isEnabled());

    QLabel* statusBanner = dlg.findChild<QLabel*>("statusBannerLabel");
    QVERIFY(statusBanner != nullptr);
    QVERIFY(statusBanner->text().contains("exceeds device limits"));

    // Test that forceExport allows it through
    opts.forceExport = true;
    ExportResult resForced = exportMgr.exportProject(&project, opts);
    QVERIFY(resForced.success);
}

void TestExportValidation::testResourceAnalysisWarningAllowsExport() {
    ResourceAnalysis analysis;
    analysis.has_device = true;
    analysis.device_name = "STM32F407VG";
    analysis.target_framework = "lvgl";
    analysis.flash_total = 1024 * 1024;
    analysis.flash_used = 850 * 1024; // ~83% (Warning threshold >= 80%)
    analysis.flash_percent = 83.0f;
    analysis.ram_total = 192 * 1024;
    analysis.ram_used = 50 * 1024;
    analysis.ram_percent = 26.0f;

    ResourceEstimator estimator;
    estimator.validateFitsOnDevice(analysis);

    QCOMPARE(analysis.status, ResourceAnalysis::WARNING);
    bool hasWarning = false;
    for (const auto& w : analysis.structuredWarnings) {
        if (w.severity == ResourceAnalysis::WarningSeverity::Warning) {
            hasWarning = true;
            break;
        }
    }
    QVERIFY(hasWarning);
}

void TestExportValidation::testResourceAnalysisOkPasses() {
    CanvasScene scene;
    Project project(&scene);

    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F407VG"; // 1MB Flash, 192KB RAM
    project.setHardwareConfig(hw);

    Screen* s = new Screen("MainScreen");
    s->addComponent(new ButtonComponent("btn_ok"));
    project.addScreen(s);

    ResourceEstimator estimator;
    ResourceAnalysis analysis = estimator.analyzeProject(&project, "lvgl");

    QCOMPARE(analysis.status, ResourceAnalysis::OK);
    QVERIFY(analysis.flash_percent < 80.0f);
    QVERIFY(analysis.ram_percent < 80.0f);

    ExportDialog dlg(&project, nullptr);
    QPushButton* exportBtn = dlg.findChild<QPushButton*>("exportButton");
    QVERIFY(exportBtn != nullptr);
    QVERIFY(exportBtn->isEnabled());
}

void TestExportValidation::testFrameworkSwitchingBaseline() {
    CanvasScene scene;
    Project project(&scene);

    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F407VG";
    project.setHardwareConfig(hw);

    ResourceEstimator estimator;
    ResourceAnalysis analysisLvgl = estimator.analyzeProject(&project, "lvgl");
    ResourceAnalysis analysisQul = estimator.analyzeProject(&project, "qt-for-mcus");
    ResourceAnalysis analysisUgfx = estimator.analyzeProject(&project, "ugfx");

    QVERIFY(analysisLvgl.breakdown.framework_flash != analysisQul.breakdown.framework_flash);
    QVERIFY(analysisLvgl.breakdown.framework_ram != analysisUgfx.breakdown.framework_ram);
}

void TestExportValidation::testAnalysisPerformanceCachingUnder100ms() {
    CanvasScene scene;
    Project project(&scene);

    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F407VG";
    project.setHardwareConfig(hw);

    Screen* s = new Screen("PerfScreen");
    for (int i = 0; i < 50; ++i) {
        s->addComponent(new ButtonComponent(QString("btn_%1").arg(i)));
    }
    project.addScreen(s);

    ResourceEstimator estimator;
    QElapsedTimer timer;
    timer.start();

    // First call (populates cache)
    ResourceAnalysis a1 = estimator.analyzeProject(&project, "lvgl");
    qint64 t1 = timer.elapsed();
    QVERIFY(t1 < 100);

    // Second call (cache hit)
    timer.restart();
    ResourceAnalysis a2 = estimator.analyzeProject(&project, "lvgl");
    qint64 t2 = timer.elapsed();
    QVERIFY(t2 < 100);
    QCOMPARE(a1.flash_used, a2.flash_used);
    QCOMPARE(a1.ram_used, a2.ram_used);
}

void TestExportValidation::testOptimizeHeaviestComponents() {
    CanvasScene scene;
    Project project(&scene);

    Screen* s = new Screen("MainScreen");
    s->addComponent(new ButtonComponent("btn1"));
    s->addComponent(new ButtonComponent("btn2"));
    s->addComponent(new LabelComponent("lbl1"));
    project.addScreen(s);

    ResourceEstimator estimator;
    QVector<ResourceEstimator::ComponentResourceEntry> heaviest = estimator.getHeaviestComponents(&project);

    QVERIFY(!heaviest.isEmpty());
    // Entries should have optimization suggestions populated
    for (const auto& entry : heaviest) {
        QVERIFY(!entry.componentType.isEmpty());
        QVERIFY(entry.count > 0);
        QVERIFY(!entry.optimizationSuggestion.isEmpty());
    }
}

void TestExportValidation::testRegressionGeneratorOutputUnchanged() {
    CanvasScene scene;
    Project project(&scene);
    project.setProjectName("RegressionApp");

    Screen* scr = new Screen("Screen1");
    auto* btn = new ButtonComponent("test_btn");
    btn->setProperty("text", "Click Me");
    auto* lbl = new LabelComponent("test_lbl");
    lbl->setProperty("text", "Status: Ready");
    auto* sld = new SliderComponent("test_sld");

    scr->addComponent(btn);
    scr->addComponent(lbl);
    scr->addComponent(sld);
    project.addScreen(scr);

    // Generate directly via CodeGenerators
    LvglGenerator lvglGen(&project, nullptr);
    QString directLvglHeader = lvglGen.generateUiHeader();
    QString directLvglSource = lvglGen.generateUiSource();

    UgfxGenerator ugfxGen(&project, nullptr);
    QString directUgfxHeader = ugfxGen.generateUiHeader();
    QString directUgfxSource = ugfxGen.generateUiSource();

    QtMcuGenerator qmlGen(&project, nullptr);
    QString directQmlSource = qmlGen.generateDesignQml();

    // Export via FirmwareExportManager with validation enabled
    QTemporaryDir lvglDir;
    QVERIFY(lvglDir.isValid());
    FirmwareExportManager exportMgr;
    ExportOptions optsLvgl;
    optsLvgl.outputDirectory = lvglDir.path();
    optsLvgl.targetFramework = "lvgl";
    optsLvgl.forceExport = true;
    ExportResult resLvgl = exportMgr.exportProject(&project, optsLvgl);
    QVERIFY(resLvgl.success);

    // Verify exported files match direct generator output exactly
    QFile exportedLvglH(QDir(lvglDir.path()).filePath("generated/ui/ui.h"));
    QVERIFY(exportedLvglH.open(QIODevice::ReadOnly | QIODevice::Text));
    QString exportedLvglHContent = QString::fromUtf8(exportedLvglH.readAll());
    exportedLvglH.close();
    QCOMPARE(exportedLvglHContent, directLvglHeader);

    QFile exportedLvglC(QDir(lvglDir.path()).filePath("generated/ui/ui.c"));
    QVERIFY(exportedLvglC.open(QIODevice::ReadOnly | QIODevice::Text));
    QString exportedLvglCContent = QString::fromUtf8(exportedLvglC.readAll());
    exportedLvglC.close();
    QCOMPARE(exportedLvglCContent, directLvglSource);

    // Verify µGFX export matches
    QTemporaryDir ugfxDir;
    QVERIFY(ugfxDir.isValid());
    ExportOptions optsUgfx;
    optsUgfx.outputDirectory = ugfxDir.path();
    optsUgfx.targetFramework = "ugfx";
    optsUgfx.forceExport = true;
    ExportResult resUgfx = exportMgr.exportProject(&project, optsUgfx);
    QVERIFY(resUgfx.success);

    QFile exportedUgfxH(QDir(ugfxDir.path()).filePath("generated/ui/gui.h"));
    QVERIFY(exportedUgfxH.open(QIODevice::ReadOnly | QIODevice::Text));
    QString exportedUgfxHContent = QString::fromUtf8(exportedUgfxH.readAll());
    exportedUgfxH.close();
    QCOMPARE(exportedUgfxHContent, directUgfxHeader);

    QFile exportedUgfxC(QDir(ugfxDir.path()).filePath("generated/ui/gui.c"));
    QVERIFY(exportedUgfxC.open(QIODevice::ReadOnly | QIODevice::Text));
    QString exportedUgfxCContent = QString::fromUtf8(exportedUgfxC.readAll());
    exportedUgfxC.close();
    QCOMPARE(exportedUgfxCContent, directUgfxSource);

    // Verify Qt for MCUs export matches
    QTemporaryDir qmlDir;
    QVERIFY(qmlDir.isValid());
    ExportOptions optsQml;
    optsQml.outputDirectory = qmlDir.path();
    optsQml.targetFramework = "qt-for-mcus";
    optsQml.forceExport = true;
    ExportResult resQml = exportMgr.exportProject(&project, optsQml);
    QVERIFY(resQml.success);

    QFile exportedQml(QDir(qmlDir.path()).filePath("generated/ui/MainScreen.qml"));
    QVERIFY(exportedQml.open(QIODevice::ReadOnly | QIODevice::Text));
    QString exportedQmlContent = QString::fromUtf8(exportedQml.readAll());
    exportedQml.close();
    QCOMPARE(exportedQmlContent, directQmlSource);
}

// A.3 housekeeping: estimation_baselines.json is a repo snapshot, not a runtime
// input. The compiled-in defaults are authoritative; the optional loader must
// reproduce exactly the same numbers and is intentionally never called at startup.
void TestExportValidation::testEstimationBaselinesDefaultsAuthoritative() {
    EstimationFormulas::initDefaultBaselines();

    auto frameworkFlash = [](const QString& framework) {
        return EstimationFormulas::FRAMEWORK_BASELINES.value(framework).flash_minimum;
    };

    // Compiled-in defaults, byte-for-byte.
    QCOMPARE(frameworkFlash("lvgl"), uint32_t(80 * 1024));
    QCOMPARE(frameworkFlash("ugfx"), uint32_t(120 * 1024));
    QCOMPARE(frameworkFlash("qt-for-mcus"), uint32_t(400 * 1024));
    QCOMPARE(frameworkFlash("embedweb"), uint32_t(16 * 1024));

    const auto buttonBefore = EstimationFormulas::COMPONENT_FOOTPRINTS.value("Button");
    QCOMPARE(buttonBefore.flash, uint32_t(200));
    QCOMPARE(buttonBefore.ram, uint32_t(80));
    QCOMPARE(buttonBefore.cpu_percent, 1.5f);

    // Locate the JSON snapshot in the repository. Prefer the working directory
    // (ctest runs tests from the repository root) and fall back to walking up
    // from the binary for in-repo build dirs; independent of build location.
    QString jsonPath;
    {
        const QString cwdCandidate = QDir::current().filePath("resources/estimation_baselines.json");
        if (QFileInfo::exists(cwdCandidate)) {
            jsonPath = cwdCandidate;
        }
    }
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 6 && jsonPath.isEmpty(); ++i) {
        const QString candidate = dir.filePath("resources/estimation_baselines.json");
        if (QFileInfo::exists(candidate)) {
            jsonPath = candidate;
            break;
        }
        if (!dir.cdUp()) break;
    }
    QVERIFY2(!jsonPath.isEmpty(), "resources/estimation_baselines.json not found");

    // Loading the snapshot must not change any number.
    const uint32_t lvglFlashBefore = frameworkFlash("lvgl");
    const uint32_t qtFlashBefore = frameworkFlash("qt-for-mcus");
    QVERIFY(EstimationFormulas::loadBaselinesFromJson(jsonPath));
    QCOMPARE(frameworkFlash("lvgl"), lvglFlashBefore);
    QCOMPARE(frameworkFlash("qt-for-mcus"), qtFlashBefore);
    QCOMPARE(frameworkFlash("embedweb"), uint32_t(16 * 1024)); // JSON has no embedweb entry

    const auto buttonAfter = EstimationFormulas::COMPONENT_FOOTPRINTS.value("Button");
    QCOMPARE(buttonAfter.flash, buttonBefore.flash);
    QCOMPARE(buttonAfter.ram, buttonBefore.ram);
    QCOMPARE(buttonAfter.cpu_percent, buttonBefore.cpu_percent);
}

void TestExportValidation::testEmptyFolderExportForAllDatabaseDevices() {
    CapabilityDatabase db;
    QStringList devices = db.listAllDevices();
    FirmwareExportManager exportMgr;
    driverpack::DriverPackRegistry& dpReg = driverpack::DriverPackRegistry::instance();

    qDebug() << "\n=== INVERTED EMPTY-FOLDER EXPORT TEST FOR ALL 17 DATABASE DEVICES ===";
    qDebug() << "Total devices to test:" << devices.size();

    int blockedCount = 0;
    int supportedCount = 0;

    for (const QString& devName : devices) {
        CanvasScene scene;
        Project project(&scene);
        project.setProjectName("App_" + devName);

        Hardware::HardwareConfig hw;
        hw.deviceId = devName;
        hw.targetType = "device";
        DeviceProfile prof = db.getDeviceProfile(devName);
        hw.family = prof.family;
        project.setHardwareConfig(hw);

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        ExportOptions opts;
        opts.outputDirectory = tempDir.path();
        opts.targetFramework = "lvgl";
        opts.forceExport = true;
        opts.targetMcu = devName;

        ExportResult res = exportMgr.exportProject(&project, opts);

        auto* pack = dpReg.findPackForDevice(devName.toStdString());
        const driverpack::McuDeviceSpec* devSpec = pack ? pack->findDevice(devName.toStdString()) : nullptr;
        const bool driverDataUnverified = (pack != nullptr) && (devSpec != nullptr) && !devSpec->driverDataVerified;
        if (!pack || driverDataUnverified) {
            // Unsupported/unverified device: UI export succeeds, native HAL is blocked with warning in warnings
            QVERIFY2(res.success, qPrintable(QString("Device %1 is unsupported but UI export failed: %2").arg(devName, res.errorMessage)));
            bool hasPackWarning = false;
            for (const QString& w : res.warnings) {
                if (!pack && w.contains(QString("No driver pack for %1; import one").arg(devName))) {
                    hasPackWarning = true;
                    break;
                }
                if (driverDataUnverified && w.contains(QString("driver data unverified"))) {
                    hasPackWarning = true;
                    break;
                }
            }
            QVERIFY2(hasPackWarning, qPrintable(QString("Expected block warning for %1 in warnings").arg(devName)));

            // No HAL files must be produced
            QVERIFY(!QFile::exists(QDir(tempDir.path()).filePath("generated/target/target_hal.h")));
            QVERIFY(!QFile::exists(QDir(tempDir.path()).filePath("generated/target/target_hal.c")));

            // UI files MUST be produced
            QVERIFY(QFile::exists(QDir(tempDir.path()).filePath("generated/ui/ui.h")));
            QVERIFY(QFile::exists(QDir(tempDir.path()).filePath("generated/ui/ui.c")));

            blockedCount++;
            qDebug() << (pack ? "Unverified driver data: native HAL correctly blocked (UI exported):"
                              : "Unsupported device native HAL correctly blocked (UI exported):") << devName;
        } else {
            // Supported device: export succeeds and HAL is generated per-family
            QVERIFY2(res.success, qPrintable(QString("Device %1 has pack %2 but export failed: %3")
                                             .arg(devName, QString::fromStdString(pack->familyId()), res.errorMessage)));
            supportedCount++;

            // Scan all generated files for foreign family leakage or F030 leakage
            QStringList f030Matches;
            QStringList foreignMatches;

            QDirIterator it(tempDir.path(), QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString filePath = it.next();
                QFile f(filePath);
                if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString content = QString::fromUtf8(f.readAll());
                    QString relPath = QDir(tempDir.path()).relativeFilePath(filePath);

                    if (devName != "STM32F030R8") {
                        if (content.contains("STM32F030", Qt::CaseInsensitive) ||
                            content.contains("F030R8", Qt::CaseInsensitive) ||
                            content.contains("NUCLEO-F030R8", Qt::CaseInsensitive) ||
                            content.contains("nucleo_f030r8", Qt::CaseInsensitive)) {
                            f030Matches.append(relPath);
                        }
                    }

                    // Check for cross-family leakage
                    if (prof.family == "ESP32") {
                        if (content.contains("pico/stdlib.h") || content.contains("fsl_gpio.h") || content.contains("zephyr/kernel.h") || content.contains("stm32f0xx") || content.contains("stm32_hal")) {
                            foreignMatches.append(relPath + " (foreign header)");
                        }
                    } else if (prof.family == "RP2040") {
                        if (content.contains("driver/gpio.h") || content.contains("fsl_gpio.h") || content.contains("zephyr/kernel.h") || content.contains("stm32f0xx") || content.contains("stm32_hal")) {
                            foreignMatches.append(relPath + " (foreign header)");
                        }
                    } else if (prof.family == "i.MX RT") {
                        if (content.contains("driver/gpio.h") || content.contains("pico/stdlib.h") || content.contains("zephyr/kernel.h") || content.contains("stm32f0xx") || content.contains("stm32_hal")) {
                            foreignMatches.append(relPath + " (foreign header)");
                        }
                    } else if (prof.family == "nRF52" || prof.family == "Nordic") {
                        if (content.contains("driver/gpio.h") || content.contains("pico/stdlib.h") || content.contains("fsl_gpio.h") || content.contains("stm32f0xx") || content.contains("stm32_hal")) {
                            foreignMatches.append(relPath + " (foreign header)");
                        }
                    } else if (prof.family == "STM32F4") {
                        if (content.contains("stm32f0xx") || content.contains("driver/gpio.h") || content.contains("pico/stdlib.h") || content.contains("fsl_gpio.h") || content.contains("zephyr/kernel.h")) {
                            foreignMatches.append(relPath + " (foreign header)");
                        }
                    }
                }
            }

            QVERIFY2(f030Matches.isEmpty(), qPrintable(QString("F030 leakage in non-F030 device %1: %2").arg(devName, f030Matches.join(", "))));
            QVERIFY2(foreignMatches.isEmpty(), qPrintable(QString("Cross-family leakage in device %1: %2").arg(devName, foreignMatches.join(", "))));
            qDebug() << "Supported device verified clean (0 leaks):" << devName << "(Pack:" << QString::fromStdString(pack->familyId()) << ")";
        }
    }

    qDebug() << "Test summary: Total devices:" << devices.size()
             << "| Supported:" << supportedCount
             << "| Unsupported (blocked):" << blockedCount;
    QCOMPARE(supportedCount + blockedCount, devices.size());
}

void TestExportValidation::testGoldenF030OutputUnchanged() {
    FirmwareExportManager exportMgr;

    // 1. Export with explicit STM32F030R8
    CanvasScene scene1;
    Project projectF030(&scene1);
    projectF030.setProjectName("GoldenF030");
    Hardware::HardwareConfig hw1;
    hw1.deviceId = "STM32F030R8";
    hw1.family = "STM32";
    projectF030.setHardwareConfig(hw1);

    QTemporaryDir dirF030;
    QVERIFY(dirF030.isValid());
    ExportOptions opts1;
    opts1.outputDirectory = dirF030.path();
    opts1.targetFramework = "lvgl";
    opts1.forceExport = true;
    opts1.targetMcu = "STM32F030R8";

    ExportResult resF030 = exportMgr.exportProject(&projectF030, opts1);
    QVERIFY(resF030.success);

    // 2. Export with no device selected (legacy unconfigured baseline)
    CanvasScene scene2;
    Project projectNoDev(&scene2);
    projectNoDev.setProjectName("GoldenF030");

    QTemporaryDir dirNoDev;
    QVERIFY(dirNoDev.isValid());
    ExportOptions opts2;
    opts2.outputDirectory = dirNoDev.path();
    opts2.targetFramework = "lvgl";
    opts2.forceExport = true;

    ExportResult resNoDev = exportMgr.exportProject(&projectNoDev, opts2);
    QVERIFY(resNoDev.success);
    QVERIFY(resNoDev.warnings.contains("Info: No device selected; using default STM32F030R8 baseline."));

    // 3. Read generated HAL headers and sources
    QFile f030H(QDir(dirF030.path()).filePath("generated/target/target_hal.h"));
    QVERIFY(f030H.open(QIODevice::ReadOnly | QIODevice::Text));
    QString f030HContent = QString::fromUtf8(f030H.readAll());
    f030H.close();

    QFile f030C(QDir(dirF030.path()).filePath("generated/target/target_hal.c"));
    QVERIFY(f030C.open(QIODevice::ReadOnly | QIODevice::Text));
    QString f030CContent = QString::fromUtf8(f030C.readAll());
    f030C.close();

    QFile noDevH(QDir(dirNoDev.path()).filePath("generated/target/target_hal.h"));
    QVERIFY(noDevH.open(QIODevice::ReadOnly | QIODevice::Text));
    QString noDevHContent = QString::fromUtf8(noDevH.readAll());
    noDevH.close();

    QFile noDevC(QDir(dirNoDev.path()).filePath("generated/target/target_hal.c"));
    QVERIFY(noDevC.open(QIODevice::ReadOnly | QIODevice::Text));
    QString noDevCContent = QString::fromUtf8(noDevC.readAll());
    noDevC.close();

    // Verify unconfigured export is byte-identical to explicit F030R8
    QCOMPARE(noDevHContent, f030HContent);
    QCOMPARE(noDevCContent, f030CContent);

    // Verify golden markers
    QVERIFY(f030HContent.contains("Target Family: STM32 | Target Board: STM32 NUCLEO-F030R8"));
    QVERIFY(f030HContent.contains("#define PIN_PA0"));
    QVERIFY(f030HContent.contains("#define PROTOCOL_HAS_SPI 1"));
    QVERIFY(f030HContent.contains("void target_hal_init(void);"));

    QVERIFY(f030CContent.contains("[HAL: STM32] Initializing target board: STM32 NUCLEO-F030R8"));
    QVERIFY(f030CContent.contains("[HAL: STM32] Atomic BSRR Write pin %s -> %s"));
    QVERIFY(f030CContent.contains("GPIOA->BSRR"));
}

void TestExportValidation::testManifestDeterministic() {
    CanvasScene scene;
    Project project(&scene);
    project.setProjectName("DeterministicProject");
    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F030R8";
    hw.family = "STM32";
    project.setHardwareConfig(hw);

    FirmwareExportManager exportMgr;

    QTemporaryDir dir1;
    QVERIFY(dir1.isValid());
    ExportOptions opts1;
    opts1.outputDirectory = dir1.path();
    opts1.targetFramework = "lvgl";
    opts1.targetMcu = "STM32F030R8";
    opts1.forceExport = true;

    ExportResult res1 = exportMgr.exportProject(&project, opts1);
    QVERIFY(res1.success);

    QString manifestPath1 = QDir(dir1.path()).filePath("generated/MANIFEST.json");
    QVERIFY(QFile::exists(manifestPath1));
    QFile f1(manifestPath1);
    QVERIFY(f1.open(QIODevice::ReadOnly));
    QByteArray manifestBytes1 = f1.readAll();
    f1.close();

    // Verify manifest JSON structure
    QJsonDocument doc1 = QJsonDocument::fromJson(manifestBytes1);
    QVERIFY(doc1.isObject());
    QJsonObject obj1 = doc1.object();
    QCOMPARE(obj1.value("generatorVersion").toString(), QString("1.0.0"));
    QCOMPARE(obj1.value("device").toString(), QString("STM32F030R8"));
    QJsonArray filesArr1 = obj1.value("files").toArray();
    QVERIFY(!filesArr1.isEmpty());

    // Verify files in manifest match real disk files and their sha256
    QStringList manifestFiles;
    for (const auto& itemVal : filesArr1) {
        QJsonObject item = itemVal.toObject();
        QString relPath = item.value("file").toString();
        QString hash = item.value("sha256").toString();
        manifestFiles.append(relPath);

        QFile diskFile(QDir(dir1.path()).filePath(relPath));
        QVERIFY(diskFile.open(QIODevice::ReadOnly));
        QByteArray actualHash = QCryptographicHash::hash(diskFile.readAll(), QCryptographicHash::Sha256).toHex();
        diskFile.close();
        QCOMPARE(hash, QString::fromUtf8(actualHash));
    }
    // Manifest must not track itself
    QVERIFY(!manifestFiles.contains("generated/MANIFEST.json"));
    // Manifest must track generated/target/target_hal.h, generated/ui/ui.h, etc.
    QVERIFY(manifestFiles.contains("generated/target/target_hal.h"));
    QVERIFY(manifestFiles.contains("generated/ui/ui.h"));

    // Second export to separate folder with exact same inputs
    QTemporaryDir dir2;
    QVERIFY(dir2.isValid());
    ExportOptions opts2;
    opts2.outputDirectory = dir2.path();
    opts2.targetFramework = "lvgl";
    opts2.targetMcu = "STM32F030R8";
    opts2.forceExport = true;

    ExportResult res2 = exportMgr.exportProject(&project, opts2);
    QVERIFY(res2.success);

    QString manifestPath2 = QDir(dir2.path()).filePath("generated/MANIFEST.json");
    QVERIFY(QFile::exists(manifestPath2));
    QFile f2(manifestPath2);
    QVERIFY(f2.open(QIODevice::ReadOnly));
    QByteArray manifestBytes2 = f2.readAll();
    f2.close();

    // Determinism: Same input -> Byte-identical MANIFEST.json output
    QCOMPARE(manifestBytes1, manifestBytes2);
}

void TestExportValidation::testOrphanRemoval() {
    CanvasScene scene;
    Project project(&scene);
    project.setProjectName("OrphanProject");
    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F030R8";
    hw.family = "STM32";
    project.setHardwareConfig(hw);

    FirmwareExportManager exportMgr;

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ExportOptions opts;
    opts.outputDirectory = dir.path();
    opts.targetFramework = "lvgl";
    opts.targetMcu = "STM32F030R8";
    opts.forceExport = true;

    ExportResult res1 = exportMgr.exportProject(&project, opts);
    QVERIFY(res1.success);

    // Create a dummy orphan in generated/ and add to generated/MANIFEST.json
    QString orphanRel = "generated/ui/dummy_orphan.c";
    QString orphanFull = QDir(dir.path()).filePath(orphanRel);
    QFile orphanF(orphanFull);
    QVERIFY(orphanF.open(QIODevice::WriteOnly | QIODevice::Text));
    orphanF.write("// Old orphan code\nvoid old_orphan(void) {}\n");
    orphanF.close();

    // Update manifest to include orphan
    QString manifestPath = QDir(dir.path()).filePath("generated/MANIFEST.json");
    QFile mfRead(manifestPath);
    QVERIFY(mfRead.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(mfRead.readAll());
    mfRead.close();

    QJsonObject obj = doc.object();
    QJsonArray arr = obj.value("files").toArray();
    QJsonObject orphanEntry;
    orphanEntry["file"] = orphanRel;
    orphanEntry["sha256"] = "dummyhash123456";
    arr.append(orphanEntry);
    obj["files"] = arr;

    QFile mfWrite(manifestPath);
    QVERIFY(mfWrite.open(QIODevice::WriteOnly | QIODevice::Text));
    mfWrite.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    mfWrite.close();

    QVERIFY(QFile::exists(orphanFull));

    // Regenerate project: should detect orphan listed in old manifest and remove it
    ExportResult res2 = exportMgr.exportProject(&project, opts);
    QVERIFY(res2.success);

    // Orphan must be deleted from disk
    QVERIFY(!QFile::exists(orphanFull));

    // Must be reported in warnings
    bool reported = false;
    for (const QString& w : res2.warnings) {
        if (w.contains("Removed orphan file: generated/ui/dummy_orphan.c")) {
            reported = true;
            break;
        }
    }
    QVERIFY(reported);
}

void TestExportValidation::testEditDetectionWithBackup() {
    CanvasScene scene;
    Project project(&scene);
    project.setProjectName("BackupProject");
    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F030R8";
    hw.family = "STM32";
    project.setHardwareConfig(hw);

    FirmwareExportManager exportMgr;

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ExportOptions opts;
    opts.outputDirectory = dir.path();
    opts.targetFramework = "lvgl";
    opts.targetMcu = "STM32F030R8";
    opts.forceExport = true;

    ExportResult res1 = exportMgr.exportProject(&project, opts);
    QVERIFY(res1.success);

    // User edits a generated file: generated/ui/ui.h
    QString uiHPath = QDir(dir.path()).filePath("generated/ui/ui.h");
    QVERIFY(QFile::exists(uiHPath));
    QFile editFile(uiHPath);
    QVERIFY(editFile.open(QIODevice::Append | QIODevice::Text));
    editFile.write("\n// USER MODIFICATION: Custom UI macro 99999\n");
    editFile.close();

    // Verify diagnostics source catches modification before regeneration
    project.setFirmwareDirectory(dir.path());
    Diagnostics::DiagnosticsService diagService;
    Diagnostics::DiagnosticsSources::registerBuiltinSources(diagService);
    diagService.setProject(&project);
    QVector<Diagnostics::Diagnostic> diags = diagService.runNow();
    bool foundDiag = false;
    for (const auto& d : diags) {
        if (d.source == "Manifest" && d.message.contains("generated/ui/ui.h has been modified by user")) {
            foundDiag = true;
            QCOMPARE(d.severity, Diagnostics::Diagnostic::Severity::Warning);
            break;
        }
    }
    QVERIFY(foundDiag);

    // Regenerate project: should detect edit, back up to generated/.backup/<timestamp>/, and warn
    ExportResult res2 = exportMgr.exportProject(&project, opts);
    QVERIFY(res2.success);

    bool backupWarnFound = false;
    for (const QString& w : res2.warnings) {
        if (w.contains("User modified file generated/ui/ui.h backed up to generated/.backup/")) {
            backupWarnFound = true;
            break;
        }
    }
    QVERIFY(backupWarnFound);

    // Verify backup file exists in generated/.backup/
    QDir backupBaseDir(QDir(dir.path()).filePath("generated/.backup"));
    QVERIFY(backupBaseDir.exists());
    QStringList timestampDirs = backupBaseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    QVERIFY(!timestampDirs.isEmpty());

    QString backupFile = backupBaseDir.filePath(timestampDirs.first() + "/generated/ui/ui.h");
    QVERIFY(QFile::exists(backupFile));

    // Verify the backup file contains the user's modifications
    QFile bf(backupFile);
    QVERIFY(bf.open(QIODevice::ReadOnly | QIODevice::Text));
    QString backupContent = QString::fromUtf8(bf.readAll());
    bf.close();
    QVERIFY(backupContent.contains("// USER MODIFICATION: Custom UI macro 99999"));

    // Verify the freshly regenerated generated/ui/ui.h does NOT contain the user edit
    QFile regeneratedF(uiHPath);
    QVERIFY(regeneratedF.open(QIODevice::ReadOnly | QIODevice::Text));
    QString regeneratedContent = QString::fromUtf8(regeneratedF.readAll());
    regeneratedF.close();
    QVERIFY(!regeneratedContent.contains("// USER MODIFICATION: Custom UI macro 99999"));
}

void TestExportValidation::testUserFilesUntouched() {
    CanvasScene scene;
    Project project(&scene);
    project.setProjectName("UserFilesProject");
    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F030R8";
    hw.family = "STM32";
    project.setHardwareConfig(hw);

    FirmwareExportManager exportMgr;

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ExportOptions opts;
    opts.outputDirectory = dir.path();
    opts.targetFramework = "lvgl";
    opts.targetMcu = "STM32F030R8";
    opts.forceExport = true;
    opts.overwriteUserCode = false;

    ExportResult res1 = exportMgr.exportProject(&project, opts);
    QVERIFY(res1.success);

    // Verify initial user files
    QString mainCPath = QDir(dir.path()).filePath("user/main.c");
    QVERIFY(QFile::exists(mainCPath));

    // User edits user/main.c
    QFile mainFile(mainCPath);
    QVERIFY(mainFile.open(QIODevice::Append | QIODevice::Text));
    mainFile.write("\n// USER PERSISTENT LOGIC 42\nint user_custom_var = 42;\n");
    mainFile.close();

    // User creates a brand new user file: user/custom_helper.c
    QString customHelperPath = QDir(dir.path()).filePath("user/custom_helper.c");
    QFile helperFile(customHelperPath);
    QVERIFY(helperFile.open(QIODevice::WriteOnly | QIODevice::Text));
    helperFile.write("// User custom helper\nvoid helper_func(void) {}\n");
    helperFile.close();

    // Regenerate project with overwriteUserCode = false
    ExportResult res2 = exportMgr.exportProject(&project, opts);
    QVERIFY(res2.success);

    // 1. user/main.c must preserve user modifications
    QFile mainCheck(mainCPath);
    QVERIFY(mainCheck.open(QIODevice::ReadOnly | QIODevice::Text));
    QString mainContent = QString::fromUtf8(mainCheck.readAll());
    mainCheck.close();
    QVERIFY(mainContent.contains("// USER PERSISTENT LOGIC 42"));
    QVERIFY(mainContent.contains("int user_custom_var = 42;"));

    // 2. user/custom_helper.c must NEVER be touched or deleted as an orphan
    QVERIFY(QFile::exists(customHelperPath));
    QFile helperCheck(customHelperPath);
    QVERIFY(helperCheck.open(QIODevice::ReadOnly | QIODevice::Text));
    QString helperContent = QString::fromUtf8(helperCheck.readAll());
    helperCheck.close();
    QVERIFY(helperContent.contains("// User custom helper"));

    // 3. User files must NOT be backed up into generated/.backup/
    QDir backupBaseDir(QDir(dir.path()).filePath("generated/.backup"));
    if (backupBaseDir.exists()) {
        QDirIterator it(backupBaseDir.path(), QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString path = it.next();
            QVERIFY(!path.contains("/user/"));
        }
    }

    // 4. Preserved user files reported in result
    bool mainPreserved = false;
    for (const QString& f : res2.preservedUserFiles) {
        if (f.contains("user/main.c")) {
            mainPreserved = true;
            break;
        }
    }
    QVERIFY(mainPreserved);
}

void TestExportValidation::testUiExportWorksForUnsupportedDeviceWithoutHal() {
    FirmwareExportManager exportMgr;

    // Test 1: STM32H745 with LVGL
    CanvasScene sceneLvgl;
    Project projectLvgl(&sceneLvgl);
    projectLvgl.setProjectName("H745_Lvgl");
    Hardware::HardwareConfig hwH7;
    hwH7.deviceId = "STM32H745";
    hwH7.family = "STM32";
    projectLvgl.setHardwareConfig(hwH7);

    QTemporaryDir dirLvgl;
    QVERIFY(dirLvgl.isValid());
    ExportOptions optsLvgl;
    optsLvgl.outputDirectory = dirLvgl.path();
    optsLvgl.targetFramework = "lvgl";
    optsLvgl.targetMcu = "STM32H745";
    optsLvgl.forceExport = true;

    ExportResult resLvgl = exportMgr.exportProject(&projectLvgl, optsLvgl);
    QVERIFY(resLvgl.success);

    // UI code is fully exported
    QVERIFY(QFile::exists(QDir(dirLvgl.path()).filePath("generated/ui/ui.h")));
    QVERIFY(QFile::exists(QDir(dirLvgl.path()).filePath("generated/ui/ui.c")));
    QVERIFY(QFile::exists(QDir(dirLvgl.path()).filePath("generated/bindings/ui_bindings.h")));
    QVERIFY(QFile::exists(QDir(dirLvgl.path()).filePath("generated/bindings/ui_bindings.c")));
    QVERIFY(QFile::exists(QDir(dirLvgl.path()).filePath("generated/MANIFEST.json")));

    // Native HAL is blocked (no target_hal.* files)
    QVERIFY(!QFile::exists(QDir(dirLvgl.path()).filePath("generated/target/target_hal.h")));
    QVERIFY(!QFile::exists(QDir(dirLvgl.path()).filePath("generated/target/target_hal.c")));

    // Warning logged in result
    bool foundH7Warn = false;
    for (const QString& w : resLvgl.warnings) {
        if (w.contains("No driver pack for STM32H745; import one")) {
            foundH7Warn = true;
            break;
        }
    }
    QVERIFY(foundH7Warn);

    // Test Problems dock diagnostics
    projectLvgl.setFirmwareDirectory(dirLvgl.path());
    Diagnostics::DiagnosticsService diagService;
    Diagnostics::DiagnosticsSources::registerBuiltinSources(diagService);
    diagService.setProject(&projectLvgl);
    QVector<Diagnostics::Diagnostic> diags = diagService.runNow();
    bool foundDiag = false;
    for (const auto& d : diags) {
        if (d.source == "DriverPack" && d.message.contains("No driver pack for STM32H745; import one")) {
            QCOMPARE(d.severity, Diagnostics::Diagnostic::Severity::Error);
            foundDiag = true;
            break;
        }
    }
    QVERIFY(foundDiag);

    // Test 2: STM32H745 with Qt for MCUs
    CanvasScene sceneQt;
    Project projectQt(&sceneQt);
    projectQt.setProjectName("H745_QtMcu");
    projectQt.setHardwareConfig(hwH7);

    QTemporaryDir dirQt;
    QVERIFY(dirQt.isValid());
    ExportOptions optsQt;
    optsQt.outputDirectory = dirQt.path();
    optsQt.targetFramework = "qt-for-mcus";
    optsQt.targetMcu = "STM32H745";
    optsQt.forceExport = true;

    ExportResult resQt = exportMgr.exportProject(&projectQt, optsQt);
    QVERIFY(resQt.success);

    // QML is fully exported
    QVERIFY(QFile::exists(QDir(dirQt.path()).filePath("generated/ui/MainScreen.qml")));
    // Native HAL is blocked
    QVERIFY(!QFile::exists(QDir(dirQt.path()).filePath("generated/target/target_hal.h")));
    QVERIFY(!QFile::exists(QDir(dirQt.path()).filePath("generated/target/target_hal.c")));
}

void TestExportValidation::testManifestSafetyRejectsPathTraversalAndExternalSymlinks() {
    CanvasScene scene;
    Project project(&scene);
    project.setProjectName("SafetyProject");
    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F030R8";
    hw.family = "STM32";
    project.setHardwareConfig(hw);

    FirmwareExportManager exportMgr;

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ExportOptions opts;
    opts.outputDirectory = dir.path();
    opts.targetFramework = "lvgl";
    opts.targetMcu = "STM32F030R8";
    opts.forceExport = true;

    ExportResult res1 = exportMgr.exportProject(&project, opts);
    QVERIFY(res1.success);

    // Create external target files that must NEVER be touched
    QString extDirPath = dir.path() + "/../outside_vault_" + QString::number(QCoreApplication::applicationPid());
    QDir().mkpath(extDirPath);
    QString outsideTraverseFile = extDirPath + "/secret_relative.txt";
    QFile fOut1(outsideTraverseFile);
    QVERIFY(fOut1.open(QIODevice::WriteOnly | QIODevice::Text));
    fOut1.write("DO_NOT_DELETE_RELATIVE");
    fOut1.close();

    QString outsideAbsFile = extDirPath + "/secret_absolute.txt";
    QFile fOut2(outsideAbsFile);
    QVERIFY(fOut2.open(QIODevice::WriteOnly | QIODevice::Text));
    fOut2.write("DO_NOT_DELETE_ABSOLUTE");
    fOut2.close();

    QString outsideSymlinkTarget = extDirPath + "/secret_symlink_target.txt";
    QFile fOut3(outsideSymlinkTarget);
    QVERIFY(fOut3.open(QIODevice::WriteOnly | QIODevice::Text));
    fOut3.write("DO_NOT_DELETE_SYMLINK");
    fOut3.close();

    // Create symlink inside generated/ pointing outside
    QString symlinkInGenerated = QDir(dir.path()).filePath("generated/symlink_escape.c");
    QFile::link(outsideSymlinkTarget, symlinkInGenerated);
    QVERIFY(QFile::exists(symlinkInGenerated));

    // Maliciously tamper with generated/MANIFEST.json to inject path traversal and external paths
    QString manifestPath = QDir(dir.path()).filePath("generated/MANIFEST.json");
    QFile mfRead(manifestPath);
    QVERIFY(mfRead.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(mfRead.readAll());
    mfRead.close();

    QJsonObject obj = doc.object();
    QJsonArray arr = obj.value("files").toArray();

    // 1. Relative traversal
    QJsonObject travEntry;
    travEntry["file"] = "../outside_vault_" + QString::number(QCoreApplication::applicationPid()) + "/secret_relative.txt";
    travEntry["sha256"] = "badhash1";
    arr.append(travEntry);

    // 2. Absolute path
    QJsonObject absEntry;
    absEntry["file"] = outsideAbsFile;
    absEntry["sha256"] = "badhash2";
    arr.append(absEntry);

    // 3. Symlink pointing outside generated/
    QJsonObject symEntry;
    symEntry["file"] = "generated/symlink_escape.c";
    symEntry["sha256"] = "badhash3";
    arr.append(symEntry);

    obj["files"] = arr;

    QFile mfWrite(manifestPath);
    QVERIFY(mfWrite.open(QIODevice::WriteOnly | QIODevice::Text));
    mfWrite.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    mfWrite.close();

    // Regenerate project: must reject all unsafe entries and NEVER touch or backup external files
    ExportResult res2 = exportMgr.exportProject(&project, opts);
    QVERIFY(res2.success);

    // 1. External files must still exist and be completely untouched
    QVERIFY(QFile::exists(outsideTraverseFile));
    QVERIFY(QFile::exists(outsideAbsFile));
    QVERIFY(QFile::exists(outsideSymlinkTarget));

    // 2. No backup of external files in generated/.backup/
    QDir backupBaseDir(QDir(dir.path()).filePath("generated/.backup"));
    if (backupBaseDir.exists()) {
        QDirIterator it(backupBaseDir.path(), QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString path = it.next();
            QVERIFY(!path.contains("secret_relative"));
            QVERIFY(!path.contains("secret_absolute"));
            QVERIFY(!path.contains("secret_symlink"));
        }
    }

    // 3. Rejected unsafe entries logged in warnings
    bool sawRejected = false;
    for (const QString& w : res2.warnings) {
        if (w.contains("Rejected unsafe manifest entry")) {
            sawRejected = true;
            break;
        }
    }
    QVERIFY(sawRejected);

    // Cleanup external test dir
    QFile::remove(outsideTraverseFile);
    QFile::remove(outsideAbsFile);
    QFile::remove(outsideSymlinkTarget);
    QDir().rmdir(extDirPath);
}

void TestExportValidation::testCompileCheckArmGccSyntax() {
    FirmwareExportManager exportMgr;

    // 1. STM32F030R8 (Cortex-M0)
    CanvasScene sceneF0;
    Project projF0(&sceneF0);
    projF0.setProjectName("F030_Syntax");
    Hardware::HardwareConfig hwF0;
    hwF0.deviceId = "STM32F030R8";
    hwF0.family = "STM32";
    projF0.setHardwareConfig(hwF0);

    QTemporaryDir dirF0;
    QVERIFY(dirF0.isValid());
    ExportOptions optsF0;
    optsF0.outputDirectory = dirF0.path();
    optsF0.targetFramework = "lvgl";
    optsF0.targetMcu = "STM32F030R8";
    optsF0.forceExport = true;
    ExportResult resF0 = exportMgr.exportProject(&projF0, optsF0);
    QVERIFY(resF0.success);

    QString f0CPath = QDir(dirF0.path()).filePath("generated/target/target_hal.c");
    QVERIFY(QFile::exists(f0CPath));

    QProcess procF0;
    QStringList argsF0 = {
        "-mcpu=cortex-m0", "-mthumb", "-fsyntax-only",
        f0CPath,
        "-I" + QDir(dirF0.path()).filePath("generated/target")
    };
    procF0.start("arm-none-eabi-gcc", argsF0);
    QVERIFY(procF0.waitForFinished(10000));
    QCOMPARE(procF0.exitStatus(), QProcess::NormalExit);
    QCOMPARE(procF0.exitCode(), 0);

    // 2. STM32F401 (Cortex-M4) — driver data unverified: native HAL blocked
    CanvasScene sceneF4;
    Project projF4(&sceneF4);
    projF4.setProjectName("F401_Syntax");
    Hardware::HardwareConfig hwF4;
    hwF4.deviceId = "STM32F401";
    hwF4.family = "STM32";
    projF4.setHardwareConfig(hwF4);

    QTemporaryDir dirF4;
    QVERIFY(dirF4.isValid());
    QDir targetDirF4(QDir(dirF4.path()).filePath("generated/target"));

    ExportOptions optsF4;
    optsF4.outputDirectory = dirF4.path();
    optsF4.targetFramework = "lvgl";
    optsF4.targetMcu = "STM32F401";
    optsF4.forceExport = true;
    ExportResult resF4 = exportMgr.exportProject(&projF4, optsF4);
    QVERIFY(resF4.success);

    // No native HAL may be produced while the F401 driver data is unverified
    QVERIFY2(!QFile::exists(targetDirF4.filePath("target_hal.c")),
             "F401 native HAL must stay blocked until its data is verified");
    QVERIFY(!QFile::exists(targetDirF4.filePath("target_hal.h")));
    bool sawUnverified = false;
    for (const QString& w : resF4.warnings) {
        if (w.contains("driver data unverified")) { sawUnverified = true; break; }
    }
    QVERIFY2(sawUnverified, "F401 export must warn that driver data is unverified");

    // UI files must still be produced for the unverified device
    QVERIFY(QFile::exists(QDir(dirF4.path()).filePath("generated/ui/ui.h")));
    QVERIFY(QFile::exists(QDir(dirF4.path()).filePath("generated/ui/ui.c")));
}

void TestExportValidation::testPlanHalGoldenEmptyConfigHashes() {
    FirmwareExportManager exportMgr;
    CanvasScene scene;
    Project project(&scene);
    project.setProjectName("GoldenHashes");
    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F030R8";
    hw.family = "STM32";
    project.setHardwareConfig(hw);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ExportOptions opts;
    opts.outputDirectory = dir.path();
    opts.targetFramework = "lvgl";
    opts.targetMcu = "STM32F030R8";
    opts.forceExport = true;
    ExportResult res = exportMgr.exportProject(&project, opts);
    QVERIFY(res.success);

    const QString cPath = QDir(dir.path()).filePath("generated/target/target_hal.c");
    const QString hPath = QDir(dir.path()).filePath("generated/target/target_hal.h");
    QVERIFY(QFile::exists(cPath));
    QVERIFY(QFile::exists(hPath));

    // Byte-identical to the pre-Phase-C baseline: an empty hardware
    // configuration always takes the template path, never the plan path.
    QCOMPARE(sha256OfFile(cPath), QStringLiteral("289cfeb2cfae39d05ac00edd7e9842711e9417cd58bda05c314e9e7b074a3219"));
    QCOMPARE(sha256OfFile(hPath), QStringLiteral("5bc23bdda8d8671112f26b3f48f75955a67b14f4c4ec3482eb624105666219c0"));
}

void TestExportValidation::testPlanHalProbeAMatchesConfig() {
    FirmwareExportManager exportMgr;
    CanvasScene scene;
    Project project(&scene);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ExportResult res;
    QVERIFY(exportConfigTo(exportMgr, scene, project, probeAConfig(), dir.path(), &res));

    const QString cPath = QDir(dir.path()).filePath("generated/target/target_hal.c");
    const QString hPath = QDir(dir.path()).filePath("generated/target/target_hal.h");
    const QString content = readTextFile(cPath);
    QVERIFY(!content.isEmpty());

    // Pins, clock enables and the USART1 baud divisor derived from 48 MHz.
    QVERIFY(content.contains("RCC->AHBENR |= RCC_AHBENR_GPIOAEN;"));
    QVERIFY(content.contains("RCC->APB2ENR |= RCC_APB2ENR_USART1EN;"));
    QVERIFY(content.contains("USART1->BRR = 208u;"));
    QVERIFY(content.contains("USART_CR1_TE | USART_CR1_RE | USART_CR1_UE"));
    QVERIFY(content.contains("GPIOA->BSRR = (1u << 5u); /* initial output: high */"));
    QVERIFY(content.contains("| (1u << ((9u - 8u) * 4u));"));
    QVERIFY(content.contains("| (1u << ((10u - 8u) * 4u));"));

    // No template leftovers, no interrupt or other-family code.
    QVERIFY(!content.contains("115200"));
    QVERIFY(!content.contains("SCK="));
    QVERIFY(!content.contains("MOSI"));
    QVERIFY(!content.contains("EXTI"));
    QVERIFY(!content.contains("NVIC"));
    QVERIFY(!content.contains("SYSCFG"));
    QVERIFY(!content.contains("ESP32"));
    QVERIFY(!content.contains("stm32f401"));
    QVERIFY(!content.contains("gpio_set_function"));

    const QString header = readTextFile(hPath);
    QVERIFY(header.contains("#define PIN_PA5 \"PA5\""));
    QVERIFY(header.contains("#define PROTOCOL_HAS_GPIO 1"));
    QVERIFY(header.contains("#define PROTOCOL_HAS_UART 1 /* USART1 */"));
    QVERIFY(!header.contains("PROTOCOL_HAS_SPI"));
    QVERIFY(!header.contains("PROTOCOL_HAS_I2C"));
    QVERIFY(!header.contains("PROTOCOL_HAS_ADC"));
    QVERIFY(!header.contains("PROTOCOL_HAS_PWM"));

    // A fully resolvable configuration produces no skip diagnostics.
    for (const QString& w : res.warnings) {
        QVERIFY2(!w.contains("init not generated"), qPrintable(w));
    }
}

void TestExportValidation::testPlanHalProbeBMatchesConfigAndWarnsI2c() {
    FirmwareExportManager exportMgr;
    CanvasScene scene;
    Project project(&scene);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ExportResult res;
    QVERIFY(exportConfigTo(exportMgr, scene, project, probeBConfig(), dir.path(), &res));

    const QString cPath = QDir(dir.path()).filePath("generated/target/target_hal.c");
    const QString hPath = QDir(dir.path()).filePath("generated/target/target_hal.h");
    const QString content = readTextFile(cPath);
    QVERIFY(!content.isEmpty());

    QVERIFY(content.contains("RCC->AHBENR |= RCC_AHBENR_GPIOCEN;"));
    QVERIFY(content.contains("GPIOC->MODER = (GPIOC->MODER & ~(3u << (13u * 2u))) | (1u << (13u * 2u));"));
    QVERIFY(content.contains("GPIOC->BSRR = (1u << (13u + 16u)); /* initial output: low */"));
    QVERIFY(content.contains("USART1->BRR = 5000u;"));
    QVERIFY(content.contains("| (0u << (6u * 4u));"));
    QVERIFY(content.contains("| (0u << (7u * 4u));"));
    QVERIFY(!content.contains("USART1->BRR = 208u;"));

    // I2C1 on PB10/PB11 is not resolvable on STM32F030R8: no I2C code at all.
    QVERIFY(!content.contains("I2C1->"));
    QVERIFY(!content.contains("TIMINGR"));
    QVERIFY(!content.contains("hal_i2c1_init"));

    const QString header = readTextFile(hPath);
    QVERIFY(header.contains("#define PROTOCOL_HAS_UART 1 /* USART1 */"));
    QVERIFY(!header.contains("PROTOCOL_HAS_I2C"));
    QVERIFY(!header.contains("PIN_PB10"));

    QVERIFY2(warningsContain(res.warnings, "I2C1 init not generated", "PB10"),
             qPrintable(res.warnings.join(QStringLiteral(" | "))));
    QVERIFY2(warningsContain(res.warnings, "PB10", "no verified AF number"),
             qPrintable(res.warnings.join(QStringLiteral(" | "))));
}

void TestExportValidation::testPlanHalProbeAAndBDiffer() {
    FirmwareExportManager exportMgr;
    CanvasScene sceneA;
    Project projectA(&sceneA);
    QTemporaryDir dirA;
    QTemporaryDir dirB;
    QVERIFY(dirA.isValid());
    QVERIFY(dirB.isValid());

    ExportResult resA;
    QVERIFY(exportConfigTo(exportMgr, sceneA, projectA, probeAConfig(), dirA.path(), &resA));

    CanvasScene sceneB;
    Project projectB(&sceneB);
    ExportResult resB;
    QVERIFY(exportConfigTo(exportMgr, sceneB, projectB, probeBConfig(), dirB.path(), &resB));

    const QString a = readTextFile(QDir(dirA.path()).filePath("generated/target/target_hal.c"));
    const QString b = readTextFile(QDir(dirB.path()).filePath("generated/target/target_hal.c"));
    QVERIFY(!a.isEmpty());
    QVERIFY(!b.isEmpty());
    QVERIFY2(a != b, "configured probe A and probe B must not share the same generated HAL");

    QVERIFY(a.contains("USART1->BRR = 208u;"));
    QVERIFY(!a.contains("USART1->BRR = 5000u;"));
    QVERIFY(b.contains("USART1->BRR = 5000u;"));
    QVERIFY(!b.contains("USART1->BRR = 208u;"));

    QVERIFY(a.contains("GPIOA->AFR[1]"));
    QVERIFY(!a.contains("GPIOA->AFR[0]"));
    QVERIFY(b.contains("GPIOB->AFR[0]"));
}

void TestExportValidation::testPlanHalValidSpiI2cUartAndCompileCheck() {
    FirmwareExportManager exportMgr;
    CanvasScene scene;
    Project project(&scene);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F030R8";
    hw.family = "STM32";
    setFullClock(hw, 48);
    hw.pins.insert("PA2", makePinConfig("PA2", "AlternateFunction", "USART2_TX"));
    hw.pins.insert("PA3", makePinConfig("PA3", "AlternateFunction", "USART2_RX"));
    hw.pins.insert("PA5", makePinConfig("PA5", "AlternateFunction", "SPI1_SCK"));
    hw.pins.insert("PA6", makePinConfig("PA6", "AlternateFunction", "SPI1_MISO"));
    hw.pins.insert("PA7", makePinConfig("PA7", "AlternateFunction", "SPI1_MOSI"));
    hw.pins.insert("PB6", makePinConfig("PB6", "AlternateFunction", "I2C1_SCL"));
    hw.pins.insert("PB7", makePinConfig("PB7", "AlternateFunction", "I2C1_SDA"));
    hw.peripherals.insert("USART2", makePeripheralConfig(
        "USART2", "USART",
        QMap<QString, QString>{{"TX", "PA2"}, {"RX", "PA3"}},
        QMap<QString, QVariant>{{"baudRate", 9600}}));
    hw.peripherals.insert("SPI1", makePeripheralConfig(
        "SPI1", "SPI",
        QMap<QString, QString>{{"SCK", "PA5"}, {"MISO", "PA6"}, {"MOSI", "PA7"}},
        QMap<QString, QVariant>{{"clockSpeedHz", 8000000}}));
    hw.peripherals.insert("I2C1", makePeripheralConfig(
        "I2C1", "I2C",
        QMap<QString, QString>{{"SCL", "PB6"}, {"SDA", "PB7"}},
        QMap<QString, QVariant>{{"clockSpeedHz", 400000}}));

    ExportResult res;
    QVERIFY(exportConfigTo(exportMgr, scene, project, hw, dir.path(), &res));

    const QString cPath = QDir(dir.path()).filePath("generated/target/target_hal.c");
    const QString hPath = QDir(dir.path()).filePath("generated/target/target_hal.h");
    const QString content = readTextFile(cPath);
    QVERIFY(!content.isEmpty());

    QVERIFY(content.contains("RCC->APB1ENR |= RCC_APB1ENR_USART2EN;"));
    QVERIFY(content.contains("USART2->BRR = 5000u;"));
    QVERIFY(content.contains("RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;"));
    QVERIFY(content.contains("(2u << SPI_CR1_BR_Pos)"));
    QVERIFY(content.contains("SPI_CR1_SSM | SPI_CR1_SSI"));
    QVERIFY(content.contains("SPI1->CR1 |= SPI_CR1_SPE;"));
    QVERIFY(content.contains("RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;"));
    QVERIFY(content.contains("I2C1->TIMINGR = 0x53300309u;"));
    QVERIFY(content.contains("I2C1->CR1 |= I2C_CR1_PE;"));

    const QString header = readTextFile(hPath);
    QVERIFY(header.contains("#define PROTOCOL_HAS_UART 1 /* USART2 */"));
    QVERIFY(header.contains("#define PROTOCOL_HAS_SPI 1 /* SPI1 */"));
    QVERIFY(header.contains("#define PROTOCOL_HAS_I2C 1 /* I2C1 */"));
    QVERIFY(!header.contains("PROTOCOL_HAS_ADC"));
    QVERIFY(!header.contains("PROTOCOL_HAS_PWM"));

    // Requested 8 MHz is quantized to 6 MHz by the SPI prescaler (BR=2):
    // the adjustment must be reported, not silently ignored.
    QVERIFY2(warningsContain(res.warnings, "SPI1", "adjusted"),
             qPrintable(res.warnings.join(QStringLiteral(" | "))));

    // Compile the generated HAL against the real ST CMSIS device header when
    // the toolchain and headers are available; otherwise skip with a message.
    const QString cmsisDev = QStringLiteral(
        "/home/cherry/STM32Cube/Repository/STM32Cube_FW_F0_V1.11.6/Drivers/CMSIS/Device/ST/STM32F0xx/Include");
    const QString cmsisCore = QStringLiteral(
        "/home/cherry/STM32Cube/Repository/STM32Cube_FW_F0_V1.11.6/Drivers/CMSIS/Include");
    if (QStandardPaths::findExecutable("arm-none-eabi-gcc").isEmpty()
        || !QDir(cmsisDev).exists() || !QDir(cmsisCore).exists()) {
        QSKIP("arm-none-eabi-gcc or ST CMSIS headers unavailable; compile check skipped");
    }
    const QString objPath = QDir(dir.path()).filePath("target_hal_c1.o");
    QProcess proc;
    const QStringList args = {
        "-mcpu=cortex-m0", "-mthumb", "-std=c11", "-Wall", "-Wextra",
        "-DSTM32F030x8", "-c", cPath, "-o", objPath,
        "-I" + QDir(dir.path()).filePath("generated/target"),
        "-I" + cmsisDev,
        "-I" + cmsisCore,
    };
    proc.start("arm-none-eabi-gcc", args);
    QVERIFY(proc.waitForFinished(30000));
    QCOMPARE(proc.exitStatus(), QProcess::NormalExit);
    QVERIFY2(proc.exitCode() == 0, proc.readAllStandardError().constData());
    QVERIFY(QFile::exists(objPath));
}

void TestExportValidation::testPlanHalConflictedPinAndPeripheralSkippedWithErrors() {
    FirmwareExportManager exportMgr;
    CanvasScene scene;
    Project project(&scene);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F030R8";
    hw.family = "STM32";
    setFullClock(hw, 48);
    Hardware::PinConfiguration led = makePinConfig("PA5", "GPIO_Output");
    led.initialOutput = "High";
    hw.pins.insert("PA5", led);
    hw.pins.insert("PA9", makePinConfig("PA9", "AlternateFunction", "USART1_TX"));
    hw.pins.insert("PA10", makePinConfig("PA10", "AlternateFunction", "USART1_RX"));
    // USART1 TX points at PA5, which is configured as a GPIO output: conflict.
    hw.peripherals.insert("USART1", makePeripheralConfig(
        "USART1", "USART",
        QMap<QString, QString>{{"TX", "PA5"}, {"RX", "PA10"}},
        QMap<QString, QVariant>{{"baudRate", 9600}}));

    ExportResult res;
    QVERIFY(exportConfigTo(exportMgr, scene, project, hw, dir.path(), &res));

    const QString cPath = QDir(dir.path()).filePath("generated/target/target_hal.c");
    const QString content = readTextFile(cPath);
    QVERIFY(!content.isEmpty());

    // The conflicted pin and its peripheral are skipped entirely.
    QVERIFY(!content.contains("(5u * 2u)"));
    QVERIFY(!content.contains("hal_usart1_init"));
    QVERIFY(!content.contains("USART1->BRR"));
    // Unaffected pins are still configured (PA9 keeps its AF1 nibble).
    QVERIFY(content.contains("| (1u << ((9u - 8u) * 4u));"));
    // Never fall back to the template when a configuration is present.
    QVERIFY(!content.contains("115200"));
    QVERIFY(!content.contains("SCK="));

    QVERIFY2(warningsContain(res.warnings, "USART1 init not generated", "multiple functions"),
             qPrintable(res.warnings.join(QStringLiteral(" | "))));
    bool sawError = false;
    for (const QString& w : res.warnings) {
        if (w.startsWith(QStringLiteral("Error:"))) sawError = true;
    }
    QVERIFY2(sawError, qPrintable(res.warnings.join(QStringLiteral(" | "))));
}

void TestExportValidation::testPlanHalUnknownClockAssumptionInfo() {
    FirmwareExportManager exportMgr;
    CanvasScene scene;
    Project project(&scene);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F030R8";
    hw.family = "STM32";
    // No clock configuration at all: 8 MHz HSI reset default is assumed.
    hw.pins.insert("PA2", makePinConfig("PA2", "AlternateFunction", "USART2_TX"));
    hw.pins.insert("PA3", makePinConfig("PA3", "AlternateFunction", "USART2_RX"));
    hw.peripherals.insert("USART2", makePeripheralConfig(
        "USART2", "USART",
        QMap<QString, QString>{{"TX", "PA2"}, {"RX", "PA3"}},
        QMap<QString, QVariant>{{"baudRate", 9600}}));

    ExportResult res;
    QVERIFY(exportConfigTo(exportMgr, scene, project, hw, dir.path(), &res));

    const QString content = readTextFile(QDir(dir.path()).filePath("generated/target/target_hal.c"));
    QVERIFY(!content.isEmpty());

    // 8 MHz / 9600 = 833 (the same worked example as RM0360 Rev 5 sec 23.4.4).
    QVERIFY(content.contains("USART2->BRR = 833u;"));
    QVERIFY(content.contains("assuming the 8 MHz HSI reset default"));

    QVERIFY2(warningsContain(res.warnings, "USART2", "8 MHz HSI reset default"),
             qPrintable(res.warnings.join(QStringLiteral(" | "))));
}

void TestExportValidation::testPlanHalNoInterruptCode() {
    FirmwareExportManager exportMgr;
    CanvasScene scene;
    Project project(&scene);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F030R8";
    hw.family = "STM32";
    setFullClock(hw, 48);
    Hardware::PinConfiguration button = makePinConfig("PA0", "GPIO_Input");
    button.interrupt = "RisingEdge";
    hw.pins.insert("PA0", button);

    ExportResult res;
    QVERIFY(exportConfigTo(exportMgr, scene, project, hw, dir.path(), &res));

    const QString cPath = QDir(dir.path()).filePath("generated/target/target_hal.c");
    const QString hPath = QDir(dir.path()).filePath("generated/target/target_hal.h");
    const QString content = readTextFile(cPath);
    QVERIFY(!content.isEmpty());

    // C1 scope: no NVIC/EXTI/SYSCFG generation; the interrupt is reported.
    QVERIFY(!content.contains("EXTI"));
    QVERIFY(!content.contains("NVIC"));
    QVERIFY(!content.contains("SYSCFG"));
    QVERIFY(content.contains("strcmp(pin_name, \"PA0\")"));

    const QString header = readTextFile(hPath);
    QVERIFY(header.contains("#define PROTOCOL_HAS_GPIO 1"));
    QVERIFY(!header.contains("PROTOCOL_HAS_UART"));

    QVERIFY2(warningsContain(res.warnings, "PA0", "interrupt"),
             qPrintable(res.warnings.join(QStringLiteral(" | "))));
}

QTEST_MAIN(TestExportValidation)
#include "test_export_validation.moc"
