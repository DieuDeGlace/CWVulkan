# Event queues and Creature effect capture

These adapters follow the separately authored
[event-queue](specs/steam-native-event-queues.md) and
[Creature core/effect](specs/steam-creature-core-and-effects.md) contracts.
They copy existing owner data, without reconstructing event production or effect
update algorithms. No SDK ServerUpdates, entity_data or buffs members remain in
the extractor source. Other SDK-dependent Game/World/Zone views still remain.

The death diagnostic observes three distinct bundles: client-World produced data,
Game received data, and hosted-World produced data. It holds the client World's
observed actor section while copying the first two, then separately acquires the
host outer section before the hosted World's actor section. The reusable host
owner lease does not require actor-registry enumeration. These scopes mirror
bounded native caller facts; they do not pin owner lifetimes, exclude every
possible writer, or create one transaction across all three owners.

Thirteen simple lists are copied by their sentinel links, stored count and fixed
payload extents. The two keyed zone-list slots and the owning wide-string slot
are excluded. Empty sentinels are never interpreted as payloads. Capture preserves
slot order, order within each list, duplicates and raw gap bytes without promoting
gaps to semantic fields. Broken/unreadable links, count mismatches, changed headers
and over-budget data publish no partial bundle. The combined tool budget is
16,384 records. Metadata rechecks do not freeze in-place payload changes.

Sound, burst and pickup decoders use explicit field copies rather than C++ struct
packing. Pickup quantity is unsigned 16-bit, independently of inventory's signed
32-bit stack quantity. The item remains a complete owned 160-byte value. Sound
retains its owner identifier and two distinct floating parameters. Burst vectors
and the unresolved four-byte fields remain separate; no universal color-space or
unit meaning is assigned. Other simple records retain their raw payloads.

All formatting happens after the capture leases end. Diagnostic suppression now
includes owner and slot, preventing a client record from hiding an identical host
record. It still suppresses repeated raw observations and clears after 20,000
keys; this log is not an exhaustive event history or occurrence counter. Queue
resets can change sentinel addresses, and frame delivery timing can leave a
bundle empty at the callback. Empty observations do not prove no event occurred.

The AI trace copies Creature effects from the separate linked list at +0x980.
Each payload is 24 bytes, containing the kind byte, two float values and a source
identifier, plus unresolved copied bytes. Capture preserves order, duplicates,
nonpositive timers and padding. It does not filter, aggregate or infer expiry;
native removal depends on the kind and other state. The per-Creature tool budget
is 16,384 effects, with failed capture explicitly printed as unavailable.

Remaining Creature diagnostics use documented scalar reads. The SDK's unverified
charge label is replaced by offset-named flag74 and charge158 values. Timer140
is not called time-since-hit, and uninitialized bytes61..63 are no longer printed
as fields. Player item-region coordinates retain signed32 values. Axis-only
velocity writes preserve vertical velocity, and drop-coordinate conversion avoids
signed negative shifts. The townsperson-name print is bounded to sixteen bytes.

Sparse fixtures check list ownership, duplicates, byte preservation, unsigned16
pickup quantity, malformed links/cycles/counts, budgets, metadata changes and
overflow. Actual Win32 tests retain and release the host/World guards, including
contention and unwinding. Scalar fixtures check separate resource/timer fields
and untouched bytes. These checks do not execute native event production or
qualify live supported-game parity. Full SDK removal and release acceptance
remain separate open work.
