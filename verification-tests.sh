#!/bin/bash
# verification-tests.sh - Run automated acceptance tests on physical device or simulator

TARGET_PORT="${1:-8080}"
echo "[VERIFY] Running full-stack integration and acceptance tests against device on port ${TARGET_PORT}..."

if command -v node > /dev/null 2>&1; then
    node src/frontend/tests/integration_runner.js
    exit $?
else
    echo "[VERIFY] Node.js not detected, executing curl-based sanity checks..."
    curl -s -f "http://localhost:${TARGET_PORT}/api/v1/device/dev1/gpio/read?pinNumber=0" | grep -q "value" && \
    curl -s -f -X POST -H "Content-Type: application/json" -d '{"pin": 10, "dutyCycle": 50}' "http://localhost:${TARGET_PORT}/api/v1/pwm/write" | grep -q "OK"
    if [ $? -eq 0 ]; then
        echo "[VERIFY] ✓ Sanity tests passed!"
        exit 0
    else
        echo "[VERIFY] ✗ Sanity tests failed!"
        exit 1
    fi
fi
