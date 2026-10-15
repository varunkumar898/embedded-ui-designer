#include "EstimationFormulas.h"
#include "UIComponent.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace EstimationFormulas {

QMap<QString, ComponentFootprint> COMPONENT_FOOTPRINTS;
QMap<QString, FrameworkBaseline> FRAMEWORK_BASELINES;

static bool s_initialized = false;

void initDefaultBaselines() {
    if (s_initialized) return;

    // Frameworks
    FRAMEWORK_BASELINES["qt-for-mcus"] = {
        "qt-for-mcus",
        400 * 1024, // 400 KB
        50 * 1024,  // 50 KB
        200,
        50
    };

    FRAMEWORK_BASELINES["ugfx"] = {
        "ugfx",
        120 * 1024, // 120 KB
        30 * 1024,  // 30 KB
        150,
        40
    };

    FRAMEWORK_BASELINES["lvgl"] = {
        "lvgl",
        80 * 1024,  // 80 KB
        25 * 1024,  // 25 KB
        100,
        30
    };

    FRAMEWORK_BASELINES["embedweb"] = {
        "embedweb",
        16 * 1024,  // 16 KB Flash minimum
        14 * 1024,  // 14 KB Static RAM minimum (128-node bounded tree)
        80,         // Flash per component
        20          // RAM per component
    };

    // Components
    COMPONENT_FOOTPRINTS["Button"]       = { 80, 200, 1.5f };
    COMPONENT_FOOTPRINTS["Label"]        = { 40, 100, 0.5f };
    COMPONENT_FOOTPRINTS["Text"]         = { 40, 100, 0.5f };
    COMPONENT_FOOTPRINTS["TextInput"]    = { 150, 300, 2.0f };
    COMPONENT_FOOTPRINTS["Image"]        = { 100, 150, 3.0f };
    COMPONENT_FOOTPRINTS["ProgressBar"]  = { 60, 120, 1.0f };
    COMPONENT_FOOTPRINTS["LinearProgress"] = { 60, 120, 1.0f };
    COMPONENT_FOOTPRINTS["CircularProgress"] = { 70, 150, 1.5f };
    COMPONENT_FOOTPRINTS["Slider"]       = { 70, 140, 1.5f };
    COMPONENT_FOOTPRINTS["Rectangle"]    = { 30, 80, 0.3f };
    COMPONENT_FOOTPRINTS["Circle"]       = { 40, 100, 0.5f };
    COMPONENT_FOOTPRINTS["Switch"]       = { 50, 110, 1.0f };
    COMPONENT_FOOTPRINTS["Checkbox"]     = { 50, 110, 1.0f };
    COMPONENT_FOOTPRINTS["Gauge"]        = { 90, 220, 2.0f };
    COMPONENT_FOOTPRINTS["Speedometer"]  = { 100, 240, 2.0f };
    COMPONENT_FOOTPRINTS["Battery"]      = { 60, 130, 1.0f };
    COMPONENT_FOOTPRINTS["Pressure"]     = { 80, 200, 1.5f };
    COMPONENT_FOOTPRINTS["Rpm"]          = { 90, 220, 2.0f };
    COMPONENT_FOOTPRINTS["Temperature"]  = { 70, 160, 1.2f };
    COMPONENT_FOOTPRINTS["TabView"]      = { 180, 350, 2.5f };
    COMPONENT_FOOTPRINTS["NavigationBar"] = { 150, 300, 2.0f };
    COMPONENT_FOOTPRINTS["List"]         = { 200, 400, 3.0f };
    COMPONENT_FOOTPRINTS["Table"]        = { 250, 500, 3.5f };
    COMPONENT_FOOTPRINTS["Group"]        = { 20, 50, 0.2f };
    COMPONENT_FOOTPRINTS["Path"]         = { 200, 400, 5.0f };
    COMPONENT_FOOTPRINTS["PathComponent"] = { 200, 400, 5.0f };
    COMPONENT_FOOTPRINTS["CustomInstance"] = { 120, 250, 1.5f };

    s_initialized = true;
}

// Optional loader for the resources/estimation_baselines.json snapshot.
// Deliberately has no startup caller: initDefaultBaselines() is the single
// source of truth (the JSON mirrors it, verified by the test suite).
bool loadBaselinesFromJson(const QString& jsonPath) {
    initDefaultBaselines();

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return false;

    QJsonObject root = doc.object();

    if (root.contains("frameworks") && root.value("frameworks").isObject()) {
        QJsonObject frameworks = root.value("frameworks").toObject();
        for (auto it = frameworks.constBegin(); it != frameworks.constEnd(); ++it) {
            QString name = it.key().toLower();
            QJsonObject fw = it.value().toObject();
            FrameworkBaseline bl;
            bl.framework = name;
            bl.flash_minimum = fw.value("flash_minimum_kb").toInt(100) * 1024;
            bl.ram_minimum = fw.value("ram_minimum_kb").toInt(20) * 1024;
            bl.flash_per_component = fw.value("flash_per_component_bytes").toInt(100);
            bl.ram_per_component = fw.value("ram_per_component_bytes").toInt(30);
            FRAMEWORK_BASELINES[name] = bl;
        }
    }

    if (root.contains("components") && root.value("components").isObject()) {
        QJsonObject comps = root.value("components").toObject();
        for (auto it = comps.constBegin(); it != comps.constEnd(); ++it) {
            QString name = it.key();
            QJsonObject c = it.value().toObject();
            ComponentFootprint fp;
            fp.ram = c.value("ram_bytes").toInt(50);
            fp.flash = c.value("flash_bytes").toInt(100);
            fp.cpu_percent = static_cast<float>(c.value("cpu_percent").toDouble(1.0));
            COMPONENT_FOOTPRINTS[name] = fp;
        }
    }

    return true;
}

ComponentFootprint estimateComponent(const UIComponent* comp) {
    initDefaultBaselines();
    if (!comp) return { 0, 0, 0.0f };

    QString type = comp->componentType();
    if (COMPONENT_FOOTPRINTS.contains(type)) {
        return COMPONENT_FOOTPRINTS.value(type);
    }

    // Default generic footprint
    return { 50, 100, 1.0f };
}

FrameworkBaseline getFrameworkBaseline(const QString& framework) {
    initDefaultBaselines();
    QString norm = framework.trimmed().toLower();
    if (FRAMEWORK_BASELINES.contains(norm)) {
        return FRAMEWORK_BASELINES.value(norm);
    }
    // Default fallback
    return FRAMEWORK_BASELINES.value("qt-for-mcus");
}

}
