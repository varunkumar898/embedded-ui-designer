# Troubleshooting Guide

## Common Issues & Remediation Steps

### 1. Device Unreachable via HTTP / REST API
* **Symptoms**: Requests to `http://<device-ip>:8080/api/v1/...` time out with `ERR_CONNECTION_TIMED_OUT` or `Connection refused`.
* **Root Causes**:
  - Wi-Fi or Ethernet interface not initialized.
  - IP address assignment conflict or DHCP renewal failure.
  - Rate limiter has exceeded quota for client IP.
* **Remediation**:
  1. Ping device IP: `ping <device-ip>`.
  2. Inspect serial console output via UART (`picocom -b 115200 /dev/ttyUSB0`).
  3. Verify port 8080 is open: `nmap -p 8080 <device-ip>`.
  4. Reset rate limiter state if client was throttled.

---

### 2. Authentication Failures (HTTP 401 Unauthorized / 403 Forbidden)
* **Symptoms**: API returns `{"error": "Unauthorized"}` or `{"error": "Forbidden"}`.
* **Root Causes**:
  - API Key or Secret mismatch.
  - Missing or expired Bearer token.
  - Required scope (e.g. `pwm.write`) missing from token permissions.
  - HMAC nonce reused or timestamp drifted > 300 seconds.
* **Remediation**:
  1. Inspect authorization header: verify format `Authorization: ApiKey <id>` with `X-API-Secret: <secret>`.
  2. For Bearer tokens, decode JWT/token expiration time against device clock.
  3. Check [AuditLogger](file:///home/cherry/embedded-ui-designer/src/security/AuditLogger.h) entries for specific rejection reasons.

---

### 3. Firmware Flashing Failures
* **Symptoms**: Flashing fails with `Target not responding` or `Error: JTAG scan chain interrogation failed`.
* **Root Causes**:
  - Incorrect debug interface configuration (ST-Link vs CMSIS-DAP vs J-Link).
  - Target MCU in low-power sleep mode or boot pin set incorrectly.
  - USB cable lacks data lines.
* **Remediation**:
  1. Verify hardware connection: `lsusb` to ensure debugger appears.
  2. Hold RESET pin while initiating OpenOCD / esptool connect sequence.
  3. For ESP32, hold BOOT (GPIO0) low during power-on to enter ROM bootloader.

---

### 4. High Latency or Out-Of-Memory (OOM)
* **Symptoms**: Request latency spikes > 500ms, or device resets unexpectedly.
* **Root Causes**:
  - Response cache size exceeded or TTL set too high.
  - Connection pool exhausted by open keep-alive connections.
  - Heap fragmentation from frequent JSON string allocations.
* **Remediation**:
  1. Check `PerformanceMetrics::getMemoryWatermark()` to detect memory leaks.
  2. Run `ConnectionPool::closeIdleConnections()`.
  3. Tune `ResponseCache` max entries and reduce default TTL.
