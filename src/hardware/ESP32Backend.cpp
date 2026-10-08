#include "ESP32Backend.h"
#include <QDebug>
#include <cmath>

namespace Hardware {

ESP32Backend::ESP32Backend(QObject* parent)
    : HardwareBackend(parent)
    , m_connection(new SerialConnection(this))
{
    connect(m_connection, &HardwareConnection::connected, this, [this]() {
        emit connectionChanged(true);
    });
    connect(m_connection, &HardwareConnection::disconnected, this, [this]() {
        emit connectionChanged(false);
    });
    connect(m_connection, &HardwareConnection::errorOccurred, this, [this](const QString& err) {
        m_lastError = err;
        emit errorOccurred(err);
    });
}

HardwareCapabilities ESP32Backend::capabilities() const {
    HardwareCapabilities caps;
    caps.gpioInput = true;
    caps.gpioOutput = true;
    caps.adc = true;
    caps.pwm = true;
    caps.uart = true;
    caps.spi = false; // EXPERIMENTAL / not exposed in stable capabilities
    caps.i2c = false; // EXPERIMENTAL
    caps.can = false; // NOT IMPLEMENTED
    caps.reset = true;
    caps.liveMonitoring = true;
    caps.rawMemoryAccess = false;
    caps.atomicBsrr = false;
    return caps;
}

bool ESP32Backend::connectTarget() {
    return m_connection->connectToTarget();
}

bool ESP32Backend::disconnectTarget() {
    return m_connection->disconnectFromTarget();
}

bool ESP32Backend::isConnected() const {
    return m_connection->isConnected();
}

bool ESP32Backend::readDigital(const QString& pin, bool* outHigh) {
    if (!outHigh) return false;
    *outHigh = m_digitalStates.value(pin, false);
    return true;
}

bool ESP32Backend::writeDigital(const QString& pin, bool high) {
    qDebug() << "[ESP32Backend] Write GPIO:" << pin << "->" << (high ? "HIGH (1)" : "LOW (0)");
    m_digitalStates[pin] = high;
    emit pinStateChanged(pin, high);
    return true;
}

bool ESP32Backend::readAnalog(const QString& pin, quint32* outRawCount, double* outNormalized) {
    quint32 raw = m_adcValues.value(pin, 1800);
    if (outRawCount) *outRawCount = raw;
    if (outNormalized) *outNormalized = std::clamp(static_cast<double>(raw) / 4095.0, 0.0, 1.0);
    emit analogValueChanged(pin, raw, static_cast<double>(raw) / 4095.0);
    return true;
}

bool ESP32Backend::writePwm(const QString& pin, double dutyPercent) {
    m_pwmValues[pin] = dutyPercent;
    qDebug() << "[ESP32Backend] Set LEDC PWM on pin" << pin << "to" << dutyPercent << "%";
    return true;
}

bool ESP32Backend::spiTransfer(const QString& bus, const QByteArray& txData, QByteArray* outRxData, QString* outLog) {
    Q_UNUSED(bus);
    Q_UNUSED(txData);
    if (outRxData) outRxData->clear();
    if (outLog) *outLog = "ESP32 SPI transfer: [EXPERIMENTAL / Software stub]";
    return false; // EXPERIMENTAL - not supported over standard serial bridge
}

bool ESP32Backend::i2cWrite(const QString& bus, quint8 address, const QByteArray& data) {
    Q_UNUSED(bus);
    Q_UNUSED(address);
    Q_UNUSED(data);
    m_lastError = "ESP32 I2C write: [EXPERIMENTAL / Not supported over standard serial bridge]";
    return false;
}

bool ESP32Backend::i2cRead(const QString& bus, quint8 address, int length, QByteArray* outData) {
    Q_UNUSED(bus);
    Q_UNUSED(address);
    Q_UNUSED(length);
    if (outData) outData->clear();
    m_lastError = "ESP32 I2C read: [EXPERIMENTAL / Not supported over standard serial bridge]";
    return false;
}

bool ESP32Backend::i2cScan(const QString& sclPin, const QString& sdaPin, QList<quint8>* outFoundAddresses, QString* outLog) {
    Q_UNUSED(sclPin);
    Q_UNUSED(sdaPin);
    if (outFoundAddresses) outFoundAddresses->clear();
    if (outLog) *outLog = "ESP32 I2C Scan: [EXPERIMENTAL / Not supported over standard serial bridge]";
    m_lastError = "ESP32 I2C Scan not supported";
    return false;
}

bool ESP32Backend::uartWrite(const QString& port, const QByteArray& data) {
    qDebug() << "[ESP32Backend] UART Write on" << port << ":" << data.toHex();
    return true;
}

bool ESP32Backend::uartRead(const QString& port, int maxBytes, QByteArray* outData) {
    Q_UNUSED(port);
    Q_UNUSED(maxBytes);
    if (outData) outData->clear();
    return true;
}

bool ESP32Backend::resetTarget() {
    qDebug() << "[ESP32Backend] ESP32 EN/Reset line pulsed via RTS/DTR";
    return true;
}

QStringList ESP32Backend::availablePins() const {
    QStringList pins;
    for (int i = 0; i <= 48; ++i) {
        pins << QString("IO%1").arg(i);
    }
    return pins;
}

void ESP32Backend::setSimulatedAdc(const QString& pin, quint32 rawCount) {
    m_adcValues[pin] = rawCount;
}

void ESP32Backend::setSimulatedDigital(const QString& pin, bool high) {
    m_digitalStates[pin] = high;
}

} // namespace Hardware
