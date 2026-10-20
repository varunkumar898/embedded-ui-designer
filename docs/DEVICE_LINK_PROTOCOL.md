# Device Link Protocol

Version: 1.0 (draft). Transport-independent framed binary protocol used between
the host (Embedded UI Designer) and an agent firmware running on a target
device. Only signals present in the device's signal table are accessible;
anything else is rejected.

## Frame format

```
+--------+--------+--------+--------+--------+--------+---------+--------+
| MAGIC  | VER    | LEN    | REQ_ID | TYPE   | PAYLOAD | CRC16   | (end) |
| 2 bytes| 1 byte | 2 bytes| 2 bytes| 1 byte | 0..N    | 2 bytes |        |
+--------+--------+--------+--------+--------+---------+--------+--------+
```

- MAGIC = 0xD1 0x4B (little-endian on wire).
- VER = protocol version (1).
- LEN = payload length in bytes, big-endian, max 256.
- REQ_ID = request id (big-endian); responses echo it; events use 0.
- TYPE = message type (see table).
- PAYLOAD = type-specific bytes.
- CRC16 = CRC-CCITT-FALSE (poly 0x1021, init 0xFFFF) over MAGIC..PAYLOAD.

## Size limits

- Max payload: 256 bytes. Frames exceeding this are rejected (ERROR_TOO_LARGE).
- Max frame on wire: 4 (header) + 2 (req id) ... actually 2+1+2+2+1 = 8 header bytes + 256 + 2 = 266 bytes.
- No unbounded allocation: the streaming parser uses a fixed 266-byte buffer.

## Message types

| Code | Name | Direction | Payload |
|---|---|---|---|
| 0x01 | HELLO | host->agent | (none) |
| 0x02 | IDENTIFY (response) | agent->host | chip family (16B), chip id (16B), fw version (4B), proto version (1B), signal-table hash (4B), capability flags (4B) |
| 0x03 | SIGNAL_READ (req) | host->agent | signal id (2B) |
| 0x04 | SIGNAL_READ (resp) | agent->host | signal id (2B), value (4B) |
| 0x05 | SIGNAL_WRITE (req) | host->agent | signal id (2B), value (4B) |
| 0x06 | SIGNAL_WRITE (resp) | agent->host | signal id (2B), status (1B) |
| 0x07 | ADC_READ (req) | host->agent | signal id (2B) |
| 0x08 | ADC_READ (resp) | agent->host | signal id (2B), raw (4B), normalized (4B float) |
| 0x09 | PWM_SET (req) | host->agent | signal id (2B), duty (4B float) |
| 0x0A | PWM_SET (resp) | agent->host | signal id (2B), status (1B) |
| 0x0B | I2C_XFER (req) | host->agent | bus id (1B), addr (1B), flags (1B), len (1B), data (len) |
| 0x0C | I2C_XFER (resp) | agent->host | status (1B), len (1B), data (len) |
| 0x0D | SPI_XFER (req) | host->agent | bus id (1B), flags (1B), len (1B), data (len) |
| 0x0E | SPI_XFER (resp) | agent->host | status (1B), len (1B), data (len) |
| 0x0F | CAN_SEND (req) | host->agent | id (4B), dlc (1B), data (0..8) |
| 0x10 | CAN_RECV (event) | agent->host | id (4B), dlc (1B), data (0..8) |
| 0x11 | SUBSCRIBE (req) | host->agent | signal id (2B), period ms (2B) |
| 0x12 | EVENT (async) | agent->host | signal id (2B), value (4B) |
| 0x13 | PING (req/resp) | both | (none) |
| 0x14 | ERROR (resp) | agent->host | code (1B), req_id (2B) |

## Error codes

0x00 OK, 0x01 UNKNOWN_SIGNAL, 0x02 BAD_VALUE, 0x03 BUS_ERROR, 0x04 TIMEOUT,
0x05 UNSUPPORTED, 0x06 BAD_FRAME, 0x07 TOO_LARGE, 0x08 PROTOCOL_MISMATCH,
0x09 IDENTITY_MISMATCH, 0x0A NOT_CONNECTED.

## Resync after corruption

The parser scans for MAGIC. On a bad CRC or unknown type, it discards one
byte and re-scans. Buffers are bounded (266 bytes); no unbounded allocation.

## Backward compatibility

- A device reports its protocol version in IDENTIFY. The host refuses to
  talk if the major version differs (ERROR protocol mismatch).
- New message types may be added; unknown types are ignored (ERROR 0x06
  BAD_FRAME) without desync.
- Payload fields are fixed-width; no variable-length strings.

## Pairing / auth

- For TCP/BLE transports, pairing is required before any command is accepted.
- Nothing is exposed on the network by default. The serial transport (UART)
  has no auth (physical access assumed).
- The agent only exposes signals present in its signal table; any other
  signal id returns UNKNOWN_SIGNAL.
## Larger transfers (chunking)

I2C and SPI payloads may exceed the 256-byte frame limit. The host splits
them into chunks of at most 256 bytes; each chunk is a separate I2C_XFER /
SPI_XFER request with a `flags` bit (`0x01` = first chunk, `0x02` = last
chunk, `0x04` = more data follows). The agent reassembles chunks by
request id and acknowledges each. A chunk with `len == 0` is invalid and
returns ERROR_BAD_FRAME. Total transfer size is bounded by the agent's
buffer (max 4096 bytes); larger transfers return ERROR_TOO_LARGE. CAN frames
are fixed at up to 8 bytes (CAN 2.0) and are never chunked.
