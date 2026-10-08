#include "HardwareConnection.h"
#include "OpenOcdManager.h"
#include <QFile>
#include <QDir>

namespace Hardware {

// ── OpenOcdConnection ────────────────────────────────────────────────────────

OpenOcdConnection::OpenOcdConnection(QObject* parent)
    : HardwareConnection(parent)
{
}

QString OpenOcdConnection::connectionInfo() const {
    if (m_connected) {
        return QString("OpenOCD Probe [%1 -> %2]").arg(m_interfaceScript, m_targetScript);
    }
    return "OpenOCD Probe (Disconnected)";
}

bool OpenOcdConnection::isConnected() const {
    return m_connected;
}

bool OpenOcdConnection::connectToTarget() {
    m_connected = true;
    emit connected();
    return true;
}

bool OpenOcdConnection::disconnectFromTarget() {
    m_connected = false;
    emit disconnected();
    return true;
}

// ── SerialConnection ─────────────────────────────────────────────────────────

SerialConnection::SerialConnection(QObject* parent)
    : HardwareConnection(parent)
{
}

QString SerialConnection::connectionInfo() const {
    if (m_connected && !m_portName.isEmpty()) {
        return QString("Serial Port %1 @ %2 baud").arg(m_portName).arg(m_baudRate);
    }
    return "Serial Port (Disconnected)";
}

bool SerialConnection::isConnected() const {
    return m_connected;
}

bool SerialConnection::connectToTarget() {
    if (m_portName.isEmpty()) {
        m_lastError = "No serial port selected";
        emit errorOccurred(m_lastError);
        return false;
    }
    m_connected = true;
    emit connected();
    return true;
}

bool SerialConnection::disconnectFromTarget() {
    m_connected = false;
    emit disconnected();
    return true;
}

// ── LinuxSysfsConnection ─────────────────────────────────────────────────────

LinuxSysfsConnection::LinuxSysfsConnection(QObject* parent)
    : HardwareConnection(parent)
{
}

QString LinuxSysfsConnection::connectionInfo() const {
    if (m_connected) {
        return "Linux Hardware Subsystem (/sys/class/gpio, /dev/i2c, /dev/spidev)";
    }
    return "Linux Hardware Subsystem (Unavailable)";
}

bool LinuxSysfsConnection::isConnected() const {
    return m_connected;
}

bool LinuxSysfsConnection::connectToTarget() {
    // Check if Linux gpio or i2c paths exist
    bool hasGpio = QFile::exists("/sys/class/gpio") || QFile::exists("/dev/gpiochip0");
    if (hasGpio) {
        m_connected = true;
        emit connected();
        return true;
    }
    // Simulation / graceful fallback
    m_connected = false;
    m_lastError = "Host is not a native Linux Raspberry Pi / SBC hardware target";
    return false;
}

bool LinuxSysfsConnection::disconnectFromTarget() {
    m_connected = false;
    emit disconnected();
    return true;
}

// ── MockConnection ───────────────────────────────────────────────────────────

MockConnection::MockConnection(QObject* parent)
    : HardwareConnection(parent)
{
}

bool MockConnection::connectToTarget() {
    m_connected = true;
    emit connected();
    return true;
}

bool MockConnection::disconnectFromTarget() {
    m_connected = false;
    emit disconnected();
    return true;
}

void MockConnection::setSimulatedConnected(bool connected) {
    if (m_connected != connected) {
        m_connected = connected;
        if (m_connected) {
            emit this->connected();
        } else {
            emit disconnected();
        }
    }
}

} // namespace Hardware
