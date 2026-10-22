#include "OpenOcdDeviceLink.h"
#include "WriteGuard.h"
#include <QDebug>
#include <QProcess>

namespace Hardware {

OpenOcdDeviceLink::OpenOcdDeviceLink(const QString& selectedBoardId, QObject* parent)
    : DeviceLink(parent), m_selectedBoardId(selectedBoardId) {
    m_identity.family = "Unknown";
    m_identity.chipId = "Unknown";
}

void OpenOcdDeviceLink::connectDevice() {
    if (!HardwareBridge::instance().isHardwareConnected()) {
        // Try to bring up OpenOCD? The existing path starts the worker externally.
        // Here we just reflect current state.
    }
    m_connected = HardwareBridge::instance().isHardwareConnected();
    if (!m_connected) {
        m_state = DeviceLinkState::NoDevice;
        m_reason = "no probe detected (OpenOCD not connected on 127.0.0.1:4444)";
        emit stateChanged(m_state, m_reason);
        emit errorOccurred(m_reason);
        return;
    }
    m_state = DeviceLinkState::DeviceFound;
    m_reason.clear();
    emit stateChanged(m_state, m_reason);
}

void OpenOcdDeviceLink::disconnectDevice() {
    m_connected = false;
    m_identityVerified = false;
    m_state = DeviceLinkState::NoDevice;
    m_reason = "disconnected";
    emit stateChanged(m_state, m_reason);
}

bool OpenOcdDeviceLink::isConnected() const { return m_connected && m_identityVerified; }

DeviceIdentity OpenOcdDeviceLink::identify() {
    if (!m_connected) {
        m_state = DeviceLinkState::Error;
        m_reason = "no device connected";
        emit stateChanged(m_state, m_reason);
        return DeviceIdentity();
    }

    // R1.d: the identity register address and the expected DEV_ID both come
    // from the SELECTED board profile (family tables in
    // HardwareBridge::applyChipIdentity). Profiles with zeroed identity data
    // mark families whose registers/DEV_IDs have not been verified yet;
    // identify must fail honestly instead of probing a guessed address.
    BoardProfile bp;
    for (const BoardProfile& candidate : HardwareBridge::instance().availableBoards()) {
        if (candidate.id == m_selectedBoardId) {
            bp = candidate;
            break;
        }
    }
    if (bp.id.isEmpty()) {
        bp = HardwareBridge::instance().currentBoard();
        HardwareBridge::instance().applyChipIdentity(bp);
    }

    // R1.d: ESP32 boards have no DBGMCU register; identity comes from
    // esptool (chip_id) on the adapter-matched serial port.
    if (bp.adapterKind.compare("esptool", Qt::CaseInsensitive) == 0
        || bp.connectionType.compare("serial", Qt::CaseInsensitive) == 0) {
        const QString port = HardwareBridge::instance().lastSerialPortName();
        if (port.isEmpty()) {
            m_state = DeviceLinkState::Error;
            m_reason = QStringLiteral("identify failed: no adapter-matched serial port for %1 (connect the board or check the USB adapter)").arg(bp.name);
            emit stateChanged(m_state, m_reason);
            emit errorOccurred(m_reason);
            return DeviceIdentity();
        }
        QProcess esptool;
        esptool.start(QStringLiteral("esptool.py"),
                      QStringList() << QStringLiteral("--port") << port
                                    << QStringLiteral("--baud") << QStringLiteral("115200")
                                    << QStringLiteral("chip_id"));
        if (!esptool.waitForStarted(4000) ||
            !esptool.waitForFinished(45000) ||
            esptool.exitStatus() != QProcess::NormalExit ||
            esptool.exitCode() != 0) {
            m_state = DeviceLinkState::Error;
            m_reason = esptool.exitCode() != 0 && esptool.state() == QProcess::NotRunning
                           ? QStringLiteral("identify failed: esptool chip_id did not succeed on %1 (%2)")
                                 .arg(port, QString::fromUtf8(esptool.readAllStandardError().trimmed()))
                           : QStringLiteral("identify failed: esptool.py is not available or timed out on %1").arg(port);
            emit stateChanged(m_state, m_reason);
            emit errorOccurred(m_reason);
            return DeviceIdentity();
        }
        const QString chipName = parseEsptoolChipName(esptool.readAllStandardOutput());
        if (chipName.isEmpty()) {
            m_state = DeviceLinkState::Error;
            m_reason = "identify failed: esptool reply did not contain a 'Chip is ...' line";
            emit stateChanged(m_state, m_reason);
            emit errorOccurred(m_reason);
            return DeviceIdentity();
        }
        m_identity = DeviceIdentity();
        m_identity.family = "ESP32";
        m_identity.chipId = chipName;
        m_identityVerified = true;
        m_state = DeviceLinkState::Connected;
        m_reason.clear();
        emit stateChanged(m_state, m_reason);
        return m_identity;
    }

    if (bp.dbgmcuAddress == 0) {
        m_state = DeviceLinkState::Error;
        m_reason = QStringLiteral("identify unavailable for %1: the %2 identity register/DEV_ID has not been verified from hardware yet (see hardware captures)")
                       .arg(bp.name, bp.mcuFamily);
        emit stateChanged(m_state, m_reason);
        emit errorOccurred(m_reason);
        return DeviceIdentity();
    }

    quint32 idcode = 0;
    bool ok = HardwareBridge::instance().openOcdManager().readMemoryWord(bp.dbgmcuAddress, &idcode);
    if (!ok) {
        m_state = DeviceLinkState::Error;
        m_reason = "identify failed: cannot read chip identity register (no probe / wrong target config / port in use / permissions)";
        emit stateChanged(m_state, m_reason);
        emit errorOccurred(m_reason);
        return DeviceIdentity();
    }
    quint32 devId = idcode & 0x0FFF;

    QString family = QString("%1 (DEV_ID 0x%2)")
                         .arg(bp.mcuFamily, QString::number(devId, 16).rightJustified(3, '0').toUpper());
    if (bp.expectedDevId != 0 && devId != bp.expectedDevId) {
        m_state = DeviceLinkState::Mismatch;
        m_reason = QStringLiteral("selected board %1 expects DEV_ID 0x%2 at 0x%3 but detected 0x%4")
                       .arg(bp.name)
                       .arg(bp.expectedDevId, 3, 16, QChar('0'))
                       .arg(bp.dbgmcuAddress, 8, 16, QChar('0'))
                       .arg(devId, 3, 16, QChar('0'));
        emit stateChanged(m_state, m_reason);
        return DeviceIdentity();
    }

    m_identity = DeviceIdentity();
    m_identity.family = bp.mcuFamily;
    m_identity.chipId = bp.name;
    m_identity.protocolVersion = "1.0";
    m_identityVerified = true;
    m_state = DeviceLinkState::Connected;
    m_reason.clear();
    emit stateChanged(m_state, m_reason);
    return m_identity;
}

// static
QString OpenOcdDeviceLink::parseEsptoolChipName(const QByteArray& output) {
    // esptool chip_id prints e.g. "Chip is ESP32-D0WD-V3 (revision v3.0)".
    for (const QByteArray& rawLine : output.split('\n')) {
        const QString line = QString::fromUtf8(rawLine).trimmed();
        if (line.startsWith("Chip is ")) {
            QString rest = line.mid(8);
            const int rev = rest.indexOf(" (revision");
            if (rev >= 0) {
                rest.truncate(rev);
            }
            return rest.trimmed();
        }
    }
    return QString();
}

bool OpenOcdDeviceLink::readSignal(const QString& signalId, QVariant* out) {
    Q_UNUSED(signalId); Q_UNUSED(out);
    return false; // not wired in this step
}

bool OpenOcdDeviceLink::writeSignal(const QString& signalId, const QVariant& value) {
    Q_UNUSED(signalId); Q_UNUSED(value);
    return false;
}

bool OpenOcdDeviceLink::ping() {
    return HardwareBridge::instance().openOcdManager().isConnected();
}

bool OpenOcdDeviceLink::writeAddressAllowed(quint32 address) const {
    return WriteGuard::allow(address,
                            HardwareBridge::instance().allowedWriteAddresses(),
                            HardwareBridge::instance().isLiveWriteAllowed(),
                            m_writeOverride);
}

} // namespace Hardware