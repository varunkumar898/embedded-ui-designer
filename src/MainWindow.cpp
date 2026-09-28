#include "MainWindow.h"
#include "QtMcuGenerator.h"
#include "UgfxGenerator.h"
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
    m_view = new CanvasView(m_scene, this);
    m_project = new Project(m_scene, this);

    setCentralWidget(m_view);

    setupUi();
    setupMenusAndToolbars();
    setupDocks();
    applyTheme();

    setWindowIcon(QIcon(":/packaging/embedded-ui-designer.png"));
    updateWindowTitle();

    // 30-second autosave under QStandardPaths::AppDataLocation
    QTimer* autoSaveTimer = new QTimer(this);
    connect(autoSaveTimer, &QTimer::timeout, this, [this]() {
        if (m_project->isDirty()) {
            m_project->autoSave();
        }
    });
    autoSaveTimer->start(30000);

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
    m_statusLabel = new QLabel("Ready", this);
    m_zoomLabel = new QLabel("Zoom: 100%", this);
    m_zoomLabel->setStyleSheet("padding-right: 12px; color: #9AA5B8; font-weight: bold;");

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
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence::Quit);

    QMenu* editMenu = menuBar()->addMenu("&Edit");
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

    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About Embedded UI Designer", this, &MainWindow::onAbout);

    // Toolbar
    QToolBar* toolbar = addToolBar("Main Toolbar");
    toolbar->setMovable(false);
    toolbar->setStyleSheet("QToolBar { background-color: #21242B; border-bottom: 1px solid #333842; padding: 4px; spacing: 8px; }");

    toolbar->addAction("New", this, &MainWindow::onNewProject);
    toolbar->addAction("Open", this, &MainWindow::onOpenProject);
    toolbar->addAction("Save", this, &MainWindow::onSaveProject);
    toolbar->addSeparator();

    toolbar->addAction("Export µGFX", this, &MainWindow::onExportUgfx);
    toolbar->addAction("Export QUL", this, &MainWindow::onExportQtMcu);
    toolbar->addSeparator();

    QLabel* resLabel = new QLabel(" Target Display: ", this);
    resLabel->setStyleSheet("color: #9AA5B8; font-weight: bold; font-size: 11px;");
    toolbar->addWidget(resLabel);

    m_resolutionCombo = new QComboBox(this);
    m_resolutionCombo->addItem("320 × 240 (QVGA - STM32/ESP32)", QSize(320, 240));
    m_resolutionCombo->addItem("480 × 320 (HVGA - 3.5\" TFT)", QSize(480, 320));
    m_resolutionCombo->addItem("800 × 480 (WVGA - 7\" Industrial HMI)", QSize(800, 480));
    m_resolutionCombo->addItem("240 × 240 (Round/Square Display)", QSize(240, 240));
    m_resolutionCombo->addItem("128 × 64 (OLED Monolithic)", QSize(128, 64));
    m_resolutionCombo->setStyleSheet(
        "QComboBox { background-color: #2F333E; color: #FFFFFF; border: 1px solid #3B404E; border-radius: 4px; padding: 4px 8px; min-width: 220px; font-weight: bold; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background-color: #252830; color: #FFFFFF; selection-background-color: #2196F3; }"
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
        UIComponent* comp = nullptr;
        if (type == "Button") comp = new ButtonComponent("button_new");
        else if (type == "Text" || type == "Label") comp = new LabelComponent("label_new");
        else if (type == "Rectangle") comp = new RectangleComponent("rect_new");
        else if (type == "ProgressBar") comp = new ProgressBarComponent("progress_new");

        if (comp) {
            comp->setPos(m_scene->snapPoint(center - QPointF(comp->compWidth()/2.0, comp->compHeight()/2.0)));
            m_scene->addUIComponent(comp);
            m_scene->clearSelection();
            comp->setSelected(true);
        }
    });

    // Right Top Dock: Properties Panel
    QDockWidget* propDock = new QDockWidget("Properties", this);
    propDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    m_propertiesPanel = new PropertiesPanel(propDock);
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
        "QMainWindow { background-color: #1A1C22; color: #E0E5EE; }"
        "QMenuBar { background-color: #21242B; color: #C5CDD9; border-bottom: 1px solid #333842; padding: 2px 4px; }"
        "QMenuBar::item:selected { background-color: #2F333E; color: #FFFFFF; border-radius: 4px; }"
        "QMenu { background-color: #252830; color: #E0E5EE; border: 1px solid #3B404E; padding: 4px; }"
        "QMenu::item:selected { background-color: #2196F3; color: #FFFFFF; border-radius: 4px; }"
        "QMenu::separator { height: 1px; background-color: #3B404E; margin: 4px 6px; }"
        "QDockWidget { color: #E0E5EE; font-weight: bold; font-size: 11px; }"
        "QDockWidget::title { background-color: #21242B; padding: 8px 12px; border-bottom: 1px solid #333842; }"
        "QStatusBar { background-color: #21242B; color: #9AA5B8; border-top: 1px solid #333842; font-size: 12px; }"
        "QToolButton { background-color: #2F333E; color: #E0E5EE; border: 1px solid #3B404E; border-radius: 4px; padding: 4px 8px; font-weight: bold; }"
        "QToolButton:hover { background-color: #3B404E; color: #FFFFFF; border-color: #2196F3; }"
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
    m_resolutionCombo->setCurrentIndex(0);
    statusBar()->showMessage("New project initialized", 3000);
}

void MainWindow::onOpenProject() {
    QString file = QFileDialog::getOpenFileName(this, "Open Embedded UI Project", "", "Embedded UI Project (*.euiproj);;All Files (*)");
    if (!file.isEmpty()) {
        if (m_project->loadFromFile(file)) {
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
            statusBar()->showMessage("Project saved successfully.", 3000);
        } else {
            QMessageBox::warning(this, "Error", "Failed to save project.");
        }
    }
}

void MainWindow::onSaveProjectAs() {
    QString file = QFileDialog::getSaveFileName(this, "Save Embedded UI Project As", m_project->projectName() + ".euiproj", "Embedded UI Project (*.euiproj);;All Files (*)");
    if (!file.isEmpty()) {
        if (m_project->saveToFile(file)) {
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

void MainWindow::onDeleteSelected() {
    for (QGraphicsItem* item : m_scene->selectedItems()) {
        if (auto comp = dynamic_cast<UIComponent*>(item)) {
            m_scene->removeUIComponent(comp);
            delete comp;
        }
    }
}

void MainWindow::onDuplicateSelected() {
    for (QGraphicsItem* item : m_scene->selectedItems()) {
        if (auto comp = dynamic_cast<UIComponent*>(item)) {
            QJsonObject json = comp->toJson();
            m_scene->clearSelection();
            if (comp->componentType() == "Button") {
                auto dup = new ButtonComponent(comp->componentId() + "_copy");
                dup->fromJson(json);
                dup->setCompPos(comp->pos().x() + 15, comp->pos().y() + 15);
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
