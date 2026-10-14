#include "DriverDependency.h"
#include <queue>
#include <algorithm>

namespace driverpack {

DriverDependencyResolver::DriverDependencyResolver() {
    // Standard system and peripheral driver dependencies
    registerDependency("USART", { "RCC", "GPIO", "NVIC" });
    registerDependency("I2C", { "RCC", "GPIO" });
    registerDependency("SPI", { "RCC", "GPIO" });
    registerDependency("ADC", { "RCC", "GPIO" });
    registerDependency("TIMER", { "RCC" });
    registerDependency("PWM", { "RCC", "GPIO", "TIMER" });
    registerDependency("DMA", { "RCC", "NVIC" });
    registerDependency("EXTI", { "SYSCFG", "NVIC", "GPIO" });
    registerDependency("CAN", { "RCC", "GPIO", "NVIC" });
    registerDependency("WIFI_EXTERNAL", { "USART", "GPIO", "TIMER" });
    registerDependency("WIFI_INTERNAL", { "WIFI_RADIO", "NETWORK_STACK" });
    registerDependency("BLE_INTERNAL", { "BLE_RADIO", "BT_STACK" });
}

void DriverDependencyResolver::registerDependency(const std::string& module, const std::vector<std::string>& dependencies) {
    dependencyGraph[module] = dependencies;
}

std::vector<std::string> DriverDependencyResolver::getDirectDependencies(const std::string& module) const {
    auto it = dependencyGraph.find(module);
    if (it != dependencyGraph.end()) {
        return it->second;
    }
    return {};
}

std::set<std::string> DriverDependencyResolver::resolveDependencies(
    const std::set<std::string>& requestedFeatures,
    const std::set<std::string>& deviceSupportedPeripherals,
    std::vector<std::string>& missingPrerequisites
) const {
    std::set<std::string> resolved;
    std::queue<std::string> toProcess;

    for (const auto& feat : requestedFeatures) {
        toProcess.push(feat);
        resolved.insert(feat);
    }

    while (!toProcess.empty()) {
        std::string current = toProcess.front();
        toProcess.pop();

        auto it = dependencyGraph.find(current);
        if (it != dependencyGraph.end()) {
            for (const auto& dep : it->second) {
                if (resolved.find(dep) == resolved.end()) {
                    resolved.insert(dep);
                    toProcess.push(dep);
                }
            }
        }
    }

    // Verify against device hardware capabilities
    for (const auto& mod : resolved) {
        // If it's a peripheral requirement that the device must physically support
        if (deviceSupportedPeripherals.find(mod) == deviceSupportedPeripherals.end()) {
            // Check if this module is a hardware peripheral expected from MCU
            if (mod == "RCC" || mod == "NVIC" || mod == "SYSCFG" || mod == "EXTI" ||
                mod == "GPIO" || mod == "USART" || mod == "I2C" || mod == "SPI" ||
                mod == "ADC" || mod == "TIMER" || mod == "PWM" || mod == "DMA" ||
                mod == "CAN" || mod == "WIFI_RADIO" || mod == "BLE_RADIO") {
                missingPrerequisites.push_back(mod);
            }
        }
    }

    return resolved;
}

} // namespace driverpack
