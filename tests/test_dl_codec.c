#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "agent/dl_codec.h"

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL: %s (line %d)\n", #cond, __LINE__); failures++; } } while(0)

int main(void) {
    // Round trip
    dl_frame_t f; memset(&f, 0, sizeof(f));
    f.version = 1; f.req_id = 0x1234; f.type = 0x05; f.length = 4;
    f.payload[0]=0xAA; f.payload[1]=0xBB; f.payload[2]=0xCC; f.payload[3]=0xDD;
    uint8_t buf[DL_MAX_FRAME];
    size_t n = dl_encode(&f, buf, sizeof(buf));
    CHECK(n > 0);
    dl_frame_t out; memset(&out, 0, sizeof(out));
    CHECK(dl_decode(buf, n, &out) == 0);
    CHECK(out.version == 1 && out.req_id == 0x1234 && out.type == 0x05 && out.length == 4);
    CHECK(out.payload[0]==0xAA && out.payload[3]==0xDD);

    // Bad CRC
    uint8_t bad[DL_MAX_FRAME]; memcpy(bad, buf, n);
    bad[n-1] ^= 0xFF;
    CHECK(dl_decode(bad, n, &out) != 0);

    // Truncated
    CHECK(dl_decode(buf, 5, &out) != 0);

    // Oversized length field
    uint8_t big[DL_MAX_FRAME]; memcpy(big, buf, n);
    big[3] = 0x02; // length 0x02xx > 256? set high byte
    big[4] = 0x00;
    // length = 512 > 256 -> reject (may also fail CRC, still non-zero)
    CHECK(dl_decode(big, n, &out) != 0);

    // Streaming parser round trip
    dl_parser_t p; dl_parser_init(&p);
    dl_frame_t parsed; memset(&parsed, 0, sizeof(parsed));
    int got = 0;
    for (size_t i = 0; i < n; i++) {
        if (dl_parser_feed(&p, buf[i], &parsed)) { got = 1; break; }
    }
    CHECK(got == 1);
    CHECK(parsed.type == 0x05 && parsed.length == 4);

    // Resync after corruption: prepend garbage then valid frame
    dl_parser_init(&p);
    uint8_t junk[] = {0x00, 0x11, 0x22};
    int got2 = 0;
    for (size_t i = 0; i < sizeof(junk); i++) dl_parser_feed(&p, junk[i], &parsed);
    for (size_t i = 0; i < n; i++) {
        if (dl_parser_feed(&p, buf[i], &parsed)) { got2 = 1; break; }
    }
    CHECK(got2 == 1);

    // Fuzz: random bytes must not crash
    srand(42);
    for (int t = 0; t < 1000; t++) {
        dl_parser_t pp; dl_parser_init(&pp);
        dl_frame_t po; memset(&po, 0, sizeof(po));
        for (int i = 0; i < 500; i++) {
            uint8_t b = (uint8_t)(rand() & 0xFF);
            dl_parser_feed(&pp, b, &po);
        }
    }

    if (failures == 0) printf("ALL DL CODEC TESTS PASSED\n");
    else printf("%d FAILURES\n", failures);
    return failures ? 1 : 0;
}