/**
 * Task 5 — Board & Display dock acceptance tests.
 *
 * Covers:
 *  - Old .euiproj files (without the optional "boardDisplay" block) load with
 *    defaults and the analyzer numbers are unchanged (full frame, single buffer).
 *  - Save/load round trip of all five BoardDisplayConfig fields.
 *  - Framebuffer formulas for RGB565/RGB888, full / partial single / partial
 *    double, including the 16-byte DMA row alignment.
 *  - STM32F030R8 (8 KB): full 320x240 RGB565 does not fit and suggests partial
 *    buffering; partial single (8 lines) fits.
 *  - STM32H745: fit is evaluated only against framebuffer-capable regions
 *    (AXI SRAM 512 KB), never against TCM / the total RAM.
 *  - STM32H742 + LTDC => Error in the Problems dock (no LCD controller).
 *  - Undocumented region suitability (usable_for_framebuffer = null) => Info,
 *    never an error, and the dock still reports "Fits (estimated)".
 *  - A dock edit refreshes the Resource Analyzer in well under 100 ms.
 *  - Resolution edits keep the canvas in sync.
 *  - DarkTheme-token-only colors.
 *  - Screenshots: F030 full (does not fit), H745 (fits), H742 + LTDC (error).
 */
#include <QTest>
#include <QObject>
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QRegularExpression>
#include <QSet>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QDockWidget>

#include "analyzer/CapabilityDatabase.h"
#include "analyzer/FramebufferFit.h"
#include "analyzer/ResourceAnalysis.h"
#include "analyzer/ResourceEstimator.h"
#include "canvas/CanvasScene.h"
#include "diagnostics/DiagnosticsService.h"
#include "diagnostics/DiagnosticsSources.h"
#include "models/BoardDisplayConfig.h"
#include "models/ButtonComponent.h"
#include "models/Screen.h"
#include "panels/BoardDisplayPanel.h"
#include "panels/ProblemsPanel.h"
#include "panels/ResourceAnalyzerPanel.h"
#include "project/Project.h"
#include "theme/Theme.h"
#include "MainWindow.h"

using Diagnostics::Diagnostic;

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
    if (cleanName.startsWith("screenshots/")) {
        cleanName = cleanName.mid(12);
    }
    QString shotPath = repoPath("screenshots/" + cleanName);
    QFileInfo(shotPath).dir().mkpath(".");
    return shotPath;
}

static void setTargetDevice(Project& project, const QString& deviceId) {
    Hardware::HardwareConfig hw = project.hardwareConfig();
    hw.deviceId = deviceId;
    hw.boardId.clear();
    hw.targetType = "device";
    project.setHardwareConfig(hw);
}

class TestBoardDisplayDock : public QObject {
    Q_OBJECT

private slots:
    void testOldProjectLoadsWithDefaults();
    void testSaveLoadRoundTrip();
    void testFramebufferFormulas();
    void testF030FullDoesNotFitPartialFits();
    void testH745FitUsesOnlyFramebufferCapableRegions();
    void testH742LtdcProducesErrorInProblemsDock();
    void testUnknownRegionSuitabilityIsInfoNotError();
    void testDockChangeRefreshesAnalyzerUnder100ms();
    void testDockSyncsCanvasResolutionAndTargetSummary();
    void testPanelUsesThemeColorsOnly();
    void testCaptureScreenshots();
};

// ─────────────────────────────────────────────────────────────────────────────
// 1. Old project files load unchanged
// ─────────────────────────────────────────────────────────────────────────────
void TestBoardDisplayDock::testOldProjectLoadsWithDefaults() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    // Build a project the way the pre-Task-5 version would have saved it:
    // serialize, then strip the new optional block.
    QString legacyPath = tmp.filePath("legacy.euiproj");
    {
        CanvasScene scene;
        Project project(&scene);
        project.newProject("LegacyProject", 320, 240);
        project.setTargetFramework("lvgl");
        setTargetDevice(project, "STM32F030R8");

        QJsonObject root = project.toJson();
        QVERIFY2(root.contains("boardDisplay"), "new saves should persist the optional block");
        root.remove("boardDisplay");
        QFile f(legacyPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    }

    CanvasScene scene;
    Project loaded(&scene);
    QVERIFY(loaded.loadFromFile(legacyPath));

    // Absence => documented defaults that reproduce the old analyzer behaviour.
    const BoardDisplayConfig config = loaded.boardDisplayConfig();
    QCOMPARE(static_cast<int>(config.framebufferMode),
             static_cast<int>(BoardDisplayConfig::FramebufferMode::Full));
    QCOMPARE(config.partialBufferLines, 8);
    QVERIFY(config.interfaceId.isEmpty());
    QVERIFY(config.controllerId.isEmpty());
    QVERIFY(config.touchDriverId.isEmpty());

    // Analyzer numbers are unchanged: 320 x 240 RGB565 full frame = 153,600 B.
    ResourceEstimator estimator;
    const ResourceAnalysis analysis = estimator.analyze(&loaded, "STM32F030R8");
    QCOMPARE(analysis.breakdown.framebuffer_ram, 153600u);

    // Control project with the same defaults must produce identical totals.
    CanvasScene controlScene;
    Project control(&controlScene);
    control.newProject("LegacyProject", 320, 240);
    control.setTargetFramework("lvgl");
    setTargetDevice(control, "STM32F030R8");
    ResourceEstimator controlEstimator;
    const ResourceAnalysis controlAnalysis = controlEstimator.analyze(&control, "STM32F030R8");
    QCOMPARE(analysis.ram_used, controlAnalysis.ram_used);
    QCOMPARE(analysis.flash_used, controlAnalysis.flash_used);
    QCOMPARE(analysis.breakdown.framebuffer_ram, controlAnalysis.breakdown.framebuffer_ram);

    // 125 x 100 RGB565 keeps the pre-existing DMA-aligned math (25,600 B).
    DisplayConfig square = loaded.displayConfig();
    square.width = 125;
    square.height = 100;
    square.colorDepth = 16;
    loaded.setDisplayConfig(square);
    ResourceEstimator squareEstimator;
    const ResourceAnalysis squareAnalysis = squareEstimator.analyze(&loaded, "STM32F030R8");
    QCOMPARE(squareAnalysis.breakdown.framebuffer_ram, 25600u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. Save / load round trip
// ─────────────────────────────────────────────────────────────────────────────
void TestBoardDisplayDock::testSaveLoadRoundTrip() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.filePath("roundtrip.euiproj");

    BoardDisplayConfig saved;
    saved.framebufferMode = BoardDisplayConfig::FramebufferMode::PartialDouble;
    saved.partialBufferLines = 12;
    saved.interfaceId = "LTDC";
    saved.controllerId = "ST7789";
    saved.touchDriverId = "XPT2046";

    {
        CanvasScene scene;
        Project project(&scene);
        project.newProject("RoundTrip", 480, 272);
        project.setBoardDisplayConfig(saved);
        QVERIFY(project.saveToFile(path));
    }

    CanvasScene scene;
    Project loaded(&scene);
    QVERIFY(loaded.loadFromFile(path));

    const BoardDisplayConfig restored = loaded.boardDisplayConfig();
    QVERIFY2(restored == saved, qPrintable(
        QString("mode=%1 lines=%2 iface=%3 ctrl=%4 touch=%5")
            .arg(static_cast<int>(restored.framebufferMode))
            .arg(restored.partialBufferLines)
            .arg(restored.interfaceId, restored.controllerId, restored.touchDriverId)));

    // And the JSON uses the documented optional keys.
    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    QVERIFY2(root.contains("boardDisplay"), "boardDisplay block missing from saved file");
    const QJsonObject bd = root.value("boardDisplay").toObject();
    QCOMPARE(bd.value("framebufferMode").toString(), QString("partialDouble"));
    QCOMPARE(bd.value("partialBufferLines").toInt(), 12);
    QCOMPARE(bd.value("interface").toString(), QString("LTDC"));
    QCOMPARE(bd.value("controller").toString(), QString("ST7789"));
    QCOMPARE(bd.value("touchDriver").toString(), QString("XPT2046"));

    // Persisted through the analyzer too: 480 x 12 x 2 x 2 = 23,040 B.
    ResourceEstimator estimator;
    const ResourceAnalysis analysis = estimator.analyze(&loaded, "STM32H745");
    QCOMPARE(analysis.breakdown.framebuffer_ram, 23040u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. Formulas
// ─────────────────────────────────────────────────────────────────────────────
void TestBoardDisplayDock::testFramebufferFormulas() {
    BoardDisplayConfig full; // defaults: Full frame
    const FramebufferFit::Metrics fullMetrics =
        FramebufferFit::computeMetrics(320, 240, 16, full);
    QCOMPARE(fullMetrics.bufferHeight, 240);
    QCOMPARE(fullMetrics.bufferCount, 1);
    QCOMPARE(fullMetrics.bytes, 153600u);
    const QString fullFormula = FramebufferFit::formulaText(fullMetrics);
    QVERIFY2(fullFormula.contains("320") && fullFormula.contains("240"),
             qPrintable(fullFormula));
    QVERIFY2(fullFormula.contains("2 B/px"), qPrintable(fullFormula));
    QVERIFY2(fullFormula.contains("153,600 B"), qPrintable(fullFormula));
    QVERIFY2(fullFormula.contains("full frame"), qPrintable(fullFormula));

    BoardDisplayConfig single;
    single.framebufferMode = BoardDisplayConfig::FramebufferMode::PartialSingle;
    single.partialBufferLines = 8;
    const FramebufferFit::Metrics singleMetrics =
        FramebufferFit::computeMetrics(320, 240, 16, single);
    QCOMPARE(singleMetrics.bufferHeight, 8);
    QCOMPARE(singleMetrics.bufferCount, 1);
    QCOMPARE(singleMetrics.bytes, 5120u);
    const QString singleFormula = FramebufferFit::formulaText(singleMetrics);
    QVERIFY2(singleFormula.contains("8 lines"), qPrintable(singleFormula));
    QVERIFY2(singleFormula.contains("5,120 B"), qPrintable(singleFormula));
    QVERIFY2(singleFormula.contains("partial buffer, single"), qPrintable(singleFormula));

    BoardDisplayConfig doubleBuffer = single;
    doubleBuffer.framebufferMode = BoardDisplayConfig::FramebufferMode::PartialDouble;
    const FramebufferFit::Metrics doubleMetrics =
        FramebufferFit::computeMetrics(320, 240, 16, doubleBuffer);
    QCOMPARE(doubleMetrics.bufferCount, 2);
    QCOMPARE(doubleMetrics.bytes, 10240u);
    const QString doubleFormula = FramebufferFit::formulaText(doubleMetrics);
    QVERIFY2(doubleFormula.contains("× 2"), qPrintable(doubleFormula));
    QVERIFY2(doubleFormula.contains("10,240 B"), qPrintable(doubleFormula));
    QVERIFY2(doubleFormula.contains("partial buffer, double"), qPrintable(doubleFormula));

    // RGB888: 3 bytes/px => 320 x 240 x 3 = 230,400 B.
    const FramebufferFit::Metrics rgb888 =
        FramebufferFit::computeMetrics(320, 240, 24, full);
    QCOMPARE(rgb888.bytes, 230400u);
    QVERIFY2(FramebufferFit::formulaText(rgb888).contains("3 B/px"),
             qPrintable(FramebufferFit::formulaText(rgb888)));

    // Sub-byte/misaligned widths keep the DMA stride visible.
    const FramebufferFit::Metrics narrow =
        FramebufferFit::computeMetrics(125, 100, 16, full);
    QCOMPARE(narrow.bytes, 25600u);
    const QString narrowFormula = FramebufferFit::formulaText(narrow);
    QVERIFY2(narrowFormula.contains("row stride 250 B → 256 B"), qPrintable(narrowFormula));

    // Full frame ignores the stored partial-line count (legacy compatibility).
    BoardDisplayConfig fullWithLines;
    fullWithLines.partialBufferLines = 37;
    const FramebufferFit::Metrics fullWithLinesMetrics =
        FramebufferFit::computeMetrics(320, 240, 16, fullWithLines);
    QCOMPARE(fullWithLinesMetrics.bytes, 153600u);

    // Partial lines are clamped to the display height.
    BoardDisplayConfig overLong = single;
    overLong.partialBufferLines = 1000;
    const FramebufferFit::Metrics clamped =
        FramebufferFit::computeMetrics(320, 240, 16, overLong);
    QCOMPARE(clamped.bufferHeight, 240);
    QCOMPARE(clamped.bytes, 153600u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. STM32F030R8 (8 KB RAM) — the documented example
// ─────────────────────────────────────────────────────────────────────────────
void TestBoardDisplayDock::testF030FullDoesNotFitPartialFits() {
    CapabilityDatabase db;
    QVERIFY(db.loadBuiltinDatabase());
    const DeviceProfile f030 = db.getDeviceProfile("STM32F030R8");
    QCOMPARE(f030.ram_kb, 8u);
    QCOMPARE(f030.has_lcd_controller, 0);

    BoardDisplayConfig full;
    const FramebufferFit::Result fullResult =
        FramebufferFit::evaluate(f030, 320, 240, 16, full);
    QVERIFY2(fullResult.doesNotFit(),
             qPrintable(QString("F030 full must not fit: %1").arg(fullResult.message)));
    QCOMPARE(fullResult.bytes, 153600u);
    QVERIFY2(fullResult.hint.contains("partial", Qt::CaseInsensitive), qPrintable(fullResult.hint));

    BoardDisplayConfig partial;
    partial.framebufferMode = BoardDisplayConfig::FramebufferMode::PartialSingle;
    partial.partialBufferLines = 8;
    const FramebufferFit::Result partialResult =
        FramebufferFit::evaluate(f030, 320, 240, 16, partial);
    QVERIFY2(partialResult.fits(),
             qPrintable(QString("F030 partial single must fit: %1").arg(partialResult.message)));
    QCOMPARE(partialResult.bytes, 5120u);
    QVERIFY2(partialResult.regionSuitabilityUnknown,
             "F030R8 has null usable_for_framebuffer, so the suitability caveat must be set");

    // 32 partial lines (20,480 B) exceed the 8 KB total again.
    BoardDisplayConfig tooManyLines = partial;
    tooManyLines.partialBufferLines = 32;
    const FramebufferFit::Result tooManyResult =
        FramebufferFit::evaluate(f030, 320, 240, 16, tooManyLines);
    QVERIFY(tooManyResult.doesNotFit());

    // UI level, without a MainWindow: the dock mirrors the same verdicts.
    CanvasScene scene;
    Project project(&scene);
    project.newProject("F030Dock", 320, 240);
    setTargetDevice(project, "STM32F030R8");

    BoardDisplayPanel panel;
    panel.setProject(&project);
    QCOMPARE(panel.currentConfig().framebufferMode,
             BoardDisplayConfig::FramebufferMode::Full);
    QVERIFY2(panel.fitStatusText().contains("Does not fit"), qPrintable(panel.fitStatusText()));
    QVERIFY2(panel.fitHintText().contains("partial", Qt::CaseInsensitive),
             qPrintable(panel.fitHintText()));
    QVERIFY2(panel.formulaText().contains("153,600 B"), qPrintable(panel.formulaText()));

    panel.setFramebufferMode(BoardDisplayConfig::FramebufferMode::PartialSingle);
    QCOMPARE(project.boardDisplayConfig().framebufferMode,
             BoardDisplayConfig::FramebufferMode::PartialSingle);
    QVERIFY2(panel.fitStatusText().contains("Fits (estimated)"),
             qPrintable(panel.fitStatusText()));
    QVERIFY2(panel.sizeText().contains("5,120 B"), qPrintable(panel.sizeText()));
    QVERIFY2(panel.fitHintText().contains("Region suitability not documented; cannot confirm"),
             qPrintable(panel.fitHintText()));

    panel.setPartialBufferLines(8);
    QCOMPARE(project.boardDisplayConfig().partialBufferLines, 8);
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. STM32H745 — only framebuffer-capable regions count
// ─────────────────────────────────────────────────────────────────────────────
void TestBoardDisplayDock::testH745FitUsesOnlyFramebufferCapableRegions() {
    CapabilityDatabase db;
    QVERIFY(db.loadBuiltinDatabase());
    const DeviceProfile h745 = db.getDeviceProfile("STM32H745");
    QCOMPARE(h745.ram_kb, 1056u);
    QCOMPARE(h745.has_lcd_controller, 1);

    BoardDisplayConfig full;
    const FramebufferFit::Result fit =
        FramebufferFit::evaluate(h745, 320, 240, 16, full);
    QVERIFY2(fit.fits(), qPrintable(fit.message));
    QCOMPARE(fit.framebufferCapableBytes, 512u * 1024u);   // AXI SRAM only
    QVERIFY2(fit.framebufferCapableBytes != h745.ram_kb * 1024u,
             "must not use the full 1056 KB total (TCM is not framebuffer capable)");

    // 800 x 480 RGB565 full = 768,000 B: below the 1056 KB total but above the
    // 512 KB framebuffer-capable AXI SRAM => must not fit.
    const FramebufferFit::Result big =
        FramebufferFit::evaluate(h745, 800, 480, 16, full);
    QCOMPARE(big.bytes, 768000u);
    QVERIFY2(big.doesNotFit(), qPrintable(big.message));
    QVERIFY2(big.bytes < h745.ram_kb * 1024u,
             "the test vector must sit between capable regions and total RAM");

    // The TCM regions are documented usable=false and must be excluded.
    uint32_t excluded = 0;
    for (const DeviceProfile::RamRegion& region : h745.ram_regions) {
        if (region.usable_for_framebuffer == 0) excluded += region.kb;
    }
    QVERIFY2(excluded > 0, "H745 data should document non-capable TCM regions");
    QCOMPARE(fit.framebufferCapableBytes + 0u, 512u * 1024u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 6. STM32H742 + LTDC => Error in the Problems dock
// ─────────────────────────────────────────────────────────────────────────────
void TestBoardDisplayDock::testH742LtdcProducesErrorInProblemsDock() {
    CapabilityDatabase db;
    QVERIFY(db.loadBuiltinDatabase());
    QCOMPARE(db.getDeviceProfile("STM32H742").has_lcd_controller, 0);

    // Service level first.
    CanvasScene scene;
    Project project(&scene);
    project.newProject("H742Ltdc", 320, 240);
    setTargetDevice(project, "STM32H742");

    Diagnostics::DiagnosticsService service;
    Diagnostics::DiagnosticsSources::registerBuiltinSources(service);
    service.setProject(&project);

    service.runNow();
    bool errorBefore = false;
    for (const Diagnostic& d : service.diagnostics()) {
        if (d.source == "BoardDisplay" && d.severity == Diagnostic::Severity::Error) {
            errorBefore = true;
        }
    }
    QVERIFY2(!errorBefore, "no LTDC error without an LTDC interface");

    BoardDisplayConfig config = project.boardDisplayConfig();
    config.interfaceId = "LTDC";
    project.setBoardDisplayConfig(config);

    service.runNow();
    bool errorFound = false;
    for (const Diagnostic& d : service.diagnostics()) {
        if (d.source == "BoardDisplay" && d.severity == Diagnostic::Severity::Error) {
            errorFound = true;
            QVERIFY2(d.message.contains("LCD controller"), qPrintable(d.message));
            QVERIFY2(d.message.contains("STM32H742"), qPrintable(d.message));
            QVERIFY2(d.message.contains("LTDC"), qPrintable(d.message));
        }
    }
    QVERIFY2(errorFound, "H742 + LTDC must raise a BoardDisplay Error");

    // And the same Error reaches the Problems dock through MainWindow.
    MainWindow win;
    win.show();
    QApplication::processEvents();

    Project* winProject = win.currentProject();
    QVERIFY(winProject);
    winProject->newProject("H742LtdcWin", 320, 240);
    setTargetDevice(*winProject, "STM32H742");

    BoardDisplayPanel* panel = win.boardDisplayPanel();
    QVERIFY2(panel, "boardDisplayPanel() accessor missing");
    panel->setInterfaceId("LTDC");
    QApplication::processEvents();
    QTest::qWait(30);

    ProblemsPanel* problems = win.problemsPanel();
    QVERIFY(problems);
    QVERIFY2(problems->countForSeverity(Diagnostic::Severity::Error) >= 1,
             "Problems dock must show at least the LTDC Error");
}

// ─────────────────────────────────────────────────────────────────────────────
// 7. Unknown region suitability => Info, never Error
// ─────────────────────────────────────────────────────────────────────────────
void TestBoardDisplayDock::testUnknownRegionSuitabilityIsInfoNotError() {
    CanvasScene scene;
    Project project(&scene);
    project.newProject("UnknownRegions", 320, 240);
    setTargetDevice(project, "STM32F030R8");

    // Full frame: does not fit => Warning (not an Error).
    Diagnostics::DiagnosticsService service;
    Diagnostics::DiagnosticsSources::registerBuiltinSources(service);
    service.setProject(&project);
    service.runNow();

    int warningCount = 0;
    for (const Diagnostic& d : service.diagnostics()) {
        if (d.source != "BoardDisplay") continue;
        QVERIFY2(d.severity != Diagnostic::Severity::Error,
                 qPrintable("Framebuffer fit problems must never be Errors: " + d.message));
        if (d.severity == Diagnostic::Severity::Warning &&
            d.message.contains("Framebuffer")) {
            warningCount++;
        }
    }
    QVERIFY2(warningCount >= 1, "full framebuffer on F030 must produce a Warning");

    // Partial single fits the total RAM, but region suitability is undocumented:
    // exactly the Info notice, never an error.
    BoardDisplayConfig config = project.boardDisplayConfig();
    config.framebufferMode = BoardDisplayConfig::FramebufferMode::PartialSingle;
    config.partialBufferLines = 8;
    project.setBoardDisplayConfig(config);
    service.runNow();

    bool infoFound = false;
    for (const Diagnostic& d : service.diagnostics()) {
        if (d.source != "BoardDisplay") continue;
        QVERIFY2(d.severity != Diagnostic::Severity::Error,
                 qPrintable("unknown region suitability must not be an Error: " + d.message));
        if (d.severity == Diagnostic::Severity::Info &&
            d.message.contains("Region suitability not documented; cannot confirm")) {
            infoFound = true;
        }
    }
    QVERIFY2(infoFound, "unknown region suitability must produce the documented Info notice");
}

// ─────────────────────────────────────────────────────────────────────────────
// 8. Dock edit refreshes the analyzer in < 100 ms
// ─────────────────────────────────────────────────────────────────────────────
void TestBoardDisplayDock::testDockChangeRefreshesAnalyzerUnder100ms() {
    MainWindow win;
    win.resize(1500, 950);
    win.show();
    QApplication::processEvents();

    Project* project = win.currentProject();
    QVERIFY(project);
    project->newProject("PerfDock", 320, 240);
    setTargetDevice(*project, "STM32F030R8");
    QApplication::processEvents();

    ResourceAnalyzerPanel* analyzer = win.resourceAnalyzerPanel();
    QVERIFY(analyzer);
    QVERIFY(analyzer->lastAnalysis().has_device);
    QCOMPARE(analyzer->lastAnalysis().device_name, QString("STM32F030R8"));
    QCOMPARE(analyzer->lastAnalysis().breakdown.framebuffer_ram, 153600u);

    BoardDisplayPanel* panel = win.boardDisplayPanel();
    QVERIFY(panel);

    QElapsedTimer timer;
    timer.start();
    panel->setFramebufferMode(BoardDisplayConfig::FramebufferMode::PartialSingle);
    panel->setPartialBufferLines(8);
    QApplication::processEvents();
    const qint64 elapsed = timer.elapsed();

    QVERIFY2(elapsed < 100, qPrintable(QString("dock refresh took %1 ms").arg(elapsed)));
    QCOMPARE(project->boardDisplayConfig().framebufferMode,
             BoardDisplayConfig::FramebufferMode::PartialSingle);
    QCOMPARE(analyzer->lastAnalysis().breakdown.framebuffer_ram, 5120u);

    // Back to full frame: the legacy number returns immediately as well.
    panel->setFramebufferMode(BoardDisplayConfig::FramebufferMode::Full);
    QApplication::processEvents();
    QCOMPARE(analyzer->lastAnalysis().breakdown.framebuffer_ram, 153600u);

    // The diagnostics wiring also picked the change up (debounced refresh).
    QTest::qWait(40);
    bool infoFound = false;
    for (const Diagnostic& d : win.diagnosticsService()->diagnostics()) {
        if (d.source == "BoardDisplay" &&
            d.severity == Diagnostic::Severity::Warning &&
            d.message.contains("Framebuffer")) {
            infoFound = true;
        }
    }
    QVERIFY2(infoFound, "F030 + full frame must surface the fit Warning in the Problems service");
}

// ─────────────────────────────────────────────────────────────────────────────
// 9. Canvas sync + target summary
// ─────────────────────────────────────────────────────────────────────────────
void TestBoardDisplayDock::testDockSyncsCanvasResolutionAndTargetSummary() {
    CanvasScene scene;
    Project project(&scene);
    project.newProject("CanvasSync", 320, 240);
    setTargetDevice(project, "STM32F030R8");

    BoardDisplayPanel panel;
    panel.setProject(&project);

    panel.setDisplaySize(480, 272);
    QCOMPARE(project.displayConfig().width, 480);
    QCOMPARE(project.displayConfig().height, 272);
    QCOMPARE(scene.displayRect().width(), 480.0);
    QCOMPARE(scene.displayRect().height(), 272.0);

    // Partial buffer height is clamped when the display shrinks.
    panel.setFramebufferMode(BoardDisplayConfig::FramebufferMode::PartialSingle);
    panel.setPartialBufferLines(16);
    panel.setDisplaySize(480, 4);
    QCOMPARE(project.boardDisplayConfig().partialBufferLines, 4);

    // No supported-format mismatch is silently rewritten.
    panel.setPixelFormat("RGB888", 24);
    QCOMPARE(project.displayConfig().colorFormat, QString("RGB888"));
    QCOMPARE(project.displayConfig().colorDepth, 24);
    panel.setPixelFormat("RGB565", 16);
    QCOMPARE(project.displayConfig().colorFormat, QString("RGB565"));
    QCOMPARE(project.displayConfig().colorDepth, 16);

    // Target summary mirrors the capability database entry.
    auto* summary = panel.findChild<QLabel*>("bdTargetSummary");
    QVERIFY(summary);
    QVERIFY2(summary->text().contains("STM32F030R8"), qPrintable(summary->text()));
    QVERIFY2(summary->text().contains("48 MHz"), qPrintable(summary->text()));
    QVERIFY2(summary->text().contains("8 KB"), qPrintable(summary->text()));
    QVERIFY2(summary->text().contains("LCD controller:</b> No"), qPrintable(summary->text()));

    // Region table: F030R8 has one region ("SRAM") with unknown suitability.
    auto* table = panel.findChild<QTableWidget*>("bdRegionsTable");
    QVERIFY(table);
    QCOMPARE(table->rowCount(), 1);
    QCOMPARE(table->item(0, 0)->text(), QString("SRAM"));
    QCOMPARE(table->item(0, 2)->text(), QString("Unknown"));

    // H745: AXI SRAM is framebuffer-capable, TCM is not.
    setTargetDevice(project, "STM32H745");
    panel.refreshFromProject();
    QVERIFY(table->rowCount() >= 2);
    bool sawYes = false, sawNo = false;
    for (int row = 0; row < table->rowCount(); ++row) {
        if (table->item(row, 2)->text() == "Yes") sawYes = true;
        if (table->item(row, 2)->text() == "No") sawNo = true;
    }
    QVERIFY2(sawYes, "AXI SRAM must be shown as framebuffer-capable");
    QVERIFY2(sawNo, "TCM regions must be shown as not framebuffer-capable");
}

// ─────────────────────────────────────────────────────────────────────────────
// 10. Theme tokens only
// ─────────────────────────────────────────────────────────────────────────────
void TestBoardDisplayDock::testPanelUsesThemeColorsOnly() {
    const QSet<QString> themeColors = {
        QString(Theme::BgBase).toUpper(),        QString(Theme::BgSurface).toUpper(),
        QString(Theme::BgRaised).toUpper(),      QString(Theme::BgOverlay).toUpper(),
        QString(Theme::BorderSubtle).toUpper(),  QString(Theme::BorderStrong).toUpper(),
        QString(Theme::Accent).toUpper(),        QString(Theme::AccentHover).toUpper(),
        QString(Theme::AccentPressed).toUpper(), QString(Theme::AccentSoft).toUpper(),
        QString(Theme::AccentText).toUpper(),    QString(Theme::TextPrimary).toUpper(),
        QString(Theme::TextSecondary).toUpper(), QString(Theme::TextMuted).toUpper(),
        QString(Theme::TextDisabled).toUpper(),  QString(Theme::Success).toUpper(),
        QString(Theme::Warning).toUpper(),       QString(Theme::Danger).toUpper(),
        QString(Theme::Info).toUpper(),          QString(Theme::SuccessBg).toUpper(),
        QString(Theme::WarningBg).toUpper(),     QString(Theme::DangerBg).toUpper(),
        QString(Theme::InfoBg).toUpper(),        QString(Theme::LiveTestActive).toUpper(),
        QString(Theme::CanvasBg).toUpper()
    };

    QRegularExpression hexRe("#([0-9A-Fa-f]{8}|[0-9A-Fa-f]{6}|[0-9A-Fa-f]{3})(?![0-9A-Za-z])");

    CanvasScene scene;
    Project project(&scene);
    project.newProject("ThemeDock", 320, 240);
    setTargetDevice(project, "STM32F030R8");

    BoardDisplayPanel panel;
    panel.setProject(&project);
    panel.setFramebufferMode(BoardDisplayConfig::FramebufferMode::PartialSingle); // exercise preview styles

    QStringList offenders;
    QList<QWidget*> widgets = panel.findChildren<QWidget*>();
    widgets.prepend(&panel);
    for (QWidget* w : widgets) {
        const QString stylesheet = w->styleSheet();
        if (stylesheet.isEmpty()) continue;
        auto it = hexRe.globalMatch(stylesheet);
        while (it.hasNext()) {
            const QString hex = it.next().captured(0).toUpper();
            if (!themeColors.contains(hex)) {
                offenders << QString("%1.%2 uses hardcoded color %3")
                                 .arg(w->metaObject()->className(), w->objectName(), hex);
            }
        }
    }
    QVERIFY2(offenders.isEmpty(), qPrintable(offenders.join("; ")));
}

// ─────────────────────────────────────────────────────────────────────────────
// 11. Screenshots
// ─────────────────────────────────────────────────────────────────────────────
void TestBoardDisplayDock::testCaptureScreenshots() {
    MainWindow win;
    win.resize(1600, 1050);
    win.show();
    QApplication::processEvents();

    Project* project = win.currentProject();
    QVERIFY(project);
    project->newProject("Task5Screens", 320, 240);

    BoardDisplayPanel* panel = win.boardDisplayPanel();
    QVERIFY(panel);

    auto* dock = win.findChild<QDockWidget*>("boardDisplayDock");
    QVERIFY2(dock, "boardDisplayDock not found (dock not registered?)");

    // Give the dock enough vertical room that the Buffering section (with the
    // fit verdict) is part of every screenshot.
    dock->show();
    dock->raise();
    // Collapse the upper-right tab group (Properties / Effects / Prototype) so
    // the Board & Display dock can use the full column height.
    for (const QString& dockName : {QStringLiteral("propertiesDock"),
                                    QStringLiteral("effectsDock"),
                                    QStringLiteral("prototypeDock")}) {
        if (auto* other = win.findChild<QDockWidget*>(dockName)) {
            other->hide();
        }
    }
    win.resizeDocks({dock}, {980}, Qt::Vertical);
    QApplication::processEvents();

    // The offscreen platform needs a few event-loop turns for dock resizes and
    // the regions table to settle before a grab reflects the final layout.
    auto settleUi = []() {
        for (int i = 0; i < 3; ++i) {
            QApplication::processEvents();
            QTest::qWait(20);
        }
        QApplication::processEvents();
    };

    // 1. F030R8 + 320x240 RGB565 full frame => red "Does not fit".
    panel->setTargetDevice("STM32F030R8");
    panel->setFramebufferMode(BoardDisplayConfig::FramebufferMode::Full);
    panel->setDisplaySize(320, 240);
    settleUi();
    QVERIFY2(panel->fitStatusText().contains("Does not fit"), qPrintable(panel->fitStatusText()));

    // Flush diagnostics so the Problems dock shows the Task-5 Warning too.
    win.diagnosticsService()->runNow();
    settleUi();

    dock->show();
    dock->raise();
    settleUi();
    QVERIFY(win.grab().save(screenshotPath("task5_board_display_f030_full.png")));
    QVERIFY(panel->grab().save(screenshotPath("task5_board_display_f030_panel.png")));

    // 2. H745 + full frame => green "Fits (estimated)".
    panel->setTargetDevice("STM32H745");
    settleUi();
    QVERIFY2(panel->fitStatusText().contains("Fits (estimated)"), qPrintable(panel->fitStatusText()));
    win.diagnosticsService()->runNow();
    settleUi();
    QVERIFY(win.grab().save(screenshotPath("task5_board_display_h745_fits.png")));
    QVERIFY(panel->grab().save(screenshotPath("task5_board_display_h745_panel.png")));

    // 3. H742 + LTDC => Error in the Problems dock.
    panel->setTargetDevice("STM32H742");
    panel->setInterfaceId("LTDC");
    QTest::qWait(40);

    win.diagnosticsService()->runNow();
    ProblemsPanel* problems = win.problemsPanel();
    QVERIFY(problems);
    QVERIFY2(problems->countForSeverity(Diagnostic::Severity::Error) >= 1,
             "H742 + LTDC must appear as an Error in the Problems dock");

    auto* problemsDock = win.findChild<QDockWidget*>("problemsDock");
    QVERIFY(problemsDock);
    problemsDock->show();
    problemsDock->raise();
    settleUi();
    QVERIFY(win.grab().save(screenshotPath("task5_board_display_h742_ltdc_error.png")));

    // The panel itself is grabbed too, keeping the error context visible.
    dock->show();
    dock->raise();
    settleUi();
    QVERIFY(panel->grab().save(screenshotPath("task5_board_display_h742_panel.png")));
}

QTEST_MAIN(TestBoardDisplayDock)
#include "test_board_display_dock.moc"
