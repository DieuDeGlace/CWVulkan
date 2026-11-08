# Steam viewed update: packet dispatch phases, 2026-11-08, v2

FINAL / STABLE / HELD. This is a new, independent, source-side behavioral handoff. Earlier handoffs were not read or modified. This file contains behavioral prose and evidence identifiers only.

## Reference identity and scope

The supported original image is the unpacked Steam x64 executable at C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe. Its independently computed SHA-256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. PE Machine is 0x8664, ImageBase is 0x140000000, and TimeDateStamp is 0x5D93AB0D. The original export summary independently records this same image hash, architecture and base. The packed executable in the same directory has a different hash and is not the image identified by the behavior below. All routine and instruction addresses below are RVAs relative to the supported base.

The assignment concerns the ordinary viewed Game update entered at 0xA5A50 and the client packet snapshot/drain routine at 0x3F4F0, which reaches the per-packet dispatcher at 0x3E450. The words Game, World, client and entity identify the roles evident at these call sites; they are not source-language class declarations.

Evidence was restricted to the original executable, original native assembly and decompilation exports, their original-image metadata, and the source-author role policy. No replacement engine, exporter, SDK, dependency source, tests, implementation history, owner outputs or captures, implementation memory, or previous unfinished network-phase handoff was inspected. There was no prohibited-material exposure.

The main update's first export timed out. Its successful original-source retry and its native instruction export were used together. The original native export records discovered instructions and owner ranges, rather than guaranteeing complete static recovery. The instruction bytes at the phase anchors listed below, at the drain-to-dispatch call, and at the message-8 reset and scalar stores were checked directly against the identified executable.

## Packet ingestion and packet dispatch are different operations

The client-side receive worker at 0x3BAE0 services Steam callbacks and polls/reads the channel used on this path. On a successful read, it owns a separate packet-buffer object and appends a sender-and-buffer entry to the client's incoming list. The list is at client +0xA40, with its count at +0xA48, protected by the critical section at +0xA18. That worker does not call 0x3E450 or apply message-7 category lists or message-8 World writes.

The callback enqueue routine at 0x3CB50 creates a packet copy, starts its read cursor at zero, and appends the sender-and-buffer entry to the same protected client list. A local-host callback registration is visible in the original initialization path at 0x7EA80 through the registration helper at 0x7D140. This registration/enqueue route is not an earlier Game-update gameplay dispatch.

At the start of 0x3F4F0, the client enters the +0xA18 critical section. It copies the queued entry sequence into a private list, clears the shared list and its count, frees the old shared entry nodes, and leaves the critical section. The copy preserves entry order and the packet-buffer pointers. Copying the entries is not another deep copy of each payload at this point.

It then visits the private list from its first entry to its end. At 0x3F5B8 it calls 0x3E450 once for the current entry, supplying the same client and that entry's sender and packet-buffer object. After the call returns, it releases that packet-buffer object and advances to the next entry. The whole private sequence is dispatched before the routine's later connection maintenance and outgoing-message work.

Thus dispatch follows the order of entries in this queue snapshot. The two inspected enqueue routes append at the end, and the lock serializes their changes to the shared queue. This proves serialized queue insertion order on these routes, not a physical network-arrival order across machines, senders or concurrent producers. It proves neither a wall-clock delivery deadline nor a timestamp sort.

A packet appended after the shared list has been emptied cannot join the already copied private list. It waits for a subsequent snapshot/drain. The critical section protects snapshot extraction and enqueue operations; it is not held across the calls that apply the snapshot's packets.

## Reached order in the viewed update

The following is the relative execution order when the ordinary path reaches the corresponding phases. Conditional branches can omit World stepping, local input, individual nonempty-list work, or optional interface work. The ordering does not assert that every phase executes in every invocation.

1. Earlier update preparation and interface/state handling occur. The viewed routine takes the World-side lock through 0x2ABB70 at call site 0xA6464. That helper is a critical-section entry operation; it is not a packet dispatcher.

2. The conditional World simulation substeps are calls to 0x2DA6C0. The repeating full-substep call site is 0xA7328; the residual-step call site is 0xA734C. These precede the drawn-position pass and the pending received-event application described next.

3. The drawn-position/interpolation pass at 0x2B68E0 is reached at 0xA7E80. It visits the World's current entity list, updates drawn-position/orientation state from the current target state and interpolation state, and performs its remaining entity presentation work. The following call at 0xA7E89 reaches 0x368230, whose supported body returns zero. Neither call is a newly established entry into the client packet dispatcher.

4. The update handles the already pending received category lists. This includes block/chunk and other category work, the category-specific entity effects visible in the update, and death bookkeeping. In particular, entries already in Game +0x1A00 are visited and their second 64-bit identifier is inserted into World +0x20 through 0x7D3F0 at call site 0xA84C3. The identifier insertion is the precise observed operation at that lead; this handoff does not replace it with an invented immediate health write or entity-destruction rule.

5. Subsequent work in the same pending-event phase copies the appropriate pending category sequences into the World's current event collections and applies further queued events. For example, the Game +0x1A00 category is also copied into the World event collection at +0x120 before local input. The update later visits that World collection at call site 0xAAF58 through 0x2C3470. That later visit operates on the collection populated earlier; it does not drain the client's incoming packet queue. The received-category group beginning at Game +0x1970 is replaced with empty category lists through 0x83640 at 0xA996D, before local input. The old group is then destroyed.

6. Local-player input/control is conditionally reached through 0x8AE50 at 0xA9B2C. Its guards include current World/player state and active interface conditions. It occurs after the pending-event pass and its clearing operation.

7. Additional ordinary Game work follows, including the visible area/list preparation. The client packet snapshot/drain call is then reached at 0xAAEBF. The caller supplies the client subobject at Game +0xD18. This is the proved direct call into 0x3F4F0 in the viewed update. That routine reaches 0x3E450 as described above.

8. After this drain returns, the update calls 0xB37F0, 0xB36A0, 0xB6240 and 0xB03D0, in that order, at 0xAAED9, 0xAAEE3, 0xAAEED and 0xAAEF7. The designated camera update at 0xADCD0 follows at 0xAAF02. The next call is 0xAF110 at 0xAAF0C.

9. Further World-event, interface/state and presentation work follows. This includes the later visit to World +0x120 at 0xAAF58, the interface property/text work visible later in the update, and the renderer-facing call to 0xE2E00 at 0xAD9D4. The claim here is that this reached renderer work is after the drain; it does not identify a display-present time or a GPU completion time. There is also interface work earlier in the update, so this is not a claim that all interface operations occur after network dispatch.

10. Near the end, 0x3CC40 is called at 0xADADB with the same client and the World's current event group. This routine selects local-player event records into the client's outgoing queues. It does not call 0x3E450, snapshot incoming packets, or apply newly arrived messages.

This order permits immediate effects of a late-dispatched packet to be visible to later camera, interface or renderer-facing work without rerunning the earlier World substeps or the earlier full drawn-position pass. It does not permit a message-7 packet first dispatched at 0xAAEBF to enter a pending-category application loop which has already finished in the same invocation.

## The check for an earlier dispatch

The native direct-call record for the two assigned entries establishes 0xAAEBF to 0x3F4F0, owned by the viewed update entry 0xA5A50, and 0x3F5B8 to 0x3E450, owned by 0x3F4F0. The original full native instruction export agrees. A bounded scan of the identified binary over the viewed update's contiguous address span from 0xA5A50 to just before 0xADBD8 found the same direct relative-call encoding to these targets and no earlier such encoding.

The earlier reached operations checked above do not establish a second ordinary dispatch into these entries. In particular, World lock acquisition, the World substeps, the drawn-position pass, packet-buffer enqueue operations, outgoing event collection and send helpers must not be relabeled as incoming gameplay dispatch.

The supported conclusion is one proved direct drain call in this viewed update, at 0xAAEBF. A two-phase ordinary receive/dispatch schedule with a newly claimed earlier call is unsupported by this evidence. This is a bounded call-site/owner statement. It is not a universal census of all program modes, other update entries, undiscovered code, unresolved indirect calls, callbacks, or possible reentrant behavior. The address-span scan identifies direct relative-call candidates only and is not a substitute for proving arbitrary indirect reachability.

## Per-packet immediate and deferred effects

The dispatcher first reads the 32-bit message identifier. Messages 3, 6, 7 and 8 on the inspected path require the sender to match the client's currently selected peer at +0x70. The limited other-sender acceptance in the entry guard applies to the handshake/control identifiers 0 and 2, not to these four gameplay identifiers. Accepted packets from the selected peer also refresh the client's receive-time value at +0x9EC before their message-specific operation.

Message 7 is deferred category data. The dispatcher decodes its payload into temporary category sequences and transfers those records into Game-owned pending lists. The transfer order is the lists at Game offsets +0x19A0, +0x1970, +0x1990, +0x1980, +0x19B0, +0x19C0, +0x19D0, +0x19E0, +0x19F0, +0x1A00, +0x1A10, +0x1A20, +0x1A30, +0x1A40 and +0x1A50. Within each category the records retain their decoded order and are appended after records already pending for that category. The death-bookkeeping category associated with 0xA84C3 is +0x1A00.

These transfers do not themselves run the main update's category-specific application loops. Multiple message-7 packets therefore accumulate category sequences. The snapshot's packet order determines append order within a category; later gameplay application uses the Game update's category order rather than replaying every packet's complete event bundle as one indivisible gameplay transaction. A message-7 packet first dispatched at the late drain remains pending for a later invocation that reaches the corresponding application pass. No fixed wall-clock wait is established.

Message 8 is immediate World day/scalar application. For a well-formed payload, it reads two signed 32-bit integers. It compares the first with the current World's 32-bit day slot at +0x08. If the values differ, it synchronously calls the reset routine at 0x2C34C0, through call site 0x3ED64, on that live World before overwriting the day slot. Native argument flow confirms that the World is supplied even though one decompilation display omits the argument.

The reset acts on live World/player state at this point; among its visible effects are clearing World +0x20 and the World collection at +0x10 and resetting selected local-player state. After any required reset, the dispatcher writes the new day at +0x08 and converts the second signed 32-bit integer to a 32-bit floating-point value for World +0x0C. The stores are at 0x3ED74 and 0x3ED8A. An equal day skips the reset while still performing the two stores. Each successive accepted message 8 sees the day left by preceding dispatched packets. This operation does not enter a Game pending message-7 list and does not wait for its application pass.

Message 3 is immediate remote-entity state application. It extracts the entity identifier from the decoded payload, excludes the local player's identifier, records a nonlocal identifier in the client's current received-identifier set, and looks up that entity in the live World. A missing entity is allocated/initialized and added to the World. The state reader at 0x138E00 then applies the supplied state fields directly to that entity. For ordinary finite values, the dispatcher resets interpolation bookkeeping and synchronizes the relevant position/orientation copies when the entity is new or its inspected +0x180 float is nonpositive. For an existing entity with a positive +0x180 float, it sets the interpolation value at +0xB58 to 60.0 and advances the saved interpolation endpoints. This proves direct state and interpolation-bookkeeping effects during dispatch; the full state-field mask schema is outside this handoff.

Because the ordinary drawn-position pass already ran, message 3 does not cause that whole pass to rerun before rendering. Conversely, the direct synchronization stores in the new/nonpositive case prevent a blanket claim that all message-3 presentation state waits until the next drawn-position pass.

Message 6 is immediate entity-set reconciliation. It visits the current World entity list and collects nonlocal entities whose identifiers are absent from the client's received-identifier set. It then removes those collected entities through 0x2D2260, which unlinks them from the live World's entity list and identifier structure and invokes their destruction path. It clears the client's received-identifier set afterward. These removals occur inside dispatch, before the next snapshot entry. They are not queued in the Game message-7 categories.

Consequently, packets of identifiers 3, 6, 7 and 8 in one snapshot are dispatched in their queued sequence, with immediate 3/6/8 effects occurring between adjacent packets and message-7 category appends occurring at their own positions in that sequence. The evidence does not support regrouping all day packets, all entity-state packets, or all event packets into separate message-type phases before dispatch. Deferred category application is a distinct later concern.

## Outgoing operations and limits

The later portion of 0x3F4F0 includes connection maintenance and conditionally builds/sends local entity state and outgoing message/event types. The sender helper at 0x3F460 uses the remote transport or queues a packet toward the local host through 0xF32C0. The local-host queue used there is a different queue, at host +0x620 with protection at +0x5D0. These sends are not an immediate recursive client-dispatch call. A host response delivered through the client enqueue callback must still enter the protected client incoming queue and await a snapshot that includes it.

The end-of-update 0x3CC40 call only gathers eligible local-player events into outgoing client lists; it is not a second incoming-drain phase. Host receive/update processing at 0xF5520, reached from the worker at 0x83E60, is a separate host-side path. Its existence does not move the proved client dispatch point earlier in this Game invocation.

All message behavior above is bounded to the inspected supported image and ordinary well-formed packet path. Truncated/malformed payload behavior, complete transport guarantees, all possible producer routes, server/client mode alternatives, universal caller discovery, arbitrary concurrent Game updates, and final graphics presentation timing were not established. No implementation advice or replacement algorithm is supplied.

## Original evidence locations

Identity: original binary PE headers and SHA-256; decompiled/summary.json and decompiled/export-start.json beneath the identified original-reference root.

Viewed phase ordering and call ownership: decompiled/assembly-functions/1400a5a50.asm; the successful original retry at decompiled/retry-180-phase1/functions/1400a5a50.c; original decompiled/assembly.txt, calls.csv and function_ranges.csv. The key phase anchors were additionally byte-checked in the identified binary.

Packet queue ownership/order and dispatch: original routines 0x3BAE0, 0x3CB50, 0x3F4F0, 0x3A850, 0x37BA0 and 0x3E450. The dispatch call is 0x3F5B8; lock release before iteration is 0x3F587.

Deferred-list and World application boundaries: original routines 0x38070, 0x38E00, 0x39CE0, 0x7D3F0, 0x83640 and 0x82CD0 together with the viewed update. World drawn-position work is 0x2B68E0. The later World +0x120 visitor is 0x2C3470.

Immediate effects: original routines 0x3E450, 0x2C34C0, 0x8A830, 0x138E00 and 0x2D2260. Outgoing-path distinctions: original routines 0x3F460, 0x3CC40, 0xF32C0, 0x7D140, 0xF5520 and 0x83E60.

The final file must be read completely before implementation uses it. After full readback the author holds all writes to this artifact.
