#pragma once
#include "IMcuDriverPack.h"

namespace driverpack {

class Rp2040DriverPack : public IMcuDriverPack {
public:
    Rp2040DriverPack();

    std::string familyId() const override { return "RP2040"; }
    std::string familyName() const override { return "Raspberry Pi RP2040"; }
    std::string vendor() const override { return "Raspberry Pi"; }

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
    DriverDependencyResolver resolver;

    void initializeDevices();
};

} // namespace driverpack
