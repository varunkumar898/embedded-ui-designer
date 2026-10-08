#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace Hardware {

/**
 * @brief Data-driven capability model for hardware targets and backends.
 * Eliminates hardcoded platform conditionals in UI and component code.
 */
struct HardwareCapabilities {
    bool gpioInput = false;
    bool gpioOutput = false;
    bool adc = false;
    bool pwm = false;
    bool uart = false;
    bool spi = false;
    bool i2c = false;
    bool can = false;
    bool reset = false;
    bool liveMonitoring = false;
    bool rawMemoryAccess = false;
    bool atomicBsrr = false;

    QJsonObject toJson() const {
        QJsonObject json;
        json["gpioInput"] = gpioInput;
        json["gpioOutput"] = gpioOutput;
        json["adc"] = adc;
        json["pwm"] = pwm;
        json["uart"] = uart;
        json["spi"] = spi;
        json["i2c"] = i2c;
        json["can"] = can;
        json["reset"] = reset;
        json["liveMonitoring"] = liveMonitoring;
        json["rawMemoryAccess"] = rawMemoryAccess;
        json["atomicBsrr"] = atomicBsrr;
        return json;
    }

    static HardwareCapabilities fromJson(const QJsonObject& json) {
        HardwareCapabilities cap;
        cap.gpioInput = json.value("gpioInput").toBool(false);
        cap.gpioOutput = json.value("gpioOutput").toBool(false);
        cap.adc = json.value("adc").toBool(false);
        cap.pwm = json.value("pwm").toBool(false);
        cap.uart = json.value("uart").toBool(false);
        cap.spi = json.value("spi").toBool(false);
        cap.i2c = json.value("i2c").toBool(false);
        cap.can = json.value("can").toBool(false);
        cap.reset = json.value("reset").toBool(false);
        cap.liveMonitoring = json.value("liveMonitoring").toBool(false);
        cap.rawMemoryAccess = json.value("rawMemoryAccess").toBool(false);
        cap.atomicBsrr = json.value("atomicBsrr").toBool(false);
        return cap;
    }

    QStringList summaryList() const {
        QStringList list;
        if (gpioInput) list << "GPIO In";
        if (gpioOutput) list << "GPIO Out";
        if (adc) list << "ADC";
        if (pwm) list << "PWM";
        if (uart) list << "UART";
        if (spi) list << "SPI";
        if (i2c) list << "I2C";
        if (can) list << "CAN";
        if (reset) list << "Reset";
        if (liveMonitoring) list << "Live Monitor";
        return list;
    }
};

} // namespace Hardware
