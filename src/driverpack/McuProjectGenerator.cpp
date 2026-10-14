#include "McuProjectGenerator.h"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace driverpack {

McuProjectGenerator::McuProjectGenerator() 
    : registry(DriverPackRegistry::instance()) {}

bool McuProjectGenerator::generateProject(
    const std::string& partNumber,
    const std::set<std::string>& selectedFeatures,
    const std::string& projectName,
    GeneratedProject& outProject,
    std::string& outError) {

    return registry.generateProject(partNumber, selectedFeatures, projectName, outProject, outError);
}

bool McuProjectGenerator::exportToDisk(
    const GeneratedProject& project,
    const std::string& destinationDirectory,
    std::string& outError) {

    try {
        fs::path root(destinationDirectory);
        fs::create_directories(root);

        // Create all designated directories
        for (const auto& dir : project.directories) {
            fs::create_directories(root / dir);
        }

        // Write all project files
        for (const auto& [relPath, content] : project.files) {
            fs::path filePath = root / relPath;
            fs::create_directories(filePath.parent_path());

            std::ofstream out(filePath, std::ios::binary);
            if (!out.is_open()) {
                outError = "Failed to open destination file for writing: " + filePath.string();
                return false;
            }
            out << content;
            out.close();
        }

        return true;
    } catch (const std::exception& e) {
        outError = std::string("Filesystem exception: ") + e.what();
        return false;
    }
}

bool McuProjectGenerator::generateAndExport(
    const std::string& partNumber,
    const std::set<std::string>& selectedFeatures,
    const std::string& projectName,
    const std::string& destinationDirectory,
    std::string& outError) {

    GeneratedProject proj;
    if (!generateProject(partNumber, selectedFeatures, projectName, proj, outError)) {
        return false;
    }
    return exportToDisk(proj, destinationDirectory, outError);
}

} // namespace driverpack
