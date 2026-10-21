#!/usr/bin/env bash
# Fails when two tracked paths differ only by case (breaks checkouts on
# case-insensitive filesystems such as Windows/macOS, which are in the CI matrix).
set -euo pipefail

cd "$(git rev-parse --show-toplevel 2>/dev/null || pwd)"

DUPS=$(git ls-files -z | tr '\0' '\n' \
  | awk '{ orig[NR]=$0; low=tolower($0); seen[low]++ } END { for (i=1;i<=NR;i++) { low=tolower(orig[i]); if (seen[low]>1) print orig[i] } }' \
  | sort)

if [ -n "$DUPS" ]; then
  echo "Case-colliding tracked paths detected:" >&2
  echo "$DUPS" >&2
  echo "" >&2
  echo "These files cannot coexist in one directory on case-insensitive filesystems." >&2
  echo "Rename them so paths differ by more than case (see scripts/README-hardening.md)." >&2
  exit 1
fi

echo "check-case-collisions.sh: OK (no tracked paths differ only by case)"
