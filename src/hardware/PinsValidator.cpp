#include "PinsValidator.h"

#include "PeripheralPlan.h"

#include <QMap>
#include <QSet>
#include <QStringList>

namespace Hardware {

namespace {

PinIssue makeIssue(PinIssue::Severity severity, const QString& message,
                   const QString& pin = QString(), const QString& peripheral = QString()) {
    PinIssue issue;
    issue.severity = severity;
    issue.message = message;
    issue.pin = pin;
    issue.peripheral = peripheral;
    return issue;
}

bool isKnownMode(const QString& mode) {
    return mode.compare(QStringLiteral("GPIO_Input"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("GPIO_Output"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("Analog"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("AlternateFunction"), Qt::CaseInsensitive) == 0;
}

bool isKnownInterrupt(const QString& interrupt) {
    return interrupt.compare(QStringLiteral("None"), Qt::CaseInsensitive) == 0
        || interrupt.compare(QStringLiteral("RisingEdge"), Qt::CaseInsensitive) == 0
        || interrupt.compare(QStringLiteral("FallingEdge"), Qt::CaseInsensitive) == 0
        || interrupt.compare(QStringLiteral("BothEdges"), Qt::CaseInsensitive) == 0;
}

bool signalDocumentedElsewhere(const PinCapabilities* caps, const QString& signal,
                               const QString& exceptPin) {
    if (!caps) return false;
    for (const QString& pin : caps->gpioPins()) {
        if (pin == exceptPin) continue;
        if (caps->afFact(pin, signal)) return true;
    }
    return false;
}

// Shared diagnostic for "alternate function cannot be resolved" cases.
// The same (pin, signal) pair can appear both as a standalone pin
// configuration and as a peripheral assignment; report it once.
void appendAfIssue(QVector<PinIssue>* out, QSet<QString>* reported,
                   const PinCapabilities* caps, const QString& deviceName,
                   const QString& pin, const QString& signal, const QString& peripheral) {
    const QString key = pin + QLatin1Char('\x1f') + signal;
    if (reported->contains(key)) return;
    reported->insert(key);

    if (const PortFacts* port = caps->portForPin(pin)) {
        if (!port->afrAvailable) {
            out->append(makeIssue(PinIssue::Severity::Info,
                QStringLiteral("%1: port %2 has no alternate-function register (AFR) on %3; '%4' on %5 cannot be configured via AFR.")
                    .arg(deviceName, port->name, deviceName, signal, pin),
                pin, peripheral));
            return;
        }
    }
    const QString suffix = peripheral.isEmpty()
        ? QString()
        : QStringLiteral(" (peripheral %1)").arg(peripheral);
    if (signalDocumentedElsewhere(caps, signal, pin)) {
        out->append(makeIssue(PinIssue::Severity::Warning,
            QStringLiteral("%1: alternate function '%2' on pin %3 is not documented for %4%5; check the datasheet alternate-function table.")
                .arg(deviceName, signal, pin, deviceName, suffix),
            pin, peripheral));
    } else {
        out->append(makeIssue(PinIssue::Severity::Info,
            QStringLiteral("%1: AF number for '%2' on %3 is not documented in the verified data%4; the generator will omit this configuration.")
                .arg(deviceName, signal, pin, suffix),
            pin, peripheral));
    }
}

} // namespace

QVector<PinIssue> PinsValidator::validate(const HardwareConfig& config, const PinCapabilities* caps) {
    QVector<PinIssue> out;
    if (PeripheralPlan::configIsEmpty(config)) return out;

    const QString deviceName = config.deviceId.trimmed().isEmpty()
        ? QStringLiteral("the selected device")
        : config.deviceId.trimmed();

    if (!caps) {
        out.append(makeIssue(PinIssue::Severity::Info,
            QStringLiteral("Pin/peripheral configuration is present but no verified pin capability data is available for %1; values cannot be validated or generated.")
                .arg(deviceName)));
        return out;
    }

    const PeripheralPlan plan = PeripheralPlan::fromConfig(config, caps);

    // pin -> list of distinct function keys that use it
    QMap<QString, QStringList> pinUses;
    // (pin, signal) pairs already reported, so pin + peripheral entries do not
    // produce duplicate diagnostics.
    QSet<QString> reportedAf;

    for (const PlannedPin& pin : plan.pins) {
        if (!pin.pinKnown) {
            // The capability data does not cover this pin. That is "unknown",
            // not "invalid": report it and keep unknowns unknown.
            out.append(makeIssue(PinIssue::Severity::Info,
                QStringLiteral("No verified pin data for '%1' on %2; its configuration cannot be validated or generated.")
                    .arg(pin.pin, deviceName), pin.pin));
            if (pin.mode.compare(QStringLiteral("AlternateFunction"), Qt::CaseInsensitive) == 0 && !pin.signal.isEmpty()) {
                pinUses[pin.pin] << QStringLiteral("AF:%1").arg(pin.signal);
            } else {
                pinUses[pin.pin] << QStringLiteral("mode:%1").arg(pin.mode);
            }
            continue;
        }

        if (!isKnownMode(pin.mode)) {
            out.append(makeIssue(PinIssue::Severity::Warning,
                QStringLiteral("Pin '%1' has unrecognized mode '%2'; the generator will ignore it.")
                    .arg(pin.pin, pin.mode), pin.pin));
            continue;
        }

        if (pin.mode.compare(QStringLiteral("AlternateFunction"), Qt::CaseInsensitive) == 0) {
            if (pin.signal.isEmpty()) {
                out.append(makeIssue(PinIssue::Severity::Warning,
                    QStringLiteral("Pin '%1' is set to AlternateFunction but no alternate function is selected.")
                        .arg(pin.pin), pin.pin));
            } else if (!pin.afKnown) {
                appendAfIssue(&out, &reportedAf, caps, deviceName, pin.pin, pin.signal, QString());
            }
            pinUses[pin.pin] << QStringLiteral("AF:%1").arg(pin.signal);
        } else {
            pinUses[pin.pin] << QStringLiteral("mode:%1").arg(pin.mode);
        }

        if (!isKnownInterrupt(pin.interrupt)) {
            out.append(makeIssue(PinIssue::Severity::Warning,
                QStringLiteral("Pin '%1' has unrecognized interrupt mode '%2'.")
                    .arg(pin.pin, pin.interrupt), pin.pin));
        } else if (pin.interrupt.compare(QStringLiteral("None"), Qt::CaseInsensitive) != 0) {
            const SystemFacts& system = caps->system();
            if (!system.extiAvailable) {
                out.append(makeIssue(PinIssue::Severity::Info,
                    QStringLiteral("Pin '%1' requests an interrupt but EXTI capability data is not available for %2; interrupt configuration will be omitted.")
                        .arg(pin.pin, deviceName), pin.pin));
            } else if (pin.pinKnown && pin.extiLine < 0) {
                out.append(makeIssue(PinIssue::Severity::Info,
                    QStringLiteral("Pin '%1' requests an interrupt but its EXTI line mapping is not documented for %2.")
                        .arg(pin.pin, deviceName), pin.pin));
            }
        }
    }

    for (const PlannedPeripheral& peripheral : plan.peripherals) {
        if (!peripheral.factsKnown) {
            out.append(makeIssue(PinIssue::Severity::Info,
                QStringLiteral("No verified capability data for peripheral '%1' on %2; its configuration cannot be generated.")
                    .arg(peripheral.name, deviceName), QString(), peripheral.name));
        } else if (!peripheral.available) {
            out.append(makeIssue(PinIssue::Severity::Error,
                QStringLiteral("Peripheral '%1' is not documented as present on %2.")
                    .arg(peripheral.name, deviceName), QString(), peripheral.name));
        }

        for (const PlannedSignalUse& use : peripheral.signalUses) {
            if (!use.pinKnown) {
                out.append(makeIssue(PinIssue::Severity::Info,
                    QStringLiteral("No verified pin data for '%1' assigned to %2.%3 on %4; the signal cannot be validated or generated.")
                        .arg(use.pin, peripheral.name, use.signal, deviceName),
                    use.pin, peripheral.name));
                pinUses[use.pin] << QStringLiteral("AF:%1").arg(use.fullSignal);
                continue;
            }
            if (!use.afKnown && !use.fullSignal.isEmpty()) {
                appendAfIssue(&out, &reportedAf, caps, deviceName, use.pin, use.fullSignal, peripheral.name);
            }
            pinUses[use.pin] << QStringLiteral("AF:%1").arg(use.fullSignal);
        }
    }

    // Conflicts: one pin carrying two different function keys.
    for (auto it = pinUses.constBegin(); it != pinUses.constEnd(); ++it) {
        QStringList distinct = it.value();
        distinct.removeDuplicates();
        if (distinct.size() > 1) {
            out.append(makeIssue(PinIssue::Severity::Error,
                QStringLiteral("Pin '%1' is assigned to multiple functions (%2); resolve the conflict.")
                    .arg(it.key(), distinct.join(QStringLiteral(", "))), it.key()));
        }
    }

    return out;
}

} // namespace Hardware
