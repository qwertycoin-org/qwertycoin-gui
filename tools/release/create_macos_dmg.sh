#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: $0 <verified-package-dir> <artifact-name> <output-dir>" >&2
  exit 64
fi

if [[ "$(uname -s)" != Darwin ]]; then
  echo "macOS DMG creation requires Darwin and hdiutil" >&2
  exit 69
fi

package_dir_input=$1
artifact_name=$2
output_dir_input=$3

if [[ ! "$artifact_name" =~ ^[A-Za-z0-9][A-Za-z0-9._-]{0,127}-macos-arm64$ ]]; then
  echo "artifact name must be a safe macOS arm64 release name" >&2
  exit 64
fi
if [[ ! -d "$package_dir_input" ]]; then
  echo "verified package directory does not exist: $package_dir_input" >&2
  exit 66
fi
if [[ ! -d "$output_dir_input" ]]; then
  echo "output directory does not exist: $output_dir_input" >&2
  exit 66
fi

package_dir=$(cd "$package_dir_input" && pwd -P)
output_dir=$(cd "$output_dir_input" && pwd -P)
if [[ "$(basename "$package_dir")" != "$artifact_name" ]]; then
  echo "package directory name does not match artifact name" >&2
  exit 65
fi

script_dir=$(cd "$(dirname "$0")" && pwd -P)
repository_root=$(cd "$script_dir/../.." && pwd -P)
settings_file="$script_dir/macos_dmg/settings.py"
background_file="$script_dir/macos_dmg/background.png"
volume_icon="$repository_root/images/appicon.icns"
output_dmg="$output_dir/$artifact_name.dmg"
output_checksum="$output_dmg.sha256"

for required_file in \
  "$settings_file" "$background_file" "$volume_icon" \
  "$package_dir/README.md" "$package_dir/LICENSE" "$package_dir/BUILD-INFO.txt"; do
  if [[ ! -f "$required_file" || -L "$required_file" ]]; then
    echo "missing or unsafe required DMG input: $required_file" >&2
    exit 66
  fi
done
if [[ ! -d "$package_dir/share" || -L "$package_dir/share" ]]; then
  echo "missing or unsafe packaged share directory" >&2
  exit 66
fi
if [[ -e "$output_dmg" || -e "$output_checksum" ]]; then
  echo "refusing to overwrite an existing DMG output" >&2
  exit 73
fi

app_bundle=""
for candidate in "$package_dir/qwertycoin-gui.app" "$package_dir/Qwertycoin GUI.app"; do
  if [[ -d "$candidate" && ! -L "$candidate" ]]; then
    if [[ -n "$app_bundle" ]]; then
      echo "verified package contains multiple application bundles" >&2
      exit 65
    fi
    app_bundle=$candidate
  fi
done
if [[ -z "$app_bundle" ]]; then
  echo "verified package contains no supported application bundle" >&2
  exit 66
fi

if ! grep -Eq '^source_revision=[0-9a-f]{40}$' "$package_dir/BUILD-INFO.txt" \
    || ! grep -Eq '^core_revision=[0-9a-f]{40}$' "$package_dir/BUILD-INFO.txt" \
    || ! grep -qx 'runner_os=macOS' "$package_dir/BUILD-INFO.txt" \
    || ! grep -qx 'runner_arch=ARM64' "$package_dir/BUILD-INFO.txt"; then
  echo "verified package has invalid macOS build identity" >&2
  exit 65
fi

dmgbuild_bin=${DMGBUILD_BIN:-dmgbuild}
if ! command -v "$dmgbuild_bin" >/dev/null 2>&1; then
  echo "dmgbuild is unavailable; install tools/release/dmgbuild-requirements.txt in an isolated environment" >&2
  exit 69
fi
for required_tool in codesign hdiutil shasum; do
  if ! command -v "$required_tool" >/dev/null 2>&1; then
    echo "required macOS tool is unavailable: $required_tool" >&2
    exit 69
  fi
done

codesign --verify --deep --strict "$app_bundle"

temporary_root=${RUNNER_TEMP:-/tmp}
working_dir=$(mktemp -d "$temporary_root/qwc-macos-dmg.XXXXXX")
cleanup() {
  rm -rf "$working_dir"
}
trap cleanup EXIT INT TERM

documentation_dir="$working_dir/Documentation"
mkdir "$documentation_dir"
cp "$package_dir/README.md" "$package_dir/LICENSE" "$package_dir/BUILD-INFO.txt" "$documentation_dir/"
cp -R "$package_dir/share" "$documentation_dir/"

temporary_dmg="$working_dir/$artifact_name.dmg"
"$dmgbuild_bin" \
  -s "$settings_file" \
  -D "app_path=$app_bundle" \
  -D "documentation_path=$documentation_dir" \
  -D "background_path=$background_file" \
  -D "volume_icon_path=$volume_icon" \
  "Qwertycoin Wallet" "$temporary_dmg"

if [[ ! -f "$temporary_dmg" || -L "$temporary_dmg" ]]; then
  echo "dmgbuild did not create the expected regular file" >&2
  exit 70
fi
hdiutil verify "$temporary_dmg"
codesign --verify --deep --strict "$app_bundle"

temporary_checksum="$working_dir/$artifact_name.dmg.sha256"
digest=$(shasum -a 256 "$temporary_dmg" | awk '{print $1}')
if [[ ! "$digest" =~ ^[0-9a-f]{64}$ ]]; then
  echo "unable to derive DMG SHA-256" >&2
  exit 70
fi
printf '%s  %s\n' "$digest" "$artifact_name.dmg" >"$temporary_checksum"

mv "$temporary_dmg" "$output_dmg"
mv "$temporary_checksum" "$output_checksum"
echo "Created $output_dmg"
