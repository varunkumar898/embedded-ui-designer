#pragma once

#include "HardwareModel.h"
#include "PinCapabilities.h"

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>

namespace Hardware {

/**
 * Resolved view of the project's hardware configuration (Phase B).
 *
 * The plan is the single source of truth code generation reads from: it pairs
 * every requested pin/peripheral with the sourced capability facts of the
 * selected device. Unknown facts stay explicitly unknown (`afKnown == false`,
 * `factsKnown == false`) so generators can omit configuration and the Pins
 * validator can raise Info diagnostics instead of inventing values.
 */
struct PlannedSignalUse {
    QString signal;         // signal key inside the peripheral, e.g. "TX"
    QString fullSignal;     // capability signal name, e.g. "USART1_TX"
    QString pin;            // e.g. "PA9"
    bool pinKnown = false;  // pin documented for the device
    bool afKnown = false;   // AF number documented for (pin, signal)
    int afNumber = -1;
    bool afrAvailable = true;
    QStringList afSources;
};

struct PlannedPeripheral {
    QString name;           // "USART1"
    QString type;           // "USART"
    QMap<QString, QString> assignedPins;
    QMap<QString, QVariant> parameters;

    bool factsKnown = false;    // capability entry exists for this device
    bool available = false;     // documented as present on the device
    QString registerBase;       // "0x40013800", empty when unknown
    QString clockBus;           // "APB2"
    QString clockRegister;      // "RCC_APB2ENR"
    int clockBit = -1;
    int irqNumber = -1;
    QString irqName;
    QStringList factSources;

    QVector<PlannedSignalUse> signalUses;
};

struct PlannedPin {
    QString pin;
    QString label;
    QString mode;               // "GPIO_Input", "GPIO_Output", "Analog", "AlternateFunction"
    QString signal;             // alternate function name when mode == AlternateFunction
    QString pull;
    QString speed;
    QString outputType;
    QString initialOutput;
    QString interrupt;          // "None", "RisingEdge", ...
    bool locked = false;

    bool pinKnown = false;
    bool afKnown = false;
    int afNumber = -1;
    bool afrAvailable = true;
    QStringList afSources;
    int extiLine = -1;          // set when interrupt != "None" and pin is known
};

/**
 * Resolved clock of one bus ("APB1"/"APB2"/"AHB") for rate calculations.
 *
 * Resolution rules (Phase C1): the bus value from the project clock
 * configuration is used when present. When the bus value is missing, SYSCLK is
 * used (APB prescaler 1 assumption); when the configuration contains no clock
 * information at all, the documented reset default is assumed. Both fallbacks
 * are reported so generators can state them in generated comments and emit
 * diagnostics instead of silently guessing.
 */
struct BusClockResolution {
    int mhz = 0;                   // resolved frequency in MHz
    bool assumedHsi = false;       // nothing configured: 8 MHz HSI reset default (RM0360 Rev 5 sec 7.2.6)
    bool assumedApbIsSys = false;  // bus value missing: assumed equal to SYSCLK
};

BusClockResolution resolveBusClock(const ClockConfiguration& clock, const QString& busName);

class PeripheralPlan {
public:
    // Builds the plan from the project's hardware configuration. `caps` may be
    // null (no verified data for the device) - the plan then reports everything
    // as unknown rather than failing.
    static PeripheralPlan fromConfig(const HardwareConfig& config, const PinCapabilities* caps);

    bool isEmpty() const { return pins.isEmpty() && peripherals.isEmpty(); }
    QString deviceName() const { return m_deviceName; }

    QVector<PlannedPin> pins;                 // configured pins only
    QVector<PlannedPeripheral> peripherals;   // enabled peripherals only
    const PinCapabilities* capabilities = nullptr;

    // True when the configuration asks for anything at all.
    static bool configIsEmpty(const HardwareConfig& config);

private:
    QString m_deviceName;
};

} // namespace Hardware
