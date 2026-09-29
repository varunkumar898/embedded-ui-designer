#include "MainWindow.h"
#include "QtMcuGenerator.h"
#include "UgfxGenerator.h"
#include "LvglGenerator.h"
#include "ButtonComponent.h"
#include "LabelComponent.h"
#include "RectangleComponent.h"
#include "ProgressBarComponent.h"
#include "ImageComponent.h"
#include "AddComponentCommand.h"
#include "DeleteComponentCommand.h"
#include <QUndoStack>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_scene = new CanvasScene(this);
    m_undoStack = new QUndoStack(this);
    m_scene->setUndoStack(m_undoStack);
    m_view = new CanvasView(m_scene, this);
    m_view->setUndoStack(m_undoStack);
    m_project = new Project(m_scene, this);

    connect(m_undoStack, &QUndoStack::cleanChanged, this, [this](bool clean) {
        m_project->setDirty(!clean);
    });

    setCentralWidget(m_view);

    setupUi();
    setupMenusAndToolbars();
    setupDocks();
    applyTheme();

    setWindowIcon(QIcon(":/packaging/embedded-ui-designer.png"));
    updateWindowTitle();

    // 60-second autosave under QStandardPaths::AppDataLocation
    QTimer* autoSaveTimer = new QTimer(this);
    connect(autoSaveTimer, &QTimer::timeout, this, [this]() {
        if (m_project->isDirty()) {
            m_project->autoSave();
        }
    });
    autoSaveTimer->start(60000);

    // Hook signals
    connect(m_scene, &CanvasScene::componentSelected, m_propertiesPanel, &PropertiesPanel::setTargetComponent);
    connect(m_scene, &CanvasScene::componentChanged, m_propertiesPanel, &PropertiesPanel::refreshValues);
    connect(m_view, &CanvasView::zoomChanged, this, &MainWindow::onZoomChanged);
    connect(m_view, &CanvasView::statusMessageRequested, this, [this](const QString& msg) {
        statusBar()->showMessage(msg, 3000);
    });
    connect(m_project, &Project::projectModified, this, &MainWindow::updateWindowTitle);
    connect(m_project, &Project::projectLoaded, this, [this]() {
        updateWindowTitle();
        m_layerPanel->refreshLayers();
        m_propertiesPanel->setTargetComponent(nullptr);
    });

    resize(1380, 880);
}

void MainWindow::setupUi() {
    m_tipLabel = new QLabel("Tip: Drag item onto canvas", this);
    m_tipLabel->setStyleSheet("padding: 2px 8px; color: #7f8e9f; font-size: 11px; font-weight: 500;");

    m_statusLabel = new QLabel("Ready", this);
    m_statusLabel->setStyleSheet("padding: 2px 8px; color: #a4b3c7; font-size: 11px; font-weight: bold;");

    m_zoomLabel = new QLabel("Zoom: 100%", this);
    m_zoomLabel->setStyleSheet("padding: 2px 14px; color: #a4b3c7; font-size: 11px; font-weight: bold;");

    statusBar()->addWidget(m_tipLabel);
    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->addPermanentWidget(m_zoomLabel);
}

void MainWindow::setupMenusAndToolbars() {
    // Top Menus
    QMenu* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&New Project", this, &MainWindow::onNewProject, QKeySequence::New);
    fileMenu->addAction("&Open Project...", this, &MainWindow::onOpenProject, QKeySequence::Open);
    fileMenu->addAction("Open &Sample Project (Thermostat)", this, &MainWindow::onOpenSampleProject);
    fileMenu->addSeparator();
    fileMenu->addAction("&Save", this, &MainWindow::onSaveProject, QKeySequence::Save);
    fileMenu->addAction("Save &As...", this, &MainWindow::onSaveProjectAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("Export &µGFX C Project...", this, &MainWindow::onExportUgfx, QKeySequence(Qt::CTRL | Qt::Key_E));
    fileMenu->addAction("Export &Qt for MCUs (QUL) Project...", this, &MainWindow::onExportQtMcu, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E));
    fileMenu->addAction("Export &LVGL (C/C++) Project...", this, &MainWindow::onExportLvgl, QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_L));
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence::Quit);

    QMenu* editMenu = menuBar()->addMenu("&Edit");
    QAction* undoAct = m_undoStack->createUndoAction(this, "&Undo");
    undoAct->setShortcut(QKeySequence::Undo);
    QAction* redoAct = m_undoStack->createRedoAction(this, "&Redo");
    redoAct->setShortcut(QKeySequence::Redo);

    editMenu->addAction(undoAct);
    editMenu->addAction(redoAct);
    editMenu->addSeparator();
    editMenu->addAction("&Duplicate", this, &MainWindow::onDuplicateSelected, QKeySequence(Qt::CTRL | Qt::Key_D));
    editMenu->addAction("&Delete", this, &MainWindow::onDeleteSelected, QKeySequence::Delete);

    QMenu* viewMenu = menuBar()->addMenu("&View");
    QAction* actGrid = viewMenu->addAction("Show &Grid");
    actGrid->setCheckable(true);
    actGrid->setChecked(true);
    connect(actGrid, &QAction::toggled, this, &MainWindow::onToggleGrid);

    QAction* actSnap = viewMenu->addAction("Snap to &Grid");
    actSnap->setCheckable(true);
    actSnap->setChecked(true);
    connect(actSnap, &QAction::toggled, this, &MainWindow::onToggleSnap);

    viewMenu->addSeparator();
    viewMenu->addAction("Zoom &In", m_view, &CanvasView::zoomIn, QKeySequence::ZoomIn);
    viewMenu->addAction("Zoom &Out", m_view, &CanvasView::zoomOut, QKeySequence::ZoomOut);
    viewMenu->addAction("&Reset Zoom (100%)", m_view, &CanvasView::resetZoom);

    QMenu* projectMenu = menuBar()->addMenu("&Project");
    projectMenu->addAction("Project &Settings...", this, &MainWindow::onProjectSettingsDialog);
    projectMenu->addAction("Export &µGFX C Project...", this, &MainWindow::onExportUgfx);
    projectMenu->addAction("Export &Qt for MCUs (QUL) Project...", this, &MainWindow::onExportQtMcu);
    projectMenu->addAction("Export &LVGL (C/C++) Project...", this, &MainWindow::onExportLvgl);

    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About Embedded UI Designer", this, &MainWindow::onAbout);

    // Toolbar
    QToolBar* toolbar = addToolBar("Main Toolbar");
    toolbar->setMovable(false);
    toolbar->setStyleSheet(
        "QToolBar { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #343b48, stop:0.04 #3b4453, stop:0.5 #292f3a, stop:0.96 #20242d, stop:1 #15181e); "
        "  border-top: 1px solid #4d576a; "
        "  border-bottom: 2px solid #0d0f13; "
        "  padding: 5px 8px; "
        "  spacing: 6px; "
        "}"
    );

    toolbar->addAction("New", this, &MainWindow::onNewProject);
    toolbar->addAction("Open", this, &MainWindow::onOpenProject);
    toolbar->addAction("Save", this, &MainWindow::onSaveProject);
    toolbar->addSeparator();

    toolbar->addAction(undoAct);
    toolbar->addAction(redoAct);
    toolbar->addSeparator();

    toolbar->addAction("Export µGFX", this, &MainWindow::onExportUgfx);
    toolbar->addAction("Export QUL", this, &MainWindow::onExportQtMcu);
    toolbar->addAction("Export LVGL", this, &MainWindow::onExportLvgl);
    toolbar->addSeparator();

    QLabel* resLabel = new QLabel(" Target Display: ", this);
    resLabel->setStyleSheet("color: #a0b0c8; font-weight: bold; font-size: 11px;");
    toolbar->addWidget(resLabel);

    m_resolutionCombo = new QComboBox(this);
    m_resolutionCombo->addItem("320 × 240 (QVGA - STM32/ESP32)", QSize(320, 240));
    m_resolutionCombo->addItem("480 × 320 (HVGA - 3.5\" TFT)", QSize(480, 320));
    m_resolutionCombo->addItem("800 × 480 (WVGA - 7\" Industrial HMI)", QSize(800, 480));
    m_resolutionCombo->addItem("240 × 240 (Round/Square Display)", QSize(240, 240));
    m_resolutionCombo->addItem("128 × 64 (OLED Monolithic)", QSize(128, 64));
    m_resolutionCombo->setStyleSheet(
        "QComboBox { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #181b22, stop:0.12 #20242e, stop:0.88 #1c2029, stop:1 #14161d); "
        "  color: #f0f4fa; "
        "  border-top: 1px solid #0f1116; "
        "  border-left: 1px solid #14171e; "
        "  border-right: 1px solid #2e3544; "
        "  border-bottom: 1px solid #3d4658; "
        "  border-radius: 5px; "
        "  padding: 4px 10px; "
        "  min-width: 230px; "
        "  font-weight: bold; "
        "  font-size: 11px; "
        "} "
        "QComboBox::drop-down { "
        "  subcontrol-origin: padding; "
        "  subcontrol-position: top right; "
        "  width: 24px; "
        "  border-left: 1px solid #121419; "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3c4352, stop:0.06 #474f60, stop:0.5 #2f3542, stop:1 #1e222b); "
        "  border-top-right-radius: 4px; "
        "  border-bottom-right-radius: 4px; "
        "} "
        "QComboBox::down-arrow { "
        "  image: url(:/combo_arrow.png); "
        "  width: 9px; "
        "  height: 6px; "
        "} "
        "QComboBox QAbstractItemView { "
        "  background-color: #21252e; "
        "  color: #e0e5ee; "
        "  border: 1px solid #3c4558; "
        "  selection-background-color: #1a96ff; "
        "  selection-color: #ffffff; "
        "}"
    );
    toolbar->addWidget(m_resolutionCombo);
    connect(m_resolutionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onResolutionPresetChanged);

    toolbar->addSeparator();
    toolbar->addAction("Zoom -", m_view, &CanvasView::zoomOut);
    toolbar->addAction("Zoom +", m_view, &CanvasView::zoomIn);
    toolbar->addAction("100%", m_view, &CanvasView::resetZoom);
}

void MainWindow::setupDocks() {
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    // Left Dock: Components Palette
    QDockWidget* paletteDock = new QDockWidget("Components", this);
    paletteDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_palette = new ComponentPalette(paletteDock);
    paletteDock->setWidget(m_palette);
    addDockWidget(Qt::LeftDockWidgetArea, paletteDock);

    connect(m_palette, &ComponentPalette::componentDoubleClicked, this, [this](const QString& type) {
        QPointF center = m_scene->displayRect().center();
        UIComponent* comp = Project::createComponentInstance(type, type.toLower() + "_new");
        if (comp) {
            comp->setPos(m_scene->snapPoint(center - QPointF(comp->compWidth()/2.0, comp->compHeight()/2.0)));
            m_scene->clearSelection();
            if (m_undoStack) {
                m_undoStack->push(new AddComponentCommand(m_scene, comp));
            } else {
                m_scene->addUIComponent(comp);
                comp->setSelected(true);
            }
        }
    });

    // Right Top Dock: Properties Panel
    QDockWidget* propDock = new QDockWidget("Properties", this);
    propDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    m_propertiesPanel = new PropertiesPanel(propDock);
    m_propertiesPanel->setUndoStack(m_undoStack);
    propDock->setWidget(m_propertiesPanel);
    addDockWidget(Qt::RightDockWidgetArea, propDock);

    // Right Bottom Dock: Layer Panel
    QDockWidget* layerDock = new QDockWidget("Layers", this);
    layerDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    m_layerPanel = new LayerPanel(m_scene, layerDock);
    layerDock->setWidget(m_layerPanel);
    addDockWidget(Qt::RightDockWidgetArea, layerDock);
}

void MainWindow::applyTheme() {
    setStyleSheet(
        // Main Window & General
        "QMainWindow { background-color: #171920; color: #E0E5EE; }"
        
        // Menu Bar: Brushed Dark Aluminum with Bevel Highlight
        "QMenuBar { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #323846, stop:0.04 #38404f, stop:0.5 #282d38, stop:0.96 #20242c, stop:1 #15171d); "
        "  color: #cad4e2; "
        "  border-bottom: 1px solid #0d0f13; "
        "  padding: 2px 6px; "
        "  font-size: 12px; "
        "  font-weight: 500; "
        "} "
        "QMenuBar::item { "
        "  background: transparent; "
        "  padding: 4px 10px; "
        "  border-radius: 4px; "
        "} "
        "QMenuBar::item:selected { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #444c5e, stop:1 #282e39); "
        "  color: #ffffff; "
        "  border-top: 1px solid #5d697f; "
        "  border-bottom: 1px solid #101216; "
        "} "

        // Popup Menus
        "QMenu { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2d323e, stop:1 #1d2129); "
        "  color: #e1e7f0; "
        "  border: 1px solid #4a5468; "
        "  border-top: 1px solid #637189; "
        "  border-bottom: 2px solid #0e1014; "
        "  border-radius: 6px; "
        "  padding: 4px; "
        "} "
        "QMenu::item { "
        "  padding: 6px 24px 6px 20px; "
        "  border-radius: 4px; "
        "  font-size: 12px; "
        "} "
        "QMenu::item:selected { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1a96ff, stop:0.08 #0a84ed, stop:0.92 #0060b8, stop:1 #004485); "
        "  color: #ffffff; "
        "  font-weight: 600; "
        "  border-top: 1px solid #7fc4ff; "
        "  border-bottom: 1px solid #002c57; "
        "} "
        "QMenu::separator { "
        "  height: 2px; "
        "  margin: 4px 6px; "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #101217, stop:0.5 #101217, stop:0.51 #3d4556, stop:1 #3d4556); "
        "} "

        // Docks: Brushed Metal Title Bars and Inset Beveled Chassis
        "QDockWidget { "
        "  color: #e0e6f2; "
        "  font-weight: bold; "
        "  font-size: 11px; "
        "} "
        "QDockWidget::title { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #353c4a, stop:0.06 #3d4656, stop:0.5 #292f3a, stop:0.95 #1f232c, stop:1 #16181f); "
        "  border-top: 1px solid #4d576a; "
        "  border-bottom: 1px solid #0e1014; "
        "  padding: 8px 12px; "
        "  color: #dce4f0; "
        "} "

        // Status Bar: Chassis Base Plate
        "QStatusBar { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2d3340, stop:0.06 #353c4a, stop:0.5 #242933, stop:1 #181b22); "
        "  border-top: 1px solid #444e60; "
        "  color: #a4b3c7; "
        "  font-size: 12px; "
        "} "
        "QStatusBar::item { border: none; } "

        // 3D Skeuomorphic Buttons: Gradient fills, glossy highlights, 3D borders, pressed recessed state
        "QToolButton, QPushButton { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3c4352, stop:0.04 #454d5d, stop:0.48 #2f3542, stop:0.52 #252a35, stop:0.96 #1f232d, stop:1 #181b23); "
        "  color: #e2e8f2; "
        "  border-top: 1px solid #586378; "
        "  border-left: 1px solid #3b4252; "
        "  border-right: 1px solid #232731; "
        "  border-bottom: 2px solid #111318; "
        "  border-radius: 5px; "
        "  padding: 5px 12px; "
        "  font-weight: bold; "
        "  font-size: 11px; "
        "} "
        "QToolButton:hover, QPushButton:hover { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4c5567, stop:0.04 #576175, stop:0.48 #3b4353, stop:0.52 #303745, stop:0.96 #282e3b, stop:1 #202530); "
        "  color: #ffffff; "
        "  border-top: 1px solid #73829c; "
        "  border-left: 1px solid #4a5468; "
        "  border-right: 1px solid #2c323f; "
        "  border-bottom: 2px solid #14171d; "
        "} "
        "QToolButton:pressed, QPushButton:pressed { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #15181f, stop:0.08 #1c2029, stop:0.5 #232833, stop:1 #2a2f3c); "
        "  color: #b0bac9; "
        "  border-top: 2px solid #0d0f13; "
        "  border-left: 1px solid #15181f; "
        "  border-right: 1px solid #363d4c; "
        "  border-bottom: 1px solid #485264; "
        "  padding-top: 6px; "
        "  padding-bottom: 4px; "
        "} "
        "QToolButton:disabled, QPushButton:disabled { "
        "  background: #252830; "
        "  color: #636b78; "
        "  border: 1px solid #2d313a; "
        "} "

        // Toolbar Separators: 3D Groove
        "QToolBar::separator { "
        "  width: 2px; "
        "  margin: 4px 6px; "
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #101216, stop:0.5 #101216, stop:0.51 #454f63, stop:1 #454f63); "
        "} "

        // Splitter Handles: 3D Divider
        "QSplitter::handle:horizontal { "
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #12141a, stop:0.4 #1a1d25, stop:0.6 #2b313e, stop:1 #12141a); "
        "  width: 5px; "
        "} "
        "QSplitter::handle:vertical { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #12141a, stop:0.4 #1a1d25, stop:0.6 #2b313e, stop:1 #12141a); "
        "  height: 5px; "
        "} "

        // Scrollbars: Physical Recessed Rail with 3D Extruded Thumb
        "QScrollBar:vertical { "
        "  background-color: #14161d; "
        "  border-left: 1px solid #0e1014; "
        "  border-right: 1px solid #282d38; "
        "  width: 14px; "
        "  margin: 0px; "
        "} "
        "QScrollBar::handle:vertical { "
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3c4353, stop:0.15 #474f62, stop:0.85 #2e3542, stop:1 #222630); "
        "  border-top: 1px solid #586378; "
        "  border-bottom: 1px solid #101217; "
        "  border-radius: 4px; "
        "  min-height: 24px; "
        "  margin: 2px 2px; "
        "} "
        "QScrollBar::handle:vertical:hover { "
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4c5567, stop:0.15 #576175, stop:0.85 #3a4252, stop:1 #2a2f3a); "
        "} "
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { "
        "  height: 0px; "
        "} "
        "QScrollBar:horizontal { "
        "  background-color: #14161d; "
        "  border-top: 1px solid #0e1014; "
        "  border-bottom: 1px solid #282d38; "
        "  height: 14px; "
        "  margin: 0px; "
        "} "
        "QScrollBar::handle:horizontal { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3c4353, stop:0.15 #474f62, stop:0.85 #2e3542, stop:1 #222630); "
        "  border-left: 1px solid #586378; "
        "  border-right: 1px solid #101217; "
        "  border-radius: 4px; "
        "  min-width: 24px; "
        "  margin: 2px 2px; "
        "} "
        "QScrollBar::handle:horizontal:hover { "
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4c5567, stop:0.15 #576175, stop:0.85 #3a4252, stop:1 #2a2f3a); "
        "} "
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { "
        "  width: 0px; "
        "}"
    );
}

void MainWindow::updateWindowTitle() {
    QString title = "Embedded UI Designer - " + m_project->projectName();
    if (m_project->isDirty()) {
        title += " *";
    }
    setWindowTitle(title);
}

void MainWindow::onNewProject() {
    if (m_project->isDirty()) {
        auto res = QMessageBox::question(this, "Unsaved Changes", "Save changes before creating a new project?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (res == QMessageBox::Yes) {
            onSaveProject();
        } else if (res == QMessageBox::Cancel) {
            return;
        }
    }
    m_project->newProject("NewEmbeddedApp", 320, 240);
    if (m_undoStack) m_undoStack->clear();
    m_resolutionCombo->setCurrentIndex(0);
    statusBar()->showMessage("New project initialized", 3000);
}

void MainWindow::onOpenProject() {
    QString defaultDir = Project::appDataDirectory();
    QString file = QFileDialog::getOpenFileName(this, "Open Embedded UI Project", defaultDir, "Embedded UI Project (*.euiproj);;All Files (*)");
    if (!file.isEmpty()) {
        if (m_project->loadFromFile(file)) {
            if (m_undoStack) m_undoStack->clear();
            // Update resolution combo to match
            DisplayConfig cfg = m_project->displayConfig();
            for (int i = 0; i < m_resolutionCombo->count(); ++i) {
                QSize sz = m_resolutionCombo->itemData(i).toSize();
                if (sz.width() == cfg.width && sz.height() == cfg.height) {
                    m_resolutionCombo->blockSignals(true);
                    m_resolutionCombo->setCurrentIndex(i);
                    m_resolutionCombo->blockSignals(false);
                    break;
                }
            }
            statusBar()->showMessage("Project loaded: " + file, 4000);
        } else {
            QMessageBox::warning(this, "Error", "Failed to load project file.");
        }
    }
}

void MainWindow::onSaveProject() {
    if (m_project->projectFilePath().isEmpty()) {
        onSaveProjectAs();
    } else {
        if (m_project->saveToFile(m_project->projectFilePath())) {
            if (m_undoStack) m_undoStack->setClean();
            statusBar()->showMessage("Project saved successfully.", 3000);
        } else {
            QMessageBox::warning(this, "Error", "Failed to save project.");
        }
    }
}

void MainWindow::onSaveProjectAs() {
    QString defaultDir = Project::appDataDirectory();
    QString defaultPath = QDir(defaultDir).filePath(m_project->projectName() + ".euiproj");
    QString file = QFileDialog::getSaveFileName(this, "Save Embedded UI Project As", defaultPath, "Embedded UI Project (*.euiproj);;All Files (*)");
    if (!file.isEmpty()) {
        if (m_project->saveToFile(file)) {
            if (m_undoStack) m_undoStack->setClean();
            statusBar()->showMessage("Project saved as: " + file, 4000);
        } else {
            QMessageBox::warning(this, "Error", "Failed to save project.");
        }
    }
}

void MainWindow::onExportUgfx() {
    QString exportDir = QFileDialog::getExistingDirectory(this, "Choose µGFX Export Destination Folder");
    if (exportDir.isEmpty()) return;

    UgfxGenerator generator(m_project, m_scene);
    if (generator.generate(exportDir)) {
        QString msg = QString("µGFX C project successfully exported to:\n\n%1\n\nWould you like to open the export directory?").arg(exportDir);
        auto res = QMessageBox::information(this, "Export Succeeded", msg, QMessageBox::Open | QMessageBox::Ok);
        if (res == QMessageBox::Open) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(exportDir));
        }
        statusBar()->showMessage("µGFX project successfully exported to " + exportDir, 5000);
    } else {
        QMessageBox::critical(this, "Export Failed", "Error exporting µGFX project: " + generator.lastError());
    }
}

void MainWindow::onExportQtMcu() {
    QString exportDir = QFileDialog::getExistingDirectory(this, "Choose Qt for MCUs (QUL) Export Destination Folder");
    if (exportDir.isEmpty()) return;

    QtMcuGenerator generator(m_project, m_scene);
    if (generator.generate(exportDir)) {
        QString msg = QString("Qt Quick Ultralite (QUL) project successfully exported to:\n\n%1\n\nWould you like to open the export directory?").arg(exportDir);
        auto res = QMessageBox::information(this, "Export Succeeded", msg, QMessageBox::Open | QMessageBox::Ok);
        if (res == QMessageBox::Open) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(exportDir));
        }
        statusBar()->showMessage("Qt for MCUs project successfully exported to " + exportDir, 5000);
    } else {
        QMessageBox::critical(this, "Export Failed", "Error exporting QUL project: " + generator.lastError());
    }
}

void MainWindow::onExportLvgl() {
    QString exportDir = QFileDialog::getExistingDirectory(this, "Choose LVGL (C/C++) Export Destination Folder");
    if (exportDir.isEmpty()) return;

    LvglGenerator generator(m_project, m_scene);
    if (generator.generate(exportDir)) {
        QString msg = QString("LVGL C/C++ project successfully exported to:\n\n%1\n\nWould you like to open the export directory?").arg(exportDir);
        auto res = QMessageBox::information(this, "Export Succeeded", msg, QMessageBox::Open | QMessageBox::Ok);
        if (res == QMessageBox::Open) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(exportDir));
        }
        statusBar()->showMessage("LVGL project successfully exported to " + exportDir, 5000);
    } else {
        QMessageBox::critical(this, "Export Failed", "Error exporting LVGL project: " + generator.lastError());
    }
}

void MainWindow::onDeleteSelected() {
    QList<UIComponent*> selComps;
    for (QGraphicsItem* item : m_scene->selectedItems()) {
        if (auto comp = dynamic_cast<UIComponent*>(item)) {
            selComps.append(comp);
        }
    }
    if (!selComps.isEmpty()) {
        if (m_undoStack) {
            m_undoStack->push(new DeleteComponentCommand(m_scene, selComps));
        } else {
            for (UIComponent* comp : selComps) {
                m_scene->removeUIComponent(comp);
                delete comp;
            }
        }
    }
}

void MainWindow::onDuplicateSelected() {
    QList<UIComponent*> selComps;
    for (QGraphicsItem* item : m_scene->selectedItems()) {
        if (auto comp = dynamic_cast<UIComponent*>(item)) {
            selComps.append(comp);
        }
    }
    if (selComps.isEmpty()) return;

    m_scene->clearSelection();
    for (UIComponent* comp : selComps) {
        UIComponent* dup = Project::createComponentInstance(comp->componentType(), comp->componentId() + "_copy");
        if (dup) {
            dup->fromJson(comp->toJson());
            dup->setComponentId(comp->componentId() + "_copy");
            dup->setCompPos(comp->pos().x() + 15, comp->pos().y() + 15);
            if (m_undoStack) {
                m_undoStack->push(new AddComponentCommand(m_scene, dup));
            } else {
                m_scene->addUIComponent(dup);
                dup->setSelected(true);
            }
        }
    }
}

void MainWindow::onToggleGrid(bool checked) {
    m_scene->setGridVisible(checked);
}

void MainWindow::onToggleSnap(bool checked) {
    m_scene->setSnapToGrid(checked);
}

void MainWindow::onZoomChanged(qreal factor) {
    int pct = static_cast<int>(factor * 100);
    m_zoomLabel->setText(QString("Zoom: %1%").arg(pct));
}

void MainWindow::onResolutionPresetChanged(int index) {
    QSize sz = m_resolutionCombo->itemData(index).toSize();
    DisplayConfig cfg = m_project->displayConfig();
    cfg.width = sz.width();
    cfg.height = sz.height();
    m_project->setDisplayConfig(cfg);
    statusBar()->showMessage(QString("Display resolution updated to %1 × %2").arg(sz.width()).arg(sz.height()), 3000);
}

void MainWindow::onProjectSettingsDialog() {
    QMessageBox::information(this, "Project Settings", 
        QString("Project: %1\nTarget: %2\nResolution: %3 × %4\nColor Depth: %5-bit")
        .arg(m_project->projectName(), m_project->targetFramework())
        .arg(m_project->displayConfig().width)
        .arg(m_project->displayConfig().height)
        .arg(m_project->displayConfig().colorDepth)
    );
}

void MainWindow::onOpenSampleProject() {
    if (m_project->isDirty()) {
        auto res = QMessageBox::question(this, "Unsaved Changes", "Save changes before opening sample project?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (res == QMessageBox::Yes) {
            onSaveProject();
        } else if (res == QMessageBox::Cancel) {
            return;
        }
    }
    if (m_project->loadSampleProject()) {
        if (m_undoStack) m_undoStack->clear();
        statusBar()->showMessage("Loaded embedded sample thermostat project", 4000);
    } else {
        QMessageBox::warning(this, "Error", "Failed to load embedded sample project.");
    }
}

void MainWindow::onAbout() {
    QString aboutText = QString(
        "<h2>Embedded UI Designer v1.0.0</h2>"
        "<p><b>Visual drag-and-drop UI designer and code generator for MCUs</b></p>"
        "<p>Supports production code generation for <b>µGFX (C)</b> and <b>Qt Quick Ultralite (Qt for MCUs)</b>.</p>"
        "<hr>"
        "<p><b>Runtime:</b> Qt %1 (%2)</p>"
        "<p><b>Build Platform:</b> %3</p>"
        "<p><b>License:</b> MIT License (Application Code)</p>"
        "<hr>"
        "<p><b>Qt Open Source License Notice:</b><br>"
        "This application is built using Qt %1 under the GNU Lesser General Public License (LGPLv3). "
        "Qt is a registered trademark of The Qt Company Ltd. and its subsidiaries. "
        "This application dynamically links against unmodified Qt libraries. "
        "For source code of Qt and license terms, visit <a href=\"https://www.qt.io/licensing/\">qt.io/licensing</a>.</p>"
    ).arg(qVersion(), QSysInfo::buildAbi(), QSysInfo::prettyProductName());

    QMessageBox::about(this, "About Embedded UI Designer", aboutText);
}
