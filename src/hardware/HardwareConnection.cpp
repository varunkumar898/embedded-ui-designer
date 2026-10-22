#include "HardwareConnection.h"
#include "OpenOcdManager.h"
#include "HardwareBridge.h"
#include <QFile>
#include <QDir>
#ifdef HAVE_QT_SERIALPORT
#include <QSerialPort>
#include <QSerialPortInfo>
#endif

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
    // R1.c: no unconditional transport claim - reflect the real OpenOCD
    // daemon session managed by the bridge (executable present, socket and
    // target handshake).
    HardwareBridge& bridge = HardwareBridge::instance();
    const BoardProfile bp = bridge.currentBoard();
    const bool ok = bridge.openOcdManager().connectToTarget(bp.openocdInterface, bp.openocdTarget);
    if (ok) {
        m_connected = true;
        emit connected();
    } else {
        m_lastError = bridge.lastConnectionError().isEmpty()
                          ? "OpenOCD daemon/target not reachable"
                          : bridge.lastConnectionError();
        emit errorOccurred(m_lastError);
    }
    return ok;
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
    // R1.c: selecting a COM port name is not a transport claim - the port
    // must actually open.
    if (m_portName.isEmpty()) {
        m_lastError = "No serial port selected";
        emit errorOccurred(m_lastError);
        return false;
    }
#ifdef HAVE_QT_SERIALPORT
    QSerialPort port(m_portName);
    port.setBaudRate(m_baudRate);
    if (!port.open(QIODevice::ReadWrite)) {
        m_lastError = QString("Serial port %1 could not be opened: %2")
                          .arg(m_portName, port.errorString());
        emit errorOccurred(m_lastError);
        return false;
    }
    port.close(); // real sessions (telemetry/DeviceMonitor) own the port
    m_connected = true;
    emit connected();
    return true;
#else
    m_lastError = "Serial transport unavailable (Qt SerialPort not built in).";
    emit errorOccurred(m_lastError);
    return false;
#endif
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
    // R1.c: subsystem PATH presence (/sys/class/gpio) is not a connection;
    // require an openable GPIO character device like the bridge does.
    QDir devDir("/dev");
    // Same filter as HardwareBridge::firstOpenableGpioChip: device nodes plus
    // regular files (for offline tests).
    const QStringList chips = devDir.entryList(QStringList() << "gpiochip*",
                                               QDir::Files | QDir::System, QDir::Name);
    for (const QString& chip : chips) {
        QFile chipDev(devDir.filePath(chip));
        if (chipDev.open(QIODevice::ReadOnly)) {
            chipDev.close();
            m_connected = true;
            emit connected();
            return true;
        }
    }
    m_connected = false;
    m_lastError = "Host is not a native Linux Raspberry Pi / SBC hardware target "
                  "(no openable /dev/gpiochip*).";
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
