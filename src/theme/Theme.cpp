#include "Theme.h"

namespace Theme {

QString stylesheet() {
    return QString(R"(
/* ====== BLACK & GRAY DESIGN SYSTEM - APPLICATION STYLESHEET (VS Code / Zed style) ====== */

QWidget {
    background-color: %1;
    color: %10;
    font-size: 12px;
    font-weight: 500;
    selection-background-color: %8;
    selection-color: %10;
    outline: none;
}

QMainWindow {
    background-color: %1;
}

/* ── Menu Bar & Menus ────────────────────────────────────── */
QMenuBar {
    background-color: %2;
    color: %10;
    border: none;
    border-bottom: 1px solid %5;
    padding: 2px 6px;
    font-size: 12px;
}

QMenuBar::item {
    background-color: transparent;
    color: %11;
    padding: 5px 10px;
    border-radius: 4px;
}

QMenuBar::item:selected, QMenuBar::item:pressed {
    background-color: %4;
    color: %10;
}

QMenu {
    background-color: %2;
    color: %10;
    border: 1px solid %6;
    border-radius: 8px;
    padding: 6px;
}

QMenu::item {
    background-color: transparent;
    color: %10;
    padding: 6px 20px 6px 12px;
    border-radius: 4px;
    font-size: 12px;
}

QMenu::item:selected {
    background-color: %8;
    color: %9;
}

QMenu::item:disabled {
    color: %13;
}

QMenu::separator {
    height: 1px;
    background-color: %5;
    margin: 4px 6px;
}

/* ── Toolbars & Tool Buttons ─────────────────────────────── */
QToolBar {
    background-color: %3;
    border: none;
    border-bottom: 1px solid %5;
    spacing: 4px;
    padding: 4px 8px;
}

QToolBar::separator {
    width: 1px;
    background-color: %5;
    margin: 4px 6px;
}

QToolButton {
    background-color: transparent;
    color: %10;
    border: 1px solid transparent;
    border-radius: 6px;
    padding: 4px 8px;
    font-size: 12px;
    font-weight: 500;
}

QToolButton:hover {
    background-color: %4;
    border: 1px solid %5;
    color: %10;
}

QToolButton:pressed {
    background-color: %2;
}

QToolButton:checked {
    background-color: %8;
    border: 1px solid %7;
    color: %9;
}

QToolButton::menu-indicator {
    subcontrol-origin: padding;
    subcontrol-position: center right;
    right: 4px;
}

/* ── Dock Widgets ────────────────────────────────────────── */
QDockWidget {
    background-color: %2;
    color: %10;
    border: 1px solid %5;
    font-size: 12px;
}

QDockWidget::title {
    background-color: %2;
    padding: 8px 12px;
    border-bottom: 1px solid %5;
    font-weight: 600;
    font-size: 13px;
    color: %10;
}

#customDockTitle {
    background-color: %2;
    border-bottom: 1px solid %5;
}

#panelTitle {
    color: %10;
    font-size: 13px;
    font-weight: 600;
}

/* ── Push Buttons ────────────────────────────────────────── */
QPushButton {
    background-color: %3;
    color: %10;
    border: 1px solid %5;
    border-radius: 6px;
    padding: 6px 14px;
    font-size: 12px;
    font-weight: 500;
    min-height: 20px;
    outline: none;
}

QPushButton:hover {
    background-color: %4;
    border: 1px solid %6;
}

QPushButton:pressed {
    background-color: %2;
    border: 1px solid %7;
}

QPushButton:disabled {
    background-color: %2;
    color: %13;
    border: 1px solid %5;
}

/* Primary Button (Action focus) */
QPushButton#primaryButton, QPushButton[primary="true"] {
    background-color: %7;
    color: %1;
    border: 1px solid %7;
    font-weight: 600;
}

QPushButton#primaryButton:hover, QPushButton[primary="true"]:hover {
    background-color: %14;
    border: 1px solid %14;
}

QPushButton#primaryButton:pressed, QPushButton[primary="true"]:pressed {
    background-color: %15;
    border: 1px solid %15;
}

/* Flat / Toolbar Button */
QPushButton#flatButton, QPushButton[flat="true"] {
    background-color: transparent;
    color: %11;
    border: none;
    min-width: 0px;
    padding: 3px 6px;
    border-radius: 4px;
}

QPushButton#flatButton:hover, QPushButton[flat="true"]:hover {
    background-color: %4;
    color: %10;
}

QPushButton#flatButton:pressed, QPushButton[flat="true"]:pressed {
    background-color: %2;
}

/* ── Text Inputs ─────────────────────────────────────────── */
QLineEdit, QPlainTextEdit, QTextEdit {
    background-color: %3;
    color: %10;
    border: 1px solid %5;
    border-radius: 6px;
    padding: 6px 10px;
    font-size: 12px;
    selection-background-color: %8;
    selection-color: %9;
}

QLineEdit:hover, QPlainTextEdit:hover, QTextEdit:hover {
    border: 1px solid %6;
}

QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus {
    border: 1px solid %7;
}

QLineEdit::placeholder, QPlainTextEdit::placeholder, QTextEdit::placeholder {
    color: %12;
}

/* ── Spin Boxes ──────────────────────────────────────────── */
QSpinBox, QDoubleSpinBox {
    background-color: %3;
    color: %10;
    border: 1px solid %5;
    border-radius: 6px;
    padding: 4px 8px;
    font-size: 12px;
}

QSpinBox:hover, QDoubleSpinBox:hover {
    border: 1px solid %6;
}

QSpinBox:focus, QDoubleSpinBox:focus {
    border: 1px solid %7;
}

QSpinBox::up-button, QSpinBox::down-button,
QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {
    background-color: transparent;
    border: none;
    width: 16px;
}

/* ── Combo Boxes ─────────────────────────────────────────── */
QComboBox {
    background-color: %3;
    color: %10;
    border: 1px solid %5;
    border-radius: 6px;
    padding: 5px 12px;
    font-size: 12px;
    min-height: 20px;
}

QComboBox:hover {
    border: 1px solid %6;
}

QComboBox:focus, QComboBox:on {
    border: 1px solid %7;
}

QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 24px;
    border: none;
}

QComboBox QAbstractItemView {
    background-color: %2;
    color: %10;
    border: 1px solid %6;
    border-radius: 8px;
    padding: 4px;
    selection-background-color: %8;
    selection-color: %9;
}

/* ── Scroll Bars (Thin 8px rounded) ──────────────────────── */
QScrollBar:vertical {
    background-color: %2;
    width: 8px;
    margin: 0px;
    border-radius: 4px;
}

QScrollBar::handle:vertical {
    background-color: %5;
    min-height: 24px;
    border-radius: 4px;
}

QScrollBar::handle:vertical:hover {
    background-color: %8;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0px;
    background: none;
}

QScrollBar:horizontal {
    background-color: %2;
    height: 8px;
    margin: 0px;
    border-radius: 4px;
}

QScrollBar::handle:horizontal {
    background-color: %5;
    min-width: 24px;
    border-radius: 4px;
}

QScrollBar::handle:horizontal:hover {
    background-color: %8;
}

QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0px;
    background: none;
}

/* ── Tab Widgets & Tab Bars ──────────────────────────────── */
QTabWidget::pane {
    border: 1px solid %5;
    background-color: %2;
    border-radius: 6px;
}

QTabBar::tab {
    background-color: %2;
    color: %12;
    border: 1px solid transparent;
    padding: 6px 14px;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
    font-size: 12px;
    font-weight: 500;
}

QTabBar::tab:hover {
    background-color: %3;
    color: %11;
}

QTabBar::tab:selected {
    background-color: %3;
    color: %10;
    border-bottom: 2px solid %7;
    font-weight: 600;
}

/* ── Item Views (Trees, Lists, Tables) ───────────────────── */
QTreeWidget, QListWidget, QTableWidget {
    background-color: %2;
    color: %10;
    border: 1px solid %5;
    border-radius: 6px;
    padding: 4px;
    font-size: 12px;
}

QTreeWidget::item, QListWidget::item, QTableWidget::item {
    padding: 4px 8px;
    border-radius: 4px;
}

QTreeWidget::item:hover, QListWidget::item:hover, QTableWidget::item:hover {
    background-color: %4;
    color: %10;
}

QTreeWidget::item:selected, QListWidget::item:selected, QTableWidget::item:selected {
    background-color: %8;
    color: %9;
    border: none;
}

QHeaderView::section {
    background-color: %3;
    color: %11;
    border: none;
    border-bottom: 1px solid %5;
    border-right: 1px solid %5;
    padding: 6px 10px;
    font-weight: 600;
    font-size: 12px;
}

/* ── Check Boxes & Radio Buttons ─────────────────────────── */
QCheckBox, QRadioButton {
    color: %10;
    spacing: 8px;
    font-size: 12px;
}

QCheckBox::indicator, QRadioButton::indicator {
    width: 16px;
    height: 16px;
    background-color: %3;
    border: 1px solid %5;
    border-radius: 4px;
}

QRadioButton::indicator {
    border-radius: 8px;
}

QCheckBox::indicator:hover, QRadioButton::indicator:hover {
    border: 1px solid %6;
}

QCheckBox::indicator:checked, QRadioButton::indicator:checked {
    background-color: %7;
    border: 1px solid %7;
}

/* ── Sliders ─────────────────────────────────────────────── */
QSlider::groove:horizontal {
    background-color: %5;
    height: 4px;
    border-radius: 2px;
}

QSlider::sub-page:horizontal {
    background-color: %7;
    height: 4px;
    border-radius: 2px;
}

QSlider::handle:horizontal {
    background-color: %10;
    width: 14px;
    height: 14px;
    margin: -5px 0;
    border-radius: 7px;
    border: 1px solid %6;
}

QSlider::handle:horizontal:hover {
    background-color: %7;
    border: 2px solid %6;
}

/* ── Progress Bars ───────────────────────────────────────── */
QProgressBar {
    background-color: %3;
    border: 1px solid %5;
    border-radius: 4px;
    text-align: center;
    color: %10;
    font-size: 11px;
    height: 12px;
}

QProgressBar::chunk {
    background-color: %7;
    border-radius: 3px;
}

/* ── Group Boxes ─────────────────────────────────────────── */
QGroupBox {
    border: 1px solid %5;
    border-radius: 6px;
    margin-top: 12px;
    padding-top: 12px;
    font-size: 12px;
    font-weight: 600;
    color: %11;
}

QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 10px;
    padding: 0 4px;
    color: %9;
}

/* ── Status Bar ──────────────────────────────────────────── */
QStatusBar {
    background-color: %2;
    border-top: 1px solid %5;
    color: %11;
    font-size: 12px;
}

QStatusBar QLabel {
    background-color: transparent;
}

/* ── Tool Tips ───────────────────────────────────────────── */
QToolTip {
    background-color: %4;
    color: %10;
    border: 1px solid %6;
    border-radius: 6px;
    padding: 6px 10px;
    font-size: 11px;
}

/* ── Splitters ───────────────────────────────────────────── */
QSplitter::handle {
    background-color: %5;
}

QSplitter::handle:hover {
    background-color: %7;
}

/* ── Dialogs ─────────────────────────────────────────────── */
QDialog {
    background-color: %1;
    color: %10;
}
)")
        .arg(BgBase)        // %1
        .arg(BgSurface)     // %2
        .arg(BgRaised)      // %3
        .arg(BgOverlay)     // %4
        .arg(BorderSubtle)  // %5
        .arg(BorderStrong)  // %6
        .arg(Accent)        // %7
        .arg(AccentSoft)    // %8
        .arg(AccentText)    // %9
        .arg(TextPrimary)   // %10
        .arg(TextSecondary) // %11
        .arg(TextMuted)     // %12
        .arg(TextDisabled)  // %13
        .arg(AccentHover)   // %14
        .arg(AccentPressed);// %15
}

} // namespace Theme
