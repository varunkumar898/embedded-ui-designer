#!/usr/bin/env bash
set -euo pipefail

echo "============================================================"
echo "  Embedded UI Designer: macOS Intel (x86_64) Setup"
echo "============================================================"

# Verify Xcode command line tools
if ! xcode-select -p &> /dev/null; then
    echo "Installing Xcode command line tools..."
    xcode-select --install
fi

echo "[1/4] Installing Homebrew dependencies..."
if command -v brew &> /dev/null; then
    brew install cmake ninja llvm jsoncpp node
fi

echo "[2/4] Verifying compiler environment..."
clang --version | head -n 1
cmake --version | head -n 1

echo "[3/4] Configuring CMake for macOS Intel..."
mkdir -p build && cd build
cmake -DPLATFORM=macOS -DARCH=x86_64 -DENABLE_TESTS=ON ..

echo "[4/4] Executing test build..."
cmake --build . --target test_project_model test_codegen_multi_language test_security_performance test_compliance_observability -j"$(sysctl -n hw.ncpu || echo 4)"

echo "✓ macOS Intel setup complete!"
