#ifndef DL_CODEC_H
#define DL_CODEC_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DL_MAGIC0 0xD1
#define DL_MAGIC1 0x4B
#define DL_MAX_PAYLOAD 256
#define DL_MAX_FRAME (2 + 1 + 2 + 2 + 1 + DL_MAX_PAYLOAD + 2)

typedef struct {
    uint8_t version;
    uint16_t length;
    uint16_t req_id;
    uint8_t type;
    uint8_t payload[DL_MAX_PAYLOAD];
} dl_frame_t;

// CRC-CCITT-FALSE
uint16_t dl_crc16(const uint8_t* data, size_t len);

// Encode a frame into buf. Returns total bytes written, or 0 on error.
size_t dl_encode(const dl_frame_t* frame, uint8_t* buf, size_t buf_size);

// Decode a complete frame from buf. Returns 0 on success.
int dl_decode(const uint8_t* buf, size_t len, dl_frame_t* out);

// Streaming parser. Feed bytes one at a time; returns 1 when a complete
// valid frame is available in *out.
typedef struct {
    uint8_t state;
    uint8_t buf[DL_MAX_FRAME];
    size_t pos;
    size_t expected;
} dl_parser_t;

void dl_parser_init(dl_parser_t* p);
int dl_parser_feed(dl_parser_t* p, uint8_t byte, dl_frame_t* out);

#ifdef __cplusplus
}
#endif

#endif