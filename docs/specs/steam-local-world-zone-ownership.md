# Steam local host/client Zone and column ownership

Independent native-only clean-room supplement. Sources are constructors,
Zone creation/restoration, the bound local connection, packet handling and
ordinary block setters. No implementation, SDK/reconstruction, owner data/
captures or game execution were used. Addresses are RVAs. This concerns the
normal successful-allocation local-loopback path, not arbitrary external
pointer mutation or a proof that every process allocation is disjoint.

## 1. Bounded conclusion and object identities

**The audited local-loopback path uses separately allocated loaded Zones and
owned six-byte column buffers for host and client. It does not hand the host's
Zone or column allocations to the client.** This conclusion follows from the
allocation/insertion and byte-delivery paths below, not merely from different
World addresses.

The Game World is reached through the pointer at Game+8. The Host itself is
inside Game at+0x6e8; its World is constructed **in place at Host+8**, hence
Game+0x6f0, by0x29a130 at0xf17e1. This latter address is the embedded World,
not another pointer slot. The constructor forwards the Game World's existing
World+0x70 value into that Host World: the content/model catalog is shared.
See [host construction §2](steam-host-crt-stream.md#2-host-and-embedded-world-construction).

## 2. Zone allocation, columns and archive restoration

For the supplied World,0x2ae0c0 first queries **that World's+0x460 map**.
A present nonnull value returns that map's existing Zone. Otherwise it
allocates0x58518 bytes, calls Zone constructor0x2f9400 at0x2ae16a and
generates it through0x2ff6f0 at0x2ae179. Constructor0x2f9400 records the
supplied World at Zone+8 and zoneXY at+0x10/+0x14. It starts the per-Zone
record vectors empty and creates that Zone's own list/cache members.
The path contains no lookup of a second World's loaded map or retrieval
of an already generated Zone through the shared model catalog.

The completed Zone pointer is stored into the supplied World's loaded map
through0x299640 at0x2ae34a, under that World's+0x3f0 and+0x3c8 critical
sections. The resolved native call list has this one caller of that specialized
map insertion helper. That is useful bounded provenance, not an exhaustive
search for every possible inlined map write or unrelated container swap.

Each Zone embeds4096 columns at+0x518 with stride0x58. Its constructor
invokes0x6cbd0 for every column. That initializes each column's block-vector
begin/end/capacity at+0x40/+0x48/+0x50 to zero; it does not attach a borrowed
host or catalog column buffer. The normal filling/setter paths operate on
these column-owned vectors. Empty null pointers or global canonical block
templates are not a shared writable materialized column allocation.

The corresponding ownership is visible in destruction:0x2f9940 invokes
0x6cc40 for all4096 columns. That destroys each nonnull block allocation
through0x6cfb0 and clears the column's three vector pointers. There is no
column sharing count or copy-on-write detachment in this reached lifetime.
It also destroys this Zone's ordinary model vectors and other record
containers. This is separate from the shared catalog resource lifetime.

Where0x2ae0c0 restores persistent state, it consults the supplied World's
**World+0x10 archive**, not another World's loaded Zone. It assigns ground
records to the new Zone+0x30 through0x29bcb0, static-object records to+0x60
through0x29baf0, copies history to+0xd8 and replays its six-byte after-values
through0x31c1c0 at0x2ae2d8. It does not transplant an archived column-pointer
array. Nonzero remote-client flag World+0x358 skips that archive-restoration
section. See [full setter §6](steam-full-block-setter.md) and
[saved zone state, native-saves §5](native-saves.md).

## 3. Actual local connection: control pointer, copied packet bytes

Game construction binds the endpoint through0x7d140 at0x7f85b, supplying
the Client at Game+0xd18 and callback0x3cb50. The allocated endpoint has
size0x18: vtable0x437e98, bound Client pointer at+8 and callback address
at+0x10. Its pointer is held at Host+0x5c8. It contains no Zone or column
storage member.

Host sender0xf4f50 checks the destination against the local SteamID. The
self branch resets the outgoing buffer cursor and invokes this endpoint.
Vtable entry0x8a360 substitutes the bound Client as receiver and jumps to
the stored callback. Therefore the bound route is **0xf4f50 →0x8a360 →
0x3cb50**, still carrying the serialized packet buffer.

Callback0x3cb50 allocates a new0x20-byte buffer object and copies its byte
vector through0x3a970. For a nonempty vector,0x3a970 allocates destination
storage and memmoves the used bytes; it does not alias the sender's byte
array. The callback resets the new cursor and appends senderID/buffer to
Client+0xa40 under Client+0xa18's critical section. Client dispatch subsequently
processes this inbox through the ordinary packet path.

The opposite local direction is also bytes:0xf32c0 makes a0x3a970 buffer
copy and appends it to Host+0x620 under Host+0x5d0. The host dispatcher
0xf4490 handles it. The shared local endpoint/Host control references
therefore do not establish shared mutable terrain storage. Full transport
and join ordering remain in [transport §§1.4–1.5 and4.3](netcode-transport.md).

## 4. Zone answers and client application

The host's0xf33b0 requested-zone path reads its own World+0x460. Missing/
null Zones produce no answer. For a present Zone it appends the saved
block-change log, ground records when the ever-changed flag+0xf9 is set,
static-object records when+0xfa is set, then the answered zone key. It
does not return that Zone's pointer, columns or mesh-cache object.

Those values pass through the normal packet7 serialization/compression and
the byte delivery above. Packet15 requests carry a zoneXY key. Packet7's
fifteen lists carry the specified fixed records and keyed ground/static
lists; they do not contain a full terrain column array or ordinary Zone+0x18
environment-model vector. See [packet layouts §§2.7/2.12](netcode-packets.md)
and [zone-answer selection](netcode-roles.md#42-events-and-zone-states-0xf33b0).

Client0x3e450 case7 decodes/appends into the Game's received-list slots.
The ordinary frame then looks up **its own client World's loaded map**:
block after-values are applied through0x91160/0xa37d0, history is appended
to that Zone+0xd8, received ground/static lists replace that Zone's own
containers, and an answered key clears its+0xf8 needs-state flag. An answer
does not replace the client map value with a host pointer. Missing client
Zones are not created by borrowing the requested host Zone. The precise
queue/application order remains in
[received World updates §8](netcode-roles.md#8-what-a-client-does-with-a-world-update-0xa5a50)
and [block-change lifecycle](steam-block-change-lifecycle.md).

Thus copied values can synchronize two independently owned records. That
includes all six block bytes carried by a notice; copying a red byte as data
does not share its underlying storage. Untouched base terrain and ordinary
environment models come from each World's generator. The supported Steam
route initializes both with seed123; the seed is not sent by this protocol.
See netcode-roles §7 for the bounded original-build world-selection facts.

## 5. Shared versus per-World mutable state

| Data | Proven relationship in this path |
| --- | --- |
| World+0x70 content/model catalog | Same supplied catalog pointer. Its asset resources, raw CUB data and runtime resource caches are outside the terrain-isolation guarantee; this supplement does not claim the whole catalog is immutable. |
| World seed, derived generation parameters and town layouts | Initialized in the respective World. Equal content inputs/values do not make the loaded Zone map or its vector allocations shared. |
| Loaded map, Zone owner+8, column block buffers | Allocated/looked up/inserted and destroyed under the supplied World's ownership. Local transport delivers values, not these allocations. |
| Zone+0x18 ordinary model records and+0x90 auxiliary records | Per-Zone containers. Catalog model IDs/resource references do not turn these records into shared terrain-model arrays. |
| Model-record+0x28 mesh lighting, stored block red and pending/active terrain mesh groups | Belong to the selected Zone/mesh job. The protocol above does not publish those caches or pointer containers into the other World. |
| Zone+0xd8 history and World+0x10 archive | Per-World/Zone containers. History can be sent and copied/replayed as data; the live host history nodes are not adopted by the client. |
| Ground/static records | Generated privately initially, then copied/replaced through the specified answer/update dataflow. |

The two loaded lifetimes need not coincide.0x2ae0c0 reuses only its supplied
map's existing entry; an answered key only changes an already loaded client
Zone's flag. Host absence causes another request later, not a host-pointer
attachment to the still existing client Zone. This supplement does not give
the full host/client streaming-window or eviction policy.

## 6. Lighting scheduling and remaining limits

The ordinary worker path is more specific than storage isolation:

* The resolved direct callers of mesh job0xe9530 are dispatch0x7d680 at
  0x7d6ce and its parallel work0x85dc0 at0x85e6f/0x860ff. Dispatch passes
  the renderer stored at **Game+0x18**. Coordinator0x842d0 selects work
  from the World reached through **Game+8**, then invokes that dispatch.
* Startup0x1355a0 constructs renderer0xc3150 with the same World supplied
  to Game constructor0x7ea80. Renderer base constructor0xcbd10, reached at
  0xc31e4, stores this World at Renderer+8. The Game stores the renderer
  argument at Game+0x18. The ordinary dispatch/job therefore addresses the
  client World's Zones, not the embedded Host World at Game+0x6f0.
* In the resolved native call graph, solver0x2a3260 has the single direct
  caller0xe9530 at0xe96f2. There is no second direct call from host tick
  0xf5520 or host zone-generation service0xf2340. The local endpoint and
  answer paths above contain no redirection of the mesh job to a host Zone.

Thus this **proven ordinary mesh-worker solve route is client-World-bound**;
it does not publish its solved column red into the embedded Host World.
The stored-red copy remains the per-target behavior in
[block-light publication](steam-block-light-publication.md). This conclusion
comes from dispatch and World binding, rather than the host lacking a view.

This bounded trace does not prove that host RGB is always its original
generation data: physical writes/restoration can change its six-byte records.
It also does not exhaust all possible indirect solver calls, renderer-World
rebinding or unrelated host-light-writing helpers. No additional ordinary
host invocation of the specified solver is established here.

This closes the inspected allocation/restoration and normal local delivery/
application paths. All catalog-cache mutation, every generator helper's
internal allocation, every inlined loaded-map write, unusual container swaps,
external hooks and failure/recovery paths were not exhaustively audited.
The conclusion concerns these actual successful ordinary paths, not a blanket
claim that no native API could ever be passed a foreign Zone pointer.
