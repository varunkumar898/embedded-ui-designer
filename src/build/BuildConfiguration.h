#pragma once

#include <QString>
#include <QStringList>

enum class ToolchainType {
    ArmGnu,
    EspIdf,
    NativeGcc,
    Msvc
};

enum class BuildType {
    Release,
    Debug,
    MinSizeRel,
    RelWithDebInfo
};

enum class OptimizationLevel {
    Os,
    O2,
    O0,
    Oz
};

struct BuildConfiguration {
    ToolchainType toolchain = ToolchainType::NativeGcc;
    BuildType buildType = BuildType::Release;
    OptimizationLevel optLevel = OptimizationLevel::Os;
    bool enableLto = true;
    int parallelJobs = 4;
    QString workingDirectory;
    QString customFlags;

    bool validate(QString* errorMessage = nullptr) const;
};
