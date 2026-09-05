# Qwertycoin GUI Porting Status

## Upstream Basis

The initial port uses `monero-project/monero-gui` tag `v0.18.5.2`.

Reasoning:

- It is the newest 0.18.5.x GUI bugfix tag available for the Monero GUI line.
- It remains close to the Qwertycoin v2 core baseline derived from Monero v0.18.5.1.
- Starting from a release tag is safer than importing upstream `master`.

## Repository Layout

- `origin`: official Qwertycoin GUI repository.
- `upstream`: Monero GUI upstream.
- `monero/`: Qwertycoin v2 core submodule, intentionally kept at the original path expected by the upstream GUI build system.

## Completed First-Port Items

- Application name: `Qwertycoin GUI`.
- Binary target: `qwertycoin-gui`.
- Local daemon binary: `qwertycoind`.
- QWC logo and 192px icon included as repository assets.
- Main visible wallet currency text changed from XMR to QWC.
- Mainnet RPC defaults changed to QWC ports.
- `qwertycoin:` URI handling added.
- QWC wallet/config/log/export paths added.
- Update checks disabled until QWC release infrastructure exists.
- Hardware wallet creation disabled in the first port because QWC device support is not validated.
- Trezor compilation is disabled by default for the same reason; enabling it needs a dedicated QWC device compatibility audit.
- Fiat conversion disabled until QWC-specific price providers are configured.
- Core submodule updated to the official Qwertycoin v2 repository at
  `1c4c1bf10c387887a42243dc690a65abb6c6e786`.
- Restore-from-seed smoke script added for built wallet CLI binaries.
- Disabled release-build workflow template retained for Linux x86_64, macOS
  Apple Silicon, and Windows x86_64 pending explicit CI activation.

## Known Porting Gaps

- QML module names and many wrapper classes still use upstream `moneroComponents` / `Monero::` API names. This is expected in the first port and should not be renamed blindly.
- Translation files still contain many upstream strings and need a separate i18n pass.
- Signed/notarized platform packaging needs real Linux/macOS/Windows validation.
- P2Pool support is inherited from upstream and not release-validated for QWC.
- The inherited Monero P2Pool download and launch path is disabled fail-closed;
  solo RandomX mining remains available.
- Hardware wallets need a dedicated compatibility audit.
- Remote-node defaults are not finalized.

## Required Smoke Tests

- Build `qwertycoin-gui` on Linux x64.
- Build on macOS ARM64.
- Create a new QWC mainnet wallet and verify the address starts with `QWC`.
- Restore the same wallet from seed and verify the same address with
  `tools/smoke/restore_from_seed.sh`.
- Connect to a QWC daemon on RPC port `8197`.
- Send QWC from wallet A to wallet B after daemon connectivity is validated.
