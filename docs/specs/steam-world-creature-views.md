# Steam client-World creature registry views

## 1. Scope and image identity

This is a bounded read-only data-view contract derived from the original Steam executable and independent native specifications. It identifies the ordinary Game's client World, its local creature pointer, registration list and ID lookup tree. It does not declare complete Game/World/Creature classes, use an SDK layout, define an exporter format or promise a universal atomic snapshot operation.

The analyzed original AMD64 image is `cubeworld.original.exe.unpacked.exe`, preferred base **0x140000000**, SHA-256 **7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244**. Addresses below are RVAs. Evidence includes constructors **0x7ea80/0x29a130**, allocation/link helpers **0x3c0b0/0x36c20/0x29c2b0/0x295fc0/0x2992e0**, lookup **0xe720**, registration **0x29d0d0**, removal/rekey/reset/destruction paths, and ordinary frame/client-dispatch call sites. No engine, exporter, tests, SDK/reconstruction implementation, owner outputs, probes or original-game execution was used.

All pointers listed here are eight-byte native addresses in this build. Widths and offsets are facts of the reached native accesses, not portable declarations of a library container.

## 2. Game, client World and local creature

|Owner-relative field|Established view|
|---|---|
|Game+0x08|Eight-byte pointer to the separately allocated client/rendered World|
|World+0x448|Eight-byte pointer to that World's local creature, or null|
|Creature+0x08|Eight-byte creature ID used as the lookup-tree key|

Ordinary startup constructs the client World through **0x29a130 at0x136163**, retains its result at0x136168 and passes it to Game construction. **0x7eade** stores that World pointer at Game+0x08. It is not an inline World and is not the embedded host's World. The separately reached host construction at Game+0x6e8 uses its own ownership path; see [local World/Zone ownership](steam-local-world-zone-ownership.md#1-bounded-conclusion-and-object-identities).

World construction explicitly sets **World+0x448 to null at0x29a2eb**. Ordinary successful Game construction subsequently creates its default local creature and CharacterData, registers that creature through **0x29d0d0 at0x7f64e**, and **then publishes its pointer to World+0x448 at0x7f657**. It is a member of the same registration list and ID tree as other registered creatures. The earlier list contents supplied to Game construction are not proved universally empty, so this sequence does not establish that the local player is always the first list entry.

Non-null World/local pointers are not themselves evidence that a saved world is loaded, the wanted/loaded world identity matches, or gameplay UI is active. Title-time/default state also has these constructor relationships. No additional gameplay admission is built into the primitive data views.

The normal main-loop controller identity and the shutdown exception for a loader-forwarded context remain [tick-context provenance §§2–3](steam-mod-loader-tick-context.md#2-established-normal-identity-and-construction). That contract does not turn every forwarded value into a lifetime-retained Game pointer; no new context-validation protocol is supplied here.

## 3. Registration list: layout and traversal meaning

|World offset|Meaning|
|---|---|
|+0x40|Eight-byte pointer to the registration-list sentinel|
|+0x48|Unsigned64 number of list elements, excluding the sentinel|

The member begins at+0x40; that field is **the sentinel pointer**, not an inline sentinel, first creature pointer or ID-map root. The separately allocated sentinel and ordinary nodes use a **0x18-byte allocation**:

|Node offset|Meaning|
|---|---|
|+0x00|Eight-byte next-node pointer|
|+0x08|Eight-byte previous-node pointer|
|+0x10|Eight-byte Creature pointer, meaningful on an ordinary element node|

Constructor **0x29a130** initializes the wrapper count to0 and constructs its header through **0x3c0b0**. That helper allocates0x18 bytes and, for the constructor's null next/previous arguments, makes both links point to the new header itself. It does **not** initialize a creature payload at header+0x10. Thus a normal empty list is a nonnull header with next/previous self-links and count0; a null header is not the native constructed-empty representation. The header's payload is not an extra creature or a guaranteed zero value.

The reached consumers begin at **sentinel.next**, follow each node's next link and stop at the **same sentinel address**. They do not stop on null, index through an array or stop after a captured initial count. Their order is **registration/link order**, not signed/unsigned ID order. World+0x50/+0x58 is another list and is not an alternative head/count for this one.

The ordinary World, drawn-state, targeting-candidate and scene consumers use this registration sequence, with their individual admission predicates. Registered entries can include the local player and dead creatures. Membership does not imply HP>0 or drawable state. In particular, the main World traversal checks HP before physics/ability/interaction but still invokes upkeep on the same entry afterward. Details and exact next-link timing remain [World visitation §§1–4](steam-world-creature-visitation.md).

Those ordinary update loops generally dereference a node's creature pointer without a general null-element filter. The World destructor separately tests nonnull payloads before invoking their destructors. These are different consumer policies; no universal skip-null or invalid-pointer repair rule is inferred. Registration itself dereferences the supplied creature's ID after appending, so passing null is not established as a supported way to create an empty entry.

## 4. ID lookup tree: distinct layout, ordering and emptiness

|World offset|Meaning|
|---|---|
|+0x450|Eight-byte pointer to the ID-tree header/sentinel|
|+0x458|Unsigned64 number of tree entries, excluding the header|

This is a separate unique-key ordered tree, not the registration list and not the loaded-zone hash container beginning at World+0x460. Its node allocation is **0x30 bytes**:

|Tree-node offset|Meaning established by the reached helpers|
|---|---|
|+0x00|Left child; on the header, minimum-entry link|
|+0x08|Parent; on the header, root link|
|+0x10|Right child; on the header, maximum-entry link|
|+0x18|One-byte balancing/color state|
|+0x19|One-byte sentinel marker: nonzero on the header, zero on ordinary nodes|
|+0x20|Signed64 ID key, meaningful on ordinary nodes|
|+0x28|Eight-byte Creature pointer value, meaningful on ordinary nodes|

World constructor0x29a130 sets the count0 and installs a header from **0x29c2b0**. That helper allocates0x30 bytes, sets all three links to the header and writes both header bytes+0x18/+0x19 to1. It does not initialize a header key or creature payload. Empty root/minimum/maximum links therefore point to the header, not null, and its key/value must not be interpreted as an entry.

The ordinary-node creation path **0x295fc0 →0x29c670** creates separate nodes with sentinel marker0, copies the eight-byte key and initially sets the value pointer0. Insert/update subsequently assigns the creature pointer. The node color is balancing state, not a validity flag for the creature payload. Constructor, insertion and lookup references establish these fields directly; no SDK or assumed complete container layout is used.

**0xe720(World,id)** searches from header.parent using **signed64 key comparisons**, stops at the sentinel marker and accepts an exact key. It returns the entry's creature pointer, otherwise0. It allocates nothing, inserts no key and does not acquire a lock or lifetime reference. A present entry with null value and an absent entry both yield0. Zero/negative/high-bit ID values are not rejected or converted to unsigned sorting keys by this helper.

Tree order, when traversed in order, is ascending **signed64 ID**. The link/header meanings and ordering are established by0xe720 and the signed-key insertion helpers0x2992e0/0x2978a0. They do not establish that a registration-list traversal has this order. The list count and tree count are separate fields; a read view must not assume that either count is the other's traversal limit or a count of HP-positive creatures.

## 5. Mutations and pointer/node lifetime boundaries

### 5.1 Registration and repeated IDs

**0x29d0d0** first allocates an ordinary list node through **0x36c20 →0x3c0b0**, with next=sentinel, previous=the old tail and payload=the supplied Creature pointer. It increments the64-bit list count and links the new node immediately before the sentinel, after the old tail. The formerly empty list consequently gains both first and last links through that same operation.

Only afterward it obtains/creates the unique ID-tree entry through **0x2992e0**, using creature+0x08, and assigns that entry's value to the supplied pointer. **There is no duplicate-ID admission test before the list append.** A repeated key reuses the tree entry and overwrites its pointer value; the primitive does not thereby remove an old registration node or destroy its old creature. Thus list/map bijection is a normal caller invariant, not guaranteed by this primitive for arbitrary repeated registration. Counts and links are changed by multiple distinct writes, not one atomic transaction.

The reached client packet3 dispatcher **0x3e450**, at creation/registration **0x3e909/0x3e924**, first calls0xe720. An existing creature is updated in place through0x138e00 and bypasses registration; its list node is not moved by that existing-ID path. A missing creature is constructed and appended. The host packet3 path **0xf4490 at0xf487d** uses its own World and analogous lookup-before-register admission. These do not prove every other factory's initial creation order; the directly traced common insertion convention is tail append.

### 5.2 Local ID reassignment and ordinary pointer removal

**0x2a2de0**, reached by client packet4, retains the World+0x448 pointer when nonnull. It erases the old ID key, writes the new64-bit ID to that same creature, obtains/creates the new key and assigns the local pointer there. It does not explicitly reorder the local registration node. Its collision branch invokes the deleting destructor of a pre-existing nonnull value under the new ID before overwriting that value. No explicit registration-list unlink for that collision appears in this helper; no broader duplicate-ID repair/recovery policy is supplied here. Normal ID assignment and malformed/colliding inputs must not be conflated.

**0x2d2260(World,Creature*)** returns without action for a null argument. Otherwise it searches the registration list by **pointer equality**, unlinks/frees the first matching node and decrements World+0x48 when found. It then erases the tree key read from the argument's ID and invokes that argument's deleting destructor. The latter erase/destruction is not conditional on finding its list node. The helper does not itself clear World+0x448. The ordinary client packet6 pruning caller excludes the local player and uses this remover for unrefreshed other creatures; see [packet semantics §§2.4–2.6](netcode-packets.md).

### 5.3 Reset, iteration and destruction

Common reset **0x2d2eb0** first collects registration nodes whose creature kind byte+0x60 is nonzero. After that scan it destroys their creatures, unlinks/frees their nodes and decrements the list count in collected order. Kind0 entries retain their creature addresses, nodes and relative order. It then clears/rebuilds the separate ID tree from the retained registration list. Tree-entry addresses are consequently not persistent handles even when retained creatures survive. Local-player retention here follows its ordinary kind0 state, not a pointer-equality exception in the reset. See [daily reset §1](steam-world-daily-reset.md#1-actorlist-reset0x2d2eb0).

The main World and drawn-state traversals read the current node's **then-current next link after callbacks**. A synchronous tail append can therefore be visited in the same pass, including an append made while processing the old tail. They do not promise safe arbitrary deletion of their current node or automatic iterator repair. This temporal behavior is independently specified in World visitation; it is not a read-snapshot mechanism.

World deleting-destructor entry **0x29c140 →0x29b290** first visits the registration list and destroys each nonnull creature. Later it clears/frees the ID-tree nodes/header through **0x2ac450 at0x29b903**, and frees the registration nodes/header through **0xcc140 at0x29bac2**. The generic tree/list node cleanup does not transfer ownership of creatures to a reader. During teardown, headers and nodes can temporarily still contain pointers to already destroyed creatures. World lifetime therefore cannot be inferred from a plausible sentinel or count alone.

## 6. Reached protection scopes and snapshot limits

The three inline native **Windows CRITICAL_SECTION** objects at World+0x3c8, +0x3f0 and+0x418 are actually initialized with spin count16 by **0x29aeeb/0x29aefd/0x29af0f**, and deleted by **0x29b32c/0x29b33a/0x29b348**. Their existence is not a complete locking contract for every World member.

For the ordinary admitted client Game frame, **0xa6464 →0x2abb70** enters **World+0x3c8**; **0xadb23 →0x2b9890** leaves that same inline critical section on the reached normal path. That scope contains the World substep batch, drawn-state/local-control work and the later client tick **0xaaebf →0x3f4f0**. The latter drains its queued inbox through0x3e450, so the packet registration/removal/rekey work above occurs under the outer World guard in this actual frame path. Packet application here is later than that frame's World/drawn/local-control work; it is not a background receive-thread registry mutation just because the packet came from a worker.

The separate client receive thread **0x3c920 →0x3bae0** enqueues buffers under the **client object's+0xa18** critical section.0x3f4f0 takes that queue lock to detach the inbox, releases it, then dispatches the detached records; see [transport §1](netcode-transport.md). That queue lock is not World+0x3c8 and is not a creature-lifetime reference.

The registration, lookup, rekey, removal, common reset and drawn/World traversal primitives above **do not acquire these World locks themselves**. Game construction performs its initial registration on its construction path; World destruction deletes its critical sections and owned data rather than exposing a lifetime-retaining read guard. Other called scopes/threads must not be assigned the ordinary frame's protection merely from their use of the same helper.

World+0x3f0 has its independently established loaded-zone protection role; it is not proved to protect the creature registration list/ID tree just because both live in World. This bounded trace confirms initialization/deletion of+0x418 but does **not** establish it as the required creature-registry lock. No purpose or universal protection is assigned to that field by its position between the other lock and clock/local-pointer fields.

The observed World+0x3c8 scope excludes concurrent work that cooperates with that same native guard; it does not establish that every actor-field writer, input/window callback, worker, reset/failure path or World lifetime transition does so. Likewise, executing a read on the Game thread avoids overlapping that thread's own normal call sequence but does not by itself prove exclusion of every other thread. The host's separately embedded World and other call paths retain their own scopes; no universal locking policy is inferred.

Consequently, **reading a head/count under a lock and then traversing unlocked does not retain either nodes or creatures**. A copied array of creature pointers also does not keep them alive after removal/reset/destruction. Holding a container-related guard is not, from this trace alone, proof of World lifetime retention or atomic consistency of all nested Creature data. No reference increment, immutable snapshot, pin/release primitive or complete quiescence guarantee is provided by0xe720 or the container headers. The current view identifies native data and actual protection boundaries; an arbitrary asynchronous whole-object snapshot remains unproved.

## 7. Explicit boundaries

This contract supplies the minimal demonstrated fields and container semantics, not every Creature payload/owned buffer, every actor factory/despawner, a complete host/AI threading audit, hash-container rules, saved-file layout or malformed-memory recovery. Sentinel key/payload bytes are uninitialized by their header constructors and are not valid fallback entries. Null lookup results do not distinguish all missing/present-null cases. Allocation failures, arbitrary ID collisions and concurrent invalidation are not assigned invented repair rules.

The Game pointer, client-World pointer, local pointer, list header, list nodes, tree header, tree nodes and Creature allocations have **distinct lifetimes**. Their steady-state correspondence in a valid ordinary World does not make them one allocation or one atomic publication. These bounds are part of the data-view contract, not a claim that the SDK-independent capture adapter is complete. Only this new independent note was written.
