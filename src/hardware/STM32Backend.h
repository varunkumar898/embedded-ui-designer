#pragma once

#include "HardwareBackend.h"
#include "HardwareConnection.h"

namespace Hardware {

/**
 * @brief STM32 Hardware Backend.
 * Encapsulates OpenOCD, ST-Link, atomic BSRR register writes, ADC polling, SPI, and I2C.
 * Keeps STM32 register addresses strictly contained within this hardware layer.
 */
class STM32Backend : public HardwareBackend {
    Q_OBJECT

public:
    explicit STM32Backend(QObject* parent = nullptr);
    ~STM32Backend() override = default;

    QString backendId() const override { return "stm32"; }
    QString backendName() const override { return "STM32 OpenOCD Backend"; }
    QString family() const override { return "STM32"; }

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

    // Direct register calculation helper (confined strictly to STM32 backend)
    bool calculateBsrrAddress(const QString& pin, quint32* outAddr, quint32* outSetMask, quint32* outResetMask) const;

    // Simulated values cache for test / disconnected mode
    void setSimulatedAdc(const QString& pin, quint32 rawCount);
    void setSimulatedDigital(const QString& pin, bool high);

private:
    OpenOcdConnection* m_connection = nullptr;
    QString m_lastError;
    QMap<QString, bool> m_digitalStates;
    QMap<QString, quint32> m_adcValues;
    QMap<QString, double> m_pwmValues;
};

} // namespace Hardware
