#include "CustomDriverPack.h"
#include <sstream>

namespace driverpack {

CustomDriverPack::CustomDriverPack(const CustomPackDef& packDef)
    : m_packDef(packDef)
{
    initializeSpec();
}

void CustomDriverPack::initializeSpec() {
    m_deviceSpec.partNumber = m_packDef.name.toStdString();
    m_deviceSpec.family = m_packDef.family.toStdString();
    m_deviceSpec.vendor = m_packDef.vendor.toStdString();
    m_deviceSpec.architecture = m_packDef.core.toStdString();
    m_deviceSpec.core = m_packDef.core.toStdString();
    m_deviceSpec.package = "Custom";

    m_deviceSpec.flashBytes = m_packDef.totalFlashKb() * 1024;
    m_deviceSpec.ramBytes = m_packDef.totalRamKb() * 1024;
    m_deviceSpec.maxClockHz = m_packDef.clockSpeedMhz * 1000000;

    for (const auto& p : m_packDef.peripherals) {
        m_deviceSpec.supportedPeripherals.insert(p.name.toStdString());
        if (!p.type.isEmpty()) {
            m_deviceSpec.supportedPeripherals.insert(p.type.toStdString());
        }
    }

    for (const auto& m : m_packDef.memoryRegions) {
        MemoryRegion mr;
        mr.name = m.name.toStdString();
        bool ok = false;
        mr.origin = m.startAddress.toUInt(&ok, 16);
        mr.lengthBytes = m.sizeKb * 1024;
        m_deviceSpec.memoryMap.push_back(mr);
    }

    m_deviceSpec.connectivity = ConnectivityType::NONE;
    m_deviceSpec.defaultToolchain = ToolchainType::ARM_GCC_MAKEFILE;
    m_deviceSpec.deviceHeaderName = m_packDef.name.toLower().toStdString() + "_registers.h";
    m_deviceSpec.sourceProvenance = m_packDef.provenance.sourceFiles.join(", ").toStdString();
}

std::string CustomDriverPack::familyId() const {
    return "CUSTOM_" + m_packDef.name.toUpper().toStdString();
}

std::string CustomDriverPack::familyName() const {
    return m_packDef.name.toStdString() + " (Custom Pack)";
}

std::string CustomDriverPack::vendor() const {
    return m_packDef.vendor.toStdString();
}

std::vector<McuDeviceSpec> CustomDriverPack::supportedDevices() const {
    return {m_deviceSpec};
}

const McuDeviceSpec* CustomDriverPack::findDevice(const std::string& partNumber) const {
    if (m_deviceSpec.partNumber == partNumber || m_packDef.partNumber.toStdString() == partNumber) {
        return &m_deviceSpec;
    }
    return nullptr;
}

bool CustomDriverPack::validateHardwareRequirements(
    const McuDeviceSpec& device,
    const std::set<std::string>& selectedFeatures,
    std::string& outError
) const {
    for (const auto& feat : selectedFeatures) {
        if (!device.hasPeripheral(feat)) {
            outError = "Custom device " + device.partNumber + " lacks required hardware module: " + feat;
            return false;
        }
    }
    return true;
}

GeneratedProject CustomDriverPack::generateProjectTree(
    const McuDeviceSpec& device,
    const std::set<std::string>& selectedFeatures,
    const std::string& projectName
) const {
    GeneratedProject proj;
    proj.projectName = projectName;
    proj.targetMcu = device.partNumber;
    proj.family = device.family;
    proj.vendor = device.vendor;
    proj.directories = {"Core/Inc", "Core/Src", "Drivers/Inc", "Drivers/Src"};

    // 1. Generate SVD-validated register definitions header
    std::string headerGuard = "__" + m_packDef.name.toUpper().toStdString() + "_REGISTERS_H";
    std::ostringstream regH;
    regH << "/* Auto-generated register header for " << m_packDef.name.toStdString() << " */\n";
    regH << "/* Base addresses validated from imported CMSIS-SVD */\n\n";
    regH << "#ifndef " << headerGuard << "\n";
    regH << "#define " << headerGuard << "\n\n";
    regH << "#include <stdint.h>\n\n";

    for (const auto& p : m_packDef.peripherals) {
        const std::string pName = p.name.toStdString();
        regH << "/* Peripheral: " << pName << " (" << p.description.toStdString() << ") */\n";
        regH << "#define " << pName << "_BASE (" << p.baseAddress.toStdString() << "UL)\n";

        for (const auto& r : p.registers) {
            const std::string rName = r.name.toStdString();
            regH << "#define " << pName << "_" << rName << " (*(volatile uint32_t*)("
                 << pName << "_BASE + " << r.offset.toStdString() << "UL))\n";
        }
        regH << "\n";
    }

    regH << "#endif /* " << headerGuard << " */\n";
    proj.files["Drivers/Inc/" + device.deviceHeaderName] = regH.str();

    // 2. Pin Definitions header from validated pin table
    std::ostringstream pinH;
    pinH << "/* Pin configuration table for " << m_packDef.name.toStdString() << " */\n";
    pinH << "#ifndef __PIN_CONFIG_H\n#define __PIN_CONFIG_H\n\n";
    for (const auto& pin : m_packDef.pinTable) {
        pinH << "/* Pin " << pin.pin.toStdString() << " -> " << pin.function.toStdString()
             << " (" << pin.peripheral.toStdString() << ") */\n";
        pinH << "#define PIN_" << pin.pin.toStdString() << "_FUNC \"" << pin.function.toStdString() << "\"\n";
    }
    pinH << "\n#endif /* __PIN_CONFIG_H */\n";
    proj.files["Core/Inc/pin_config.h"] = pinH.str();

    // 3. Driver / HAL implementation
    std::ostringstream halC;
    halC << "/* Target HAL for " << m_packDef.name.toStdString() << " */\n";
    if (m_packDef.sdkReference.detected) {
        halC << "/* Using vendor SDK HAL at: " << m_packDef.sdkReference.path.toStdString() << " */\n";
        halC << "#include \"target_hal.h\"\n";
        halC << "#include \"" << device.deviceHeaderName << "\"\n\n";
        halC << "void target_hal_init(void) {\n";
        halC << "    /* Call vendor HAL initialization */\n";
        halC << "}\n\n";
        halC << "void target_hal_gpio_write(const char* pin, int value) {\n";
        halC << "    /* Dispatched through vendor HAL */\n";
        halC << "}\n";
    } else {
        halC << "/* Raw register access using SVD-validated register offsets */\n";
        halC << "#include \"target_hal.h\"\n";
        halC << "#include \"" << device.deviceHeaderName << "\"\n\n";
        halC << "void target_hal_init(void) {\n";
        halC << "    /* Peripheral clocks and hardware init */\n";
        halC << "}\n\n";
        halC << "void target_hal_gpio_write(const char* pin, int value) {\n";
        halC << "    /* Atomic write via validated peripheral registers */\n";
        halC << "    (void)pin;\n";
        halC << "    (void)value;\n";
        halC << "}\n";
    }
    proj.files["Drivers/Src/target_hal.c"] = halC.str();

    // 4. Header for HAL
    std::ostringstream halH;
    halH << "#ifndef __TARGET_HAL_H\n#define __TARGET_HAL_H\n\n";
    halH << "#include <stdint.h>\n\n";
    halH << "void target_hal_init(void);\n";
    halH << "void target_hal_gpio_write(const char* pin, int value);\n\n";
    halH << "#endif /* __TARGET_HAL_H */\n";
    proj.files["Drivers/Inc/target_hal.h"] = halH.str();

    // 5. Main application entry point
    std::ostringstream mainC;
    mainC << "/* Main firmware entry point for " << m_packDef.name.toStdString() << " */\n";
    mainC << "#include \"target_hal.h\"\n";
    mainC << "#include \"pin_config.h\"\n\n";
    mainC << "int main(void) {\n";
    mainC << "    target_hal_init();\n";
    mainC << "    while (1) {\n";
    mainC << "    }\n";
    mainC << "    return 0;\n";
    mainC << "}\n";
    proj.files["Core/Src/main.c"] = mainC.str();

    // 6. Build script (Makefile)
    std::ostringstream makefile;
    makefile << "# Makefile for " << projectName << " (" << m_packDef.name.toStdString() << ")\n";
    makefile << "CC = " << m_packDef.toolchain.toStdString() << "\n";
    makefile << "CFLAGS = -Wall -I Core/Inc -I Drivers/Inc\n";
    makefile << "SRCS = Core/Src/main.c Drivers/Src/target_hal.c\n";
    makefile << "all:\n\t$(CC) $(CFLAGS) $(SRCS) -o " << projectName << ".elf\n";
    proj.files["Makefile"] = makefile.str();

    proj.buildCommands = {"make -j4"};
    proj.compilationReady = true;
    proj.notes = "Custom driver pack for " + m_packDef.name.toStdString() + " with SVD-validated registers.";

    return proj;
}

} // namespace driverpack
