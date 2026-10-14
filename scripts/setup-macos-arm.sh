#!/usr/bin/env bash
set -euo pipefail

echo "============================================================"
echo "  Embedded UI Designer: macOS Apple Silicon (ARM64) Setup"
echo "============================================================"

# Verify Xcode command line tools
if ! xcode-select -p &> /dev/null; then
    echo "Installing Xcode command line tools..."
    xcode-select --install
fi

echo "[1/4] Installing Homebrew dependencies for Apple Silicon..."
if command -v brew &> /dev/null; then
    brew install cmake ninja arm-none-eabi-gcc jsoncpp node
fi

echo "[2/4] Verifying Apple Silicon Clang..."
clang --version | head -n 1

echo "[3/4] Configuring CMake for macOS ARM64..."
mkdir -p build && cd build
cmake -DPLATFORM=macOS -DARCH=ARM64 -DENABLE_TESTS=ON ..

echo "[4/4] Executing test build..."
cmake --build . --target test_project_model test_codegen_multi_language test_security_performance test_compliance_observability -j"$(sysctl -n hw.ncpu || echo 4)"

echo "✓ macOS Apple Silicon setup complete!"
