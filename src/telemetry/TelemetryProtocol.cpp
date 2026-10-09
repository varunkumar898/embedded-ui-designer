#include "TelemetryProtocol.h"
#include <QStringList>
#include <QRegularExpression>

TelemetryProtocol::TelemetryProtocol() {
}

uint8_t TelemetryProtocol::calculateChecksum(const QByteArray& payload) {
    uint8_t csum = 0;
    for (char c : payload) {
        csum ^= static_cast<uint8_t>(c);
    }
    return csum;
}

QByteArray TelemetryProtocol::formatFrame(const QString& sourceId, TelemetryDataType type, const QVariant& value, uint32_t seq) {
    QString valStr;
    switch (type) {
        case TelemetryDataType::Float:
            valStr = QString::number(value.toDouble(), 'f', 2);
            break;
        case TelemetryDataType::Bool:
            valStr = value.toBool() ? "1" : "0";
            break;
        case TelemetryDataType::Hex:
            valStr = QString("0x%1").arg(value.toUInt(), 0, 16).toUpper();
            break;
        default:
            valStr = value.toString();
            break;
    }

    // Payload between '$' and '*'
    QByteArray payload = QString("TLM,%1,%2,%3,%4,%5")
        .arg(CURRENT_VERSION)
        .arg(sourceId)
        .arg(telemetryDataTypeToString(type))
        .arg(valStr)
        .arg(seq)
        .toUtf8();

    uint8_t csum = calculateChecksum(payload);
    QString hexCsum = QString("%1").arg(csum, 2, 16, QChar('0')).toUpper();

    return "$" + payload + "*" + hexCsum.toUtf8() + "\r\n";
}

TelemetryFrame TelemetryProtocol::parseFrame(const QString& frameString) {
    TelemetryFrame frame;
    frame.rawFrame = frameString.trimmed();
    frame.isValid = false;

    QString trimmed = frameString.trimmed();
    if (!trimmed.startsWith('$')) {
        frame.errorMessage = "Missing start delimiter '$'";
        return frame;
    }

    int starIdx = trimmed.lastIndexOf('*');
    if (starIdx == -1) {
        frame.errorMessage = "Missing checksum delimiter '*'";
        return frame;
    }

    // Extract payload and checksum string
    QString payloadStr = trimmed.mid(1, starIdx - 1);
    QString checksumStr = trimmed.mid(starIdx + 1);

    if (checksumStr.length() != 2) {
        frame.errorMessage = "Checksum must be exactly 2 hex characters";
        return frame;
    }

    bool ok = false;
    uint8_t expectedChecksum = static_cast<uint8_t>(checksumStr.toUInt(&ok, 16));
    if (!ok) {
        frame.errorMessage = "Invalid hex format in checksum";
        return frame;
    }

    uint8_t actualChecksum = calculateChecksum(payloadStr.toUtf8());
    frame.receivedChecksum = expectedChecksum;
    frame.calculatedChecksum = actualChecksum;

    if (actualChecksum != expectedChecksum) {
        frame.errorMessage = QString("Checksum mismatch: received 0x%1, calculated 0x%2")
            .arg(checksumStr.toUpper())
            .arg(QString("%1").arg(actualChecksum, 2, 16, QChar('0')).toUpper());
        return frame;
    }

    // Split payload: TLM,version,source_id,data_type,value,seq
    QStringList fields = payloadStr.split(',');
    if (fields.size() < 6) {
        frame.errorMessage = QString("Insufficient fields in frame: expected 6, got %1").arg(fields.size());
        return frame;
    }

    if (fields[0] != "TLM") {
        frame.errorMessage = QString("Unsupported message header: %1").arg(fields[0]);
        return frame;
    }

    frame.version = fields[1];
    if (frame.version != CURRENT_VERSION) {
        frame.errorMessage = QString("Unsupported protocol version: %1").arg(frame.version);
        return frame;
    }

    frame.sourceId = fields[2];
    frame.type = stringToTelemetryDataType(fields[3]);
    if (frame.type == TelemetryDataType::Unknown) {
        frame.errorMessage = QString("Unknown data type: %1").arg(fields[3]);
        return frame;
    }

    QString rawVal = fields[4];
    switch (frame.type) {
        case TelemetryDataType::Int:
            frame.value = rawVal.toInt();
            break;
        case TelemetryDataType::Float:
            frame.value = rawVal.toDouble();
            break;
        case TelemetryDataType::Bool:
            frame.value = (rawVal == "1" || rawVal.compare("true", Qt::CaseInsensitive) == 0);
            break;
        case TelemetryDataType::Hex: {
            uint val = rawVal.startsWith("0x", Qt::CaseInsensitive) ? rawVal.mid(2).toUInt(nullptr, 16) : rawVal.toUInt(nullptr, 16);
            frame.value = val;
            break;
        }
        case TelemetryDataType::String:
        default:
            frame.value = rawVal;
            break;
    }

    frame.sequence = fields[5].toUInt();
    frame.isValid = true;
    return frame;
}

QList<TelemetryFrame> TelemetryProtocol::feedBytes(const QByteArray& data) {
    QList<TelemetryFrame> frames;
    m_streamBuffer.append(data);

    // Guard against oversized buffers
    if (m_streamBuffer.size() > static_cast<int>(MAX_FRAME_LENGTH * 8)) {
        m_streamBuffer.remove(0, m_streamBuffer.size() - static_cast<int>(MAX_FRAME_LENGTH * 2));
    }

    while (!m_streamBuffer.isEmpty()) {
        int startIdx = m_streamBuffer.indexOf('$');
        if (startIdx == -1) {
            // No start of frame found, discard garbage
            m_streamBuffer.clear();
            break;
        }

        if (startIdx > 0) {
            // Discard bytes preceding '$'
            m_streamBuffer.remove(0, startIdx);
        }

        int newlineIdx = m_streamBuffer.indexOf('\n');
        if (newlineIdx == -1) {
            // Partial frame, await more data unless oversized
            if (m_streamBuffer.size() > static_cast<int>(MAX_FRAME_LENGTH)) {
                // Malformed oversized frame, skip first byte and retry
                m_streamBuffer.remove(0, 1);
            }
            break;
        }

        // Complete candidate frame
        QByteArray frameBytes = m_streamBuffer.left(newlineIdx + 1);
        m_streamBuffer.remove(0, newlineIdx + 1);

        QString frameStr = QString::fromUtf8(frameBytes).trimmed();
        if (!frameStr.isEmpty()) {
            frames.append(parseFrame(frameStr));
        }
    }

    return frames;
}

void TelemetryProtocol::reset() {
    m_streamBuffer.clear();
}
