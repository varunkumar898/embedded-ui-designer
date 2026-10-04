#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include "HardwareBridge.h"

class BoardConfigParser {
public:
    enum class ConfigFormat {
        Unknown,
        Stm32CubeIoc,
        EspIdfSdkConfig,
        CPinHeader
    };

    struct InferredPin {
        QString pinName;
        QString rawSignal;
        QString inferredRole;      // "ADC", "PWM", "GPIO Output", "GPIO Input", "SPI", "I2C", "Unused / Unknown"
        QList<PinMode> supportedModes;
        PinMode defaultMode = PinMode::None;
        AdcMapping adc;
        PwmMapping pwm;
        SpiMapping spi;
        QString notes;
    };

    struct ParseResult {
        bool success = false;
        QString errorMessage;
        ConfigFormat format = ConfigFormat::Unknown;
        QString boardId;
        QString boardName;
        QString mcuFamily;
        QList<InferredPin> pins;

        BoardProfile toBoardProfile() const;
    };

    static ParseResult parseFile(const QString& filePath);
    static ParseResult parseContent(const QString& content, const QString& fileNameHint = QString());

private:
    static ParseResult parseStm32Ioc(const QString& content, const QString& fileName);
    static ParseResult parseEspSdkConfig(const QString& content, const QString& fileName);
    static ParseResult parseCPinHeader(const QString& content, const QString& fileName);
};
