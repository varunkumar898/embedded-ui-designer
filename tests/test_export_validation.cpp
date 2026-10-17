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
#include "dialogs/ExportDialog.h"

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

    // Locate the JSON snapshot in the repository (build dir is inside the repo).
    QDir dir(QCoreApplication::applicationDirPath());
    QString jsonPath;
    for (int i = 0; i < 6; ++i) {
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

    qDebug() << "\n=== EMPTY-FOLDER EXPORT TEST FOR ALL DATABASE DEVICES ===";
    qDebug() << "Total devices to test:" << devices.size();

    int f030LeakageCount = 0;
    for (const QString& devName : devices) {
        CanvasScene scene;
        Project project(&scene);
        project.setProjectName("App_" + devName);

        Hardware::HardwareConfig hw;
        hw.deviceId = devName;
        hw.targetType = "device";
        // Also get family from profile if available
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
        QVERIFY(res.success);

        // Scan all generated files for F030R8-specific strings
        QStringList f030Matches;
        QDirIterator it(tempDir.path(), QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();
            QFile f(filePath);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString content = QString::fromUtf8(f.readAll());
                QString relPath = QDir(tempDir.path()).relativeFilePath(filePath);
                if (content.contains("STM32F030", Qt::CaseInsensitive) ||
                    content.contains("F030R8", Qt::CaseInsensitive) ||
                    content.contains("NUCLEO-F030R8", Qt::CaseInsensitive) ||
                    content.contains("nucleo_f030r8", Qt::CaseInsensitive)) {
                    f030Matches.append(relPath);
                }
            }
        }

        if (devName == "STM32F030R8") {
            qDebug() << "Device:" << devName << "(baseline) -> expected F030 matches in:" << f030Matches;
        } else if (!f030Matches.isEmpty()) {
            f030LeakageCount++;
            qDebug() << "LEAK DETECTED for device:" << devName << "in files:" << f030Matches;
        } else {
            qDebug() << "Clean device (no F030 leakage):" << devName;
        }
    }
    qDebug() << "Summary: devices with F030 leakage:" << f030LeakageCount << "/" << (devices.size() - 1);
}

QTEST_MAIN(TestExportValidation)
#include "test_export_validation.moc"
