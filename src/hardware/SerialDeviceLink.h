#pragma once

#include "DeviceLink.h"
#include <QSerialPort>
#include <QTimer>

namespace Hardware {

class SerialDeviceLink : public DeviceLink {
    Q_OBJECT
public:
    explicit SerialDeviceLink(const QString& portName, QObject* parent = nullptr);
    ~SerialDeviceLink() override;

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

    static QStringList availablePorts();

private slots:
    void onReadyRead();
    void onTimeout();

private:
    QString m_portName;
    QSerialPort* m_port = nullptr;
    QTimer* m_timer = nullptr;
    DeviceLinkState m_state = DeviceLinkState::NoDevice;
    QString m_reason;
    bool m_connected = false;
    bool m_identityVerified = false;
    DeviceIdentity m_identity;
    bool m_writeOverride = false;
    bool m_permissionError = false;
};

} // namespace Hardware