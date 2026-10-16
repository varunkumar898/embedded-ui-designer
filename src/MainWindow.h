#pragma once

#include <QMainWindow>
#include <QDockWidget>
#include <QCloseEvent>
#include <QLabel>
#include <QComboBox>
#include <QAction>
#include <QList>
#include "CanvasScene.h"
#include "CanvasView.h"
#include "ComponentPalette.h"
#include "PropertiesPanel.h"
#include "PrototypePanel.h"
#include "LayerPanel.h"
#include "StylesPanel.h"
#include "ScreensPanel.h"
#include "Project.h"
#include "OpenOCDManager.h"
#include "workspace/WorkspaceMode.h"
#include "workspace/WorkspaceManager.h"
#include "workspace/DesignWorkspace.h"
#include "hardware/BoardDetector.h"
#include "hardware/BoardDatabase.h"

class ContextualTopToolbar;
class FloatingSelectionToolbar;
class EffectsPanel;
class UploadsPanel;
class ResourceAnalyzerPanel;
class BoardDisplayPanel;
class QToolButton;
class QToolBar;
class QStackedWidget;
class QButtonGroup;
class CodeWorkspaceView;

namespace Simulator {
class SimulatorWindow;
}

namespace AI {
class AIAssistantPanel;
}

class DeviceManager;
class DesignerController;
class DesignerLocalServer;
class ResourceAnalyzerPanel;
class BuildOutputPanel;
class ProblemsPanel;
class QProgressBar;
class OpenOCDWorker;
class QThread;
class DeviceMonitorPanel;

namespace Diagnostics {
class DiagnosticsService;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    CanvasScene* canvasScene() const { return m_scene; }
    CanvasView* canvasView() const { return m_view; }
    Project* currentProject() const { return m_project; }
    class QUndoStack* undoStack() const { return m_undoStack; }
    PropertiesPanel* propertiesPanel() const { return m_propertiesPanel; }
    LayerPanel* layerPanel() const { return m_layerPanel; }
    ScreensPanel* screensPanel() const { return m_screensPanel; }
    ContextualTopToolbar* contextualTopToolbar() const { return m_topToolbar; }
    FloatingSelectionToolbar* floatingToolbar() const { return m_floatingToolbar; }
    EffectsPanel* effectsPanel() const { return m_effectsPanel; }
    UploadsPanel* uploadsPanel() const { return m_uploadsPanel; }
    ResourceAnalyzerPanel* resourceAnalyzerPanel() const { return m_resourcePanel; }
    BoardDisplayPanel* boardDisplayPanel() const { return m_boardDisplayPanel; }
    ProblemsPanel* problemsPanel() const { return m_problemsPanel; }
    Diagnostics::DiagnosticsService* diagnosticsService() const { return m_diagnosticsService; }

    DesignerController* designerController() const { return m_controller; }
    DesignerLocalServer* localServer() const { return m_localServer; }
    bool startLocalMcpServer(quint16 port = 8765);
    void stopLocalMcpServer();

    // Hardware-in-the-Loop & OpenOCD Controls
    class OpenOCDManager& openOcdManager() { return m_openocd; }
    void createGpioControls();

    // Workspace Switching
    WorkspaceMode workspaceMode() const { return m_workspaceMode; }
    WorkspaceManager* workspaceManager() const { return m_workspaceManager; }
    DesignWorkspace* designWorkspace() const { return m_designWorkspace; }
    CodeWorkspaceView* codeWorkspaceView() const { return m_codeWorkspace; }
    class DetachableTabWidget* centralTabWidget() const { return m_centralTabs; }
    BoardDetector& boardDetector() { return m_boardDetector; }
    void setWorkspaceMode(WorkspaceMode mode);
    QWidget* boardBanner() const { return m_boardBanner; }
    void checkFirstRunOnboarding(bool forceShow = false);

public slots:
    void switchToDesignWorkspace();
    void switchToCodeWorkspace();
    void onOpenBoardSelectorDialog();
    void onChangeBoard();
    void saveWindowLayout();
    void restoreWindowLayout();
    void resetLayout();
    void moveActiveWindowToScreen(class QScreen* screen);
    void detachCanvasWindow();
    void detachCodeWorkspaceWindow();
    void redockAllDetachedWindows();
    static void setDockCollapsed(QDockWidget* dock, bool collapsed, QPushButton* minBtn = nullptr);
    static bool isDockCollapsed(QDockWidget* dock);
    static QRect clampGeometryToScreens(const QRect& targetRect, const QRect& fallbackRect = QRect());

private slots:
    // File Actions
    void onNewProject();
    void onCreateEmbeddedProject();
    void onOpenProject();
    void onOpenFirmwareFolder();
    void onOpenSampleProject();
    void onImportProject();
    void onSaveProject();
    void onSaveProjectAs();
    void onExport();
    void onExportUgfx();
    void onExportQtMcu();
    void onExportLvgl();
    void onFlashFirmware();
    void onSerialMonitor();
    void onPinConfiguration();
    void onImportBoardConfig();
    void onImportQmlDesign();
    void onImportHtmlCss();

    // Edit Actions
    void onDeleteSelected();
    void onDuplicateSelected();

    // View Actions
    void onToggleGrid(bool checked);
    void onToggleSnap(bool checked);
    void onZoomChanged(qreal factor);

    // Project & Hardware Actions
    void onResolutionPresetChanged(int index);
    void onProjectSettingsDialog();
    void onAISettingsDialog();
    void onGitDialog();
    void onCICDDialog();
    void onDiagnosticsDialog();
    void onAbout();
    void onRunSimulator();

    // Align Actions (Task 3)
    void onAlignLeft();
    void onAlignRight();
    void onAlignHCenter();
    void onAlignTop();
    void onAlignBottom();
    void onAlignVCenter();

    // Distribute Actions (Task 4)
    void onDistributeH();
    void onDistributeV();

    // Boolean Path Operations (Task 2)
    void onBooleanUnion();
    void onBooleanSubtract();
    void onBooleanIntersect();
    void onBooleanXor();
    void runBooleanOp(int opCode);

signals:
    void liveTestModeChanged(bool enabled);

    friend class TestFunctionalRunner;

private:
    // Core Subsystems
    CanvasScene* m_scene = nullptr;
    CanvasView* m_view = nullptr;
    CanvasView* m_canvasView = nullptr;
    Project* m_project = nullptr;
    DeviceManager* m_deviceManager = nullptr;
    class QUndoStack* m_undoStack = nullptr;

    // Dock Panels
    ComponentPalette* m_palette = nullptr;
    QDockWidget* m_paletteDock = nullptr;
    PropertiesPanel* m_propertiesPanel = nullptr;
    QDockWidget* m_propDock = nullptr;
    PrototypePanel* m_prototypePanel = nullptr;
    QDockWidget* m_prototypeDock = nullptr;
    LayerPanel* m_layerPanel = nullptr;
    QDockWidget* m_layerDock = nullptr;
    StylesPanel* m_stylesPanel = nullptr;
    QDockWidget* m_stylesDock = nullptr;
    ScreensPanel* m_screensPanel = nullptr;
    QDockWidget* m_screensDock = nullptr;
    AI::AIAssistantPanel* m_aiAssistantPanel = nullptr;
    QDockWidget* m_aiDockWidget = nullptr;
    EffectsPanel* m_effectsPanel = nullptr;
    QDockWidget* m_effectsDock = nullptr;
    UploadsPanel* m_uploadsPanel = nullptr;
    QDockWidget* m_uploadsDock = nullptr;
    ResourceAnalyzerPanel* m_resourcePanel = nullptr;
    QDockWidget* m_resourceDock = nullptr;
    QDockWidget* m_resourceAnalyzerPanel = nullptr;
    BoardDisplayPanel* m_boardDisplayPanel = nullptr;
    QDockWidget* m_boardDisplayDock = nullptr;
    BuildOutputPanel* m_buildOutputPanel = nullptr;
    QDockWidget* m_buildDock = nullptr;
    DeviceMonitorPanel* m_deviceMonitorPanel = nullptr;
    DeviceMonitorPanel* m_deviceMonitor = nullptr;
    QDockWidget* m_deviceMonitorDock = nullptr;
    ProblemsPanel* m_problemsPanel = nullptr;
    QDockWidget* m_problemsDock = nullptr;
    Diagnostics::DiagnosticsService* m_diagnosticsService = nullptr;
    QLabel* m_problemsBadgeLabel = nullptr;

    QAction* m_actBuildOutput = nullptr;
    QAction* m_actDeviceMonitor = nullptr;
    QAction* m_actProblems = nullptr;
    QAction* m_actResourceAnalyzer = nullptr;
    QAction* m_actBoardDisplay = nullptr;

    // Workspace Central Stack & Switcher
    WorkspaceMode m_workspaceMode = WorkspaceMode::Design;
    WorkspaceManager* m_workspaceManager = nullptr;
    DesignWorkspace* m_designWorkspace = nullptr;
    QWidget* m_boardBanner = nullptr;
    BoardDetector m_boardDetector;
    QStackedWidget* m_centralStack = nullptr;
    class DetachableTabWidget* m_centralTabs = nullptr;
    QWidget* m_designContainer = nullptr;
    CodeWorkspaceView* m_codeWorkspace = nullptr;
    QToolButton* m_designModeBtn = nullptr;
    QToolButton* m_codeModeBtn = nullptr;
    QButtonGroup* m_workspaceGroup = nullptr;
    QMap<QString, bool> m_dockVisibilityBeforeCodeMode;
    bool m_inWorkspaceModeSwitch = false;

    ContextualTopToolbar* m_topToolbar = nullptr;
    QWidget* m_contextualToolbar = nullptr;
    FloatingSelectionToolbar* m_floatingToolbar = nullptr;
    OpenOCDManager m_openocd;

    bool validateExportResources(const QString& targetFramework);


    // UI Chrome
    QToolBar* m_toolbar = nullptr;
    QComboBox* m_resolutionCombo = nullptr;
    QLabel* m_tipLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_zoomLabel = nullptr;
    QLabel* m_hardwareStatusBadge = nullptr;
    QPushButton* m_hardwareConnectBtn = nullptr;

    // NEW: Live Test Mode
    bool m_liveTestMode = false;
    QToolButton* m_liveTestButton = nullptr;
    void toggleLiveTestMode(bool enabled);

    void updateHardwareStatusBadge(bool connected, const QString& probeName);

    // Align/Distribute toolbar actions

    DesignerController* m_controller = nullptr;
    DesignerLocalServer* m_localServer = nullptr;
    Simulator::SimulatorWindow* m_simulatorWindow = nullptr;

    // OpenOCD connection in background thread
    QThread* m_openocdThread = nullptr;
    OpenOCDWorker* m_openocdWorker = nullptr;
    
    // UI for connection progress
    QProgressBar* m_connectionProgressBar = nullptr;
    QPushButton* m_cancelConnectionButton = nullptr;
    QLabel* m_connectionStatusLabel = nullptr;
    
    // MCU selection
    QComboBox* m_mcuSelectComboBox = nullptr;
    QPushButton* m_browseMCUConfigButton = nullptr;
    QDockWidget* m_mcuDock = nullptr;

    void setupUi();
    void setupMenusAndToolbars();
    void setupDocks();
    void setupDiagnostics();
    void updateProblemsBadge();
    void navigateToDiagnostic(const QString& screenId, const QString& componentId);
    void setupMCUSelection();
    void applyTheme();
    void setTheme(const QString& theme);
    void updateWindowTitle();

    /// Returns the current UIComponent selection (convenience).
    QList<UIComponent*> selectedComponents() const;

private slots:
    void onConnectToBoard();
    void onOpenOCDConnected(const QString& boardName);
    void onOpenOCDConnectionFailed(const QString& errorMessage);
    void onOpenOCDProgress(const QString& message, int progress);
    void onCancelOpenOCDConnection();
    void onMCUSelectionChanged(const QString& mcuName);
    void onBrowseMCUConfig();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    QByteArray m_defaultLayoutState;
};
