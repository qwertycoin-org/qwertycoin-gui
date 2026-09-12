# Qwertycoin GUI porting status

## Reviewed source pair

| Component | Revision | Role |
| --- | --- | --- |
| GUI baseline | `25719fb7ab08af5465b1ce4d7a51208bcf9a1e8b` | Qwertycoin GUI baseline derived from Monero GUI `v0.18.5.2` |
| Qwertycoin Core | `09086f7dbaaf1a4ff16bddeaa1d729f9ff65eca6` | Final QWC v2 Core plus out-of-tree GUI CMake integration fix |
| Core mainnet merge | `e6e0b46b6603bc5c1402df63696b514ba735f8ee` | Final consensus/network baseline contained by the pinned Core revision |

The Core is a Git submodule at `qwertycoin/`, not `monero/`. Its remote is
`https://github.com/qwertycoin-org/qwertycoin.git`. Builds with
`DEV_MODE=OFF` respect the recorded Gitlink and never fetch, check out or
discard another Core revision.

Mainnet binding:

- genesis: `906629482787e94cb00463696a0e95ec75a480da09257c6270c65ba1a74a76b0`;
- EPoSe parameter hash: `e5654b4f5fa27faa51a80ca1e93bb877c3bd3345d0a803b6e7ab55c05189c20d`;
- P2P/RPC/wallet-RPC/ZMQ ports: 8196/8197/8198/8199;
- URI scheme: `qwertycoin:`.

## Implemented

- Qwertycoin light/dark design system using bundled Inter and Archivo fonts,
  approved Q mark/wordmark, platform application icons and local resources.
- Shared controls, wizard, accounts, transfer, receive, history, address book,
  signing, settings, merchant view, dialogs and main navigation migrated to the
  common QML style tokens.
- QWC names, eight-decimal amounts, ports, data paths, daemon executable and
  URI handling retained or corrected without resetting existing wallet files.
- Typed asynchronous EPoSe observer with per-source status/time, bounded
  requests, exact integer transport and stale-response rejection after daemon
  or network changes.
- Dedicated EPoSe view separating observed network state, local producer
  process state, registration, effectiveness, qualification and reward preview.
- Explicit local producer configuration using the current Core flags and a
  public primary QWC reward address. The GUI never asks for wallet private view
  or spend keys.
- Local daemon validation for public restricted-RPC probe endpoints, distinct
  administrative RPC port and network/genesis-bound service keystore path.
- German and English text updated for changed views. Other inherited
  translations remain available but are not represented as fully refreshed.
- Packaging helper now fails when required binaries, fonts, icons, Qt platform
  or SVG plugins, QML imports or `qt.conf` are missing.

## Deliberately unavailable

- Legacy `get_service_node_registration_payload` is observed only as a retired
  compatibility method; registration and renewal remain owned by the local
  Core producer.
- Reward preview is shown as unavailable when the pinned Core returns
  `preview_available=false`; this is not labelled as disabled EPoSe.
- Descriptor update, deregistration, key recovery and envelope submission have
  no GUI buttons because no reviewed administrative backend contract supports
  them at this pin.
- Inherited Monero P2Pool download/launch remains disabled. Solo RandomX mining
  remains separate from EPoSe.
- Hardware-wallet creation, fiat feeds and automatic updates remain disabled
  until QWC-specific compatibility and release infrastructure are reviewed.

## Compatibility names and attribution

Internal `Monero::`, `moneroComponents`, translation catalog names and other
upstream ABI/API identifiers intentionally remain where renaming would add risk
without user benefit. Copyright notices, BSD-3-Clause terms and third-party
licenses are preserved. They are not public Qwertycoin product labels.

## Validation status

Linux x86_64 is the required review platform and is exercised with a clean
dynamic Qt 5.15 build, QML tests, EPoSe adapter tests, offline restore, isolated
create/send regtest and package-content verification. The pinned Core EPoSe
suite is run from the exact Gitlink. See `GUI_REVIEW_EVIDENCE.md` for commands
and results.

No connected native macOS Apple Silicon or Windows x86_64 build machine was
available for this review. Their disabled workflow templates and packaging
paths were updated statically, but neither a cross-build nor Linux screenshots
are reported as native platform evidence. Codesigning, notarization, release
tags and public packages remain later release tasks.

## Workflow state

Templates remain only under `.github/workflows-disabled/`. This work does not
create `.github/workflows/`, enable triggers or start manual GitHub Actions.
All acceptance configurations explicitly set `DEV_MODE=OFF`.
