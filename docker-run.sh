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

# 3. Ensure credentials come from the environment (never hard-code them here).
#    Requires ANTHROPIC_AUTH_TOKEN (and optionally ANTHROPIC_BASE_URL) to be set,
#    e.g. from a local .env that is NOT committed: `set -a; . ./.env; set +a`.
if [ -z "${ANTHROPIC_AUTH_TOKEN:-}" ]; then
    cat >&2 <<'EOF'
ERROR: ANTHROPIC_AUTH_TOKEN is not set.

This script no longer hard-codes credentials. Export it before running:

  export ANTHROPIC_AUTH_TOKEN="<your-token>"
  export ANTHROPIC_BASE_URL="${ANTHROPIC_BASE_URL:-https://api.z.ai/api/anthropic}"

or keep it in a local, git-ignored .env and load it:
  set -a; . ./.env; set +a
EOF
    exit 1
fi

ANTHROPIC_BASE_URL="${ANTHROPIC_BASE_URL:-https://api.z.ai/api/anthropic}"

# Launch Docker Container
docker run -it --rm \
  --name "${CONTAINER_NAME}" \
  -v "${PROJECT_DIR}":/app \
  -v "${CLAUDE_CONFIG_DIR}":/root/.claude \
  -w /app \
  -p 3000:3000 \
  -e ANTHROPIC_AUTH_TOKEN \
  -e ANTHROPIC_BASE_URL \
  -e ANTHROPIC_API_KEY="" \
  -e ANTHROPIC_DEFAULT_OPUS_MODEL="glm-5.3-flash" \
  -e ANTHROPIC_DEFAULT_SONNET_MODEL="glm-5.3-flash" \
  -e ANTHROPIC_DEFAULT_HAIKU_MODEL="glm-5.3-flash" \
  node:20-slim \
  npx @anthropic-ai/claude-code@latest
