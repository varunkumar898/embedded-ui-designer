#pragma once

#include "DeviceLink.h"
#include "HardwareBridge.h"

namespace Hardware {

// Wraps the existing OpenOCD manager behind DeviceLink. identify() reads the
// chip identity register (DBGMCU_IDCODE, RM0360 rev.5 section 30.6.1) and
// compares it to the selected board; state becomes Connected only after a
// successful identify().
class OpenOcdDeviceLink : public DeviceLink {
    Q_OBJECT
public:
    explicit OpenOcdDeviceLink(const QString& selectedBoardId, QObject* parent = nullptr);

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

private:
    QString m_selectedBoardId;
    DeviceLinkState m_state = DeviceLinkState::NoDevice;
    QString m_reason;
    bool m_connected = false;
    bool m_identityVerified = false;
    DeviceIdentity m_identity;
    bool m_writeOverride = false;
};

} // namespace Hardware