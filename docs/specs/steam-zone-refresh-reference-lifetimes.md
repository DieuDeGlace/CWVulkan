# Original Zone refresh reference lifetimes

This bounded contract supports cubeworld.original.exe.unpacked.exe, AMD64 PE32+, 5,757,952 bytes, preferred image base 0x140000000, SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. Addresses are RVAs. Evidence is restricted to the original binary and original native function, assembly and call records. No game, probe or build was run. All World and Zone offsets below retain their respective object bases.

Refresh 0x31c540 can retain the Zone's actual address in a World-owned collection before returning. World+0x1a0 is not merely a set of coordinate copies. Its later native consumer dereferences those retained Zone pointers. This is a directly established refresh lifetime edge; it does not resolve the different, still-unproved full transitive escape scope of initial generator 0x2ff6f0.

## Refresh entry and retained value

Entry 0x31c540 receives a constructed Zone in RCX. It obtains the World from the Zone's eight-byte pointer at +0x08. It compares the World's four-byte day counter at +0x08 with the Zone's four-byte last-processed value at +0x4f8 and returns immediately when they match. Otherwise it writes the new counter value before continuing. The gate is a native counter comparison, not an independent calendar or elapsed-duration check.

The routine refreshes Zone-local content, compares old and new records and has two observed notification-insertion sites. At each site it puts the Zone base itself into an eight-byte local value and passes the address of that local to insertion helper 0x73f40 for World+0x1a0. It also sets Zone byte+0xfa to one. The first insertion can follow a content difference detected after rebuilding Zone+0x60 from other local content; the later insertion can follow the final comparison. Complete refresh algorithms and the semantic taxonomy of the records are outside this audit.

Helper 0x73f40 compares the eight-byte key against existing node values as an unsigned quantity and suppresses a duplicate equal value. Its insertion path reaches 0x711e0, then 0x6e510 and allocator 0x878c0. Copier 0x6e510 copies the eight-byte value addressed by its source into the allocated node at +0x20. Thus the collection owns a copied pointer value, not the local stack variable's address, not a coordinate pair copied from Zone+0x10/+0x14, and not a new copy of the Zone object.

No retain/release counter update on the Zone was established in this insertion path. The pointer is a borrowed reference held by the notification collection, although the World owns the tree-node storage containing it. Inserting a Zone twice does not establish two independently owned Zone instances or extend the Zone's lifetime through reference counting.

## World+0x1a0 representation

Constructor 0x29a130 initializes a tree header at World+0x1a0. The first eight bytes hold a separately allocated sentinel pointer and the next eight bytes, World+0x1a8, hold a count initialized to zero. Sentinel allocator 0x29c310 and ordinary node allocator 0x878c0 request 0x28 bytes.

Nodes have eight-byte links at +0x00, +0x08 and +0x10. The examined tree operations use them as left, parent and right links. Byte+0x18 is tree color/state and byte+0x19 distinguishes the sentinel from ordinary nodes; the sentinel initializes the two-byte state at +0x18 to 0x0101. The ordinary node's eight-byte key/value at +0x20 is the Zone pointer in this use. The pointer's numeric ordering, not Zone coordinate ordering, drives the observed insertion search. Padding and other uses of this generic helper are not assigned meanings here.

This header and node layout must not be confused with a simple next/previous event list. Nor is World+0x1a0 the same field as Creature+0x1a0 or Zone+0x1a0. The retained value's meaning is established by the refresh caller and consumer, not by the offset alone.

## Normal consumption and clearing

Consumer 0x2e30b0 receives the World. It traverses the tree at World+0x1a0 and obtains each Zone from node+0x20. It sets that Zone's byte+0xfa, reads its four-byte coordinate components at +0x10 and +0x14, and reads the content range represented by Zone+0x60/+0x68. It constructs a temporary linked collection from those content records and appends a notification under World+0xf0.

Helpers 0x7650/0x92c0 allocate new list nodes and copy records from the Zone's content range. Notification constructor 0x7600 reaches 0x98e0, which copies the two coordinate components and transfers the temporary nested-list storage into the notification. The temporary is subsequently cleaned up. Thus this notification owns its newly constructed collection storage and copied coordinate values; it does not obtain its Zone reference by retaining the local pointer variable used for tree insertion. Complete semantics of every field within the copied content records remain outside scope, so this is not a universal claim about all possible addresses embedded in those records.

The same consumer makes another pass over World+0x1a0 and calls 0x31d700 on every retained Zone. That routine inspects and changes Zone-local spatial state. A Zone must therefore remain usable beyond the initial coordinate-copy step; producing a notification is not the end of the consumer's Zone dereferences.

On the observed normal path, 0x2e30b0 then destroys the ordinary tree nodes, restores all three sentinel links to the sentinel and sets World+0x1a8 to zero. It retains the sentinel. This clearing frees the storage that held the pointer values; it does not destroy the referenced Zones. The consumer also handles the analogous World+0x190 collection and a different notification group, which must not be confused with the +0x1a0-to-+0xf0 path.

World destructor 0x29b290 separately clears the +0x1a0 tree through 0x2ac600 and releases its sentinel. The generic clear helpers, including 0x8aad0 and recursive tree cleanup, free tree nodes without invoking Zone destruction for their +0x20 values. World Zone ownership/publication uses a different collection rooted at World+0x460. Refresh notification membership is therefore distinct from owning publication membership.

The examined direct Zone destructor 0x2f9940 does not remove its own address from World+0x1a0. It is not a notification-detach operation. Destroying a Zone while its address remains pending in this tree would leave that pointer available to the later consumer's established dereferences. The normal tree-clear path bounds this particular reference interval; it does not prove that every other external reference has ended.

## Zone-local child refresh and side effects

Refresh also invalidates and recreates some Zone-local children. Its list at Zone+0x4e8 tracks child pointers. The routine removes a tracked child's identifier from the Zone-local collection at +0x460 through 0x2ff4f0, invokes child destructor 0x118d30 and frees the child, then clears the tracking-list nodes. Later paths allocate children, register them through 0x119530 and add their pointers to that Zone-local tracking list.

Those examined child-tracking and registration edges stay in the Zone's own collections. They do not turn the World+0x1a0 entry into a child pointer: that World entry remains the Zone base. This does not prove that arbitrary other readers hold no borrowed child addresses when refresh destroys a child. Refresh also calls srand/rand and performs allocations, record copies and spatial updates. The routine is not a read-only day check or a pointer-registration-only operation.

## Observed callers and protection

Native World update 0x2e17d0 gathers Zone pointers from the World's published coordinate collection into a temporary set and calls refresh on each selected pointer. Assembly at 0x2e1d70 through 0x2e1d7c loads the selected pointer and supplies it in RCX. The examined ordinary route therefore refreshes an already published Zone. No native normal path that refreshes an unpublished temporary and then immediately disposes of it was established here.

World simulation 0x2da6c0 reaches 0x2e17d0 in one mode. Hosted update 0xf5520 holds hosted-object+0x5f8, enters its embedded World's +0x3c8 section, performs simulation and then calls 0x2e30b0 before leaving that World section. This is an established enclosing scope covering refresh and subsequent consumption on that hosted route. The frame routine 0xa5a50 also calls 0x2e30b0 within its observed caller-held World+0x3c8 interval.

Refresh, insertion and consumer bodies examined here do not themselves establish a universal lock around all affected owners. The enclosing hosted/frame facts do not prove exclusion against every writer, Zone destruction, caller reentrancy or every worker using those objects. Exception or abnormal exits before normal consumer clearing are not qualified as a completed reference-release interval.

## Separation from initial generation

Original direct-call records for generator 0x2ff6f0 do not show a direct call to 0x31c540 or the +0x1a0 insertion helper 0x73f40. Consequently the proved refresh edge must not be silently attributed to every initial-generation call. Conversely, a missing direct call does not exclude an indirect/transitive refresh, another registration route or another retained address.

The readily inspected generator ownership edges include 0x119530, which registers generated children in Zone+0x460 and replaces/destroys an existing child there, and late calls to 0x31d470 and 0x31d700 with the Zone base. These establish local ownership/state changes. No additional direct-generator callee retaining the Zone or its children in a World queue, worker or shared owner was proved by this bounded follow-up. Both available original decompilations of the large generator time out; selected assembly/callee checks are not a full escape census.

The conclusions are therefore separate: refresh can leave a raw Zone reference pending in World+0x1a0 until its normal consumer clears the collection; safe disposal of an arbitrary initially generated unpublished Zone remains unproved for the broader generator graph. No generation algorithm, engine capture policy, universal lock order or quiescence guarantee follows from this note.

Status: FINAL / STABLE / HELD after readback.
