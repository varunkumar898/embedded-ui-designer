#include "OpenOcdDeviceLink.h"
#include "WriteGuard.h"
#include <QDebug>

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
    // Read DBGMCU_IDCODE at 0x40015800 (RM0360 rev.5, section 30.6.1).
    // Identity is sourced only for families we have documented registers for.
    quint32 idcode = 0;
    bool ok = HardwareBridge::instance().openOcdManager().readMemoryWord(0x40015800, &idcode);
    if (!ok) {
        m_state = DeviceLinkState::Error;
        m_reason = "identify failed: cannot read chip identity register (no probe / wrong target config / port in use / permissions)";
        emit stateChanged(m_state, m_reason);
        emit errorOccurred(m_reason);
        return DeviceIdentity();
    }
    quint32 devId = idcode & 0x0FFF;
    // Known device IDs (sourced from RM0360 §30.6.1 DEV_ID table).
    QString family;
    if (devId == 0x440) family = "STM32F030x8";
    else if (devId == 0x413) family = "STM32F407xx";
    else if (devId == 0x450) family = "STM32H7xx";
    else family = QString("Unknown(0x%1)").arg(devId, 3, 16, QChar('0'));

    // Compare to selected board.
    if (m_selectedBoardId.contains("f030", Qt::CaseInsensitive) && devId != 0x440) {
        m_state = DeviceLinkState::Mismatch;
        m_reason = QString("selected board is STM32F030R8 but detected %1").arg(family);
        emit stateChanged(m_state, m_reason);
        return DeviceIdentity();
    }
    m_identity.family = family;
    m_identity.chipId = family;
    m_identity.protocolVersion = "1.0";
    m_identityVerified = true;
    m_state = DeviceLinkState::Connected;
    m_reason.clear();
    emit stateChanged(m_state, m_reason);
    return m_identity;
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