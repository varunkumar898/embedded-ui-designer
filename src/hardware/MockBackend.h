#pragma once

#include "HardwareBackend.h"
#include "HardwareConnection.h"

namespace Hardware {

/**
 * @brief In-Memory Mock Hardware Backend.
 * Used for deterministic automated unit/integration tests and offline design simulation.
 * Allows setting simulated pin values, ADC counts, and triggering connection failures.
 */
class MockBackend : public HardwareBackend {
    Q_OBJECT

public:
    explicit MockBackend(QObject* parent = nullptr);
    ~MockBackend() override = default;

    QString backendId() const override { return "mock"; }
    QString backendName() const override { return "Simulated Mock Backend"; }
    QString family() const override { return "Mock"; }

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

    // Mock Control Methods for Unit Testing
    void setMockDigitalPin(const QString& pin, bool high);
    void setMockAdcValue(const QString& pin, quint32 rawCount, double normalized = -1.0);
    void setMockConnected(bool connected);
    void setMockI2cDevices(const QList<quint8>& addresses);
    void setMockLastError(const QString& error) { m_lastError = error; }

private:
    MockConnection* m_connection = nullptr;
    QString m_lastError;
    QMap<QString, bool> m_digitalStates;
    QMap<QString, quint32> m_adcRawValues;
    QMap<QString, double> m_adcNormalizedValues;
    QMap<QString, double> m_pwmValues;
    QList<quint8> m_mockI2cAddresses = { 0x48, 0x68, 0x76 };
};

} // namespace Hardware
