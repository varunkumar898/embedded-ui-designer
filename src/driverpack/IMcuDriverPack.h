#pragma once
#include "McuDeviceSpec.h"
#include "DriverDependency.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

namespace driverpack {

struct GeneratedProject {
    std::string projectName;
    std::string targetMcu;
    std::string family;
    std::string vendor;
    std::map<std::string, std::string> files; // relative path -> file content
    std::vector<std::string> directories;     // list of folder paths
    std::set<std::string> includedDrivers;    // e.g. {"RCC_Driver", "GPIO_Driver", "USART_Driver"}
    std::vector<std::string> buildCommands;   // e.g. ["make -j4", "arm-none-eabi-gcc ..."]
    bool compilationReady = true;
    std::string notes;

    bool hasFile(const std::string& relativePath) const {
        return files.find(relativePath) != files.end();
    }

    std::string getFileContent(const std::string& relativePath) const {
        auto it = files.find(relativePath);
        if (it != files.end()) return it->second;
        return "";
    }
};

class IMcuDriverPack {
public:
    virtual ~IMcuDriverPack() = default;

    virtual std::string familyId() const = 0;
    virtual std::string familyName() const = 0;
    virtual std::string vendor() const = 0;

    virtual std::vector<McuDeviceSpec> supportedDevices() const = 0;
    virtual const McuDeviceSpec* findDevice(const std::string& partNumber) const = 0;

    virtual bool validateHardwareRequirements(
        const McuDeviceSpec& device,
        const std::set<std::string>& selectedFeatures,
        std::string& outError
    ) const = 0;

    virtual GeneratedProject generateProjectTree(
        const McuDeviceSpec& device,
        const std::set<std::string>& selectedFeatures,
        const std::string& projectName
    ) const = 0;
};

} // namespace driverpack
