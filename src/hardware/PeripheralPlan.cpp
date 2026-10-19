#include "PeripheralPlan.h"

namespace Hardware {

namespace {

// Pin modes that carry no configuration and are ignored by planning.
bool isInactiveMode(const QString& mode) {
    return mode.isEmpty()
        || mode.compare(QStringLiteral("None"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("Disabled"), Qt::CaseInsensitive) == 0
        || mode.compare(QStringLiteral("Reset"), Qt::CaseInsensitive) == 0;
}

QString fullSignalName(const QString& peripheralName, const QString& signal) {
    if (peripheralName.isEmpty()) return signal;
    if (signal.startsWith(peripheralName + QLatin1Char('_'), Qt::CaseInsensitive)) return signal;
    return peripheralName + QLatin1Char('_') + signal;
}

} // namespace

bool PeripheralPlan::configIsEmpty(const HardwareConfig& config) {
    for (auto it = config.pins.constBegin(); it != config.pins.constEnd(); ++it) {
        if (!isInactiveMode(it.value().mode)) return false;
    }
    for (auto it = config.peripherals.constBegin(); it != config.peripherals.constEnd(); ++it) {
        if (it.value().enabled) return false;
    }
    return true;
}

PeripheralPlan PeripheralPlan::fromConfig(const HardwareConfig& config, const PinCapabilities* caps) {
    PeripheralPlan plan;
    plan.capabilities = caps;
    plan.m_deviceName = config.deviceId.trimmed();

    for (auto it = config.pins.constBegin(); it != config.pins.constEnd(); ++it) {
        const PinConfiguration& cfg = it.value();
        if (isInactiveMode(cfg.mode)) continue;

        PlannedPin pin;
        pin.pin = cfg.pin.isEmpty() ? it.key() : cfg.pin;
        pin.label = cfg.label;
        pin.mode = cfg.mode;
        pin.signal = cfg.alternateFunction;
        pin.pull = cfg.pull;
        pin.speed = cfg.speed;
        pin.outputType = cfg.outputType;
        pin.initialOutput = cfg.initialOutput;
        pin.interrupt = cfg.interrupt;
        pin.locked = cfg.locked;

        if (caps) {
            pin.pinKnown = caps->hasGpioPin(pin.pin);
            if (const PortFacts* port = caps->portForPin(pin.pin)) {
                pin.afrAvailable = port->afrAvailable;
            }
            if (pin.mode.compare(QStringLiteral("AlternateFunction"), Qt::CaseInsensitive) == 0
                && !pin.signal.isEmpty()) {
                if (const AfFact* fact = caps->afFact(pin.pin, pin.signal)) {
                    pin.afKnown = fact->known();
                    pin.afNumber = fact->af;
                    pin.afSources = fact->sources;
                }
            }
            if (pin.interrupt.compare(QStringLiteral("None"), Qt::CaseInsensitive) != 0) {
                const int index = PinCapabilities::pinIndex(pin.pin);
                if (index >= 0 && caps->system().extiIrqForLine(index)) {
                    pin.extiLine = index;
                }
            }
        }

        plan.pins.append(pin);
    }

    for (auto it = config.peripherals.constBegin(); it != config.peripherals.constEnd(); ++it) {
        const PeripheralConfiguration& cfg = it.value();
        if (!cfg.enabled) continue;

        PlannedPeripheral peripheral;
        peripheral.name = cfg.name.isEmpty() ? it.key() : cfg.name;
        peripheral.type = cfg.type;
        peripheral.assignedPins = cfg.assignedPins;
        peripheral.parameters = cfg.parameters;

        const PeripheralFacts* facts = caps ? caps->peripheral(peripheral.name) : nullptr;
        if (facts) {
            peripheral.factsKnown = true;
            peripheral.available = facts->available;
            peripheral.registerBase = facts->registerBase.asString();
            peripheral.clockBus = facts->clockBus;
            peripheral.clockRegister = facts->clock.registerName;
            peripheral.clockBit = facts->clock.bit;
            peripheral.factSources = facts->allSources();
            if (const IrqFact* irq = facts->primaryIrq()) {
                peripheral.irqNumber = irq->number;
                peripheral.irqName = irq->name;
            }
        }

        for (auto sit = cfg.assignedPins.constBegin(); sit != cfg.assignedPins.constEnd(); ++sit) {
            PlannedSignalUse use;
            use.signal = sit.key();
            use.pin = sit.value();
            if (use.pin.isEmpty()) continue;

            use.fullSignal = fullSignalName(peripheral.name, use.signal);
            if (caps) {
                use.pinKnown = caps->hasGpioPin(use.pin);
                if (const PortFacts* port = caps->portForPin(use.pin)) {
                    use.afrAvailable = port->afrAvailable;
                }
                const AfFact* fact = caps->afFact(use.pin, use.fullSignal);
                if (!fact && use.fullSignal != use.signal) {
                    fact = caps->afFact(use.pin, use.signal);
                }
                if (fact) {
                    use.afKnown = fact->known();
                    use.afNumber = fact->af;
                    use.afSources = fact->sources;
                }
            }
            peripheral.signalUses.append(use);
        }

        plan.peripherals.append(peripheral);
    }

    return plan;
}

} // namespace Hardware
