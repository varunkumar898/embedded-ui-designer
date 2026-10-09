#pragma once

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDateTime>
#include <QList>
#include <QVariant>
#include "TelemetryProtocol.h"

class Project;

struct TelemetryLogEntry {
    QDateTime timestamp;
    QString direction; // "RX" or "TX"
    QString sourceId;
    QString dataType;
    QVariant value;
    uint32_t sequence = 0;
    bool isValid = true;
    QString errorMessage;
    QString rawMessage;
};

class DeviceMonitorRuntime : public QObject {
    Q_OBJECT

public:
    explicit DeviceMonitorRuntime(Project* project = nullptr, QObject* parent = nullptr);
    ~DeviceMonitorRuntime() override;

    void setProject(Project* project) { m_project = project; }
    Project* project() const { return m_project; }

    bool isConnected() const;
    QString currentPortName() const { return m_portName; }
    qint32 currentBaudRate() const { return m_baudRate; }

    static QStringList availablePorts();

    const QList<TelemetryLogEntry>& logEntries() const { return m_logEntries; }
    size_t maxLogCapacity() const { return m_maxLogCapacity; }
    void setMaxLogCapacity(size_t cap) { m_maxLogCapacity = cap; }
    void clearLog();

    bool exportCsv(const QString& filePath, QString* errorMsg = nullptr) const;

public slots:
    bool connectDevice(const QString& portName, qint32 baudRate = 115200);
    void disconnectDevice();
    bool sendCommand(const QString& sourceId, TelemetryDataType type, const QVariant& value);

signals:
    void connectionStateChanged(bool connected, const QString& portName);
    void frameReceived(const TelemetryFrame& frame);
    void logEntryAdded(const TelemetryLogEntry& entry);
    void errorOccurred(const QString& errorMsg);

private slots:
    void onSerialReadyRead();
    void onSerialErrorOccurred(QSerialPort::SerialPortError error);

private:
    Project* m_project = nullptr;
    QSerialPort* m_serialPort = nullptr;
    TelemetryProtocol m_protocol;
    QString m_portName;
    qint32 m_baudRate = 115200;
    uint32_t m_txSequence = 0;

    size_t m_maxLogCapacity = 5000;
    QList<TelemetryLogEntry> m_logEntries;

    void appendLogEntry(const TelemetryLogEntry& entry);
    void updateProjectDataSource(const TelemetryFrame& frame);
};
