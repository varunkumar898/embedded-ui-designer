#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QDir>
#include "DeviceManager.h"

/**
 * @brief Asynchronous Hardware Flashing Subsystem
 * 
 * Executes external microcontroller toolchains (OpenOCD, ST-Link, FTDI/RISC-V)
 * asynchronously via QProcess without blocking the Qt GUI thread.
 * 
 * Listens to the selected port from DeviceManager and streams real-time
 * console output to the frontend via consoleOutputUpdate.
 */
class FlashController : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString selectedPort READ selectedPort WRITE setSelectedPort NOTIFY selectedPortChanged)
    Q_PROPERTY(bool isFlashing READ isFlashing NOTIFY isFlashingChanged)
    Q_PROPERTY(QString openOcdPath READ openOcdPath WRITE setOpenOcdPath NOTIFY openOcdPathChanged)
    Q_PROPERTY(int lastExitCode READ lastExitCode NOTIFY flashingFinished)

public:
    explicit FlashController(DeviceManager* deviceManager = nullptr, QObject* parent = nullptr);
    virtual ~FlashController() override;

    // Device Manager Binding
    void setDeviceManager(DeviceManager* deviceManager);
    DeviceManager* deviceManager() const { return m_deviceManager; }

    QString selectedPort() const { return m_selectedPort; }
    void setSelectedPort(const QString& port);

    bool isFlashing() const { return m_isFlashing; }
    int lastExitCode() const { return m_lastExitCode; }

    QString openOcdPath() const { return m_openOcdPath; }
    void setOpenOcdPath(const QString& path);

    // Flashing Actions (Prompt 3 Requirements)
    Q_INVOKABLE bool flashSTM32(const QString& binaryPath);
    Q_INVOKABLE bool flashRISCV(const QString& binaryPath, const QString& adapterConfig = QString());
    Q_INVOKABLE bool flashESP32(const QString& binaryPath);
    Q_INVOKABLE bool flashDevice(const QString& boardType, const QString& binaryPath);
    Q_INVOKABLE void abort();
    Q_INVOKABLE void cancelFlash() { abort(); }

signals:
    void selectedPortChanged(const QString& port);
    void isFlashingChanged(bool flashing);
    void openOcdPathChanged(const QString& path);
    void consoleOutputUpdate(const QString& output);
    void flashingStarted();
    void flashingFinished(bool success, int exitCode);

private slots:
    void onProcessReadyReadStandardOutput();
    void onProcessReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessErrorOccurred(QProcess::ProcessError error);

private:
    DeviceManager* m_deviceManager = nullptr;
    QString m_selectedPort;
    QString m_openOcdPath;
    bool m_isFlashing = false;
    int m_lastExitCode = 0;

    QProcess* m_process = nullptr;

    void setupProcess();
    bool executeOpenOcd(const QStringList& arguments);
    QString findOpenOcdExecutable() const;
};
