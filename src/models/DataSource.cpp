#include "DataSource.h"

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
           m_metadata == other.m_metadata;
}
