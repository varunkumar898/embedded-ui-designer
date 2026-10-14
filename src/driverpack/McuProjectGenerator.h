#pragma once
#include "DriverPackRegistry.h"
#include <string>
#include <set>

namespace driverpack {

class McuProjectGenerator {
public:
    McuProjectGenerator();

    // Generate project and optionally export to disk
    bool generateProject(
        const std::string& partNumber,
        const std::set<std::string>& selectedFeatures,
        const std::string& projectName,
        GeneratedProject& outProject,
        std::string& outError
    );

    // Export generated project structure to filesystem
    bool exportToDisk(
        const GeneratedProject& project,
        const std::string& destinationDirectory,
        std::string& outError
    );

    // Generate and write directly to disk
    bool generateAndExport(
        const std::string& partNumber,
        const std::set<std::string>& selectedFeatures,
        const std::string& projectName,
        const std::string& destinationDirectory,
        std::string& outError
    );

private:
    DriverPackRegistry& registry;
};

} // namespace driverpack
