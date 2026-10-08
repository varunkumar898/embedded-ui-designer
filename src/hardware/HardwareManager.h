#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QPointer>
#include <QJsonObject>
#include <QTimer>
#include <memory>
#include "HardwareCapabilities.h"
#include "HardwareTarget.h"
#include "HardwareBackend.h"
#include "STM32Backend.h"
#include "ESP32Backend.h"
#include "RaspberryPiBackend.h"
#include "MockBackend.h"
#include "DataSource.h"

namespace Hardware {

/**
 * @brief Central coordinator and singleton entry point for the Hardware Abstraction Layer (HAL).
 * Decouples Project, Canvas, Components, and DataSources from specific hardware targets and debug probes.
 */
class HardwareManager : public QObject {
    Q_OBJECT

public:
    static HardwareManager& instance();

    // Backend Registration & Selection
    void registerBackend(std::shared_ptr<HardwareBackend> backend);
    bool setActiveBackend(const QString& backendId);
    HardwareBackend* activeBackend() const;
    QString activeBackendId() const;
    QStringList registeredBackendIds() const;

    // Target Management
    void setTarget(const HardwareTarget& target);
    HardwareTarget currentTarget() const { return m_currentTarget; }
    HardwareCapabilities currentCapabilities() const;

    // Connection Control
    bool connectHardware();
    bool disconnectHardware();
    bool isHardwareConnected() const;
    QString connectionStatusText() const;

    // High-Level Unified Hardware I/O
    bool readDigital(const QString& pinOrRef, bool* outHigh);
    bool writeDigital(const QString& pinOrRef, bool high);
    bool readAnalog(const QString& pinOrRef, quint32* outRawCount, double* outNormalized);
    bool writePwm(const QString& pinOrRef, double dutyPercent);
    bool spiTransfer(const QString& bus, const QByteArray& txData, QByteArray* outRxData, QString* outLog = nullptr);
    bool i2cScan(const QString& sclPin, const QString& sdaPin, QList<quint8>* outFoundAddresses, QString* outLog = nullptr);
    bool resetTarget();

    // Pin & Hardware Reference Resolution
    QString resolvePinReference(const QString& pinOrRef) const;
    QStringList availablePins() const;

    // DataSource Hardware Resolution
    bool resolveDataSourceValue(const DataSource& source, QVariant* outValue);
    bool writeDataSourceValue(const DataSource& source, const QVariant& value);

    // Live Polling
    bool isLivePolling() const { return m_livePolling; }
    void startLivePolling(int intervalMs = 50);
    void stopLivePolling();

    // Reset backends to defaults (useful for test resets)
    void resetToDefaults();

signals:
    void targetChanged(const HardwareTarget& target);
    void backendChanged(const QString& backendId);
    void connectionStatusChanged(bool connected, const QString& info);
    void digitalPinChanged(const QString& pin, bool high);
    void analogValueChanged(const QString& pin, quint32 raw, double normalized);
    void errorOccurred(const QString& error);

private slots:
    void onBackendPinStateChanged(const QString& pin, bool high);
    void onBackendAnalogValueChanged(const QString& pin, quint32 raw, double normalized);
    void onBackendConnectionChanged(bool connected);
    void onBackendError(const QString& err);
    void onPollTimer();

private:
    explicit HardwareManager(QObject* parent = nullptr);
    ~HardwareManager() override = default;

    void initializeBuiltinBackends();

    QMap<QString, std::shared_ptr<HardwareBackend>> m_backends;
    std::shared_ptr<HardwareBackend> m_activeBackend;
    HardwareTarget m_currentTarget;
    QTimer* m_pollTimer = nullptr;
    bool m_livePolling = false;
};

} // namespace Hardware
