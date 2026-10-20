#include "MockDeviceLink.h"

namespace Hardware {

MockDeviceLink::MockDeviceLink(QObject* parent) : DeviceLink(parent) {
    m_identity.family = "Simulated";
    m_identity.chipId = "Simulated";
    m_identity.protocolVersion = "1.0";
}

void MockDeviceLink::setSimulatedConnected(bool on, const QString& probeName) {
    m_probeName = probeName.isEmpty() ? "Simulated" : probeName;
    // Never show a real board name in simulated mode.
    if (m_probeName.contains("ST-LINK", Qt::CaseInsensitive) ||
        m_probeName.contains("STM32", Qt::CaseInsensitive) ||
        m_probeName.contains("ESP", Qt::CaseInsensitive)) {
        m_probeName = "Simulated";
    }
    if (on) {
        m_connected = true;
        m_state = DeviceLinkState::DeviceFound;
        m_reason.clear();
        emit stateChanged(m_state, m_reason);
    } else {
        m_connected = false;
        m_state = DeviceLinkState::NoDevice;
        m_reason = "disconnected";
        m_identityVerified = false;
        emit stateChanged(m_state, m_reason);
    }
}

void MockDeviceLink::setSimulatedIdentity(const DeviceIdentity& id) { m_identity = id; }
void MockDeviceLink::setAllowedAddresses(const QSet<quint32>& addrs) { m_allowedAddresses = addrs; }
void MockDeviceLink::setSimulatedSignal(const QString& id, const QVariant& v) { m_signals[id] = v; }

void MockDeviceLink::connectDevice() {
    if (!m_connected) {
        m_state = DeviceLinkState::NoDevice;
        m_reason = "no device";
        emit stateChanged(m_state, m_reason);
        emit errorOccurred(m_reason);
        return;
    }
    m_state = DeviceLinkState::DeviceFound;
    m_reason.clear();
    emit stateChanged(m_state, m_reason);
}

void MockDeviceLink::disconnectDevice() {
    m_connected = false;
    m_identityVerified = false;
    m_state = DeviceLinkState::NoDevice;
    m_reason = "disconnected";
    emit stateChanged(m_state, m_reason);
}

bool MockDeviceLink::isConnected() const { return m_connected && m_identityVerified; }

DeviceIdentity MockDeviceLink::identify() {
    if (!m_connected) {
        m_state = DeviceLinkState::Error;
        m_reason = "no device connected";
        emit stateChanged(m_state, m_reason);
        return DeviceIdentity();
    }
    // Simulated identity returns the configured (simulated) identity.
    m_identityVerified = true;
    m_state = DeviceLinkState::Connected;
    m_reason.clear();
    emit stateChanged(m_state, m_reason);
    return m_identity;
}

bool MockDeviceLink::readSignal(const QString& signalId, QVariant* out) {
    if (!m_connected || !m_identityVerified) return false;
    if (!m_signals.contains(signalId)) return false;
    if (out) *out = m_signals.value(signalId);
    emit signalChanged(signalId, m_signals.value(signalId));
    return true;
}

bool MockDeviceLink::writeSignal(const QString& signalId, const QVariant& value) {
    if (!m_connected || !m_identityVerified) return false;
    m_signals[signalId] = value;
    emit signalChanged(signalId, value);
    return true;
}

bool MockDeviceLink::ping() {
    return m_connected;
}

bool MockDeviceLink::writeAddressAllowed(quint32 address) const {
    if (m_writeOverride) return true;
    return m_allowedAddresses.contains(address);
}

} // namespace Hardware