#include "RaspberryPiBackend.h"
#include <QDebug>
#include <QFile>

namespace Hardware {

RaspberryPiBackend::RaspberryPiBackend(QObject* parent)
    : HardwareBackend(parent)
    , m_connection(new LinuxSysfsConnection(this))
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

HardwareCapabilities RaspberryPiBackend::capabilities() const {
    HardwareCapabilities caps;
    caps.gpioInput = true;
    caps.gpioOutput = true;
    caps.adc = false; // Native Raspberry Pi headers have no internal ADC without external SPI/I2C ADC chip
    caps.pwm = true;  // Hardware PWM pins GPIO12, GPIO13, GPIO18, GPIO19
    caps.uart = true;
    caps.spi = true;
    caps.i2c = true;
    caps.can = false;
    caps.reset = false;
    caps.liveMonitoring = true;
    return caps;
}

bool RaspberryPiBackend::connectTarget() {
    return m_connection->connectToTarget();
}

bool RaspberryPiBackend::disconnectTarget() {
    return m_connection->disconnectFromTarget();
}

bool RaspberryPiBackend::isConnected() const {
    return m_connection->isConnected();
}

bool RaspberryPiBackend::readDigital(const QString& pin, bool* outHigh) {
    if (!outHigh) return false;
    *outHigh = m_digitalStates.value(pin, false);
    return true;
}

bool RaspberryPiBackend::writeDigital(const QString& pin, bool high) {
    qDebug() << "[RaspberryPiBackend] Write GPIO:" << pin << "->" << (high ? "1 (3.3V)" : "0 (0V)");
    m_digitalStates[pin] = high;
    emit pinStateChanged(pin, high);
    return true;
}

bool RaspberryPiBackend::readAnalog(const QString& pin, quint32* outRawCount, double* outNormalized) {
    Q_UNUSED(pin);
    if (outRawCount) *outRawCount = 0;
    if (outNormalized) *outNormalized = 0.0;
    m_lastError = "Raspberry Pi native header does not have integrated ADC channels (external I2C/SPI ADC required)";
    return false;
}

bool RaspberryPiBackend::writePwm(const QString& pin, double dutyPercent) {
    qDebug() << "[RaspberryPiBackend] PWM duty on" << pin << "set to" << dutyPercent << "%";
    return true;
}

bool RaspberryPiBackend::spiTransfer(const QString& bus, const QByteArray& txData, QByteArray* outRxData, QString* outLog) {
    if (outRxData) outRxData->fill(0, txData.size());
    if (outLog) *outLog = QString("Raspberry Pi SPI %1 transfer (%2 bytes)").arg(bus).arg(txData.size());
    return true;
}

bool RaspberryPiBackend::i2cWrite(const QString& bus, quint8 address, const QByteArray& data) {
    Q_UNUSED(bus);
    Q_UNUSED(address);
    Q_UNUSED(data);
    return true;
}

bool RaspberryPiBackend::i2cRead(const QString& bus, quint8 address, int length, QByteArray* outData) {
    Q_UNUSED(bus);
    Q_UNUSED(address);
    if (outData) outData->fill(0, length);
    return true;
}

bool RaspberryPiBackend::i2cScan(const QString& sclPin, const QString& sdaPin, QList<quint8>* outFoundAddresses, QString* outLog) {
    Q_UNUSED(sclPin);
    Q_UNUSED(sdaPin);
    if (outFoundAddresses) {
        *outFoundAddresses = { 0x48 }; // Default detectable simulated I2C sensor
    }
    if (outLog) *outLog = "Raspberry Pi I2C-1 scan complete: Found 0x48";
    return true;
}

bool RaspberryPiBackend::uartWrite(const QString& port, const QByteArray& data) {
    qDebug() << "[RaspberryPiBackend] Serial write to" << port << ":" << data.toHex();
    return true;
}

bool RaspberryPiBackend::uartRead(const QString& port, int maxBytes, QByteArray* outData) {
    Q_UNUSED(port);
    Q_UNUSED(maxBytes);
    if (outData) outData->clear();
    return true;
}

bool RaspberryPiBackend::resetTarget() {
    m_lastError = "Target reset not supported directly on Raspberry Pi without external relay";
    return false;
}

QStringList RaspberryPiBackend::availablePins() const {
    return {
        "GPIO2", "GPIO3", "GPIO4", "GPIO17", "GPIO27", "GPIO22", "GPIO10", "GPIO9", "GPIO11",
        "GPIO5", "GPIO6", "GPIO13", "GPIO19", "GPIO26", "GPIO14", "GPIO15", "GPIO18", "GPIO23",
        "GPIO24", "GPIO25", "GPIO8", "GPIO7", "GPIO1", "GPIO12", "GPIO16", "GPIO20", "GPIO21"
    };
}

void RaspberryPiBackend::setSimulatedDigital(const QString& pin, bool high) {
    m_digitalStates[pin] = high;
}

} // namespace Hardware
