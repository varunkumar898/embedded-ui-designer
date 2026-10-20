#pragma once

#include <QObject>
#include <QByteArray>
#include "dl_codec.h"

// In-memory mock agent that speaks the Device Link protocol over a
// QIODevice-like byte buffer. Used by host-side tests.
class MockAgent : public QObject {
    Q_OBJECT
public:
    explicit MockAgent(QObject* parent = nullptr);

    // Feed raw bytes from the host; returns response bytes to send back.
    QByteArray handleBytes(const QByteArray& input);

    void setChipId(const QString& id) { m_chipId = id; }
    void setSignalTableHash(quint32 h) { m_signalTableHash = h; }
    void setProtocolVersion(uint8_t v) { m_protoVersion = v; }

private:
    QString m_chipId = "Simulated device";
    quint32 m_signalTableHash = 0;
    uint8_t m_protoVersion = 1;
};