#pragma once

// Task 5 — Board & Display settings that extend the existing DisplayConfig
// without changing it. All fields are optional in the .euiproj file: when the
// "boardDisplay" object is absent the defaults below reproduce exactly the
// behaviour the Resource Analyzer had before Task 5 (full framebuffer, single
// buffer), so existing estimates do not change until the user edits a setting.

#include <QString>
#include <QJsonObject>
#include <QStringList>

struct BoardDisplayConfig {
    // ── Buffering (feeds ResourceAnalysis::Breakdown::framebuffer_ram) ────
    enum class FramebufferMode {
        Full = 0,            // width x height x bytes (legacy behaviour)
        PartialSingle = 1,   // width x bufferLines x bytes
        PartialDouble = 2    // width x bufferLines x bytes x 2
    };

    FramebufferMode framebufferMode = FramebufferMode::Full;
    int partialBufferLines = 8;  // sensible default: 320x8x2 = 5 KB RGB565

    // ── Wiring (informational; no code-generator output changes in Task 5) ─
    // Empty strings mean "Custom / not specified".
    QString interfaceId;   // "SPI", "Parallel 8080", "RGB Dot-Clock", "MIPI-DSI", "LTDC", "eLCDIF"
    QString controllerId;  // "ST7789", "ILI9341", "ST7796"
    QString touchDriverId; // "XPT2046", "FT6206", "CST816S" (empty = None)

    // ── Effective framebuffer geometry for the analyzer ────────────────────
    struct FramebufferLayout {
        int bufferHeight = 0;
        int bufferCount = 1;
    };

    FramebufferLayout framebufferLayout(int displayHeight) const {
        FramebufferLayout layout;
        switch (framebufferMode) {
        case FramebufferMode::PartialSingle:
        case FramebufferMode::PartialDouble: {
            int lines = partialBufferLines;
            if (displayHeight > 0) {
                if (lines < 1) lines = 1;
                if (lines > displayHeight) lines = displayHeight;
            }
            layout.bufferHeight = lines;
            layout.bufferCount = (framebufferMode == FramebufferMode::PartialDouble) ? 2 : 1;
            break;
        }
        case FramebufferMode::Full:
        default:
            layout.bufferHeight = displayHeight;
            layout.bufferCount = 1;
            break;
        }
        return layout;
    }

    bool isPartial() const { return framebufferMode != FramebufferMode::Full; }

    static QString modeToString(FramebufferMode mode) {
        switch (mode) {
        case FramebufferMode::PartialSingle: return QStringLiteral("partialSingle");
        case FramebufferMode::PartialDouble: return QStringLiteral("partialDouble");
        case FramebufferMode::Full:
        default: return QStringLiteral("full");
        }
    }

    static FramebufferMode modeFromString(const QString& value) {
        const QString v = value.trimmed().toLower();
        if (v == QLatin1String("partialsingle") || v == QLatin1String("partial_single")) {
            return FramebufferMode::PartialSingle;
        }
        if (v == QLatin1String("partialdouble") || v == QLatin1String("partial_double")) {
            return FramebufferMode::PartialDouble;
        }
        return FramebufferMode::Full;
    }

    bool operator==(const BoardDisplayConfig& other) const {
        return framebufferMode == other.framebufferMode &&
               partialBufferLines == other.partialBufferLines &&
               interfaceId == other.interfaceId &&
               controllerId == other.controllerId &&
               touchDriverId == other.touchDriverId;
    }
    bool operator!=(const BoardDisplayConfig& other) const { return !(*this == other); }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["framebufferMode"] = modeToString(framebufferMode);
        obj["partialBufferLines"] = partialBufferLines;
        obj["interface"] = interfaceId;
        obj["controller"] = controllerId;
        obj["touchDriver"] = touchDriverId;
        return obj;
    }

    static BoardDisplayConfig fromJson(const QJsonObject& obj) {
        BoardDisplayConfig config;
        config.framebufferMode = modeFromString(obj.value("framebufferMode").toString("full"));
        int lines = obj.value("partialBufferLines").toInt(config.partialBufferLines);
        config.partialBufferLines = lines > 0 ? lines : 8;
        config.interfaceId = obj.value("interface").toString();
        config.controllerId = obj.value("controller").toString();
        config.touchDriverId = obj.value("touchDriver").toString();
        return config;
    }

    // ── UI option lists: only entries backed by the existing runtime HAL ────
    // src/runtime/ew_hal.h: ew_hal_bus_type_t + ew_adapter_config_t
    // (display_controller / touch_controller comments).
    static QStringList interfaceOptions() {
        return { QStringLiteral("SPI"),
                 QStringLiteral("Parallel 8080"),
                 QStringLiteral("RGB Dot-Clock"),
                 QStringLiteral("MIPI-DSI"),
                 QStringLiteral("LTDC"),
                 QStringLiteral("eLCDIF") };
    }

    static QStringList controllerOptions() {
        return { QStringLiteral("ST7789"),
                 QStringLiteral("ILI9341"),
                 QStringLiteral("ST7796") };
    }

    static QStringList touchDriverOptions() {
        return { QStringLiteral("XPT2046"),
                 QStringLiteral("FT6206"),
                 QStringLiteral("CST816S") };
    }

    // Interfaces that cannot drive a panel without an on-chip LCD/TFT
    // controller. SPI and 8080 panels bring their own controller.
    static bool interfaceRequiresLcdController(const QString& id) {
        return id == QLatin1String("LTDC") ||
               id == QLatin1String("eLCDIF") ||
               id == QLatin1String("MIPI-DSI") ||
               id == QLatin1String("RGB Dot-Clock");
    }
};
