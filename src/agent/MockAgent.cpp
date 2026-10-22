#include "MockAgent.h"
#include <cstring>

MockAgent::MockAgent(QObject* parent) : QObject(parent) {}

QByteArray MockAgent::handleBytes(const QByteArray& input) {
    QByteArray out;
    dl_parser_t p; dl_parser_init(&p);
    dl_frame_t frame; memset(&frame, 0, sizeof(frame));
    for (int i = 0; i < input.size(); i++) {
        if (dl_parser_feed(&p, (uint8_t)input[i], &frame)) {
            // Respond to PING (0x13) with PING; to HELLO (0x01) with IDENTIFY (0x02).
            uint8_t resp[DL_MAX_FRAME];
            dl_frame_t r; memset(&r, 0, sizeof(r));
            r.version = m_protoVersion;
            r.req_id = frame.req_id;
            if (frame.type == 0x01) { // HELLO
                r.type = 0x02; // IDENTIFY
                // chip id into payload (first 16 bytes)
                QByteArray chip = m_chipId.toUtf8().left(16);
                while (chip.size() < 16) chip.append('\0');
                memcpy(r.payload, chip.constData(), 16);
                r.payload[16+4+0] = (uint8_t)(m_protoVersion);
                // signal table hash at offset 16+4+1 = 21? keep simple: bytes 20..23
                r.payload[20] = (uint8_t)((m_signalTableHash >> 24) & 0xFF);
                r.payload[21] = (uint8_t)((m_signalTableHash >> 16) & 0xFF);
                r.payload[22] = (uint8_t)((m_signalTableHash >> 8) & 0xFF);
                r.payload[23] = (uint8_t)(m_signalTableHash & 0xFF);
                r.length = 28;
            } else if (frame.type == 0x13) { // PING
                r.type = 0x13;
                r.length = 0;
            } else {
                r.type = 0x14; // ERROR
                r.payload[0] = 0x06; // BAD_FRAME
                r.payload[1] = (uint8_t)((frame.req_id >> 8) & 0xFF);
                r.payload[2] = (uint8_t)(frame.req_id & 0xFF);
                r.length = 3;
            }
            size_t rn = dl_encode(&r, resp, sizeof(resp));
            if (rn > 0) out.append((const char*)resp, (int)rn);
        }
    }
    return out;
}