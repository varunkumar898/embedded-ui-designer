#include "MainWindow.h"
#include "QtMcuGenerator.h"
#include "UgfxGenerator.h"
#include "LvglGenerator.h"
#include "FlashDialog.h"
#include "SerialMonitorDialog.h"
#include "PinBindingDialog.h"
#include "NewProjectDialog.h"
#include "DeviceManager.h"
#include "ButtonComponent.h"
#include "LabelComponent.h"
#include "RectangleComponent.h"
#include "ProgressBarComponent.h"
#include "ImageComponent.h"
#include "AddComponentCommand.h"
#include "DeleteComponentCommand.h"
#include "AlignDistributeCommand.h"
#include "BooleanPathCommand.h"
#include "PathComponent.h"
#include "CustomComponentInstance.h"
#include "ComponentDefinition.h"
#include "project/QmlImporter.h"
#include <QUndoStack>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>
#include <QTimer>
#include <QDialog>
#include <QFormLayout>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_scene = new CanvasScene(this);
    m_undoStack = new QUndoStack(this);
    m_scene->setUndoStack(m_undoStack);
    m_view = new CanvasView(m_scene, this);
    m_view->setUndoStack(m_undoStack);
    m_project = new Project(m_scene, this);
    m_deviceManager = new DeviceManager(this);

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
    connect(m_scene, &CanvasScene::selectionListChanged, m_propertiesPanel, &PropertiesPanel::setSelectedComponents);
    connect(m_scene, &CanvasScene::componentSelected, m_prototypePanel, &PrototypePanel::setTargetComponent);
    connect(m_scene, &CanvasScene::componentChanged, m_propertiesPanel, &PropertiesPanel::refreshValues);

    // Connect PropertiesPanel align & distribute buttons
    connect(m_propertiesPanel, &PropertiesPanel::alignLeftRequested, this, &MainWindow::onAlignLeft);
    connect(m_propertiesPanel, &PropertiesPanel::alignHCenterRequested, this, &MainWindow::onAlignHCenter);
    connect(m_propertiesPanel, &PropertiesPanel::alignRightRequested, this, &MainWindow::onAlignRight);
    connect(m_propertiesPanel, &PropertiesPanel::alignTopRequested, this, &MainWindow::onAlignTop);
    connect(m_propertiesPanel, &PropertiesPanel::alignVCenterRequested, this, &MainWindow::onAlignVCenter);
    connect(m_propertiesPanel, &PropertiesPanel::alignBottomRequested, this, &MainWindow::onAlignBottom);
    connect(m_propertiesPanel, &PropertiesPanel::distributeHRequested, this, &MainWindow::onDistributeH);
    connect(m_propertiesPanel, &PropertiesPanel::distributeVRequested, this, &MainWindow::onDistributeV);
    connect(m_propertiesPanel, &PropertiesPanel::booleanUnionRequested,     this, &MainWindow::onBooleanUnion);
    connect(m_propertiesPanel, &PropertiesPanel::booleanSubtractRequested,  this, &MainWindow::onBooleanSubtract);
    connect(m_propertiesPanel, &PropertiesPanel::booleanIntersectRequested, this, &MainWindow::onBooleanIntersect);
    connect(m_propertiesPanel, &PropertiesPanel::booleanXorRequested,       this, &MainWindow::onBooleanXor);

    connect(m_view, &CanvasView::zoomChanged, this, &MainWindow::onZoomChanged);
    connect(m_view, &CanvasView::statusMessageRequested, this, [this](const QString& msg) {
        statusBar()->showMessage(msg, 3000);
    });
    connect(m_project, &Project::projectModified, this, &MainWindow::updateWindowTitle);
    connect(m_project, &Project::projectLoaded, this, [this]() {
        updateWindowTitle();
        m_layerPanel->refreshLayers();
        m_propertiesPanel->setSelectedComponents({});
    });

    resize(1380, 880);
}
void MainWindow::setupUi() {
    QWidget* statusWidget = new QWidget(this);
    QHBoxLayout* statusLayout = new QHBoxLayout(statusWidget);
    statusLayout->setContentsMargins(4, 0, 0, 0);
    statusLayout->setSpacing(8);

    QLabel* greenDot = new QLabel(statusWidget);
    greenDot->setFixedSize(8, 8);
    greenDot->setStyleSheet("background-color: #10b981; border-radius: 4px;");

    m_statusLabel = new QLabel("Ready", statusWidget);
    m_statusLabel->setStyleSheet("color: #e2e8f0; font-size: 12px; font-weight: 600;");

    statusLayout->addWidget(greenDot);
    statusLayout->addWidget(m_statusLabel);

    statusBar()->setSizeGripEnabled(false);
    m_zoomLabel = new QLabel("Zoom: 100%", this);
    m_zoomLabel->setStyleSheet("padding-right: 20px; color: #e2e8f0; font-size: 12px; font-weight: 600;");

    statusBar()->addWidget(statusWidget, 1);
    statusBar()->addPermanentWidget(m_zoomLabel);
}

void MainWindow::setupMenusAndToolbars() {
    // Top Menus
    QMenu* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("New Project", this, &MainWindow::onNewProject, QKeySequence::New);
    fileMenu->addAction("Open Project...", this, &MainWindow::onOpenProject, QKeySequence::Open);
    fileMenu->addAction("Open Sample Project (Thermostat)", this, &MainWindow::onOpenSampleProject);
    fileMenu->addAction("Import...", this, &MainWindow::onImportProject, QKeySequence(Qt::CTRL | Qt::Key_I));
    fileMenu->addAction("Import Board Config...", this, &MainWindow::onImportBoardConfig);
    fileMenu->addAction("Import QML Design...", this, &MainWindow::onImportQmlDesign);
    fileMenu->addSeparator();
    fileMenu->addAction("Save", this, &MainWindow::onSaveProject, QKeySequence::Save);
    fileMenu->addAction("Save As...", this, &MainWindow::onSaveProjectAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("Export...", this, &MainWindow::onExport, QKeySequence(Qt::CTRL | Qt::Key_E));
    fileMenu->addSeparator();
    QAction* exitAction = fileMenu->addAction("Exit", QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu* editMenu = menuBar()->addMenu("Edit");
    QAction* undoAct = m_undoStack->createUndoAction(this, "Undo");
    undoAct->setShortcut(QKeySequence::Undo);
    QAction* redoAct = m_undoStack->createRedoAction(this, "Redo");
    redoAct->setShortcut(QKeySequence::Redo);

    editMenu->addAction(undoAct);
    editMenu->addAction(redoAct);
    editMenu->addSeparator();
    editMenu->addAction("Duplicate", this, &MainWindow::onDuplicateSelected, QKeySequence(Qt::CTRL | Qt::Key_D));
    editMenu->addAction("Delete", this, &MainWindow::onDeleteSelected, QKeySequence::Delete);

    QMenu* viewMenu = menuBar()->addMenu("View");
    QAction* actGrid = viewMenu->addAction("Show Grid");
    actGrid->setCheckable(true);
    actGrid->setChecked(true);
    connect(actGrid, &QAction::toggled, this, &MainWindow::onToggleGrid);

    QAction* actSnap = viewMenu->addAction("Snap to Grid");
    actSnap->setCheckable(true);
    actSnap->setChecked(true);
    connect(actSnap, &QAction::toggled, this, &MainWindow::onToggleSnap);

    viewMenu->addSeparator();
    viewMenu->addAction("Zoom In", m_view, &CanvasView::zoomIn, QKeySequence::ZoomIn);
    viewMenu->addAction("Zoom Out", m_view, &CanvasView::zoomOut, QKeySequence::ZoomOut);
    viewMenu->addAction("Reset Zoom (100%)", m_view, &CanvasView::resetZoom);

    QMenu* projectMenu = menuBar()->addMenu("Project");
    projectMenu->addAction("Project Settings...", this, &MainWindow::onProjectSettingsDialog);
    projectMenu->addAction("Export...", this, &MainWindow::onExport);

    QMenu* deviceMenu = menuBar()->addMenu("Device");
    deviceMenu->addAction("Pin Configuration & Binding...", this, &MainWindow::onPinConfiguration);
    deviceMenu->addAction("Serial Monitor...", this, &MainWindow::onSerialMonitor);
    deviceMenu->addAction("Flash Firmware...", this, &MainWindow::onFlashFirmware);

    QMenu* helpMenu = menuBar()->addMenu("Help");
    helpMenu->addAction("About Embedded UI Designer", this, &MainWindow::onAbout);

    // Toolbar
    QToolBar* toolbar = addToolBar("Main Toolbar");
    toolbar->setMovable(false);
    toolbar->setStyleSheet(
        "QToolBar { "
        "  background: #121316; "
        "  border-bottom: 1px solid #1c1f26; "
        "  padding: 6px 12px; "
        "  spacing: 8px; "
        "}"
    );

    toolbar->addAction("Undo", this, [this]() {
        if (m_undoStack && m_undoStack->canUndo()) m_undoStack->undo();
    });
    toolbar->addAction("Redo", this, [this]() {
        if (m_undoStack && m_undoStack->canRedo()) m_undoStack->redo();
    });
    toolbar->addSeparator();

    toolbar->addAction("Export...", this, &MainWindow::onExport);
    toolbar->addSeparator();

    QLabel* resLabel = new QLabel("Target Display:", this);
    resLabel->setStyleSheet("color: #6a7382; font-size: 11.5px; font-weight: 500; margin-left: 2px; margin-right: 2px;");
    toolbar->addWidget(resLabel);

    m_resolutionCombo = new QComboBox(this);
    m_resolutionCombo->addItem("320 × 240 (QVGA - STM32/ESP32)", QSize(320, 240));
    m_resolutionCombo->addItem("480 × 320 (HVGA - 3.5\" TFT)", QSize(480, 320));
    m_resolutionCombo->addItem("800 × 480 (WVGA - 7\" Industrial HMI)", QSize(800, 480));
    m_resolutionCombo->addItem("240 × 240 (Round/Square Display)", QSize(240, 240));
    m_resolutionCombo->addItem("128 × 64 (OLED Monolithic)", QSize(128, 64));
    m_resolutionCombo->setStyleSheet(
        "QComboBox { "
        "  background: #1c1f26; "
        "  color: #ffffff; "
        "  border: 1px solid #2b2f38; "
        "  border-radius: 6px; "
        "  padding: 5px 12px; "
        "  min-width: 250px; "
        "  font-weight: bold; "
        "  font-size: 11px; "
        "} "
        "QComboBox::drop-down { "
        "  subcontrol-origin: padding; "
        "  subcontrol-position: top right; "
        "  width: 26px; "
        "  border: none; "
        "} "
        "QComboBox::down-arrow { "
        "  image: url(:/combo_arrow.png); "
        "  width: 12px; "
        "  height: 7px; "
        "  margin-right: 8px; "
        "} "
        "QComboBox QAbstractItemView { "
        "  background-color: #1a1d24; "
        "  color: #e0e5ee; "
        "  border: 1px solid #2c313e; "
        "  selection-background-color: #1a73e8; "
        "  selection-color: #ffffff; "
        "}"
    );
    toolbar->addWidget(m_resolutionCombo);
    connect(m_resolutionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onResolutionPresetChanged);

    toolbar->addSeparator();
    QAction* zoomOutAction = toolbar->addAction("−", m_view, &CanvasView::zoomOut);
    zoomOutAction->setToolTip("Zoom out");
    QAction* zoomInAction = toolbar->addAction("+", m_view, &CanvasView::zoomIn);
    zoomInAction->setToolTip("Zoom in");
    toolbar->addAction("100%", m_view, &CanvasView::resetZoom);
}

static QWidget* createDockTitleBar(const QString& titleText, QDockWidget* dock) {
    QWidget* bar = new QWidget(dock);
    bar->setObjectName("customDockTitle");
    bar->setStyleSheet("background: #1e2025; border: none; padding: 0px;");
    QHBoxLayout* l = new QHBoxLayout(bar);
    l->setContentsMargins(14, 10, 14, 8);
    l->setSpacing(8);

    QLabel* label = new QLabel(titleText, bar);
    label->setStyleSheet("color: #e2e8f0; font-size: 13px; font-weight: bold; border: none; background: transparent;");
    l->addWidget(label);
    l->addStretch(1);

    QPushButton* minBtn = new QPushButton("—", bar);
    minBtn->setFixedSize(18, 18);
    minBtn->setStyleSheet("QPushButton { background: transparent; border: none; color: #768090; font-size: 13px; font-weight: bold; padding: 0px; } QPushButton:hover { color: #ffffff; }");
    l->addWidget(minBtn);

    QPushButton* closeBtn = new QPushButton("✕", bar);
    closeBtn->setFixedSize(18, 18);
    closeBtn->setStyleSheet("QPushButton { background: transparent; border: none; color: #768090; font-size: 11px; font-weight: bold; padding: 0px; } QPushButton:hover { color: #ffffff; }");
    QObject::connect(closeBtn, &QPushButton::clicked, dock, &QDockWidget::close);
    l->addWidget(closeBtn);

    return bar;
}

void MainWindow::setupDocks() {
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    // Left Dock: Components Palette
    QDockWidget* paletteDock = new QDockWidget("Components", this);
    paletteDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    paletteDock->setTitleBarWidget(createDockTitleBar("Components", paletteDock));
    m_palette = new ComponentPalette(paletteDock);
    paletteDock->setWidget(m_palette);
    addDockWidget(Qt::LeftDockWidgetArea, paletteDock);

    connect(m_palette, &ComponentPalette::componentDoubleClicked, this, [this](const QString& type) {
        // Custom component instances are prefixed "CustomInstance::<defId>"
        if (type.startsWith("CustomInstance::")) {
            const QString defId = type.mid(16);
            auto* ci = new CustomComponentInstance(defId.toLower() + "_inst", defId);
            const auto variants = ComponentDefinition::defaultVariants();
            if (!variants.isEmpty()) ci->applyVariant(variants.first());
            QPointF center = m_scene->displayRect().center();
            ci->setPos(m_scene->snapPoint(center - QPointF(ci->compWidth()/2.0, ci->compHeight()/2.0)));
            m_scene->clearSelection();
            if (m_undoStack) m_undoStack->push(new AddComponentCommand(m_scene, ci));
            return;
        }
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
    connect(m_palette, &ComponentPalette::shapeToolSelected, m_view, &CanvasView::setActiveDrawingTool);
    connect(m_palette, &ComponentPalette::saveAsComponentRequested, this, [this]() {
        const auto selected = m_scene->selectedItems();
        if (selected.isEmpty()) {
            QMessageBox::information(this, "Save as Component",
                "Select at least one component on the canvas first.");
            return;
        }
        bool ok = false;
        const QString name = QInputDialog::getText(this, "Save as Component",
            "Component name:", QLineEdit::Normal, "MyComponent", &ok);
        if (!ok || name.trimmed().isEmpty()) return;
        const QString defId = name.trimmed().replace(' ', '_');
        ComponentDefinition def(defId, name.trimmed());
        def.setVariants(ComponentDefinition::defaultVariants());
        if (auto* comp = dynamic_cast<UIComponent*>(selected.first()))
            def.setBaseShapeJson(comp->toJson());
        m_project->addComponentDefinition(def);
        QStringList names;
        for (const ComponentDefinition& d : m_project->componentLibrary())
            names << d.displayName();
        m_palette->refreshCustomComponents(names);
        m_statusLabel->setText(QString("Saved \"%1\" to My Components").arg(name.trimmed()));
    });

    // Right Top Dock: Properties Panel
    QDockWidget* propDock = new QDockWidget("Properties", this);
    propDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    propDock->setTitleBarWidget(createDockTitleBar("Properties", propDock));
    m_propertiesPanel = new PropertiesPanel(propDock);
    m_propertiesPanel->setUndoStack(m_undoStack);
    m_propertiesPanel->setProject(m_project);
    propDock->setWidget(m_propertiesPanel);
    addDockWidget(Qt::RightDockWidgetArea, propDock);

    QDockWidget* prototypeDock = new QDockWidget("Prototype", this);
    prototypeDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    prototypeDock->setTitleBarWidget(createDockTitleBar("Prototype", prototypeDock));
    m_prototypePanel = new PrototypePanel(m_scene, prototypeDock);
    prototypeDock->setWidget(m_prototypePanel);
    addDockWidget(Qt::RightDockWidgetArea, prototypeDock);
    tabifyDockWidget(propDock, prototypeDock);
    propDock->raise();

    // Right Bottom Dock: Layer Panel
    QDockWidget* layerDock = new QDockWidget("Layers", this);
    layerDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    layerDock->setTitleBarWidget(createDockTitleBar("Layers", layerDock));
    m_layerPanel = new LayerPanel(m_scene, layerDock);
    layerDock->setWidget(m_layerPanel);
    addDockWidget(Qt::RightDockWidgetArea, layerDock);

    // Right Dock: Named Color Styles (tabified with Layers)
    QDockWidget* stylesDock = new QDockWidget("Color Styles", this);
    stylesDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    stylesDock->setTitleBarWidget(createDockTitleBar("Color Styles", stylesDock));
    m_stylesPanel = new StylesPanel(m_project, stylesDock);
    stylesDock->setWidget(m_stylesPanel);
    addDockWidget(Qt::RightDockWidgetArea, stylesDock);
    tabifyDockWidget(layerDock, stylesDock);

    resizeDocks({paletteDock}, {210}, Qt::Horizontal);
    resizeDocks({propDock, prototypeDock, layerDock, stylesDock}, {260, 260, 260, 260}, Qt::Horizontal);
    resizeDocks({propDock, layerDock}, {550, 330}, Qt::Vertical);
}

void MainWindow::applyTheme() {
    setStyleSheet(
        // Main Window & General
        "QMainWindow { background-color: #121418; color: #E0E5EE; }"
        
        // Menu Bar: Elegant Matte Dark
        "QMenuBar { "
        "  background: #1e2025; "
        "  color: #8e96a4; "
        "  border-bottom: 1px solid #242730; "
        "  padding: 5px 8px; "
        "  font-size: 12px; "
        "  font-weight: 500; "
        "} "
        "QMenuBar::item { "
        "  background: transparent; "
        "  padding: 4px 12px; "
        "  margin-right: 4px; "
        "  border-radius: 4px; "
        "} "
        "QMenuBar::item:selected { "
        "  background: #282c35; "
        "  color: #ffffff; "
        "} "

        // Popup Menus
        "QMenu { "
        "  background: #1a1d25; "
        "  color: #e1e7f0; "
        "  border: 1px solid #2a2f3c; "
        "  border-radius: 6px; "
        "  padding: 4px; "
        "} "
        "QMenu::item { "
        "  padding: 6px 24px 6px 20px; "
        "  border-radius: 4px; "
        "  font-size: 12px; "
        "} "
        "QMenu::item:selected { "
        "  background: #1a73e8; "
        "  color: #ffffff; "
        "  font-weight: 600; "
        "} "
        "QMenu::separator { "
        "  height: 1px; "
        "  margin: 4px 6px; "
        "  background: #282c38; "
        "} "

        // Docks: Dark Graphite Panels with Clean Title Bar
        "QDockWidget { "
        "  color: #9da6b5; "
        "  font-weight: 600; "
        "  font-size: 12px; "
        "  background-color: #1e2025; "
        "  border: none; "
        "} "
        "QDockWidget::title { "
        "  background: #1e2025; "
        "  border: none; "
        "  padding: 0px; "
        "} "

        // Status Bar: Clean Matte Dark Footer
        "QStatusBar { "
        "  background: #181a1f; "
        "  border-top: 1px solid #242831; "
        "  color: #e2e8f0; "
        "  font-size: 12px; "
        "  padding: 2px 8px; "
        "} "
        "QStatusBar::item { border: none; } "

        // Buttons: Refined Dark Inset Graphite Cards with Subtle 1px Border
        "QToolButton, QPushButton { "
        "  background: #1f2125; "
        "  color: #ffffff; "
        "  border: 1px solid #2b2f38; "
        "  border-radius: 6px; "
        "  padding: 6px 14px; "
        "  font-weight: bold; "
        "  font-size: 11px; "
        "} "
        "QToolButton:hover, QPushButton:hover { "
        "  background: #272a31; "
        "  color: #ffffff; "
        "  border-color: #3b4250; "
        "} "
        "QToolButton:pressed, QPushButton:pressed { "
        "  background: #17191d; "
        "  color: #9aa5b8; "
        "  border-color: #1e2025; "
        "} "
        "QToolButton:disabled, QPushButton:disabled { "
        "  background: #1f2125; "
        "  color: #ffffff; "
        "} "

        // Toolbar Separators: Clean 1px Vertical Line
        "QToolBar::separator { "
        "  width: 1px; "
        "  margin: 5px 8px; "
        "  background: #232631; "
        "} "

        // Splitters
        "QSplitter::handle:horizontal { "
        "  background: #121419; "
        "  width: 3px; "
        "} "
        "QSplitter::handle:vertical { "
        "  background: #121419; "
        "  height: 3px; "
        "} "

        // Scrollbars
        "QScrollBar:vertical { "
        "  background-color: transparent; "
        "  width: 8px; "
        "  margin: 0px; "
        "} "
        "QScrollBar::handle:vertical { "
        "  background: #2c313e; "
        "  border-radius: 4px; "
        "  min-height: 20px; "
        "} "
        "QScrollBar::handle:vertical:hover { "
        "  background: #3b4254; "
        "} "
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { "
        "  height: 0px; "
        "} "
        "QScrollBar:horizontal { "
        "  background-color: transparent; "
        "  height: 8px; "
        "  margin: 0px; "
        "} "
        "QScrollBar::handle:horizontal { "
        "  background: #2c313e; "
        "  border-radius: 4px; "
        "  min-width: 20px; "
        "} "
        "QScrollBar::handle:horizontal:hover { "
        "  background: #3b4254; "
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
    NewProjectDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    if (m_project->isDirty()) {
        auto res = QMessageBox::question(this, "Unsaved Changes", "Save changes before creating a new project?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (res == QMessageBox::Yes) {
            onSaveProject();
        } else if (res == QMessageBox::Cancel) {
            return;
        }
    }
    if (dialog.startsFromTemplate()) {
        if (!m_project->loadFromFile(dialog.templateResource())) {
            QMessageBox::warning(this, "Template Error", "The selected project template could not be loaded.");
            return;
        }
        m_project->setProjectName(dialog.projectName());
        m_project->setProjectFilePath(QString());
    } else {
        m_project->newProject(dialog.projectName(), dialog.projectWidth(), dialog.projectHeight());
        m_project->setTargetFramework(dialog.targetFramework());
        DisplayConfig config = m_project->displayConfig();
        config.colorDepth = dialog.colorDepth();
        config.round = dialog.roundDisplay();
        m_project->setDisplayConfig(config);
    }
    if (m_undoStack) m_undoStack->clear();
    m_resolutionCombo->blockSignals(true);
    m_resolutionCombo->setCurrentIndex(-1);
    m_resolutionCombo->blockSignals(false);
    statusBar()->showMessage(dialog.startsFromTemplate()
                                 ? "Project created from template"
                                 : "New project initialized", 3000);
}

void MainWindow::onFlashFirmware() {
    FlashDialog dialog(m_deviceManager, this);
    dialog.exec();
}

void MainWindow::onSerialMonitor() {
    SerialMonitorDialog dialog(m_deviceManager, this);
    dialog.exec();
}

void MainWindow::onPinConfiguration() {
    PinBindingDialog dialog(m_scene, this);
    dialog.exec();
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

void MainWindow::onImportProject() {
    const QString file = QFileDialog::getOpenFileName(
        this, "Import Embedded UI Project", Project::appDataDirectory(),
        "Embedded UI Project (*.euiproj);;All Files (*)");
    if (file.isEmpty()) return;

    const int count = m_project->importFromFile(file, QPointF(20, 20));
    if (count < 0) {
        QMessageBox::warning(this, "Import Failed", "Could not read or parse the selected project file.");
        return;
    }
    if (m_propertiesPanel) m_propertiesPanel->setSelectedComponents({});
    if (m_layerPanel) m_layerPanel->refreshLayers();
    statusBar()->showMessage(QString("Imported %1 component(s) from %2").arg(count).arg(file), 5000);
}

void MainWindow::onImportBoardConfig() {
    QString filter = "Board Config Files (*.ioc sdkconfig* *.h *.ini *.txt);;STM32CubeMX (*.ioc);;ESP-IDF (sdkconfig*);;All Files (*)";
    QString filePath = QFileDialog::getOpenFileName(this, "Import Board Configuration (.ioc / sdkconfig)", QString(), filter);
    if (filePath.isEmpty()) return;

    QString errorMsg;
    if (!m_deviceManager->importBoardConfig(filePath, &errorMsg)) {
        QMessageBox::warning(this, "Board Configuration Import Failed", errorMsg);
        return;
    }

    statusBar()->showMessage(QString("Successfully imported board configuration from %1").arg(QFileInfo(filePath).fileName()), 5000);
    onPinConfiguration();
}

void MainWindow::onImportQmlDesign() {
    QString filter = "QML Files (*.qml);;All Files (*)";
    QString filePath = QFileDialog::getOpenFileName(this, "Import QML Design", QString(), filter);
    if (filePath.isEmpty()) return;

    QmlImportResult result = QmlImporter::importFromFile(filePath);
    if (!result.errorMessage.isEmpty()) {
        QMessageBox::warning(this, "QML Import Failed", result.errorMessage);
        return;
    }

    QSet<QString> existingIds;
    for (UIComponent* c : m_scene->uiComponents()) {
        existingIds.insert(c->componentId());
    }

    for (UIComponent* comp : result.components) {
        QString originalId = comp->componentId();
        QString uniqueId = originalId;
        int suffix = 2;
        while (existingIds.contains(uniqueId)) {
            uniqueId = QString("%1_%2").arg(originalId).arg(suffix++);
        }
        comp->setComponentId(uniqueId);
        existingIds.insert(uniqueId);
        m_scene->addUIComponent(comp);
    }

    if (m_layerPanel) m_layerPanel->refreshLayers();

    if (result.rejectedCount() > 0) {
        QString summary = QString("QML Import completed with %1 component(s) imported.\n\n%2 item(s) were out of scope and rejected:")
                              .arg(result.importedCount())
                              .arg(result.rejectedCount());
        QMessageBox box(result.importedCount() > 0 ? QMessageBox::Information : QMessageBox::Warning,
                        "QML Import Report", summary, QMessageBox::Ok, this);
        box.setDetailedText(result.rejectedItems.join("\n"));
        box.exec();
    } else {
        statusBar()->showMessage(QString("Successfully imported %1 component(s) from %2")
                                     .arg(result.importedCount())
                                     .arg(QFileInfo(filePath).fileName()), 5000);
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

void MainWindow::onExport() {
    const QStringList formats = {"µGFX C Project", "Qt for MCUs (QUL) Project", "LVGL C/C++ Project"};
    bool accepted = false;
    const QString selected = QInputDialog::getItem(this, "Export Project", "Target format:", formats, 0, false, &accepted);
    if (!accepted) return;
    if (selected == formats.at(0)) onExportUgfx();
    else if (selected == formats.at(1)) onExportQtMcu();
    else if (selected == formats.at(2)) onExportLvgl();
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

// ============================================================================
// Helper: current selection
// ============================================================================
QList<UIComponent*> MainWindow::selectedComponents() const {
    QList<UIComponent*> list;
    for (QGraphicsItem* item : m_scene->selectedItems()) {
        if (auto comp = dynamic_cast<UIComponent*>(item)) {
            list.append(comp);
        }
    }
    return list;
}

// ============================================================================
// Align Slots (Task 3)
// ============================================================================
static QRectF selectionBoundingBox(const QList<UIComponent*>& comps) {
    if (comps.isEmpty()) return QRectF();
    qreal minX = std::numeric_limits<qreal>::max();
    qreal minY = std::numeric_limits<qreal>::max();
    qreal maxX = std::numeric_limits<qreal>::lowest();
    qreal maxY = std::numeric_limits<qreal>::lowest();
    for (UIComponent* c : comps) {
        minX = std::min(minX, c->pos().x());
        minY = std::min(minY, c->pos().y());
        maxX = std::max(maxX, c->pos().x() + c->compWidth());
        maxY = std::max(maxY, c->pos().y() + c->compHeight());
    }
    return QRectF(minX, minY, maxX - minX, maxY - minY);
}

void MainWindow::onAlignLeft() {
    QList<UIComponent*> sel = selectedComponents();
    if (sel.size() < 2) return;
    QRectF box = selectionBoundingBox(sel);
    QList<AlignDistributeCommand::Entry> entries;
    for (UIComponent* c : sel) {
        entries.append({c, c->pos(), QPointF(box.left(), c->pos().y())});
    }
    m_undoStack->push(new AlignDistributeCommand(entries, "Align Left"));
}

void MainWindow::onAlignRight() {
    QList<UIComponent*> sel = selectedComponents();
    if (sel.size() < 2) return;
    QRectF box = selectionBoundingBox(sel);
    QList<AlignDistributeCommand::Entry> entries;
    for (UIComponent* c : sel) {
        entries.append({c, c->pos(), QPointF(box.right() - c->compWidth(), c->pos().y())});
    }
    m_undoStack->push(new AlignDistributeCommand(entries, "Align Right"));
}

void MainWindow::onAlignHCenter() {
    QList<UIComponent*> sel = selectedComponents();
    if (sel.size() < 2) return;
    QRectF box = selectionBoundingBox(sel);
    qreal cx = box.center().x();
    QList<AlignDistributeCommand::Entry> entries;
    for (UIComponent* c : sel) {
        entries.append({c, c->pos(), QPointF(cx - c->compWidth() / 2.0, c->pos().y())});
    }
    m_undoStack->push(new AlignDistributeCommand(entries, "Align Horizontal Center"));
}

void MainWindow::onAlignTop() {
    QList<UIComponent*> sel = selectedComponents();
    if (sel.size() < 2) return;
    QRectF box = selectionBoundingBox(sel);
    QList<AlignDistributeCommand::Entry> entries;
    for (UIComponent* c : sel) {
        entries.append({c, c->pos(), QPointF(c->pos().x(), box.top())});
    }
    m_undoStack->push(new AlignDistributeCommand(entries, "Align Top"));
}

void MainWindow::onAlignBottom() {
    QList<UIComponent*> sel = selectedComponents();
    if (sel.size() < 2) return;
    QRectF box = selectionBoundingBox(sel);
    QList<AlignDistributeCommand::Entry> entries;
    for (UIComponent* c : sel) {
        entries.append({c, c->pos(), QPointF(c->pos().x(), box.bottom() - c->compHeight())});
    }
    m_undoStack->push(new AlignDistributeCommand(entries, "Align Bottom"));
}

void MainWindow::onAlignVCenter() {
    QList<UIComponent*> sel = selectedComponents();
    if (sel.size() < 2) return;
    QRectF box = selectionBoundingBox(sel);
    qreal cy = box.center().y();
    QList<AlignDistributeCommand::Entry> entries;
    for (UIComponent* c : sel) {
        entries.append({c, c->pos(), QPointF(c->pos().x(), cy - c->compHeight() / 2.0)});
    }
    m_undoStack->push(new AlignDistributeCommand(entries, "Align Vertical Center"));
}

// ============================================================================
// Distribute Slots (Task 4)
// ============================================================================
void MainWindow::onDistributeH() {
    QList<UIComponent*> sel = selectedComponents();
    if (sel.size() < 3) return;

    std::sort(sel.begin(), sel.end(), [](UIComponent* a, UIComponent* b) {
        return a->pos().x() < b->pos().x();
    });

    qreal leftEdge  = sel.first()->pos().x();
    qreal rightEdge = sel.last()->pos().x() + sel.last()->compWidth();

    qreal totalItemWidth = 0.0;
    for (UIComponent* c : sel) totalItemWidth += c->compWidth();

    qreal totalGapSpace = (rightEdge - leftEdge) - totalItemWidth;
    qreal gap = totalGapSpace / (sel.size() - 1);

    QList<AlignDistributeCommand::Entry> entries;
    qreal cursor = leftEdge;
    for (int i = 0; i < sel.size(); ++i) {
        UIComponent* c = sel[i];
        entries.append({c, c->pos(), QPointF(cursor, c->pos().y())});
        cursor += c->compWidth() + gap;
    }
    m_undoStack->push(new AlignDistributeCommand(entries, "Distribute Horizontal"));
}

void MainWindow::onDistributeV() {
    QList<UIComponent*> sel = selectedComponents();
    if (sel.size() < 3) return;

    std::sort(sel.begin(), sel.end(), [](UIComponent* a, UIComponent* b) {
        return a->pos().y() < b->pos().y();
    });

    qreal topEdge    = sel.first()->pos().y();
    qreal bottomEdge = sel.last()->pos().y() + sel.last()->compHeight();

    qreal totalItemHeight = 0.0;
    for (UIComponent* c : sel) totalItemHeight += c->compHeight();

    qreal totalGapSpace = (bottomEdge - topEdge) - totalItemHeight;
    qreal gap = totalGapSpace / (sel.size() - 1);

    QList<AlignDistributeCommand::Entry> entries;
    qreal cursor = topEdge;
    for (int i = 0; i < sel.size(); ++i) {
        UIComponent* c = sel[i];
        entries.append({c, c->pos(), QPointF(c->pos().x(), cursor)});
        cursor += c->compHeight() + gap;
    }
    m_undoStack->push(new AlignDistributeCommand(entries, "Distribute Vertical"));
}

// ── Boolean Path Operations ───────────────────────────────────────────────

void MainWindow::runBooleanOp(int opCode) {
    QList<UIComponent*> sel = selectedComponents();
    if (sel.size() < 2) return;

    auto op = static_cast<BooleanPathCommand::Operation>(opCode);
    auto* cmd = new BooleanPathCommand(m_scene, sel, op);
    // compute() returns nullptr when the result is empty; guard against that
    if (cmd->isValid())
        m_undoStack->push(cmd);
    else
        delete cmd;
}

void MainWindow::onBooleanUnion()     { runBooleanOp(static_cast<int>(BooleanPathCommand::Operation::Union)); }
void MainWindow::onBooleanSubtract()  { runBooleanOp(static_cast<int>(BooleanPathCommand::Operation::Subtract)); }
void MainWindow::onBooleanIntersect() { runBooleanOp(static_cast<int>(BooleanPathCommand::Operation::Intersect)); }
void MainWindow::onBooleanXor()       { runBooleanOp(static_cast<int>(BooleanPathCommand::Operation::Xor)); }
