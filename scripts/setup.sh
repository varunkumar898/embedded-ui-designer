#!/usr/bin/env bash
set -euo pipefail

echo "============================================================"
echo "  Embedded UI Designer: Unified Setup"
echo "============================================================"

OS="$(uname -s)"
ARCH="$(uname -m)"

case "$OS" in
    Linux)
        if [[ "$ARCH" == "aarch64" || "$ARCH" == "armv7l" ]]; then
            exec "$(dirname "$0")/setup-linux-arm.sh"
        else
            exec "$(dirname "$0")/setup-linux-x86.sh"
        fi
        ;;
    Darwin)
        if [[ "$ARCH" == "arm64" ]]; then
            exec "$(dirname "$0")/setup-macos-arm.sh"
        else
            exec "$(dirname "$0")/setup-macos-x86.sh"
        fi
        ;;
    *)
        echo "Unknown or unsupported OS for auto-setup: $OS ($ARCH)"
        echo "Please execute the specific script in ./scripts/"
        exit 1
        ;;
esac
