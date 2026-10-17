#pragma once

#include <QString>
#include <QMap>
#include "Project.h"
#include "hardware/BoardRegistry.h"

namespace CodeGen {

/**
 * @brief Generates target-specific Hardware Abstraction Layer (HAL) source & header files
 * for generated embedded projects (STM32 HAL, ESP-IDF, Raspberry Pi Linux, Generic PC Simulator).
 */
class TargetHalGenerator {
public:
    static QString generateHalHeader(const Project* project, const QString& targetMcu = QString());
    static QString generateHalSource(const Project* project, const QString& targetMcu = QString());
    static QString generateDriverInitCode(const BoardDef& board,
                                         const QString& protocol,
                                         const QMap<QString, QString>& pinAssignments = {});
};

} // namespace CodeGen
