#pragma once

#include <QString>
#include <QVector>
#include <QDateTime>
#include <cstdint>

#include "CapabilityDatabase.h"

struct ResourceAnalysis {
    // RAM Usage (bytes)
    uint32_t ram_used = 0;
    uint32_t ram_total = 0;
    float ram_percent = 0.0f;

    // Flash Usage (bytes)
    uint32_t flash_used = 0;
    uint32_t flash_total = 0;
    float flash_percent = 0.0f;

    // CPU Estimate
    float cpu_overhead_percent = 0.0f;  // Estimated render + event loop overhead

    // Breakdown
    struct Breakdown {
        uint32_t framework_flash = 0;  // Framework baseline
        uint32_t framework_ram = 0;    // Framework baseline RAM
        uint32_t components_ram = 0;   // All components combined RAM
        uint32_t components_flash = 0; // Component code Flash
        uint32_t assets_flash = 0;     // Images, fonts Flash
        uint32_t assets_ram = 0;       // Decoded assets in RAM
        uint32_t user_space_flash = 0; // Available for user code
        uint32_t user_space_ram = 0;   // Available for user dynamic allocation
    } breakdown;

    // Warnings & Status
    enum Status { OK, WARNING, CRITICAL, MISMATCH };
    Status status = OK;
    QVector<QString> warnings;
    QString summary;

    // Timestamps and Metadata
    QDateTime analyzed_at;
    QString device_name;
    QString target_framework;      // "qt-for-mcus", "ugfx", "lvgl"
    DeviceProfile device;
};