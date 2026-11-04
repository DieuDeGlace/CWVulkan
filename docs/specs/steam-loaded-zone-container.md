# Steam client World: loaded-zone element sequence

Independent native-only supplement. Only original executable/native evidence and
independent prose were read. No SDK, engine, exporter, test, sister implementation,
reconstruction, owner output/capture or game execution was used. This specifies
the sentinel-linked element sequence, not bucket lookup, a complete World class
or a finished independent capture adapter.

## 1. Image identity and owner boundary

Addresses are RVAs in the analyzed original unpacked AMD64 image
`cubeworld.original.exe.unpacked.exe`, preferred base **0x140000000**, SHA-256
`7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244`.
This identifies one analyzed image, not all distributions or modified builds.

The input is a presently valid **client World pointer**, reached through the
native controller's +0x08 pointer on the ordinary path bounded in
[tick-context provenance](steam-mod-loader-tick-context.md). This sequence is
owned by that supplied World. Host/client catalog sharing does not make their
loaded Zones shared; see [World ownership](steam-local-world-zone-ownership.md).
Establishing and retaining the World itself remains separate from interpreting
its container fields.

## 2. Minimal sequence layout

| World offset | Established meaning |
| --- | --- |
| +0x460 | Beginning of the loaded-zone container member; **not** the sentinel pointer |
| +0x468 | Eight-byte pointer to the element-sequence sentinel |
| +0x470 | 64-bit live container-entry count |

The required element-node fields are:

| Node offset | Content |
| --- | --- |
| +0x00 | Eight-byte next-node pointer |
| +0x08 | Eight-byte previous-node pointer |
| +0x18 | Eight-byte Zone pointer |

Observed node allocation is **0x20 bytes**, through **0x29c370**. Nodes are linked
allocations, not a contiguous array; this allocation size is not an iteration
stride. The Zone pointer is at +0x18. In particular, the +0x10 payload offset of
the separate terrain mesh-group list must not be reused for this sequence.
Keys and bucket/hash machinery are outside this view.

## 3. Construction, empty state and native traversal

World constructor **0x29a130** invokes **0x29a010** on the member at World+0x460.
The container constructor initially clears its sentinel/count fields, obtains a
sentinel through 0x29c370 and sets the live count to zero. With the allocator's
empty-list arguments, the new sentinel's next and previous links both refer to
that sentinel. Its key/value payload is not initialized as a live Zone entry and
must not be interpreted as one.

This is the initial container state, not a claim that completed startup or the
first loader callback observes an empty World. A null sentinel is not the
established ordinary empty representation.

Native consumer **0x2d2410** reloads the sentinel while holding the demonstrated
container lock. It starts at the sentinel's next link, follows next links, and
terminates on pointer equality with the sentinel. It does not terminate on null,
advance by node allocation size or use the count as a loop-iteration limit.
No tree color/nil-byte interpretation belongs to this element-sequence view.

For each ordinary node, 0x2d2410 reads **node+0x18** and skips a null Zone value.
The count describes container entries; the schema therefore does not establish
that it equals a separate count of nonnull Zones, completed meshes or currently
visible Zones. Visitation order is the sequence's **current forward-link order**.
No ascending coordinate, insertion-order or portable ordering guarantee is
supplied. Malformed links/counts, corrupt sentinels and incomplete construction
are not assigned a recovery policy.

## 4. Actual lock identity and reached insertion

**World+0x3f0 is an inline Windows CRITICAL_SECTION**, not a pointer slot.
World constructor **0x29a130** initializes it through imported
**InitializeCriticalSectionAndSpinCount**, with spin count **16**. Helper
**0x2abb80** passes its address to imported **KERNEL32 EnterCriticalSection**;
**0x2b98a0** passes the same address to **LeaveCriticalSection**. These helper
bodies establish the native API identity directly.

The completed Zone-publication path in **0x2ae0c0**, beginning at **0x2ae31a**,
enters the World+0x3f0 critical section and then the separate **World+0x3c8** mesh
critical section. It obtains/inserts the entry through **0x299640**, stores the
Zone pointer at the returned node's +0x18, then releases +0x3c8 followed by +0x3f0.
The insertion helper updates the live-entry count when creating a new entry.

The preliminary lookup/early-return route in 0x2ae0c0 does **not itself acquire
those locks**. Consequently these findings do not turn every native lookup into
a self-synchronizing or lifetime-retaining accessor. Caller protection and other
entry paths require their own evidence.

## 5. Reached removal and lifetime limits

Consumer/remover **0x2d2410** holds World+0x3f0 while traversing and choosing its
removal candidate. With that lock still held, it enters World+0x3c8 for the
selected entry's removal. It performs the reached Zone-state handling and calls
**0x2abd20** on the container/node.

Erasure helper 0x2abd20 repairs the neighboring sequence links, decrements the
64-bit count and frees the selected **container node**. This is not the Zone
destructor. The caller releases World+0x3c8, then World+0x3f0, and only afterward
invokes cleanup on the retained removed Zone pointer. Node unlink/free and Zone
destruction are therefore distinct native lifetime events. The ordinary erase
does not free the container sentinel; removing the final entry leaves its empty
self-linked sequence with count zero.

**Reading only the sentinel/count under World+0x3f0 and then traversing unlocked
is insufficient to establish a coherent read.** The demonstrated insertion and
erasure paths can change links/counts and free nodes after that header read.
Copying a Zone pointer from a node does not confer ownership, an automatic
reference or post-unlock lifetime. Nor does entering this critical section prove
that a previously saved World or Zone pointer is still valid.

World lifetime and mesh-buffer protection remain separate. The mesh critical
section's identity and active/pending-list mutation boundary are established in
[terrain buffer resources §5](steam-terrain-buffer-resources.md#5-proven-native-lock-identity-and-lifetime-boundary).
The reached insertion/removal lock order above is not asserted as a universal
policy for every World reset, Zone removal, container replacement or external
mutation. Complete outer-caller protection, concurrent recovery, bucket lookup,
full class declarations and runtime-oracle behavior remain outside this contract.
