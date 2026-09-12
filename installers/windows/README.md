# Qwertycoin GUI Windows Installer

Copyright (c) 2017-2024, The Monero Project
Copyright (c) 2026, The Qwertycoin Project

This directory contains the Inno Setup definition for a future Qwertycoin GUI
Windows package. The original BSD-3-Clause license and Monero Project
attribution are retained in `LICENSE`.

## Status

The installer is a packaging template. It is not a signed Qwertycoin release
until the exact Windows x86_64 build has completed the documented native test,
code-signing and release review gates.

## Building locally

1. Produce the reviewed Qwertycoin Windows artifact set.
2. Copy its complete contents into `installers/windows/bin/`.
3. Confirm that at least these files exist:
   - `qwertycoin-gui.exe`
   - `qwertycoind.exe`
   - `qwertycoin-wallet-cli.exe`
   - `qwertycoin-wallet-rpc.exe`
   - all required Qt/QML plugins and runtime libraries
4. Open `Qwertycoin.iss` in the agreed Inno Setup version and compile it.
5. Verify the resulting installer on a disposable Windows machine before any
   signing or publication.

The template registers only the `qwertycoin:` payment URI. It does not enable
the updater, hardware-wallet support, P2Pool or any other unvalidated feature.

See [Deterministic.md](Deterministic.md) for reproducibility constraints.
