# Qwertycoin GUI

Native Qt Quick desktop wallet for Qwertycoin v2.

The application is derived from `monero-project/monero-gui` tag `v0.18.5.2`.
The original BSD-3-Clause license, copyright notices and Monero Project
attribution are retained. User-visible product language, network defaults and
release metadata are Qwertycoin-specific; compatible internal wrapper names
such as `Monero::` and `moneroComponents` intentionally remain unchanged.

## Current source line

The current Qwertycoin GUI source line provides:

- light and dark Qwertycoin design system using locally bundled Inter and
  Archivo fonts plus the approved Q mark and wordmark;
- redesigned shared controls, wizard, wallet pages, dialogs, settings,
  merchant view and native platform icons;
- Qwertycoin Core submodule at the explicit `qwertycoin/` path;
- `qwertycoind` local daemon integration and QWC mainnet defaults;
- typed asynchronous EPoSe observer and a dedicated EPoSe page;
- explicit local EPoSe producer setup through the supported Core flags;
- QWC 8-decimal amounts and `qwertycoin:` URI handling;
- DNSSEC-validated update checks with fixed QWC GitHub release targets;
- unvalidated hardware wallets, inherited P2Pool launching and fiat feeds kept
  fail-closed.

EPoSe service operation is opt-in. RandomX remains responsible for block
production and chain selection.

## Core dependency and network identity

The Core is tracked as a submodule at `qwertycoin/` with remote:

```text
https://github.com/qwertycoin-org/qwertycoin.git
```

This release line pins reviewed Core commit
`24d66aab67c96fb46806818bf26cc3615e8d507a`. It contains the current QWC v2
mainnet genesis and network identity.

| Binding | Value |
| --- | --- |
| Mainnet genesis | `4f95857586e2c66063c277370eda99cd75897d773af09f0c3cd1e22f7e87db39` |
| Mainnet network ID | `515743324d41494e3230323652303102` |
| EPoSe parameter hash | `2c26755094535871dd3ede7bd1b50aba82a9fb6831f0a17f32968eb0385145c6` |
| P2P | 8196 |
| Daemon RPC | 8197 |
| Wallet RPC | 8198 |
| ZMQ | 8199 |
| URI scheme | `qwertycoin:` |

Initialize or migrate an existing checkout with:

```sh
git submodule sync --recursive
git submodule update --init --recursive
tools/check_core_pin.sh
```

Preserve local Core changes before switching branches. Configuration and build
steps never fetch another Core revision or force a submodule checkout.

## Linux review build

The tested review configuration uses Qt 5.15, C++17 and a dynamic Linux build:

```sh
tools/check_core_pin.sh
cmake -S . -B build/gui-review -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DSTATIC=OFF \
  -DMANUAL_SUBMODULES=1 \
  -DDEV_MODE=OFF \
  -DWITH_UPDATER=ON \
  -DBUILD_TAG=linux-x64 \
  -DUSE_DEVICE_TREZOR=OFF \
  -DQML_TESTS=ON
cmake --build build/gui-review \
  --target qwertycoin-gui qwertycoin-gui-epose-tests \
           simplewallet wallet_rpc_server \
  --parallel 2
tools/check_core_pin.sh
```

`DEV_MODE=OFF` is mandatory for review and release candidates. Normal Makefile
targets also respect the recorded Gitlink.

## Update metadata and trust boundary

Desktop release builds periodically query the DNS TXT records at
`updates.qwertycoin.org`. Metadata is accepted only when DNSSEC is available
and validates successfully. A matching record must use exactly:

```text
qwertycoin-gui:<build-tag>:<three-part-version>:<lowercase-sha256>
```

The software name, build tag, version grammar, GitHub repository and release
asset filename are fixed in the application. Conflicting hashes or malformed
matching records fail closed. The downloaded file is exposed to the user only
after its SHA-256 matches the DNSSEC-authenticated metadata; the GUI never
executes an update automatically.

The local daemon launched by the GUI continues to use
`--check-updates disabled`; the GUI owns the package-level notification and
verification flow, so one application does not present competing Core and GUI
update prompts.

Supported build tags are `linux-x64`, `mac-armv8`, `install-win-x64` and
`win-x64`. Flatpak builds keep this updater disabled because updates are owned
by the package manager. Users can opt out in Settings or start the GUI with
`--disable-check-updates`.

The placeholder TXT record `qwc:update-metadata-not-yet-published`, an unsigned
zone or any DNSSEC validation failure means "no update available". Existing
v2.0.1 installations do not contain this QWC updater and therefore require a
manual upgrade to the first release that enables it.

## Local verification

```sh
build/gui-review/bin/qwertycoin-gui-epose-tests -txt
build/gui-review/bin/qwertycoin-gui --test-qml
tools/smoke/restore_from_seed.sh \
  build/gui-review/bin/qwertycoin-wallet-cli
tools/smoke/create_send_regtest.py build/gui-review
```

The create/send smoke starts one offline regtest daemon and two disposable
wallet RPC processes. It creates new test wallets, mines only regtest outputs,
sends 1 QWC and verifies the recipient after confirmation. It never connects
to mainnet and never prints seeds or private keys.

See [GUI review evidence](docs/GUI_REVIEW_EVIDENCE.md), [EPoSe RPC
compatibility](docs/EPOSE_RPC_COMPATIBILITY.md) and [porting
status](docs/PORTING_STATUS.md) for the exact matrix and remaining platform
limits.

## Local EPoSe service

The GUI configures the pinned Core producer with these unchanged technical
flags:

```text
--epose-v2-service
--epose-v2-keystore
--epose-v2-reward-address
--epose-v2-endpoint-host
--epose-v2-endpoint-port
--epose-v2-discovery-endpoint
```

The setup accepts a public primary QWC reward address. It does not request or
store a wallet private view or spend key. The service keystore is separate and
bound by Core to network, genesis and parameters. Remote-node connections are
observation-only and cannot manage a service producer.

## Packaging and workflows

The local packaging helper fails when the GUI, daemon, wallet binaries, bundled
fonts/icons or required Qt platform/SVG/QML runtime modules are absent:

```sh
tools/release/package_artifacts.sh \
  build/gui-review qwertycoin-gui-linux-x86_64-review dist
```

The macOS release job keeps that verified tarball and additionally creates a
compressed, read-only drag-and-drop DMG from the same packaged app bundle. The
DMG presents **Qwertycoin.app** beside an **Applications** link and retains
build metadata and licenses under **Documentation**. No second compile is
performed. For local packaging from an existing verified macOS package, use
`tools/release/create_macos_dmg.sh`; see the
[native release guide](docs/RELEASE_CANDIDATE_BUILDS.md#create-a-dmg-from-an-existing-verified-package).

The Windows job keeps the same verified portable ZIP and additionally creates
an all-users Inno Setup installer from that exact package, without a second
compile. Its stable AppId supports upgrade and same-version repair while an
authenticated program-file manifest limits replacement and obsolete-file
cleanup to installer-owned files. Wallets, settings, blockchain data, EPoSE
keystores and unknown files are retained by updates and uninstall. See the
[Windows installer data boundary](docs/WINDOWS_INSTALLER.md).

One manual release-candidate workflow is available at
`.github/workflows/release.yml`. It accepts only an explicit 40-character GUI
commit SHA and builds one selected platform per invocation; pushes, pull
requests and tags do not start it. The normal build and Flatpak templates remain
disabled. See [manual release-candidate builds](docs/RELEASE_CANDIDATE_BUILDS.md)
for the review sequence and artifact checks. Windows code signing, macOS
Developer ID signing/notarization and publication remain separate release
tasks. The current ad-hoc-signed macOS build may require **System Settings →
Privacy & Security → Open Anyway** after the first launch attempt; the release
process never removes quarantine metadata or disables Gatekeeper.

## Deliberately unavailable

- inherited Monero P2Pool download/launch;
- unvalidated Ledger/Trezor wallet creation;
- fiat conversion without QWC-specific providers;
- automatic QWC release updating;
- write actions that the selected EPoSe Core contract does not expose.

Solo RandomX mining remains available independently of EPoSe.

## License and attribution

Qwertycoin GUI is derived from Monero GUI.

- Original upstream: `https://github.com/monero-project/monero-gui`
- Imported upstream tag: `v0.18.5.2`
- Original upstream README: `README.monero-upstream.md`
- License: BSD-3-Clause, see `LICENSE`

Copyright notices from the Monero Project, CryptoNote notices and third-party
licenses remain intact. Qwertycoin-specific changes are copyright (c) 2026 The
Qwertycoin Project where applicable.
