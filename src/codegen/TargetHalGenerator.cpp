#include "TargetHalGenerator.h"
#include "analyzer/CapabilityDatabase.h"
#include "driverpack/DriverPackRegistry.h"
#include "driverpack/CustomPackManager.h"
#include <algorithm>

namespace CodeGen {

namespace {

struct TargetResolution {
    bool hasTarget = false;
    bool isDefaultF030 = false;
    bool hasDriverPack = false;
    QString deviceName;
    QString boardDisplayName;
    QString family;
    QMap<QString, PinDef> pinMap;
    QMap<QString, QStringList> protocols;
};

TargetResolution resolveTarget(const Project* project, const QString& targetMcu = QString()) {
    TargetResolution res;
    QString boardId = project ? project->hardwareConfig().boardId.trimmed() : QString();
    if (boardId.isEmpty()) {
        BoardPreset preset = BoardRegistry::instance().activePreset();
        if (!preset.boardId.isEmpty()) {
            boardId = preset.boardId;
        }
    }
    QString deviceId = project ? project->hardwareConfig().deviceId.trimmed() : QString();
    DeviceProfile prof = project ? project->getDeviceProfile() : DeviceProfile();
    if (deviceId.isEmpty()) {
        deviceId = prof.name.trimmed();
    }
    if (deviceId.isEmpty() && !targetMcu.isEmpty() &&
        targetMcu != "stm32" && targetMcu != "generic" &&
        targetMcu != "esp32" && targetMcu != "pico") {
        deviceId = targetMcu.trimmed();
    }

    if (boardId.isEmpty() && deviceId.isEmpty()) {
        res.hasTarget = false;
        res.isDefaultF030 = true;
        res.hasDriverPack = true;
        res.deviceName = "STM32F030R8";
        res.family = "STM32";
        res.boardDisplayName = "STM32 NUCLEO-F030R8";
        BoardDef nucleo = BoardRegistry::instance().board("nucleo_f030r8");
        if (nucleo.isValid()) {
            res.pinMap = nucleo.pinMap;
            res.protocols = nucleo.protocols;
        } else {
            res.pinMap = BoardRegistry::instance().activeBoard().pinMap;
            res.protocols = BoardRegistry::instance().activeBoard().protocols;
        }
        return res;
    }

    res.hasTarget = true;
    BoardDef boardDef;
    if (!boardId.isEmpty()) {
        boardDef = BoardRegistry::instance().board(boardId);
        BoardDef active = BoardRegistry::instance().activeBoard();
        if (active.isValid() && active.id == boardId) {
            boardDef = active;
        }
    }

    if (boardDef.isValid()) {
        res.boardDisplayName = !boardDef.displayName.isEmpty() ? boardDef.displayName : boardDef.id;
        res.family = boardDef.family;
        res.deviceName = !boardDef.mcu.isEmpty() ? boardDef.mcu : boardId;
        res.pinMap = boardDef.pinMap;
        res.protocols = boardDef.protocols;
    } else {
        res.deviceName = !deviceId.isEmpty() ? deviceId : prof.name;
        res.family = !prof.family.isEmpty() ? prof.family : (project ? project->hardwareConfig().family : QString());
        if (res.deviceName == "STM32F030R8") {
            res.boardDisplayName = "STM32 NUCLEO-F030R8";
        } else {
            res.boardDisplayName = res.deviceName;
        }
    }

    if (driverpack::CustomPackManager::instance().isCustomDevice(res.deviceName)) {
        driverpack::CustomPackDef customPack = driverpack::CustomPackManager::instance().getPack(res.deviceName);
        res.family = customPack.family.isEmpty() ? "Custom" : customPack.family;
        res.boardDisplayName = customPack.name + " (Custom Pack)";
        res.pinMap.clear();
        for (const auto& pin : customPack.pinTable) {
            PinDef pd;
            pd.function = pin.function;
            res.pinMap[pin.pin] = pd;
        }
        for (const auto& p : customPack.peripherals) {
            if (!p.type.isEmpty()) {
                res.protocols[p.type].append(p.name);
            }
        }
    }

    if (res.family.isEmpty()) {
        if (res.deviceName.startsWith("STM32", Qt::CaseInsensitive)) res.family = "STM32";
        else if (res.deviceName.startsWith("ESP32", Qt::CaseInsensitive)) res.family = "ESP32";
        else if (res.deviceName.contains("RP2040", Qt::CaseInsensitive) || res.deviceName.contains("Pico", Qt::CaseInsensitive)) res.family = "RP2040";
        else if (res.deviceName.contains("RT10", Qt::CaseInsensitive) || res.deviceName.contains("MIMX", Qt::CaseInsensitive)) res.family = "i.MX RT";
        else if (res.deviceName.contains("nRF", Qt::CaseInsensitive) || res.deviceName.contains("52840", Qt::CaseInsensitive)) res.family = "Nordic";
        else res.family = "Generic";
    }

    if (res.pinMap.isEmpty()) {
        BoardDef synthetic;
        synthetic.family = res.family;
        BoardRegistry::instance().ensureFamilyPinDefaults(synthetic);
        res.pinMap = synthetic.pinMap;
    }

    if (res.protocols.isEmpty()) {
        res.protocols["GPIO"] = {"GPIO"};
        res.protocols["UART"] = {"UART1"};
        res.protocols["SPI"] = {"SPI1"};
        res.protocols["I2C"] = {"I2C1"};
        res.protocols["ADC"] = {"ADC1"};
        res.protocols["PWM"] = {"PWM1"};
    }

    if (res.deviceName == "STM32F030R8" || (boardDef.isValid() && boardDef.id == "nucleo_f030r8")) {
        res.isDefaultF030 = true;
        res.boardDisplayName = "STM32 NUCLEO-F030R8";
        res.family = "STM32";
        if (!boardDef.isValid()) {
            BoardDef nucleo = BoardRegistry::instance().board("nucleo_f030r8");
            if (nucleo.isValid()) {
                res.pinMap = nucleo.pinMap;
                res.protocols = nucleo.protocols;
            }
        }
    }

    auto* pack = driverpack::DriverPackRegistry::instance().findPackForDevice(res.deviceName.toStdString());
    res.hasDriverPack = (pack != nullptr);

    return res;
}

} // namespace

QString TargetHalGenerator::generateHalHeader(const Project* project, const QString& targetMcu) {
    TargetResolution target = resolveTarget(project, targetMcu);
    if (!target.hasDriverPack && !target.isDefaultF030) {
        return QString(); // Unsupported device -> no HAL output
    }

    QString fam = target.family;
    QString board = target.boardDisplayName;

    QString code;
    code += "/**\n";
    code += " * @file target_hal.h\n";
    code += " * @brief Target Hardware Abstraction Layer Adapter\n";
    code += QString(" * Target Family: %1 | Target Board: %2\n").arg(fam, board);
    code += " * Auto-generated by Embedded UI Designer (Phase 4 Code Generator)\n";
    code += " */\n\n";
    code += "#ifndef TARGET_HAL_H\n";
    code += "#define TARGET_HAL_H\n\n";
    code += "#include <stdint.h>\n";
    code += "#include <stdbool.h>\n";

    if (target.isDefaultF030) {
        // Keep byte-identical to golden STM32F030R8 baseline
    } else if (fam == "ESP32") {
        code += "#include \"driver/gpio.h\"\n";
        code += "#include \"esp_timer.h\"\n";
    } else if (fam == "RP2040") {
        code += "#include \"pico/stdlib.h\"\n";
        code += "#include \"hardware/gpio.h\"\n";
    } else if (fam == "STM32") {
        if (target.deviceName.contains("F4", Qt::CaseInsensitive)) {
            code += "#include \"stm32f4xx_hal.h\"\n";
        } else {
            code += "#include \"stm32_hal.h\"\n";
        }
    } else if (fam == "i.MX RT") {
        code += "#include \"fsl_gpio.h\"\n";
        code += "#include \"fsl_common.h\"\n";
    } else if (fam == "Nordic") {
        code += "#include <zephyr/kernel.h>\n";
        code += "#include <zephyr/drivers/gpio.h>\n";
    } else if (driverpack::CustomPackManager::instance().isCustomDevice(target.deviceName)) {
        driverpack::CustomPackDef customPack = driverpack::CustomPackManager::instance().getPack(target.deviceName);
        code += QString("/* Custom device: %1 (%2) */\n").arg(customPack.name, customPack.vendor);
        code += QString("#include \"%1_registers.h\"\n").arg(customPack.name.toLower());
    }

    code += "\n#ifdef __cplusplus\n";
    code += "extern \"C\" {\n";
    code += "#endif\n\n";

    code += "/* Target Board Pin Definitions */\n";
    for (auto it = target.pinMap.begin(); it != target.pinMap.end(); ++it) {
        code += QString("#define PIN_%1 \"%1\" /* %2%3 */\n")
            .arg(it.key(), it.value().function,
                 it.value().strapping ? " [Strapping]" : (it.value().reserved ? " [Reserved]" : ""));
    }

    code += "\n/* Supported Hardware Protocols */\n";
    for (auto it = target.protocols.begin(); it != target.protocols.end(); ++it) {
        code += QString("#define PROTOCOL_HAS_%1 1 /* %2 */\n").arg(it.key(), it.value().join(", "));
    }

    code += "\n/* HAL Lifecycle Initializers */\n";
    code += "void target_hal_init(void);\n";
    code += "void target_hal_poll(void);\n\n";

    code += "/* Digital GPIO */\n";
    code += "bool hal_read_digital_pin(const char* pin_name);\n";
    code += "void hal_write_digital_pin(const char* pin_name, bool high);\n\n";

    code += "/* Analog ADC */\n";
    code += "uint32_t hal_read_adc_raw(const char* pin_name);\n";
    code += "float hal_read_adc_normalized(const char* pin_name);\n\n";

    code += "/* PWM Output */\n";
    code += "void hal_write_pwm_duty(const char* pin_name, float duty_percent);\n\n";

    code += "/* Sensor Readouts */\n";
    code += "float hal_read_sensor_value(const char* sensor_name);\n\n";

    code += "#ifdef __cplusplus\n";
    code += "}\n";
    code += "#endif\n\n";
    code += "#endif /* TARGET_HAL_H */\n";
    return code;
}

QString TargetHalGenerator::generateDriverInitCode(const BoardDef& board,
                                                 const QString& protocol,
                                                 const QMap<QString, QString>& pinAssignments)
{
    QString code;
    QString fam = board.family.isEmpty() ? "Generic" : board.family;
    QString p = protocol.toUpper();

    if (p == "SPI") {
        QString bus = board.protocols.contains("SPI") && !board.protocols["SPI"].isEmpty() ? board.protocols["SPI"].first() : "SPI1";
        QString sck = pinAssignments.value("sck", "SCK");
        QString mosi = pinAssignments.value("mosi", "MOSI");
        QString miso = pinAssignments.value("miso", "MISO");
        QString ss = pinAssignments.value("ss", "SS");
        code += QString("/* SPI Driver: %1 on %2 (%3) */\n").arg(bus, board.displayName.isEmpty() ? board.id : board.displayName, fam);
        code += "void hal_spi_init(void) {\n";
        code += QString("    printf(\"[HAL: %1] Configuring SPI bus %2 (SCK=%3, MOSI=%4, MISO=%5, CS=%6)...\\n\");\n")
            .arg(fam, bus, sck, mosi, miso, ss);
        if (fam.contains("STM32", Qt::CaseInsensitive)) {
            code += QString("    /* __HAL_RCC_%1_CLK_ENABLE(); */\n").arg(bus);
            code += QString("    /* h%1.Instance = %2; h%1.Init.Mode = SPI_MODE_MASTER; */\n").arg(bus.toLower(), bus);
        } else if (fam.contains("ESP32", Qt::CaseInsensitive)) {
            code += QString("    /* spi_bus_config_t buscfg = {.mosi_io_num = %1, .miso_io_num = %2, .sclk_io_num = %3}; */\n").arg(mosi, miso, sck);
            code += "    /* spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO); */\n";
        } else if (fam.contains("RP2040", Qt::CaseInsensitive)) {
            code += QString("    /* spi_init(spi0, 1000 * 1000); gpio_set_function(%1, GPIO_FUNC_SPI); */\n").arg(sck);
        } else if (fam.contains("i.MX", Qt::CaseInsensitive) || fam.compare("NXP", Qt::CaseInsensitive) == 0) {
            code += "    /* CLOCK_EnableClock(kCLOCK_Lpspi1); */\n";
        } else if (fam.contains("Nordic", Qt::CaseInsensitive) || fam.contains("nRF", Qt::CaseInsensitive)) {
            code += "    /* nrfx_spim_init(&spim_instance, &spim_config, NULL, NULL); */\n";
        }
        code += "}\n";
    } else if (p == "I2C") {
        QString bus = board.protocols.contains("I2C") && !board.protocols["I2C"].isEmpty() ? board.protocols["I2C"].first() : "I2C1";
        QString scl = pinAssignments.value("scl", "SCL");
        QString sda = pinAssignments.value("sda", "SDA");
        code += QString("/* I2C Driver: %1 on %2 (%3) */\n").arg(bus, board.displayName.isEmpty() ? board.id : board.displayName, fam);
        code += "void hal_i2c_init(void) {\n";
        code += QString("    printf(\"[HAL: %1] Configuring I2C bus %2 (SCL=%3, SDA=%4)...\\n\");\n")
            .arg(fam, bus, scl, sda);
        if (fam.contains("STM32", Qt::CaseInsensitive)) {
            code += QString("    /* __HAL_RCC_%1_CLK_ENABLE(); */\n").arg(bus);
        } else if (fam.contains("ESP32", Qt::CaseInsensitive)) {
            code += QString("    /* i2c_config_t conf = {.scl_io_num = %1, .sda_io_num = %2}; */\n").arg(scl, sda);
        } else if (fam.contains("RP2040", Qt::CaseInsensitive)) {
            code += QString("    /* i2c_init(i2c0, 100 * 1000); gpio_set_function(%1, GPIO_FUNC_I2C); */\n").arg(sda);
        } else if (fam.contains("i.MX", Qt::CaseInsensitive) || fam.compare("NXP", Qt::CaseInsensitive) == 0) {
            code += "    /* LPI2C_MasterInit(LPI2C1, &masterConfig, CLOCK_GetFreq(kCLOCK_OscClk)); */\n";
        } else if (fam.contains("Nordic", Qt::CaseInsensitive) || fam.contains("nRF", Qt::CaseInsensitive)) {
            code += "    /* nrfx_twim_init(&twim_instance, &twim_config, NULL, NULL); */\n";
        }
        code += "}\n";
    } else if (p == "PWM") {
        code += QString("/* PWM Timer Driver: %1 */\n").arg(board.displayName.isEmpty() ? board.id : board.displayName);
        code += "void hal_pwm_init(void) {\n";
        code += QString("    printf(\"[HAL: %1] Initializing PWM timer hardware channels...\\n\");\n").arg(fam);
        code += "}\n";
    } else if (p == "ADC") {
        code += QString("/* ADC Driver: %1 */\n").arg(board.displayName.isEmpty() ? board.id : board.displayName);
        code += "void hal_adc_init(void) {\n";
        code += QString("    printf(\"[HAL: %1] Calibrating and enabling ADC converter...\\n\");\n").arg(fam);
        code += "}\n";
    } else if (p == "UART" || p == "USART") {
        QString bus = board.protocols.contains("UART") && !board.protocols["UART"].isEmpty() ? board.protocols["UART"].first() : "UART1";
        code += QString("/* UART Driver: %1 */\n").arg(bus);
        code += "void hal_uart_init(void) {\n";
        code += QString("    printf(\"[HAL: %1] Configuring Serial UART %2 at 115200 baud...\\n\");\n").arg(fam, bus);
        code += "}\n";
    } else if (p == "GPIO") {
        QString pin = pinAssignments.value("pin", "GPIO");
        code += QString("/* GPIO Driver: %1 */\n").arg(pin);
        code += "void hal_gpio_init(void) {\n";
        code += QString("    printf(\"[HAL: %1] Configuring pin %2 as Digital Output...\\n\");\n").arg(fam, pin);
        code += "}\n";
    } else {
        code += "/* Generic Idle Driver */\n";
        code += "void hal_idle_init(void) {\n";
        code += "    /* No active protocol driver configured */\n";
        code += "}\n";
    }
    return code;
}

QString TargetHalGenerator::generateHalSource(const Project* project, const QString& targetMcu) {
    TargetResolution target = resolveTarget(project, targetMcu);
    if (!target.hasDriverPack && !target.isDefaultF030) {
        return QString(); // Unsupported device -> no HAL output
    }

    QString fam = target.family;
    QString board = target.boardDisplayName;

    BoardDef activeBoard;
    activeBoard.family = fam;
    activeBoard.displayName = board;
    activeBoard.id = target.deviceName;
    activeBoard.pinMap = target.pinMap;
    activeBoard.protocols = target.protocols;

    QString code;
    code += "/**\n";
    code += " * @file target_hal.c\n";
    code += " * @brief Target Hardware Abstraction Layer Adapter Implementation\n";
    code += QString(" * Target Family: %1 | Target Board: %2\n").arg(fam, board);
    code += " * Auto-generated by Embedded UI Designer\n";
    code += " */\n\n";
    code += "#include \"target_hal.h\"\n";
    code += "#include <stdio.h>\n";
    code += "#include <string.h>\n\n";

    // Driver routines for board's protocols
    code += "/* Protocol Driver Implementations */\n";
    for (auto it = activeBoard.protocols.begin(); it != activeBoard.protocols.end(); ++it) {
        code += generateDriverInitCode(activeBoard, it.key());
        code += "\n";
    }

    code += "/* Central Hardware Initialization */\n";
    code += "void target_hal_init(void) {\n";
    code += QString("    printf(\"[HAL: %1] Initializing target board: %2\\n\");\n").arg(fam, board);

    if (activeBoard.protocols.contains("SPI")) code += "    hal_spi_init();\n";
    if (activeBoard.protocols.contains("I2C")) code += "    hal_i2c_init();\n";
    if (activeBoard.protocols.contains("PWM")) code += "    hal_pwm_init();\n";
    if (activeBoard.protocols.contains("ADC")) code += "    hal_adc_init();\n";
    if (activeBoard.protocols.contains("UART")) code += "    hal_uart_init();\n";
    code += "}\n\n";

    if (driverpack::CustomPackManager::instance().isCustomDevice(target.deviceName)) {
        code += "bool hal_read_digital_pin(const char* pin_name) {\n";
        code += QString("    printf(\"[HAL: Custom %1] gpio_read(%%s)\\n\", pin_name);\n").arg(target.deviceName);
        code += "    return false;\n";
        code += "}\n\n";
    } else {
        code += "bool hal_read_digital_pin(const char* pin_name) {\n";
        code += "    (void)pin_name;\n";
        code += "    return false;\n";
        code += "}\n\n";
    }

    if (driverpack::CustomPackManager::instance().isCustomDevice(target.deviceName)) {
        driverpack::CustomPackDef customPack = driverpack::CustomPackManager::instance().getPack(target.deviceName);
        code += "void hal_write_digital_pin(const char* pin_name, bool high) {\n";
        code += QString("    /* Generated for custom device %1 (SVD-validated base addresses) */\n").arg(customPack.name);
        code += QString("    printf(\"[HAL: Custom %1] gpio_write(%%s, %%d)\\n\", pin_name, high ? 1 : 0);\n").arg(customPack.name);
        code += "}\n\n";
    } else if (fam.compare("STM32", Qt::CaseInsensitive) == 0) {
        code += "void hal_write_digital_pin(const char* pin_name, bool high) {\n";
        code += "    printf(\"[HAL: STM32] Atomic BSRR Write pin %s -> %s\\n\", pin_name, high ? \"HIGH (1)\" : \"LOW (0)\");\n";
        code += "    /* GPIOA->BSRR = high ? (1 << pin_num) : (1 << (pin_num + 16)); */\n";
        code += "}\n\n";
    } else if (fam.compare("ESP32", Qt::CaseInsensitive) == 0) {
        code += "void hal_write_digital_pin(const char* pin_name, bool high) {\n";
        code += "    printf(\"[HAL: ESP32] gpio_set_level(%s, %d)\\n\", pin_name, high ? 1 : 0);\n";
        code += "}\n\n";
    } else if (fam.compare("RP2040", Qt::CaseInsensitive) == 0) {
        code += "void hal_write_digital_pin(const char* pin_name, bool high) {\n";
        code += "    printf(\"[HAL: RP2040] gpio_put(%s, %d)\\n\", pin_name, high ? 1 : 0);\n";
        code += "}\n\n";
    } else if (fam.contains("i.MX", Qt::CaseInsensitive) || fam.compare("NXP", Qt::CaseInsensitive) == 0) {
        code += "void hal_write_digital_pin(const char* pin_name, bool high) {\n";
        code += "    printf(\"[HAL: i.MX RT] GPIO_PinWrite(%s, %d)\\n\", pin_name, high ? 1 : 0);\n";
        code += "}\n\n";
    } else if (fam.contains("Nordic", Qt::CaseInsensitive) || fam.contains("nRF", Qt::CaseInsensitive)) {
        code += "void hal_write_digital_pin(const char* pin_name, bool high) {\n";
        code += "    printf(\"[HAL: Nordic] gpio_pin_set(dev, %s, %d)\\n\", pin_name, high ? 1 : 0);\n";
        code += "}\n\n";
    } else {
        code += "void hal_write_digital_pin(const char* pin_name, bool high) {\n";
        code += QString("    printf(\"[HAL: %1] Atomic Write pin %s -> %s\\n\", pin_name, high ? \"HIGH\" : \"LOW\");\n").arg(fam);
        code += "}\n\n";
    }

    code += "uint32_t hal_read_adc_raw(const char* pin_name) {\n";
    code += "    (void)pin_name;\n";
    code += "    return 2048;\n";
    code += "}\n\n";

    code += "float hal_read_adc_normalized(const char* pin_name) {\n";
    code += "    return (float)hal_read_adc_raw(pin_name) / 4095.0f;\n";
    code += "}\n\n";

    code += "void hal_write_pwm_duty(const char* pin_name, float duty_percent) {\n";
    code += QString("    printf(\"[HAL: %1] Set PWM duty on %s -> %.1f%%\\n\", pin_name, duty_percent);\n").arg(fam);
    code += "}\n\n";

    code += "float hal_read_sensor_value(const char* sensor_name) {\n";
    code += "    (void)sensor_name;\n";
    code += "    return 25.0f;\n";
    code += "}\n\n";

    code += "void target_hal_poll(void) {\n";
    code += "    /* Poll hardware peripherals and tick counters */\n";
    code += "}\n";

    return code;
}

} // namespace CodeGen
