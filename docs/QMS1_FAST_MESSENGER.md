# QMS1/Fast Messenger

Status: experimental desktop application feature.

The Linux, Windows and macOS GUI use Qwertycoin Core's QMS1/Fast profile at the
pinned Core gitlink. The wire format is the same version/profile `1/1` used by
the production Web Wallet: signed 214-byte invitations, authenticated sealed
messages, subtype `0x72` inside existing `TX_EXTRA_NONCE` fields, 600-byte
fragments and the unchanged 1,060-byte `tx_extra` relay limit.

## Security boundary

- Messenger appears only for an unlocked full wallet with a non-empty session
  password. Watch-only, hardware, light and multisig wallets are rejected by
  the Core transaction builder.
- Import and identity verification are separate. Sending and incoming plaintext
  display remain disabled until the complete 64-hex-character contact
  fingerprint is compared over an independently authenticated channel and
  confirmed locally.
- Identity keys, invitations, contact secrets, incomplete/unknown ciphertext,
  plaintext history and prepared signed transactions are stored in the
  wallet's encrypted cache. A pre-release state is migrated fail closed by
  requiring contact fingerprints to be confirmed again.
- A send is prepare/review/commit. The signed transaction journal is persisted
  before broadcast. After an ambiguous broadcast failure, cancellation and
  transaction rebuilding are disabled; retry reuses only the remaining signed
  plan.
- Every prepared outgoing message persists the exact signed carrier transaction
  hashes in the encrypted QMS state. Transaction history uses only that mapping
  to classify and group carriers; it never guesses from the 1-atomic-unit
  self-transfer amount. Missing, malformed, duplicate or ambiguously claimed
  hashes remain ordinary payment rows. Messages created by older desktop
  builds therefore remain unclassified unless an exact hash mapping is already
  present; amount-only migration is intentionally forbidden.
- Confirmed incoming messages are deduplicated by signed message ID. Reorgs
  invalidate their chain proof, and a canonical replay restores confirmation.
  Incomplete and unknown-sender queues are bounded to 64 and 32 messages.

QMS1/Fast deliberately has no forward secrecy, post-compromise recovery,
continuous ratchet or post-quantum protection. Long-term recipient-key
compromise can expose retained historical ciphertext. Transaction timing,
fees, sizes and fragment relationships remain public metadata. Do not describe
this profile as Signal-like or as QMS2.

## Web Wallet compatibility

Compatibility is at the application wire layer, not by sharing local state.
Complete invitation hex from either client can be imported by the other. Core's
`qms` library performs canonical invitation, fragment, signature, genesis,
fingerprint, UTF-8 and ciphertext validation for the desktop client. The Web
Wallet's independent JavaScript implementation is continuously checked against
the same Core implementation.

The desktop MVP uses its bootstrap invitation for contacts. Its transaction
history offers **All**, **Payments** and **Messenger** views. Carrier
transactions for one outgoing message are represented by one Messenger row
with their total network fee; expanding the row exposes every underlying hash.
The raw wallet history and CSV export remain unchanged and auditable.

The desktop MVP does not yet include the Web Wallet's encrypted backup format,
per-contact invitation export, mempool placeholder, unread counters or
multi-profile state transfer. Those are local UX/storage features and do not
prevent bidirectional QMS1/Fast messages.

## Verification

```sh
tools/check_core_pin.sh
cmake -S . -B build/gui-review -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DSTATIC=OFF -DMANUAL_SUBMODULES=1 \
  -DDEV_MODE=OFF -DUSE_DEVICE_TREZOR=OFF -DQML_TESTS=ON
cmake --build build/gui-review --target qwertycoin-gui qms_unit_tests --parallel 2
ctest --test-dir build/gui-review --output-on-failure
QT_QPA_PLATFORM=offscreen build/gui-review/bin/qwertycoin-gui --test-qml
```

Release artifacts must still be built and smoke-tested by the repository's
native Linux x86_64, Windows x86_64 and macOS Apple Silicon workflows. A Linux
build does not prove Windows packaging or a self-contained, signed/notarized
macOS application.
