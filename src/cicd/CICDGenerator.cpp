#include "CICDGenerator.h"
#include "Project.h"

#include <QUuid>

namespace CICD {

QString providerToString(Provider p) {
    switch (p) {
        case Provider::GitHubActions: return "GitHub Actions";
        case Provider::GitLabCI: return "GitLab CI";
        case Provider::Jenkins: return "Jenkins";
        case Provider::AzurePipelines: return "Azure Pipelines";
        case Provider::BitbucketPipelines: return "Bitbucket Pipelines";
        case Provider::Custom: return "Custom";
    }
    return "GitHub Actions";
}

Provider stringToProvider(const QString& str) {
    QString s = str.toLower().trimmed();
    if (s.contains("gitlab")) return Provider::GitLabCI;
    if (s.contains("jenkins")) return Provider::Jenkins;
    if (s.contains("azure")) return Provider::AzurePipelines;
    if (s.contains("bitbucket")) return Provider::BitbucketPipelines;
    if (s.contains("custom")) return Provider::Custom;
    return Provider::GitHubActions;
}

QStringList CICDGenerator::supportedProviders() {
    return QStringList()
        << "GitHub Actions"
        << "GitLab CI"
        << "Jenkins"
        << "Azure Pipelines"
        << "Bitbucket Pipelines";
}

QString CICDGenerator::defaultPipelineFilePath(Provider provider) {
    switch (provider) {
        case Provider::GitHubActions: return ".github/workflows/embedded_pipeline.yml";
        case Provider::GitLabCI: return ".gitlab-ci.yml";
        case Provider::Jenkins: return "Jenkinsfile";
        case Provider::AzurePipelines: return "azure-pipelines.yml";
        case Provider::BitbucketPipelines: return "bitbucket-pipelines.yml";
        case Provider::Custom: return "pipeline.sh";
    }
    return ".github/workflows/embedded_pipeline.yml";
}

QList<Diagnostics::DiagnosticItem> CICDGenerator::validate(const CICDConfig& config, const Project* project) {
    QList<Diagnostics::DiagnosticItem> items;

    // 1. Target MCU validation
    if (config.targetMcu.isEmpty()) {
        Diagnostics::DiagnosticItem item;
        item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        item.category = Diagnostics::Category::CICD;
        item.severity = Diagnostics::Severity::Warning;
        item.title = "Target MCU Not Specified";
        item.message = "No specific target MCU is configured. Pipeline will use generic cross-compiler defaults.";
        item.fixSuggestion = "Select a target MCU (e.g. STM32F030R8 or ESP32) in Hardware Settings.";
        items.append(item);
    }

    // 2. Generator validation
    if (config.generator.isEmpty()) {
        Diagnostics::DiagnosticItem item;
        item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        item.category = Diagnostics::Category::CICD;
        item.severity = Diagnostics::Severity::Error;
        item.title = "Missing UI Generator";
        item.message = "UI Generator framework must be specified (e.g. LVGL, Qt/QUL, µGFX).";
        item.fixSuggestion = "Set generator to LVGL or Qt/QUL.";
        items.append(item);
    }

    // 3. Absolute path checks (Critical: never use machine-specific absolute paths!)
    if (config.artifactPath.startsWith("/") || config.artifactPath.contains(":\\")) {
        Diagnostics::DiagnosticItem item;
        item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        item.category = Diagnostics::Category::CICD;
        item.severity = Diagnostics::Severity::Error;
        item.title = "Absolute Artifact Path";
        item.message = QString("Artifact path '%1' must be relative to the repository workspace, not machine-specific.").arg(config.artifactPath);
        item.fixSuggestion = "Change to a relative path like 'build/firmware.bin'.";
        items.append(item);
    }

    // 4. Incompatible toolchain/framework warnings
    if (config.targetMcu.startsWith("ESP32", Qt::CaseInsensitive) && config.toolchain.contains("arm-none-eabi")) {
        Diagnostics::DiagnosticItem item;
        item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        item.category = Diagnostics::Category::CICD;
        item.severity = Diagnostics::Severity::Warning;
        item.title = "Toolchain Mismatch";
        item.message = "ESP32 targets require Xtensa / ESP-IDF toolchain rather than arm-none-eabi-gcc.";
        item.fixSuggestion = "Use PlatformIO or ESP-IDF build system for ESP32.";
        items.append(item);
    }

    // 5. Info diagnostic
    if (items.isEmpty()) {
        Diagnostics::DiagnosticItem item;
        item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        item.category = Diagnostics::Category::CICD;
        item.severity = Diagnostics::Severity::Info;
        item.title = "CI/CD Configuration Valid";
        item.message = QString("Configuration for %1 with target %2 is ready for generation.").arg(providerToString(config.provider), config.targetMcu);
        items.append(item);
    }

    return items;
}

QString CICDGenerator::generatePipeline(const CICDConfig& config, const Project* project) {
    switch (config.provider) {
        case Provider::GitHubActions: return generateGitHubActions(config);
        case Provider::GitLabCI: return generateGitLabCI(config);
        case Provider::Jenkins: return generateJenkinsfile(config);
        case Provider::AzurePipelines: return generateAzurePipelines(config);
        case Provider::BitbucketPipelines: return generateBitbucketPipelines(config);
        case Provider::Custom: return config.customScript.isEmpty() ? "#!/bin/bash\nset -e\necho 'Running embedded build...'\n" : config.customScript;
    }
    return generateGitHubActions(config);
}

QString CICDGenerator::generateGitHubActions(const CICDConfig& config) {
    QString yaml;
    yaml += "name: Embedded Firmware Pipeline\n\n";
    yaml += "on:\n";
    yaml += "  push:\n";
    yaml += "    branches: [ main, master, develop ]\n";
    yaml += "  pull_request:\n";
    yaml += "    branches: [ main, master ]\n\n";
    yaml += "jobs:\n";
    yaml += "  build-and-test:\n";
    yaml += "    runs-on: ubuntu-latest\n";
    yaml += "    steps:\n";
    yaml += "      - name: Checkout Repository\n";
    yaml += "        uses: actions/checkout@v4\n";
    yaml += "        with:\n";
    yaml += "          submodules: recursive\n\n";

    yaml += "      - name: Install Embedded Toolchains & Dependencies\n";
    yaml += "        run: |\n";
    yaml += "          sudo apt-get update\n";
    yaml += "          sudo apt-get install -y cmake ninja-build gcc-arm-none-eabi libnewlib-arm-none-eabi libsdl2-dev python3-pip\n";
    yaml += "          pip3 install --upgrade platformio\n\n";

    if (config.runBuild) {
        yaml += "      - name: Configure and Build Embedded Firmware\n";
        yaml += "        run: |\n";
        yaml += "          cmake -B build -G Ninja \\\n";
        yaml += QString("            -DTARGET_MCU=%1 \\\n").arg(config.targetMcu.isEmpty() ? "STM32F030R8" : config.targetMcu);
        yaml += QString("            -DUI_GENERATOR=%1 \\\n").arg(config.generator);
        yaml += "            -DCMAKE_BUILD_TYPE=Release\n";
        yaml += "          cmake --build build\n\n";
    }

    if (config.runTests) {
        yaml += "      - name: Run Simulator & Unit Tests\n";
        yaml += "        run: |\n";
        yaml += "          ctest --test-dir build --output-on-failure\n\n";
    }

    if (config.archiveArtifacts) {
        yaml += "      - name: Upload Firmware Artifacts\n";
        yaml += "        uses: actions/upload-artifact@v4\n";
        yaml += "        with:\n";
        yaml += QString("          name: firmware-%1\n").arg(config.targetMcu.isEmpty() ? "embedded" : config.targetMcu.toLower());
        yaml += QString("          path: %1\n").arg(config.artifactPath);
        yaml += "          if-no-files-found: warn\n";
    }

    return yaml;
}

QString CICDGenerator::generateGitLabCI(const CICDConfig& config) {
    QString yml;
    yml += "image: ubuntu:22.04\n\n";
    yml += "variables:\n";
    yml += "  DEBIAN_FRONTEND: noninteractive\n\n";
    yml += "stages:\n";
    if (config.runBuild) yml += "  - build\n";
    if (config.runTests) yml += "  - test\n";
    yml += "\n";

    yml += "before_script:\n";
    yml += "  - apt-get update && apt-get install -y cmake ninja-build gcc-arm-none-eabi libnewlib-arm-none-eabi libsdl2-dev python3-pip\n\n";

    if (config.runBuild) {
        yml += "build_firmware:\n";
        yml += "  stage: build\n";
        yml += "  script:\n";
        yml += QString("    - cmake -B build -G Ninja -DTARGET_MCU=%1 -DUI_GENERATOR=%2 -DCMAKE_BUILD_TYPE=Release\n")
            .arg(config.targetMcu.isEmpty() ? "STM32F030R8" : config.targetMcu, config.generator);
        yml += "    - cmake --build build\n";
        if (config.archiveArtifacts) {
            yml += "  artifacts:\n";
            yml += "    paths:\n";
            yml += QString("      - %1\n").arg(config.artifactPath);
            yml += "    expire_in: 1 week\n";
        }
        yml += "\n";
    }

    if (config.runTests) {
        yml += "run_tests:\n";
        yml += "  stage: test\n";
        yml += "  script:\n";
        yml += "    - ctest --test-dir build --output-on-failure\n\n";
    }

    return yml;
}

QString CICDGenerator::generateJenkinsfile(const CICDConfig& config) {
    QString jf;
    jf += "pipeline {\n";
    jf += "    agent any\n\n";
    jf += "    environment {\n";
    jf += QString("        TARGET_MCU = '%1'\n").arg(config.targetMcu.isEmpty() ? "STM32F030R8" : config.targetMcu);
    jf += QString("        UI_GENERATOR = '%1'\n").arg(config.generator);
    jf += "    }\n\n";
    jf += "    stages {\n";

    if (config.runBuild) {
        jf += "        stage('Build Firmware') {\n";
        jf += "            steps {\n";
        jf += "                sh '''\n";
        jf += "                    cmake -B build -G Ninja -DTARGET_MCU=${TARGET_MCU} -DUI_GENERATOR=${UI_GENERATOR} -DCMAKE_BUILD_TYPE=Release\n";
        jf += "                    cmake --build build\n";
        jf += "                '''\n";
        jf += "            }\n";
        jf += "        }\n";
    }

    if (config.runTests) {
        jf += "        stage('Run Tests') {\n";
        jf += "            steps {\n";
        jf += "                sh 'ctest --test-dir build --output-on-failure'\n";
        jf += "            }\n";
        jf += "        }\n";
    }

    jf += "    }\n";

    if (config.archiveArtifacts) {
        jf += "    post {\n";
        jf += "        always {\n";
        jf += QString("            archiveArtifacts artifacts: '%1', allowEmptyArchive: true\n").arg(config.artifactPath);
        jf += "        }\n";
        jf += "    }\n";
    }

    jf += "}\n";
    return jf;
}

QString CICDGenerator::generateAzurePipelines(const CICDConfig& config) {
    QString az;
    az += "trigger:\n";
    az += "- main\n\n";
    az += "pool:\n";
    az += "  vmImage: 'ubuntu-latest'\n\n";
    az += "steps:\n";
    az += "- script: |\n";
    az += "    sudo apt-get update\n";
    az += "    sudo apt-get install -y cmake ninja-build gcc-arm-none-eabi libnewlib-arm-none-eabi libsdl2-dev\n";
    az += "  displayName: 'Install Embedded Toolchain'\n\n";

    if (config.runBuild) {
        az += "- script: |\n";
        az += QString("    cmake -B build -G Ninja -DTARGET_MCU=%1 -DUI_GENERATOR=%2 -DCMAKE_BUILD_TYPE=Release\n")
            .arg(config.targetMcu.isEmpty() ? "STM32F030R8" : config.targetMcu, config.generator);
        az += "    cmake --build build\n";
        az += "  displayName: 'Build Embedded Target'\n\n";
    }

    if (config.runTests) {
        az += "- script: |\n";
        az += "    ctest --test-dir build --output-on-failure\n";
        az += "  displayName: 'Execute Simulator & Unit Tests'\n\n";
    }

    if (config.archiveArtifacts) {
        az += "- task: PublishBuildArtifacts@1\n";
        az += "  inputs:\n";
        az += QString("    PathtoPublish: '%1'\n").arg(config.artifactPath);
        az += "    ArtifactName: 'firmware'\n";
        az += "  displayName: 'Publish Firmware Binary'\n";
    }

    return az;
}

QString CICDGenerator::generateBitbucketPipelines(const CICDConfig& config) {
    QString bb;
    bb += "image: ubuntu:22.04\n\n";
    bb += "pipelines:\n";
    bb += "  default:\n";
    bb += "    - step:\n";
    bb += "        name: Build & Test Firmware\n";
    bb += "        script:\n";
    bb += "          - apt-get update && apt-get install -y cmake ninja-build gcc-arm-none-eabi libnewlib-arm-none-eabi libsdl2-dev\n";
    if (config.runBuild) {
        bb += QString("          - cmake -B build -G Ninja -DTARGET_MCU=%1 -DUI_GENERATOR=%2 -DCMAKE_BUILD_TYPE=Release\n")
            .arg(config.targetMcu.isEmpty() ? "STM32F030R8" : config.targetMcu, config.generator);
        bb += "          - cmake --build build\n";
    }
    if (config.runTests) {
        bb += "          - ctest --test-dir build --output-on-failure\n";
    }
    if (config.archiveArtifacts) {
        bb += "        artifacts:\n";
        bb += QString("          - %1\n").arg(config.artifactPath);
    }
    return bb;
}

} // namespace CICD
