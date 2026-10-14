#pragma once

#include <QString>
#include <QMap>
#include <cstdint>

class UIComponent;

namespace EstimationFormulas {
    // Per-component RAM/Flash overhead (bytes)
    struct ComponentFootprint {
        uint32_t ram = 0;
        uint32_t flash = 0;
        float cpu_percent = 0.0f;  // Render time as % of frame time @ 60fps
    };

    // Framework baseline (bytes)
    struct FrameworkBaseline {
        QString framework;            // "qt-for-mcus", "ugfx", "lvgl"
        uint32_t flash_minimum = 0;   // Absolute minimum flash for framework
        uint32_t ram_minimum = 0;     // Absolute minimum RAM for framework
        uint32_t flash_per_component = 0; // Adder per component
        uint32_t ram_per_component = 0;   // Adder per component
    };

    extern QMap<QString, ComponentFootprint> COMPONENT_FOOTPRINTS;
    extern QMap<QString, FrameworkBaseline> FRAMEWORK_BASELINES;

    void initDefaultBaselines();
    bool loadBaselinesFromJson(const QString& jsonPath);

    ComponentFootprint estimateComponent(const UIComponent* comp);
    FrameworkBaseline getFrameworkBaseline(const QString& framework);
}
