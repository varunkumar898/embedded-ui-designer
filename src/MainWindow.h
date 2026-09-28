#pragma once

#include <QMainWindow>
#include <QDockWidget>
#include <QLabel>
#include <QComboBox>
#include "CanvasScene.h"
#include "CanvasView.h"
#include "ComponentPalette.h"
#include "PropertiesPanel.h"
#include "LayerPanel.h"
#include "Project.h"

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
    void onSaveProject();
    void onSaveProjectAs();
    void onExportUgfx();
    void onExportQtMcu();

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

private:
    // Core Subsystems
    CanvasScene* m_scene = nullptr;
    CanvasView* m_view = nullptr;
    Project* m_project = nullptr;

    // Dock Panels
    ComponentPalette* m_palette = nullptr;
    PropertiesPanel* m_propertiesPanel = nullptr;
    LayerPanel* m_layerPanel = nullptr;

    // UI Chrome
    QComboBox* m_resolutionCombo = nullptr;
    QLabel* m_zoomLabel = nullptr;
    QLabel* m_statusLabel = nullptr;

    void setupUi();
    void setupMenusAndToolbars();
    void setupDocks();
    void applyTheme();
    void updateWindowTitle();
};
