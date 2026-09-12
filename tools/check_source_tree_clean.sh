#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
status=$(git -C "$repo_root" status --porcelain --untracked-files=no)

if [[ -n "$status" ]]; then
  echo "tracked GUI source-tree changes detected:" >&2
  printf '%s\n' "$status" >&2
  exit 1
fi

echo "GUI source tree is clean"
