#include "DataSource.h"
#include <QRegularExpression>
#include <QJsonDocument>
#include <QtMath>
#include <cstring>

// ============================================================================
// String Conversion Utilities
// ============================================================================
QString dataSourceTypeToString(DataSourceType type) {
    switch (type) {
        case DataSourceType::Gpio:        return "GPIO";
        case DataSourceType::Adc:         return "ADC";
        case DataSourceType::Pwm:         return "PWM";
        case DataSourceType::Uart:        return "UART";
        case DataSourceType::I2c:         return "I2C";
        case DataSourceType::Spi:         return "SPI";
        case DataSourceType::Can:         return "CAN";
        case DataSourceType::Sensor:      return "SENSOR";
        case DataSourceType::Variable:    return "VARIABLE";
        case DataSourceType::Constant:    return "CONSTANT";
        case DataSourceType::Timer:       return "TIMER";
        case DataSourceType::Calculated:  return "CALCULATED";
        case DataSourceType::Modbus:      return "MODBUS";
        default:                          return "VARIABLE";
    }
}

DataSourceType stringToDataSourceType(const QString& str) {
    QString upper = str.trimmed().toUpper();
    if (upper == "GPIO") return DataSourceType::Gpio;
    if (upper == "ADC") return DataSourceType::Adc;
    if (upper == "PWM") return DataSourceType::Pwm;
    if (upper == "UART") return DataSourceType::Uart;
    if (upper == "I2C") return DataSourceType::I2c;
    if (upper == "SPI") return DataSourceType::Spi;
    if (upper == "CAN") return DataSourceType::Can;
    if (upper == "SENSOR") return DataSourceType::Sensor;
    if (upper == "VARIABLE") return DataSourceType::Variable;
    if (upper == "CONSTANT") return DataSourceType::Constant;
    if (upper == "TIMER") return DataSourceType::Timer;
    if (upper == "CALCULATED") return DataSourceType::Calculated;
    if (upper == "MODBUS") return DataSourceType::Modbus;
    return DataSourceType::Variable;
}

QString dataDirectionToString(DataDirection dir) {
    switch (dir) {
        case DataDirection::Input:         return "input";
        case DataDirection::Output:        return "output";
        case DataDirection::Bidirectional: return "bidirectional";
        default:                           return "input";
    }
}

DataDirection stringToDataDirection(const QString& str) {
    QString lower = str.trimmed().toLower();
    if (lower == "output" || lower == "out" || lower == "write") return DataDirection::Output;
    if (lower == "bidirectional" || lower == "both" || lower == "read_write" || lower == "readwrite") return DataDirection::Bidirectional;
    return DataDirection::Input;
}

QString dataTypeToString(DataType type) {
    switch (type) {
        case DataType::Boolean:          return "boolean";
        case DataType::Integer:          return "integer";
        case DataType::UnsignedInteger:  return "unsigned_integer";
        case DataType::Float:            return "float";
        case DataType::String:           return "string";
        case DataType::Enum:             return "enum";
        case DataType::Raw:              return "raw";
        default:                         return "float";
    }
}

DataType stringToDataType(const QString& str) {
    QString lower = str.trimmed().toLower();
    if (lower == "boolean" || lower == "bool") return DataType::Boolean;
    if (lower == "integer" || lower == "int") return DataType::Integer;
    if (lower == "unsigned_integer" || lower == "uint") return DataType::UnsignedInteger;
    if (lower == "float" || lower == "double" || lower == "number") return DataType::Float;
    if (lower == "string" || lower == "str" || lower == "text") return DataType::String;
    if (lower == "enum") return DataType::Enum;
    if (lower == "raw" || lower == "binary") return DataType::Raw;
    return DataType::Float;
}

// ============================================================================
// CAN Signal Config Implementation
// ============================================================================
QJsonObject CanSignalConfig::toJson() const {
    QJsonObject obj;
    obj["busId"] = busId;
    obj["messageId"] = static_cast<double>(messageId);
    obj["isExtended"] = isExtended;
    obj["isFd"] = isFd;
    obj["bitrate"] = bitrate;
    obj["startBit"] = startBit;
    obj["bitLength"] = bitLength;
    obj["isBigEndian"] = isBigEndian;
    obj["isSigned"] = isSigned;
    obj["factor"] = factor;
    obj["offset"] = offset;
    obj["minVal"] = minVal;
    obj["maxVal"] = maxVal;
    if (!unit.isEmpty()) obj["unit"] = unit;
    return obj;
}

CanSignalConfig CanSignalConfig::fromJson(const QJsonObject& obj) {
    CanSignalConfig cfg;
    cfg.busId = obj.value("busId").toString(cfg.busId);
    cfg.messageId = static_cast<quint32>(obj.value("messageId").toDouble(cfg.messageId));
    cfg.isExtended = obj.value("isExtended").toBool(cfg.isExtended);
    cfg.isFd = obj.value("isFd").toBool(cfg.isFd);
    cfg.bitrate = obj.value("bitrate").toInt(cfg.bitrate);
    cfg.startBit = obj.value("startBit").toInt(cfg.startBit);
    cfg.bitLength = obj.value("bitLength").toInt(cfg.bitLength);
    cfg.isBigEndian = obj.value("isBigEndian").toBool(cfg.isBigEndian);
    cfg.isSigned = obj.value("isSigned").toBool(cfg.isSigned);
    cfg.factor = obj.value("factor").toDouble(cfg.factor);
    cfg.offset = obj.value("offset").toDouble(cfg.offset);
    cfg.minVal = obj.value("minVal").toDouble(cfg.minVal);
    cfg.maxVal = obj.value("maxVal").toDouble(cfg.maxVal);
    cfg.unit = obj.value("unit").toString();
    return cfg;
}

double CanSignalConfig::decodePayload(const QByteArray& payload) const {
    if (payload.isEmpty() || bitLength <= 0) return offset;

    quint64 rawValue = 0;
    if (!isBigEndian) {
        // Intel Little Endian extraction
        quint64 fullBuf = 0;
        int maxBytes = qMin(8, payload.size());
        for (int i = 0; i < maxBytes; ++i) {
            fullBuf |= (static_cast<quint64>(static_cast<quint8>(payload[i])) << (i * 8));
        }
        quint64 mask = (bitLength >= 64) ? ~0ULL : ((1ULL << bitLength) - 1ULL);
        if (startBit < 64) {
            rawValue = (fullBuf >> startBit) & mask;
        }
    } else {
        // Motorola Big Endian extraction
        quint64 fullBuf = 0;
        int maxBytes = qMin(8, payload.size());
        for (int i = 0; i < maxBytes; ++i) {
            fullBuf = (fullBuf << 8) | static_cast<quint8>(payload[i]);
        }
        int totalBits = maxBytes * 8;
        quint64 mask = (bitLength >= 64) ? ~0ULL : ((1ULL << bitLength) - 1ULL);
        int shift = totalBits - startBit - bitLength;
        if (shift >= 0 && shift < 64) {
            rawValue = (fullBuf >> shift) & mask;
        } else {
            rawValue = (fullBuf >> qMax(0, 64 - startBit - bitLength)) & mask;
        }
    }

    if (isSigned && bitLength < 64) {
        quint64 signBit = 1ULL << (bitLength - 1);
        if (rawValue & signBit) {
            qint64 signedVal = static_cast<qint64>(rawValue | (~0ULL << bitLength));
            return (signedVal * factor) + offset;
        }
    }
    return (static_cast<double>(rawValue) * factor) + offset;
}

void CanSignalConfig::encodePayload(double value, QByteArray& payload) const {
    if (payload.size() < 8) {
        payload.resize(isFd ? 64 : 8);
        payload.fill(0);
    }
    double unscaled = (value - offset) / (factor != 0.0 ? factor : 1.0);
    quint64 rawValue = static_cast<quint64>(qRound64(unscaled));
    quint64 mask = (bitLength >= 64) ? ~0ULL : ((1ULL << bitLength) - 1ULL);
    rawValue &= mask;

    if (!isBigEndian) {
        quint64 fullBuf = 0;
        int maxBytes = qMin(8, payload.size());
        for (int i = 0; i < maxBytes; ++i) {
            fullBuf |= (static_cast<quint64>(static_cast<quint8>(payload[i])) << (i * 8));
        }
        fullBuf &= ~(mask << startBit);
        fullBuf |= (rawValue << startBit);
        for (int i = 0; i < maxBytes; ++i) {
            payload[i] = static_cast<char>((fullBuf >> (i * 8)) & 0xFF);
        }
    }
}

bool CanSignalConfig::operator==(const CanSignalConfig& other) const {
    return busId == other.busId &&
           messageId == other.messageId &&
           isExtended == other.isExtended &&
           isFd == other.isFd &&
           bitrate == other.bitrate &&
           startBit == other.startBit &&
           bitLength == other.bitLength &&
           isBigEndian == other.isBigEndian &&
           isSigned == other.isSigned &&
           qFuzzyCompare(factor, other.factor) &&
           qFuzzyCompare(offset, other.offset) &&
           unit == other.unit;
}

// ============================================================================
// UART Stream Config Implementation
// ============================================================================
QString uartParseModeToString(UartParseMode mode) {
    switch (mode) {
        case UartParseMode::Raw:            return "raw";
        case UartParseMode::DelimiterIndex: return "delimiter_index";
        case UartParseMode::KeyValue:       return "key_value";
        case UartParseMode::RegexCapture:   return "regex_capture";
        case UartParseMode::JsonPath:       return "json_path";
        default:                            return "delimiter_index";
    }
}

UartParseMode stringToUartParseMode(const QString& str) {
    QString lower = str.trimmed().toLower();
    if (lower == "raw") return UartParseMode::Raw;
    if (lower == "delimiter_index" || lower == "csv" || lower == "token") return UartParseMode::DelimiterIndex;
    if (lower == "key_value" || lower == "keyvalue") return UartParseMode::KeyValue;
    if (lower == "regex_capture" || lower == "regex") return UartParseMode::RegexCapture;
    if (lower == "json_path" || lower == "json") return UartParseMode::JsonPath;
    return UartParseMode::DelimiterIndex;
}

QJsonObject UartStreamConfig::toJson() const {
    QJsonObject obj;
    obj["portName"] = portName;
    obj["baudRate"] = baudRate;
    obj["dataBits"] = dataBits;
    obj["stopBits"] = stopBits;
    obj["parity"] = parity;
    obj["parseMode"] = uartParseModeToString(parseMode);
    obj["delimiter"] = delimiter;
    obj["tokenIndex"] = tokenIndex;
    if (!keyName.isEmpty()) obj["keyName"] = keyName;
    if (!regexPattern.isEmpty()) obj["regexPattern"] = regexPattern;
    obj["factor"] = factor;
    obj["offset"] = offset;
    return obj;
}

UartStreamConfig UartStreamConfig::fromJson(const QJsonObject& obj) {
    UartStreamConfig cfg;
    cfg.portName = obj.value("portName").toString(cfg.portName);
    cfg.baudRate = obj.value("baudRate").toInt(cfg.baudRate);
    cfg.dataBits = obj.value("dataBits").toInt(cfg.dataBits);
    cfg.stopBits = obj.value("stopBits").toInt(cfg.stopBits);
    cfg.parity = obj.value("parity").toString(cfg.parity);
    cfg.parseMode = stringToUartParseMode(obj.value("parseMode").toString());
    cfg.delimiter = obj.value("delimiter").toString(cfg.delimiter);
    cfg.tokenIndex = obj.value("tokenIndex").toInt(cfg.tokenIndex);
    cfg.keyName = obj.value("keyName").toString();
    cfg.regexPattern = obj.value("regexPattern").toString();
    cfg.factor = obj.value("factor").toDouble(cfg.factor);
    cfg.offset = obj.value("offset").toDouble(cfg.offset);
    return cfg;
}

QVariant UartStreamConfig::parseIncomingText(const QString& frame) const {
    QString trimmed = frame.trimmed();
    if (trimmed.isEmpty()) return QVariant();

    switch (parseMode) {
        case UartParseMode::Raw:
            return trimmed;

        case UartParseMode::DelimiterIndex: {
            QString sep = delimiter.isEmpty() ? "," : delimiter;
            QStringList tokens = trimmed.split(sep);
            if (tokenIndex >= 0 && tokenIndex < tokens.size()) {
                QString tok = tokens[tokenIndex].trimmed();
                bool ok = false;
                double num = tok.toDouble(&ok);
                if (ok) return (num * factor) + offset;
                return tok;
            }
            break;
        }

        case UartParseMode::KeyValue: {
            if (keyName.isEmpty()) return QVariant();
            QString pattern = QString("(?:^|[;,\\s])%1\\s*[:=]\\s*([^;,\\s]+)").arg(QRegularExpression::escape(keyName));
            QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch m = re.match(trimmed);
            if (m.hasMatch()) {
                QString valStr = m.captured(1).trimmed();
                bool ok = false;
                double num = valStr.toDouble(&ok);
                if (ok) return (num * factor) + offset;
                return valStr;
            }
            break;
        }

        case UartParseMode::RegexCapture: {
            if (regexPattern.isEmpty()) return QVariant();
            QRegularExpression re(regexPattern);
            QRegularExpressionMatch m = re.match(trimmed);
            if (m.hasMatch()) {
                QString captured = m.lastCapturedIndex() >= 1 ? m.captured(1).trimmed() : m.captured(0).trimmed();
                bool ok = false;
                double num = captured.toDouble(&ok);
                if (ok) return (num * factor) + offset;
                return captured;
            }
            break;
        }

        case UartParseMode::JsonPath: {
            QJsonDocument doc = QJsonDocument::fromJson(trimmed.toUtf8());
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.contains(keyName)) {
                    QJsonValue val = obj.value(keyName);
                    if (val.isDouble()) return (val.toDouble() * factor) + offset;
                    return val.toVariant();
                }
            }
            break;
        }
    }
    return QVariant();
}

bool UartStreamConfig::operator==(const UartStreamConfig& other) const {
    return portName == other.portName &&
           baudRate == other.baudRate &&
           dataBits == other.dataBits &&
           stopBits == other.stopBits &&
           parity == other.parity &&
           parseMode == other.parseMode &&
           delimiter == other.delimiter &&
           tokenIndex == other.tokenIndex &&
           keyName == other.keyName &&
           regexPattern == other.regexPattern &&
           qFuzzyCompare(factor, other.factor) &&
           qFuzzyCompare(offset, other.offset);
}

// ============================================================================
// Modbus Config Implementation
// ============================================================================
QString modbusModeToString(ModbusMode mode) {
    switch (mode) {
        case ModbusMode::Rtu:   return "rtu";
        case ModbusMode::Tcp:   return "tcp";
        case ModbusMode::Ascii: return "ascii";
        default:                return "rtu";
    }
}

ModbusMode stringToModbusMode(const QString& str) {
    QString lower = str.trimmed().toLower();
    if (lower == "tcp") return ModbusMode::Tcp;
    if (lower == "ascii") return ModbusMode::Ascii;
    return ModbusMode::Rtu;
}

QString modbusRegisterTypeToString(ModbusRegisterType type) {
    switch (type) {
        case ModbusRegisterType::Coil:            return "coil";
        case ModbusRegisterType::DiscreteInput:   return "discrete_input";
        case ModbusRegisterType::HoldingRegister: return "holding_register";
        case ModbusRegisterType::InputRegister:    return "input_register";
        default:                                  return "holding_register";
    }
}

ModbusRegisterType stringToModbusRegisterType(const QString& str) {
    QString lower = str.trimmed().toLower();
    if (lower == "coil" || lower == "coils") return ModbusRegisterType::Coil;
    if (lower == "discrete_input" || lower == "discrete" || lower == "di") return ModbusRegisterType::DiscreteInput;
    if (lower == "input_register" || lower == "input" || lower == "ir") return ModbusRegisterType::InputRegister;
    return ModbusRegisterType::HoldingRegister;
}

QString modbusDataTypeToString(ModbusDataType type) {
    switch (type) {
        case ModbusDataType::Bit:       return "bit";
        case ModbusDataType::Int16:     return "int16";
        case ModbusDataType::Uint16:    return "uint16";
        case ModbusDataType::Int32BE:   return "int32_be";
        case ModbusDataType::Int32LE:   return "int32_le";
        case ModbusDataType::Uint32BE:  return "uint32_be";
        case ModbusDataType::Uint32LE:  return "uint32_le";
        case ModbusDataType::Float32BE: return "float32_be";
        case ModbusDataType::Float32LE: return "float32_le";
        default:                        return "uint16";
    }
}

ModbusDataType stringToModbusDataType(const QString& str) {
    QString lower = str.trimmed().toLower();
    if (lower == "bit" || lower == "bool") return ModbusDataType::Bit;
    if (lower == "int16" || lower == "short") return ModbusDataType::Int16;
    if (lower == "uint16" || lower == "ushort") return ModbusDataType::Uint16;
    if (lower == "int32_be" || lower == "int32") return ModbusDataType::Int32BE;
    if (lower == "int32_le") return ModbusDataType::Int32LE;
    if (lower == "uint32_be" || lower == "uint32") return ModbusDataType::Uint32BE;
    if (lower == "uint32_le") return ModbusDataType::Uint32LE;
    if (lower == "float32_be" || lower == "float32" || lower == "float") return ModbusDataType::Float32BE;
    if (lower == "float32_le") return ModbusDataType::Float32LE;
    return ModbusDataType::Uint16;
}

QJsonObject ModbusConfig::toJson() const {
    QJsonObject obj;
    obj["mode"] = modbusModeToString(mode);
    obj["slaveId"] = slaveId;
    obj["registerType"] = modbusRegisterTypeToString(registerType);
    obj["address"] = address;
    obj["dataType"] = modbusDataTypeToString(dataType);
    obj["pollIntervalMs"] = pollIntervalMs;
    obj["scale"] = scale;
    obj["offset"] = offset;
    return obj;
}

ModbusConfig ModbusConfig::fromJson(const QJsonObject& obj) {
    ModbusConfig cfg;
    cfg.mode = stringToModbusMode(obj.value("mode").toString());
    cfg.slaveId = obj.value("slaveId").toInt(cfg.slaveId);
    cfg.registerType = stringToModbusRegisterType(obj.value("registerType").toString());
    cfg.address = obj.value("address").toInt(cfg.address);
    cfg.dataType = stringToModbusDataType(obj.value("dataType").toString());
    cfg.pollIntervalMs = obj.value("pollIntervalMs").toInt(cfg.pollIntervalMs);
    cfg.scale = obj.value("scale").toDouble(cfg.scale);
    cfg.offset = obj.value("offset").toDouble(cfg.offset);
    return cfg;
}

QVariant ModbusConfig::decodeRegisters(const QVector<quint16>& rawRegs) const {
    if (rawRegs.isEmpty()) return QVariant();

    switch (dataType) {
        case ModbusDataType::Bit:
            return (rawRegs[0] & 1) != 0;

        case ModbusDataType::Int16: {
            qint16 v = static_cast<qint16>(rawRegs[0]);
            return (v * scale) + offset;
        }

        case ModbusDataType::Uint16: {
            quint16 v = rawRegs[0];
            return (v * scale) + offset;
        }

        case ModbusDataType::Int32BE: {
            if (rawRegs.size() < 2) return QVariant();
            qint32 v = (static_cast<qint32>(rawRegs[0]) << 16) | rawRegs[1];
            return (v * scale) + offset;
        }

        case ModbusDataType::Int32LE: {
            if (rawRegs.size() < 2) return QVariant();
            qint32 v = (static_cast<qint32>(rawRegs[1]) << 16) | rawRegs[0];
            return (v * scale) + offset;
        }

        case ModbusDataType::Uint32BE: {
            if (rawRegs.size() < 2) return QVariant();
            quint32 v = (static_cast<quint32>(rawRegs[0]) << 16) | rawRegs[1];
            return (v * scale) + offset;
        }

        case ModbusDataType::Uint32LE: {
            if (rawRegs.size() < 2) return QVariant();
            quint32 v = (static_cast<quint32>(rawRegs[1]) << 16) | rawRegs[0];
            return (v * scale) + offset;
        }

        case ModbusDataType::Float32BE: {
            if (rawRegs.size() < 2) return QVariant();
            quint32 bits = (static_cast<quint32>(rawRegs[0]) << 16) | rawRegs[1];
            float f = 0.0f;
            std::memcpy(&f, &bits, sizeof(float));
            return (static_cast<double>(f) * scale) + offset;
        }

        case ModbusDataType::Float32LE: {
            if (rawRegs.size() < 2) return QVariant();
            quint32 bits = (static_cast<quint32>(rawRegs[1]) << 16) | rawRegs[0];
            float f = 0.0f;
            std::memcpy(&f, &bits, sizeof(float));
            return (static_cast<double>(f) * scale) + offset;
        }
    }
    return QVariant();
}

QVector<quint16> ModbusConfig::encodeRegisters(const QVariant& val) const {
    QVector<quint16> out;
    double raw = (val.toDouble() - offset) / (scale != 0.0 ? scale : 1.0);

    switch (dataType) {
        case ModbusDataType::Bit:
            out.append(val.toBool() ? 1 : 0);
            break;

        case ModbusDataType::Int16:
            out.append(static_cast<quint16>(static_cast<qint16>(qRound(raw))));
            break;

        case ModbusDataType::Uint16:
            out.append(static_cast<quint16>(qBound(0.0, raw, 65535.0)));
            break;

        case ModbusDataType::Int32BE: {
            qint32 i32 = static_cast<qint32>(qRound64(raw));
            out.append(static_cast<quint16>((i32 >> 16) & 0xFFFF));
            out.append(static_cast<quint16>(i32 & 0xFFFF));
            break;
        }

        case ModbusDataType::Int32LE: {
            qint32 i32 = static_cast<qint32>(qRound64(raw));
            out.append(static_cast<quint16>(i32 & 0xFFFF));
            out.append(static_cast<quint16>((i32 >> 16) & 0xFFFF));
            break;
        }

        case ModbusDataType::Uint32BE: {
            quint32 u32 = static_cast<quint32>(qRound64(raw));
            out.append(static_cast<quint16>((u32 >> 16) & 0xFFFF));
            out.append(static_cast<quint16>(u32 & 0xFFFF));
            break;
        }

        case ModbusDataType::Uint32LE: {
            quint32 u32 = static_cast<quint32>(qRound64(raw));
            out.append(static_cast<quint16>(u32 & 0xFFFF));
            out.append(static_cast<quint16>((u32 >> 16) & 0xFFFF));
            break;
        }

        case ModbusDataType::Float32BE: {
            float f = static_cast<float>(raw);
            quint32 bits = 0;
            std::memcpy(&bits, &f, sizeof(float));
            out.append(static_cast<quint16>((bits >> 16) & 0xFFFF));
            out.append(static_cast<quint16>(bits & 0xFFFF));
            break;
        }

        case ModbusDataType::Float32LE: {
            float f = static_cast<float>(raw);
            quint32 bits = 0;
            std::memcpy(&bits, &f, sizeof(float));
            out.append(static_cast<quint16>(bits & 0xFFFF));
            out.append(static_cast<quint16>((bits >> 16) & 0xFFFF));
            break;
        }
    }
    return out;
}

bool ModbusConfig::operator==(const ModbusConfig& other) const {
    return mode == other.mode &&
           slaveId == other.slaveId &&
           registerType == other.registerType &&
           address == other.address &&
           dataType == other.dataType &&
           pollIntervalMs == other.pollIntervalMs &&
           qFuzzyCompare(scale, other.scale) &&
           qFuzzyCompare(offset, other.offset);
}

// ============================================================================
// Variable Config Implementation
// ============================================================================
QString variableScopeToString(VariableScope scope) {
    switch (scope) {
        case VariableScope::Project: return "project";
        case VariableScope::Screen:  return "screen";
        case VariableScope::Global:  return "global";
        default:                     return "project";
    }
}

VariableScope stringToVariableScope(const QString& str) {
    QString lower = str.trimmed().toLower();
    if (lower == "screen") return VariableScope::Screen;
    if (lower == "global") return VariableScope::Global;
    return VariableScope::Project;
}

QString simulationWaveformToString(SimulationWaveform wf) {
    switch (wf) {
        case SimulationWaveform::None:     return "none";
        case SimulationWaveform::Sine:     return "sine";
        case SimulationWaveform::Square:   return "square";
        case SimulationWaveform::Triangle: return "triangle";
        case SimulationWaveform::Sawtooth: return "sawtooth";
        case SimulationWaveform::Random:   return "random";
        case SimulationWaveform::Ramp:     return "ramp";
        default:                           return "none";
    }
}

SimulationWaveform stringToSimulationWaveform(const QString& str) {
    QString lower = str.trimmed().toLower();
    if (lower == "sine") return SimulationWaveform::Sine;
    if (lower == "square") return SimulationWaveform::Square;
    if (lower == "triangle") return SimulationWaveform::Triangle;
    if (lower == "sawtooth") return SimulationWaveform::Sawtooth;
    if (lower == "random") return SimulationWaveform::Random;
    if (lower == "ramp") return SimulationWaveform::Ramp;
    return SimulationWaveform::None;
}

QJsonObject VariableConfig::toJson() const {
    QJsonObject obj;
    obj["scope"] = variableScopeToString(scope);
    obj["isPersistent"] = isPersistent;
    obj["waveform"] = simulationWaveformToString(waveform);
    obj["minVal"] = minVal;
    obj["maxVal"] = maxVal;
    obj["periodMs"] = periodMs;
    obj["phaseOffset"] = phaseOffset;
    return obj;
}

VariableConfig VariableConfig::fromJson(const QJsonObject& obj) {
    VariableConfig cfg;
    cfg.scope = stringToVariableScope(obj.value("scope").toString());
    cfg.isPersistent = obj.value("isPersistent").toBool(cfg.isPersistent);
    cfg.waveform = stringToSimulationWaveform(obj.value("waveform").toString());
    cfg.minVal = obj.value("minVal").toDouble(cfg.minVal);
    cfg.maxVal = obj.value("maxVal").toDouble(cfg.maxVal);
    cfg.periodMs = obj.value("periodMs").toInt(cfg.periodMs);
    cfg.phaseOffset = obj.value("phaseOffset").toDouble(cfg.phaseOffset);
    return cfg;
}

double VariableConfig::evaluateWaveform(qint64 elapsedMs) const {
    if (periodMs <= 0) return minVal;
    double range = maxVal - minVal;
    double fraction = static_cast<double>(elapsedMs % periodMs) / periodMs;

    switch (waveform) {
        case SimulationWaveform::None:
            return minVal;

        case SimulationWaveform::Sine: {
            double angle = (2.0 * M_PI * fraction) + phaseOffset;
            double halfRange = range / 2.0;
            double mid = minVal + halfRange;
            return mid + (halfRange * std::sin(angle));
        }

        case SimulationWaveform::Square:
            return (fraction < 0.5) ? maxVal : minVal;

        case SimulationWaveform::Triangle:
            if (fraction < 0.5) {
                return minVal + (range * (fraction * 2.0));
            } else {
                return maxVal - (range * ((fraction - 0.5) * 2.0));
            }

        case SimulationWaveform::Sawtooth:
            return minVal + (range * fraction);

        case SimulationWaveform::Ramp: {
            double rampFraction = qBound(0.0, static_cast<double>(elapsedMs) / periodMs, 1.0);
            return minVal + (range * rampFraction);
        }

        case SimulationWaveform::Random: {
            quint64 seed = static_cast<quint64>(elapsedMs / periodMs) * 6364136223846793005ULL + 1ULL;
            double r = static_cast<double>(seed & 0xFFFF) / 65535.0;
            return minVal + (range * r);
        }
    }
    return minVal;
}

bool VariableConfig::operator==(const VariableConfig& other) const {
    return scope == other.scope &&
           isPersistent == other.isPersistent &&
           waveform == other.waveform &&
           periodMs == other.periodMs &&
           qFuzzyCompare(minVal, other.minVal) &&
           qFuzzyCompare(maxVal, other.maxVal) &&
           qFuzzyCompare(phaseOffset, other.phaseOffset);
}

// ============================================================================
// DataSource Constructor & Methods
// ============================================================================
DataSource::DataSource(const QString& id, const QString& name, DataSourceType type,
                       DataDirection dir, DataType dataType)
    : m_id(id)
    , m_name(name.isEmpty() ? id : name)
    , m_type(type)
    , m_direction(dir)
    , m_dataType(dataType)
{
}

QJsonObject DataSource::toJson() const {
    QJsonObject obj;
    obj["id"] = m_id;
    obj["name"] = m_name;
    obj["type"] = dataSourceTypeToString(m_type);
    obj["direction"] = dataDirectionToString(m_direction);
    obj["dataType"] = dataTypeToString(m_dataType);
    if (!m_hardwareRef.isEmpty()) {
        obj["hardwareRef"] = m_hardwareRef;
    }
    obj["available"] = m_available;
    if (m_value.isValid()) {
        obj["value"] = QJsonValue::fromVariant(m_value);
    }
    if (!m_metadata.isEmpty()) {
        obj["metadata"] = m_metadata;
    }

    if (m_type == DataSourceType::Can) {
        obj["canConfig"] = m_canConfig.toJson();
    } else if (m_type == DataSourceType::Uart) {
        obj["uartConfig"] = m_uartConfig.toJson();
    } else if (m_type == DataSourceType::Modbus) {
        obj["modbusConfig"] = m_modbusConfig.toJson();
    } else if (m_type == DataSourceType::Variable) {
        obj["variableConfig"] = m_variableConfig.toJson();
    }

    return obj;
}

DataSource DataSource::fromJson(const QJsonObject& json) {
    DataSource ds;
    ds.m_id = json.value("id").toString();
    ds.m_name = json.value("name").toString(ds.m_id);
    ds.m_type = stringToDataSourceType(json.value("type").toString("VARIABLE"));
    ds.m_direction = stringToDataDirection(json.value("direction").toString("input"));
    ds.m_dataType = stringToDataType(json.value("dataType").toString("float"));
    ds.m_hardwareRef = json.value("hardwareRef").toString();
    ds.m_available = json.value("available").toBool(true);
    if (json.contains("value")) {
        ds.m_value = json.value("value").toVariant();
    }
    if (json.contains("metadata") && json.value("metadata").isObject()) {
        ds.m_metadata = json.value("metadata").toObject();
    }

    if (json.contains("canConfig") && json.value("canConfig").isObject()) {
        ds.m_canConfig = CanSignalConfig::fromJson(json.value("canConfig").toObject());
    }
    if (json.contains("uartConfig") && json.value("uartConfig").isObject()) {
        ds.m_uartConfig = UartStreamConfig::fromJson(json.value("uartConfig").toObject());
    }
    if (json.contains("modbusConfig") && json.value("modbusConfig").isObject()) {
        ds.m_modbusConfig = ModbusConfig::fromJson(json.value("modbusConfig").toObject());
    }
    if (json.contains("variableConfig") && json.value("variableConfig").isObject()) {
        ds.m_variableConfig = VariableConfig::fromJson(json.value("variableConfig").toObject());
    }

    return ds;
}

bool DataSource::operator==(const DataSource& other) const {
    return m_id == other.m_id &&
           m_name == other.m_name &&
           m_type == other.m_type &&
           m_direction == other.m_direction &&
           m_dataType == other.m_dataType &&
           m_hardwareRef == other.m_hardwareRef &&
           m_available == other.m_available &&
           m_metadata == other.m_metadata &&
           m_canConfig == other.m_canConfig &&
           m_uartConfig == other.m_uartConfig &&
           m_modbusConfig == other.m_modbusConfig &&
           m_variableConfig == other.m_variableConfig;
}
