#include "Rp2040DriverPack.h"
#include <sstream>
#include <algorithm>

namespace driverpack {

Rp2040DriverPack::Rp2040DriverPack() {
    initializeDevices();
}

void Rp2040DriverPack::initializeDevices() {
    McuDeviceSpec rp2040;
    rp2040.partNumber = "RP2040";
    rp2040.family = "RP2000";
    rp2040.vendor = "Raspberry Pi";
    rp2040.architecture = "ARM Cortex-M0+";
    rp2040.core = "Dual Cortex-M0+";
    rp2040.package = "QFN56";
    rp2040.flashBytes = 2 * 1024 * 1024; // 2MB typical external QSPI
    rp2040.ramBytes = 264 * 1024;        // 264KB multi-bank SRAM
    rp2040.maxClockHz = 133000000;       // 133 MHz nominal
    rp2040.supportedPeripherals = {
        "GPIO", "ADC", "PWM", "UART", "SPI", "I2C", "DMA", "PIO", "TIMER", "USB"
    };
    rp2040.connectivity = ConnectivityType::EXTERNAL_MODULE_SUPPORTED;
    rp2040.defaultToolchain = ToolchainType::PICO_SDK_CMAKE;
    rp2040.sourceProvenance = "Raspberry Pi RP2040 Datasheet Release 2.5";
    devices.push_back(rp2040);
}

std::vector<McuDeviceSpec> Rp2040DriverPack::supportedDevices() const {
    return devices;
}

const McuDeviceSpec* Rp2040DriverPack::findDevice(const std::string& partNumber) const {
    std::string upper = partNumber;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper.find("PICO 2") != std::string::npos || upper.find("PICO_2") != std::string::npos || upper.find("RP2350") != std::string::npos) {
        return nullptr; // RP2350 / Pico 2 is a separate chip
    }
    for (const auto& dev : devices) {
        std::string devUpper = dev.partNumber;
        std::transform(devUpper.begin(), devUpper.end(), devUpper.begin(), ::toupper);
        if (devUpper == upper) return &dev;
    }
    if (upper.find("RP2040") != std::string::npos || upper.find("PICO") != std::string::npos) {
        if (!devices.empty()) return &devices[0];
    }
    return nullptr;
}

bool Rp2040DriverPack::validateHardwareRequirements(
    const McuDeviceSpec& device,
    const std::set<std::string>& selectedFeatures,
    std::string& outError) const {
    (void)device;
    (void)selectedFeatures;
    (void)outError;
    return true;
}

GeneratedProject Rp2040DriverPack::generateProjectTree(
    const McuDeviceSpec& device,
    const std::set<std::string>& selectedFeatures,
    const std::string& projectName) const {

    GeneratedProject proj;
    proj.projectName = projectName;
    proj.targetMcu = device.partNumber;
    proj.family = device.family;
    proj.vendor = device.vendor;

    proj.directories = {
        "src",
        "include",
        "build"
    };

    // CMakeLists.txt
    std::stringstream cmk;
    cmk << "cmake_minimum_required(VERSION 3.13)\n"
        << "include(pico_sdk_import.cmake)\n"
        << "project(" << projectName << " C CXX ASM)\n"
        << "set(CMAKE_C_STANDARD 11)\n"
        << "set(CMAKE_CXX_STANDARD 17)\n"
        << "pico_sdk_init()\n\n"
        << "add_executable(" << projectName << "\n"
        << "    src/main.c\n"
        << "    src/app.c\n"
        << ")\n\n"
        << "target_include_directories(" << projectName << " PRIVATE include)\n"
        << "target_link_libraries(" << projectName << " pico_stdlib hardware_gpio hardware_adc hardware_pwm hardware_uart)\n"
        << "pico_add_extra_outputs(" << projectName << ")\n";
    proj.files["CMakeLists.txt"] = cmk.str();

    // pico_sdk_import.cmake
    proj.files["pico_sdk_import.cmake"] = 
        "# Pico SDK import helper script\n"
        "if (DEFINED ENV{PICO_SDK_PATH} AND (NOT PICO_SDK_PATH))\n"
        "    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})\n"
        "endif()\n";

    // src/main.c
    proj.files["src/main.c"] = 
        "#include <stdio.h>\n"
        "#include \"pico/stdlib.h\"\n"
        "#include \"hardware/gpio.h\"\n"
        "#include \"hardware/adc.h\"\n"
        "#include \"hardware/pwm.h\"\n"
        "#include \"app_config.h\"\n\n"
        "int main(void) {\n"
        "    stdio_init_all();\n"
        "    const uint LED_PIN = 25;\n"
        "    gpio_init(LED_PIN);\n"
        "    gpio_set_dir(LED_PIN, GPIO_OUT);\n"
        "    while (true) {\n"
        "        gpio_put(LED_PIN, 1);\n"
        "        sleep_ms(250);\n"
        "        gpio_put(LED_PIN, 0);\n"
        "        sleep_ms(250);\n"
        "    }\n"
        "    return 0;\n"
        "}\n";

    proj.files["src/app.c"] = "#include \"app_config.h\"\nvoid app_loop(void) {}\n";
    proj.files["include/app_config.h"] = 
        "#pragma once\n"
        "#define TARGET_MCU \"" + device.partNumber + "\"\n"
        "#define CLOCK_HZ " + std::to_string(device.maxClockHz) + "UL\n";

    proj.includedDrivers.insert("hardware_gpio");
    proj.includedDrivers.insert("hardware_adc");
    proj.includedDrivers.insert("hardware_pwm");
    proj.includedDrivers.insert("hardware_uart");

    // Wi-Fi requires a radio: RP2040 has none (Pico W / Pico 2 W carry an
    // external CYW43439 module and would need their own device entry declaring
    // a WIFI peripheral). Emitting a stub for a radio-less part is misleading.
    const bool hasRadio = device.hasPeripheral("WIFI");
    if (hasRadio && selectedFeatures.find("WIFI") != selectedFeatures.end()) {
        proj.files["include/pico_wifi.h"] = "#pragma once\nint pico_wifi_init(void);\n";
        proj.files["src/pico_wifi.c"] = "#include \"pico_wifi.h\"\nint pico_wifi_init(void) { return 0; }\n";
        proj.includedDrivers.insert("pico_wifi");
    }

    proj.files["README.md"] = "# " + projectName + "\n\nRaspberry Pi Pico SDK project for " + device.partNumber + " generated by Embedded UI Designer.\n";
    proj.files[".gitignore"] = "build/\n*.elf\n*.uf2\n*.bin\n";
    proj.buildCommands = { "mkdir -p build && cd build && cmake ..", "make -j4" };

    return proj;
}

} // namespace driverpack
