#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QGroupBox>
#include <QHeaderView>
#include "HardwareBridge.h"
#include "project/Project.h"

#include "hardware/PinoutViewWidget.h"
#include "hardware/PinMuxEngine.h"
#include "hardware/BoardRegistry.h"

class CanvasScene;

class PinBindingDialog : public QDialog {
    Q_OBJECT

public:
    explicit PinBindingDialog(CanvasScene* scene, Project* project = nullptr, QWidget* parent = nullptr);
    ~PinBindingDialog() override = default;

    void refreshTable();
    void updateStatusBadge();
    void updateConflictBanner();

private slots:
    void onBoardChanged(int index);
    void onTaskFilterChanged(int id);
    void onPortFilterChanged(int index);
    void onSearchTextChanged(const QString& text);
    void onTaskAModeChanged(int row, int index);
    void onTaskBModeChanged(int row, int index);
    void onPeripheralChanged(int row, int index);
    void onComponentBindingChanged(int row, int index);
    void onConnectClicked();
    void onDisconnectClicked();
    void onDetectProbeClicked();
    void onSimulationToggleClicked(bool checked);
    void onHardwareBridgeUpdate();
    void onImportBoardConfigClicked();
    void onSaveCustomPresetClicked();

private:
    void setupUi();
    void populatePortFilterCombo();
    void updateRowLiveValue(int row, const QString& pinName, const PinProfile& prof);

    CanvasScene* m_scene = nullptr;
    Project* m_project = nullptr;
    HardwareBridge& m_bridge;

    // Top Toolbar
    QComboBox* m_boardCombo = nullptr;
    QPushButton* m_importBtn = nullptr;
    QPushButton* m_savePresetBtn = nullptr;
    QPushButton* m_detectProbeBtn = nullptr;
    QPushButton* m_connectBtn = nullptr;
    QPushButton* m_disconnectBtn = nullptr;
    QPushButton* m_simToggleBtn = nullptr;
    QPushButton* m_livePollBtn = nullptr;
    QLabel* m_statusBadge = nullptr;

    // Task & Filter Controls
    QButtonGroup* m_taskButtonGroup = nullptr;
    QPushButton* m_taskABtn = nullptr;
    QPushButton* m_taskBBtn = nullptr;
    QPushButton* m_allTasksBtn = nullptr;
    QComboBox* m_portFilterCombo = nullptr;
    QCheckBox* m_advancedPinsToggle = nullptr;
    QPushButton* m_overrideGuardBtn = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QLabel* m_conflictBanner = nullptr;

    // Pin Table & Pinout View
    QTableWidget* m_pinTable = nullptr;
    Hardware::PinoutViewWidget* m_pinoutWidget = nullptr;
    Hardware::PinMuxEngine m_pinMuxEngine;

    QString m_selectedTask = "Task A"; // "Task A", "Task B", "All Tasks"
    QString m_selectedPort = "All Ports";
    QString m_searchQuery;
};
