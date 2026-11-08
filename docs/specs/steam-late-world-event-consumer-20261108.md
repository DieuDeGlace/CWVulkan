# Steam late World-record visit before rendering, 2026-11-08

FINAL / STABLE / HELD. This new, code-free original-source audit resolves call site 0xAAF58 and the surrounding viewed-update order. Earlier held artifacts remain immutable.

## Supported image and exposure

Original reference: C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe. SHA-256 independently rechecked: 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. PE Machine is x64 0x8664, ImageBase 0x140000000, TimeDateStamp 0x5D93AB0D. All addresses below are RVAs.

Research used the standalone source-author policy, filename-selected original-image routines and their native evidence, original reference metadata, and the supported image. No replacement engine, SDK/dependency source, project history/specifications/tests, owner outputs/captures, implementation memory or held handoff artifact was read for this assignment. No prohibited-material exposure occurred.

The complete small callee at 0x2C3470 was examined in original native assembly alongside the decompilation. The principal caller/phase instructions and callee boundary/branch anchors were checked directly against the identified image.

## Result at the assigned lead

The call at 0xAAF58 does not return a collection consumed by the viewed update. The callee at 0x2C3470 is a 70-byte read-only routine. Its caller already owns the iteration over World +0x120, and discards the return register after each call.

The actual bounded operation is a visit to each existing record in that World collection. The callee conditionally walks the current World identifier lookup. It makes no stores and no calls. No World, player or Game mutation, entity deletion, experience award, currency award, level change or new event production is established at this lead.

Accordingly, no returned collection can be followed from this call into a later record consumer. An accessor interpretation or business effect attached to the callee's incidental return-register value is unsupported by this native path.

## Record storage, identifiers and actual iteration

The viewed Game pointer is retained by the caller; Game +0x08 supplies the World. At 0xAAF38 the caller loads the list header from World +0x120. It reads the first list node and compares it with the end/header node. Empty lists skip directly to 0xAAF65.

For a nonempty list, the loop body beginning at 0xAAF50 passes the current node's record at node +0x10 and the current World pointer to 0x2C3470. After the call at 0xAAF58, it advances to the current node's next link at 0xAAF5D and repeats until the header is reached. The result register is neither tested nor used to select a new collection or update state.

The relevant record is 16 bytes:

- At record +0x00: the first 64-bit entity identifier.
- At record +0x08: the second 64-bit entity identifier.

The inspected visit does not read a separate event-kind field, packet opcode or fixed business-event identifier from this record. These are two identifier values, rather than a dispatch on numeric record-kind tags.

The list visit does not erase a record, unlink its node, decrement the record count or clear the collection. It is an iteration over existing records, not a destructive queue drain.

## Callee admission and current lookup dependence

At 0x2C3470, the routine reads World +0x448, the current local-player pointer. A null pointer returns immediately. Otherwise it reads the current local entity identifier at entity +0x08 and compares it with the record's first identifier. A mismatch also returns immediately.

If those identifiers match, the routine reads the current lookup structure at World +0x450 and uses the record's second identifier for a tree walk. Comparisons against node keys use the native signed 64-bit branch ordering. The walk terminates at an end/nil node.

The routine does not read a found node's mapped entity pointer, does not dereference the second entity's state, and does not perform a final found-entity action. There is no call to the World removal helper, no unlink of World +0x40, no modification of World +0x450, and no write to World +0x448.

This creates a limited dependence on current identity and lookup structure: changing the current local identifier can change whether the walk is entered, and changing lookup membership can change its traversal. It does not establish a later award, deletion or mutation conditional on the second entity still existing. A missing second entity produces no special record-application action on this path.

The callee has its own null-local check, but the ordinary caller accesses the local player for preceding baseline reads before entering this loop. The callee check therefore does not prove that the entire viewed update safely admits a null local player.

There is no separate World +0x358 role-byte guard, health check or interface-admission guard around this particular record loop. The following interface block begins only after the loop at 0xAAF65.

## Population and earlier bookkeeping

One directly proved incoming population route is accepted message 7 at dispatcher 0x3E450. Its selected-peer guard applies before decoding. The category reader at 0x39CE0 reads a count and 16-byte records; the node constructor at 0x36F00 copies the two 64-bit values into each record node. That category is appended to Game +0x1A00. This operation queues category data; it does not call the late visitor.

Earlier in the ordinary viewed update, Game +0x1A00 records are visited for separate bookkeeping. At 0xA84C3 the second identifier is supplied to the insertion helper at 0x7D3F0 for World +0x20. This is an earlier, directly observed mutation; it must not be moved to 0xAAF58 merely because both phases concern the same record category.

At 0xA880F, the update copies the pending pair records into World +0x120 through 0x39000 and its record-copy helper at 0x38330. The first and second 64-bit values remain in order. This is the collection later iterated at 0xAAF58. The pending Game category group is cleared before the local-input phase and before the late packet drain.

A separate original local production route is visible in the damage/death routine at 0x29DCF0. In its reached death branch, it appends records to World +0x120. The second value is the affected entity's identifier. When the entity's +0xB88 relation structure is empty, the first value is zero; otherwise a record is appended for each visited relation key, using that key as the first value. These observations establish the pair provenance for that route, not the complete combat eligibility policy or a universal producer census.

The late visitor compares the first value with the current local identifier even when that value is zero. This audit does not introduce an unconditional rule that all zero-first-value records are skipped independently of the current local identifier.

## Order relative to packet dispatch, camera and rendering

The reached ordinary sequence in the viewed update at 0xA5A50 is:

1. Earlier World stepping and pending-category application, including the 0xA84C3 bookkeeping and 0xA880F pair-record population.
2. Local input/control and further ordinary work.
3. The client packet snapshot/dispatch routine at call site 0xAAEBF.
4. Calls to the ordinary helpers at 0xAAED9, 0xAAEE3, 0xAAEED and 0xAAEF7.
5. The designated camera update at 0xAAF02, then 0xAF110 at 0xAAF0C.
6. Local-player baseline reads, then the World +0x120 record loop whose per-record call is 0xAAF58.
7. Following interface/control and presentation work, including the interface update at 0xAB5CE.
8. The renderer-facing call at 0xAD9D4.

The World +0x120 records in this visit were populated before the late packet drain on the proved incoming category route. A message-7 packet first decoded during that drain appends to the Game pending category; it does not enter the already completed earlier population pass merely because this late World list is visited afterward.

Immediate messages can change the live state read by the visitor. Message 4 can change the current local identifier; message 6 or a peer transition can change the current identifier lookup membership. The visitor reads that resulting state when reached. It still performs only the conditional read-only walk described above.

The visited collection is distinct from the client's protected incoming packet list. The loop does not dispatch another packet or resnapshot transport entries.

## Following ordinary work and mutation attribution

At 0xAAF65, control enters an independent interface/control block. Its admission tests include the interface state at Game +0x398, byte +0x1D8 of that object, and the active-state tests for the interfaces stored at Game +0x118, +0x130 and +0x170. Its button-edge handling and interface visibility/property updates follow the World-record loop; they do not consume an output from 0x2C3470.

The subsequent ordinary control branch can clear Game +0x668 and +0x420 and hide its associated interfaces when the shown control/input guards require it. Later presentation calculations update the relevant Game view/control state, and 0x103200 is called at 0xAB5CE with the interface rooted at Game +0x118. These are ordinary control/interface operations, not mutations selected by the pair record's two identifiers.

Following that interface update, the main routine also examines the current World entity list and per-entity relation state for presentation/music decisions, performs further current-player/interface work and reaches the renderer-facing call. No use of a collection returned by 0x2C3470 exists across these operations.

The later experience/currency/level presentation comparisons are independent current-player comparisons visible in the main routine. Their text/effect production must not be attributed to 0x2C3470 as if it awarded those values. This audit does not establish a complete mutation census for every nested GUI action or presentation helper before rendering; the bounded result is that they have no record-result flow from the assigned lead.

Near the end of this update, after 0xAD9D4 and the outgoing-event collection call, the World event group beginning at +0x90 is replaced with an empty group through 0x83640 at 0xADB0E. World +0x120 lies in that group. This is separate from the nondestructive late list visit.

## Bounds and original evidence

The directly reached record visit has no state-changing business operation to specify beyond the caller's iteration. Its exact reads, guards and ordering are supported; a collection-accessor or mutation-producing interpretation is not.

The original direct-reference export identifies this viewed call as the discovered caller of 0x2C3470. That is not a proof that no other indirect, undiscovered or alternate-mode caller exists. Likewise, the incoming and local population routes described above are selected proved routes, not all possible producers.

Malformed records, invalid list/tree memory, concurrent/reentrant mutation, a complete combat/death policy, all nested interface-side mutations, runtime packet timing and actual display presentation were not established. No replacement algorithm or implementation change is supplied.

Original evidence: the native instruction export and successful original-source retry for 0xA5A50; the complete native/decompiled body of 0x2C3470; incoming category routines 0x3E450, 0x39CE0, 0x36F00, 0x39000, 0x38330 and 0x36960; earlier bookkeeping helper 0x7D3F0; selected local producer 0x29DCF0; following interface routine 0x103200; final group-reset helper 0x83640; original call-reference metadata and the supported executable. Relevant anchors include 0xA84C3, 0xA880F, 0xAAEBF, 0xAAF02, 0xAAF38, 0xAAF58, 0xAAF5D, 0xAAF65, 0xAB5CE, 0xAD9D4 and 0xADB0E.

The implementer must read the final artifact completely before using its facts. After full final readback the author holds all writes.
