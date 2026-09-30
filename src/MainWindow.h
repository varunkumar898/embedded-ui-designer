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

    // Selection changed → update enabled state of align/distribute buttons
    void onSelectionListChanged(const QList<UIComponent*>& selected);

    friend class TestFunctionalRunner;

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

    // UI Chrome
    QComboBox* m_resolutionCombo = nullptr;
    QLabel* m_tipLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_zoomLabel = nullptr;

    // Align/Distribute toolbar actions
    QAction* m_actAlignLeft    = nullptr;
    QAction* m_actAlignRight   = nullptr;
    QAction* m_actAlignHCenter = nullptr;
    QAction* m_actAlignTop     = nullptr;
    QAction* m_actAlignBottom  = nullptr;
    QAction* m_actAlignVCenter = nullptr;
    QAction* m_actDistributeH  = nullptr;
    QAction* m_actDistributeV  = nullptr;

    void setupUi();
    void setupMenusAndToolbars();
    void setupDocks();
    void applyTheme();
    void updateWindowTitle();

    /// Returns the current UIComponent selection (convenience).
    QList<UIComponent*> selectedComponents() const;
};
