#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
core_path="$repo_root/qwertycoin"
expected_sha=09086f7dbaaf1a4ff16bddeaa1d729f9ff65eca6
expected_remote=https://github.com/qwertycoin-org/qwertycoin.git

gitlink_sha=$(git -C "$repo_root" ls-files --stage -- qwertycoin | awk '$1 == "160000" {print $2}')
if [[ "$gitlink_sha" != "$expected_sha" ]]; then
  echo "unexpected qwertycoin Gitlink: ${gitlink_sha:-missing}" >&2
  exit 1
fi

if [[ ! -e "$core_path/.git" ]]; then
  echo "qwertycoin submodule is not initialized" >&2
  exit 1
fi

checkout_sha=$(git -C "$core_path" rev-parse HEAD)
if [[ "$checkout_sha" != "$expected_sha" ]]; then
  echo "qwertycoin checkout does not match the pinned Gitlink: $checkout_sha" >&2
  exit 1
fi

remote_url=$(git -C "$core_path" remote get-url origin)
if [[ "$remote_url" != "$expected_remote" ]]; then
  echo "unexpected qwertycoin origin: $remote_url" >&2
  exit 1
fi

core_status=$(git -C "$core_path" status --porcelain --untracked-files=normal)
if [[ -n "$core_status" ]]; then
  echo "qwertycoin submodule has local changes" >&2
  printf '%s\n' "$core_status" >&2
  exit 1
fi

if grep -A3 '^\[submodule "qwertycoin"\]' "$repo_root/.gitmodules" | grep -q 'ignore[[:space:]]*='; then
  echo "qwertycoin submodule changes must not be hidden with an ignore rule" >&2
  exit 1
fi

echo "qwertycoin core pin verified: $expected_sha"
