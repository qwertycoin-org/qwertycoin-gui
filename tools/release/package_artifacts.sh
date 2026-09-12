#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "usage: $0 <build-dir> <artifact-name> <output-dir>" >&2
  exit 64
fi

build_dir=$1
artifact_name=$2
output_dir=$3

if [[ ! "$artifact_name" =~ ^[A-Za-z0-9][A-Za-z0-9._-]{0,127}$ ]]; then
  echo "artifact name contains unsupported characters" >&2
  exit 64
fi

if [[ ! -d "$build_dir" ]]; then
  echo "build directory does not exist: $build_dir" >&2
  exit 1
fi

source_revision=$(git rev-parse HEAD)
if [[ -n "${EXPECTED_REVISION:-}" && "$source_revision" != "$EXPECTED_REVISION" ]]; then
  echo "source revision does not match the requested release revision" >&2
  exit 1
fi

if [[ -e "$output_dir/$artifact_name" || -e "$output_dir/$artifact_name.sha256" \
      || -e "$output_dir/$artifact_name.tar.gz" || -e "$output_dir/$artifact_name.zip" ]]; then
  echo "refusing to reuse an existing artifact path: $output_dir/$artifact_name" >&2
  exit 1
fi

mkdir -p "$output_dir/$artifact_name"

artifact_dir="$output_dir/$artifact_name"
gui_binary=""
platform=$(uname -s)
linux_glibc_ceiling=${QWC_LINUX_GLIBC_CEILING:-2.35}

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

if [[ "$platform" == "Darwin" ]]; then
  # The macOS deploy target embeds the daemon and wallet command-line tools in
  # the signed application bundle. Do not also ship unbundled Homebrew-linked
  # copies beside it.
  copy_bundle_if_found
else
  copy_if_found qwertycoin-gui
  copy_if_found qwertycoind
  copy_if_found qwertycoin-wallet-cli
  copy_if_found qwertycoin-wallet-rpc
fi

cp README.md "$output_dir/$artifact_name/" 2>/dev/null || true
cp LICENSE "$output_dir/$artifact_name/" 2>/dev/null || true

# Keep the exact bundled brand/font sources and their licenses reviewable even
# though Qt embeds them into the GUI resource collection at build time.
mkdir -p "$artifact_dir/share/qwertycoin-gui"
cp -R fonts/Archivo fonts/Inter images/appicons images/brand \
  "$artifact_dir/share/qwertycoin-gui/"

if [[ "$platform" == "Linux" && -n "$gui_binary" ]]; then
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
    [[ -n "${qt_lib_dir:-}" ]] || qt_lib_dir=$(qmake -query QT_INSTALL_LIBS)
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

  QWC_RUNTIME_LIBRARY_PATH="$qt_lib_dir${QWC_RUNTIME_LIBRARY_PATH:+:$QWC_RUNTIME_LIBRARY_PATH}" \
    "$(dirname "$0")/bundle_linux_runtime.sh" "$artifact_dir"
  "$(dirname "$0")/verify_linux_abi.sh" "$artifact_dir" "$linux_glibc_ceiling"
fi

# Deployment tools place runtime directories next to the GUI executable.
# Preserve the complete windeployqt application directory on Windows: Qt 5
# distributions use both plugin roots (for example platforms/) and QML import
# roots (for example QtQuick/ and Qt/), and the exact set follows the imports
# discovered for this build.
if [[ -n "$gui_binary" ]]; then
  gui_binary_dir=$(dirname "$gui_binary")
  if [[ "${RUNNER_OS:-}" == "Windows" ]]; then
    while IFS= read -r -d '' runtime_dir; do
      runtime_name=$(basename "$runtime_dir")
      if [[ ! -e "$artifact_dir/$runtime_name" ]]; then
        cp -R "$runtime_dir" "$artifact_dir/"
      fi
    done < <(find "$gui_binary_dir" -mindepth 1 -maxdepth 1 -type d -print0)

    while IFS= read -r -d '' runtime_dll; do
      cp "$runtime_dll" "$artifact_dir/"
    done < <(find "$gui_binary_dir" -maxdepth 1 -type f -iname '*.dll' -print0)
  else
    for runtime_name in platforms imageformats qml; do
      if [[ -d "$gui_binary_dir/$runtime_name" && ! -e "$artifact_dir/$runtime_name" ]]; then
        cp -R "$gui_binary_dir/$runtime_name" "$artifact_dir/"
      fi
    done
  fi
fi

core_revision=$(git rev-parse HEAD:qwertycoin)
qt_version=unknown
if command -v qmake >/dev/null 2>&1; then
  qt_version=$(qmake -query QT_VERSION)
elif command -v qmake-qt5 >/dev/null 2>&1; then
  qt_version=$(qmake-qt5 -query QT_VERSION)
fi
printf 'source_revision=%s\ncore_revision=%s\nrunner_os=%s\nrunner_arch=%s\nqt_version=%s\n' \
  "$source_revision" "$core_revision" "${RUNNER_OS:-$platform}" "${RUNNER_ARCH:-unknown}" "$qt_version" \
  >"$artifact_dir/BUILD-INFO.txt"
if [[ "$platform" == "Linux" ]]; then
  printf 'glibc_ceiling=%s\n' "$linux_glibc_ceiling" >>"$artifact_dir/BUILD-INFO.txt"
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
if [[ "$platform" != "Darwin" ]]; then
  require_file "qwertycoind" qwertycoind qwertycoind.exe
  require_file "wallet CLI" qwertycoin-wallet-cli qwertycoin-wallet-cli.exe
  require_file "wallet RPC" qwertycoin-wallet-rpc qwertycoin-wallet-rpc.exe
fi
require_file "Archivo display font" share/qwertycoin-gui/Archivo/Archivo-Black.otf
require_file "Archivo license" share/qwertycoin-gui/Archivo/OFL.txt
require_file "Inter regular font" share/qwertycoin-gui/Inter/Inter-Regular.otf
require_file "Inter license" share/qwertycoin-gui/Inter/LICENSE.txt
require_file "Qwertycoin application icon" share/qwertycoin-gui/appicons/256x256.png
require_file "Qwertycoin brand mark" share/qwertycoin-gui/brand/qwertycoin-mark.svg

if [[ "$platform" == "Linux" && -n "$gui_binary" ]]; then
  require_file "Qt xcb platform plugin" plugins/platforms/libqxcb.so
  require_file "Qt SVG image plugin" plugins/imageformats/libqsvg.so
  require_file "Qt Core shared library" lib/libQt5Core.so.5
  require_file "Qt GUI shared library" lib/libQt5Gui.so.5
  require_file "Qt QML shared library" lib/libQt5Qml.so.5
  require_file "Qt Quick shared library" lib/libQt5Quick.so.5
  require_file "Qt Quick Controls 2 QML module" qml/QtQuick/Controls.2/qmldir
  require_file "Qt Quick Layouts QML module" qml/QtQuick/Layouts/qmldir
  require_file "Qt Graphical Effects QML module" qml/QtGraphicalEffects/qmldir
  require_file "Qt Labs Platform QML module" qml/Qt/labs/platform/qmldir
  require_file "Qt runtime path configuration" qt.conf
fi

if [[ "${RUNNER_OS:-}" == "Windows" ]]; then
  require_file "Qt Core DLL" Qt5Core.dll
  require_file "Qt GUI DLL" Qt5Gui.dll
  require_file "Qt QML DLL" Qt5Qml.dll
  require_file "Qt Quick DLL" Qt5Quick.dll
  require_file "ANGLE EGL runtime" libEGL.dll
  require_file "ANGLE OpenGL ES runtime" libGLESv2.dll
  require_file "Unbound runtime" libunbound-8.dll
  require_file "LDNS runtime" libldns-3.dll
  require_file "Expat runtime" libexpat-1.dll
  require_file "PCRE2 8-bit runtime" libpcre2-8-0.dll
  require_file "MD4C runtime" libmd4c.dll
  require_file "Qt Windows platform plugin" platforms/qwindows.dll
  require_file "Qt SVG image plugin" imageformats/qsvg.dll
  # windeployqt places QML imports beside the executable. Some Qt 5
  # distributions preserve a qml/ prefix, while MSYS2's deployment tool
  # writes the standard import roots directly into the application folder.
  require_file "Qt Quick Controls 2 QML module" QtQuick/Controls.2/qmldir qml/QtQuick/Controls.2/qmldir
  require_file "Qt Quick Layouts QML module" QtQuick/Layouts/qmldir qml/QtQuick/Layouts/qmldir
  require_file "Qt Labs Platform QML module" Qt/labs/platform/qmldir qml/Qt/labs/platform/qmldir
  "$(dirname "$0")/verify_windows_runtime.sh" "$artifact_dir"
fi

if [[ "$platform" == "Darwin" ]]; then
  mac_bundle=$(find "$artifact_dir" -type d \( -name 'qwertycoin-gui.app' -o -name 'Qwertycoin GUI.app' \) | head -n 1 || true)
  if [[ -z "$mac_bundle" ]]; then
    echo "missing required macOS application bundle" >&2
    exit 1
  fi
  mac_bundle_relative=${mac_bundle#"$artifact_dir/"}
  require_file "macOS Qt Core framework" "$mac_bundle_relative/Contents/Frameworks/QtCore.framework"
  require_file "macOS Qt Quick framework" "$mac_bundle_relative/Contents/Frameworks/QtQuick.framework"
  require_file "macOS Cocoa platform plugin" "$mac_bundle_relative/Contents/PlugIns/platforms/libqcocoa.dylib"
  require_file "macOS SVG image plugin" "$mac_bundle_relative/Contents/PlugIns/imageformats/libqsvg.dylib"
  require_file "macOS Qt Quick Controls 2 QML module" "$mac_bundle_relative/Contents/Resources/qml/QtQuick/Controls.2/qmldir"
  require_file "macOS Qt Quick Layouts QML module" "$mac_bundle_relative/Contents/Resources/qml/QtQuick/Layouts/qmldir"
  require_file "macOS Qt Labs Platform QML module" "$mac_bundle_relative/Contents/Resources/qml/Qt/labs/platform/qmldir"
  require_file "embedded macOS qwertycoind" "$mac_bundle_relative/Contents/MacOS/qwertycoind"
  require_file "embedded macOS wallet CLI" "$mac_bundle_relative/Contents/MacOS/qwertycoin-wallet-cli"
  require_file "embedded macOS wallet RPC" "$mac_bundle_relative/Contents/MacOS/qwertycoin-wallet-rpc"

  codesign --verify --deep --strict "$mac_bundle"
  mac_mach_count=0
  while IFS= read -r -d '' mach_file; do
    [[ "$(file -Lb "$mach_file")" == Mach-O* ]] || continue
    mac_mach_count=$((mac_mach_count + 1))
    if ! lipo -archs "$mach_file" | tr ' ' '\n' | grep -qx arm64; then
      echo "macOS bundle contains a non-arm64 Mach-O file: $mach_file" >&2
      exit 1
    fi
    if otool -L "$mach_file" | tail -n +2 | grep -E '/opt/homebrew|/usr/local|/Users/runner|/opt/hostedtoolcache'; then
      echo "macOS bundle contains a non-portable dependency: $mach_file" >&2
      exit 1
    fi
    if otool -l "$mach_file" |
      awk '
        $1 == "cmd" && $2 == "LC_RPATH" { want_path = 1; next }
        want_path && $1 == "path" {
          want_path = 0
          if ($2 ~ "^/opt/homebrew/" ||
              $2 ~ "^/usr/local/" ||
              $2 ~ "^/Users/runner/" ||
              $2 ~ "^/opt/hostedtoolcache/") {
            found = 1
          }
        }
        END { exit found ? 0 : 1 }
      '; then
      echo "macOS bundle contains a non-portable runtime search path: $mach_file" >&2
      exit 1
    fi
  done < <(find "$mac_bundle" -type f -print0)
  if (( mac_mach_count == 0 )); then
    echo "macOS bundle contains no Mach-O files" >&2
    exit 1
  fi
  printf 'macOS runtime verified: %d arm64 Mach-O files, no runner-local dependencies\n' "$mac_mach_count"
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
