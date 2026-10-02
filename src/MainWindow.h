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
#include "StylesPanel.h"
#include "Project.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

    Project* project() const { return m_project; }
    CanvasScene* canvasScene() const { return m_scene; }
    CanvasView* canvasView() const { return m_view; }
    StylesPanel* stylesPanel() const { return m_stylesPanel; }
    PropertiesPanel* propertiesPanel() const { return m_propertiesPanel; }
    ComponentPalette* palette() const { return m_palette; }
    class QComboBox* resolutionCombo() const { return m_resolutionCombo; }

private slots:
    // File Actions
    void onNewProject();
    void onOpenProject();
    void onImportProject();      // Feature 2: merge .euiproj into current project
    void onOpenSampleProject();
    void onSaveProject();
    void onSaveProjectAs();
    void onExport();             // Feature 3: unified export dialog
    // Keep these as implementation helpers (called from onExport):
    void onExportUgfx();
    void onExportQtMcu();
    void onExportLvgl();

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
    class QUndoStack* m_undoStack = nullptr;

    // Dock Panels
    ComponentPalette* m_palette = nullptr;
    PropertiesPanel* m_propertiesPanel = nullptr;
    LayerPanel* m_layerPanel = nullptr;
    StylesPanel* m_stylesPanel = nullptr;

    // UI Chrome
    QComboBox* m_resolutionCombo = nullptr;
    QLabel* m_tipLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_zoomLabel = nullptr;

    void setupUi();
    void setupMenusAndToolbars();
    void setupDocks();
    void applyTheme();
    void updateWindowTitle();
};
