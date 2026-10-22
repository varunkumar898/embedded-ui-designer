#pragma once

#include "IMcuDriverPack.h"
#include "CustomPackModel.h"
#include <memory>

namespace driverpack {

class CustomDriverPack : public IMcuDriverPack {
public:
    explicit CustomDriverPack(const CustomPackDef& packDef);
    ~CustomDriverPack() override = default;

    std::string familyId() const override;
    std::string familyName() const override;
    std::string vendor() const override;

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

    // Explicit "Run compile check" action. Never runs automatically on import.
    bool runCompileCheck(GeneratedProject& project, QString* note = nullptr) const;

    const CustomPackDef& packDef() const { return m_packDef; }

private:
    CustomPackDef m_packDef;
    McuDeviceSpec m_deviceSpec;

    void initializeSpec();
};

} // namespace driverpack
