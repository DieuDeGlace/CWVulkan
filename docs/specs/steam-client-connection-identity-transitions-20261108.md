# Steam client connection and local-identity transitions, 2026-11-08

FINAL / STABLE / HELD. This independent, code-free source-side contract concerns selected client connection transitions and their immediate World effects. Previously held artifacts remain immutable.

## Image, exposure and scope

The supported original is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe, x64 PE Machine 0x8664, ImageBase 0x140000000, TimeDateStamp 0x5D93AB0D, SHA-256 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. The hash and PE metadata were checked again for this assignment. All addresses below are RVAs. Original exported metadata identifies this same image.

Research used the source-author policy and filename-selected original-image routines, native assembly and image metadata only. No implementation, project history or specifications, SDK/dependency source, tests, owner outputs/captures, memory, or prior handoff artifact was read for this assignment. No prohibited-material exposure occurred. Native bytes for the key transition calls and stores were checked directly against the identified executable.

Game and World below denote the roles of the objects at the inspected calls. The client is the Game subobject at +0xD18 on the viewed path. Its owning Game pointer is at client +0xA50; the viewed World pointer is at Game +0x08. The selected peer is a 64-bit value at client +0x70. The local-player pointer is at World +0x448; that entity's 64-bit identifier is at entity +0x08. World +0x450 is the entity identifier lookup structure. These are observed locations, not a proposed implementation layout.

## The local destination value

The value used for local-host routing is stored at global RVA 0x54DDC8. The client constructor at 0x3AA10 obtains it from a Steam context query and stores it at 0x3ABAC. It is not established as a fixed numeric zero or an arbitrary invented sentinel.

The sender/connection helpers compare a destination against this stored value. An equal destination uses the attached local host, when client +0xA10 is nonzero, through the host packet enqueue at 0xF32C0. Other destinations use the Steam transport call. This local-host routing is the basis for calling the value the local destination here.

## Accepted message 4 changes identity immediately

The dispatcher at 0x3E450 reads the 32-bit message identifier, then checks the sender against the current selected peer. Message 4, like 3, 6, 7 and 8, requires equality with client +0x70. The limited acceptance of other senders applies to message identifiers 0 and 2, not 4. A packet from the selected peer refreshes the 32-bit receive-time field at client +0x9EC before its message-specific action.

For an ordinary well-formed message 4, the dispatcher reads the following 64-bit identifier and immediately calls the live World's identity helper at 0x2A2DE0, through call site 0x3E7A3. It then writes 1 to the byte at client +0x9E8, at 0x3E7A8. This is synchronous per-packet application, with no insertion into a Game message-7 pending category.

The identity helper first tests the current World +0x448 local-player pointer. If it is null, it returns without creating a local player or inserting a replacement entity. The dispatcher's later +0x9E8 write still occurs.

With a local player present, the helper performs these operations in order:

1. Remove the old local identifier's entry from World +0x450 through the lookup-removal helper at 0x2AC530, reached at 0x2A2E19.
2. Store the supplied 64-bit identifier directly into the same local entity's +0x08 slot, at 0x2A2E29.
3. Obtain the lookup entry for the supplied identifier, creating an entry if necessary. If that destination entry already contains a nonnull entity pointer, invoke its destruction path synchronously. The destruction call is at 0x2A2F33.
4. Store the existing World +0x448 local-player pointer as the destination entry's entity pointer, at 0x2A2FBA.

The observed operation rekeys the existing local entity and its identifier lookup; it does not replace the World +0x448 pointer with a newly allocated local entity. The destination-collision destruction is a separate visible effect. Its complete consequences for every possible colliding entity type or invalid ownership state are not established here.

Neither the message-4 body nor this identity helper directly changes the selected peer, writes World +0x358, invokes the connection reset at 0x3D820, resets the pending source-request coordinate list, clears the client's received-identifier set, or performs the day-reset operation. Those effects must not be attached to message 4 merely because it commonly participates in connection establishment.

## The shared peer-transition helper

Routine 0x3D820 is a teardown-and-select transition. Its input is a client and a 64-bit destination. The operation commonly described as disconnecting the remote client reaches this helper with the local destination described above; it does not finish by leaving the selected peer cleared.

The ordinary returning path has this order:

1. Test whether the old selected-peer value passes the validity test at 0x3BE00. If valid, construct/send message 2 to that old peer, using local-host enqueue or remote transport as appropriate. Remove that old peer from the invitation registry at client +0xA00. An invalid old value skips this preliminary notification/removal branch.

2. Reset the client's local-state comparison record at +0x78 from the default record constructor at 0x4D600, then set client +0x9E8 to 1. The default-record copy call is at 0x3D93F and the flag store is at 0x3D944.

3. Clear the selected-peer value at +0x70, and clear the client's current received-identifier set at +0x9F0 through 0x3D6C0, reached at 0x3D968. Empty/free the outgoing client category lists at +0x20, +0x30, +0x50 and +0x40, in that order, including their counts.

4. Synchronously call the viewed World cleanup at 0x2D2C00, through 0x3DA3C. It destroys and unlinks entities other than the World +0x448 local player, rebuilds the identifier lookup from the remaining entity list, and clears the World list at +0x50. The local-player object is retained. This cleanup is not the message-7 application pass.

5. Synchronously call the loaded-chunk restoration/request-state operation at 0x2DA090, through 0x3DA4C. Its detailed effects appear below.

6. If an attached host pointer exists at client +0xA10, call its reset at 0xF4FF0 through 0x3DA5D. That routine operates on the attached host's own state and embedded World, including host connection/event/packet queues. This is distinct from the viewed World pointer used in the preceding two calls.

7. Construct/send message 1 to the supplied destination, again using local-host enqueue only for the local destination and an attached host, or remote transport otherwise.

8. Sample the clock and store the client receive-time at +0x9EC. Store the supplied destination as the selected peer at client +0x70, at 0x3DB1E. Then write 1 to the viewed World byte at +0x358, at 0x3DB2D.

The resulting selected peer is the supplied destination, and the directly written World +0x358 value is 1 even when that destination is local. The old selected-peer value is zero only during the transition. Message-1 send completion or a subsequent message-4 identity response is not required before the final peer and World-byte stores.

The transport return value is not used here as an ordinary conditional rollback point. If the destination is local and there is no attached host, the local enqueue is skipped but the later selected-peer and World-byte stores still occur on this returning path. These observations do not establish behavior after allocation failure, exceptions, thread failure or invalid object state.

## Directly reached loaded-source/request state

The viewed World operation at 0x2DA090 iterates entries in the loaded-chunk collection at World +0x468. For every entry whose chunk pointer is nonnull, it writes 1 to chunk +0xF8, at 0x2DA0CE.

For a chunk whose saved-change count at +0xE0 is nonzero, the routine visits the saved-change records at +0xD8 from the list's back toward its front. Each saved record supplies coordinates and saved block data to the chunk writer at 0x31C1C0, reached at 0x2DA10F. After restoration, it empties/frees the saved-change list and clears its count. It then sets the chunk byte at +0x23C to 1 and records the current 32-bit time at +0x238, through stores at 0x2DA1A4 and 0x2DA1B1. A chunk with no saved changes still receives +0xF8 = 1 but does not take that restoration-and-dirty-time branch.

These are direct synchronous changes to already loaded chunks during the connection transition. The routine does not simply enqueue a future reset, and it does not establish that every loaded chunk is removed from the World.

The connection with ordinary source requests is visible in the viewed update at 0xA5A50. Before its packet drain at 0xAAEBF, that update clears the coordinate list at Game +0x2240. If World +0x358 is nonzero, it scans the relevant area, and for an existing chunk with nonzero +0xF8 it adds that chunk's coordinates to the list. The later outgoing portion of 0x3F4F0 can send these coordinate records as message 15 under its separate outgoing timing/connection conditions.

Consequently, 0x3D820's per-chunk request-eligibility stores happen immediately, but the helper does not itself rebuild Game +0x2240. If the transition is first reached while dispatching the late packet snapshot, the update's earlier coordinate-list construction has already occurred. This evidence does not justify treating every newly marked chunk as newly added to that same invocation's already built outgoing coordinate list, nor does it establish a fixed request or response deadline.

The message-4 identity helper has no directly reached invocation of this loaded-chunk reset. Its state-send flag write and the peer-transition helper's chunk/request changes are different operations.

## Ordinary disconnect, selection and timeout routes

Message 2 is admitted by the dispatch entry guard even from a nonselected sender. Its body invokes 0x3D820 with the local destination only when the sender equals the current selected peer; the call is at 0x3E752. It then removes that sender from the invitation registry. A message 2 from another sender therefore does not itself establish a selected-peer change.

The small wrapper at 0x3DBF0 also invokes 0x3D820 with the local destination. The selected-peer interface action at 0x27B0F0 has a bounded client branch: with an in-range selected entry and its client-action mode enabled, it compares that entry's peer with the client's selected peer. An equal entry reaches the wrapper at 0x27B180. A different entry reaches 0x3D820 with that entry's peer only if the invitation-registry membership test succeeds, at 0x27B1B0. Otherwise the inspected branch returns without selecting it. The action is installed by the interface construction path at 0x279DB0. This establishes the action's transition choices, not an exact callback time within every Game update.

After 0x3F4F0 finishes dispatching its copied packet sequence, its connection maintenance compares the unsigned 32-bit elapsed clock value against 5000. A value greater than 5000 reaches 0x3D820 with the local destination at 0x3F721. Equality does not take this timeout branch. The decision is based on the current receive-time field after the snapshot's packet effects; it is not a wall-clock timeout guarantee from the physical arrival of the last network packet.

The same maintenance routine also expires invitation-registry entries after its separate observed elapsed threshold of more than 6000. That invitation removal does not itself select a peer or invoke the World connection reset.

Additional discovered callers include the client destruction path at 0x3B100. The World entity cleanup at 0x2D2C00 also has a separate caller at 0x8A500. These are reasons to keep helper behavior separate from the particular ordinary connection trigger. They do not establish extra ordinary frame transitions.

## Ordering with messages 3, 6, 7 and 8

At the proved late drain, 0x3F4F0 snapshots the incoming entry sequence under the client queue lock, releases that lock, and calls 0x3E450 for each entry in sequence. The per-packet selected-peer test reads the client's current +0x70 value each time. It is not one peer decision captured for the whole snapshot.

An accepted message 4 completes its local-identity changes before the next entry is dispatched. Therefore a later message 3 compares its entity identifier with the newly assigned local identifier. A message 3 which precedes message 4 uses the earlier local identifier. The message-3 remote-entity state application remains direct synchronous World work.

Message 6 likewise performs its live-entity reconciliation at its own position in the snapshot and clears the current received-identifier set afterward. Its exclusion of the local entity uses the retained local-player pointer. Neither the rekey operation nor the transition reset may be moved to the end of the snapshot while preserving these observed intervening effects.

Accepted message 8 performs its day-change reset, when needed, and its World day/scalar stores at its own dispatch position. It is not delayed to the message-4 response or the message-7 pending-event pass. These calls use the live viewed World and its current local-player pointer.

If a selected peer's message 2 switches the client to the local destination partway through the snapshot, later entries are still visited, but subsequent gameplay messages from the former peer no longer pass the selected-peer equality test unless that value is also the newly selected destination. A later entry from the new selected destination can pass the test in this same snapshot. The transition helper does not directly discard the current private packet sequence or clear the shared incoming queue at +0xA40.

Message 7 still only appends decoded category records to Game pending lists during dispatch. No Game pending-category clearing operation is directly reached by the inspected transition body. Thus outgoing-list clearing and received-identifier clearing in 0x3D820 must not be generalized to a proved clearing of all deferred incoming message-7 work.

These are per-packet sequencing facts, not a global network-arrival order, a transactional whole-drain rollback, or a concurrency guarantee for arbitrary reentrant calls.

## World +0x358 and alternate paths

Three explicit writer contexts were identified in the native evidence selected for this assignment:

- The World constructor at 0x29A130 initializes +0x358 to zero, at 0x29A276.
- The client peer-transition helper writes it to 1 on the viewed World at 0x3DB2D, including local fallback.
- The separate socket/thread disconnect helper at 0x911E0 can write it to zero on the Game's viewed World, at 0x91249 and 0x91309. That helper always clears the 32-bit Game field at +0x2260, and, when its active byte at Game +0x21E8 is set, closes an existing socket, joins the corresponding threads and takes its World-side disconnect path. The late zero write is inside the World lock. Discovered callers include the Game destruction path and 0x9FDF0; this is not a call directly reached by 0x3D820 or message 4.

The socket/thread helper and the Steam peer-transition helper are distinct source paths. Their different writes prevent interpreting every operation called disconnect as an unconditional World +0x358 = 0 operation. Equally, the value 1 by itself does not prove that the selected peer is remote.

This is not a universal writer census. Other undiscovered or indirect stores, object aliases, alternate connection paths, and the complete meaning or writers of other role bytes were not proved. A peer transition's directly observed stores do not authorize resetting unrelated role state.

## Limits and evidence identifiers

The ordinary returning, well-formed paths above are supported. In particular, the truncated message-4 payload branch does not show a clean packet rejection before the helper call: it bounds the read cursor and continues using the local value slot. This handoff does not assign a deterministic zero identifier, safe ignore rule, or arbitrary-memory value to that malformed case. Truncated headers, other malformed messages, invalid identifiers, allocation/exception paths, impossible ownership collisions and transport failures beyond the observed unchecked return are outside the established contract.

The caller/registration observations come from discovered static references. They do not establish all indirect callers, all modes, a universal role transition graph, a runtime trace, or reliable transport completion. No replacement implementation or implementation guidance is included.

Original evidence: routines 0x3AA10, 0x3D820, 0x3DBF0, 0x3E450, 0x3F4F0, 0x3BE00, 0x3E310, 0x3E320, 0x3D6C0, 0x3DCE0, 0x2A2DE0, 0x2AC530, 0x2D2C00, 0x2DA090, 0x31C1C0, 0xF4FF0, 0x27B0F0, 0x279DB0, 0x29A130, 0x911E0, 0x818D0, 0x9FDF0 and 0x8A500. Viewed coordinate-list order uses the native instruction export for 0xA5A50 and its successful original-source retry. Native direct-reference metadata, original export identity metadata and the supported executable substantiate the cited call/store anchors.

The implementer must read the final artifact completely before using its facts. After final full readback the author holds all writes.
