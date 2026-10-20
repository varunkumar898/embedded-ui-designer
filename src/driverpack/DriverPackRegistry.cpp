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
    if (!pack) return;
    const std::string id = pack->familyId();
    const bool incomingCustom = (id.rfind("custom:", 0) == 0);
    for (auto& existing : packs) {
        if (existing && existing->familyId() == id) {
            const bool existingCustom = (existing->familyId().rfind("custom:", 0) == 0);
            if (!existingCustom) {
                // Never let a custom (or any) pack replace a built-in family pack.
                return;
            }
            if (!incomingCustom) {
                // A non-custom pack must not replace an existing custom pack either.
                return;
            }
            existing = pack; // custom replaces custom
            return;
        }
    }
    packs.push_back(pack);
}

void DriverPackRegistry::unregisterPack(const std::string& familyId) {
    packs.erase(
        std::remove_if(packs.begin(), packs.end(),
            [&familyId](const std::shared_ptr<IMcuDriverPack>& p) {
                return p && p->familyId() == familyId;
            }),
        packs.end());
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
