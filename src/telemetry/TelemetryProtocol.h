#pragma once

#include <QString>
#include <QByteArray>
#include <QList>
#include <QVariant>
#include <cstdint>

enum class TelemetryDataType {
    Int,
    Float,
    Bool,
    String,
    Hex,
    Unknown
};

inline QString telemetryDataTypeToString(TelemetryDataType type) {
    switch (type) {
        case TelemetryDataType::Int:    return "INT";
        case TelemetryDataType::Float:  return "FLOAT";
        case TelemetryDataType::Bool:   return "BOOL";
        case TelemetryDataType::String: return "STR";
        case TelemetryDataType::Hex:    return "HEX";
        default:                        return "UNKNOWN";
    }
}

inline TelemetryDataType stringToTelemetryDataType(const QString& str) {
    QString s = str.trimmed().toUpper();
    if (s == "INT") return TelemetryDataType::Int;
    if (s == "FLOAT") return TelemetryDataType::Float;
    if (s == "BOOL") return TelemetryDataType::Bool;
    if (s == "STR" || s == "STRING") return TelemetryDataType::String;
    if (s == "HEX") return TelemetryDataType::Hex;
    return TelemetryDataType::Unknown;
}

struct TelemetryFrame {
    bool isValid = false;
    QString version;          // "1.0"
    QString sourceId;         // e.g. "PA0_ADC", "TEMP_SENSOR", "CAN_SPEED"
    TelemetryDataType type = TelemetryDataType::Unknown;
    QVariant value;
    uint32_t sequence = 0;
    uint8_t receivedChecksum = 0;
    uint8_t calculatedChecksum = 0;
    QString errorMessage;
    QString rawFrame;
};

class TelemetryProtocol {
public:
    static constexpr size_t MAX_FRAME_LENGTH = 512;
    static constexpr const char* CURRENT_VERSION = "1.0";

    TelemetryProtocol();

    /// Calculates XOR checksum across the payload bytes (excluding leading '$' and trailing '*XX\r\n')
    static uint8_t calculateChecksum(const QByteArray& payload);

    /// Formats a valid telemetry wire frame: $TLM,1.0,<source_id>,<data_type>,<value>,<seq>*<checksum>\r\n
    static QByteArray formatFrame(const QString& sourceId, TelemetryDataType type, const QVariant& value, uint32_t seq);

    /// Parses a single complete ASCII frame
    static TelemetryFrame parseFrame(const QString& frameString);

    /// Bounded streaming parser: feeds newly received serial byte chunks and returns all parsed frames
    QList<TelemetryFrame> feedBytes(const QByteArray& data);

    /// Clears internal stream buffer
    void reset();

private:
    QByteArray m_streamBuffer;
};
