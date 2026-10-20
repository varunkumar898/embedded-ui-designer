#pragma once
#include "IMcuDriverPack.h"
#include <vector>
#include <memory>
#include <string>

namespace driverpack {

class DriverPackRegistry {
public:
    static DriverPackRegistry& instance();

    // Register a driver pack (replaces an existing pack with the same
    // family id, so reload never creates duplicates).
    void registerPack(std::shared_ptr<IMcuDriverPack> pack);

    // Remove all packs with the given family id (used by uninstall/reload).
    void unregisterPack(const std::string& familyId);

    // Lookups
    IMcuDriverPack* findPackByFamily(const std::string& familyId) const;
    IMcuDriverPack* findPackForDevice(const std::string& partNumber) const;
    const McuDeviceSpec* findDeviceSpec(const std::string& partNumber) const;

    // Collections
    std::vector<std::string> getAllFamilyIds() const;
    std::vector<McuDeviceSpec> getAllSupportedDevices() const;

    // Unified Project Generation
    bool generateProject(
        const std::string& partNumber,
        const std::set<std::string>& selectedFeatures,
        const std::string& projectName,
        GeneratedProject& outProject,
        std::string& outError
    ) const;

private:
    DriverPackRegistry();
    std::vector<std::shared_ptr<IMcuDriverPack>> packs;
    void registerBuiltInPacks();
};

} // namespace driverpack
