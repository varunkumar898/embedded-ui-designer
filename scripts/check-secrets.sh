#!/usr/bin/env bash
# Grep-based hardcoded-secret scan over tracked files.
# Exit 1 when a likely secret is found. Keep patterns conservative to avoid noise.
set -euo pipefail

cd "$(git rev-parse --show-toplevel 2>/dev/null || pwd)"

EXCLUDE_PATHS='node_modules/|(^|/)tests/|(^|/)fixtures/|(^|/)docs/|(^|/)examples/|CHANGELOG'
# Lines containing obvious placeholders/tests are not secrets
EXCLUDE_LINES='REDACTED|mock|Mock|placeholder|YOURTOKEN|your-token|<token>|EXAMPLE|sample[- ]|/tests/|node_modules/'

PATTERNS=(
  # Known cloud/AI providers
  'ANTHROPIC_AUTH_TOKEN="[A-Za-z0-9._-]{8,}"'
  'ANTHROPIC_API_KEY="sk-[A-Za-z0-9_-]{12,}"'
  '(OPENAI_API_KEY|OPENAI_TOKEN)\s*[=:]\s*"[A-Za-z0-9._-]{16,}"'
  'sk-ant-[A-Za-z0-9_-]{16,}'
  'sk-[A-Za-z0-9]{32,}'
  # SCM/infra tokens
  'gh[pousr]_[A-Za-z0-9]{20,}'
  'github_pat_[A-Za-z0-9_]{20,}'
  'xox[baprs]-[A-Za-z0-9-]{10,}'
  'glpat-[A-Za-z0-9_-]{15,}'
  # AWS/GCP
  'AKIA[0-9A-Z]{16}'
  'AIza[0-9A-Za-z_-]{30,}'
  # Generic assignments of nontrivial secrets
  '(authToken|auth_token|apiKey|api_key|secretKey|secret_key|clientSecret|client_secret)\s*[=:]\s*"[A-Za-z0-9._+/-]{24,}"'
  # Key material
  'BEGIN [A-Z ]*PRIVATE KEY'
)

FOUND=0
while IFS= read -r -d '' file; do
  echo "$file" | grep -qE "$EXCLUDE_PATHS" && continue
  matches=$(grep -InE "$(printf '%s|' "${PATTERNS[@]}" | sed 's/|$//')" "$file" 2>/dev/null | grep -ivE "$EXCLUDE_LINES" || true)
  if [ -n "$matches" ]; then
    FOUND=1
    # Redact: never print matched line content in CI logs (path + rule index only)
    rule_no=0
    echo "Possible secret in: $file"
    while IFS= read -r line; do
      rule_no=$((rule_no+1))
      lineno="${line%%:*}"
      echo "  $file:$lineno (rule #$rule_no)"
      echo
    done <<< "$matches"
    echo
  fi
done < <(git ls-files -z)

if [ "$FOUND" -eq 1 ]; then
  echo "check-secrets.sh: FAILED — remove the credential(s), load via env instead." >&2
  exit 1
fi
echo "check-secrets.sh: OK (no hardcoded secrets found in tracked files)"
