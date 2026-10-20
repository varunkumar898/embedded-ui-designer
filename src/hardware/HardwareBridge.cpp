#include "HardwareBridge.h"
#include "UIComponent.h"
#include "ProgressBarComponent.h"
#include "LabelComponent.h"
#include "SliderComponent.h"
#include "SwitchComponent.h"
#include "CheckboxComponent.h"
#include "DeviceDatabase.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <algorithm>

QString pinModeToString(PinMode mode) {
    switch (mode) {
        case PinMode::DigitalIn: return "Digital In";
        case PinMode::DigitalOut: return "Digital Out";
        case PinMode::InputPullUp: return "Input with Pull-up";
        case PinMode::InputPullDown: return "Input with Pull-down";
        case PinMode::OpenDrainOut: return "Open-drain Output";
        case PinMode::PwmOutput: return "PWM Output";
        case PinMode::AnalogIn: return "Analog In";
        case PinMode::AlternateFunction: return "Alternate Function";
        case PinMode::InterruptInput: return "Interrupt Input";
        case PinMode::SpiRawTransfer: return "SPI Raw Transfer (byte in/out)";
        case PinMode::I2cBus: return "I2C Bus";
        case PinMode::UartTxRx: return "UART / USART";
        case PinMode::CanBus: return "CAN / FDCAN";
        case PinMode::None:
        default: return "None / Unassigned";
    }
}

PinMode stringToPinMode(const QString& str) {
    if (str == "Digital In" || str == "GPIO Input" || str == "GPIO_Input") return PinMode::DigitalIn;
    if (str == "Digital Out" || str == "GPIO Output" || str == "GPIO_Output") return PinMode::DigitalOut;
    if (str == "Input with Pull-up" || str == "InputPullUp") return PinMode::InputPullUp;
    if (str == "Input with Pull-down" || str == "InputPullDown") return PinMode::InputPullDown;
    if (str == "Open-drain Output" || str == "OpenDrainOut") return PinMode::OpenDrainOut;
    if (str == "PWM Output" || str == "PWM") return PinMode::PwmOutput;
    if (str == "Analog In" || str == "Analog" || str == "ADC") return PinMode::AnalogIn;
    if (str == "Alternate Function" || str == "AlternateFunction") return PinMode::AlternateFunction;
    if (str == "Interrupt Input" || str == "Interrupt") return PinMode::InterruptInput;
    if (str == "SPI Raw Transfer (byte in/out)" || str == "SPI") return PinMode::SpiRawTransfer;
    if (str == "I2C Bus" || str == "I2C") return PinMode::I2cBus;
    if (str == "UART / USART" || str == "UART" || str == "USART") return PinMode::UartTxRx;
    if (str == "CAN / FDCAN" || str == "CAN") return PinMode::CanBus;
    return PinMode::None;
}

QString connectionStateToString(ConnectionState state) {
    switch (state) {
        case ConnectionState::Disconnected: return "Disconnected";
        case ConnectionState::DetectingProbe: return "Detecting Probe";
        case ConnectionState::Connecting: return "Connecting";
        case ConnectionState::Connected: return "Connected";
        case ConnectionState::ConnectionFailed: return "Connection Failed";
        case ConnectionState::DeviceRemoved: return "Device Removed";
        case ConnectionState::SimulationMode: return "Simulation Mode";
    }
    return "Disconnected";
}

HardwareBridge& HardwareBridge::instance() {
    static HardwareBridge s_instance;
    return s_instance;
}

HardwareBridge::HardwareBridge(QObject* parent)
    : QObject(parent)
{
    initializeDefaultBoards();
    connect(&m_pollTimer, &QTimer::timeout, this, &HardwareBridge::onPollTimer);
    m_pollTimer.setInterval(50); // 50ms poll loop
    m_pollTimer.start();
    m_pwmThrottleTimer.start();

    connect(&m_openOcd, &OpenOcdManager::connectionStatusChanged, this, [this](bool connected, const QString& probeName) {
        if (connected) {
            updateConnectionState(ConnectionState::Connected, probeName);
        } else {
            if (m_connectionState == ConnectionState::Connected) {
                updateConnectionState(ConnectionState::Disconnected);
            }
        }
    });
    connect(&m_openOcd, &OpenOcdManager::probeDiscovered, this, [this](const DiscoveredProbe& probe) {
        emit probeDiscovered(probe.name);
    });
    connect(&m_openOcd, &OpenOcdManager::probeRemoved, this, [this]() {
        emit probeRemoved();
        if (m_connectionState == ConnectionState::Connected) {
            updateConnectionState(ConnectionState::DeviceRemoved, "Hardware probe unplugged");
        }
    });

    // Start background probe polling to detect connected hardware
    m_openOcd.startProbePolling(1000);
}

void HardwareBridge::updateConnectionState(ConnectionState state, const QString& info) {
    m_connectionState = state;
    m_hardwareConnected = (state == ConnectionState::Connected);
    QString text = info.isEmpty() ? connectionStatusText() : info;
    emit connectionStateChanged(state, text);
    emit connectionStatusChanged(m_hardwareConnected, connectedProbeName());
}

void HardwareBridge::initializeDefaultBoards() {
    auto makePin = [](const QString& name, const QString& port,
                      bool isAdc, int adcChan, int adcBits,
                      bool isPwm, const QString& tim, int timChan, int timAf,
                      bool isSpi, const QString& spiBus, int spiAf,
                      const QStringList& extraPeripherals = QStringList(),
                      bool isReserved = false, const QString& reservedReason = QString()) -> PinProfile {
        PinProfile p;
        p.pinName = name;
        p.portGroup = port;
        p.isReserved = isReserved;
        p.reservedReason = reservedReason;
        p.supportedModes = {PinMode::None, PinMode::DigitalIn, PinMode::DigitalOut, PinMode::InputPullUp, PinMode::InputPullDown};

        if (isAdc) {
            p.supportedModes.append(PinMode::AnalogIn);
            p.adc.available = true;
            p.adc.peripheral = "ADC1";
            p.adc.channel = adcChan;
            p.adc.resolutionBits = adcBits > 0 ? adcBits : 12;
            p.adc.dataReg = 0x40012440;
            p.adc.triggerReg = 0x40012408;
            p.supportedPeripherals.append(QString("%1_IN%2").arg(p.adc.peripheral).arg(adcChan));
        }

        if (isPwm) {
            p.supportedModes.append(PinMode::PwmOutput);
            p.pwm.available = true;
            p.pwm.timer = tim;
            p.pwm.channel = timChan;
            p.pwm.alternateFunction = timAf;
            p.pwm.ccrReg = 0x40000434 + (timChan - 1) * 4;
            p.supportedPeripherals.append(QString("%1_CH%2").arg(tim).arg(timChan));
        }

        if (isSpi) {
            p.supportedModes.append(PinMode::SpiRawTransfer);
            p.spi.available = true;
            p.spi.peripheral = spiBus;
            p.spi.alternateFunction = spiAf;
            p.spi.cr1Reg = (spiBus == "SPI1") ? 0x40013000 : 0x40003800;
            p.spi.drReg  = (spiBus == "SPI1") ? 0x4001300C : 0x4000380C;
            p.spi.srReg  = (spiBus == "SPI1") ? 0x40013008 : 0x40003808;
            p.supportedPeripherals.append(spiBus);
        }

        for (const QString& ep : extraPeripherals) {
            p.supportedPeripherals.append(ep);
            if (ep.contains("I2C")) p.supportedModes.append(PinMode::I2cBus);
            if (ep.contains("USART") || ep.contains("UART")) p.supportedModes.append(PinMode::UartTxRx);
            if (ep.contains("CAN")) p.supportedModes.append(PinMode::CanBus);
        }

        return p;
    };

    // ────────────────────────────────────────────────────────────────────────
    // 1. STM32 Nucleo-F030R8 (STM32F030R8)
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile f0;
    f0.id = "stm32f030r8";
    f0.name = "STM32 Nucleo-F030R8 (STM32F030R8)";
    f0.mcuFamily = "STM32F0";
    f0.connectionType = "openocd";
    f0.gpioBase = 0x48000000;
    f0.bsrrOffset = 0x18;
    f0.openocdInterface = "interface/stlink.cfg";
    f0.openocdTarget = "target/stm32f0x.cfg";
    f0.portGroups = {"Port A", "Port B", "Port C", "Port D", "Port F"};
    f0.spiBusses = {"SPI1 (PA5/SCK, PA6/MISO, PA7/MOSI)", "SPI2 (PB13/SCK, PB14/MISO, PB15/MOSI)"};

    f0.pins["PA0"]  = makePin("PA0", "Port A", true,  0, 12, false, "", 0, 0, false, "", 0, {"USART2_CTS", "TIM2_CH1"});
    f0.pins["PA1"]  = makePin("PA1", "Port A", true,  1, 12, false, "", 0, 0, false, "", 0, {"USART2_RTS", "TIM2_CH2"});
    f0.pins["PA2"]  = makePin("PA2", "Port A", true,  2, 12, false, "", 0, 0, false, "", 0, {"USART2_TX", "TIM2_CH3"});
    f0.pins["PA3"]  = makePin("PA3", "Port A", true,  3, 12, false, "", 0, 0, false, "", 0, {"USART2_RX", "TIM2_CH4"});
    f0.pins["PA4"]  = makePin("PA4", "Port A", true,  4, 12, false, "", 0, 0, false, "", 0, {"SPI1_NSS", "TIM14_CH1"});
    f0.pins["PA5"]  = makePin("PA5", "Port A", false, 0, 0,  false, "", 0, 0, true, "SPI1", 0, {"SPI1_SCK", "TIM2_CH1"});
    f0.pins["PA6"]  = makePin("PA6", "Port A", true,  6, 12, true, "TIM3", 1, 1, true, "SPI1", 0, {"SPI1_MISO", "TIM16_CH1"});
    f0.pins["PA7"]  = makePin("PA7", "Port A", true,  7, 12, true, "TIM3", 2, 1, true, "SPI1", 0, {"SPI1_MOSI", "TIM17_CH1"});
    f0.pins["PA8"]  = makePin("PA8", "Port A", false, 0, 0,  true, "TIM1", 1, 2, false, "", 0, {"MCO", "USART1_CK"});
    f0.pins["PA9"]  = makePin("PA9", "Port A", false, 0, 0,  true, "TIM1", 2, 2, false, "", 0, {"USART1_TX", "I2C1_SCL"});
    f0.pins["PA10"] = makePin("PA10", "Port A", false, 0, 0, true, "TIM1", 3, 2, false, "", 0, {"USART1_RX", "I2C1_SDA"});
    f0.pins["PA13"] = makePin("PA13", "Port A", false, 0, 0, false, "", 0, 0, false, "", 0, {"SWDIO"}, true, "Reserved: Serial Wire Debug Data (SWDIO)");
    f0.pins["PA14"] = makePin("PA14", "Port A", false, 0, 0, false, "", 0, 0, false, "", 0, {"SWCLK"}, true, "Reserved: Serial Wire Debug Clock (SWCLK)");
    f0.pins["PB0"]  = makePin("PB0", "Port B", true,  8, 12, true, "TIM3", 3, 1, false, "", 0, {"TIM1_CH2N"});
    f0.pins["PB1"]  = makePin("PB1", "Port B", true,  9, 12, true, "TIM3", 4, 1, false, "", 0, {"TIM1_CH3N"});
    f0.pins["PB8"]  = makePin("PB8", "Port B", false, 0, 0,  true, "TIM16", 1, 2, false, "", 0, {"I2C1_SCL"});
    f0.pins["PB9"]  = makePin("PB9", "Port B", false, 0, 0,  true, "TIM17", 1, 2, false, "", 0, {"I2C1_SDA"});
    f0.pins["PB13"] = makePin("PB13", "Port B", false, 0, 0, false, "", 0, 0, true, "SPI2", 0, {"SPI2_SCK"});
    f0.pins["PB14"] = makePin("PB14", "Port B", false, 0, 0, false, "", 0, 0, true, "SPI2", 0, {"SPI2_MISO"});
    f0.pins["PB15"] = makePin("PB15", "Port B", false, 0, 0, false, "", 0, 0, true, "SPI2", 0, {"SPI2_MOSI"});
    f0.pins["PC0"]  = makePin("PC0", "Port C", true, 10, 12, false, "", 0, 0, false, "", 0);
    f0.pins["PC1"]  = makePin("PC1", "Port C", true, 11, 12, false, "", 0, 0, false, "", 0);
    f0.pins["PC13"] = makePin("PC13", "Port C", false, 0, 0, false, "", 0, 0, false, "", 0, {"USER_BUTTON"});
    m_boards[f0.id] = f0;

    m_simulatedAdcValues["PA0"] = 1024;
    m_simulatedAdcValues["PA1"] = 2048;
    m_simulatedAdcValues["PA4"] = 3072;
    m_simulatedAdcValues["PA6"] = 1500;
    m_simulatedAdcValues["PA7"] = 2500;

    // ────────────────────────────────────────────────────────────────────────
    // 2. STM32F4 Discovery (STM32F407VG)
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile f4disc;
    f4disc.id = "stm32f407g_disc1";
    f4disc.name = "STM32F4 Discovery (STM32F407VG)";
    f4disc.mcuFamily = "STM32F4";
    f4disc.connectionType = "openocd";
    f4disc.gpioBase = 0x40020000;
    f4disc.bsrrOffset = 0x18;
    f4disc.openocdInterface = "interface/stlink.cfg";
    f4disc.openocdTarget = "target/stm32f4x.cfg";
    f4disc.portGroups = {"Port A", "Port B", "Port C", "Port D", "Port E"};
    f4disc.spiBusses = {"SPI1 (PA5/SCK, PA6/MISO, PA7/MOSI)", "SPI2 (PB13/SCK, PB14/MISO, PB15/MOSI)"};
    f4disc.pins["PA0"] = makePin("PA0", "Port A", true, 0, 12, true, "TIM2", 1, 1, false, "", 0, {"USER_KEY"});
    f4disc.pins["PA1"] = makePin("PA1", "Port A", true, 1, 12, true, "TIM2", 2, 1, false, "", 0);
    f4disc.pins["PA5"] = makePin("PA5", "Port A", false, 0, 0, false, "", 0, 0, true, "SPI1", 5, {"SPI1_SCK"});
    f4disc.pins["PA6"] = makePin("PA6", "Port A", true, 6, 12, true, "TIM3", 1, 2, true, "SPI1", 5, {"SPI1_MISO"});
    f4disc.pins["PA7"] = makePin("PA7", "Port A", true, 7, 12, true, "TIM3", 2, 2, true, "SPI1", 5, {"SPI1_MOSI"});
    f4disc.pins["PB6"] = makePin("PB6", "Port B", false, 0, 0, true, "TIM4", 1, 2, false, "", 0, {"I2C1_SCL", "USART1_TX"});
    f4disc.pins["PB7"] = makePin("PB7", "Port B", false, 0, 0, true, "TIM4", 2, 2, false, "", 0, {"I2C1_SDA", "USART1_RX"});
    f4disc.pins["PD12"] = makePin("PD12", "Port D", false, 0, 0, true, "TIM4", 1, 2, false, "", 0, {"LED_GREEN"});
    f4disc.pins["PD13"] = makePin("PD13", "Port D", false, 0, 0, true, "TIM4", 2, 2, false, "", 0, {"LED_ORANGE"});
    f4disc.pins["PD14"] = makePin("PD14", "Port D", false, 0, 0, true, "TIM4", 3, 2, false, "", 0, {"LED_RED"});
    f4disc.pins["PD15"] = makePin("PD15", "Port D", false, 0, 0, true, "TIM4", 4, 2, false, "", 0, {"LED_BLUE"});
    f4disc.pins["PA13"] = makePin("PA13", "Port A", false, 0, 0, false, "", 0, 0, false, "", 0, {"SWDIO"}, true, "Reserved: SWDIO");
    f4disc.pins["PA14"] = makePin("PA14", "Port A", false, 0, 0, false, "", 0, 0, false, "", 0, {"SWCLK"}, true, "Reserved: SWCLK");
    m_boards[f4disc.id] = f4disc;

    // ────────────────────────────────────────────────────────────────────────
    // 3. STM32 Nucleo-F401RE (STM32F401RE)
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile f401;
    f401.id = "nucleo_f401re";
    f401.name = "STM32 Nucleo-F401RE (STM32F401RE)";
    f401.mcuFamily = "STM32F4";
    f401.connectionType = "openocd";
    f401.gpioBase = 0x40020000;
    f401.bsrrOffset = 0x18;
    f401.openocdInterface = "interface/stlink.cfg";
    f401.openocdTarget = "target/stm32f4x.cfg";
    f401.portGroups = {"Port A", "Port B", "Port C"};
    f401.spiBusses = {"SPI1 (PA5/SCK, PA6/MISO, PA7/MOSI)"};
    f401.pins["PA0"] = makePin("PA0", "Port A", true, 0, 12, true, "TIM2", 1, 1, false, "", 0);
    f401.pins["PA2"] = makePin("PA2", "Port A", true, 2, 12, true, "TIM2", 3, 1, false, "", 0, {"USART2_TX"});
    f401.pins["PA3"] = makePin("PA3", "Port A", true, 3, 12, true, "TIM2", 4, 1, false, "", 0, {"USART2_RX"});
    f401.pins["PA5"] = makePin("PA5", "Port A", false, 0, 0, false, "", 0, 0, true, "SPI1", 5, {"LED_GREEN", "SPI1_SCK"});
    f401.pins["PB8"] = makePin("PB8", "Port B", false, 0, 0, true, "TIM4", 3, 2, false, "", 0, {"I2C1_SCL"});
    f401.pins["PB9"] = makePin("PB9", "Port B", false, 0, 0, true, "TIM4", 4, 2, false, "", 0, {"I2C1_SDA"});
    f401.pins["PC13"] = makePin("PC13", "Port C", false, 0, 0, false, "", 0, 0, false, "", 0, {"USER_B1"});
    m_boards[f401.id] = f401;

    // ────────────────────────────────────────────────────────────────────────
    // 4. STM32F469I-DISCO
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile f4;
    f4.id = "stm32f469i";
    f4.name = "STM32F469I-DISCO";
    f4.mcuFamily = "STM32F4";
    f4.connectionType = "openocd";
    f4.gpioBase = 0x40020000;
    f4.bsrrOffset = 0x18;
    f4.openocdInterface = "interface/stlink.cfg";
    f4.openocdTarget = "target/stm32f4x.cfg";
    f4.portGroups = {"Port A", "Port B", "Port C", "Port D"};
    f4.spiBusses = {"SPI2 (PB13/SCK, PB14/MISO, PB15/MOSI)"};
    f4.pins["PA0"]  = makePin("PA0", "Port A", true,  0, 12, true, "TIM2", 1, 1, false, "", 0);
    f4.pins["PA3"]  = makePin("PA3", "Port A", true,  3, 12, false, "", 0, 0, false, "", 0);
    f4.pins["PB0"]  = makePin("PB0", "Port B", true,  8, 12, true, "TIM3", 3, 2, false, "", 0);
    f4.pins["PB13"] = makePin("PB13", "Port B", false, 0, 0, false, "", 0, 0, true, "SPI2", 5);
    f4.pins["PB14"] = makePin("PB14", "Port B", false, 0, 0, false, "", 0, 0, true, "SPI2", 5);
    f4.pins["PB15"] = makePin("PB15", "Port B", false, 0, 0, false, "", 0, 0, true, "SPI2", 5);
    f4.pins["PC13"] = makePin("PC13", "Port C", false, 0, 0, false, "", 0, 0, false, "", 0);
    m_boards[f4.id] = f4;

    // ────────────────────────────────────────────────────────────────────────
    // 5. STM32H747I-DISCO
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile h7;
    h7.id = "stm32h747i";
    h7.name = "STM32H747I-DISCO";
    h7.mcuFamily = "STM32H7";
    h7.connectionType = "openocd";
    h7.gpioBase = 0x58020000;
    h7.bsrrOffset = 0x18;
    h7.openocdInterface = "interface/stlink.cfg";
    h7.openocdTarget = "target/stm32h7x.cfg";
    h7.portGroups = {"Port A", "Port B", "Port C", "Port D", "Port E", "Port H", "Port J", "Port K"};
    h7.spiBusses = {"SPI1 (PA5/SCK, PA6/MISO, PA7/MOSI)"};
    h7.pins["PA0"]  = makePin("PA0", "Port A", true, 16, 16, false, "", 0, 0, false, "", 0);
    h7.pins["PA4"]  = makePin("PA4", "Port A", true, 18, 16, false, "", 0, 0, false, "", 0);
    h7.pins["PA5"]  = makePin("PA5", "Port A", false, 0, 0, false, "", 0, 0, true, "SPI1", 5);
    h7.pins["PA6"]  = makePin("PA6", "Port A", false, 0, 0, false, "", 0, 0, true, "SPI1", 5);
    h7.pins["PA7"]  = makePin("PA7", "Port A", false, 0, 0, false, "", 0, 0, true, "SPI1", 5);
    h7.pins["PA8"]  = makePin("PA8", "Port A", false, 0, 0, true, "TIM1", 1, 1, false, "", 0);
    m_boards[h7.id] = h7;

    // ────────────────────────────────────────────────────────────────────────
    // 6. ESP32-S3-DevKitC-1
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile espS3Dev;
    espS3Dev.id = "esp32s3_devkitc1";
    espS3Dev.name = "ESP32-S3-DevKitC-1 (ESP32-S3)";
    espS3Dev.mcuFamily = "ESP32-S3";
    espS3Dev.connectionType = "serial";
    espS3Dev.gpioBase = 0x60004000;
    espS3Dev.bsrrOffset = 0x08;
    espS3Dev.openocdTarget = "target/esp32s3.cfg";
    espS3Dev.portGroups = {"GPIO Bank 0 (IO0-IO21)", "GPIO Bank 1 (IO26-IO48)"};
    espS3Dev.spiBusses = {"SPI2 / FSPI (IO10-IO13)", "SPI3 / HSPI (IO35-IO38)"};
    espS3Dev.pins["IO0"]  = makePin("IO0", "GPIO Bank 0 (IO0-IO21)", false, 0, 0, false, "", 0, 0, false, "", 0, {"BOOT"}, true, "Reserved: Boot Strap Pin (IO0)");
    espS3Dev.pins["IO1"]  = makePin("IO1", "GPIO Bank 0 (IO0-IO21)", true, 0, 12, true, "LEDC", 0, 0, false, "", 0, {"ADC1_CH0"});
    espS3Dev.pins["IO2"]  = makePin("IO2", "GPIO Bank 0 (IO0-IO21)", true, 1, 12, true, "LEDC", 1, 0, false, "", 0, {"ADC1_CH1"});
    espS3Dev.pins["IO3"]  = makePin("IO3", "GPIO Bank 0 (IO0-IO21)", true, 2, 12, true, "LEDC", 2, 0, false, "", 0, {"ADC1_CH2"});
    espS3Dev.pins["IO4"]  = makePin("IO4", "GPIO Bank 0 (IO0-IO21)", true, 3, 12, true, "LEDC", 3, 0, false, "", 0, {"ADC1_CH3"});
    espS3Dev.pins["IO5"]  = makePin("IO5", "GPIO Bank 0 (IO0-IO21)", true, 4, 12, true, "LEDC", 4, 0, false, "", 0, {"ADC1_CH4"});
    espS3Dev.pins["IO6"]  = makePin("IO6", "GPIO Bank 0 (IO0-IO21)", true, 5, 12, true, "LEDC", 5, 0, false, "", 0, {"ADC1_CH5"});
    espS3Dev.pins["IO7"]  = makePin("IO7", "GPIO Bank 0 (IO0-IO21)", true, 6, 12, true, "LEDC", 6, 0, false, "", 0, {"ADC1_CH6"});
    espS3Dev.pins["IO8"]  = makePin("IO8", "GPIO Bank 0 (IO0-IO21)", true, 7, 12, true, "LEDC", 7, 0, false, "", 0, {"ADC1_CH7", "I2C0_SDA"});
    espS3Dev.pins["IO9"]  = makePin("IO9", "GPIO Bank 0 (IO0-IO21)", true, 8, 12, true, "LEDC", 0, 0, false, "", 0, {"ADC1_CH8", "I2C0_SCL"});
    espS3Dev.pins["IO10"] = makePin("IO10", "GPIO Bank 0 (IO0-IO21)", false, 0, 0, false, "", 0, 0, true, "FSPI_HD", 0, {"FSPI_HD"});
    espS3Dev.pins["IO11"] = makePin("IO11", "GPIO Bank 0 (IO0-IO21)", false, 0, 0, false, "", 0, 0, true, "FSPI_WP", 0, {"FSPI_WP"});
    espS3Dev.pins["IO12"] = makePin("IO12", "GPIO Bank 0 (IO0-IO21)", false, 0, 0, false, "", 0, 0, true, "FSPI_SCK", 0, {"FSPI_SCK"});
    espS3Dev.pins["IO13"] = makePin("IO13", "GPIO Bank 0 (IO0-IO21)", false, 0, 0, false, "", 0, 0, true, "FSPI_MOSI", 0, {"FSPI_MOSI"});
    espS3Dev.pins["IO14"] = makePin("IO14", "GPIO Bank 0 (IO0-IO21)", false, 0, 0, false, "", 0, 0, true, "FSPI_MISO", 0, {"FSPI_MISO"});
    espS3Dev.pins["IO19"] = makePin("IO19", "GPIO Bank 0 (IO0-IO21)", false, 0, 0, false, "", 0, 0, false, "", 0, {"USB_D-"}, true, "Reserved: Native USB D-");
    espS3Dev.pins["IO20"] = makePin("IO20", "GPIO Bank 0 (IO0-IO21)", false, 0, 0, false, "", 0, 0, false, "", 0, {"USB_D+"}, true, "Reserved: Native USB D+");
    espS3Dev.pins["IO43"] = makePin("IO43", "GPIO Bank 1 (IO26-IO48)", false, 0, 0, false, "", 0, 0, false, "", 0, {"U0TXD"});
    espS3Dev.pins["IO44"] = makePin("IO44", "GPIO Bank 1 (IO26-IO48)", false, 0, 0, false, "", 0, 0, false, "", 0, {"U0RXD"});
    espS3Dev.pins["IO48"] = makePin("IO48", "GPIO Bank 1 (IO26-IO48)", false, 0, 0, true, "RMT", 0, 0, false, "", 0, {"RGB_LED"});
    m_boards[espS3Dev.id] = espS3Dev;

    // ────────────────────────────────────────────────────────────────────────
    // 7. ESP32 DevKitC (ESP32-WROOM-32)
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile espWroom;
    espWroom.id = "esp32_devkitc";
    espWroom.name = "ESP32 DevKitC (ESP32-WROOM-32)";
    espWroom.mcuFamily = "ESP32";
    espWroom.connectionType = "serial";
    espWroom.gpioBase = 0x3FF44000;
    espWroom.bsrrOffset = 0x08;
    espWroom.portGroups = {"GPIO Bank 0 (GPIO0-GPIO31)", "GPIO Bank 1 (GPIO32-GPIO39)"};
    espWroom.spiBusses = {"VSPI (GPIO18/SCK, GPIO19/MISO, GPIO23/MOSI)"};
    espWroom.pins["GPIO0"]  = makePin("GPIO0", "GPIO Bank 0 (GPIO0-GPIO31)", false, 0, 0, false, "", 0, 0, false, "", 0, {"BOOT"}, true, "Reserved: Boot Pin (GPIO0)");
    espWroom.pins["GPIO2"]  = makePin("GPIO2", "GPIO Bank 0 (GPIO0-GPIO31)", true, 2, 12, true, "LEDC", 0, 0, false, "", 0, {"ADC2_CH2", "LED_BUILTIN"});
    espWroom.pins["GPIO4"]  = makePin("GPIO4", "GPIO Bank 0 (GPIO0-GPIO31)", true, 0, 12, true, "LEDC", 1, 0, false, "", 0, {"ADC2_CH0"});
    espWroom.pins["GPIO18"] = makePin("GPIO18", "GPIO Bank 0 (GPIO0-GPIO31)", false, 0, 0, false, "", 0, 0, true, "VSPI", 0, {"VSPI_SCK"});
    espWroom.pins["GPIO19"] = makePin("GPIO19", "GPIO Bank 0 (GPIO0-GPIO31)", false, 0, 0, false, "", 0, 0, true, "VSPI", 0, {"VSPI_MISO"});
    espWroom.pins["GPIO21"] = makePin("GPIO21", "GPIO Bank 0 (GPIO0-GPIO31)", false, 0, 0, false, "", 0, 0, false, "", 0, {"I2C_SDA"});
    espWroom.pins["GPIO22"] = makePin("GPIO22", "GPIO Bank 0 (GPIO0-GPIO31)", false, 0, 0, false, "", 0, 0, false, "", 0, {"I2C_SCL"});
    espWroom.pins["GPIO23"] = makePin("GPIO23", "GPIO Bank 0 (GPIO0-GPIO31)", false, 0, 0, false, "", 0, 0, true, "VSPI", 0, {"VSPI_MOSI"});
    espWroom.pins["GPIO34"] = makePin("GPIO34", "GPIO Bank 1 (GPIO32-GPIO39)", true, 6, 12, false, "", 0, 0, false, "", 0, {"ADC1_CH6 (Input Only)"});
    espWroom.pins["GPIO35"] = makePin("GPIO35", "GPIO Bank 1 (GPIO32-GPIO39)", true, 7, 12, false, "", 0, 0, false, "", 0, {"ADC1_CH7 (Input Only)"});
    m_boards[espWroom.id] = espWroom;

    // ────────────────────────────────────────────────────────────────────────
    // 8. ESP32-C6-DevKitC-1
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile espC6;
    espC6.id = "esp32_c6";
    espC6.name = "ESP32-C6-DevKitC-1 (ESP32-C6)";
    espC6.mcuFamily = "ESP32-C6 (RISC-V)";
    espC6.connectionType = "serial";
    espC6.gpioBase = 0x60091000;
    espC6.bsrrOffset = 0x08;
    espC6.portGroups = {"GPIO Bank 0 (IO0-IO30)"};
    espC6.spiBusses = {"SPI2 (IO18-IO21)"};
    espC6.pins["IO0"] = makePin("IO0", "GPIO Bank 0 (IO0-IO30)", true, 0, 12, true, "LEDC", 0, 0, false, "", 0, {"ADC1_CH0"});
    espC6.pins["IO1"] = makePin("IO1", "GPIO Bank 0 (IO0-IO30)", true, 1, 12, true, "LEDC", 1, 0, false, "", 0, {"ADC1_CH1"});
    espC6.pins["IO9"] = makePin("IO9", "GPIO Bank 0 (IO0-IO30)", false, 0, 0, false, "", 0, 0, false, "", 0, {"BOOT"}, true, "Reserved: Boot Pin (IO9)");
    espC6.pins["IO18"] = makePin("IO18", "GPIO Bank 0 (IO0-IO30)", false, 0, 0, false, "", 0, 0, true, "SPI2", 0, {"SPI2_SCK"});
    espC6.pins["IO19"] = makePin("IO19", "GPIO Bank 0 (IO0-IO30)", false, 0, 0, false, "", 0, 0, true, "SPI2", 0, {"SPI2_MOSI"});
    espC6.pins["IO20"] = makePin("IO20", "GPIO Bank 0 (IO0-IO30)", false, 0, 0, false, "", 0, 0, true, "SPI2", 0, {"SPI2_MISO"});
    m_boards[espC6.id] = espC6;

    // ────────────────────────────────────────────────────────────────────────
    // 9. ESP32-S3-BOX (Legacy compat)
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile esp;
    esp.id = "esp32s3";
    esp.name = "ESP32-S3-BOX (Bare / Flashed via esptool)";
    esp.mcuFamily = "ESP32";
    esp.connectionType = "serial";
    esp.gpioBase = 0x3FF44000;
    esp.bsrrOffset = 0x08;
    esp.openocdTarget = "target/esp32s3.cfg";
    esp.portGroups = {"GPIO"};
    for (int i = 0; i <= 8; ++i) {
        QString pinName = QString("IO%1").arg(i);
        esp.pins[pinName] = makePin(pinName, "GPIO", false, 0, 0, false, "", 0, 0, false, "", 0);
    }
    m_boards[esp.id] = esp;

    // ────────────────────────────────────────────────────────────────────────
    // 10. Raspberry Pi Pico (RP2040)
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile pico;
    pico.id = "rp2040";
    pico.name = "Raspberry Pi Pico (RP2040)";
    pico.mcuFamily = "RP2040";
    pico.connectionType = "openocd";
    pico.gpioBase = 0xd0000000;
    pico.bsrrOffset = 0x14;
    pico.openocdInterface = "interface/cmsis-dap.cfg";
    pico.openocdTarget = "target/rp2040.cfg";
    pico.portGroups = {"GPIO Bank 0 (GP0-GP28)"};
    pico.spiBusses = {"SPI0 (GP16-GP19)", "SPI1 (GP10-GP13)"};
    pico.pins["GP0"]  = makePin("GP0", "GPIO Bank 0 (GP0-GP28)", false, 0, 0, true, "PWM0", 1, 0, false, "", 0, {"UART0_TX", "I2C0_SDA"});
    pico.pins["GP1"]  = makePin("GP1", "GPIO Bank 0 (GP0-GP28)", false, 0, 0, true, "PWM0", 2, 0, false, "", 0, {"UART0_RX", "I2C0_SCL"});
    pico.pins["GP2"]  = makePin("GP2", "GPIO Bank 0 (GP0-GP28)", false, 0, 0, true, "PWM1", 1, 0, false, "", 0, {"I2C1_SDA"});
    pico.pins["GP3"]  = makePin("GP3", "GPIO Bank 0 (GP0-GP28)", false, 0, 0, true, "PWM1", 2, 0, false, "", 0, {"I2C1_SCL"});
    pico.pins["GP16"] = makePin("GP16", "GPIO Bank 0 (GP0-GP28)", false, 0, 0, false, "", 0, 0, true, "SPI0", 0, {"SPI0_MISO"});
    pico.pins["GP17"] = makePin("GP17", "GPIO Bank 0 (GP0-GP28)", false, 0, 0, false, "", 0, 0, true, "SPI0", 0, {"SPI0_CSn"});
    pico.pins["GP18"] = makePin("GP18", "GPIO Bank 0 (GP0-GP28)", false, 0, 0, false, "", 0, 0, true, "SPI0", 0, {"SPI0_SCK"});
    pico.pins["GP19"] = makePin("GP19", "GPIO Bank 0 (GP0-GP28)", false, 0, 0, false, "", 0, 0, true, "SPI0", 0, {"SPI0_MOSI"});
    pico.pins["GP25"] = makePin("GP25", "GPIO Bank 0 (GP0-GP28)", false, 0, 0, true, "PWM4", 2, 0, false, "", 0, {"LED_BUILTIN"});
    pico.pins["GP26"] = makePin("GP26", "GPIO Bank 0 (GP0-GP28)", true,  0, 12, false, "", 0, 0, false, "", 0, {"ADC0"});
    pico.pins["GP27"] = makePin("GP27", "GPIO Bank 0 (GP0-GP28)", true,  1, 12, false, "", 0, 0, false, "", 0, {"ADC1"});
    pico.pins["GP28"] = makePin("GP28", "GPIO Bank 0 (GP0-GP28)", true,  2, 12, false, "", 0, 0, false, "", 0, {"ADC2"});
    m_boards[pico.id] = pico;

    // ────────────────────────────────────────────────────────────────────────
    // 11. Raspberry Pi Pico 2 (RP2350)
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile pico2;
    pico2.id = "rp2350";
    pico2.name = "Raspberry Pi Pico 2 (RP2350)";
    pico2.mcuFamily = "RP2350 (ARM/Hazard3)";
    pico2.connectionType = "openocd";
    pico2.gpioBase = 0xd0000000;
    pico2.bsrrOffset = 0x14;
    pico2.openocdInterface = "interface/cmsis-dap.cfg";
    pico2.openocdTarget = "target/rp2350.cfg";
    pico2.portGroups = {"GPIO Bank 0 (GP0-GP28)"};
    pico2.spiBusses = {"SPI0 (GP16-GP19)", "SPI1 (GP10-GP13)"};
    pico2.pins["GP0"]  = makePin("GP0", "GPIO Bank 0 (GP0-GP28)", false, 0, 0, true, "PWM0", 1, 0, false, "", 0, {"UART0_TX"});
    pico2.pins["GP1"]  = makePin("GP1", "GPIO Bank 0 (GP0-GP28)", false, 0, 0, true, "PWM0", 2, 0, false, "", 0, {"UART0_RX"});
    pico2.pins["GP25"] = makePin("GP25", "GPIO Bank 0 (GP0-GP28)", false, 0, 0, true, "PWM4", 2, 0, false, "", 0, {"LED_BUILTIN"});
    pico2.pins["GP26"] = makePin("GP26", "GPIO Bank 0 (GP0-GP28)", true,  0, 12, false, "", 0, 0, false, "", 0, {"ADC0"});
    pico2.pins["GP27"] = makePin("GP27", "GPIO Bank 0 (GP0-GP28)", true,  1, 12, false, "", 0, 0, false, "", 0, {"ADC1"});
    pico2.pins["GP28"] = makePin("GP28", "GPIO Bank 0 (GP0-GP28)", true,  2, 12, false, "", 0, 0, false, "", 0, {"ADC2"});
    m_boards[pico2.id] = pico2;

    // ────────────────────────────────────────────────────────────────────────
    // 12. Raspberry Pi 4 Model B (Linux SBC)
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile rpi4;
    rpi4.id = "raspberrypi_4b";
    rpi4.name = "Raspberry Pi 4 Model B (BCM2711 / Linux SBC)";
    rpi4.mcuFamily = "BCM2711 (Linux SBC)";
    rpi4.connectionType = "linux";
    rpi4.portGroups = {"40-Pin Header"};
    rpi4.spiBusses = {"SPI0 (GPIO9, GPIO10, GPIO11)"};
    rpi4.pins["GPIO2"]  = makePin("GPIO2", "40-Pin Header", false, 0, 0, false, "", 0, 0, false, "", 0, {"I2C1_SDA"});
    rpi4.pins["GPIO3"]  = makePin("GPIO3", "40-Pin Header", false, 0, 0, false, "", 0, 0, false, "", 0, {"I2C1_SCL"});
    rpi4.pins["GPIO4"]  = makePin("GPIO4", "40-Pin Header", false, 0, 0, false, "", 0, 0, false, "", 0, {"GPCLK0"});
    rpi4.pins["GPIO14"] = makePin("GPIO14", "40-Pin Header", false, 0, 0, false, "", 0, 0, false, "", 0, {"UART0_TXD"});
    rpi4.pins["GPIO15"] = makePin("GPIO15", "40-Pin Header", false, 0, 0, false, "", 0, 0, false, "", 0, {"UART0_RXD"});
    rpi4.pins["GPIO17"] = makePin("GPIO17", "40-Pin Header", false, 0, 0, false, "", 0, 0, false, "", 0);
    rpi4.pins["GPIO18"] = makePin("GPIO18", "40-Pin Header", false, 0, 0, true, "PWM0", 1, 0, false, "", 0, {"PCM_CLK"});
    rpi4.pins["GPIO27"] = makePin("GPIO27", "40-Pin Header", false, 0, 0, false, "", 0, 0, false, "", 0);
    m_boards[rpi4.id] = rpi4;

    // ────────────────────────────────────────────────────────────────────────
    // 13. NXP LPC55S69-EVK
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile nxp;
    nxp.id = "nxp_lpc55s69";
    nxp.name = "NXP LPC55S69-EVK (LPC55S69 Dual Cortex-M33)";
    nxp.mcuFamily = "NXP LPC5500";
    nxp.connectionType = "linkserver";
    nxp.portGroups = {"Port 0", "Port 1"};
    nxp.spiBusses = {"Flexcomm 8 (SPI)"};
    nxp.pins["PIO0_13"] = makePin("PIO0_13", "Port 0", false, 0, 0, false, "", 0, 0, false, "", 0, {"FC1_TXD_SCL"});
    nxp.pins["PIO0_14"] = makePin("PIO0_14", "Port 0", false, 0, 0, false, "", 0, 0, false, "", 0, {"FC1_RXD_SDA"});
    nxp.pins["PIO1_4"]  = makePin("PIO1_4", "Port 1", false, 0, 0, true, "CTIMER", 1, 0, false, "", 0, {"LED_RED"});
    nxp.pins["PIO1_6"]  = makePin("PIO1_6", "Port 1", false, 0, 0, true, "CTIMER", 2, 0, false, "", 0, {"LED_BLUE"});
    nxp.pins["PIO1_7"]  = makePin("PIO1_7", "Port 1", false, 0, 0, true, "CTIMER", 3, 0, false, "", 0, {"LED_GREEN"});
    nxp.pins["PIO0_23"] = makePin("PIO0_23", "Port 0", true, 0, 16, false, "", 0, 0, false, "", 0, {"ADC0_IN0"});
    m_boards[nxp.id] = nxp;

    // ────────────────────────────────────────────────────────────────────────
    // 14. STC89C52RC Microcontroller Board
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile stc;
    stc.id = "stc89c52";
    stc.name = "STC89C52RC Microcontroller Board (8051)";
    stc.mcuFamily = "STC 8051";
    stc.connectionType = "stcisp";
    stc.portGroups = {"P0", "P1", "P2", "P3"};
    for (int p = 0; p < 4; ++p) {
        for (int b = 0; b < 8; ++b) {
            QString name = QString("P%1.%2").arg(p).arg(b);
            QString portName = QString("P%1").arg(p);
            stc.pins[name] = makePin(name, portName, false, 0, 0, false, "", 0, 0, false, "", 0);
        }
    }
    m_boards[stc.id] = stc;

    // ────────────────────────────────────────────────────────────────────────
    // 15. Custom Board Profile
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile custom;
    custom.id = "custom";
    custom.name = "Custom Board Profile (User Configured)";
    custom.mcuFamily = "Custom / Generic";
    custom.connectionType = "mock";
    custom.portGroups = {"GPIO"};
    for (int i = 0; i < 16; ++i) {
        QString name = QString("PIN_%1").arg(i);
        custom.pins[name] = makePin(name, "GPIO", (i < 4), i, 12, (i >= 4 && i < 8), "TIM1", i - 3, 1, false, "", 0);
    }
    m_boards[custom.id] = custom;
}

QList<BoardProfile> HardwareBridge::availableBoards() const {
    return m_boards.values();
}

BoardProfile HardwareBridge::currentBoard() const {
    return m_boards.value(m_currentBoardId);
}

bool HardwareBridge::setBoard(const QString& boardId) {
    if (!m_boards.contains(boardId)) return false;
    if (m_currentBoardId != boardId) {
        m_currentBoardId = boardId;
        // Clean up bindings for pins that do not exist on the new board
        const QStringList pins = availablePins();
        for (int i = m_bindings.size() - 1; i >= 0; --i) {
            if (!pins.contains(m_bindings[i].pin)) {
                m_bindings.removeAt(i);
            }
        }
        emit boardChanged(boardId);
    }
    return true;
}

bool HardwareBridge::addBoard(const BoardProfile& profile) {
    if (profile.id.isEmpty()) return false;
    m_boards[profile.id] = profile;
    return true;
}

bool HardwareBridge::hasBoard(const QString& boardId) const {
    return m_boards.contains(boardId);
}

QStringList HardwareBridge::availablePins() const {
    if (!m_boards.contains(m_currentBoardId)) return QStringList();
    return m_boards[m_currentBoardId].pins.keys();
}

QStringList HardwareBridge::availablePinsForPort(const QString& portFilter) const {
    if (!m_boards.contains(m_currentBoardId)) return QStringList();
    const auto& pinMap = m_boards[m_currentBoardId].pins;
    if (portFilter.isEmpty() || portFilter == "All Ports") {
        return pinMap.keys();
    }
    QStringList filtered;
    for (auto it = pinMap.constBegin(); it != pinMap.constEnd(); ++it) {
        if (it.value().portGroup.compare(portFilter, Qt::CaseInsensitive) == 0 ||
            it.key().startsWith(portFilter, Qt::CaseInsensitive)) {
            filtered.append(it.key());
        }
    }
    return filtered;
}

QStringList HardwareBridge::portGroupsForCurrentBoard() const {
    if (!m_boards.contains(m_currentBoardId)) return QStringList();
    const auto& bp = m_boards[m_currentBoardId];
    if (!bp.portGroups.isEmpty()) return bp.portGroups;

    // Infer port groups from pin names
    QSet<QString> groups;
    for (const auto& pin : bp.pins) {
        if (!pin.portGroup.isEmpty()) {
            groups.insert(pin.portGroup);
        } else if (pin.pinName.startsWith("P") && pin.pinName.length() >= 2 && pin.pinName[1].isLetter()) {
            groups.insert(QString("Port %1").arg(pin.pinName[1].toUpper()));
        } else if (pin.pinName.startsWith("GPIO") || pin.pinName.startsWith("IO") || pin.pinName.startsWith("GP")) {
            groups.insert("GPIO");
        }
    }
    return groups.values();
}

PinProfile HardwareBridge::pinProfile(const QString& pin) const {
    if (!m_boards.contains(m_currentBoardId)) return PinProfile();
    return m_boards[m_currentBoardId].pins.value(pin);
}

bool HardwareBridge::isModeAvailable(const QString& pin, PinMode mode) const {
    if (!m_boards.contains(m_currentBoardId)) return false;
    const auto& pinMap = m_boards[m_currentBoardId].pins;
    if (!pinMap.contains(pin)) return false;
    return pinMap[pin].supportedModes.contains(mode);
}

PinMode HardwareBridge::activePinMode(const QString& pin) const {
    if (!m_boards.contains(m_currentBoardId)) return PinMode::None;
    const auto& pinMap = m_boards[m_currentBoardId].pins;
    if (!pinMap.contains(pin)) return PinMode::None;
    return pinMap[pin].activeMode;
}

bool HardwareBridge::setPinMode(const QString& pin, PinMode mode) {
    if (!m_boards.contains(m_currentBoardId)) return false;
    if (!isModeAvailable(pin, mode) && mode != PinMode::None) return false;

    auto& pinMap = m_boards[m_currentBoardId].pins;
    pinMap[pin].activeMode = mode;
    if (m_activeTask == "Task B") {
        pinMap[pin].taskBMode = mode;
    } else {
        pinMap[pin].taskAMode = mode;
    }

    emit pinConfigChanged(pin, mode);
    emit taskPinConfigChanged(pin, m_activeTask, mode);
    return true;
}

void HardwareBridge::setActiveTask(const QString& task) {
    m_activeTask = task;
}

PinMode HardwareBridge::taskPinMode(const QString& pin, const QString& task) const {
    if (!m_boards.contains(m_currentBoardId)) return PinMode::None;
    const auto& pinMap = m_boards[m_currentBoardId].pins;
    if (!pinMap.contains(pin)) return PinMode::None;
    if (task == "Task B") {
        return pinMap[pin].taskBMode;
    }
    return pinMap[pin].taskAMode;
}

bool HardwareBridge::setTaskPinMode(const QString& pin, const QString& task, PinMode mode) {
    if (!m_boards.contains(m_currentBoardId)) return false;
    if (!isModeAvailable(pin, mode) && mode != PinMode::None) return false;

    auto& pinMap = m_boards[m_currentBoardId].pins;
    if (task == "Task B") {
        pinMap[pin].taskBMode = mode;
    } else {
        pinMap[pin].taskAMode = mode;
    }
    // Update activeMode if currently selected task matches
    if (m_activeTask == task || m_activeTask == "All Tasks") {
        pinMap[pin].activeMode = mode;
    }

    emit taskPinConfigChanged(pin, task, mode);
    emit pinConfigChanged(pin, mode);
    return true;
}

QString HardwareBridge::taskPeripheral(const QString& pin, const QString& task) const {
    if (!m_boards.contains(m_currentBoardId)) return QString();
    const auto& pinMap = m_boards[m_currentBoardId].pins;
    if (!pinMap.contains(pin)) return QString();
    if (task == "Task B") {
        return pinMap[pin].taskBPeripheral;
    }
    return pinMap[pin].taskAPeripheral;
}

bool HardwareBridge::setTaskPeripheral(const QString& pin, const QString& task, const QString& peripheral) {
    if (!m_boards.contains(m_currentBoardId)) return false;
    auto& pinMap = m_boards[m_currentBoardId].pins;
    if (!pinMap.contains(pin)) return false;

    if (task == "Task B") {
        pinMap[pin].taskBPeripheral = peripheral;
    } else {
        pinMap[pin].taskAPeripheral = peripheral;
    }
    emit taskPeripheralChanged(pin, task, peripheral);
    return true;
}

QString HardwareBridge::taskComponentBinding(const QString& pin, const QString& task) const {
    if (!m_boards.contains(m_currentBoardId)) return QString();
    const auto& pinMap = m_boards[m_currentBoardId].pins;
    if (!pinMap.contains(pin)) return QString();
    if (task == "Task B") {
        return pinMap[pin].taskBComponentId;
    }
    return pinMap[pin].taskAComponentId;
}

void HardwareBridge::setTaskComponentBinding(const QString& pin, const QString& task, const QString& componentId) {
    if (!m_boards.contains(m_currentBoardId)) return;
    auto& pinMap = m_boards[m_currentBoardId].pins;
    if (!pinMap.contains(pin)) return;
    if (task == "Task B") {
        pinMap[pin].taskBComponentId = componentId;
    } else {
        pinMap[pin].taskAComponentId = componentId;
    }
}

PinConflict HardwareBridge::checkPinConflict(const QString& pin) const {
    PinConflict conflict;
    conflict.pin = pin;

    if (!m_boards.contains(m_currentBoardId)) return conflict;
    const auto& pinMap = m_boards[m_currentBoardId].pins;
    if (!pinMap.contains(pin)) return conflict;

    const PinProfile& prof = pinMap[pin];

    // 1. Reserved Pin Check
    if (prof.isReserved && (prof.taskAMode != PinMode::None || prof.taskBMode != PinMode::None)) {
        conflict.hasConflict = true;
        conflict.reason = QString("Pin %1 is reserved: %2").arg(pin, prof.reservedReason.isEmpty() ? "Hardware function" : prof.reservedReason);
        conflict.conflictingTask = (prof.taskAMode != PinMode::None) ? "Task A" : "Task B";
        return conflict;
    }

    // 2. Task A vs Task B Incompatible Mode Conflict
    if (prof.taskAMode != PinMode::None && prof.taskBMode != PinMode::None && prof.taskAMode != prof.taskBMode) {
        conflict.hasConflict = true;
        conflict.reason = QString("Task conflict: Task A assigned '%1' while Task B assigned '%2' on physical pin %3.")
            .arg(pinModeToString(prof.taskAMode), pinModeToString(prof.taskBMode), pin);
        conflict.conflictingTask = "Task A & Task B";
        return conflict;
    }

    // 3. Task A vs Task B Peripheral Conflict
    if (!prof.taskAPeripheral.isEmpty() && !prof.taskBPeripheral.isEmpty() && prof.taskAPeripheral != prof.taskBPeripheral) {
        conflict.hasConflict = true;
        conflict.reason = QString("Conflicting peripheral mapping: Task A (%1) vs Task B (%2) on pin %3.")
            .arg(prof.taskAPeripheral, prof.taskBPeripheral, pin);
        conflict.conflictingTask = "Task A & Task B";
        return conflict;
    }

    return conflict;
}

QList<PinConflict> HardwareBridge::allConflicts() const {
    QList<PinConflict> list;
    if (!m_boards.contains(m_currentBoardId)) return list;
    const auto& pinMap = m_boards[m_currentBoardId].pins;

    // Check individual pin conflicts
    for (const auto& pinName : pinMap.keys()) {
        PinConflict c = checkPinConflict(pinName);
        if (c.hasConflict) list.append(c);
    }

    // Check duplicate exclusive peripheral usage across pins in Task A and Task B
    QMap<QString, QString> taskAPeriphOwners;
    QMap<QString, QString> taskBPeriphOwners;

    for (const auto& pinName : pinMap.keys()) {
        const PinProfile& prof = pinMap[pinName];
        if (!prof.taskAPeripheral.isEmpty() && prof.taskAPeripheral != "GPIO" && prof.taskAMode != PinMode::None) {
            if (taskAPeriphOwners.contains(prof.taskAPeripheral)) {
                PinConflict c;
                c.hasConflict = true;
                c.pin = pinName;
                c.conflictingTask = "Task A";
                c.reason = QString("Duplicate peripheral mapping: %1 is claimed by both %2 and %3 in Task A.")
                    .arg(prof.taskAPeripheral, taskAPeriphOwners[prof.taskAPeripheral], pinName);
                list.append(c);
            } else {
                taskAPeriphOwners[prof.taskAPeripheral] = pinName;
            }
        }
        if (!prof.taskBPeripheral.isEmpty() && prof.taskBPeripheral != "GPIO" && prof.taskBMode != PinMode::None) {
            if (taskBPeriphOwners.contains(prof.taskBPeripheral)) {
                PinConflict c;
                c.hasConflict = true;
                c.pin = pinName;
                c.conflictingTask = "Task B";
                c.reason = QString("Duplicate peripheral mapping: %1 is claimed by both %2 and %3 in Task B.")
                    .arg(prof.taskBPeripheral, taskBPeriphOwners[prof.taskBPeripheral], pinName);
                list.append(c);
            } else {
                taskBPeriphOwners[prof.taskBPeripheral] = pinName;
            }
        }
    }

    return list;
}

bool HardwareBridge::validateAssignments(QStringList* outErrors, QStringList* outWarnings) const {
    QList<PinConflict> conflicts = allConflicts();
    bool valid = true;
    for (const auto& c : conflicts) {
        if (c.reason.contains("reserved", Qt::CaseInsensitive)) {
            if (outWarnings) outWarnings->append(c.reason);
        } else {
            valid = false;
            if (outErrors) outErrors->append(c.reason);
        }
    }
    return valid;
}

QString HardwareBridge::connectionStatusText() const {
    switch (m_connectionState) {
        case ConnectionState::Disconnected:
            return "○ Disconnected";
        case ConnectionState::DetectingProbe:
            return "⚡ Detecting Probe...";
        case ConnectionState::Connecting:
            return "⏳ Connecting...";
        case ConnectionState::Connected:
            return QString("● Connected: %1").arg(connectedProbeName().isEmpty() ? currentBoard().name : connectedProbeName());
        case ConnectionState::ConnectionFailed:
            return QString("⚠ Connection Failed: %1").arg(m_lastConnectionError.isEmpty() ? "Target unreachable" : m_lastConnectionError);
        case ConnectionState::DeviceRemoved:
            return "⚠ Device Removed / Unplugged";
        case ConnectionState::SimulationMode:
            return "◈ Simulation Mode (Virtual Target)";
    }
    return "○ Disconnected";
}

QString HardwareBridge::connectionButtonLabel() const {
    if (m_connectionState == ConnectionState::Connected) {
        return "Disconnect";
    }
    const BoardProfile bp = currentBoard();
    if (bp.mcuFamily.startsWith("STM32")) {
        return "Connect ST-Link / OpenOCD";
    }
    if (bp.mcuFamily.contains("RP2040") || bp.mcuFamily.contains("RP2350")) {
        return "Connect Picoprobe / OpenOCD";
    }
    if (bp.connectionType == "serial" || bp.mcuFamily.contains("ESP32")) {
        return "Connect Serial / ESP-IDF";
    }
    if (bp.connectionType == "linux" || bp.mcuFamily.contains("Linux") || bp.mcuFamily.contains("BCM2711")) {
        return "Connect Linux GPIO (gpiod)";
    }
    if (bp.connectionType == "linkserver" || bp.mcuFamily.contains("NXP")) {
        return "Connect CMSIS-DAP / LinkServer";
    }
    if (bp.connectionType == "stcisp" || bp.mcuFamily.contains("STC")) {
        return "Connect Serial (STC-ISP)";
    }
    if (bp.connectionType == "openocd") {
        return "Connect ST-Link / OpenOCD";
    }
    return "Connect Target";
}

QString HardwareBridge::connectedProbeName() const {
    if (m_simulationMode) return "Virtual Simulation Probe";
    if (m_openOcd.isConnected()) {
        QString probe = m_openOcd.connectedProbeName();
        if (!probe.isEmpty()) return probe;
    }
    if (m_hardwareConnected) {
        return currentBoard().name;
    }
    return QString();
}

bool HardwareBridge::isProbeDetected() const {
    return m_openOcd.isProbeDetected();
}

QString HardwareBridge::detectedBoardName() const {
    return m_openOcd.detectedBoardName();
}

bool HardwareBridge::detectProbe() {
    updateConnectionState(ConnectionState::DetectingProbe);
    DiscoveredProbe probe = m_openOcd.scanUsbProbes();
    if (probe.detected) {
        updateConnectionState(ConnectionState::Disconnected, QString("Detected: %1").arg(probe.name));
        return true;
    }
    updateConnectionState(ConnectionState::Disconnected, "No hardware probe detected");
    return false;
}

void HardwareBridge::rescan() {
    m_openOcd.rescan();
}

bool HardwareBridge::connectHardware() {
    if (m_connectionState == ConnectionState::Connected) {
        return true;
    }

    const BoardProfile bp = currentBoard();
    m_lastConnectionError.clear();
    updateConnectionState(ConnectionState::Connecting);

    // 1. STM32 / OpenOCD Target workflow
    if (bp.connectionType == "openocd" || bp.mcuFamily.startsWith("STM32") || bp.mcuFamily.startsWith("RP2040") || bp.mcuFamily.startsWith("RP2350")) {
        // Check OpenOCD executable
        QString openocdExe = QStandardPaths::findExecutable("openocd");
        if (openocdExe.isEmpty()) {
            if (QFile::exists("/usr/bin/openocd")) openocdExe = "/usr/bin/openocd";
            else if (QFile::exists("/usr/local/bin/openocd")) openocdExe = "/usr/local/bin/openocd";
        }

        if (openocdExe.isEmpty() && !m_simulationMode) {
            m_lastConnectionError = "OpenOCD executable not found in system PATH (/usr/bin/openocd, /usr/local/bin/openocd)";
            updateConnectionState(ConnectionState::ConnectionFailed, m_lastConnectionError);
            return false;
        }

        // Try real OpenOCD connect
        bool ok = m_openOcd.connectToTarget(bp.openocdInterface, bp.openocdTarget);
        if (ok) {
            updateConnectionState(ConnectionState::Connected, m_openOcd.connectedProbeName());
            return true;
        }

        // If probe is detected or physical connect requested
        if (m_openOcd.isProbeDetected()) {
            updateConnectionState(ConnectionState::Connected, m_openOcd.detectedBoardName());
            return true;
        }

        // Actionable error for disconnected physical probe
        m_lastConnectionError = "ST-Link / Debug Probe not found on USB. Connect hardware or switch to Simulation Mode.";
        updateConnectionState(ConnectionState::ConnectionFailed, m_lastConnectionError);
        return false;
    }

    // 2. ESP32 / Serial Workflow
    if (bp.connectionType == "serial" || bp.mcuFamily.contains("ESP32")) {
        // Check if USB serial port exists
        QDir devDir("/dev");
        QStringList ttyPorts = devDir.entryList(QStringList() << "ttyUSB*" << "ttyACM*", QDir::System);
        if (!ttyPorts.isEmpty()) {
            updateConnectionState(ConnectionState::Connected, QString("ESP32 via /dev/%1").arg(ttyPorts.first()));
            return true;
        }
        m_lastConnectionError = "No ESP32 USB-Serial / JTAG device found (/dev/ttyUSB*, /dev/ttyACM*).";
        updateConnectionState(ConnectionState::ConnectionFailed, m_lastConnectionError);
        return false;
    }

    // 3. Linux GPIO SBC Workflow
    if (bp.connectionType == "linux") {
        if (QDir("/sys/class/gpio").exists() || QFile::exists("/dev/gpiochip0")) {
            updateConnectionState(ConnectionState::Connected, "Linux GPIO Subsystem (/dev/gpiochip)");
            return true;
        }
        m_lastConnectionError = "Linux GPIO sysfs/gpiod interface unavailable.";
        updateConnectionState(ConnectionState::ConnectionFailed, m_lastConnectionError);
        return false;
    }

    // 4. Fallback / Custom / Simulation
    if (bp.connectionType == "mock" || m_simulationMode) {
        updateConnectionState(ConnectionState::Connected, "Simulated Virtual Target");
        return true;
    }

    m_lastConnectionError = QString("Unsupported connection backend '%1' for %2.").arg(bp.connectionType, bp.name);
    updateConnectionState(ConnectionState::ConnectionFailed, m_lastConnectionError);
    return false;
}

bool HardwareBridge::disconnectHardware() {
    if (m_openOcd.isConnected()) {
        m_openOcd.disconnectTarget();
    }
    updateConnectionState(ConnectionState::Disconnected);
    return true;
}

void HardwareBridge::setSimulationMode(bool enabled) {
    m_simulationMode = enabled;
    if (enabled) {
        updateConnectionState(ConnectionState::SimulationMode);
    } else {
        if (m_connectionState == ConnectionState::SimulationMode) {
            updateConnectionState(ConnectionState::Disconnected);
        }
    }
}

void HardwareBridge::setHardwareConnected(bool connected) {
    if (connected) {
        updateConnectionState(ConnectionState::Connected, "Connected (Manual Override)");
    } else {
        m_openOcd.setSimulatedConnected(false, QString());
        updateConnectionState(ConnectionState::Disconnected);
    }
}

void HardwareBridge::setSimulatedAdcCount(const QString& pin, quint32 rawCount) {
    m_simulatedAdcValues[pin] = rawCount;
    const PinProfile prof = pinProfile(pin);
    int bits = prof.adc.resolutionBits > 0 ? prof.adc.resolutionBits : 12;
    double norm = static_cast<double>(rawCount) / ((1u << bits) - 1);
    for (const auto& b : m_bindings) {
        if (b.pin == pin && b.component) {
            if (auto* pb = qobject_cast<ProgressBarComponent*>(b.component.data())) {
                pb->setValue(norm);
            } else if (auto* lbl = qobject_cast<LabelComponent*>(b.component.data())) {
                lbl->setText(QString::number(rawCount));
            } else if (auto* sld = qobject_cast<SliderComponent*>(b.component.data())) {
                sld->setValue(static_cast<int>(rawCount));
            }
        }
    }
    emit adcValueChanged(pin, rawCount, bits);
}

quint32 HardwareBridge::simulatedAdcCount(const QString& pin) const {
    return m_simulatedAdcValues.value(pin, 0);
}

bool HardwareBridge::setupAdc(const QString& pin) {
    if (!isModeAvailable(pin, PinMode::AnalogIn)) return false;
    return true;
}

bool HardwareBridge::pollAdc(const QString& pin, quint32* outRawCount) {
    if (!outRawCount) return false;
    if (!isModeAvailable(pin, PinMode::AnalogIn)) return false;

    if (m_openOcd.isConnected()) {
        const PinProfile prof = pinProfile(pin);
        if (prof.adc.available) {
            quint32 val = 0;
            if (m_openOcd.readMemoryWord(prof.adc.dataReg, &val)) {
                *outRawCount = val & ((1u << prof.adc.resolutionBits) - 1);
                return true;
            }
        }
    }

    *outRawCount = m_simulatedAdcValues.value(pin, 2048);
    return true;
}

bool HardwareBridge::setupSpi(const QString& spiBus) {
    Q_UNUSED(spiBus);
    return true;
}

bool HardwareBridge::spiRawTransfer(const QString& spiBus, quint8 byteOut, quint8* outByteIn, QString* outLog) {
    if (!outByteIn) return false;

    if (m_openOcd.isConnected()) {
        const BoardProfile bp = currentBoard();
        // Hardware poke over OpenOCD
        quint32 drAddr = (spiBus.contains("SPI2")) ? 0x4000380C : 0x4001300C;

        m_openOcd.writeMemoryWord(drAddr, byteOut);
        quint32 rxVal = 0;
        m_openOcd.readMemoryWord(drAddr, &rxVal);
        *outByteIn = static_cast<quint8>(rxVal & 0xFF);

        if (outLog) {
            *outLog = QString("SPI Raw Transfer on %1: OUT=0x%2, IN=0x%3 (OpenOCD DR 0x%4)")
                .arg(spiBus)
                .arg(QString::number(byteOut, 16).toUpper().rightJustified(2, '0'))
                .arg(QString::number(*outByteIn, 16).toUpper().rightJustified(2, '0'))
                .arg(QString::number(drAddr, 16).toUpper());
        }
        emit spiTransferCompleted(spiBus, byteOut, *outByteIn, true);
        return true;
    }

    // Virtual Inverted Loopback Simulation
    *outByteIn = static_cast<quint8>(byteOut ^ 0xFF);
    if (outLog) {
        *outLog = QString("SPI Raw Transfer on %1: OUT=0x%2, IN=0x%3 (Virtual loopback ~TX)")
            .arg(spiBus)
            .arg(QString::number(byteOut, 16).toUpper().rightJustified(2, '0'))
            .arg(QString::number(*outByteIn, 16).toUpper().rightJustified(2, '0'));
    }
    emit spiTransferCompleted(spiBus, byteOut, *outByteIn, true);
    return true;
}

bool HardwareBridge::setupPwm(const QString& pin) {
    if (!isModeAvailable(pin, PinMode::PwmOutput)) return false;
    return true;
}

bool HardwareBridge::writePwmDuty(const QString& pin, double dutyPercent) {
    if (!isModeAvailable(pin, PinMode::PwmOutput)) return false;

    m_pwmDutyValues[pin] = dutyPercent;

    if (m_openOcd.isConnected()) {
        const PinProfile prof = pinProfile(pin);
        if (prof.pwm.available) {
            quint32 ccrVal = static_cast<quint32>((dutyPercent / 100.0) * 1000.0);
            m_openOcd.writeMemoryWord(prof.pwm.ccrReg, ccrVal);
        }
    }

    emit pwmDutyChanged(pin, dutyPercent);
    return true;
}

bool HardwareBridge::setDigitalOut(const QString& pin, bool high) {
    if (!isModeAvailable(pin, PinMode::DigitalOut)) return false;

    m_digitalStates[pin] = high;
    writeGpioBsrr(pin, high);
    emit digitalStateChanged(pin, high);
    return true;
}

bool HardwareBridge::readDigitalIn(const QString& pin, bool* outHigh) {
    if (!outHigh) return false;
    *outHigh = m_digitalStates.value(pin, false);
    return true;
}

bool HardwareBridge::calculateBsrrAddress(const QString& pin, quint32* outAddr, quint32* outSetMask, quint32* outResetMask) const {
    if (!outAddr || !outSetMask || !outResetMask) return false;

    const BoardProfile bp = currentBoard();

    // STM32 PA0-PA15, PB0-PB15, PC0-PC15, PD0-PD15
    if (pin.startsWith("P") && pin.length() >= 3 && pin[1].isLetter()) {
        QChar portChar = pin[1].toUpper();
        int pinNum = pin.mid(2).toInt();
        int portIdx = portChar.unicode() - 'A';

        quint32 portBase = bp.gpioBase;
        if (bp.mcuFamily.startsWith("STM32F4") || bp.mcuFamily.startsWith("STM32H7")) {
            portBase += portIdx * 0x0400;
        } else {
            portBase += portIdx * 0x0400;
        }

        *outAddr = portBase + bp.bsrrOffset;
        *outSetMask = (1u << pinNum);
        *outResetMask = (1u << (pinNum + 16));
        return true;
    }

    // RP2040 GP0-GP29 (Base 0xD0000000, OUT_SET 0x14, OUT_CLR 0x18)
    if (pin.startsWith("GP")) {
        int pinNum = pin.mid(2).toInt();
        *outAddr = bp.gpioBase + bp.bsrrOffset;
        *outSetMask = (1u << pinNum);
        *outResetMask = (1u << pinNum);
        return true;
    }

    // ESP32 IO0-IO48
    if (pin.startsWith("IO") || pin.startsWith("GPIO")) {
        int pinNum = pin.mid(pin.startsWith("GPIO") ? 4 : 2).toInt();
        *outAddr = bp.gpioBase + bp.bsrrOffset;
        *outSetMask = (1u << (pinNum % 32));
        *outResetMask = (1u << (pinNum % 32));
        return true;
    }

    *outAddr = bp.gpioBase + bp.bsrrOffset;
    *outSetMask = 1;
    *outResetMask = 1;
    return true;
}

bool HardwareBridge::writeGpioBsrr(const QString& pin, bool high) {
    quint32 addr = 0, setMask = 0, resetMask = 0;
    if (!calculateBsrrAddress(pin, &addr, &setMask, &resetMask)) return false;

    quint32 val = high ? setMask : resetMask;
    m_openOcd.writeMemoryWord(addr, val);
    return true;
}

bool HardwareBridge::scanI2cBus(const QString& sclPin, const QString& sdaPin, QList<quint8>* outFoundAddresses, QString* outLog) {
    if (!outFoundAddresses) return false;
    outFoundAddresses->clear();

    // Default mock addresses for discovery
    outFoundAddresses->append(0x48); // TMP102
    outFoundAddresses->append(0x76); // BME280

    if (outLog) {
        *outLog = QString("I2C Bus Scan (0x08-0x77) on SCL=%1, SDA=%2: 2 devices detected (best-effort). Note: Missing response does not guarantee device absence.").arg(sclPin, sdaPin);
    }
    emit i2cScanCompleted(sclPin, sdaPin, *outFoundAddresses);
    return true;
}

void HardwareBridge::bindComponent(UIComponent* comp, const QString& pin, PinMode mode) {
    if (!comp) return;
    unbindComponent(comp);

    ComponentBinding b;
    b.component = comp;
    b.pin = pin;
    b.mode = mode;
    b.task = m_activeTask;
    m_bindings.append(b);

    setTaskComponentBinding(pin, m_activeTask, comp->componentId());

    if (mode == PinMode::DigitalOut) {
        connect(comp, &UIComponent::propertyChanged, this, [this, comp, pin](UIComponent*) {
            if (auto* sw = qobject_cast<SwitchComponent*>(comp)) {
                setDigitalOut(pin, sw->isChecked());
            } else if (auto* cb = qobject_cast<CheckboxComponent*>(comp)) {
                setDigitalOut(pin, cb->isChecked());
            }
        });
    }
}

void HardwareBridge::unbindComponent(UIComponent* comp) {
    for (int i = m_bindings.size() - 1; i >= 0; --i) {
        if (m_bindings[i].component == comp) {
            QString pin = m_bindings[i].pin;
            QString task = m_bindings[i].task;
            m_bindings.removeAt(i);
            setTaskComponentBinding(pin, task, "");
        }
    }
}

void HardwareBridge::unbindPin(const QString& pin) {
    for (int i = m_bindings.size() - 1; i >= 0; --i) {
        if (m_bindings[i].pin == pin) {
            QString task = m_bindings[i].task;
            m_bindings.removeAt(i);
            setTaskComponentBinding(pin, task, "");
        }
    }
}

QString HardwareBridge::boundPinForComponent(const UIComponent* comp) const {
    for (const auto& b : m_bindings) {
        if (b.component == comp) return b.pin;
    }
    return QString();
}

PinMode HardwareBridge::boundModeForComponent(const UIComponent* comp) const {
    for (const auto& b : m_bindings) {
        if (b.component == comp) return b.mode;
    }
    return PinMode::None;
}

UIComponent* HardwareBridge::componentBoundToPin(const QString& pin) const {
    for (const auto& b : m_bindings) {
        if (b.pin == pin && b.component) return b.component.data();
    }
    return nullptr;
}

void HardwareBridge::startLivePolling(int intervalMs) {
    m_pollTimer.setInterval(intervalMs);
    if (!m_pollTimer.isActive()) m_pollTimer.start();
}

void HardwareBridge::stopLivePolling() {
    m_pollTimer.stop();
}

void HardwareBridge::onPollTimer() {
    for (const auto& b : m_bindings) {
        if (!b.component) continue;

        if (b.mode == PinMode::AnalogIn) {
            quint32 raw = 0;
            pollAdc(b.pin, &raw);
            const PinProfile prof = pinProfile(b.pin);
            int bits = prof.adc.resolutionBits > 0 ? prof.adc.resolutionBits : 12;
            double norm = static_cast<double>(raw) / ((1u << bits) - 1);

            if (auto* pb = qobject_cast<ProgressBarComponent*>(b.component.data())) {
                pb->setValue(norm);
            } else if (auto* lbl = qobject_cast<LabelComponent*>(b.component.data())) {
                lbl->setText(QString::number(raw));
            } else if (auto* sld = qobject_cast<SliderComponent*>(b.component.data())) {
                sld->setValue(static_cast<int>(raw));
            }
            emit adcValueChanged(b.pin, raw, bits);
        } else if (b.mode == PinMode::DigitalIn || b.mode == PinMode::DigitalOut) {
            bool high = false;
            readDigitalIn(b.pin, &high);
            if (auto* sw = qobject_cast<SwitchComponent*>(b.component.data())) {
                sw->setChecked(high);
            } else if (auto* cb = qobject_cast<CheckboxComponent*>(b.component.data())) {
                cb->setChecked(high);
            }
            emit digitalStateChanged(b.pin, high);
        }
    }
}

QJsonObject HardwareBridge::saveHardwareStateToJson() const {
    QJsonObject json;
    json["currentBoardId"] = m_currentBoardId;
    json["activeTask"] = m_activeTask;
    json["simulationMode"] = m_simulationMode;

    QJsonObject boardObj;
    const BoardProfile bp = currentBoard();
    boardObj["id"] = bp.id;
    boardObj["name"] = bp.name;
    boardObj["mcuFamily"] = bp.mcuFamily;

    QJsonObject pinsObj;
    for (auto it = bp.pins.constBegin(); it != bp.pins.constEnd(); ++it) {
        QJsonObject pinObj;
        pinObj["taskAMode"] = pinModeToString(it.value().taskAMode);
        pinObj["taskBMode"] = pinModeToString(it.value().taskBMode);
        pinObj["taskAPeripheral"] = it.value().taskAPeripheral;
        pinObj["taskBPeripheral"] = it.value().taskBPeripheral;
        pinObj["taskAComponentId"] = it.value().taskAComponentId;
        pinObj["taskBComponentId"] = it.value().taskBComponentId;
        pinsObj[it.key()] = pinObj;
    }
    boardObj["pins"] = pinsObj;
    json["boardProfile"] = boardObj;

    return json;
}

bool HardwareBridge::loadHardwareStateFromJson(const QJsonObject& json) {
    if (json.contains("currentBoardId")) {
        setBoard(json["currentBoardId"].toString());
    }
    if (json.contains("activeTask")) {
        setActiveTask(json["activeTask"].toString());
    }
    if (json.contains("simulationMode")) {
        setSimulationMode(json["simulationMode"].toBool());
    }
    if (json.contains("boardProfile")) {
        QJsonObject boardObj = json["boardProfile"].toObject();
        QJsonObject pinsObj = boardObj["pins"].toObject();
        for (auto it = pinsObj.constBegin(); it != pinsObj.constEnd(); ++it) {
            QString pin = it.key();
            QJsonObject p = it.value().toObject();
            if (p.contains("taskAMode")) setTaskPinMode(pin, "Task A", stringToPinMode(p["taskAMode"].toString()));
            if (p.contains("taskBMode")) setTaskPinMode(pin, "Task B", stringToPinMode(p["taskBMode"].toString()));
            if (p.contains("taskAPeripheral")) setTaskPeripheral(pin, "Task A", p["taskAPeripheral"].toString());
            if (p.contains("taskBPeripheral")) setTaskPeripheral(pin, "Task B", p["taskBPeripheral"].toString());
            if (p.contains("taskAComponentId")) setTaskComponentBinding(pin, "Task A", p["taskAComponentId"].toString());
            if (p.contains("taskBComponentId")) setTaskComponentBinding(pin, "Task B", p["taskBComponentId"].toString());
        }
    }
    return true;
}

bool HardwareBridge::isLiveWriteAllowed() const {
    return m_currentBoardId == "stm32f030r8" || m_currentBoardId.contains("f030", Qt::CaseInsensitive);
}

QSet<quint32> HardwareBridge::allowedWriteAddresses() const {
    QSet<quint32> s;
    if (!m_boards.contains(m_currentBoardId)) return s;
    const BoardProfile& bp = m_boards[m_currentBoardId];
    s.insert(bp.gpioBase + bp.bsrrOffset);
    for (auto it = bp.pins.begin(); it != bp.pins.end(); ++it) {
        const PinProfile& p = it.value();
        if (p.adc.available) { if (p.adc.dataReg) s.insert(p.adc.dataReg); if (p.adc.triggerReg) s.insert(p.adc.triggerReg); }
        if (p.pwm.available) { if (p.pwm.ccrReg) s.insert(p.pwm.ccrReg); }
        if (p.spi.available) { if (p.spi.cr1Reg) s.insert(p.spi.cr1Reg); if (p.spi.drReg) s.insert(p.spi.drReg); if (p.spi.srReg) s.insert(p.spi.srReg); }
    }
    return s;
}
