#include "PinMuxEngine.h"

namespace Hardware {

PinMuxEngine::PinMuxEngine(QObject* parent)
    : QObject(parent)
{
}

void PinMuxEngine::setDevice(const DeviceDefinition& device) {
    m_device = device;
    m_config.clear();
    m_config.deviceId = device.partNumber;
    m_config.vendor = device.vendor;
    m_config.family = device.family;
    m_config.series = device.series;
    m_config.architecture = device.architecture;
    m_config.core = device.core;
    m_config.package = device.package;
    m_config.flashBytes = device.flashBytes;
    m_config.ramBytes = device.ramBytes;
    m_config.clock.sysClockMhz = device.maxClockMhz;
    emit configurationChanged();
}

void PinMuxEngine::loadConfig(const HardwareConfig& config) {
    m_config = config;
    emit configurationChanged();
}

bool PinMuxEngine::isPinDefined(const QString& pin) const {
    return m_device.findPin(pin) != nullptr;
}

bool PinMuxEngine::isPinOccupied(const QString& pin) const {
    const PinDefinition* pinDef = m_device.findPin(pin);
    if (pinDef && (pinDef->isPower || pinDef->isGround || pinDef->isReset || pinDef->isReserved)) {
        return true;
    }
    if (m_config.pins.contains(pin)) {
        const auto& pcfg = m_config.pins.value(pin);
        return pcfg.mode != "None" && pcfg.mode != "Disabled";
    }
    return false;
}

QString PinMuxEngine::getPinOwner(const QString& pin) const {
    const PinDefinition* pinDef = m_device.findPin(pin);
    if (pinDef) {
        if (pinDef->isPower) return "Power (Supply)";
        if (pinDef->isGround) return "Ground (VSS/GND)";
        if (pinDef->isReset) return "System Reset";
        if (pinDef->isReserved) return "Reserved / Debug System";
    }

    if (m_config.pins.contains(pin)) {
        const auto& pcfg = m_config.pins.value(pin);
        if (!pcfg.alternateFunction.isEmpty()) {
            return pcfg.alternateFunction;
        }
        if (pcfg.mode == "GPIO_Output") return "GPIO Output";
        if (pcfg.mode == "GPIO_Input") return "GPIO Input";
        if (pcfg.mode == "Analog") return "Analog Mode";
        if (pcfg.mode != "None" && pcfg.mode != "Disabled") return pcfg.mode;
    }

    // Check if any active peripheral assigned this pin
    for (auto it = m_config.peripherals.begin(); it != m_config.peripherals.end(); ++it) {
        if (it.value().enabled) {
            for (auto sit = it.value().assignedPins.begin(); sit != it.value().assignedPins.end(); ++sit) {
                if (sit.value().compare(pin, Qt::CaseInsensitive) == 0) {
                    return QString("%1_%2").arg(it.key(), sit.key());
                }
            }
        }
    }

    return QString();
}

QStringList PinMuxEngine::getFreePins() const {
    QStringList freePins;
    for (const auto& pinDef : m_device.pins) {
        if (!pinDef.isPower && !pinDef.isGround && !pinDef.isReset && !pinDef.isReserved) {
            if (!isPinOccupied(pinDef.name)) {
                freePins.append(pinDef.name);
            }
        }
    }
    return freePins;
}

QStringList PinMuxEngine::getAllMcuPins() const {
    QStringList list;
    for (const auto& pinDef : m_device.pins) {
        list.append(pinDef.name);
    }
    return list;
}

bool PinMuxEngine::isAlternateFunctionSupported(const QString& pin, const QString& af) const {
    const PinDefinition* pinDef = m_device.findPin(pin);
    if (!pinDef) return false;
    if (af == "GPIO_Input" || af == "GPIO_Output" || af == "Analog" || af == "None" || af == "Disabled") {
        return true;
    }
    for (const QString& existingAf : pinDef->alternateFunctions) {
        if (existingAf.compare(af, Qt::CaseInsensitive) == 0) return true;
    }
    return false;
}

ConflictInfo PinMuxEngine::checkPinConflict(const QString& pin, const QString& requestedFunction) const {
    ConflictInfo info;
    info.pin = pin;
    info.proposedOwner = requestedFunction;

    const PinDefinition* pinDef = m_device.findPin(pin);
    if (!pinDef) {
        info.hasConflict = true;
        info.reason = QString("Pin '%1' does not exist on device %2.").arg(pin, m_device.partNumber);
        return info;
    }

    if (pinDef->isPower) {
        info.hasConflict = true;
        info.currentOwner = "Power";
        info.reason = QString("Pin '%1' is a dedicated power supply rail and cannot be reconfigured.").arg(pin);
        return info;
    }
    if (pinDef->isGround) {
        info.hasConflict = true;
        info.currentOwner = "Ground";
        info.reason = QString("Pin '%1' is a dedicated ground reference and cannot be reconfigured.").arg(pin);
        return info;
    }
    if (pinDef->isReset) {
        info.hasConflict = true;
        info.currentOwner = "Reset";
        info.reason = QString("Pin '%1' is the hardware reset signal.").arg(pin);
        return info;
    }
    if (pinDef->isReserved) {
        info.hasConflict = true;
        info.currentOwner = "Reserved";
        info.reason = QString("Pin '%1' is reserved (e.g. SWD/JTAG debug line).").arg(pin);
        return info;
    }

    // Check alternate function capability
    if (!requestedFunction.isEmpty() && requestedFunction != "None" && requestedFunction != "Disabled" &&
        requestedFunction != "GPIO_Input" && requestedFunction != "GPIO_Output" && requestedFunction != "Analog") {
        if (!isAlternateFunctionSupported(pin, requestedFunction)) {
            info.hasConflict = true;
            info.reason = QString("Pin '%1' does not support alternate function '%2'.").arg(pin, requestedFunction);
            return info;
        }
    }

    // Check if already occupied
    QString currentOwner = getPinOwner(pin);
    if (!currentOwner.isEmpty() && currentOwner != requestedFunction) {
        info.hasConflict = true;
        info.currentOwner = currentOwner;
        info.reason = QString("Pin %1 is already assigned to: %2. Do you want to replace the existing assignment?").arg(pin, currentOwner);
        return info;
    }

    info.hasConflict = false;
    return info;
}

bool PinMuxEngine::assignPin(const QString& pin, const PinConfiguration& config, bool forceReplace, ConflictInfo* outConflict) {
    ConflictInfo conflict = checkPinConflict(pin, config.alternateFunction.isEmpty() ? config.mode : config.alternateFunction);
    if (outConflict) *outConflict = conflict;

    if (conflict.hasConflict && !forceReplace) {
        return false;
    }

    // If force replacing and pin was used by a peripheral, clear it from that peripheral
    if (conflict.hasConflict && forceReplace) {
        for (auto pit = m_config.peripherals.begin(); pit != m_config.peripherals.end(); ++pit) {
            for (auto sit = pit.value().assignedPins.begin(); sit != pit.value().assignedPins.end();) {
                if (sit.value().compare(pin, Qt::CaseInsensitive) == 0) {
                    sit = pit.value().assignedPins.erase(sit);
                } else {
                    ++sit;
                }
            }
        }
    }

    PinConfiguration finalCfg = config;
    finalCfg.pin = pin;
    if (finalCfg.gpio.isEmpty()) finalCfg.gpio = pin;
    m_config.pins[pin] = finalCfg;

    emit pinChanged(pin);
    emit configurationChanged();
    return true;
}

bool PinMuxEngine::unassignPin(const QString& pin) {
    if (!m_config.pins.contains(pin)) return false;
    m_config.pins.remove(pin);

    // Also remove from any peripheral
    for (auto pit = m_config.peripherals.begin(); pit != m_config.peripherals.end(); ++pit) {
        for (auto sit = pit.value().assignedPins.begin(); sit != pit.value().assignedPins.end();) {
            if (sit.value().compare(pin, Qt::CaseInsensitive) == 0) {
                sit = pit.value().assignedPins.erase(sit);
            } else {
                ++sit;
            }
        }
    }

    emit pinChanged(pin);
    emit configurationChanged();
    return true;
}

ConflictInfo PinMuxEngine::checkPeripheralAssignmentConflict(const QString& peripheralName, const QMap<QString, QString>& signalMappings) const {
    ConflictInfo info;
    for (auto it = signalMappings.begin(); it != signalMappings.end(); ++it) {
        QString signalName = it.key();
        QString pin = it.value();
        if (pin.isEmpty()) continue;

        QString proposedAf = QString("%1_%2").arg(peripheralName, signalName);
        ConflictInfo pinConflict = checkPinConflict(pin, proposedAf);
        if (pinConflict.hasConflict) {
            return pinConflict;
        }
    }
    info.hasConflict = false;
    return info;
}

bool PinMuxEngine::assignPeripheral(const QString& peripheralName, const QString& type, const QMap<QString, QString>& signalMappings, const QMap<QString, QVariant>& params, bool forceReplace) {
    ConflictInfo conflict = checkPeripheralAssignmentConflict(peripheralName, signalMappings);
    if (conflict.hasConflict && !forceReplace) {
        return false;
    }

    PeripheralConfiguration pcfg;
    pcfg.name = peripheralName;
    pcfg.type = type;
    pcfg.enabled = true;
    pcfg.assignedPins = signalMappings;
    pcfg.parameters = params;
    m_config.peripherals[peripheralName] = pcfg;

    // Also configure the pins in m_config.pins
    for (auto it = signalMappings.begin(); it != signalMappings.end(); ++it) {
        QString pin = it.value();
        if (pin.isEmpty()) continue;
        PinConfiguration pinCfg;
        pinCfg.pin = pin;
        pinCfg.gpio = pin;
        pinCfg.mode = "AlternateFunction";
        pinCfg.alternateFunction = QString("%1_%2").arg(peripheralName, it.key());
        pinCfg.speed = "VeryHigh";
        m_config.pins[pin] = pinCfg;
        emit pinChanged(pin);
    }

    emit peripheralChanged(peripheralName);
    emit configurationChanged();
    return true;
}

void PinMuxEngine::unassignPeripheral(const QString& peripheralName) {
    if (!m_config.peripherals.contains(peripheralName)) return;

    PeripheralConfiguration pcfg = m_config.peripherals.value(peripheralName);
    for (const QString& pin : pcfg.assignedPins.values()) {
        if (m_config.pins.contains(pin)) {
            m_config.pins.remove(pin);
            emit pinChanged(pin);
        }
    }
    m_config.peripherals.remove(peripheralName);

    emit peripheralChanged(peripheralName);
    emit configurationChanged();
}

QStringList PinMuxEngine::availablePinsForSignal(const QString& peripheralName, const QString& signalName) const {
    return m_device.availablePinsForSignal(peripheralName, signalName);
}

bool PinMuxEngine::autoAssignPeripheral(const QString& peripheralName, QString* outError) {
    const PeripheralDescriptor* desc = m_device.findPeripheral(peripheralName);
    if (!desc) {
        if (outError) *outError = QString("Peripheral '%1' not found on target device.").arg(peripheralName);
        return false;
    }

    QMap<QString, QString> assignedSignals;
    for (auto it = desc->signalOptions.begin(); it != desc->signalOptions.end(); ++it) {
        QString signalName = it.key();
        const QStringList& candidates = it.value();
        QString selectedPin;
        for (const QString& candidate : candidates) {
            if (!isPinOccupied(candidate)) {
                selectedPin = candidate;
                break;
            }
        }
        if (selectedPin.isEmpty() && !candidates.isEmpty()) {
            if (outError) *outError = QString("Could not find a free pin for signal '%1' of peripheral '%2'.").arg(signalName, peripheralName);
            return false;
        }
        assignedSignals[signalName] = selectedPin;
    }

    QMap<QString, QVariant> defaultParams;
    for (auto it = desc->defaults.begin(); it != desc->defaults.end(); ++it) {
        defaultParams[it.key()] = it.value().toVariant();
    }

    return assignPeripheral(peripheralName, desc->type, assignedSignals, defaultParams, true);
}

bool PinMuxEngine::validateConfiguration(QStringList* outErrors, QStringList* outWarnings) const {
    bool valid = true;

    // Check duplicate pin usage
    QMap<QString, QStringList> pinUsage;
    for (auto it = m_config.peripherals.begin(); it != m_config.peripherals.end(); ++it) {
        if (!it.value().enabled) continue;
        for (auto sit = it.value().assignedPins.begin(); sit != it.value().assignedPins.end(); ++sit) {
            const QString& pin = sit.value();
            if (!pin.isEmpty()) {
                pinUsage[pin].append(QString("%1:%2").arg(it.key(), sit.key()));
            }
        }
    }

    for (auto it = pinUsage.begin(); it != pinUsage.end(); ++it) {
        if (it.value().size() > 1) {
            valid = false;
            if (outErrors) {
                outErrors->append(QString("Pin Conflict: Pin %1 is concurrently assigned to multiple functions: %2")
                                      .arg(it.key(), it.value().join(", ")));
            }
        }
    }

    // Check peripheral completeness
    for (auto it = m_config.peripherals.begin(); it != m_config.peripherals.end(); ++it) {
        if (!it.value().enabled) continue;
        const PeripheralDescriptor* desc = m_device.findPeripheral(it.key());
        if (!desc) continue;

        // Check required signals (e.g. for SPI: SCK, MOSI; for I2C: SCL, SDA; for UART: TX)
        if (desc->type == "SPI") {
            if (!it.value().assignedPins.contains("SCK") || it.value().assignedPins.value("SCK").isEmpty()) {
                valid = false;
                if (outErrors) outErrors->append(QString("%1: SPI clock pin (SCK) is required but unassigned.").arg(it.key()));
            }
        } else if (desc->type == "I2C") {
            if (!it.value().assignedPins.contains("SCL") || it.value().assignedPins.value("SCL").isEmpty() ||
                !it.value().assignedPins.contains("SDA") || it.value().assignedPins.value("SDA").isEmpty()) {
                valid = false;
                if (outErrors) outErrors->append(QString("%1: Both SCL and SDA pins must be assigned.").arg(it.key()));
            }
        } else if (desc->type == "USART" || desc->type == "UART") {
            if (!it.value().assignedPins.contains("TX") && !it.value().assignedPins.contains("RX")) {
                if (outWarnings) outWarnings->append(QString("%1: Neither TX nor RX pins are assigned.").arg(it.key()));
            }
        }
    }

    return valid;
}

} // namespace Hardware
