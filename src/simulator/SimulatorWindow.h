#pragma once

#include <QMainWindow>
#include <QToolBar>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include "Project.h"
#include "SimulationRuntime.h"
#include "SimulationCanvasView.h"
#include "SimulationControlPanel.h"

namespace Simulator {

/**
 * @brief SimulatorWindow is the dedicated desktop simulation environment for Embedded UI Designer.
 * Enables interactive multi-screen navigation, live data bindings, and virtual hardware execution.
 */
class SimulatorWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit SimulatorWindow(Project* project, QWidget* parent = nullptr);
    ~SimulatorWindow() override;

    SimulationRuntime* runtime() const { return m_runtime; }
    SimulationCanvasView* canvasView() const { return m_canvasView; }
    SimulationControlPanel* controlPanel() const { return m_controlPanel; }

public slots:
    void onRun();
    void onPause();
    void onStop();
    void onRestart();
    void onStep();
    void onScreenComboChanged(int index);
    void onSpeedComboChanged(int index);
    void onZoomComboChanged(int index);
    void onRuntimeStatusChanged(SimulationStatus status);
    void onRuntimeActiveScreenChanged(const QString& screenId);
    void onClockTicked(qint64 elapsedMs);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUi();
    void setupToolBar();
    void updateStatusIndicator();

    Project* m_project = nullptr;
    SimulationRuntime* m_runtime = nullptr;
    SimulationCanvasView* m_canvasView = nullptr;
    SimulationControlPanel* m_controlPanel = nullptr;

    // Toolbar controls
    QToolBar* m_toolBar = nullptr;
    QPushButton* m_btnRun = nullptr;
    QPushButton* m_btnPause = nullptr;
    QPushButton* m_btnStop = nullptr;
    QPushButton* m_btnRestart = nullptr;
    QPushButton* m_btnStep = nullptr;
    QComboBox* m_screenCombo = nullptr;
    QComboBox* m_speedCombo = nullptr;
    QComboBox* m_zoomCombo = nullptr;
    QLabel* m_statusBadge = nullptr;
    QLabel* m_clockLabel = nullptr;
};

} // namespace Simulator
