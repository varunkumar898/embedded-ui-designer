#!/usr/bin/env bash
set -euo pipefail

echo "============================================================"
echo "  Embedded UI Designer: Linux ARM64 / Raspberry Pi Setup"
echo "============================================================"

OS="$(uname -s)"
ARCH="$(uname -m)"

echo "Detected OS: $OS, Architecture: $ARCH"

echo "[1/4] Installing dependencies for ARM platform..."
if command -v apt-get &> /dev/null; then
    sudo apt-get update -qq
    sudo apt-get install -y -qq build-essential cmake ninja-build \
        arm-none-eabi-gcc arm-none-eabi-newlib \
        libjsoncpp-dev nodejs npm
fi

echo "[2/4] Verifying native ARM toolchain..."
gcc -v 2>&1 | tail -n 1

echo "[3/4] Configuring CMake for Linux ARM..."
mkdir -p build && cd build
cmake -DPLATFORM=Linux -DARCH=ARM64 -DENABLE_TESTS=ON ..

echo "[4/4] Running build verification on ARM..."
cmake --build . --target test_project_model test_codegen_multi_language test_security_performance test_compliance_observability -j"$(nproc)"

echo "✓ Linux ARM setup and verification complete!"
