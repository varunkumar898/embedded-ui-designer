#include "ResourceAnalyzer.h"
#include "project/Project.h"
#include "models/Screen.h"
#include "models/UIComponent.h"
#include "models/DataSource.h"
#include "models/DataBinding.h"
#include "hardware/DeviceDatabase.h"
#include <algorithm>

ResourceAnalyzer::ResourceAnalyzer() {
}

size_t ResourceAnalyzer::calculateFramebufferBytes(int width, int height, int bpp, int bufferCount, size_t dmaAlignment) {
    if (width <= 0 || height <= 0 || bpp <= 0 || bufferCount <= 0) {
        return 0;
    }

    // Calculate raw row bytes
    size_t unalignedRowBytes = (static_cast<size_t>(width) * static_cast<size_t>(bpp) + 7) / 8;

    // Align row stride to dmaAlignment boundary (typically 16 bytes for DMA2D / Chrom-ART)
    size_t rowStrideBytes = unalignedRowBytes;
    if (dmaAlignment > 1) {
        rowStrideBytes = (unalignedRowBytes + (dmaAlignment - 1)) & ~(dmaAlignment - 1);
    }

    size_t singleFrameBytes = rowStrideBytes * static_cast<size_t>(height);
    return singleFrameBytes * static_cast<size_t>(bufferCount);
}

size_t ResourceAnalyzer::estimateComponentFlash(const UIComponent* comp) const {
    if (!comp) return 0;
    QString type = comp->componentType().toLower();

    // Complex custom widgets have larger C drawing / logic routines
    if (type == "speedometer" || type == "gauge" || type == "rpm") return 1800;
    if (type == "table" || type == "list" || type == "tabview") return 1400;
    if (type == "path" || type == "circularprogress" || type == "valuevisualization") return 1200;
    if (type == "battery" || type == "pressure" || type == "temperature") return 1000;
    if (type == "slider" || type == "progressbar" || type == "switch") return 800;
    if (type == "button" || type == "checkbox" || type == "textinput") return 600;
    if (type == "label" || type == "rectangle" || type == "circle" || type == "image") return 400;

    return 500;
}

size_t ResourceAnalyzer::estimateComponentRam(const UIComponent* comp) const {
    if (!comp) return 0;
    QString type = comp->componentType().toLower();

    // Widget C runtime state structures (lv_obj_t + widget specific state struct)
    if (type == "table" || type == "list") return 512;
    if (type == "speedometer" || type == "gauge" || type == "rpm" || type == "path") return 384;
    if (type == "tabview" || type == "valuevisualization") return 320;
    if (type == "slider" || type == "progressbar" || type == "textinput") return 256;
    if (type == "button" || type == "checkbox" || type == "switch" || type == "battery") return 192;
    if (type == "label" || type == "rectangle" || type == "circle" || type == "image") return 128;

    return 160;
}

ResourceReport ResourceAnalyzer::analyzeProject(const Project* project) const {
    ResourceReport report;
    if (!project) return report;

    // 1. Display Configuration & Framebuffer
    DisplayConfig disp = project->displayConfig();
    report.framebufferWidth = disp.width > 0 ? disp.width : 320;
    report.framebufferHeight = disp.height > 0 ? disp.height : 240;

    // Map color format to bits per pixel
    QString cf = disp.colorFormat.toLower();
    if (cf.contains("rgb888") || cf.contains("argb8888") || cf.contains("32")) {
        report.bitsPerPixel = (cf.contains("32") || cf.contains("argb")) ? 32 : 24;
    } else if (cf.contains("rgb565") || cf.contains("16")) {
        report.bitsPerPixel = 16;
    } else if (cf.contains("monochrome") || cf.contains("1bit") || cf.contains("1")) {
        report.bitsPerPixel = 1;
    } else {
        report.bitsPerPixel = 16; // Default to RGB565 for embedded
    }

    report.bufferCount = disp.doubleBuffered ? 2 : 1;

    size_t unalignedRow = (report.framebufferWidth * report.bitsPerPixel + 7) / 8;
    report.rowStrideBytes = (unalignedRow + 15) & ~15; // 16-byte DMA alignment
    report.framebufferBytes = calculateFramebufferBytes(report.framebufferWidth, report.framebufferHeight, report.bitsPerPixel, report.bufferCount, 16);
    report.ramFramebufferBytes = report.framebufferBytes;

    // 2. Framework Footprint Baseline
    QString framework = project->targetFramework().toLower();
    if (framework.contains("lvgl")) {
        report.flashFrameworkBaseBytes = 64 * 1024; // ~64 KB LVGL v8/v9 base
        report.ramFrameworkWorkBufferBytes = (report.framebufferWidth * (report.bitsPerPixel / 8) * (report.framebufferHeight / 10)); // 1/10th screen draw buffer
        if (report.ramFrameworkWorkBufferBytes < 4096) report.ramFrameworkWorkBufferBytes = 4096;
    } else if (framework.contains("ugfx")) {
        report.flashFrameworkBaseBytes = 28 * 1024; // ~28 KB uGFX
        report.ramFrameworkWorkBufferBytes = 6 * 1024;  // ~6 KB work buffer
    } else if (framework.contains("qtmcu") || framework.contains("qul")) {
        report.flashFrameworkBaseBytes = 128 * 1024; // ~128 KB Qt for MCUs
        report.ramFrameworkWorkBufferBytes = 32 * 1024;  // ~32 KB QUL runtime
    } else {
        report.flashFrameworkBaseBytes = 32 * 1024;
        report.ramFrameworkWorkBufferBytes = 8 * 1024;
    }

    // 3. Components across all screens
    report.totalScreens = project->screens().size();
    size_t componentFlash = 0;
    size_t componentRam = 0;
    int compCount = 0;

    for (const Screen* screen : project->screens()) {
        if (!screen) continue;
        for (const UIComponent* comp : screen->components()) {
            if (!comp) continue;
            compCount++;
            componentFlash += estimateComponentFlash(comp);
            componentRam += estimateComponentRam(comp);

            // If image component, estimate uncompressed bitmap asset in Flash
            if (comp->componentType().compare("image", Qt::CaseInsensitive) == 0) {
                size_t imgW = std::max(1, static_cast<int>(comp->compWidth()));
                size_t imgH = std::max(1, static_cast<int>(comp->compHeight()));
                size_t assetBytes = imgW * imgH * (report.bitsPerPixel / 8);
                report.flashAssetsBytes += assetBytes;
            }
        }
    }
    report.totalComponents = compCount;
    report.flashComponentCodeBytes = componentFlash;
    report.ramComponentStateBytes = componentRam;

    // 4. DataSources and DataBindings
    report.totalDataSources = project->dataSources().size();
    report.totalDataBindings = project->dataBindings().size();
    report.ramDataSourcesBytes = (report.totalDataSources * 96) + (report.totalDataBindings * 48);
    report.flashBindingsBytes = (report.totalDataSources * 128) + (report.totalDataBindings * 160);

    // 5. Default embedded typography font tables (12pt, 16pt, 24pt subsets)
    report.flashFontsBytes = 24 * 1024; // Standard embedded font glyphs

    // 6. Total Flash and RAM
    report.flashTotalEstimatedBytes = report.flashFrameworkBaseBytes +
                                      report.flashComponentCodeBytes +
                                      report.flashAssetsBytes +
                                      report.flashFontsBytes +
                                      report.flashBindingsBytes;

    report.ramTotalEstimatedBytes = report.ramFramebufferBytes +
                                    report.ramComponentStateBytes +
                                    report.ramDataSourcesBytes +
                                    report.ramFrameworkWorkBufferBytes;

    // 7. Target Hardware Capacity Lookup
    Hardware::HardwareConfig hw = project->hardwareConfig();
    report.targetDeviceName = !hw.deviceId.isEmpty() ? hw.deviceId : hw.boardId;

    if (hw.flashBytes > 0 && hw.ramBytes > 0) {
        report.hasTargetLimits = true;
        report.targetFlashCapacityBytes = hw.flashBytes;
        report.targetRamCapacityBytes = hw.ramBytes;
    } else if (!report.targetDeviceName.isEmpty()) {
        // Look up in DeviceDatabase
        Hardware::DeviceDefinition dev = Hardware::DeviceDatabase::instance().findDevice(report.targetDeviceName);
        if (dev.flashBytes > 0 && dev.ramBytes > 0) {
            report.hasTargetLimits = true;
            report.targetFlashCapacityBytes = dev.flashBytes;
            report.targetRamCapacityBytes = dev.ramBytes;
        } else {
            Hardware::BoardDefinition board = Hardware::DeviceDatabase::instance().findBoard(report.targetDeviceName);
            if (!board.mcuPartNumber.isEmpty()) {
                Hardware::DeviceDefinition mcuDev = Hardware::DeviceDatabase::instance().findDevice(board.mcuPartNumber);
                if (mcuDev.flashBytes > 0 && mcuDev.ramBytes > 0) {
                    report.hasTargetLimits = true;
                    report.targetFlashCapacityBytes = mcuDev.flashBytes;
                    report.targetRamCapacityBytes = mcuDev.ramBytes;
                }
            }
        }
    }

    // 8. Utilization & Warnings
    if (report.hasTargetLimits && report.targetFlashCapacityBytes > 0) {
        report.flashUtilizationPercent = (static_cast<double>(report.flashTotalEstimatedBytes) / static_cast<double>(report.targetFlashCapacityBytes)) * 100.0;
        if (report.flashUtilizationPercent > 100.0) {
            report.warnings.append({ResourceWarningSeverity::Critical, "Flash", QString("Estimated Flash (%1 KB) exceeds target capacity (%2 KB) by %3 KB!")
                .arg(report.flashTotalEstimatedBytes / 1024)
                .arg(report.targetFlashCapacityBytes / 1024)
                .arg((report.flashTotalEstimatedBytes - report.targetFlashCapacityBytes) / 1024)});
        } else if (report.flashUtilizationPercent >= m_criticalThreshold) {
            report.warnings.append({ResourceWarningSeverity::Critical, "Flash", QString("Flash utilization is critically high (%1% of %2 KB).")
                .arg(QString::number(report.flashUtilizationPercent, 'f', 1))
                .arg(report.targetFlashCapacityBytes / 1024)});
        } else if (report.flashUtilizationPercent >= m_warningThreshold) {
            report.warnings.append({ResourceWarningSeverity::Warning, "Flash", QString("Flash utilization exceeds warning threshold (%1% of %2 KB).")
                .arg(QString::number(report.flashUtilizationPercent, 'f', 1))
                .arg(report.targetFlashCapacityBytes / 1024)});
        }
    }

    if (report.hasTargetLimits && report.targetRamCapacityBytes > 0) {
        report.ramUtilizationPercent = (static_cast<double>(report.ramTotalEstimatedBytes) / static_cast<double>(report.targetRamCapacityBytes)) * 100.0;
        if (report.ramUtilizationPercent > 100.0) {
            report.warnings.append({ResourceWarningSeverity::Critical, "RAM", QString("Estimated RAM (%1 KB) exceeds target internal SRAM (%2 KB) by %3 KB! An external PSRAM/SDRAM frame buffer is required.")
                .arg(report.ramTotalEstimatedBytes / 1024)
                .arg(report.targetRamCapacityBytes / 1024)
                .arg((report.ramTotalEstimatedBytes - report.targetRamCapacityBytes) / 1024)});
        } else if (report.ramUtilizationPercent >= m_criticalThreshold) {
            report.warnings.append({ResourceWarningSeverity::Critical, "RAM", QString("RAM utilization is critically high (%1% of %2 KB).")
                .arg(QString::number(report.ramUtilizationPercent, 'f', 1))
                .arg(report.targetRamCapacityBytes / 1024)});
        } else if (report.ramUtilizationPercent >= m_warningThreshold) {
            report.warnings.append({ResourceWarningSeverity::Warning, "RAM", QString("RAM utilization exceeds warning threshold (%1% of %2 KB).")
                .arg(QString::number(report.ramUtilizationPercent, 'f', 1))
                .arg(report.targetRamCapacityBytes / 1024)});
        }
    } else if (!report.hasTargetLimits) {
        report.warnings.append({ResourceWarningSeverity::Info, "Target", "No target MCU selected with defined memory capacities. Metrics display estimated footprints."});
    }

    return report;
}

QString ResourceReport::summaryText() const {
    QString str;
    str += QString("Resolution: %1x%2 (%3 bpp, %4 buffer%5)\n")
        .arg(framebufferWidth).arg(framebufferHeight).arg(bitsPerPixel).arg(bufferCount).arg(bufferCount > 1 ? "s" : "");
    str += QString("Framebuffer RAM: %1 KB (DMA row stride: %2 bytes)\n")
        .arg(framebufferBytes / 1024).arg(rowStrideBytes);
    str += QString("Total Estimated Flash: %1 KB\n").arg(flashTotalEstimatedBytes / 1024);
    str += QString("Total Estimated RAM: %1 KB\n").arg(ramTotalEstimatedBytes / 1024);
    if (hasTargetLimits) {
        str += QString("Target: %1 (Flash: %2 KB [%3%], RAM: %4 KB [%5%])\n")
            .arg(targetDeviceName)
            .arg(targetFlashCapacityBytes / 1024)
            .arg(QString::number(flashUtilizationPercent, 'f', 1))
            .arg(targetRamCapacityBytes / 1024)
            .arg(QString::number(ramUtilizationPercent, 'f', 1));
    }
    return str;
}
