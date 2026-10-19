#pragma once

#include "HardwareModel.h"
#include "PinCapabilities.h"

#include <QString>
#include <QVector>

namespace Hardware {

/**
 * Validates the project's pin/peripheral configuration against the sourced
 * capability data of the selected device.
 *
 * Rules:
 *  - Values documented in the capability data are checked strictly
 *    (peripherals documented as absent, and pin conflicts, are Errors).
 *  - Anything the data does not cover stays Info (unknown) or Warning
 *    (signal documented on other pins): the generator must omit it, never
 *    guess, and never crash.
 *  - Conflicts (one pin assigned to two different functions) are Errors.
 */
struct PinIssue {
    enum class Severity { Error, Warning, Info };

    Severity severity = Severity::Info;
    QString message;
    QString pin;            // related pin, may be empty
    QString peripheral;     // related peripheral, may be empty
};

class PinsValidator {
public:
    static QVector<PinIssue> validate(const HardwareConfig& config, const PinCapabilities* caps);
};

} // namespace Hardware
