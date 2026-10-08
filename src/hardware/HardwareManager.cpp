#include "HardwareManager.h"
#include <QTimer>
#include <QDebug>

namespace Hardware {

HardwareManager& HardwareManager::instance() {
    static HardwareManager s_instance;
    return s_instance;
}

HardwareManager::HardwareManager(QObject* parent)
    : QObject(parent)
    , m_pollTimer(new QTimer(this))
{
    connect(m_pollTimer, &QTimer::timeout, this, &HardwareManager::onPollTimer);
    initializeBuiltinBackends();
}

void HardwareManager::initializeBuiltinBackends() {
    auto stm32 = std::make_shared<STM32Backend>(this);
    auto esp32 = std::make_shared<ESP32Backend>(this);
    auto rpi = std::make_shared<RaspberryPiBackend>(this);
    auto mock = std::make_shared<MockBackend>(this);

    registerBackend(stm32);
    registerBackend(esp32);
    registerBackend(rpi);
    registerBackend(mock);

    // Default to STM32 backend
    setActiveBackend("stm32");
    setTarget(HardwareTarget("target_stm32f030", "STM32F030R8 NUCLEO", "STM32", "STM32F030R8", "NUCLEO-F030R8"));
}

void HardwareManager::resetToDefaults() {
    m_backends.clear();
    initializeBuiltinBackends();
}

void HardwareManager::registerBackend(std::shared_ptr<HardwareBackend> backend) {
    if (!backend) return;
    QString id = backend->backendId().toLower();
    m_backends[id] = backend;

    connect(backend.get(), &HardwareBackend::pinStateChanged, this, &HardwareManager::onBackendPinStateChanged);
    connect(backend.get(), &HardwareBackend::analogValueChanged, this, &HardwareManager::onBackendAnalogValueChanged);
    connect(backend.get(), &HardwareBackend::connectionChanged, this, &HardwareManager::onBackendConnectionChanged);
    connect(backend.get(), &HardwareBackend::errorOccurred, this, &HardwareManager::onBackendError);
}

bool HardwareManager::setActiveBackend(const QString& backendId) {
    QString id = backendId.toLower();
    if (!m_backends.contains(id)) return false;

    m_activeBackend = m_backends.value(id);
    emit backendChanged(id);
    emit connectionStatusChanged(isHardwareConnected(), connectionStatusText());
    return true;
}

HardwareBackend* HardwareManager::activeBackend() const {
    return m_activeBackend ? m_activeBackend.get() : nullptr;
}

QString HardwareManager::activeBackendId() const {
    return m_activeBackend ? m_activeBackend->backendId() : QString();
}

QStringList HardwareManager::registeredBackendIds() const {
    return m_backends.keys();
}

void HardwareManager::setTarget(const HardwareTarget& target) {
    m_currentTarget = target;

    // Switch to corresponding backend if family matches
    QString fam = target.family().toLower();
    if (fam.contains("stm32")) {
        setActiveBackend("stm32");
    } else if (fam.contains("esp32")) {
        setActiveBackend("esp32");
    } else if (fam.contains("raspberry") || fam.contains("rpi")) {
        setActiveBackend("raspberrypi");
    } else if (fam.contains("mock")) {
        setActiveBackend("mock");
    }

    emit targetChanged(m_currentTarget);
}

HardwareCapabilities HardwareManager::currentCapabilities() const {
    if (m_activeBackend) {
        return m_activeBackend->capabilities();
    }
    return HardwareCapabilities();
}

bool HardwareManager::connectHardware() {
    if (m_activeBackend) {
        return m_activeBackend->connectTarget();
    }
    return false;
}

bool HardwareManager::disconnectHardware() {
    if (m_activeBackend) {
        return m_activeBackend->disconnectTarget();
    }
    return false;
}

bool HardwareManager::isHardwareConnected() const {
    if (m_activeBackend) {
        return m_activeBackend->isConnected();
    }
    return false;
}

QString HardwareManager::connectionStatusText() const {
    if (!m_activeBackend) return "No Hardware Backend Configured";
    if (m_activeBackend->connection()) {
        return m_activeBackend->connection()->connectionInfo();
    }
    return m_activeBackend->isConnected() ? "Connected" : "Disconnected";
}

QString HardwareManager::resolvePinReference(const QString& pinOrRef) const {
    return m_currentTarget.resolvePin(pinOrRef);
}

QStringList HardwareManager::availablePins() const {
    if (m_activeBackend) {
        return m_activeBackend->availablePins();
    }
    return QStringList();
}

bool HardwareManager::readDigital(const QString& pinOrRef, bool* outHigh) {
    if (!m_activeBackend) return false;
    QString physicalPin = resolvePinReference(pinOrRef);
    return m_activeBackend->readDigital(physicalPin, outHigh);
}

bool HardwareManager::writeDigital(const QString& pinOrRef, bool high) {
    if (!m_activeBackend) return false;
    QString physicalPin = resolvePinReference(pinOrRef);
    return m_activeBackend->writeDigital(physicalPin, high);
}

bool HardwareManager::readAnalog(const QString& pinOrRef, quint32* outRawCount, double* outNormalized) {
    if (!m_activeBackend) return false;
    QString physicalPin = resolvePinReference(pinOrRef);
    return m_activeBackend->readAnalog(physicalPin, outRawCount, outNormalized);
}

bool HardwareManager::writePwm(const QString& pinOrRef, double dutyPercent) {
    if (!m_activeBackend) return false;
    QString physicalPin = resolvePinReference(pinOrRef);
    return m_activeBackend->writePwm(physicalPin, dutyPercent);
}

bool HardwareManager::spiTransfer(const QString& bus, const QByteArray& txData, QByteArray* outRxData, QString* outLog) {
    if (!m_activeBackend) return false;
    return m_activeBackend->spiTransfer(bus, txData, outRxData, outLog);
}

bool HardwareManager::i2cScan(const QString& sclPin, const QString& sdaPin, QList<quint8>* outFoundAddresses, QString* outLog) {
    if (!m_activeBackend) return false;
    return m_activeBackend->i2cScan(sclPin, sdaPin, outFoundAddresses, outLog);
}

bool HardwareManager::resetTarget() {
    if (!m_activeBackend) return false;
    return m_activeBackend->resetTarget();
}

bool HardwareManager::resolveDataSourceValue(const DataSource& source, QVariant* outValue) {
    if (!outValue) return false;

    QString ref = source.hardwareRef();
    if (ref.isEmpty()) ref = source.id();

    switch (source.type()) {
        case DataSourceType::Gpio: {
            bool high = false;
            if (readDigital(ref, &high)) {
                *outValue = high;
                return true;
            }
            break;
        }
        case DataSourceType::Adc:
        case DataSourceType::Sensor: {
            quint32 raw = 0;
            double norm = 0.0;
            if (readAnalog(ref, &raw, &norm)) {
                *outValue = norm;
                return true;
            }
            break;
        }
        case DataSourceType::Pwm: {
            *outValue = source.value();
            return true;
        }
        case DataSourceType::Constant:
        case DataSourceType::Variable:
        case DataSourceType::Timer:
        case DataSourceType::Calculated: {
            *outValue = source.value();
            return true;
        }
        default:
            break;
    }
    return false;
}

bool HardwareManager::writeDataSourceValue(const DataSource& source, const QVariant& value) {
    QString ref = source.hardwareRef();
    if (ref.isEmpty()) ref = source.id();

    switch (source.type()) {
        case DataSourceType::Gpio: {
            bool high = value.toBool();
            return writeDigital(ref, high);
        }
        case DataSourceType::Pwm: {
            double duty = value.toDouble();
            return writePwm(ref, duty);
        }
        default:
            break;
    }
    return false;
}

void HardwareManager::startLivePolling(int intervalMs) {
    m_livePolling = true;
    m_pollTimer->start(intervalMs);
}

void HardwareManager::stopLivePolling() {
    m_livePolling = false;
    m_pollTimer->stop();
}

void HardwareManager::onPollTimer() {
    // Notify subscribers
}

void HardwareManager::onBackendPinStateChanged(const QString& pin, bool high) {
    emit digitalPinChanged(pin, high);
}

void HardwareManager::onBackendAnalogValueChanged(const QString& pin, quint32 raw, double normalized) {
    emit analogValueChanged(pin, raw, normalized);
}

void HardwareManager::onBackendConnectionChanged(bool connected) {
    emit connectionStatusChanged(connected, connectionStatusText());
}

void HardwareManager::onBackendError(const QString& err) {
    emit errorOccurred(err);
}

} // namespace Hardware
