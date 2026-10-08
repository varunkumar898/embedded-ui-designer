#include "SimulationBackend.h"
#include <QDateTime>
#include <QDebug>
#include <algorithm>

namespace Simulator {

SimulationBackend::SimulationBackend(QObject* parent)
    : HardwareBackend(parent)
    , m_connection(new Hardware::MockConnection(this))
{
    resetAllSimulationData();
}

Hardware::HardwareCapabilities SimulationBackend::capabilities() const {
    Hardware::HardwareCapabilities caps;
    caps.gpioInput = true;
    caps.gpioOutput = true;
    caps.atomicBsrr = true;
    caps.adc = true;
    caps.pwm = true;
    caps.spi = true;
    caps.i2c = true;
    caps.uart = true;
    caps.can = true;
    caps.liveMonitoring = true;
    return caps;
}

bool SimulationBackend::connectTarget() {
    if (m_simulateErrors) {
        m_lastError = "Simulation Error: Target power failure injected";
        emit errorOccurred(m_lastError);
        return false;
    }
    m_connection->setSimulatedConnected(true);
    emit connectionChanged(true);
    return true;
}

bool SimulationBackend::disconnectTarget() {
    m_connection->setSimulatedConnected(false);
    emit connectionChanged(false);
    return true;
}

bool SimulationBackend::isConnected() const {
    return m_connection && m_connection->isConnected();
}

bool SimulationBackend::readDigital(const QString& pin, bool* outHigh) {
    if (!outHigh) return false;
    if (m_simulateErrors) {
        m_lastError = QString("Simulation Error: Digital read failed on pin %1").arg(pin);
        return false;
    }
    *outHigh = m_digitalStates.value(pin, false);
    return true;
}

bool SimulationBackend::writeDigital(const QString& pin, bool high) {
    if (m_simulateErrors) {
        m_lastError = QString("Simulation Error: Digital write failed on pin %1").arg(pin);
        return false;
    }
    m_digitalStates[pin] = high;
    emit pinStateChanged(pin, high);
    return true;
}

bool SimulationBackend::readAnalog(const QString& pin, quint32* outRawCount, double* outNormalized) {
    if (!outRawCount || !outNormalized) return false;
    if (m_simulateErrors) {
        m_lastError = QString("Simulation Error: ADC sampling failed on pin %1").arg(pin);
        return false;
    }
    quint32 raw = m_adcRaw.value(pin, 0);
    double norm = m_adcNormalized.value(pin, static_cast<double>(raw) / 4095.0);
    *outRawCount = raw;
    *outNormalized = std::clamp(norm, 0.0, 1.0);
    return true;
}

bool SimulationBackend::writePwm(const QString& pin, double dutyPercent) {
    if (m_simulateErrors) {
        m_lastError = QString("Simulation Error: PWM write failed on pin %1").arg(pin);
        return false;
    }
    m_pwmDuty[pin] = std::clamp(dutyPercent, 0.0, 100.0);
    return true;
}

bool SimulationBackend::spiTransfer(const QString& bus, const QByteArray& txData, QByteArray* outRxData, QString* outLog) {
    Q_UNUSED(bus);
    if (!outRxData) return false;
    if (m_simulateErrors) {
        m_lastError = "Simulation Error: SPI bus timeout";
        return false;
    }
    // Simulation Loopback / Inversion response
    QByteArray rx;
    for (char c : txData) {
        rx.append(static_cast<char>(~c));
    }
    *outRxData = rx;
    if (outLog) {
        *outLog = QString("Simulated SPI transfer (%1 bytes)").arg(txData.size());
    }
    return true;
}

bool SimulationBackend::i2cWrite(const QString& bus, quint8 address, const QByteArray& data) {
    Q_UNUSED(bus);
    if (m_simulateErrors) {
        m_lastError = QString("Simulation Error: I2C NACK on addr 0x%1").arg(address, 2, 16, QChar('0'));
        return false;
    }
    m_i2cRegisters[address] = data;
    return true;
}

bool SimulationBackend::i2cRead(const QString& bus, quint8 address, int length, QByteArray* outData) {
    Q_UNUSED(bus);
    if (!outData) return false;
    if (m_simulateErrors) {
        m_lastError = QString("Simulation Error: I2C read failed on addr 0x%1").arg(address, 2, 16, QChar('0'));
        return false;
    }
    QByteArray reg = m_i2cRegisters.value(address);
    if (reg.size() >= length) {
        *outData = reg.left(length);
    } else {
        *outData = QByteArray(length, static_cast<char>(0xAA));
    }
    return true;
}

bool SimulationBackend::i2cScan(const QString& sclPin, const QString& sdaPin, QList<quint8>* outFoundAddresses, QString* outLog) {
    Q_UNUSED(sclPin);
    Q_UNUSED(sdaPin);
    if (!outFoundAddresses) return false;
    if (m_simulateErrors) {
        m_lastError = "Simulation Error: I2C Bus Arbitration Lost";
        return false;
    }
    *outFoundAddresses = m_i2cDevices;
    if (outLog) {
        *outLog = QString("Simulation I2C Scan found %1 devices").arg(m_i2cDevices.size());
    }
    return true;
}

bool SimulationBackend::uartWrite(const QString& port, const QByteArray& data) {
    if (m_simulateErrors) {
        m_lastError = QString("Simulation Error: UART write failed on %1").arg(port);
        return false;
    }
    m_uartTxBuffers[port].append(data);
    emit uartDataTransmitted(port, data);
    return true;
}

bool SimulationBackend::uartRead(const QString& port, int maxBytes, QByteArray* outData) {
    if (!outData) return false;
    if (m_simulateErrors) {
        m_lastError = QString("Simulation Error: UART framing error on %1").arg(port);
        return false;
    }
    QByteArray& queue = m_uartRxQueues[port];
    int toRead = std::min(maxBytes, static_cast<int>(queue.size()));
    *outData = queue.left(toRead);
    queue.remove(0, toRead);
    return true;
}

bool SimulationBackend::resetTarget() {
    resetAllSimulationData();
    return true;
}

QStringList SimulationBackend::availablePins() const {
    return {
        "PA0", "PA1", "PA2", "PA3", "PA4", "PA5", "PA6", "PA7",
        "PB0", "PB1", "PB2", "PB3", "PB4", "PB5", "PB6", "PB7", "PB8", "PB9",
        "PC0", "PC1", "PC13"
    };
}

void SimulationBackend::setSimulatedDigital(const QString& pin, bool high) {
    m_digitalStates[pin] = high;
    emit pinStateChanged(pin, high);
}

bool SimulationBackend::simulatedDigital(const QString& pin) const {
    return m_digitalStates.value(pin, false);
}

void SimulationBackend::setSimulatedAdc(const QString& pin, quint32 rawCount, double normalized) {
    rawCount = std::min(rawCount, 4095u);
    double norm = (normalized >= 0.0) ? normalized : (static_cast<double>(rawCount) / 4095.0);
    m_adcRaw[pin] = rawCount;
    m_adcNormalized[pin] = std::clamp(norm, 0.0, 1.0);
    emit analogValueChanged(pin, rawCount, m_adcNormalized[pin]);
}

quint32 SimulationBackend::simulatedAdcRaw(const QString& pin) const {
    return m_adcRaw.value(pin, 0);
}

double SimulationBackend::simulatedAdcNormalized(const QString& pin) const {
    return m_adcNormalized.value(pin, 0.0);
}

void SimulationBackend::setSimulatedPwm(const QString& pin, double dutyPercent) {
    m_pwmDuty[pin] = std::clamp(dutyPercent, 0.0, 100.0);
}

double SimulationBackend::simulatedPwm(const QString& pin) const {
    return m_pwmDuty.value(pin, 0.0);
}

void SimulationBackend::injectUartData(const QString& port, const QByteArray& data) {
    m_uartRxQueues[port].append(data);
}

QByteArray SimulationBackend::getUartTxBuffer(const QString& port) const {
    return m_uartTxBuffers.value(port);
}

void SimulationBackend::clearUartBuffers(const QString& port) {
    m_uartTxBuffers.remove(port);
    m_uartRxQueues.remove(port);
}

void SimulationBackend::injectCanFrame(quint32 id, const QByteArray& payload, bool isExtended) {
    VirtualCanFrame f;
    f.id = id;
    f.payload = payload;
    f.isExtended = isExtended;
    f.timestampMs = QDateTime::currentMSecsSinceEpoch();
    m_canHistory.append(f);
    emit canFrameTransmitted(id, payload);
}

void SimulationBackend::setSimulatedConnected(bool connected) {
    m_connection->setSimulatedConnected(connected);
    emit connectionChanged(connected);
}

void SimulationBackend::resetAllSimulationData() {
    m_digitalStates.clear();
    m_adcRaw.clear();
    m_adcNormalized.clear();
    m_pwmDuty.clear();
    m_uartTxBuffers.clear();
    m_uartRxQueues.clear();
    m_canHistory.clear();
    m_lastError.clear();
    m_simulateErrors = false;

    // Default pin states
    m_digitalStates["PA5"] = false; // Built-in LED LOW
    m_digitalStates["PA4"] = false;
    m_adcRaw["PA0"] = 2048;         // 50% ADC
    m_adcNormalized["PA0"] = 0.5;

    m_connection->setSimulatedConnected(true);
}

} // namespace Simulator
