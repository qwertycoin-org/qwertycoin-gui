# Qwertycoin GUI

Native Qt Quick desktop wallet for Qwertycoin v2.

The application is derived from `monero-project/monero-gui` tag `v0.18.5.2`.
The original BSD-3-Clause license, copyright notices and Monero Project
attribution are retained. User-visible product language, network defaults and
release metadata are Qwertycoin-specific; compatible internal wrapper names
such as `Monero::` and `moneroComponents` intentionally remain unchanged.

## Current branch status

This branch is a locally tested GUI modernization, not a signed release:

- light and dark Qwertycoin design system using locally bundled Inter and
  Archivo fonts plus the approved Q mark and wordmark;
- redesigned shared controls, wizard, wallet pages, dialogs, settings,
  merchant view and native platform icons;
- Qwertycoin Core submodule at the explicit `qwertycoin/` path;
- `qwertycoind` local daemon integration and QWC mainnet defaults;
- typed asynchronous EPoSe observer and a dedicated EPoSe page;
- explicit local EPoSe producer setup through the supported Core flags;
- QWC 8-decimal amounts and `qwertycoin:` URI handling;
- update checks, unvalidated hardware wallets, inherited P2Pool launching and
  fiat feeds kept fail-closed.

EPoSe service operation is opt-in. RandomX remains responsible for block
production and chain selection.

## Core dependency and network identity

The Core is tracked as a submodule at `qwertycoin/` with remote:

```text
https://github.com/qwertycoin-org/qwertycoin.git
```

This branch pins reviewed Core commit
`09086f7dbaaf1a4ff16bddeaa1d729f9ff65eca6`. It contains the final QWC v2
mainnet state from `e6e0b46b6603bc5c1402df63696b514ba735f8ee` plus the
out-of-tree GUI CMake integration correction tracked by Core PR #185.

| Binding | Value |
| --- | --- |
| Mainnet genesis | `906629482787e94cb00463696a0e95ec75a480da09257c6270c65ba1a74a76b0` |
| EPoSe parameter hash | `e5654b4f5fa27faa51a80ca1e93bb877c3bd3345d0a803b6e7ab55c05189c20d` |
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
  -DWITH_UPDATER=OFF \
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

Workflow templates remain under `.github/workflows-disabled/`; they are review
material and cannot run as GitHub Actions. They must not be moved into
`.github/workflows/` until native platform validation and explicit release
approval. Local artifacts are unsigned review builds. Windows code signing,
macOS signing/notarization and publication remain separate release tasks.

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
