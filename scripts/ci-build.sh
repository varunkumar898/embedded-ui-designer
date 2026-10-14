#!/usr/bin/env bash
set -euo pipefail

echo "============================================================"
echo "  Embedded UI Designer: Unified CI Build & Test Script"
echo "============================================================"

BUILD_DIR="${BUILD_DIR:-build}"
TARGET_PLATFORM="${PLATFORM:-auto}"
TARGET_ARCH="${ARCH:-auto}"
RUN_TESTS="${ENABLE_TESTS:-ON}"

echo "Configuration:"
echo "  Build Directory: $BUILD_DIR"
echo "  Platform:        $TARGET_PLATFORM"
echo "  Architecture:    $TARGET_ARCH"
echo "  Run Tests:       $RUN_TESTS"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake -DPLATFORM="$TARGET_PLATFORM" \
      -DARCH="$TARGET_ARCH" \
      -DENABLE_TESTS="$RUN_TESTS" \
      -DENABLE_SECURITY_HARDENING=ON \
      -DENABLE_OBSERVABILITY=ON ..

cmake --build . --target test_project_model test_codegen_multi_language test_security_performance test_compliance_observability -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

if [[ "$RUN_TESTS" == "ON" ]]; then
    echo "Executing all acceptance tests..."
    ctest --verbose --output-on-failure
fi

echo "✓ Unified CI build succeeded!"
