#!/usr/bin/env bash
set -euo pipefail

echo "============================================================"
echo "  Embedded UI Designer: Linux x86_64 Environment Setup"
echo "============================================================"

# OS and Architecture Detection
OS="$(uname -s)"
ARCH="$(uname -m)"

if [[ "$OS" != "Linux" ]] || [[ "$ARCH" != "x86_64" ]]; then
    echo "Warning: Detected OS=$OS, ARCH=$ARCH. Recommended target: Linux x86_64"
fi

echo "[1/4] Installing system dependencies (Ubuntu/Debian)..."
if command -v apt-get &> /dev/null; then
    sudo apt-get update -qq
    sudo apt-get install -y -qq build-essential cmake ninja-build \
        arm-none-eabi-gcc arm-none-eabi-newlib \
        libjsoncpp-dev libgl1-mesa-dev nodejs npm
fi

echo "[2/4] Verifying CMake and compilers..."
cmake --version | head -n 1
gcc --version | head -n 1

echo "[3/4] Configuring build with CMake..."
mkdir -p build && cd build
cmake -DPLATFORM=Linux -DARCH=x86_64 -DENABLE_TESTS=ON ..

echo "[4/4] Executing test build..."
cmake --build . --target test_project_model test_codegen_multi_language test_security_performance test_compliance_observability -j"$(nproc)"

echo "✓ Linux x86_64 setup and verification complete!"
