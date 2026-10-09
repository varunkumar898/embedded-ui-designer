#include "DeviceMonitorPanel.h"
#include "project/Project.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>

DeviceMonitorPanel::DeviceMonitorPanel(Project* project, QWidget* parent)
    : QWidget(parent)
    , m_project(project)
    , m_runtime(project, this)
{
    setupUi();
    setProject(project);

    connect(&m_runtime, &DeviceMonitorRuntime::connectionStateChanged, this, &DeviceMonitorPanel::onConnectionStateChanged);
    connect(&m_runtime, &DeviceMonitorRuntime::frameReceived, this, &DeviceMonitorPanel::onFrameReceived);
    connect(&m_runtime, &DeviceMonitorRuntime::logEntryAdded, this, &DeviceMonitorPanel::onLogEntryAdded);
    connect(&m_runtime, &DeviceMonitorRuntime::errorOccurred, this, &DeviceMonitorPanel::onErrorOccurred);

    refreshPortList();
}

void DeviceMonitorPanel::setProject(Project* project) {
    m_project = project;
    m_runtime.setProject(project);
    m_dataSourceRowMap.clear();
    m_dataSourceUpdateCounts.clear();
    m_dataSourcesTable->setRowCount(0);

    if (m_project) {
        for (const auto& ds : m_project->dataSources()) {
            updateDataSourceRow(ds.id(), dataTypeToString(ds.dataType()), ds.value());
        }
    }
}

void DeviceMonitorPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);

    // 1. Connection Toolbar
    auto* toolBarLayout = new QHBoxLayout();

    toolBarLayout->addWidget(new QLabel("Port:", this));
    m_portCombo = new QComboBox(this);
    m_portCombo->setMinimumWidth(160);
    toolBarLayout->addWidget(m_portCombo);

    m_refreshPortsBtn = new QPushButton("↻", this);
    m_refreshPortsBtn->setToolTip("Refresh Port List");
    m_refreshPortsBtn->setFixedWidth(28);
    connect(m_refreshPortsBtn, &QPushButton::clicked, this, &DeviceMonitorPanel::refreshPortList);
    toolBarLayout->addWidget(m_refreshPortsBtn);

    toolBarLayout->addWidget(new QLabel("Baud:", this));
    m_baudCombo = new QComboBox(this);
    m_baudCombo->addItems(QStringList() << "9600" << "19200" << "38400" << "57600" << "115200" << "230400" << "460800" << "921600");
    m_baudCombo->setCurrentText("115200");
    toolBarLayout->addWidget(m_baudCombo);

    m_connectBtn = new QPushButton("Connect", this);
    m_connectBtn->setStyleSheet("background-color: #27ae60; color: white; font-weight: bold; padding: 4px 12px;");
    connect(m_connectBtn, &QPushButton::clicked, this, &DeviceMonitorPanel::onConnectClicked);
    toolBarLayout->addWidget(m_connectBtn);

    m_statusBadge = new QLabel("🔴 Disconnected", this);
    m_statusBadge->setStyleSheet("color: #e74c3c; font-weight: bold; margin-left: 8px;");
    toolBarLayout->addWidget(m_statusBadge);

    toolBarLayout->addStretch();

    m_exportCsvBtn = new QPushButton("Export CSV Log", this);
    connect(m_exportCsvBtn, &QPushButton::clicked, this, &DeviceMonitorPanel::onExportCsvClicked);
    toolBarLayout->addWidget(m_exportCsvBtn);

    m_clearLogBtn = new QPushButton("Clear Log", this);
    connect(m_clearLogBtn, &QPushButton::clicked, this, &DeviceMonitorPanel::onClearLogClicked);
    toolBarLayout->addWidget(m_clearLogBtn);

    mainLayout->addLayout(toolBarLayout);

    // Splitter for Live DataSources & Raw Traffic Stream
    auto* splitter = new QSplitter(Qt::Vertical, this);

    // Top: Live DataSources Table
    auto* dsGroup = new QGroupBox("Live Data Sources & Physical Telemetry Channels", this);
    auto* dsLayout = new QVBoxLayout(dsGroup);
    dsLayout->setContentsMargins(4, 4, 4, 4);

    m_dataSourcesTable = new QTableWidget(this);
    m_dataSourcesTable->setColumnCount(5);
    m_dataSourcesTable->setHorizontalHeaderLabels(QStringList() << "Source ID" << "Data Type" << "Live Value" << "Updates" << "Last Timestamp");
    m_dataSourcesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_dataSourcesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_dataSourcesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_dataSourcesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_dataSourcesTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_dataSourcesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_dataSourcesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_dataSourcesTable->setAlternatingRowColors(true);
    dsLayout->addWidget(m_dataSourcesTable);

    splitter->addWidget(dsGroup);

    // Bottom: Serial Traffic Stream & TX Bar
    auto* logGroup = new QGroupBox("Telemetry Protocol Stream ($TLM v1.0)", this);
    auto* logLayout = new QVBoxLayout(logGroup);
    logLayout->setContentsMargins(4, 4, 4, 4);

    m_logTable = new QTableWidget(this);
    m_logTable->setColumnCount(7);
    m_logTable->setHorizontalHeaderLabels(QStringList() << "Time" << "Dir" << "Source ID" << "Type" << "Value" << "Seq" << "Raw Frame / Status");
    m_logTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_logTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_logTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_logTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_logTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_logTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_logTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    m_logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_logTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_logTable->setAlternatingRowColors(true);
    logLayout->addWidget(m_logTable);

    // TX Control Bar
    auto* txLayout = new QHBoxLayout();
    txLayout->addWidget(new QLabel("Send Command:", this));
    m_txSourceIdEdit = new QLineEdit(this);
    m_txSourceIdEdit->setPlaceholderText("Source ID (e.g. LED1)");
    m_txSourceIdEdit->setFixedWidth(130);
    txLayout->addWidget(m_txSourceIdEdit);

    m_txTypeCombo = new QComboBox(this);
    m_txTypeCombo->addItem("BOOL", static_cast<int>(TelemetryDataType::Bool));
    m_txTypeCombo->addItem("INT", static_cast<int>(TelemetryDataType::Int));
    m_txTypeCombo->addItem("FLOAT", static_cast<int>(TelemetryDataType::Float));
    m_txTypeCombo->addItem("STR", static_cast<int>(TelemetryDataType::String));
    m_txTypeCombo->addItem("HEX", static_cast<int>(TelemetryDataType::Hex));
    txLayout->addWidget(m_txTypeCombo);

    m_txValueEdit = new QLineEdit(this);
    m_txValueEdit->setPlaceholderText("Value (e.g. 1)");
    txLayout->addWidget(m_txValueEdit);

    m_sendBtn = new QPushButton("Send TX Frame", this);
    m_sendBtn->setEnabled(false);
    connect(m_sendBtn, &QPushButton::clicked, this, &DeviceMonitorPanel::onSendClicked);
    txLayout->addWidget(m_sendBtn);

    m_autoScrollCheck = new QCheckBox("Auto-scroll", this);
    m_autoScrollCheck->setChecked(true);
    txLayout->addWidget(m_autoScrollCheck);

    logLayout->addLayout(txLayout);
    splitter->addWidget(logGroup);

    mainLayout->addWidget(splitter);
}

void DeviceMonitorPanel::refreshPortList() {
    QString current = m_portCombo->currentText();
    m_portCombo->clear();
    QStringList ports = DeviceMonitorRuntime::availablePorts();
    if (ports.isEmpty()) {
        m_portCombo->addItem("No Serial Ports Found");
    } else {
        m_portCombo->addItems(ports);
        int idx = m_portCombo->findText(current);
        if (idx >= 0) m_portCombo->setCurrentIndex(idx);
    }
}

void DeviceMonitorPanel::onConnectClicked() {
    if (m_runtime.isConnected()) {
        m_runtime.disconnectDevice();
    } else {
        QString port = m_portCombo->currentText();
        if (port.isEmpty() || port.contains("No Serial Ports")) {
            QMessageBox::warning(this, "Connection Warning", "No valid serial port selected.");
            return;
        }
        qint32 baud = m_baudCombo->currentText().toInt();
        m_runtime.connectDevice(port, baud);
    }
}

void DeviceMonitorPanel::onConnectionStateChanged(bool connected, const QString& portName) {
    if (connected) {
        m_connectBtn->setText("Disconnect");
        m_connectBtn->setStyleSheet("background-color: #c0392b; color: white; font-weight: bold; padding: 4px 12px;");
        m_statusBadge->setText(QString("🟢 Connected (%1)").arg(portName));
        m_statusBadge->setStyleSheet("color: #2ecc71; font-weight: bold; margin-left: 8px;");
        m_sendBtn->setEnabled(true);
    } else {
        m_connectBtn->setText("Connect");
        m_connectBtn->setStyleSheet("background-color: #27ae60; color: white; font-weight: bold; padding: 4px 12px;");
        m_statusBadge->setText("🔴 Disconnected");
        m_statusBadge->setStyleSheet("color: #e74c3c; font-weight: bold; margin-left: 8px;");
        m_sendBtn->setEnabled(false);
    }
}

void DeviceMonitorPanel::onSendClicked() {
    QString sourceId = m_txSourceIdEdit->text().trimmed();
    if (sourceId.isEmpty()) {
        QMessageBox::warning(this, "TX Warning", "Source ID cannot be empty.");
        return;
    }

    auto type = static_cast<TelemetryDataType>(m_txTypeCombo->currentData().toInt());
    QString rawVal = m_txValueEdit->text().trimmed();
    QVariant val;
    switch (type) {
        case TelemetryDataType::Bool:
            val = (rawVal == "1" || rawVal.compare("true", Qt::CaseInsensitive) == 0);
            break;
        case TelemetryDataType::Int:
            val = rawVal.toInt();
            break;
        case TelemetryDataType::Float:
            val = rawVal.toDouble();
            break;
        case TelemetryDataType::Hex:
            val = rawVal.startsWith("0x", Qt::CaseInsensitive) ? rawVal.mid(2).toUInt(nullptr, 16) : rawVal.toUInt(nullptr, 16);
            break;
        default:
            val = rawVal;
            break;
    }

    m_runtime.sendCommand(sourceId, type, val);
}

void DeviceMonitorPanel::onFrameReceived(const TelemetryFrame& frame) {
    if (frame.isValid) {
        updateDataSourceRow(frame.sourceId, telemetryDataTypeToString(frame.type), frame.value);
    }
}

void DeviceMonitorPanel::updateDataSourceRow(const QString& sourceId, const QString& dataType, const QVariant& value) {
    int row = -1;
    if (m_dataSourceRowMap.contains(sourceId)) {
        row = m_dataSourceRowMap[sourceId];
    } else {
        row = m_dataSourcesTable->rowCount();
        m_dataSourcesTable->insertRow(row);
        m_dataSourceRowMap[sourceId] = row;
        m_dataSourcesTable->setItem(row, 0, new QTableWidgetItem(sourceId));
        m_dataSourcesTable->setItem(row, 1, new QTableWidgetItem(dataType));
    }

    m_dataSourceUpdateCounts[sourceId]++;

    auto* valItem = new QTableWidgetItem(value.toString());
    valItem->setForeground(QColor("#2ecc71"));
    m_dataSourcesTable->setItem(row, 2, valItem);
    m_dataSourcesTable->setItem(row, 3, new QTableWidgetItem(QString::number(m_dataSourceUpdateCounts[sourceId])));
    m_dataSourcesTable->setItem(row, 4, new QTableWidgetItem(QDateTime::currentDateTime().toString("hh:mm:ss.zzz")));
}

void DeviceMonitorPanel::onLogEntryAdded(const TelemetryLogEntry& entry) {
    int row = m_logTable->rowCount();
    m_logTable->insertRow(row);

    m_logTable->setItem(row, 0, new QTableWidgetItem(entry.timestamp.toString("hh:mm:ss.zzz")));

    auto* dirItem = new QTableWidgetItem(entry.direction);
    dirItem->setForeground(entry.direction == "TX" ? QColor("#3498db") : QColor("#2ecc71"));
    m_logTable->setItem(row, 1, dirItem);

    m_logTable->setItem(row, 2, new QTableWidgetItem(entry.sourceId));
    m_logTable->setItem(row, 3, new QTableWidgetItem(entry.dataType));
    m_logTable->setItem(row, 4, new QTableWidgetItem(entry.value.toString()));
    m_logTable->setItem(row, 5, new QTableWidgetItem(QString::number(entry.sequence)));

    auto* rawItem = new QTableWidgetItem();
    if (entry.isValid) {
        rawItem->setText(entry.rawMessage);
    } else {
        rawItem->setText(QString("❌ %1 (Raw: %2)").arg(entry.errorMessage, entry.rawMessage));
        rawItem->setForeground(QColor("#e74c3c"));
    }
    m_logTable->setItem(row, 6, rawItem);

    if (m_autoScrollCheck->isChecked()) {
        m_logTable->scrollToBottom();
    }
}

void DeviceMonitorPanel::onErrorOccurred(const QString& errorMsg) {
    // Log error row
    TelemetryLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.direction = "SYS";
    entry.sourceId = "SYSTEM";
    entry.dataType = "ERR";
    entry.isValid = false;
    entry.errorMessage = errorMsg;
    onLogEntryAdded(entry);
}

void DeviceMonitorPanel::onClearLogClicked() {
    m_runtime.clearLog();
    m_logTable->setRowCount(0);
}

void DeviceMonitorPanel::onExportCsvClicked() {
    QString path = QFileDialog::getSaveFileName(this, "Export Telemetry CSV Log", "telemetry_log.csv", "CSV Files (*.csv)");
    if (!path.isEmpty()) {
        QString err;
        if (m_runtime.exportCsv(path, &err)) {
            QMessageBox::information(this, "Export Complete", QString("Telemetry log successfully exported to:\n%1").arg(path));
        } else {
            QMessageBox::critical(this, "Export Error", err);
        }
    }
}
