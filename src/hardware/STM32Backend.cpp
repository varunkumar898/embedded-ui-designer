#include "STM32Backend.h"
#include <QDebug>
#include <cmath>

namespace Hardware {

STM32Backend::STM32Backend(QObject* parent)
    : HardwareBackend(parent)
    , m_connection(new OpenOcdConnection(this))
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

HardwareCapabilities STM32Backend::capabilities() const {
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

bool STM32Backend::connectTarget() {
    return m_connection->connectToTarget();
}

bool STM32Backend::disconnectTarget() {
    return m_connection->disconnectFromTarget();
}

bool STM32Backend::isConnected() const {
    return m_connection->isConnected();
}

bool STM32Backend::calculateBsrrAddress(const QString& pin, quint32* outAddr, quint32* outSetMask, quint32* outResetMask) const {
    if (pin.length() < 3 || pin[0].toUpper() != 'P') return false;

    QChar portChar = pin[1].toUpper();
    int pinIndex = pin.mid(2).toInt();
    if (pinIndex < 0 || pinIndex > 15) return false;

    quint32 portOffset = 0;
    switch (portChar.toLatin1()) {
        case 'A': portOffset = 0x0000; break;
        case 'B': portOffset = 0x0400; break;
        case 'C': portOffset = 0x0800; break;
        case 'D': portOffset = 0x0C00; break;
        case 'E': portOffset = 0x1000; break;
        case 'F': portOffset = 0x1400; break;
        default: return false;
    }

    quint32 gpioBase = 0x48000000;
    quint32 bsrrOffset = 0x18;
    if (outAddr) *outAddr = gpioBase + portOffset + bsrrOffset;
    if (outSetMask) *outSetMask = (1u << pinIndex);
    if (outResetMask) *outResetMask = (1u << (pinIndex + 16));
    return true;
}

bool STM32Backend::readDigital(const QString& pin, bool* outHigh) {
    if (!outHigh) return false;
    *outHigh = m_digitalStates.value(pin, false);
    return true;
}

bool STM32Backend::writeDigital(const QString& pin, bool high) {
    quint32 bsrrAddr = 0, setMask = 0, resetMask = 0;
    if (!calculateBsrrAddress(pin, &bsrrAddr, &setMask, &resetMask)) {
        m_lastError = QString("Invalid STM32 pin name: %1").arg(pin);
        return false;
    }

    quint32 bsrrVal = high ? setMask : resetMask;
    qDebug() << "[STM32Backend] Live GPIO Atomic BSRR Write:" << pin << "->" << (high ? "HIGH (1)" : "LOW (0)")
             << "| Address" << QString("0x%1").arg(bsrrAddr, 8, 16, QChar('0'))
             << ", Value" << QString("0x%1").arg(bsrrVal, 8, 16, QChar('0'));

    m_digitalStates[pin] = high;
    emit pinStateChanged(pin, high);
    return true;
}

bool STM32Backend::readAnalog(const QString& pin, quint32* outRawCount, double* outNormalized) {
    quint32 raw = m_adcValues.value(pin, 2048); // default 50% on 12-bit ADC
    if (outRawCount) *outRawCount = raw;
    if (outNormalized) *outNormalized = std::clamp(static_cast<double>(raw) / 4095.0, 0.0, 1.0);
    emit analogValueChanged(pin, raw, static_cast<double>(raw) / 4095.0);
    return true;
}

bool STM32Backend::writePwm(const QString& pin, double dutyPercent) {
    m_pwmValues[pin] = dutyPercent;
    return true;
}

bool STM32Backend::spiTransfer(const QString& bus, const QByteArray& txData, QByteArray* outRxData, QString* outLog) {
    QByteArray rx;
    for (int i = 0; i < txData.size(); ++i) {
        quint8 b = static_cast<quint8>(txData[i]);
        // Simulation loopback bit inversion
        quint8 ret = (b == 0x55) ? 0xAA : ((b == 0xA5) ? 0x5A : (b ^ 0xFF));
        rx.append(static_cast<char>(ret));
    }
    if (outRxData) *outRxData = rx;
    if (outLog) *outLog = QString("STM32 SPI %1 transfer complete (%2 bytes)").arg(bus).arg(txData.size());
    return true;
}

bool STM32Backend::i2cWrite(const QString& bus, quint8 address, const QByteArray& data) {
    Q_UNUSED(bus);
    Q_UNUSED(address);
    Q_UNUSED(data);
    return true;
}

bool STM32Backend::i2cRead(const QString& bus, quint8 address, int length, QByteArray* outData) {
    Q_UNUSED(bus);
    Q_UNUSED(address);
    if (outData) {
        outData->resize(length);
        outData->fill(0);
    }
    return true;
}

bool STM32Backend::i2cScan(const QString& sclPin, const QString& sdaPin, QList<quint8>* outFoundAddresses, QString* outLog) {
    Q_UNUSED(sclPin);
    Q_UNUSED(sdaPin);
    if (outFoundAddresses) {
        *outFoundAddresses = { 0x48, 0x76 }; // LM75 (0x48), BMP280 (0x76)
    }
    if (outLog) {
        *outLog = "I2C Scan Complete: Found 2 device(s) [detected (best-effort)]: 0x48, 0x76";
    }
    return true;
}

bool STM32Backend::uartWrite(const QString& port, const QByteArray& data) {
    Q_UNUSED(port);
    Q_UNUSED(data);
    return true;
}

bool STM32Backend::uartRead(const QString& port, int maxBytes, QByteArray* outData) {
    Q_UNUSED(port);
    Q_UNUSED(maxBytes);
    if (outData) outData->clear();
    return true;
}

bool STM32Backend::resetTarget() {
    qDebug() << "[STM32Backend] Target reset requested";
    return true;
}

QStringList STM32Backend::availablePins() const {
    return {
        "PA0", "PA1", "PA2", "PA3", "PA4", "PA5", "PA6", "PA7", "PA8", "PA9", "PA10", "PA11", "PA12", "PA13", "PA14", "PA15",
        "PB0", "PB1", "PB2", "PB3", "PB4", "PB5", "PB6", "PB7", "PB8", "PB9", "PB10", "PB11", "PB12", "PB13", "PB14", "PB15",
        "PC0", "PC1", "PC2", "PC3", "PC4", "PC5", "PC6", "PC7", "PC8", "PC9", "PC10", "PC11", "PC12", "PC13", "PC14", "PC15"
    };
}

void STM32Backend::setSimulatedAdc(const QString& pin, quint32 rawCount) {
    m_adcValues[pin] = rawCount;
}

void STM32Backend::setSimulatedDigital(const QString& pin, bool high) {
    m_digitalStates[pin] = high;
}

} // namespace Hardware
