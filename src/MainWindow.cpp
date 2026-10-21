#include "MainWindow.h"
#include "QtMcuGenerator.h"
#include "UgfxGenerator.h"
#include "LvglGenerator.h"
#include "FlashDialog.h"
#include "SerialMonitorDialog.h"
#include "PinBindingDialog.h"
#include "NewProjectDialog.h"
#include "HardwareWizardDialog.h"
#include "dialogs/InvalidBindingsDialog.h"
#include "DeviceManager.h"
#include "HardwareBridge.h"
#include "HardwareManager.h"
#include "SimulatorWindow.h"
#include "hardware/BoardRegistry.h"
#include "dialogs/AISettingsDialog.h"
#include "panels/AIAssistantPanel.h"
#include "dialogs/GitDialog.h"
#include "dialogs/CICDDialog.h"
#include "models/DiagnosticsEngine.h"
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
#include "GroupComponent.h"
#include "panels/ContextualTopToolbar.h"
#include "panels/FloatingSelectionToolbar.h"
#include "panels/EffectsPanel.h"
#include "panels/UploadsPanel.h"
#include "panels/ResourceAnalyzerPanel.h"
#include "panels/BoardDisplayPanel.h"
#include "dialogs/ExportValidationDialog.h"
#include "dialogs/ExportDialog.h"
#include "panels/BuildOutputPanel.h"
#include "panels/DeviceMonitorPanel.h"
#include "panels/ProblemsPanel.h"
#include "diagnostics/DiagnosticsService.h"
#include "diagnostics/DiagnosticsSources.h"
#include "analyzer/ResourceEstimator.h"
#include "codegen/BackendRegistry.h"
#include "project/QmlImporter.h"
#include "importers/HtmlCssImporter.h"
#include "ir/IRAdapter.h"
#include "api/DesignerController.h"
#include "ui/dialogs/BoardSelectorDialog.h"
#include "components/ComponentLibrary.h"
#include "api/DesignerLocalServer.h"
#include "styles/StyleManager.h"
#include "theme/Theme.h"
#include "hardware/OpenOCDWorker.h"
#include "workspace/CodeWorkspaceView.h"
#include "workspace/TabbedCodeEditor.h"
#include "workspace/ProjectExplorerWidget.h"
#include "workspace/DetachableTabWidget.h"
#include <QScreen>
#include <QGuiApplication>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QProgressBar>
#include <QUndoStack>
#include <QMenuBar>
#include <QToolBar>
#include <QToolButton>
#include <QAction>
#include <QStatusBar>
#include <QKeySequence>
#include <QDebug>
#include <QSettings>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QTcpSocket>
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>
#include <QTimer>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTextBrowser>
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

    m_diagnosticsService = new Diagnostics::DiagnosticsService(this);
    Diagnostics::DiagnosticsSources::registerBuiltinSources(*m_diagnosticsService);
    m_diagnosticsService->setProject(m_project);

    connect(m_undoStack, &QUndoStack::cleanChanged, this, [this](bool clean) {
        m_project->setDirty(!clean);
    });

    // Top Context-sensitive Canva-inspired toolbar & Floating selection toolbar
    m_topToolbar = new ContextualTopToolbar(m_view, m_scene, m_undoStack, m_project, this);
    m_contextualToolbar = m_topToolbar;
    m_floatingToolbar = new FloatingSelectionToolbar(m_view, m_scene, m_undoStack, m_view->viewport());

    m_centralTabs = new DetachableTabWidget(this);
    m_centralTabs->setObjectName("centralTabs");

    m_centralStack = new QStackedWidget(this);

    // Page 0: Visual Design Workspace
    m_designWorkspace = new DesignWorkspace(m_centralTabs);
    m_designWorkspace->setCanvasView(m_view);
    m_designWorkspace->setTopToolbar(m_topToolbar);
    m_designWorkspace->syncWithProject(m_project);
    m_designContainer = m_designWorkspace;

    // Phase 3: Setup banner in editor
    m_boardBanner = new QWidget(m_designWorkspace);
    m_boardBanner->setObjectName("boardBanner");
    QHBoxLayout* bannerLayout = new QHBoxLayout(m_boardBanner);
    bannerLayout->setContentsMargins(12, 6, 12, 6);
    bannerLayout->setSpacing(10);
    m_boardBanner->setStyleSheet(QString(
        "QWidget#boardBanner { "
        "  background-color: #2a2415; "
        "  border-bottom: 1px solid #d97706; "
        "} "
    ));

    QLabel* bannerIcon = new QLabel("⚠", m_boardBanner);
    bannerIcon->setStyleSheet("color: #f59e0b; font-size: 14px; font-weight: bold;");
    bannerLayout->addWidget(bannerIcon);

    QLabel* bannerLabel = new QLabel("No target board configured. Operating with placeholder generic board.", m_boardBanner);
    bannerLabel->setObjectName("boardBannerLabel");
    bannerLabel->setStyleSheet("color: #fef3c7; font-size: 12px; font-weight: 500;");
    bannerLayout->addWidget(bannerLabel, 1);

    QPushButton* btnChooseBoard = new QPushButton("Choose Board...", m_boardBanner);
    btnChooseBoard->setObjectName("btnBannerChooseBoard");
    btnChooseBoard->setCursor(Qt::PointingHandCursor);
    btnChooseBoard->setStyleSheet(QString(
        "QPushButton { "
        "  background-color: #d97706; "
        "  color: #ffffff; "
        "  border: none; "
        "  border-radius: %1px; "
        "  padding: 4px 12px; "
        "  font-weight: 600; "
        "  font-size: 11px; "
        "} "
        "QPushButton:hover { background-color: #b45309; } "
    ).arg(Theme::RadiusSm));
    connect(btnChooseBoard, &QPushButton::clicked, this, &MainWindow::onChangeBoard);
    bannerLayout->addWidget(btnChooseBoard);

    m_designWorkspace->setBoardBanner(m_boardBanner);
    m_boardBanner->hide();

    m_centralTabs->addTab(m_designWorkspace, "Canvas");

    // Page 1: Firmware Code Workspace
    m_codeWorkspace = new CodeWorkspaceView(m_project, m_deviceManager, m_centralTabs);
    m_centralTabs->addTab(m_codeWorkspace, "</> Code Workspace");

    // Keep m_centralStack populated for compatibility
    m_centralStack->addWidget(new QWidget(m_centralStack));
    m_centralStack->addWidget(new QWidget(m_centralStack));

    m_workspaceManager = new WorkspaceManager(this, m_centralStack, m_designWorkspace, m_codeWorkspace, this);
    m_workspaceManager->setProject(m_project);

    setCentralWidget(m_centralTabs);

    connect(m_centralTabs, &QTabWidget::currentChanged, this, [this](int idx) {
        if (m_inWorkspaceModeSwitch || idx < 0) return;
        QWidget* w = m_centralTabs->widget(idx);
        if (w == m_designWorkspace || w == m_designContainer) {
            setWorkspaceMode(WorkspaceMode::Design);
        } else if (w == m_codeWorkspace) {
            setWorkspaceMode(WorkspaceMode::Code);
        }
    });

    connect(m_centralTabs, &DetachableTabWidget::activeEditorChanged, this, [this](QWidget* activeWidget) {
        if (!activeWidget) return;
        CanvasView* view = qobject_cast<CanvasView*>(activeWidget);
        if (!view) view = activeWidget->findChild<CanvasView*>();
        if (view && m_propertiesPanel) {
            m_propertiesPanel->setSelectedComponents(selectedComponents());
        }
    });

    connect(m_project, &Project::projectDirectoryChanged, this, [this](const QString&) {
        if (m_codeWorkspace) m_codeWorkspace->syncWithProject(m_project);
    });
    connect(m_project, &Project::projectLoaded, this, [this]() {
        if (m_codeWorkspace) m_codeWorkspace->syncWithProject(m_project);
    });
    connect(m_project, &Project::projectModified, this, [this]() {
        if (m_codeWorkspace) {
            QString err;
            m_project->syncFirmwareFiles(&err);
            if (m_codeWorkspace->projectExplorer()) {
                m_codeWorkspace->projectExplorer()->refresh();
            }
        }
    });

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
    m_canvasView = m_view;
    m_deviceMonitor = m_deviceMonitorPanel;

    // Pass OpenOCD manager to canvas
    if (m_canvasView) {
        m_canvasView->setOcdTelnetManager(&m_openocd);
    }

    // Connect Live Test mode toggle to canvas
    connect(this, &MainWindow::liveTestModeChanged,
            m_canvasView, &CanvasView::setLiveTestMode);

    // When canvas emits GPIO change, send to Device Monitor
    connect(m_canvasView, &CanvasView::gpioStateChanged,
            m_deviceMonitor, &DeviceMonitorPanel::onGPIOStateChanged);

    connect(m_scene, &CanvasScene::selectionListChanged, m_propertiesPanel, &PropertiesPanel::setSelectedComponents);
    connect(m_scene, &CanvasScene::selectionListChanged, m_topToolbar, &ContextualTopToolbar::updateSelection);
    connect(m_scene, &CanvasScene::selectionListChanged, m_floatingToolbar, &FloatingSelectionToolbar::updateSelection);
    connect(m_scene, &CanvasScene::selectionListChanged, m_effectsPanel, &EffectsPanel::updateSelection);
    connect(m_scene, &CanvasScene::componentSelected, m_prototypePanel, &PrototypePanel::setTargetComponent);
    connect(m_scene, &CanvasScene::componentChanged, m_propertiesPanel, &PropertiesPanel::refreshValues);
    connect(m_scene, &CanvasScene::componentChanged, m_floatingToolbar, &FloatingSelectionToolbar::reposition);

    // Top Toolbar actions
    connect(m_topToolbar, &ContextualTopToolbar::openEffectsPanelRequested, this, [this]() {
        if (m_effectsDock) {
            m_effectsDock->show();
            m_effectsDock->raise();
        }
    });
    connect(m_topToolbar, &ContextualTopToolbar::openUploadsPanelRequested, this, [this]() {
        if (m_uploadsDock) {
            m_uploadsDock->show();
            m_uploadsDock->raise();
        }
    });
    connect(m_topToolbar, &ContextualTopToolbar::replaceImageRequested, this, [this]() {
        if (m_uploadsDock) {
            m_uploadsDock->show();
            m_uploadsDock->raise();
        }
    });
    connect(m_topToolbar, &ContextualTopToolbar::addTextRequested, this, [this]() {
        QPointF center = m_scene->displayRect().center();
        UIComponent* comp = Project::createComponentInstance("Text", "text_new");
        if (comp) {
            comp->setPos(m_scene->snapPoint(center - QPointF(comp->compWidth()/2.0, comp->compHeight()/2.0)));
            m_scene->clearSelection();
            if (m_undoStack) m_undoStack->push(new AddComponentCommand(m_scene, comp));
        }
    });
    connect(m_topToolbar, &ContextualTopToolbar::addShapeRequested, this, [this]() {
        QPointF center = m_scene->displayRect().center();
        UIComponent* comp = Project::createComponentInstance("Rectangle", "rect_new");
        if (comp) {
            comp->setPos(m_scene->snapPoint(center - QPointF(comp->compWidth()/2.0, comp->compHeight()/2.0)));
            m_scene->clearSelection();
            if (m_undoStack) m_undoStack->push(new AddComponentCommand(m_scene, comp));
        }
    });

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
    connect(m_view, &CanvasView::zoomChanged, m_floatingToolbar, &FloatingSelectionToolbar::reposition);
    connect(m_view, &CanvasView::statusMessageRequested, this, [this](const QString& msg) {
        statusBar()->showMessage(msg, 3000);
    });
    connect(m_project, &Project::projectModified, this, &MainWindow::updateWindowTitle);
    connect(m_project, &Project::activeScreenChanged, this, [this](Screen*) {
        updateWindowTitle();
        if (m_layerPanel) m_layerPanel->refreshLayers();
        if (m_propertiesPanel) m_propertiesPanel->setTargetComponent(nullptr);
        if (m_resourcePanel) m_resourcePanel->refresh();
    });
    connect(m_project, &Project::projectLoaded, this, [this]() {
        updateWindowTitle();
        m_layerPanel->refreshLayers();
        m_propertiesPanel->setSelectedComponents({});
        if (m_screensPanel) m_screensPanel->refreshScreenList();
        if (m_resourcePanel) m_resourcePanel->refresh();
    });
    connect(m_project, &Project::projectModified, this, [this]() {
        if (m_resourcePanel) m_resourcePanel->refresh();
    });
    connect(m_project, &Project::hardwareConfigChanged, this, [this]() {
        if (m_resourcePanel && m_project) {
            QString devId = !m_project->hardwareConfig().deviceId.isEmpty() ? m_project->hardwareConfig().deviceId : (!m_project->hardwareConfig().boardId.isEmpty() ? m_project->hardwareConfig().boardId : "STM32H745");
            m_resourcePanel->setDevice(devId);
        }
    });
    // Task 5: Board & Display edits refresh the analyzer immediately and ask the
    // diagnostics service for a (debounced) recompute of the Problems list.
    if (m_boardDisplayPanel) {
        connect(m_boardDisplayPanel, &BoardDisplayPanel::settingsChanged, this, [this]() {
            if (m_resourcePanel) m_resourcePanel->refresh();
            if (m_diagnosticsService) m_diagnosticsService->scheduleRefresh(0);
        });
    }
    connect(m_scene, &CanvasScene::componentChanged, this, [this]() {
        if (m_resourcePanel) m_resourcePanel->refresh();
    });
    connect(m_scene, &CanvasScene::componentAdded, this, [this](UIComponent*) {
        if (m_resourcePanel) m_resourcePanel->refresh();
    });
    connect(m_scene, &CanvasScene::componentRemoved, this, [this](UIComponent*) {
        if (m_resourcePanel) m_resourcePanel->refresh();
    });

    setupDiagnostics();


    // ── Live MCP Controller & Local IPC Server ──────────────────────────────
    m_controller = new DesignerController(this, this);
    m_localServer = new DesignerLocalServer(m_controller, this);
    m_localServer->start(8765);

    resize(1380, 880);

    // First-run board onboarding check
    QTimer::singleShot(0, this, [this]() {
        checkFirstRunOnboarding();
    });
}

MainWindow::~MainWindow() {
    saveWindowLayout();
    stopLocalMcpServer();
    if (m_openocdThread) {
        if (m_openocdWorker) {
            m_openocdWorker->cancelConnection();
        }
        m_openocdThread->quit();
        m_openocdThread->wait(1000);
    }
    if (m_undoStack) {
        m_undoStack->disconnect();
    }
    if (m_project) {
        m_project->disconnect();
    }
}

bool MainWindow::startLocalMcpServer(quint16 port) {
    if (!m_localServer) {
        if (!m_controller) m_controller = new DesignerController(this, this);
        m_localServer = new DesignerLocalServer(m_controller, this);
    }
    return m_localServer->start(port);
}

void MainWindow::stopLocalMcpServer() {
    if (m_localServer) {
        m_localServer->stop();
    }
}

void MainWindow::setupUi() {
    QWidget* statusWidget = new QWidget(this);
    QHBoxLayout* statusLayout = new QHBoxLayout(statusWidget);
    statusLayout->setContentsMargins(4, 0, 0, 0);
    statusLayout->setSpacing(8);

    QLabel* greenDot = new QLabel(statusWidget);
    greenDot->setFixedSize(8, 8);
    greenDot->setStyleSheet(QString("background-color: %1; border-radius: %2px;")
        .arg(Theme::Success).arg(Theme::RadiusSm));

    m_statusLabel = new QLabel("Ready", statusWidget);
    m_statusLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: %2;")
        .arg(Theme::TextPrimary).arg(Theme::FontWeightEmphasized));

    m_hardwareStatusBadge = new QLabel(statusWidget);
    m_hardwareStatusBadge->setObjectName("hardwareStatusBadge");

    m_hardwareConnectBtn = new QPushButton("Connect", statusWidget);
    m_hardwareConnectBtn->setObjectName("hardwareConnectBtn");
    m_hardwareConnectBtn->setCursor(Qt::PointingHandCursor);
    m_hardwareConnectBtn->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; border: 1px solid %3; "
        "border-radius: %4px; padding: 2px 10px; font-weight: bold; font-size: 11px; } "
        "QPushButton:hover { background: %5; } "
        "QPushButton:disabled { color: %6; background: %7; border-color: %3; }"
    ).arg(Theme::BgRaised, Theme::AccentText, Theme::BorderSubtle, QString::number(Theme::RadiusSm),
          Theme::BgOverlay, Theme::TextDisabled, Theme::BgSurface));
    connect(m_hardwareConnectBtn, &QPushButton::clicked, this, [this]() {
        if (HardwareBridge::instance().isHardwareConnected() || m_openocd.isConnected()) {
            HardwareBridge::instance().disconnectHardware();
            m_openocd.disconnect();
            if (m_connectionStatusLabel) {
                m_connectionStatusLabel->setText("Not connected");
                m_connectionStatusLabel->setStyleSheet("color: #FF6B6B; font-style: italic;");
            }
            updateHardwareStatusBadge(false, "");
        } else {
            HardwareBridge::instance().connectHardware();
            onConnectToBoard();
        }
    });

    QPushButton* rescanBtn = new QPushButton("Rescan", statusWidget);
    rescanBtn->setObjectName("hardwareRescanBtn");
    rescanBtn->setCursor(Qt::PointingHandCursor);
    rescanBtn->setToolTip("Rescan for connected USB debug probes");
    rescanBtn->setStyleSheet(m_hardwareConnectBtn->styleSheet());
    connect(rescanBtn, &QPushButton::clicked, this, [this]() {
        HardwareBridge::instance().rescan();
        updateHardwareStatusBadge(HardwareBridge::instance().isHardwareConnected(),
                                  HardwareBridge::instance().connectedProbeName());
    });

    statusLayout->addWidget(greenDot);
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addSpacing(16);
    statusLayout->addWidget(m_hardwareStatusBadge);
    statusLayout->addSpacing(6);
    statusLayout->addWidget(m_hardwareConnectBtn);
    statusLayout->addWidget(rescanBtn);

    updateHardwareStatusBadge(HardwareBridge::instance().isHardwareConnected(),
                              HardwareBridge::instance().connectedProbeName());

    connect(&HardwareBridge::instance(), &HardwareBridge::connectionStatusChanged,
            this, &MainWindow::updateHardwareStatusBadge);
    connect(&HardwareBridge::instance(), &HardwareBridge::boardChanged,
            this, [this](const QString&) {
                updateHardwareStatusBadge(HardwareBridge::instance().isHardwareConnected(),
                                          HardwareBridge::instance().connectedProbeName());
            });
    connect(&HardwareBridge::instance(), &HardwareBridge::probeDiscovered,
            this, [this](const QString&) {
                updateHardwareStatusBadge(HardwareBridge::instance().isHardwareConnected(),
                                          HardwareBridge::instance().connectedProbeName());
            });
    connect(&HardwareBridge::instance(), &HardwareBridge::probeRemoved,
            this, [this]() {
                updateHardwareStatusBadge(HardwareBridge::instance().isHardwareConnected(),
                                          HardwareBridge::instance().connectedProbeName());
            });

    statusBar()->setSizeGripEnabled(false);
    m_zoomLabel = new QLabel("Zoom: 100%", this);
    m_zoomLabel->setStyleSheet(QString("padding-right: 20px; color: %1; font-size: 12px; font-weight: %2;")
        .arg(Theme::TextSecondary).arg(Theme::FontWeightEmphasized));

    statusBar()->addWidget(statusWidget, 1);
    statusBar()->addPermanentWidget(m_zoomLabel);
}

void MainWindow::updateHardwareStatusBadge(bool connected, const QString& probeName) {
    if (!m_hardwareStatusBadge) return;

    if (connected) {
        QString name = probeName.isEmpty() ? "STM32F030R8 via ST-Link" : probeName;
        m_hardwareStatusBadge->setText(QString("● Connected: %1").arg(name));
        m_hardwareStatusBadge->setStyleSheet(QString(
            "color: %1; background-color: %2; border: 1px solid %1; "
            "border-radius: %3px; padding: 2px 8px; font-weight: bold; font-size: 11px;"
        ).arg(Theme::Success, Theme::SuccessBg, QString::number(Theme::RadiusSm)));
        if (m_hardwareConnectBtn) {
            m_hardwareConnectBtn->setText("Disconnect");
            m_hardwareConnectBtn->setEnabled(true);
        }
    } else if (HardwareBridge::instance().isProbeDetected()) {
        QString detectedName = HardwareBridge::instance().detectedBoardName();
        if (detectedName.isEmpty()) detectedName = "STM32F030R8 via ST-Link";
        m_hardwareStatusBadge->setText(QString("⚡ Board detected: %1").arg(detectedName));
        m_hardwareStatusBadge->setStyleSheet(QString(
            "color: %1; background-color: %2; border: 1px solid %1; "
            "border-radius: %3px; padding: 2px 8px; font-weight: bold; font-size: 11px;"
        ).arg(Theme::Info, Theme::InfoBg, QString::number(Theme::RadiusSm)));
        if (m_hardwareConnectBtn) {
            m_hardwareConnectBtn->setText("Connect");
            m_hardwareConnectBtn->setEnabled(true);
        }
    } else {
        m_hardwareStatusBadge->setText("No probe connected (Not Found - Simulated Mode)");
        m_hardwareStatusBadge->setStyleSheet(QString(
            "color: %1; background-color: %2; border: 1px solid %1; "
            "border-radius: %3px; padding: 2px 8px; font-weight: bold; font-size: 11px;"
        ).arg(Theme::Warning, Theme::WarningBg, QString::number(Theme::RadiusSm)));
        if (m_hardwareConnectBtn) {
            m_hardwareConnectBtn->setText("Connect");
            m_hardwareConnectBtn->setEnabled(true);
        }
    }
}

void MainWindow::setupMenusAndToolbars() {
    // Top Menus
    QMenu* fileMenu = menuBar()->addMenu("File");
    fileMenu->addAction("New Project...", this, &MainWindow::onNewProject, QKeySequence::New);
    fileMenu->addAction("New Embedded Project...", this, &MainWindow::onCreateEmbeddedProject, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
    fileMenu->addAction("Open Project...", this, &MainWindow::onOpenProject, QKeySequence::Open);
    fileMenu->addAction("Open Firmware Folder...", this, &MainWindow::onOpenFirmwareFolder, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    fileMenu->addAction("Open Sample Project (Thermostat)", this, &MainWindow::onOpenSampleProject);
    fileMenu->addAction("Import...", this, &MainWindow::onImportProject, QKeySequence(Qt::CTRL | Qt::Key_I));
    fileMenu->addAction("Import HTML/CSS Design...", this, &MainWindow::onImportHtmlCss);
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
    viewMenu->addAction("Design / Canvas Workspace", this, &MainWindow::switchToDesignWorkspace, QKeySequence("Ctrl+1"));
    viewMenu->addAction("</> Code Workspace", this, &MainWindow::switchToCodeWorkspace, QKeySequence("Ctrl+2"));
    viewMenu->addSeparator();

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
    viewMenu->addSeparator();

    m_actBuildOutput = viewMenu->addAction("Build Output", this, [this]() {
        if (m_buildDock) {
            m_buildDock->setVisible(!m_buildDock->isVisible());
        }
    });
    m_actBuildOutput->setCheckable(true);
    m_actBuildOutput->setChecked(false);
    if (m_buildDock && m_actBuildOutput) {
        connect(m_buildDock, &QDockWidget::visibilityChanged, m_actBuildOutput, &QAction::setChecked);
    }

    m_actDeviceMonitor = viewMenu->addAction("Device Monitor", this, [this]() {
        if (m_deviceMonitorDock) {
            m_deviceMonitorDock->setVisible(!m_deviceMonitorDock->isVisible());
        }
    });
    m_actDeviceMonitor->setCheckable(true);
    m_actDeviceMonitor->setChecked(false);
    if (m_deviceMonitorDock && m_actDeviceMonitor) {
        connect(m_deviceMonitorDock, &QDockWidget::visibilityChanged, m_actDeviceMonitor, &QAction::setChecked);
    }

    m_actProblems = viewMenu->addAction("Problems", this, [this]() {
        if (m_problemsDock) {
            m_problemsDock->setVisible(!m_problemsDock->isVisible());
            if (m_problemsDock->isVisible()) m_problemsDock->raise();
        }
    });
    m_actProblems->setCheckable(true);
    m_actProblems->setChecked(false);
    if (m_problemsDock && m_actProblems) {
        connect(m_problemsDock, &QDockWidget::visibilityChanged, m_actProblems, &QAction::setChecked);
    }

    m_actResourceAnalyzer = viewMenu->addAction("Resource Analyzer", this, [this]() {
        if (m_resourceDock) {
            m_resourceDock->setVisible(!m_resourceDock->isVisible());
        }
    });
    m_actResourceAnalyzer->setCheckable(true);
    m_actResourceAnalyzer->setChecked(false);
    if (m_resourceDock && m_actResourceAnalyzer) {
        connect(m_resourceDock, &QDockWidget::visibilityChanged, m_actResourceAnalyzer, &QAction::setChecked);
    }

    m_actBoardDisplay = viewMenu->addAction("Board && Display", this, [this]() {
        if (m_boardDisplayDock) {
            m_boardDisplayDock->setVisible(!m_boardDisplayDock->isVisible());
            if (m_boardDisplayDock->isVisible()) m_boardDisplayDock->raise();
        }
    });
    m_actBoardDisplay->setCheckable(true);
    m_actBoardDisplay->setChecked(false);
    if (m_boardDisplayDock && m_actBoardDisplay) {
        connect(m_boardDisplayDock, &QDockWidget::visibilityChanged, m_actBoardDisplay, &QAction::setChecked);
    }

    viewMenu->addSeparator();
    viewMenu->addAction("Toggle AI Assistant Panel", this, [this]() {
        if (m_aiDockWidget) m_aiDockWidget->setVisible(!m_aiDockWidget->isVisible());
    });
    viewMenu->addSeparator();
    viewMenu->addAction("Reset Layout", this, &MainWindow::resetLayout, QKeySequence("Ctrl+Shift+R"));

    QMenu* projectMenu = menuBar()->addMenu("Project");
    projectMenu->addAction("Project Settings...", this, &MainWindow::onProjectSettingsDialog);
    projectMenu->addAction("Export...", this, &MainWindow::onExport, QKeySequence(Qt::CTRL | Qt::Key_E));
    projectMenu->addAction("Resource Analyzer", this, [this]() {
        if (m_resourcePanel && m_resourcePanel->parentWidget()) {
            m_resourcePanel->parentWidget()->show();
            m_resourcePanel->parentWidget()->raise();
        }
    });
    projectMenu->addSeparator();
    projectMenu->addAction("Git & GitHub...", this, &MainWindow::onGitDialog);
    projectMenu->addAction("CI/CD Pipeline Generator...", this, &MainWindow::onCICDDialog);
    projectMenu->addAction("Project Diagnostics...", this, &MainWindow::onDiagnosticsDialog);

    QMenu* simMenu = menuBar()->addMenu("Simulation");
    simMenu->addAction("Run Desktop Simulator...", this, &MainWindow::onRunSimulator, QKeySequence(Qt::CTRL | Qt::Key_R));

    QMenu* deviceMenu = menuBar()->addMenu("Device");
    deviceMenu->addAction("Create Embedded Project / Target Selector...", this, &MainWindow::onCreateEmbeddedProject);
    deviceMenu->addAction("Pin Configuration & Binding...", this, &MainWindow::onPinConfiguration);
    deviceMenu->addAction("Target Board Specifications (espboards.dev)...", this, &MainWindow::onOpenBoardSelectorDialog);
    deviceMenu->addAction("Device Monitor (Live Telemetry)...", this, [this]() {
        if (m_deviceMonitorPanel && m_deviceMonitorPanel->parentWidget()) {
            m_deviceMonitorPanel->parentWidget()->show();
            m_deviceMonitorPanel->parentWidget()->raise();
        }
    });
    deviceMenu->addAction("Serial Monitor...", this, &MainWindow::onSerialMonitor);
    deviceMenu->addAction("Flash Firmware...", this, &MainWindow::onFlashFirmware);
    deviceMenu->addSeparator();
    deviceMenu->addAction("Load Preset...", this, [this]() {
        auto presets = BoardRegistry::instance().presets();
        if (presets.isEmpty()) {
            QMessageBox::information(this, "Load Preset", "No saved presets found.");
            return;
        }
        QStringList names;
        for (const auto& p : presets) names.append(p.name);
        bool ok = false;
        QString chosen = QInputDialog::getItem(this, "Load Preset", "Select preset to load:", names, 0, false, &ok);
        if (ok && !chosen.isEmpty()) {
            BoardRegistry::instance().selectPreset(chosen);
            statusBar()->showMessage(QString("Preset '%1' loaded").arg(chosen), 3000);
        }
    });
    deviceMenu->addAction("Save as Preset...", this, [this]() {
        bool ok = false;
        QString defaultName = BoardRegistry::instance().activePreset().name.isEmpty() ?
            (BoardRegistry::instance().activeBoard().displayName + " Preset") :
            BoardRegistry::instance().activePreset().name;
        QString name = QInputDialog::getText(this, "Save Preset", "Preset Name:", QLineEdit::Normal, defaultName, &ok);
        if (ok && !name.trimmed().isEmpty()) {
            BoardRegistry::instance().savePreset(name.trimmed(), BoardRegistry::instance().activeBoard().id);
            statusBar()->showMessage(QString("Preset '%1' saved").arg(name.trimmed()), 3000);
        }
    });
    deviceMenu->addAction("Delete Preset...", this, [this]() {
        auto presets = BoardRegistry::instance().presets();
        if (presets.isEmpty()) {
            QMessageBox::information(this, "Delete Preset", "No saved presets found.");
            return;
        }
        QStringList names;
        for (const auto& p : presets) names.append(p.name);
        bool ok = false;
        QString chosen = QInputDialog::getItem(this, "Delete Preset", "Select preset to delete:", names, 0, false, &ok);
        if (ok && !chosen.isEmpty()) {
            BoardRegistry::instance().deletePreset(chosen);
            statusBar()->showMessage(QString("Preset '%1' deleted").arg(chosen), 3000);
        }
    });
    deviceMenu->addSeparator();
    deviceMenu->addAction("Change Board...", this, &MainWindow::onChangeBoard);

    QMenu* settingsMenu = menuBar()->addMenu("Settings");
    settingsMenu->addAction("AI Providers...", this, &MainWindow::onAISettingsDialog);

    QMenu* themeMenu = settingsMenu->addMenu("Theme");
    QAction* darkAction = themeMenu->addAction("Dark Theme", this, [this]() {
        setTheme("dark");
    });
    QAction* lightAction = themeMenu->addAction("Light Theme", this, [this]() {
        setTheme("light");
    });
    darkAction->setCheckable(true);
    lightAction->setCheckable(true);
    if (StyleManager::currentTheme().toLower() == QLatin1String("light")) {
        lightAction->setChecked(true);
    } else {
        darkAction->setChecked(true);
    }

    QMenu* windowMenu = menuBar()->addMenu("Window");
    QMenu* moveMenu = windowMenu->addMenu("Move to Monitor");
    connect(moveMenu, &QMenu::aboutToShow, this, [this, moveMenu]() {
        moveMenu->clear();
        const auto screens = QGuiApplication::screens();
        for (int i = 0; i < screens.size(); ++i) {
            QString label = QString("Monitor %1: %2x%3")
                .arg(i + 1)
                .arg(screens[i]->geometry().width())
                .arg(screens[i]->geometry().height());
            if (screens[i] == QGuiApplication::primaryScreen()) {
                label += " (Primary)";
            }
            QAction* act = moveMenu->addAction(label);
            connect(act, &QAction::triggered, this, [this, scr = screens[i]]() {
                moveActiveWindowToScreen(scr);
            });
        }
    });

    windowMenu->addAction("Detach Canvas to Window", this, &MainWindow::detachCanvasWindow);
    windowMenu->addAction("Detach Code Workspace to Window", this, &MainWindow::detachCodeWorkspaceWindow);
    windowMenu->addAction("Re-dock All Detached Windows", this, &MainWindow::redockAllDetachedWindows);
    windowMenu->addSeparator();
    windowMenu->addAction("Reset Layout", this, &MainWindow::resetLayout, QKeySequence("Ctrl+Shift+R"));

    QMenu* helpMenu = menuBar()->addMenu("Help");
    helpMenu->addAction("About Embedded UI Designer", this, &MainWindow::onAbout);

    // Toolbar
    QToolBar* toolbar = addToolBar("Main Toolbar");
    toolbar->setObjectName("mainToolbar");
    toolbar->setMovable(false);
    m_toolbar = toolbar;

    // Workspace Switching Controls (Design / Canvas vs </> Code)
    QWidget* switcherWidget = new QWidget(this);
    switcherWidget->setObjectName("workspaceSwitcher");
    QHBoxLayout* switcherLayout = new QHBoxLayout(switcherWidget);
    switcherLayout->setContentsMargins(2, 0, 8, 0);
    switcherLayout->setSpacing(4);

    m_designModeBtn = new QToolButton(switcherWidget);
    m_designModeBtn->setText("Design / Canvas");
    m_designModeBtn->setCheckable(true);
    m_designModeBtn->setChecked(true);
    m_designModeBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_designModeBtn->setCursor(Qt::PointingHandCursor);
    m_designModeBtn->setToolTip("Open Visual UI Designer Workspace (Ctrl+1)");

    m_codeModeBtn = new QToolButton(switcherWidget);
    m_codeModeBtn->setText("</> Code");
    m_codeModeBtn->setCheckable(true);
    m_codeModeBtn->setChecked(false);
    m_codeModeBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_codeModeBtn->setCursor(Qt::PointingHandCursor);
    m_codeModeBtn->setToolTip("Open Firmware Code Editor Workspace (Ctrl+2)");

    m_workspaceGroup = new QButtonGroup(this);
    m_workspaceGroup->setExclusive(true);
    m_workspaceGroup->addButton(m_designModeBtn, 0);
    m_workspaceGroup->addButton(m_codeModeBtn, 1);

    switcherLayout->addWidget(m_designModeBtn);
    switcherLayout->addWidget(m_codeModeBtn);

    switcherWidget->setStyleSheet(QString(
        "QToolButton { "
        "  background-color: %1; "
        "  color: %2; "
        "  border: 1px solid %3; "
        "  border-radius: %4px; "
        "  padding: 5px 12px; "
        "  font-weight: 600; "
        "  font-size: 11.5px; "
        "} "
        "QToolButton:hover { "
        "  background-color: %5; "
        "  color: %6; "
        "} "
        "QToolButton:checked { "
        "  background-color: #1f6feb; "
        "  color: #ffffff; "
        "  border: 1px solid #388bfd; "
        "  font-weight: bold; "
        "} "
    ).arg(Theme::BgRaised, Theme::TextSecondary, Theme::BorderSubtle, QString::number(Theme::RadiusSm),
          Theme::BgOverlay, Theme::TextPrimary));

    connect(m_designModeBtn, &QToolButton::clicked, this, &MainWindow::switchToDesignWorkspace);
    connect(m_codeModeBtn, &QToolButton::clicked, this, &MainWindow::switchToCodeWorkspace);

    toolbar->addWidget(switcherWidget);
    toolbar->addSeparator();

    toolbar->addAction("Undo", this, [this]() {
        if (m_undoStack && m_undoStack->canUndo()) m_undoStack->undo();
    });
    toolbar->addAction("Redo", this, [this]() {
        if (m_undoStack && m_undoStack->canRedo()) m_undoStack->redo();
    });
    toolbar->addSeparator();

    toolbar->addAction("▶ Run Simulator", this, &MainWindow::onRunSimulator);
    toolbar->addSeparator();

    toolbar->addAction("Export...", this, &MainWindow::onExport);
    toolbar->addSeparator();

    QLabel* resLabel = new QLabel("Target Display:", this);
    resLabel->setStyleSheet(QString("color: %1; font-size: 11.5px; font-weight: 500; margin-left: 2px; margin-right: 2px;")
        .arg(Theme::TextMuted));
    toolbar->addWidget(resLabel);

    m_resolutionCombo = new QComboBox(this);
    m_resolutionCombo->addItem("320 × 240 (QVGA - STM32/ESP32)", QSize(320, 240));
    m_resolutionCombo->addItem("480 × 320 (HVGA - 3.5\" TFT)", QSize(480, 320));
    m_resolutionCombo->addItem("800 × 480 (WVGA - 7\" Industrial HMI)", QSize(800, 480));
    m_resolutionCombo->addItem("240 × 240 (Round/Square Display)", QSize(240, 240));
    m_resolutionCombo->addItem("128 × 64 (OLED Monolithic)", QSize(128, 64));
    m_resolutionCombo->setStyleSheet(QString(
        "QComboBox { "
        "  background: %1; "
        "  color: %2; "
        "  border: 1px solid %3; "
        "  border-radius: %4px; "
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
        "  background-color: %5; "
        "  color: %2; "
        "  border: 1px solid %6; "
        "  selection-background-color: %7; "
        "  selection-color: %8; "
        "}"
    ).arg(Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle, QString::number(Theme::RadiusMd),
          Theme::BgSurface, Theme::BorderStrong, Theme::AccentSoft, Theme::AccentText));
    toolbar->addWidget(m_resolutionCombo);
    connect(m_resolutionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onResolutionPresetChanged);

    toolbar->addSeparator();
    QAction* zoomOutAction = toolbar->addAction("−", m_view, &CanvasView::zoomOut);
    zoomOutAction->setToolTip("Zoom out");
    QAction* zoomInAction = toolbar->addAction("+", m_view, &CanvasView::zoomIn);
    zoomInAction->setToolTip("Zoom in");
    toolbar->addAction("100%", m_view, &CanvasView::resetZoom);
    toolbar->addSeparator();
    toolbar->addAction("Change Board...", this, &MainWindow::onChangeBoard);

    // ============ NEW: LIVE TEST MODE BUTTON ============

    // Create the Live Test button
    m_liveTestButton = new QToolButton(this);
    m_liveTestButton->setText("● Live Test");
    m_liveTestButton->setCheckable(true);        // Makes it a toggle button
    m_liveTestButton->setChecked(false);         // Start OFF
    m_liveTestButton->setToolTip("Enable Live Test Mode (Ctrl+L)");
    m_liveTestButton->setObjectName("liveTestButton");

    // Add button to toolbar
    m_toolbar->addSeparator();                   // Visual separator
    m_toolbar->addWidget(m_liveTestButton);

    // Connect button click to slot
    connect(m_liveTestButton, &QToolButton::toggled,
            this, &MainWindow::toggleLiveTestMode);

    // Add keyboard shortcut (Ctrl+L)
    QAction* liveTestAction = new QAction(this);
    liveTestAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_L));
    addAction(liveTestAction);

    connect(liveTestAction, &QAction::triggered, this, [this]() {
        m_liveTestButton->setChecked(!m_liveTestButton->isChecked());
    });

    // ============ END: LIVE TEST MODE BUTTON ============
}

void MainWindow::setDockCollapsed(QDockWidget* dock, bool collapsed, QPushButton* minBtn) {
    if (!dock) return;
    QWidget* content = dock->widget();
    QWidget* titleBar = dock->titleBarWidget();
    if (!minBtn && titleBar) {
        minBtn = titleBar->findChild<QPushButton*>("dockMinButton");
    }

    int titleH = (titleBar && titleBar->sizeHint().height() > 0) ? titleBar->sizeHint().height() : 32;
    if (titleH > 32) titleH = 32;

    // Cache original constraints if not yet cached
    if (!dock->property("origConstraintsCached").toBool()) {
        dock->setProperty("origMinWidth", dock->minimumWidth());
        dock->setProperty("origMaxWidth", dock->maximumWidth());
        dock->setProperty("origMinHeight", dock->minimumHeight());
        dock->setProperty("origMaxHeight", dock->maximumHeight());
        dock->setProperty("origConstraintsCached", true);
    }

    int origMinW = dock->property("origMinWidth").toInt();
    int origMaxW = dock->property("origMaxWidth").toInt();
    int origMinH = dock->property("origMinHeight").toInt();
    int origMaxH = dock->property("origMaxHeight").toInt();

    dock->setProperty("isCollapsed", collapsed);

    if (collapsed) {
        if (content) content->setVisible(false);
        dock->setMinimumHeight(titleH);
        dock->setMaximumHeight(titleH);
        if (minBtn) {
            minBtn->setText("+");
            minBtn->setToolTip("Restore Panel");
        }
    } else {
        if (content) content->setVisible(true);
        dock->setMinimumHeight(origMinH > 0 ? origMinH : 0);
        dock->setMaximumHeight(origMaxH > 0 ? origMaxH : QWIDGETSIZE_MAX);
        dock->setMinimumWidth(origMinW > 0 ? origMinW : 0);
        dock->setMaximumWidth(origMaxW > 0 ? origMaxW : QWIDGETSIZE_MAX);
        if (minBtn) {
            minBtn->setText("—");
            minBtn->setToolTip("Minimize Panel");
        }
    }

    QString dockId = !dock->objectName().isEmpty() ? dock->objectName() : dock->windowTitle().remove(' ');
    QSettings settings("EmbeddedUIDesigner", "EmbeddedUIDesigner");
    settings.setValue(QString("dock/%1/collapsed").arg(dockId), collapsed);
}

bool MainWindow::isDockCollapsed(QDockWidget* dock) {
    return dock ? dock->property("isCollapsed").toBool() : false;
}

static QWidget* createDockTitleBar(const QString& titleText, QDockWidget* dock) {
    QWidget* bar = new QWidget(dock);
    bar->setObjectName("customDockTitle");
    bar->setFixedHeight(32);
    QHBoxLayout* l = new QHBoxLayout(bar);
    l->setContentsMargins(12, 4, 12, 4);
    l->setSpacing(8);

    QLabel* label = new QLabel(titleText, bar);
    label->setFont(DesignSystem::panelTitleFont());
    label->setObjectName("panelTitle");
    l->addWidget(label);
    l->addStretch(1);

    QPushButton* minBtn = new QPushButton("—", bar);
    minBtn->setFixedSize(22, 22);
    minBtn->setObjectName("dockMinButton");
    minBtn->setToolTip("Minimize Panel");
    minBtn->setCursor(Qt::PointingHandCursor);
    minBtn->setStyleSheet(QString(
        "QPushButton#dockMinButton { "
        "  background: transparent; "
        "  border: none; "
        "  color: %1; "
        "  font-size: 12px; "
        "  font-weight: bold; "
        "  border-radius: 3px; "
        "} "
        "QPushButton#dockMinButton:hover { "
        "  background-color: %2; "
        "  color: %3; "
        "}"
    ).arg(Theme::TextMuted, Theme::BgRaised, Theme::TextPrimary));
    l->addWidget(minBtn);

    QObject::connect(minBtn, &QPushButton::clicked, dock, [dock, minBtn]() {
        bool currentlyCollapsed = dock->property("isCollapsed").toBool();
        MainWindow::setDockCollapsed(dock, !currentlyCollapsed, minBtn);
    });

    QPushButton* closeBtn = new QPushButton("✕", bar);
    closeBtn->setFixedSize(22, 22);
    closeBtn->setObjectName("dockCloseButton");
    closeBtn->setToolTip("Close Panel");
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(QString(
        "QPushButton#dockCloseButton { "
        "  background: transparent; "
        "  border: none; "
        "  color: %1; "
        "  font-size: 11px; "
        "  border-radius: 3px; "
        "} "
        "QPushButton#dockCloseButton:hover { "
        "  background-color: rgba(235, 87, 87, 0.2); "
        "  color: #ff6b6b; "
        "}"
    ).arg(Theme::TextMuted));
    QObject::connect(closeBtn, &QPushButton::clicked, dock, &QDockWidget::close);
    l->addWidget(closeBtn);

    bar->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(bar, &QWidget::customContextMenuRequested, dock, [dock, bar](const QPoint& pos) {
        QMenu menu(bar);
        bool isFloating = dock->isFloating();
        QAction* detachAct = menu.addAction(isFloating ? "Dock panel" : "Detach panel");
        QObject::connect(detachAct, &QAction::triggered, dock, [dock, isFloating]() {
            dock->setFloating(!isFloating);
            if (!isFloating) {
                dock->raise();
                dock->activateWindow();
            }
        });

        QMenu* monitorSubMenu = menu.addMenu("Move to Monitor");
        const auto screens = QGuiApplication::screens();
        for (int i = 0; i < screens.size(); ++i) {
            QString name = QString("Monitor %1: %2x%3")
                .arg(i + 1)
                .arg(screens[i]->geometry().width())
                .arg(screens[i]->geometry().height());
            if (screens[i] == QGuiApplication::primaryScreen()) {
                name += " (Primary)";
            }
            QAction* scrAct = monitorSubMenu->addAction(name);
            QObject::connect(scrAct, &QAction::triggered, dock, [dock, scr = screens[i]]() {
                if (!dock->isFloating()) {
                    dock->setFloating(true);
                }
                QRect avail = scr->availableGeometry();
                QSize sz = dock->size();
                int x = avail.x() + qMax(0, (avail.width() - sz.width()) / 2);
                int y = avail.y() + qMax(0, (avail.height() - sz.height()) / 2);
                dock->move(x, y);
                dock->raise();
                dock->activateWindow();
            });
        }

        menu.addSeparator();
        bool currentlyCollapsed = dock->property("isCollapsed").toBool();
        QAction* minAct = menu.addAction(currentlyCollapsed ? "Restore panel" : "Minimize panel");
        QObject::connect(minAct, &QAction::triggered, dock, [dock]() {
            bool col = dock->property("isCollapsed").toBool();
            MainWindow::setDockCollapsed(dock, !col);
        });

        menu.exec(bar->mapToGlobal(pos));
    });

    return bar;
}

void MainWindow::setupDocks() {
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks);

    auto configureDock = [](QDockWidget* dock, const QString& objName, int minW, int maxW, int minH, int maxH) {
        dock->setObjectName(objName);
        dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
        if (minW > 0) dock->setMinimumWidth(minW);
        if (maxW > 0) dock->setMaximumWidth(maxW);
        if (minH > 0) dock->setMinimumHeight(minH);
        if (maxH > 0) dock->setMaximumHeight(maxH);
        dock->setProperty("origMinWidth", minW);
        dock->setProperty("origMaxWidth", maxW);
        dock->setProperty("origMinHeight", minH);
        dock->setProperty("origMaxHeight", maxH);
        dock->setProperty("origConstraintsCached", true);
    };

    // Left Dock: Components Palette
    QDockWidget* paletteDock = new QDockWidget("Components", this);
    m_paletteDock = paletteDock;
    configureDock(paletteDock, "paletteDock", 180, 500, 0, QWIDGETSIZE_MAX);
    paletteDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    paletteDock->setTitleBarWidget(createDockTitleBar("Components", paletteDock));
    m_palette = new ComponentPalette(paletteDock);
    paletteDock->setWidget(m_palette);
    addDockWidget(Qt::LeftDockWidgetArea, paletteDock);

    // Left Dock: Screens Panel
    QDockWidget* screensDock = new QDockWidget("Screens", this);
    m_screensDock = screensDock;
    configureDock(screensDock, "screensDock", 180, 500, 0, QWIDGETSIZE_MAX);
    screensDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    screensDock->setTitleBarWidget(createDockTitleBar("Screens", screensDock));
    m_screensPanel = new ScreensPanel(m_project, screensDock);
    m_screensPanel->setUndoStack(m_undoStack);
    screensDock->setWidget(m_screensPanel);
    addDockWidget(Qt::LeftDockWidgetArea, screensDock);
    tabifyDockWidget(paletteDock, screensDock);
    paletteDock->raise();


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

    // Left Dock: Uploads & Assets Panel
    m_uploadsDock = new QDockWidget("Uploads", this);
    configureDock(m_uploadsDock, "uploadsDock", 180, 500, 0, QWIDGETSIZE_MAX);
    m_uploadsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_uploadsDock->setTitleBarWidget(createDockTitleBar("Uploads & Assets", m_uploadsDock));
    m_uploadsPanel = new UploadsPanel(m_scene, m_undoStack, m_project, m_uploadsDock);
    m_uploadsDock->setWidget(m_uploadsPanel);
    addDockWidget(Qt::LeftDockWidgetArea, m_uploadsDock);
    tabifyDockWidget(screensDock, m_uploadsDock);

    // Right Top Dock: Properties Panel
    QDockWidget* propDock = new QDockWidget("Properties", this);
    m_propDock = propDock;
    configureDock(propDock, "propertiesDock", 220, 600, 0, QWIDGETSIZE_MAX);
    propDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    propDock->setTitleBarWidget(createDockTitleBar("Properties", propDock));
    m_propertiesPanel = new PropertiesPanel(propDock);
    m_propertiesPanel->setUndoStack(m_undoStack);
    m_propertiesPanel->setProject(m_project);
    propDock->setWidget(m_propertiesPanel);
    addDockWidget(Qt::RightDockWidgetArea, propDock);

    // Right Top Dock: Effects Panel (Canva-inspired)
    m_effectsDock = new QDockWidget("Effects", this);
    configureDock(m_effectsDock, "effectsDock", 220, 600, 0, QWIDGETSIZE_MAX);
    m_effectsDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    m_effectsDock->setTitleBarWidget(createDockTitleBar("Effects & Adjustments", m_effectsDock));
    m_effectsPanel = new EffectsPanel(m_scene, m_undoStack, m_effectsDock);
    m_effectsDock->setWidget(m_effectsPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_effectsDock);
    tabifyDockWidget(propDock, m_effectsDock);

    QDockWidget* prototypeDock = new QDockWidget("Prototype", this);
    m_prototypeDock = prototypeDock;
    configureDock(prototypeDock, "prototypeDock", 220, 600, 0, QWIDGETSIZE_MAX);
    prototypeDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    prototypeDock->setTitleBarWidget(createDockTitleBar("Prototype", prototypeDock));
    m_prototypePanel = new PrototypePanel(m_scene, prototypeDock);
    prototypeDock->setWidget(m_prototypePanel);
    addDockWidget(Qt::RightDockWidgetArea, prototypeDock);
    tabifyDockWidget(m_effectsDock, prototypeDock);
    propDock->raise();

    // Right Bottom Dock: Layer Panel
    QDockWidget* layerDock = new QDockWidget("Layers", this);
    m_layerDock = layerDock;
    configureDock(layerDock, "layerDock", 200, 600, 0, QWIDGETSIZE_MAX);
    layerDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    layerDock->setTitleBarWidget(createDockTitleBar("Layers", layerDock));
    m_layerPanel = new LayerPanel(m_scene, layerDock);
    layerDock->setWidget(m_layerPanel);
    addDockWidget(Qt::RightDockWidgetArea, layerDock);

    // Right Dock: Named Color Styles (tabified with Layers)
    QDockWidget* stylesDock = new QDockWidget("Color Styles", this);
    m_stylesDock = stylesDock;
    configureDock(stylesDock, "stylesDock", 200, 600, 0, QWIDGETSIZE_MAX);
    stylesDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    stylesDock->setTitleBarWidget(createDockTitleBar("Color Styles", stylesDock));
    m_stylesPanel = new StylesPanel(m_project, stylesDock);
    stylesDock->setWidget(m_stylesPanel);
    addDockWidget(Qt::RightDockWidgetArea, stylesDock);
    tabifyDockWidget(layerDock, stylesDock);

    // Right Dock: AI Assistant Panel
    m_aiDockWidget = new QDockWidget("AI Assistant", this);
    configureDock(m_aiDockWidget, "aiDockWidget", 240, 700, 0, QWIDGETSIZE_MAX);
    m_aiDockWidget->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    m_aiDockWidget->setTitleBarWidget(createDockTitleBar("AI Assistant", m_aiDockWidget));
    m_aiAssistantPanel = new AI::AIAssistantPanel(m_controller, m_project, m_undoStack, m_aiDockWidget);
    m_aiDockWidget->setWidget(m_aiAssistantPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_aiDockWidget);
    tabifyDockWidget(stylesDock, m_aiDockWidget);

    // Right Dock: Resource Analyzer Panel (Phase 2.1)
    m_resourceDock = new QDockWidget("Resource Analyzer", this);
    m_resourceAnalyzerPanel = m_resourceDock;
    configureDock(m_resourceDock, "resourceDock", 220, 600, 0, QWIDGETSIZE_MAX);
    m_resourceDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    m_resourceDock->setTitleBarWidget(createDockTitleBar("Resource Analyzer", m_resourceDock));
    m_resourcePanel = new ResourceAnalyzerPanel(m_resourceDock);
    m_resourcePanel->setProject(m_project);
    m_resourceDock->setWidget(m_resourcePanel);
    addDockWidget(Qt::RightDockWidgetArea, m_resourceDock);
    tabifyDockWidget(m_aiDockWidget, m_resourceDock);

    // Right Dock: Board & Display (Task 5), tabified with the Resource Analyzer.
    m_boardDisplayDock = new QDockWidget("Board & Display", this);
    configureDock(m_boardDisplayDock, "boardDisplayDock", 240, 650, 0, QWIDGETSIZE_MAX);
    m_boardDisplayDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    m_boardDisplayDock->setTitleBarWidget(createDockTitleBar("Board & Display", m_boardDisplayDock));
    m_boardDisplayPanel = new BoardDisplayPanel(m_boardDisplayDock);
    m_boardDisplayPanel->setProject(m_project);
    m_boardDisplayDock->setWidget(m_boardDisplayPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_boardDisplayDock);
    tabifyDockWidget(m_resourceDock, m_boardDisplayDock);

    // Bottom Dock: Build & Compiler Output
    m_buildDock = new QDockWidget("Build Output", this);
    configureDock(m_buildDock, "buildDock", 200, QWIDGETSIZE_MAX, 100, 600);
    m_buildDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);
    m_buildDock->setTitleBarWidget(createDockTitleBar("Build Output", m_buildDock));
    m_buildOutputPanel = new BuildOutputPanel(m_project, m_buildDock);
    m_buildDock->setWidget(m_buildOutputPanel);
    addDockWidget(Qt::BottomDockWidgetArea, m_buildDock);
    m_buildDock->hide();

    // Bottom Dock: Device Monitor (Live Telemetry)
    m_deviceMonitorDock = new QDockWidget("Device Monitor", this);
    configureDock(m_deviceMonitorDock, "deviceMonitorDock", 200, QWIDGETSIZE_MAX, 100, 600);
    m_deviceMonitorDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);
    m_deviceMonitorDock->setTitleBarWidget(createDockTitleBar("Device Monitor", m_deviceMonitorDock));
    m_deviceMonitorPanel = new DeviceMonitorPanel(m_project, m_deviceMonitorDock);
    m_deviceMonitor = m_deviceMonitorPanel;
    m_deviceMonitorDock->setWidget(m_deviceMonitorPanel);
    addDockWidget(Qt::BottomDockWidgetArea, m_deviceMonitorDock);
    tabifyDockWidget(m_buildDock, m_deviceMonitorDock);
    m_deviceMonitorDock->hide();

    // Bottom Dock: Problems (project diagnostics)
    m_problemsDock = new QDockWidget("Problems", this);
    configureDock(m_problemsDock, "problemsDock", 200, 750, 100, 600);
    m_problemsDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::RightDockWidgetArea);
    m_problemsDock->setTitleBarWidget(createDockTitleBar("Problems", m_problemsDock));
    m_problemsPanel = new ProblemsPanel(m_diagnosticsService, m_problemsDock);
    m_problemsDock->setWidget(m_problemsPanel);
    addDockWidget(Qt::BottomDockWidgetArea, m_problemsDock);
    tabifyDockWidget(m_deviceMonitorDock, m_problemsDock);
    m_problemsDock->hide();

    if (m_actProblems && m_problemsDock) {
        connect(m_problemsDock, &QDockWidget::visibilityChanged, m_actProblems, &QAction::setChecked);
        m_actProblems->setChecked(m_problemsDock->isVisible());
    }

    if (m_actResourceAnalyzer && m_resourceDock) {
        connect(m_resourceDock, &QDockWidget::visibilityChanged, m_actResourceAnalyzer, &QAction::setChecked);
        m_actResourceAnalyzer->setChecked(m_resourceDock->isVisible());
    }
    if (m_actBoardDisplay && m_boardDisplayDock) {
        connect(m_boardDisplayDock, &QDockWidget::visibilityChanged, m_actBoardDisplay, &QAction::setChecked);
        m_actBoardDisplay->setChecked(m_boardDisplayDock->isVisible());
    }
    if (m_actBuildOutput && m_buildDock) {
        connect(m_buildDock, &QDockWidget::visibilityChanged, m_actBuildOutput, &QAction::setChecked);
        m_actBuildOutput->setChecked(m_buildDock->isVisible());
    }
    if (m_actDeviceMonitor && m_deviceMonitorDock) {
        connect(m_deviceMonitorDock, &QDockWidget::visibilityChanged, m_actDeviceMonitor, &QAction::setChecked);
        m_actDeviceMonitor->setChecked(m_deviceMonitorDock->isVisible());
    }

    resizeDocks({paletteDock, screensDock, m_uploadsDock}, {210, 210, 210}, Qt::Horizontal);
    resizeDocks({propDock, m_effectsDock, prototypeDock, layerDock, stylesDock, m_aiDockWidget, m_resourceDock}, {260, 260, 260, 260, 260, 260, 260}, Qt::Horizontal);
    resizeDocks({propDock, layerDock}, {550, 330}, Qt::Vertical);

    if (m_workspaceManager) {
        m_workspaceManager->registerDesignDock("palette", m_paletteDock);
        m_workspaceManager->registerDesignDock("screens", m_screensDock);
        m_workspaceManager->registerDesignDock("uploads", m_uploadsDock);
        m_workspaceManager->registerDesignDock("prop", m_propDock);
        m_workspaceManager->registerDesignDock("effects", m_effectsDock);
        m_workspaceManager->registerDesignDock("prototype", m_prototypeDock);
        m_workspaceManager->registerDesignDock("layer", m_layerDock);
        m_workspaceManager->registerDesignDock("styles", m_stylesDock);
        m_workspaceManager->registerDesignDock("ai", m_aiDockWidget);
        m_workspaceManager->registerDesignDock("resource", m_resourceDock);
        m_workspaceManager->registerDesignDock("boardDisplay", m_boardDisplayDock);
        m_workspaceManager->registerDesignDock("mcu", m_mcuDock);
        m_workspaceManager->registerDesignDock("build", m_buildDock);
        m_workspaceManager->registerDesignDock("deviceMonitor", m_deviceMonitorDock);
    }

    // Auto-detect and initialize board after OpenOCD connects
    if (m_openocd.isConnected()) {
        if (!m_openocd.autoDetectAndInitializeBoard()) {
            BoardPreset active = BoardRegistry::instance().activePreset();
            QString fallbackBoard = !active.boardId.isEmpty() ? active.boardId : "STM32F030R8";
            qWarning() << "Board auto-detection failed. Falling back to" << fallbackBoard;
            m_openocd.setBoard(fallbackBoard);
        }
    }

    setupMCUSelection();

    m_defaultLayoutState = saveState();
    restoreWindowLayout();
}

void MainWindow::setupDiagnostics() {
    if (!m_diagnosticsService || !m_project) return;

    // Debounced refresh on any project state change so the Problems panel
    // reflects the live design, not just the last export.
    const auto scheduleRefresh = [this]() {
        if (m_diagnosticsService) m_diagnosticsService->scheduleRefresh();
    };
    connect(m_project, &Project::projectModified, this, scheduleRefresh);
    connect(m_project, &Project::projectLoaded, this, scheduleRefresh);
    connect(m_project, &Project::screenListChanged, this, scheduleRefresh);
    connect(m_project, &Project::dataSourcesChanged, this, scheduleRefresh);
    connect(m_project, &Project::dataBindingsChanged, this, scheduleRefresh);
    connect(m_project, &Project::hardwareConfigChanged, this, scheduleRefresh);
    connect(m_scene, &CanvasScene::componentAdded, this, scheduleRefresh);
    connect(m_scene, &CanvasScene::componentRemoved, this, scheduleRefresh);
    connect(m_scene, &CanvasScene::componentChanged, this, scheduleRefresh);

    if (m_problemsPanel) {
        connect(m_problemsPanel, &ProblemsPanel::navigateRequested,
                this, &MainWindow::navigateToDiagnostic);
    }

    // Status-bar count badge, e.g. "2 errors, 5 warnings".
    if (statusBar()) {
        m_problemsBadgeLabel = new QLabel(this);
        m_problemsBadgeLabel->setObjectName("problemsBadgeLabel");
        m_problemsBadgeLabel->setToolTip("Problems detected in the current project");
        statusBar()->addPermanentWidget(m_problemsBadgeLabel);
    }

    connect(m_diagnosticsService, &Diagnostics::DiagnosticsService::diagnosticsChanged,
            this, [this](const QVector<Diagnostics::Diagnostic>&) { updateProblemsBadge(); });

    updateProblemsBadge();
    m_diagnosticsService->scheduleRefresh(0);
}

void MainWindow::updateProblemsBadge() {
    if (!m_problemsBadgeLabel || !m_diagnosticsService) return;

    const int errors = m_diagnosticsService->errorCount();
    const int warnings = m_diagnosticsService->warningCount();
    const int info = m_diagnosticsService->infoCount();

    QString text = QString("%1 error%2, %3 warning%4")
                       .arg(errors)
                       .arg(errors == 1 ? "" : "s")
                       .arg(warnings)
                       .arg(warnings == 1 ? "" : "s");
    if (info > 0) {
        text += QString(", %1 info").arg(info);
    }

    QString color = Theme::TextMuted;
    if (errors > 0) color = Theme::Danger;
    else if (warnings > 0) color = Theme::Warning;
    else if (info > 0) color = Theme::Info;

    m_problemsBadgeLabel->setText(text);
    m_problemsBadgeLabel->setStyleSheet(QString("color: %1; padding-right: 12px;").arg(color));
}

void MainWindow::navigateToDiagnostic(const QString& screenId, const QString& componentId) {
    if (!m_project) return;

    Screen* target = screenId.isEmpty() ? nullptr : m_project->findScreen(screenId);
    UIComponent* component = nullptr;

    if (!componentId.isEmpty()) {
        if (target) component = target->findComponentById(componentId);
        if (!component) {
            for (Screen* screen : m_project->screens()) {
                if (!screen) continue;
                component = screen->findComponentById(componentId);
                if (component) {
                    target = screen;
                    break;
                }
            }
        }
    }

    // Switch to the owning screen first so the component is attached to the scene.
    if (target && target != m_project->activeScreen()) {
        m_project->setActiveScreen(target);
    }

    if (component && m_canvasView) {
        m_canvasView->revealComponent(component);
    }
}

void MainWindow::applyTheme() {
    setStyleSheet(Theme::stylesheet());
    if (qApp) {
        qApp->setStyleSheet(Theme::stylesheet());
        qApp->setFont(DesignSystem::bodyFont());
    }
}

void MainWindow::setTheme(const QString& theme) {
    StyleManager::setTheme(theme);
    applyTheme();
}

void MainWindow::updateWindowTitle() {
    QString screenName = (m_project && m_project->activeScreen()) ? m_project->activeScreen()->name() : "";
    QString title = "Embedded UI Designer - " + m_project->projectName();
    if (!screenName.isEmpty()) {
        title += " [" + screenName + "]";
    }
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
        QString tpl = dialog.templateResource();
        if (!tpl.isEmpty()) {
            m_project->loadFromFile(tpl);
            m_project->setProjectName(dialog.projectName());

            Hardware::HardwareConfig hw = dialog.hardwareConfiguration();
            if (hw.isValid()) {
                m_project->setHardwareConfig(hw);
                if (!hw.boardId.isEmpty()) {
                    HardwareBridge::instance().setBoard(hw.boardId);
                } else if (!hw.deviceId.isEmpty()) {
                    HardwareBridge::instance().setBoard(hw.deviceId.toLower());
                }
                Hardware::HardwareManager::instance().setTarget(dialog.hardwareTarget());
            }
        }
    } else {
        m_project->newProject(dialog.projectName(), dialog.projectWidth(), dialog.projectHeight());
        DisplayConfig config = m_project->displayConfig();
        config.round = dialog.roundDisplay();
        config.colorDepth = dialog.colorDepth();
        m_project->setDisplayConfig(config);
        m_project->setTargetFramework(dialog.targetFramework());

        Hardware::HardwareConfig hw = dialog.hardwareConfiguration();
        m_project->setHardwareConfig(hw);

        if (!hw.boardId.isEmpty()) {
            HardwareBridge::instance().setBoard(hw.boardId);
        } else if (!hw.deviceId.isEmpty()) {
            HardwareBridge::instance().setBoard(hw.deviceId.toLower());
        }
        Hardware::HardwareManager::instance().setTarget(dialog.hardwareTarget());
    }

    if (m_undoStack) m_undoStack->clear();
    m_resolutionCombo->blockSignals(true);
    m_resolutionCombo->setCurrentIndex(-1);
    m_resolutionCombo->blockSignals(false);
    statusBar()->showMessage(QString("Embedded project '%1' created for %2")
        .arg(dialog.projectName(), dialog.hardwareBoard()), 3000);
}

void MainWindow::onCreateEmbeddedProject() {
    HardwareWizardDialog dialog(this);
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

    m_project->newProject(dialog.projectName(), dialog.displayWidth(), dialog.displayHeight());
    DisplayConfig config = m_project->displayConfig();
    config.round = dialog.isRound();
    config.colorDepth = dialog.colorDepth();
    m_project->setDisplayConfig(config);

    Hardware::HardwareConfig hw = dialog.hardwareConfiguration();
    m_project->setHardwareConfig(hw);
    m_project->setTargetFramework(hw.toolchain.framework);

    // Sync HardwareBridge to newly configured hardware
    if (!hw.deviceId.isEmpty()) {
        HardwareBridge::instance().setBoard(hw.boardId.isEmpty() ? hw.deviceId.toLower() : hw.boardId.toLower());
    }

    // Sync HardwareManager target
    Hardware::HardwareTarget target(hw.boardId.isEmpty() ? hw.deviceId : hw.boardId,
                                    hw.boardId.isEmpty() ? hw.deviceId : hw.boardId,
                                    hw.family.isEmpty() ? "STM32" : hw.family,
                                    hw.deviceId, hw.boardId);
    target.setArchitecture(hw.architecture);
    Hardware::HardwareManager::instance().setTarget(target);

    if (m_undoStack) m_undoStack->clear();
    m_resolutionCombo->blockSignals(true);
    m_resolutionCombo->setCurrentIndex(-1);
    m_resolutionCombo->blockSignals(false);
    statusBar()->showMessage(QString("Embedded project '%1' created for %2")
        .arg(dialog.projectName(), (hw.targetType == "board") ? hw.boardId : hw.deviceId), 3000);
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

void MainWindow::onOpenFirmwareFolder() {
    QString current = m_project ? m_project->firmwareDirectory() : QDir::homePath();
    QString dir = QFileDialog::getExistingDirectory(this, "Select Firmware Project Directory", current);
    if (!dir.isEmpty() && m_project) {
        m_project->setFirmwareDirectory(dir);
        switchToCodeWorkspace();
        statusBar()->showMessage(QString("Firmware project folder opened: %1").arg(dir), 4000);
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

void MainWindow::onImportHtmlCss() {
    QString filter = "HTML Files (*.html *.htm);;All Files (*)";
    QString filePath = QFileDialog::getOpenFileName(this, "Import HTML/CSS Design", QString(), filter);
    if (filePath.isEmpty()) return;

    Importers::ImportResult result = Importers::HtmlCssImporter::importHtml(filePath);
    if (!result.success) {
        QMessageBox::critical(this, "HTML Import Failed", "Failed to import HTML file:\n" + result.errors.join("\n"));
        return;
    }

    if (!result.warnings.isEmpty()) {
        statusBar()->showMessage(QString("HTML Import warning: %1").arg(result.warnings.first()), 5000);
    }

    auto importedProj = IR::IRToProjectAdapter::convert(result.ir, nullptr);

    QSet<QString> existingIds;
    for (UIComponent* c : m_scene->uiComponents()) {
        existingIds.insert(c->componentId());
    }

    int importedCount = 0;
    Screen* impScreen = importedProj ? importedProj->activeScreen() : nullptr;
    Screen* targetScreen = m_project->activeScreen();

    if (impScreen && targetScreen) {
        for (UIComponent* comp : impScreen->components()) {
            impScreen->removeComponent(comp);
            QString originalId = comp->componentId();
            QString uniqueId = originalId;
            int suffix = 2;
            while (existingIds.contains(uniqueId)) {
                uniqueId = QString("%1_%2").arg(originalId).arg(suffix++);
            }
            comp->setComponentId(uniqueId);
            existingIds.insert(uniqueId);
            comp->setScreenId(targetScreen->id());
            m_scene->addUIComponent(comp);
            targetScreen->addComponent(comp);
            importedCount++;
        }
    }

    if (m_layerPanel) m_layerPanel->refreshLayers();
    if (m_resourcePanel) m_resourcePanel->refresh();

    statusBar()->showMessage(QString("Successfully imported %1 component(s) from %2")
                                 .arg(importedCount)
                                 .arg(QFileInfo(filePath).fileName()), 5000);
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
    ExportDialog dlg(m_project, this);
    dlg.exec();
    // Task 6: the export dialog can change the target backend (single source of
    // truth in the Project); reflect that immediately in analyzer + Problems.
    if (m_resourcePanel) m_resourcePanel->refresh();
    if (m_diagnosticsService) m_diagnosticsService->scheduleRefresh(0);
}

bool MainWindow::validateExportResources(const QString& targetFramework) {
    if (!m_project) return true;

    // Task 6: backend compatibility findings for the dedicated menu exports.
    // Errors block outright; warnings join the resource confirmation below so
    // the user confirms all export-affecting warnings exactly once.
    const BackendDescriptor backend = BackendRegistry::backendForFramework(targetFramework);
    QStringList backendErrors;
    QStringList backendWarnings;
    for (const BackendFinding& finding : BackendRegistry::validate(m_project, backend.id)) {
        if (finding.severity == BackendFindingSeverity::Error) {
            if (!backendErrors.contains(finding.message)) backendErrors.append(finding.message);
        } else if (finding.severity == BackendFindingSeverity::Warning) {
            if (!backendWarnings.contains(finding.message)) backendWarnings.append(finding.message);
        }
    }
    if (!backendErrors.isEmpty()) {
        QMessageBox::critical(this, "Export Blocked",
            QString("The %1 backend cannot generate this design:\n\n• %2\n\n"
                    "Fix the reported components or choose a different target backend.")
                .arg(backend.displayName, backendErrors.join("\n• ")));
        return false;
    }

    ResourceEstimator estimator;
    DeviceProfile dev = m_project->getDeviceProfile();
    const bool hasDevice = !(dev.name.isEmpty() || (dev.flash_kb == 0 && dev.ram_kb == 0));

    ResourceAnalysis analysis{};
    bool resourceWarning = false;
    if (hasDevice) {
        analysis = estimator.analyze(m_project, dev, targetFramework);

        if (analysis.status == ResourceAnalysis::CRITICAL) {
            QMessageBox::critical(this, "Export Blocked", "Design exceeds device limits. Optimize or select a larger MCU.");
            return false;
        }
        resourceWarning = (analysis.status == ResourceAnalysis::WARNING ||
                           analysis.status == ResourceAnalysis::MISMATCH ||
                           analysis.flash_percent >= 80.0f || analysis.ram_percent >= 80.0f);
    }

    if (resourceWarning || !backendWarnings.isEmpty()) {
        QStringList sections;
        if (resourceWarning) {
            sections << QString("Estimated resource utilization is high (%1% Flash, %2% RAM).\n\n"
                                "Exporting may lead to runtime memory constraints or allocation failures on hardware.")
                            .arg(analysis.flash_percent, 0, 'f', 1)
                            .arg(analysis.ram_percent, 0, 'f', 1);
        }
        if (!backendWarnings.isEmpty()) {
            sections << QString("The %1 backend cannot fully generate every component:\n\n• %2\n\n"
                                "Exporting will produce output that differs from the design.")
                            .arg(backend.displayName, backendWarnings.join("\n• "));
        }
        auto res = QMessageBox::warning(this,
            resourceWarning ? "Estimated Resource Warning" : "Backend Compatibility Warning",
            sections.join("\n\n") + "\n\nDo you want to proceed with firmware export anyway?",
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (res != QMessageBox::Yes) {
            return false;
        }
    }
    return true;
}

void MainWindow::onExportUgfx() {
    if (!validateExportResources("ugfx")) return;

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
    if (!validateExportResources("qt-for-mcus")) return;

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
    if (!validateExportResources("lvgl")) return;

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

void MainWindow::onOpenBoardSelectorDialog() {
    BoardSelectorDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        BoardSpec board = dlg.selectedBoard();
        if (board.isValid()) {
            qDebug() << "[MainWindow] Active target board switched to:" << board.name;
            statusBar()->showMessage(QString("Target Board: %1 (%2)").arg(board.name, board.mcu), 4000);
            if (m_connectionStatusLabel) {
                m_connectionStatusLabel->setText(QString("Board: %1").arg(board.name));
            }
            if (m_project) {
                m_project->setTargetBoard(board.id);
            }
        }
    }
}

void MainWindow::onAISettingsDialog() {
    AISettingsDialog dlg(this);
    dlg.exec();
}

void MainWindow::onGitDialog() {
    QString repoDir = m_project && !m_project->projectFilePath().isEmpty() ?
        QFileInfo(m_project->projectFilePath()).dir().absolutePath() : QDir::currentPath();
    Git::GitDialog dlg(repoDir, this);
    dlg.exec();
}

void MainWindow::onCICDDialog() {
    CICD::CICDDialog dlg(m_project, this);
    dlg.exec();
}

void MainWindow::onDiagnosticsDialog() {
    Diagnostics::DiagnosticsEngine::instance().scanProject(m_project);
    QString report = Diagnostics::DiagnosticsEngine::instance().toMarkdown();
    QDialog dlg(this);
    dlg.setWindowTitle("Project Diagnostics");
    dlg.resize(550, 420);
    auto l = new QVBoxLayout(&dlg);
    auto tb = new QTextBrowser(&dlg);
    tb->setMarkdown(report);
    l->addWidget(tb);
    auto bb = new QDialogButtonBox(QDialogButtonBox::Ok, &dlg);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    l->addWidget(bb);
    dlg.exec();
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

void MainWindow::onRunSimulator() {
    if (!m_simulatorWindow) {
        m_simulatorWindow = new Simulator::SimulatorWindow(m_project, this);
    }
    m_simulatorWindow->show();
    m_simulatorWindow->raise();
    m_simulatorWindow->activateWindow();
}

void MainWindow::createGpioControls() {
    QWidget* gpioWidget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout();

    // Pin selection
    QComboBox* pinCombo = new QComboBox();
    auto updatePins = [pinCombo]() {
        pinCombo->clear();
        QStringList pins = BoardRegistry::instance().activeBoard().pinNames();
        if (pins.isEmpty()) pins = {"PA4", "PA5", "PB0", "PB1", "PA0", "PA1"};
        pinCombo->addItems(pins);
    };
    updatePins();
    connect(&BoardRegistry::instance(), &BoardRegistry::activeBoardChanged, pinCombo, [updatePins]() {
        updatePins();
    });
    layout->addWidget(new QLabel("Pin:"));
    layout->addWidget(pinCombo);

    // Toggle button (interactive in canvas)
    QPushButton* toggleBtn = new QPushButton("Toggle LED");
    toggleBtn->setCheckable(true);

    connect(toggleBtn, &QPushButton::toggled, this, [this, pinCombo, toggleBtn](bool checked) {
        QString cmd = checked ? "LED_ON" : "LED_OFF";

        if (m_openocd.isConnected()) {
            m_openocd.sendCommand(cmd);
            QString response = m_openocd.readResponse();
            qDebug() << "Response:" << response;
        } else {
            QMessageBox::warning(this, "Not Connected", "OpenOCD not connected");
            toggleBtn->setChecked(false);
        }
    });

    layout->addWidget(toggleBtn);
    gpioWidget->setLayout(layout);

    if (m_contextualToolbar) {
        if (auto* bl = qobject_cast<QBoxLayout*>(m_contextualToolbar->layout())) {
            bl->insertWidget(0, gpioWidget);
        }
    }
}

void MainWindow::toggleLiveTestMode(bool enabled) {
    m_liveTestMode = enabled;
    
    if (enabled) {
        // LIVE TEST ON
        m_liveTestButton->setStyleSheet(QString(
            "background-color: %1; color: white; font-weight: bold;"
        ).arg(Theme::LiveTestActive));
        statusBar()->showMessage("✓ Live Test: ON (Click switches to control GPIO)");
        
        // Resource Analyzer stays visible in Live Test (superseded spec: it was hidden).
        if (m_resourceDock) {
            m_resourceDock->show();
            m_resourceDock->raise();
        }
        if (m_resourcePanel) {
            m_resourcePanel->setLiveTestMode(true);
        }
        
        qDebug() << "Live Test Mode: ON";
        qDebug() << "Resource Analyzer: VISIBLE (Live Test banner shown)";
        
    } else {
        // LIVE TEST OFF
        m_liveTestButton->setStyleSheet("");
        statusBar()->showMessage("Design Mode (Resource Analyzer enabled)");
        
        if (m_resourceDock) {
            m_resourceDock->show();
        }
        if (m_resourcePanel) {
            m_resourcePanel->setLiveTestMode(false);
        }
        
        qDebug() << "Live Test Mode: OFF";
        qDebug() << "Resource Analyzer: VISIBLE (Live Test banner hidden)";
    }

    emit liveTestModeChanged(enabled);
}

void MainWindow::setupMCUSelection() {
    // Create MCU selection panel
    QWidget* mcuPanel = new QWidget(this);
    QVBoxLayout* mcuLayout = new QVBoxLayout(mcuPanel);
    mcuLayout->setContentsMargins(12, 12, 12, 12);
    mcuLayout->setSpacing(8);
    
    // Label
    QLabel* mcuLabel = new QLabel("Target Microcontroller:", this);
    mcuLabel->setStyleSheet("font-weight: bold;");
    mcuLayout->addWidget(mcuLabel);
    
    // MCU selection dropdown
    m_mcuSelectComboBox = new QComboBox(this);
    m_mcuSelectComboBox->setObjectName("mcuSelectComboBox");
    m_mcuSelectComboBox->addItems({
        "STM32F030R8 (NUCLEO-F030R8)",
        "ESP32 (Auto-detect)",
        "ESP32-S3",
        "Custom (Browse Config...)"
    });
    m_mcuSelectComboBox->setCurrentIndex(0);  // Default to STM32
    connect(m_mcuSelectComboBox, &QComboBox::currentTextChanged,
            this, &MainWindow::onMCUSelectionChanged);
    mcuLayout->addWidget(m_mcuSelectComboBox);
    
    // Browse button
    m_browseMCUConfigButton = new QPushButton("Browse OpenOCD Config", this);
    m_browseMCUConfigButton->setObjectName("browseMCUConfigButton");
    m_browseMCUConfigButton->setVisible(false);  // Hidden until "Custom" selected
    connect(m_browseMCUConfigButton, &QPushButton::clicked,
            this, &MainWindow::onBrowseMCUConfig);
    mcuLayout->addWidget(m_browseMCUConfigButton);
    
    // Connection status
    m_connectionStatusLabel = new QLabel("Not connected", this);
    m_connectionStatusLabel->setObjectName("connectionStatusLabel");
    m_connectionStatusLabel->setStyleSheet("color: #FF6B6B; font-style: italic;");
    mcuLayout->addWidget(m_connectionStatusLabel);
    
    // Connection progress bar (hidden by default)
    m_connectionProgressBar = new QProgressBar(this);
    m_connectionProgressBar->setObjectName("connectionProgressBar");
    m_connectionProgressBar->setVisible(false);
    m_connectionProgressBar->setRange(0, 100);
    mcuLayout->addWidget(m_connectionProgressBar);
    
    // Connect button
    QPushButton* connectButton = new QPushButton("Connect to Board", this);
    connectButton->setObjectName("connectToBoardButton");
    connectButton->setStyleSheet("background-color: #2ecc71; color: white; font-weight: bold; padding: 8px; border-radius: 4px;");
    connect(connectButton, &QPushButton::clicked,
            this, &MainWindow::onConnectToBoard);
    mcuLayout->addWidget(connectButton);
    
    // Cancel button (hidden by default)
    m_cancelConnectionButton = new QPushButton("Cancel", this);
    m_cancelConnectionButton->setObjectName("cancelConnectionButton");
    m_cancelConnectionButton->setVisible(false);
    m_cancelConnectionButton->setStyleSheet("background-color: #FF6B6B; color: white; padding: 6px; border-radius: 4px;");
    connect(m_cancelConnectionButton, &QPushButton::clicked,
            this, &MainWindow::onCancelOpenOCDConnection);
    mcuLayout->addWidget(m_cancelConnectionButton);
    
    mcuLayout->addStretch();
    
    // Add as dock widget
    m_mcuDock = new QDockWidget("Board Configuration", this);
    m_mcuDock->setObjectName("boardConfigDock");
    m_mcuDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    m_mcuDock->setMinimumWidth(220);
    m_mcuDock->setMaximumWidth(600);
    m_mcuDock->setProperty("origMinWidth", 220);
    m_mcuDock->setProperty("origMaxWidth", 600);
    m_mcuDock->setProperty("origMinHeight", 0);
    m_mcuDock->setProperty("origMaxHeight", QWIDGETSIZE_MAX);
    m_mcuDock->setProperty("origConstraintsCached", true);
    m_mcuDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    m_mcuDock->setTitleBarWidget(createDockTitleBar("Board Configuration", m_mcuDock));
    m_mcuDock->setWidget(mcuPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_mcuDock);
    if (m_resourceDock) {
        tabifyDockWidget(m_resourceDock, m_mcuDock);
    }
}

void MainWindow::onConnectToBoard() {
    qDebug() << "=== CONNECT TO BOARD START ===";

    // Test if OpenOCD is listening BEFORE starting worker
    QTcpSocket testSocket;
    testSocket.connectToHost("127.0.0.1", 4444);

    if (testSocket.waitForConnected(1000)) {
        qDebug() << "✓ Pre-check: OpenOCD is listening on 127.0.0.1:4444";
        testSocket.disconnectFromHost();
    } else {
        qWarning() << "✗ Pre-check FAILED: OpenOCD not responding on 127.0.0.1:4444";
        QMessageBox::critical(this, "OpenOCD Not Running",
            "Cannot connect to OpenOCD on 127.0.0.1:4444\n\n"
            "Make sure OpenOCD is running in another terminal:\n"
            "  openocd -f target/esp32.cfg");
        return;
    }

    QString selectedMCU = m_mcuSelectComboBox ? m_mcuSelectComboBox->currentText() : "STM32";
    
    // Determine board to connect
    QString boardName;
    if (selectedMCU.contains("STM32", Qt::CaseInsensitive)) {
        boardName = "STM32F030R8";
    } else if (selectedMCU.contains("ESP32-S3", Qt::CaseInsensitive)) {
        boardName = "ESP32-S3";
    } else if (selectedMCU.contains("ESP32", Qt::CaseInsensitive)) {
        boardName = "ESP32";  // Auto-detect
    } else if (selectedMCU.contains("Custom", Qt::CaseInsensitive)) {
        boardName = "ESP32";
    } else {
        qWarning() << "Unknown board selection:" << selectedMCU;
        if (m_connectionStatusLabel) {
            m_connectionStatusLabel->setText("Unknown board");
        }
        return;
    }
    
    // Pre-set board (worker will override with auto-detect if detected)
    m_openocd.setBoard(boardName);
    
    // Show progress UI, show cancel button
    if (m_connectionProgressBar) {
        m_connectionProgressBar->setVisible(true);
        m_connectionProgressBar->setValue(0);
    }
    if (m_cancelConnectionButton) {
        m_cancelConnectionButton->setVisible(true);
    }
    if (m_connectionStatusLabel) {
        m_connectionStatusLabel->setText("Connecting...");
        m_connectionStatusLabel->setStyleSheet("color: #FFD93D; font-style: italic;");
    }
    
    // Create worker thread if not already created
    if (!m_openocdThread) {
        m_openocdThread = new QThread(this);
        m_openocdWorker = new OpenOCDWorker(&m_openocd);
        m_openocdWorker->moveToThread(m_openocdThread);
        
        connect(m_openocdWorker, &OpenOCDWorker::connectionSucceeded,
                this, &MainWindow::onOpenOCDConnected);
        connect(m_openocdWorker, &OpenOCDWorker::connectionFailed,
                this, &MainWindow::onOpenOCDConnectionFailed);
        connect(m_openocdWorker, &OpenOCDWorker::progressUpdated,
                this, &MainWindow::onOpenOCDProgress);
        connect(m_openocdThread, &QThread::finished,
                m_openocdWorker, &QObject::deleteLater);
        
        m_openocdThread->start();
    }
    
    // Start connection in background
    QMetaObject::invokeMethod(m_openocdWorker, [this]() {
        m_openocdWorker->connectToOpenOCD("127.0.0.1", 4444, 5);
    }, Qt::QueuedConnection);
}

void MainWindow::onOpenOCDConnected(const QString& boardName) {
    if (m_connectionProgressBar) {
        m_connectionProgressBar->setVisible(false);
    }
    if (m_cancelConnectionButton) {
        m_cancelConnectionButton->setVisible(false);
    }
    if (m_connectionStatusLabel) {
        m_connectionStatusLabel->setText(QString("✓ Connected: %1").arg(boardName));
        m_connectionStatusLabel->setStyleSheet("color: #2ecc71; font-weight: bold;");
    }
    
    updateHardwareStatusBadge(true, boardName);
    qDebug() << "OpenOCD connected. Board:" << boardName;
    if (statusBar()) {
        statusBar()->showMessage(QString("Board connected: %1").arg(boardName), 4000);
    }
}

void MainWindow::onOpenOCDConnectionFailed(const QString& errorMessage) {
    if (m_connectionProgressBar) {
        m_connectionProgressBar->setVisible(false);
    }
    if (m_cancelConnectionButton) {
        m_cancelConnectionButton->setVisible(false);
    }
    if (m_connectionStatusLabel) {
        m_connectionStatusLabel->setText("❌ Connection Failed");
        m_connectionStatusLabel->setStyleSheet("color: #FF6B6B; font-weight: bold;");
    }
    
    updateHardwareStatusBadge(false, "");
    qCritical() << "OpenOCD connection failed:" << errorMessage;
    if (statusBar()) {
        statusBar()->showMessage("Connection failed: Check OpenOCD and board connection", 4000);
    }
    
    // Show error dialog
    QMessageBox::critical(this, "Connection Failed", 
        "Failed to connect to OpenOCD:\n\n" + errorMessage);
}

void MainWindow::onOpenOCDProgress(const QString& message, int progress) {
    if (m_connectionProgressBar) {
        m_connectionProgressBar->setValue(progress);
    }
    if (m_connectionStatusLabel) {
        m_connectionStatusLabel->setText(message);
    }
    qDebug() << message << "(" << progress << "%)";
}

void MainWindow::onCancelOpenOCDConnection() {
    if (m_openocdWorker) {
        m_openocdWorker->cancelConnection();
    }
    if (m_connectionProgressBar) {
        m_connectionProgressBar->setVisible(false);
    }
    if (m_cancelConnectionButton) {
        m_cancelConnectionButton->setVisible(false);
    }
    if (m_connectionStatusLabel) {
        m_connectionStatusLabel->setText("Cancelled");
        m_connectionStatusLabel->setStyleSheet("color: #FF6B6B; font-style: italic;");
    }
}

void MainWindow::onMCUSelectionChanged(const QString& mcuName) {
    // Show browse button only if "Custom" selected
    bool showBrowse = mcuName.contains("Custom", Qt::CaseInsensitive);
    if (m_browseMCUConfigButton) {
        m_browseMCUConfigButton->setVisible(showBrowse);
    }
}

void MainWindow::onBrowseMCUConfig() {
    QString configPath = QFileDialog::getOpenFileName(this,
        "Select OpenOCD Config File", "",
        "OpenOCD Config Files (*.cfg);;All Files (*)");
    
    if (!configPath.isEmpty()) {
        qDebug() << "User selected OpenOCD config:" << configPath;
        if (m_connectionStatusLabel) {
            m_connectionStatusLabel->setText(QString("Config: %1").arg(
                QFileInfo(configPath).fileName()));
            m_connectionStatusLabel->setStyleSheet("color: #3498db; font-style: italic;");
        }
    }
}

void MainWindow::switchToDesignWorkspace() {
    setWorkspaceMode(WorkspaceMode::Design);
}

void MainWindow::switchToCodeWorkspace() {
    setWorkspaceMode(WorkspaceMode::Code);
}

void MainWindow::setWorkspaceMode(WorkspaceMode mode) {
    if (m_inWorkspaceModeSwitch) {
        return;
    }
    if (m_workspaceMode == mode && m_centralStack && m_centralStack->currentIndex() == (mode == WorkspaceMode::Design ? 0 : 1)) {
        return;
    }

    m_inWorkspaceModeSwitch = true;
    m_workspaceMode = mode;

    if (mode == WorkspaceMode::Code) {
        // 1. Save current visibility of all design dock widgets
        m_dockVisibilityBeforeCodeMode["palette"] = m_paletteDock ? m_paletteDock->isVisible() : true;
        m_dockVisibilityBeforeCodeMode["screens"] = m_screensDock ? m_screensDock->isVisible() : false;
        m_dockVisibilityBeforeCodeMode["uploads"] = m_uploadsDock ? m_uploadsDock->isVisible() : false;
        m_dockVisibilityBeforeCodeMode["prop"] = m_propDock ? m_propDock->isVisible() : true;
        m_dockVisibilityBeforeCodeMode["effects"] = m_effectsDock ? m_effectsDock->isVisible() : false;
        m_dockVisibilityBeforeCodeMode["prototype"] = m_prototypeDock ? m_prototypeDock->isVisible() : false;
        m_dockVisibilityBeforeCodeMode["layer"] = m_layerDock ? m_layerDock->isVisible() : true;
        m_dockVisibilityBeforeCodeMode["styles"] = m_stylesDock ? m_stylesDock->isVisible() : false;
        m_dockVisibilityBeforeCodeMode["ai"] = m_aiDockWidget ? m_aiDockWidget->isVisible() : false;
        m_dockVisibilityBeforeCodeMode["resource"] = m_resourceDock ? m_resourceDock->isVisible() : false;
        m_dockVisibilityBeforeCodeMode["boardDisplay"] = m_boardDisplayDock ? m_boardDisplayDock->isVisible() : false;
        m_dockVisibilityBeforeCodeMode["mcu"] = m_mcuDock ? m_mcuDock->isVisible() : false;
        if (m_buildDock) m_dockVisibilityBeforeCodeMode["build"] = m_buildDock->isVisible();
        if (m_deviceMonitorDock) m_dockVisibilityBeforeCodeMode["deviceMonitor"] = m_deviceMonitorDock->isVisible();

        // 2. Hide visual canvas editing panels while Code mode is active
        if (m_paletteDock) m_paletteDock->hide();
        if (m_screensDock) m_screensDock->hide();
        if (m_uploadsDock) m_uploadsDock->hide();
        if (m_propDock) m_propDock->hide();
        if (m_effectsDock) m_effectsDock->hide();
        if (m_prototypeDock) m_prototypeDock->hide();
        if (m_layerDock) m_layerDock->hide();
        if (m_stylesDock) m_stylesDock->hide();
        if (m_aiDockWidget) m_aiDockWidget->hide();
        if (m_resourceDock) m_resourceDock->hide();
        if (m_boardDisplayDock) m_boardDisplayDock->hide();
        if (m_mcuDock) m_mcuDock->hide();
        if (m_buildDock) m_buildDock->hide();
        if (m_deviceMonitorDock) m_deviceMonitorDock->hide();

        // 3. Switch the central workspace to the code editor
        if (m_centralTabs && m_codeWorkspace && m_centralTabs->indexOf(m_codeWorkspace) >= 0) {
            m_centralTabs->setCurrentWidget(m_codeWorkspace);
        }
        if (m_codeWorkspace) {
            m_codeWorkspace->syncWithProject(m_project);
        }
        if (m_centralStack) {
            m_centralStack->setCurrentIndex(1);
        }

        // 4. Update switcher button states
        if (m_codeModeBtn) {
            m_codeModeBtn->blockSignals(true);
            m_codeModeBtn->setChecked(true);
            m_codeModeBtn->blockSignals(false);
        }
        if (m_designModeBtn) {
            m_designModeBtn->blockSignals(true);
            m_designModeBtn->setChecked(false);
            m_designModeBtn->blockSignals(false);
        }

        statusBar()->showMessage("Switched to Firmware Code Workspace", 3000);
    } else {
        // Return to visual designer
        // 1. Switch the central workspace back to design canvas container
        QWidget* designTarget = m_designWorkspace ? m_designWorkspace : m_designContainer;
        if (m_centralTabs && designTarget && m_centralTabs->indexOf(designTarget) >= 0) {
            m_centralTabs->setCurrentWidget(designTarget);
        }
        if (m_centralStack) {
            m_centralStack->setCurrentIndex(0);
        }

        // 2. Restore previous canvas dock panels
        if (m_dockVisibilityBeforeCodeMode.value("palette", true) && m_paletteDock) m_paletteDock->show();
        if (m_dockVisibilityBeforeCodeMode.value("screens", false) && m_screensDock) m_screensDock->show();
        if (m_dockVisibilityBeforeCodeMode.value("uploads", false) && m_uploadsDock) m_uploadsDock->show();
        if (m_dockVisibilityBeforeCodeMode.value("prop", true) && m_propDock) m_propDock->show();
        if (m_dockVisibilityBeforeCodeMode.value("effects", false) && m_effectsDock) m_effectsDock->show();
        if (m_dockVisibilityBeforeCodeMode.value("prototype", false) && m_prototypeDock) m_prototypeDock->show();
        if (m_dockVisibilityBeforeCodeMode.value("layer", true) && m_layerDock) m_layerDock->show();
        if (m_dockVisibilityBeforeCodeMode.value("styles", false) && m_stylesDock) m_stylesDock->show();
        if (m_dockVisibilityBeforeCodeMode.value("ai", false) && m_aiDockWidget) m_aiDockWidget->show();
        if (m_dockVisibilityBeforeCodeMode.value("resource", false) && m_resourceDock) m_resourceDock->show();
        if (m_dockVisibilityBeforeCodeMode.value("boardDisplay", false) && m_boardDisplayDock) m_boardDisplayDock->show();
        if (m_dockVisibilityBeforeCodeMode.value("mcu", false) && m_mcuDock) m_mcuDock->show();

        // Ensure primary docks are visible
        if (m_paletteDock && !m_paletteDock->isVisible()) m_paletteDock->show();
        if (m_propDock && !m_propDock->isVisible()) m_propDock->show();
        if (m_layerDock && !m_layerDock->isVisible()) m_layerDock->show();

        // 3. Update switcher button states
        if (m_designModeBtn) {
            m_designModeBtn->blockSignals(true);
            m_designModeBtn->setChecked(true);
            m_designModeBtn->blockSignals(false);
        }
        if (m_codeModeBtn) {
            m_codeModeBtn->blockSignals(true);
            m_codeModeBtn->setChecked(false);
            m_codeModeBtn->blockSignals(false);
        }

        statusBar()->showMessage("Switched to Visual Design Canvas", 3000);
    }

    m_inWorkspaceModeSwitch = false;
}

void MainWindow::checkFirstRunOnboarding(bool forceShow) {
    if (!forceShow && BoardRegistry::instance().isOnboardingCompleted()) {
        if (m_boardBanner) {
            m_boardBanner->hide();
        }
        return;
    }

    if (!forceShow) {
        if (qApp && (qApp->applicationName().contains("test", Qt::CaseInsensitive) ||
                     qApp->property("suppressFirstRunWizard").toBool() ||
                     (qEnvironmentVariableIsSet("QT_QPA_PLATFORM") && qgetenv("QT_QPA_PLATFORM") == "offscreen"))) {
            BoardRegistry::instance().setActiveBoard("custom_board");
            if (m_boardBanner) {
                m_boardBanner->show();
            }
            return;
        }
    }

    HardwareWizardDialog wizard(this);
    wizard.setWindowTitle("Choose Your Board");
    int res = wizard.exec();

    if (res == QDialog::Accepted) {
        if (m_boardBanner) {
            m_boardBanner->hide();
        }
        BoardDef b = BoardRegistry::instance().activeBoard();
        statusBar()->showMessage(QString("Target board set to: %1").arg(b.displayName.isEmpty() ? b.id : b.displayName), 3000);

        if (m_scene) {
            QList<InvalidBindingItem> invalidItems;
            for (UIComponent* comp : m_scene->uiComponents()) {
                if (!comp) continue;
                QString proto = comp->protocol();
                if (proto.isEmpty() || proto == "None") continue;

                QStringList pinsToCheck;
                if (proto == "GPIO" || proto == "PWM" || proto == "ADC") {
                    pinsToCheck.append(comp->protocolPin("pin"));
                } else if (proto == "SPI") {
                    pinsToCheck << comp->protocolPin("sck") << comp->protocolPin("mosi")
                                << comp->protocolPin("miso") << comp->protocolPin("ss");
                } else if (proto == "I2C") {
                    pinsToCheck << comp->protocolPin("scl") << comp->protocolPin("sda");
                }

                for (const QString& p : pinsToCheck) {
                    if (p.isEmpty() || p == "[None]") continue;
                    if (!b.pinMap.contains(p)) {
                        InvalidBindingItem item;
                        item.component = comp;
                        item.componentId = comp->componentId();
                        item.componentName = comp->componentId();
                        item.currentPin = p;
                        item.currentProtocol = proto;
                        item.reason = QString("Pin '%1' not supported on target '%2'").arg(p, b.displayName.isEmpty() ? b.id : b.displayName);
                        invalidItems.append(item);
                    } else if (b.pinMap[p].reserved) {
                        InvalidBindingItem item;
                        item.component = comp;
                        item.componentId = comp->componentId();
                        item.componentName = comp->componentId();
                        item.currentPin = p;
                        item.currentProtocol = proto;
                        item.reason = QString("Pin '%1' is reserved for system use (%2)").arg(p, b.pinMap[p].note.isEmpty() ? "System" : b.pinMap[p].note);
                        invalidItems.append(item);
                    }
                }
            }

            if (!invalidItems.isEmpty()) {
                InvalidBindingsDialog invDlg(b, invalidItems, this);
                if (invDlg.exec() == QDialog::Accepted) {
                    auto resolved = invDlg.resolvedBindings();
                    for (const auto& item : resolved) {
                        if (item.component && !item.keepAsCustom && !item.remappedPin.isEmpty()) {
                            QString proto = item.currentProtocol;
                            if (proto == "GPIO" || proto == "PWM" || proto == "ADC") {
                                item.component->setProtocolPin("pin", item.remappedPin);
                            } else if (proto == "SPI") {
                                if (item.component->protocolPin("sck") == item.currentPin) item.component->setProtocolPin("sck", item.remappedPin);
                                if (item.component->protocolPin("mosi") == item.currentPin) item.component->setProtocolPin("mosi", item.remappedPin);
                                if (item.component->protocolPin("miso") == item.currentPin) item.component->setProtocolPin("miso", item.remappedPin);
                                if (item.component->protocolPin("ss") == item.currentPin) item.component->setProtocolPin("ss", item.remappedPin);
                            } else if (proto == "I2C") {
                                if (item.component->protocolPin("scl") == item.currentPin) item.component->setProtocolPin("scl", item.remappedPin);
                                if (item.component->protocolPin("sda") == item.currentPin) item.component->setProtocolPin("sda", item.remappedPin);
                            }
                        }
                    }
                }
            }
        }
    } else {
        // Skipped with "Choose later" or cancelled
        BoardRegistry::instance().setActiveBoard("custom_board");
        if (m_boardBanner) {
            m_boardBanner->show();
        }
    }
}

void MainWindow::onChangeBoard() {
    checkFirstRunOnboarding(true);
}

void MainWindow::saveWindowLayout() {
    if (m_workspaceMode == WorkspaceMode::Code) {
        return; // Preserve design dock layout
    }
    QSettings settings("EmbeddedUIDesigner", "EmbeddedUIDesigner");
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/layoutState", saveState());

    const QList<QDockWidget*> docks = findChildren<QDockWidget*>();
    for (QDockWidget* dock : docks) {
        if (!dock->objectName().isEmpty()) {
            bool collapsed = dock->property("isCollapsed").toBool();
            settings.setValue(QString("dock/%1/collapsed").arg(dock->objectName()), collapsed);
            settings.setValue(QString("dock/%1/floating").arg(dock->objectName()), dock->isFloating());
            if (dock->isFloating()) {
                settings.setValue(QString("dock/%1/geometry").arg(dock->objectName()), dock->geometry());
            }
        }
    }
}

void MainWindow::restoreWindowLayout() {
    QSettings settings("EmbeddedUIDesigner", "EmbeddedUIDesigner");
    if (settings.contains("window/geometry")) {
        restoreGeometry(settings.value("window/geometry").toByteArray());
        QRect clamped = clampGeometryToScreens(geometry());
        if (clamped != geometry()) {
            setGeometry(clamped);
        }
    }
    if (settings.contains("window/layoutState")) {
        restoreState(settings.value("window/layoutState").toByteArray());
    }

    const QList<QDockWidget*> docks = findChildren<QDockWidget*>();
    for (QDockWidget* dock : docks) {
        if (!dock->objectName().isEmpty()) {
            QString key = QString("dock/%1/collapsed").arg(dock->objectName());
            if (settings.contains(key)) {
                bool collapsed = settings.value(key).toBool();
                setDockCollapsed(dock, collapsed);
            }
            QString floatKey = QString("dock/%1/floating").arg(dock->objectName());
            if (settings.contains(floatKey)) {
                bool isFloat = settings.value(floatKey).toBool();
                dock->setFloating(isFloat);
            }
            QString geomKey = QString("dock/%1/geometry").arg(dock->objectName());
            if (dock->isFloating() && settings.contains(geomKey)) {
                QRect savedGeom = settings.value(geomKey).toRect();
                QRect clampedGeom = clampGeometryToScreens(savedGeom, geometry());
                dock->setGeometry(clampedGeom);
            }
        }
    }
}

void MainWindow::resetLayout() {
    // 0. Redock any detached windows
    redockAllDetachedWindows();

    // 1. Uncollapse and unfloat all docks
    const QList<QDockWidget*> docks = findChildren<QDockWidget*>();
    for (QDockWidget* dock : docks) {
        setDockCollapsed(dock, false);
        dock->setFloating(false);
    }

    // 2. Restore default state if captured
    if (!m_defaultLayoutState.isEmpty()) {
        restoreState(m_defaultLayoutState);
    }

    // 3. Re-apply default sizing
    if (m_paletteDock && m_screensDock && m_uploadsDock) {
        resizeDocks({m_paletteDock, m_screensDock, m_uploadsDock}, {210, 210, 210}, Qt::Horizontal);
    }
    if (m_propDock && m_layerDock) {
        resizeDocks({m_propDock, m_effectsDock, m_prototypeDock, m_layerDock, m_stylesDock, m_aiDockWidget, m_resourceDock},
                    {260, 260, 260, 260, 260, 260, 260}, Qt::Horizontal);
        resizeDocks({m_propDock, m_layerDock}, {550, 330}, Qt::Vertical);
    }

    // 4. Clear saved settings in QSettings and re-save
    QSettings settings("EmbeddedUIDesigner", "EmbeddedUIDesigner");
    settings.remove("window/layoutState");
    for (QDockWidget* dock : docks) {
        if (!dock->objectName().isEmpty()) {
            settings.remove(QString("dock/%1/collapsed").arg(dock->objectName()));
            settings.remove(QString("dock/%1/floating").arg(dock->objectName()));
            settings.remove(QString("dock/%1/geometry").arg(dock->objectName()));
        }
    }
    saveWindowLayout();
}

void MainWindow::moveActiveWindowToScreen(QScreen* screen) {
    if (!screen) return;
    QWidget* active = QApplication::activeWindow();
    if (!active) active = this;

    QDockWidget* floatingDock = qobject_cast<QDockWidget*>(active);
    if (!floatingDock) {
        for (QWidget* p = active; p; p = p->parentWidget()) {
            if (auto* d = qobject_cast<QDockWidget*>(p)) {
                if (d->isFloating()) {
                    floatingDock = d;
                    break;
                }
            }
        }
    }

    QWidget* toMove = floatingDock ? static_cast<QWidget*>(floatingDock) : active;
    QRect avail = screen->availableGeometry();
    QSize sz = toMove->size();
    int x = avail.x() + qMax(0, (avail.width() - sz.width()) / 2);
    int y = avail.y() + qMax(0, (avail.height() - sz.height()) / 2);
    toMove->move(x, y);
    toMove->raise();
    toMove->activateWindow();
}

void MainWindow::detachCanvasWindow() {
    if (!m_centralTabs) return;
    int idx = m_centralTabs->indexOf(m_designWorkspace);
    if (idx >= 0) {
        m_centralTabs->detachTab(idx);
    }
}

void MainWindow::detachCodeWorkspaceWindow() {
    if (!m_centralTabs) return;
    int idx = m_centralTabs->indexOf(m_codeWorkspace);
    if (idx >= 0) {
        m_centralTabs->detachTab(idx);
    }
}

void MainWindow::redockAllDetachedWindows() {
    if (m_centralTabs) {
        m_centralTabs->redockAll();
    }
    if (m_codeWorkspace && m_codeWorkspace->tabbedEditor() && m_codeWorkspace->tabbedEditor()->tabWidget()) {
        m_codeWorkspace->tabbedEditor()->tabWidget()->redockAll();
    }
}

QRect MainWindow::clampGeometryToScreens(const QRect& targetRect, const QRect& fallbackRect) {
    return DetachableTabWidget::clampGeometryToScreens(targetRect, fallbackRect);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveWindowLayout();
    QMainWindow::closeEvent(event);
}





