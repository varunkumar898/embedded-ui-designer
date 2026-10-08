#pragma once

#include "HardwareBackend.h"
#include "HardwareConnection.h"

namespace Hardware {

/**
 * @brief Raspberry Pi / Linux SBC Hardware Backend.
 * Encapsulates Linux GPIO, I2C, SPI, and UART subsystems.
 * Gracefully reports unsupported / unavailable when running on non-SBC hosts.
 */
class RaspberryPiBackend : public HardwareBackend {
    Q_OBJECT

public:
    explicit RaspberryPiBackend(QObject* parent = nullptr);
    ~RaspberryPiBackend() override = default;

    QString backendId() const override { return "raspberrypi"; }
    QString backendName() const override { return "Raspberry Pi Linux Backend"; }
    QString family() const override { return "Raspberry Pi"; }

    HardwareCapabilities capabilities() const override;
    HardwareConnection* connection() const override { return m_connection; }

    bool connectTarget() override;
    bool disconnectTarget() override;
    bool isConnected() const override;
    QString lastError() const override { return m_lastError; }

    bool readDigital(const QString& pin, bool* outHigh) override;
    bool writeDigital(const QString& pin, bool high) override;
    bool readAnalog(const QString& pin, quint32* outRawCount, double* outNormalized) override;
    bool writePwm(const QString& pin, double dutyPercent) override;
    bool spiTransfer(const QString& bus, const QByteArray& txData, QByteArray* outRxData, QString* outLog = nullptr) override;
    bool i2cWrite(const QString& bus, quint8 address, const QByteArray& data) override;
    bool i2cRead(const QString& bus, quint8 address, int length, QByteArray* outData) override;
    bool i2cScan(const QString& sclPin, const QString& sdaPin, QList<quint8>* outFoundAddresses, QString* outLog = nullptr) override;
    bool uartWrite(const QString& port, const QByteArray& data) override;
    bool uartRead(const QString& port, int maxBytes, QByteArray* outData) override;
    bool resetTarget() override;

    QStringList availablePins() const override;

    void setSimulatedDigital(const QString& pin, bool high);

private:
    LinuxSysfsConnection* m_connection = nullptr;
    QString m_lastError;
    QMap<QString, bool> m_digitalStates;
};

} // namespace Hardware
