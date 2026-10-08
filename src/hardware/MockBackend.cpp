#include "MockBackend.h"
#include <cmath>

namespace Hardware {

MockBackend::MockBackend(QObject* parent)
    : HardwareBackend(parent)
    , m_connection(new MockConnection(this))
{
    connect(m_connection, &HardwareConnection::connected, this, [this]() {
        emit connectionChanged(true);
    });
    connect(m_connection, &HardwareConnection::disconnected, this, [this]() {
        emit connectionChanged(false);
    });
}

HardwareCapabilities MockBackend::capabilities() const {
    HardwareCapabilities caps;
    caps.gpioInput = true;
    caps.gpioOutput = true;
    caps.adc = true;
    caps.pwm = true;
    caps.uart = true;
    caps.spi = true;
    caps.i2c = true;
    caps.can = true;
    caps.reset = true;
    caps.liveMonitoring = true;
    caps.rawMemoryAccess = true;
    caps.atomicBsrr = true;
    return caps;
}

bool MockBackend::connectTarget() {
    return m_connection->connectToTarget();
}

bool MockBackend::disconnectTarget() {
    return m_connection->disconnectFromTarget();
}

bool MockBackend::isConnected() const {
    return m_connection->isConnected();
}

bool MockBackend::readDigital(const QString& pin, bool* outHigh) {
    if (!outHigh) return false;
    *outHigh = m_digitalStates.value(pin, false);
    return true;
}

bool MockBackend::writeDigital(const QString& pin, bool high) {
    m_digitalStates[pin] = high;
    emit pinStateChanged(pin, high);
    return true;
}

bool MockBackend::readAnalog(const QString& pin, quint32* outRawCount, double* outNormalized) {
    quint32 raw = m_adcRawValues.value(pin, 2048);
    double norm = m_adcNormalizedValues.contains(pin)
                      ? m_adcNormalizedValues.value(pin)
                      : std::clamp(static_cast<double>(raw) / 4095.0, 0.0, 1.0);
    if (outRawCount) *outRawCount = raw;
    if (outNormalized) *outNormalized = norm;
    emit analogValueChanged(pin, raw, norm);
    return true;
}

bool MockBackend::writePwm(const QString& pin, double dutyPercent) {
    m_pwmValues[pin] = dutyPercent;
    return true;
}

bool MockBackend::spiTransfer(const QString& bus, const QByteArray& txData, QByteArray* outRxData, QString* outLog) {
    QByteArray rx;
    for (int i = 0; i < txData.size(); ++i) {
        quint8 b = static_cast<quint8>(txData[i]);
        rx.append(static_cast<char>(b ^ 0xFF));
    }
    if (outRxData) *outRxData = rx;
    if (outLog) *outLog = QString("Mock SPI %1 transfer complete (%2 bytes)").arg(bus).arg(txData.size());
    return true;
}

bool MockBackend::i2cWrite(const QString& bus, quint8 address, const QByteArray& data) {
    Q_UNUSED(bus);
    Q_UNUSED(address);
    Q_UNUSED(data);
    return true;
}

bool MockBackend::i2cRead(const QString& bus, quint8 address, int length, QByteArray* outData) {
    Q_UNUSED(bus);
    Q_UNUSED(address);
    if (outData) {
        outData->resize(length);
        outData->fill(0x55);
    }
    return true;
}

bool MockBackend::i2cScan(const QString& sclPin, const QString& sdaPin, QList<quint8>* outFoundAddresses, QString* outLog) {
    Q_UNUSED(sclPin);
    Q_UNUSED(sdaPin);
    if (outFoundAddresses) {
        *outFoundAddresses = m_mockI2cAddresses;
    }
    if (outLog) *outLog = "Mock I2C scan: Found simulated sensors";
    return true;
}

bool MockBackend::uartWrite(const QString& port, const QByteArray& data) {
    Q_UNUSED(port);
    Q_UNUSED(data);
    return true;
}

bool MockBackend::uartRead(const QString& port, int maxBytes, QByteArray* outData) {
    Q_UNUSED(port);
    Q_UNUSED(maxBytes);
    if (outData) outData->clear();
    return true;
}

bool MockBackend::resetTarget() {
    return true;
}

QStringList MockBackend::availablePins() const {
    QStringList pins;
    for (int i = 0; i < 16; ++i) pins << QString("PA%1").arg(i);
    for (int i = 0; i < 16; ++i) pins << QString("PB%1").arg(i);
    for (int i = 0; i < 16; ++i) pins << QString("PC%1").arg(i);
    for (int i = 0; i < 32; ++i) pins << QString("GPIO%1").arg(i);
    return pins;
}

void MockBackend::setMockDigitalPin(const QString& pin, bool high) {
    writeDigital(pin, high);
}

void MockBackend::setMockAdcValue(const QString& pin, quint32 rawCount, double normalized) {
    m_adcRawValues[pin] = rawCount;
    if (normalized >= 0.0) {
        m_adcNormalizedValues[pin] = normalized;
    } else {
        m_adcNormalizedValues[pin] = std::clamp(static_cast<double>(rawCount) / 4095.0, 0.0, 1.0);
    }
    emit analogValueChanged(pin, rawCount, m_adcNormalizedValues[pin]);
}

void MockBackend::setMockConnected(bool connected) {
    m_connection->setSimulatedConnected(connected);
}

void MockBackend::setMockI2cDevices(const QList<quint8>& addresses) {
    m_mockI2cAddresses = addresses;
}

} // namespace Hardware
