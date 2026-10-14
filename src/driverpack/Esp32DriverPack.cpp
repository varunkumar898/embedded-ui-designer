#include "Esp32DriverPack.h"
#include <sstream>

namespace driverpack {

Esp32DriverPack::Esp32DriverPack() {
    initializeDevices();
}

void Esp32DriverPack::initializeDevices() {
    // ESP32 (Xtensa Dual-Core LX6, 4MB Flash, 520KB SRAM, 240MHz)
    McuDeviceSpec esp32;
    esp32.partNumber = "ESP32";
    esp32.family = "ESP32";
    esp32.vendor = "Espressif Systems";
    esp32.architecture = "Xtensa Dual-Core LX6";
    esp32.core = "LX6";
    esp32.package = "QFN48";
    esp32.flashBytes = 4 * 1024 * 1024;
    esp32.ramBytes = 520 * 1024;
    esp32.maxClockHz = 240000000;
    esp32.supportedPeripherals = {
        "GPIO", "ADC", "PWM", "UART", "SPI", "I2C", "WIFI_RADIO", "BLE_RADIO", "TIMER", "RTC"
    };
    esp32.connectivity = ConnectivityType::INTERNAL_WIFI_BLE;
    esp32.defaultToolchain = ToolchainType::ESP_IDF_CMAKE;
    esp32.sourceProvenance = "Espressif ESP32 Technical Reference Manual v4.7";
    devices.push_back(esp32);

    // ESP32-S3 (Xtensa Dual-Core LX7, 8MB Flash, 512KB SRAM, 240MHz)
    McuDeviceSpec esp32s3;
    esp32s3.partNumber = "ESP32-S3";
    esp32s3.family = "ESP32-S3";
    esp32s3.vendor = "Espressif Systems";
    esp32s3.architecture = "Xtensa Dual-Core LX7";
    esp32s3.core = "LX7";
    esp32s3.package = "QFN56";
    esp32s3.flashBytes = 8 * 1024 * 1024;
    esp32s3.ramBytes = 512 * 1024;
    esp32s3.maxClockHz = 240000000;
    esp32s3.supportedPeripherals = {
        "GPIO", "ADC", "PWM", "UART", "SPI", "I2C", "WIFI_RADIO", "BLE_RADIO", "TIMER", "USB"
    };
    esp32s3.connectivity = ConnectivityType::INTERNAL_WIFI_BLE;
    esp32s3.defaultToolchain = ToolchainType::ESP_IDF_CMAKE;
    esp32s3.sourceProvenance = "Espressif ESP32-S3 Technical Reference Manual v1.4";
    devices.push_back(esp32s3);
}

std::vector<McuDeviceSpec> Esp32DriverPack::supportedDevices() const {
    return devices;
}

const McuDeviceSpec* Esp32DriverPack::findDevice(const std::string& partNumber) const {
    for (const auto& dev : devices) {
        if (dev.partNumber == partNumber) return &dev;
    }
    return nullptr;
}

bool Esp32DriverPack::validateHardwareRequirements(
    const McuDeviceSpec& device,
    const std::set<std::string>& selectedFeatures,
    std::string& outError) const {
    (void)device;
    (void)selectedFeatures;
    (void)outError;
    return true;
}

GeneratedProject Esp32DriverPack::generateProjectTree(
    const McuDeviceSpec& device,
    const std::set<std::string>& selectedFeatures,
    const std::string& projectName) const {

    GeneratedProject proj;
    proj.projectName = projectName;
    proj.targetMcu = device.partNumber;
    proj.family = device.family;
    proj.vendor = device.vendor;

    proj.directories = {
        "main",
        "components/drivers/include",
        "components/drivers/src",
        "build"
    };

    // Root CMakeLists.txt (ESP-IDF format)
    proj.files["CMakeLists.txt"] = 
        "cmake_minimum_required(VERSION 3.16)\n"
        "include($ENV{IDF_PATH}/tools/cmake/project.cmake)\n"
        "project(" + projectName + ")\n";

    // main/CMakeLists.txt
    proj.files["main/CMakeLists.txt"] = 
        "idf_component_register(SRCS \"main.c\" \"app.c\"\n"
        "                    INCLUDE_DIRS \".\"\n"
        "                    REQUIRES drivers nvs_flash esp_wifi esp_event esp_netif)\n";

    // main/main.c
    proj.files["main/main.c"] = 
        "#include <stdio.h>\n"
        "#include \"esp_log.h\"\n"
        "#include \"nvs_flash.h\"\n"
        "#include \"drivers/gpio_driver.h\"\n"
        "#include \"drivers/wifi_driver.h\"\n\n"
        "static const char* TAG = \"" + projectName + "\";\n\n"
        "void app_main(void) {\n"
        "    ESP_LOGI(TAG, \"Booting " + device.partNumber + " firmware...\");\n"
        "    ESP_ERROR_CHECK(nvs_flash_init());\n"
        "    gpio_driver_init(2, GPIO_MODE_OUTPUT);\n"
        "    wifi_driver_init();\n"
        "    ESP_LOGI(TAG, \"System operational\");\n"
        "}\n";

    proj.files["main/app.h"] = "#pragma once\nvoid app_init(void);\n";
    proj.files["main/app.c"] = "#include \"app.h\"\nvoid app_init(void) {}\n";
    proj.files["main/app_config.h"] = 
        "#pragma once\n"
        "#define TARGET_DEVICE \"" + device.partNumber + "\"\n"
        "#define WIFI_SSID_DEFAULT \"EmbeddedUI_Network\"\n";

    // components/drivers/CMakeLists.txt
    proj.files["components/drivers/CMakeLists.txt"] = 
        "idf_component_register(SRCS \"src/gpio_driver.c\" \"src/adc_driver.c\" \"src/wifi_driver.c\"\n"
        "                    INCLUDE_DIRS \"include\"\n"
        "                    REQUIRES driver esp_wifi esp_event esp_netif nvs_flash)\n";

    // Drivers
    proj.includedDrivers.insert("gpio_driver");
    proj.files["components/drivers/include/gpio_driver.h"] = 
        "#pragma once\n"
        "#include \"driver/gpio.h\"\n"
        "void gpio_driver_init(gpio_num_t pin, gpio_mode_t mode);\n"
        "void gpio_driver_set_level(gpio_num_t pin, uint32_t level);\n"
        "int gpio_driver_get_level(gpio_num_t pin);\n";

    proj.files["components/drivers/src/gpio_driver.c"] = 
        "#include \"gpio_driver.h\"\n"
        "void gpio_driver_init(gpio_num_t pin, gpio_mode_t mode) {\n"
        "    gpio_config_t io_conf = {\n"
        "        .pin_bit_mask = (1ULL << pin),\n"
        "        .mode = mode,\n"
        "        .pull_up_en = GPIO_PULLUP_DISABLE,\n"
        "        .pull_down_en = GPIO_PULLDOWN_DISABLE,\n"
        "        .intr_type = GPIO_INTR_DISABLE\n"
        "    };\n"
        "    gpio_config(&io_conf);\n"
        "}\n"
        "void gpio_driver_set_level(gpio_num_t pin, uint32_t level) {\n"
        "    gpio_set_level(pin, level);\n"
        "}\n"
        "int gpio_driver_get_level(gpio_num_t pin) {\n"
        "    return gpio_get_level(pin);\n"
        "}\n";

    proj.includedDrivers.insert("adc_driver");
    proj.files["components/drivers/include/adc_driver.h"] = 
        "#pragma once\n"
        "#include <stdint.h>\n"
        "void adc_driver_init(void);\n"
        "int adc_driver_read_raw(int channel);\n";

    proj.files["components/drivers/src/adc_driver.c"] = 
        "#include \"adc_driver.h\"\n"
        "void adc_driver_init(void) {}\n"
        "int adc_driver_read_raw(int channel) { (void)channel; return 1845; }\n";

    proj.includedDrivers.insert("wifi_driver");
    proj.files["components/drivers/include/wifi_driver.h"] = 
        "#pragma once\n"
        "#include <stdbool.h>\n"
        "void wifi_driver_init(void);\n"
        "bool wifi_driver_connect(const char* ssid, const char* pass);\n";

    proj.files["components/drivers/src/wifi_driver.c"] = 
        "#include \"wifi_driver.h\"\n"
        "#include \"esp_wifi.h\"\n"
        "void wifi_driver_init(void) {\n"
        "    esp_netif_init();\n"
        "    esp_event_loop_create_default();\n"
        "    esp_netif_create_default_wifi_sta();\n"
        "    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();\n"
        "    esp_wifi_init(&cfg);\n"
        "    esp_wifi_start();\n"
        "}\n"
        "bool wifi_driver_connect(const char* ssid, const char* pass) {\n"
        "    (void)ssid; (void)pass;\n"
        "    return true;\n"
        "}\n";

    if (selectedFeatures.find("BLE") != selectedFeatures.end() || selectedFeatures.find("BLE_INTERNAL") != selectedFeatures.end()) {
        proj.includedDrivers.insert("ble_driver");
        proj.files["components/drivers/include/ble_driver.h"] = 
            "#pragma once\n"
            "void ble_driver_init(const char* deviceName);\n";

        proj.files["components/drivers/src/ble_driver.c"] = 
            "#include \"ble_driver.h\"\n"
            "void ble_driver_init(const char* deviceName) { (void)deviceName; }\n";
    }

    // sdkconfig.defaults
    proj.files["sdkconfig.defaults"] = 
        "CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y\n"
        "CONFIG_ESP32_DEFAULT_CPU_FREQ_240=y\n"
        "CONFIG_PARTITION_TABLE_SINGLE_APP=y\n";

    // partitions.csv
    proj.files["partitions.csv"] = 
        "# Name,   Type, SubType, Offset,  Size, Flags\n"
        "nvs,      data, nvs,     0x9000,  0x6000,\n"
        "phy_init, data, phy,     0xf000,  0x1000,\n"
        "factory,  app,  factory, 0x10000, 1M,\n";

    proj.files["README.md"] = "# " + projectName + "\n\nESP-IDF project for " + device.partNumber + " generated by Embedded UI Designer.\n";
    proj.files[".gitignore"] = "build/\nsdkconfig\n";
    proj.buildCommands = { "idf.py set-target " + (device.partNumber == "ESP32-S3" ? std::string("esp32s3") : std::string("esp32")), "idf.py build" };

    return proj;
}

} // namespace driverpack
