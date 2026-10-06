#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include "HardwareModel.h"

namespace Hardware {

struct ConflictInfo {
    bool hasConflict = false;
    QString pin;
    QString currentOwner;       // e.g. "SPI1_SCK" or "GPIO_Output"
    QString proposedOwner;      // e.g. "GPIO_Output"
    QString reason;             // Human-readable explanation
};

class PinMuxEngine : public QObject {
    Q_OBJECT

public:
    explicit PinMuxEngine(QObject* parent = nullptr);

    void setDevice(const DeviceDefinition& device);
    DeviceDefinition currentDevice() const { return m_device; }

    void loadConfig(const HardwareConfig& config);
    HardwareConfig currentConfig() const { return m_config; }

    // Pin inspection
    bool isPinDefined(const QString& pin) const;
    bool isPinOccupied(const QString& pin) const;
    QString getPinOwner(const QString& pin) const;
    QStringList getFreePins() const;
    QStringList getAllMcuPins() const;

    // Pinmux conflict checking
    ConflictInfo checkPinConflict(const QString& pin, const QString& requestedFunction) const;
    bool isAlternateFunctionSupported(const QString& pin, const QString& af) const;

    // Assignment & modification
    bool assignPin(const QString& pin, const PinConfiguration& config, bool forceReplace, ConflictInfo* outConflict = nullptr);
    bool unassignPin(const QString& pin);

    // Peripheral pin assignment
    ConflictInfo checkPeripheralAssignmentConflict(const QString& peripheralName, const QMap<QString, QString>& signalMappings) const;
    bool assignPeripheral(const QString& peripheralName, const QString& type, const QMap<QString, QString>& signalMappings, const QMap<QString, QVariant>& params, bool forceReplace);
    void unassignPeripheral(const QString& peripheralName);

    // Available pin query for specific peripheral signal
    QStringList availablePinsForSignal(const QString& peripheralName, const QString& signalName) const;

    // Validation
    bool validateConfiguration(QStringList* outErrors = nullptr, QStringList* outWarnings = nullptr) const;

    // Auto-assignment
    bool autoAssignPeripheral(const QString& peripheralName, QString* outError = nullptr);

signals:
    void configurationChanged();
    void pinChanged(const QString& pin);
    void peripheralChanged(const QString& peripheralName);

private:
    DeviceDefinition m_device;
    HardwareConfig m_config;
};

} // namespace Hardware
