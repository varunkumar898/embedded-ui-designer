/**
 * Task 3 — Diagnostics Service + Problems Panel acceptance suite.
 *
 * Covers:
 *  - DiagnosticsService aggregation from multiple registered sources.
 *  - Severity filtering, counts and the filter buttons in the Problems panel.
 *  - Built-in sources: missing assets (Error) and out-of-screen components (Warning).
 *  - Double-click navigation: switch screen, select component, scroll into view
 *    (and stay quiet when a diagnostic has no component).
 *  - Status-bar badge counts.
 *  - DarkTheme-token-only colors.
 *  - Debounced recompute stays well under 100 ms for a typical project.
 *  - Screenshot of the panel with a mix of Error/Warning/Info.
 */
#include <QTest>
#include <QObject>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QLabel>
#include <QRegularExpression>
#include <QSet>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QToolButton>
#include <QDockWidget>

#include "diagnostics/DiagnosticsService.h"
#include "diagnostics/DiagnosticsSources.h"
#include "panels/ProblemsPanel.h"
#include "project/Project.h"
#include "canvas/CanvasScene.h"
#include "models/Screen.h"
#include "models/ButtonComponent.h"
#include "models/ImageComponent.h"
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

namespace {

Diagnostic makeDiagnostic(Diagnostic::Severity severity, const QString& message,
                          const QString& source, const QString& screenId = QString(),
                          const QString& componentId = QString()) {
    Diagnostic d;
    d.severity = severity;
    d.message = message;
    d.source = source;
    d.screenId = screenId;
    d.componentId = componentId;
    return d;
}

int findRowBySource(QTableWidget* table, const QString& source) {
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* item = table->item(row, 2);
        if (item && item->text() == source) return row;
    }
    return -1;
}

} // namespace

class TestProblemsDiagnostics : public QObject {
    Q_OBJECT

private slots:
    void testServiceAggregatesRegisteredSources();
    void testSeverityFilteringAndCounts();
    void testMissingAssetAndOutOfBoundsSources();
    void testDoubleClickNavigatesToComponent();
    void testStatusBarBadgeReflectsCounts();
    void testPanelUsesThemeColorsOnly();
    void testRefreshPerformanceUnder100ms();
    void testCaptureProblemsPanelScreenshot();
};

// ─────────────────────────────────────────────────────────────────────────────
// Service aggregation
// ─────────────────────────────────────────────────────────────────────────────
void TestProblemsDiagnostics::testServiceAggregatesRegisteredSources() {
    CanvasScene scene;
    Project project(&scene);
    Diagnostics::DiagnosticsService service;
    service.setProject(&project);

    service.registerSource("SourceA", [](const Project* project) {
        QVector<Diagnostic> out;
        if (!project) return out;
        out << makeDiagnostic(Diagnostic::Severity::Warning, "warning A", "SourceA");
        out << makeDiagnostic(Diagnostic::Severity::Error, "error A", "SourceA");
        return out;
    });
    service.registerSource("SourceB", [](const Project* project) {
        QVector<Diagnostic> out;
        if (!project) return out;
        out << makeDiagnostic(Diagnostic::Severity::Info, "info B", "SourceB");
        return out;
    });

    QCOMPARE(service.sourceNames(), QStringList() << "SourceA" << "SourceB");
    QCOMPARE(service.totalCount(), 0); // nothing computed yet

    const QVector<Diagnostic> all = service.runNow();
    QCOMPARE(all.size(), 3);
    QCOMPARE(service.totalCount(), 3);
    QCOMPARE(service.errorCount(), 1);
    QCOMPARE(service.warningCount(), 1);
    QCOMPARE(service.infoCount(), 1);

    // Diagnostics are ordered Error → Warning → Info.
    QVERIFY(all.at(0).severity == Diagnostic::Severity::Error);
    QVERIFY(all.at(1).severity == Diagnostic::Severity::Warning);
    QVERIFY(all.at(2).severity == Diagnostic::Severity::Info);
    QCOMPARE(service.severityLabel(Diagnostic::Severity::Warning), QString("Warning"));

    // A null project must not crash sources and produces no diagnostics.
    service.setProject(nullptr);
    QCOMPARE(service.runNow().size(), 0);

    service.clearSources();
    QCOMPARE(service.sourceNames().size(), 0);
    QCOMPARE(service.runNow().size(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Filtering and counts
// ─────────────────────────────────────────────────────────────────────────────
void TestProblemsDiagnostics::testSeverityFilteringAndCounts() {
    Diagnostics::DiagnosticsService service;
    service.registerSource("Mixed", [](const Project*) {
        QVector<Diagnostic> out;
        out << makeDiagnostic(Diagnostic::Severity::Error, "one error", "Mixed");
        out << makeDiagnostic(Diagnostic::Severity::Warning, "warning 1", "Mixed");
        out << makeDiagnostic(Diagnostic::Severity::Warning, "warning 2", "Mixed");
        out << makeDiagnostic(Diagnostic::Severity::Info, "one info", "Mixed");
        return out;
    });
    service.runNow();

    ProblemsPanel panel(&service);
    QApplication::processEvents();

    QCOMPARE(panel.countForSeverity(Diagnostic::Severity::Error), 1);
    QCOMPARE(panel.countForSeverity(Diagnostic::Severity::Warning), 2);
    QCOMPARE(panel.countForSeverity(Diagnostic::Severity::Info), 1);

    auto* allBtn = panel.findChild<QToolButton*>("filterAllButton");
    auto* errBtn = panel.findChild<QToolButton*>("filterErrorsButton");
    auto* warnBtn = panel.findChild<QToolButton*>("filterWarningsButton");
    auto* infoBtn = panel.findChild<QToolButton*>("filterInfoButton");
    QVERIFY(allBtn && errBtn && warnBtn && infoBtn);
    QCOMPARE(allBtn->text(), QString("All (4)"));
    QCOMPARE(errBtn->text(), QString("Errors (1)"));
    QCOMPARE(warnBtn->text(), QString("Warnings (2)"));
    QCOMPARE(infoBtn->text(), QString("Info (1)"));

    panel.setFilter(ProblemsPanel::Filter::Errors);
    QCOMPARE(panel.visibleRowCount(), 1);
    panel.setFilter(ProblemsPanel::Filter::Warnings);
    QCOMPARE(panel.visibleRowCount(), 2);
    panel.setFilter(ProblemsPanel::Filter::Info);
    QCOMPARE(panel.visibleRowCount(), 1);
    panel.setFilter(ProblemsPanel::Filter::All);
    QCOMPARE(panel.visibleRowCount(), 4);

    auto* table = panel.findChild<QTableWidget*>("problemsTable");
    QVERIFY(table);
    QCOMPARE(table->rowCount(), 4);
    QCOMPARE(table->columnCount(), 4);
    QCOMPARE(table->item(0, 0)->text(), QString("Error"));
    QCOMPARE(table->item(1, 0)->text(), QString("Warning"));
    QCOMPARE(table->item(3, 0)->text(), QString("Info"));
    QCOMPARE(table->item(0, 1)->text(), QString("one error"));

    // Severity is communicated by color AND text (color is never the only signal).
    QVERIFY(table->item(0, 0)->foreground().color() == QColor(Theme::Danger));
    QVERIFY(table->item(1, 0)->foreground().color() == QColor(Theme::Warning));
    QVERIFY(table->item(3, 0)->foreground().color() == QColor(Theme::Info));
}

// ─────────────────────────────────────────────────────────────────────────────
// Built-in sources: missing asset + out of screen bounds
// ─────────────────────────────────────────────────────────────────────────────
void TestProblemsDiagnostics::testMissingAssetAndOutOfBoundsSources() {
    CanvasScene scene;
    Project project(&scene);

    auto* screen = new Screen("BoundsScreen", "Bounds Screen");
    project.addScreen(screen);

    auto* image = new ImageComponent("img_missing");
    image->setImagePath("/nonexistent/task3/definitely_missing_asset.png");
    screen->addComponent(image);

    auto* offscreen = new ButtonComponent("btn_oob");
    offscreen->setPos(-40, -30);
    screen->addComponent(offscreen);

    project.setActiveScreen(screen);

    Diagnostics::DiagnosticsService service;
    Diagnostics::DiagnosticsSources::registerBuiltinSources(service);
    service.setProject(&project);
    service.runNow();

    bool sawAsset = false;
    bool sawLayout = false;
    for (const Diagnostic& d : service.diagnostics()) {
        if (d.source == QStringLiteral("Asset")) {
            sawAsset = true;
            QVERIFY(d.severity == Diagnostic::Severity::Error);
            QCOMPARE(d.componentId, QString("img_missing"));
            QCOMPARE(d.screenId, screen->id());
            QVERIFY(d.message.contains("definitely_missing_asset.png"));
        }
        if (d.source == QStringLiteral("Layout")) {
            sawLayout = true;
            QVERIFY(d.severity == Diagnostic::Severity::Warning);
            QCOMPARE(d.componentId, QString("btn_oob"));
            QVERIFY(d.message.contains("extends outside"));
        }
    }
    QVERIFY2(sawAsset, "missing asset must produce an Asset/Error diagnostic");
    QVERIFY2(sawLayout, "out-of-bounds component must produce a Layout/Warning diagnostic");

    // Fixing the project clears both diagnostics on the next (debounced) refresh.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString assetPath = dir.filePath("asset.png");
    QFile asset(assetPath);
    QVERIFY(asset.open(QIODevice::WriteOnly));
    asset.write("png");
    asset.close();

    image->setImagePath(assetPath);
    offscreen->setPos(20, 20);
    service.runNow();

    for (const Diagnostic& d : service.diagnostics()) {
        QVERIFY2(d.source != QStringLiteral("Asset"), "asset diagnostic must clear when the file exists");
        QVERIFY2(d.source != QStringLiteral("Layout"), "bounds diagnostic must clear when the component is inside");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Double-click navigation
// ─────────────────────────────────────────────────────────────────────────────
void TestProblemsDiagnostics::testDoubleClickNavigatesToComponent() {
    MainWindow win;
    win.resize(1500, 950);
    win.show();
    QApplication::processEvents();

    Project* project = win.currentProject();
    QVERIFY(project);
    Screen* screenA = project->activeScreen();
    QVERIFY(screenA);

    auto* screenB = new Screen("DiagTargetScreen", "Diag Target Screen");
    project->addScreen(screenB);
    auto* target = new ButtonComponent("diag_target");
    target->setPos(40, 50);
    screenB->addComponent(target);
    const QString screenBId = screenB->id();

    Diagnostics::DiagnosticsService* service = win.diagnosticsService();
    QVERIFY(service);
    service->registerSource("TestNavigation", [screenBId](const Project*) {
        QVector<Diagnostic> out;
        out << makeDiagnostic(Diagnostic::Severity::Error, "navigation test",
                              "TestNavigation", screenBId, "diag_target");
        return out;
    });
    service->runNow();

    auto* panel = win.problemsPanel();
    QVERIFY(panel);
    auto* table = win.findChild<QTableWidget*>("problemsTable");
    QVERIFY(table);

    QVERIFY(project->activeScreen() == screenA);
    QVERIFY(!target->isSelected());

    const int row = findRowBySource(table, "TestNavigation");
    QVERIFY2(row >= 0, "navigation diagnostic row not found in Problems table");

    // Exactly what a user double-click emits.
    QMetaObject::invokeMethod(table, "cellDoubleClicked", Qt::DirectConnection,
                              Q_ARG(int, row), Q_ARG(int, 0));
    QApplication::processEvents();

    QVERIFY2(project->activeScreen() == screenB, "double-click must switch to the owning screen");
    QVERIFY2(target->isSelected(), "double-click must select the component on the canvas");

    // A diagnostic without a component must be ignored quietly.
    service->registerSource("TestNoTarget", [](const Project*) {
        QVector<Diagnostic> out;
        out << makeDiagnostic(Diagnostic::Severity::Info, "no component here", "TestNoTarget");
        return out;
    });
    service->runNow();

    const int noTargetRow = findRowBySource(table, "TestNoTarget");
    QVERIFY(noTargetRow >= 0);
    QMetaObject::invokeMethod(table, "cellDoubleClicked", Qt::DirectConnection,
                              Q_ARG(int, noTargetRow), Q_ARG(int, 0));
    QApplication::processEvents();
    QVERIFY2(project->activeScreen() == screenB, "component-less diagnostics must not navigate");
}

// ─────────────────────────────────────────────────────────────────────────────
// Status-bar badge
// ─────────────────────────────────────────────────────────────────────────────
void TestProblemsDiagnostics::testStatusBarBadgeReflectsCounts() {
    MainWindow win;
    win.show();
    QApplication::processEvents();

    auto* badge = win.findChild<QLabel*>("problemsBadgeLabel");
    QVERIFY2(badge, "problemsBadgeLabel missing from the status bar");

    Diagnostics::DiagnosticsService* service = win.diagnosticsService();
    QVERIFY(service);
    service->clearSources();
    service->registerSource("BadgeCounts", [](const Project*) {
        QVector<Diagnostic> out;
        for (int i = 0; i < 2; ++i) {
            out << makeDiagnostic(Diagnostic::Severity::Error, QString("error %1").arg(i), "BadgeCounts");
        }
        for (int i = 0; i < 5; ++i) {
            out << makeDiagnostic(Diagnostic::Severity::Warning, QString("warning %1").arg(i), "BadgeCounts");
        }
        return out;
    });
    service->runNow();

    QVERIFY2(badge->text().contains("2 errors"), qPrintable(badge->text()));
    QVERIFY2(badge->text().contains("5 warnings"), qPrintable(badge->text()));
}

// ─────────────────────────────────────────────────────────────────────────────
// Theme tokens only
// ─────────────────────────────────────────────────────────────────────────────
void TestProblemsDiagnostics::testPanelUsesThemeColorsOnly() {
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

    Diagnostics::DiagnosticsService service;
    service.registerSource("Theme", [](const Project*) {
        QVector<Diagnostic> out;
        out << makeDiagnostic(Diagnostic::Severity::Error, "error", "Theme");
        out << makeDiagnostic(Diagnostic::Severity::Warning, "warning", "Theme");
        out << makeDiagnostic(Diagnostic::Severity::Info, "info", "Theme");
        return out;
    });
    service.runNow();

    ProblemsPanel panel(&service);

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
// Performance
// ─────────────────────────────────────────────────────────────────────────────
void TestProblemsDiagnostics::testRefreshPerformanceUnder100ms() {
    CanvasScene scene;
    Project project(&scene);

    // Typical project: 3 screens, 60 components total.
    for (int s = 0; s < 3; ++s) {
        auto* screen = new Screen(QString("PerfScreen%1").arg(s));
        for (int i = 0; i < 20; ++i) {
            screen->addComponent(new ButtonComponent(QString("btn_%1_%2").arg(s).arg(i)));
        }
        project.addScreen(screen);
    }

    Diagnostics::DiagnosticsService service;
    Diagnostics::DiagnosticsSources::registerBuiltinSources(service);
    service.setProject(&project);

    service.runNow(); // warm-up (device DB + estimator cache)

    QElapsedTimer timer;
    timer.start();
    const QVector<Diagnostic> diagnostics = service.runNow();
    const qint64 elapsed = timer.elapsed();

    QVERIFY2(elapsed < 100, qPrintable(QString("diagnostics recompute took %1 ms").arg(elapsed)));
    QVERIFY(!diagnostics.isEmpty()); // e.g. the "no device selected" info

    // Debounce must coalesce rapid refreshes into a single recompute.
    int changeCount = 0;
    connect(&service, &Diagnostics::DiagnosticsService::diagnosticsChanged,
            this, [&changeCount]() { ++changeCount; });
    service.scheduleRefresh(20);
    service.scheduleRefresh(20);
    service.scheduleRefresh(20);
    QTest::qWait(150);
    QCOMPARE(changeCount, 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// Screenshot with a mix of Error / Warning / Info
// ─────────────────────────────────────────────────────────────────────────────
void TestProblemsDiagnostics::testCaptureProblemsPanelScreenshot() {
    MainWindow win;
    win.resize(1600, 1000);
    win.show();
    QApplication::processEvents();

    Project* project = win.currentProject();
    QVERIFY(project);
    Screen* screen = project->activeScreen();
    QVERIFY(screen);

    // Error: Image component referencing a file that does not exist.
    auto* image = new ImageComponent("img_dashboard_missing");
    image->setImagePath("assets/dashboard_hero.png");
    screen->addComponent(image);

    // Warning: component outside the screen bounds.
    auto* offscreen = new ButtonComponent("btn_offscreen");
    offscreen->setPos(-30, 12);
    screen->addComponent(offscreen);

    // Info: no target device selected (Resource Analysis info) + a build info
    // from a registered extension source.
    win.diagnosticsService()->registerSource("Build", [](const Project*) {
        QVector<Diagnostic> out;
        out << makeDiagnostic(Diagnostic::Severity::Info, "No build has been run for this project.", "Build");
        return out;
    });
    win.diagnosticsService()->runNow();

    auto* panel = win.problemsPanel();
    QVERIFY(panel);
    QVERIFY2(panel->countForSeverity(Diagnostic::Severity::Error) >= 1, "screenshot needs an error");
    QVERIFY2(panel->countForSeverity(Diagnostic::Severity::Warning) >= 1, "screenshot needs a warning");
    QVERIFY2(panel->countForSeverity(Diagnostic::Severity::Info) >= 1, "screenshot needs an info");

    auto* dock = win.findChild<QDockWidget*>("problemsDock");
    QVERIFY(dock);
    dock->show();
    dock->raise();
    QApplication::processEvents();

    const QString windowShot = screenshotPath("task3_problems_panel_mixed.png");
    QVERIFY(win.grab().save(windowShot));
    const QString panelShot = screenshotPath("task3_problems_panel.png");
    QVERIFY(panel->grab().save(panelShot));
}

QTEST_MAIN(TestProblemsDiagnostics)
#include "test_problems_diagnostics.moc"
