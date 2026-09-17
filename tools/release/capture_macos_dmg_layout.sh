#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <dmg-path> <screenshot-path>" >&2
  exit 64
fi
if [[ "$(uname -s)" != Darwin ]]; then
  echo "Finder layout capture requires Darwin" >&2
  exit 69
fi

dmg_input=$1
screenshot_input=$2
if [[ ! -f "$dmg_input" || -L "$dmg_input" ]]; then
  echo "DMG is missing or unsafe: $dmg_input" >&2
  exit 66
fi
if [[ -e "$screenshot_input" ]]; then
  echo "refusing to overwrite an existing screenshot" >&2
  exit 73
fi

dmg_path=$(cd "$(dirname "$dmg_input")" && printf '%s/%s\n' "$(pwd -P)" "$(basename "$dmg_input")")
screenshot_dir=$(cd "$(dirname "$screenshot_input")" && pwd -P)
screenshot_path="$screenshot_dir/$(basename "$screenshot_input")"
temporary_root=${RUNNER_TEMP:-/tmp}
working_dir=$(mktemp -d "$temporary_root/qwc-macos-dmg-layout.XXXXXX")
mount_dir="$working_dir/Qwertycoin Wallet"
mkdir "$mount_dir"
mounted=false
cleanup() {
  osascript -e 'tell application "Finder" to close every window whose name is "Qwertycoin Wallet"' >/dev/null 2>&1 || true
  if [[ "$mounted" == true ]]; then
    hdiutil detach "$mount_dir" -quiet || true
  fi
  rm -rf "$working_dir"
}
trap cleanup EXIT INT TERM

hdiutil attach -readonly -nobrowse -mountpoint "$mount_dir" "$dmg_path" >/dev/null
mounted=true
open "$mount_dir"

bounds=$(osascript <<'APPLESCRIPT'
tell application "Finder"
  activate
  repeat 30 times
    if exists window "Qwertycoin Wallet" then exit repeat
    delay 0.25
  end repeat
  if not (exists window "Qwertycoin Wallet") then error "Qwertycoin Wallet Finder window did not open"
  delay 2
  return bounds of window "Qwertycoin Wallet"
end tell
APPLESCRIPT
)
IFS=', ' read -r left top right bottom <<<"$bounds"
for coordinate in "$left" "$top" "$right" "$bottom"; do
  [[ "$coordinate" =~ ^[0-9]+$ ]] || { echo "invalid Finder window bounds: $bounds" >&2; exit 65; }
done
width=$((right - left))
height=$((bottom - top))
if (( width < 650 || height < 390 )); then
  echo "Finder window is smaller than the configured DMG layout: $bounds" >&2
  exit 65
fi
screencapture -x -R"$left,$top,$width,$height" "$screenshot_path"
if [[ ! -s "$screenshot_path" ]]; then
  echo "Finder screenshot was not created" >&2
  exit 70
fi
echo "Captured Finder layout: $screenshot_path"
