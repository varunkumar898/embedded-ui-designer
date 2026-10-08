#pragma once

#include "HardwareBackend.h"
#include "HardwareConnection.h"

namespace Hardware {

/**
 * @brief ESP32 / ESP32-S3 Hardware Backend.
 * Supports GPIO in/out, 12-bit ADC, LEDC PWM duty, and UART.
 * Cleanly separates supported, experimental, and unsupported features.
 */
class ESP32Backend : public HardwareBackend {
    Q_OBJECT

public:
    explicit ESP32Backend(QObject* parent = nullptr);
    ~ESP32Backend() override = default;

    QString backendId() const override { return "esp32"; }
    QString backendName() const override { return "ESP32-S3 Serial Backend"; }
    QString family() const override { return "ESP32"; }

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

    void setSimulatedAdc(const QString& pin, quint32 rawCount);
    void setSimulatedDigital(const QString& pin, bool high);

private:
    SerialConnection* m_connection = nullptr;
    QString m_lastError;
    QMap<QString, bool> m_digitalStates;
    QMap<QString, quint32> m_adcValues;
    QMap<QString, double> m_pwmValues;
};

} // namespace Hardware
