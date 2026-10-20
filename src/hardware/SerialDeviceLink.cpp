#include "SerialDeviceLink.h"
#include <QSerialPortInfo>
#include <QDebug>

namespace Hardware {

SerialDeviceLink::SerialDeviceLink(const QString& portName, QObject* parent)
    : DeviceLink(parent), m_portName(portName) {
    m_port = new QSerialPort(this);
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_port, &QSerialPort::readyRead, this, &SerialDeviceLink::onReadyRead);
    connect(m_timer, &QTimer::timeout, this, &SerialDeviceLink::onTimeout);
}

SerialDeviceLink::~SerialDeviceLink() {
    disconnectDevice();
}

QStringList SerialDeviceLink::availablePorts() {
    QStringList out;
    for (const QSerialPortInfo& info : QSerialPortInfo::availablePorts()) {
        out << info.portName() + " (" + info.description() + ", VID:" + QString::number(info.vendorIdentifier(), 16) + ")";
    }
    return out;
}

void SerialDeviceLink::connectDevice() {
    m_port->setPortName(m_portName);
    m_port->setBaudRate(QSerialPort::Baud115200);
    m_port->setDataBits(QSerialPort::Data8);
    m_port->setParity(QSerialPort::NoParity);
    m_port->setStopBits(QSerialPort::OneStop);
    if (!m_port->open(QIODevice::ReadWrite)) {
        m_state = DeviceLinkState::Error;
        m_reason = m_port->error() == QSerialPort::PermissionError
            ? QString("permission denied opening %1 (add user to 'dialout' group)").arg(m_portName)
            : QString("cannot open %1: %2").arg(m_portName, m_port->errorString());
        m_permissionError = (m_port->error() == QSerialPort::PermissionError);
        emit stateChanged(m_state, m_reason);
        emit errorOccurred(m_reason);
        return;
    }
    m_connected = true;
    m_state = DeviceLinkState::DeviceFound;
    m_reason.clear();
    emit stateChanged(m_state, m_reason);
    m_timer->start(2000);
}

void SerialDeviceLink::disconnectDevice() {
    if (m_port && m_port->isOpen()) m_port->close();
    m_connected = false;
    m_identityVerified = false;
    m_state = DeviceLinkState::NoDevice;
    m_reason = "disconnected";
    emit stateChanged(m_state, m_reason);
}

bool SerialDeviceLink::isConnected() const { return m_connected && m_identityVerified; }

DeviceIdentity SerialDeviceLink::identify() {
    if (!m_connected) {
        m_state = DeviceLinkState::Error;
        m_reason = "no device connected";
        emit stateChanged(m_state, m_reason);
        return DeviceIdentity();
    }
    // Send HELLO; wait for IDENTIFY (handled in onReadyRead). For now, return
    // a placeholder identity; full handshake is wired when the agent firmware
    // lands. State stays DeviceFound until identity is verified.
    m_state = DeviceLinkState::DeviceFound;
    m_reason = "identity not verifiable (no agent firmware yet)";
    emit stateChanged(m_state, m_reason);
    return DeviceIdentity();
}

bool SerialDeviceLink::readSignal(const QString&, QVariant*) { return false; }
bool SerialDeviceLink::writeSignal(const QString&, const QVariant&) { return false; }
bool SerialDeviceLink::ping() { return m_connected; }

bool SerialDeviceLink::writeAddressAllowed(quint32) const { return false; }

void SerialDeviceLink::onReadyRead() {
    // Placeholder: parse frames when agent firmware lands.
}

void SerialDeviceLink::onTimeout() {
    if (!m_identityVerified) {
        m_state = DeviceLinkState::Error;
        m_reason = "identify timeout";
        emit stateChanged(m_state, m_reason);
        emit errorOccurred(m_reason);
    }
}

} // namespace Hardware