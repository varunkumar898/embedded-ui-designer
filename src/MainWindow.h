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
#include "Project.h"

class DeviceManager;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    // File Actions
    void onNewProject();
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
    void onAbout();

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

    // UI Chrome
    QComboBox* m_resolutionCombo = nullptr;
    QLabel* m_tipLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_zoomLabel = nullptr;

    // Align/Distribute toolbar actions

    void setupUi();
    void setupMenusAndToolbars();
    void setupDocks();
    void applyTheme();
    void updateWindowTitle();

    /// Returns the current UIComponent selection (convenience).
    QList<UIComponent*> selectedComponents() const;
};
