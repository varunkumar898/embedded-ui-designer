#pragma once

#include "DeviceLink.h"
#include <QSet>

namespace Hardware {

class MockDeviceLink : public DeviceLink {
    Q_OBJECT
public:
    explicit MockDeviceLink(QObject* parent = nullptr);

    void connectDevice() override;
    void disconnectDevice() override;
    bool isConnected() const override;
    DeviceLinkState state() const override { return m_state; }
    QString stateReason() const override { return m_reason; }

    DeviceIdentity identify() override;
    bool readSignal(const QString& signalId, QVariant* out) override;
    bool writeSignal(const QString& signalId, const QVariant& value) override;
    bool ping() override;

    bool writeAddressAllowed(quint32 address) const override;
    void setWriteOverride(bool on) override { m_writeOverride = on; }
    bool writeOverride() const override { return m_writeOverride; }

    // Test helpers (labelled Simulated).
    void setSimulatedConnected(bool on, const QString& probeName = "Simulated");
    void setSimulatedIdentity(const DeviceIdentity& id);
    void setAllowedAddresses(const QSet<quint32>& addrs);
    void setSimulatedSignal(const QString& id, const QVariant& v);

private:
    DeviceLinkState m_state = DeviceLinkState::NoDevice;
    QString m_reason;
    bool m_connected = false;
    QString m_probeName = "Simulated";
    DeviceIdentity m_identity;
    QSet<quint32> m_allowedAddresses;
    QMap<QString, QVariant> m_signals;
    bool m_writeOverride = false;
    bool m_identityVerified = false;
};

} // namespace Hardware