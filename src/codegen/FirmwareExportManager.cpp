#include "FirmwareExportManager.h"
#include "project/Project.h"
#include "codegen/LvglGenerator.h"
#include "codegen/UgfxGenerator.h"
#include "codegen/QtMcuGenerator.h"
#include "codegen/BindingLayerGenerator.h"
#include "codegen/TargetHalGenerator.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDateTime>

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

    // 2. Safely Generate User Application Files (main.c, custom_logic.c/h)
    if (!generateUserFiles(project, userDir, options, result)) {
        result.success = false;
        return result;
    }

    // 3. Generate Hardware & UI Artifacts in generated/
    if (!generateGeneratedFiles(project, genDir, options, result)) {
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

bool FirmwareExportManager::generateGeneratedFiles(const Project* project, const QDir& genDir, const ExportOptions& options, ExportResult& result) {
    QString fw = options.targetFramework.isEmpty() ? project->targetFramework().toLower() : options.targetFramework.toLower();

    // 1. Generate UI Files (generated/ui/)
    QDir uiDir(genDir.filePath("ui"));
    if (fw.contains("ugfx")) {
        UgfxGenerator gen(const_cast<Project*>(project), nullptr);

        QFile hFile(uiDir.filePath("gui.h"));
        if (hFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&hFile);
            out << gen.generateGuiHeader();
            hFile.close();
            result.generatedFiles.append("generated/ui/gui.h");
        }

        QFile cFile(uiDir.filePath("gui.c"));
        if (cFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&cFile);
            out << gen.generateGuiSource();
            cFile.close();
            result.generatedFiles.append("generated/ui/gui.c");
        }
    } else if (fw.contains("qtmcu") || fw.contains("qul")) {
        QtMcuGenerator gen(const_cast<Project*>(project), nullptr);

        QFile qmlFile(uiDir.filePath("MainScreen.qml"));
        if (qmlFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&qmlFile);
            out << gen.generateMainQml();
            qmlFile.close();
            result.generatedFiles.append("generated/ui/MainScreen.qml");
        }
    } else {
        // Default to LVGL v8 / v9
        LvglGenerator gen(const_cast<Project*>(project), nullptr);

        QFile hFile(uiDir.filePath("ui.h"));
        if (hFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&hFile);
            out << gen.generateUiHeader();
            hFile.close();
            result.generatedFiles.append("generated/ui/ui.h");
        }

        QFile cFile(uiDir.filePath("ui.c"));
        if (cFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&cFile);
            out << gen.generateUiSource();
            cFile.close();
            result.generatedFiles.append("generated/ui/ui.c");
        }
    }

    // 2. Generate Data Bindings (generated/bindings/)
    QDir bindingsDir(genDir.filePath("bindings"));
    QFile bindH(bindingsDir.filePath("ui_bindings.h"));
    if (bindH.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&bindH);
        out << CodeGen::BindingLayerGenerator::generateBindingsHeader(project);
        bindH.close();
        result.generatedFiles.append("generated/bindings/ui_bindings.h");
    }

    QFile bindC(bindingsDir.filePath("ui_bindings.c"));
    if (bindC.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&bindC);
        out << CodeGen::BindingLayerGenerator::generateBindingsSource(project);
        bindC.close();
        result.generatedFiles.append("generated/bindings/ui_bindings.c");
    }

    // 3. Generate Hardware HAL (generated/target/)
    QDir targetDir(genDir.filePath("target"));
    QFile halH(targetDir.filePath("target_hal.h"));
    if (halH.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&halH);
        out << CodeGen::TargetHalGenerator::generateHalHeader(project);
        halH.close();
        result.generatedFiles.append("generated/target/target_hal.h");
    }

    QFile halC(targetDir.filePath("target_hal.c"));
    if (halC.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&halC);
        out << CodeGen::TargetHalGenerator::generateHalSource(project);
        halC.close();
        result.generatedFiles.append("generated/target/target_hal.c");
    }

    // 4. Generate Assets Header (generated/assets/)
    QDir assetsDir(genDir.filePath("assets"));
    QFile assetH(assetsDir.filePath("assets.h"));
    if (assetH.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&assetH);
        out << "/* Auto-generated asset declarations */\n#pragma once\n#include <stdint.h>\n\n";
        assetH.close();
        result.generatedFiles.append("generated/assets/assets.h");
    }

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
    } else if (!fw.contains("qtmcu")) {
        code += "#include \"ui.h\"\n\n";
    }

    code += "int main(void) {\n";
    code += "    // 1. Initialize Hardware Board & Peripherals\n";
    code += "    target_hal_init();\n\n";

    if (fw.contains("ugfx")) {
        code += "    // 2. Initialize uGFX GUI\n";
        code += "    gui_init();\n\n";
    } else if (!fw.contains("qtmcu")) {
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
