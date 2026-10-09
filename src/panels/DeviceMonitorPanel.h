#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QSplitter>
#include "telemetry/DeviceMonitorRuntime.h"

class Project;

class DeviceMonitorPanel : public QWidget {
    Q_OBJECT

public:
    explicit DeviceMonitorPanel(Project* project, QWidget* parent = nullptr);
    ~DeviceMonitorPanel() override = default;

    void setProject(Project* project);
    DeviceMonitorRuntime* runtime() { return &m_runtime; }

public slots:
    void refreshPortList();
    void onConnectClicked();
    void onSendClicked();
    void onExportCsvClicked();
    void onClearLogClicked();

private slots:
    void onConnectionStateChanged(bool connected, const QString& portName);
    void onFrameReceived(const TelemetryFrame& frame);
    void onLogEntryAdded(const TelemetryLogEntry& entry);
    void onErrorOccurred(const QString& errorMsg);

private:
    void setupUi();
    void updateDataSourceRow(const QString& sourceId, const QString& dataType, const QVariant& value);

    Project* m_project = nullptr;
    DeviceMonitorRuntime m_runtime;

    // Controls
    QComboBox* m_portCombo = nullptr;
    QPushButton* m_refreshPortsBtn = nullptr;
    QComboBox* m_baudCombo = nullptr;
    QPushButton* m_connectBtn = nullptr;
    QLabel* m_statusBadge = nullptr;

    // TX Controls
    QLineEdit* m_txSourceIdEdit = nullptr;
    QComboBox* m_txTypeCombo = nullptr;
    QLineEdit* m_txValueEdit = nullptr;
    QPushButton* m_sendBtn = nullptr;

    // Tables
    QTableWidget* m_dataSourcesTable = nullptr;
    QTableWidget* m_logTable = nullptr;

    // Bottom tools
    QCheckBox* m_autoScrollCheck = nullptr;
    QPushButton* m_exportCsvBtn = nullptr;
    QPushButton* m_clearLogBtn = nullptr;

    QMap<QString, int> m_dataSourceRowMap;
    QMap<QString, int> m_dataSourceUpdateCounts;
};
