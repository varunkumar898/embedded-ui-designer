#include "DeviceManager.h"
#include <QDebug>

DeviceManager::DeviceManager(QObject* parent)
    : QObject(parent)
{
    connect(&m_pollTimer, &QTimer::timeout, this, &DeviceManager::onPollTimer);
    m_pollTimer.setInterval(2000); // Check ports every 2 seconds
    m_pollTimer.start();

    refreshPorts();
}

void DeviceManager::setSelectedPort(const QString& port) {
    if (m_selectedPort != port) {
        m_selectedPort = port;
        emit selectedPortChanged(m_selectedPort);
    }
}

void DeviceManager::setAutoScanning(bool enabled) {
    if (enabled && !m_pollTimer.isActive()) {
        m_pollTimer.start();
        emit autoScanningChanged(true);
    } else if (!enabled && m_pollTimer.isActive()) {
        m_pollTimer.stop();
        emit autoScanningChanged(false);
    }
}

void DeviceManager::refreshPorts() {
    updatePortsList(QSerialPortInfo::availablePorts());
}

void DeviceManager::onPollTimer() {
    QList<QSerialPortInfo> currentList = QSerialPortInfo::availablePorts();
    QStringList currentNames;
    for (const auto& info : currentList) {
        currentNames.append(info.portName());
    }

    if (currentNames != m_portNames) {
        updatePortsList(currentList);
    }
}

void DeviceManager::updatePortsList(const QList<QSerialPortInfo>& portList) {
    QStringList newNames;
    QVariantList newPorts;

    for (const QSerialPortInfo& info : portList) {
        newNames.append(info.portName());

        QVariantMap portMap;
        portMap["portName"] = info.portName();
        portMap["systemLocation"] = info.systemLocation();
        portMap["description"] = info.description();
        portMap["manufacturer"] = info.manufacturer();
        portMap["serialNumber"] = info.serialNumber();
        portMap["vendorId"] = info.hasVendorIdentifier() ? QString::number(info.vendorIdentifier(), 16).toUpper() : "";
        portMap["productId"] = info.hasProductIdentifier() ? QString::number(info.productIdentifier(), 16).toUpper() : "";
        portMap["detectedBoard"] = detectBoardType(info.portName());

        newPorts.append(portMap);
    }

    m_portNames = newNames;
    m_ports = newPorts;

    if (!m_portNames.contains(m_selectedPort) && !m_portNames.isEmpty()) {
        setSelectedPort(m_portNames.first());
    }

    emit portsChanged();
}

QVariantMap DeviceManager::getPortDetails(const QString& portName) const {
    for (const QVariant& p : m_ports) {
        QVariantMap map = p.toMap();
        if (map.value("portName").toString() == portName) {
            return map;
        }
    }
    return QVariantMap();
}

QString DeviceManager::detectBoardType(const QString& portName) const {
    QSerialPortInfo info(portName);
    if (info.isNull()) return "Generic Serial Device";

    quint16 vid = info.vendorIdentifier();
    quint16 pid = info.productIdentifier();
    QString desc = info.description().toLower();
    QString mfg = info.manufacturer().toLower();

    // Espressif (VID: 0x303A, 0x10C4 CP210x, 0x1A86 CH340)
    if (vid == 0x303A || desc.contains("esp32") || mfg.contains("espressif")) {
        if (pid == 0x1001) return "ESP32-S3 (USB-JTAG/Serial)";
        return "ESP32 Device";
    }

    // STMicroelectronics (VID: 0x0483)
    if (vid == 0x0483 || desc.contains("stlink") || mfg.contains("stmicroelectronics")) {
        return "STM32 ST-LINK V2/V3";
    }

    // Raspberry Pi RP2040 (VID: 0x2E8A)
    if (vid == 0x2E8A || desc.contains("rp2040") || desc.contains("pico")) {
        return "Raspberry Pi Pico (RP2040)";
    }

    // NXP (VID: 0x1FC9, 0x0D28 DAPLink)
    if (vid == 0x1FC9 || vid == 0x0D28 || desc.contains("daplink")) {
        return "NXP / CMSIS-DAP Debugger";
    }

    if (!info.description().isEmpty()) {
        return info.description();
    }
    return "Generic COM Port";
}

QString DeviceManager::getFlashCommand(const QString& boardType, const QString& binaryPath) const {
    QString b = boardType.toLower();
    QString bin = binaryPath.isEmpty() ? "<output.bin>" : binaryPath;

    if (b.contains("esp32")) {
        return QString("esptool.py -p %1 -b 921600 write_flash 0x0 %2").arg(m_selectedPort, bin);
    } else if (b.contains("stm32")) {
        return QString("STM32_Programmer_CLI -c port=SWD -w %1 0x08000000 -v -rst").arg(bin);
    } else if (b.contains("pico")) {
        return QString("picotool load -x %1").arg(bin);
    }
    return QString("// Select specific device profile to generate flash command");
}
