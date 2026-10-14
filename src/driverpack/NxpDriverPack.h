#pragma once
#include "IMcuDriverPack.h"

namespace driverpack {

class NxpDriverPack : public IMcuDriverPack {
public:
    NxpDriverPack();

    std::string familyId() const override { return "NXP_IMXRT"; }
    std::string familyName() const override { return "NXP i.MX RT"; }
    std::string vendor() const override { return "NXP Semiconductors"; }

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
