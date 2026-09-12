#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <artifact-directory>" >&2
  exit 64
fi

artifact_dir=$(realpath "$1")
library_dir="$artifact_dir/lib"

if [[ "$(uname -s)" != "Linux" ]]; then
  echo "Linux runtime bundling must run on Linux" >&2
  exit 1
fi

for command_name in file ldd patchelf realpath; do
  if ! command -v "$command_name" >/dev/null 2>&1; then
    echo "missing Linux runtime bundling command: $command_name" >&2
    exit 1
  fi
done

mkdir -p "$library_dir"

is_elf_file() {
  local candidate=$1
  [[ -f "$candidate" ]] || return 1
  [[ "$(file -Lb "$candidate")" == ELF* ]]
}

is_system_runtime() {
  local basename=$1
  case "$basename" in
    ld-linux-*.so.*|libanl.so.*|libc.so.*|libdl.so.*|libm.so.*|libnss_*.so.*|\
      libpthread.so.*|libresolv.so.*|librt.so.*|libutil.so.*)
      return 0
      ;;
  esac
  return 1
}

list_dependencies() {
  local elf_file=$1
  local search_path=${2:-}
  if [[ -n "$search_path" ]]; then
    LD_LIBRARY_PATH="$search_path" ldd "$elf_file" 2>/dev/null
  else
    ldd "$elf_file" 2>/dev/null
  fi \
    | sed -n -E \
        -e 's/^[[:space:]]*[^[:space:]]+[[:space:]]+=>[[:space:]]+(\/[^[:space:]]+).*/\1/p' \
        -e 's/^[[:space:]]*(\/[^[:space:]]+).*/\1/p' \
    || true
}

while true; do
  copied_count=0
  while IFS= read -r -d '' elf_file; do
    is_elf_file "$elf_file" || continue
    while IFS= read -r dependency; do
      [[ -n "$dependency" && -f "$dependency" ]] || continue
      dependency_name=$(basename "$dependency")
      is_system_runtime "$dependency_name" && continue

      destination="$library_dir/$dependency_name"
      if [[ -e "$destination" ]]; then
        if ! cmp -s "$dependency" "$destination"; then
          echo "dependency basename collision for $dependency_name" >&2
          echo "existing: $destination" >&2
          echo "new:      $dependency" >&2
          exit 1
        fi
        continue
      fi

      cp -L -- "$dependency" "$destination"
      copied_count=$((copied_count + 1))
    done < <(list_dependencies "$elf_file" "${QWC_RUNTIME_LIBRARY_PATH:-}")
  done < <(find "$artifact_dir" -type f -print0)

  [[ "$copied_count" -eq 0 ]] && break
done

while IFS= read -r -d '' elf_file; do
  is_elf_file "$elf_file" || continue
  relative_library_dir=$(realpath --relative-to="$(dirname "$elf_file")" "$library_dir")
  if [[ "$relative_library_dir" == "." ]]; then
    runtime_path='$ORIGIN'
  else
    runtime_path="\$ORIGIN/$relative_library_dir"
  fi
  patchelf --set-rpath "$runtime_path" "$elf_file"
done < <(find "$artifact_dir" -type f -print0)

verification_failed=0
while IFS= read -r -d '' elf_file; do
  is_elf_file "$elf_file" || continue
  if ldd "$elf_file" 2>&1 | grep -q 'not found'; then
    echo "unresolved dependency in $elf_file" >&2
    ldd "$elf_file" >&2 || true
    verification_failed=1
    continue
  fi

  while IFS= read -r dependency; do
    [[ -n "$dependency" && -f "$dependency" ]] || continue
    dependency_name=$(basename "$dependency")
    is_system_runtime "$dependency_name" && continue
    case "$dependency" in
      "$artifact_dir"/*) ;;
      *)
        echo "non-system dependency resolves outside the artifact: $elf_file -> $dependency" >&2
        verification_failed=1
        ;;
    esac
  done < <(list_dependencies "$elf_file")
done < <(find "$artifact_dir" -type f -print0)

if [[ "$verification_failed" -ne 0 ]]; then
  exit 1
fi

for required_library in libQt5Core.so.5 libQt5Gui.so.5 libQt5Qml.so.5 libQt5Quick.so.5; do
  if [[ ! -f "$library_dir/$required_library" ]]; then
    echo "missing bundled Qt library: $required_library" >&2
    exit 1
  fi
done

echo "Linux dependency closure verified inside $artifact_dir"
