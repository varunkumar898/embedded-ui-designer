#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QList>
#include <QMap>
#include "HardwareCapabilities.h"
#include "HardwareConnection.h"

namespace Hardware {

/**
 * @brief Abstract interface for MCU / MPU / SBC Hardware Backends.
 * Decouples application UI and DataSource bindings from concrete target registers and probes.
 */
class HardwareBackend : public QObject {
    Q_OBJECT

public:
    explicit HardwareBackend(QObject* parent = nullptr) : QObject(parent) {}
    ~HardwareBackend() override = default;

    virtual QString backendId() const = 0;   // "stm32", "esp32", "raspberrypi", "mock", "custom"
    virtual QString backendName() const = 0; // "STM32 OpenOCD Backend", etc.
    virtual QString family() const = 0;      // "STM32", "ESP32", "Raspberry Pi", "Mock"

    virtual HardwareCapabilities capabilities() const = 0;
    virtual HardwareConnection* connection() const = 0;

    virtual bool connectTarget() = 0;
    virtual bool disconnectTarget() = 0;
    virtual bool isConnected() const = 0;
    virtual QString lastError() const = 0;

    // Digital GPIO
    virtual bool readDigital(const QString& pin, bool* outHigh) = 0;
    virtual bool writeDigital(const QString& pin, bool high) = 0;

    // Analog ADC
    virtual bool readAnalog(const QString& pin, quint32* outRawCount, double* outNormalized) = 0;

    // PWM Output
    virtual bool writePwm(const QString& pin, double dutyPercent) = 0;

    // SPI Bus
    virtual bool spiTransfer(const QString& bus, const QByteArray& txData, QByteArray* outRxData, QString* outLog = nullptr) = 0;

    // I2C Bus
    virtual bool i2cWrite(const QString& bus, quint8 address, const QByteArray& data) = 0;
    virtual bool i2cRead(const QString& bus, quint8 address, int length, QByteArray* outData) = 0;
    virtual bool i2cScan(const QString& sclPin, const QString& sdaPin, QList<quint8>* outFoundAddresses, QString* outLog = nullptr) = 0;

    // UART Serial
    virtual bool uartWrite(const QString& port, const QByteArray& data) = 0;
    virtual bool uartRead(const QString& port, int maxBytes, QByteArray* outData) = 0;

    // Target Control
    virtual bool resetTarget() = 0;

    // Available Pins for active target
    virtual QStringList availablePins() const = 0;

signals:
    void connectionChanged(bool connected);
    void pinStateChanged(const QString& pin, bool high);
    void analogValueChanged(const QString& pin, quint32 raw, double normalized);
    void errorOccurred(const QString& error);
};

} // namespace Hardware
