#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <cstddef>
#include "DisplayConfig.h"

class Project;
class Screen;
class UIComponent;

enum class ResourceWarningSeverity {
    Info,
    Warning,
    Critical
};

struct ResourceWarning {
    ResourceWarningSeverity severity = ResourceWarningSeverity::Info;
    QString category;
    QString message;
};

struct ResourceReport {
    // Framebuffer
    size_t framebufferBytes = 0;
    size_t framebufferWidth = 0;
    size_t framebufferHeight = 0;
    int bitsPerPixel = 16;
    int bufferCount = 1;
    size_t rowStrideBytes = 0;

    // Flash estimations (Bytes)
    size_t flashFrameworkBaseBytes = 0;
    size_t flashComponentCodeBytes = 0;
    size_t flashAssetsBytes = 0;
    size_t flashFontsBytes = 0;
    size_t flashBindingsBytes = 0;
    size_t flashTotalEstimatedBytes = 0;

    // RAM estimations (Bytes)
    size_t ramFramebufferBytes = 0;
    size_t ramComponentStateBytes = 0;
    size_t ramDataSourcesBytes = 0;
    size_t ramFrameworkWorkBufferBytes = 0;
    size_t ramTotalEstimatedBytes = 0;

    // Counts
    int totalScreens = 0;
    int totalComponents = 0;
    int totalDataSources = 0;
    int totalDataBindings = 0;

    // Target Hardware Capacity (Bytes)
    bool hasTargetLimits = false;
    QString targetDeviceName;
    size_t targetFlashCapacityBytes = 0;
    size_t targetRamCapacityBytes = 0;

    // Utilization Percentages (0 - 100+ %)
    double flashUtilizationPercent = 0.0;
    double ramUtilizationPercent = 0.0;

    // Warnings and Diagnostics
    QList<ResourceWarning> warnings;

    QString summaryText() const;
};

class ResourceAnalyzer {
public:
    ResourceAnalyzer();

    /// Calculates exact framebuffer size accounting for dimensions, bpp, buffer count, and DMA alignment
    static size_t calculateFramebufferBytes(int width, int height, int bpp, int bufferCount, size_t dmaAlignment = 16);

    /// Generates a comprehensive resource estimation report for a given Project
    ResourceReport analyzeProject(const Project* project) const;

    /// Configurable warning thresholds (defaults: 80% warning, 95% critical)
    void setWarningThreshold(double percent) { m_warningThreshold = percent; }
    void setCriticalThreshold(double percent) { m_criticalThreshold = percent; }
    double warningThreshold() const { return m_warningThreshold; }
    double criticalThreshold() const { return m_criticalThreshold; }

private:
    double m_warningThreshold = 80.0;
    double m_criticalThreshold = 95.0;

    size_t estimateComponentFlash(const UIComponent* comp) const;
    size_t estimateComponentRam(const UIComponent* comp) const;
};
