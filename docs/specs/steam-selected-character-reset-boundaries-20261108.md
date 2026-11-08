# Selected-character entity reset boundaries

This independent, code-free original-side contract covers entity reset RVA 0x64A90 and its reached core reset 0x64C00 at the selected-character decoder's entry. It separates those calls from entity construction, the decoder's later default/copy operations and record reads, and separately reached runtime cleanup. All routine addresses are RVAs in the supported original Steam x64 image.

The supported file is binary/cubeworld.original.exe.unpacked.exe beneath C:/Users/DieuDeGlace/Documents/decompiled cube world steam. Its SHA256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244, Machine 0x8664, image base 0x140000000 and PE timestamp 0x5D93AB0D. These values were checked against the executable and original export metadata. The decoded instructions of both reset bodies, plus selected decoder/copy sites, were compared with the file-backed executable: 158 instruction comparisons matched.

Only the standalone author policy and filename-selected original executable, native/decompiled routines and original metadata were inspected. No implementation, exporter, project tests/results/history/startup/memory, SDK/dependency source, owner captures or previous held artifacts were read. Existing held artifacts were not changed. No implementation guidance is supplied.

E denotes the entity passed to the decoder and reset. P denotes E+0x10, the concrete receiver of the core reset. D denotes the separate CharacterData object addressed by E+0x1528. Every core offset below is translated to an E-relative address; this avoids treating P-relative offsets as whole-entity offsets. Container names are used only where the selected operands support them. A gameplay label does not expand a helper's demonstrated write range.

## Reached call and outer order

Decoder 0x147800 calls 0x64A90 at 0x14784B before its first payload-word read. At this entry there is no record-version, saved-health, class, role or name condition around that call. It assumes a constructed destination with valid container ownership; it is not a null-safe initializer for arbitrary bytes.

RVA 0x64A90 first calls 0x64C00 on P at 0x64AA6. After that returns, the outer routine performs the following sequence:

1. It clears the eight-byte slots E+0x9E0, +0x9D8 and +0xAD8, then the four-byte slot +0x1544.
2. It empties the linked list rooted at E+0xAB8, sets its eight-byte count at +0xAC0 to zero, and frees its former nodes.
3. It empties the tree at E+0xB88 through 0xB810 at 0x64B0B, then the tree at +0xB98 through 0xB810 at 0x64B17, then the tree at +0xB78 through 0x508E0 at 0x64B23.
4. It makes the additional scalar writes listed below.
5. It destroys the active inner inventory buffers through 0x372C0 at 0x64B87 and changes the outer inventory end pointer to its begin pointer.
6. It clears E+0xC18, makes the E+0xC20 range logically empty by writing its end at +0xC28, and finally empties the buff list at +0x980, zeroes its count at +0x988 and frees its former nodes.

The reset contains no World lookup/removal operation, no local-player-pointer reassignment, no call to an entity's deleting virtual function, no CharacterData destruction and no whole-entity memory clear. Clearing a relationship slot or freeing an owned list/tree node does not demonstrate destruction of the entity named by a stored identifier.

The reset bodies themselves do not acquire a lock. Any outer decoder/owner protection is a separate caller property. No exclusion or cross-thread visibility guarantee is inferred from these stores.

## Core reset: exact selected writes

RVA 0x64C00 performs mixed scalar reset, equipment clearing and an appearance-default copy. It does not reconstruct the entire core. The following table inventories its writes relative to E; grouped rows are not a replacement ordering for the native instructions.

| E-relative destination | Width and result |
| --- | --- |
| +0x34, +0x3C, +0x44, +0x4C | Eight bytes zero at each address |
| +0x54 | Four bytes zero, completing the cleared motion area +0x34 through +0x57 |
| +0x68 | One byte zero |
| +0x6C | Four bytes zero |
| +0x130 | Four-byte bits 0xC53B8000, single-precision -3000 |
| +0x134, +0x13C | Eight bytes zero at each address |
| +0x278 | Exactly 0x6E0 consecutive bytes zero, ending before +0x958 |
| +0x128, +0x19C | Two bytes zero at each address |
| +0x70 | Eight bytes zero |
| +0x158 | Four bytes zero |
| +0x184, +0x18C | Eight bytes zero at each address |
| +0x194 | Four-byte bits 0x3F800000, single-precision 1 |
| +0x198 | Four-byte bits 0x41200000, single-precision 10 |
| +0x1A0, +0x1A8 | Eight bytes zero at each address |
| +0x1D0, +0x1D4 | Four-byte -1 at each address, the home pair |
| +0x15C, +0x164, +0x16C | Eight bytes zero at each address |
| +0x1B0, +0x1B4 | Four-byte -1 at each address |
| +0x144 | Eight-byte value 1: four-byte level becomes 1 and following four-byte experience becomes 0 |
| +0x14C | Two bytes zero: the class/specification bytes |
| +0x78 | Appearance defaults copied as described below |
| +0x958, +0x960, +0x968 | Eight bytes zero at each address |
| +0x970 | One byte zero |
| +0x974 | Four bytes zero |
| +0x978 | Eight bytes zero |

The equipment write is a real contiguous byte clear at 0x64C65. It empties the eleven 160-byte equipment blocks including their padding at this reset boundary. This is stronger than merely assigning an empty item type.

After the level/class and coordinate writes, the reset calls 0x4D1F0 on a temporary appearance record at 0x64D2C and assigns its defined fields to E+0x78 through 0x7E780 at 0x64D38. Those defaults mix zero values, 0xFFFF identifier sentinels, float 1 values and other fixed constants; this area is not simply zeroed. The copy handles bytes +0 through +4 and the displayed fields from +8 through +0xAF relative to E+0x78. It does not copy the three padding bytes at relative +5 through +7. The first four-byte default is 0xFFFF0000, byte +4 is 0xFF and the four-byte flags field at relative +8 is zero. The later name/storage clearing follows this appearance copy.

At the return from this reset alone, important core fields remain unchanged: position at E+0x10/+0x18/+0x20; orientation at +0x28 through +0x33; the +0x58 through +0x5F area; byte +0x60; four bytes at +0x64; four bytes at +0x12C; the four-byte pair +0x150/+0x154; +0x174 through +0x183, including health +0x180; the three eight-byte slots +0x1B8/+0x1C0/+0x1C8; and the active item area +0x1D8 through +0x277. This list names selected preserved operands, not a promise that every other unlisted byte was written.

In particular, this reset does not assign or clamp health, does not zero orientation, and does not place the actor from home. It does clear the action byte at +0x68 and its +0x6C four-byte value, but that does not establish every action-related byte as zero. Partial-width fields leave adjacent padding untouched: for example the one-byte action store is not a four-byte write, and the two-byte +0x19C store does not clear +0x19E/+0x19F.

## Outer scalar and relationship writes

Apart from the container operations, 0x64A90 makes these outer writes after the core call:

| E-relative destination | Width and result |
| --- | --- |
| +0x9E0, +0x9D8, +0xAD8 | Eight bytes zero at each address |
| +0x1544 | Four bytes zero |
| +0xBC0 | Eight bytes zero |
| +0xBC8 | Four bytes zero |
| +0x994 | Eight bytes zero |
| +0x9A0 | Four-byte bits 0x3F800000 |
| +0xB58 | Four bytes zero |
| +0x99C | Four bytes zero |
| +0xBB0 | Eight bytes zero |
| +0x9E8 | Four bytes zero |
| +0xBA8 | Four bytes zero |
| +0xBB8 | One byte zero |
| +0xC18 | Eight bytes zero |

Eight-byte writes such as +0x994 and +0xBB0 cover two adjacent four-byte slots. The eight-byte +0xBC0 write and four-byte +0xBC8 write do not clear all neighboring timer/state fields. For example +0xBBC and +0xBCC are not covered by these outer writes. The +0x9A0 assignment must not be substituted for +0x9A4 or +0x9A8: the latter pair is not reset by either routine.

The cleared +0x1A0/+0x1A8, +0x9D8/+0x9E0 and +0xAD8 slots sever only the demonstrated stored relationships/state. No reached operation notifies all referenced objects or removes their reciprocal records. The appearance flags at E+0x80 are reset through the appearance copy, separately from these relationship slots.

## Owned containers and retained storage

| Entity member | Effect of 0x64A90, including its core call |
| --- | --- |
| Buff list +0x980, count +0x988 | Keeps the sentinel/head allocation, makes both links self-referential, sets the eight-byte count to zero and frees every old list node |
| Identifier sets +0x9B8 and +0x9C8, counts +0x9C0 and +0x9D0 | No clear, destruction, head replacement or count write in either reset |
| Outer inventory +0x9F0/+0x9F8/+0xA00 | Frees active inner buffers and empties the outer active range; retains the outer begin allocation and capacity |
| Cursor count +0xA08 and item +0xA0C | No reset write in these two routines |
| Four-byte values +0xAAC/+0xAB0 | No reset write in these two routines |
| Linked work list +0xAB8, count +0xAC0 | Keeps its sentinel/head, zeroes the eight-byte count, unlinks and frees old nodes |
| Tree +0xAC8, count +0xAD0 | No reset clear or replacement; zeroing the separate +0xAD8 slot is not clearing this tree |
| Trees +0xB88/+0xB98, counts +0xB90/+0xBA0 | Frees old nodes, retains each sentinel/head, restores the three sentinel links and zeroes each eight-byte count |
| Tree +0xB78, count +0xB80 | Same empty-tree result through its concrete clear helper |
| Four-byte-element ranges +0xBD0/+0xBD8/+0xBE0 and +0xBF0/+0xBF8/+0xC00 | No reset clear, end adjustment, deallocation or rebuild; the constructor's separate four-byte +0xBE8 value is not reassigned here |
| Identifier set +0xC08, count +0xC10 | No reset clear or replacement |
| Range +0xC20/+0xC28/+0xC30 | Only the end at +0xC28 is set to the begin at +0xC20; allocation, capacity and old bytes remain |
| Embedded owned object beginning +0xC38 | No reset constructor/destructor call or wholesale clear |
| CharacterData pointer +0x1528 | No reset destruction or pointer replacement |
| Linked container +0x15E8, count +0x15F0 | No reset clear or count write |

The inventory destruction helper 0x372C0 walks 24-byte outer descriptors between the prior begin/end. For each nonnull inner allocation it frees the backing buffer, whose capacity arithmetic uses 0xA4-byte elements, and zeros that descriptor's three pointers. The outer reset then writes +0x9F8 equal to +0x9F0. It does not call the outer-buffer deallocator, clear +0xA00, resize back to six, or touch cursor/gold. Invalid count/alignment cases in the helper can reach the displayed nonreturning parameter-error paths; a successful cleanup is assumed for the normal reset boundary.

The tree helpers 0xB810 and 0x508E0 reach 0x87A90 to free subtrees, free the remaining old nodes, and restore the original sentinel's links at +0, +8 and +0x10. The field pointing to that sentinel is retained. Their selected bodies free plain owned nodes; they do not invoke a deleting virtual function on identifiers or entity payloads.

The two +0x9B8/+0x9C8 sets and +0xC08 set are constructed with 0x3BFF0, which allocates a 0x28-byte self-linked sentinel with its sentinel flags. The +0xB78 tree uses 0x4E0E0's 0x28-byte sentinel; +0xB88/+0xB98 use 0x4E140's 0x30-byte sentinel; +0xAC8 uses 0x4E1A0's 0x40-byte sentinel. These constructor facts distinguish ownership and head identity. They do not make this reset equivalent to reconstructing those containers.

The separate entity destructor 0x4D8B0 destroys/frees several containers this reset retains, including +0x9B8/+0x9C8, +0xAC8, +0xC08, both additional four-byte-element ranges, the +0xC20 allocation, the +0xC38 object and +0x15E8. Its +0xC20 deallocation uses a 32-byte element-capacity calculation. That supports a retained 32-byte-element range at +0xC20; this reset merely makes it logically empty. It does not zero or deallocate its previous element bytes.

## Interaction, scored identifiers, cooldown and group boundaries

The +0xAB8 list has a concrete interaction producer in 0x98180: the reached “Sleep” branch constructs a record and appends it through 0x129A0 to the local entity's +0xAB8 list. The reset frees this queued work. It does not demonstrate applying those interactions before deletion, preserving them for later execution, or rebuilding them from the selected character record.

For the action/cooldown-related +0xB78 tree, selected action admission at 0x507B0 searches a signed four-byte action key at node+0x1C and tests the four-byte float at node+0x20; an existing nonzero value can block that action. RVA 0x29D3C0 inserts/locates the same key and writes the float from its reached computed value. Resetting +0xB78 empties that concrete map; it is not evidence that all cooldown state anywhere in the entity is reset.

The +0xB88/+0xB98 maps have concrete identifier/float uses in the event handler 0x29DCF0. It resolves entity identifiers through the World lookup and accumulates float values in those maps under the selected event guards. The selector 0x5F920 scans +0xB88 for a nonzero eight-byte key with a greatest qualifying float, unless its selected buff record supplies an overriding key. These operands support their scored-target/damage-context use. The reset empties both maps and the old buff list; no universal aggro/damage notification or cleanup in other entities follows from that.

For skill/summon/group-related questions, the preserved four-byte-element ranges at +0xBD0 and +0xBF0, the +0xC08 identifier set, the zeroed +0xC18 scalar and the logically emptied +0xC20 range must remain separate facts. No original class/member name or complete semantic mapping for all those members was established in this bounded trace. A request about skills, summons or groups does not imply that their every container is cleared. In particular, the reset contains no call that destroys an entire group or every entity named by a set.

There is a separate later consumer of +0x9B8/+0x9C8 in 0x2DE4C0. Under its reached admission and zero-action-byte branch it compares +0x9C8 identifiers with +0x9B8, resolves selected missing identifiers through the World, emits work, and calls 0x3D6C0 on +0x9B8 at 0x2DFC91 and +0x9C8 at 0x2DFC99. Those later calls really empty both sets. They are not calls made by 0x64A90 or by the decoder's initial reset, and they are not a universal timing guarantee that every loaded entity's sets are immediately cleared. The membership can survive this reset boundary while remaining subject to subsequent independently admitted World work.

## What the decoder does after this reset

A reader result must not be attributed solely to 0x64A90. After the initial reset returns, 0x147800 separately destroys/replaces D, prepares empty equipment and assigns it, assigns an empty inventory/cursor/gold aggregate through 0x83540 at 0x147915, resizes the outer inventory to six at 0x14796F, constructs a default core through 0x4D600 at 0x147978, copies that core to E+0x10 through 0x3B7A0 at 0x147984, and sets byte E+0x60 to zero.

The aggregate assignment includes +0xA08, the defined cursor-item fields at +0xA0C and the two four-byte +0xAAC/+0xAB0 values. Thus they are cleared by this later reader operation, although the two reset routines preserve them. It also frees/replaces the retained outer inventory allocation through its reached 0x4E7A0 cleanup. The saved inventory counts and records later replace the temporary six-entry state.

The default core supplies, among other things, zero position/orientation, health bits 0x43FA0000 (float 500), +0x150/+0x154 zero, +0x184 bits 0x3F800000, and target-coordinate sentinels +0x1B0/+0x1B4 equal to 0x7FFFFFFF. Those differ from the reset-alone preserved fields, zero +0x184, and -1 target sentinels. The core constructor initially puts byte +0x60 at 3; the decoder's following store changes it to zero. The copy also assigns the empty active item at +0x1D8 through the defined-field item-copy routine. These changes are not extra stores inside 0x64C00.

A specific temporary-buffer fact matters for +0x1B8/+0x1C0/+0x1C8. At 0x1478A8 the decoder zeroes the first 0x6E0 bytes of a stack buffer, initially using it for empty equipment. It later passes that same buffer address, RBP+0x60, to 0x4D600. The three source slots subsequently copied by 0x3B7A0 to E+0x1B8/+0x1C0/+0x1C8 lie within this prior zeroed range. Thus the selected reader's later copy supplies zero there, although 0x64C00 itself preserves those destination slots and the generic 0x4D600 body does not directly assign those three source slots. This does not establish that the whole 0x970-byte temporary, every padding byte or a separately constructed entity is zero-initialized.

The subsequent complete-record reads overwrite saved position, health, level/experience, class/specification, +0x9A4/+0x9A8, equipment, name, inventory, +0xAAC/+0xAB0, home and D sections. Saved orientation is consumed then explicitly zeroed. The decoder also destroys/replaces D again after the name read. It does not decode or explicitly clear the +0x9B8/+0x9C8, +0xAC8 or +0xC08 containers, the two retained four-byte-element ranges, the embedded +0xC38 object or +0x15E8 in the selected ordinary body.

At the later buff section, 0x3C9C0 clears the buff list again at 0x148A0E. The reader then appends twenty-four-byte decoded records through 0x4CCB0 at 0x148A86 and increments +0x988. A complete decoded list survives the rest of this reader; the earlier reset does not erase records appended afterward. No expiration filter or health recomputation is part of these two reset routines.

## Constructor and evidence limits

Entity constructor 0x4CE80 creates empty container heads/ranges and a separate core default through 0x4D600. Its +0x9A4/+0x9A8 pair starts as 0x3F800000 and zero, and its +0xBE8 four-byte value starts at 1. Those are constructor facts, not assignments performed whenever 0x64A90 is called. The reset also preserves E's identity at +8, its virtual-table slot, the CharacterData pointer and drawn-position slots beginning +0xB10 until separately reached work changes them.

The demonstrated boundary assumes normal helper returns, valid sentinel/range ownership and noncorrupt entity/container invariants. Neither reset is an exception-safe transactional reload or a repair routine for malformed pointers. Record truncation happens after mutations have already begun; no rollback to the prior entity is supplied by these resets. Later record-length overflow, malformed ownership and untraced indirect effects are outside this contract.

The concrete clear inventory and store lists are deliberately finite. An omitted field is not certified zero, defaulted, harmless, serializable or safe to retain indefinitely. Preserving a tree head/count or old buffer bytes at this boundary does not prove that every later consumer tolerates arbitrary stale contents. There is no exhaustive field-type, gameplay-label, caller, writer, external-reference-cleanup or concurrency census here.
