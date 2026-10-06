#include "OpenOcdManager.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDebug>
#include <QThread>

#ifdef HAVE_QT_SERIALPORT
#include <QSerialPortInfo>
#endif

OpenOcdManager::OpenOcdManager(QObject* parent)
    : QObject(parent)
{
    connect(&m_pollTimer, &QTimer::timeout, this, &OpenOcdManager::onPollTimer);
    m_pollTimer.setInterval(1000);
}

OpenOcdManager::~OpenOcdManager() {
    stopProbePolling();
    disconnectTarget();
}

void OpenOcdManager::startProbePolling(int intervalMs) {
    m_pollTimer.setInterval(intervalMs);
    if (!m_pollTimer.isActive()) {
        m_pollTimer.start();
    }
    // Run an initial scan immediately
    onPollTimer();
}

void OpenOcdManager::stopProbePolling() {
    m_pollTimer.stop();
}

DiscoveredProbe OpenOcdManager::scanUsbProbes() {
    DiscoveredProbe found;

    // 1. Scan Linux sysfs USB devices
    QDir usbDir("/sys/bus/usb/devices");
    if (usbDir.exists()) {
        const QStringList entries = usbDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& entry : entries) {
            QString devPath = usbDir.filePath(entry);
            QFile vidFile(devPath + "/idVendor");
            QFile pidFile(devPath + "/idProduct");

            if (vidFile.exists() && pidFile.exists()) {
                if (vidFile.open(QIODevice::ReadOnly) && pidFile.open(QIODevice::ReadOnly)) {
                    QString vid = QString::fromUtf8(vidFile.readAll()).trimmed().toLower();
                    QString pid = QString::fromUtf8(pidFile.readAll()).trimmed().toLower();
                    vidFile.close();
                    pidFile.close();

                    QString mfg, prod, serial;
                    QFile mfgFile(devPath + "/manufacturer");
                    if (mfgFile.open(QIODevice::ReadOnly)) {
                        mfg = QString::fromUtf8(mfgFile.readAll()).trimmed();
                    }
                    QFile prodFile(devPath + "/product");
                    if (prodFile.open(QIODevice::ReadOnly)) {
                        prod = QString::fromUtf8(prodFile.readAll()).trimmed();
                    }
                    QFile serFile(devPath + "/serial");
                    if (serFile.open(QIODevice::ReadOnly)) {
                        serial = QString::fromUtf8(serFile.readAll()).trimmed();
                    }

                    // STMicroelectronics (VID: 0x0483)
                    if (vid == "0483") {
                        found.detected = true;
                        found.vendorId = "0483";
                        found.productId = pid;
                        found.serialNumber = serial;
                        found.suggestedInterface = "interface/stlink.cfg";
                        if (!prod.isEmpty()) {
                            found.name = prod;
                        } else if (pid == "374b" || pid == "3748" || pid == "374e" || pid == "374f") {
                            found.name = "ST-LINK/V2.1 (STMicroelectronics)";
                        } else {
                            found.name = "STMicroelectronics Debug Probe";
                        }
                        return found;
                    }

                    // Raspberry Pi Pico / Picoprobe (VID: 0x2e8a)
                    if (vid == "2e8a") {
                        found.detected = true;
                        found.vendorId = "2e8a";
                        found.productId = pid;
                        found.serialNumber = serial;
                        found.suggestedInterface = "interface/cmsis-dap.cfg";
                        found.name = prod.isEmpty() ? "Raspberry Pi Debug Probe" : prod;
                        return found;
                    }

                    // ARM DAPLink / CMSIS-DAP (VID: 0x0d28)
                    if (vid == "0d28") {
                        found.detected = true;
                        found.vendorId = "0d28";
                        found.productId = pid;
                        found.serialNumber = serial;
                        found.suggestedInterface = "interface/cmsis-dap.cfg";
                        found.name = prod.isEmpty() ? "CMSIS-DAP / DAPLink Probe" : prod;
                        return found;
                    }

                    // Espressif USB-JTAG (VID: 0x303a, PID: 0x1001)
                    if (vid == "303a") {
                        found.detected = true;
                        found.vendorId = "303a";
                        found.productId = pid;
                        found.serialNumber = serial;
                        found.suggestedInterface = "interface/esp_usb_jtag.cfg";
                        found.name = prod.isEmpty() ? "ESP32 USB-JTAG / Serial" : prod;
                        return found;
                    }

                    // FTDI (VID: 0x0403)
                    if (vid == "0403") {
                        found.detected = true;
                        found.vendorId = "0403";
                        found.productId = pid;
                        found.serialNumber = serial;
                        found.suggestedInterface = "interface/ftdi/ft2232h.cfg";
                        found.name = prod.isEmpty() ? "FTDI Debug Probe" : prod;
                        return found;
                    }

                    // SEGGER J-Link (VID: 0x1366)
                    if (vid == "1366") {
                        found.detected = true;
                        found.vendorId = "1366";
                        found.productId = pid;
                        found.serialNumber = serial;
                        found.suggestedInterface = "interface/jlink.cfg";
                        found.name = prod.isEmpty() ? "SEGGER J-Link" : prod;
                        return found;
                    }
                }
            }
        }
    }

#ifdef HAVE_QT_SERIALPORT
    // 2. Fallback to serial port descriptors
    const auto ports = QSerialPortInfo::availablePorts();
    for (const auto& p : ports) {
        QString desc = p.description().toLower();
        QString mfg = p.manufacturer().toLower();
        quint16 vid = p.vendorIdentifier();
        quint16 pid = p.productIdentifier();

        if (vid == 0x0483 || desc.contains("stlink") || mfg.contains("stmicroelectronics")) {
            found.detected = true;
            found.vendorId = QString::number(vid, 16);
            found.productId = QString::number(pid, 16);
            found.name = p.description().isEmpty() ? "ST-LINK Debug Probe" : p.description();
            found.suggestedInterface = "interface/stlink.cfg";
            return found;
        }
        if (vid == 0x2e8a || desc.contains("picoprobe") || desc.contains("rp2040")) {
            found.detected = true;
            found.vendorId = QString::number(vid, 16);
            found.productId = QString::number(pid, 16);
            found.name = p.description().isEmpty() ? "Raspberry Pi Debug Probe" : p.description();
            found.suggestedInterface = "interface/cmsis-dap.cfg";
            return found;
        }
        if (vid == 0x0d28 || desc.contains("daplink") || desc.contains("cmsis-dap")) {
            found.detected = true;
            found.vendorId = QString::number(vid, 16);
            found.productId = QString::number(pid, 16);
            found.name = p.description().isEmpty() ? "CMSIS-DAP Debugger" : p.description();
            found.suggestedInterface = "interface/cmsis-dap.cfg";
            return found;
        }
    }
#endif

    return found;
}

void OpenOcdManager::onPollTimer() {
    if (m_simulatedMode) {
        return; // Don't override simulated testing state
    }

    DiscoveredProbe probe = scanUsbProbes();

    if (probe.detected && !m_isConnected) {
        m_lastProbe = probe;
        emit probeDiscovered(probe);
        qDebug().noquote() << QString("[OpenOcdManager] Auto-detected probe: %1 (VID: %2, PID: %3). Connecting...")
            .arg(probe.name, probe.vendorId, probe.productId);

        QString iface = probe.suggestedInterface.isEmpty() ? m_activeInterface : probe.suggestedInterface;
        QString target = m_activeTarget.isEmpty() ? "target/stm32f0x.cfg" : m_activeTarget;
        connectToTarget(iface, target, m_tclPort, m_telnetPort);

    } else if (!probe.detected && m_isConnected) {
        qDebug().noquote() << "[OpenOcdManager] Hardware probe unplugged! Disconnecting OpenOCD session.";
        m_lastProbe = DiscoveredProbe();
        emit probeRemoved();
        disconnectTarget();
    }
}

bool OpenOcdManager::connectToTarget(const QString& interfaceCfg, const QString& targetCfg, int tclPort, int telnetPort) {
    if (m_isConnected) {
        return true;
    }

    m_activeInterface = interfaceCfg;
    m_activeTarget = targetCfg;
    m_tclPort = tclPort;
    m_telnetPort = telnetPort;

    // Check if OpenOCD executable is available
    QString openocdExe = QStandardPaths::findExecutable("openocd");
    if (openocdExe.isEmpty()) {
        qWarning() << "[OpenOcdManager] openocd binary not found in system PATH!";
        return false;
    }

    // Launch OpenOCD background daemon
    if (!launchOpenOcdProcess(interfaceCfg, targetCfg, tclPort, telnetPort)) {
        return false;
    }

    // Create / Connect TCP socket to OpenOCD TCL port
    if (!m_socket) {
        m_socket = new QTcpSocket(this);
        connect(m_socket, &QTcpSocket::connected, this, &OpenOcdManager::onSocketConnected);
        connect(m_socket, &QTcpSocket::disconnected, this, &OpenOcdManager::onSocketDisconnected);
    }

    // Allow OpenOCD a brief moment to initialize its sockets
    QThread::msleep(250);

    m_socket->connectToHost("127.0.0.1", static_cast<quint16>(tclPort));
    if (m_socket->waitForConnected(1500)) {
        QString response;
        if (executeTclCommand("version", &response)) {
            m_isConnected = true;
            m_connectedProbeName = m_lastProbe.name.isEmpty() ? "Hardware Target" : m_lastProbe.name;
            emit connectionStatusChanged(true, m_connectedProbeName);
            emit logMessage(QString("[OpenOcdManager] Connected to OpenOCD TCL server: %1").arg(response.trimmed()));
            return true;
        }
    }

    qWarning() << "[OpenOcdManager] Failed to establish handshake with OpenOCD TCL port" << tclPort;
    return false;
}

void OpenOcdManager::disconnectTarget() {
    if (m_socket) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->waitForDisconnected(500);
        }
    }

    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(1000)) {
            m_process->kill();
            m_process->waitForFinished(500);
        }
    }

    const bool wasConnected = m_isConnected;
    m_isConnected = false;
    m_connectedProbeName.clear();

    if (wasConnected) {
        emit connectionStatusChanged(false, QString());
        emit logMessage("[OpenOcdManager] Disconnected from target hardware.");
    }
}

bool OpenOcdManager::launchOpenOcdProcess(const QString& interfaceCfg, const QString& targetCfg, int tclPort, int telnetPort) {
    if (!m_process) {
        m_process = new QProcess(this);
        connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &OpenOcdManager::onProcessFinished);
        connect(m_process, &QProcess::errorOccurred,
                this, &OpenOcdManager::onProcessError);
    }

    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        m_process->waitForFinished(500);
    }

    QStringList args;
    if (!interfaceCfg.isEmpty()) {
        args << "-f" << interfaceCfg;
    }
    if (!targetCfg.isEmpty()) {
        args << "-f" << targetCfg;
    }
    args << "-c" << QString("tcl_port %1").arg(tclPort)
         << "-c" << QString("telnet_port %1").arg(telnetPort)
         << "-c" << "init";

    qDebug().noquote() << "[OpenOcdManager] Launching OpenOCD:" << "openocd" << args.join(' ');
    m_process->start("openocd", args);
    return m_process->waitForStarted(2000);
}

bool OpenOcdManager::executeTclCommand(const QString& cmd, QString* outResponse) {
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        return false;
    }

    QByteArray packet = cmd.toUtf8() + '\x1a';
    if (m_socket->write(packet) == -1) {
        return false;
    }
    m_socket->flush();

    // Wait for response terminated by 0x1A
    QByteArray respData;
    while (m_socket->waitForReadyRead(500)) {
        respData.append(m_socket->readAll());
        if (respData.contains('\x1a')) {
            break;
        }
    }

    respData.replace('\x1a', "");
    if (outResponse) {
        *outResponse = QString::fromUtf8(respData);
    }
    return true;
}

bool OpenOcdManager::writeMemoryWord(quint32 address, quint32 value) {
    m_writeHistory.append({address, value});
    m_simulatedMemory[address] = value;

    if (m_isConnected && m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        QString cmd = QString("mww 0x%1 0x%2")
            .arg(QString::number(address, 16).toUpper())
            .arg(QString::number(value, 16).toUpper());
        return executeTclCommand(cmd);
    }

    return true; // Accepted into simulated/test cache
}

bool OpenOcdManager::readMemoryWord(quint32 address, quint32* outValue) {
    if (!outValue) return false;

    if (m_isConnected && m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        QString cmd = QString("mdw 0x%1")
            .arg(QString::number(address, 16).toUpper());
        QString resp;
        if (executeTclCommand(cmd, &resp)) {
            // Expected output format: 0x48000018: 00000000
            QRegularExpression re(":\\s*([0-9a-fA-F]+)");
            auto match = re.match(resp);
            if (match.hasMatch()) {
                bool ok = false;
                *outValue = match.captured(1).toUInt(&ok, 16);
                if (ok) return true;
            }
        }
    }

    // Simulated fallback
    *outValue = m_simulatedMemory.value(address, 0);
    return true;
}

void OpenOcdManager::setSimulatedConnected(bool connected, const QString& probeName) {
    m_simulatedMode = true;
    m_isConnected = connected;
    m_connectedProbeName = connected ? probeName : QString();
    if (connected) {
        m_lastProbe.detected = true;
        m_lastProbe.name = probeName;
    } else {
        m_lastProbe = DiscoveredProbe();
    }
    emit connectionStatusChanged(m_isConnected, m_connectedProbeName);
}

quint32 OpenOcdManager::simulatedMemoryWord(quint32 address) const {
    return m_simulatedMemory.value(address, 0);
}

void OpenOcdManager::setSimulatedMemoryWord(quint32 address, quint32 value) {
    m_simulatedMemory[address] = value;
}

void OpenOcdManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
    if (m_isConnected) {
        m_isConnected = false;
        m_connectedProbeName.clear();
        emit connectionStatusChanged(false, QString());
    }
}

void OpenOcdManager::onProcessError(QProcess::ProcessError error) {
    qWarning() << "[OpenOcdManager] Process error:" << error;
    if (m_isConnected) {
        m_isConnected = false;
        m_connectedProbeName.clear();
        emit connectionStatusChanged(false, QString());
    }
}

void OpenOcdManager::onSocketConnected() {
    emit logMessage("[OpenOcdManager] Connected to OpenOCD socket.");
}

void OpenOcdManager::onSocketDisconnected() {
    if (m_isConnected && !m_simulatedMode) {
        m_isConnected = false;
        m_connectedProbeName.clear();
        emit connectionStatusChanged(false, QString());
    }
}
