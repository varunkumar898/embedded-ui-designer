#pragma once

#include <QObject>
#include <QString>

namespace Hardware {

/**
 * @brief Abstract base class for debug / transport connections (OpenOCD, Serial, Linux, Mock).
 */
class HardwareConnection : public QObject {
    Q_OBJECT

public:
    explicit HardwareConnection(QObject* parent = nullptr) : QObject(parent) {}
    ~HardwareConnection() override = default;

    virtual QString connectionType() const = 0; // "openocd", "serial", "linux", "mock"
    virtual QString connectionInfo() const = 0;
    virtual bool isConnected() const = 0;
    virtual bool connectToTarget() = 0;
    virtual bool disconnectFromTarget() = 0;
    virtual QString lastError() const = 0;

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& errorMessage);
};

/**
 * @brief OpenOCD / ST-Link probe connection.
 */
class OpenOcdConnection : public HardwareConnection {
    Q_OBJECT

public:
    explicit OpenOcdConnection(QObject* parent = nullptr);
    ~OpenOcdConnection() override = default;

    QString connectionType() const override { return "openocd"; }
    QString connectionInfo() const override;
    bool isConnected() const override;
    bool connectToTarget() override;
    bool disconnectFromTarget() override;
    QString lastError() const override { return m_lastError; }

    void setInterfaceScript(const QString& script) { m_interfaceScript = script; }
    void setTargetScript(const QString& script) { m_targetScript = script; }

private:
    QString m_interfaceScript = "interface/stlink.cfg";
    QString m_targetScript = "target/stm32f0x.cfg";
    QString m_lastError;
    bool m_connected = false;
};

/**
 * @brief Serial / COM / USB CDC connection.
 */
class SerialConnection : public HardwareConnection {
    Q_OBJECT

public:
    explicit SerialConnection(QObject* parent = nullptr);
    ~SerialConnection() override = default;

    QString connectionType() const override { return "serial"; }
    QString connectionInfo() const override;
    bool isConnected() const override;
    bool connectToTarget() override;
    bool disconnectFromTarget() override;
    QString lastError() const override { return m_lastError; }

    void setPortName(const QString& port) { m_portName = port; }
    QString portName() const { return m_portName; }
    void setBaudRate(int baud) { m_baudRate = baud; }

private:
    QString m_portName;
    int m_baudRate = 115200;
    QString m_lastError;
    bool m_connected = false;
};

/**
 * @brief Linux sysfs / gpiod / dev connection for Raspberry Pi and Linux SBCs.
 */
class LinuxSysfsConnection : public HardwareConnection {
    Q_OBJECT

public:
    explicit LinuxSysfsConnection(QObject* parent = nullptr);
    ~LinuxSysfsConnection() override = default;

    QString connectionType() const override { return "linux"; }
    QString connectionInfo() const override;
    bool isConnected() const override;
    bool connectToTarget() override;
    bool disconnectFromTarget() override;
    QString lastError() const override { return m_lastError; }

private:
    QString m_lastError;
    bool m_connected = false;
};

/**
 * @brief In-memory mock connection for deterministic tests and offline simulation.
 */
class MockConnection : public HardwareConnection {
    Q_OBJECT

public:
    explicit MockConnection(QObject* parent = nullptr);
    ~MockConnection() override = default;

    QString connectionType() const override { return "mock"; }
    QString connectionInfo() const override { return "Simulated Virtual Target"; }
    bool isConnected() const override { return m_connected; }
    bool connectToTarget() override;
    bool disconnectFromTarget() override;
    QString lastError() const override { return m_lastError; }

    void setSimulatedConnected(bool connected);

private:
    bool m_connected = true;
    QString m_lastError;
};

} // namespace Hardware
