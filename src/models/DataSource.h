#pragma once

#include <QString>
#include <QJsonObject>
#include <QVariant>
#include <QByteArray>
#include <QVector>

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
    Calculated,
    Modbus
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

// ============================================================================
// CAN Bus Signal Configuration
// ============================================================================
struct CanSignalConfig {
    QString busId = "CAN1";
    quint32 messageId = 0x100;
    bool isExtended = false;  // false = 11-bit standard ID, true = 29-bit extended ID
    bool isFd = false;        // false = Classic CAN (8 bytes), true = CAN-FD (up to 64 bytes)
    int bitrate = 500000;     // 500 kbps
    int startBit = 0;         // 0..63 (or 0..511 for CAN-FD)
    int bitLength = 8;        // 1..64
    bool isBigEndian = false; // false = Intel (Little Endian), true = Motorola (Big Endian)
    bool isSigned = false;
    double factor = 1.0;
    double offset = 0.0;
    double minVal = 0.0;
    double maxVal = 100.0;
    QString unit;

    QJsonObject toJson() const;
    static CanSignalConfig fromJson(const QJsonObject& obj);

    double decodePayload(const QByteArray& payload) const;
    void encodePayload(double value, QByteArray& payload) const;
    bool operator==(const CanSignalConfig& other) const;
};

// ============================================================================
// UART Stream Parsing Configuration
// ============================================================================
enum class UartParseMode {
    Raw,
    DelimiterIndex, // Token at index in delimited frame (e.g., CSV)
    KeyValue,       // "KEY=VALUE" or "KEY:VALUE" token matching
    RegexCapture,   // Regular expression group 1 capture
    JsonPath        // Key in JSON object frame
};

QString uartParseModeToString(UartParseMode mode);
UartParseMode stringToUartParseMode(const QString& str);

struct UartStreamConfig {
    QString portName = "UART1";
    int baudRate = 115200;
    int dataBits = 8;
    int stopBits = 1;
    QString parity = "None"; // None, Even, Odd
    UartParseMode parseMode = UartParseMode::DelimiterIndex;
    QString delimiter = ",";
    int tokenIndex = 0;
    QString keyName;
    QString regexPattern;
    double factor = 1.0;
    double offset = 0.0;

    QJsonObject toJson() const;
    static UartStreamConfig fromJson(const QJsonObject& obj);

    QVariant parseIncomingText(const QString& frame) const;
    bool operator==(const UartStreamConfig& other) const;
};

// ============================================================================
// Modbus Configuration (RTU / TCP / ASCII)
// ============================================================================
enum class ModbusMode {
    Rtu,
    Tcp,
    Ascii
};

enum class ModbusRegisterType {
    Coil,            // FC 01 / 05 / 15 (1-bit discrete output)
    DiscreteInput,   // FC 02 (1-bit discrete input)
    HoldingRegister, // FC 03 / 06 / 16 (16-bit holding register)
    InputRegister    // FC 04 (16-bit input register)
};

enum class ModbusDataType {
    Bit,
    Int16,
    Uint16,
    Int32BE,
    Int32LE,
    Uint32BE,
    Uint32LE,
    Float32BE,
    Float32LE
};

QString modbusModeToString(ModbusMode mode);
ModbusMode stringToModbusMode(const QString& str);

QString modbusRegisterTypeToString(ModbusRegisterType type);
ModbusRegisterType stringToModbusRegisterType(const QString& str);

QString modbusDataTypeToString(ModbusDataType type);
ModbusDataType stringToModbusDataType(const QString& str);

struct ModbusConfig {
    ModbusMode mode = ModbusMode::Rtu;
    int slaveId = 1; // 1..247
    ModbusRegisterType registerType = ModbusRegisterType::HoldingRegister;
    int address = 0; // 0-based register address
    ModbusDataType dataType = ModbusDataType::Uint16;
    int pollIntervalMs = 100;
    double scale = 1.0;
    double offset = 0.0;

    QJsonObject toJson() const;
    static ModbusConfig fromJson(const QJsonObject& obj);

    QVariant decodeRegisters(const QVector<quint16>& rawRegs) const;
    QVector<quint16> encodeRegisters(const QVariant& val) const;
    bool operator==(const ModbusConfig& other) const;
};

// ============================================================================
// Internal Project Variable Configuration
// ============================================================================
enum class VariableScope {
    Project,
    Screen,
    Global
};

enum class SimulationWaveform {
    None,
    Sine,
    Square,
    Triangle,
    Sawtooth,
    Random,
    Ramp
};

QString variableScopeToString(VariableScope scope);
VariableScope stringToVariableScope(const QString& str);

QString simulationWaveformToString(SimulationWaveform wf);
SimulationWaveform stringToSimulationWaveform(const QString& str);

struct VariableConfig {
    VariableScope scope = VariableScope::Project;
    bool isPersistent = false; // Non-volatile flash retention
    SimulationWaveform waveform = SimulationWaveform::None;
    double minVal = 0.0;
    double maxVal = 100.0;
    int periodMs = 1000;
    double phaseOffset = 0.0;

    QJsonObject toJson() const;
    static VariableConfig fromJson(const QJsonObject& obj);

    double evaluateWaveform(qint64 elapsedMs) const;
    bool operator==(const VariableConfig& other) const;
};

// ============================================================================
// Unified DataSource Class
// ============================================================================
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

    // Protocol Specific Configurations
    CanSignalConfig canConfig() const { return m_canConfig; }
    void setCanConfig(const CanSignalConfig& cfg) { m_canConfig = cfg; }

    UartStreamConfig uartConfig() const { return m_uartConfig; }
    void setUartConfig(const UartStreamConfig& cfg) { m_uartConfig = cfg; }

    ModbusConfig modbusConfig() const { return m_modbusConfig; }
    void setModbusConfig(const ModbusConfig& cfg) { m_modbusConfig = cfg; }

    VariableConfig variableConfig() const { return m_variableConfig; }
    void setVariableConfig(const VariableConfig& cfg) { m_variableConfig = cfg; }

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

    CanSignalConfig m_canConfig;
    UartStreamConfig m_uartConfig;
    ModbusConfig m_modbusConfig;
    VariableConfig m_variableConfig;
};
