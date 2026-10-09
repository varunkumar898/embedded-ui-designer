#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QDir>
#include "DisplayConfig.h"

class Project;

struct ExportOptions {
    QString outputDirectory;
    QString targetFramework; // "lvgl_v8", "lvgl_v9", "ugfx", "qtmcu"
    QString targetMcu;       // e.g. "STM32F407VG", "ESP32-S3", "RP2040"
    bool generateCmake = true;
    bool overwriteUserCode = false; // Always defaults to FALSE to preserve user code!
    bool optimizeAssets = true;
};

struct ExportResult {
    bool success = false;
    QString errorMessage;
    QStringList generatedFiles;
    QStringList preservedUserFiles;
    QStringList warnings;
    QString exportSummary;
};

class FirmwareExportManager {
public:
    FirmwareExportManager();

    /// Performs safe firmware export with clean generated/ vs user/ separation
    ExportResult exportProject(const Project* project, const ExportOptions& options);

private:
    bool createDirectoryStructure(const QDir& rootDir, QString* errorMsg);
    bool generateUserFiles(const Project* project, const QDir& userDir, const ExportOptions& options, ExportResult& result);
    bool generateGeneratedFiles(const Project* project, const QDir& genDir, const ExportOptions& options, ExportResult& result);
    bool generateRootCMake(const Project* project, const QDir& rootDir, const ExportOptions& options, ExportResult& result);

    QString generateMainEntryCode(const Project* project, const ExportOptions& options) const;
    QString generateCustomLogicHeader(const Project* project) const;
    QString generateCustomLogicSource(const Project* project) const;
};
