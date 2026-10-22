#pragma once

#include <QString>
#include <QStringList>
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

    /**
     * Phase C1 diagnostics from the most recent generateHalSource() call for a
     * configured project (skipped pins/peripherals, clock assumptions). Empty
     * when the template path was used. Exports run on the GUI thread, so the
     * single-slot storage is safe in practice.
     */
    static QStringList planDiagnostics();

private:
    static QStringList s_lastPlanDiagnostics;
};

} // namespace CodeGen
