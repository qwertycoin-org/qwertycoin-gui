# Qwertycoin GUI

Desktop wallet for Qwertycoin v2.

This repository is an initial Qwertycoin port of the Monero GUI. The upstream import is based on `monero-project/monero-gui` tag `v0.18.5.2`; the original BSD-3-Clause license and Monero Project attribution are retained.

## Current Status

This is a first functional porting branch, not a signed release build.

Implemented in this branch:

- Qwertycoin GUI application metadata and visible branding.
- QWC logo and icon assets.
- Qwertycoin v2 core submodule wired at the existing `monero/` path expected by the GUI build system.
- `qwertycoind` local daemon integration.
- QWC mainnet defaults: daemon RPC `8197`, wallet RPC `8198`, P2P `8196`, ZMQ `8199`.
- `qwertycoin:` URI handling.
- QWC balance labels and 8-decimal amount entry on the main wallet flows.
- Qwertycoin wallet/config/log/export paths.
- Monero update checks disabled until QWC-specific release infrastructure exists.
- Hardware wallet creation disabled in the first port because QWC Ledger/Trezor support has not been validated.
- Restore-from-seed smoke automation for built `qwertycoin-wallet-cli` binaries.
- Manual release-build workflow for Linux x86_64, macOS Apple Silicon, and Windows x86_64 artifacts.

Still under porting review:

- Signed/notarized platform packaging.
- P2Pool integration for Qwertycoin.
- Hardware wallet support.
- QWC-specific remote node defaults.
- Complete translation refresh.
- Full create/restore/send smoke matrix on every target platform.

The inherited Monero P2Pool download and launch path is disabled until a
Qwertycoin-compatible implementation has passed a dedicated security and
protocol compatibility review. Solo RandomX mining remains available.

## Core Dependency

The Qwertycoin core is tracked as a submodule at `monero/` for compatibility with the upstream GUI build scripts.

```
git submodule update --init --recursive
```

The submodule remote points to the official Qwertycoin core repository:

```
https://github.com/qwertycoin-org/qwertycoin.git
```

This branch pins the submodule to reviewed Qwertycoin v2 core commit
`1c4c1bf10c387887a42243dc690a65abb6c6e786`.

## Network Defaults

| Setting | Value |
| --- | --- |
| Network | Qwertycoin v2 mainnet |
| Ticker | QWC |
| Decimals | 8 |
| Block target | 120 seconds |
| P2P | 8196 |
| Daemon RPC | 8197 |
| Wallet RPC | 8198 |
| ZMQ | 8199 |
| URI scheme | `qwertycoin:` |

## Build

The upstream Monero GUI build system is still used. For a local development smoke, install the normal Monero GUI build dependencies plus the Qwertycoin v2 core dependencies, then configure with the updater disabled:

```
cmake -S . -B build -DMANUAL_SUBMODULES=1 -DWITH_UPDATER=OFF
cmake --build build --target qwertycoin-gui
```

The produced GUI expects Qwertycoin v2 wallet/core APIs from the submodule.

## Smoke Tests

After building `qwertycoin-wallet-cli`, verify deterministic wallet restore:

```
tools/smoke/restore_from_seed.sh build/bin/qwertycoin-wallet-cli
```

The script creates an offline wallet, extracts its 25-word seed, restores a
second wallet from that seed, and fails unless both primary QWC addresses match.

## Release Builds

Release workflow templates are retained under `.github/workflows-disabled/`
for review. They do not run until CI and release publication are explicitly
enabled after platform validation.

- the manual template builds development artifacts;
- the tag template builds release-candidate artifacts;
- Linux x86_64, Windows x86_64, and native macOS Apple Silicon artifacts are
  uploaded with SHA-256 manifests.

These artifacts are unsigned. macOS notarization and Windows code signing remain
separate release-hardening tasks.

## License and Attribution

Qwertycoin GUI is derived from Monero GUI.

- Original upstream: `https://github.com/monero-project/monero-gui`
- Imported upstream tag: `v0.18.5.2`
- Original upstream README retained as `README.monero-upstream.md`
- License: BSD-3-Clause, see `LICENSE`

Copyright notices from the Monero Project, CryptoNote notices, and third-party licenses must remain intact. Qwertycoin-specific changes are copyright (c) 2026 The Qwertycoin Project where applicable.
