#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <portable-windows-directory>" >&2
  exit 64
fi

artifact_dir=$1
if [[ ! -d "$artifact_dir" ]]; then
  echo "Windows artifact directory does not exist: $artifact_dir" >&2
  exit 1
fi

for tool in basename find objdump sed; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "Windows runtime verification requires $tool" >&2
    exit 1
  fi
done

declare -A provided=()
pe_count=0
while IFS= read -r -d '' binary; do
  name=$(basename "$binary")
  provided["${name,,}"]=1
  header=$(objdump -f "$binary")
  if [[ "$header" != *"architecture: i386:x86-64"* ]]; then
    echo "unexpected Windows binary architecture: ${binary#"$artifact_dir"/}" >&2
    exit 1
  fi
  ((pe_count += 1))
done < <(find "$artifact_dir" -type f \( -iname '*.exe' -o -iname '*.dll' \) -print0)

if (( pe_count == 0 )); then
  echo "Windows artifact contains no PE binaries" >&2
  exit 1
fi

is_windows_system_dll() {
  case "$1" in
    api-ms-win-*|ext-ms-win-*|advapi32.dll|bcrypt.dll|cfgmgr32.dll|comctl32.dll|comdlg32.dll|crypt32.dll|cryptui.dll|d3d11.dll|d3d9.dll|dbghelp.dll|dnsapi.dll|dwmapi.dll|dwrite.dll|dxgi.dll|gdi32.dll|imm32.dll|iphlpapi.dll|kernel32.dll|kernelbase.dll|mpr.dll|msvcp_win.dll|msvcrt.dll|mswsock.dll|netapi32.dll|normaliz.dll|ntdll.dll|ole32.dll|oleacc.dll|oleaut32.dll|opengl32.dll|powrprof.dll|propsys.dll|psapi.dll|rpcrt4.dll|secur32.dll|setupapi.dll|shell32.dll|shlwapi.dll|user32.dll|userenv.dll|usp10.dll|uxtheme.dll|version.dll|winhttp.dll|winmm.dll|winspool.drv|wintrust.dll|ws2_32.dll|wtsapi32.dll|wldap32.dll)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

declare -A missing=()
while IFS= read -r -d '' binary; do
  while IFS= read -r dependency; do
    [[ -n "$dependency" ]] || continue
    normalized=${dependency,,}
    if [[ -z "${provided[$normalized]:-}" ]] && ! is_windows_system_dll "$normalized"; then
      missing["$normalized"]=1
      echo "unresolved Windows import: ${binary#"$artifact_dir"/} -> $dependency" >&2
    fi
  done < <(objdump -p "$binary" | sed -n 's/^[[:space:]]*DLL Name:[[:space:]]*//p')
done < <(find "$artifact_dir" -type f \( -iname '*.exe' -o -iname '*.dll' \) -print0)

if (( ${#missing[@]} != 0 )); then
  echo "Windows artifact has ${#missing[@]} unresolved non-system DLL import(s)" >&2
  exit 1
fi

echo "Windows runtime verified: $pe_count x86-64 PE files, all imports resolved"
