#include "TargetHalGenerator.h"
#include "analyzer/CapabilityDatabase.h"
#include "driverpack/DriverPackRegistry.h"
#include "driverpack/CustomPackManager.h"
#include <algorithm>
#include "hardware/PeripheralPlan.h"

namespace CodeGen {

namespace {

struct TargetResolution {
    bool hasTarget = false;
    bool isDefaultF030 = false;
    bool hasDriverPack = false;
    QString deviceName;
    QString boardDisplayName;
    QString family;
    QMap<QString, PinDef> pinMap;
    QMap<QString, QStringList> protocols;
};

TargetResolution resolveTarget(const Project* project, const QString& targetMcu = QString()) {
    TargetResolution res;
    QString boardId = project ? project->hardwareConfig().boardId.trimmed() : QString();
    QString deviceId = project ? project->hardwareConfig().deviceId.trimmed() : QString();
    DeviceProfile prof = project ? project->getDeviceProfile() : DeviceProfile();
    if (deviceId.isEmpty()) {
        deviceId = prof.name.trimmed();
    }
    if (deviceId.isEmpty() && !targetMcu.isEmpty() &&
        targetMcu != "stm32" && targetMcu != "generic" &&
        targetMcu != "esp32" && targetMcu != "pico") {
        deviceId = targetMcu.trimmed();
    }

    if (boardId.isEmpty() && deviceId.isEmpty()) {
        res.hasTarget = false;
        res.isDefaultF030 = true;
        res.hasDriverPack = true;
        res.deviceName = "STM32F030R8";
        res.family = "STM32";
        res.boardDisplayName = "STM32 NUCLEO-F030R8";
        BoardDef nucleo = BoardRegistry::instance().board("nucleo_f030r8");
        if (nucleo.isValid()) {
            res.pinMap = nucleo.pinMap;
            res.protocols = nucleo.protocols;
        } else {
            BoardDef synthetic;
            synthetic.family = res.family;
            BoardRegistry::instance().ensureFamilyPinDefaults(synthetic);
            res.pinMap = synthetic.pinMap;
        }
        if (res.protocols.isEmpty()) {
            res.protocols["GPIO"] = {"GPIO"};
            res.protocols["UART"] = {"UART1"};
            res.protocols["SPI"] = {"SPI1"};
            res.protocols["I2C"] = {"I2C1"};
            res.protocols["ADC"] = {"ADC1"};
            res.protocols["PWM"] = {"PWM1"};
        }
        return res;
    }

    res.hasTarget = true;
    BoardDef boardDef;
    if (!boardId.isEmpty()) {
        boardDef = BoardRegistry::instance().board(boardId);
    }

    if (boardDef.isValid()) {
        res.boardDisplayName = !boardDef.displayName.isEmpty() ? boardDef.displayName : boardDef.id;
        res.family = boardDef.family;
        res.deviceName = !boardDef.mcu.isEmpty() ? boardDef.mcu : boardId;
        res.pinMap = boardDef.pinMap;
        res.protocols = boardDef.protocols;
    } else {
        res.deviceName = !deviceId.isEmpty() ? deviceId : prof.name;
        res.family = !prof.family.isEmpty() ? prof.family : (project ? project->hardwareConfig().family : QString());
        if (res.deviceName == "STM32F030R8") {
            res.boardDisplayName = "STM32 NUCLEO-F030R8";
        } else {
            res.boardDisplayName = res.deviceName;
        }
    }

    if (driverpack::CustomPackManager::instance().isCustomDevice(res.deviceName)) {
        driverpack::CustomPackDef customPack = driverpack::CustomPackManager::instance().getPack(res.deviceName);
        res.family = customPack.family.isEmpty() ? "Custom" : customPack.family;
        res.boardDisplayName = customPack.name + " (Custom Pack)";
        res.pinMap.clear();
        for (const auto& pin : customPack.pinTable) {
            PinDef pd;
            pd.function = pin.function;
            res.pinMap[pin.pin] = pd;
        }
        for (const auto& p : customPack.peripherals) {
            if (!p.type.isEmpty()) {
                res.protocols[p.type].append(p.name);
            }
        }
    }

    if (res.family.isEmpty()) {
        if (res.deviceName.startsWith("STM32", Qt::CaseInsensitive)) res.family = "STM32";
        else if (res.deviceName.startsWith("ESP32", Qt::CaseInsensitive)) res.family = "ESP32";
        else if (res.deviceName.contains("RP2040", Qt::CaseInsensitive) || res.deviceName.contains("Pico", Qt::CaseInsensitive)) res.family = "RP2040";
        else if (res.deviceName.contains("RT10", Qt::CaseInsensitive) || res.deviceName.contains("MIMX", Qt::CaseInsensitive)) res.family = "i.MX RT";
        else if (res.deviceName.contains("nRF", Qt::CaseInsensitive) || res.deviceName.contains("52840", Qt::CaseInsensitive)) res.family = "Nordic";
        else res.family = "Generic";
    }

    if (res.pinMap.isEmpty()) {
        BoardDef synthetic;
        synthetic.family = res.family;
        BoardRegistry::instance().ensureFamilyPinDefaults(synthetic);
        res.pinMap = synthetic.pinMap;
    }

    if (res.protocols.isEmpty()) {
        res.protocols["GPIO"] = {"GPIO"};
        res.protocols["UART"] = {"UART1"};
        res.protocols["SPI"] = {"SPI1"};
        res.protocols["I2C"] = {"I2C1"};
        res.protocols["ADC"] = {"ADC1"};
        res.protocols["PWM"] = {"PWM1"};
    }

    if (res.deviceName == "STM32F030R8" || (boardDef.isValid() && boardDef.id == "nucleo_f030r8")) {
        res.isDefaultF030 = true;
        res.boardDisplayName = "STM32 NUCLEO-F030R8";
        res.family = "STM32";
        if (!boardDef.isValid()) {
            BoardDef nucleo = BoardRegistry::instance().board("nucleo_f030r8");
            if (nucleo.isValid()) {
                res.pinMap = nucleo.pinMap;
                res.protocols = nucleo.protocols;
            }
        }
    }

    auto* pack = driverpack::DriverPackRegistry::instance().findPackForDevice(res.deviceName.toStdString());
    bool driverDataVerified = true;
    if (pack) {
        const auto* devSpec = pack->findDevice(res.deviceName.toStdString());
        if (devSpec) driverDataVerified = devSpec->driverDataVerified;
    }
    res.hasDriverPack = (pack != nullptr) && driverDataVerified;

    return res;
}

// ---------------------------------------------------------------------------
// Phase C1: plan-driven HAL generation for the verified STM32F030R8 target.
// Used only when the project has a non-empty hardware configuration; the
// template path stays byte-identical for empty configurations (golden test).
//
// Everything emitted here is derived from the sourced capability data
// (`stm32f030r8_pin_data.json`) and the project hardware configuration:
// pins, AF numbers, clock-enable bits, instances and rates. Anything not
// documented is omitted and reported through `diagnostics` instead of being
// guessed. Interrupts (NVIC/EXTI), SYSCFG, ADC and timers are out of scope.
// ---------------------------------------------------------------------------

struct PlanHalBuild {
    bool active = false;
    QString deviceName;
    QString header;
    QString source;
    QStringList diagnostics;
};

int severityRank(const QString& severity) {
    if (severity == QStringLiteral("Error")) return 2;
    if (severity == QStringLiteral("Warning")) return 1;
    return 0;
}

QString rccMember(const QString& registerName) {
    return registerName.startsWith(QStringLiteral("RCC_")) ? registerName.mid(4) : registerName;
}

QString sanitizePinMacro(const QString& raw, const QString& fallback) {
    QString out;
    for (const QChar ch : raw.toUpper()) {
        if ((ch >= QLatin1Char('A') && ch <= QLatin1Char('Z'))
            || (ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))) {
            out.append(ch);
        } else if (!out.isEmpty() && !out.endsWith(QLatin1Char('_'))) {
            out.append(QLatin1Char('_'));
        }
    }
    while (out.endsWith(QLatin1Char('_'))) out.chop(1);
    if (out.isEmpty()) return fallback;
    if (out.at(0).isDigit()) out.prepend(QLatin1Char('_'));
    return out;
}

int moderValueForMode(const QString& mode) {
    if (mode.compare(QStringLiteral("GPIO_Output"), Qt::CaseInsensitive) == 0) return 1;
    if (mode.compare(QStringLiteral("GPIO_Input"), Qt::CaseInsensitive) == 0) return 0;
    if (mode.compare(QStringLiteral("AlternateFunction"), Qt::CaseInsensitive) == 0) return 2;
    if (mode.compare(QStringLiteral("Analog"), Qt::CaseInsensitive) == 0) return 3;
    return -1;
}

int pupdrValueForPull(const QString& pull) {
    if (pull.compare(QStringLiteral("PullUp"), Qt::CaseInsensitive) == 0) return 1;
    if (pull.compare(QStringLiteral("PullDown"), Qt::CaseInsensitive) == 0) return 2;
    return 0;
}

int ospeedrValueForSpeed(const QString& speed) {
    if (speed.compare(QStringLiteral("Medium"), Qt::CaseInsensitive) == 0) return 1;
    if (speed.compare(QStringLiteral("High"), Qt::CaseInsensitive) == 0) return 2;
    if (speed.compare(QStringLiteral("VeryHigh"), Qt::CaseInsensitive) == 0) return 3;
    return 0;
}

QString normalizedPeripheralKind(const QString& type, const QString& name) {
    QString kind = type.trimmed().toUpper();
    if (kind.isEmpty()) {
        if (name.startsWith(QStringLiteral("USART"), Qt::CaseInsensitive)
            || name.startsWith(QStringLiteral("UART"), Qt::CaseInsensitive)) {
            kind = QStringLiteral("UART");
        } else if (name.startsWith(QStringLiteral("SPI"), Qt::CaseInsensitive)) {
            kind = QStringLiteral("SPI");
        } else if (name.startsWith(QStringLiteral("I2C"), Qt::CaseInsensitive)) {
            kind = QStringLiteral("I2C");
        }
    }
    if (kind == QStringLiteral("USART")) kind = QStringLiteral("UART");
    return kind;
}

bool isCIdentifier(const QString& name) {
    if (name.isEmpty()) return false;
    const QChar first = name.at(0);
    if (!(first.isLetter() && first.unicode() < 128)) return false;
    for (const QChar ch : name) {
        if (!((ch.isLetterOrNumber() && ch.unicode() < 128) || ch == QLatin1Char('_'))) {
            return false;
        }
    }
    return true;
}

// Rate parameters are stored under different keys depending on which part of
// the UI produced the configuration: descriptor defaults (baudRate,
// clockSpeedHz, speedHz) or the wizard's manual parameter box (speedOrFreq).
qint64 readRateParameter(const QMap<QString, QVariant>& parameters, const QStringList& keys) {
    for (const QString& key : keys) {
        bool ok = false;
        const qint64 value = parameters.value(key).toLongLong(&ok);
        if (ok && value > 0) return value;
    }
    return 0;
}

struct PlanPin {
    QString pin;
    QString port;
    int index = -1;
    QString mode;
    QString signal;
    int afNumber = -1;
    QString pull;
    QString speed;
    QString outputType;
    QString initialOutput;
    QString label;
    bool derived = false;      // configured by a peripheral assignment
};

struct PlanPeripheral {
    QString name;
    QString kind;                        // UART / SPI / I2C
    QString busName;                     // resolved bus, e.g. "APB2"
    QStringList signalNotes;             // "TX=PA9"
    QMap<QString, QString> signalPins;    // upper signal -> pin
    bool generate = false;
    QString skipSeverity;                // "Info" / "Warning" / "Error"
    QStringList skipReasons;
    // UART
    qint64 baud = 0;
    qint64 busHz = 0;
    int brr = 0;
    bool clockAssumedHsi = false;
    bool clockAssumedApb = false;
    // SPI
    int brBits = -1;
    qint64 spiActualHz = 0;
    bool spiAdjusted = false;
    bool hasNss = false;
    // I2C
    quint32 timingr = 0;
    qint64 speedHz = 0;
};

PlanHalBuild buildPlanHal(const Project* project, const TargetResolution& target) {
    PlanHalBuild build;
    if (!project || !target.isDefaultF030) return build;

    const Hardware::HardwareConfig& hw = project->hardwareConfig();
    if (Hardware::PeripheralPlan::configIsEmpty(hw)) return build;

    build.active = true;
    build.deviceName = target.deviceName.isEmpty() ? QStringLiteral("STM32F030R8") : target.deviceName;

    const Hardware::PinCapabilities* caps =
        Hardware::PinCapabilityDatabase::instance().forDevice(build.deviceName);
    if (!caps) {
        build.diagnostics << QStringLiteral("Info: no verified pin capability data for %1; only HAL stubs are generated.").arg(build.deviceName);
    }

    const Hardware::PeripheralPlan plan = Hardware::PeripheralPlan::fromConfig(hw, caps);

    // ---- pins: explicit entries win over peripheral assignments -------------
    QMap<QString, PlanPin> pinPlans;
    QMap<QString, QStringList> pinUseKeys;

    for (const Hardware::PlannedPin& p : plan.pins) {
        const bool af = p.mode.compare(QStringLiteral("AlternateFunction"), Qt::CaseInsensitive) == 0;
        pinUseKeys[p.pin] << (af && !p.signal.isEmpty()
                                  ? QStringLiteral("AF:%1").arg(p.signal)
                                  : QStringLiteral("mode:%1").arg(p.mode));

        if (!p.pinKnown) {
            build.diagnostics << QStringLiteral("Info: no verified pin data for '%1'; its configuration is skipped.").arg(p.pin);
            continue;
        }
        if (af) {
            if (p.signal.isEmpty()) continue;                       // validator warns
            if (!p.afKnown) {
                const bool elsewhere = caps && caps->signalDocumentedElsewhere(p.signal, p.pin);
                build.diagnostics << QStringLiteral("%1: '%2' on %3 has no verified AF number; configuration skipped.")
                    .arg(elsewhere ? QStringLiteral("Warning") : QStringLiteral("Info"), p.signal, p.pin);
                continue;
            }
            if (!p.afrAvailable) {
                build.diagnostics << QStringLiteral("Info: the port of '%1' has no alternate-function register (AFR); configuration skipped.").arg(p.pin);
                continue;
            }
        } else if (moderValueForMode(p.mode) < 0) {
            continue;                                               // validator warns
        }

        PlanPin pp;
        pp.pin = p.pin;
        pp.port = Hardware::PinCapabilities::portNameForPin(p.pin);
        pp.index = Hardware::PinCapabilities::pinIndex(p.pin);
        pp.mode = p.mode;
        pp.signal = p.signal;
        pp.afNumber = p.afNumber;
        pp.pull = p.pull;
        pp.speed = p.speed;
        pp.outputType = p.outputType;
        pp.initialOutput = p.initialOutput;
        pp.label = p.label;
        pinPlans.insert(pp.pin, pp);
    }

    QVector<PlanPeripheral> peripherals;
    for (const Hardware::PlannedPeripheral& per : plan.peripherals) {
        PlanPeripheral out;
        out.name = per.name;
        out.kind = normalizedPeripheralKind(per.type, per.name);

        for (const Hardware::PlannedSignalUse& use : per.signalUses) {
            if (use.pin.isEmpty()) continue;
            pinUseKeys[use.pin] << QStringLiteral("AF:%1").arg(use.fullSignal);
            out.signalPins.insert(use.signal.trimmed().toUpper(), use.pin);
            out.signalNotes << QStringLiteral("%1=%2").arg(use.signal, use.pin);
            if (out.kind == QStringLiteral("SPI")
                && use.signal.compare(QStringLiteral("NSS"), Qt::CaseInsensitive) == 0) {
                out.hasNss = true;
            }

            const bool supportedKind = out.kind == QStringLiteral("UART")
                || out.kind == QStringLiteral("SPI") || out.kind == QStringLiteral("I2C");
            if (!supportedKind) continue;
            if (!use.pinKnown) continue;                            // reported below
            if (!use.afKnown) continue;                             // reported below
            if (!pinPlans.contains(use.pin)) {
                PlanPin pp;
                pp.pin = use.pin;
                pp.port = Hardware::PinCapabilities::portNameForPin(use.pin);
                pp.index = Hardware::PinCapabilities::pinIndex(use.pin);
                pp.mode = QStringLiteral("AlternateFunction");
                pp.signal = use.fullSignal;
                pp.afNumber = use.afNumber;
                pp.pull = QStringLiteral("NoPull");
                pp.speed = QStringLiteral("Medium");
                pp.outputType = QStringLiteral("PushPull");
                pp.initialOutput = QStringLiteral("Low");
                pp.derived = true;
                pinPlans.insert(pp.pin, pp);
            }
        }
        peripherals.append(out);
    }

    // A pin carrying more than one distinct function cannot be configured.
    QStringList conflicted;
    for (auto it = pinUseKeys.constBegin(); it != pinUseKeys.constEnd(); ++it) {
        QStringList distinct = it.value();
        distinct.removeDuplicates();
        if (distinct.size() > 1) conflicted << it.key();
    }
    for (const QString& pin : conflicted) {
        if (pinPlans.remove(pin) > 0) {
            build.diagnostics << QStringLiteral("Error: pin '%1' is assigned to multiple functions; its configuration is skipped.").arg(pin);
        }
    }

    // Deterministic pin order: port, then bit index.
    QStringList orderedPins = pinPlans.keys();
    std::sort(orderedPins.begin(), orderedPins.end(), [](const QString& a, const QString& b) {
        const QString portA = Hardware::PinCapabilities::portNameForPin(a);
        const QString portB = Hardware::PinCapabilities::portNameForPin(b);
        if (portA != portB) return portA < portB;
        const int indexA = Hardware::PinCapabilities::pinIndex(a);
        const int indexB = Hardware::PinCapabilities::pinIndex(b);
        if (indexA != indexB) return indexA < indexB;
        return a < b;
    });

    // ---- peripherals ---------------------------------------------------------
    for (PlanPeripheral& per : peripherals) {
        auto addSkip = [&per](const QString& severity, const QString& reason) {
            if (severityRank(severity) > severityRank(per.skipSeverity)) {
                per.skipSeverity = severity;
            }
            per.skipReasons << reason;
        };

        const bool supportedKind = per.kind == QStringLiteral("UART")
            || per.kind == QStringLiteral("SPI") || per.kind == QStringLiteral("I2C");
        if (!supportedKind) {
            addSkip(QStringLiteral("Info"),
                    QStringLiteral("peripheral type %1 is not generated by this version (GPIO/UART/SPI/I2C only)")
                        .arg(per.kind.isEmpty() ? QStringLiteral("unknown") : per.kind));
            continue;
        }
        if (!isCIdentifier(per.name)) {
            addSkip(QStringLiteral("Info"), QStringLiteral("'%1' is not a valid device peripheral name").arg(per.name));
            continue;
        }

        const Hardware::PeripheralFacts* facts = caps ? caps->peripheral(per.name) : nullptr;
        const Hardware::PlannedPeripheral* planned = nullptr;
        for (const Hardware::PlannedPeripheral& p : plan.peripherals) {
            if (p.name == per.name) { planned = &p; break; }
        }
        if (!facts || !planned || !planned->factsKnown) {
            addSkip(QStringLiteral("Info"),
                    QStringLiteral("no verified capability data for peripheral '%1' on %2").arg(per.name, build.deviceName));
            continue;
        }
        if (!facts->available) {
            addSkip(QStringLiteral("Error"),
                    QStringLiteral("peripheral '%1' is not documented as present on %2").arg(per.name, build.deviceName));
            continue;
        }
        if (!facts->clock.known()) {
            addSkip(QStringLiteral("Info"), QStringLiteral("the clock enable bit of '%1' is not documented").arg(per.name));
            continue;
        }
        if (facts->clockBus.trimmed().isEmpty()) {
            addSkip(QStringLiteral("Info"), QStringLiteral("the bus of '%1' is not documented").arg(per.name));
            continue;
        }

        QStringList badSignals;
        QString worstSeverity = QStringLiteral("Info");
        auto noteBad = [&](const QString& severity, const QString& token) {
            if (severityRank(severity) > severityRank(worstSeverity)) worstSeverity = severity;
            badSignals << token;
        };
        for (const Hardware::PlannedSignalUse& use : planned->signalUses) {
            if (use.pin.isEmpty()) continue;
            if (!use.pinKnown) {
                noteBad(QStringLiteral("Info"),
                        QStringLiteral("%1 on %2 (pin not documented)").arg(use.signal, use.pin));
                continue;
            }
            // A pin conflict is the dominant (Error-level) reason: the pin has
            // already been removed from the pin plan above.
            if (conflicted.contains(use.pin)) {
                noteBad(QStringLiteral("Error"),
                        QStringLiteral("%1 on %2 (pin assigned to multiple functions)").arg(use.signal, use.pin));
                continue;
            }
            if (!use.afKnown) {
                const bool elsewhere = caps && caps->signalDocumentedElsewhere(use.fullSignal, use.pin);
                noteBad(elsewhere ? QStringLiteral("Warning") : QStringLiteral("Info"),
                        QStringLiteral("%1 on %2 (no verified AF number)").arg(use.signal, use.pin));
                continue;
            }
        }
        if (!badSignals.isEmpty()) {
            addSkip(worstSeverity, QStringLiteral("signals %1").arg(badSignals.join(QStringLiteral(", "))));
            continue;
        }

        const Hardware::BusClockResolution bus = Hardware::resolveBusClock(hw.clock, facts->clockBus);
        const qint64 busHz = qint64(bus.mhz) * 1000000;
        per.busName = facts->clockBus;

        if (per.kind == QStringLiteral("UART")) {
            const qint64 baud = readRateParameter(planned->parameters,
                {QStringLiteral("baudRate"), QStringLiteral("speedOrFreq")});
            if (baud <= 0) {
                addSkip(QStringLiteral("Info"), QStringLiteral("no 'baudRate' parameter is configured"));
                continue;
            }
            // On STM32F0 BRR holds USARTDIV as 12.4 fixed point, which equals
            // the integer-rounded fCK/baud ratio (RM0360 Rev 5 sec 23.4.4).
            const qint64 brr = (busHz + baud / 2) / baud;
            if (brr <= 0 || brr > 0xFFFF) {
                addSkip(QStringLiteral("Warning"),
                        QStringLiteral("baud rate %1 cannot be represented in USART_BRR at %2 MHz").arg(baud).arg(bus.mhz));
                continue;
            }
            per.baud = baud;
            per.brr = int(brr);
            per.busHz = busHz;
            per.clockAssumedHsi = bus.assumedHsi;
            per.clockAssumedApb = bus.assumedApbIsSys;
        } else if (per.kind == QStringLiteral("SPI")) {
            const qint64 speed = readRateParameter(planned->parameters,
                {QStringLiteral("clockSpeedHz"), QStringLiteral("speedOrFreq")});
            if (speed <= 0) {
                addSkip(QStringLiteral("Info"), QStringLiteral("no 'clockSpeedHz' parameter is configured"));
                continue;
            }
            // BR field selects fPCLK / 2^(BR+1) (RM0360 Rev 5 sec 24.7.2); use
            // the highest documented division that does not exceed the request.
            int br = 0;
            qint64 div = 2;
            while (br < 7 && div * speed < busHz) { div <<= 1; ++br; }
            per.brBits = br;
            per.spiActualHz = busHz / div;
            per.spiAdjusted = (per.spiActualHz != speed);
            per.busHz = busHz;
            per.clockAssumedHsi = bus.assumedHsi;
            per.clockAssumedApb = bus.assumedApbIsSys;
        } else { // I2C
            const qint64 speed = readRateParameter(planned->parameters,
                {QStringLiteral("speedHz"), QStringLiteral("clockSpeedHz"), QStringLiteral("speedOrFreq")});
            if (speed <= 0) {
                addSkip(QStringLiteral("Info"), QStringLiteral("no 'speedHz' parameter is configured"));
                continue;
            }
            if (facts->maxClockHz.known && speed > qint64(facts->maxClockHz.asInt())) {
                addSkip(QStringLiteral("Warning"),
                        QStringLiteral("%1 Hz exceeds the documented maximum of %2 Hz for %3")
                            .arg(speed).arg(facts->maxClockHz.asInt()).arg(per.name));
                continue;
            }
            bool found = false;
            for (const Hardware::TimingExampleFact& ex : caps->timingExamples(QStringLiteral("I2C"))) {
                if (ex.clockMhz == bus.mhz && ex.speedHz == int(speed) && ex.known()) {
                    per.timingr = ex.timingr();
                    found = true;
                    break;
                }
            }
            if (!found) {
                // The F0 I2C peripheral is TIMINGR-only; values must come from
                // the reference manual's example tables, never be computed here.
                addSkip(QStringLiteral("Info"),
                        QStringLiteral("no verified TIMINGR example for %1 Hz at %2 MHz").arg(speed).arg(bus.mhz));
                continue;
            }
            per.speedHz = speed;
            per.busHz = busHz;
            per.clockAssumedHsi = bus.assumedHsi;
            per.clockAssumedApb = bus.assumedApbIsSys;
        }

        per.generate = true;
    }

    // Deterministic order: UART, SPI, I2C, then by name.
    auto kindRank = [](const QString& kind) {
        if (kind == QStringLiteral("UART")) return 0;
        if (kind == QStringLiteral("SPI")) return 1;
        if (kind == QStringLiteral("I2C")) return 2;
        return 3;
    };
    std::sort(peripherals.begin(), peripherals.end(),
              [&kindRank](const PlanPeripheral& a, const PlanPeripheral& b) {
                  const int ra = kindRank(a.kind);
                  const int rb = kindRank(b.kind);
                  if (ra != rb) return ra < rb;
                  return a.name < b.name;
              });

    // ---- diagnostics ---------------------------------------------------------
    for (const PlanPeripheral& per : peripherals) {
        if (per.generate || per.skipReasons.isEmpty()) continue;
        build.diagnostics << QStringLiteral("%1: %2 init not generated: %3.")
            .arg(per.skipSeverity.isEmpty() ? QStringLiteral("Info") : per.skipSeverity,
                 per.name, per.skipReasons.join(QStringLiteral("; ")));
    }
    for (const Hardware::PlannedPin& p : plan.pins) {
        if (p.interrupt.compare(QStringLiteral("None"), Qt::CaseInsensitive) != 0) {
            build.diagnostics << QStringLiteral("Info: pin '%1' requests an interrupt; interrupt (NVIC/EXTI) generation is not part of this version.").arg(p.pin);
        }
    }
    for (const PlanPeripheral& per : peripherals) {
        if (!per.generate) continue;
        if (per.clockAssumedHsi) {
            build.diagnostics << QStringLiteral("Info: %1: no system clock configured; driver timing assumes the 8 MHz HSI reset default (RM0360 Rev 5 sec 7.2.6).").arg(per.name);
        } else if (per.clockAssumedApb) {
            build.diagnostics << QStringLiteral("Info: %1: %2 clock not configured; driver timing assumes the APB bus equals SYSCLK (%3 MHz).")
                .arg(per.name, per.busName).arg(per.busHz / 1000000);
        }
        if (per.kind == QStringLiteral("SPI") && per.spiAdjusted && per.spiActualHz > 0) {
            build.diagnostics << QStringLiteral("Info: %1: requested SPI clock was adjusted by the baud-rate prescaler to %2 Hz (BR=%3).")
                .arg(per.name).arg(per.spiActualHz).arg(per.brBits);
        }
    }

    // ---- header --------------------------------------------------------------
    QString h;
    h += "/**\n";
    h += " * @file target_hal.h\n";
    h += " * @brief Target Hardware Abstraction Layer Adapter\n";
    h += QString(" * Target Family: %1 | Target Board: %2\n").arg(target.family, target.boardDisplayName);
    h += " * Auto-generated by Embedded UI Designer (Phase 4 Code Generator)\n";
    h += QString(" * Generated from the project pin/peripheral plan for %1 (Phase C1).\n").arg(build.deviceName);
    h += " * Only verified capability data is used; undocumented values are omitted.\n";
    h += " */\n\n";
    h += "#ifndef TARGET_HAL_H\n";
    h += "#define TARGET_HAL_H\n\n";
    h += "#include <stdint.h>\n";
    h += "#include <stdbool.h>\n";
    h += "\n/* Registers are accessed through the ST CMSIS device header stm32f030x8.h\n";
    h += " * (STM32CubeF0). It is not vendored in this repository; add STM32CubeF0\n";
    h += " * to the include path and build with -DSTM32F030x8. */\n\n";

    h += "#ifdef __cplusplus\n";
    h += "extern \"C\" {\n";
    h += "#endif\n\n";

    h += "/* Configured pins */\n";
    for (const QString& pinName : orderedPins) {
        const PlanPin& pin = pinPlans.value(pinName);
        h += QString("#define PIN_%1 \"%2\"\n").arg(sanitizePinMacro(pin.pin, QStringLiteral("PIN")), pin.pin);
        const QString labelMacro = sanitizePinMacro(pin.label, QString());
        if (!labelMacro.isEmpty() && labelMacro != sanitizePinMacro(pin.pin, QString())) {
            h += QString("#define PIN_%1 \"%2\" /* user label */\n").arg(labelMacro, pin.pin);
        }
    }

    h += "\n/* Generated hardware protocols */\n";
    const bool hasGpioMode = std::any_of(pinPlans.constBegin(), pinPlans.constEnd(), [](const PlanPin& p) {
        return p.mode.compare(QStringLiteral("AlternateFunction"), Qt::CaseInsensitive) != 0;
    });
    if (hasGpioMode) h += "#define PROTOCOL_HAS_GPIO 1\n";
    const QStringList kindOrder = {QStringLiteral("UART"), QStringLiteral("SPI"), QStringLiteral("I2C")};
    for (const QString& kind : kindOrder) {
        QStringList instances;
        for (const PlanPeripheral& per : peripherals) {
            if (per.generate && per.kind == kind) instances << per.name;
        }
        if (!instances.isEmpty()) {
            h += QString("#define PROTOCOL_HAS_%1 1 /* %2 */\n").arg(kind, instances.join(QStringLiteral(", ")));
        }
    }

    h += "\n/* HAL Lifecycle Initializers */\n";
    h += "void target_hal_init(void);\n";
    h += "void target_hal_poll(void);\n\n";
    h += "/* Digital GPIO */\n";
    h += "bool hal_read_digital_pin(const char* pin_name);\n";
    h += "void hal_write_digital_pin(const char* pin_name, bool high);\n\n";
    h += "/* Analog ADC */\n";
    h += "uint32_t hal_read_adc_raw(const char* pin_name);\n";
    h += "float hal_read_adc_normalized(const char* pin_name);\n\n";
    h += "/* PWM Output */\n";
    h += "void hal_write_pwm_duty(const char* pin_name, float duty_percent);\n\n";
    h += "/* Sensor Readouts */\n";
    h += "float hal_read_sensor_value(const char* sensor_name);\n\n";

    h += "#ifdef __cplusplus\n";
    h += "}\n";
    h += "#endif\n\n";
    h += "#endif /* TARGET_HAL_H */\n";
    build.header = h;

    // ---- source --------------------------------------------------------------
    QString c;
    c += "/**\n";
    c += " * @file target_hal.c\n";
    c += " * @brief Target Hardware Abstraction Layer Adapter Implementation\n";
    c += QString(" * Target Family: %1 | Target Board: %2\n").arg(target.family, target.boardDisplayName);
    c += " * Auto-generated by Embedded UI Designer (Phase 4 Code Generator)\n";
    c += " * AF numbers transcribed from DS9773 Rev 5 Tables 12/13; clock enables,\n";
    c += " * bus mappings and register field names from CMSIS stm32f030x8.h\n";
    c += " * (STM32CubeF0). See export diagnostics for skipped configuration.\n";
    c += " */\n\n";
    c += "#include \"target_hal.h\"\n";
    c += "#include \"stm32f030x8.h\"\n";
    c += "#include <string.h>\n\n";

    c += "/* Pin configuration from the project's pin/peripheral plan */\n";
    c += "static void target_hal_gpio_init(void) {\n";
    QStringList portEnables;
    for (const QString& pinName : orderedPins) {
        const PlanPin& pin = pinPlans.value(pinName);
        if (!pin.port.isEmpty() && !portEnables.contains(pin.port)) portEnables << pin.port;
    }
    std::sort(portEnables.begin(), portEnables.end());
    for (const QString& port : portEnables) {
        const Hardware::PortFacts* portFacts = caps ? caps->port(port) : nullptr;
        if (!portFacts || !portFacts->clock.known()) continue;
        c += QString("    RCC->%1 |= %2_%3EN;\n")
            .arg(rccMember(portFacts->clock.registerName), portFacts->clock.registerName, port);
    }
    for (const QString& pinName : orderedPins) {
        const PlanPin& pin = pinPlans.value(pinName);
        const int idx = pin.index;
        const QString port = pin.port;
        const bool af = pin.mode.compare(QStringLiteral("AlternateFunction"), Qt::CaseInsensitive) == 0;
        c += "\n";
        if (af) {
            c += QString("    /* %1: %2, AF%3 */\n").arg(pin.pin, pin.signal).arg(pin.afNumber);
        } else {
            c += QString("    /* %1: %2 */\n").arg(pin.pin, pin.mode);
        }
        const int moder = moderValueForMode(pin.mode);
        if (moder == 0) {
            c += QString("    %1->MODER &= ~(3u << (%2u * 2u));\n").arg(port).arg(idx);
        } else {
            c += QString("    %1->MODER = (%1->MODER & ~(3u << (%2u * 2u))) | (%3u << (%2u * 2u));\n")
                .arg(port).arg(idx).arg(moder);
        }
        if (pin.mode.compare(QStringLiteral("GPIO_Input"), Qt::CaseInsensitive) != 0) {
            if (pin.outputType.compare(QStringLiteral("OpenDrain"), Qt::CaseInsensitive) == 0) {
                c += QString("    %1->OTYPER |= (1u << %2u);\n").arg(port).arg(idx);
            } else {
                c += QString("    %1->OTYPER &= ~(1u << %2u);\n").arg(port).arg(idx);
            }
        }
        const int ospeed = ospeedrValueForSpeed(pin.speed);
        if (ospeed == 0) {
            c += QString("    %1->OSPEEDR &= ~(3u << (%2u * 2u));\n").arg(port).arg(idx);
        } else {
            c += QString("    %1->OSPEEDR = (%1->OSPEEDR & ~(3u << (%2u * 2u))) | (%3u << (%2u * 2u));\n")
                .arg(port).arg(idx).arg(ospeed);
        }
        const int pupd = pupdrValueForPull(pin.pull);
        if (pupd == 0) {
            c += QString("    %1->PUPDR &= ~(3u << (%2u * 2u));\n").arg(port).arg(idx);
        } else {
            c += QString("    %1->PUPDR = (%1->PUPDR & ~(3u << (%2u * 2u))) | (%3u << (%2u * 2u));\n")
                .arg(port).arg(idx).arg(pupd);
        }
        if (af) {
            if (idx < 8) {
                c += QString("    %1->AFR[0] = (%1->AFR[0] & ~(0xFu << (%2u * 4u))) | (%3u << (%2u * 4u));\n")
                    .arg(port).arg(idx).arg(pin.afNumber);
            } else {
                c += QString("    %1->AFR[1] = (%1->AFR[1] & ~(0xFu << ((%2u - 8u) * 4u))) | (%3u << ((%2u - 8u) * 4u));\n")
                    .arg(port).arg(idx).arg(pin.afNumber);
            }
        }
        if (pin.mode.compare(QStringLiteral("GPIO_Output"), Qt::CaseInsensitive) == 0) {
            if (pin.initialOutput.compare(QStringLiteral("High"), Qt::CaseInsensitive) == 0) {
                c += QString("    %1->BSRR = (1u << %2u); /* initial output: high */\n").arg(port).arg(idx);
            } else {
                c += QString("    %1->BSRR = (1u << (%2u + 16u)); /* initial output: low */\n").arg(port).arg(idx);
            }
        }
    }
    if (pinPlans.isEmpty()) {
        c += "    /* No pin configuration was generated (see export diagnostics). */\n";
    }
    c += "}\n\n";

    for (const PlanPeripheral& per : peripherals) {
        if (!per.generate) continue;
        const Hardware::PeripheralFacts* facts = caps->peripheral(per.name);
        c += QString("/* %1: %2 */\n").arg(per.name, per.signalNotes.join(QStringLiteral(", ")));
        const QString fn = QStringLiteral("hal_%1_init").arg(per.name.toLower());
        c += QString("static void %1(void) {\n").arg(fn);
        c += QString("    RCC->%1 |= %2_%3EN;\n")
            .arg(rccMember(facts->clock.registerName), facts->clock.registerName, per.name);
        if (per.kind == QStringLiteral("UART")) {
            if (per.clockAssumedHsi) {
                c += "    /* No system clock configured: assuming the 8 MHz HSI reset default (RM0360 Rev 5 sec 7.2.6) */\n";
            } else if (per.clockAssumedApb) {
                c += QString("    /* APB clock not configured: assuming the bus equals SYSCLK (%1 MHz) */\n").arg(per.busHz / 1000000);
            }
            c += QString("    %1->BRR = %2u; /* %3 MHz / %4 baud (rounded; RM0360 Rev 5 sec 23.4.4) */\n")
                .arg(per.name).arg(per.brr).arg(per.busHz / 1000000).arg(per.baud);
            QStringList cr1;
            if (per.signalPins.contains(QStringLiteral("TX"))) cr1 << QStringLiteral("USART_CR1_TE");
            if (per.signalPins.contains(QStringLiteral("RX"))) cr1 << QStringLiteral("USART_CR1_RE");
            cr1 << QStringLiteral("USART_CR1_UE");
            c += QString("    %1->CR1 = %2;\n").arg(per.name, cr1.join(QStringLiteral(" | ")));
        } else if (per.kind == QStringLiteral("SPI")) {
            QStringList cr1;
            cr1 << QStringLiteral("SPI_CR1_MSTR");
            if (!per.hasNss) {
                cr1 << QStringLiteral("SPI_CR1_SSM") << QStringLiteral("SPI_CR1_SSI");
            }
            cr1 << QString("(%1u << SPI_CR1_BR_Pos)").arg(per.brBits);
            c += QString("    %1->CR1 = %2; /* BR=%3: %4 MHz / %5 = %6 Hz (RM0360 Rev 5 sec 24.7.2) */\n")
                .arg(per.name, cr1.join(QStringLiteral(" | ")))
                .arg(per.brBits).arg(per.busHz / 1000000).arg(1 << (per.brBits + 1)).arg(per.spiActualHz);
            if (per.hasNss) {
                c += QString("    %1->CR2 = SPI_CR2_SSOE;\n").arg(per.name);
            }
            c += QString("    %1->CR1 |= SPI_CR1_SPE;\n").arg(per.name);
        } else { // I2C
            c += QString("    %1->TIMINGR = 0x%2u; /* %3 Hz at %4 MHz (RM0360 Rev 5 Tables 75-77 example point) */\n")
                .arg(per.name)
                .arg(per.timingr, 8, 16, QLatin1Char('0'))
                .arg(per.speedHz).arg(per.busHz / 1000000);
            c += QString("    %1->CR1 |= I2C_CR1_PE;\n").arg(per.name);
        }
        c += "}\n\n";
    }

    c += "/* Central hardware initialization */\n";
    c += "void target_hal_init(void) {\n";
    c += "    target_hal_gpio_init();\n";
    for (const PlanPeripheral& per : peripherals) {
        if (per.generate) {
            c += QString("    hal_%1_init();\n").arg(per.name.toLower());
        } else {
            c += QString("    /* %1 init not generated: %2 */\n")
                .arg(per.name, per.skipReasons.isEmpty() ? QStringLiteral("unsupported") : per.skipReasons.join(QStringLiteral("; ")));
        }
    }
    c += "}\n\n";

    // ---- digital pin accessors ----------------------------------------------
    c += "bool hal_read_digital_pin(const char* pin_name) {\n";
    bool anyReadable = false;
    for (const QString& pinName : orderedPins) {
        const PlanPin& pin = pinPlans.value(pinName);
        if (pin.mode.compare(QStringLiteral("GPIO_Input"), Qt::CaseInsensitive) != 0
            && pin.mode.compare(QStringLiteral("GPIO_Output"), Qt::CaseInsensitive) != 0) {
            continue;
        }
        anyReadable = true;
        QStringList names;
        names << QString("strcmp(pin_name, \"%1\") == 0").arg(pin.pin);
        const QString macro = sanitizePinMacro(pin.label, QString());
        if (!macro.isEmpty()) names << QString("strcmp(pin_name, \"%1\") == 0").arg(macro);
        c += QString("    if (%1) return (%2->IDR & (1u << %3u)) != 0u;\n")
            .arg(names.join(QStringLiteral(" || ")), pin.port).arg(pin.index);
    }
    if (!anyReadable) c += "    (void)pin_name;\n";
    c += "\n    return false;\n";
    c += "}\n\n";

    c += "void hal_write_digital_pin(const char* pin_name, bool high) {\n";
    bool anyOutput = false;
    for (const QString& pinName : orderedPins) {
        const PlanPin& pin = pinPlans.value(pinName);
        if (pin.mode.compare(QStringLiteral("GPIO_Output"), Qt::CaseInsensitive) != 0) continue;
        anyOutput = true;
        QStringList names;
        names << QString("strcmp(pin_name, \"%1\") == 0").arg(pin.pin);
        const QString macro = sanitizePinMacro(pin.label, QString());
        if (!macro.isEmpty()) names << QString("strcmp(pin_name, \"%1\") == 0").arg(macro);
        c += QString("    if (%1) { %2->BSRR = high ? (1u << %3u) : (1u << (%3u + 16u)); return; }\n")
            .arg(names.join(QStringLiteral(" || ")), pin.port).arg(pin.index);
    }
    if (!anyOutput) c += "    (void)pin_name;\n    (void)high;\n";
    c += "}\n\n";

    c += "uint32_t hal_read_adc_raw(const char* pin_name) {\n";
    c += "    (void)pin_name; /* ADC generation is not part of this version. */\n";
    c += "    return 2048u;\n";
    c += "}\n\n";
    c += "float hal_read_adc_normalized(const char* pin_name) {\n";
    c += "    return (float)hal_read_adc_raw(pin_name) / 4095.0f;\n";
    c += "}\n\n";
    c += "void hal_write_pwm_duty(const char* pin_name, float duty_percent) {\n";
    c += "    (void)pin_name; (void)duty_percent; /* PWM generation is not part of this version. */\n";
    c += "}\n\n";
    c += "float hal_read_sensor_value(const char* sensor_name) {\n";
    c += "    (void)sensor_name;\n";
    c += "    return 25.0f;\n";
    c += "}\n\n";
    c += "void target_hal_poll(void) {\n";
    c += "    /* Poll hardware peripherals and tick counters */\n";
    c += "}\n";

    build.source = c;
    return build;
}

} // namespace

QString TargetHalGenerator::generateHalHeader(const Project* project, const QString& targetMcu) {
    TargetResolution target = resolveTarget(project, targetMcu);
    if (!target.hasDriverPack && !target.isDefaultF030) {
        return QString(); // Unsupported device -> no HAL output
    }

    // Phase C1: configured STM32F030R8 projects are generated from their
    // pin/peripheral plan instead of the generic template.
    const PlanHalBuild plan = buildPlanHal(project, target);
    if (plan.active) {
        return plan.header;
    }

    QString fam = target.family;
    QString board = target.boardDisplayName;

    QString code;
    code += "/**\n";
    code += " * @file target_hal.h\n";
    code += " * @brief Target Hardware Abstraction Layer Adapter\n";
    code += QString(" * Target Family: %1 | Target Board: %2\n").arg(fam, board);
    code += " * Auto-generated by Embedded UI Designer (Phase 4 Code Generator)\n";
    code += " */\n\n";
    code += "#ifndef TARGET_HAL_H\n";
    code += "#define TARGET_HAL_H\n\n";
    code += "#include <stdint.h>\n";
    code += "#include <stdbool.h>\n";

    if (target.isDefaultF030) {
        // Keep byte-identical to golden STM32F030R8 baseline
    } else if (fam == "ESP32") {
        code += "#include \"driver/gpio.h\"\n";
        code += "#include \"esp_timer.h\"\n";
    } else if (fam == "RP2040") {
        code += "#include \"pico/stdlib.h\"\n";
        code += "#include \"hardware/gpio.h\"\n";
    } else if (fam == "STM32") {
        if (target.deviceName.contains("F4", Qt::CaseInsensitive)) {
            code += "#include \"stm32f4xx_hal.h\"\n";
        } else {
            code += "#include \"stm32_hal.h\"\n";
        }
    } else if (fam == "i.MX RT") {
        code += "#include \"fsl_gpio.h\"\n";
        code += "#include \"fsl_common.h\"\n";
    } else if (fam == "Nordic") {
        code += "#include <zephyr/kernel.h>\n";
        code += "#include <zephyr/drivers/gpio.h>\n";
    } else if (driverpack::CustomPackManager::instance().isCustomDevice(target.deviceName)) {
        driverpack::CustomPackDef customPack = driverpack::CustomPackManager::instance().getPack(target.deviceName);
        code += QString("/* Custom device: %1 (%2) */\n").arg(customPack.name, customPack.vendor);
        code += QString("#include \"%1_registers.h\"\n").arg(customPack.name.toLower());
    }

    code += "\n#ifdef __cplusplus\n";
    code += "extern \"C\" {\n";
    code += "#endif\n\n";

    code += "/* Target Board Pin Definitions */\n";
    for (auto it = target.pinMap.begin(); it != target.pinMap.end(); ++it) {
        code += QString("#define PIN_%1 \"%1\" /* %2%3 */\n")
            .arg(it.key(), it.value().function,
                 it.value().strapping ? " [Strapping]" : (it.value().reserved ? " [Reserved]" : ""));
    }

    code += "\n/* Supported Hardware Protocols */\n";
    for (auto it = target.protocols.begin(); it != target.protocols.end(); ++it) {
        code += QString("#define PROTOCOL_HAS_%1 1 /* %2 */\n").arg(it.key(), it.value().join(", "));
    }

    code += "\n/* HAL Lifecycle Initializers */\n";
    code += "void target_hal_init(void);\n";
    code += "void target_hal_poll(void);\n\n";

    code += "/* Digital GPIO */\n";
    code += "bool hal_read_digital_pin(const char* pin_name);\n";
    code += "void hal_write_digital_pin(const char* pin_name, bool high);\n\n";

    code += "/* Analog ADC */\n";
    code += "uint32_t hal_read_adc_raw(const char* pin_name);\n";
    code += "float hal_read_adc_normalized(const char* pin_name);\n\n";

    code += "/* PWM Output */\n";
    code += "void hal_write_pwm_duty(const char* pin_name, float duty_percent);\n\n";

    code += "/* Sensor Readouts */\n";
    code += "float hal_read_sensor_value(const char* sensor_name);\n\n";

    code += "#ifdef __cplusplus\n";
    code += "}\n";
    code += "#endif\n\n";
    code += "#endif /* TARGET_HAL_H */\n";
    return code;
}

QString TargetHalGenerator::generateDriverInitCode(const BoardDef& board,
                                                 const QString& protocol,
                                                 const QMap<QString, QString>& pinAssignments)
{
    QString code;
    QString fam = board.family.isEmpty() ? "Generic" : board.family;
    QString p = protocol.toUpper();

    if (p == "SPI") {
        QString bus = board.protocols.contains("SPI") && !board.protocols["SPI"].isEmpty() ? board.protocols["SPI"].first() : "SPI1";
        QString sck = pinAssignments.value("sck", "SCK");
        QString mosi = pinAssignments.value("mosi", "MOSI");
        QString miso = pinAssignments.value("miso", "MISO");
        QString ss = pinAssignments.value("ss", "SS");
        code += QString("/* SPI Driver: %1 on %2 (%3) */\n").arg(bus, board.displayName.isEmpty() ? board.id : board.displayName, fam);
        code += "void hal_spi_init(void) {\n";
        code += QString("    printf(\"[HAL: %1] Configuring SPI bus %2 (SCK=%3, MOSI=%4, MISO=%5, CS=%6)...\\n\");\n")
            .arg(fam, bus, sck, mosi, miso, ss);
        if (fam.contains("STM32", Qt::CaseInsensitive)) {
            code += QString("    /* __HAL_RCC_%1_CLK_ENABLE(); */\n").arg(bus);
            code += QString("    /* h%1.Instance = %2; h%1.Init.Mode = SPI_MODE_MASTER; */\n").arg(bus.toLower(), bus);
        } else if (fam.contains("ESP32", Qt::CaseInsensitive)) {
            code += QString("    /* spi_bus_config_t buscfg = {.mosi_io_num = %1, .miso_io_num = %2, .sclk_io_num = %3}; */\n").arg(mosi, miso, sck);
            code += "    /* spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO); */\n";
        } else if (fam.contains("RP2040", Qt::CaseInsensitive)) {
            code += QString("    /* spi_init(spi0, 1000 * 1000); gpio_set_function(%1, GPIO_FUNC_SPI); */\n").arg(sck);
        } else if (fam.contains("i.MX", Qt::CaseInsensitive) || fam.compare("NXP", Qt::CaseInsensitive) == 0) {
            code += "    /* CLOCK_EnableClock(kCLOCK_Lpspi1); */\n";
        } else if (fam.contains("Nordic", Qt::CaseInsensitive) || fam.contains("nRF", Qt::CaseInsensitive)) {
            code += "    /* nrfx_spim_init(&spim_instance, &spim_config, NULL, NULL); */\n";
        }
        code += "}\n";
    } else if (p == "I2C") {
        QString bus = board.protocols.contains("I2C") && !board.protocols["I2C"].isEmpty() ? board.protocols["I2C"].first() : "I2C1";
        QString scl = pinAssignments.value("scl", "SCL");
        QString sda = pinAssignments.value("sda", "SDA");
        code += QString("/* I2C Driver: %1 on %2 (%3) */\n").arg(bus, board.displayName.isEmpty() ? board.id : board.displayName, fam);
        code += "void hal_i2c_init(void) {\n";
        code += QString("    printf(\"[HAL: %1] Configuring I2C bus %2 (SCL=%3, SDA=%4)...\\n\");\n")
            .arg(fam, bus, scl, sda);
        if (fam.contains("STM32", Qt::CaseInsensitive)) {
            code += QString("    /* __HAL_RCC_%1_CLK_ENABLE(); */\n").arg(bus);
        } else if (fam.contains("ESP32", Qt::CaseInsensitive)) {
            code += QString("    /* i2c_config_t conf = {.scl_io_num = %1, .sda_io_num = %2}; */\n").arg(scl, sda);
        } else if (fam.contains("RP2040", Qt::CaseInsensitive)) {
            code += QString("    /* i2c_init(i2c0, 100 * 1000); gpio_set_function(%1, GPIO_FUNC_I2C); */\n").arg(sda);
        } else if (fam.contains("i.MX", Qt::CaseInsensitive) || fam.compare("NXP", Qt::CaseInsensitive) == 0) {
            code += "    /* LPI2C_MasterInit(LPI2C1, &masterConfig, CLOCK_GetFreq(kCLOCK_OscClk)); */\n";
        } else if (fam.contains("Nordic", Qt::CaseInsensitive) || fam.contains("nRF", Qt::CaseInsensitive)) {
            code += "    /* nrfx_twim_init(&twim_instance, &twim_config, NULL, NULL); */\n";
        }
        code += "}\n";
    } else if (p == "PWM") {
        code += QString("/* PWM Timer Driver: %1 */\n").arg(board.displayName.isEmpty() ? board.id : board.displayName);
        code += "void hal_pwm_init(void) {\n";
        code += QString("    printf(\"[HAL: %1] Initializing PWM timer hardware channels...\\n\");\n").arg(fam);
        code += "}\n";
    } else if (p == "ADC") {
        code += QString("/* ADC Driver: %1 */\n").arg(board.displayName.isEmpty() ? board.id : board.displayName);
        code += "void hal_adc_init(void) {\n";
        code += QString("    printf(\"[HAL: %1] Calibrating and enabling ADC converter...\\n\");\n").arg(fam);
        code += "}\n";
    } else if (p == "UART" || p == "USART") {
        QString bus = board.protocols.contains("UART") && !board.protocols["UART"].isEmpty() ? board.protocols["UART"].first() : "UART1";
        code += QString("/* UART Driver: %1 */\n").arg(bus);
        code += "void hal_uart_init(void) {\n";
        code += QString("    printf(\"[HAL: %1] Configuring Serial UART %2 at 115200 baud...\\n\");\n").arg(fam, bus);
        code += "}\n";
    } else if (p == "GPIO") {
        QString pin = pinAssignments.value("pin", "GPIO");
        code += QString("/* GPIO Driver: %1 */\n").arg(pin);
        code += "void hal_gpio_init(void) {\n";
        code += QString("    printf(\"[HAL: %1] Configuring pin %2 as Digital Output...\\n\");\n").arg(fam, pin);
        code += "}\n";
    } else {
        code += "/* Generic Idle Driver */\n";
        code += "void hal_idle_init(void) {\n";
        code += "    /* No active protocol driver configured */\n";
        code += "}\n";
    }
    return code;
}

QString TargetHalGenerator::generateHalSource(const Project* project, const QString& targetMcu) {
    s_lastPlanDiagnostics.clear();
    TargetResolution target = resolveTarget(project, targetMcu);
    if (!target.hasDriverPack && !target.isDefaultF030) {
        return QString(); // Unsupported device -> no HAL output
    }

    // Phase C1: configured STM32F030R8 projects are generated from their
    // pin/peripheral plan; empty configurations keep the template output.
    const PlanHalBuild plan = buildPlanHal(project, target);
    if (plan.active) {
        s_lastPlanDiagnostics = plan.diagnostics;
        return plan.source;
    }

    QString fam = target.family;
    QString board = target.boardDisplayName;

    BoardDef activeBoard;
    activeBoard.family = fam;
    activeBoard.displayName = board;
    activeBoard.id = target.deviceName;
    activeBoard.pinMap = target.pinMap;
    activeBoard.protocols = target.protocols;

    QString code;
    code += "/**\n";
    code += " * @file target_hal.c\n";
    code += " * @brief Target Hardware Abstraction Layer Adapter Implementation\n";
    code += QString(" * Target Family: %1 | Target Board: %2\n").arg(fam, board);
    code += " * Auto-generated by Embedded UI Designer\n";
    code += " */\n\n";
    code += "#include \"target_hal.h\"\n";
    code += "#include <stdio.h>\n";
    code += "#include <string.h>\n\n";

    // Driver routines for board's protocols
    code += "/* Protocol Driver Implementations */\n";
    for (auto it = activeBoard.protocols.begin(); it != activeBoard.protocols.end(); ++it) {
        code += generateDriverInitCode(activeBoard, it.key());
        code += "\n";
    }

    code += "/* Central Hardware Initialization */\n";
    code += "void target_hal_init(void) {\n";
    code += QString("    printf(\"[HAL: %1] Initializing target board: %2\\n\");\n").arg(fam, board);

    if (activeBoard.protocols.contains("SPI")) code += "    hal_spi_init();\n";
    if (activeBoard.protocols.contains("I2C")) code += "    hal_i2c_init();\n";
    if (activeBoard.protocols.contains("PWM")) code += "    hal_pwm_init();\n";
    if (activeBoard.protocols.contains("ADC")) code += "    hal_adc_init();\n";
    if (activeBoard.protocols.contains("UART")) code += "    hal_uart_init();\n";
    code += "}\n\n";

    if (driverpack::CustomPackManager::instance().isCustomDevice(target.deviceName)) {
        code += "bool hal_read_digital_pin(const char* pin_name) {\n";
        code += QString("    printf(\"[HAL: Custom %1] gpio_read(%%s)\\n\", pin_name);\n").arg(target.deviceName);
        code += "    return false;\n";
        code += "}\n\n";
    } else {
        code += "bool hal_read_digital_pin(const char* pin_name) {\n";
        code += "    (void)pin_name;\n";
        code += "    return false;\n";
        code += "}\n\n";
    }

    if (driverpack::CustomPackManager::instance().isCustomDevice(target.deviceName)) {
        driverpack::CustomPackDef customPack = driverpack::CustomPackManager::instance().getPack(target.deviceName);
        code += "void hal_write_digital_pin(const char* pin_name, bool high) {\n";
        code += QString("    /* Generated for custom device %1 (SVD-validated base addresses) */\n").arg(customPack.name);
        code += QString("    printf(\"[HAL: Custom %1] gpio_write(%%s, %%d)\\n\", pin_name, high ? 1 : 0);\n").arg(customPack.name);
        code += "}\n\n";
    } else if (fam.compare("STM32", Qt::CaseInsensitive) == 0) {
        code += "void hal_write_digital_pin(const char* pin_name, bool high) {\n";
        code += "    printf(\"[HAL: STM32] Atomic BSRR Write pin %s -> %s\\n\", pin_name, high ? \"HIGH (1)\" : \"LOW (0)\");\n";
        code += "    /* GPIOA->BSRR = high ? (1 << pin_num) : (1 << (pin_num + 16)); */\n";
        code += "}\n\n";
    } else if (fam.compare("ESP32", Qt::CaseInsensitive) == 0) {
        code += "void hal_write_digital_pin(const char* pin_name, bool high) {\n";
        code += "    printf(\"[HAL: ESP32] gpio_set_level(%s, %d)\\n\", pin_name, high ? 1 : 0);\n";
        code += "}\n\n";
    } else if (fam.compare("RP2040", Qt::CaseInsensitive) == 0) {
        code += "void hal_write_digital_pin(const char* pin_name, bool high) {\n";
        code += "    printf(\"[HAL: RP2040] gpio_put(%s, %d)\\n\", pin_name, high ? 1 : 0);\n";
        code += "}\n\n";
    } else if (fam.contains("i.MX", Qt::CaseInsensitive) || fam.compare("NXP", Qt::CaseInsensitive) == 0) {
        code += "void hal_write_digital_pin(const char* pin_name, bool high) {\n";
        code += "    printf(\"[HAL: i.MX RT] GPIO_PinWrite(%s, %d)\\n\", pin_name, high ? 1 : 0);\n";
        code += "}\n\n";
    } else if (fam.contains("Nordic", Qt::CaseInsensitive) || fam.contains("nRF", Qt::CaseInsensitive)) {
        code += "void hal_write_digital_pin(const char* pin_name, bool high) {\n";
        code += "    printf(\"[HAL: Nordic] gpio_pin_set(dev, %s, %d)\\n\", pin_name, high ? 1 : 0);\n";
        code += "}\n\n";
    } else {
        code += "void hal_write_digital_pin(const char* pin_name, bool high) {\n";
        code += QString("    printf(\"[HAL: %1] Atomic Write pin %s -> %s\\n\", pin_name, high ? \"HIGH\" : \"LOW\");\n").arg(fam);
        code += "}\n\n";
    }

    code += "uint32_t hal_read_adc_raw(const char* pin_name) {\n";
    code += "    (void)pin_name;\n";
    code += "    return 2048;\n";
    code += "}\n\n";

    code += "float hal_read_adc_normalized(const char* pin_name) {\n";
    code += "    return (float)hal_read_adc_raw(pin_name) / 4095.0f;\n";
    code += "}\n\n";

    code += "void hal_write_pwm_duty(const char* pin_name, float duty_percent) {\n";
    code += QString("    printf(\"[HAL: %1] Set PWM duty on %s -> %.1f%%\\n\", pin_name, duty_percent);\n").arg(fam);
    code += "}\n\n";

    code += "float hal_read_sensor_value(const char* sensor_name) {\n";
    code += "    (void)sensor_name;\n";
    code += "    return 25.0f;\n";
    code += "}\n\n";

    code += "void target_hal_poll(void) {\n";
    code += "    /* Poll hardware peripherals and tick counters */\n";
    code += "}\n";

    return code;
}

QStringList TargetHalGenerator::s_lastPlanDiagnostics;

QStringList TargetHalGenerator::planDiagnostics() {
    return s_lastPlanDiagnostics;
}

} // namespace CodeGen
