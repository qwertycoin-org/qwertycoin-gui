# EPoSe GUI/Core compatibility matrix

This matrix is normative for GUI Core pin
`09086f7dbaaf1a4ff16bddeaa1d729f9ff65eca6`. The pinned Core binds mainnet
genesis `906629482787e94cb00463696a0e95ec75a480da09257c6270c65ba1a74a76b0`
and parameter hash
`e5654b4f5fa27faa51a80ca1e93bb877c3bd3345d0a803b6e7ab55c05189c20d`.
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
