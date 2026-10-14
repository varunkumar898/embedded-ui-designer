#pragma once
#include "IMcuDriverPack.h"

namespace driverpack {

class NordicDriverPack : public IMcuDriverPack {
public:
    NordicDriverPack();

    std::string familyId() const override { return "NORDIC_NRF52"; }
    std::string familyName() const override { return "Nordic Semiconductor nRF52"; }
    std::string vendor() const override { return "Nordic Semiconductor"; }

    std::vector<McuDeviceSpec> supportedDevices() const override;
    const McuDeviceSpec* findDevice(const std::string& partNumber) const override;

    bool validateHardwareRequirements(
        const McuDeviceSpec& device,
        const std::set<std::string>& selectedFeatures,
        std::string& outError
    ) const override;

    GeneratedProject generateProjectTree(
        const McuDeviceSpec& device,
        const std::set<std::string>& selectedFeatures,
        const std::string& projectName
    ) const override;

private:
    std::vector<McuDeviceSpec> devices;
    void initializeDevices();
};

} // namespace driverpack
