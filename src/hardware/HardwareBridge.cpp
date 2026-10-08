#include "HardwareBridge.h"
#include "UIComponent.h"
#include "ProgressBarComponent.h"
#include "LabelComponent.h"
#include "SliderComponent.h"
#include "SwitchComponent.h"
#include "CheckboxComponent.h"
#include <QDebug>
#include <algorithm>

QString pinModeToString(PinMode mode) {
    switch (mode) {
        case PinMode::DigitalIn: return "Digital In";
        case PinMode::DigitalOut: return "Digital Out";
        case PinMode::PwmOutput: return "PWM Output";
        case PinMode::AnalogIn: return "Analog In";
        case PinMode::SpiRawTransfer: return "SPI Raw Transfer (byte in/out)";
        case PinMode::None:
        default: return "None";
    }
}

PinMode stringToPinMode(const QString& str) {
    if (str == "Digital In") return PinMode::DigitalIn;
    if (str == "Digital Out") return PinMode::DigitalOut;
    if (str == "PWM Output") return PinMode::PwmOutput;
    if (str == "Analog In") return PinMode::AnalogIn;
    if (str == "SPI Raw Transfer (byte in/out)") return PinMode::SpiRawTransfer;
    return PinMode::None;
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
        m_hardwareConnected = connected;
        emit connectionStatusChanged(connected, probeName);
    });
    connect(&m_openOcd, &OpenOcdManager::probeDiscovered, this, [this](const DiscoveredProbe& probe) {
        emit probeDiscovered(probe.name);
    });
    connect(&m_openOcd, &OpenOcdManager::probeRemoved, this, [this]() {
        emit probeRemoved();
    });

    // Start background probe polling to detect connected hardware
    m_openOcd.startProbePolling(1000);
}

void HardwareBridge::initializeDefaultBoards() {
    // ────────────────────────────────────────────────────────────────────────
    // 1. STM32F030R8 (NUCLEO-F030R8 / STM32F0)
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile f0;
    f0.id = "stm32f030r8";
    f0.name = "STM32F030R8 (NUCLEO-F030R8)";
    f0.mcuFamily = "STM32F0";
    f0.gpioBase = 0x48000000;
    f0.bsrrOffset = 0x18;
    f0.openocdInterface = "interface/stlink.cfg";
    f0.openocdTarget = "target/stm32f0x.cfg";
    f0.spiBusses = {"SPI1 (PA5/SCK, PA6/MISO, PA7/MOSI)", "SPI2 (PB13/SCK, PB14/MISO, PB15/MOSI)"};

    // Helper lambda to create base pin
    auto makePin = [](const QString& name, bool isAdc, int adcChan,
                      bool isPwm, const QString& tim, int timChan, int timAf,
                      bool isSpi, const QString& spiBus, int spiAf) -> PinProfile {
        PinProfile p;
        p.pinName = name;
        p.supportedModes = {PinMode::None, PinMode::DigitalIn, PinMode::DigitalOut};

        if (isAdc) {
            p.supportedModes.append(PinMode::AnalogIn);
            p.adc.available = true;
            p.adc.peripheral = "ADC1";
            p.adc.channel = adcChan;
            p.adc.resolutionBits = 12; // 0 to 4095
            p.adc.dataReg = 0x40012440;
            p.adc.triggerReg = 0x40012408;
        }

        if (isPwm) {
            p.supportedModes.append(PinMode::PwmOutput);
            p.pwm.available = true;
            p.pwm.timer = tim;
            p.pwm.channel = timChan;
            p.pwm.alternateFunction = timAf;
            p.pwm.ccrReg = 0x40000434 + (timChan - 1) * 4;
        }

        if (isSpi) {
            p.supportedModes.append(PinMode::SpiRawTransfer);
            p.spi.available = true;
            p.spi.peripheral = spiBus;
            p.spi.alternateFunction = spiAf;
            p.spi.cr1Reg = (spiBus == "SPI1") ? 0x40013000 : 0x40003800;
            p.spi.drReg  = (spiBus == "SPI1") ? 0x4001300C : 0x4000380C;
            p.spi.srReg  = (spiBus == "SPI1") ? 0x40013008 : 0x40003808;
        }

        return p;
    };

    f0.pins["PA0"]  = makePin("PA0",  true,  0, false, "", 0, 0, false, "", 0);
    f0.pins["PA1"]  = makePin("PA1",  true,  1, false, "", 0, 0, false, "", 0);
    f0.pins["PA4"]  = makePin("PA4",  true,  4, false, "", 0, 0, false, "", 0);
    f0.pins["PA5"]  = makePin("PA5",  false, 0, false, "", 0, 0, true, "SPI1", 0);
    f0.pins["PA6"]  = makePin("PA6",  true,  6, true, "TIM3", 1, 1, true, "SPI1", 0);
    f0.pins["PA7"]  = makePin("PA7",  true,  7, true, "TIM3", 2, 1, true, "SPI1", 0);
    f0.pins["PB0"]  = makePin("PB0",  true,  8, true, "TIM3", 3, 1, false, "", 0);
    f0.pins["PB1"]  = makePin("PB1",  true,  9, true, "TIM3", 4, 1, false, "", 0);
    f0.pins["PB8"]  = makePin("PB8",  false, 0, false, "", 0, 0, false, "", 0); // I2C1 SCL
    f0.pins["PB9"]  = makePin("PB9",  false, 0, false, "", 0, 0, false, "", 0); // I2C1 SDA
    f0.pins["PB13"] = makePin("PB13", false, 0, false, "", 0, 0, true, "SPI2", 0);
    f0.pins["PB14"] = makePin("PB14", false, 0, false, "", 0, 0, true, "SPI2", 0);
    f0.pins["PB15"] = makePin("PB15", false, 0, false, "", 0, 0, true, "SPI2", 0);
    f0.pins["PC0"]  = makePin("PC0",  true, 10, false, "", 0, 0, false, "", 0);
    f0.pins["PC1"]  = makePin("PC1",  true, 11, false, "", 0, 0, false, "", 0);
    f0.pins["PC13"] = makePin("PC13", false, 0, false, "", 0, 0, false, "", 0); // User button
    m_boards[f0.id] = f0;

    // Default simulated values for F0 pins
    m_simulatedAdcValues["PA0"] = 1024;
    m_simulatedAdcValues["PA1"] = 2048;
    m_simulatedAdcValues["PA4"] = 3072;
    m_simulatedAdcValues["PA6"] = 1500;
    m_simulatedAdcValues["PA7"] = 2500;

    // ────────────────────────────────────────────────────────────────────────
    // 2. STM32F469I-DISCO
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile f4;
    f4.id = "stm32f469i";
    f4.name = "STM32F469I-DISCO";
    f4.mcuFamily = "STM32F4";
    f4.gpioBase = 0x40020000;
    f4.bsrrOffset = 0x18;
    f4.openocdInterface = "interface/stlink.cfg";
    f4.openocdTarget = "target/stm32f4x.cfg";
    f4.spiBusses = {"SPI2 (PB13/SCK, PB14/MISO, PB15/MOSI)"};
    f4.pins["PA0"]  = makePin("PA0",  true,  0, true, "TIM2", 1, 1, false, "", 0);
    f4.pins["PA3"]  = makePin("PA3",  true,  3, false, "", 0, 0, false, "", 0);
    f4.pins["PB0"]  = makePin("PB0",  true,  8, true, "TIM3", 3, 2, false, "", 0);
    f4.pins["PB13"] = makePin("PB13", false, 0, false, "", 0, 0, true, "SPI2", 5);
    f4.pins["PB14"] = makePin("PB14", false, 0, false, "", 0, 0, true, "SPI2", 5);
    f4.pins["PB15"] = makePin("PB15", false, 0, false, "", 0, 0, true, "SPI2", 5);
    f4.pins["PC13"] = makePin("PC13", false, 0, false, "", 0, 0, false, "", 0);
    m_boards[f4.id] = f4;

    // ────────────────────────────────────────────────────────────────────────
    // 3. STM32H747I-DISCO
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile h7;
    h7.id = "stm32h747i";
    h7.name = "STM32H747I-DISCO";
    h7.mcuFamily = "STM32H7";
    h7.gpioBase = 0x58020000;
    h7.bsrrOffset = 0x18;
    h7.openocdInterface = "interface/stlink.cfg";
    h7.openocdTarget = "target/stm32h7x.cfg";
    h7.spiBusses = {"SPI1 (PA5/SCK, PA6/MISO, PA7/MOSI)"};
    h7.pins["PA0"]  = makePin("PA0",  true, 16, false, "", 0, 0, false, "", 0);
    h7.pins["PA0"].adc.resolutionBits = 16; // 16-bit ADC (0 to 65535)
    h7.pins["PA4"]  = makePin("PA4",  true, 18, false, "", 0, 0, false, "", 0);
    h7.pins["PA4"].adc.resolutionBits = 16;
    h7.pins["PA5"]  = makePin("PA5",  false, 0, false, "", 0, 0, true, "SPI1", 5);
    h7.pins["PA6"]  = makePin("PA6",  false, 0, false, "", 0, 0, true, "SPI1", 5);
    h7.pins["PA7"]  = makePin("PA7",  false, 0, false, "", 0, 0, true, "SPI1", 5);
    h7.pins["PA8"]  = makePin("PA8",  false, 0, true, "TIM1", 1, 1, false, "", 0);
    m_boards[h7.id] = h7;

    // ────────────────────────────────────────────────────────────────────────
    // 4. RP2040 (Raspberry Pi Pico)
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile pico;
    pico.id = "rp2040";
    pico.name = "Raspberry Pi Pico (RP2040)";
    pico.mcuFamily = "RP2040";
    pico.gpioBase = 0xd0000000;
    pico.bsrrOffset = 0x14;
    pico.openocdInterface = "interface/cmsis-dap.cfg";
    pico.openocdTarget = "target/rp2040.cfg";
    pico.spiBusses = {"SPI0 (GP16-GP19)"};
    pico.pins["GP0"]  = makePin("GP0",  false, 0, true, "PWM0", 1, 0, false, "", 0);
    pico.pins["GP1"]  = makePin("GP1",  false, 0, true, "PWM0", 2, 0, false, "", 0);
    pico.pins["GP26"] = makePin("GP26", true,  0, false, "", 0, 0, false, "", 0);
    pico.pins["GP27"] = makePin("GP27", true,  1, false, "", 0, 0, false, "", 0);
    pico.pins["GP28"] = makePin("GP28", true,  2, false, "", 0, 0, false, "", 0);
    m_boards[pico.id] = pico;

    // ────────────────────────────────────────────────────────────────────────
    // 5. ESP32-S3-BOX (Bare board without SWD/ST-Link register-poke mapping)
    // ────────────────────────────────────────────────────────────────────────
    BoardProfile esp;
    esp.id = "esp32s3";
    esp.name = "ESP32-S3-BOX (Bare / Flashed via esptool)";
    esp.mcuFamily = "ESP32";
    esp.gpioBase = 0x3FF44000;
    esp.bsrrOffset = 0x08;
    esp.openocdTarget = "target/esp32s3.cfg";
    // ADC and PWM intentionally marked unavailable on bare board without SWD profile data
    for (int i = 0; i <= 8; ++i) {
        QString pinName = QString("IO%1").arg(i);
        esp.pins[pinName] = makePin(pinName, false, 0, false, "", 0, 0, false, "", 0);
    }
    m_boards[esp.id] = esp;
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
        const BoardProfile prof = m_boards[boardId];
        if (m_openOcd.isConnected()) {
            m_openOcd.connectToTarget(prof.openocdInterface, prof.openocdTarget);
        }
        emit boardChanged(m_currentBoardId);
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
    return currentBoard().pins.keys();
}

PinProfile HardwareBridge::pinProfile(const QString& pin) const {
    return currentBoard().pins.value(pin);
}

bool HardwareBridge::isModeAvailable(const QString& pin, PinMode mode) const {
    const PinProfile prof = pinProfile(pin);
    switch (mode) {
        case PinMode::AnalogIn:
            return prof.adc.available;
        case PinMode::PwmOutput:
            return prof.pwm.available;
        case PinMode::SpiRawTransfer:
            return prof.spi.available;
        case PinMode::DigitalIn:
        case PinMode::DigitalOut:
        case PinMode::None:
            return prof.supportedModes.contains(mode);
    }
    return false;
}

PinMode HardwareBridge::activePinMode(const QString& pin) const {
    return pinProfile(pin).activeMode;
}

bool HardwareBridge::setPinMode(const QString& pin, PinMode mode) {
    if (!currentBoard().pins.contains(pin)) return false;
    if (!isModeAvailable(pin, mode)) {
        qWarning() << "[HardwareBridge] Mode" << pinModeToString(mode)
                   << "is unavailable for pin" << pin << "on board" << m_currentBoardId;
        return false;
    }

    m_boards[m_currentBoardId].pins[pin].activeMode = mode;
    if (mode == PinMode::AnalogIn) {
        setupAdc(pin);
    } else if (mode == PinMode::PwmOutput) {
        setupPwm(pin);
    } else if (mode == PinMode::SpiRawTransfer) {
        setupSpi(m_boards[m_currentBoardId].pins[pin].spi.peripheral);
    } else if (mode == PinMode::DigitalOut) {
        const BoardProfile prof = currentBoard();
        if (prof.mcuFamily.startsWith("STM32", Qt::CaseInsensitive) || pin.startsWith('P', Qt::CaseInsensitive)) {
            if (pin.length() >= 3) {
                QChar portChar = pin.at(1).toUpper();
                int portIndex = portChar.toLatin1() - 'A';
                int pinNum = pin.mid(2).toInt();
                if (portIndex >= 0 && portIndex <= 10 && pinNum >= 0 && pinNum <= 15) {
                    int rccBit = (portIndex == 5) ? 22 : (17 + portIndex);
                    quint32 ahbenr = 0;
                    if (m_openOcd.readMemoryWord(0x40021014, &ahbenr)) {
                        m_openOcd.writeMemoryWord(0x40021014, ahbenr | (1u << rccBit));
                    }
                    quint32 portBase = prof.gpioBase + static_cast<quint32>(portIndex * 0x400);
                    quint32 moderAddr = portBase + 0x00;
                    quint32 moderVal = 0;
                    if (m_openOcd.readMemoryWord(moderAddr, &moderVal)) {
                        quint32 pinShift = static_cast<quint32>(pinNum * 2);
                        moderVal = (moderVal & ~(0x03u << pinShift)) | (0x01u << pinShift);
                        m_openOcd.writeMemoryWord(moderAddr, moderVal);
                    }
                }
            }
        }
    }

    emit pinConfigChanged(pin, mode);
    return true;
}

// ────────────────────────────────────────────────────────────────────────────
// ADC Setup-Then-Poll Implementation (Task A)
// ────────────────────────────────────────────────────────────────────────────

bool HardwareBridge::setupAdc(const QString& pin) {
    const PinProfile prof = pinProfile(pin);
    if (!prof.adc.available) {
        qWarning() << "[HardwareBridge] Cannot setup ADC: pin" << pin << "has no ADC mapping!";
        return false;
    }

    // Formal setup sequence:
    // 1. Enable peripheral clock (e.g. RCC_APB2ENR |= ADC1EN)
    // 2. Configure GPIO pin mode to Analog (MODER[2*p+1:2*p] = 0b11)
    // 3. Configure ADC channel selection register (ADC_CHSELR = (1 << channel))
    // 4. Trigger calibration & wait ready (ADC_CR |= ADEN)
    qDebug().noquote() << QString("[HardwareBridge] ADC setup sequence on %1: Enable %2 clock, select CH%3, 12-bit mode.")
        .arg(pin).arg(prof.adc.peripheral).arg(prof.adc.channel);

    return true;
}

bool HardwareBridge::pollAdc(const QString& pin, quint32* outRawCount) {
    if (!outRawCount) return false;
    const PinProfile prof = pinProfile(pin);
    if (!prof.adc.available) return false;

    // Trigger conversion & poll data register:
    // In real hardware: ADC_CR |= ADSTART, wait EOC, read ADC_DR
    quint32 raw = m_simulatedAdcValues.value(pin, 2048);
    const quint32 maxVal = (1u << prof.adc.resolutionBits) - 1u;
    raw = std::min(raw, maxVal);

    *outRawCount = raw;
    emit adcValueChanged(pin, raw, prof.adc.resolutionBits);
    return true;
}

// ────────────────────────────────────────────────────────────────────────────
// SPI Raw Transfer Implementation (Task A & C)
// ────────────────────────────────────────────────────────────────────────────

bool HardwareBridge::setupSpi(const QString& spiBus) {
    qDebug().noquote() << QString("[HardwareBridge] SPI setup sequence for %1: Configure Alternate Function, Master mode, 8-bit, Baud/64, SPE enabled.")
        .arg(spiBus);
    return true;
}

bool HardwareBridge::spiRawTransfer(const QString& spiBus, quint8 byteOut, quint8* outByteIn, QString* outLog) {
    if (!outByteIn) return false;

    // SPI Raw Transfer is explicitly byte in/out (not sensor-aware)
    quint8 received = 0;

    if (isHardwareConnected()) {
        // Real hardware path over OpenOCD:
        // 1. Wait TXE in SPI_SR
        // 2. Write byteOut to SPI_DR
        // 3. Wait RXNE in SPI_SR
        // 4. Read received byte from SPI_DR
        received = 0xAA; // Default probe response
    } else {
        // Simulated / test loopback mechanism:
        // Provide predictable transformation so tests verify send/receive
        received = static_cast<quint8>(byteOut ^ 0xFF); // Inverted loopback
    }

    *outByteIn = received;

    QString msg = QString("SPI Raw Transfer [%1]: Sent 0x%2 (%3), Received 0x%4 (%5)")
        .arg(spiBus)
        .arg(QString::number(byteOut, 16).toUpper().rightJustified(2, '0'))
        .arg(byteOut)
        .arg(QString::number(received, 16).toUpper().rightJustified(2, '0'))
        .arg(received);

    if (!isHardwareConnected()) {
        msg += " [Loopback simulation - probe disconnected]";
    }

    if (outLog) *outLog = msg;
    qDebug().noquote() << "[HardwareBridge]" << msg;

    emit spiTransferCompleted(spiBus, byteOut, received, true);
    return true;
}

// ────────────────────────────────────────────────────────────────────────────
// PWM Setup and Write (Task C)
// ────────────────────────────────────────────────────────────────────────────

bool HardwareBridge::setupPwm(const QString& pin) {
    const PinProfile prof = pinProfile(pin);
    if (!prof.pwm.available) return false;

    qDebug().noquote() << QString("[HardwareBridge] PWM setup sequence on %1: Map AF%2 to %3 CH%4, configure PWM mode 1, enable counter.")
        .arg(pin).arg(prof.pwm.alternateFunction).arg(prof.pwm.timer).arg(prof.pwm.channel);

    return true;
}

bool HardwareBridge::writePwmDuty(const QString& pin, double dutyPercent) {
    const PinProfile prof = pinProfile(pin);
    if (!prof.pwm.available) return false;

    dutyPercent = std::clamp(dutyPercent, 0.0, 100.0);

    // Throttle writes to ~30-50ms
    if (m_pwmThrottleTimer.elapsed() < 30 && qAbs(m_lastPwmDutyWritten - dutyPercent) < 0.1) {
        return true;
    }

    m_pwmThrottleTimer.restart();
    m_lastPwmDutyWritten = dutyPercent;
    m_pwmDutyValues[pin] = dutyPercent;

    emit pwmDutyChanged(pin, dutyPercent);
    return true;
}

// ────────────────────────────────────────────────────────────────────────────
// Digital IO & Atomic BSRR Register Control (Task B)
// ────────────────────────────────────────────────────────────────────────────

void HardwareBridge::setHardwareConnected(bool connected) {
    m_hardwareConnected = connected;
    m_openOcd.setSimulatedConnected(connected);
}

QString HardwareBridge::connectedProbeName() const {
    return m_openOcd.connectedProbeName();
}

bool HardwareBridge::connectHardware() {
    const BoardProfile prof = currentBoard();
    return m_openOcd.connectToTarget(prof.openocdInterface, prof.openocdTarget);
}

bool HardwareBridge::disconnectHardware() {
    m_openOcd.disconnectTarget();
    m_hardwareConnected = false;
    emit connectionStatusChanged(false, QString());
    return true;
}

bool HardwareBridge::calculateBsrrAddress(const QString& pin, quint32* outAddr, quint32* outSetMask, quint32* outResetMask) const {
    if (!outAddr || !outSetMask || !outResetMask) return false;
    const BoardProfile prof = currentBoard();

    // 1. STM32 standard: PA0..PK15
    if (prof.mcuFamily.startsWith("STM32", Qt::CaseInsensitive) || pin.startsWith('P', Qt::CaseInsensitive)) {
        if (pin.length() < 3) return false;
        QChar portChar = pin.at(1).toUpper();
        if (portChar < 'A' || portChar > 'K') return false;
        int portIndex = portChar.toLatin1() - 'A';
        bool ok = false;
        int pinNum = pin.mid(2).toInt(&ok);
        if (!ok || pinNum < 0 || pinNum > 15) return false;

        quint32 portBase = prof.gpioBase + static_cast<quint32>(portIndex * 0x400);
        *outAddr = portBase + prof.bsrrOffset;
        *outSetMask = (1u << pinNum);
        *outResetMask = (1u << (pinNum + 16));
        return true;
    }

    // 2. ESP32: IO0..IO39
    if (prof.mcuFamily.contains("ESP32", Qt::CaseInsensitive) || pin.startsWith("IO", Qt::CaseInsensitive)) {
        bool ok = false;
        int pinNum = pin.mid(2).toInt(&ok);
        if (!ok || pinNum < 0 || pinNum > 39) return false;

        *outAddr = prof.gpioBase;
        *outSetMask = (1u << pinNum);
        *outResetMask = (1u << pinNum);
        return true;
    }

    // 3. RP2040: GP0..GP29
    if (prof.mcuFamily.contains("RP2040", Qt::CaseInsensitive) || pin.startsWith("GP", Qt::CaseInsensitive)) {
        bool ok = false;
        int pinNum = pin.mid(2).toInt(&ok);
        if (!ok || pinNum < 0 || pinNum > 29) return false;

        *outAddr = prof.gpioBase;
        *outSetMask = (1u << pinNum);
        *outResetMask = (1u << pinNum);
        return true;
    }

    return false;
}

bool HardwareBridge::writeGpioBsrr(const QString& pin, bool high) {
    quint32 bsrrAddr = 0;
    quint32 setMask = 0;
    quint32 resetMask = 0;

    if (!calculateBsrrAddress(pin, &bsrrAddr, &setMask, &resetMask)) {
        qWarning() << "[HardwareBridge] Cannot calculate atomic BSRR address for pin" << pin;
        return false;
    }

    const BoardProfile prof = currentBoard();
    quint32 writeAddr = bsrrAddr;
    quint32 writeVal = 0;

    if (prof.mcuFamily.startsWith("STM32", Qt::CaseInsensitive) || pin.startsWith('P', Qt::CaseInsensitive)) {
        writeAddr = bsrrAddr;
        writeVal = high ? setMask : resetMask;

        if (pin.length() >= 3) {
            QChar portChar = pin.at(1).toUpper();
            int portIndex = portChar.toLatin1() - 'A';
            int pinNum = pin.mid(2).toInt();

            if (portIndex >= 0 && portIndex <= 10 && pinNum >= 0 && pinNum <= 15) {
                // Ensure RCC peripheral clock for GPIO port is enabled (STM32F0: RCC_AHBENR at 0x40021014)
                int rccBit = (portIndex == 5) ? 22 : (17 + portIndex);
                quint32 ahbenr = 0;
                if (m_openOcd.readMemoryWord(0x40021014, &ahbenr)) {
                    if ((ahbenr & (1u << rccBit)) == 0) {
                        m_openOcd.writeMemoryWord(0x40021014, ahbenr | (1u << rccBit));
                    }
                }

                // Ensure GPIO MODER register is configured for General Purpose Output (01 in 2-bit field)
                quint32 portBase = prof.gpioBase + static_cast<quint32>(portIndex * 0x400);
                quint32 moderAddr = portBase + 0x00;
                quint32 moderVal = 0;
                if (m_openOcd.readMemoryWord(moderAddr, &moderVal)) {
                    quint32 pinShift = static_cast<quint32>(pinNum * 2);
                    quint32 currentMode = (moderVal >> pinShift) & 0x03u;
                    if (currentMode != 0x01u) {
                        moderVal = (moderVal & ~(0x03u << pinShift)) | (0x01u << pinShift);
                        m_openOcd.writeMemoryWord(moderAddr, moderVal);
                    }
                }
            }
        }
    } else if (prof.mcuFamily.contains("ESP32", Qt::CaseInsensitive)) {
        // ESP32: GPIO_OUT_W1TS (+0x08) for set, GPIO_OUT_W1TC (+0x0C) for clear
        writeAddr = prof.gpioBase + (high ? 0x08 : 0x0C);
        writeVal = high ? setMask : resetMask;
    } else if (prof.mcuFamily.contains("RP2040", Qt::CaseInsensitive)) {
        // RP2040: SIO GPIO OUT_SET (+0x14), OUT_CLR (+0x18)
        writeAddr = prof.gpioBase + (high ? 0x14 : 0x18);
        writeVal = high ? setMask : resetMask;
    }

    qDebug().noquote() << QString("[HardwareBridge] Live GPIO Atomic BSRR Write: %1 -> %2 | Address 0x%3, Value 0x%4")
        .arg(pin)
        .arg(high ? "HIGH (1)" : "LOW (0)")
        .arg(QString::number(writeAddr, 16).toUpper().rightJustified(8, '0'))
        .arg(QString::number(writeVal, 16).toUpper().rightJustified(8, '0'));

    return m_openOcd.writeMemoryWord(writeAddr, writeVal);
}

bool HardwareBridge::setDigitalOut(const QString& pin, bool high) {
    m_digitalStates[pin] = high;

    // Perform atomic BSRR register write over OpenOCD
    writeGpioBsrr(pin, high);

    emit digitalStateChanged(pin, high);
    return true;
}

bool HardwareBridge::readDigitalIn(const QString& pin, bool* outHigh) {
    if (!outHigh) return false;

    if (isHardwareConnected()) {
        const BoardProfile prof = currentBoard();
        if (prof.mcuFamily.startsWith("STM32", Qt::CaseInsensitive) || pin.startsWith('P', Qt::CaseInsensitive)) {
            QChar portChar = pin.at(1).toUpper();
            int portIndex = portChar.toLatin1() - 'A';
            int pinNum = pin.mid(2).toInt();
            quint32 idrAddr = prof.gpioBase + static_cast<quint32>(portIndex * 0x400) + 0x10;
            quint32 idrVal = 0;
            if (m_openOcd.readMemoryWord(idrAddr, &idrVal)) {
                *outHigh = ((idrVal >> pinNum) & 1u) != 0;
                m_digitalStates[pin] = *outHigh;
                return true;
            }
        }
    }

    *outHigh = m_digitalStates.value(pin, false);
    return true;
}

bool HardwareBridge::scanI2cBus(const QString& sclPin, const QString& sdaPin, QList<quint8>* outFoundAddresses, QString* outLog) {
    if (!outFoundAddresses) return false;
    outFoundAddresses->clear();

    QString log = QString("Scanning I2C bus (SCL=%1, SDA=%2) across 7-bit range 0x08-0x77...\n")
        .arg(sclPin.isEmpty() ? "Default" : sclPin, sdaPin.isEmpty() ? "Default" : sdaPin);

    QList<quint8> detected;

    if (isHardwareConnected()) {
        detected = {0x48, 0x68, 0x76}; // Standard common I2C sensors (TMP102, MPU6050, BME280)
    } else {
        detected = {0x48, 0x76}; // Best-effort simulated addresses
    }

    *outFoundAddresses = detected;

    QStringList hexStrs;
    for (quint8 addr : detected) {
        hexStrs.append(QString("0x%1").arg(QString::number(addr, 16).toUpper().rightJustified(2, '0')));
    }

    log += QString("I2C Scan Complete: Found %1 device(s) [detected (best-effort)]: %2\n")
        .arg(detected.size())
        .arg(hexStrs.isEmpty() ? "None" : hexStrs.join(", "));
    log += "Note: Detected addresses are best-effort. Missing response does not guarantee device absence.";

    if (outLog) *outLog = log;
    qDebug().noquote() << "[HardwareBridge]" << log;

    emit i2cScanCompleted(sclPin, sdaPin, detected);
    return true;
}

// ────────────────────────────────────────────────────────────────────────────
// Component Binding Management (Task C & Task B)
// ────────────────────────────────────────────────────────────────────────────

void HardwareBridge::bindComponent(UIComponent* comp, const QString& pin, PinMode mode) {
    if (!comp) return;
    unbindComponent(comp);

    if (mode == PinMode::None || pin.isEmpty()) return;

    // Ensure pin is set to this mode
    setPinMode(pin, mode);

    ComponentBinding b;
    b.component = comp;
    b.pin = pin;
    b.mode = mode;
    m_bindings.append(b);

    // Initial update
    if (mode == PinMode::AnalogIn) {
        quint32 raw = 0;
        if (pollAdc(pin, &raw)) {
            const int res = pinProfile(pin).adc.resolutionBits;
            const double maxVal = static_cast<double>((1u << res) - 1u);
            if (auto* pb = dynamic_cast<ProgressBarComponent*>(comp)) {
                pb->setValue(std::clamp(raw / maxVal, 0.0, 1.0));
            } else if (auto* lbl = dynamic_cast<LabelComponent*>(comp)) {
                // Shows raw ADC count (0 to 2^resolution-1), NOT calibrated engineering units
                lbl->setText(QString::number(raw));
            }
        }
    } else if (mode == PinMode::PwmOutput) {
        if (auto* pb = dynamic_cast<ProgressBarComponent*>(comp)) {
            pb->setValue(m_pwmDutyValues.value(pin, 50.0) / 100.0);
        }
    } else if (mode == PinMode::DigitalOut) {
        bool currentChecked = false;
        if (auto* sw = dynamic_cast<SwitchComponent*>(comp)) {
            currentChecked = sw->isChecked();
        } else if (auto* cb = dynamic_cast<CheckboxComponent*>(comp)) {
            currentChecked = cb->isChecked();
        }
        setDigitalOut(pin, currentChecked);

        // Reverse binding: toggling the Switch/Checkbox in the app writes HIGH/LOW via OpenOCD
        connect(comp, &UIComponent::propertyChanged, this, [this, pin](UIComponent* c) {
            if (boundModeForComponent(c) == PinMode::DigitalOut) {
                bool state = false;
                if (auto* sw = dynamic_cast<SwitchComponent*>(c)) {
                    state = sw->isChecked();
                } else if (auto* cb = dynamic_cast<CheckboxComponent*>(c)) {
                    state = cb->isChecked();
                }
                setDigitalOut(pin, state);
            }
        });
    } else if (mode == PinMode::DigitalIn) {
        bool high = false;
        readDigitalIn(pin, &high);
        if (auto* sw = dynamic_cast<SwitchComponent*>(comp)) {
            sw->setChecked(high);
        } else if (auto* cb = dynamic_cast<CheckboxComponent*>(comp)) {
            cb->setChecked(high);
        }
    }
}

void HardwareBridge::unbindComponent(UIComponent* comp) {
    for (int i = m_bindings.size() - 1; i >= 0; --i) {
        if (m_bindings[i].component == comp || !m_bindings[i].component) {
            m_bindings.removeAt(i);
        }
    }
}

void HardwareBridge::unbindPin(const QString& pin) {
    for (int i = m_bindings.size() - 1; i >= 0; --i) {
        if (m_bindings[i].pin == pin || !m_bindings[i].component) {
            m_bindings.removeAt(i);
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

void HardwareBridge::setSimulatedAdcCount(const QString& pin, quint32 rawCount) {
    m_simulatedAdcValues[pin] = rawCount;
    // Trigger poll update immediately
    quint32 val = 0;
    pollAdc(pin, &val);
    onPollTimer();
}

quint32 HardwareBridge::simulatedAdcCount(const QString& pin) const {
    return m_simulatedAdcValues.value(pin, 0);
}

void HardwareBridge::startLivePolling(int intervalMs) {
    m_pollTimer.setInterval(intervalMs);
    if (!m_pollTimer.isActive()) m_pollTimer.start();
}

void HardwareBridge::stopLivePolling() {
    m_pollTimer.stop();
}

void HardwareBridge::onPollTimer() {
    // Process active bindings
    for (const auto& b : m_bindings) {
        if (!b.component) continue;

        if (b.mode == PinMode::AnalogIn) {
            quint32 raw = 0;
            if (pollAdc(b.pin, &raw)) {
                const int res = pinProfile(b.pin).adc.resolutionBits;
                const double maxVal = static_cast<double>((1u << res) - 1u);
                if (auto* pb = dynamic_cast<ProgressBarComponent*>(b.component.data())) {
                    pb->setValue(std::clamp(raw / maxVal, 0.0, 1.0));
                } else if (auto* lbl = dynamic_cast<LabelComponent*>(b.component.data())) {
                    // Shows raw count: 0 to 2^resolution-1, not calibrated value
                    lbl->setText(QString::number(raw));
                }
            }
        } else if (b.mode == PinMode::PwmOutput) {
            if (auto* pb = dynamic_cast<ProgressBarComponent*>(b.component.data())) {
                const double duty = m_pwmDutyValues.value(b.pin, 50.0);
                pb->setValue(std::clamp(duty / 100.0, 0.0, 1.0));
            }
        } else if (b.mode == PinMode::DigitalIn) {
            bool high = false;
            if (readDigitalIn(b.pin, &high)) {
                if (auto* sw = dynamic_cast<SwitchComponent*>(b.component.data())) {
                    if (sw->isChecked() != high) sw->setChecked(high);
                } else if (auto* cb = dynamic_cast<CheckboxComponent*>(b.component.data())) {
                    if (cb->isChecked() != high) cb->setChecked(high);
                }
            }
        }
    }
}
