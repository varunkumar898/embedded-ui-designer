#include "DriverPackRegistry.h"
#include "Stm32DriverPack.h"
#include "Esp32DriverPack.h"
#include "Rp2040DriverPack.h"
#include "NxpDriverPack.h"
#include "NordicDriverPack.h"
#include <algorithm>

namespace driverpack {

DriverPackRegistry& DriverPackRegistry::instance() {
    static DriverPackRegistry reg;
    return reg;
}

DriverPackRegistry::DriverPackRegistry() {
    registerBuiltInPacks();
}

void DriverPackRegistry::registerBuiltInPacks() {
    registerPack(std::make_shared<Stm32DriverPack>());
    registerPack(std::make_shared<Esp32DriverPack>());
    registerPack(std::make_shared<Rp2040DriverPack>());
    registerPack(std::make_shared<NxpDriverPack>());
    registerPack(std::make_shared<NordicDriverPack>());
}

void DriverPackRegistry::registerPack(std::shared_ptr<IMcuDriverPack> pack) {
    if (pack) {
        packs.push_back(pack);
    }
}

IMcuDriverPack* DriverPackRegistry::findPackByFamily(const std::string& familyId) const {
    for (const auto& pack : packs) {
        if (pack->familyId() == familyId) {
            return pack.get();
        }
    }
    return nullptr;
}

IMcuDriverPack* DriverPackRegistry::findPackForDevice(const std::string& partNumber) const {
    for (const auto& pack : packs) {
        if (pack->findDevice(partNumber) != nullptr) {
            return pack.get();
        }
    }
    return nullptr;
}

const McuDeviceSpec* DriverPackRegistry::findDeviceSpec(const std::string& partNumber) const {
    for (const auto& pack : packs) {
        const auto* dev = pack->findDevice(partNumber);
        if (dev) return dev;
    }
    return nullptr;
}

std::vector<std::string> DriverPackRegistry::getAllFamilyIds() const {
    std::vector<std::string> ids;
    for (const auto& pack : packs) {
        ids.push_back(pack->familyId());
    }
    return ids;
}

std::vector<McuDeviceSpec> DriverPackRegistry::getAllSupportedDevices() const {
    std::vector<McuDeviceSpec> all;
    for (const auto& pack : packs) {
        auto devList = pack->supportedDevices();
        all.insert(all.end(), devList.begin(), devList.end());
    }
    return all;
}

bool DriverPackRegistry::generateProject(
    const std::string& partNumber,
    const std::set<std::string>& selectedFeatures,
    const std::string& projectName,
    GeneratedProject& outProject,
    std::string& outError) const {

    IMcuDriverPack* pack = findPackForDevice(partNumber);
    if (!pack) {
        outError = "No driver pack registered for MCU part number: " + partNumber;
        return false;
    }

    const McuDeviceSpec* devSpec = pack->findDevice(partNumber);
    if (!devSpec) {
        outError = "Device specification not found for: " + partNumber;
        return false;
    }

    if (!pack->validateHardwareRequirements(*devSpec, selectedFeatures, outError)) {
        return false;
    }

    outProject = pack->generateProjectTree(*devSpec, selectedFeatures, projectName);
    return true;
}

} // namespace driverpack
