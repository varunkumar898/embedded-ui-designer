#pragma once

#include <QMainWindow>
#include <QDockWidget>
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

namespace Simulator {
class SimulatorWindow;
}

class DeviceManager;
class DesignerController;
class DesignerLocalServer;

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


    DesignerController* designerController() const { return m_controller; }
    DesignerLocalServer* localServer() const { return m_localServer; }
    bool startLocalMcpServer(quint16 port = 8765);
    void stopLocalMcpServer();

private slots:
    // File Actions
    void onNewProject();
    void onCreateEmbeddedProject();
    void onOpenProject();
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

    friend class TestFunctionalRunner;

private:
    // Core Subsystems
    CanvasScene* m_scene = nullptr;
    CanvasView* m_view = nullptr;
    Project* m_project = nullptr;
    DeviceManager* m_deviceManager = nullptr;
    class QUndoStack* m_undoStack = nullptr;

    // Dock Panels
    ComponentPalette* m_palette = nullptr;
    PropertiesPanel* m_propertiesPanel = nullptr;
    PrototypePanel* m_prototypePanel = nullptr;
    LayerPanel* m_layerPanel = nullptr;
    StylesPanel* m_stylesPanel = nullptr;
    ScreensPanel* m_screensPanel = nullptr;


    // UI Chrome
    QComboBox* m_resolutionCombo = nullptr;
    QLabel* m_tipLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_zoomLabel = nullptr;
    QLabel* m_hardwareStatusBadge = nullptr;
    QPushButton* m_hardwareConnectBtn = nullptr;

    void updateHardwareStatusBadge(bool connected, const QString& probeName);

    // Align/Distribute toolbar actions

    DesignerController* m_controller = nullptr;
    DesignerLocalServer* m_localServer = nullptr;
    Simulator::SimulatorWindow* m_simulatorWindow = nullptr;

    void setupUi();
    void setupMenusAndToolbars();
    void setupDocks();
    void applyTheme();
    void updateWindowTitle();

    /// Returns the current UIComponent selection (convenience).
    QList<UIComponent*> selectedComponents() const;
};
