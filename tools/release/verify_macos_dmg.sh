#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <dmg-path> <verified-package-dir>" >&2
  exit 64
fi
if [[ "$(uname -s)" != Darwin ]]; then
  echo "macOS DMG verification requires Darwin" >&2
  exit 69
fi

dmg_input=$1
package_dir_input=$2
if [[ ! -f "$dmg_input" || -L "$dmg_input" ]]; then
  echo "DMG is missing or is not a regular file: $dmg_input" >&2
  exit 66
fi
if [[ ! -d "$package_dir_input" || -L "$package_dir_input" ]]; then
  echo "verified package directory is missing or unsafe: $package_dir_input" >&2
  exit 66
fi

dmg_path=$(cd "$(dirname "$dmg_input")" && printf '%s/%s\n' "$(pwd -P)" "$(basename "$dmg_input")")
package_dir=$(cd "$package_dir_input" && pwd -P)
script_dir=$(cd "$(dirname "$0")" && pwd -P)

source_app=""
for candidate in "$package_dir/qwertycoin-gui.app" "$package_dir/Qwertycoin GUI.app"; do
  if [[ -d "$candidate" && ! -L "$candidate" ]]; then
    if [[ -n "$source_app" ]]; then
      echo "verified package contains multiple application bundles" >&2
      exit 65
    fi
    source_app=$candidate
  fi
done
if [[ -z "$source_app" ]]; then
  echo "verified package contains no supported application bundle" >&2
  exit 66
fi

for required_tool in codesign diskutil hdiutil shasum; do
  if ! command -v "$required_tool" >/dev/null 2>&1; then
    echo "required macOS tool is unavailable: $required_tool" >&2
    exit 69
  fi
done

temporary_root=${RUNNER_TEMP:-/tmp}
working_dir=$(mktemp -d "$temporary_root/qwc-macos-dmg-verify.XXXXXX")
mount_dir="$working_dir/mount"
mkdir "$mount_dir"
mounted=false
cleanup() {
  if [[ "$mounted" == true ]]; then
    hdiutil detach "$mount_dir" -quiet || true
  fi
  rm -rf "$working_dir"
}
trap cleanup EXIT INT TERM

hdiutil verify "$dmg_path"
codesign --verify --deep --strict "$source_app"
hdiutil attach -readonly -nobrowse -mountpoint "$mount_dir" "$dmg_path" >/dev/null
mounted=true

diskutil info -plist "$mount_dir" >"$working_dir/volume.plist"
volume_name=$(/usr/libexec/PlistBuddy -c 'Print :VolumeName' "$working_dir/volume.plist")
if [[ "$volume_name" != "Qwertycoin Wallet" ]]; then
  echo "unexpected DMG volume name: $volume_name" >&2
  exit 65
fi
if touch "$mount_dir/.qwc-write-probe" 2>/dev/null; then
  rm -f "$mount_dir/.qwc-write-probe"
  echo "DMG unexpectedly accepted a write" >&2
  exit 65
fi

mounted_app="$mount_dir/Qwertycoin.app"
if [[ ! -d "$mounted_app" || -L "$mounted_app" ]]; then
  echo "DMG does not contain Qwertycoin.app" >&2
  exit 66
fi
if [[ ! -L "$mount_dir/Applications" || "$(readlink "$mount_dir/Applications")" != /Applications ]]; then
  echo "DMG Applications link does not point exactly to /Applications" >&2
  exit 65
fi
for required_path in \
  "$mount_dir/.background.png" \
  "$mount_dir/Documentation/README.md" \
  "$mount_dir/Documentation/LICENSE" \
  "$mount_dir/Documentation/BUILD-INFO.txt" \
  "$mount_dir/Documentation/share/qwertycoin-gui/Archivo/OFL.txt" \
  "$mount_dir/Documentation/share/qwertycoin-gui/Inter/LICENSE.txt"; do
  if [[ ! -e "$required_path" || -L "$required_path" ]]; then
    echo "DMG is missing required content: $required_path" >&2
    exit 66
  fi
done

cmp "$package_dir/README.md" "$mount_dir/Documentation/README.md"
cmp "$package_dir/LICENSE" "$mount_dir/Documentation/LICENSE"
cmp "$package_dir/BUILD-INFO.txt" "$mount_dir/Documentation/BUILD-INFO.txt"
codesign --verify --deep --strict "$mounted_app"
python3 "$script_dir/compare_bundle_trees.py" "$source_app" "$mounted_app"

copied_app="$working_dir/Qwertycoin.app"
ditto --rsrc --extattr "$mounted_app" "$copied_app"
hdiutil detach "$mount_dir" -quiet
mounted=false

codesign --verify --deep --strict "$copied_app"
python3 "$script_dir/compare_bundle_trees.py" "$source_app" "$copied_app"

smoke_home="$working_dir/home"
smoke_tmp="$working_dir/tmp"
smoke_log="$working_dir/qwertycoin-gui.log"
mkdir "$smoke_home" "$smoke_tmp"
set +e
env \
  HOME="$smoke_home" \
  TMPDIR="$smoke_tmp" \
  QT_QPA_PLATFORM=cocoa \
  QT_QUICK_BACKEND=software \
  "$copied_app/Contents/MacOS/qwertycoin-gui" \
    --disable-check-updates --test-qml >"$smoke_log" 2>&1
smoke_status=$?
set -e
cat "$smoke_log"
if [[ $smoke_status -ne 0 ]]; then
  echo "copied DMG wallet smoke exited with status $smoke_status" >&2
  exit 1
fi
if grep -Eiq \
  'TypeError|ReferenceError|module .* is not installed|QQmlApplicationEngine failed|failed to load component|no root objects|cannot load library|could not load the Qt platform plugin' \
  "$smoke_log"; then
  echo "copied DMG wallet smoke reported a QML or runtime-loading error" >&2
  exit 1
fi

echo "Verified read-only DMG, exact app bundle, signature, Applications link and detached-copy launch"
