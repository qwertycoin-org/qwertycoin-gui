#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: $0 <build-dir> <artifact-name> <output-dir>" >&2
  exit 64
fi

build_dir=$1
artifact_name=$2
output_dir=$3

mkdir -p "$output_dir/$artifact_name"

copy_if_found() {
  local name=$1
  local found
  found=$(find "$build_dir" -type f -perm -111 \( -name "$name" -o -name "$name.exe" \) | head -n 1 || true)
  if [[ -n "$found" ]]; then
    cp "$found" "$output_dir/$artifact_name/"
  fi
}

copy_bundle_if_found() {
  local found
  found=$(find "$build_dir" -type d \( -name "qwertycoin-gui.app" -o -name "Qwertycoin GUI.app" \) | head -n 1 || true)
  if [[ -n "$found" ]]; then
    cp -R "$found" "$output_dir/$artifact_name/"
  fi
}

copy_if_found qwertycoin-gui
copy_if_found qwertycoind
copy_if_found qwertycoin-wallet-cli
copy_if_found qwertycoin-wallet-rpc
copy_bundle_if_found

cp README.md "$output_dir/$artifact_name/" 2>/dev/null || true
cp LICENSE "$output_dir/$artifact_name/" 2>/dev/null || true

if [[ -z "$(find "$output_dir/$artifact_name" -mindepth 1 -maxdepth 1 -print -quit)" ]]; then
  echo "no release files found below $build_dir" >&2
  exit 1
fi

(
  cd "$output_dir"
  if command -v sha256sum >/dev/null 2>&1; then
    find "$artifact_name" -maxdepth 4 -type f -print0 | sort -z | xargs -0 sha256sum >"$artifact_name.sha256"
  else
    find "$artifact_name" -maxdepth 4 -type f -print0 | sort -z | xargs -0 shasum -a 256 >"$artifact_name.sha256"
  fi

  if [[ "${RUNNER_OS:-}" == "Windows" ]] && command -v zip >/dev/null 2>&1; then
    zip -qr "$artifact_name.zip" "$artifact_name" "$artifact_name.sha256"
  else
    tar -czf "$artifact_name.tar.gz" "$artifact_name" "$artifact_name.sha256"
  fi
)
