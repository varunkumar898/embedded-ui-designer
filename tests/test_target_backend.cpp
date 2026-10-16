/**
 * Task 6 - Target Backend selector + widget support matrix acceptance tests.
 *
 * Covers:
 *  - BackendRegistry lists only real generators plus the planned native entry,
 *    which is disabled with the "Not implemented yet." tooltip and can never be
 *    selected (dock combo and export dialog).
 *  - Backend selection persists through the existing Project::targetFramework
 *    field ("target" JSON key) - no second framework setting is introduced -
 *    and round-trips through save/load.
 *  - Old projects load with the same effective backend as before (including
 *    legacy spellings such as "qul"/"qtmcu" handled exactly like the export
 *    dispatch).
 *  - Unsupported/partial/unknown components produce the documented diagnostics
 *    per backend, reachable in the Problems dock with screen + component
 *    location.
 *  - Switching the backend changes the analyzer baseline and refreshes in
 *    well under 100 ms.
 *  - Generator output regression: exports stay byte-identical to the direct
 *    generator output and the canonical framework id produces the same bytes
 *    as the legacy export-dialog id for Qt for MCUs.
 *  - Screenshots: backend selector in the dock, Backend Error in the Problems
 *    dock, export dialog backend combo.
 *  - Export gate: backend Warnings confirm through a single Task 1-style
 *    warning dialog; backend Errors (EmbedWeb + unsupported component) block
 *    the export before any output is written.
 */
#include <QTest>
#include <QAbstractButton>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QStandardItemModel>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>

#include <functional>

#include "analyzer/ResourceAnalysis.h"
#include "analyzer/ResourceEstimator.h"
#include "canvas/CanvasScene.h"
#include "codegen/BackendRegistry.h"
#include "codegen/FirmwareExportManager.h"
#include "codegen/LvglGenerator.h"
#include "codegen/QtMcuGenerator.h"
#include "codegen/UgfxGenerator.h"
#include "diagnostics/DiagnosticsService.h"
#include "diagnostics/DiagnosticsSources.h"
#include "dialogs/ExportDialog.h"
#include "models/ButtonComponent.h"
#include "models/CustomComponentInstance.h"
#include "models/GaugeComponent.h"
#include "models/LabelComponent.h"
#include "models/ProgressBarComponent.h"
#include "models/Screen.h"
#include "models/SliderComponent.h"
#include "panels/BoardDisplayPanel.h"
#include "panels/ProblemsPanel.h"
#include "panels/ResourceAnalyzerPanel.h"
#include "project/Project.h"
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

static QByteArray readAllBytes(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QByteArray();
    return file.readAll();
}

// Minimal modal-dialog pump for the export-gate tests (same approach as the
// Task 1 export-validation suite).
class MessageBoxPump {
public:
    void expect(const QString& title, QMessageBox::StandardButton button) {
        m_expected.append({title, button, false});
    }
    bool seen(const QString& title) const {
        for (const auto& e : m_expected) if (e.title == title) return e.seen;
        return false;
    }
    void run(const std::function<void()>& clickFn) {
        QTimer timer;
        timer.setInterval(10);
        QElapsedTimer total;
        total.start();
        QObject::connect(&timer, &QTimer::timeout, [&]() {
            auto* mb = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (!mb) return;
            for (auto& e : m_expected) {
                if (!e.seen && mb->windowTitle() == e.title) {
                    e.seen = true;
                    if (QAbstractButton* b = mb->button(e.button)) {
                        b->click();
                    } else {
                        mb->reject();
                    }
                    return;
                }
            }
            if (total.elapsed() > 8000) mb->reject(); // never hang on unexpected modals
        });
        timer.start();
        clickFn();
        timer.stop();
    }

private:
    struct Expectation {
        QString title;
        QMessageBox::StandardButton button;
        bool seen;
    };
    QVector<Expectation> m_expected;
};

class TestTargetBackend : public QObject {
    Q_OBJECT

private slots:
    void testRegistryListsOnlyRealBackends();
    void testPlannedBackendCannotBeSelected();
    void testBackendSelectionPersistsAndRoundTrips();
    void testOldProjectLoadsWithSameEffectiveBackend();
    void testSupportMatrixPerBackend();
    void testBackendDiagnosticsReachProblemsPanel();
    void testSwitchingBackendChangesAnalyzerBaselineUnder100Ms();
    void testGeneratorOutputUnchangedRegression();
    void testRegistryExtensionPoint();
    void testBackendWarningsUseSingleConfirmFlow();
    void testBackendErrorsBlockExport();
    void testCaptureScreenshots();
};

// ─────────────────────────────────────────────────────────────────────────────
// 1. Registry contents
// ─────────────────────────────────────────────────────────────────────────────
void TestTargetBackend::testRegistryListsOnlyRealBackends() {
    const QList<BackendDescriptor> backends = BackendRegistry::backends();
    QVERIFY(backends.size() >= 5);

    QStringList ids;
    for (const BackendDescriptor& backend : backends) {
        ids << backend.id;
    }
    // The four real generators plus the planned native runtime, in order.
    QCOMPARE(ids.mid(0, 5),
             QStringList({"lvgl", "ugfx", "qt-for-mcus", "embedweb", "native"}));

    QCOMPARE(BackendRegistry::backend("lvgl").status, QString("Stable"));
    QCOMPARE(BackendRegistry::backend("ugfx").status, QString("Stable"));
    QCOMPARE(BackendRegistry::backend("qt-for-mcus").status, QString("Stable"));
    QCOMPARE(BackendRegistry::backend("embedweb").status, QString("Experimental"));

    // EmbedWeb is its own backend - it is not relabeled as the native runtime.
    const BackendDescriptor embedweb = BackendRegistry::backend("embedweb");
    QVERIFY(!embedweb.displayName.contains("Native Runtime"));

    // The planned native entry is disabled with the exact tooltip.
    const BackendDescriptor native = BackendRegistry::backend("native");
    QCOMPARE(native.status, QString("Planned"));
    QVERIFY(!native.selectable);
    QCOMPARE(native.disabledTooltip, QString("Not implemented yet."));
    QVERIFY(native.frameworkId.isEmpty());

    // Every selectable backend maps to a real generator through the same
    // dispatch rules FirmwareExportManager uses (round trip + legacy ids).
    for (const BackendDescriptor& backend : backends) {
        if (!backend.selectable) continue;
        QVERIFY(!backend.frameworkId.isEmpty());
        QCOMPARE(BackendRegistry::backendForFramework(backend.frameworkId).id, backend.id);
        QVERIFY(!backend.pixelFormats.isEmpty());
    }
    QCOMPARE(BackendRegistry::backendForFramework("qul").id, QString("qt-for-mcus"));
    QCOMPARE(BackendRegistry::backendForFramework("qtmcu").id, QString("qt-for-mcus"));
    QCOMPARE(BackendRegistry::backendForFramework("").id, QString("lvgl"));
    QCOMPARE(BackendRegistry::backendForFramework("stm32cube").id, QString("lvgl"));

    // EmbedWeb is honest about its single pixel format.
    QCOMPARE(BackendRegistry::backend("embedweb").pixelFormats, QStringList({"RGB565"}));
    QVERIFY(BackendRegistry::backend("lvgl").pixelFormats.contains("RGB888"));
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. Planned backend cannot be selected (dock + export dialog)
// ─────────────────────────────────────────────────────────────────────────────
void TestTargetBackend::testPlannedBackendCannotBeSelected() {
    CanvasScene scene;
    Project project(&scene);
    project.newProject("PlannedBackend", 320, 240);
    project.setTargetFramework("lvgl");

    BoardDisplayPanel panel;
    panel.setProject(&project);

    auto* combo = panel.findChild<QComboBox*>("bdBackendCombo");
    QVERIFY2(combo, "bdBackendCombo not found in the Board & Display dock");
    QCOMPARE(combo->count(), BackendRegistry::backends().size());

    const int nativeIndex = combo->findData("native");
    QVERIFY(nativeIndex >= 0);
    auto* model = qobject_cast<QStandardItemModel*>(combo->model());
    QVERIFY(model);
    QVERIFY2(!model->item(nativeIndex)->isEnabled(), "planned backend entry must be disabled");
    QCOMPARE(combo->itemData(nativeIndex, Qt::ToolTipRole).toString(),
             QString("Not implemented yet."));

    // Programmatic selection of the planned entry is ignored.
    panel.setTargetBackend("native");
    QCOMPARE(project.targetFramework(), QString("lvgl"));
    QCOMPARE(panel.currentBackendId(), QString("lvgl"));
    QCOMPARE(combo->currentData().toString(), QString("lvgl"));

    // Even a forced combo jump reverts to the effective backend.
    combo->setCurrentIndex(nativeIndex);
    QApplication::processEvents();
    QCOMPARE(project.targetFramework(), QString("lvgl"));
    QCOMPARE(panel.currentBackendId(), QString("lvgl"));
    QCOMPARE(combo->currentData().toString(), QString("lvgl"));

    // The export dialog shows the same registry list; the planned entry is
    // disabled there as well and the effective backend is preselected.
    setTargetDevice(project, "STM32F030R8");
    ExportDialog dialog(&project);
    QComboBox* exportCombo = dialog.frameworkCombo();
    QVERIFY(exportCombo);
    QCOMPARE(exportCombo->count(), BackendRegistry::backends().size());
    const int exportNativeIndex = exportCombo->findData("native");
    QVERIFY(exportNativeIndex >= 0);
    auto* exportModel = qobject_cast<QStandardItemModel*>(exportCombo->model());
    QVERIFY(exportModel);
    QVERIFY(!exportModel->item(exportNativeIndex)->isEnabled());
    QCOMPARE(exportCombo->itemData(exportNativeIndex, Qt::ToolTipRole).toString(),
             QString("Not implemented yet."));
    QCOMPARE(exportCombo->currentData().toString(), QString("lvgl"));

    // Switching the dock to a real backend is reflected in the export dialog.
    panel.setTargetBackend("ugfx");
    ExportDialog dialog2(&project);
    QCOMPARE(dialog2.frameworkCombo()->currentData().toString(), QString("ugfx"));
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. Selection persists and round-trips; no second framework setting
// ─────────────────────────────────────────────────────────────────────────────
void TestTargetBackend::testBackendSelectionPersistsAndRoundTrips() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    CanvasScene scene;
    Project project(&scene);
    project.newProject("BackendPersist", 320, 240);
    project.setTargetFramework("lvgl");

    BoardDisplayPanel panel;
    panel.setProject(&project);
    panel.setTargetBackend("ugfx");
    QCOMPARE(project.targetFramework(), QString("ugfx"));
    QCOMPARE(panel.currentBackendId(), QString("ugfx"));

    const QJsonObject json = project.toJson();
    QCOMPARE(json.value("target").toString(), QString("ugfx"));
    // One source of truth: no separate "backend" key is ever written.
    QVERIFY2(!json.contains("backend"), "a second framework setting must not be persisted");

    const QString path = tmp.filePath("persist.euiproj");
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QJsonDocument(json).toJson());
    }

    CanvasScene restoredScene;
    Project restored(&restoredScene);
    QVERIFY(restored.loadFromFile(path));
    QCOMPARE(restored.targetFramework(), QString("ugfx"));
    QCOMPARE(BackendRegistry::backendForFramework(restored.targetFramework()).id, QString("ugfx"));

    BoardDisplayPanel restoredPanel;
    restoredPanel.setProject(&restored);
    auto* combo = restoredPanel.findChild<QComboBox*>("bdBackendCombo");
    QVERIFY(combo);
    QCOMPARE(combo->currentData().toString(), QString("ugfx"));

    // The selection is the same value the export and analyzer consume.
    ResourceEstimator estimator;
    const ResourceAnalysis canonical = estimator.analyzeProject(&restored, "ugfx");
    const ResourceAnalysis selected = estimator.analyzeProject(
        &restored, BackendRegistry::backendForFramework(restored.targetFramework()).frameworkId);
    QCOMPARE(selected.breakdown.framework_flash, canonical.breakdown.framework_flash);
    QCOMPARE(selected.breakdown.framework_ram, canonical.breakdown.framework_ram);
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. Old projects keep their effective backend
// ─────────────────────────────────────────────────────────────────────────────
void TestTargetBackend::testOldProjectLoadsWithSameEffectiveBackend() {
    // Legacy file without a "target" key: fromJson default is "qt-for-mcus",
    // exactly the framework the export/analyzer consumed before Task 6.
    QJsonObject legacy;
    legacy["name"] = "LegacyNoTarget";
    CanvasScene scene;
    Project project(&scene);
    QVERIFY(project.fromJson(legacy));
    QCOMPARE(project.targetFramework(), QString("qt-for-mcus"));
    QCOMPARE(BackendRegistry::backendForFramework(project.targetFramework()).id,
             QString("qt-for-mcus"));

    // Legacy spellings resolve exactly like the generator dispatch.
    QCOMPARE(BackendRegistry::backendForFramework("qul").id, QString("qt-for-mcus"));
    QCOMPARE(BackendRegistry::backendForFramework("qtmcu").id, QString("qt-for-mcus"));
    QCOMPARE(BackendRegistry::backendForFramework("lvgl").id, QString("lvgl"));
    QCOMPARE(BackendRegistry::backendForFramework("ugfx").id, QString("ugfx"));
    QCOMPARE(BackendRegistry::backendForFramework("embedweb").id, QString("embedweb"));

    // The analyzer numbers for a legacy spelling equal the canonical ones.
    ResourceEstimator estimator;
    QCOMPARE(estimator.analyzeProject(&project, "qtmcu").breakdown.framework_flash,
             estimator.analyzeProject(&project, "qt-for-mcus").breakdown.framework_flash);
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. Widget support matrix per backend (the derived table)
// ─────────────────────────────────────────────────────────────────────────────
void TestTargetBackend::testSupportMatrixPerBackend() {
    // Reference project: one gauge (unsupported nearly everywhere), one
    // progress bar (supported everywhere) and one custom widget (unknown).
    CanvasScene scene;
    Project project(&scene);
    project.newProject("Matrix", 320, 240);
    Screen* screen = project.activeScreen();
    QVERIFY(screen);
    screen->setName("Main Screen");
    screen->addComponent(new GaugeComponent("gauge_speed"));
    screen->addComponent(new ProgressBarComponent("bar_load"));
    screen->addComponent(new CustomComponentInstance("custom_widget"));

    auto findingFor = [](const QList<BackendFinding>& findings, const QString& componentId) {
        for (const BackendFinding& finding : findings) {
            if (finding.componentId == componentId) return finding;
        }
        return BackendFinding();
    };
    auto hasFindingFor = [&findingFor](const QList<BackendFinding>& findings, const QString& componentId) {
        return !findingFor(findings, componentId).componentId.isEmpty();
    };

    // LVGL: gauge silently omitted -> Warning; custom widget -> Info (unknown).
    {
        const QList<BackendFinding> findings = BackendRegistry::validate(&project, "lvgl");
        QVERIFY(!hasFindingFor(findings, "bar_load"));
        const BackendFinding gauge = findingFor(findings, "gauge_speed");
        QCOMPARE(gauge.level, BackendSupportLevel::Unsupported);
        QCOMPARE(gauge.severity, BackendFindingSeverity::Warning);
        QVERIFY2(gauge.message.contains("Gauge is not supported by LVGL backend"),
                 qPrintable(gauge.message));
        QVERIFY(gauge.message.contains("omitted"));
        QCOMPARE(gauge.screenId, screen->id());

        const BackendFinding custom = findingFor(findings, "custom_widget");
        QCOMPARE(custom.level, BackendSupportLevel::Unknown);
        QCOMPARE(custom.severity, BackendFindingSeverity::Info);
        QVERIFY(custom.message.contains("not documented"));
    }

    // µGFX: same omission warning with the backend's own name.
    {
        const QList<BackendFinding> findings = BackendRegistry::validate(&project, "ugfx");
        const BackendFinding gauge = findingFor(findings, "gauge_speed");
        QCOMPARE(gauge.severity, BackendFindingSeverity::Warning);
        QVERIFY2(gauge.message.contains("Gauge is not supported by µGFX backend"),
                 qPrintable(gauge.message));
    }

    // Qt for MCUs: graceful placeholder -> partial support, still a Warning.
    {
        const QList<BackendFinding> findings = BackendRegistry::validate(&project, "qt-for-mcus");
        const BackendFinding gauge = findingFor(findings, "gauge_speed");
        QCOMPARE(gauge.level, BackendSupportLevel::Partial);
        QCOMPARE(gauge.severity, BackendFindingSeverity::Warning);
        QVERIFY2(gauge.message.contains("not fully supported by Qt for MCUs (QUL) backend"),
                 qPrintable(gauge.message));
        QVERIFY(gauge.message.contains("placeholder"));
    }

    // EmbedWeb: hydration aborts -> Error (the only blocking backend).
    {
        const QList<BackendFinding> findings = BackendRegistry::validate(&project, "embedweb");
        const BackendFinding gauge = findingFor(findings, "gauge_speed");
        QCOMPARE(gauge.level, BackendSupportLevel::Unsupported);
        QCOMPARE(gauge.severity, BackendFindingSeverity::Error);
        QVERIFY2(gauge.message.contains("Gauge is not supported by EmbedWeb backend"),
                 qPrintable(gauge.message));
        QVERIFY(gauge.message.contains("export cannot be generated"));
        // The progress bar remains supported on EmbedWeb.
        QVERIFY(!hasFindingFor(findings, "bar_load"));
    }

    // The planned backend never produces findings.
    QCOMPARE(BackendRegistry::validate(&project, "native").size(), 0);

    // supportFor() reports the same levels directly.
    QCOMPARE(BackendRegistry::supportFor("ugfx", "Gauge").level, BackendSupportLevel::Unsupported);
    QCOMPARE(BackendRegistry::supportFor("qt-for-mcus", "Gauge").level, BackendSupportLevel::Partial);
    QCOMPARE(BackendRegistry::supportFor("embedweb", "Button").level, BackendSupportLevel::Supported);
    QCOMPARE(BackendRegistry::supportFor("embedweb", "Slider").level, BackendSupportLevel::Unsupported);
    QCOMPARE(BackendRegistry::supportFor("lvgl", "Image").level, BackendSupportLevel::Partial);
    QCOMPARE(BackendRegistry::supportFor("embedweb", "Image").level, BackendSupportLevel::Supported);
    QCOMPARE(BackendRegistry::supportFor("lvgl", "CustomInstance").level, BackendSupportLevel::Unknown);
}

// ─────────────────────────────────────────────────────────────────────────────
// 6. Backend diagnostics reach the Problems dock with a location
// ─────────────────────────────────────────────────────────────────────────────
void TestTargetBackend::testBackendDiagnosticsReachProblemsPanel() {
    CanvasScene scene;
    Project project(&scene);
    project.newProject("BackendProblems", 480, 320);
    Screen* screen = project.activeScreen();
    QVERIFY(screen);
    screen->setName("Main Screen");
    screen->addComponent(new GaugeComponent("gauge_speed"));
    screen->addComponent(new ProgressBarComponent("bar_load"));
    project.setTargetFramework("ugfx");

    Diagnostics::DiagnosticsService service;
    Diagnostics::DiagnosticsSources::registerBuiltinSources(service);
    QVERIFY(service.sourceNames().contains("Backend"));
    service.setProject(&project);
    service.runNow();

    int backendCount = 0;
    bool sawGaugeWarning = false;
    for (const Diagnostic& diagnostic : service.diagnostics()) {
        if (diagnostic.source != "Backend") continue;
        ++backendCount;
        if (diagnostic.message.contains("Gauge is not supported by µGFX backend")) {
            sawGaugeWarning = true;
            QCOMPARE(diagnostic.severity, Diagnostic::Severity::Warning);
            QCOMPARE(diagnostic.screenId, screen->id());
            QCOMPARE(diagnostic.componentId, QString("gauge_speed"));
        }
    }
    QCOMPARE(backendCount, 1); // progress bar is supported, no other components
    QVERIFY2(sawGaugeWarning, "the µGFX gauge warning must be registered by the Backend source");

    ProblemsPanel panel(&service);
    QApplication::processEvents();
    QVERIFY2(panel.countForSeverity(Diagnostic::Severity::Warning) >= 1,
             "the Problems panel must show the Backend warning");

    auto* table = panel.findChild<QTableWidget*>("problemsTable");
    QVERIFY(table);
    bool sawRow = false;
    for (int row = 0; row < table->rowCount(); ++row) {
        const QString message = table->item(row, 1) ? table->item(row, 1)->text() : QString();
        if (!message.contains("Gauge is not supported by µGFX backend")) continue;
        sawRow = true;
        QCOMPARE(table->item(row, 0)->text(), QString("Warning"));
        QCOMPARE(table->item(row, 2)->text(), QString("Backend"));
        const QString location = table->item(row, 3)->text();
        QVERIFY2(location.contains("Main Screen"), qPrintable(location));
        QVERIFY2(location.contains("gauge_speed"), qPrintable(location));
    }
    QVERIFY2(sawRow, "the Backend warning must carry screen + component location");

    // Switching the project to EmbedWeb turns the same component into a
    // blocking Error; no other source must be involved.
    project.setTargetFramework("embedweb");
    service.runNow();
    bool sawGaugeError = false;
    for (const Diagnostic& diagnostic : service.diagnostics()) {
        if (diagnostic.source == "Backend" &&
            diagnostic.message.contains("Gauge is not supported by EmbedWeb backend")) {
            sawGaugeError = true;
            QCOMPARE(diagnostic.severity, Diagnostic::Severity::Error);
            QCOMPARE(diagnostic.componentId, QString("gauge_speed"));
        }
    }
    QVERIFY2(sawGaugeError, "EmbedWeb + unsupported component must be an Error");
    QVERIFY(panel.countForSeverity(Diagnostic::Severity::Error) >= 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// 7. Switching the backend refreshes the analyzer baseline under 100 ms
// ─────────────────────────────────────────────────────────────────────────────
void TestTargetBackend::testSwitchingBackendChangesAnalyzerBaselineUnder100Ms() {
    MainWindow win;
    win.resize(1400, 900);
    win.show();
    QApplication::processEvents();

    Project* project = win.currentProject();
    QVERIFY(project);
    project->newProject("BackendPerf", 320, 240);
    setTargetDevice(*project, "STM32F030R8");
    project->setTargetFramework("lvgl");
    QApplication::processEvents();

    ResourceAnalyzerPanel* analyzer = win.resourceAnalyzerPanel();
    QVERIFY(analyzer);
    analyzer->refresh();
    const uint32_t lvglFlash = analyzer->lastAnalysis().breakdown.framework_flash;

    ResourceEstimator estimator;
    QCOMPARE(lvglFlash, estimator.analyzeProject(project, "lvgl").breakdown.framework_flash);
    const uint32_t ugfxFlash = estimator.analyzeProject(project, "ugfx").breakdown.framework_flash;
    QVERIFY2(ugfxFlash != lvglFlash, "framework baselines must differ");

    BoardDisplayPanel* panel = win.boardDisplayPanel();
    QVERIFY(panel);
    QCOMPARE(panel->currentBackendId(), QString("lvgl"));

    QElapsedTimer timer;
    timer.start();
    panel->setTargetBackend("ugfx");
    QApplication::processEvents();
    const qint64 elapsed = timer.elapsed();

    QVERIFY2(elapsed < 100, qPrintable(QString("backend refresh took %1 ms").arg(elapsed)));
    QCOMPARE(project->targetFramework(), QString("ugfx"));
    QCOMPARE(analyzer->lastAnalysis().breakdown.framework_flash, ugfxFlash);

    // Back to LVGL restores the original baseline immediately.
    panel->setTargetBackend("lvgl");
    QApplication::processEvents();
    QCOMPARE(analyzer->lastAnalysis().breakdown.framework_flash, lvglFlash);
}

// ─────────────────────────────────────────────────────────────────────────────
// 8. Generator output regression (byte-identical for all generators)
// ─────────────────────────────────────────────────────────────────────────────
void TestTargetBackend::testGeneratorOutputUnchangedRegression() {
    CanvasScene scene;
    Project project(&scene);
    project.setProjectName("RegressionApp");
    project.setTargetFramework("lvgl");

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

    FirmwareExportManager exportMgr;
    auto exportTo = [&](const QString& framework, const QString& dir) {
        ExportOptions opts;
        opts.outputDirectory = dir;
        opts.targetFramework = framework;
        opts.forceExport = true;
        return exportMgr.exportProject(&project, opts);
    };

    // LVGL: exported files match the direct generator byte for byte.
    QTemporaryDir lvglDir;
    QVERIFY(lvglDir.isValid());
    QVERIFY(exportTo("lvgl", lvglDir.path()).success);
    LvglGenerator lvglGen(&project, nullptr);
    QCOMPARE(readAllBytes(QDir(lvglDir.path()).filePath("generated/ui/ui.h")),
             lvglGen.generateUiHeader().toUtf8());
    QCOMPARE(readAllBytes(QDir(lvglDir.path()).filePath("generated/ui/ui.c")),
             lvglGen.generateUiSource().toUtf8());

    // µGFX: same guarantee.
    QTemporaryDir ugfxDir;
    QVERIFY(ugfxDir.isValid());
    QVERIFY(exportTo("ugfx", ugfxDir.path()).success);
    UgfxGenerator ugfxGen(&project, nullptr);
    QCOMPARE(readAllBytes(QDir(ugfxDir.path()).filePath("generated/ui/gui.h")),
             ugfxGen.generateUiHeader().toUtf8());
    QCOMPARE(readAllBytes(QDir(ugfxDir.path()).filePath("generated/ui/gui.c")),
             ugfxGen.generateUiSource().toUtf8());

    // Qt for MCUs: exported QML matches the direct generator and the canonical
    // registry id produces exactly the same bytes as the legacy dialog id.
    QTemporaryDir qmlCanonicalDir;
    QTemporaryDir qmlLegacyDir;
    QVERIFY(qmlCanonicalDir.isValid());
    QVERIFY(qmlLegacyDir.isValid());
    QVERIFY(exportTo("qt-for-mcus", qmlCanonicalDir.path()).success);
    QVERIFY(exportTo("qtmcu", qmlLegacyDir.path()).success);
    QtMcuGenerator qmlGen(&project, nullptr);
    QCOMPARE(readAllBytes(QDir(qmlCanonicalDir.path()).filePath("generated/ui/MainScreen.qml")),
             qmlGen.generateDesignQml().toUtf8());
    QCOMPARE(readAllBytes(QDir(qmlCanonicalDir.path()).filePath("generated/ui/MainScreen.qml")),
             readAllBytes(QDir(qmlLegacyDir.path()).filePath("generated/ui/MainScreen.qml")));
    QCOMPARE(readAllBytes(QDir(qmlCanonicalDir.path()).filePath("user/main.c")),
             readAllBytes(QDir(qmlLegacyDir.path()).filePath("user/main.c")));

    // EmbedWeb: exports a compatible project through the registry's framework
    // id and keeps the existing rejection for unsupported components.
    // Note: the EmbedWeb generator hydrates the project's *active* screen.
    CanvasScene ewScene;
    Project ewProject(&ewScene);
    ewProject.setProjectName("EmbedWebCompatible");
    ewProject.setTargetFramework("embedweb");
    Screen* ewScreen = ewProject.activeScreen();
    QVERIFY(ewScreen);
    ewScreen->addComponent(new ButtonComponent("ew_btn"));
    ewScreen->addComponent(new LabelComponent("ew_lbl"));
    ewScreen->addComponent(new ProgressBarComponent("ew_bar"));

    QTemporaryDir ewDir;
    QVERIFY(ewDir.isValid());
    ExportOptions ewOpts;
    ewOpts.outputDirectory = ewDir.path();
    ewOpts.targetFramework = BackendRegistry::backend("embedweb").frameworkId;
    ewOpts.forceExport = true;
    ExportResult ewResult = exportMgr.exportProject(&ewProject, ewOpts);
    QVERIFY2(ewResult.success, qPrintable(ewResult.errorMessage));
    QVERIFY(QFile::exists(QDir(ewDir.path()).filePath("generated/ui/ui_tree.c")));

    CanvasScene badScene;
    Project badProject(&badScene);
    badProject.setProjectName("EmbedWebUnsupported");
    badProject.setTargetFramework("embedweb");
    Screen* badScreen = badProject.activeScreen();
    QVERIFY(badScreen);
    badScreen->addComponent(new SliderComponent("bad_slider"));
    QTemporaryDir badDir;
    QVERIFY(badDir.isValid());
    ExportOptions badOpts;
    badOpts.outputDirectory = badDir.path();
    badOpts.targetFramework = "embedweb";
    badOpts.forceExport = true;
    const ExportResult badResult = exportMgr.exportProject(&badProject, badOpts);
    QVERIFY2(!badResult.success, "EmbedWeb must keep rejecting unsupported components");
    QVERIFY(badResult.errorMessage.contains("Unsupported component type"));
}

// ─────────────────────────────────────────────────────────────────────────────
// 8b. Extension point: later backends register themselves without UI edits
// ─────────────────────────────────────────────────────────────────────────────
void TestTargetBackend::testRegistryExtensionPoint() {
    BackendDescriptor fake;
    fake.id = "unit-test-backend";
    fake.displayName = "Unit Test Backend";
    fake.status = "Experimental";
    fake.description = "Registered by the acceptance test.";
    fake.frameworkId = "unit-test-framework";
    fake.pixelFormats = { "RGB565" };
    fake.unsupportedPolicy = BackendUnsupportedPolicy::OmitWarning;

    QVERIFY(BackendRegistry::registerBackend(fake));
    QVERIFY2(!BackendRegistry::registerBackend(fake), "duplicate ids must be rejected");
    QVERIFY(BackendRegistry::contains("unit-test-backend"));
    QCOMPARE(BackendRegistry::backend("unit-test-backend").displayName, QString("Unit Test Backend"));

    CanvasScene scene;
    Project project(&scene);
    project.newProject("Extension", 320, 240);
    project.setTargetFramework("lvgl");

    // A backend registered at runtime appears in the dock ...
    BoardDisplayPanel panel;
    panel.setProject(&project);
    auto* combo = panel.findChild<QComboBox*>("bdBackendCombo");
    QVERIFY(combo);
    QVERIFY2(combo->findData("unit-test-backend") >= 0,
             "a backend registered at runtime must appear in the dock without UI edits");

    // ... and in the export dialog, without any UI code knowing about it.
    ExportDialog dialog(&project);
    QVERIFY(dialog.frameworkCombo()->findData("unit-test-backend") >= 0);

    QVERIFY(BackendRegistry::unregisterBackend("unit-test-backend"));
    QVERIFY(!BackendRegistry::contains("unit-test-backend"));
    QVERIFY2(!BackendRegistry::unregisterBackend("unit-test-backend"), "double removal must fail");
    panel.refreshFromProject();
    QCOMPARE(combo->findData("unit-test-backend"), -1);
}

// ─────────────────────────────────────────────────────────────────────────────
// 8c. Export gate: backend Warnings confirm through a single dialog
// ─────────────────────────────────────────────────────────────────────────────
void TestTargetBackend::testBackendWarningsUseSingleConfirmFlow() {
    CanvasScene scene;
    Project project(&scene);
    project.newProject("GateWarn", 480, 320);
    setTargetDevice(project, "STM32F401");
    project.setTargetFramework("lvgl");
    project.activeScreen()->addComponent(new GaugeComponent("gauge_gate"));

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString exportDir = tmp.filePath("gate_warning_export");

    ExportDialog dlg(&project, nullptr);
    dlg.show();
    QApplication::processEvents();
    auto* pathEdit = dlg.findChild<QLineEdit*>();
    QVERIFY(pathEdit);
    pathEdit->setText(exportDir);
    auto* exportBtn = dlg.findChild<QPushButton*>("exportButton");
    QVERIFY(exportBtn);

    // 1) Cancelling the backend warning must not export.
    MessageBoxPump cancelPump;
    cancelPump.expect("Backend Compatibility Warning", QMessageBox::Cancel);
    cancelPump.run([&]() { exportBtn->click(); });
    QVERIFY2(cancelPump.seen("Backend Compatibility Warning"),
             "backend warning confirmation dialog did not appear");
    QVERIFY2(!QDir(exportDir).exists(), "export must not run when the backend warning is cancelled");

    // 2) Accepting exports successfully (exactly one confirmation appears:
    //    resource estimates on the F401 are well below the warning band).
    MessageBoxPump acceptPump;
    acceptPump.expect("Backend Compatibility Warning", QMessageBox::Yes);
    acceptPump.expect("Export Succeeded", QMessageBox::Ok);
    acceptPump.run([&]() { exportBtn->click(); });
    QVERIFY(acceptPump.seen("Backend Compatibility Warning"));
    QVERIFY2(acceptPump.seen("Export Succeeded"),
             "export must proceed after confirming the backend warning");
    QVERIFY(QDir(exportDir).exists());
    QVERIFY(!QDir(exportDir).entryList(QDir::Files).isEmpty());
}

// ─────────────────────────────────────────────────────────────────────────────
// 8d. Export gate: backend Errors block before any output is written
// ─────────────────────────────────────────────────────────────────────────────
void TestTargetBackend::testBackendErrorsBlockExport() {
    CanvasScene scene;
    Project project(&scene);
    project.newProject("GateError", 480, 320);
    project.setTargetFramework("embedweb");
    project.activeScreen()->addComponent(new GaugeComponent("gauge_block"));

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString exportDir = tmp.filePath("gate_error_export");

    ExportDialog dlg(&project, nullptr);
    dlg.show();
    QApplication::processEvents();
    auto* pathEdit = dlg.findChild<QLineEdit*>();
    QVERIFY(pathEdit);
    pathEdit->setText(exportDir);
    auto* exportBtn = dlg.findChild<QPushButton*>("exportButton");
    QVERIFY(exportBtn);

    MessageBoxPump pump;
    pump.expect("Export Blocked", QMessageBox::Ok);
    pump.run([&]() { exportBtn->click(); });
    QVERIFY2(pump.seen("Export Blocked"), "EmbedWeb + unsupported component must block the export");
    QVERIFY2(!QDir(exportDir).exists(), "blocked export must not write any output");
}

// ─────────────────────────────────────────────────────────────────────────────
// 9. Screenshots: selector, Backend Error in Problems, export dialog
// ─────────────────────────────────────────────────────────────────────────────
void TestTargetBackend::testCaptureScreenshots() {
    MainWindow win;
    win.resize(1600, 1050);
    win.show();
    QApplication::processEvents();

    Project* project = win.currentProject();
    QVERIFY(project);
    project->newProject("Task6Screens", 480, 320);
    Screen* screen = project->activeScreen();
    QVERIFY(screen);
    screen->setName("Dashboard");
    auto* gauge = new GaugeComponent("gauge_speed");
    gauge->setCompPos(30, 40);
    gauge->setCompSize(180, 180);
    screen->addComponent(gauge);
    auto* bar = new ProgressBarComponent("bar_load");
    bar->setCompPos(240, 60);
    bar->setCompSize(200, 30);
    screen->addComponent(bar);
    auto* button = new ButtonComponent("btn_start");
    button->setCompPos(240, 140);
    button->setCompSize(140, 44);
    screen->addComponent(button);

    BoardDisplayPanel* panel = win.boardDisplayPanel();
    QVERIFY(panel);
    auto* dock = win.findChild<QDockWidget*>("boardDisplayDock");
    QVERIFY2(dock, "boardDisplayDock not found (dock not registered?)");
    dock->show();
    dock->raise();
    for (const QString& dockName : { QStringLiteral("propertiesDock"),
                                     QStringLiteral("effectsDock"),
                                     QStringLiteral("prototypeDock") }) {
        if (auto* other = win.findChild<QDockWidget*>(dockName)) other->hide();
    }
    win.resizeDocks({ dock }, { 980 }, Qt::Vertical);
    QApplication::processEvents();

    auto settleUi = []() {
        for (int i = 0; i < 3; ++i) {
            QApplication::processEvents();
            QTest::qWait(20);
        }
        QApplication::processEvents();
    };

    // 1. Backend selector at the top of the Target section (LVGL selected;
    //    the gauge already shows up as a Backend warning).
    panel->setTargetBackend("lvgl");
    win.diagnosticsService()->runNow();
    settleUi();
    QVERIFY(win.grab().save(screenshotPath("task6_backend_selector.png")));
    QVERIFY(panel->grab().save(screenshotPath("task6_backend_dock.png")));

    // 2. EmbedWeb selected: the gauge becomes a blocking Backend Error in the
    //    Problems dock (with location).
    panel->setTargetBackend("embedweb");
    win.diagnosticsService()->runNow();
    ProblemsPanel* problems = win.problemsPanel();
    QVERIFY(problems);
    QVERIFY2(problems->countForSeverity(Diagnostic::Severity::Error) >= 1,
             "EmbedWeb + gauge must appear as an Error in the Problems dock");
    auto* problemsDock = win.findChild<QDockWidget*>("problemsDock");
    QVERIFY(problemsDock);
    problemsDock->show();
    problemsDock->raise();
    settleUi();
    QVERIFY(win.grab().save(screenshotPath("task6_backend_diagnostic.png")));

    // 3. Export dialog shows the same backend list/value.
    ExportDialog dialog(project);
    dialog.resize(640, 720);
    dialog.show();
    settleUi();
    QCOMPARE(dialog.frameworkCombo()->currentData().toString(), QString("embedweb"));
    QVERIFY(dialog.grab().save(screenshotPath("task6_export_dialog_backend.png")));
    dialog.close();
}

QTEST_MAIN(TestTargetBackend)
#include "test_target_backend.moc"
