#!/usr/bin/env bash
set -euo pipefail

verify_only=false
if [[ ${1:-} == --verify-only ]]; then
  verify_only=true
  shift
fi

if [[ "$verify_only" == true ]]; then
  if [[ $# -ne 1 ]]; then
    echo "usage: $0 --verify-only <application-bundle>" >&2
    exit 64
  fi
else
  if [[ $# -lt 2 ]]; then
    echo "usage: $0 <application-bundle> <dependency-search-root> [...]" >&2
    exit 64
  fi
fi

bundle=$1
shift
if [[ ! -d "$bundle/Contents/MacOS" || -L "$bundle" ]]; then
  echo "invalid macOS application bundle: $bundle" >&2
  exit 66
fi
bundle=$(cd "$bundle" && pwd -P)
frameworks="$bundle/Contents/Frameworks"
mkdir -p "$frameworks"

for tool in file otool install_name_tool; do
  command -v "$tool" >/dev/null 2>&1 || {
    echo "required macOS deployment tool is unavailable: $tool" >&2
    exit 69
  }
done

search_roots=()
for root in "$@"; do
  if [[ -d "$root" ]]; then
    search_roots+=("$(cd "$root" && pwd -P)")
  fi
done
if [[ "$verify_only" == false && ${#search_roots[@]} -eq 0 ]]; then
  echo "no dependency search root exists" >&2
  exit 66
fi

is_system_dependency() {
  case "$1" in
    /System/*|/usr/lib/*) return 0 ;;
    *) return 1 ;;
  esac
}

dependency_resolves() {
  local mach_file=$1 dependency=$2 suffix candidate rpath
  case "$dependency" in
    @executable_path/*)
      candidate="$bundle/Contents/MacOS/${dependency#@executable_path/}"
      [[ -e "$candidate" ]]
      ;;
    @loader_path/*)
      candidate="$(dirname "$mach_file")/${dependency#@loader_path/}"
      [[ -e "$candidate" ]]
      ;;
    @rpath/*)
      suffix=${dependency#@rpath/}
      if [[ -e "$frameworks/$suffix" || -e "$frameworks/$(basename "$suffix")" ]]; then
        return 0
      fi
      while IFS= read -r rpath; do
        [[ -n "$rpath" ]] || continue
        case "$rpath" in
          @executable_path/*)
            candidate="$bundle/Contents/MacOS/${rpath#@executable_path/}/$suffix"
            ;;
          @loader_path/*)
            candidate="$(dirname "$mach_file")/${rpath#@loader_path/}/$suffix"
            ;;
          /*)
            candidate="$rpath/$suffix"
            ;;
          *)
            continue
            ;;
        esac
        [[ -e "$candidate" ]] && return 0
      done < <(
        otool -l "$mach_file" |
          awk '$1 == "cmd" && $2 == "LC_RPATH" { want_path = 1; next }
               want_path && $1 == "path" { print $2; want_path = 0 }'
      )
      return 1
      ;;
    /*)
      case "$dependency" in
        "$bundle"/*) [[ -e "$dependency" ]] ;;
        *) return 1 ;;
      esac
      ;;
    *)
      candidate="$(dirname "$mach_file")/$dependency"
      [[ -e "$candidate" ]]
      ;;
  esac
}

find_dependency_source() {
  local dependency=$1 basename_candidate root found
  if [[ "$dependency" == /* && -f "$dependency" ]]; then
    printf '%s\n' "$dependency"
    return 0
  fi
  basename_candidate=$(basename "$dependency")
  for root in "${search_roots[@]}"; do
    if [[ -f "$root/$basename_candidate" || -L "$root/$basename_candidate" ]]; then
      printf '%s\n' "$root/$basename_candidate"
      return 0
    fi
    found=$(find -L "$root" -maxdepth 5 -type f -name "$basename_candidate" -print -quit 2>/dev/null || true)
    if [[ -n "$found" ]]; then
      printf '%s\n' "$found"
      return 0
    fi
  done
  return 1
}

framework_root_from_path() {
  local path=$1 current
  current=$path
  while [[ "$current" != / && "$current" != . ]]; do
    if [[ "$(basename "$current")" == *.framework ]]; then
      printf '%s\n' "$current"
      return 0
    fi
    current=$(dirname "$current")
  done
  return 1
}

find_framework_source() {
  local dependency=$1 framework_name direct_root root found
  direct_root=$(framework_root_from_path "$dependency" || true)
  if [[ -n "$direct_root" && -d "$direct_root" ]]; then
    printf '%s\n' "$direct_root"
    return 0
  fi

  framework_name=$(printf '%s\n' "$dependency" | sed -nE 's#.*\/([^/]+\.framework)\/.*#\1#p')
  [[ -n "$framework_name" ]] || return 1
  for root in "${search_roots[@]}"; do
    if [[ -d "$root/$framework_name" ]]; then
      printf '%s\n' "$root/$framework_name"
      return 0
    fi
    found=$(find -L "$root" -maxdepth 5 -type d -name "$framework_name" -print -quit 2>/dev/null || true)
    if [[ -n "$found" ]]; then
      printf '%s\n' "$found"
      return 0
    fi
  done
  return 1
}

copied_total=0
rewritten_total=0
iteration=0
while :; do
  iteration=$((iteration + 1))
  if (( iteration > 32 )); then
    echo "macOS runtime dependency closure did not converge" >&2
    exit 70
  fi
  copied_this_pass=0
  rewritten_this_pass=0
  unresolved_this_pass=0

  while IFS= read -r -d '' mach_file; do
    [[ "$(file -Lb "$mach_file")" == Mach-O* ]] || continue
    install_id=$(otool -D "$mach_file" 2>/dev/null | tail -n +2 | head -n 1 || true)
    while IFS= read -r dependency; do
      [[ -n "$dependency" && "$dependency" != "$install_id" ]] || continue
      is_system_dependency "$dependency" && continue
      dependency_resolves "$mach_file" "$dependency" && continue

      if [[ "$verify_only" == true ]]; then
        echo "unresolved bundled dependency: $dependency ($mach_file)" >&2
        unresolved_this_pass=$((unresolved_this_pass + 1))
        continue
      fi

      if [[ "$dependency" =~ /([^/]+\.framework)/(.+)$ ]]; then
        framework_name=${BASH_REMATCH[1]}
        framework_suffix=${BASH_REMATCH[2]}
        source_framework=$(find_framework_source "$dependency" || true)
        if [[ -z "$source_framework" ]]; then
          echo "unable to locate framework in approved search roots: $dependency ($mach_file)" >&2
          exit 66
        fi
        destination_framework="$frameworks/$framework_name"
        if [[ ! -e "$destination_framework" ]]; then
          cp -R "$source_framework" "$destination_framework"
          chmod -R u+w "$destination_framework"
          copied_this_pass=$((copied_this_pass + 1))
          copied_total=$((copied_total + 1))
          echo "Bundled transitive framework: $framework_name"
        fi
        if [[ ! -e "$destination_framework/$framework_suffix" ]]; then
          echo "bundled framework is missing requested binary: $framework_name/$framework_suffix" >&2
          exit 66
        fi

        replacement="@executable_path/../Frameworks/$framework_name/$framework_suffix"
        if [[ "$dependency" != "$replacement" ]]; then
          install_name_tool -change "$dependency" "$replacement" "$mach_file"
          rewritten_this_pass=$((rewritten_this_pass + 1))
          rewritten_total=$((rewritten_total + 1))
        fi
        continue
      fi

      dependency_basename=$(basename "$dependency")
      if [[ "$dependency_basename" != *.dylib ]]; then
        echo "unsupported unresolved non-dylib dependency: $dependency ($mach_file)" >&2
        exit 65
      fi
      source_path=$(find_dependency_source "$dependency" || true)
      if [[ -z "$source_path" ]]; then
        echo "unable to locate dependency in approved search roots: $dependency ($mach_file)" >&2
        exit 66
      fi
      destination="$frameworks/$dependency_basename"
      if [[ ! -e "$destination" ]]; then
        cp -L "$source_path" "$destination"
        chmod u+w "$destination"
        copied_this_pass=$((copied_this_pass + 1))
        copied_total=$((copied_total + 1))
        echo "Bundled transitive dependency: $dependency_basename"
      fi

      replacement="@executable_path/../Frameworks/$dependency_basename"
      if [[ "$dependency" != "$replacement" ]]; then
        install_name_tool -change "$dependency" "$replacement" "$mach_file"
        rewritten_this_pass=$((rewritten_this_pass + 1))
        rewritten_total=$((rewritten_total + 1))
      fi
    done < <(otool -L "$mach_file" | tail -n +2 | sed -E 's/^[[:space:]]*([^[:space:]]+).*/\1/')
  done < <(find "$bundle" -type f -print0)

  if [[ "$verify_only" == true ]]; then
    if (( unresolved_this_pass > 0 )); then
      exit 1
    fi
    printf 'macOS runtime closure verified for %s\n' "$bundle"
    exit 0
  fi
  if (( copied_this_pass == 0 && rewritten_this_pass == 0 )); then
    break
  fi
done

"$0" --verify-only "$bundle"
printf 'Completed macOS runtime closure: copied %d libraries, rewrote %d dependencies\n' \
  "$copied_total" "$rewritten_total"
