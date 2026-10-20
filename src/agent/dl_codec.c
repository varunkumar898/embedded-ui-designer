#include "dl_codec.h"

uint16_t dl_crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else crc <<= 1;
        }
    }
    return crc;
}

size_t dl_encode(const dl_frame_t* frame, uint8_t* buf, size_t buf_size) {
    if (!frame || !buf) return 0;
    if (frame->length > DL_MAX_PAYLOAD) return 0;
    size_t total = 2 + 1 + 2 + 2 + 1 + frame->length + 2;
    if (buf_size < total) return 0;
    buf[0] = DL_MAGIC0;
    buf[1] = DL_MAGIC1;
    buf[2] = frame->version;
    buf[3] = (uint8_t)((frame->length >> 8) & 0xFF);
    buf[4] = (uint8_t)(frame->length & 0xFF);
    buf[5] = (uint8_t)((frame->req_id >> 8) & 0xFF);
    buf[6] = (uint8_t)(frame->req_id & 0xFF);
    buf[7] = frame->type;
    for (uint16_t i = 0; i < frame->length; i++) buf[8 + i] = frame->payload[i];
    uint16_t crc = dl_crc16(buf, 8 + frame->length);
    buf[8 + frame->length] = (uint8_t)((crc >> 8) & 0xFF);
    buf[9 + frame->length] = (uint8_t)(crc & 0xFF);
    return total;
}

int dl_decode(const uint8_t* buf, size_t len, dl_frame_t* out) {
    if (!buf || !out) return -1;
    if (len < 10) return -1;
    if (buf[0] != DL_MAGIC0 || buf[1] != DL_MAGIC1) return -1;
    uint8_t ver = buf[2];
    uint16_t length = ((uint16_t)buf[3] << 8) | buf[4];
    if (length > DL_MAX_PAYLOAD) return -1;
    if (len < (size_t)(8 + length + 2)) return -1;
    uint16_t crc_calc = dl_crc16(buf, 8 + length);
    uint16_t crc_wire = ((uint16_t)buf[8 + length] << 8) | buf[9 + length];
    if (crc_calc != crc_wire) return -1;
    out->version = ver;
    out->length = length;
    out->req_id = ((uint16_t)buf[5] << 8) | buf[6];
    out->type = buf[7];
    for (uint16_t i = 0; i < length; i++) out->payload[i] = buf[8 + i];
    return 0;
}

void dl_parser_init(dl_parser_t* p) {
    if (!p) return;
    p->state = 0;
    p->pos = 0;
    p->expected = 0;
}

// state machine: 0 scanning for magic, 1 collecting header+payload, 2 collecting crc
int dl_parser_feed(dl_parser_t* p, uint8_t byte, dl_frame_t* out) {
    if (!p) return 0;
    if (p->state == 0) {
        if (p->pos == 0) {
            if (byte == DL_MAGIC0) { p->buf[p->pos++] = byte; }
        } else {
            if (byte == DL_MAGIC1) {
                p->buf[p->pos++] = byte;
                p->state = 1;
                p->expected = 0; // will compute after header
            } else {
                // resync
                p->pos = 0;
                if (byte == DL_MAGIC0) p->buf[p->pos++] = byte;
            }
        }
        return 0;
    }
    // state 1: collect until we have 8 header bytes + payload
    if (p->pos < DL_MAX_FRAME) p->buf[p->pos++] = byte;
    if (p->pos == 8) {
        uint16_t len = ((uint16_t)p->buf[3] << 8) | p->buf[4];
        if (len > DL_MAX_PAYLOAD) {
            // resync
            dl_parser_init(p);
            return 0;
        }
        p->expected = 8 + len + 2;
    }
    if (p->expected > 0 && p->pos >= p->expected) {
        // try decode
        if (dl_decode(p->buf, p->pos, out) == 0) {
            dl_parser_init(p);
            return 1;
        }
        // bad crc / type: drop first byte and resync
        // shift buffer left by one
        for (size_t i = 1; i < p->pos; i++) p->buf[i-1] = p->buf[i];
        p->pos--;
        p->state = 0;
        p->expected = 0;
        if (p->pos > 0 && p->buf[0] == DL_MAGIC0) {
            // keep scanning
        } else {
            p->pos = 0;
        }
    }
    return 0;
}