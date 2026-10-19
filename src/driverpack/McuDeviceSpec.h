#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <cstdint>

namespace driverpack {

enum class ConnectivityType {
    NONE = 0,
    INTERNAL_WIFI_BLE = 1,
    INTERNAL_BLE_ONLY = 2,
    EXTERNAL_MODULE_SUPPORTED = 3
};

enum class ToolchainType {
    ARM_GCC_MAKEFILE = 0,
    ARM_GCC_CMAKE = 1,
    ESP_IDF_CMAKE = 2,
    PICO_SDK_CMAKE = 3,
    NORDIC_NRF_CMAKE = 4,
    MCUXPRESSO_MAKEFILE = 5
};

struct MemoryRegion {
    std::string name;
    uint32_t origin;
    uint32_t lengthBytes;
};

struct McuDeviceSpec {
    std::string partNumber;         // e.g. "STM32F030R8", "ESP32", "RP2040"
    std::string family;             // e.g. "STM32F0", "ESP32", "RP2000", "i.MX RT", "nRF52"
    std::string vendor;             // e.g. "STMicroelectronics", "Espressif", "Raspberry Pi", "NXP", "Nordic"
    std::string architecture;       // e.g. "ARM Cortex-M0", "Xtensa Dual-Core", "ARM Cortex-M0+", "ARM Cortex-M7", "ARM Cortex-M4F"
    std::string core;               // e.g. "Cortex-M0", "LX6", "Cortex-M0+", "Cortex-M7", "Cortex-M4"
    std::string package;            // e.g. "LQFP64", "QFN48", "QFN56"
    
    uint32_t flashBytes = 0;        // e.g. 64KB (65536)
    uint32_t ramBytes = 0;          // e.g. 8KB (8192)
    uint32_t maxClockHz = 0;        // e.g. 48000000 (48 MHz)
    
    std::set<std::string> supportedPeripherals; // e.g. {"RCC", "NVIC", "SYSCFG", "EXTI", "GPIO", "USART", "I2C", "SPI", "ADC", "TIMER", "DMA"}
    ConnectivityType connectivity = ConnectivityType::NONE;
    ToolchainType defaultToolchain = ToolchainType::ARM_GCC_MAKEFILE;
    
    std::vector<MemoryRegion> memoryMap;
    std::map<std::string, std::string> peripheralClockBuses; // e.g. "USART1" -> "APB2", "GPIOA" -> "AHB"
    std::string deviceHeaderName;   // e.g. "STM32F030x8.h"
    std::string startupFileName;    // e.g. "startup_stm32f030x8.s"
    std::string linkerFileName;     // e.g. "STM32F030R8Tx_FLASH.ld"
    std::string sourceProvenance;   // e.g. "STMicroelectronics RM0360 Reference Manual Rev 5"

    // Native-HAL generation gate. When false, no target HAL/project code is
    // emitted for this device; an Error diagnostic ("driver data unverified")
    // is reported instead until sourced data replaces the hand-written values.
    bool driverDataVerified = true;
    std::string driverDataNote;     // human-readable reason shown in diagnostics

    bool hasPeripheral(const std::string& periph) const {
        return supportedPeripherals.find(periph) != supportedPeripherals.end();
    }
};

} // namespace driverpack
