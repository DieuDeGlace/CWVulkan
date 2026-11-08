# Selected-character default-core assignment and copy footprint

This independent, code-free original-side contract audits the default-core assignment reached by selected-character decoding: RVA 0x4D600 on its temporary, then RVA 0x3B7A0 with destination E+0x10 at native call site 0x147984. It covers the complete reached copy footprint, source defaults, preserved gaps, nested appearance/item copies, prior temporary initialization, and differences from reset-alone 0x64C00. All routine addresses are RVAs in the supported original Steam x64 image.

The supported file is binary/cubeworld.original.exe.unpacked.exe beneath C:/Users/DieuDeGlace/Documents/decompiled cube world steam. SHA256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244; Machine is 0x8664, image base 0x140000000, and PE timestamp 0x5D93AB0D. File identity and original export metadata were checked. The decoded instructions of the copy, default constructors, nested copy/default helpers and comparison reset, plus selected decoder sites, were compared with file-backed executable bytes: all 678 instruction comparisons matched.

Only the standalone author policy and filename-selected original executable, native/decompiled routines and original metadata were read. No implementation, exporter, tests/results, project startup/memory/history, SDK/dependency source, owner captures or prior held artifacts were inspected. Held artifacts were not changed. This document supplies no implementation guidance.

E is the destination entity. P is its core destination E+0x10. S is the selected decoder's separate stack buffer used as the source core. A is the appearance destination E+0x78. Where offsets are given relative to an item or A, that relative base is stated explicitly. Bit patterns are little-endian stored values; float interpretations do not assign an unproved gameplay purpose.

## Selected call and source preparation

Decoder 0x147800 first reaches entity reset 0x64A90 at 0x14784B. That separately reaches core reset 0x64C00 and clears selected outer containers. Those mutations precede, and are not performed by, the copy under audit.

After replacing the CharacterData object, the reader zeroes exactly 0x6E0 bytes of S at 0x1478A8. The native destination is the stack address RBP+0x60. It calls equipment-default helper 0x4D300 on that same address at 0x1478B1 and assigns the empty equipment to E+0x278 through 0x3B6B0 at 0x1478C0. Other inventory initialization follows.

At 0x147974 it again selects RBP+0x60 as the receiver, calls 0x4D600 at 0x147978 and passes the returned source pointer to 0x3B7A0. The native second argument at 0x14797D is that returned pointer; the destination argument at 0x147980 is E+0x10. Thus the earlier byte clear applies to the same temporary later used as the core source. It is not a clear of the entity, and its length is not the full core extent.

After 0x3B7A0 returns, the reader writes byte E+0x60 to zero at 0x147989 and only then begins reading the record word. This copy is therefore pre-read default assignment. It is not an after-read normalization pass.

RVA 0x3B7A0 copies same-offset fields from S to P. It does not perform a single 0x970-byte memory copy. It calls the appearance copier at 0x3B848, the active-item copier at 0x3BA57, and the equipment copier at 0x3BA6A. The final sixteen name bytes and trailing scalar fields follow those nested copies. Its normal return value is the destination core pointer. No byte of E's identity or virtual-table slot is part of that destination.

## Complete top-level copy destinations and selected values

The following table inventories every top-level destination group, including nested subareas. The value column describes S as established for this selected decoder call. It does not say that the general copying routine generates those values itself.

| E-relative destination | Concrete copy width | Selected source result |
| --- | --- | --- |
| +0x10, +0x18, +0x20 | Eight bytes at each address | Zero |
| +0x28 through +0x5C, at every four-byte starting address | Four bytes at each address | Zero |
| +0x60 | One byte | 3 during the copy; the reader then writes zero |
| +0x64 | Four bytes | Zero |
| +0x68 | One byte | Zero |
| +0x6C, +0x70, +0x74 | Four bytes at each address | Zero |
| +0x78 appearance area | Defined fields through relative +0xAF; detailed below | Mixed appearance defaults |
| +0x128 | Two bytes | Zero |
| +0x12C | Four bytes | Zero |
| +0x130 | Four bytes | 0xC53B8000, float -3000 |
| +0x134, +0x138, +0x13C, +0x140 | Four bytes at each address | Zero |
| +0x144 | Four bytes | 1 |
| +0x148 | Four bytes | Zero |
| +0x14C, +0x14D | One byte at each address | Zero |
| +0x150 through +0x17C, at every four-byte starting address | Four bytes at each address | Zero |
| +0x180 | Four bytes | 0x43FA0000, float 500 |
| +0x184 | Four bytes | 0x3F800000, float 1 |
| +0x188, +0x18C, +0x190 | Four bytes at each address | Zero |
| +0x194 | Four bytes | 0x3F800000, float 1 |
| +0x198 | Four bytes | 0x41200000, float 10 |
| +0x19C, +0x19D | One byte at each address | Zero |
| +0x1A0, +0x1A8 | Eight bytes at each address | Zero |
| +0x1B0, +0x1B4 | Four bytes at each address | 0x7FFFFFFF |
| +0x1B8, +0x1C0, +0x1C8 | Eight bytes at each address | Zero, dependent on S's earlier byte clear |
| +0x1D0, +0x1D4 | Four bytes at each address | 0xFFFFFFFF, signed -1 |
| +0x1D8 active item | Defined item fields in a 160-byte slot; detailed below | Defined fields zero |
| +0x278 equipment | Eleven 160-byte slots, defined item fields only | Defined fields zero |
| +0x958 through +0x967 | Sixteen successive one-byte copies | Zero |
| +0x968 | Eight bytes | Zero |
| +0x970 | One byte | Zero |
| +0x974 | Four bytes | Zero |
| +0x978 | Eight bytes | Zero |

For example, the eight-byte source defaults at core offsets +0x134 and +0x174 are consumed as separate four-byte copies. They become E+0x144 equal to 1 followed by experience E+0x148 equal to zero, and E+0x184 equal to float 1 followed by E+0x188 equal to zero. The destination write widths must not be inferred from the source constructor's wider stores.

The complete copied footprint contains 2302 distinct destination bytes within the 0x970-byte extent from E+0x10 through E+0x97F. The remaining 114 bytes are omitted by these copies. This count follows the explicit fields, including the nested helpers; it is not evidence of a 2302-byte contiguous transfer.

## Appearance copy and defaults

The appearance copy is RVA 0x7E780, reached with source S+0x68 and destination A, which is E+0x78. It copies five separate one-byte fields at A-relative +0, +1, +2, +3 and +4; four-byte fields at +8, +0xC, +0x10 and +0x14; eight two-byte fields starting at +0x18, +0x1A, +0x1C, +0x1E, +0x20, +0x22, +0x24 and +0x26; and four-byte fields at every four-byte start from +0x28 through +0xAC. These cover 173 bytes of the 176-byte subarea.

RVA 0x4D600 constructs S's appearance through 0x4D1F0. The complete values of the copied fields are:

| A-relative field or group | Selected value |
| --- | --- |
| +0 through +3, each copied separately | Together the four-byte pattern 0xFFFF0000 |
| +4, one byte | 0xFF |
| +0x18 through +0x26, at every two-byte start | 0xFFFF |
| Four-byte fields +0xC, +0x10, +0x14, +0x2C, +0x30, +0x38, +0x44, +0x48, +0x90 | 0x3F800000 |
| Four-byte fields +8, +0x4C, +0x50, +0x54, +0x58, +0x5C, +0x60, +0x64, +0x68, +0x6C, +0x74, +0x84, +0x88, +0x98, +0xA4, +0xA8, +0xAC | Zero |
| +0x28 | 0x3F8147AE |
| +0x34 | 0x3F7AE148 |
| +0x3C | 0x3F733333 |
| +0x40 | 0x3F4CCCCD |
| +0x70 | 0xC0A00000 |
| +0x78 | 0x3F000000 |
| +0x7C | 0x40A00000 |
| +0x80 | 0x40C00000 |
| +0x8C | 0x40400000 |
| +0x94 | 0xC1280000 |
| +0x9C | 0xC1000000 |
| +0xA0 | 0x40000000 |

All individually listed +0x28-and-later values in the lower part of this table are four-byte patterns. Some constructor stores combine adjacent values in eight bytes; the copier still uses the individual widths stated above.

A-relative +5 through +7 are not copied. At the entity those are E+0x7D through +0x7F. Their previous destination bytes remain unchanged even though S's earlier partial clear establishes zero in the corresponding source padding. This distinction also applies to other omitted fields.

## Active item and equipment fields versus padding

The active item begins at E+0x1D8, core-relative +0x1C8. It is assigned through 0x9CD0. The equipment helper 0x3B6B0 reaches that same item copier eleven times, in order, at the following E-relative bases:

+0x278, +0x318, +0x3B8, +0x458, +0x4F8, +0x598, +0x638, +0x6D8, +0x778, +0x818 and +0x8B8.

For the active item and each equipment base, 0x9CD0 has this complete defined-field footprint relative to that base:

- One byte at +0.
- Four bytes at each of +4, +8, +0xC and +0x10.
- One byte at +0x14.
- Four bytes at +0x18.
- One byte at +0x1C.
- All 128 bytes from +0x1D through +0x9C, copied by the native byte loop.
- One byte at +0x9D.

That is 152 bytes in each 160-byte slot. The three bytes +1 through +3, three bytes +0x15 through +0x17 and two bytes +0x9E through +0x9F are omitted. The fixed component area is copied in full; the count/type bytes do not cause the copy routine to skip trailing component bytes or skip an empty item.

RVA 0x4D600 initializes the active item's defined fields to zero, including its 128-byte component area. Its reached 0x4D300 initializes all eleven equipment records' same defined fields to zero. Neither routine's selected body supplies a blanket clear of each full 160-byte slot. The nested copier likewise does not copy the eight padding bytes per slot.

For the active item, the preserved padding in E is therefore +0x1D9 through +0x1DB, +0x1ED through +0x1EF and +0x276 through +0x277. For equipment, those same three relative gap ranges apply at each of the eleven bases listed above.

There is a separate selected-decoder history for destination equipment: the preceding 0x64C00 reset already zeroed all 0x6E0 equipment bytes at E+0x278. The later empty-item assignments preserve that already-zero padding. Thus all equipment bytes can still be zero at this particular pre-read boundary, while the copying routine itself never writes its padding. The corresponding active-item padding was not covered by that equipment clear and does not gain a zero guarantee merely because an empty active item is assigned.

## Complete omitted destination ranges

Outside the twelve item slots, the complete omitted ranges within the destination core extent are:

| E-relative omitted range | Length |
| --- | --- |
| +0x61 through +0x63 | Three bytes |
| +0x69 through +0x6B | Three bytes |
| +0x7D through +0x7F | Three bytes |
| +0x12A through +0x12B | Two bytes |
| +0x14E through +0x14F | Two bytes |
| +0x19E through +0x19F | Two bytes |
| +0x971 through +0x973 | Three bytes |

Those eighteen bytes plus eight bytes in each of the twelve item slots account for all 114 omitted bytes. The copy makes no write to those bytes. There is no hidden whole-core memcpy or trailing padding clear in the reached helpers.

This is a copy-boundary statement. An omitted destination byte might have been written by a different earlier operation, as the equipment example demonstrates. It is not generally assigned zero by this call, and no semantic use or harmlessness of padding is inferred.

## Prior temporary clear versus generic construction

Generic 0x4D600 does not directly write core offsets +0x1A8 through +0x1BF. Nonetheless 0x3B7A0 reads those three eight-byte slots and publishes them as E+0x1B8, +0x1C0 and +0x1C8. The selected decoder has already zeroed them because they lie in the first 0x6E0 bytes of S cleared at 0x1478A8, and the intervening equipment/default operations do not supply nonzero values there. The native copy stores are at 0x3BA1C, 0x3BA2A and 0x3BA38.

This is a selected caller proof of zero, not a value guaranteed by 0x4D600 alone. The entity constructor 0x4CE80 directly calls 0x4D600 on its core and then separately constructs extra entity members. The selected constructor body supplies no identical preceding whole-core clear. Its otherwise unwritten core slots cannot be assigned the decoder's temporary values without further caller evidence.

The generic default constructor also leaves the padding ranges omitted by the copier unwritten. In the selected decoder, some of those source padding bytes fall within the prior partial clear, while later source padding lies beyond it. Their source values do not affect this copy because the copier omits them. In particular, the 0x6E0-byte stack clear must not be extended by inference to the entire 0x970-byte source buffer.

All source bytes that this selected assignment actually copies have demonstrated values from the reached defaults and, for the three otherwise unwritten eight-byte slots, the prior temporary clear. That establishes its written destination values without claiming that every byte of S or E was initialized.

## Separation from reset-alone, identity and later record reads

Reset-alone 0x64C00 and this assignment have overlapping effects but different boundaries. Selected differences include:

- Reset-alone preserves position, orientation and E+0x58 through +0x5F; the selected default assignment copies zeros to their defined fields.
- Reset-alone preserves byte E+0x60 and four bytes at +0x64; the default assignment supplies 3 and zero, followed by the decoder's separate +0x60 zero store.
- Reset-alone preserves E+0x12C and +0x150/+0x154; the selected assignment supplies zero.
- Reset-alone preserves E+0x174 through +0x183, including health +0x180; the selected assignment supplies zeros to +0x174/+0x178/+0x17C and float 500 to +0x180.
- Reset-alone supplies zero at E+0x184; the default assignment supplies float 1 there.
- Reset-alone writes -1 to target-coordinate fields +0x1B0/+0x1B4; this assignment supplies 0x7FFFFFFF.
- Reset-alone preserves +0x1B8/+0x1C0/+0x1C8; this selected assignment supplies the separately proved temporary zeros.
- Reset-alone preserves the active item at +0x1D8; this assignment copies empty defined fields while preserving its padding.
- Reset-alone's equipment operation is a contiguous 0x6E0-byte zero; this assignment's equipment operation is eleven defined-field item copies.

The appearance and name defaults overlap the reset's defaults, but the concrete copy widths remain those documented here. Neither routine establishes that every adjacent padding byte is zero.

This copy starts at E+0x10 and ends with the eight-byte destination field at E+0x978. E's virtual-table slot at +0 and identifier at +8 are excluded. The owned buff list beginning +0x980, values +0x9A4/+0x9A8, identifier sets +0x9B8/+0x9C8, inventory/cursor/gold aggregate beginning +0x9F0, further transient containers, drawn-position fields beginning +0xB10 and CharacterData pointer +0x1528 are also outside its footprint. The reached copy helpers perform field transfers and contain no entity deletion, container-node destruction, allocation or World membership operation. Earlier or later decoder changes to those members are separate calls.

Subsequent record reads overwrite the saved core fields they admit. In particular, the later raw health store at 0x147A68 replaces the pre-read default when its four bytes are available. The default assignment is not a post-read health clamp or rescaling operation. The complete saved orientation is separately consumed and zeroed later; that operation is also distinct from these pre-read zeros.

The demonstrated contract assumes the selected disjoint stack source and valid entity destination, normal helper returns and the supported image. Generic overlapping-source cases, malformed entity ownership, all other callers, later indirect effects and concurrent observers were not audited. Field-copy order is not an atomic publication guarantee. No unmentioned byte is certified zero, safe or irrelevant.
