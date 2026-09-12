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
frameworks="$bundle/Contents/Frameworks"

for tool in file otool install_name_tool; do
  command -v "$tool" >/dev/null 2>&1 || {
    echo "required macOS deployment tool is unavailable: $tool" >&2
    exit 1
  }
done

normalized_ids=0
normalized_dependencies=0
removed_rpaths=0
while IFS= read -r -d '' mach_file; do
  [[ "$(file -Lb "$mach_file")" == Mach-O* ]] || continue

  install_id=$(otool -D "$mach_file" 2>/dev/null | tail -n +2 | head -n 1 || true)
  case "$install_id" in
    /opt/homebrew/*|/usr/local/*|/Users/runner/*|/opt/hostedtoolcache/*)
      install_name_tool -id "@rpath/$(basename "$mach_file")" "$mach_file"
      normalized_ids=$((normalized_ids + 1))
      ;;
  esac

  while IFS= read -r dependency; do
    [[ -n "$dependency" && "$dependency" != "$install_id" ]] || continue
    case "$dependency" in
      /opt/homebrew/*|/usr/local/*|/Users/runner/*|/opt/hostedtoolcache/*)
        replacement=""
        if [[ "$dependency" =~ /([^/]+)\.framework/(.+)$ ]]; then
          framework_name=${BASH_REMATCH[1]}
          framework_suffix=${BASH_REMATCH[2]}
          bundled_dependency="$frameworks/$framework_name.framework/$framework_suffix"
          if [[ -e "$bundled_dependency" ]]; then
            replacement="@executable_path/../Frameworks/$framework_name.framework/$framework_suffix"
          fi
        else
          dependency_name=$(basename "$dependency")
          if [[ -e "$frameworks/$dependency_name" ]]; then
            replacement="@executable_path/../Frameworks/$dependency_name"
          fi
        fi

        if [[ -z "$replacement" ]]; then
          echo "runner-local dependency was not bundled: $dependency ($mach_file)" >&2
          exit 1
        fi
        install_name_tool -change "$dependency" "$replacement" "$mach_file"
        normalized_dependencies=$((normalized_dependencies + 1))
        ;;
    esac
  done < <(otool -L "$mach_file" | tail -n +2 | sed -E 's/^[[:space:]]*([^[:space:]]+).*/\1/')

  while IFS= read -r rpath; do
    [[ -n "$rpath" ]] || continue
    case "$rpath" in
      /opt/homebrew/*|/usr/local/*|/Users/runner/*|/opt/hostedtoolcache/*)
        install_name_tool -delete_rpath "$rpath" "$mach_file"
        removed_rpaths=$((removed_rpaths + 1))
        ;;
    esac
  done < <(
    otool -l "$mach_file" |
      awk '$1 == "cmd" && $2 == "LC_RPATH" { want_path = 1; next }
           want_path && $1 == "path" { print $2; want_path = 0 }'
  )
done < <(find "$bundle" -type f -print0)

printf 'Normalized %d runner-local Mach-O install IDs and %d dependencies; removed %d runner-local rpaths\n' \
  "$normalized_ids" "$normalized_dependencies" "$removed_rpaths"
