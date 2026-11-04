# Steam use completion and creature-copy collection

## 1. Scope and ownership identities

This bounded native-only contract supplements [use-prompt.md §3.3](use-prompt.md#33-abilities-that-complete-here-no-prompt) and [steam-sound-event-addenda.md §1](steam-sound-event-addenda.md#1-two-formerly-computed-ids-are-constants). It describes the automatic ability0x96/0x97 branches in **0x98180**, their collection primitives and directly reached construction/destruction. It establishes neither a generic persistent pet system nor an opening, input or serialization protocol.

The supported original AMD64 image is `cubeworld.original.exe.unpacked.exe`, preferred base **0x140000000**, SHA-256 **7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244**. All cited addresses are RVAs. No game execution or owner data was used.

The Game/controller's **+0x08** points to the client World, whose local creature pointer is at World+0x448. Game **+0x10** points to a separate **0x1f0-byte owner constructed by0x2eaa00**, called WorldMap in the existing map/use prose. This owner keeps the collection at **+0x138**, its count at **+0x140**, and selected creature ID at **+0x148**. The Game's signed32 collection-selection index is a distinct field, **Game+0x20c8**. These are not the client's World+0x450 live-creature registry or the creature's active-pet field.

Ordinary startup allocates that owner before constructing the Game, supplies the client World to owner+0x08, and passes the owner to the Game constructor0x7ea80 as its third argument; see **0x1355a0 →0x2eaa00** and **0x136331 →0x7ea80**. The owner constructor initializes a fresh empty collection through **0x29c2b0** and selected ID0. The Game constructor initializes its separate index to0. These initialization writes do not establish a later collection-loading or save-restoration path.

## 2. Admission and exact completion threshold

The automatic actions run only if **0x98180 is reached and passes its preceding returns**. In order, the handler rejects held dodge at Game+0x1a7a, held walk at +0x1a7d, and visible general-dialog widget Game+0x418. Its earlier Fall and admitted Climb branches also return before these completions. The existing use-prompt suppression/Fall/Climb contract supplies their complete predicates. Completion does **not** require a new Use key edge or a held Use key.

After those gates the routine resolves the ordinary selected creature from Game+0x1e58, then tests automatic completion at **0x984cc–0x9868e**, before the Revive/remaining prompt branches. That earlier selected-creature lookup is separate from the ability-target lookup described below.

For **both0x96 and0x97**, the elapsed value is the player's **float32 ability time at+0x6c**, in milliseconds. The threshold comes from **0x4ead0(player,−1)** while the corresponding ability is still current. The negative argument makes that helper read the ability byte at player+0x68. For these two abilities its reached default path takes **float32 400**, word **0x43c80000** at **0x42e7e8**, divides by the float scalar returned by **0x66460**, then truncates the float32 quotient toward zero to signed32. The caller converts that signed32 threshold to float32 before comparing it to the retained elapsed float. The ordinary finite positive scalar is an external input here; the full stat/speed calculation of0x66460 is outside this supplement.

Completion requires the **strict ordered comparison elapsed greater than threshold**. Equality does not complete; an unordered elapsed comparison does not enter the branch. This is the **effect threshold**, not the whole ability duration supplied by **0x4f220**. The preceding Climb admission's use of0x4f220 does not change which helper these branches call. No arrival-distance, spatial overlap, line-of-sight or target-health test is made at this completion cut.

## 3. Ability0x96: reset, copy, sound, selection

At **0x984f3–0x985d5**, an admitted completion performs these operations in order:

1. Reload the local creature through Game+0x08/World+0x448 and write its ability byte+0x68 to0.
2. Reload the local creature again and write its entire float32 timer word+0x6c to positive zero.
3. Reload the local creature and resolve its **ability target ID at+0x1a8** in the client World's live-creature registry through **0xe720**. The ordinary selected ID at Game+0x1e58 is not substituted.
4. If that target resolves, call **0x2f4290(owner,target)** to add/update the owned copy described in section5.
5. After that operation, submit **sound0x30**, then write owner+0x148 to the source target's64-bit ID at target+0x08.
6. Build the ordered pointer list through **0x93d10**, set Game+0x20c8 to0, and advance that signed32 index once for each list element before the first whose copied creature ID equals the source target ID. Dispose of the temporary pointer-vector storage afterward.

If target resolution fails, the ability and timer resets still happened. There is then **no add, completion sound, selected-ID assignment or index reset**. The ability-target ID itself is not cleared by this branch. No source creature is removed from the client World, reset or transferred into this collection.

The successful scan normally finds the unique newly added/updated ID. The scan has no special not-found recovery: if it reaches the end, the written index is the number of visited entries. It dereferences entries without a separate null-value filter.

## 4. Ability0x97: reset, lookup, erase, reselection

At **0x985da–0x9868e**, this branch separately rereads the current local creature's ability and timer and uses the same strict effect-threshold test. On completion it first resets ability and timer in the same order as0x96. It then passes player+0x1a8 to **0x9c000(owner,id)**.

If the lookup returns a nonnull stored creature pointer, the handler:

1. Calls **0x2f7ef0(owner,storedCreature)**, which removes the map entry and destroys its stored copy.
2. Submits **sound0x3c** through **0xa03f0**.
3. Calls **0x9bf60(Game)** using the **unchanged Game+0x20c8** against the now-shorter collection. It writes owner+0x148 to that returned creature's ID, or to0 if the returned pointer is null.

A missing/null-valued lookup still leaves the player's completed ability and timer reset, but does **not** remove anything, play the completion sound, change selected ID or change the index. Neither completion clears player+0x1a8. Removing a copy does not unregister or delete a live client-World creature with the same ID.

For both successful actions, sound position is the fixed64 XYZ tuple at **Game+0x1dc0**, the camera/listener position, rather than the target's physical coordinates. Volume and pitch are1, with the ordinary final playback flag1. The0x30 call at **0x98561** occurs after copying and before selection writes; the0x3c call at **0x98660** occurs after erasure/destruction and before wrapped reselection. Helper0xa03f0 supplies the same position/default values to0xa0420. This note does not re-audit downstream audio resource selection or its internals.

Neither branch returns from0x98180 merely because it completed. The routine reloads the local creature afterward, beginning with the health/Revive decision at **0x9868e**. Later prompt decisions therefore see the reset ability/time and current player fields. The earlier ordinary selected-creature pointer remains a separate lookup result; completing an ability does not recompute Game+0x1e58 here.

## 5. Add/update and the copy boundary

**0x2f4290** keys the owner's collection by the source creature's entire **64-bit ID at+0x08**. Its insertion helper **0x2992e0** is a unique-key ordered tree operation. An existing ID returns its existing entry; no duplicate-ID entry is appended.

For a new/null-valued entry, add allocates **0x15f8 bytes** and calls **0x4ce80** with the address of the source ID. That constructor installs the creature's normal native base/default state and copies that ID; it does not allocate a different ID for the collection entry. The resulting independent creature pointer becomes the entry value. For an already nonnull entry, this allocation/construction is skipped and the **existing stored pointer is reused**.

In both cases the update then performs these transfers, in order:

|Operation|Established transfer|
|---|---|
|0x3b7a0|Fieldwise value-state assignment from source+0x10 to stored copy+0x10; this is not a whole0x15f8-byte creature copy|
|First extra scalar|32 raw bits at source+0x998 to stored copy+0x998|
|Second extra scalar|32 raw bits at source+0x99c to stored copy+0x99c|
|Third extra scalar|32 raw bits at source+0xc44 to stored copy+0xc44|
|Fourth extra scalar|32 raw bits at source+0xc40 to stored copy+0xc40|

The value-state helper's touched extent ends at creature+0x97f. It includes the physical fixed64 XYZ at+0x10/+0x18/+0x20, ordinary movement/ability/race/health/target fields, the appearance value at+0x78 through **0x7e780**, the original held item at+0x1d8 through **0x9cd0**, and the **eleven0xa0-byte equipped item values beginning+0x278** through **0x3b6b0**. It also copies the sixteen-byte name area beginning+0x958, the64-bit value at+0x968, byte+0x970,32-bit value+0x974 and64-bit value+0x978. The appearance/item assignments are their reached value-copy operations, not a claim that padding throughout that extent is initialized or copied.

This does **not** copy the source's whole live lists, inventory, character-data pointer at+0x1528, cached drawn/pose state or arbitrary fields beyond that extent. New copies keep their constructor defaults for fields not subsequently assigned; duplicate-ID updates keep the existing stored object's other fields. The four separately listed scalars are the proved exception beyond the main value-state extent. No ongoing link or automatic synchronization with later source mutations is established by this copy operation.

### 5.1 Reached construction and RNG bounds

The new-entry route really reaches **0x4ce80**, not merely a byte-copy allocation. Its reached setup includes **0x4d600**, default appearance **0x4d1f0**, default equipment **0x4d300**, and empty-container/head setup before the source assignment. The direct value-copy helpers0x3b7a0/0x7e780/0x9cd0/0x3b6b0 contain no rand/srand calls; the inspected base/default appearance/equipment setup also establishes no such draw or reseed. This does not invoke the separate randomized appearance producer0x53e80 along the directly inspected constructor/copy path.

No randomized fresh-spawn/stat producer is established here. Conversely this bounded audit does **not** supply a universal zero-draw claim for every deeper allocation/helper/CRT failure path: allocator internals and every transitive constructor support routine were not exhaustively traced. New allocation, initialized container heads and constructor defaults must not be inferred away from the word “copy.” A duplicate-ID update bypasses the new-copy constructor. Audio's downstream effects are separately outside this add/copy audit.

### 5.2 Exact primitive value-state transfers

This is a transfer view of **0x3b7a0**, not a reconstructed creature class. In this caller the source and destination value-state bases are respectively **source creature+0x10** and **stored creature+0x10**. Every scalar in the table is copied from the indicated source offset to the **same destination offset**, preserving its bits without numeric conversion, normalization or validation. Ranges below enumerate **starting offsets**, inclusively; the stated width applies at each start. Rows follow native assignment order. A nested operation's extent is its value-record size, not a claim that all bytes in that extent are assigned.

|Offset(s) from value-state base|Offset(s) from creature base|Transfer|
|---|---|---|
|0x000, 0x008, 0x010|0x010, 0x018, 0x020|Three8-byte scalars|
|0x018…0x04c, step4|0x028…0x05c, step4|Fourteen4-byte scalars|
|0x050|0x060|One byte|
|0x054|0x064|One4-byte scalar|
|0x058|0x068|One byte|
|0x05c, 0x060, 0x064|0x06c, 0x070, 0x074|Three4-byte scalars|
|0x068|0x078|Appearance assignment0x7e780; extent0xb0, detailed below|
|0x118|0x128|One2-byte scalar|
|0x11c…0x138, step4|0x12c…0x148, step4|Eight4-byte scalars|
|0x13c, 0x13d|0x14c, 0x14d|Two individual bytes|
|0x140…0x188, step4|0x150…0x198, step4|Nineteen4-byte scalars|
|0x18c, 0x18d|0x19c, 0x19d|Two individual bytes|
|0x190, 0x198|0x1a0, 0x1a8|Two8-byte scalars|
|0x1a0, 0x1a4|0x1b0, 0x1b4|Two4-byte scalars|
|0x1a8, 0x1b0, 0x1b8|0x1b8, 0x1c0, 0x1c8|Three8-byte scalars|
|0x1c0, 0x1c4|0x1d0, 0x1d4|Two4-byte scalars|
|0x1c8|0x1d8|Original-item assignment0x9cd0; extent0xa0, detailed below|
|0x268|0x278|Equipment assignment0x3b6b0; eleven0xa0-byte item extents, total0x6e0|
|0x948…0x957, step1|0x958…0x967, step1|Sixteen successive one-byte transfers|
|0x958|0x968|One8-byte scalar|
|0x960|0x970|One byte|
|0x964|0x974|One4-byte scalar|
|0x968|0x978|One8-byte scalar|

**Appearance0x7e780.** Offsets in this paragraph/table are local to the0xb0-byte appearance value at creature+0x78. Its exact transferred starts and widths are:

|Appearance-relative starts|Width/count|
|---|---|
|0x00…0x04, step1|Five individual bytes|
|0x08, 0x0c, 0x10, 0x14|Four4-byte scalars|
|0x18…0x26, step2|Eight2-byte scalars|
|0x28…0xac, step4|Thirty-four4-byte scalars|

Appearance-relative **0x05…0x07 are not assigned**. In this route those are creature+0x7d…+0x7f. All other bytes of this appearance extent are covered by the listed transfers. There is no raw-resource rebuild, pointer replacement or random choice in this helper.

**Original-item0x9cd0.** The following offsets are local to each0xa0-byte item value. The same transfer applies to the held item and to every equipped item:

|Item-relative starts/span|Width/count|
|---|---|
|0x00|One byte|
|0x04, 0x08, 0x0c, 0x10|Four4-byte scalars|
|0x14|One byte|
|0x18|One4-byte scalar|
|0x1c|One byte|
|0x1d…0x9c|Thirty-two consecutive four-byte attribute slots, each transferred as four individual bytes|
|0x9d|One byte, assigned after the attribute slots|

The attribute transfer covers **all32 slots**, irrespective of the attribute-count byte or which slots a later semantic consumer uses. Item-relative **0x01…0x03, 0x15…0x17 and0x9e…0x9f are not assigned**. This operation does not clear unused attributes or padding. **0x3b6b0** applies0x9cd0 to each of the eleven equipment items in ascending slot order, from local offset0 through0x640 at stride0xa0. Their creature-relative bases are0x278 through0x8b8. Its total extent ends just before the name at creature+0x958; the same unassigned item-local gaps recur in every slot.

**Other gaps and exclusions.** Outside those nested records, the following value-state-relative spans are not assigned by0x3b7a0:

|Value-state-relative unassigned bytes|Creature-relative unassigned bytes|
|---|---|
|0x051…0x053|0x061…0x063|
|0x059…0x05b|0x069…0x06b|
|0x11a…0x11b|0x12a…0x12b|
|0x13e…0x13f|0x14e…0x14f|
|0x18e…0x18f|0x19e…0x19f|
|0x961…0x963|0x971…0x973|

Together with the appearance/item-local gaps, these are the unassigned bytes within the0x970-byte value-state extent. The helper does not assign the creature's vptr or ID before+0x10, or fields at+0x980 and above. The outer add helper's four additional scalar assignments remain exactly those in section5. “Unassigned” means the destination bytes retain their previous contents; it does not establish that they are zero or safe initialized padding. Conversely, every listed transfer takes the source bits even for fields whose producer initialization is unproved.

**Constructor-default reference and limit.** Existing independent [worldgen-creatures.md §7.3](worldgen-creatures.md#73-the-creature-records-defaults-0x4ce80) specifies the ordinary0x4ce80/0x4d600 defaults and the0x4d1f0 appearance defaults. That is the relevant default-state reference, not an all-zero object rule. It explicitly identifies unwritten fields, including creature+0x1b8…+0x1cf, and unwritten appearance padding. In this add path the three8-byte home-position fields in that span are subsequently assigned from the source by0x3b7a0. Unassigned bytes in a fresh copy remain subject to their actual constructor/allocator provenance; in a reused copy they retain that copy's prior contents. This cross-reference does not enlarge section5.1's bounded constructor/allocator/RNG audit or establish save/reset semantics.

## 6. Lookup, order, removal and wrapped selection

The collection's wrapper holds its eight-byte tree-head pointer at **owner+0x138** and unsigned64 entry count at **+0x140**. The reached0x30-byte node has left/parent/right pointers at+0/+8/+0x10, sentinel byte at+0x19, signed64 key at+0x20 and creature pointer at+0x28. The header is a sentinel, not a creature entry. Constructor0x29c2b0 makes its three links self-referential and marks its sentinel state; owner construction sets count0.

**0x9c000** searches using **signed64 key comparisons**. It returns the stored creature pointer for an exact key, otherwise null. A null-valued present key also returns null. No zero/negative-ID rejection or replacement is made by this lookup or the add helper.

**0x93d10** constructs a new temporary contiguous vector of eight-byte creature pointers. It visits the tree from its minimum entry, then follows in-order successors until the header. The result is therefore **ascending signed64 ID order**, not arrival/insertion order, World registration order or an unsigned-ID sort. It appends each entry value, including a null value if one is present. This temporary vector owns its pointer-array storage, not the creatures; it is freed after the reached selection operations.

**0x2f7ef0** obtains a key from its creature argument+0x08, obtains that key's current entry through0x2992e0, saves the entry's stored pointer, erases the key through **0x2ac530**, then invokes the saved object's deleting destructor if nonnull. Thus the owned copy is destroyed **after** its entry is erased. The actual stored value is the object destroyed, not an arbitrary different argument object carrying the same ID. If used directly with an absent key and a valid readable argument, the helper's indexing operation can briefly create a null entry before erasing it; ordinary0x97 avoids this route by its nonnull lookup guard. There is no independent selected-ID/index repair inside the primitive remover.

**0x9bf60** builds that ordered vector afresh. Empty begin/end returns null before any division. Otherwise it reads Game+0x20c8 as **signed32**, wraps locally against the vector's unsigned64 element count and returns the pointer at that wrapped slot. It **does not write the wrapped index back**. For ordinary counts no larger than INT32_MAX, the result is the usual nonnegative modulo slot for either positive or negative signed32 indices, including INT32_MIN. For example a three-entry collection maps indices−1,−3,3,5 to slots2,0,0,2. These are arithmetic examples, not runtime captures.

The precise negative-index staging at **0x9bf86–0x9bfc6** is narrower than an arbitrary unlimited-integer modulo abstraction: negate the low32-bit index, sign-extend that word, use it as an unsigned64 dividend for a quotient by count, increment that quotient, multiply by count and add the original sign-extended index with64-bit word arithmetic, then take an unsigned remainder. The final slot reduction uses the sign-extended low32 bits of that intermediate result and another unsigned64 remainder by count. Nonnegative indices skip the negative adjustment. Very large counts/overflow domains outside the stated ordinary count bound are not assigned a simpler portable normalization rule here.

After a successful0x97 removal the old index is retained. Removing the last entry consequently selects none; removing the formerly last slot from a nonempty list can wrap selection to the first entry. The routine does not search for the old selected ID or decrement the index merely because a lower ID was erased.

## 7. Lifetime and explicit limits

The owner owns the ordinarily produced copies. Its deleting-destructor entry **0x2f27c0** reaches **0x2eb0b0**, which walks this collection in tree order, invokes each nonnull stored creature's deleting destructor, then clears/frees the tree nodes and header. The copy's reached deleting-destructor entry is **0x4e0a0 →0x4d8b0**, followed by object deallocation when its deleting flag is set. Removing a copy or destroying the owner invalidates borrowed pointers to that copy. Reusing an existing ID through add does not by itself replace the object's address.

The directly reached primitives establish no collection lock/acquire protocol, no persisted inventory association and no selection-driven UI opening. This note does not establish every ordinary world/character transition writer, a save/archive/network transfer of the collection, a complete reset policy while the same owner survives, or all indirect collection mutations. Constructor emptiness and destructor ownership do **not** prove either permanent persistence or universal clearing on world changes. Allocation failure, malformed pointers/null entries in unchecked scans, concurrent mutation and deeper constructor/allocator side effects remain outside the ordinary successful contract. No missing-entry recovery, generated replacement companion or serialization format is invented.
