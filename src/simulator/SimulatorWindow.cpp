#include "SimulatorWindow.h"
#include <QStatusBar>
#include <QCloseEvent>
#include <QTime>

namespace Simulator {

SimulatorWindow::SimulatorWindow(Project* project, QWidget* parent)
    : QMainWindow(parent)
    , m_project(project)
    , m_runtime(new SimulationRuntime(project, this))
{
    setupUi();

    connect(m_runtime, &SimulationRuntime::statusChanged, this, &SimulatorWindow::onRuntimeStatusChanged);
    connect(m_runtime, &SimulationRuntime::activeScreenChanged, this, &SimulatorWindow::onRuntimeActiveScreenChanged);
    connect(m_runtime, &SimulationRuntime::clockTicked, this, &SimulatorWindow::onClockTicked);

    // Auto-start simulation upon opening window
    onRun();
}

SimulatorWindow::~SimulatorWindow() {
    if (m_runtime) {
        m_runtime->stop();
    }
}

void SimulatorWindow::setupUi() {
    setWindowTitle("Embedded UI Designer — Desktop Simulator");
    resize(1200, 750);
    setStyleSheet(
        "QMainWindow { background-color: #0f172a; color: #f8fafc; font-family: sans-serif; }"
        "QToolBar { background-color: #1e293b; border-bottom: 1px solid #334155; spacing: 6px; padding: 4px; }"
        "QStatusBar { background-color: #1e293b; color: #94a3b8; border-top: 1px solid #334155; }"
        "QSplitter::handle { background-color: #334155; width: 2px; }"
    );

    setupToolBar();

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    m_canvasView = new SimulationCanvasView(m_runtime, splitter);
    m_controlPanel = new SimulationControlPanel(m_runtime, splitter);

    splitter->addWidget(m_canvasView);
    splitter->addWidget(m_controlPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    setCentralWidget(splitter);

    // Status bar
    statusBar()->showMessage("Desktop Simulation Engine Ready | Hardware Isolated");
    m_clockLabel = new QLabel("Sim Time: 00:00:00.000", this);
    m_clockLabel->setStyleSheet("color: #38bdf8; font-family: monospace; padding-right: 12px;");
    statusBar()->addPermanentWidget(m_clockLabel);
}

void SimulatorWindow::setupToolBar() {
    m_toolBar = addToolBar("Simulation Controls");
    m_toolBar->setMovable(false);

    // Run Button
    m_btnRun = new QPushButton("▶ Run", this);
    m_btnRun->setObjectName("simRunBtn");
    m_btnRun->setStyleSheet("background-color: #15803d; color: #f0fdf4; font-weight: bold; padding: 5px 12px; border-radius: 4px;");
    connect(m_btnRun, &QPushButton::clicked, this, &SimulatorWindow::onRun);
    m_toolBar->addWidget(m_btnRun);

    // Pause Button
    m_btnPause = new QPushButton("⏸ Pause", this);
    m_btnPause->setObjectName("simPauseBtn");
    m_btnPause->setStyleSheet("background-color: #b45309; color: #fef3c7; font-weight: bold; padding: 5px 12px; border-radius: 4px;");
    connect(m_btnPause, &QPushButton::clicked, this, &SimulatorWindow::onPause);
    m_toolBar->addWidget(m_btnPause);

    // Step Button
    m_btnStep = new QPushButton("⏯ Step", this);
    m_btnStep->setObjectName("simStepBtn");
    m_btnStep->setStyleSheet("background-color: #334155; color: #f8fafc; font-weight: bold; padding: 5px 10px; border-radius: 4px;");
    connect(m_btnStep, &QPushButton::clicked, this, &SimulatorWindow::onStep);
    m_toolBar->addWidget(m_btnStep);

    // Stop Button
    m_btnStop = new QPushButton("⏹ Stop", this);
    m_btnStop->setObjectName("simStopBtn");
    m_btnStop->setStyleSheet("background-color: #b91c1c; color: #fef2f2; font-weight: bold; padding: 5px 12px; border-radius: 4px;");
    connect(m_btnStop, &QPushButton::clicked, this, &SimulatorWindow::onStop);
    m_toolBar->addWidget(m_btnStop);

    // Restart Button
    m_btnRestart = new QPushButton("🔄 Restart", this);
    m_btnRestart->setObjectName("simRestartBtn");
    m_btnRestart->setStyleSheet("background-color: #0369a1; color: #f0f9ff; font-weight: bold; padding: 5px 12px; border-radius: 4px;");
    connect(m_btnRestart, &QPushButton::clicked, this, &SimulatorWindow::onRestart);
    m_toolBar->addWidget(m_btnRestart);

    m_toolBar->addSeparator();

    // Screen Selector
    m_toolBar->addWidget(new QLabel(" Screen: ", this));
    m_screenCombo = new QComboBox(this);
    m_screenCombo->setObjectName("simScreenCombo");
    if (m_project) {
        for (Screen* scr : m_project->screens()) {
            if (scr) m_screenCombo->addItem(scr->name(), scr->id());
        }
    }
    connect(m_screenCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SimulatorWindow::onScreenComboChanged);
    m_toolBar->addWidget(m_screenCombo);

    m_toolBar->addSeparator();

    // Speed Selector
    m_toolBar->addWidget(new QLabel(" Speed: ", this));
    m_speedCombo = new QComboBox(this);
    m_speedCombo->setObjectName("simSpeedCombo");
    m_speedCombo->addItem("0.25x", 0.25);
    m_speedCombo->addItem("0.5x", 0.5);
    m_speedCombo->addItem("1.0x (Realtime)", 1.0);
    m_speedCombo->addItem("2.0x", 2.0);
    m_speedCombo->addItem("5.0x", 5.0);
    m_speedCombo->addItem("10.0x", 10.0);
    m_speedCombo->setCurrentIndex(2); // 1.0x
    connect(m_speedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SimulatorWindow::onSpeedComboChanged);
    m_toolBar->addWidget(m_speedCombo);

    m_toolBar->addSeparator();

    // Zoom Selector
    m_toolBar->addWidget(new QLabel(" Zoom: ", this));
    m_zoomCombo = new QComboBox(this);
    m_zoomCombo->setObjectName("simZoomCombo");
    m_zoomCombo->addItem("100%", 1.0);
    m_zoomCombo->addItem("150%", 1.5);
    m_zoomCombo->addItem("200%", 2.0);
    m_zoomCombo->addItem("Fit Viewport", 0.0);
    connect(m_zoomCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SimulatorWindow::onZoomComboChanged);
    m_toolBar->addWidget(m_zoomCombo);

    m_toolBar->addSeparator();

    // Mode Status Badge
    m_statusBadge = new QLabel(" ● SIMULATION MODE (Hardware Isolated) ", this);
    m_statusBadge->setObjectName("simStatusBadge");
    m_statusBadge->setStyleSheet("background-color: #0c4a6e; color: #38bdf8; font-weight: bold; border-radius: 4px; padding: 4px 8px;");
    m_toolBar->addWidget(m_statusBadge);
}

void SimulatorWindow::onRun() {
    if (m_runtime) {
        m_runtime->start();
    }
}

void SimulatorWindow::onPause() {
    if (m_runtime) {
        m_runtime->pause();
    }
}

void SimulatorWindow::onStop() {
    if (m_runtime) {
        m_runtime->stop();
    }
}

void SimulatorWindow::onRestart() {
    if (m_runtime) {
        m_runtime->restart();
        if (m_controlPanel) m_controlPanel->refreshValues();
    }
}

void SimulatorWindow::onStep() {
    if (m_runtime) {
        m_runtime->step(50);
        if (m_controlPanel) m_controlPanel->refreshValues();
    }
}

void SimulatorWindow::onScreenComboChanged(int index) {
    if (index >= 0 && m_runtime && m_screenCombo) {
        QString sId = m_screenCombo->itemData(index).toString();
        m_runtime->setActiveScreenId(sId);
    }
}

void SimulatorWindow::onSpeedComboChanged(int index) {
    if (index >= 0 && m_runtime && m_speedCombo) {
        double factor = m_speedCombo->itemData(index).toDouble();
        m_runtime->setSpeedFactor(factor);
    }
}

void SimulatorWindow::onZoomComboChanged(int index) {
    if (index >= 0 && m_canvasView && m_zoomCombo) {
        double factor = m_zoomCombo->itemData(index).toDouble();
        if (factor <= 0.01) {
            m_canvasView->fitInViewport();
        } else {
            m_canvasView->setZoomFactor(factor);
        }
    }
}

void SimulatorWindow::onRuntimeStatusChanged(SimulationStatus status) {
    updateStatusIndicator();
}

void SimulatorWindow::onRuntimeActiveScreenChanged(const QString& screenId) {
    if (m_screenCombo) {
        int idx = m_screenCombo->findData(screenId);
        if (idx >= 0 && m_screenCombo->currentIndex() != idx) {
            m_screenCombo->blockSignals(true);
            m_screenCombo->setCurrentIndex(idx);
            m_screenCombo->blockSignals(false);
        }
    }
    if (m_canvasView) {
        m_canvasView->refreshScreen();
    }
}

void SimulatorWindow::onClockTicked(qint64 elapsedMs) {
    if (m_clockLabel) {
        int ms = static_cast<int>(elapsedMs % 1000);
        int secs = static_cast<int>((elapsedMs / 1000) % 60);
        int mins = static_cast<int>((elapsedMs / (1000 * 60)) % 60);
        int hrs = static_cast<int>(elapsedMs / (1000 * 60 * 60));
        m_clockLabel->setText(QString("Sim Time: %1:%2:%3.%4")
            .arg(hrs, 2, 10, QChar('0'))
            .arg(mins, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'))
            .arg(ms, 3, 10, QChar('0')));
    }
}

void SimulatorWindow::updateStatusIndicator() {
    if (!m_runtime || !m_statusBadge) return;

    if (m_runtime->isRunning()) {
        m_statusBadge->setText(" ● RUNNING (Simulation HAL) ");
        m_statusBadge->setStyleSheet("background-color: #065f46; color: #a7f3d0; font-weight: bold; border-radius: 4px; padding: 4px 8px;");
    } else if (m_runtime->isPaused()) {
        m_statusBadge->setText(" ⏸ PAUSED ");
        m_statusBadge->setStyleSheet("background-color: #78350f; color: #fde68a; font-weight: bold; border-radius: 4px; padding: 4px 8px;");
    } else {
        m_statusBadge->setText(" ⏹ STOPPED ");
        m_statusBadge->setStyleSheet("background-color: #450a0a; color: #fca5a5; font-weight: bold; border-radius: 4px; padding: 4px 8px;");
    }
}

void SimulatorWindow::closeEvent(QCloseEvent* event) {
    if (m_runtime) {
        m_runtime->stop();
    }
    event->accept();
}

} // namespace Simulator
