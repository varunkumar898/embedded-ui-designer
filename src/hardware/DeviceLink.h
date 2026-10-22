#pragma once

#include <QString>
#include <QObject>
#include <QVariant>
#include <functional>

namespace Hardware {

enum class DeviceLinkState {
    NoDevice,
    DeviceFound,
    Connected,   // only after successful identify()
    Mismatch,    // selected vs detected mismatch
    Error
};

QString deviceLinkStateToString(DeviceLinkState s);

struct DeviceIdentity {
    QString family;        // e.g. "STM32F0"
    QString chipId;        // e.g. "STM32F030R8"
    QString firmwareVersion;
    QString protocolVersion;
    quint32 signalTableHash = 0;
    QStringList capabilities;
};

class DeviceLink : public QObject {
    Q_OBJECT
public:
    explicit DeviceLink(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~DeviceLink() = default;

    virtual void connectDevice() = 0;
    virtual void disconnectDevice() = 0;
    virtual bool isConnected() const = 0;
    virtual DeviceLinkState state() const = 0;
    virtual QString stateReason() const = 0;

    virtual DeviceIdentity identify() = 0;   // throws/returns empty on failure
    virtual bool readSignal(const QString& signalId, QVariant* out) = 0;
    virtual bool writeSignal(const QString& signalId, const QVariant& value) = 0;
    virtual bool ping() = 0;

    // Write guard: address must belong to the board profile / signal table.
    virtual bool writeAddressAllowed(quint32 address) const = 0;
    virtual void setWriteOverride(bool on) = 0;
    virtual bool writeOverride() const = 0;

signals:
    void stateChanged(DeviceLinkState state, const QString& reason);
    void signalChanged(const QString& signalId, const QVariant& value);
    void errorOccurred(const QString& reason);
};

} // namespace Hardware