#include "DeviceMonitorRuntime.h"
#include "project/Project.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

DeviceMonitorRuntime::DeviceMonitorRuntime(Project* project, QObject* parent)
    : QObject(parent)
    , m_project(project)
    , m_serialPort(new QSerialPort(this))
{
    connect(m_serialPort, &QSerialPort::readyRead, this, &DeviceMonitorRuntime::onSerialReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &DeviceMonitorRuntime::onSerialErrorOccurred);
}

DeviceMonitorRuntime::~DeviceMonitorRuntime() {
    disconnectDevice();
}

bool DeviceMonitorRuntime::isConnected() const {
    return m_serialPort && m_serialPort->isOpen();
}

QStringList DeviceMonitorRuntime::availablePorts() {
    QStringList ports;
    const auto portList = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : portList) {
        QString desc = info.description().isEmpty() ? info.portName() : QString("%1 (%2)").arg(info.portName(), info.description());
        ports.append(desc);
    }
    return ports;
}

bool DeviceMonitorRuntime::connectDevice(const QString& portName, qint32 baudRate) {
    if (isConnected()) {
        disconnectDevice();
    }

    // Extract raw port name (e.g. "COM3" from "COM3 (STMicroelectronics STLink Virtual COM Port)")
    QString cleanPort = portName.split(' ').first();
    m_portName = cleanPort;
    m_baudRate = baudRate;

    m_serialPort->setPortName(cleanPort);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    m_protocol.reset();

    if (!m_serialPort->open(QIODevice::ReadWrite)) {
        QString err = QString("Failed to open serial port %1: %2").arg(cleanPort, m_serialPort->errorString());
        emit errorOccurred(err);
        emit connectionStateChanged(false, cleanPort);
        return false;
    }

    emit connectionStateChanged(true, cleanPort);
    return true;
}

void DeviceMonitorRuntime::disconnectDevice() {
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->close();
        emit connectionStateChanged(false, m_portName);
    }
    m_protocol.reset();
}

bool DeviceMonitorRuntime::sendCommand(const QString& sourceId, TelemetryDataType type, const QVariant& value) {
    if (!isConnected()) {
        emit errorOccurred("Cannot send telemetry command: Device not connected.");
        return false;
    }

    m_txSequence++;
    QByteArray frameData = TelemetryProtocol::formatFrame(sourceId, type, value, m_txSequence);
    qint64 bytesWritten = m_serialPort->write(frameData);
    if (bytesWritten == -1) {
        emit errorOccurred(QString("Serial write failed: %1").arg(m_serialPort->errorString()));
        return false;
    }

    TelemetryLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.direction = "TX";
    entry.sourceId = sourceId;
    entry.dataType = telemetryDataTypeToString(type);
    entry.value = value;
    entry.sequence = m_txSequence;
    entry.isValid = true;
    entry.rawMessage = QString::fromUtf8(frameData).trimmed();
    appendLogEntry(entry);

    return true;
}

void DeviceMonitorRuntime::onSerialReadyRead() {
    if (!m_serialPort) return;
    QByteArray data = m_serialPort->readAll();
    if (data.isEmpty()) return;

    QList<TelemetryFrame> frames = m_protocol.feedBytes(data);
    for (const TelemetryFrame& frame : frames) {
        TelemetryLogEntry entry;
        entry.timestamp = QDateTime::currentDateTime();
        entry.direction = "RX";
        entry.sourceId = frame.sourceId;
        entry.dataType = telemetryDataTypeToString(frame.type);
        entry.value = frame.value;
        entry.sequence = frame.sequence;
        entry.isValid = frame.isValid;
        entry.errorMessage = frame.errorMessage;
        entry.rawMessage = frame.rawFrame;

        appendLogEntry(entry);

        if (frame.isValid) {
            updateProjectDataSource(frame);
            emit frameReceived(frame);
        }
    }
}

void DeviceMonitorRuntime::onSerialErrorOccurred(QSerialPort::SerialPortError error) {
    if (error == QSerialPort::NoError) return;
    if (error == QSerialPort::ResourceError || error == QSerialPort::DeviceNotFoundError) {
        disconnectDevice();
    }
    emit errorOccurred(QString("Serial port error: %1").arg(m_serialPort ? m_serialPort->errorString() : "Unknown"));
}

void DeviceMonitorRuntime::appendLogEntry(const TelemetryLogEntry& entry) {
    if (m_logEntries.size() >= static_cast<int>(m_maxLogCapacity)) {
        m_logEntries.removeFirst(); // Circular buffer
    }
    m_logEntries.append(entry);
    emit logEntryAdded(entry);
}

void DeviceMonitorRuntime::updateProjectDataSource(const TelemetryFrame& frame) {
    if (!m_project || frame.sourceId.isEmpty()) return;

    // Update live DataSource in project
    m_project->updateDataSourceValue(frame.sourceId, frame.value);
}

void DeviceMonitorRuntime::clearLog() {
    m_logEntries.clear();
}

bool DeviceMonitorRuntime::exportCsv(const QString& filePath, QString* errorMsg) const {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMsg) *errorMsg = QString("Failed to open file for CSV export: %1").arg(filePath);
        return false;
    }

    QTextStream out(&file);
    // RFC4180 CSV Header
    out << "Timestamp,Direction,SourceID,DataType,Value,Sequence,Valid,ErrorMessage,RawMessage\n";

    for (const TelemetryLogEntry& e : m_logEntries) {
        QString cleanRaw = e.rawMessage;
        cleanRaw.replace("\"", "\"\"");
        QString cleanErr = e.errorMessage;
        cleanErr.replace("\"", "\"\"");

        out << QString("\"%1\",\"%2\",\"%3\",\"%4\",\"%5\",%6,\"%7\",\"%8\",\"%9\"\n")
            .arg(e.timestamp.toString(Qt::ISODateWithMs))
            .arg(e.direction)
            .arg(e.sourceId)
            .arg(e.dataType)
            .arg(e.value.toString())
            .arg(e.sequence)
            .arg(e.isValid ? "TRUE" : "FALSE")
            .arg(cleanErr)
            .arg(cleanRaw);
    }

    file.close();
    return true;
}
