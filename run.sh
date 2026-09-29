#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="$SCRIPT_DIR/build-app/EmbeddedUIDesigner"

if [ ! -f "$BIN" ]; then
    echo "Binary not found. Building..."
    cmake -B "$SCRIPT_DIR/build-app" -S "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$SCRIPT_DIR/build-app" -j$(nproc)
fi

echo "Launching Embedded UI Designer..."
"$BIN" "$@"
