/**
 * Task 2 — Resource Analyzer in Live Test Mode + Export dialog verification suite.
 *
 * Covers:
 *  - Part A: Export dialog "Resource Analysis (Estimated)" group, bar percentages,
 *    80%/95% color thresholds, critical blocking, warning confirmation, no-device
 *    info notice, Optimize... sorting and DarkTheme-only colors.
 *  - Part B: Resource Analyzer panel must stay VISIBLE while Live Test is ON,
 *    with a banner; an over-limit design must still run in Live Test while the
 *    export path stays blocked exactly as in Task 1.
 */
#include <QTest>
#include <QObject>
#include <QApplication>
#include <QTimer>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QListWidget>
#include <QTableWidget>
#include <QToolButton>
#include <QDockWidget>
#include <QMessageBox>
#include <QDialog>
#include <QRegularExpression>
#include <QSet>
#include <QVector>
#include <QStringList>
#include <functional>

#include "analyzer/ResourceAnalysis.h"
#include "analyzer/ResourceEstimator.h"
#include "hardware/DeviceDatabase.h"
#include "hardware/BoardRegistry.h"
#include "hardware/HardwareModel.h"
#include "project/Project.h"
#include "canvas/CanvasScene.h"
#include "models/ButtonComponent.h"
#include "models/LabelComponent.h"
#include "models/ImageComponent.h"
#include "codegen/FirmwareExportManager.h"
#include "dialogs/ExportDialog.h"
#include "panels/ResourceAnalyzerPanel.h"
#include "theme/Theme.h"
#include "MainWindow.h"

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

// Answers modal QMessageBoxes while a (blocking) click handler runs.
class MessageBoxPump {
public:
    void expect(const QString& title, QMessageBox::StandardButton button) {
        m_expected.append({title, button, QString(), false});
    }
    bool seen(const QString& title) const {
        for (const auto& e : m_expected) if (e.title == title) return e.seen;
        return false;
    }
    QString text(const QString& title) const {
        for (const auto& e : m_expected) if (e.title == title) return e.text;
        return QString();
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
                    e.text = mb->text();
                    if (QAbstractButton* b = mb->button(e.button)) {
                        b->click();
                    } else {
                        mb->reject();
                    }
                    return;
                }
            }
            if (total.elapsed() > 8000) {
                mb->reject(); // never hang the suite on an unexpected modal
            }
        });
        timer.start();
        clickFn();
        timer.stop();
    }

private:
    struct Expectation {
        QString title;
        QMessageBox::StandardButton button;
        QString text;
        bool seen;
    };
    QVector<Expectation> m_expected;
};

class TestLiveTestResources : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    void testExportDialogShowsResourceAnalysisGroup();
    void testExportDialogBarColorsAtThresholds();
    void testExportDialogWarningAsksForConfirmation();
    void testExportDialogNoDeviceInfoNotice();
    void testOptimizeDialogSortedByWeight();
    void testAnalyzerUiUsesThemeColorsOnly();

    void testLiveTestKeepsResourceAnalyzerVisible();
    void testOverLimitDesignRunsLiveTestButExportStillBlocked();
};

void TestLiveTestResources::initTestCase() {
    Hardware::DeviceDatabase::instance().initialize();
    BoardRegistry::instance().loadFromBoardsJson();
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
namespace {

void setDevice(Project& project, const QString& deviceId) {
    Hardware::HardwareConfig hw;
    hw.deviceId = deviceId;
    project.setHardwareConfig(hw);
}

QString makeAssetFile(QTemporaryDir& dir, int bytes) {
    const QString path = dir.filePath("asset.bin");
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return QString();
    f.write(QByteArray(bytes, 'A'));
    f.close();
    return path;
}

// OK band: comfortably below 80%
Project* makeOkProject(CanvasScene& scene) {
    auto* project = new Project(&scene);
    project->setTargetFramework("lvgl");
    setDevice(*project, "STM32F429");
    auto* screen = new Screen("MainScreen");
    screen->addComponent(new ButtonComponent("btn_ok_1"));
    screen->addComponent(new ButtonComponent("btn_ok_2"));
    screen->addComponent(new LabelComponent("lbl_ok"));
    project->addScreen(screen);
    return project;
}

// WARNING band (80% <= RAM < 95%)
Project* makeWarningProject(CanvasScene& scene, const QString& assetPath, int imgW, int imgH) {
    auto* project = new Project(&scene);
    project->setTargetFramework("lvgl");
    setDevice(*project, "STM32F401");
    auto* screen = new Screen("WarnScreen");
    screen->addComponent(new ButtonComponent("btn_warn"));
    auto* img = new ImageComponent("img_warn");
    img->setImagePath(assetPath);
    img->setCompSize(imgW, imgH);
    screen->addComponent(img);
    project->addScreen(screen);
    return project;
}

// CRITICAL band (>= 95%)
Project* makeCriticalProject(CanvasScene& scene, const QString& assetPath, int imgW, int imgH) {
    auto* project = new Project(&scene);
    project->setTargetFramework("lvgl");
    setDevice(*project, "STM32F401");
    auto* screen = new Screen("CritScreen");
    screen->addComponent(new ButtonComponent("btn_crit"));
    auto* img = new ImageComponent("img_crit");
    img->setImagePath(assetPath);
    img->setCompSize(imgW, imgH);
    screen->addComponent(img);
    project->addScreen(screen);
    return project;
}

QString expectedBarColor(int pct) {
    if (pct >= 95) return Theme::Danger;
    if (pct >= 80) return Theme::Warning;
    return Theme::Success;
}

void verifyDialogState(ExportDialog& dlg, const ResourceAnalysis& analysis, bool expectCritical) {
    auto* flashBar = dlg.findChild<QProgressBar*>("flashBar");
    auto* ramBar = dlg.findChild<QProgressBar*>("ramBar");
    auto* flashLabel = dlg.findChild<QLabel*>("flashLabel");
    auto* ramLabel = dlg.findChild<QLabel*>("ramLabel");
    auto* banner = dlg.findChild<QLabel*>("statusBannerLabel");
    auto* exportBtn = dlg.findChild<QPushButton*>("exportButton");
    QVERIFY2(flashBar, "flashBar not found");
    QVERIFY2(ramBar, "ramBar not found");
    QVERIFY2(flashLabel, "flashLabel not found");
    QVERIFY2(ramLabel, "ramLabel not found");
    QVERIFY2(banner, "statusBannerLabel not found");
    QVERIFY2(exportBtn, "exportButton not found");

    const int flashPct = qBound(0, static_cast<int>(analysis.flash_percent), 100);
    const int ramPct = qBound(0, static_cast<int>(analysis.ram_percent), 100);

    // Bars show the correct percentages
    QCOMPARE(flashBar->value(), flashPct);
    QCOMPARE(ramBar->value(), ramPct);

    // Labels show the same percentages the bars display
    const QString flashText = QString("%1").arg(analysis.flash_percent, 0, 'f', 1);
    const QString ramText = QString("%1").arg(analysis.ram_percent, 0, 'f', 1);
    QVERIFY2(flashLabel->text().contains(flashText),
             qPrintable(QString("flash label %1 lacks %2").arg(flashLabel->text(), flashText)));
    QVERIFY2(ramLabel->text().contains(ramText),
             qPrintable(QString("ram label %1 lacks %2").arg(ramLabel->text(), ramText)));
    QVERIFY(flashLabel->text().contains(QString("%1").arg(analysis.flash_total / 1024)));
    QVERIFY(ramLabel->text().contains(QString("%1").arg(analysis.ram_total / 1024)));

    // Colors change at the 80% and 95% thresholds
    const QString flashColor = expectedBarColor(flashPct);
    const QString ramColor = expectedBarColor(ramPct);
    QVERIFY2(flashBar->styleSheet().contains(flashColor),
             qPrintable(QString("flash bar stylesheet missing %1").arg(flashColor)));
    QVERIFY2(ramBar->styleSheet().contains(ramColor),
             qPrintable(QString("ram bar stylesheet missing %1").arg(ramColor)));

    if (expectCritical) {
        QVERIFY(!exportBtn->isEnabled());
        QVERIFY(banner->text().contains("exceeds device limits"));
    } else {
        QVERIFY(exportBtn->isEnabled());
    }
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Part A — Export dialog verification
// ─────────────────────────────────────────────────────────────────────────────
void TestLiveTestResources::testExportDialogShowsResourceAnalysisGroup() {
    CanvasScene scene;
    QScopedPointer<Project> project(makeOkProject(scene));

    ExportDialog dlg(project.data(), nullptr);
    dlg.show();
    QApplication::processEvents();

    auto* group = dlg.findChild<QGroupBox*>("resourceGroupBox");
    QVERIFY2(group, "Resource Analysis group box missing");
    QCOMPARE(group->title(), QString("Resource Analysis (Estimated)"));
    QVERIFY(group->isVisible());

    ResourceEstimator estimator;
    ResourceAnalysis analysis = estimator.analyze(project.data(),
                                                  project->getDeviceProfile(),
                                                  "lvgl");
    QVERIFY(analysis.has_device);
    QCOMPARE(analysis.status, ResourceAnalysis::OK);
    verifyDialogState(dlg, analysis, false);

    // OK banner text
    auto* banner = dlg.findChild<QLabel*>("statusBannerLabel");
    QVERIFY(banner->text().contains("within normal device limits"));

    const QString shot = screenshotPath("verify_task2_export_dialog_ok.png");
    QVERIFY(dlg.grab().save(shot));
    QVERIFY(QFile::exists(shot));
}

void TestLiveTestResources::testExportDialogBarColorsAtThresholds() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString assetPath = makeAssetFile(tmp, 1024);
    QVERIFY(!assetPath.isEmpty());

    // ── 1. OK (<80%) ────────────────────────────────────────────────────────
    {
        CanvasScene scene;
        QScopedPointer<Project> project(makeOkProject(scene));
        ExportDialog dlg(project.data(), nullptr);
        ResourceEstimator estimator;
        ResourceAnalysis a = estimator.analyze(project.data(), project->getDeviceProfile(), "lvgl");
        QVERIFY(a.status == ResourceAnalysis::OK);
        QVERIFY(a.flash_percent < 80.0f && a.ram_percent < 80.0f);
        verifyDialogState(dlg, a, false);
    }

    // ── 2. WARNING (80% <= RAM < 95%) ───────────────────────────────────────
    {
        CanvasScene scene;
        QScopedPointer<Project> project(makeWarningProject(scene, assetPath, 173, 172));
        ExportDialog dlg(project.data(), nullptr);
        dlg.show();
        QApplication::processEvents();

        ResourceEstimator estimator;
        ResourceAnalysis a = estimator.analyze(project.data(), project->getDeviceProfile(), "lvgl");
        QCOMPARE(a.status, ResourceAnalysis::WARNING);
        QVERIFY(a.ram_percent >= 80.0f && a.ram_percent < 95.0f);
        verifyDialogState(dlg, a, false);

        auto* banner = dlg.findChild<QLabel*>("statusBannerLabel");
        QVERIFY(banner->text().contains("Confirmation required before export"));
        QVERIFY(banner->styleSheet().contains(Theme::Warning));

        const QString shot = screenshotPath("verify_task2_export_dialog_warning.png");
        QVERIFY(dlg.grab().save(shot));
    }

    // ── 3. CRITICAL (>= 95%) ────────────────────────────────────────────────
    {
        CanvasScene scene;
        QScopedPointer<Project> project(makeCriticalProject(scene, assetPath, 185, 185));
        ExportDialog dlg(project.data(), nullptr);
        dlg.show();
        QApplication::processEvents();

        ResourceEstimator estimator;
        ResourceAnalysis a = estimator.analyze(project.data(), project->getDeviceProfile(), "lvgl");
        QCOMPARE(a.status, ResourceAnalysis::CRITICAL);
        QVERIFY(a.ram_percent >= 95.0f);
        verifyDialogState(dlg, a, true);

        auto* banner = dlg.findChild<QLabel*>("statusBannerLabel");
        QVERIFY(banner->styleSheet().contains(Theme::Danger));
        auto* exportBtn = dlg.findChild<QPushButton*>("exportButton");
        QVERIFY(exportBtn->toolTip().contains("Export blocked"));

        const QString shot = screenshotPath("verify_task2_export_dialog_critical.png");
        QVERIFY(dlg.grab().save(shot));
    }
}

void TestLiveTestResources::testExportDialogWarningAsksForConfirmation() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString assetPath = makeAssetFile(tmp, 1024);
    QVERIFY(!assetPath.isEmpty());

    CanvasScene scene;
    QScopedPointer<Project> project(makeWarningProject(scene, assetPath, 173, 172));

    ExportDialog dlg(project.data(), nullptr);
    dlg.show();
    QApplication::processEvents();

    const QString exportDir = tmp.filePath("warning_export");
    auto* pathEdit = dlg.findChild<QLineEdit*>();
    QVERIFY(pathEdit != nullptr);
    pathEdit->setText(exportDir);

    auto* exportBtn = dlg.findChild<QPushButton*>("exportButton");
    QVERIFY(exportBtn != nullptr);
    QVERIFY(exportBtn->isEnabled());

    // 1) Cancelling the confirmation must NOT export
    MessageBoxPump cancelPump;
    cancelPump.expect("Estimated Resource Warning", QMessageBox::Cancel);
    cancelPump.run([&]() { exportBtn->click(); });
    QVERIFY2(cancelPump.seen("Estimated Resource Warning"), "Warning confirmation dialog did not appear");
    QVERIFY(cancelPump.text("Estimated Resource Warning").contains("Do you want to proceed"));
    QVERIFY(cancelPump.text("Estimated Resource Warning").contains("%"));
    QVERIFY2(!QDir(exportDir).exists(), "Export must not run when the warning is cancelled");

    // 2) Accepting the confirmation exports successfully
    MessageBoxPump acceptPump;
    acceptPump.expect("Estimated Resource Warning", QMessageBox::Yes);
    acceptPump.expect("Export Succeeded", QMessageBox::Ok);
    acceptPump.run([&]() { exportBtn->click(); });
    QVERIFY(acceptPump.seen("Estimated Resource Warning"));
    QVERIFY2(acceptPump.seen("Export Succeeded"), "Export did not proceed after confirming the warning");
    QVERIFY(QDir(exportDir).exists());
    QVERIFY(!QDir(exportDir).entryList(QDir::Files).isEmpty());
}

void TestLiveTestResources::testExportDialogNoDeviceInfoNotice() {
    CanvasScene scene;
    QScopedPointer<Project> project(new Project(&scene));
    project->setTargetFramework("lvgl");
    Hardware::HardwareConfig emptyHw;
    project->setHardwareConfig(emptyHw);

    ExportDialog dlg(project.data(), nullptr);
    dlg.show();
    QApplication::processEvents();

    auto* group = dlg.findChild<QGroupBox*>("resourceGroupBox");
    QVERIFY(group && group->isVisible());

    auto* banner = dlg.findChild<QLabel*>("statusBannerLabel");
    QVERIFY(banner != nullptr);
    QVERIFY(banner->text().contains("No device selected"));
    QVERIFY(banner->text().contains("will not block export"));
    QVERIFY(banner->styleSheet().contains(Theme::Info));

    auto* warnings = dlg.findChild<QListWidget*>("warningListWidget");
    QVERIFY(warnings != nullptr);
    QVERIFY(warnings->count() >= 1);
    bool infoNotice = false;
    for (int i = 0; i < warnings->count(); ++i) {
        if (warnings->item(i)->text().startsWith("[INFO]") &&
            warnings->item(i)->text().contains("No device selected")) {
            infoNotice = true;
        }
    }
    QVERIFY(infoNotice);

    // Export is NOT blocked
    auto* exportBtn = dlg.findChild<QPushButton*>("exportButton");
    QVERIFY(exportBtn != nullptr);
    QVERIFY(exportBtn->isEnabled());

    // End-to-end proof: export runs (into a temp dir) with no device selected
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString exportDir = tmp.filePath("no_device_export");
    auto* pathEdit = dlg.findChild<QLineEdit*>();
    QVERIFY(pathEdit != nullptr);
    pathEdit->setText(exportDir);

    MessageBoxPump pump;
    pump.expect("Export Succeeded", QMessageBox::Ok);
    pump.run([&]() { exportBtn->click(); });
    QVERIFY2(pump.seen("Export Succeeded"), "Export was blocked although no device is selected");
    QVERIFY(QDir(exportDir).exists());
    QVERIFY(!QDir(exportDir).entryList(QDir::Files).isEmpty());

    const QString shot = screenshotPath("verify_task2_export_dialog_no_device.png");
    QVERIFY(dlg.grab().save(shot));
}

void TestLiveTestResources::testOptimizeDialogSortedByWeight() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString assetPath = makeAssetFile(tmp, 1024);

    CanvasScene scene;
    QScopedPointer<Project> project(new Project(&scene));
    project->setTargetFramework("lvgl");
    setDevice(*project, "STM32F401");
    auto* screen = new Screen("OptimizeScreen");
    screen->addComponent(new ButtonComponent("opt_btn_1"));
    screen->addComponent(new ButtonComponent("opt_btn_2"));
    screen->addComponent(new ButtonComponent("opt_btn_3"));
    screen->addComponent(new LabelComponent("opt_lbl_1"));
    screen->addComponent(new LabelComponent("opt_lbl_2"));
    auto* img = new ImageComponent("opt_img");
    img->setImagePath(assetPath);
    img->setCompSize(64, 64);
    screen->addComponent(img);
    project->addScreen(screen);

    ExportDialog dlg(project.data(), nullptr);
    dlg.show();
    QApplication::processEvents();

    auto* optimizeBtn = dlg.findChild<QPushButton*>("optimizeButton");
    QVERIFY(optimizeBtn != nullptr);

    QStringList capturedHeaders;
    QStringList capturedTypes;
    QVector<qreal> capturedWeights;
    bool captured = false;
    QString titleLabelText;

    QTimer timer;
    timer.setInterval(10);
    QElapsedTimer total;
    total.start();
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        QDialog* modal = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!modal || modal->windowTitle() != "Estimated Heaviest Components & Optimization") {
            if (total.elapsed() > 8000 && modal) modal->reject();
            return;
        }
        if (captured) return;
        captured = true;

        const auto labels = modal->findChildren<QLabel*>();
        for (QLabel* l : labels) {
            if (l->text().contains("Sorted Descending")) titleLabelText = l->text();
        }

        auto* table = modal->findChild<QTableWidget*>();
        if (table) {
            for (int c = 0; c < table->columnCount(); ++c) {
                capturedHeaders << table->horizontalHeaderItem(c)->text();
            }
            for (int r = 0; r < table->rowCount(); ++r) {
                capturedTypes << table->item(r, 0)->text();
                const qreal flashKb = table->item(r, 2)->text().toDouble();
                const qreal ramBytes = table->item(r, 3)->text().toDouble();
                capturedWeights << (flashKb * 1024.0 + ramBytes);
            }
            const QString shot = screenshotPath("verify_task2_optimize_dialog.png");
            table->grab().save(shot);
        }
        const QString fullShot = screenshotPath("verify_task2_optimize_dialog_full.png");
        modal->grab().save(fullShot);
        modal->accept();
    });
    timer.start();
    optimizeBtn->click();
    timer.stop();

    QVERIFY2(captured, "Optimize dialog did not open");
    QVERIFY(titleLabelText.contains("Sorted Descending"));
    QVERIFY(capturedHeaders.contains("Component Type"));
    QVERIFY(capturedHeaders.contains("Optimization Suggestion"));
    QCOMPARE(capturedTypes.size(), capturedWeights.size());
    QVERIFY(capturedTypes.size() >= 3); // Button, Label, Image

    // Sorted descending by estimated weight (flash + RAM)
    for (int i = 1; i < capturedWeights.size(); ++i) {
        QVERIFY2(capturedWeights[i - 1] >= capturedWeights[i],
                 "Optimize dialog rows are not sorted by descending weight");
    }

    // Order matches the estimator output
    ResourceEstimator estimator;
    const auto heaviest = estimator.getHeaviestComponents(project.data());
    QStringList expectedTypes;
    for (const auto& e : heaviest) expectedTypes << e.componentType;
    QCOMPARE(capturedTypes, expectedTypes);
    // 3 Buttons (3 x 280 B) outweigh 2 Texts (280 B) and 1 Image (250 B);
    // the table groups rows by component type
    QCOMPARE(capturedTypes.first(), QString("Button"));
    QVERIFY(capturedTypes.contains("Button"));
    QVERIFY(capturedTypes.contains("Text"));
    QVERIFY(capturedTypes.contains("Image"));
    const QSet<QString> uniqueTypes(capturedTypes.begin(), capturedTypes.end());
    QCOMPARE(capturedTypes.size(), uniqueTypes.size());
}

void TestLiveTestResources::testAnalyzerUiUsesThemeColorsOnly() {
    const QSet<QString> themeColors = {
        QString(Theme::BgBase).toUpper(),       QString(Theme::BgSurface).toUpper(),
        QString(Theme::BgRaised).toUpper(),     QString(Theme::BgOverlay).toUpper(),
        QString(Theme::BorderSubtle).toUpper(), QString(Theme::BorderStrong).toUpper(),
        QString(Theme::Accent).toUpper(),       QString(Theme::AccentHover).toUpper(),
        QString(Theme::AccentPressed).toUpper(),QString(Theme::AccentSoft).toUpper(),
        QString(Theme::AccentText).toUpper(),   QString(Theme::TextPrimary).toUpper(),
        QString(Theme::TextSecondary).toUpper(),QString(Theme::TextMuted).toUpper(),
        QString(Theme::TextDisabled).toUpper(), QString(Theme::Success).toUpper(),
        QString(Theme::Warning).toUpper(),      QString(Theme::Danger).toUpper(),
        QString(Theme::Info).toUpper(),         QString(Theme::SuccessBg).toUpper(),
        QString(Theme::WarningBg).toUpper(),    QString(Theme::DangerBg).toUpper(),
        QString(Theme::InfoBg).toUpper(),       QString(Theme::LiveTestActive).toUpper(),
        QString(Theme::CanvasBg).toUpper()
    };

    QRegularExpression hexRe("#([0-9A-Fa-f]{8}|[0-9A-Fa-f]{6}|[0-9A-Fa-f]{3})(?![0-9A-Za-z])");

    CanvasScene scene;
    QScopedPointer<Project> project(makeOkProject(scene));
    ExportDialog dlg(project.data(), nullptr);
    ResourceAnalyzerPanel panel;

    QStringList offenders;
    const QList<QWidget*> roots = {&dlg, &panel};
    for (QWidget* root : roots) {
        QList<QWidget*> widgets = root->findChildren<QWidget*>();
        widgets.prepend(root);
        for (QWidget* w : widgets) {
            const QString ss = w->styleSheet();
            if (ss.isEmpty()) continue;
            auto it = hexRe.globalMatch(ss);
            while (it.hasNext()) {
                const QString hex = it.next().captured(0).toUpper();
                if (!themeColors.contains(hex)) {
                    offenders << QString("%1.%2 uses hardcoded color %3")
                                     .arg(w->metaObject()->className(), w->objectName(), hex);
                }
            }
        }
    }
    QVERIFY2(offenders.isEmpty(), qPrintable(offenders.join("; ")));

    const QString shot = screenshotPath("verify_task2_resource_analyzer_panel.png");
    QVERIFY(panel.grab().save(shot));
}

// ─────────────────────────────────────────────────────────────────────────────
// Part B — Resource Analyzer panel in Live Test Mode
// ─────────────────────────────────────────────────────────────────────────────
void TestLiveTestResources::testLiveTestKeepsResourceAnalyzerVisible() {
    MainWindow win;
    win.resize(1600, 1000);
    win.show();
    QApplication::processEvents();

    auto* dock = win.findChild<QDockWidget*>("resourceDock");
    QVERIFY2(dock != nullptr, "Resource Analyzer dock missing");
    dock->show();
    dock->raise();
    QApplication::processEvents();
    QVERIFY(dock->isVisible());

    auto* panel = win.resourceAnalyzerPanel();
    QVERIFY(panel != nullptr);
    auto* banner = win.findChild<QLabel*>("liveTestBannerLabel");
    QVERIFY2(banner != nullptr, "Live Test banner label missing");

    auto* button = win.findChild<QToolButton*>("liveTestButton");
    QVERIFY2(button != nullptr, "Live Test toggle button missing");
    QVERIFY(!button->isChecked());

    // OFF: banner hidden, panel visible
    QVERIFY(!banner->isVisible());
    QVERIFY(!panel->isHidden());

    // ── Toggle ON ───────────────────────────────────────────────────────────
    button->click();
    QApplication::processEvents();

    QVERIFY(win.canvasView()->isLiveTestMode());
    QVERIFY(!dock->isHidden());
    QVERIFY2(dock->isVisible(), "Resource Analyzer dock must stay visible while Live Test is ON");
    QVERIFY(!panel->isHidden());
    QVERIFY2(banner->isVisible(), "Live Test banner must be visible while Live Test is ON");

    const QString device = panel->lastAnalysis().device_name;
    QVERIFY(!device.isEmpty());
    QVERIFY(banner->text().startsWith("Live Test: the design runs on your PC. Figures show what the exported firmware would need on "));
    QVERIFY2(banner->text().contains(device),
             qPrintable(QString("Banner should name the analyzed device %1: %2").arg(device, banner->text())));
    QVERIFY(banner->text().endsWith("."));

    const QString onShot = screenshotPath("verify_task2_live_test_banner_on.png");
    QVERIFY(win.grab().save(onShot));

    // Bars/warnings stay live and accurate while Live Test is ON
    ResourceEstimator estimator;
    ResourceAnalysis expected = estimator.analyzeProject(win.currentProject(),
                                                         panel->lastAnalysis().device,
                                                         panel->lastAnalysis().target_framework);
    QCOMPARE(panel->lastAnalysis().flash_used, expected.flash_used);
    QCOMPARE(panel->lastAnalysis().ram_used, expected.ram_used);
    QCOMPARE(panel->lastAnalysis().status, expected.status);

    // ── Toggle OFF ──────────────────────────────────────────────────────────
    button->click();
    QApplication::processEvents();

    QVERIFY(!win.canvasView()->isLiveTestMode());
    QVERIFY(!dock->isHidden());
    QVERIFY2(dock->isVisible(), "Resource Analyzer dock must stay visible after Live Test is switched OFF");
    QVERIFY2(!banner->isVisible(), "Live Test banner must be hidden while Live Test is OFF");

    // Rapid toggling must not hide the panel at any point
    for (int i = 0; i < 10; ++i) {
        button->click();
        QApplication::processEvents();
        QVERIFY(!dock->isHidden());
        QVERIFY(!panel->isHidden());
        QCOMPARE(banner->isVisible(), button->isChecked());
    }
    if (button->isChecked()) {
        button->click();
        QApplication::processEvents();
    }
    QVERIFY(!banner->isVisible());
}

void TestLiveTestResources::testOverLimitDesignRunsLiveTestButExportStillBlocked() {
    MainWindow win;
    win.resize(1600, 1000);
    win.show();
    QApplication::processEvents();

    auto* dock = win.findChild<QDockWidget*>("resourceDock");
    QVERIFY(dock != nullptr);
    dock->show();
    dock->raise();
    QApplication::processEvents();

    // Oversized design: STM32F030R8 has 64 KB flash / 8 KB RAM — far too small
    Project* project = win.currentProject();
    QVERIFY(project != nullptr);
    project->setTargetFramework("qt-for-mcus");
    Hardware::HardwareConfig hw;
    hw.deviceId = "STM32F030R8";
    project->setHardwareConfig(hw);

    for (int s = 0; s < 5; ++s) {
        Screen* scr = new Screen(QString("LiveScreen_%1").arg(s));
        for (int c = 0; c < 20; ++c) {
            scr->addComponent(new ButtonComponent(QString("live_btn_%1_%2").arg(s).arg(c)));
            scr->addComponent(new LabelComponent(QString("live_lbl_%1_%2").arg(s).arg(c)));
        }
        project->addScreen(scr);
    }

    ResourceEstimator estimator;
    ResourceAnalysis analysis = estimator.analyzeProject(project, "qt-for-mcus");
    QCOMPARE(analysis.status, ResourceAnalysis::CRITICAL);

    // ── Live Test is NOT blocked by resource warnings ──────────────────────
    auto* button = win.findChild<QToolButton*>("liveTestButton");
    QVERIFY(button != nullptr);
    if (!button->isChecked()) {
        button->click();
        QApplication::processEvents();
    }
    QVERIFY2(win.canvasView()->isLiveTestMode(), "Live Test must run even for an over-limit design");
    QVERIFY(QApplication::activeModalWidget() == nullptr);
    QVERIFY(!dock->isHidden());
    QVERIFY(dock->isVisible());

    auto* banner = win.findChild<QLabel*>("liveTestBannerLabel");
    QVERIFY(banner != nullptr && banner->isVisible());
    QVERIFY(banner->text().contains("STM32F030R8"));

    auto* panel = win.resourceAnalyzerPanel();
    QVERIFY(panel != nullptr);
    QCOMPARE(panel->lastAnalysis().status, ResourceAnalysis::CRITICAL);
    QVERIFY(panel->lastAnalysis().flash_percent >= 95.0f || panel->lastAnalysis().ram_percent >= 95.0f);

    const QString liveShot = screenshotPath("verify_task2_live_test_over_limit.png");
    QVERIFY(win.grab().save(liveShot));

    // ── Export remains blocked exactly as in Task 1 ────────────────────────
    ExportDialog dlg(project, nullptr);
    auto* exportBtn = dlg.findChild<QPushButton*>("exportButton");
    QVERIFY(exportBtn != nullptr);
    QVERIFY(!exportBtn->isEnabled());
    auto* statusBanner = dlg.findChild<QLabel*>("statusBannerLabel");
    QVERIFY(statusBanner != nullptr);
    QVERIFY(statusBanner->text().contains("exceeds device limits"));

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    FirmwareExportManager exportMgr;
    ExportOptions opts;
    opts.outputDirectory = tmp.path();
    opts.targetFramework = "qt-for-mcus";
    opts.forceExport = false;
    ExportResult res = exportMgr.exportProject(project, opts);
    QVERIFY(!res.success);
    QVERIFY(res.errorMessage.contains("Export blocked"));

    if (button->isChecked()) {
        button->click();
        QApplication::processEvents();
    }
}

QTEST_MAIN(TestLiveTestResources)
#include "test_live_test_resources.moc"
