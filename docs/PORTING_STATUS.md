# Qwertycoin GUI porting status

## Reviewed source pair

| Component | Revision | Role |
| --- | --- | --- |
| GUI baseline | `25719fb7ab08af5465b1ce4d7a51208bcf9a1e8b` | Qwertycoin GUI baseline derived from Monero GUI `v0.18.5.2` |
| Qwertycoin Core | `1e7de336faeff8f1dc6604fe75d038dc3568d7fa` | Reset-1 QWC v2 Core and release workflow line |
| Core Reset-1 merge | `9953ea40a92a71fbfccc57ffe38e868ddfa9e00a` | Current consensus/network baseline contained by the pinned Core revision |

The Core is a Git submodule at `qwertycoin/`, not `monero/`. Its remote is
`https://github.com/qwertycoin-org/qwertycoin.git`. Builds with
`DEV_MODE=OFF` respect the recorded Gitlink and never fetch, check out or
discard another Core revision.

Mainnet binding:

- genesis: `4f95857586e2c66063c277370eda99cd75897d773af09f0c3cd1e22f7e87db39`;
- network ID: `515743324d41494e3230323652303102`;
- EPoSe parameter hash: `2c26755094535871dd3ede7bd1b50aba82a9fb6831f0a17f32968eb0385145c6`;
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
  process state, registration, effectiveness and qualification.
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
- The pinned Core's unavailable reward-preview result is not presented as a
  warning or observation source. The GUI does not guess a future payee from
  independently observed service-node data.
- Descriptor update, deregistration, key recovery and envelope submission have
  no GUI buttons because no reviewed administrative backend contract supports
  them at this pin.
- Inherited Monero P2Pool download/launch remains disabled. Solo RandomX mining
  remains separate from EPoSe.
- Hardware-wallet creation and fiat feeds remain disabled until QWC-specific
  compatibility is reviewed. Desktop update checks are enabled only for the
  fixed QWC GitHub assets authenticated by DNSSEC TXT metadata; Flatpak updates
  remain package-manager-owned.

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
available for the GUI integration review. A manual-only release-candidate
workflow now provides native hosted builds for those targets, but a workflow
completion is not recorded as a platform pass until its downloaded artifact is
inspected and launched. Codesigning, notarization, release tags and public
packages remain later release tasks.

## Workflow state

Only `.github/workflows/release.yml` is active, with a manual
`workflow_dispatch` trigger and one selected platform per invocation. Build and
Flatpak templates remain under `.github/workflows-disabled/`. The release
candidate workflow requires an exact GUI SHA, verifies the committed Core pin
before and after compilation, and sets `DEV_MODE=OFF`. It creates temporary
review artifacts only; it has no push, pull-request, tag, release or publication
trigger.
