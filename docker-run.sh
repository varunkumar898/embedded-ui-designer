#!/usr/bin/env bash
set -e

CONTAINER_NAME="embedded-ui-claude"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLAUDE_CONFIG_DIR="$HOME/.claude"

# 1. Clean up old container instance if running
if [ "$(docker ps -aq -f name=^${CONTAINER_NAME}$)" ]; then
    echo "🧹 Removing old container: ${CONTAINER_NAME}..."
    docker rm -f "${CONTAINER_NAME}" > /dev/null
fi

# 2. Ensure host config directory exists and clean up conflicting json config
mkdir -p "$CLAUDE_CONFIG_DIR"
rm -f "$CLAUDE_CONFIG_DIR/.claude.json"

echo "🚀 Launching Clean Claude Code Sandbox (${CONTAINER_NAME})..."
echo "📦 Workspace: ${PROJECT_DIR}"
echo "🤖 Model: glm-5.3-flash"

# 3. Launch Docker Container
docker run -it --rm \
  --name "${CONTAINER_NAME}" \
  -v "${PROJECT_DIR}":/app \
  -v "${CLAUDE_CONFIG_DIR}":/root/.claude \
  -w /app \
  -p 3000:3000 \
  -e ANTHROPIC_AUTH_TOKEN="89a1f1559eeb4ba3bf75dfb779d6863b.60JtyH7gxJAztMyC" \
  -e ANTHROPIC_BASE_URL="https://api.z.ai/api/anthropic" \
  -e ANTHROPIC_API_KEY="" \
  -e ANTHROPIC_DEFAULT_OPUS_MODEL="glm-5.3-flash" \
  -e ANTHROPIC_DEFAULT_SONNET_MODEL="glm-5.3-flash" \
  -e ANTHROPIC_DEFAULT_HAIKU_MODEL="glm-5.3-flash" \
  node:20-slim \
  npx @anthropic-ai/claude-code@latest
