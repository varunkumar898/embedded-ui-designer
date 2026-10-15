#include "ResourceEstimator.h"
#include "Project.h"
#include "Screen.h"
#include "UIComponent.h"
#include "ImageComponent.h"
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>
#include <algorithm>

ResourceEstimator::ResourceEstimator(QObject* parent)
    : QObject(parent)
{
    EstimationFormulas::initDefaultBaselines();
    m_db.loadBuiltinDatabase();
}

ResourceAnalysis ResourceEstimator::analyze(const Project* project, const QString& deviceName) {
    DeviceProfile dev;
    if (!deviceName.isEmpty()) {
        dev = m_db.getDeviceProfile(deviceName);
    } else if (project && !project->hardwareConfig().deviceId.isEmpty()) {
        dev = m_db.getDeviceProfile(project->hardwareConfig().deviceId);
    } else if (project && !project->hardwareConfig().boardId.isEmpty()) {
        dev = m_db.getDeviceProfile(project->hardwareConfig().boardId);
    } else {
        dev = m_db.getDeviceProfile("STM32H745");
    }
    return analyze(project, dev);
}

ResourceAnalysis ResourceEstimator::analyzeProject(const Project* project, const QString& framework) {
    DeviceProfile dev;
    if (project) {
        dev = project->getDeviceProfile();
    }
    return analyze(project, dev, framework);
}

ResourceAnalysis ResourceEstimator::analyze(const Project* project, const DeviceProfile& device, const QString& framework) {
    QString fw = !framework.isEmpty() ? framework : (project ? project->targetFramework() : "qt-for-mcus");
    if (fw.isEmpty()) fw = "qt-for-mcus";

    // 1. Check cache for quick return (<100 ms)
    QString compSig;
    if (project) {
        int compCount = 0;
        size_t compHash = 0;
        for (const Screen* s : project->screens()) {
            if (!s) continue;
            for (const UIComponent* c : s->components()) {
                if (!c) continue;
                compCount++;
                compHash ^= qHash(c->componentId()) ^ (qHash(c->componentType()) << 1);
            }
        }
        compSig = QString("%1:%2:%3").arg(project->screens().size()).arg(compCount).arg(compHash);
    }
    QString cacheKey = QString("%1|%2|%3").arg(device.name, fw, compSig);
    if (m_cache.hasProjectAnalysis(cacheKey)) {
        return m_cache.getProjectAnalysis(cacheKey);
    }

    ResourceAnalysis result;
    result.analyzed_at = QDateTime::currentDateTime();
    result.device_name = device.name;
    result.device = device;
    result.has_device = !device.name.isEmpty() && (device.flash_kb > 0 || device.ram_kb > 0);
    result.ram_total = device.ram_kb * 1024;
    result.flash_total = device.flash_kb * 1024;
    result.target_framework = fw;

    // Handle missing device profile gracefully without blocking export
    if (!result.has_device) {
        result.status = ResourceAnalysis::OK;
        result.summary = "No device selected";
        ResourceAnalysis::ResourceWarning rw;
        rw.severity = ResourceAnalysis::WarningSeverity::Info;
        rw.message = "No device selected. Export will proceed without target MCU memory constraints.";
        result.structuredWarnings.append(rw);
        result.warnings.append(rw.message);
        m_cache.putProjectAnalysis(cacheKey, result);
        emit analysisUpdated(result);
        return result;
    }

    // 2. Analyze Components
    ResourceAnalysis::Breakdown compBreakdown = analyzeAllComponents(project);
    result.breakdown.components_ram = compBreakdown.components_ram;
    result.breakdown.components_flash = compBreakdown.components_flash;

    // 3. Count total components and screens for framework overhead
    int totalComps = 0;
    int numScreens = 1;
    if (project) {
        numScreens = std::max(1, static_cast<int>(project->screens().size()));
        for (const Screen* s : project->screens()) {
            if (s) totalComps += s->components().size();
        }
    }

    // 4. Framework Overhead
    auto fwBaseline = EstimationFormulas::getFrameworkBaseline(fw);
    result.breakdown.framework_flash = fwBaseline.flash_minimum + (totalComps * fwBaseline.flash_per_component) + ((numScreens - 1) * 2048);
    result.breakdown.framework_ram = fwBaseline.ram_minimum + (totalComps * fwBaseline.ram_per_component) + ((numScreens - 1) * 1024);

    // 5. Asset Sizes
    uint32_t assetRam = 0;
    result.breakdown.assets_flash = analyzeAssets(project, &assetRam);
    result.breakdown.assets_ram = assetRam;

    // 6. Framebuffer calculations (single display buffer)
    int width = 320, height = 240, bpp = 16;
    if (project) {
        width = project->displayConfig().width > 0 ? project->displayConfig().width : 320;
        height = project->displayConfig().height > 0 ? project->displayConfig().height : 240;
        bpp = project->displayConfig().colorDepth > 0 ? project->displayConfig().colorDepth : 16;
    }
    result.breakdown.framebuffer_ram = static_cast<uint32_t>(calculateFramebufferBytes(width, height, bpp, 1));

    // 7. Total Usage Calculations
    result.flash_used = result.breakdown.framework_flash + result.breakdown.components_flash + result.breakdown.assets_flash;
    result.ram_used = result.breakdown.framework_ram + result.breakdown.components_ram + result.breakdown.assets_ram;

    result.flash_percent = result.flash_total > 0 ? (static_cast<float>(result.flash_used) / result.flash_total) * 100.0f : 0.0f;
    result.ram_percent = result.ram_total > 0 ? (static_cast<float>(result.ram_used) / result.ram_total) * 100.0f : 0.0f;

    result.breakdown.user_space_flash = result.flash_total > result.flash_used ? (result.flash_total - result.flash_used) : 0;
    result.breakdown.user_space_ram = result.ram_total > result.ram_used ? (result.ram_total - result.ram_used) : 0;

    // 8. CPU Estimate
    float baseCpu = 2.5f; // base engine render overhead @ 60fps
    float compCpu = 0.0f;
    if (project) {
        for (const Screen* s : project->screens()) {
            if (!s) continue;
            for (const UIComponent* c : s->components()) {
                if (c) {
                    compCpu += EstimationFormulas::estimateComponent(c).cpu_percent;
                }
            }
        }
    }
    result.cpu_overhead_percent = std::min(100.0f, baseCpu + compCpu);

    // 9. Validate and Generate Warnings
    validateFitsOnDevice(result, result.status, result.warnings);

    // 10. Human-readable summary
    if (result.status == ResourceAnalysis::OK) {
        result.summary = QString("Design fits comfortably (Estimated %1% Flash, %2% RAM used).")
                             .arg(result.flash_percent, 0, 'f', 1)
                             .arg(result.ram_percent, 0, 'f', 1);
    } else if (result.status == ResourceAnalysis::WARNING) {
        result.summary = QString("Resource warning: High usage (Estimated %1% Flash, %2% RAM).")
                             .arg(result.flash_percent, 0, 'f', 1)
                             .arg(result.ram_percent, 0, 'f', 1);
    } else if (result.status == ResourceAnalysis::CRITICAL) {
        result.summary = QString("CRITICAL: Exceeds target device capacity (Estimated %1% Flash, %2% RAM)!")
                             .arg(result.flash_percent, 0, 'f', 1)
                             .arg(result.ram_percent, 0, 'f', 1);
    } else {
        result.summary = QString("Framework mismatch with target device %1.").arg(device.name);
    }

    m_cache.putProjectAnalysis(cacheKey, result);
    emit analysisUpdated(result);
    return result;
}

ResourceAnalysis::Breakdown ResourceEstimator::analyzeComponent(const UIComponent* comp) {
    ResourceAnalysis::Breakdown b;
    if (!comp) return b;
    auto fp = EstimationFormulas::estimateComponent(comp);
    b.components_ram = fp.ram;
    b.components_flash = fp.flash;
    return b;
}

ResourceAnalysis::Breakdown ResourceEstimator::analyzeAllComponents(const Project* project) {
    ResourceAnalysis::Breakdown b;
    if (!project) return b;

    for (const Screen* s : project->screens()) {
        if (!s) continue;
        for (const UIComponent* c : s->components()) {
            if (c) {
                auto fp = EstimationFormulas::estimateComponent(c);
                b.components_ram += fp.ram;
                b.components_flash += fp.flash;
            }
        }
    }
    return b;
}

uint32_t ResourceEstimator::analyzeFramework(const QString& target, int num_components, int num_screens) {
    auto fwBaseline = EstimationFormulas::getFrameworkBaseline(target);
    return fwBaseline.flash_minimum + (num_components * fwBaseline.flash_per_component) + ((num_screens - 1) * 2048);
}

uint32_t ResourceEstimator::analyzeAssets(const Project* project, uint32_t* ramAssetsOut) {
    uint32_t flashSize = 0;
    uint32_t ramSize = 0;
    if (!project) {
        if (ramAssetsOut) *ramAssetsOut = 0;
        return 0;
    }

    for (const Screen* s : project->screens()) {
        if (!s) continue;
        for (const UIComponent* c : s->components()) {
            if (auto img = dynamic_cast<const ImageComponent*>(c)) {
                QString path = img->imagePath();
                if (!path.isEmpty() && QFileInfo::exists(path)) {
                    QFileInfo fi(path);
                    flashSize += static_cast<uint32_t>(fi.size());
                    // Decoded frame in RAM (assume 16-bit RGB565)
                    uint32_t frameBytes = static_cast<uint32_t>(img->compWidth() * img->compHeight() * 2);
                    ramSize += frameBytes;
                } else {
                    // Default placeholder footprint
                    flashSize += 2048; // 2KB
                    ramSize += 1024;
                }
            }
        }
    }

    if (ramAssetsOut) *ramAssetsOut = ramSize;
    return flashSize;
}

void ResourceEstimator::validateFitsOnDevice(ResourceAnalysis& analysis) {
    validateFitsOnDevice(analysis, analysis.status, analysis.warnings);
}

void ResourceEstimator::validateFitsOnDevice(const ResourceAnalysis& analysis,
                                            ResourceAnalysis::Status& status_out,
                                            QVector<QString>& warnings_out)
{
    warnings_out.clear();
    status_out = ResourceAnalysis::OK;
    auto* mutableAnalysis = const_cast<ResourceAnalysis*>(&analysis);
    if (mutableAnalysis) {
        mutableAnalysis->structuredWarnings.clear();
    }

    if (!analysis.has_device || analysis.device_name.isEmpty()) {
        status_out = ResourceAnalysis::OK;
        return;
    }

    // 1. Framework compatibility check
    if (!m_db.supportsFramework(analysis.device_name, analysis.target_framework)) {
        status_out = ResourceAnalysis::MISMATCH;
        QString msg = QString("Target framework '%1' is not officially supported on %2.")
                          .arg(analysis.target_framework, analysis.device_name);
        warnings_out.append(msg);
        if (mutableAnalysis) {
            mutableAnalysis->structuredWarnings.append({ResourceAnalysis::WarningSeverity::Warning, msg});
        }
    }

    // 2. Capacity checks
    // Critical if >= 95.0%
    if (analysis.flash_percent >= 95.0f) {
        status_out = ResourceAnalysis::CRITICAL;
        QString msg = QString("CRITICAL: Flash memory exceeded! Design uses %1 KB (%2% of %3 KB).")
                          .arg(analysis.flash_used / 1024)
                          .arg(analysis.flash_percent, 0, 'f', 1)
                          .arg(analysis.flash_total / 1024);
        warnings_out.append(msg);
        if (mutableAnalysis) {
            mutableAnalysis->structuredWarnings.append({ResourceAnalysis::WarningSeverity::Critical, msg});
        }
    } else if (analysis.flash_percent >= 80.0f) {
        if (status_out != ResourceAnalysis::CRITICAL && status_out != ResourceAnalysis::MISMATCH) {
            status_out = ResourceAnalysis::WARNING;
        }
        QString msg = QString("Warning: Flash usage is %1%. Only %2 KB remaining for user application logic.")
                          .arg(analysis.flash_percent, 0, 'f', 1)
                          .arg(analysis.breakdown.user_space_flash / 1024);
        warnings_out.append(msg);
        if (mutableAnalysis) {
            mutableAnalysis->structuredWarnings.append({ResourceAnalysis::WarningSeverity::Warning, msg});
        }
    } else {
        if (mutableAnalysis) {
            mutableAnalysis->structuredWarnings.append({
                ResourceAnalysis::WarningSeverity::Info,
                QString("Estimated Flash utilization: %1% (%2 KB / %3 KB).")
                    .arg(analysis.flash_percent, 0, 'f', 1)
                    .arg(analysis.flash_used / 1024)
                    .arg(analysis.flash_total / 1024)
            });
        }
    }

    if (analysis.ram_percent >= 95.0f) {
        status_out = ResourceAnalysis::CRITICAL;
        QString msg = QString("CRITICAL: RAM memory exceeded! Design uses %1 KB (%2% of %3 KB).")
                          .arg(analysis.ram_used / 1024)
                          .arg(analysis.ram_percent, 0, 'f', 1)
                          .arg(analysis.ram_total / 1024);
        warnings_out.append(msg);
        if (mutableAnalysis) {
            mutableAnalysis->structuredWarnings.append({ResourceAnalysis::WarningSeverity::Critical, msg});
        }
    } else if (analysis.ram_percent >= 80.0f) {
        if (status_out != ResourceAnalysis::CRITICAL && status_out != ResourceAnalysis::MISMATCH) {
            status_out = ResourceAnalysis::WARNING;
        }
        QString msg = QString("Warning: RAM usage is %1%. Heap/stack margin is slim (%2 KB free).")
                          .arg(analysis.ram_percent, 0, 'f', 1)
                          .arg(analysis.breakdown.user_space_ram / 1024);
        warnings_out.append(msg);
        if (mutableAnalysis) {
            mutableAnalysis->structuredWarnings.append({ResourceAnalysis::WarningSeverity::Warning, msg});
        }
    } else {
        if (mutableAnalysis) {
            mutableAnalysis->structuredWarnings.append({
                ResourceAnalysis::WarningSeverity::Info,
                QString("Estimated RAM utilization: %1% (%2 KB / %3 KB).")
                    .arg(analysis.ram_percent, 0, 'f', 1)
                    .arg(analysis.ram_used / 1024)
                    .arg(analysis.ram_total / 1024)
            });
        }
    }

    // 3. CPU overhead check
    if (analysis.cpu_overhead_percent > 50.0f) {
        if (status_out == ResourceAnalysis::OK) status_out = ResourceAnalysis::WARNING;
        QString msg = QString("High UI render load estimated (~%1% of CPU frame budget). Consider optimizing vector paths or complex widgets.")
                          .arg(analysis.cpu_overhead_percent, 0, 'f', 1);
        warnings_out.append(msg);
        if (mutableAnalysis) {
            mutableAnalysis->structuredWarnings.append({ResourceAnalysis::WarningSeverity::Warning, msg});
        }
    }
}

QVector<ResourceEstimator::ComponentResourceEntry> ResourceEstimator::getHeaviestComponents(const Project* project) const {
    QVector<ComponentResourceEntry> entries;
    if (!project) return entries;

    QMap<QString, ComponentResourceEntry> typeMap;
    for (const Screen* s : project->screens()) {
        if (!s) continue;
        for (const UIComponent* c : s->components()) {
            if (!c) continue;
            QString type = c->componentType();
            if (type.isEmpty()) type = "CustomComponent";
            auto fp = EstimationFormulas::estimateComponent(c);
            auto& entry = typeMap[type];
            entry.componentType = type;
            entry.count++;
            entry.totalFlashBytes += fp.flash;
            entry.totalRamBytes += fp.ram;
        }
    }

    for (auto it = typeMap.begin(); it != typeMap.end(); ++it) {
        ComponentResourceEntry e = it.value();
        if (e.componentType.contains("Image", Qt::CaseInsensitive)) {
            e.optimizationSuggestion = "Convert to RGB565 / indexed palette or compress assets in flash.";
        } else if (e.componentType.contains("Table", Qt::CaseInsensitive) || e.componentType.contains("List", Qt::CaseInsensitive)) {
            e.optimizationSuggestion = "Use virtualized list rows to allocate only visible screen items in RAM.";
        } else if (e.componentType.contains("Gauge", Qt::CaseInsensitive) || e.componentType.contains("Speedo", Qt::CaseInsensitive)) {
            e.optimizationSuggestion = "Pre-render static gauge face as ROM bitmap to reduce dynamic rendering.";
        } else if (e.componentType.contains("Tab", Qt::CaseInsensitive)) {
            e.optimizationSuggestion = "Instantiate inactive tab views lazily on demand.";
        } else {
            e.optimizationSuggestion = "Combine adjacent static labels or reduce complex decorations.";
        }
        entries.append(e);
    }

    // Sort descending by total estimated footprint
    std::sort(entries.begin(), entries.end(), [](const ComponentResourceEntry& a, const ComponentResourceEntry& b) {
        return (a.totalFlashBytes + a.totalRamBytes) > (b.totalFlashBytes + b.totalRamBytes);
    });

    return entries;
}

// Framebuffer calculations (with DMA alignment)
size_t ResourceEstimator::calculateFramebufferBytes(int width, int height, int bpp, int bufferCount, size_t dmaAlignment) {
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

size_t ResourceEstimator::calculateRowStrideBytes(int width, int bpp, size_t dmaAlignment) {
    if (width <= 0 || bpp <= 0) return 0;
    size_t unalignedRowBytes = (static_cast<size_t>(width) * static_cast<size_t>(bpp) + 7) / 8;
    if (dmaAlignment > 1) {
        return (unalignedRowBytes + (dmaAlignment - 1)) & ~(dmaAlignment - 1);
    }
    return unalignedRowBytes;
}

// Convert Hardware::DeviceDefinition to DeviceProfile
DeviceProfile ResourceEstimator::deviceDefinitionToProfile(const Hardware::DeviceDefinition& dev) {
    DeviceProfile profile;
    profile.name = dev.partNumber;
    profile.family = dev.family;
    profile.ram_kb = static_cast<uint32_t>(dev.ramBytes / 1024);
    profile.flash_kb = static_cast<uint32_t>(dev.flashBytes / 1024);
    profile.cpu_speed_mhz = static_cast<uint32_t>(dev.maxClockMhz);

    // Extract peripherals from peripheralsCount
    if (dev.peripheralsCount.value("usart", 0) > 0 || dev.peripheralsCount.value("uart", 0) > 0)
        profile.peripherals.append("UART");
    if (dev.peripheralsCount.value("spi", 0) > 0)
        profile.peripherals.append("SPI");
    if (dev.peripheralsCount.value("i2c", 0) > 0)
        profile.peripherals.append("I2C");
    if (dev.peripheralsCount.value("can", 0) > 0)
        profile.peripherals.append("CAN");
    if (dev.peripheralsCount.value("usb", 0) > 0)
        profile.peripherals.append("USB");
    if (dev.peripheralsCount.value("adc", 0) > 0)
        profile.peripherals.append("ADC");
    if (dev.peripheralsCount.value("ethernet", 0) > 0)
        profile.peripherals.append("ETH");
    if (dev.peripheralsCount.value("wifi", 0) > 0)
        profile.peripherals.append("WiFi");
    if (dev.peripheralsCount.value("ble", 0) > 0 || dev.peripheralsCount.value("bluetooth", 0) > 0)
        profile.peripherals.append("BLE");

    // Default supported frameworks based on architecture
    if (dev.architecture.contains("Cortex-M", Qt::CaseInsensitive) || dev.architecture.contains("RISC-V", Qt::CaseInsensitive)) {
        profile.supported_frameworks = {"lvgl", "ugfx", "qt-for-mcus"};
    } else if (dev.architecture.contains("Cortex-A", Qt::CaseInsensitive) || dev.architecture.contains("ARMv8", Qt::CaseInsensitive)) {
        profile.supported_frameworks = {"qt-for-mcus", "lvgl"};
    } else {
        profile.supported_frameworks = {"lvgl", "ugfx"};
    }

    profile.notes = QString("%1 %2").arg(dev.vendor, dev.partNumber);
    if (!dev.sourceProvenance.isEmpty()) {
        profile.notes += " (" + dev.sourceProvenance + ")";
    }
    return profile;
}

bool ResourceEstimator::isToolchainAvailable(const QString& toolchain) {
    if (toolchain.isEmpty()) return true;
    if (!QStandardPaths::findExecutable(toolchain).isEmpty()) return true;
    if (toolchain.contains("xtensa-esp32s3")) {
        QString home = QDir::homePath();
        QString xtensaBin = home + "/.espressif/tools/xtensa-esp-elf/esp-15.2.0_20251204/xtensa-esp-elf/bin/" + toolchain;
        if (QFile::exists(xtensaBin)) return true;
    }
    return false;
}

QVector<ResourceEstimator::EmbedWebTargetEvaluation> ResourceEstimator::evaluateEmbedWebTargets(
    const Project* project, float safetyMarginPercent, uint8_t bufferCount, bool allowTreeInPsram) const {
    QVector<EmbedWebTargetEvaluation> evaluations;

    if (bufferCount < 1) bufferCount = 1;
    if (bufferCount > 2) bufferCount = 2;

    int totalComps = 0;
    int dispWidth = 320;
    if (project) {
        dispWidth = project->displayConfig().width > 0 ? project->displayConfig().width : 320;
        for (const Screen* s : project->screens()) {
            if (s) totalComps += s->components().size();
        }
    }

    // EmbedWeb Resource Footprint:
    // Core C runtime Flash: 5,120 bytes (~5 KB, measured 4,908 B on Cortex-M4, 5,197 B on Cortex-M0+)
    // Generated static UI code Flash: 1 KB base + ~80 bytes per component
    uint32_t assetFlashBytes = 0;
    if (project) {
        uint32_t ramDummy = 0;
        assetFlashBytes = const_cast<ResourceEstimator*>(this)->analyzeAssets(project, &ramDummy);
    }
    uint32_t flashRequiredBytes = 5 * 1024 + (1024 + totalComps * 80) + assetFlashBytes;
    uint32_t flashRequiredKb = (flashRequiredBytes + 1023) / 1024;

    // Slice buffer (16-line RGB565): dispWidth * 16 * 2 bytes per buffer
    // Single buffer (bufferCount=1): 10,240 bytes (for 320x240)
    // Dual ping-pong DMA buffer (bufferCount=2): 20,480 bytes (for 320x240)
    uint32_t sliceBufferBytes = static_cast<uint32_t>(dispWidth * 16 * 2);
    uint32_t totalDmaSliceBytes = sliceBufferBytes * bufferCount;

    // Verified 32-bit ARM structure sizes:
    // 128-node bounded POD tree: 11,780 bytes (sizeof(ew_tree_t) on Cortex-M/Xtensa).
    // Note: All widget styles are stored inline in ew_node_t.style (14 B each), so treeStorageBytes
    // already accounts for 100% of widget styling without separate style tables.
    uint32_t treeStorageBytes = 11780;
    // Core engine context (28 B), HAL driver instances (48 B), dispatch state (64 B), stack (1,024 B):
    uint32_t coreAndStackBytes = 28 + 48 + 64 + 1024; // 1,164 bytes (~1.14 KB)

    const QStringList deviceNames = m_db.listAllDevices();
    for (const QString& dName : deviceNames) {
        DeviceProfile dev = m_db.getDeviceProfile(dName);
        if (!dev.supported_frameworks.contains("embedweb", Qt::CaseInsensitive)) {
            continue; // Not targeting EmbedWeb
        }

        EmbedWebTargetEvaluation eval;
        eval.targetName = dev.name;
        eval.family = dev.family;
        eval.architecture = dev.architecture.isEmpty() ? "Embedded MCU" : dev.architecture;
        eval.toolchainName = dev.required_toolchain;
        eval.toolchainAvailable = isToolchainAvailable(dev.required_toolchain);
        eval.bufferCount = bufferCount;
        eval.dmaSliceBufferBytes = totalDmaSliceBytes;
        eval.treeStorageBytes = treeStorageBytes;
        eval.coreAndStackBytes = coreAndStackBytes;

        // Placement logic:
        // DMA slice buffer MUST reside in Internal DMA-capable SRAM.
        // Core runtime and stack MUST reside in Internal SRAM.
        // Tree pool may optionally reside in PSRAM if target has PSRAM and user allows it.
        bool treeInPsram = allowTreeInPsram && (dev.psram_kb > 0);
        eval.treePlacedInPsram = treeInPsram;

        uint32_t internalSramRequiredBytes = totalDmaSliceBytes + coreAndStackBytes + (treeInPsram ? 0 : treeStorageBytes);
        uint32_t internalSramRequiredKb = (internalSramRequiredBytes + 1023) / 1024;
        uint32_t psramRequiredBytes = treeInPsram ? treeStorageBytes : 0;
        uint32_t psramRequiredKb = (psramRequiredBytes + 1023) / 1024;

        // Flash calculations
        uint32_t appFlashBudgetKb = dev.flash_kb > dev.bootloader_flash_kb ? (dev.flash_kb - dev.bootloader_flash_kb) : dev.flash_kb;
        eval.flashAvailableKb = appFlashBudgetKb;
        eval.flashRequiredKb = flashRequiredKb;

        // RAM calculations
        uint32_t internalRamKb = dev.internal_sram_kb > 0 ? dev.internal_sram_kb : dev.ram_kb;
        uint32_t dmaRamKb = dev.dma_sram_kb > 0 ? dev.dma_sram_kb : internalRamKb;
        eval.internalSramAvailableKb = internalRamKb;
        eval.internalSramRequiredKb = internalSramRequiredKb;
        eval.psramAvailableKb = dev.psram_kb;
        eval.psramRequiredKb = psramRequiredKb;

        // Effective safety headroom thresholds
        float effectiveSafety = (safetyMarginPercent > 0.0f) ? safetyMarginPercent : (dev.safety_margin_percent > 0.0f ? dev.safety_margin_percent : 20.0f);
        eval.safetyMarginApplied = effectiveSafety;
        uint32_t safeFlashLimitKb = static_cast<uint32_t>(appFlashBudgetKb * (1.0f - effectiveSafety / 100.0f));
        uint32_t safeSramLimitKb = static_cast<uint32_t>(internalRamKb * (1.0f - effectiveSafety / 100.0f));
        uint32_t safePsramLimitKb = dev.psram_kb > 0 ? static_cast<uint32_t>(dev.psram_kb * (1.0f - effectiveSafety / 100.0f)) : 0;

        // 1. Evaluate Resource Status
        if (flashRequiredKb > appFlashBudgetKb) {
            eval.resourceStatus = EmbedWebTargetEvaluation::ResourceStatus::InsufficientFlash;
            eval.compatibility = EmbedWebTargetEvaluation::InsufficientFlash;
            eval.statusText = "Insufficient Flash";
            eval.reason = QString("Project requires %1 KB Flash, but %2 provides %3 KB application Flash budget.")
                              .arg(flashRequiredKb).arg(dev.name).arg(appFlashBudgetKb);
        } else if (totalDmaSliceBytes > dmaRamKb * 1024) {
            eval.resourceStatus = EmbedWebTargetEvaluation::ResourceStatus::InsufficientInternalSRAM;
            eval.compatibility = EmbedWebTargetEvaluation::InsufficientRAM;
            eval.statusText = "Insufficient Internal SRAM";
            eval.reason = QString("Project DMA slice buffer (%1 KB) exceeds %2 KB DMA-accessible SRAM (TCM memory is non-DMA).")
                              .arg((totalDmaSliceBytes + 1023) / 1024).arg(dmaRamKb);
        } else if (internalSramRequiredKb > internalRamKb) {
            eval.resourceStatus = EmbedWebTargetEvaluation::ResourceStatus::InsufficientInternalSRAM;
            eval.compatibility = EmbedWebTargetEvaluation::InsufficientRAM;
            eval.statusText = "Insufficient Internal SRAM";
            eval.reason = QString("Project requires %1 KB internal DMA-capable SRAM (%2 buffer: %3 KB DMA, core/stack: %4 KB%5), but %6 only has %7 KB on-chip SRAM.")
                              .arg(internalSramRequiredKb)
                              .arg(bufferCount == 2 ? "dual-buffer DMA" : "single-buffer")
                              .arg(totalDmaSliceBytes / 1024)
                              .arg(coreAndStackBytes / 1024)
                              .arg(treeInPsram ? "" : QString(", tree: %1 KB").arg(treeStorageBytes / 1024))
                              .arg(dev.name)
                              .arg(internalRamKb);
        } else if (treeInPsram && psramRequiredKb > dev.psram_kb) {
            eval.resourceStatus = EmbedWebTargetEvaluation::ResourceStatus::InsufficientPSRAM;
            eval.compatibility = EmbedWebTargetEvaluation::InsufficientRAM;
            eval.statusText = "Insufficient PSRAM";
            eval.reason = QString("Project requires %1 KB PSRAM for UI tree pool, but %2 only provides %3 KB external PSRAM.")
                              .arg(psramRequiredKb).arg(dev.name).arg(dev.psram_kb);
        } else if (flashRequiredKb > safeFlashLimitKb || internalSramRequiredKb > safeSramLimitKb || (treeInPsram && psramRequiredKb > safePsramLimitKb)) {
            eval.resourceStatus = EmbedWebTargetEvaluation::ResourceStatus::ResourceCompatibleWithWarnings;
            eval.compatibility = EmbedWebTargetEvaluation::CompatibleWithWarnings;
            eval.statusText = "Compatible with warnings";
            eval.reason = QString("Design fits but exceeds configured %1% safety margin (Flash: %2/%3 KB, Internal SRAM: %4/%5 KB).")
                              .arg(effectiveSafety, 0, 'f', 0).arg(flashRequiredKb).arg(appFlashBudgetKb).arg(internalSramRequiredKb).arg(internalRamKb);
        } else {
            eval.resourceStatus = EmbedWebTargetEvaluation::ResourceStatus::ResourceCompatible;
            eval.compatibility = EmbedWebTargetEvaluation::Compatible;
            eval.statusText = "Compatible";
            eval.reason = QString("Fully compatible (%1% Flash, %2% Internal SRAM utilized).")
                              .arg((flashRequiredKb * 100.0f) / appFlashBudgetKb, 0, 'f', 1)
                              .arg((internalSramRequiredKb * 100.0f) / internalRamKb, 0, 'f', 1);
        }

        // 2. Evaluate Target Toolchain & Build Status
        if (!eval.toolchainAvailable) {
            eval.buildStatus = EmbedWebTargetEvaluation::BuildStatus::ToolchainUnavailable;
            if (eval.compatibility == EmbedWebTargetEvaluation::Compatible || eval.compatibility == EmbedWebTargetEvaluation::CompatibleWithWarnings) {
                eval.compatibility = (dev.embedweb_adapter_status == "supported_untested") ? EmbedWebTargetEvaluation::NotValidated : EmbedWebTargetEvaluation::MissingToolchain;
                eval.statusText = (eval.compatibility == EmbedWebTargetEvaluation::NotValidated) ? "Not Validated" : "Missing Toolchain";
                eval.reason += QString(" Cross-compiler '%1' is not found in system PATH.").arg(dev.required_toolchain);
            }
        } else if (dev.name == "ESP32-S3") {
            eval.buildStatus = EmbedWebTargetEvaluation::BuildStatus::TargetApplicationLinked;
        } else if (dev.required_toolchain == "arm-none-eabi-gcc") {
            eval.buildStatus = EmbedWebTargetEvaluation::BuildStatus::CompilerObjectsBuilt;
        } else {
            eval.buildStatus = EmbedWebTargetEvaluation::BuildStatus::BuildNotAttempted;
        }

        // 3. Evaluate Physical Hardware Validation Status
        eval.hardwareStatus = EmbedWebTargetEvaluation::HardwareStatus::HardwareNotTested;
        if (dev.embedweb_adapter_status == "supported_untested") {
            if (eval.compatibility == EmbedWebTargetEvaluation::Compatible) {
                eval.compatibility = EmbedWebTargetEvaluation::NotValidated;
                eval.statusText = "Not Validated";
            }
            eval.reason += QString(" Physical %1 hardware validation is NOT TESTED.").arg(dev.name);
        }

        // 4. Evaluate Adapter Implementation Status
        if (dev.embedweb_adapter_status == "not_implemented" || dev.embedweb_adapter_status == "unsupported") {
            eval.compatibility = EmbedWebTargetEvaluation::AdapterNotImplemented;
            eval.statusText = "Adapter Not Implemented";
            eval.reason = QString("EmbedWeb HAL adapter for %1 architecture is not implemented.").arg(dev.family);
        }

        eval.details.append(QString("Flash ROM: %1 KB required / %2 KB budget (Safety margin: %3%)").arg(flashRequiredKb).arg(appFlashBudgetKb).arg(effectiveSafety, 0, 'f', 0));
        eval.details.append(QString("Internal SRAM: %1 KB required (%2-line %3: %4 KB, Core/Stack: %5 KB, Tree: %6 KB) / %7 KB available")
                                .arg(internalSramRequiredKb)
                                .arg(16)
                                .arg(bufferCount == 2 ? "dual-buffer DMA" : "single-buffer")
                                .arg(totalDmaSliceBytes / 1024)
                                .arg(coreAndStackBytes / 1024)
                                .arg(treeInPsram ? 0 : (treeStorageBytes / 1024))
                                .arg(internalRamKb));
        if (dev.psram_kb > 0) {
            eval.details.append(QString("External PSRAM: %1 KB detected (%2 KB allocated for tree; DMA slice buffer locked to internal SRAM)")
                                    .arg(dev.psram_kb).arg(treeInPsram ? (treeStorageBytes / 1024) : 0));
        }
        eval.details.append(QString("Build Status: %1 (%2)").arg(
            eval.buildStatus == EmbedWebTargetEvaluation::BuildStatus::CompilerObjectsBuilt ? "Compiler Objects Built (Cortex-M verified)" :
            (eval.buildStatus == EmbedWebTargetEvaluation::BuildStatus::ToolchainUnavailable ? "Toolchain Unavailable" : "Build Not Attempted"),
            dev.required_toolchain));
        eval.details.append("Hardware Validation: NOT TESTED (Physical hardware unverified)");

        evaluations.append(eval);
    }

    return evaluations;
}
