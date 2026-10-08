#pragma once

#include "HardwareBackend.h"
#include "HardwareConnection.h"
#include <QObject>
#include <QString>
#include <QMap>
#include <QList>
#include <QByteArray>
#include <QQueue>

namespace Simulator {

struct VirtualCanFrame {
    quint32 id = 0;
    bool isExtended = false;
    QByteArray payload;
    qint64 timestampMs = 0;
};

/**
 * @brief SimulationBackend provides a safe, completely isolated virtual hardware environment
 * for desktop simulation. It ensures no physical debug probes or real MCU pins are ever touched.
 */
class SimulationBackend : public Hardware::HardwareBackend {
    Q_OBJECT

public:
    explicit SimulationBackend(QObject* parent = nullptr);
    ~SimulationBackend() override = default;

    QString backendId() const override { return "simulation"; }
    QString backendName() const override { return "Desktop Simulation HAL"; }
    QString family() const override { return "Simulator"; }

    Hardware::HardwareCapabilities capabilities() const override;
    Hardware::HardwareConnection* connection() const override { return m_connection; }

    bool connectTarget() override;
    bool disconnectTarget() override;
    bool isConnected() const override;
    QString lastError() const override { return m_lastError; }

    // Digital GPIO
    bool readDigital(const QString& pin, bool* outHigh) override;
    bool writeDigital(const QString& pin, bool high) override;

    // Analog ADC
    bool readAnalog(const QString& pin, quint32* outRawCount, double* outNormalized) override;

    // PWM Output
    bool writePwm(const QString& pin, double dutyPercent) override;

    // SPI Bus
    bool spiTransfer(const QString& bus, const QByteArray& txData, QByteArray* outRxData, QString* outLog = nullptr) override;

    // I2C Bus
    bool i2cWrite(const QString& bus, quint8 address, const QByteArray& data) override;
    bool i2cRead(const QString& bus, quint8 address, int length, QByteArray* outData) override;
    bool i2cScan(const QString& sclPin, const QString& sdaPin, QList<quint8>* outFoundAddresses, QString* outLog = nullptr) override;

    // UART Serial
    bool uartWrite(const QString& port, const QByteArray& data) override;
    bool uartRead(const QString& port, int maxBytes, QByteArray* outData) override;

    // Target Control
    bool resetTarget() override;

    // Available Pins
    QStringList availablePins() const override;

    // ── Simulation Specific Control & Inspection ───────────────────────────
    void setSimulatedDigital(const QString& pin, bool high);
    bool simulatedDigital(const QString& pin) const;

    void setSimulatedAdc(const QString& pin, quint32 rawCount, double normalized = -1.0);
    quint32 simulatedAdcRaw(const QString& pin) const;
    double simulatedAdcNormalized(const QString& pin) const;

    void setSimulatedPwm(const QString& pin, double dutyPercent);
    double simulatedPwm(const QString& pin) const;

    // UART Buffers
    void injectUartData(const QString& port, const QByteArray& data);
    QByteArray getUartTxBuffer(const QString& port) const;
    void clearUartBuffers(const QString& port);

    // CAN Simulation
    void injectCanFrame(quint32 id, const QByteArray& payload, bool isExtended = false);
    QList<VirtualCanFrame> getCanRxHistory() const { return m_canHistory; }
    void clearCanHistory() { m_canHistory.clear(); }

    // Error Injection & Testing
    void setSimulatedConnected(bool connected);
    void setSimulateErrors(bool enable) { m_simulateErrors = enable; }
    bool simulateErrors() const { return m_simulateErrors; }
    void setLastError(const QString& err) { m_lastError = err; }

    void resetAllSimulationData();

signals:
    void uartDataTransmitted(const QString& port, const QByteArray& data);
    void canFrameTransmitted(quint32 id, const QByteArray& payload);

private:
    Hardware::MockConnection* m_connection = nullptr;
    QString m_lastError;
    bool m_simulateErrors = false;

    QMap<QString, bool> m_digitalStates;
    QMap<QString, quint32> m_adcRaw;
    QMap<QString, double> m_adcNormalized;
    QMap<QString, double> m_pwmDuty;

    QMap<QString, QByteArray> m_uartTxBuffers;
    QMap<QString, QByteArray> m_uartRxQueues;
    QList<VirtualCanFrame> m_canHistory;
    QList<quint8> m_i2cDevices = { 0x48, 0x68, 0x76 };
    QMap<quint8, QByteArray> m_i2cRegisters;
};

} // namespace Simulator
