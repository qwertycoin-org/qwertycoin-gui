# EPoSe GUI/Core compatibility matrix

This matrix is normative for GUI Core pin
`1e7de336faeff8f1dc6604fe75d038dc3568d7fa`. The pinned Core binds mainnet
genesis `4f95857586e2c66063c277370eda99cd75897d773af09f0c3cd1e22f7e87db39`
and network ID `515743324d41494e3230323652303102`
and parameter hash
`2c26755094535871dd3ede7bd1b50aba82a9fb6831f0a17f32968eb0385145c6`.
Code and tests at that revision take precedence over older setup documents.

## Read-only RPC contract

| Method | Request | Relevant response | Access | GUI use and required semantics |
| --- | --- | --- | --- | --- |
| `get_epose_info` | Empty | Protocol/epoch boundaries, node and attestation counts, state hash, reward BPS; separate local producer/key/registered/active/qualified fields | Restricted RPC allowed | Capability and current network/local summary. Missing method means unsupported, not disabled. Numeric identifiers are transported to QML as strings. |
| `get_service_nodes` | `limit`, default 100 | Entries, `total_count`, `returned_count` | Restricted RPC allowed | Current service-node list. `returned_count < total_count` is labelled as a partial list; there is no cursor and no invented pagination. |
| `get_service_node_status` | `service_public_key` | `found`, one descriptor | Restricted RPC allowed | Exact current-key lookup. Stable identity and rotating service key remain separate fields. Reward-address equality never proves operator control. |
| `get_service_node_registration_payload` | Empty | `ready`, `error_details` and legacy payload fields | Restricted RPC allowed | Compatibility probe only. At this pin it intentionally returns `ready=false`; the GUI never presents this retired path as registration. |
| `get_epose_epoch` | `epoch` | Epoch boundaries and active/qualified counts | Restricted RPC allowed | Current-epoch detail. Request value 0 is the Core alias for the current epoch and is not shown as a historical epoch-zero query. |
| `get_epose_service_endpoint_v2` | Optional `descriptor_hash` | Local descriptor readiness, host/port, sequence, expiry and hashes | Restricted RPC allowed; direct JSON URI | Local producer descriptor/status only when observing the managed local daemon. It is not treated as proof of registration or qualification. |
| `get_service_rewards` | Optional `height`, where 0 means current height | Preview availability, height/epoch, reward BPS, qualified count and optional payee keys | Restricted RPC allowed | At this pin `preview_available=false` and payee fields are empty. The GUI displays “preview unavailable” and does not infer that EPoSe or actual rewards are inactive. |
| `submit_epose_envelope` | Canonical binary envelope encoded by Core helpers | accepted/relayed | Unrestricted RPC only | Not used by the GUI. Local producer registration and renewal are automatic; QML does not serialize or submit protocol envelopes. |

Every data source keeps its own successful-observation time. These responses do
not form an atomic common block snapshot, so the GUI does not merge them into a
single falsely anchored state. A daemon/network generation counter rejects late
responses after connection changes.

## Local producer contract

The supported producer path is the pinned daemon with these exact technical
flags:

```text
--epose-v2-service
--epose-v2-keystore
--epose-v2-reward-address
--epose-v2-endpoint-host
--epose-v2-endpoint-port
--epose-v2-discovery-endpoint
```

The GUI accepts a public primary QWC reward address only. It never requests a
private view or spend key. The endpoint port is the public restricted-RPC probe
endpoint, not the P2P port. Enabling the producer is an explicit local-node
action and restarts the managed daemon with structured arguments. Remote-node
connections remain observation-only.

The Core owns admission work, initial registration, renewal, descriptor
sequence and service-key rotation. A running process, loaded key, registered
descriptor, active descriptor, qualification and reward eligibility are shown
as distinct states. Missing producer progress remains unknown rather than being
estimated from elapsed time.

## Fixed profile at this pin

- Public name: EPoSe; wire protocol version: 2.
- Epoch length: 720 blocks; anchor depth: 60 blocks.
- Admission target: 18-bit RandomX.
- Committee target: 9; quorum threshold: 6.
- Three attestation rounds at offsets 0, 200 and 400; two required rounds.
- Maximum active population: 100.
- Service reward: 1000 basis points (10%); an empty qualified set falls back to
  the miner according to the Core reward path.

These values are displayed from verified Core responses or documented as the
pin profile; they are not copied into GUI decision logic.
