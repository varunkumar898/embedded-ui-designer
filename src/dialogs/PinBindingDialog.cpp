#include "PinBindingDialog.h"
#include "CanvasScene.h"
#include "UIComponent.h"
#include "ProgressBarComponent.h"
#include "LabelComponent.h"
#include "SliderComponent.h"
#include "SwitchComponent.h"
#include "CheckboxComponent.h"
#include "BoardConfigParser.h"
#include "theme/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QMessageBox>
#include <QDateTime>
#include <QFileDialog>
#include <QApplication>
#include <QSettings>
#include <QSplitter>
#include <QInputDialog>
#include "InvalidBindingsDialog.h"

PinBindingDialog::PinBindingDialog(CanvasScene* scene, Project* project, QWidget* parent)
    : QDialog(parent)
    , m_scene(scene)
    , m_project(project)
    , m_bridge(HardwareBridge::instance())
{
    setWindowTitle("Hardware Pin Configuration & Universal Connection Manager");
    resize(1080, 720);
    setStyleSheet(QString("QDialog { background-color: %1; color: %2; }").arg(Theme::BgBase, Theme::TextPrimary));

    setupUi();
    populatePortFilterCombo();
    refreshTable();
    updateStatusBadge();
    updateConflictBanner();

    connect(&m_bridge, &HardwareBridge::adcValueChanged, this, &PinBindingDialog::onHardwareBridgeUpdate);
    connect(&m_bridge, &HardwareBridge::pwmDutyChanged, this, &PinBindingDialog::onHardwareBridgeUpdate);
    connect(&m_bridge, &HardwareBridge::digitalStateChanged, this, &PinBindingDialog::onHardwareBridgeUpdate);
    connect(&m_bridge, &HardwareBridge::connectionStateChanged, this, [this](ConnectionState, const QString&) {
        updateStatusBadge();
        refreshTable();
    });
    connect(&m_bridge, &HardwareBridge::connectionStatusChanged, this, [this](bool, const QString&) {
        updateStatusBadge();
        refreshTable();
    });
    connect(&m_bridge, &HardwareBridge::probeDiscovered, this, [this](const QString&) {
        updateStatusBadge();
    });
    connect(&m_bridge, &HardwareBridge::probeRemoved, this, [this]() {
        updateStatusBadge();
        refreshTable();
    });
}

void PinBindingDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // ────────────────────────────────────────────────────────────────────────
    // 1. Top Toolbar: Board Profile, Import, Connection Actions & Status
    // ────────────────────────────────────────────────────────────────────────
    auto* topToolbar = new QHBoxLayout();
    topToolbar->setSpacing(10);

    auto* boardLabel = new QLabel("Target Board:", this);
    boardLabel->setStyleSheet(QString("font-weight: 600; color: %1; font-size: 12px;").arg(Theme::TextSecondary));
    topToolbar->addWidget(boardLabel);

    m_boardCombo = new QComboBox(this);
    m_boardCombo->setObjectName("boardProfileCombo");
    m_boardCombo->setStyleSheet(QString(
        "QComboBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 6px 12px; font-weight: 500; min-width: 220px; }"
        "QComboBox:hover { border-color: %5; }"
        "QComboBox QAbstractItemView { background-color: %1; color: %2; selection-background-color: %6; selection-color: %7; border: 1px solid %3; }"
    ).arg(Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle, QString::number(Theme::RadiusMd),
          Theme::BorderStrong, Theme::AccentSoft, Theme::AccentText));
    for (const auto& b : BoardRegistry::instance().boards()) {
        m_boardCombo->addItem(QString("%1 (%2)").arg(b.displayName.isEmpty() ? b.id : b.displayName, b.family), b.id);
    }
    for (const auto& bp : m_bridge.availableBoards()) {
        if (m_boardCombo->findData(bp.id) < 0) {
            m_boardCombo->addItem(QString("%1 (%2)").arg(bp.name.isEmpty() ? bp.id : bp.name, bp.mcuFamily), bp.id);
        }
    }
    QString activeId = !m_bridge.currentBoard().id.isEmpty() ? m_bridge.currentBoard().id : BoardRegistry::instance().activeBoard().id;
    int curIdx = m_boardCombo->findData(activeId);
    if (curIdx < 0) curIdx = m_boardCombo->findData(BoardRegistry::instance().activeBoard().id);
    if (curIdx >= 0) m_boardCombo->setCurrentIndex(curIdx);
    connect(m_boardCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PinBindingDialog::onBoardChanged);
    topToolbar->addWidget(m_boardCombo);

    m_savePresetBtn = new QPushButton("Save Custom Preset...", this);
    m_savePresetBtn->setObjectName("saveCustomPresetBtn");
    m_savePresetBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 6px 12px; font-weight: 600; font-size: 12px; }"
        "QPushButton:hover { background-color: %5; border-color: %6; }"
    ).arg(Theme::BgRaised, Theme::AccentText, Theme::BorderSubtle, QString::number(Theme::RadiusMd),
          Theme::BgOverlay, Theme::Accent));
    connect(m_savePresetBtn, &QPushButton::clicked, this, &PinBindingDialog::onSaveCustomPresetClicked);
    topToolbar->addWidget(m_savePresetBtn);

    m_importBtn = new QPushButton("Import Config...", this);
    m_importBtn->setObjectName("importBoardConfigBtn");
    m_importBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 6px 12px; font-weight: 600; font-size: 12px; }"
        "QPushButton:hover { background-color: %5; border-color: %6; }"
    ).arg(Theme::BgRaised, Theme::AccentText, Theme::BorderSubtle, QString::number(Theme::RadiusMd),
          Theme::BgOverlay, Theme::Accent));
    connect(m_importBtn, &QPushButton::clicked, this, &PinBindingDialog::onImportBoardConfigClicked);
    topToolbar->addWidget(m_importBtn);

    m_detectProbeBtn = new QPushButton("Detect Probe", this);
    m_detectProbeBtn->setObjectName("detectProbeBtn");
    m_detectProbeBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 6px 12px; font-weight: 600; font-size: 12px; }"
        "QPushButton:hover { background-color: %5; color: %6; }"
    ).arg(Theme::BgRaised, Theme::TextSecondary, Theme::BorderSubtle, QString::number(Theme::RadiusMd),
          Theme::BgOverlay, Theme::TextPrimary));
    connect(m_detectProbeBtn, &QPushButton::clicked, this, &PinBindingDialog::onDetectProbeClicked);
    topToolbar->addWidget(m_detectProbeBtn);

    m_connectBtn = new QPushButton("Connect", this);
    m_connectBtn->setObjectName("probeConnectBtn");
    m_connectBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %1; border-radius: %3px; padding: 6px 14px; font-weight: 600; font-size: 12px; }"
        "QPushButton:hover { background-color: %4; }"
        "QPushButton:disabled { background-color: %5; color: %6; border-color: %5; }"
    ).arg(Theme::Accent, Theme::BgBase, QString::number(Theme::RadiusMd),
          Theme::AccentHover, Theme::BgRaised, Theme::TextDisabled));
    connect(m_connectBtn, &QPushButton::clicked, this, &PinBindingDialog::onConnectClicked);
    topToolbar->addWidget(m_connectBtn);

    m_disconnectBtn = new QPushButton("Disconnect", this);
    m_disconnectBtn->setObjectName("probeDisconnectBtn");
    m_disconnectBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 6px 12px; font-weight: 600; font-size: 12px; }"
        "QPushButton:hover { background-color: %5; border-color: %2; }"
    ).arg(Theme::BgRaised, Theme::Danger, Theme::DangerBg, QString::number(Theme::RadiusMd),
          Theme::DangerBg));
    connect(m_disconnectBtn, &QPushButton::clicked, this, &PinBindingDialog::onDisconnectClicked);
    topToolbar->addWidget(m_disconnectBtn);

    m_simToggleBtn = new QPushButton("Simulation Mode", this);
    m_simToggleBtn->setObjectName("simulationModeBtn");
    m_simToggleBtn->setCheckable(true);
    m_simToggleBtn->setChecked(m_bridge.isSimulationMode());
    m_simToggleBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 6px 12px; font-weight: 600; font-size: 12px; }"
        "QPushButton:checked { background-color: %5; color: %2; border-color: %6; }"
        "QPushButton:hover { background-color: %7; }"
    ).arg(Theme::BgRaised, Theme::AccentText, Theme::BorderSubtle, QString::number(Theme::RadiusMd),
          Theme::AccentSoft, Theme::Accent, Theme::BgOverlay));
    connect(m_simToggleBtn, &QPushButton::toggled, this, &PinBindingDialog::onSimulationToggleClicked);
    topToolbar->addWidget(m_simToggleBtn);

    m_livePollBtn = new QPushButton(m_bridge.isLivePolling() ? "Pause Polling" : "Resume Polling", this);
    m_livePollBtn->setObjectName("livePollBtn");
    m_livePollBtn->setCheckable(true);
    m_livePollBtn->setChecked(m_bridge.isLivePolling());
    m_livePollBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 6px 12px; font-weight: 600; font-size: 12px; }"
        "QPushButton:checked { background-color: %5; color: %6; border-color: %6; }"
        "QPushButton:hover { background-color: %7; }"
    ).arg(Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle, QString::number(Theme::RadiusMd),
          Theme::SuccessBg, Theme::Success, Theme::BgOverlay));
    connect(m_livePollBtn, &QPushButton::clicked, this, [this](bool checked) {
        if (checked) {
            m_bridge.startLivePolling();
            m_livePollBtn->setText("Pause Polling");
        } else {
            m_bridge.stopLivePolling();
            m_livePollBtn->setText("Resume Polling");
        }
    });
    topToolbar->addWidget(m_livePollBtn);

    topToolbar->addStretch();

    m_statusBadge = new QLabel(this);
    m_statusBadge->setObjectName("probeStatusBadge");
    topToolbar->addWidget(m_statusBadge);

    mainLayout->addLayout(topToolbar);

    // ────────────────────────────────────────────────────────────────────────
    // 2. Task & Filter Workspace Header: Task Selector, Port Filter, Search
    // ────────────────────────────────────────────────────────────────────────
    auto* filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(12);

    auto* taskBox = new QHBoxLayout();
    taskBox->setSpacing(0);
    m_taskButtonGroup = new QButtonGroup(this);

    m_taskABtn = new QPushButton("Task A", this);
    m_taskABtn->setObjectName("taskABtn");
    m_taskABtn->setCheckable(true);
    m_taskABtn->setChecked(true);
    m_taskABtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-top-left-radius: %4px; border-bottom-left-radius: %4px; padding: 6px 14px; font-weight: 600; font-size: 12px; }"
        "QPushButton:checked { background-color: %5; color: %6; border-color: %5; }"
    ).arg(Theme::BgRaised, Theme::TextSecondary, Theme::BorderSubtle, QString::number(Theme::RadiusMd), Theme::Accent, Theme::BgBase));
    m_taskButtonGroup->addButton(m_taskABtn, 0);
    taskBox->addWidget(m_taskABtn);

    m_taskBBtn = new QPushButton("Task B", this);
    m_taskBBtn->setObjectName("taskBBtn");
    m_taskBBtn->setCheckable(true);
    m_taskBBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-left: none; border-right: none; padding: 6px 14px; font-weight: 600; font-size: 12px; }"
        "QPushButton:checked { background-color: %4; color: %5; border-color: %4; }"
    ).arg(Theme::BgRaised, Theme::TextSecondary, Theme::BorderSubtle, Theme::Accent, Theme::BgBase));
    m_taskButtonGroup->addButton(m_taskBBtn, 1);
    taskBox->addWidget(m_taskBBtn);

    m_allTasksBtn = new QPushButton("All Tasks", this);
    m_allTasksBtn->setObjectName("allTasksBtn");
    m_allTasksBtn->setCheckable(true);
    m_allTasksBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-top-right-radius: %4px; border-bottom-right-radius: %4px; padding: 6px 14px; font-weight: 600; font-size: 12px; }"
        "QPushButton:checked { background-color: %5; color: %6; border-color: %5; }"
    ).arg(Theme::BgRaised, Theme::TextSecondary, Theme::BorderSubtle, QString::number(Theme::RadiusMd), Theme::Accent, Theme::BgBase));
    m_taskButtonGroup->addButton(m_allTasksBtn, 2);
    taskBox->addWidget(m_allTasksBtn);

    connect(m_taskButtonGroup, &QButtonGroup::idClicked, this, &PinBindingDialog::onTaskFilterChanged);
    filterLayout->addLayout(taskBox);

    filterLayout->addSpacing(8);

    auto* portLabel = new QLabel("Port Filter:", this);
    portLabel->setStyleSheet(QString("font-weight: 600; color: %1; font-size: 12px;").arg(Theme::TextSecondary));
    filterLayout->addWidget(portLabel);

    m_portFilterCombo = new QComboBox(this);
    m_portFilterCombo->setObjectName("portFilterCombo");
    m_portFilterCombo->setStyleSheet(QString(
        "QComboBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 6px 12px; font-weight: 500; min-width: 130px; }"
        "QComboBox QAbstractItemView { background-color: %1; color: %2; selection-background-color: %5; selection-color: %6; }"
    ).arg(Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle, QString::number(Theme::RadiusMd), Theme::AccentSoft, Theme::AccentText));
    connect(m_portFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PinBindingDialog::onPortFilterChanged);
    filterLayout->addWidget(m_portFilterCombo);

    filterLayout->addSpacing(8);

    m_overrideGuardBtn = new QPushButton("Override write guard (session)", this);
    m_overrideGuardBtn->setObjectName("overrideGuardBtn");
    connect(m_overrideGuardBtn, &QPushButton::clicked, this, [this]() {
        auto reply = QMessageBox::warning(this, "Override write guard",
            "This enables live register writes for boards whose addresses are\n"
            "unverified. Writes will be logged. Continue?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            HardwareBridge::instance().setWriteOverride(true);
        }
    });
    filterLayout->addWidget(m_overrideGuardBtn);

    filterLayout->addSpacing(8);

    m_advancedPinsToggle = new QCheckBox("Advanced: show all board pins", this);
    m_advancedPinsToggle->setObjectName("advancedPinsToggle");
    QSettings _s("EmbeddedUIDesigner", "PinBindingDialog");
    m_advancedPinsToggle->setChecked(_s.value("showAllBoardPins", false).toBool());
    connect(m_advancedPinsToggle, &QCheckBox::toggled, this, [this](bool on) {
        QSettings s("EmbeddedUIDesigner", "PinBindingDialog");
        s.setValue("showAllBoardPins", on);
        refreshTable();
    });
    filterLayout->addWidget(m_advancedPinsToggle);

    filterLayout->addSpacing(8);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setObjectName("pinSearchEdit");
    m_searchEdit->setPlaceholderText("Search pin, peripheral, or mode...");
    m_searchEdit->setStyleSheet(QString(
        "QLineEdit { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 6px 12px; font-size: 12px; min-width: 200px; }"
        "QLineEdit:focus { border-color: %5; }"
    ).arg(Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle, QString::number(Theme::RadiusMd), Theme::Accent));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &PinBindingDialog::onSearchTextChanged);
    filterLayout->addWidget(m_searchEdit);

    filterLayout->addStretch();
    mainLayout->addLayout(filterLayout);

    // ────────────────────────────────────────────────────────────────────────
    // 3. Conflict Banner
    // ────────────────────────────────────────────────────────────────────────
    m_conflictBanner = new QLabel(this);
    m_conflictBanner->setObjectName("conflictBanner");
    m_conflictBanner->setWordWrap(true);
    m_conflictBanner->setVisible(false);
    mainLayout->addWidget(m_conflictBanner);

    // ────────────────────────────────────────────────────────────────────────
    // 4. Main Pin Configuration Table
    // ────────────────────────────────────────────────────────────────────────
    m_pinTable = new QTableWidget(this);
    m_pinTable->setObjectName("pinTable");
    m_pinTable->setColumnCount(7);
    m_pinTable->setHorizontalHeaderLabels({
        "Pin", "Task A Mode", "Task B Mode", "Peripheral / Function", "Canvas Component", "Live Value", "Status"
    });
    m_pinTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_pinTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_pinTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_pinTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_pinTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_pinTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_pinTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_pinTable->verticalHeader()->setVisible(false);
    m_pinTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pinTable->setAlternatingRowColors(true);
    m_pinTable->setStyleSheet(QString(
        "QTableWidget { background-color: %1; alternate-background-color: %2; gridline-color: %3; color: %4; border: 1px solid %5; border-radius: %6px; }"
        "QHeaderView::section { background-color: %7; color: %8; padding: 8px; font-weight: bold; border: none; border-bottom: 1px solid %5; }"
        "QTableWidget::item { padding: 4px; }"
        "QTableWidget::item:selected { background-color: %9; color: %10; }"
    ).arg(Theme::BgBase, Theme::BgSurface, Theme::BorderSubtle, Theme::TextPrimary, Theme::BorderStrong,
          QString::number(Theme::RadiusMd), Theme::BgRaised, Theme::TextSecondary, Theme::AccentSoft, Theme::AccentText));

    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->addWidget(m_pinTable);

    m_pinoutWidget = new Hardware::PinoutViewWidget(this);
    m_pinoutWidget->setObjectName("mcuPinoutView");
    m_pinoutWidget->setEngine(&m_pinMuxEngine);
    m_pinoutWidget->setMinimumWidth(320);
    mainSplitter->addWidget(m_pinoutWidget);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 2);

    connect(m_pinoutWidget, &Hardware::PinoutViewWidget::pinClicked, this, [this](const QString& pin) {
        if (!m_pinTable) return;
        for (int r = 0; r < m_pinTable->rowCount(); ++r) {
            if (m_pinTable->item(r, 0) && (m_pinTable->item(r, 0)->text().startsWith(pin + " ") || m_pinTable->item(r, 0)->text() == pin)) {
                m_pinTable->selectRow(r);
                m_pinTable->scrollToItem(m_pinTable->item(r, 0));
                break;
            }
        }
    });
    connect(m_pinTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        int r = m_pinTable->currentRow();
        if (r >= 0 && m_pinTable->item(r, 0)) {
            QString pin = m_pinTable->item(r, 0)->data(Qt::UserRole).toString();
            if (pin.isEmpty()) pin = m_pinTable->item(r, 0)->text().split(' ').first();
            if (m_pinoutWidget) m_pinoutWidget->setSelectedPin(pin);
        }
    });

    mainLayout->addWidget(mainSplitter, 1);

    // ────────────────────────────────────────────────────────────────────────
    // 5. Bottom Action Bar
    // ────────────────────────────────────────────────────────────────────────
    auto* bottomLayout = new QHBoxLayout();
    auto* hintLabel = new QLabel("Pin modes and peripheral routing are device-validated in real-time. Incompatible shared task assignments trigger immediate conflict warnings.", this);
    hintLabel->setStyleSheet(QString("font-size: 11px; color: %1; font-style: italic;").arg(Theme::TextMuted));
    bottomLayout->addWidget(hintLabel);
    bottomLayout->addStretch();

    auto* closeBtn = new QPushButton("Close", this);
    closeBtn->setStyleSheet(QString("QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 6px 16px; font-weight: 600; } QPushButton:hover { background-color: %5; border-color: %6; }")
        .arg(Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle, QString::number(Theme::RadiusMd), Theme::BgOverlay, Theme::Accent));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomLayout->addWidget(closeBtn);
    mainLayout->addLayout(bottomLayout);
}

void PinBindingDialog::populatePortFilterCombo() {
    m_portFilterCombo->blockSignals(true);
    m_portFilterCombo->clear();
    m_portFilterCombo->addItem("All Ports");

    QStringList ports = m_bridge.portGroupsForCurrentBoard();
    std::sort(ports.begin(), ports.end());
    for (const QString& port : ports) {
        m_portFilterCombo->addItem(port);
    }
    m_portFilterCombo->setCurrentIndex(0);
    m_selectedPort = "All Ports";
    m_portFilterCombo->blockSignals(false);
}

void PinBindingDialog::updateStatusBadge() {
    if (!m_statusBadge) return;

    ConnectionState state = m_bridge.connectionState();
    m_connectBtn->setText(m_bridge.connectionButtonLabel());

    if (state == ConnectionState::Connected) {
        QString probe = m_bridge.connectedProbeName();
        if (probe.isEmpty()) probe = m_bridge.currentBoard().name;
        m_statusBadge->setText(QString("● OpenOCD: CONNECTED (%1)").arg(probe));
        m_statusBadge->setStyleSheet(QString("color: %1; font-weight: bold; padding: 5px 12px; background: %2; border: 1px solid %1; border-radius: %3px; font-size: 12px;").arg(Theme::Success, Theme::SuccessBg, QString::number(Theme::RadiusMd)));
        m_connectBtn->setEnabled(false);
        m_disconnectBtn->setEnabled(true);
    } else if (state == ConnectionState::SimulationMode || m_bridge.isSimulationMode()) {
        m_statusBadge->setText("○ OpenOCD: NOT FOUND (Simulated Test Mode)");
        m_statusBadge->setStyleSheet(QString("color: %1; font-weight: bold; padding: 5px 12px; background: %2; border: 1px solid %1; border-radius: %3px; font-size: 12px;").arg(Theme::Info, Theme::InfoBg, QString::number(Theme::RadiusMd)));
        m_connectBtn->setEnabled(true);
        m_disconnectBtn->setEnabled(false);
    } else if (state == ConnectionState::Connecting) {
        m_statusBadge->setText("⏳ Connecting to target...");
        m_statusBadge->setStyleSheet(QString("color: %1; font-weight: bold; padding: 5px 12px; background: %2; border: 1px solid %1; border-radius: %3px; font-size: 12px;").arg(Theme::Warning, Theme::WarningBg, QString::number(Theme::RadiusMd)));
        m_connectBtn->setEnabled(false);
        m_disconnectBtn->setEnabled(true);
    } else if (state == ConnectionState::ConnectionFailed) {
        m_statusBadge->setText(QString("⚠ Failed: %1").arg(m_bridge.lastConnectionError().isEmpty() ? "Connection failed" : m_bridge.lastConnectionError()));
        m_statusBadge->setStyleSheet(QString("color: %1; font-weight: bold; padding: 5px 12px; background: %2; border: 1px solid %1; border-radius: %3px; font-size: 12px;").arg(Theme::Danger, Theme::DangerBg, QString::number(Theme::RadiusMd)));
        m_connectBtn->setEnabled(true);
        m_disconnectBtn->setEnabled(false);
    } else if (state == ConnectionState::DeviceRemoved) {
        m_statusBadge->setText("⚠ Device Removed / Unplugged");
        m_statusBadge->setStyleSheet(QString("color: %1; font-weight: bold; padding: 5px 12px; background: %2; border: 1px solid %1; border-radius: %3px; font-size: 12px;").arg(Theme::Warning, Theme::WarningBg, QString::number(Theme::RadiusMd)));
        m_connectBtn->setEnabled(true);
        m_disconnectBtn->setEnabled(false);
    } else if (m_bridge.isProbeDetected()) {
        QString detectedName = m_bridge.detectedBoardName();
        if (detectedName.isEmpty()) detectedName = "ST-Link Debug Probe";
        m_statusBadge->setText(QString("⚡ Board detected: %1").arg(detectedName));
        m_statusBadge->setStyleSheet(QString("color: %1; font-weight: bold; padding: 5px 12px; background: %2; border: 1px solid %1; border-radius: %3px; font-size: 12px;").arg(Theme::Info, Theme::InfoBg, QString::number(Theme::RadiusMd)));
        m_connectBtn->setEnabled(true);
        m_disconnectBtn->setEnabled(false);
    } else {
        m_statusBadge->setText("No probe connected (OpenOCD: NOT FOUND)");
        m_statusBadge->setStyleSheet(QString("color: %1; font-weight: bold; padding: 5px 12px; background: %2; border: 1px solid %1; border-radius: %3px; font-size: 12px;").arg(Theme::Warning, Theme::WarningBg, QString::number(Theme::RadiusMd)));
        m_connectBtn->setEnabled(true);
        m_disconnectBtn->setEnabled(false);
    }
}

void PinBindingDialog::updateConflictBanner() {
    QList<PinConflict> conflicts = m_bridge.allConflicts();
    if (conflicts.isEmpty()) {
        m_conflictBanner->setVisible(false);
        return;
    }

    QStringList msgList;
    for (const auto& c : conflicts) {
        msgList.append(QString("• <b>%1 (%2)</b>: %3").arg(c.pin, c.conflictingTask, c.reason));
    }

    m_conflictBanner->setText(QString("<b>⚠ Hardware Pin Assignment Conflicts (%1):</b><br/>%2")
        .arg(conflicts.size()).arg(msgList.join("<br/>")));
    m_conflictBanner->setStyleSheet(QString(
        "background-color: %1; color: %2; border: 1px solid %2; border-radius: %3px; padding: 8px 12px; font-size: 12px;"
    ).arg(Theme::DangerBg, Theme::Danger, QString::number(Theme::RadiusMd)));
    m_conflictBanner->setVisible(true);
}

void PinBindingDialog::refreshTable() {
    BoardDef activeBoard = BoardRegistry::instance().activeBoard();
    QString curId = m_boardCombo ? m_boardCombo->currentData().toString() : m_bridge.currentBoard().id;
    if (!curId.isEmpty() && curId != activeBoard.id) {
        BoardDef b = BoardRegistry::instance().board(curId);
        if (b.isValid()) activeBoard = b;
    }
    QStringList boardPins = activeBoard.pinNames();
    if (boardPins.isEmpty() || (!m_bridge.currentBoard().id.isEmpty() && m_bridge.currentBoard().id == curId)) {
        boardPins = m_bridge.availablePins();
    }

    // H1: when not in advanced mode, offer only the pins that have a signal
    // derived from the project's pin/peripheral plan.
    QSettings settings("EmbeddedUIDesigner", "PinBindingDialog");
    bool advanced = settings.value("showAllBoardPins", false).toBool();
    if (m_advancedPinsToggle) advanced = m_advancedPinsToggle->isChecked();
    QStringList signalPins;
    if (m_project) {
        Hardware::HardwareConfig hc = m_project->hardwareConfig();
        if (hc.signalList.isEmpty()) hc.deriveSignals();
        for (const Hardware::Signal& s : hc.signalList) if (!s.pin.isEmpty() && !signalPins.contains(s.pin)) signalPins.append(s.pin);
    }
    if (!advanced && !signalPins.isEmpty()) {
        boardPins = signalPins;
    }

    // No signals -> show a "Configure pins first" message.

    QStringList filteredPins;
    for (const QString& pin : boardPins) {
        const PinProfile prof = m_bridge.pinProfile(pin);
        const PinDef pd = activeBoard.pinMap.value(pin);
        if (m_selectedPort != "All Ports" && !prof.portGroup.isEmpty() && prof.portGroup != m_selectedPort) {
            continue;
        }
        if (!m_searchQuery.isEmpty()) {
            bool matches = pin.contains(m_searchQuery, Qt::CaseInsensitive) ||
                           pd.function.contains(m_searchQuery, Qt::CaseInsensitive) ||
                           pd.note.contains(m_searchQuery, Qt::CaseInsensitive) ||
                           prof.description.contains(m_searchQuery, Qt::CaseInsensitive) ||
                           prof.supportedPeripherals.join(" ").contains(m_searchQuery, Qt::CaseInsensitive);
            if (!matches) continue;
        }
        filteredPins.append(pin);
    }

    std::sort(filteredPins.begin(), filteredPins.end());
    m_pinTable->setRowCount(filteredPins.size());

    // Gather candidate canvas components
    QList<UIComponent*> candidateComps;
    if (m_scene) {
        candidateComps = m_scene->uiComponents();
    }

    const QList<PinMode> generalModes = {
        PinMode::None,
        PinMode::DigitalIn,
        PinMode::DigitalOut,
        PinMode::InputPullUp,
        PinMode::InputPullDown,
        PinMode::OpenDrainOut,
        PinMode::AnalogIn,
        PinMode::PwmOutput,
        PinMode::AlternateFunction,
        PinMode::InterruptInput,
        PinMode::SpiRawTransfer,
        PinMode::I2cBus,
        PinMode::UartTxRx,
        PinMode::CanBus
    };

    for (int r = 0; r < filteredPins.size(); ++r) {
        const QString pinName = filteredPins.at(r);
        const PinProfile prof = m_bridge.pinProfile(pinName);
        const PinDef pd = activeBoard.pinMap.value(pinName);
        const PinConflict conflict = m_bridge.checkPinConflict(pinName);

        // Column 0: Pin Name with function and markings
        QString pinDisplay = pinName;
        if (!pd.function.isEmpty()) pinDisplay += QString(" (%1)").arg(pd.function);
        if (pd.strapping) pinDisplay += " [Strapping]";
        else if (pd.reserved || prof.isReserved) pinDisplay += " [Reserved]";

        auto* pinItem = new QTableWidgetItem(pinDisplay);
        pinItem->setData(Qt::UserRole, pinName);
        pinItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pinItem->setFont(QFont("Monospace", 9, QFont::Bold));

        QString tip = QString("Pin: %1\nDefault Function: %2\nPort Group: %3")
            .arg(pinName, pd.function.isEmpty() ? "GPIO" : pd.function, prof.portGroup.isEmpty() ? "Default" : prof.portGroup);
        if (pd.reserved || prof.isReserved) tip += QString("\nRESERVED: %1").arg(pd.note.isEmpty() ? prof.reservedReason : pd.note);
        if (pd.strapping) tip += QString("\nSTRAPPING PIN (Boot Mode)");
        if (!prof.supportedPeripherals.isEmpty()) tip += QString("\nPeripherals: %1").arg(prof.supportedPeripherals.join(", "));
        pinItem->setToolTip(tip);

        if (conflict.hasConflict) {
            pinItem->setForeground(QBrush(Theme::cDanger()));
        } else if (pd.reserved || prof.isReserved || pd.strapping) {
            pinItem->setForeground(QBrush(Theme::cWarning()));
        } else {
            pinItem->setForeground(QBrush(Theme::cInfo()));
        }
        m_pinTable->setItem(r, 0, pinItem);

        // Helper lambda to configure mode combo
        auto createModeCombo = [this, &generalModes, &prof, &pd, &pinName](PinMode currentMode, bool isEnabledContext) -> QComboBox* {
            auto* combo = new QComboBox(m_pinTable);
            combo->setStyleSheet(QString(
                "QComboBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 2px 6px; font-size: 11px; }"
                "QComboBox:disabled { background-color: %5; color: %6; border-color: %3; }"
            ).arg(Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle, QString::number(Theme::RadiusSm),
                  Theme::BgBase, Theme::TextDisabled));
            combo->setEnabled(isEnabledContext);

            for (PinMode m : generalModes) {
                bool available = prof.supportedModes.contains(m) || (m == PinMode::None);
                if (pd.function == "ADC" && m == PinMode::AnalogIn) available = true;
                if (pd.function == "PWM" && m == PinMode::PwmOutput) available = true;
                if (pd.function == "SPI" && m == PinMode::SpiRawTransfer) available = true;
                if (pd.function == "I2C" && m == PinMode::I2cBus) available = true;
                if (pd.function == "UART" && m == PinMode::UartTxRx) available = true;

                QString label = pinModeToString(m);
                if (!available) {
                    label += " [Unavailable]";
                }
                combo->addItem(label, static_cast<int>(m));
                if (!available) {
                    auto* model = qobject_cast<QStandardItemModel*>(combo->model());
                    if (model) {
                        auto* item = model->item(combo->count() - 1);
                        if (item) item->setEnabled(false);
                    }
                }
            }

            const int modeIdx = combo->findData(static_cast<int>(currentMode));
            if (modeIdx >= 0) combo->setCurrentIndex(modeIdx);
            return combo;
        };

        // Column 1: Task A Mode
        bool enableTaskA = (m_selectedTask == "Task A" || m_selectedTask == "All Tasks");
        auto* modeACombo = createModeCombo(prof.taskAMode, enableTaskA);
        connect(modeACombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this, r](int index) { onTaskAModeChanged(r, index); });
        m_pinTable->setCellWidget(r, 1, modeACombo);

        // Column 2: Task B Mode
        bool enableTaskB = (m_selectedTask == "Task B" || m_selectedTask == "All Tasks");
        auto* modeBCombo = createModeCombo(prof.taskBMode, enableTaskB);
        connect(modeBCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this, r](int index) { onTaskBModeChanged(r, index); });
        m_pinTable->setCellWidget(r, 2, modeBCombo);

        // Column 3: Peripheral / Function
        auto* periphCombo = new QComboBox(m_pinTable);
        periphCombo->setStyleSheet(QString(
            "QComboBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 2px 6px; font-size: 11px; }"
        ).arg(Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle, QString::number(Theme::RadiusSm)));
        periphCombo->addItem(QString("(Default: %1)").arg(pd.function.isEmpty() ? "GPIO" : pd.function), "GPIO");
        for (const QString& periph : prof.supportedPeripherals) {
            periphCombo->addItem(periph, periph);
        }
        for (const QString& alt : pd.alternates) {
            if (periphCombo->findText(alt) < 0) {
                periphCombo->addItem(alt, alt);
            }
        }

        QString activePeriph = (m_selectedTask == "Task B") ? prof.taskBPeripheral : prof.taskAPeripheral;
        if (activePeriph.isEmpty()) activePeriph = "GPIO";
        int pIdx = periphCombo->findData(activePeriph);
        if (pIdx >= 0) periphCombo->setCurrentIndex(pIdx);
        connect(periphCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this, r](int index) { onPeripheralChanged(r, index); });
        m_pinTable->setCellWidget(r, 3, periphCombo);

        // Column 4: Canvas Component Binding
        auto* compCombo = new QComboBox(m_pinTable);
        compCombo->setStyleSheet(QString(
            "QComboBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 2px 6px; font-size: 11px; }"
        ).arg(Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle, QString::number(Theme::RadiusSm)));
        compCombo->addItem("(None)", "");
        QString boundCompId = (m_selectedTask == "Task B") ? prof.taskBComponentId : prof.taskAComponentId;

        for (UIComponent* comp : candidateComps) {
            if (!comp) continue;
            // H1b compatibility: only offer components whose needs match the pin.
            const QString ctype = comp->componentType().toLower();
            bool compatible = true;
            if (ctype.contains("switch") || ctype.contains("button")) {
                compatible = prof.supportedModes.contains(PinMode::DigitalOut) || prof.supportedModes.contains(PinMode::OpenDrainOut);
            } else if (ctype.contains("indicator") || ctype.contains("led")) {
                compatible = prof.supportedModes.contains(PinMode::DigitalIn) || prof.supportedModes.contains(PinMode::DigitalOut);
            } else if (ctype.contains("gauge") || ctype.contains("progress") || ctype.contains("label")) {
                compatible = prof.supportedModes.contains(PinMode::AnalogIn) || prof.supportedModes.contains(PinMode::DigitalIn);
            } else if (ctype.contains("slider")) {
                compatible = prof.supportedModes.contains(PinMode::PwmOutput);
            }
            if (!compatible) continue;

            const QString compText = QString("%1 (%2)").arg(comp->componentId(), comp->componentType());
            compCombo->addItem(compText, comp->componentId());
            if (!boundCompId.isEmpty() && comp->componentId() == boundCompId) {
                compCombo->setCurrentIndex(compCombo->count() - 1);
            } else if (comp->protocolPin("pin") == pinName || comp->protocolPin("sck") == pinName || comp->protocolPin("scl") == pinName) {
                compCombo->setCurrentIndex(compCombo->count() - 1);
            }
        }
        connect(compCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this, r](int index) { onComponentBindingChanged(r, index); });
        m_pinTable->setCellWidget(r, 4, compCombo);

        // Column 5: Live Value
        updateRowLiveValue(r, pinName, prof);

        // Column 6: Status / Conflict Badge
        QString statusText = "✓ Ready";
        QColor statusColor = Theme::cSuccess();

        if (conflict.hasConflict) {
            statusText = "✗ Conflict";
            statusColor = Theme::cDanger();
        } else if (pd.reserved || prof.isReserved) {
            statusText = "⚠ Reserved";
            statusColor = Theme::cWarning();
        } else if (pd.strapping) {
            statusText = "⚡ Strapping";
            statusColor = Theme::cWarning();
        } else if (prof.taskAMode != PinMode::None || prof.taskBMode != PinMode::None) {
            statusText = "● Active";
            statusColor = Theme::cInfo();
        }

        auto* statusItem = new QTableWidgetItem(statusText);
        statusItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        statusItem->setTextAlignment(Qt::AlignCenter);
        statusItem->setForeground(QBrush(statusColor));
        statusItem->setFont(QFont("sans-serif", 9, QFont::Bold));
        if (conflict.hasConflict) statusItem->setToolTip(conflict.reason);
        m_pinTable->setItem(r, 6, statusItem);
    }

    // Synchronize PinMuxEngine for visual pinout diagram
    Hardware::DeviceDefinition dev;
    dev.partNumber = activeBoard.mcu.isEmpty() ? activeBoard.displayName : activeBoard.mcu;
    dev.vendor = activeBoard.vendor;
    dev.family = activeBoard.family;
    dev.package = "LQFP64";
    for (auto it = activeBoard.pinMap.begin(); it != activeBoard.pinMap.end(); ++it) {
        Hardware::PinDefinition d;
        d.name = it.key();
        d.defaultFunction = it.value().function;
        d.alternateFunctions = it.value().alternates;
        d.isReserved = it.value().reserved;
        d.isBoot = it.value().strapping;
        if (it.value().function == "POWER") d.isPower = true;
        if (it.value().function == "GND") d.isGround = true;
        dev.pins.append(d);
    }
    m_pinMuxEngine.setDevice(dev);

    // Highlight bound pins from components & bridge
    for (const QString& p : activeBoard.pinMap.keys()) {
        PinMode mode = m_bridge.taskPinMode(p, "Task A");
        if (mode != PinMode::None) {
            Hardware::PinConfiguration cfg;
            cfg.pin = p;
            cfg.mode = pinModeToString(mode);
            m_pinMuxEngine.assignPin(p, cfg, true);
        }
    }
    if (m_scene) {
        for (UIComponent* comp : m_scene->uiComponents()) {
            if (!comp) continue;
            QString proto = comp->protocol();
            if (proto.isEmpty() || proto == "None") continue;
            QStringList bound;
            if (proto == "GPIO" || proto == "PWM" || proto == "ADC") bound << comp->protocolPin("pin");
            else if (proto == "SPI") bound << comp->protocolPin("miso") << comp->protocolPin("mosi") << comp->protocolPin("sck") << comp->protocolPin("ss");
            else if (proto == "I2C") bound << comp->protocolPin("scl") << comp->protocolPin("sda");
            for (const QString& bp : bound) {
                if (!bp.isEmpty() && bp != "[None]") {
                    Hardware::PinConfiguration cfg;
                    cfg.pin = bp;
                    cfg.mode = proto;
                    m_pinMuxEngine.assignPin(bp, cfg, true);
                }
            }
        }
    }
    if (m_pinoutWidget) m_pinoutWidget->update();
}

void PinBindingDialog::updateRowLiveValue(int row, const QString& pinName, const PinProfile& prof) {
    QString valStr = "-";
    PinMode activeMode = (m_selectedTask == "Task B") ? prof.taskBMode : prof.taskAMode;
    if (activeMode == PinMode::None) activeMode = prof.activeMode;

    bool isConnectedOrSim = m_bridge.isHardwareConnected() || m_bridge.isSimulationMode();

    if (isConnectedOrSim && activeMode != PinMode::None) {
        if (activeMode == PinMode::AnalogIn) {
            quint32 raw = 0;
            m_bridge.pollAdc(pinName, &raw);
            int bits = prof.adc.resolutionBits > 0 ? prof.adc.resolutionBits : 12;
            valStr = QString("Raw: %1 / %2").arg(raw).arg((1u << bits) - 1);
        } else if (activeMode == PinMode::PwmOutput) {
            valStr = "PWM: 50.0%";
        } else if (activeMode == PinMode::SpiRawTransfer) {
            valStr = "SPI: Ready";
        } else if (activeMode == PinMode::DigitalOut || activeMode == PinMode::DigitalIn) {
            bool high = false;
            m_bridge.readDigitalIn(pinName, &high);
            valStr = high ? "HIGH (1)" : "LOW (0)";
        } else {
            valStr = "Active";
        }
    } else if (!isConnectedOrSim) {
        valStr = "Disconnected";
    }

    auto* valItem = new QTableWidgetItem(valStr);
    valItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    valItem->setTextAlignment(Qt::AlignCenter);
    valItem->setFont(QFont("Monospace", 9));
    valItem->setForeground(QBrush(isConnectedOrSim ? Theme::cTextPrimary() : Theme::cTextMuted()));
    m_pinTable->setItem(row, 5, valItem);
}

void PinBindingDialog::onBoardChanged(int index) {
    const QString boardId = m_boardCombo->itemData(index).toString();
    BoardDef newBoard = BoardRegistry::instance().board(boardId);

    // Conflict detection across current component bindings
    QList<InvalidBindingItem> invalidItems;
    if (m_scene) {
        for (UIComponent* comp : m_scene->uiComponents()) {
            if (!comp) continue;
            QString proto = comp->protocol();
            if (proto.isEmpty() || proto == "None") continue;

            QStringList pinsToCheck;
            if (proto == "GPIO" || proto == "PWM" || proto == "ADC") {
                pinsToCheck << comp->protocolPin("pin");
            } else if (proto == "SPI") {
                pinsToCheck << comp->protocolPin("miso") << comp->protocolPin("mosi")
                            << comp->protocolPin("sck") << comp->protocolPin("ss");
            } else if (proto == "I2C") {
                pinsToCheck << comp->protocolPin("scl") << comp->protocolPin("sda");
            }

            for (const QString& p : pinsToCheck) {
                if (p.isEmpty() || p == "[None]") continue;
                if (!newBoard.pinMap.contains(p)) {
                    InvalidBindingItem item;
                    item.component = comp;
                    item.componentId = comp->componentId();
                    item.componentName = comp->componentId();
                    item.currentPin = p;
                    item.currentProtocol = proto;
                    item.reason = QString("Pin '%1' not supported on target '%2'").arg(p, newBoard.displayName.isEmpty() ? newBoard.id : newBoard.displayName);
                    invalidItems.append(item);
                } else if (newBoard.pinMap[p].reserved) {
                    InvalidBindingItem item;
                    item.component = comp;
                    item.componentId = comp->componentId();
                    item.componentName = comp->componentId();
                    item.currentPin = p;
                    item.currentProtocol = proto;
                    item.reason = QString("Pin '%1' is reserved for system use (%2)").arg(p, newBoard.pinMap[p].note.isEmpty() ? "System" : newBoard.pinMap[p].note);
                    invalidItems.append(item);
                }
            }
        }
    }

    if (!invalidItems.isEmpty()) {
        InvalidBindingsDialog invDlg(newBoard, invalidItems, this);
        if (invDlg.exec() == QDialog::Accepted) {
            auto resolved = invDlg.resolvedBindings();
            for (const auto& item : resolved) {
                if (item.component && !item.keepAsCustom && !item.remappedPin.isEmpty()) {
                    QString proto = item.currentProtocol;
                    if (proto == "GPIO" || proto == "PWM" || proto == "ADC") {
                        item.component->setProtocolPin("pin", item.remappedPin);
                        m_bridge.bindComponent(item.component, item.remappedPin,
                            proto == "ADC" ? PinMode::AnalogIn : (proto == "PWM" ? PinMode::PwmOutput : PinMode::DigitalOut));
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

    BoardRegistry::instance().setActiveBoard(boardId);
    m_bridge.setBoard(boardId);
    populatePortFilterCombo();
    refreshTable();
    updateStatusBadge();
    updateConflictBanner();
}

void PinBindingDialog::onTaskFilterChanged(int id) {
    if (id == 0) m_selectedTask = "Task A";
    else if (id == 1) m_selectedTask = "Task B";
    else m_selectedTask = "All Tasks";

    m_bridge.setActiveTask(m_selectedTask);
    refreshTable();
}

void PinBindingDialog::onPortFilterChanged(int index) {
    m_selectedPort = m_portFilterCombo->itemText(index);
    refreshTable();
}

void PinBindingDialog::onSearchTextChanged(const QString& text) {
    m_searchQuery = text.trimmed();
    refreshTable();
}

void PinBindingDialog::onTaskAModeChanged(int row, int index) {
    if (row < 0 || row >= m_pinTable->rowCount()) return;
    const QString pinName = m_pinTable->item(row, 0)->data(Qt::UserRole).toString().isEmpty()
        ? m_pinTable->item(row, 0)->text().split(' ').first()
        : m_pinTable->item(row, 0)->data(Qt::UserRole).toString();

    BoardDef board = BoardRegistry::instance().activeBoard();
    const PinDef pd = board.pinMap.value(pinName);
    if (pd.reserved || pd.strapping) {
        QString tag = pd.reserved ? "Reserved" : "Strapping";
        auto reply = QMessageBox::warning(
            this,
            QString("%1 Pin Warning").arg(tag),
            QString("Pin '%1' is designated as %2 (%3).\n"
                    "Binding this pin may interfere with flashing, debug, or boot operations.\n\n"
                    "Are you sure you want to proceed?")
                .arg(pinName, tag, pd.note.isEmpty() ? pd.function : pd.note),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            refreshTable();
            return;
        }
    }

    auto* combo = qobject_cast<QComboBox*>(m_pinTable->cellWidget(row, 1));
    if (!combo) return;

    const PinMode mode = static_cast<PinMode>(combo->itemData(index).toInt());
    if (!m_bridge.setTaskPinMode(pinName, "Task A", mode)) {
        const PinMode cur = m_bridge.taskPinMode(pinName, "Task A");
        combo->blockSignals(true);
        combo->setCurrentIndex(combo->findData(static_cast<int>(cur)));
        combo->blockSignals(false);
        QMessageBox::warning(this, "Mode Unavailable",
            QString("Mode '%1' is not available on pin %2 for the selected board.").arg(pinModeToString(mode), pinName));
    }

    updateConflictBanner();
    refreshTable();
}

void PinBindingDialog::onTaskBModeChanged(int row, int index) {
    if (row < 0 || row >= m_pinTable->rowCount()) return;
    const QString pinName = m_pinTable->item(row, 0)->data(Qt::UserRole).toString().isEmpty()
        ? m_pinTable->item(row, 0)->text().split(' ').first()
        : m_pinTable->item(row, 0)->data(Qt::UserRole).toString();

    BoardDef board = BoardRegistry::instance().activeBoard();
    const PinDef pd = board.pinMap.value(pinName);
    if (pd.reserved || pd.strapping) {
        QString tag = pd.reserved ? "Reserved" : "Strapping";
        auto reply = QMessageBox::warning(
            this,
            QString("%1 Pin Warning").arg(tag),
            QString("Pin '%1' is designated as %2 (%3).\n"
                    "Binding this pin may interfere with flashing, debug, or boot operations.\n\n"
                    "Are you sure you want to proceed?")
                .arg(pinName, tag, pd.note.isEmpty() ? pd.function : pd.note),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            refreshTable();
            return;
        }
    }

    auto* combo = qobject_cast<QComboBox*>(m_pinTable->cellWidget(row, 2));
    if (!combo) return;

    const PinMode mode = static_cast<PinMode>(combo->itemData(index).toInt());
    if (!m_bridge.setTaskPinMode(pinName, "Task B", mode)) {
        const PinMode cur = m_bridge.taskPinMode(pinName, "Task B");
        combo->blockSignals(true);
        combo->setCurrentIndex(combo->findData(static_cast<int>(cur)));
        combo->blockSignals(false);
        QMessageBox::warning(this, "Mode Unavailable",
            QString("Mode '%1' is not available on pin %2 for the selected board.").arg(pinModeToString(mode), pinName));
    }

    updateConflictBanner();
    refreshTable();
}

void PinBindingDialog::onPeripheralChanged(int row, int index) {
    if (row < 0 || row >= m_pinTable->rowCount()) return;
    const QString pinName = m_pinTable->item(row, 0)->data(Qt::UserRole).toString().isEmpty()
        ? m_pinTable->item(row, 0)->text().split(' ').first()
        : m_pinTable->item(row, 0)->data(Qt::UserRole).toString();
    auto* combo = qobject_cast<QComboBox*>(m_pinTable->cellWidget(row, 3));
    if (!combo) return;

    const QString periph = combo->itemData(index).toString();
    QString task = (m_selectedTask == "Task B") ? "Task B" : "Task A";
    m_bridge.setTaskPeripheral(pinName, task, periph);

    updateConflictBanner();
}

void PinBindingDialog::onComponentBindingChanged(int row, int index) {
    if (row < 0 || row >= m_pinTable->rowCount()) return;
    const QString pinName = m_pinTable->item(row, 0)->data(Qt::UserRole).toString().isEmpty()
        ? m_pinTable->item(row, 0)->text().split(' ').first()
        : m_pinTable->item(row, 0)->data(Qt::UserRole).toString();

    auto* compCombo = qobject_cast<QComboBox*>(m_pinTable->cellWidget(row, 4));
    if (!compCombo) return;

    const QString compId = compCombo->itemData(index).toString();

    if (!compId.isEmpty()) {
        BoardDef board = BoardRegistry::instance().activeBoard();
        const PinDef pd = board.pinMap.value(pinName);
        if (pd.reserved || pd.strapping) {
            QString tag = pd.reserved ? "Reserved" : "Strapping";
            auto reply = QMessageBox::warning(
                this,
                QString("%1 Pin Warning").arg(tag),
                QString("Pin '%1' is designated as %2 (%3).\n"
                        "Binding this pin may interfere with flashing, debug, or boot operations.\n\n"
                        "Are you sure you want to proceed?")
                    .arg(pinName, tag, pd.note.isEmpty() ? pd.function : pd.note),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
            if (reply != QMessageBox::Yes) {
                refreshTable();
                return;
            }
        }
    }

    QString task = (m_selectedTask == "Task B") ? "Task B" : "Task A";

    if (compId.isEmpty()) {
        m_bridge.setTaskComponentBinding(pinName, task, "");
        m_bridge.unbindPin(pinName);
    } else if (m_scene) {
        for (UIComponent* comp : m_scene->uiComponents()) {
            if (comp && comp->componentId() == compId) {
                PinMode mode = m_bridge.taskPinMode(pinName, task);
                if (mode == PinMode::None) {
                    if (m_bridge.isModeAvailable(pinName, PinMode::AnalogIn)) mode = PinMode::AnalogIn;
                    else if (m_bridge.isModeAvailable(pinName, PinMode::PwmOutput)) mode = PinMode::PwmOutput;
                    else mode = PinMode::DigitalOut;
                    m_bridge.setTaskPinMode(pinName, task, mode);
                }
                m_bridge.bindComponent(comp, pinName, mode);
                break;
            }
        }
    }

    refreshTable();
}

void PinBindingDialog::onSaveCustomPresetClicked() {
    BoardDef activeBoard = BoardRegistry::instance().activeBoard();
    bool ok = false;
    QString defaultName = (activeBoard.displayName.isEmpty() ? activeBoard.id : activeBoard.displayName) + " Custom Preset";
    QString name = QInputDialog::getText(this, "Save Custom Pin Preset",
                                         "Preset Name:", QLineEdit::Normal,
                                         defaultName, &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QMap<QString, PinDef> pinOverrides;
    for (int r = 0; r < m_pinTable->rowCount(); ++r) {
        if (!m_pinTable->item(r, 0)) continue;
        QString pin = m_pinTable->item(r, 0)->data(Qt::UserRole).toString();
        if (pin.isEmpty()) pin = m_pinTable->item(r, 0)->text().split(' ').first();

        auto* aCombo = qobject_cast<QComboBox*>(m_pinTable->cellWidget(r, 1));
        if (aCombo && aCombo->currentText() != "None" && !aCombo->currentText().contains("Unavailable")) {
            PinDef pd = activeBoard.pinMap.value(pin);
            pd.function = aCombo->currentText();
            pinOverrides[pin] = pd;
        }
    }

    BoardRegistry::instance().savePreset(name.trimmed(), activeBoard.id, pinOverrides);
    QMessageBox::information(this, "Preset Saved",
        QString("Custom pin configuration preset '%1' has been saved successfully.").arg(name.trimmed()));
}

void PinBindingDialog::onConnectClicked() {
    bool ok = m_bridge.connectHardware();
    if (!ok) {
        QMessageBox::critical(this, "Hardware Connection Failed",
            QString("Failed to connect to target:\n%1").arg(m_bridge.lastConnectionError()));
    }
    updateStatusBadge();
    refreshTable();
}

void PinBindingDialog::onDisconnectClicked() {
    m_bridge.disconnectHardware();
    updateStatusBadge();
    refreshTable();
}

void PinBindingDialog::onDetectProbeClicked() {
    bool found = m_bridge.detectProbe();
    updateStatusBadge();
    if (found) {
        QMessageBox::information(this, "Probe Detected",
            QString("Debug Probe detected: %1").arg(m_bridge.detectedBoardName()));
    } else {
        QMessageBox::warning(this, "No Probe Detected",
            "No compatible USB debug probe / hardware interface was found.");
    }
}

void PinBindingDialog::onSimulationToggleClicked(bool checked) {
    m_bridge.setSimulationMode(checked);
    updateStatusBadge();
    refreshTable();
}

void PinBindingDialog::onHardwareBridgeUpdate() {
    for (int r = 0; r < m_pinTable->rowCount(); ++r) {
        auto* pinItem = m_pinTable->item(r, 0);
        if (!pinItem) continue;
        const QString pinName = pinItem->text();
        const PinProfile prof = m_bridge.pinProfile(pinName);
        updateRowLiveValue(r, pinName, prof);
    }
}

void PinBindingDialog::onImportBoardConfigClicked() {
    QString filter = "Board Config Files (*.ioc sdkconfig* *.h *.ini *.txt);;STM32CubeMX (*.ioc);;ESP-IDF (sdkconfig*);;All Files (*)";
    QString filePath = QFileDialog::getOpenFileName(this, "Import Board Configuration (.ioc / sdkconfig)", QString(), filter);
    if (filePath.isEmpty()) return;

    auto res = BoardConfigParser::parseFile(filePath);
    if (!res.success) {
        QMessageBox::warning(this, "Board Configuration Import Failed", res.errorMessage);
        return;
    }

    BoardProfile bp = res.toBoardProfile();
    m_bridge.addBoard(bp);
    m_bridge.setBoard(bp.id);

    m_boardCombo->blockSignals(true);
    m_boardCombo->clear();
    for (const auto& b : m_bridge.availableBoards()) {
        m_boardCombo->addItem(QString("%1 (%2)").arg(b.name, b.mcuFamily), b.id);
    }
    const int curIdx = m_boardCombo->findData(bp.id);
    if (curIdx >= 0) m_boardCombo->setCurrentIndex(curIdx);
    m_boardCombo->blockSignals(false);

    populatePortFilterCombo();
    refreshTable();
    updateConflictBanner();
}
