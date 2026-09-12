#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <application-bundle>" >&2
  exit 64
fi

bundle=$1
if [[ ! -d "$bundle/Contents/MacOS" ]]; then
  echo "invalid macOS application bundle: $bundle" >&2
  exit 1
fi

for tool in file otool install_name_tool; do
  command -v "$tool" >/dev/null 2>&1 || {
    echo "required macOS deployment tool is unavailable: $tool" >&2
    exit 1
  }
done

normalized=0
while IFS= read -r -d '' mach_file; do
  [[ "$(file -Lb "$mach_file")" == Mach-O* ]] || continue

  install_id=$(otool -D "$mach_file" 2>/dev/null | tail -n +2 | head -n 1 || true)
  case "$install_id" in
    /opt/homebrew/*|/usr/local/*|/Users/runner/*|/opt/hostedtoolcache/*)
      install_name_tool -id "@rpath/$(basename "$mach_file")" "$mach_file"
      normalized=$((normalized + 1))
      ;;
  esac
done < <(find "$bundle" -type f -print0)

printf 'Normalized %d runner-local Mach-O install IDs\n' "$normalized"
