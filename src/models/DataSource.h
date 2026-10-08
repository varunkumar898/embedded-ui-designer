#pragma once

#include <QString>
#include <QJsonObject>
#include <QVariant>

enum class DataSourceType {
    Gpio,
    Adc,
    Pwm,
    Uart,
    I2c,
    Spi,
    Can,
    Sensor,
    Variable,
    Constant,
    Timer,
    Calculated
};

enum class DataDirection {
    Input,
    Output,
    Bidirectional
};

enum class DataType {
    Boolean,
    Integer,
    UnsignedInteger,
    Float,
    String,
    Enum,
    Raw
};

QString dataSourceTypeToString(DataSourceType type);
DataSourceType stringToDataSourceType(const QString& str);

QString dataDirectionToString(DataDirection dir);
DataDirection stringToDataDirection(const QString& str);

QString dataTypeToString(DataType type);
DataType stringToDataType(const QString& str);

class DataSource {
public:
    DataSource() = default;
    DataSource(const QString& id, const QString& name, DataSourceType type,
               DataDirection dir = DataDirection::Input, DataType dataType = DataType::Float);

    QString id() const { return m_id; }
    void setId(const QString& id) { m_id = id; }

    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    DataSourceType type() const { return m_type; }
    void setType(DataSourceType t) { m_type = t; }

    DataDirection direction() const { return m_direction; }
    void setDirection(DataDirection dir) { m_direction = dir; }

    DataType dataType() const { return m_dataType; }
    void setDataType(DataType t) { m_dataType = t; }

    QVariant value() const { return m_value; }
    void setValue(const QVariant& val) { m_value = val; }

    QString hardwareRef() const { return m_hardwareRef; }
    void setHardwareRef(const QString& ref) { m_hardwareRef = ref; }

    bool isAvailable() const { return m_available; }
    void setAvailable(bool avail) { m_available = avail; }

    QJsonObject metadata() const { return m_metadata; }
    void setMetadata(const QJsonObject& meta) { m_metadata = meta; }

    QJsonObject toJson() const;
    static DataSource fromJson(const QJsonObject& json);

    bool operator==(const DataSource& other) const;

private:
    QString m_id;
    QString m_name;
    DataSourceType m_type = DataSourceType::Variable;
    DataDirection m_direction = DataDirection::Input;
    DataType m_dataType = DataType::Float;
    QVariant m_value;
    QString m_hardwareRef;
    bool m_available = true;
    QJsonObject m_metadata;
};
