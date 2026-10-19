#include "FirmwareExportManager.h"
#include "project/Project.h"
#include "codegen/LvglGenerator.h"
#include "codegen/UgfxGenerator.h"
#include "codegen/QtMcuGenerator.h"
#include "codegen/BindingLayerGenerator.h"
#include "codegen/TargetHalGenerator.h"
#include "codegen/EmbedWebGenerator.h"
#include "analyzer/ResourceEstimator.h"
#include "driverpack/DriverPackRegistry.h"
#include "driverpack/CustomPackManager.h"
#include "driverpack/CustomDriverPack.h"
#include "hardware/BoardRegistry.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDateTime>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace {

bool isSafeManifestPath(const QString& relPath, const QDir& rootDir) {
    if (relPath.isEmpty()) return false;
    if (relPath.contains("..") || relPath.startsWith("/") || relPath.startsWith("\\")) {
        return false;
    }
    if (QDir::isAbsolutePath(relPath)) {
        return false;
    }
    if (!relPath.startsWith("generated/") && !relPath.startsWith("generated\\")) {
        return false;
    }

    QDir genDir(rootDir.filePath("generated"));
    QString genCanonical = genDir.canonicalPath();
    if (genCanonical.isEmpty()) {
        genCanonical = QFileInfo(genDir.absolutePath()).absoluteFilePath();
    }

    QString fullPath = rootDir.filePath(relPath);
    QFileInfo fi(fullPath);
    if (fi.exists() || fi.isSymbolicLink()) {
        if (fi.isSymbolicLink()) {
            QString target = fi.symLinkTarget();
            QFileInfo targetFi(target);
            QString targetCanonical = targetFi.canonicalFilePath();
            if (targetCanonical.isEmpty()) {
                targetCanonical = targetFi.absoluteFilePath();
            }
            if (!targetCanonical.startsWith(genCanonical + "/") && targetCanonical != genCanonical) {
                return false;
            }
        }
        QString fileCanonical = fi.canonicalFilePath();
        if (!fileCanonical.isEmpty() && !fileCanonical.startsWith(genCanonical + "/")) {
            return false;
        }
    } else {
        QString clean = QDir::cleanPath(fullPath);
        QString cleanGen = QDir::cleanPath(rootDir.filePath("generated"));
        if (!clean.startsWith(cleanGen + "/")) {
            return false;
        }
    }

    return true;
}

} // namespace

FirmwareExportManager::FirmwareExportManager() {
}

ExportResult FirmwareExportManager::exportProject(const Project* project, const ExportOptions& options) {
    ExportResult result;
    if (!project) {
        result.success = false;
        result.errorMessage = "Null project supplied for firmware export.";
        return result;
    }

    if (options.outputDirectory.isEmpty()) {
        result.success = false;
        result.errorMessage = "Export output directory cannot be empty.";
        return result;
    }

    // Shared Pre-Export Resource Validation Hook
    ResourceEstimator estimator;
    DeviceProfile dev = project->getDeviceProfile();
    if (!dev.name.isEmpty() && (dev.flash_kb > 0 || dev.ram_kb > 0)) {
        ResourceAnalysis analysis = estimator.analyze(project, dev, options.targetFramework);
        for (const auto& w : analysis.structuredWarnings) {
            result.warnings.append(w.message);
        }
        if (analysis.status == ResourceAnalysis::CRITICAL && !options.forceExport) {
            result.success = false;
            result.errorMessage = QString("Export blocked: %1").arg(analysis.summary.isEmpty() ? "Design exceeds device limits. Optimize or select a larger MCU." : analysis.summary);
            return result;
        }
    }

    // Target device and driver pack validation
    QString boardId = project->hardwareConfig().boardId.trimmed();
    QString deviceId = project->hardwareConfig().deviceId.trimmed();
    if (deviceId.isEmpty()) {
        deviceId = dev.name.trimmed();
    }
    if (deviceId.isEmpty() && !options.targetMcu.isEmpty() &&
        options.targetMcu != "stm32" && options.targetMcu != "generic" &&
        options.targetMcu != "esp32" && options.targetMcu != "pico") {
        deviceId = options.targetMcu.trimmed();
    }

    QString targetDevice;
    if (!boardId.isEmpty()) {
        BoardDef bd = BoardRegistry::instance().board(boardId);
        if (bd.isValid()) {
            targetDevice = !bd.mcu.isEmpty() ? bd.mcu : boardId;
        } else {
            targetDevice = boardId;
        }
    } else if (!deviceId.isEmpty()) {
        targetDevice = deviceId;
    }

    if (targetDevice.isEmpty()) {
        result.warnings.append(QStringLiteral("Info: No device selected; using default STM32F030R8 baseline."));
    } else {
        auto* pack = driverpack::DriverPackRegistry::instance().findPackForDevice(targetDevice.toStdString());
        if (!pack) {
            result.warnings.append(QString("No driver pack for %1; import one").arg(targetDevice));
            result.warnings.append(QString("Native HAL export blocked for unsupported device %1; UI export proceeding.").arg(targetDevice));
        } else {
            const auto* devSpec = pack->findDevice(targetDevice.toStdString());
            if (devSpec && !devSpec->driverDataVerified) {
                result.warnings.append(QString("%1: driver data unverified; native HAL export blocked. UI export proceeding.").arg(targetDevice));
            }
        }
    }

    QDir rootDir(options.outputDirectory);
    if (!rootDir.exists()) {
        if (!rootDir.mkpath(".")) {
            result.success = false;
            result.errorMessage = QString("Failed to create export output directory: %1").arg(options.outputDirectory);
            return result;
        }
    }

    // 1. Create Directory Hierarchy (generated/ and user/)
    QString dirErr;
    if (!createDirectoryStructure(rootDir, &dirErr)) {
        result.success = false;
        result.errorMessage = dirErr;
        return result;
    }

    QDir genDir(rootDir.filePath("generated"));
    QDir userDir(rootDir.filePath("user"));

    // Read existing manifest if any
    QString manifestPath = rootDir.filePath("generated/MANIFEST.json");
    QMap<QString, QString> oldManifest;
    if (QFile::exists(manifestPath)) {
        QFile mf(manifestPath);
        if (mf.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(mf.readAll());
            mf.close();
            if (doc.isObject()) {
                QJsonArray arr = doc.object().value("files").toArray();
                for (const auto& v : arr) {
                    if (v.isObject()) {
                        QString p = v.toObject().value("file").toString();
                        if (isSafeManifestPath(p, rootDir)) {
                            oldManifest[p] = v.toObject().value("sha256").toString();
                        } else {
                            result.warnings.append(QString("Rejected unsafe manifest entry: %1").arg(p));
                        }
                    }
                }
            }
        }
    }

    QString backupTimestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");

    // 2. Safely Generate User Application Files (main.c, custom_logic.c/h)
    if (!generateUserFiles(project, userDir, options, result)) {
        result.success = false;
        return result;
    }

    // 3. Generate Hardware & UI Artifacts in generated/
    if (!generateGeneratedFiles(project, rootDir, genDir, options, result, oldManifest, backupTimestamp)) {
        result.success = false;
        return result;
    }

    // 4. Generate Top-Level CMakeLists.txt
    if (options.generateCmake) {
        if (!generateRootCMake(project, rootDir, options, result)) {
            result.success = false;
            return result;
        }
    }

    // 5. Remove orphans listed in the old manifest but no longer produced (never touches user/ code)
    for (auto it = oldManifest.begin(); it != oldManifest.end(); ++it) {
        QString oldRelPath = it.key();
        if (!isSafeManifestPath(oldRelPath, rootDir)) {
            result.warnings.append(QString("Rejected unsafe manifest entry: %1").arg(oldRelPath));
            continue; // Manifest safety: only touch files inside generated/
        }
        if (oldRelPath == "generated/MANIFEST.json") {
            continue;
        }
        if (!result.generatedFiles.contains(oldRelPath)) {
            QString fullPath = rootDir.filePath(oldRelPath);
            QFileInfo fi(fullPath);
            if (fi.exists() || fi.isSymbolicLink()) {
                if (fi.isSymbolicLink()) {
                    QString target = fi.symLinkTarget();
                    QFileInfo targetFi(target);
                    QString targetCanonical = targetFi.canonicalFilePath();
                    QDir gDir(rootDir.filePath("generated"));
                    QString gCanonical = gDir.canonicalPath();
                    if (!targetCanonical.startsWith(gCanonical + "/")) {
                        result.warnings.append(QString("Skipping orphan symlink resolving outside generated/: %1").arg(oldRelPath));
                        continue;
                    }
                }
                QFile f(fullPath);
                if (f.open(QIODevice::ReadOnly)) {
                    QByteArray actualHash = QCryptographicHash::hash(f.readAll(), QCryptographicHash::Sha256).toHex();
                    f.close();
                    if (actualHash != it.value().toUtf8()) {
                        QString backupRel = QString("generated/.backup/%1/%2").arg(backupTimestamp, oldRelPath);
                        QString backupFull = rootDir.filePath(backupRel);
                        QFileInfo(backupFull).dir().mkpath(".");
                        QFile::copy(fullPath, backupFull);
                        result.warnings.append(QString("User modified orphan file %1 backed up to %2 before removal.").arg(oldRelPath, backupRel));
                    }
                }
                QFile::remove(fullPath);
                result.warnings.append(QString("Removed orphan file: %1").arg(oldRelPath));
            }
        }
    }

    // 6. Write generated/MANIFEST.json (file, SHA-256, generator version, device)
    QJsonObject manifestObj;
    manifestObj["generatorVersion"] = "1.0.0";
    manifestObj["device"] = targetDevice.isEmpty() ? "STM32F030R8" : targetDevice;

    QStringList filesToTrack;
    for (const QString& f : result.generatedFiles) {
        if (f.startsWith("generated/") && f != "generated/MANIFEST.json") {
            filesToTrack.append(f);
        }
    }
    filesToTrack.sort();
    filesToTrack.removeDuplicates();

    QJsonArray filesArr;
    for (const QString& relPath : filesToTrack) {
        QString fullPath = rootDir.filePath(relPath);
        QFile f(fullPath);
        if (f.open(QIODevice::ReadOnly)) {
            QByteArray hash = QCryptographicHash::hash(f.readAll(), QCryptographicHash::Sha256).toHex();
            f.close();
            QJsonObject item;
            item["file"] = relPath;
            item["sha256"] = QString::fromUtf8(hash);
            filesArr.append(item);
        }
    }
    manifestObj["files"] = filesArr;

    QJsonDocument doc(manifestObj);
    QFile mfOut(manifestPath);
    if (mfOut.open(QIODevice::WriteOnly | QIODevice::Text)) {
        mfOut.write(doc.toJson(QJsonDocument::Indented));
        mfOut.close();
        result.generatedFiles.append("generated/MANIFEST.json");
    }

    result.success = true;
    result.exportSummary = QString("Firmware successfully exported to: %1\n"
                                   "Generated files: %2\n"
                                   "Preserved user files: %3\n"
                                   "Warnings: %4")
                               .arg(options.outputDirectory)
                               .arg(result.generatedFiles.size())
                               .arg(result.preservedUserFiles.size())
                               .arg(result.warnings.size());

    return result;
}

bool FirmwareExportManager::createDirectoryStructure(const QDir& rootDir, QString* errorMsg) {
    QStringList subdirs = {
        "generated",
        "generated/ui",
        "generated/runtime",
        "generated/bindings",
        "generated/assets",
        "generated/target",
        "user"
    };

    for (const QString& sub : subdirs) {
        if (!rootDir.exists(sub)) {
            if (!rootDir.mkpath(sub)) {
                if (errorMsg) *errorMsg = QString("Failed to create subdirectory: %1/%2").arg(rootDir.absolutePath(), sub);
                return false;
            }
        }
    }
    return true;
}

bool FirmwareExportManager::generateUserFiles(const Project* project, const QDir& userDir, const ExportOptions& options, ExportResult& result) {
    // User file 1: main.c
    QString mainPath = userDir.filePath("main.c");
    if (QFile::exists(mainPath) && !options.overwriteUserCode) {
        result.preservedUserFiles.append("user/main.c (Preserved existing user logic)");
    } else {
        QFile mainFile(mainPath);
        if (mainFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&mainFile);
            out << generateMainEntryCode(project, options);
            mainFile.close();
            result.generatedFiles.append("user/main.c");
        } else {
            result.errorMessage = QString("Failed to write %1").arg(mainPath);
            return false;
        }
    }

    // User file 2: custom_logic.h
    QString customHPath = userDir.filePath("custom_logic.h");
    if (QFile::exists(customHPath) && !options.overwriteUserCode) {
        result.preservedUserFiles.append("user/custom_logic.h (Preserved)");
    } else {
        QFile hFile(customHPath);
        if (hFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&hFile);
            out << generateCustomLogicHeader(project);
            hFile.close();
            result.generatedFiles.append("user/custom_logic.h");
        }
    }

    // User file 3: custom_logic.c
    QString customCPath = userDir.filePath("custom_logic.c");
    if (QFile::exists(customCPath) && !options.overwriteUserCode) {
        result.preservedUserFiles.append("user/custom_logic.c (Preserved)");
    } else {
        QFile cFile(customCPath);
        if (cFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&cFile);
            out << generateCustomLogicSource(project);
            cFile.close();
            result.generatedFiles.append("user/custom_logic.c");
        }
    }

    return true;
}

bool FirmwareExportManager::writeGeneratedFile(const QString& relPath, const QString& content, const QDir& rootDir, const QMap<QString, QString>& oldManifest, const QString& backupTimestamp, ExportResult& result) {
    if (!isSafeManifestPath(relPath, rootDir)) {
        result.errorMessage = QString("Attempted unsafe write outside generated/: %1").arg(relPath);
        return false;
    }

    QString fullPath = rootDir.filePath(relPath);
    QFileInfo fi(fullPath);
    fi.dir().mkpath(".");

    if (QFile::exists(fullPath)) {
        QFile existing(fullPath);
        if (existing.open(QIODevice::ReadOnly)) {
            QByteArray actualHash = QCryptographicHash::hash(existing.readAll(), QCryptographicHash::Sha256).toHex();
            existing.close();

            bool shouldBackup = false;
            if (oldManifest.contains(relPath)) {
                if (actualHash != oldManifest.value(relPath).toUtf8()) {
                    shouldBackup = true;
                }
            } else if (!oldManifest.isEmpty()) {
                shouldBackup = true;
            }

            if (shouldBackup) {
                QString backupRel = QString("generated/.backup/%1/%2").arg(backupTimestamp, relPath);
                QString backupFull = rootDir.filePath(backupRel);
                QFileInfo(backupFull).dir().mkpath(".");
                QFile::copy(fullPath, backupFull);
                result.warnings.append(QString("User modified file %1 backed up to %2").arg(relPath, backupRel));
            }
        }
    }

    QFile outFile(fullPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.errorMessage = QString("Failed to write %1").arg(relPath);
        return false;
    }
    QTextStream out(&outFile);
    out << content;
    outFile.close();
    result.generatedFiles.append(relPath);
    return true;
}

bool FirmwareExportManager::generateGeneratedFiles(const Project* project, const QDir& rootDir, const QDir& genDir, const ExportOptions& options, ExportResult& result, const QMap<QString, QString>& oldManifest, const QString& backupTimestamp) {
    QString fw = options.targetFramework.isEmpty() ? project->targetFramework().toLower() : options.targetFramework.toLower();

    // 1. Generate UI Files (generated/ui/)
    if (fw.contains("ugfx")) {
        UgfxGenerator gen(const_cast<Project*>(project), nullptr);
        if (!writeGeneratedFile("generated/ui/gui.h", gen.generateUiHeader(), rootDir, oldManifest, backupTimestamp, result)) return false;
        if (!writeGeneratedFile("generated/ui/gui.c", gen.generateUiSource(), rootDir, oldManifest, backupTimestamp, result)) return false;
    } else if (fw.contains("qtmcu") || fw.contains("qul") || fw.contains("qt")) {
        QtMcuGenerator gen(const_cast<Project*>(project), nullptr);
        if (!writeGeneratedFile("generated/ui/MainScreen.qml", gen.generateDesignQml(), rootDir, oldManifest, backupTimestamp, result)) return false;
    } else if (fw.contains("embedweb")) {
        CodeGen::EmbedWebGenerator gen(project);
        if (!gen.generate(genDir, result.generatedFiles, &result.errorMessage)) {
            return false;
        }
    } else {
        // Default to LVGL v8 / v9
        LvglGenerator gen(const_cast<Project*>(project), nullptr);
        if (!writeGeneratedFile("generated/ui/ui.h", gen.generateUiHeader(), rootDir, oldManifest, backupTimestamp, result)) return false;
        if (!writeGeneratedFile("generated/ui/ui.c", gen.generateUiSource(), rootDir, oldManifest, backupTimestamp, result)) return false;
    }

    // 2. Generate Data Bindings (generated/bindings/)
    if (!writeGeneratedFile("generated/bindings/ui_bindings.h", CodeGen::BindingLayerGenerator::generateBindingsHeader(project), rootDir, oldManifest, backupTimestamp, result)) return false;
    if (!writeGeneratedFile("generated/bindings/ui_bindings.c", CodeGen::BindingLayerGenerator::generateBindingsSource(project), rootDir, oldManifest, backupTimestamp, result)) return false;

    // 3. Generate Hardware HAL (generated/target/)
    QString halHContent = CodeGen::TargetHalGenerator::generateHalHeader(project, options.targetMcu);
    if (!halHContent.isEmpty()) {
        if (!writeGeneratedFile("generated/target/target_hal.h", halHContent, rootDir, oldManifest, backupTimestamp, result)) return false;
    }

    QString halCContent = CodeGen::TargetHalGenerator::generateHalSource(project, options.targetMcu);
    if (!halCContent.isEmpty()) {
        if (!writeGeneratedFile("generated/target/target_hal.c", halCContent, rootDir, oldManifest, backupTimestamp, result)) return false;
    }

    const QString effectiveDev = !options.targetMcu.isEmpty() ? options.targetMcu : (project ? project->hardwareConfig().deviceId : QString());
    if (driverpack::CustomPackManager::instance().isCustomDevice(effectiveDev)) {
        driverpack::CustomPackDef customPack = driverpack::CustomPackManager::instance().getPack(effectiveDev);
        driverpack::CustomDriverPack customDriver(customPack);
        driverpack::GeneratedProject proj = customDriver.generateProjectTree(customDriver.supportedDevices().front(), {}, "Export");
        std::string regRelPath = "Drivers/Inc/" + customPack.name.toLower().toStdString() + "_registers.h";
        if (proj.hasFile(regRelPath)) {
            QString regContent = QString::fromStdString(proj.getFileContent(regRelPath));
            QString regTargetFile = QString("generated/target/%1_registers.h").arg(customPack.name.toLower());
            if (!writeGeneratedFile(regTargetFile, regContent, rootDir, oldManifest, backupTimestamp, result)) return false;
        }
    }

    // 4. Generate Assets Header (generated/assets/)
    if (!writeGeneratedFile("generated/assets/assets.h", "/* Auto-generated asset declarations */\n#pragma once\n#include <stdint.h>\n\n", rootDir, oldManifest, backupTimestamp, result)) return false;

    return true;
}

bool FirmwareExportManager::generateRootCMake(const Project* project, const QDir& rootDir, const ExportOptions& options, ExportResult& result) {
    QFile cmakeFile(rootDir.filePath("CMakeLists.txt"));
    if (!cmakeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.errorMessage = "Failed to write root CMakeLists.txt";
        return false;
    }

    QString projName = project->projectName().isEmpty() ? "EmbeddedFirmware" : project->projectName();
    projName.replace(" ", "_");

    QTextStream out(&cmakeFile);
    out << "cmake_minimum_required(VERSION 3.20)\n";
    out << "project(" << projName << " C CXX)\n\n";
    out << "set(CMAKE_C_STANDARD 11)\n";
    out << "set(CMAKE_CXX_STANDARD 17)\n\n";

    out << "# Include Generated and User Source Directories\n";
    out << "include_directories(\n";
    out << "    ${CMAKE_CURRENT_SOURCE_DIR}/user\n";
    out << "    ${CMAKE_CURRENT_SOURCE_DIR}/generated/ui\n";
    out << "    ${CMAKE_CURRENT_SOURCE_DIR}/generated/runtime\n";
    out << "    ${CMAKE_CURRENT_SOURCE_DIR}/generated/bindings\n";
    out << "    ${CMAKE_CURRENT_SOURCE_DIR}/generated/target\n";
    out << "    ${CMAKE_CURRENT_SOURCE_DIR}/generated/assets\n";
    out << ")\n\n";

    out << "file(GLOB_RECURSE GENERATED_SOURCES\n";
    out << "    \"generated/*.c\"\n";
    out << "    \"generated/*.cpp\"\n";
    out << ")\n\n";

    out << "file(GLOB USER_SOURCES\n";
    out << "    \"user/*.c\"\n";
    out << "    \"user/*.cpp\"\n";
    out << ")\n\n";

    out << "add_executable(" << projName << "\n";
    out << "    ${GENERATED_SOURCES}\n";
    out << "    ${USER_SOURCES}\n";
    out << ")\n\n";

    out << "# Target Compiler Optimizations for MCU\n";
    out << "if(CMAKE_C_COMPILER_ID MATCHES \"GNU|Clang\")\n";
    out << "    target_compile_options(" << projName << " PRIVATE -Wall -Wextra -Os -ffunction-sections -fdata-sections)\n";
    out << "    target_link_options(" << projName << " PRIVATE -Wl,--gc-sections)\n";
    out << "endif()\n";

    cmakeFile.close();
    result.generatedFiles.append("CMakeLists.txt");
    return true;
}

QString FirmwareExportManager::generateMainEntryCode(const Project* project, const ExportOptions& options) const {
    QString fw = options.targetFramework.isEmpty() ? project->targetFramework().toLower() : options.targetFramework.toLower();

    QString code;
    code += "/**\n";
    code += " * @file main.c\n";
    code += " * @brief Application Main Entry Point (User-Maintained)\n";
    code += " * Note: This file will NOT be overwritten during GUI regeneration.\n";
    code += " */\n\n";
    code += "#include <stdio.h>\n";
    code += "#include <stdbool.h>\n";
    code += "#include \"custom_logic.h\"\n";
    code += "#include \"target_hal.h\"\n";
    code += "#include \"ui_bindings.h\"\n\n";

    if (fw.contains("ugfx")) {
        code += "#include \"gui.h\"\n\n";
    } else if (fw.contains("embedweb")) {
        code += "#include \"ui_tree.h\"\n#include \"ew_hal.h\"\n\n";
    } else if (!fw.contains("qtmcu") && !fw.contains("qt")) {
        code += "#include \"ui.h\"\n\n";
    }

    code += "int main(void) {\n";
    code += "    // 1. Initialize Hardware Board & Peripherals\n";
    code += "    target_hal_init();\n\n";

    if (fw.contains("ugfx")) {
        code += "    // 2. Initialize uGFX GUI\n";
        code += "    gui_init();\n\n";
    } else if (fw.contains("embedweb")) {
        code += "    // 2. Initialize EmbedWeb Native GUI\n";
        code += "    static ew_tree_t ui_tree;\n";
        code += "    ui_tree_init(&ui_tree);\n\n";
    } else if (!fw.contains("qtmcu") && !fw.contains("qt")) {
        code += "    // 2. Initialize LVGL GUI\n";
        code += "    ui_init();\n\n";
    }

    code += "    // 3. Initialize Hardware Data Bindings\n";
    code += "    ui_bindings_init();\n\n";
    code += "    // 4. Initialize User Custom Application Logic\n";
    code += "    custom_logic_init();\n\n";
    code += "    printf(\"Embedded UI application started successfully.\\n\");\n\n";
    code += "    // Main Event Loop\n";
    code += "    while (1) {\n";
    code += "        target_hal_poll();\n";
    code += "        ui_bindings_update();\n";
    code += "        custom_logic_loop();\n";
    code += "    }\n\n";
    code += "    return 0;\n";
    code += "}\n";

    return code;
}

QString FirmwareExportManager::generateCustomLogicHeader(const Project* project) const {
    Q_UNUSED(project);
    return "/**\n"
           " * @file custom_logic.h\n"
           " * @brief User custom algorithms and hardware business logic\n"
           " */\n"
           "#pragma once\n\n"
           "#ifdef __cplusplus\n"
           "extern \"C\" {\n"
           "#endif\n\n"
           "void custom_logic_init(void);\n"
           "void custom_logic_loop(void);\n\n"
           "#ifdef __cplusplus\n"
           "}\n"
           "#endif\n";
}

QString FirmwareExportManager::generateCustomLogicSource(const Project* project) const {
    Q_UNUSED(project);
    return "/**\n"
           " * @file custom_logic.c\n"
           " * @brief User custom algorithms and hardware business logic\n"
           " */\n"
           "#include \"custom_logic.h\"\n"
           "#include <stdio.h>\n\n"
           "void custom_logic_init(void) {\n"
           "    // Place your sensor/network startup logic here\n"
           "}\n\n"
           "void custom_logic_loop(void) {\n"
           "    // Place your periodic application routines here\n"
           "}\n";
}
