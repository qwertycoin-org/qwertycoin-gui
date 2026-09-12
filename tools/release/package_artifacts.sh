#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: $0 <build-dir> <artifact-name> <output-dir>" >&2
  exit 64
fi

build_dir=$1
artifact_name=$2
output_dir=$3

if [[ -e "$output_dir/$artifact_name" || -e "$output_dir/$artifact_name.sha256" \
      || -e "$output_dir/$artifact_name.tar.gz" || -e "$output_dir/$artifact_name.zip" ]]; then
  echo "refusing to reuse an existing artifact path: $output_dir/$artifact_name" >&2
  exit 1
fi

mkdir -p "$output_dir/$artifact_name"

artifact_dir="$output_dir/$artifact_name"
gui_binary=""

copy_if_found() {
  local name=$1
  local found
  found=$(find "$build_dir" -type f -perm -111 \( -name "$name" -o -name "$name.exe" \) | head -n 1 || true)
  if [[ -n "$found" ]]; then
    cp "$found" "$artifact_dir/"
    if [[ "$name" == "qwertycoin-gui" ]]; then
      gui_binary=$found
    fi
  fi
}

copy_bundle_if_found() {
  local found
  found=$(find "$build_dir" -type d \( -name "qwertycoin-gui.app" -o -name "Qwertycoin GUI.app" \) | head -n 1 || true)
  if [[ -n "$found" ]]; then
    cp -R "$found" "$artifact_dir/"
  fi
}

copy_if_found qwertycoin-gui
copy_if_found qwertycoind
copy_if_found qwertycoin-wallet-cli
copy_if_found qwertycoin-wallet-rpc
copy_bundle_if_found

cp README.md "$output_dir/$artifact_name/" 2>/dev/null || true
cp LICENSE "$output_dir/$artifact_name/" 2>/dev/null || true

# Keep the exact bundled brand/font sources and their licenses reviewable even
# though Qt embeds them into the GUI resource collection at build time.
mkdir -p "$artifact_dir/share/qwertycoin-gui"
cp -R fonts/Archivo fonts/Inter images/appicons images/brand \
  "$artifact_dir/share/qwertycoin-gui/"

if [[ "$(uname -s)" == "Linux" && -n "$gui_binary" ]]; then
  qt_plugin_dir=${QWC_QT_PLUGIN_DIR:-}
  qt_qml_dir=${QWC_QT_QML_DIR:-}

  # Prefer the Qt installation that configured this exact build tree. This is
  # important for review prefixes and extracted SDKs where the qmake found in
  # PATH may describe a different, incomplete system Qt installation.
  qt_core_cmake_dir=""
  if [[ -f "$build_dir/CMakeCache.txt" ]]; then
    qt_core_cmake_dir=$(sed -n 's/^Qt5Core_DIR:PATH=//p' "$build_dir/CMakeCache.txt" | head -n 1)
  fi
  if [[ "$qt_core_cmake_dir" == */cmake/Qt5Core ]]; then
    qt_lib_dir=${qt_core_cmake_dir%/cmake/Qt5Core}
    if [[ -z "$qt_plugin_dir" && -d "$qt_lib_dir/qt5/plugins" ]]; then
      qt_plugin_dir="$qt_lib_dir/qt5/plugins"
    fi
    if [[ -z "$qt_qml_dir" && -d "$qt_lib_dir/qt5/qml" ]]; then
      qt_qml_dir="$qt_lib_dir/qt5/qml"
    fi
  fi

  if [[ -z "$qt_plugin_dir" || -z "$qt_qml_dir" ]]; then
    if ! command -v qmake >/dev/null 2>&1; then
      echo "unable to locate Qt runtime paths from CMakeCache.txt or qmake" >&2
      exit 1
    fi
    [[ -n "$qt_plugin_dir" ]] || qt_plugin_dir=$(qmake -query QT_INSTALL_PLUGINS)
    [[ -n "$qt_qml_dir" ]] || qt_qml_dir=$(qmake -query QT_INSTALL_QML)
  fi
  for runtime_dir in platforms imageformats xcbglintegrations platformthemes; do
    if [[ -d "$qt_plugin_dir/$runtime_dir" ]]; then
      mkdir -p "$artifact_dir/plugins/$runtime_dir"
      cp -R "$qt_plugin_dir/$runtime_dir/." "$artifact_dir/plugins/$runtime_dir/"
    fi
  done
  mkdir -p "$artifact_dir/qml"
  cp -R "$qt_qml_dir/." "$artifact_dir/qml/"
  printf '[Paths]\nPlugins = plugins\nQml2Imports = qml\n' >"$artifact_dir/qt.conf"
fi

# windeployqt places these directories next to the GUI executable. Preserve
# them when packaging a native Windows build.
if [[ -n "$gui_binary" ]]; then
  gui_binary_dir=$(dirname "$gui_binary")
  for runtime_dir in platforms imageformats qml; do
    if [[ -d "$gui_binary_dir/$runtime_dir" && ! -e "$artifact_dir/$runtime_dir" ]]; then
      cp -R "$gui_binary_dir/$runtime_dir" "$artifact_dir/"
    fi
  done
fi

require_file() {
  local description=$1
  shift
  local candidate
  for candidate in "$@"; do
    if [[ -e "$output_dir/$artifact_name/$candidate" ]]; then
      return 0
    fi
  done
  echo "missing required release file: $description" >&2
  exit 1
}

require_file "GUI binary or app bundle" qwertycoin-gui qwertycoin-gui.exe qwertycoin-gui.app "Qwertycoin GUI.app"
require_file "qwertycoind" qwertycoind qwertycoind.exe
require_file "wallet CLI" qwertycoin-wallet-cli qwertycoin-wallet-cli.exe
require_file "wallet RPC" qwertycoin-wallet-rpc qwertycoin-wallet-rpc.exe
require_file "Archivo display font" share/qwertycoin-gui/Archivo/Archivo-Black.otf
require_file "Archivo license" share/qwertycoin-gui/Archivo/OFL.txt
require_file "Inter regular font" share/qwertycoin-gui/Inter/Inter-Regular.otf
require_file "Inter license" share/qwertycoin-gui/Inter/LICENSE.txt
require_file "Qwertycoin application icon" share/qwertycoin-gui/appicons/256x256.png
require_file "Qwertycoin brand mark" share/qwertycoin-gui/brand/qwertycoin-mark.svg

if [[ "$(uname -s)" == "Linux" && -n "$gui_binary" ]]; then
  require_file "Qt xcb platform plugin" plugins/platforms/libqxcb.so
  require_file "Qt SVG image plugin" plugins/imageformats/libqsvg.so
  require_file "Qt Quick Controls 2 QML module" qml/QtQuick/Controls.2/qmldir
  require_file "Qt Quick Layouts QML module" qml/QtQuick/Layouts/qmldir
  require_file "Qt Graphical Effects QML module" qml/QtGraphicalEffects/qmldir
  require_file "Qt runtime path configuration" qt.conf
fi

(
  cd "$output_dir"
  if command -v sha256sum >/dev/null 2>&1; then
    find "$artifact_name" -type f -print0 | sort -z | xargs -0 sha256sum >"$artifact_name.sha256"
  else
    find "$artifact_name" -type f -print0 | sort -z | xargs -0 shasum -a 256 >"$artifact_name.sha256"
  fi

  if [[ "${RUNNER_OS:-}" == "Windows" ]] && command -v zip >/dev/null 2>&1; then
    zip -qr "$artifact_name.zip" "$artifact_name" "$artifact_name.sha256"
  else
    tar -czf "$artifact_name.tar.gz" "$artifact_name" "$artifact_name.sha256"
  fi
)
