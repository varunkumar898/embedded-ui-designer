#include "EmbedWebGenerator.h"
#include "ir/IRAdapter.h"
#include "runtime/ew_hydrate.h"
#include "runtime/ew_resource.h"
#include <QFile>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonDocument>
#include <QCoreApplication>

namespace CodeGen {

EmbedWebGenerator::EmbedWebGenerator(const Project* project)
    : m_project(project) {}

static bool writeTextFile(const QString& filePath, const QString& content, QStringList& files, QString* error) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) *error = QString("Failed to write %1").arg(filePath);
        return false;
    }
    QTextStream out(&file);
    out << content;
    file.close();
    files.append(filePath);
    return true;
}

bool EmbedWebGenerator::generate(const QDir& genDir, QStringList& generatedFiles, QString* outError) {
    if (!m_project) {
        if (outError) *outError = "Null project supplied to EmbedWebGenerator.";
        return false;
    }

    // 1. Convert Project to Universal IR
    IR::IRProject ir = IR::ProjectToIRAdapter::convert(*m_project);

    // 2. Validate and hydrate IR using EmbedWeb::IRHydrator
    EmbedWeb::IRHydrator hydrator;
    EmbedWeb::HydrationResult hr = hydrator.hydrate(ir);
    if (!hr.success) {
        if (outError) *outError = QString("EmbedWeb hydration failed: %1").arg(hr.errorMessages().join("; "));
        return false;
    }

    // 3. Emit ui_tree.h and ui_tree.c into generated/ui/
    QDir uiDir(genDir.filePath("ui"));
    if (!uiDir.exists()) uiDir.mkpath(".");

    QString treeH = hydrator.generateStaticTreeHeader(ir);
    if (!writeTextFile(uiDir.filePath("ui_tree.h"), treeH, generatedFiles, outError)) return false;

    QString treeC = hydrator.generateStaticTreeSource(ir, -1, outError);
    if (treeC.isEmpty()) return false;
    if (!writeTextFile(uiDir.filePath("ui_tree.c"), treeC, generatedFiles, outError)) return false;

    // 4. Emit ui_assets.h and ui_assets.c into generated/ui/
    QString assetH = hydrator.generateStaticAssetsHeader(ir);
    if (!writeTextFile(uiDir.filePath("ui_assets.h"), assetH, generatedFiles, outError)) return false;

    QString assetC = hydrator.generateStaticAssetsSource(ir);
    if (!writeTextFile(uiDir.filePath("ui_assets.c"), assetC, generatedFiles, outError)) return false;

    // 4b. Emit ui_board_config.h with display, bus, and DMA configuration
    int dispW = m_project->displayConfig().width > 0 ? m_project->displayConfig().width : 320;
    int dispH = m_project->displayConfig().height > 0 ? m_project->displayConfig().height : 240;
    QString boardCfg = QString(
        "/**\n"
        " * @file ui_board_config.h\n"
        " * @brief EmbedWeb Target Display and Hardware Configuration\n"
        " */\n"
        "#ifndef UI_BOARD_CONFIG_H\n"
        "#define UI_BOARD_CONFIG_H\n\n"
        "#include \"ew_hal.h\"\n\n"
        "#define UI_DISPLAY_WIDTH       %1\n"
        "#define UI_DISPLAY_HEIGHT      %2\n"
        "#define UI_DISPLAY_SLICE_LINES 16\n"
        "#define UI_DISPLAY_COLOR_FORMAT \"RGB565\"\n"
        "#define UI_BUS_TYPE            EW_HAL_BUS_SPI\n"
        "#define UI_TOUCH_TYPE          EW_HAL_TOUCH_NONE\n"
        "#define UI_DMA_ENABLED         1\n"
        "#define UI_DMA_INTERNAL_SRAM   1\n\n"
        "#endif /* UI_BOARD_CONFIG_H */\n"
    ).arg(dispW).arg(dispH);

    QDir targetDir(genDir.filePath("target"));
    if (!targetDir.exists()) targetDir.mkpath(".");
    if (!writeTextFile(targetDir.filePath("ui_board_config.h"), boardCfg, generatedFiles, outError)) return false;
    writeTextFile(uiDir.filePath("ui_board_config.h"), boardCfg, generatedFiles, outError);

    // 5. Copy/Emit runtime files into generated/runtime/
    QDir runtimeDir(genDir.filePath("runtime"));
    if (!runtimeDir.exists()) runtimeDir.mkpath(".");
    QDir adaptersDir(runtimeDir.filePath("adapters"));
    if (!adaptersDir.exists()) adaptersDir.mkpath(".");

    QStringList runtimeFileNames = {
        "ew_types.h", "ew_tree.h", "ew_tree.c",
        "ew_layout.h", "ew_layout.c",
        "ew_raster.h", "ew_raster.c",
        "ew_hal.h", "ew_hal.c",
        "ew_resource.h", "ew_resource.c",
        "ew_input.h", "ew_input.c"
    };

    QStringList adapterFileNames = {
        "adapters/ew_adapter_stm32.h", "adapters/ew_adapter_stm32.c",
        "adapters/ew_adapter_rp2040.h", "adapters/ew_adapter_rp2040.c",
        "adapters/ew_adapter_esp32.h", "adapters/ew_adapter_esp32.c",
        "adapters/ew_adapter_nxp.h", "adapters/ew_adapter_nxp.c",
        "adapters/ew_adapter_host.h", "adapters/ew_adapter_host.c"
    };

    QStringList candidateDirs = {
        "src/runtime",
        "../src/runtime",
        "../../src/runtime",
        QDir(QCoreApplication::applicationDirPath()).filePath("../src/runtime"),
        QDir(QCoreApplication::applicationDirPath()).filePath("../../src/runtime"),
        QDir(QCoreApplication::applicationDirPath()).filePath("../../../src/runtime")
    };
    QString resolvedRuntimeDir;
    for (const QString& cand : candidateDirs) {
        if (QDir(cand).exists("ew_types.h")) {
            resolvedRuntimeDir = cand;
            break;
        }
    }

    auto copyFiles = [&](const QStringList& fileList) {
        for (const QString& fname : fileList) {
            QString srcPath = resolvedRuntimeDir.isEmpty() ? QString("src/runtime/%1").arg(fname)
                                                           : QDir(resolvedRuntimeDir).filePath(fname);
            QString dstPath = runtimeDir.filePath(fname);
            QFile::remove(dstPath);
            if (QFile::copy(srcPath, dstPath)) {
                generatedFiles.append(dstPath);
            } else {
                QFile srcFile(srcPath);
                if (srcFile.open(QIODevice::ReadOnly)) {
                    QByteArray data = srcFile.readAll();
                    srcFile.close();
                    QFile dstFile(dstPath);
                    if (dstFile.open(QIODevice::WriteOnly)) {
                        dstFile.write(data);
                        dstFile.close();
                        generatedFiles.append(dstPath);
                    }
                }
            }
        }
    };

    copyFiles(runtimeFileNames);
    copyFiles(adapterFileNames);

    // 6. Emit Manifest and Resource Report
    QString manifest = generateManifestJson();
    if (!writeTextFile(genDir.filePath("manifest.json"), manifest, generatedFiles, outError)) return false;

    return true;
}

QString EmbedWebGenerator::generateManifestJson() const {
    int w = m_project->displayConfig().width > 0 ? m_project->displayConfig().width : 320;
    int h = m_project->displayConfig().height > 0 ? m_project->displayConfig().height : 240;
    ew_memory_summary_t mem = ew_resource_summarize(w, 16, 2, 128, 4096);

    QString devId = m_project->hardwareConfig().deviceId;
    QString fam = m_project->hardwareConfig().family;

    QJsonObject root;
    root["generator"] = "EmbedWeb Native Multi-MCU Generator";
    root["version"] = "1.5.0";
    root["target_framework"] = "embedweb";
    root["target_device"] = devId.isEmpty() ? "STM32H745" : devId;
    root["target_family"] = fam.isEmpty() ? "STM32" : fam;
    root["display_width"] = w;
    root["display_height"] = h;
    root["color_format"] = "RGB565";
    root["max_nodes"] = EW_MAX_NODES;

    QJsonObject res;
    res["slice_buffer_sram_bytes"] = (qint64)mem.slice_buffer_ram;
    res["node_pool_sram_bytes"] = (qint64)mem.tree_pool_ram;
    res["engine_stack_sram_bytes"] = (qint64)mem.engine_ram;
    res["total_runtime_sram_bytes"] = (qint64)mem.total_static_ram;
    res["sram_under_64k_budget"] = !mem.exceeds_64k_sram;
    res["font_rom_bytes"] = (qint64)mem.font_rom;
    res["image_assets_rom_bytes"] = (qint64)mem.image_assets_rom;
    res["total_flash_rom_bytes"] = (qint64)(mem.font_rom + mem.image_assets_rom);
    root["resource_summary"] = res;

    // Supported Adapters
    QJsonObject adapters;
    adapters["STM32"] = "ew_adapter_stm32 (Verified Cortex-M)";
    adapters["RP2040"] = "ew_adapter_rp2040 (Verified Cortex-M0+)";
    adapters["ESP32"] = "ew_adapter_esp32 (Prototype, NOT TESTED)";
    adapters["NXP"] = "ew_adapter_nxp (Verified Cortex-M7)";
    adapters["Host"] = "ew_adapter_host (Verified Desktop)";
    root["supported_adapters"] = adapters;

    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}


} // namespace CodeGen
