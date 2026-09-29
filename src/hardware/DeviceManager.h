#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QTimer>
#ifdef HAVE_QT_SERIALPORT
#include <QSerialPortInfo>
#endif

/**
 * @brief Hardware Bridge: Manages connected serial / COM / tty devices.
 * 
 * Exposes live detected serial ports and hardware vendor details to QML.
 * Includes a polling timer to detect dynamic USB device insertions / removals.
 */
class DeviceManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(QStringList portNames READ portNames NOTIFY portsChanged)
    Q_PROPERTY(QVariantList ports READ ports NOTIFY portsChanged)
    Q_PROPERTY(int portCount READ portCount NOTIFY portsChanged)
    Q_PROPERTY(QString selectedPort READ selectedPort WRITE setSelectedPort NOTIFY selectedPortChanged)
    Q_PROPERTY(bool isAutoScanning READ isAutoScanning WRITE setAutoScanning NOTIFY autoScanningChanged)

public:
    explicit DeviceManager(QObject* parent = nullptr);
    virtual ~DeviceManager() override = default;

    QStringList portNames() const { return m_portNames; }
    QVariantList ports() const { return m_ports; }
    int portCount() const { return m_portNames.size(); }
    QString selectedPort() const { return m_selectedPort; }
    void setSelectedPort(const QString& port);

    bool isAutoScanning() const { return m_pollTimer.isActive(); }
    void setAutoScanning(bool enabled);

    Q_INVOKABLE void refreshPorts();
    Q_INVOKABLE QVariantMap getPortDetails(const QString& portName) const;
    Q_INVOKABLE QString detectBoardType(const QString& portName) const;
    Q_INVOKABLE QString getFlashCommand(const QString& boardType, const QString& binaryPath) const;

signals:
    void portsChanged();
    void selectedPortChanged(const QString& port);
    void autoScanningChanged(bool isScanning);
    void deviceConnected(const QString& portName, const QString& description);
    void deviceDisconnected(const QString& portName);

private slots:
    void onPollTimer();

private:
    QStringList m_portNames;
    QVariantList m_ports;
    QString m_selectedPort;
    QTimer m_pollTimer;

#ifdef HAVE_QT_SERIALPORT
    void updatePortsList(const QList<QSerialPortInfo>& portList);
#else
    void updatePortsList();
#endif
};
