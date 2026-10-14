#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QJsonObject>
#include "DiagnosticsEngine.h"

class Project;

namespace CICD {

enum class Provider {
    GitHubActions,
    GitLabCI,
    Jenkins,
    AzurePipelines,
    BitbucketPipelines,
    Custom
};

QString providerToString(Provider p);
Provider stringToProvider(const QString& str);

struct CICDConfig {
    Provider provider = Provider::GitHubActions;
    QString targetMcu;           // e.g. STM32F030R8, ESP32, STM32F429
    QString boardName;           // e.g. NUCLEO-F030R8, ESP32-WROVER
    QString generator = "LVGL";  // LVGL, Qt/QUL, µGFX
    QString buildSystem = "cmake"; // cmake, platformio, idf
    QString toolchain = "arm-none-eabi-gcc";
    bool runBuild = true;
    bool runTests = true;
    bool archiveArtifacts = true;
    QString artifactPath = "build/firmware.bin";
    QString customScript;
};

class CICDGenerator {
public:
    CICDGenerator() = default;

    // Validation returning Diagnostics items (ERROR, WARNING, INFO)
    static QList<Diagnostics::DiagnosticItem> validate(const CICDConfig& config, const Project* project = nullptr);

    // Generation of pipeline definition content
    static QString generatePipeline(const CICDConfig& config, const Project* project = nullptr);

    // Default target path relative to project root
    static QString defaultPipelineFilePath(Provider provider);

    // List of supported providers
    static QStringList supportedProviders();

private:
    static QString generateGitHubActions(const CICDConfig& config);
    static QString generateGitLabCI(const CICDConfig& config);
    static QString generateJenkinsfile(const CICDConfig& config);
    static QString generateAzurePipelines(const CICDConfig& config);
    static QString generateBitbucketPipelines(const CICDConfig& config);
};

} // namespace CICD
