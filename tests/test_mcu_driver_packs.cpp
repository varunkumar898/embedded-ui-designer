#include "driverpack/McuDeviceSpec.h"
#include "driverpack/DriverDependency.h"
#include "driverpack/IMcuDriverPack.h"
#include "driverpack/DriverPackRegistry.h"
#include "driverpack/McuProjectGenerator.h"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <set>
#include <filesystem>
#include <cstdlib>

using namespace driverpack;
namespace fs = std::filesystem;

void testStm32ProjectTreeGeneration() {
    DriverPackRegistry& reg = DriverPackRegistry::instance();
    IMcuDriverPack* pack = reg.findPackForDevice("STM32F030R8");
    assert(pack != nullptr);

    const McuDeviceSpec* dev = pack->findDevice("STM32F030R8");
    assert(dev != nullptr);
    assert(dev->flashBytes == 64 * 1024);
    assert(dev->ramBytes == 8 * 1024);
    assert(dev->maxClockHz == 48000000);

    std::set<std::string> features = {
        "RCC", "NVIC", "SYSCFG", "EXTI", "GPIO", "USART", "I2C", "SPI", "ADC", "TIMER", "DMA"
    };

    std::string err;
    assert(pack->validateHardwareRequirements(*dev, features, err) == true);

    GeneratedProject proj = pack->generateProjectTree(*dev, features, "Stm32DashboardDemo");

    // Verify Core structure
    assert(proj.hasFile("Core/Inc/main.h"));
    assert(proj.hasFile("Core/Inc/app_config.h"));
    assert(proj.hasFile("Core/Src/main.c"));
    assert(proj.hasFile("Core/Src/app.c"));

    // Verify Device header
    assert(proj.hasFile("Drivers/Device/STM32F030x8.h"));
    std::string devHeader = proj.getFileContent("Drivers/Device/STM32F030x8.h");
    assert(devHeader.find("GPIOA_BASE") != std::string::npos);
    assert(devHeader.find("RCC_BASE") != std::string::npos);
    assert(devHeader.find("GPIO_TypeDef") != std::string::npos);

    // Verify Drivers Inc
    assert(proj.hasFile("Drivers/Inc/RCC_Driver.h"));
    assert(proj.hasFile("Drivers/Inc/NVIC_Driver.h"));
    assert(proj.hasFile("Drivers/Inc/SYSCFG_Driver.h"));
    assert(proj.hasFile("Drivers/Inc/EXTI_Driver.h"));
    assert(proj.hasFile("Drivers/Inc/GPIO_Driver.h"));
    assert(proj.hasFile("Drivers/Inc/USART_Driver.h"));
    assert(proj.hasFile("Drivers/Inc/I2C_Driver.h"));
    assert(proj.hasFile("Drivers/Inc/SPI_Driver.h"));
    assert(proj.hasFile("Drivers/Inc/ADC_Driver.h"));
    assert(proj.hasFile("Drivers/Inc/Timer_Driver.h"));
    assert(proj.hasFile("Drivers/Inc/DMA_Driver.h"));

    // Verify Drivers Src
    assert(proj.hasFile("Drivers/Src/RCC_Driver.c"));
    assert(proj.hasFile("Drivers/Src/NVIC_Driver.c"));
    assert(proj.hasFile("Drivers/Src/SYSCFG_Driver.c"));
    assert(proj.hasFile("Drivers/Src/EXTI_Driver.c"));
    assert(proj.hasFile("Drivers/Src/GPIO_Driver.c"));
    assert(proj.hasFile("Drivers/Src/USART_Driver.c"));
    assert(proj.hasFile("Drivers/Src/I2C_Driver.c"));
    assert(proj.hasFile("Drivers/Src/SPI_Driver.c"));
    assert(proj.hasFile("Drivers/Src/ADC_Driver.c"));
    assert(proj.hasFile("Drivers/Src/Timer_Driver.c"));
    assert(proj.hasFile("Drivers/Src/DMA_Driver.c"));

    // Verify Startup and Linker
    assert(proj.hasFile("Startup/startup.s"));
    assert(proj.hasFile("Linker/linker.ld"));
    std::string linker = proj.getFileContent("Linker/linker.ld");
    assert(linker.find("FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K") != std::string::npos);
    assert(linker.find("RAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 8K") != std::string::npos);

    // Verify Build files
    assert(proj.hasFile("Makefile"));
    assert(proj.hasFile("README.md"));
    assert(proj.hasFile(".gitignore"));

    std::cout << "  [PASS] STM32 Project Tree: Complete Core, Drivers, Startup, and Linker files generated\n";
}

void testCapabilityAwareHardwareValidation() {
    DriverPackRegistry& reg = DriverPackRegistry::instance();
    IMcuDriverPack* pack = reg.findPackForDevice("STM32F030R8");
    assert(pack != nullptr);

    const McuDeviceSpec* dev = pack->findDevice("STM32F030R8");
    assert(dev != nullptr);

    // Request CAN on STM32F030R8 which lacks CAN hardware peripheral
    std::set<std::string> unsupported = { "CAN" };
    std::string err;
    bool valid = pack->validateHardwareRequirements(*dev, unsupported, err);
    assert(!valid);
    assert(err.find("lacks required hardware peripheral(s): CAN") != std::string::npos);

    std::cout << "  [PASS] Capability Validation: Unsupported peripheral (CAN) rejected with descriptive error\n";
}

void testDriverDependencyResolution() {
    DriverDependencyResolver resolver;
    std::set<std::string> devPeripherals = { "RCC", "NVIC", "SYSCFG", "EXTI", "GPIO", "USART", "TIMER" };
    std::vector<std::string> missing;

    // Request only USART: should automatically pull in RCC, GPIO, and NVIC
    std::set<std::string> requested = { "USART" };
    std::set<std::string> resolved = resolver.resolveDependencies(requested, devPeripherals, missing);

    assert(missing.empty());
    assert(resolved.find("USART") != resolved.end());
    assert(resolved.find("RCC") != resolved.end());
    assert(resolved.find("GPIO") != resolved.end());
    assert(resolved.find("NVIC") != resolved.end());

    std::cout << "  [PASS] Driver Dependencies: USART automatically pulled RCC, GPIO, and NVIC prerequisites\n";
}

void testExternalWifiModuleHandling() {
    DriverPackRegistry& reg = DriverPackRegistry::instance();
    const McuDeviceSpec* dev = reg.findDeviceSpec("STM32F030R8");
    assert(dev != nullptr);

    // Verify connectivity capability is external module, not fake internal Wi-Fi
    assert(dev->connectivity == ConnectivityType::EXTERNAL_MODULE_SUPPORTED);

    GeneratedProject proj;
    std::string err;
    bool ok = reg.generateProject("STM32F030R8", { "GPIO", "WIFI" }, "Stm32WifiProject", proj, err);
    assert(ok);

    // Should include Middleware/WiFi_AT/esp_at_driver.h/.c
    assert(proj.hasFile("Middleware/WiFi_AT/esp_at_driver.h"));
    assert(proj.hasFile("Middleware/WiFi_AT/esp_at_driver.c"));

    std::cout << "  [PASS] External Wi-Fi: STM32 cleanly integrates external AT module driver over UART\n";
}

void testEsp32ProjectTreeGeneration() {
    DriverPackRegistry& reg = DriverPackRegistry::instance();
    IMcuDriverPack* pack = reg.findPackForDevice("ESP32");
    assert(pack != nullptr);

    const McuDeviceSpec* dev = pack->findDevice("ESP32");
    assert(dev != nullptr);
    assert(dev->connectivity == ConnectivityType::INTERNAL_WIFI_BLE);
    assert(dev->defaultToolchain == ToolchainType::ESP_IDF_CMAKE);

    GeneratedProject proj = pack->generateProjectTree(*dev, { "GPIO", "WIFI", "BLE" }, "Esp32Demo");

    // Verify ESP-IDF structure
    assert(proj.hasFile("CMakeLists.txt"));
    assert(proj.hasFile("main/CMakeLists.txt"));
    assert(proj.hasFile("main/main.c"));
    assert(proj.hasFile("main/app.c"));
    assert(proj.hasFile("main/app.h"));
    assert(proj.hasFile("components/drivers/CMakeLists.txt"));
    assert(proj.hasFile("components/drivers/include/gpio_driver.h"));
    assert(proj.hasFile("components/drivers/src/gpio_driver.c"));
    assert(proj.hasFile("components/drivers/include/wifi_driver.h"));
    assert(proj.hasFile("components/drivers/src/wifi_driver.c"));
    assert(proj.hasFile("components/drivers/include/ble_driver.h"));
    assert(proj.hasFile("components/drivers/src/ble_driver.c"));
    assert(proj.hasFile("sdkconfig.defaults"));
    assert(proj.hasFile("partitions.csv"));

    std::cout << "  [PASS] ESP32 Project Tree: ESP-IDF component structure with native Wi-Fi & BLE generated\n";
}

void testRp2040ProjectTreeGeneration() {
    DriverPackRegistry& reg = DriverPackRegistry::instance();
    IMcuDriverPack* pack = reg.findPackForDevice("RP2040");
    assert(pack != nullptr);

    const McuDeviceSpec* dev = pack->findDevice("RP2040");
    assert(dev != nullptr);
    assert(dev->flashBytes == 2 * 1024 * 1024);
    assert(dev->ramBytes == 264 * 1024);

    GeneratedProject proj = pack->generateProjectTree(*dev, { "GPIO", "ADC", "PWM" }, "PicoDemo");

    // Verify Pico SDK structure
    assert(proj.hasFile("CMakeLists.txt"));
    assert(proj.hasFile("pico_sdk_import.cmake"));
    assert(proj.hasFile("src/main.c"));
    assert(proj.hasFile("include/app_config.h"));

    std::string cmk = proj.getFileContent("CMakeLists.txt");
    assert(cmk.find("pico_sdk_init()") != std::string::npos);
    assert(cmk.find("hardware_gpio") != std::string::npos);

    std::cout << "  [PASS] RP2040 Project Tree: Pico SDK CMake structure with hardware headers generated\n";
}

void testNxpAndNordicProjectTreeGeneration() {
    DriverPackRegistry& reg = DriverPackRegistry::instance();

    // NXP i.MX RT1062
    IMcuDriverPack* nxpPack = reg.findPackForDevice("MIMXRT1062");
    assert(nxpPack != nullptr);
    const McuDeviceSpec* nxpDev = nxpPack->findDevice("MIMXRT1062");
    assert(nxpDev->core == "Cortex-M7");
    GeneratedProject nxpProj = nxpPack->generateProjectTree(*nxpDev, {}, "NxpDemo");
    assert(nxpProj.hasFile("source/main.c"));
    assert(nxpProj.hasFile("board/board.h"));
    assert(nxpProj.hasFile("drivers/fsl_gpio.h"));

    // Nordic nRF52840
    IMcuDriverPack* nordicPack = reg.findPackForDevice("nRF52840");
    assert(nordicPack != nullptr);
    const McuDeviceSpec* nordicDev = nordicPack->findDevice("nRF52840");
    assert(nordicDev->connectivity == ConnectivityType::INTERNAL_BLE_ONLY);
    GeneratedProject nordicProj = nordicPack->generateProjectTree(*nordicDev, {}, "NordicDemo");
    assert(nordicProj.hasFile("CMakeLists.txt"));
    assert(nordicProj.hasFile("prj.conf"));
    assert(nordicProj.hasFile("src/main.c"));

    std::cout << "  [PASS] NXP & Nordic Project Trees: Vendor-accurate SDK conventions generated\n";
}

void testFilesystemExportAndCompilation() {
    McuProjectGenerator generator;
    std::string outDir = "/tmp/embedded_ui_mcu_export_test";
    fs::remove_all(outDir);

    std::string err;
    bool ok = generator.generateAndExport(
        "STM32F030R8",
        { "RCC", "GPIO", "USART" },
        "Stm32ExportDemo",
        outDir,
        err
    );
    assert(ok);
    assert(fs::exists(outDir + "/Core/Inc/main.h"));
    assert(fs::exists(outDir + "/Drivers/Src/GPIO_Driver.c"));
    assert(fs::exists(outDir + "/Drivers/Device/STM32F030x8.h"));
    assert(fs::exists(outDir + "/Linker/linker.ld"));
    assert(fs::exists(outDir + "/Makefile"));

    // Test syntax compilation with gcc
    std::string compileCmd = "gcc -fsyntax-only -I" + outDir + "/Drivers/Device -I" + outDir + "/Drivers/Inc " + outDir + "/Drivers/Src/GPIO_Driver.c";
    int res = std::system(compileCmd.c_str());
    assert(res == 0);

    // Clean up temporary test files
    fs::remove_all(outDir);

    std::cout << "  [PASS] Filesystem Export & Compilation: Generated C drivers compile without error\n";
}

int main() {
    std::cout << "============================================================\n";
    std::cout << "  MCU-SPECIFIC DRIVER LIBRARY & PROJECT TREE TESTS (8 TESTS)\n";
    std::cout << "============================================================\n\n";

    testStm32ProjectTreeGeneration();
    testCapabilityAwareHardwareValidation();
    testDriverDependencyResolution();
    testExternalWifiModuleHandling();
    testEsp32ProjectTreeGeneration();
    testRp2040ProjectTreeGeneration();
    testNxpAndNordicProjectTreeGeneration();
    testFilesystemExportAndCompilation();

    std::cout << "\n============================================================\n";
    std::cout << "RESULTS: 8/8 MCU driver library tests passed (100%)\n";
    std::cout << "============================================================\n";

    return 0;
}
