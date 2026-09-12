# Qwertycoin GUI deployment notes

These instructions describe local candidate packaging. They do not authorize a
release, tag, signature, notarization or publication.

## Common requirements

1. Use a clean recursive checkout of the reviewed GUI revision.
2. Run `git submodule sync --recursive` and
   `git submodule update --init --recursive`.
3. Run `tools/check_core_pin.sh` before and after the build.
4. Configure every candidate with `MANUAL_SUBMODULES=1`, `DEV_MODE=OFF`,
   `WITH_UPDATER=OFF` and unvalidated device support disabled.
5. Build `qwertycoin-gui`, `qwertycoind`, `qwertycoin-wallet-cli` and
   `qwertycoin-wallet-rpc` from the same checkout.
6. Verify the package contains the Qt platform/SVG plugins, required QML import
   tree, bundled fonts/licenses, icons, README, license and `qt.conf`.

The exact Core Gitlink, genesis and EPoSe parameter binding are documented in
`README.md`. Do not substitute a daemon from another revision.

## Linux x86_64 review package

```sh
cmake -S . -B build/release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DSTATIC=OFF \
  -DMANUAL_SUBMODULES=1 \
  -DDEV_MODE=OFF \
  -DWITH_UPDATER=OFF \
  -DUSE_DEVICE_TREZOR=OFF \
  -DQML_TESTS=OFF
cmake --build build/release \
  --target qwertycoin-gui simplewallet wallet_rpc_server --parallel 2
tools/release/package_artifacts.sh \
  build/release qwertycoin-gui-linux-x86_64-review dist
```

Run the package from an empty environment and verify the manifest before it is
considered a candidate. Local review archives are unsigned.

## macOS Apple Silicon

Use a supported Qt 5.15 build containing the required Qt Quick, SVG and image
plugins. Configure with `ARCH=armv8-a` and `BUILD_64=ON`, build the four required
binaries, then run the CMake `deploy` target before the package-content check.

Native launch, Retina scaling, file dialogs, menu/window controls, icon, daemon
startup and wallet/EPoSe smokes are mandatory. Only after those gates may a
separate release procedure perform hardened-runtime signing, notarization and
stapling. An ad-hoc signature is test evidence only.

## Windows x86_64

The disabled release template documents the MinGW Qt 5 build path. Run the
CMake `deploy` target, verify all runtime DLLs/plugins/QML modules, and stage the
result under `installers/windows/bin/` for `Qwertycoin.iss`.

Native Windows launch, 100/125/150/200% DPI, file dialogs, taskbar/tray icons,
daemon startup and wallet/EPoSe smokes are mandatory before signing. The
installer template is not a release artifact on its own.

## Current validation boundary

Linux x86_64 has local build, functional, visual and package evidence in
`docs/GUI_REVIEW_EVIDENCE.md`. Native macOS Apple Silicon and Windows x86_64
were not available during that review and remain explicit release blockers.

Workflow templates stay in `.github/workflows-disabled/`. Do not move them or
start GitHub-hosted jobs without a separate budget and release authorization.
