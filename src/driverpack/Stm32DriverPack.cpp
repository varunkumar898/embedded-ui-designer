#include "Stm32DriverPack.h"
#include <sstream>
#include <algorithm>

namespace driverpack {

Stm32DriverPack::Stm32DriverPack() {
    initializeDevices();
}

void Stm32DriverPack::initializeDevices() {
    // STM32F030R8 (ARM Cortex-M0, 64KB Flash, 8KB RAM, 48MHz)
    McuDeviceSpec f030;
    f030.partNumber = "STM32F030R8";
    f030.family = "STM32F0";
    f030.vendor = "STMicroelectronics";
    f030.architecture = "ARM Cortex-M0";
    f030.core = "Cortex-M0";
    f030.package = "LQFP64";
    f030.flashBytes = 64 * 1024;
    f030.ramBytes = 8 * 1024;
    f030.maxClockHz = 48000000;
    f030.supportedPeripherals = {
        "RCC", "NVIC", "SYSCFG", "EXTI", "GPIO", "USART", "I2C", "SPI", "ADC", "TIMER", "DMA"
    };
    f030.connectivity = ConnectivityType::EXTERNAL_MODULE_SUPPORTED;
    f030.defaultToolchain = ToolchainType::ARM_GCC_MAKEFILE;
    f030.memoryMap = {
        { "FLASH", 0x08000000, 64 * 1024 },
        { "RAM",   0x20000000,  8 * 1024 }
    };
    f030.peripheralClockBuses = {
        { "GPIOA", "AHB" }, { "GPIOB", "AHB" }, { "GPIOC", "AHB" },
        { "USART1", "APB2" }, { "USART2", "APB1" },
        { "I2C1", "APB1" }, { "I2C2", "APB1" },
        { "SPI1", "APB2" }, { "SPI2", "APB1" },
        { "ADC1", "APB2" }, { "TIM1", "APB2" }, { "TIM3", "APB1" }, { "DMA1", "AHB" }
    };
    f030.deviceHeaderName = "STM32F030x8.h";
    f030.startupFileName = "startup.s";
    f030.linkerFileName = "linker.ld";
    f030.sourceProvenance = "STMicroelectronics RM0360 Reference Manual Rev 5, Datasheet DS9773 Rev 4";
    devices.push_back(f030);

    // STM32F401RE (ARM Cortex-M4F, 512KB Flash, 96KB RAM, 84MHz)
    McuDeviceSpec f401;
    f401.partNumber = "STM32F401RE";
    f401.family = "STM32F4";
    f401.vendor = "STMicroelectronics";
    f401.architecture = "ARM Cortex-M4F";
    f401.core = "Cortex-M4";
    f401.package = "LQFP64";
    f401.flashBytes = 512 * 1024;
    f401.ramBytes = 96 * 1024;
    f401.maxClockHz = 84000000;
    f401.supportedPeripherals = {
        "RCC", "NVIC", "SYSCFG", "EXTI", "GPIO", "USART", "I2C", "SPI", "ADC", "TIMER", "DMA"
    };
    f401.connectivity = ConnectivityType::EXTERNAL_MODULE_SUPPORTED;
    f401.defaultToolchain = ToolchainType::ARM_GCC_MAKEFILE;
    f401.memoryMap = {
        { "FLASH", 0x08000000, 512 * 1024 },
        { "RAM",   0x20000000,  96 * 1024 }
    };
    f401.deviceHeaderName = "STM32F401xE.h";
    f401.startupFileName = "startup.s";
    f401.linkerFileName = "linker.ld";
    f401.sourceProvenance = "STMicroelectronics RM0368 Reference Manual Rev 5";
    devices.push_back(f401);
}

std::vector<McuDeviceSpec> Stm32DriverPack::supportedDevices() const {
    return devices;
}

const McuDeviceSpec* Stm32DriverPack::findDevice(const std::string& partNumber) const {
    std::string upper = partNumber;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    for (const auto& dev : devices) {
        std::string devUpper = dev.partNumber;
        std::transform(devUpper.begin(), devUpper.end(), devUpper.begin(), ::toupper);
        if (devUpper == upper) return &dev;
    }
    if (upper.find("F030") != std::string::npos) {
        for (const auto& dev : devices) {
            if (dev.partNumber == "STM32F030R8") return &dev;
        }
    }
    if (upper.find("F401") != std::string::npos || upper.find("F407") != std::string::npos) {
        for (const auto& dev : devices) {
            if (dev.partNumber == "STM32F401RE") return &dev;
        }
    }
    return nullptr;
}

bool Stm32DriverPack::validateHardwareRequirements(
    const McuDeviceSpec& device,
    const std::set<std::string>& selectedFeatures,
    std::string& outError) const {
    
    std::vector<std::string> missing;
    resolver.resolveDependencies(selectedFeatures, device.supportedPeripherals, missing);
    
    if (!missing.empty()) {
        std::stringstream ss;
        ss << "Target MCU " << device.partNumber << " lacks required hardware peripheral(s): ";
        for (size_t i = 0; i < missing.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << missing[i];
        }
        outError = ss.str();
        return false;
    }
    return true;
}

GeneratedProject Stm32DriverPack::generateProjectTree(
    const McuDeviceSpec& device,
    const std::set<std::string>& selectedFeatures,
    const std::string& projectName) const {

    GeneratedProject proj;
    proj.projectName = projectName;
    proj.targetMcu = device.partNumber;
    proj.family = device.family;
    proj.vendor = device.vendor;

    std::vector<std::string> missing;
    std::set<std::string> activeFeatures = resolver.resolveDependencies(
        selectedFeatures, device.supportedPeripherals, missing
    );

    // Standard STM32 directory hierarchy
    proj.directories = {
        "Core/Inc",
        "Core/Src",
        "Drivers/CMSIS",
        "Drivers/Device",
        "Drivers/Inc",
        "Drivers/Src",
        "Startup",
        "Linker",
        "Middleware",
        "generated",
        "Build"
    };

    // 1. Device Register Header: Drivers/Device/STM32F030x8.h
    std::stringstream devHeader;
    devHeader << "/**\n"
              << " * @file " << device.deviceHeaderName << "\n"
              << " * @brief Device peripheral register definitions for " << device.partNumber << "\n"
              << " * @provenance " << device.sourceProvenance << "\n"
              << " */\n"
              << "#pragma once\n"
              << "#include <stdint.h>\n\n"
              << "/* Memory Map */\n"
              << "#define FLASH_BASE       (0x08000000UL)\n"
              << "#define SRAM_BASE        (0x20000000UL)\n"
              << "#define PERIPH_BASE      (0x40000000UL)\n\n"
              << "#define APB_BASE         (PERIPH_BASE)\n"
              << "#define AHB_BASE         (PERIPH_BASE + 0x00020000UL)\n"
              << "#define AHB2_BASE        (PERIPH_BASE + 0x08000000UL)\n\n"
              << "#define GPIOA_BASE       (AHB2_BASE + 0x0000UL)\n"
              << "#define GPIOB_BASE       (AHB2_BASE + 0x0400UL)\n"
              << "#define GPIOC_BASE       (AHB2_BASE + 0x0800UL)\n"
              << "#define RCC_BASE         (AHB_BASE + 0x1000UL)\n"
              << "#define SYSCFG_BASE      (APB_BASE + 0x00010000UL)\n"
              << "#define EXTI_BASE        (APB_BASE + 0x00010400UL)\n"
              << "#define USART1_BASE      (APB_BASE + 0x00013800UL)\n"
              << "#define USART2_BASE      (APB_BASE + 0x4400UL)\n"
              << "#define I2C1_BASE        (APB_BASE + 0x5400UL)\n"
              << "#define SPI1_BASE        (APB_BASE + 0x00013000UL)\n"
              << "#define ADC1_BASE        (APB_BASE + 0x00012400UL)\n"
              << "#define TIM3_BASE        (APB_BASE + 0x0400UL)\n"
              << "#define DMA1_BASE        (AHB_BASE + 0x0000UL)\n\n"
              << "typedef struct {\n"
              << "    volatile uint32_t MODER;\n"
              << "    volatile uint32_t OTYPER;\n"
              << "    volatile uint32_t OSPEEDR;\n"
              << "    volatile uint32_t PUPDR;\n"
              << "    volatile uint32_t IDR;\n"
              << "    volatile uint32_t ODR;\n"
              << "    volatile uint32_t BSRR;\n"
              << "    volatile uint32_t LCKR;\n"
              << "    volatile uint32_t AFR[2];\n"
              << "    volatile uint32_t BRR;\n"
              << "} GPIO_TypeDef;\n\n"
              << "typedef struct {\n"
              << "    volatile uint32_t CR;\n"
              << "    volatile uint32_t CFGR;\n"
              << "    volatile uint32_t CIR;\n"
              << "    volatile uint32_t APB2RSTR;\n"
              << "    volatile uint32_t APB1RSTR;\n"
              << "    volatile uint32_t AHBENR;\n"
              << "    volatile uint32_t APB2ENR;\n"
              << "    volatile uint32_t APB1ENR;\n"
              << "} RCC_TypeDef;\n\n"
              << "#define GPIOA ((GPIO_TypeDef*) GPIOA_BASE)\n"
              << "#define GPIOB ((GPIO_TypeDef*) GPIOB_BASE)\n"
              << "#define GPIOC ((GPIO_TypeDef*) GPIOC_BASE)\n"
              << "#define RCC   ((RCC_TypeDef*) RCC_BASE)\n";
    proj.files["Drivers/Device/" + device.deviceHeaderName] = devHeader.str();

    // 2. RCC Driver (Core System)
    if (activeFeatures.find("RCC") != activeFeatures.end()) {
        proj.includedDrivers.insert("RCC_Driver");
        proj.files["Drivers/Inc/RCC_Driver.h"] = 
            "#pragma once\n"
            "#include \"../Device/" + device.deviceHeaderName + "\"\n"
            "#include <stdint.h>\n"
            "#include <stdbool.h>\n\n"
            "void RCC_InitSystemClock(uint32_t targetHz);\n"
            "void RCC_EnablePeripheralClock(const char* periphName);\n";

        proj.files["Drivers/Src/RCC_Driver.c"] = 
            "#include \"../Inc/RCC_Driver.h\"\n"
            "#include <string.h>\n\n"
            "void RCC_InitSystemClock(uint32_t targetHz) {\n"
            "    (void)targetHz;\n"
            "    /* Configure internal 48MHz PLL or HSI clock */\n"
            "    RCC->CR |= (1 << 0); /* HSI ON */\n"
            "    while (!(RCC->CR & (1 << 1))); /* Wait HSI ready */\n"
            "}\n\n"
            "void RCC_EnablePeripheralClock(const char* periphName) {\n"
            "    if (strcmp(periphName, \"GPIOA\") == 0) RCC->AHBENR |= (1 << 17);\n"
            "    else if (strcmp(periphName, \"GPIOB\") == 0) RCC->AHBENR |= (1 << 18);\n"
            "    else if (strcmp(periphName, \"GPIOC\") == 0) RCC->AHBENR |= (1 << 19);\n"
            "    else if (strcmp(periphName, \"USART1\") == 0) RCC->APB2ENR |= (1 << 14);\n"
            "    else if (strcmp(periphName, \"SPI1\") == 0) RCC->APB2ENR |= (1 << 12);\n"
            "    else if (strcmp(periphName, \"TIM3\") == 0) RCC->APB1ENR |= (1 << 1);\n"
            "}\n";
    }

    // 3. NVIC Driver
    if (activeFeatures.find("NVIC") != activeFeatures.end()) {
        proj.includedDrivers.insert("NVIC_Driver");
        proj.files["Drivers/Inc/NVIC_Driver.h"] = 
            "#pragma once\n"
            "#include <stdint.h>\n"
            "void NVIC_EnableIRQ(uint32_t irqNumber);\n"
            "void NVIC_SetPriority(uint32_t irqNumber, uint32_t priority);\n";

        proj.files["Drivers/Src/NVIC_Driver.c"] = 
            "#include \"../Inc/NVIC_Driver.h\"\n"
            "void NVIC_EnableIRQ(uint32_t irqNumber) { (void)irqNumber; }\n"
            "void NVIC_SetPriority(uint32_t irqNumber, uint32_t priority) { (void)irqNumber; (void)priority; }\n";
    }

    // 4. SYSCFG Driver
    if (activeFeatures.find("SYSCFG") != activeFeatures.end()) {
        proj.includedDrivers.insert("SYSCFG_Driver");
        proj.files["Drivers/Inc/SYSCFG_Driver.h"] = 
            "#pragma once\n"
            "#include <stdint.h>\n"
            "void SYSCFG_RouteExti(uint8_t extiLine, uint8_t portSource);\n";

        proj.files["Drivers/Src/SYSCFG_Driver.c"] = 
            "#include \"../Inc/SYSCFG_Driver.h\"\n"
            "void SYSCFG_RouteExti(uint8_t extiLine, uint8_t portSource) { (void)extiLine; (void)portSource; }\n";
    }

    // 5. EXTI Driver
    if (activeFeatures.find("EXTI") != activeFeatures.end()) {
        proj.includedDrivers.insert("EXTI_Driver");
        proj.files["Drivers/Inc/EXTI_Driver.h"] = 
            "#pragma once\n"
            "#include <stdint.h>\n"
            "#include <stdbool.h>\n"
            "void EXTI_ConfigurePin(uint8_t pin, bool rising, bool falling);\n";

        proj.files["Drivers/Src/EXTI_Driver.c"] = 
            "#include \"../Inc/EXTI_Driver.h\"\n"
            "void EXTI_ConfigurePin(uint8_t pin, bool rising, bool falling) { (void)pin; (void)rising; (void)falling; }\n";
    }

    // 6. GPIO Driver
    if (activeFeatures.find("GPIO") != activeFeatures.end()) {
        proj.includedDrivers.insert("GPIO_Driver");
        proj.files["Drivers/Inc/GPIO_Driver.h"] = 
            "#pragma once\n"
            "#include \"../Device/" + device.deviceHeaderName + "\"\n"
            "#include <stdint.h>\n"
            "#include <stdbool.h>\n\n"
            "typedef enum { GPIO_MODE_INPUT = 0, GPIO_MODE_OUTPUT = 1, GPIO_MODE_AF = 2, GPIO_MODE_ANALOG = 3 } GpioMode;\n"
            "void GPIO_InitPin(GPIO_TypeDef* port, uint8_t pin, GpioMode mode);\n"
            "void GPIO_WritePin(GPIO_TypeDef* port, uint8_t pin, bool state);\n"
            "bool GPIO_ReadPin(GPIO_TypeDef* port, uint8_t pin);\n";

        proj.files["Drivers/Src/GPIO_Driver.c"] = 
            "#include \"../Inc/GPIO_Driver.h\"\n"
            "void GPIO_InitPin(GPIO_TypeDef* port, uint8_t pin, GpioMode mode) {\n"
            "    port->MODER &= ~(3U << (pin * 2));\n"
            "    port->MODER |= ((uint32_t)mode << (pin * 2));\n"
            "}\n"
            "void GPIO_WritePin(GPIO_TypeDef* port, uint8_t pin, bool state) {\n"
            "    if (state) port->BSRR = (1U << pin);\n"
            "    else port->BRR = (1U << pin);\n"
            "}\n"
            "bool GPIO_ReadPin(GPIO_TypeDef* port, uint8_t pin) {\n"
            "    return (port->IDR & (1U << pin)) != 0;\n"
            "}\n";
    }

    // 7. USART Driver
    if (activeFeatures.find("USART") != activeFeatures.end()) {
        proj.includedDrivers.insert("USART_Driver");
        proj.files["Drivers/Inc/USART_Driver.h"] = 
            "#pragma once\n"
            "#include <stdint.h>\n"
            "#include <stdbool.h>\n"
            "void USART_Init(uint32_t baudRate);\n"
            "void USART_SendByte(uint8_t byte);\n"
            "uint8_t USART_ReceiveByte(void);\n";

        proj.files["Drivers/Src/USART_Driver.c"] = 
            "#include \"../Inc/USART_Driver.h\"\n"
            "void USART_Init(uint32_t baudRate) { (void)baudRate; }\n"
            "void USART_SendByte(uint8_t byte) { (void)byte; }\n"
            "uint8_t USART_ReceiveByte(void) { return 0; }\n";
    }

    // 8. I2C Driver
    if (activeFeatures.find("I2C") != activeFeatures.end()) {
        proj.includedDrivers.insert("I2C_Driver");
        proj.files["Drivers/Inc/I2C_Driver.h"] = 
            "#pragma once\n"
            "#include <stdint.h>\n"
            "void I2C_Init(uint32_t clockSpeed);\n"
            "bool I2C_Write(uint8_t devAddr, const uint8_t* data, uint16_t len);\n";

        proj.files["Drivers/Src/I2C_Driver.c"] = 
            "#include \"../Inc/I2C_Driver.h\"\n"
            "void I2C_Init(uint32_t clockSpeed) { (void)clockSpeed; }\n"
            "bool I2C_Write(uint8_t devAddr, const uint8_t* data, uint16_t len) { (void)devAddr; (void)data; (void)len; return true; }\n";
    }

    // 9. SPI Driver
    if (activeFeatures.find("SPI") != activeFeatures.end()) {
        proj.includedDrivers.insert("SPI_Driver");
        proj.files["Drivers/Inc/SPI_Driver.h"] = 
            "#pragma once\n"
            "#include <stdint.h>\n"
            "void SPI_Init(void);\n"
            "uint8_t SPI_TransferByte(uint8_t byte);\n";

        proj.files["Drivers/Src/SPI_Driver.c"] = 
            "#include \"../Inc/SPI_Driver.h\"\n"
            "void SPI_Init(void) {}\n"
            "uint8_t SPI_TransferByte(uint8_t byte) { return byte; }\n";
    }

    // 10. ADC Driver
    if (activeFeatures.find("ADC") != activeFeatures.end()) {
        proj.includedDrivers.insert("ADC_Driver");
        proj.files["Drivers/Inc/ADC_Driver.h"] = 
            "#pragma once\n"
            "#include <stdint.h>\n"
            "void ADC_Init(void);\n"
            "uint16_t ADC_ReadChannel(uint8_t channel);\n";

        proj.files["Drivers/Src/ADC_Driver.c"] = 
            "#include \"../Inc/ADC_Driver.h\"\n"
            "void ADC_Init(void) {}\n"
            "uint16_t ADC_ReadChannel(uint8_t channel) { (void)channel; return 2048; }\n";
    }

    // 11. Timer / PWM Driver
    if (activeFeatures.find("TIMER") != activeFeatures.end() || activeFeatures.find("PWM") != activeFeatures.end()) {
        proj.includedDrivers.insert("Timer_Driver");
        proj.files["Drivers/Inc/Timer_Driver.h"] = 
            "#pragma once\n"
            "#include <stdint.h>\n"
            "void Timer_Init(uint32_t frequencyHz);\n"
            "void Timer_SetPwmDuty(uint8_t channel, uint8_t dutyPercent);\n";

        proj.files["Drivers/Src/Timer_Driver.c"] = 
            "#include \"../Inc/Timer_Driver.h\"\n"
            "void Timer_Init(uint32_t frequencyHz) { (void)frequencyHz; }\n"
            "void Timer_SetPwmDuty(uint8_t channel, uint8_t dutyPercent) { (void)channel; (void)dutyPercent; }\n";
    }

    // 12. DMA Driver (Only when device physically supports DMA)
    if (device.hasPeripheral("DMA") && activeFeatures.find("DMA") != activeFeatures.end()) {
        proj.includedDrivers.insert("DMA_Driver");
        proj.files["Drivers/Inc/DMA_Driver.h"] = 
            "#pragma once\n"
            "#include <stdint.h>\n"
            "void DMA_Init(void);\n"
            "void DMA_StartTransfer(uint32_t src, uint32_t dst, uint16_t count);\n";

        proj.files["Drivers/Src/DMA_Driver.c"] = 
            "#include \"../Inc/DMA_Driver.h\"\n"
            "void DMA_Init(void) {}\n"
            "void DMA_StartTransfer(uint32_t src, uint32_t dst, uint16_t count) { (void)src; (void)dst; (void)count; }\n";
    }

    // 13. External Wi-Fi Middleware (e.g. ESP-AT over USART)
    if (selectedFeatures.find("WIFI") != selectedFeatures.end() || selectedFeatures.find("WIFI_EXTERNAL") != selectedFeatures.end()) {
        proj.files["Middleware/WiFi_AT/esp_at_driver.h"] = 
            "#pragma once\n"
            "#include <stdbool.h>\n"
            "bool WiFi_ConnectAP(const char* ssid, const char* pass);\n"
            "bool WiFi_StartServer(int port);\n";

        proj.files["Middleware/WiFi_AT/esp_at_driver.c"] = 
            "#include \"esp_at_driver.h\"\n"
            "bool WiFi_ConnectAP(const char* ssid, const char* pass) { (void)ssid; (void)pass; return true; }\n"
            "bool WiFi_StartServer(int port) { (void)port; return true; }\n";
        proj.includedDrivers.insert("WiFi_External_AT");
    }

    // 14. Core Inc/Src
    proj.files["Core/Inc/main.h"] = 
        "#pragma once\n"
        "#include \"../Drivers/Inc/RCC_Driver.h\"\n"
        "#include \"../Drivers/Inc/GPIO_Driver.h\"\n"
        "void System_Init(void);\n";

    proj.files["Core/Inc/app_config.h"] = 
        "#pragma once\n"
        "#define TARGET_MCU \"" + device.partNumber + "\"\n"
        "#define CLOCK_HZ " + std::to_string(device.maxClockHz) + "UL\n";

    proj.files["Core/Src/main.c"] = 
        "#include \"../Inc/main.h\"\n"
        "#include \"../Inc/app_config.h\"\n\n"
        "int main(void) {\n"
        "    RCC_InitSystemClock(CLOCK_HZ);\n"
        "    RCC_EnablePeripheralClock(\"GPIOA\");\n"
        "    GPIO_InitPin(GPIOA, 5, GPIO_MODE_OUTPUT);\n"
        "    while (1) {\n"
        "        GPIO_WritePin(GPIOA, 5, true);\n"
        "    }\n"
        "    return 0;\n"
        "}\n";

    proj.files["Core/Src/app.c"] = 
        "#include \"../Inc/app_config.h\"\n"
        "void app_run(void) {}\n";

    // 15. Startup File: Startup/startup.s
    proj.files["Startup/startup.s"] = 
        "/* Bare-metal vector table and startup code for " + device.partNumber + " */\n"
        ".syntax unified\n"
        ".cpu cortex-m0\n"
        ".thumb\n\n"
        ".global g_pfnVectors\n"
        ".global Reset_Handler\n\n"
        ".section .isr_vector,\"a\",%progbits\n"
        "g_pfnVectors:\n"
        "    .word _estack\n"
        "    .word Reset_Handler\n"
        "    .word NMI_Handler\n"
        "    .word HardFault_Handler\n\n"
        ".section .text.Reset_Handler\n"
        "Reset_Handler:\n"
        "    ldr r0, =_estack\n"
        "    mov sp, r0\n"
        "    bl main\n"
        "LoopForever:\n"
        "    b LoopForever\n"
        ".thumb_func\n"
        "NMI_Handler: b .\n"
        ".thumb_func\n"
        "HardFault_Handler: b .\n";

    // 16. Linker Script: Linker/linker.ld
    std::stringstream linker;
    linker << "/* Linker script for " << device.partNumber << " (" 
           << (device.flashBytes / 1024) << "KB Flash, " 
           << (device.ramBytes / 1024) << "KB RAM) */\n"
           << "ENTRY(Reset_Handler)\n\n"
           << "_estack = ORIGIN(RAM) + LENGTH(RAM);\n\n"
           << "MEMORY\n"
           << "{\n"
           << "  RAM (xrw)   : ORIGIN = 0x20000000, LENGTH = " << (device.ramBytes / 1024) << "K\n"
           << "  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = " << (device.flashBytes / 1024) << "K\n"
           << "}\n\n"
           << "SECTIONS\n"
           << "{\n"
           << "  .isr_vector : { KEEP(*(.isr_vector)) } > FLASH\n"
           << "  .text : { *(.text*) *(.rodata*) } > FLASH\n"
           << "  .data : { *(.data*) } > RAM AT > FLASH\n"
           << "  .bss : { *(.bss*) *(COMMON) } > RAM\n"
           << "}\n";
    proj.files["Linker/linker.ld"] = linker.str();

    // 17. Makefile & Build configuration
    std::stringstream mk;
    mk << "CC = arm-none-eabi-gcc\n"
       << "CFLAGS = -mcpu=cortex-m0 -mthumb -Wall -Wextra -O2 -I./Core/Inc -I./Drivers/Inc -I./Drivers/Device\n"
       << "LDFLAGS = -T./Linker/linker.ld -nostartfiles\n\n"
       << "SRCS = $(wildcard Core/Src/*.c) $(wildcard Drivers/Src/*.c)\n"
       << "OBJS = $(SRCS:.c=.o) Startup/startup.o\n\n"
       << "all: Build/" << projectName << ".elf\n\n"
       << "Build/" << projectName << ".elf: $(OBJS)\n"
       << "\t@mkdir -p Build\n"
       << "\t$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@\n\n"
       << "%.o: %.c\n"
       << "\t$(CC) $(CFLAGS) -c $< -o $@\n\n"
       << "Startup/startup.o: Startup/startup.s\n"
       << "\t$(CC) $(CFLAGS) -c $< -o $@\n\n"
       << "clean:\n"
       << "\trm -f $(OBJS) Build/" << projectName << ".elf\n";
    proj.files["Makefile"] = mk.str();
    proj.files["README.md"] = "# " + projectName + "\n\nFirmware project generated for " + device.partNumber + " using Embedded UI Designer.\n";
    proj.files[".gitignore"] = "Build/\n*.o\n*.elf\n*.bin\n";

    proj.buildCommands = { "make -j4", "arm-none-eabi-size Build/" + projectName + ".elf" };
    return proj;
}

} // namespace driverpack
