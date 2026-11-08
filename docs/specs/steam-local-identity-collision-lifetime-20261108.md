# Steam local-identity collision lifetime, 2026-11-08

FINAL / STABLE / HELD. This is a bounded original-only, code-free behavioral audit of the destination collision in World identity helper 0x2A2DE0. Previously held files remain unchanged.

## Reference and exposure

Supported image: C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe. Independently checked SHA-256: 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. PE ImageBase: 0x140000000; TimeDateStamp: 0x5D93AB0D; x64 Machine: 0x8664. Addresses below are RVAs.

Only the source-author policy, filename-selected original-image routines, native assembly, native pointer data, and original-image reference metadata were used. No replacement implementation, SDK/dependency source, project history/specifications/tests, owner outputs/captures, or implementation memory was inspected. No held contract was read or modified for this assignment. No prohibited-material exposure occurred.

The collision call and publication anchors, selected constructor/deleting-target anchors, and the selected virtual-table pointer were checked against the identified executable. Native exports and decompilations were used together. The actual runtime contents of a collision receiver's virtual pointer were not observed.

## Unconditional order in the rekey helper

The local-player pointer is at World +0x448, the entity identifier lookup at World +0x450, and the entity's own 64-bit identifier at entity +0x08.

If World +0x448 is null, 0x2A2DE0 returns before the old-identifier lookup removal, identifier assignment, destination collision call or destination publication. It does not allocate a local player. On the inspected message-4 caller path, the client state-send byte at +0x9E8 is nevertheless set after this helper returns; that is a caller action, not a local-entity creation effect.

With a nonnull local-player pointer, the directly observed order is:

1. Erase the lookup entry for the local player's current identifier through 0x2AC530, at call site 0x2A2E19.
2. Store the supplied identifier in that local entity at 0x2A2E29.
3. Find or create the destination-identifier lookup entry. New entries created through 0x295FC0 have a null mapped entity pointer. The null initialization is explicit at 0x295FF8.
4. If the destination entry contains a nonnull pointer, obtain that object's virtual pointer and invoke its first virtual slot with deletion flag 1, at 0x2A2F33. The receiver comes from the destination entry's mapped pointer at 0x2A2F22; it is not necessarily the retained local player or an object whose concrete type is established by the rekey helper.
5. After that call returns, find or create the destination entry again. Reload the World +0x448 pointer at 0x2A2FAE and write that current pointer into the destination entry at 0x2A2FBA.

The final publication does not occur before deletion returns. The helper does not retain and reuse an earlier destination-entry iterator across deletion. It also reloads World +0x448 for the final pointer value, so an unknown virtual target's effects cannot be ruled out merely by calling that value the retained local entity.

The old local identifier lookup removal is a separate operation from deletion of the destination object. The lookup erasure helpers remove/free lookup nodes; they do not dispatch the mapped entity's virtual deletion. The helper body itself has no explicit write to World +0x448 and no explicit unlink of the collision receiver's World entity-list node.

## A concrete target supported for ordinary message-3 entities

A concrete virtual target can be established conditionally for one source-supported entity provenance.

When the ordinary message-3 path at 0x3E450 receives a nonlocal identifier absent from the World, it allocates an object of size 0x15F8, constructs it through 0x4CE80, and registers it through 0x29D0D0. The constructor explicitly installs the virtual table at RVA 0x431D58. Reading that table from the supported binary shows its first slot points to 0x4E0A0. The registration helper appends the entity pointer to the World list at +0x40, updates the list count at +0x48, and publishes its identifier mapping at +0x450.

The later state reader on this path receives entity +0x10, rather than overwriting the object's initial virtual pointer. Thus this provenance supplies a concrete ordinary entity type for a collision, subject to no intervening replacement or mutation of that object or its virtual pointer.

For such a receiver, the call at 0x2A2F33 reaches 0x4E0A0. That target calls the object-storage destructor at 0x4D8B0 through 0x4E0AF. It then tests the low bit of the supplied deletion flag at 0x4E0B4. Flag 1 from the collision call takes the heap-release operation at 0x4E0C1, with the observed 0x15F8 allocation size.

The storage destructor reinstalls the same virtual table and cleans the receiver's own storage. Its visible work includes cleanup and release of an optional attachment at entity +0x1528, release of owned list/tree/vector storage, cleanup of the contained object beginning at +0xC38, and cleanup of the owned containers associated with positions such as +0x980, +0x9B8, +0x9C8, +0x9F0, +0xAB8, +0xAC8, +0xB78, +0xB88, +0xB98, +0xC08, +0xC20 and +0x15E8. The optional attachment's cleanup is 0x4DB70. Selected nested cleanup helpers release contained allocations and nodes; they do not establish a World-level lifetime notification.

This proves owned-storage destruction followed by release of the receiver allocation for this selected ordinary type. It is not evidence that every possible first virtual slot in World +0x450 has the same target, object size, ownership policy or effects.

A sequence in which message 3 creates an ordinary nonlocal entity under an absent identifier, followed by message 4 assigning that same identifier to the local player without an intervening mutation, would select the ordinary target just described. This is a conditional consequence of source behavior, not evidence that such a collision sequence occurs during normal server traffic.

## World list, lookup and external references

The collision helper does not call the World entity-removal routine at 0x2D2260. The distinction is visible in the original code: 0x2D2260 explicitly searches World +0x40 for the entity pointer, unlinks/frees a matching list node, decrements World +0x48, erases the entity's identifier mapping, and only then invokes its first virtual slot with deletion flag 1. The collision helper instead invokes the virtual slot directly from the destination lookup entry.

No World-list unlink, World-list count decrement, or World lookup erasure is established as an additional effect of the selected ordinary entity storage/deleting target. Its inspected cleanup is for object-owned allocations. Those private lists and maps must not be confused with the World entity list or with pending Game/World event collections.

Accordingly, the proved caller operations are:

- Remove the old local identifier's lookup entry.
- Leave the collision's destination entry available for a post-deletion search unless the selected virtual target changes it.
- After deletion returns, overwrite the destination entry's mapped pointer with the freshly reloaded World +0x448 value.
- Perform no explicit World entity-list unlink or count update in this rekey helper.

For a source-supported ordinary receiver, this audit establishes no automatic removal of its existing World-list node before its allocation is released. It would be incorrect to import the explicit list-removal steps from 0x2D2260 into this different call path. Whether a particular collision is prevented by higher-level protocol/state invariants, or later cleanup removes a stale reference, was not proved.

No direct scan or repair of external event/reference holders was established in the rekey helper or selected ordinary deleting-target chain. This includes no proved global cleanup of Game pending message-7 records, World event groups, other entities' references, camera/selection references or other external pointer/identifier holders. Owned-container destruction inside the receiver is not evidence of that wider cleanup. This statement is an evidence limit; it does not claim a universal inventory of every reference in the program.

For an arbitrary collision receiver with unproved concrete type, only the virtual dispatch and surrounding caller order are unconditional. This audit cannot promise that its target preserves World +0x448, leaves lookup membership unchanged, unlinks a list node, clears references, or even has the ordinary entity deletion semantics. An invalid alias in which a distinct destination key points at the local-player allocation is also outside the normal consistent-lookup assumptions; the call site contains no universal receiver-equality guard that would make that alias safe.

## Rekeying to the already-current identifier

There is no early equality return for a supplied identifier equal to the local player's current identifier. Under a valid uniquely keyed lookup with the ordinary current-local mapping, the helper first erases that key, stores the supplied value back into the local identifier slot, then obtains a destination entry for the same key.

Because the erased key is recreated with a null mapped pointer, this ordinary same-identifier path does not delete the local-player object through the collision call. It finally publishes the current World +0x448 pointer again. The lookup node may be replaced even though the entity's identifier value is unchanged.

This conclusion assumes ordinary valid map state and ordinary synchronous execution without an intervening mutation. It does not establish recovery from a corrupt lookup, allocation failure, reentrant modification, or an unrelated alias under another key.

## Limits and original evidence

The helper is reached by accepted message 4 at 0x3E7A3, and its changes occur synchronously before the next packet is dispatched. This audit concerns that helper and the selected ordinary deleting target. It does not establish a universal concrete receiver type or a universal entity-destructor contract.

Allocation failure, malformed packet inputs, invalid ownership/aliasing, exception/nonreturning paths, unknown virtual targets, concurrent/reentrant World changes, all other deletion callers and runtime occurrence of a collision are not established. No replacement behavior, mitigation or implementation algorithm is supplied.

Original evidence: 0x2A2DE0 and native call/store anchors 0x2A2E19, 0x2A2E29, 0x2A2F22, 0x2A2F33, 0x2A2FAE and 0x2A2FBA; lookup helpers 0x2AC530, 0x2AC450, 0x2A30C0, 0xF8C0, 0x295FC0 and 0x2978A0; ordinary creation/registration at 0x3E450, 0x4CE80 and 0x29D0D0; original virtual-table datum at 0x431D58; selected deleting/storage targets 0x4E0A0, 0x4D8B0 and 0x4DB70; contained-storage helpers including 0x4D840, 0x4E7A0, 0x4E340, 0x4E2C0, 0x372C0, 0x2F2C20, 0xCC140, 0x91000, 0x34F9E0 and 0x36BC40; comparison World removal at 0x2D2260. Original per-routine files, original native assembly/reference metadata and the identified executable support these findings.

The implementer must read this artifact completely before using its facts. After final full readback the author holds all writes.
