#!/bin/bash
# health-check.sh - Verify embedded device REST server is responsive

TARGET_PORT="${1:-8080}"
MAX_RETRIES=5
RETRY_DELAY=1
URL="http://localhost:${TARGET_PORT}/api/v1/adc/read?channel=0"

echo "[HEALTH CHECK] Probing target device at ${URL}..."

for i in $(seq 1 $MAX_RETRIES); do
    if curl -s -f -m 2 "$URL" > /dev/null 2>&1; then
        echo "[HEALTH CHECK] ✓ Target device responded successfully (Attempt $i/$MAX_RETRIES)"
        exit 0
    fi
    echo "[HEALTH CHECK] Attempt $i failed, waiting ${RETRY_DELAY}s..."
    sleep $RETRY_DELAY
done

echo "[HEALTH CHECK] ✗ Device probe failed after $MAX_RETRIES attempts"
exit 1
