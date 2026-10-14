#pragma once
#include "IMcuDriverPack.h"

namespace driverpack {

class Stm32DriverPack : public IMcuDriverPack {
public:
    Stm32DriverPack();

    std::string familyId() const override { return "STM32"; }
    std::string familyName() const override { return "STMicroelectronics STM32"; }
    std::string vendor() const override { return "STMicroelectronics"; }

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
