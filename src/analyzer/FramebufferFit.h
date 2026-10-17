#pragma once

// Task 5 — framebuffer size & fit evaluation shared by the Resource Analyzer,
// the Board & Display dock and the "BoardDisplay" diagnostics source.
//
// The byte count deliberately reuses ResourceEstimator's framebuffer math
// (including the 16-byte DMA row alignment) so the analyzer, the dock's live
// preview and the diagnostics always agree with each other and with the
// pre-Task-5 estimator when the default (full frame) mode is used.

#include <QString>
#include <cstdint>

#include "CapabilityDatabase.h"
#include "models/BoardDisplayConfig.h"
#include "ResourceEstimator.h"

namespace FramebufferFit {

struct Metrics {
    int width = 0;
    int height = 0;             // display lines
    int colorDepth = 16;        // bits per pixel as stored in DisplayConfig
    int bufferHeight = 0;       // lines held in one buffer (== height for full frame)
    int bufferCount = 1;        // 1 for full/partial-single, 2 for partial-double
    bool fullFrame = true;
    uint32_t rawStrideBytes = 0;   // width x colorDepth / 8 (rounded up)
    uint32_t strideBytes = 0;      // raw stride aligned to the DMA boundary
    uint32_t bytes = 0;            // strideBytes x bufferHeight x bufferCount
};

struct Result {
    enum class Status { Fits, DoesNotFit, Unknown };

    Status status = Status::Unknown;
    uint32_t bytes = 0;
    uint32_t totalRamBytes = 0;
    uint32_t framebufferCapableBytes = 0;      // sum of regions documented usable=yes
    bool framebufferCapableRegionsDocumented = false;
    bool regionSuitabilityUnknown = false;     // no usable=yes region documented
    QString formula;                           // human-readable size formula
    QString message;                           // one-line status sentence
    QString hint;                              // remediation / caveat

    bool fits() const { return status == Status::Fits; }
    bool doesNotFit() const { return status == Status::DoesNotFit; }
};

inline int bytesPerPixelForDepth(int colorDepth) {
    if (colorDepth >= 24) return 3;
    if (colorDepth >= 16) return 2;
    if (colorDepth >= 8) return 1;
    return 1; // 1 bpp: handled by the stride math, only used for display
}

inline Metrics computeMetrics(int width, int height, int colorDepth,
                              const BoardDisplayConfig& config,
                              size_t dmaAlignment = 16) {
    Metrics m;
    m.width = width;
    m.height = height;
    m.colorDepth = colorDepth;

    const BoardDisplayConfig::FramebufferLayout layout = config.framebufferLayout(height);
    m.bufferHeight = layout.bufferHeight;
    m.bufferCount = layout.bufferCount;
    m.fullFrame = config.framebufferMode == BoardDisplayConfig::FramebufferMode::Full;

    if (width > 0 && colorDepth > 0) {
        m.rawStrideBytes = static_cast<uint32_t>((static_cast<size_t>(width) *
                                                  static_cast<size_t>(colorDepth) + 7) / 8);
    }
    m.strideBytes = static_cast<uint32_t>(
        ResourceEstimator::calculateRowStrideBytes(width, colorDepth, dmaAlignment));
    m.bytes = static_cast<uint32_t>(
        ResourceEstimator::calculateFramebufferBytes(width, m.bufferHeight, colorDepth,
                                                     m.bufferCount, dmaAlignment));
    return m;
}

inline QString numberWithGroups(quint64 value) {
    const QString digits = QString::number(value);
    QString out;
    out.reserve(digits.size() + digits.size() / 3);
    const int len = digits.size();
    for (int i = 0; i < len; ++i) {
        if (i > 0 && (len - i) % 3 == 0) {
            out += QLatin1Char(',');
        }
        out += digits.at(i);
    }
    return out;
}

inline QString formatBytes(uint32_t bytes) {
    if (bytes >= 1024u * 1024u) {
        return QString("%1 B (%2 MB)").arg(numberWithGroups(bytes)).arg(bytes / (1024.0 * 1024.0), 0, 'f', 2);
    }
    return QString("%1 B (%2 KB)").arg(numberWithGroups(bytes)).arg(bytes / 1024.0, 0, 'f', 1);
}

inline QString formulaText(const Metrics& m) {
    QString s;
    if (m.colorDepth == 8 || m.colorDepth == 16 || m.colorDepth == 24 || m.colorDepth == 32) {
        // Exact byte-per-pixel formats (RGB332/RGB565/RGB888/ARGB8888).
        s = QString("%1 px × %2 lines × %3 B/px")
                .arg(m.width)
                .arg(m.bufferHeight)
                .arg(m.colorDepth / 8);
    } else {
        // Sub-byte formats (1 bpp / indexed): keep the stride in place.
        s = QString("%1 px × %2 bits/px = %3 B/row × %4 lines")
                .arg(m.width)
                .arg(m.colorDepth)
                .arg(m.strideBytes)
                .arg(m.bufferHeight);
    }
    if (m.bufferCount > 1) {
        s += QStringLiteral(" × 2");
    }
    s += QString(" = %1 B").arg(numberWithGroups(m.bytes));
    if (m.fullFrame) {
        s += QStringLiteral(" (full frame)");
    } else if (m.bufferCount > 1) {
        s += QStringLiteral(" (partial buffer, double)");
    } else {
        s += QStringLiteral(" (partial buffer, single)");
    }
    if (m.strideBytes != m.rawStrideBytes) {
        s += QString(" · row stride %1 B → %2 B (DMA alignment)")
                 .arg(numberWithGroups(m.rawStrideBytes))
                 .arg(numberWithGroups(m.strideBytes));
    }
    return s;
}

// Evaluates whether the configured framebuffer fits the target's RAM.
//
// Rules (Task 5):
//   * needs more than the device's total RAM          -> DoesNotFit (red)
//   * documented framebuffer-capable regions exist    -> Fits iff it fits them
//   * otherwise (region suitability undocumented)     -> Fits (estimated) plus
//     regionSuitabilityUnknown, surfaced as an Info notice, never an error.
inline Result evaluate(const DeviceProfile& device, int width, int height,
                       int colorDepth, const BoardDisplayConfig& config) {
    Result r;
    const Metrics m = computeMetrics(width, height, colorDepth, config);
    r.bytes = m.bytes;
    r.formula = formulaText(m);

    if (device.name.isEmpty() || device.ram_kb == 0) {
        r.status = Result::Status::Unknown;
        r.message = QStringLiteral("Cannot check the framebuffer fit: target RAM is unknown.");
        r.hint = QStringLiteral("Select a target MCU in the Target section to check the framebuffer against its RAM.");
        return r;
    }

    r.totalRamBytes = device.ram_kb * 1024u;

    uint32_t capableBytes = 0;
    bool anyCapable = false;
    bool anyUnknownRegion = false;
    for (const DeviceProfile::RamRegion& region : device.ram_regions) {
        if (region.usable_for_framebuffer == 1) {
            capableBytes += region.kb * 1024u;
            anyCapable = true;
        } else if (region.usable_for_framebuffer == -1) {
            anyUnknownRegion = true;
        }
    }
    r.framebufferCapableBytes = capableBytes;
    r.framebufferCapableRegionsDocumented = anyCapable;
    r.regionSuitabilityUnknown = !anyCapable;

    if (m.bytes > r.totalRamBytes) {
        r.status = Result::Status::DoesNotFit;
        r.message = QString("Does not fit: the framebuffer needs %1 but %2 only has %3 of RAM.")
                        .arg(formatBytes(m.bytes), device.name, formatBytes(r.totalRamBytes));
        r.hint = m.fullFrame
            ? QStringLiteral("Try partial buffering: a partial buffer stores only a few lines at a time and needs far less RAM.")
            : QStringLiteral("Reduce the partial buffer height or the display resolution.");
        return r;
    }

    if (anyCapable) {
        if (m.bytes <= capableBytes) {
            r.status = Result::Status::Fits;
            r.message = QStringLiteral("Fits (estimated).");
            return r;
        }
        r.status = Result::Status::DoesNotFit;
        r.message = QString("Does not fit framebuffer-capable RAM: needs %1, documented capable regions total %2.")
                        .arg(formatBytes(m.bytes), formatBytes(capableBytes));
        r.hint = QStringLiteral("Try partial buffering or choose a device with more framebuffer-capable RAM.");
        return r;
    }

    if (!device.ram_regions.isEmpty() && !anyUnknownRegion) {
        // Every documented region explicitly says "not usable for a framebuffer".
        r.status = Result::Status::DoesNotFit;
        r.message = QString("No framebuffer-capable RAM region is documented for %1.").arg(device.name);
        r.hint = QStringLiteral("The framebuffer cannot be placed in the documented memory regions.");
        return r;
    }

    // Total RAM is sufficient but region suitability is not documented: report
    // a fit estimate with the caveat instead of failing the check.
    r.status = Result::Status::Fits;
    r.message = QStringLiteral("Fits (estimated).");
    r.hint = QStringLiteral("Region suitability not documented; cannot confirm the framebuffer can be placed in this RAM.");
    return r;
}

} // namespace FramebufferFit
