/**
 * Task 7 - Minimal VS Code-style UI acceptance tests.
 *
 * Phase A coverage:
 *  - The Lucide icon set ships as resources (SVG + ISC license) and renders
 *    recolored from theme tokens.
 *  - Icon-state tokens (normal / hover / active / disabled) exist and keep
 *    >= 4.5:1 contrast on the raised surface for interactive states.
 *  - The main toolbar is icon-only: every action has an icon, a tooltip with
 *    its shortcut hint, and an accessible name on its tool button.
 *  - The workspace switcher and Live Test button keep their identity
 *    (text / objectName) while gaining icons; the switcher uses theme tokens
 *    only (no hardcoded hex).
 */
#include <QTest>
#include <QAction>
#include <QApplication>
#include <QFile>
#include <QKeySequence>
#include <QToolBar>
#include <QToolButton>
#include <QSettings>
#include <QCheckBox>

#include <cmath>

#include "MainWindow.h"
#include "theme/Theme.h"
#include "ui/IconProvider.h"

namespace {

qreal channelLuminance(qreal component)
{
    component /= 255.0;
    return component <= 0.03928 ? component / 12.92
                                : std::pow((component + 0.055) / 1.055, 2.4);
}

qreal relativeLuminance(const QColor& color)
{
    return 0.2126 * channelLuminance(color.red())
         + 0.7152 * channelLuminance(color.green())
         + 0.0722 * channelLuminance(color.blue());
}

qreal contrastRatio(const QColor& a, const QColor& b)
{
    const qreal la = relativeLuminance(a);
    const qreal lb = relativeLuminance(b);
    return (qMax(la, lb) + 0.05) / (qMin(la, lb) + 0.05);
}

} // namespace

class TestMinimalUi : public QObject {
    Q_OBJECT

private slots:
    void iconResourcesExist();
    void iconStateTokensHaveContrast();
    void iconsRenderRecolored();
    void mainToolbarIsIconOnly();
    void workspaceSwitcherUsesIconsAndThemeTokens();
    void showLabelsToggleRestoresTextBesideIcons();
    void layoutStateRoundTrips();
    void liveTestButtonKeepsIdentityAndGainsIcon();
    void propertiesDockVisibleAfterFirstLaunch();
    void activityBarHasEightIconActions();
};

void TestMinimalUi::iconResourcesExist()
{
    const QStringList iconNames = {
        "activity", "circuit-board", "code", "download", "pencil-ruler", "play",
        "redo-2", "scan", "undo-2", "zoom-in", "zoom-out",
    };
    for (const QString& name : iconNames) {
        QFile file(Icons::resourcePath(name));
        QVERIFY2(file.exists(), qPrintable("missing icon resource " + Icons::resourcePath(name)));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray data = file.readAll();
        QVERIFY2(data.contains("<svg"), qPrintable(name + " is not an SVG"));
        QVERIFY2(data.contains("currentColor"), qPrintable(name + " must use currentColor"));
    }
    QVERIFY(QFile(":/icons/LICENSE").exists());
}

void TestMinimalUi::iconStateTokensHaveContrast()
{
    // WCAG AA (>= 4.5:1) for interactive icon states on the raised surface
    // (toolbars, buttons). Disabled controls are exempt per WCAG, but the
    // token must stay the shared TextDisabled color.
    const QColor surface(Theme::BgRaised);
    QVERIFY(contrastRatio(QColor(Theme::IconNormal), surface) >= 4.5);
    QVERIFY(contrastRatio(QColor(Theme::IconHover), surface) >= 4.5);
    QVERIFY(contrastRatio(QColor(Theme::IconActive), surface) >= 4.5);
    QCOMPARE(QString(Theme::IconDisabled), QString(Theme::TextDisabled));
}

void TestMinimalUi::iconsRenderRecolored()
{
    const QPixmap normal = Icons::pixmap("settings", Theme::IconNormal, Icons::ActionIconSize, 1.0);
    QVERIFY(!normal.isNull());
    QCOMPARE(normal.width(), Icons::ActionIconSize);

    // The SVG is recolored: two token colors must yield different pixels.
    const QPixmap active = Icons::pixmap("settings", Theme::IconActive, Icons::ActionIconSize, 1.0);
    QVERIFY(!active.isNull());
    QVERIFY(normal.toImage() != active.toImage());

    // High-DPI rendering (2x) for scaled displays.
    const QPixmap hidpi = Icons::pixmap("settings", Theme::IconNormal, Icons::ActionIconSize, 2.0);
    QCOMPARE(hidpi.width(), Icons::ActionIconSize * 2);
    QCOMPARE(hidpi.devicePixelRatio(), 2.0);
}

void TestMinimalUi::mainToolbarIsIconOnly()
{
    MainWindow win;
    QToolBar* toolbar = win.findChild<QToolBar*>("mainToolbar");
    QVERIFY(toolbar);
    QCOMPARE(toolbar->toolButtonStyle(), Qt::ToolButtonIconOnly);
    QCOMPARE(toolbar->iconSize(), QSize(Icons::ActionIconSize, Icons::ActionIconSize));

    int decoratedActions = 0;
    for (QAction* action : toolbar->actions()) {
        if (action->isSeparator() || action->text().isEmpty()) {
            continue; // separators and embedded widgets (switcher, combo, ...)
        }
        QVERIFY2(!action->icon().isNull(), qPrintable("missing icon: " + action->text()));
        QVERIFY2(!action->toolTip().isEmpty(), qPrintable("missing tooltip: " + action->text()));

        auto* button = qobject_cast<QToolButton*>(toolbar->widgetForAction(action));
        QVERIFY2(button != nullptr, qPrintable("no tool button for: " + action->text()));
        QVERIFY2(!button->accessibleName().isEmpty(),
                 qPrintable("missing accessible name: " + action->text()));
        ++decoratedActions;
    }
    QVERIFY(decoratedActions >= 8);

    auto tooltipFor = [toolbar](const QString& text) {
        for (QAction* action : toolbar->actions()) {
            if (action->text() == text) {
                return action->toolTip();
            }
        }
        return QString();
    };

    const QString undoHint = QKeySequence(QKeySequence::Undo).toString(QKeySequence::NativeText);
    const QString redoHint = QKeySequence(QKeySequence::Redo).toString(QKeySequence::NativeText);
    QVERIFY(tooltipFor("Undo").contains(undoHint));
    QVERIFY(tooltipFor("Redo").contains(redoHint));
    QVERIFY(tooltipFor("▶ Run Simulator").contains("Ctrl+R"));
    QVERIFY(tooltipFor("Export...").contains("Ctrl+E"));
    QVERIFY(tooltipFor("−").contains("Ctrl+-"));
    QVERIFY(tooltipFor("+").contains("Ctrl++"));
    QVERIFY(!tooltipFor("100%").isEmpty());
    QVERIFY(!tooltipFor("Change Board...").isEmpty());
}

void TestMinimalUi::workspaceSwitcherUsesIconsAndThemeTokens()
{
    MainWindow win;
    QWidget* switcher = win.findChild<QWidget*>("workspaceSwitcher");
    QVERIFY(switcher);

    const auto buttons = switcher->findChildren<QToolButton*>();
    QCOMPARE(buttons.size(), 2);

    QToolButton* designButton = nullptr;
    QToolButton* codeButton = nullptr;
    for (QToolButton* button : buttons) {
        QVERIFY(!button->icon().isNull());
        QVERIFY(!button->toolTip().isEmpty());
        QVERIFY(!button->accessibleName().isEmpty());
        QCOMPARE(button->toolButtonStyle(), Qt::ToolButtonIconOnly);
        if (button->text() == "Design / Canvas") {
            designButton = button;
        } else if (button->text() == "</> Code") {
            codeButton = button;
        }
    }
    // Text is kept for compatibility with existing UI tests and screen readers.
    QVERIFY(designButton != nullptr);
    QVERIFY(codeButton != nullptr);
    QVERIFY(designButton->toolTip().contains("Ctrl+1"));
    QVERIFY(codeButton->toolTip().contains("Ctrl+2"));

    // Theme tokens only - no hardcoded hex (Task 7 theme rule).
    const QString qss = switcher->styleSheet();
    QVERIFY2(!qss.contains("#1f6feb"), "hardcoded blue must be replaced by theme tokens");
    QVERIFY2(!qss.contains("#388bfd"), "hardcoded blue must be replaced by theme tokens");
    QVERIFY(qss.contains(Theme::BgRaised));
    QVERIFY(qss.contains(Theme::AccentSoft));
    QVERIFY(qss.contains(Theme::AccentText));
}

void TestMinimalUi::liveTestButtonKeepsIdentityAndGainsIcon()
{
    MainWindow win;
    auto* button = win.findChild<QToolButton*>("liveTestButton");
    QVERIFY(button);
    QCOMPARE(button->text(), QString("● Live Test"));
    QVERIFY(!button->icon().isNull());
    QVERIFY(button->toolTip().contains("Ctrl+L"));
    QVERIFY(!button->accessibleName().isEmpty());
    QCOMPARE(button->toolButtonStyle(), Qt::ToolButtonIconOnly);
}

void TestMinimalUi::activityBarHasEightIconActions()
{
    MainWindow win;
    QWidget* activityBar = win.findChild<QWidget*>("activityBar");
    QVERIFY(activityBar);
    QCOMPARE(activityBar->width(), 48);

    const auto buttons = activityBar->findChildren<QToolButton*>();
    QCOMPARE(buttons.size(), 8);

    for (int i = 0; i < 8; ++i) {
        QToolButton* button = win.findChild<QToolButton*>(QString("activityItem%1").arg(i));
        QVERIFY2(button != nullptr, qPrintable(QString("missing activity item %1").arg(i)));
        QVERIFY2(!button->icon().isNull(), qPrintable(QString("activity %1 missing icon").arg(i)));
        QVERIFY2(!button->toolTip().isEmpty(), qPrintable(QString("activity %1 missing tooltip").arg(i)));
        QVERIFY2(!button->accessibleName().isEmpty(), qPrintable(QString("activity %1 missing accessible name").arg(i)));
        QCOMPARE(button->toolButtonStyle(), Qt::ToolButtonIconOnly);
    }
}

void TestMinimalUi::propertiesDockVisibleAfterFirstLaunch()
{
    {
        QSettings s("EmbeddedUIDesigner", "EmbeddedUIDesigner");
        s.clear();
    }
    MainWindow win;
    win.resize(1600, 1000);
    win.show();
    QApplication::processEvents();
    auto* dock = win.findChild<QDockWidget*>("propertiesDock");
    QVERIFY(dock != nullptr);
    QVERIFY2(dock->isVisible(),
             "Properties dock should be visible on a default (upgraded) launch");
}

void TestMinimalUi::showLabelsToggleRestoresTextBesideIcons()
{
    MainWindow win;
    win.resize(1600, 1000);
    win.show();
    QApplication::processEvents();

    auto* cb = win.findChild<QCheckBox*>("showLabelsCheckbox");
    if (!cb) {
        QSKIP("settings checkbox not reachable in this configuration");
    }
    auto* toolbar = win.findChild<QToolBar*>("mainToolbar");
    QVERIFY(toolbar);

    QAction* undoAct = nullptr;
    for (QAction* act : toolbar->actions()) {
        if (act->text() == "Undo") undoAct = act;
    }
    QVERIFY(undoAct);

    auto* btn = qobject_cast<QToolButton*>(toolbar->widgetForAction(undoAct));
    QVERIFY(btn);

    cb->setChecked(true);
    for (int i = 0; i < 5; ++i) QApplication::processEvents();
    QCOMPARE(btn->toolButtonStyle(), Qt::ToolButtonTextBesideIcon);

    cb->setChecked(false);
    for (int i = 0; i < 5; ++i) QApplication::processEvents();
    QCOMPARE(btn->toolButtonStyle(), Qt::ToolButtonIconOnly);
}

void TestMinimalUi::layoutStateRoundTrips()
{
    MainWindow win;
    win.saveWindowLayout();
    QSettings s("EmbeddedUIDesigner", "EmbeddedUIDesigner");
    QVERIFY(s.contains("window/geometry"));
    QVERIFY(s.value("ui/layoutVersion").toInt() >= 2);
    QVERIFY(s.value("ui/activeActivity").toInt() >= 0);
    // Ensure restore does not crash on the persisted state.
    win.restoreWindowLayout();
    s.clear();
}

QTEST_MAIN(TestMinimalUi)
#include "test_minimal_ui.moc"
