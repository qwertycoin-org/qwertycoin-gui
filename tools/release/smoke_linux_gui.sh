#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <packaged-linux-directory>" >&2
  exit 2
fi

artifact_dir=$(cd "$1" && pwd -P)
gui="$artifact_dir/qwertycoin-gui"
if [[ ! -x "$gui" ]]; then
  echo "missing executable packaged GUI: $gui" >&2
  exit 1
fi

temporary_root=${RUNNER_TEMP:-/tmp}
smoke_dir=$(mktemp -d "$temporary_root/qwc-gui-smoke.XXXXXX")
runtime_dir="$smoke_dir/runtime"
config_dir="$smoke_dir/config"
data_dir="$smoke_dir/data"
log_file="$smoke_dir/qwertycoin-gui.log"
mkdir "$runtime_dir" "$config_dir" "$data_dir"
chmod 700 "$runtime_dir"

set +e
env \
  XDG_RUNTIME_DIR="$runtime_dir" \
  XDG_CONFIG_HOME="$config_dir" \
  XDG_DATA_HOME="$data_dir" \
  QT_QPA_PLATFORM=offscreen \
  QT_QUICK_BACKEND=software \
  "$gui" --test-qml >"$log_file" 2>&1
smoke_status=$?
set -e

cat "$log_file"
if [[ $smoke_status -ne 0 ]]; then
  echo "packaged GUI smoke exited with status $smoke_status" >&2
  exit 1
fi

if grep -Eiq \
  'TypeError|ReferenceError|module .* is not installed|QQmlApplicationEngine failed|failed to load component|no root objects|cannot load library|could not load the Qt platform plugin|error while loading shared libraries' \
  "$log_file"; then
  echo "packaged GUI smoke reported a QML or runtime-loading error" >&2
  exit 1
fi

echo "Packaged Linux GUI smoke verified"
