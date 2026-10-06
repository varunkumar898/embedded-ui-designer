#include "BoardConfigParser.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>

BoardProfile BoardConfigParser::ParseResult::toBoardProfile() const {
    BoardProfile bp;
    bp.id = boardId;
    bp.name = boardName;
    bp.mcuFamily = mcuFamily;

    // Default openocd scripts and GPIO registers based on family
    if (mcuFamily.contains("STM32F4", Qt::CaseInsensitive)) {
        bp.openocdInterface = "interface/stlink.cfg";
        bp.openocdTarget = "target/stm32f4x.cfg";
        bp.gpioBase = 0x40020000;
        bp.bsrrOffset = 0x18;
    } else if (mcuFamily.contains("STM32H7", Qt::CaseInsensitive)) {
        bp.openocdInterface = "interface/stlink.cfg";
        bp.openocdTarget = "target/stm32h7x.cfg";
        bp.gpioBase = 0x58020000;
        bp.bsrrOffset = 0x18;
    } else if (mcuFamily.contains("STM32F0", Qt::CaseInsensitive) || mcuFamily.contains("STM32G0", Qt::CaseInsensitive)) {
        bp.openocdInterface = "interface/stlink.cfg";
        bp.openocdTarget = "target/stm32f0x.cfg";
        bp.gpioBase = 0x48000000;
        bp.bsrrOffset = 0x18;
    } else if (mcuFamily.contains("ESP32", Qt::CaseInsensitive)) {
        bp.openocdInterface = "interface/esp_usb_jtag.cfg";
        bp.openocdTarget = "target/esp32.cfg";
        bp.gpioBase = 0x3FF44000;
        bp.bsrrOffset = 0x08;
    } else if (mcuFamily.contains("RP2040", Qt::CaseInsensitive)) {
        bp.openocdInterface = "interface/cmsis-dap.cfg";
        bp.openocdTarget = "target/rp2040.cfg";
        bp.gpioBase = 0xd0000000;
        bp.bsrrOffset = 0x14;
    }

    QStringList spiPins;
    for (const auto& ip : pins) {
        PinProfile pp;
        pp.pinName = ip.pinName;
        pp.supportedModes = ip.supportedModes;
        pp.activeMode = ip.defaultMode;
        pp.adc = ip.adc;
        pp.pwm = ip.pwm;
        pp.spi = ip.spi;
        bp.pins[ip.pinName] = pp;

        if (ip.spi.available) {
            spiPins.append(ip.pinName);
        }
    }

    if (!spiPins.isEmpty()) {
        bp.spiBusses.append(QString("SPI (%1)").arg(spiPins.join(", ")));
    }

    return bp;
}

BoardConfigParser::ParseResult BoardConfigParser::parseFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ParseResult r;
        r.success = false;
        r.errorMessage = QString("Cannot open file: %1").arg(filePath);
        return r;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    QFileInfo fi(filePath);
    return parseContent(content, fi.fileName());
}

BoardConfigParser::ParseResult BoardConfigParser::parseContent(const QString& content, const QString& fileNameHint) {
    QString lower = fileNameHint.toLower();
    if (lower.endsWith(".ioc") || content.contains("Mcu.Family", Qt::CaseInsensitive) || content.contains("Mcu.Name", Qt::CaseInsensitive)) {
        return parseStm32Ioc(content, fileNameHint);
    }
    if (lower.contains("sdkconfig") || content.contains("CONFIG_IDF_TARGET", Qt::CaseInsensitive)) {
        return parseEspSdkConfig(content, fileNameHint);
    }
    if (lower.endsWith(".h") || content.contains("#define", Qt::CaseInsensitive)) {
        return parseCPinHeader(content, fileNameHint);
    }

    // Default fallback try .ioc first, then sdkconfig
    if (content.contains("=")) {
        ParseResult r1 = parseStm32Ioc(content, fileNameHint);
        if (r1.success && !r1.pins.isEmpty()) return r1;
        ParseResult r2 = parseEspSdkConfig(content, fileNameHint);
        if (r2.success && !r2.pins.isEmpty()) return r2;
    }

    ParseResult r;
    r.success = false;
    r.errorMessage = "Unrecognized board configuration format. Please select an STM32CubeMX (.ioc) or ESP-IDF (sdkconfig) file.";
    return r;
}

BoardConfigParser::ParseResult BoardConfigParser::parseStm32Ioc(const QString& content, const QString& fileName) {
    ParseResult res;
    res.format = ConfigFormat::Stm32CubeIoc;

    QString mcuFamily = "STM32";
    QString mcuName;
    QMap<QString, QString> pinSignals;

    QStringList lines = content.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
    for (const QString& rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        int eqIdx = line.indexOf('=');
        if (eqIdx < 0) continue;

        QString key = line.left(eqIdx).trimmed();
        QString val = line.mid(eqIdx + 1).trimmed();

        if (key.compare("Mcu.Family", Qt::CaseInsensitive) == 0) {
            mcuFamily = val;
        } else if (key.compare("Mcu.Name", Qt::CaseInsensitive) == 0 || key.compare("Mcu.UserName", Qt::CaseInsensitive) == 0) {
            if (mcuName.isEmpty() || key.compare("Mcu.Name", Qt::CaseInsensitive) == 0) {
                mcuName = val;
            }
        } else if (key.endsWith(".Signal", Qt::CaseInsensitive)) {
            QString pinName = key.left(key.length() - 7).trimmed();
            pinSignals[pinName] = val;
        }
    }

    if (pinSignals.isEmpty()) {
        res.success = false;
        res.errorMessage = "No pin assignments (.Signal) found in .ioc file.";
        return res;
    }

    if (mcuName.isEmpty()) {
        QFileInfo fi(fileName);
        mcuName = fi.baseName();
        if (mcuName.isEmpty()) mcuName = "STM32-Custom";
    }

    res.boardId = QString("imported_%1").arg(mcuName.toLower().replace(QRegularExpression("[^a-z0-9_]"), "_"));
    res.boardName = QString("%1 (Imported .ioc)").arg(mcuName);
    res.mcuFamily = mcuFamily;

    // Parse each pin's signal
    for (auto it = pinSignals.begin(); it != pinSignals.end(); ++it) {
        const QString& pinName = it.key();
        const QString& signal = it.value();

        InferredPin ip;
        ip.pinName = pinName;
        ip.rawSignal = signal;
        ip.supportedModes.append(PinMode::None);

        QString sigUpper = signal.toUpper();

        // 1. ADC check (e.g. ADC1_IN0, ADC_IN5)
        if (sigUpper.contains("ADC") || sigUpper.contains("ANALOG")) {
            ip.inferredRole = "ADC";
            ip.supportedModes.append(PinMode::AnalogIn);
            ip.defaultMode = PinMode::AnalogIn;
            ip.adc.available = true;
            ip.adc.resolutionBits = 12;

            // Extract channel if present
            static QRegularExpression reChan("IN(\\d+)");
            auto match = reChan.match(sigUpper);
            if (match.hasMatch()) {
                ip.adc.channel = match.captured(1).toInt();
            }
            if (sigUpper.contains("ADC2")) ip.adc.peripheral = "ADC2";
            else if (sigUpper.contains("ADC3")) ip.adc.peripheral = "ADC3";
            else ip.adc.peripheral = "ADC1";
            ip.notes = QString("%1 Channel %2").arg(ip.adc.peripheral).arg(ip.adc.channel);
        }
        // 2. PWM / Timer check (e.g. TIM3_CH1, S_TIM3_CH3)
        else if (sigUpper.contains("TIM") && sigUpper.contains("CH")) {
            ip.inferredRole = "PWM";
            ip.supportedModes.append(PinMode::PwmOutput);
            ip.defaultMode = PinMode::PwmOutput;
            ip.pwm.available = true;

            static QRegularExpression reTim("TIM(\\d+).*CH(\\d+)");
            auto match = reTim.match(sigUpper);
            if (match.hasMatch()) {
                ip.pwm.timer = QString("TIM%1").arg(match.captured(1));
                ip.pwm.channel = match.captured(2).toInt();
            } else {
                ip.pwm.timer = "TIM1";
                ip.pwm.channel = 1;
            }
            ip.notes = QString("%1 Channel %2").arg(ip.pwm.timer).arg(ip.pwm.channel);
        }
        // 3. SPI check (e.g. SPI1_SCK, SPI1_MISO, SPI1_MOSI, SPI1_NSS)
        else if (sigUpper.contains("SPI")) {
            ip.inferredRole = "SPI";
            ip.supportedModes.append(PinMode::SpiRawTransfer);
            ip.defaultMode = PinMode::SpiRawTransfer;
            ip.spi.available = true;

            if (sigUpper.contains("SPI2")) ip.spi.peripheral = "SPI2";
            else if (sigUpper.contains("SPI3")) ip.spi.peripheral = "SPI3";
            else ip.spi.peripheral = "SPI1";

            if (sigUpper.contains("SCK") || sigUpper.contains("CLK")) ip.spi.sckPin = pinName;
            else if (sigUpper.contains("MISO")) ip.spi.misoPin = pinName;
            else if (sigUpper.contains("MOSI")) ip.spi.mosiPin = pinName;
            ip.notes = QString("%1 (%2)").arg(ip.spi.peripheral, signal);
        }
        // 4. I2C check (e.g. I2C1_SCL, I2C1_SDA)
        else if (sigUpper.contains("I2C")) {
            ip.inferredRole = "I2C";
            // Per policy, I2C is configuration only, not live register polling
            ip.defaultMode = PinMode::None;
            ip.notes = QString("I2C Bus Role (%1)").arg(signal);
        }
        // 5. GPIO Output
        else if (sigUpper.contains("GPIO_OUTPUT") || sigUpper == "OUTPUT") {
            ip.inferredRole = "GPIO Output";
            ip.supportedModes.append(PinMode::DigitalOut);
            ip.supportedModes.append(PinMode::DigitalIn);
            ip.defaultMode = PinMode::DigitalOut;
            ip.notes = "General Purpose Output";
        }
        // 6. GPIO Input / External Interrupt
        else if (sigUpper.contains("GPIO_INPUT") || sigUpper.contains("GPIO_EXTI") || sigUpper == "INPUT") {
            ip.inferredRole = "GPIO Input";
            ip.supportedModes.append(PinMode::DigitalIn);
            ip.defaultMode = PinMode::DigitalIn;
            ip.notes = "General Purpose Input";
        }
        // 7. System / Debug / Unknown
        else {
            // Cannot confidently determine role -> mark Unused / Unknown
            ip.inferredRole = QString("Unused / Unknown (%1)").arg(signal);
            ip.defaultMode = PinMode::None;
            ip.notes = "System / Unused function — gated from runtime drivers";
        }

        res.pins.append(ip);
    }

    res.success = true;
    return res;
}

BoardConfigParser::ParseResult BoardConfigParser::parseEspSdkConfig(const QString& content, const QString& fileName) {
    ParseResult res;
    res.format = ConfigFormat::EspIdfSdkConfig;

    QString targetMcu = "ESP32";
    QMap<QString, QString> pinRoles;

    QStringList lines = content.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
    for (const QString& rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        int eqIdx = line.indexOf('=');
        if (eqIdx < 0) continue;

        QString key = line.left(eqIdx).trimmed();
        QString val = line.mid(eqIdx + 1).trimmed().replace('"', "");

        if (key == "CONFIG_IDF_TARGET") {
            targetMcu = val.toUpper();
        } else if (key.contains("PIN") || key.contains("GPIO")) {
            bool ok = false;
            int pinNum = val.toInt(&ok);
            if (ok && pinNum >= 0 && pinNum <= 48) {
                QString pinName = QString("IO%1").arg(pinNum);
                pinRoles[pinName] = key;
            }
        }
    }

    if (pinRoles.isEmpty()) {
        res.success = false;
        res.errorMessage = "No GPIO/Pin configuration entries found in sdkconfig.";
        return res;
    }

    QFileInfo fi(fileName);
    QString base = fi.baseName();
    if (base.isEmpty()) base = "esp32_project";

    res.boardId = QString("imported_%1").arg(base.toLower().replace(QRegularExpression("[^a-z0-9_]"), "_"));
    res.boardName = QString("%1 (%2 sdkconfig)").arg(base, targetMcu);
    res.mcuFamily = targetMcu;

    for (auto it = pinRoles.begin(); it != pinRoles.end(); ++it) {
        const QString& pinName = it.key();
        const QString& configKey = it.value();
        QString keyUpper = configKey.toUpper();

        InferredPin ip;
        ip.pinName = pinName;
        ip.rawSignal = configKey;
        ip.supportedModes.append(PinMode::None);

        if (keyUpper.contains("ADC")) {
            ip.inferredRole = "ADC";
            ip.supportedModes.append(PinMode::AnalogIn);
            ip.defaultMode = PinMode::AnalogIn;
            ip.adc.available = true;
            ip.adc.peripheral = "ADC1";
            ip.adc.channel = 0;
            ip.adc.resolutionBits = 12;
            ip.notes = "ADC Channel";
        } else if (keyUpper.contains("PWM") || keyUpper.contains("LEDC")) {
            ip.inferredRole = "PWM";
            ip.supportedModes.append(PinMode::PwmOutput);
            ip.defaultMode = PinMode::PwmOutput;
            ip.pwm.available = true;
            ip.pwm.timer = "LEDC";
            ip.pwm.channel = 0;
            ip.notes = "LEDC / PWM Output";
        } else if (keyUpper.contains("SPI")) {
            ip.inferredRole = "SPI";
            ip.supportedModes.append(PinMode::SpiRawTransfer);
            ip.defaultMode = PinMode::SpiRawTransfer;
            ip.spi.available = true;
            ip.spi.peripheral = "SPI2";
            ip.notes = "SPI Raw Transfer";
        } else if (keyUpper.contains("I2C") || keyUpper.contains("SCL") || keyUpper.contains("SDA")) {
            ip.inferredRole = "I2C";
            ip.defaultMode = PinMode::None;
            ip.notes = "I2C Bus Pin";
        } else if (keyUpper.contains("BUTTON") || keyUpper.contains("INPUT")) {
            ip.inferredRole = "GPIO Input";
            ip.supportedModes.append(PinMode::DigitalIn);
            ip.defaultMode = PinMode::DigitalIn;
            ip.notes = "Input Button / Sensor";
        } else if (keyUpper.contains("LED") || keyUpper.contains("OUTPUT") || keyUpper.contains("BLINK")) {
            ip.inferredRole = "GPIO Output";
            ip.supportedModes.append(PinMode::DigitalOut);
            ip.supportedModes.append(PinMode::DigitalIn);
            ip.defaultMode = PinMode::DigitalOut;
            ip.notes = "Output Indicator / Control";
        } else {
            ip.inferredRole = QString("Unused / Unknown (%1)").arg(configKey);
            ip.defaultMode = PinMode::None;
            ip.notes = "Unassigned / Unknown";
        }

        res.pins.append(ip);
    }

    res.success = true;
    return res;
}

BoardConfigParser::ParseResult BoardConfigParser::parseCPinHeader(const QString& content, const QString& fileName) {
    ParseResult res;
    res.format = ConfigFormat::CPinHeader;
    res.mcuFamily = "Embedded MCU";

    QFileInfo fi(fileName);
    QString base = fi.baseName();
    if (base.isEmpty()) base = "pin_definitions";
    res.boardId = QString("imported_%1").arg(base.toLower());
    res.boardName = QString("%1 (Pin Header)").arg(base);

    static QRegularExpression reDefine("#define\\s+([A-Za-z0-9_]+)\\s+([A-Za-z0-9_]+)");
    auto matches = reDefine.globalMatch(content);
    while (matches.hasNext()) {
        auto m = matches.next();
        QString macro = m.captured(1);
        QString val = m.captured(2);

        QString pinName;
        if (val.startsWith("PA") || val.startsWith("PB") || val.startsWith("PC") || val.startsWith("PD")) {
            pinName = val;
        } else {
            bool ok = false;
            int num = val.toInt(&ok);
            if (ok) pinName = QString("IO%1").arg(num);
        }

        if (pinName.isEmpty()) continue;

        InferredPin ip;
        ip.pinName = pinName;
        ip.rawSignal = macro;
        ip.supportedModes.append(PinMode::None);

        QString u = macro.toUpper();
        if (u.contains("ADC") || u.contains("ANALOG")) {
            ip.inferredRole = "ADC";
            ip.supportedModes.append(PinMode::AnalogIn);
            ip.defaultMode = PinMode::AnalogIn;
            ip.adc.available = true;
        } else if (u.contains("PWM")) {
            ip.inferredRole = "PWM";
            ip.supportedModes.append(PinMode::PwmOutput);
            ip.defaultMode = PinMode::PwmOutput;
            ip.pwm.available = true;
        } else if (u.contains("SPI")) {
            ip.inferredRole = "SPI";
            ip.supportedModes.append(PinMode::SpiRawTransfer);
            ip.defaultMode = PinMode::SpiRawTransfer;
            ip.spi.available = true;
        } else if (u.contains("I2C") || u.contains("SCL") || u.contains("SDA")) {
            ip.inferredRole = "I2C";
            ip.defaultMode = PinMode::None;
        } else if (u.contains("LED") || u.contains("OUT")) {
            ip.inferredRole = "GPIO Output";
            ip.supportedModes.append(PinMode::DigitalOut);
            ip.defaultMode = PinMode::DigitalOut;
        } else if (u.contains("BTN") || u.contains("KEY") || u.contains("IN")) {
            ip.inferredRole = "GPIO Input";
            ip.supportedModes.append(PinMode::DigitalIn);
            ip.defaultMode = PinMode::DigitalIn;
        } else {
            ip.inferredRole = "Unused / Unknown";
            ip.defaultMode = PinMode::None;
        }

        res.pins.append(ip);
    }

    if (res.pins.isEmpty()) {
        res.success = false;
        res.errorMessage = "No pin definitions found in header.";
        return res;
    }

    res.success = true;
    return res;
}
