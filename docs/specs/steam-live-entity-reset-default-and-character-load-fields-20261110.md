# Live entity construction, reset, default core and selected character loading

FINAL / STABLE / HELD source-side behavioral contract, 2026-11-10. This is an independent audit of the supported original. It contains factual prose and field tables, with the bounded evidence and exclusions stated below.

## Authority, scope and notation

The authority is C:/Users/DieuDeGlace/Documents/decompiled cube world steam/binary/cubeworld.original.exe.unpacked.exe, read as data and never executed. Its length is 5,757,952 bytes and its SHA256 is 7AF18CB28C8BF848F355287428C6390B2EA2A682CA466190F5EB9859C5D6F244. The PE machine is AMD64 0x8664, preferred image base 0x140000000, timestamp 0x5D93AB0D, entry RVA 0x3939F0, and SizeOfImage 0x58A000. Addresses below are RVAs unless a preferred-base address is explicitly identified.

The primary owners are 0x4CE80, 0x64A90, 0x64C00, 0x4D600, 0x3B7A0, 0x147750, 0x147800, 0x9BB20, 0x9F7C0, 0x913B0 and 0x9EA10. Their complete exported C bodies were read; every instruction and every owned range recorded for those owners was checked against the original image. The interpretation uses native operands when C presentation omits an argument, widens a store, groups stores, or presents a runtime helper ambiguously. The manifest also distinguishes complete supporting owners from bounded windows in larger callers and the selectively interpreted appearance owner.

E denotes the concrete entity receiver. C denotes its inline core beginning at E+0x10. Thus a core offset in a C table is sixteen bytes earlier than the corresponding entity offset. G is the Game receiver; W is the current pointer read from G+8; the selected live entity is the pointer read from W+0x448. I is the inline Interface receiver at G+0x118. D denotes the separately allocated object addressed by E+0x1528, called CharacterData here to distinguish it from both the inline core and the inventory aggregate. These names identify operand relationships, not an assertion of a recovered C++ declaration.

A byte is one byte, a word two, a DWORD four, and a QWORD eight. Inclusive byte ranges are explicitly called ranges. A list of offsets in a store table is in native store order unless stated otherwise. Repeated offsets with a specified stride mean separate stores at each listed position, not a bulk initialization of intervening gaps. Hexadecimal floating words are exact bit patterns. The useful exact binary32 values include 0x3F800000 = 1, 0x41200000 = 10, 0x43FA0000 = 500, 0x43480000 = 200, 0xC53B8000 = -3000 and 0x47800000 = 65536.

No whole-allocation zeroing is established by an allocator call or by the word reset. The tables identify actual destinations. A field omitted from a particular operation is retained by that operation only, subject to the explicitly described reached helpers and ordinary nonoverlapping, valid-object preconditions. They do not make uninitialized bytes valid, prove a semantic name for an opaque scalar, or establish a universal census of every computed caller or writer.

## Ordinary receiver joins

The Game construction windows establish the receiver without relying on a field-name guess. At 0x7EABF the incoming Game receiver is retained; at 0x7EADE the caller-supplied World pointer is stored at G+8. At 0x7EB14..0x7EB23 the Interface constructor 0xF8690 is called on G+0x118 with G as its second argument. The complete Interface constructor stores that argument at I+0x5C0.

One constructor loop requests 0x15F8 bytes for each stored-character entity, passes a QWORD identifier of zero to 0x4CE80, calls 0x9BB20 with G, the current signed 32-bit loop index and that entity, and appends its pointer to the G+0x1778 vector. This is a stored-character object, not proof that every such object is the live W+0x448 entity.

The later ordinary construction window requests another 0x15F8 bytes and calls 0x4CE80 with identifier one. A bounded imported strcpy_s call receives its E+0x958 destination, size 16, and the image literal Wollay at RVA 0x4360C8. Subsequent explicit preparation and calls are separate from 0x4CE80. That window clears the three QWORD position components, copies them to E+0xB10/+0xB18/+0xB20, obtains another three-QWORD result from 0x2D3DB0 and stores it at E+0x1B8/+0x1C0/+0x1C8, clears the motion-related QWORDs at E+0x28/+0x30/+0x38 and DWORD E+0x5C, sets level E+0x144 to one, and assigns the return of 0x5FAC0 to health E+0x180. It then prepares selected equipment fields, byte E+0x60, class byte E+0x14C, DWORD E+0x64 = 0x101, and a fresh D. It calls 0x53E80 with E+0x64, E+0x78, a null third argument and zero fourth byte. It calls 0x29D0D0 on the current W with E, then explicitly publishes E to the current W+0x448 at 0x7F657. The internals of 0x29D0D0 and 0x2D3DB0 are outside this contract.

The separate Interface setup window calls 0xF9AC0 on I. The selected setup instructions pass I+0x5C0 as callback context and the address 0x9F7C0 or 0x9EA10 as the registered target, with signed event number 3 and both registration bytes one. The selected targets are supplied through controls at I+0x38 and I+0x70 respectively. The complete registration owner 0xF8410 finds or creates the event-key entry and forwards context, target and the selected byte to 0xF8380; with its recursion byte nonzero it repeats registration over the collection returned by 0x32AC80, then destroys that temporary collection. The internal callback representation, actual triggering, UI admission and callback lifetime are not closed here.

The selected setup also calls 0x26FDA0 with G and publishes its return at I+0x60, which is G+0x178. That constructor stores G at the widget's +0x1E8. This proves the current-G joins used by 0x272950, 0x272D90 and their reached call to 0x913B0. Other style-button callbacks registered by that constructor, asset-driven child construction, drawing and general UI traversal are outside the selected entity-field contract.

## Default core 0x4D600

The following table gives the default constructor's explicit native stores and reached initializers, relative to its C receiver. It is also the default construction reached directly by 0x4CE80. Store order matters where later code reuses the same area.

| Order | Core destinations and widths | Value or operation |
|---|---|---|
| 1 | QWORDs +0, +8, +0x10, +0x18, +0x20, +0x28, +0x30, +0x38, +0x40, +0x48 | Zero, in that order |
| 2 | Byte +0x50; DWORD +0x54; byte +0x58; QWORD +0x5C; DWORD +0x64 | 3; zero; zero; zero; zero |
| 3 | +0x68 | Appearance initializer 0x4D1F0, described below |
| 4 | Word +0x118; DWORD +0x11C; DWORD +0x120; QWORD +0x124; QWORD +0x12C | Zero; zero; 0xC53B8000; zero; zero |
| 5 | QWORD +0x134; word +0x13C | One; zero. These establish entity level one, experience zero, and zero class/specification bytes |
| 6 | QWORDs +0x140, +0x148, +0x150, +0x158, +0x160, +0x168 | Zero |
| 7 | DWORD +0x170; QWORD +0x174; QWORD +0x17C; DWORD +0x184; DWORD +0x188; word +0x18C; QWORD +0x190; QWORD +0x198 | 0x43FA0000; 0x000000003F800000; zero; 0x3F800000; 0x41200000; zero; zero; zero |
| 8 | DWORD +0x1A0; DWORD +0x1A4; QWORD +0x1C0 | 0x7FFFFFFF; 0x7FFFFFFF; all one bits |
| 9 | Byte +0x1C8; QWORD +0x1CC; QWORD +0x1D4; byte +0x1DC; DWORD +0x1E0; byte +0x1E4; byte +0x265 | Zero |
| 10 | Range +0x1E5..+0x264 | Explicit 128-byte zero fill |
| 11 | +0x268 | Eleven-record initializer 0x4D300 |
| 12 | QWORD +0x958; byte +0x960; DWORD +0x964; QWORD +0x968; QWORD +0x948; QWORD +0x950 | Zero, in this native order |

In particular, entity health E+0x180 starts with 500 in this operation. Entity name E+0x958..+0x967 is zeroed by the two final QWORDs. Entity E+0x968 is a different QWORD and is zeroed separately. It is not the entity identifier at E+8.

This owner does not initialize the core QWORDs +0x1A8, +0x1B0 or +0x1B8. It does not initialize every gap in its scalar, appearance or item records. The selected decoder's prior temporary-buffer fill, discussed later, supplies some otherwise unwritten source bytes; that is a property of that caller, not of this constructor.

## Appearance and item defined fields

The complete appearance initializer 0x4D1F0 operates on a 0xB0-byte field span, A. It performs the following native writes. All entries in the final two rows remain separate native stores.

| Ordered destination in A | Width | Exact value |
|---|---:|---|
| +0; +4 | DWORD; byte | 0xFFFF0000; 0xFF |
| +8; +0xC; +0x10; +0x14 | Four DWORDs | 0; 0x3F800000; 0x3F800000; 0x3F800000 |
| +0x18, +0x1A, +0x1C, +0x1E, +0x20, +0x22, +0x24, +0x26 | Eight words | Each 0xFFFF |
| +0x28, +0x2C, +0x30, +0x34, +0x38, +0x3C, +0x40, +0x44 | Eight DWORDs | 0x3F8147AE, 0x3F800000, 0x3F800000, 0x3F7AE148, 0x3F800000, 0x3F733333, 0x3F4CCCCD, 0x3F800000 |
| +0x48; +0x50; +0x58; +0x60; +0x68 | Five QWORDs | 0x000000003F800000; then four zeros |
| +0x70, +0x74, +0x78, +0x7C | Four DWORDs | 0xC0A00000, 0, 0x3F000000, 0x40A00000 |
| +0x80; +0x88; +0x8C; +0x90; +0x94; +0x98; +0x9C; +0xA0; +0xA8 | QWORD; six DWORDs; two QWORDs | 0x0000000040C00000; 0; 0x40400000; 0x3F800000; 0xC1280000; 0; 0xC1000000; 0x0000000040000000; 0 |

A+5..+7 is not initialized. The copy helper 0x7E780 copies bytes +0, +1, +2, +3 and +4 separately, then DWORDs +8/+0xC/+0x10/+0x14, the eight words +0x18..+0x26, and separate DWORDs at every four-byte position from +0x28 through +0xAC. It preserves destination bytes +5..+7.

An item record, J, has a 0xA0 stride in the selected core and equipment routes. Its defined-field copy at 0x9CD0 and the corresponding construction copy at 0x35400 have this order: byte +0; DWORDs +4/+8/+0xC/+0x10; byte +0x14; DWORD +0x18; byte +0x1C; the 128 bytes +0x1D..+0x9C in increasing groups of four individual byte copies; and byte +0x9D. They do not copy J+1..+3, +0x15..+0x17 or +0x9E..+0x9F. There is no whole-record copy or pointer ownership transfer in these defined-field helpers.

The complete 0x4D300 initializer applies the same zero-valued defined-field pattern to eleven records at increasing 0xA0 strides. For each record it writes byte +0, QWORD +4, QWORD +0xC, byte +0x14, DWORD +0x18, byte +0x1C, byte +0x9D, then explicitly fills +0x1D..+0x9C with zero. Its eleven record starts correspond to entity offsets +0x278, +0x318, +0x3B8, +0x458, +0x4F8, +0x598, +0x638, +0x6D8, +0x778, +0x818 and +0x8B8. Padding is not written by this helper. The companion 0x3B6B0 invokes 0x9CD0 on those eleven records in the same order.

The separate active-item-shaped core record at C+0x1C8, hence E+0x1D8, is initialized by explicit stores in 0x4D600, not by the eleven-record loop. It is outside the equipment zero-fill performed by core reset.


## Core assignment 0x3B7A0

This helper copies fields from a source core to an existing destination core and returns the destination. It does not construct a new entity, replace E+8, or copy any container beginning at E+0x980. It has no general alias/overlap guard and no lock. The selected decoder and fresh-copy callers supply distinct, live source and destination areas.

The complete ordered destination list is as follows. Source offsets are the same as destination offsets.

| Order | Destination core offsets | Width and operation |
|---|---|---|
| 1 | +0, +8, +0x10 | Three QWORD copies |
| 2 | Every four-byte position +0x18..+0x4C | Separate DWORD copies in increasing order |
| 3 | +0x50; +0x54; +0x58; +0x5C; +0x60; +0x64 | Byte; DWORD; byte; three DWORDs |
| 4 | +0x68 | Appearance defined-field copy 0x7E780 |
| 5 | +0x118; every four-byte position +0x11C..+0x138; +0x13C; +0x13D | Word; eight DWORDs; two separate bytes |
| 6 | Every four-byte position +0x140..+0x188; +0x18C; +0x18D | Nineteen DWORDs; two separate bytes |
| 7 | +0x190; +0x198; +0x1A0; +0x1A4; +0x1A8; +0x1B0; +0x1B8; +0x1C0; +0x1C4 | Two QWORDs; two DWORDs; three QWORDs; two DWORDs |
| 8 | +0x1C8 | Active-item defined-field copy 0x9CD0 |
| 9 | +0x268 | Eleven equipment defined-field copies through 0x3B6B0 |
| 10 | +0x948..+0x957 | Sixteen individual byte copies, increasing destination order |
| 11 | +0x958; +0x960; +0x964; +0x968 | QWORD; byte; DWORD; QWORD |

For example, health E+0x180 is copied as a DWORD, not recomputed or compared. Level and experience are separate DWORD copies; class and specialization are separate byte copies. The sixteen-byte entity name is copied independently from the following QWORD at E+0x968.

Within the core span +0..+0x96F, the copy preserves these destination gaps: +0x51..+0x53, +0x59..+0x5B, appearance padding +0x6D..+0x6F, +0x11A..+0x11B, +0x13E..+0x13F, +0x18E..+0x18F and +0x961..+0x963. It also preserves the item padding described above in the active record and all eleven equipment records. Thus the active-record gaps are +0x1C9..+0x1CB, +0x1DD..+0x1DF and +0x266..+0x267. The equipment gaps repeat at record-relative +1..+3, +0x15..+0x17 and +0x9E..+0x9F. There is no inferred copy or zeroing of those gaps.

## Concrete entity construction 0x4CE80

The selected callers request 0x15F8 bytes, but allocation is outside 0x4CE80 itself. The constructor writes the preferred-base primary table address 0x140431D58 at E+0, copies the caller's QWORD identifier through its second-argument pointer to E+8, and calls 0x4D600 on E+0x10. These are distinct operations. The rest of its explicit initialization follows the order in this table.

| Order | Entity destinations | Exact construction effect |
|---|---|---|
| 1 | +0x980 head, +0x988 count | Clear both QWORDs, call 0x4E260 with null link arguments, publish returned head |
| 2 | QWORDs +0x990, +0x998; DWORD +0x9A0; QWORD +0x9A4 | Zero; zero; 0x3F800000; 0x000000003F800000 |
| 3 | +0x9B8/+0x9C0, then +0x9C8/+0x9D0 | Clear head/count QWORDs, separately construct and publish each head with 0x3BFF0 |
| 4 | QWORD +0x9D8; QWORD +0x9E0; DWORD +0x9E8 | Zero |
| 5 | QWORDs +0x9F0, +0x9F8, +0xA00; DWORD +0xA08 | Zero |
| 6 | Byte +0xA0C; QWORD +0xA10; QWORD +0xA18; byte +0xA20; DWORD +0xA24; byte +0xA28; byte +0xAA9 | Zero |
| 7 | +0xA29..+0xAA8; QWORD +0xAAC | Explicit 128-byte zero fill; zero |
| 8 | +0xAB8/+0xAC0, then +0xAC8/+0xAD0 | Clear head/count QWORDs, construct/publish the first head through 0x9F80 and the second through 0x4E1A0 |
| 9 | QWORDs +0xAD8, +0xB10, +0xB18, +0xB20, +0xB34; DWORD +0xB3C; QWORD +0xB4C; QWORD +0xB54; DWORD +0xB5C | Zero |
| 10 | QWORDs +0xB60, +0xB68, +0xB70 | Zero |
| 11 | +0xB78/+0xB80, then +0xB88/+0xB90, then +0xB98/+0xBA0 | Clear head/count QWORDs and publish separately constructed heads through 0x4E0E0, 0x4E140 and 0x4E140 |
| 12 | DWORD +0xBA8; byte +0xBAC; QWORD +0xBB0; byte +0xBB8; QWORD +0xBBC; QWORD +0xBC4; DWORD +0xBCC; QWORDs +0xBD0, +0xBD8, +0xBE0 | Zero |
| 13 | DWORD +0xBE8; QWORDs +0xBF0, +0xBF8, +0xC00 | One; then zero |
| 14 | +0xC08/+0xC10 | Clear head/count QWORDs, construct and publish head with 0x3BFF0 |
| 15 | QWORDs +0xC18, +0xC20, +0xC28, +0xC30 | Zero |
| 16 | Inline receiver +0xC38 | Call 0x17010 with E as its second argument |
| 17 | QWORD +0x1528; DWORD +0x1530; byte +0x1540; DWORD +0x1544; byte +0x1548; QWORD +0x154C; QWORD +0x1554; byte +0x155C; DWORD +0x1560; byte +0x1564; byte +0x15E5 | Zero; 0x43480000; then zero |
| 18 | +0x1565..+0x15E4 | Explicit 128-byte zero fill |
| 19 | +0x15E8/+0x15F0 | Clear head/count QWORDs, construct/publish a head through 0xA100 |

E+0x9A4 and E+0x9A8 therefore begin as separate binary32 words 1 and 0, even though the constructor uses a single QWORD store. The raw widths of the unaligned fields matter: the constructor's QWORD at E+0xAAC covers the two later independently decoded DWORDs +0xAAC and +0xAB0.

The selected head constructors establish these allocation requests and shapes. 0x4E260 requests 0x28 bytes and writes two links, using the new allocation itself for both when its link arguments are null. This is the head used by the E+0x980 list. 0x9F80 requests 0xD8 bytes and establishes analogous two-link storage for E+0xAB8. 0xA100 requests 0x20 bytes with two links for E+0x15E8. The tree-head helpers 0x3BFF0 and 0x4E0E0 request 0x28 bytes, 0x4E140 requests 0x30, and 0x4E1A0 requests 0x40. Each writes three self-links and word +0x18 = 0x0101. These helpers do not zero every payload byte. Their null-allocation paths call the imported invalid-parameter nonreturning boundary; they do not return an ordinary empty head on failure.

The +0x9F0 inventory aggregate consists, in the selected paths, of three QWORD vector pointers, a DWORD at aggregate +0x18 (E+0xA08), a defined-field item at aggregate +0x1C (E+0xA0C), and two DWORDs at aggregate +0xBC/+0xC0 (E+0xAAC/+0xAB0). The item is the cursor-shaped record. This layout is distinct from the eleven core equipment items. The outer vector has a 0x18 descriptor stride; each inner vector has a 0xA4 counted-item stride, with a DWORD count followed by the item at +4.

For the inline +0xC38 receiver, T, complete owner 0x17010 stores E at T+0, zero at QWORD +8, DWORD +0x10, QWORDs at every eight-byte position +0x18..+0xC8, DWORD +0xD0, QWORDs at every eight-byte position +0xD8..+0x118, and QWORDs +0x128/+0x130. It stores DWORD +0x5B8 = 0x3F800000, QWORD +0x8BC = 0, DWORD +0x8C4 = 0, byte +0x8C8 = 0, and QWORDs +0x8D0/+0x8D8 = 0. It clears head/count QWORDs +0x8E0/+0x8E8, calls 0x17760 for the head, then explicitly fills T+0x5BC..+0x8BB with zero. The internal producer/consumer semantics of this inline owner and its head helper are not closed. Its unwritten gaps are not implied to be zero. The reset owners audited here do not reconstruct it.

In particular, 0x4CE80 leaves E+0x1528 null. A CharacterData allocation belongs to callers, not to this constructor. It also does not run 0x64A90 after construction. Newly allocated bytes outside these explicit destinations and reached field initializers have no default value established by this contract.


## Core reset 0x64C00 and outer reset 0x64A90

0x64A90 calls 0x64C00 on E+0x10 first. Core reset has these ordered stores and calls, relative to C.

| Order | Core destination | Width/value or action |
|---|---|---|
| 1 | +0x24, +0x2C, +0x34, +0x3C; +0x44; +0x58; +0x5C | Four zero QWORDs; zero DWORD; zero byte; zero DWORD |
| 2 | +0x120; +0x124; +0x12C | DWORD 0xC53B8000; zero QWORD; zero QWORD |
| 3 | +0x268..+0x947 | Explicit zero fill of exactly 0x6E0 bytes |
| 4 | +0x118; +0x18C; +0x60; +0x148; +0x174; +0x17C | Zero word; zero word; zero QWORD; zero DWORD; zero QWORD; zero QWORD |
| 5 | +0x184; +0x188; +0x190; +0x198 | DWORD 0x3F800000; DWORD 0x41200000; zero QWORD; zero QWORD |
| 6 | +0x1C0, +0x1C4 | Two separate DWORD stores of 0xFFFFFFFF |
| 7 | +0x14C; +0x154; +0x15C | Three zero QWORDs |
| 8 | +0x1A0, +0x1A4 | Two separate DWORD stores of 0xFFFFFFFF |
| 9 | +0x134; +0x13C | QWORD one; word zero |
| 10 | Temporary appearance, then C+0x68 | Construct appearance with 0x4D1F0, then copy its defined fields through 0x7E780 |
| 11 | +0x948; +0x950; +0x958; +0x960; +0x964; +0x968 | Three zero QWORDs; zero byte; zero DWORD; zero QWORD |

The 0x6E0 fill clears all eleven equipment records, including their padding. This differs from 0x4D300 and 0x3B6B0, which touch only defined item fields. The appearance copy still preserves its three destination padding bytes.

Reset alone preserves the position QWORDs E+0x10/+0x18/+0x20 and the orientation-related twelve bytes E+0x28..+0x33. Its first zero range instead begins at E+0x34. Reset also preserves health E+0x180. It is therefore incorrect to obtain health 500, position zero or zero orientation from 0x64C00 alone. Level/experience become one/zero through the QWORD at C+0x134, class/specification bytes become zero, and both DWORDs of the home-shaped pair E+0x1D0/+0x1D4 become all one bits. The separate pair at E+0x1B0/+0x1B4 likewise becomes all one bits; its default-constructor values were 0x7FFFFFFF instead.

Other important preserved core areas include C+0x50, C+0x54, C+0x140..+0x147 (entity +0x150..+0x157), C+0x164..+0x173 including health, C+0x1A8..+0x1BF, and the active-item-shaped record C+0x1C8..+0x267. These are positive boundary statements for this reset, not permission to assign meanings to all those bytes. Name E+0x958..+0x967 and the separate QWORD E+0x968 are cleared. E+8 is outside the core and remains unchanged.

After core reset returns, the outer 0x64A90 performs the following work in this order.

| Order | Entity destinations or collection | Action |
|---|---|---|
| 1 | QWORD +0x9E0; QWORD +0x9D8; QWORD +0xAD8; DWORD +0x1544 | Zero |
| 2 | List at +0xAB8, count +0xAC0 | Capture old first link; set the retained head's two links to itself; set count to zero; then free old nodes in forward-link order |
| 3 | Trees at +0xB88, +0xB98, then +0xB78 | Clear in that order through 0xB810, 0xB810 and 0x508E0 |
| 4 | QWORD +0xBC0; DWORD +0xBC8; QWORD +0x994; DWORD +0x9A0; DWORD +0xB58; DWORD +0x99C | Zero; zero; zero; 0x3F800000; zero; zero |
| 5 | QWORD +0xBB0; DWORD +0x9E8; DWORD +0xBA8; byte +0xBB8 | Zero |
| 6 | Nested inventory descriptors from current +0x9F0 to current +0x9F8 | Destroy/free inner allocations with 0x372C0; then assign +0x9F8 from +0x9F0 |
| 7 | QWORD +0xC18; QWORD +0xC28 | Zero; then copy the current +0xC20 begin pointer to +0xC28 |
| 8 | List at +0x980, count +0x988 | Capture old first link; restore both retained-head links to the head; set count zero; then free old nodes in forward-link order |

The tree clear owners recursively release right subtrees, advance down left links, free non-sentinel nodes, restore all three retained-head links to the head, and clear the QWORD count. The selected recursive leaf 0x87A90 frees nodes; it does not run an entity virtual destructor on each payload. No generic entity-deletion meaning is attached to these tree clears.

0x372C0 walks 0x18-byte inner-vector descriptors. A nonnull inner begin pointer is deallocated after the selected capacity/alignment checks, and that descriptor's begin/end/capacity QWORDs are cleared. A null begin is skipped rather than causing an unconditional three-QWORD zeroing. 0x64A90 retains the outer vector's begin and capacity fields and merely resets its end to its begin. This is not the later full inventory-aggregate replacement performed by decoding or rebuilding.

Several stores intentionally touch only part of a larger-looking area. QWORD E+0x994 clears bytes +0x994..+0x99B, while +0x990..+0x993 is not touched by that store. DWORD +0x99C is cleared later. QWORD +0xBC0 followed by DWORD +0xBC8 does not clear +0xBBC..+0xBBF or DWORD +0xBCC. DWORD +0xB58 does not clear the earlier four bytes at +0xB54. None of these may be expanded to a larger vector or record clear.

The boundary between reset, decoding and construction is particularly important for the following ownership/state groups.

| Area | 0x64A90 result, including its core reset | Later selected decoder result |
|---|---|---|
| E+0 primary table and E+8 identifier | Retained | Retained |
| Position E+0x10..+0x27 | Retained | Replaced by default core, then copied from a complete record |
| Orientation E+0x28..+0x33 | Retained | Complete input is consumed, then this twelve-byte area is forced to zero |
| Health E+0x180 | Retained | First defaulted to 500, then replaced by the record's four bytes when present |
| E+0x150/+0x154 and other core gaps outside reset's stores | Retained as bounded above | Default-core assignment supplies zero to the copied +0x150/+0x154 fields |
| Name and E+0x968 | Cleared by core reset | Defaulted again; name is subsequently read; E+0x968 has no independent field read here |
| Eleven equipment records | All 0x6E0 bytes zeroed | Later default defined-field copy, then eleven 0xA0 raw reads |
| Active record E+0x1D8 | Preserved | Default defined fields copied from temporary core |
| Buff list E+0x980 | Old nodes freed, retained head, count zero | Cleared again at its read position, then rebuilt from 24-byte records |
| Tree heads/counts E+0x9B8/+0x9C0 and +0x9C8/+0x9D0 | Not cleared or traversed | No direct replacement or clear in 0x147800 |
| E+0x9A4/+0x9A8 | Preserved | Separate four-byte reads later; they are outside core assignment |
| Inventory outer vector | Inner allocations freed; outer storage retained, end reset | Full aggregate move-assignment from an empty temporary then resize/read |
| Cursor count/item E+0xA08/+0xA0C and E+0xAAC/+0xAB0 gold storage | Preserved | Zero-valued aggregate fields installed; gold DWORDs later read |
| E+0xAB8 list | Nodes freed, head/count retained and emptied | No direct reconstruction here |
| E+0xAC8 tree | Retained | No direct reconstruction here |
| E+0xB78/+0xB88/+0xB98 trees | Emptied with retained heads | No direct reconstruction here |
| E+0xC08 tree | Retained | No direct reconstruction here |
| E+0xC20 vector | Begin/capacity retained; end reset to begin, without element cleanup in this owner | No direct reconstruction here |
| Inline owner E+0xC38 | Not reconstructed | Not reconstructed |
| E+0x1528 CharacterData pointer and object | Retained by reset alone | Destroyed/replaced twice by decoder |
| Tail item-shaped area E+0x1548 and list +0x15E8 | Apart from DWORD +0x1544, not reset by this owner | No direct decoder replacement |

This table does not identify every opaque field as an interaction, aggro, damage, cooldown, skill, summon or group value. Where such a label is not established by selected operands, the exact field/container boundary is the supported fact. In particular, clearing a relationship-like QWORD does not prove a corresponding referenced entity was destroyed or notified.

Neither reset owner acquires a World, Game or entity lock itself. Valid ownership, nonnull sentinel heads and ordinary container invariants are preconditions of their observed normal paths. Allocation failures, corrupted links, alternative virtual receivers and exception-unwind cleanup outside the recorded owners are not generalized into a safe-reset guarantee.


## Reader context, CharacterData and decoder preparation

0x147750 is a context initializer, not the entity reader. On its receiver P it stores the second argument as a QWORD at +0, DWORD zero at +0x18, QWORD 0xB at +8, and QWORD 0x2C at +0x10, then returns P. The upper DWORD of the QWORD at +8 initially makes P+0xC zero. The decoder later stores the record's first DWORD specifically at P+0xC; it does not replace the whole QWORD at P+8.

P+0 addresses a separate buffer descriptor, B. The selected read operations reload B through P+0. B+0 is the begin pointer, B+8 the end pointer, B+0x10 the allocation-limit pointer used by cleanup, and B+0x18 the signed 32-bit read cursor. The total byte length is formed from the pointer difference, narrowed to a signed DWORD for the bound comparison. Requested end positions are also formed in 32-bit arithmetic, and cursor addresses use sign extension. For ordinary representable, nonnegative lengths/cursors with no arithmetic overflow, a request is admitted when its requested end is at most the total length. On shortage the cursor is set to the narrowed total length and the field copy is omitted. The reader generally continues. It is not a transactional record decoder with a returned success result.

The separately allocated D has a 0x300-byte request size in these callers. Complete constructor 0x7FF20 writes DWORD +0 = 0, byte +4 = 0, QWORD +8 = 0, word +0x10 = 0xFFFF, and byte +0x12 = 0xFF. It constructs nineteen head/count pairs, in increasing order at +0x18, +0x28, +0x38, +0x48, +0x58, +0x68, +0x78, +0x88, +0x98, +0xA8, +0xB8, +0xC8, +0xD8, +0xE8, +0xF8, +0x108, +0x118, +0x128 and +0x138. Before each head call it clears that pair's head and QWORD count, then publishes the returned head. Their called construction helpers are boundaries except where a selected insertion/cleanup closure is described below.

D+0x148 and D+0x1E8 are separately initialized item-shaped records with the zero-valued defined-field pattern. Their padding is not implicitly initialized. D+0x288 is a zero QWORD. D+0x2A8 and D+0x2D0 are empty narrow-string objects: QWORD length +0x10 is zero, QWORD capacity +0x18 is 15, and the first inline byte is zero. This constructor does not initialize every byte of D, including the DWORD later read at D+0x2C8 and the larger scalar gaps before the strings. A short record therefore cannot be described as leaving every missing D field at zero.

Complete destructor 0x4DB70 first destroys the strings at D+0x2D0 and +0x2A8. It invokes selected range-removal/cleanup owners for +0x138, +0x128, +0x108, +0xF8, +0xE8, +0xD8, +0xC8, +0xB8, +0xA8, +0x98, +0x88, +0x78, +0x68, +0x58, +0x48, +0x38 and +0x28, with head release calls after them. The explicit +0x118 list-node/head cleanup falls between +0x128 and +0x108. The destructor finishes through list cleanup 0xCC140 on +0x18. The transitive range-removal and allocator internals are not all closed here. The selected reader additionally calls the sized deallocation boundary 0x393488 with size 0x300 after invoking this destructor; it does not retain the old D pointer for subsequent restoration.

The complete 0x147800 preparation sequence precedes the first record-bound check:

1. Retain its entity argument as E and call 0x64A90 on that E.
2. If E+0x1528 is nonnull, destroy and deallocate that old D. Request 0x300 bytes, conditionally call 0x7FF20, and publish the result or zero at E+0x1528.
3. Explicitly zero exactly 0x6E0 bytes of a stack temporary, run 0x4D300 on it, and copy those eleven defined-field equipment records to E+0x278 through 0x3B6B0.
4. Form an empty inventory aggregate temporary. Its three vector pointers and cursor-count DWORD are zero, the cursor item receives zero defined fields, and its gold-storage QWORD is zero. Call 0x83540 on E+0x9F0 with this temporary.
5. Attempt cleanup of any nonnull outer-vector allocation still in the temporary, using 0x372C0 and 0xDABE0. On this normal move route 0x83540 has cleared the temporary's three vector pointers.
6. Resize the live outer inventory vector to six descriptors through 0x64F30.
7. Reuse the earlier stack area as a core: call 0x4D600 on it, then call 0x3B7A0 on E+0x10 with the returned core.
8. Store byte E+0x60 = 0. Only then attempt to read the version/header DWORD.

0x83540 first destroys/deallocates the destination aggregate's existing outer vector through 0x4E7A0 when source and destination differ. It transfers the source's three vector QWORDs to the destination and zeros all three in the source. It then copies the cursor-count DWORD, the cursor item's defined fields, and the two gold DWORDs. It is not a whole-aggregate byte copy and it does not transfer source item padding. 0x4E7A0 destroys inner allocations, deallocates the outer allocation and clears its three pointer fields when the begin is nonnull. These operations make decoding's inventory replacement stronger than reset alone.

0x64F30 uses 0x18-byte descriptors. Shrinking destroys the removed inner descriptors and reduces the outer end. Growing requests sufficient storage through 0x4E6D0, constructs newly exposed descriptors as three zero QWORDs through 0x4CD30, and advances the end. 0xA2540 performs the analogous selected resize for 0xA4 counted-item records: shrinking reduces the end, while growth goes through 0x4E600 and zero-initializes the newly exposed count and defined item fields through 0x7C7F0. Both capacity helpers compare the requested addition with spare capacity, check their native maximum lengths, and otherwise request at least the needed length, using a one-and-a-half capacity candidate where admitted. The allocator/reallocation helpers below those selected owners are not a successful-allocation or failure-rollback guarantee.

The temporary-core distinction closes a specific otherwise-unwritten-source issue. Its initial 0x6E0 zero fill begins at the same address later passed to 0x4D600. Therefore C+0x1A8/+0x1B0/+0x1B8 are zero in this selected source temporary even though 0x4D600 does not write them. All defined fields later read by 0x3B7A0 have values supplied by the prior fill, 0x4D600 and its reached field initializers. The initial fill is not 0x970 bytes and does not make every byte of the temporary core zero. Equipment/appearance/item padding excluded by assignment remains excluded; destination preservation is as specified in the copy contract.

## Complete ordered character decode

For the following table, a complete record means all required bytes are present, all lengths/counts used for allocation and copying are representable and nonnegative, the name copy fits the actual destination region, and the ordinary object/container invariants hold. This is a conditional description of the native record path, not a record validator that the original itself supplies. The nominal selected format initializes the context with 0xB, but the actual version gates use the signed DWORD decoded at P+0xC.

| Order | Record consumption or call | Destination and resulting behavior |
|---|---|---|
| 1 | Four bytes | P+0xC version DWORD |
| 2 | 24 bytes | A sixteen-byte store at E+0x10 followed by an eight-byte store at E+0x20, preserving the three QWORD bit patterns |
| 3 | Twelve bytes, then unconditional stores | Copy a QWORD to E+0x28 and a DWORD to E+0x30 when available; then zero those same widths regardless of the read outcome |
| 4 | Four bytes each, in order | Health DWORD E+0x180; level DWORD +0x144; experience DWORD +0x148 |
| 5 | One byte each | Class byte E+0x14C; specialization byte +0x14D |
| 6 | Four bytes each | E+0x9A4, then E+0x9A8 |
| 7 | Eleven calls of 0x1496D0 | Raw 0xA0-byte record reads to the eleven equipment starts in increasing order |
| 8 | Signed length DWORD, then that many bytes | Copy bytes to E+0x958 when the whole request is admitted; unconditionally write a zero byte at E+0x958 plus that signed length |
| 9 | No record bytes | Destroy/deallocate the first freshly created D, request another 0x300 bytes, construct it if nonnull, and publish the second D at E+0x1528 |
| 10 | 20 bytes | Copy sixteen bytes to D+0 first, then a DWORD to D+0x10 |
| 11 | Call 0x53E80 | Arguments E+0x64, E+0x78, the current D pointer and fourth byte zero |
| 12 | Outer count DWORD; for each admitted positive outer count, an inner count DWORD and counted-item bytes | Resize the E+0x9F0 outer vector to the sign-extended count; resize each inner descriptor to its count; for nonzero inner counts request one bulk copy of count times 0xA4 bytes into that inner begin |
| 13 | Four bytes, then four bytes | E+0xAAC, then E+0xAB0, followed by the native low-DWORD adjustment described below |
| 14 | Count DWORD; then count raw 0xA0-byte records for positive signed count | Append defined-field copies of those records to D+0x18 in stream order |
| 15 | Only when signed version is greater than 9 | The versioned collections listed in the next table |
| 16 | Only when signed version is less than 4 | Consume the bounded legacy skip structure described below |
| 17 | Four bytes | D+0x2C8 |
| 18 | Length DWORD and that many bytes | Construct a narrow temporary string, read its content through 0x3E3F0, then assign it to D+0x2D0 through 0x337F0 |
| 19 | Count DWORD, clear, then count eight-byte pairs | Clear D+0x28 through 0x50AE0; insert the pairs through 0x740A0 |
| 20 | Eight bytes | Home pair E+0x1D0/+0x1D4 as one QWORD bit copy |
| 21 | Count DWORD, clear, then 24 bytes for each positive signed count | Clear E+0x980 through 0x3C9C0; append buff records to that retained head in stream order |
| 22 | Count DWORD; each entry eight-byte key then four-byte value | Find/insert D+0xF8 entry through 0x1473E0; store the value DWORD at returned node +0x24 |
| 23 | Count DWORD; each entry sixteen bytes | Compute the pair key described below; find/insert its D+0x108 group and append the record to its list |
| 24 | Count DWORD; each entry eight bytes | Append to the D+0x118 list through 0xF4420 |
| 25 | Outer count DWORD; each group key eight bytes, inner count DWORD, then raw 0xA0-byte records | Find/insert the signed-QWORD key in D+0x138 through 0x7BFF0 and append each decoded item to the returned node's +0x28 list through 0xA17D0 |
| 26 | No record bytes | Destroy the temporary narrow string and return; no final entity reset or health assignment |

The eleven 0x1496D0 calls perform an all-or-omitted 160-byte copy, including padding, when their signed length test admits it. Each successful copy consists of ten sixteen-byte transfers at increasing record offsets +0 through +0x90. The selected 0x1474C0 leaf used later for D+0x138 records has the same ten-transfer order. Their low return byte is one on both the selected short and successful paths and is not tested by 0x147800. The counted inventory bulk reads similarly include every byte of the 0xA4 counted records. By contrast, D+0x18 and D+0x138 receive item defined-field copies from the raw 0xA0 temporaries. Those are different padding contracts.

The name path neither reads UTF-16 nor clamps its length to 16. The physical sixteen-byte name span established by core copy is E+0x958..+0x967; an in-bounds name including its final terminator fits there. The native decoder has no corresponding range guard against a larger or negative length. Even a short input path still attempts the final indexed zero store. Out-of-range, overflow, allocation and malformed-record behavior is not promoted to normal loading.

The gold-storage adjustment is narrower than a whole unsigned-QWORD cap. After the two separate DWORD reads, the native code sign-extends only E+0xAAC to 64 bits and compares that unsigned value with 10,000,000,000, using an unsigned at-most branch. Only when that comparison fails does it store DWORD 0x540BE400 back to E+0xAAC. It does not change E+0xAB0 in that adjustment. Thus every nonnegative signed low DWORD is below the 64-bit threshold, whereas a negative signed low DWORD takes this store. The test is not an unsigned comparison of the assembled E+0xAAC/+0xAB0 QWORD.

The version-greater-than-nine block has the following exact collection order. Every count is a four-byte input; iteration requires its signed value to be positive.

| Order within block | D destination | Input size per entry and selected insertion |
|---|---|---|
| 1, 2, 3 | +0x38, +0x48, +0x58 | Eight-byte signed-DWORD pair; 0x740A0 |
| 4, 5 | +0x88, +0x98 | Twelve-byte signed-DWORD triple; 0x8FD0 |
| 6, 7, 8, 9 | +0xA8, +0xB8, +0xC8, +0xD8 | Eight-byte signed-DWORD pair; 0x740A0 |
| 10 | +0xE8 | Twelve-byte record; allocate a candidate through 0x146AD0, then insert through 0x737F0 using its first two DWORDs as the key |

The selected pair/triple insertion owners compare signed DWORD components lexicographically and suppress equivalent keys. 0x737F0 frees its supplied candidate when that pair key already exists; the third DWORD therefore does not make a second key. Their allocation/tree-balancing leaves are not all closed. This does not establish a comparator for every D tree that happens to share a head constructor.

For signed versions below four, a positive count admits a legacy loop that advances over four bytes, reads a length DWORD, reads/discards a narrow string of that length, then advances over 24 bytes. Each advance saturates the cursor at the selected narrowed end when its bound test fails. This block is not a producer of E fields. Versions 4 through 9 omit both this legacy block and the greater-than-nine block. No version-range rejection or format-migration success result is supplied by the selected owner.

The D+0x108 input is grouped by two signed 32-bit coordinate operands at record +4 and +8. Each is divided by 4096 with truncation toward zero, using the native negative-value bias followed by an arithmetic shift. Those two results form the signed lexicographic pair supplied to 0x7C280. Within a group, 0x6E6C0 appends a payload containing input byte +0 and DWORDs +4/+8/+0xC, leaving the three payload-padding bytes outside its explicit stores. Group order follows the tree's pair lookup/insertion, while list order within a group is the record-arrival order. The selected map lookup 0x7BFF0 at D+0x138 uses a signed 64-bit key; repeated keys select the existing group, to which later inner records are appended. The lower key constructors and balancing paths remain bounded helper dependencies. The D+0x118 append leaf 0x37200 stores the first and second input DWORDs at node +0x10 and +0x14, in that order; its underlying head/allocation helper remains a boundary.

The buff node helper 0x4CCB0 allocates through 0x4E260 and copies three QWORDs into node +0x10/+0x18/+0x20. The caller increments the QWORD count and publishes the new tail links after allocation, subject to its explicit maximum-length check. Its 24-byte payload is not interpreted as an entity destructor or general reference-cleanup request. Old buff nodes were already removed by reset and are cleared again at this later read position; newly read buffs are retained at the decoder's normal return.

For complete fields, health, level, experience, class, specialization and E+0x9A4/+0x9A8 are bit copies with no decoder validation, clamp, rescaling or class-based health recalculation. In particular zero and negative finite health values are not changed by a comparison in 0x147800. Its one later appearance call has the bounded output footprint described below, not a health destination.

Short and malformed input have a different boundary. There is no initial whole-record admission check, and destructive preparation happens before reading even the first DWORD. A short fixed-field read omits that field store, leaves the cursor at the selected end and continues. Some count temporaries have explicit zero initialization, while others can retain prior temporary/register values on a short read. Some record temporaries initialize only defined fields. The name's indexed terminator and later collection insertion can still occur after an omitted read. Negative or overflowing counts can reach unsigned resize/allocation calculations; normal allocation and list invariants are not proved for those cases. The owner returns no Boolean that its callers test for a complete decode. No uniform rollback, universal default-for-all-missing-fields result, or guarantee about malformed memory accesses is established.


## Bounded appearance recalculation and health-source calls

The decoder calls large owner 0x53E80 after reading the second D header. This audit matched its complete 0x53E80..0x5DEAB instruction range and checked its two switch-data regions, but does not specify the numerical appearance result for all its branches. A bounded receiver/write audit is sufficient to separate this call from a health reset.

Its incoming first pointer is retained for the selected DWORD species/selection store; the second pointer is retained as the appearance base. The complete native memory-destination census has no non-stack store through an unrelated base. The only store through the first pointer is a DWORD at that pointer. The appearance writes and reached vector/three-byte helpers stay inside appearance offsets +0..+0xAF. The selected register changes preserve the appearance base; the secondary appearance pointer is fixed at base+0xC. The reached helpers 0x16FB0, 0x16FD0, 0x171C0, 0x4CE70, 0x4DF40, 0x4CBF0 and 0x4E070 were read completely and byte-matched: they copy/construct three DWORDs, scale three binary32 components, copy/construct three bytes, or convert three scalar components to bytes at the passed output. They do not acquire an entity receiver from a process pointer.

For the decoder's ordinary disjoint E+0x64, E+0x78 and D arguments, these output areas exclude E+0x180, E+8, E+0x980, E+0x9B8/+0x9C8, inventory and the D pointer. The call therefore does not supply a reached health clamp, buff clear or entity replacement through those outputs. Numeric appearance values, all category/variant branches, imported random results, malformed aliases and provider side effects are not closed.

The entry has a null-D alternative with five successive direct rand imports before a local header is substituted. Nonnull D uses its signed DWORD at +0, guarded by an unsigned at-most-seven branch, and its byte +4 low bit to choose the corresponding species/selection result. Its eight alternatives use base values 2, 0, 9, 11, 4, 7, 15 and 13 in that order. The later species switch has its own rand callsites. It would be incorrect to turn the entry's five calls into a universal total or claim that nonnull D eliminates every later random call. This contract leaves that large numerical/random closure explicit.

0x5FAC0 is a different, complete owner used to produce a value later assigned to health by new/rebuild callers. Its direct operation order is:

- Start with binary32 100 at RVA 0x42E5BC; use binary32 125 at 0x432938 only when class byte E+0x14C and specialization byte E+0x14D both equal one.
- In order, conditionally add the return of 0x1097B0 for records E+0x638 and +0x6D8 when their first byte is 3; E+0x3B8 when its first byte is 4; +0x458 when 6; +0x4F8 when 5; and +0x598 when 7. Each admitted result is added with a separate binary32 addition.
- Capture DWORD E+0x80. In order, multiply by binary32 10, 1.5 and 15 when bits 9, 17 and 18 respectively are set.
- Traverse the E+0x980 list from its first link to find the first payload whose first byte is 0x16; if found and the resulting payload address is nonnull, multiply by its binary32 value at payload +4.
- Restart at the list's first link and, in list order, multiply by payload +4 for every payload whose first byte is 0x20.
- Finally, when captured flag bit 26 is set, multiply by binary32 word 0x3F8CCCCD from RVA 0x42E538.

The owner retains its E receiver throughout. It captures the flag DWORD after the admitted item-helper calls and captures the list head once; the second list pass uses that same captured head. It returns the resulting binary32 value and does not itself store E+0x180. There is no clamp in this owner. The selected 0x1097B0 body and its downstream call 0x109C50 remain a numerical/provider boundary for these references; no complete equipment-stat result or universal draw count is supplied here. The exact constants 100, 125, 10, 1.5, 15 and 0x3F8CCCCD were read from the original image independently of C labels.

## Storage loader 0x9BB20

This complete caller accepts G, a signed 32-bit numeric slot, and a separate E argument. It retains that E argument and passes it to 0x147800; it does not secretly replace it with a current W+0x448 reread. Thus the same loader is concretely used both for the stored-character constructor objects and for the ordinary live entity.

The sequence is: enter the critical section reached through 0x2ABB70 on current G+8; construct an imported narrow stream and stream the signed slot into it; initialize a local buffer descriptor; enter G+0x1C98; obtain a narrow key through 0xA5870; call 0x670F0 on G+0x2268 to obtain bytes; and clean up the temporary key. If the returned low byte is zero, decoding and the later D-list work are skipped. The normal path then releases G+0x1C98, calls 0x2B9890 on a current reread of G+8, and destroys its buffer/stream temporaries.

On a nonzero storage result, it first calls 0x66D10 on the local bytes, initializes P with 0x147750 and invokes 0x147800 with P and the retained E. The transformation/compression/storage backend and stream formatting internals are not closed here. A nonzero storage result is the caller's admission to decoding; it is not proof that the resulting record is complete, and the decoder's void return is not tested.

After decoding, the loader constructs a zero-defined-field item-shaped temporary, then changes its first byte to 1 and DWORD +4 to 7. Its byte +0x14 is explicitly zero. It tests for a corresponding item in the current E-attached D+0x18 list through 0x5F440. If absent, it allocates a node, copies item defined fields through 0x35400, checks count against 0x1745D1745D1745C, increments the QWORD D+0x20 count and publishes tail links. It does not clear the decoded buff list, assign health, set level to one, or invoke 0x913B0 after decoding.

The selected item-equality owner 0x4DF60 compares the first byte, DWORDs +4/+8/+0xC/+0x10/+0x18, bytes +0x1C/+0x9D/+0x14, and, when byte +0x9D is nonzero, that many four-byte entries starting at +0x1D. Padding is not part of this equality. Bounds for an excessive entry-count byte in malformed data are not established.

The small World lock helpers are concrete: 0x2ABB70 adds 0x3C8 to its World argument and tail-calls the EnterCriticalSection import; 0x2B9890 does the corresponding LeaveCriticalSection call. The loader's storage critical section remains held across transformation, decoding and its D-list append on the normal path. No exception-unwind, scheduler, imported critical-section or storage-provider implementation is specified. Because normal exits reread G+8, this contract does not silently assume that the exit receiver would equal the earlier entry receiver under arbitrary concurrent or reentrant mutation.

## Selection owner 0x9F7C0

This owner first stores DWORD zero through the selected-integer receiver reached from direct node G+0x130: node+0x68, DWORD array +0xE0, signed index +0xAC. It enters the current W lock, captures signed selection G+0x6C0 and stores it to G+0x2264. Its branch is exact: an index below zero, or at least the signed 32-bit element count derived from the G+0x1778/+0x1780 pointer difference with an eight-byte stride, takes the new-selection branch. A currently in-range index takes the existing-character branch. No character-name test substitutes for this index test.

For an existing index it calls 0x9BB20 with that captured index and the current W+0x448 entity. Afterwards it constructs a four-byte payload from a fresh G+0x6C0 read and submits it through 0x672C0 on G+0x2268 under the exact narrow key slot, length 4, at RVA 0x4360D0. This need not equal the captured selection under arbitrary intervening mutation. The body does not explicitly acquire G+0x1C98 around this separate write; any internal storage protection remains a callee boundary.

It then calls 0x107690 on I, the direct guard helper at 0xE8A20, and 0xB2180 on G. The complete native 0xE8A20 body is only three bytes and has no state-store effect; it is not a menu/page transition inferred from a C-injected name. The existing branch next stores binary32 8 at G+0x1DB8 and +0x1DB4, binary64 90 at +0x1D98, QWORD zero at +0x1DA0, binary64 180 at +0x1DA8, binary64 90 at +0x1D80, then copies the current +0x1DA0/+0x1DA8 values to +0x1D88/+0x1D90. Those are Game preparation stores, not writes to the loaded entity's position or health.

The new-selection branch instead first installs binary32 8 at the same two Game DWORDs, binary64 90 at +0x1D98, zero QWORDs at +0x1DA0/+0x1DA8, binary64 90 at +0x1D80 and copies those two zero-valued fields to +0x1D88/+0x1D90. It writes selected-integer one through direct node G+0x170's +0x68 receiver, +0xE0 array and +0xAC signed index.

It then calls 0x64A90 on the current W+0x448 entity. Subsequent explicit entity writes, with the actual repeated current-World/current-entity reloads, are: byte +0x958 zero; byte +0x60 zero; QWORD +0x28 and DWORD +0x30 zero; QWORD +0x34 and DWORD +0x3C zero; DWORD +0x5C zero; DWORD level +0x144 one; call 0x5FAC0 using a captured current E and store its return to that captured E+0x180; byte +0x6D8 = 3; DWORD +0x6DC = 0; byte +0x6F4 = 1; byte +0x638 = 3; DWORD +0x63C = 0; byte +0x654 = 1; byte +0x3D4 = 1; byte +0x60 = 0; byte class +0x14C = 1.

If these reloads retain the entity just reset, its just-cleared equipment/buffs and defaulted appearance flags admit no 0x5FAC0 additions/multipliers at that point, so that particular call returns binary32 100. This is a conditional native result of that admitted path, not a guarantee about a different concurrently substituted receiver. It occurs before the following synchronous style/equipment rebuild, which later assigns health again.

The new branch captures a current E, queries 0x2D1100 using a current W and a result buffer, and stores the returned two DWORDs to that E's +0x1D0/+0x1D4. It then rereads current E. For X it shifts the signed 32-bit home-X value left four bits, adds eight in 32-bit arithmetic, sign-extends the resulting DWORD, and shifts that 64-bit value left sixteen before storing E+0x10. For Y it adds two to home-Y in 32-bit arithmetic, shifts left four, sign-extends and shifts left sixteen before storing E+0x18. It stores QWORD zero at E+0x20.

The next current W/current-E pair supplies a call to 0x35EA0. Each signed 64-bit position is converted to the integer sample coordinate by adding 65535 only when negative, then arithmetic-shifting right sixteen; the fourth argument is zero. The returned binary32 value is multiplied by original binary32 65536 and converted to signed 64 bits by the native truncating conversion, then stored to the E captured for that call at +0x20. Home-search/terrain numerical behavior, out-of-range conversion, floating control state and transitive random/provider work remain outside this contract.

The branch then tests and reloads the current D before destruction, destroys/deallocates a nonnull D, requests/constructs a new D and publishes it through another current-E reread. It sends an empty UTF-16 string to 0x334C50 on G+0x190, calls 0x272950 on G+0x178, then calls 0x272D90 on that current widget. The latter invokes 0x913B0 synchronously; no ordinary frame or draw is required between these direct calls.

Both selection branches finally call 0x2E0B50 on current W and leave through 0x2B9890 on another current-W read. Complete 0x2E0B50 directly clears the D+0x128 tree and rebuilds selected derived entries from the current D+0xA8 tree using reached World queries. It is not a second core/entity reset and has no direct health or buff-list clear. Its classifier/World-query closure is explicitly outside this field contract.

The existing-character branch therefore contains a decoder call, while the new-selection branch contains reset plus new initialization and rebuild. Reset alone does not represent the complete behavior of either branch.


## Synchronous style publication 0x272950 and 0x272D90

On its widget receiver Q, 0x272950 stores QWORD zero at +0x244, DWORD zero at +0x240, QWORD zero at +0x24C, word +0x254 = 0xC8FF, byte +0x256 = 0x32, word +0x257 = 0xFFFF, and byte +0x259 = 0xFF. The QWORD at +0x244 clears both DWORDs +0x244/+0x248; the QWORD at +0x24C clears both +0x24C/+0x250.

0x272D90 enters the World lock through current Q+0x1E8, G+8. It reads signed DWORD Q+0x240 for a guarded eight-way selection. Cases 0..7 respectively store to current E+0x64 the current signed DWORD Q+0x248 plus 2, plus 0, plus 9, plus 11, plus 4, plus 7, plus 15 or plus 13. These are native 32-bit operations; the otherwise branch makes no preliminary E+0x64 store. It then reads only byte Q+0x244, increments that byte and publishes it to current E+0x14C. This is not a DWORD class read or a saturation at class four.

The next guarded selection reads signed DWORD current E+0x64. Its comparison admits only unsigned values through 16 to the option table; other values use the default bounds. The bounds used by this owner are literal numeric intervals, not business labels:

| Current E+0x64 value | First low..high | Second low..high |
|---:|---|---|
| 0 | 0x5A3..0x5A6 | 0x5D1..0x5DA |
| 1 | 0x5A7..0x5AC | 0x5DB..0x5E5 |
| 3 | 0x5C2..0x5C7 | 0x5C8..0x5D0 |
| 4 | 0x97..0x9B | 0x9C..0xA1 |
| 5 | 0xA2..0xA6 | 0xA7..0xAC |
| 7 | 0xAE..0xAF | 0xB0..0xB5 |
| 8 | 0xB6..0xBA | 0xB0..0xB5 |
| 9 | 0x18F..0x193 | 0x194..0x196 |
| 10 | 0x197..0x19B | 0x19C..0x1A1 |
| 11 | 0x5E6..0x5EA | 0x5F3..0x5FA |
| 12 | 0x5EB..0x5EF | 0x5FD..0x600 |
| 13 | 0x601..0x605 | 0x606..0x60A |
| 14 | 0x60B..0x60E | 0x60F..0x612 |
| 15 | 0x1A5..0x1AA | 0x1AB..0x1B0 |
| 16 | 0x1B1..0x1B6 | 0x1B7..0x1BC |
| 2, 6 and otherwise | 0x5AD..0x5B2 | 0x5B3..0x5C1 |

For the first interval it subtracts low from high. A negative signed Q+0x24C is replaced by that difference; it then rereads Q+0x24C and replaces it with zero when it is at least difference plus one. It repeats this sequence for Q+0x250 using the second interval. This is wrap-style selection normalization, not a floating clamp or a general absolute-bound store.

Using current G/W/E/D reloads, it publishes DWORD Q+0x24C to D+8, then DWORD Q+0x250 to D+0xC, bytes Q+0x257/+0x258/+0x259 to D+0x10/+0x11/+0x12, DWORD Q+0x240 to D+0, and byte Q+0x248 to D+4. It constructs a default temporary appearance and copies its defined fields to a captured current E+0x78. It then obtains current E again, calls 0x53E80 with that E+0x64, E+0x78, D and byte zero, calls 0x913B0 with current Q+0x1E8, and leaves the World lock through a current Q/G/W reread.

The preliminary +0x64 computation uses the full Q+0x248 DWORD, whereas the later D+4 publication is one byte and the appearance entry's selected alternate uses that byte's low bit. They must not be conflated for arbitrary widget values. For the directly reached new-selection initialization, Q+0x240/+0x244/+0x248/+0x24C/+0x250 are all zero, and the three published bytes at D+0x10..+0x12 are 0xFF. The other style-button routes and their field-value domains are not closed here.

This call nests a World enter/leave within the outer selection owner's enter/leave. The source establishes that static call nesting; it does not supply a substitute implementation of the imported lock, message pump or scheduler.

## Rebuild owner 0x913B0

The complete rebuild first reads current G+8, then current W+0x448, and passes DWORD E+0x968 to the imported srand boundary. Only the low 32 bits of that QWORD participate. The complete imported provider state/algorithm and interference by other callers are not specified.

It next constructs zero-valued equipment defined fields in an explicitly 0x6E0-zeroed temporary and copies them to the E captured before that equipment-initializer call. It move-assigns an empty inventory aggregate to a freshly reread current E+0x9F0 through 0x83540, then resizes current outer inventory to six descriptors. In order it resizes inner descriptors 0, 1 and 2 to ten records each and descriptors 3 and 4 to thirty each. Descriptor 5 remains empty on the normal freshly emptied route. Cursor count/item and gold-storage defined fields become zero through this aggregate operation.

The equipment/initial carried-item branch reads current class byte E+0x14C. The table records the exact field patterns, without assigning names to numeric item identifiers. An equipment tuple below lists byte +0, DWORD +4, byte +0x1C and byte +0x14 of that 0xA0 item.

| Class byte | Equipment at E+0x6D8 | Equipment at E+0x638 | Equipment at E+0x3B8 | Ordered carried item requests, each quantity one |
|---:|---|---|---|---|
| 1 | 3, 17, 2, 0 | Remains zero-defined | 4, 0, 1, 0 | Two copies of item 3,0,1,0; then item 3,13,1,0 |
| 2 | 3, 6, 2, 0 | Remains zero-defined | 4, 0, 27, 0 | Item 3,7,2,0; then item 3,8,2,0 |
| 3 | 3, 10, 2, 0 | Remains zero-defined | 4, 0, 26, 0 | Two copies of item 3,12,11,0; then item 3,11,2,0 |
| 4 | 3, 3, 1, 0 | 3, 3, 1, 0 | 4, 0, 28, 0 | Two copies of item 3,4,1,0; then item 3,5,1,0 |
| Other | Remains zero-defined | Remains zero-defined | Remains zero-defined | None of the class-specific requests |

In each supported-class branch the +0x6D8 fields are stored first. Class four then stores its +0x638 fields. The +0x3B8 fields follow. Each field store uses the current G/W/E reload shown in the selected body; there is no permanent entity snapshot for the entire routine. The carried-item temporary has zero defined fields apart from the tuple changes. In class four the variant byte remains one for the final identifier-five request; in class one it remains one for identifier thirteen. The other table rows reflect their explicit intermediate variant stores.

After these class-specific requests, all branches make five successive quantity-one requests for item tuple 1,1,0,0, using a newly zero-defined temporary. Every selected request calls 0x4E810 with signed fourth argument -1, so that helper first derives the target descriptor through 0x5F100. For item first byte 3 that result is 0; for first byte 1 it is the default 2. No imported keyboard/action enumeration is involved.

The complete insertion helper has a zero-quantity early return and, after target selection, a zero-item-first-byte early return. It grows the outer vector if the selected descriptor is beyond its current count. First byte 12 with byte +0x1C equal 11 adds quantity to aggregate DWORD +0xBC, while first byte 14 adds it to aggregate DWORD +0xC0. Other admitted items scan the selected counted-item records, remember the first zero-count slot, and first test whether equality permits stacking. An equal stack receives quantity -1 as a direct DWORD all-one store, otherwise a DWORD addition. If no equal stack is chosen, an empty slot receives a defined-field item copy followed by the count store; if no empty slot exists, the helper constructs a counted temporary and appends through 0x61020. The latter growth/allocator closure is bounded.

The complete 0x5F0E0 byte predicate admits stacking exactly for first-byte values 1, 10, 11, 12, 13, 22 and 28, using its guarded immediate bit mask 0x10403C02. Thus on the ordinary freshly initialized path, class-specific type-3 requests occupy separate descriptor-zero records, while the five identical type-1 requests accumulate count five in descriptor two's first available slot. This conclusion is conditional on the stated ordinary fresh aggregate and normal allocation/receiver invariants, not on arbitrary malformed contents or a changed current entity.

For reference, the full target-descriptor helper 0x5F100 returns 3 for bytes 2 and 11; 0 for 3, 4, 5, 6, 7, 8, 9, 15, 25 and 26; 4 for 20 and 21; 1 for 22, 24 and 29; 5 for 23; and 2 otherwise. Its byte remap and six destination entries were read from the image, rather than obtaining these domains from an SDK.

Next, in order, it tests current equipment first byte at E+0x638, then +0x6D8, then +0x3B8 for nonzero. For each admitted record it copies current E+0x150 and +0x154 into that item's DWORDs +0xC and +0x10, rereads current E for the destination, calls rand, and stores the returned DWORD at item +8. The destination entity is captured before that import returns. Neither zero/nonzero admission nor the returned random DWORD is converted into a greater-than-zero count test.

It then captures current E's outer begin/end and walks descriptors in increasing 0x18 strides, with each inner begin/end pair captured for its increasing 0xA4 walk. A record is admitted when its DWORD count is nonzero and its item's first byte is 3 through 9 inclusive or 26. On admission it reloads current E, copies current +0x150/+0x154 into counted-record DWORDs +0x10/+0x14, calls rand and stores its returned DWORD at counted-record +0xC. These are respectively item-relative +0xC/+0x10/+8. Negative nonzero counts would pass this particular count test, but arbitrary malformed vector contents are not otherwise admitted as normal inputs.

Only after those direct random imports does the owner capture current E, call 0x5FAC0 and store the returned binary32 value to that E+0x180. This overwrites earlier health, unlike reset alone and unlike the selected existing-character decode. It then rereads current D, clears only D+0x18's list nodes/count with its retained head, and calls 0x9AFB0 with G and signed second argument zero. It does not replace the whole D at this point.

The direct rand-call ordering in this owner is therefore selected equipment +0x638, +0x6D8, +0x3B8, followed by admitted inventory records in outer/inner traversal order, followed by the health-source call. The preceding style/appearance call and the transitive health/stat/allocator providers do not become a universally specified random sequence. No call to 0x64A90 occurs inside 0x913B0 itself. Standalone rebuilding consequently preserves E+8, position/orientation, name, class/specification, home, the active core item at E+0x1D8, E+0x9A4/+0x9A8, buff-list ownership and the tree-headed collections not explicitly touched. In particular, a preexisting buff list can influence its final 0x5FAC0 result. In the ordinary new-selection route the earlier reset has already cleared that list.

## Rebuild's selected D-list population 0x9AFB0

This complete helper does not clear D+0x18 itself. For each candidate it calls 0x5F440 on a current E; if absent it appends a defined-field copy to that current D list. It reloads the current G/W/E/D across candidates. The common zero-defined item temporary uses its first byte and DWORD +4 as the identifiers below, byte +0x14 as a variant derived from the low byte of its second argument, and otherwise retains the stated zero defined fields.

| Order | First byte | DWORD +4 | Byte +0x14 | Byte +0x1C |
|---|---:|---:|---:|---:|
| 1..7 | 1 | 1, 2, 4, 10, 8, 11, 7, in that order | Low byte of second argument | 0 |

When the signed second argument is zero, the owner continues with the following ordered candidates:

| Order after common entries | First byte | DWORD +4 | Byte +0x14 | Byte +0x1C / condition |
|---|---:|---:|---:|---|
| 1..7 | 28 | 0, 1, 2, 3, 4, 5, 6 | 0 | 0 |
| 8 | 11 | 26 | 0 | 0 |
| 9 | 11 | 22 | 0 | 0 |
| 10 | 11 | 9 | 0 | 28, only if current class byte equals 4 |
| 11 | 11 | 9 | 0 | 27, only if current class byte equals 2 |
| 12 | 11 | 9 | 0 | 26, only if current class byte equals 3 |
| 13 | 11 | 9 | 0 | 26, a second separate current-class-three test and absence check |
| 14..17 | 11 | 10 | 0 | 1, 2, 11, 12, in that order |

The two class-three tests are separate source operations. With an unchanged valid D list the second identical candidate is suppressed by equality after the first has been inserted. No stronger snapshot/reentrancy result is inferred. With a nonzero second argument only the seven common candidates are processed. The helper does not assign health, decode an entity or reconstruct the surrounding containers.


## Confirmation 0x9EA10, saving snapshot and stored-character copy

This complete confirmation owner begins by storing selected-integer zero through direct node G+0x170's +0x68 receiver, +0xE0 DWORD array and signed +0xAC index. It constructs the exact four-unit UTF-16 search string edit (0065, 0064, 0069, 0074), queries 0x333690 on G+0x190 with the recursive/admission byte one, and passes the returned string object to 0x354C30. The query's child-search/callback internals are a boundary here; no generic edit-control behavior or asset-provided maximum name length is assumed.

The complete converter 0x354C30 reads the source's QWORD length at +0x10 and capacity at +0x18; capacity below eight chooses inline UTF-16 storage, otherwise the pointed storage. It initializes an empty narrow result with capacity 15 and length zero. A zero-length source uses the original empty narrow literal at RVA 0x435FBF and zero length. A nonempty source requests capacity for the UTF-16 unit count through 0xA23B0, which preserves the prior string length and terminator rather than expanding the content length. It then calls 0x6EDA0 over the two-byte-unit input span. Complete 0x6EDA0 reads only the low byte of each successive two-byte source unit, appends that byte, updates the QWORD narrow length, writes a trailing zero, and advances the source by two bytes. This is low-byte extraction, not a UTF-8, locale or operating-system text conversion.

After disposing of temporary wide strings, 0x9EA10 takes the narrow inline or pointed storage according to capacity 15 and performs a byte-by-byte copy to a current E+0x958 until and including the first zero byte. It supplies no 16-byte destination-size guard and no nonempty-name check in this owner. It does not clear all remaining name bytes after an earlier terminator. A narrow result may contain embedded zeroes even though its stored QWORD length is longer; the confirmation copy stops at that first zero.

The next direct stores/calls occur in this order:

1. Store DWORD level one to a fresh current E+0x144. There is no corresponding experience-zero or health store in this owner.
2. Read the two home DWORDs from current W+0x448, form their eight-byte pair, and insert it through 0x745E0 into current D+0xA8. The selected insertion compares signed DWORD pairs lexicographically and suppresses equivalent keys.
3. Store selected-integer one through the +0x68 receiver of the node reached from G+0x390, then that object's +0x178 child, using its +0xE0 array and signed +0xAC index. Actual root/display/draw admission beyond this store remains a UI boundary.
4. Derive a signed 32-bit stored-character count from the G+0x1778/+0x1780 pointer difference at eight-byte stride. Submit count plus one as a four-byte value through 0x672C0 on G+0x2268 under narrow key num, length 3, RVA 0x436070.
5. Enter current W's lock; store the captured old count to G+0x2264; call 0xA27B0 on G; then leave through a current W reread.
6. Submit a four-byte value obtained from a fresh G+0x2264 read under narrow key slot, length 4. This is the literal at RVA 0x4360D0, not a guessed persistence-key name.
7. Request and construct another 0x15F8-byte entity with identifier zero, then populate and append that separate entity as described below.
8. Call 0x107440 on I, 0x107690 on I, the direct no-state-store guard 0xE8A20 on I, and 0xB2180 on G. Destroy local buffers and narrow strings and return.

The name, level, home-set insertion and child selected-integer store precede the explicit World enter in step 5. The separately shown storage writes do not acquire G+0x1C98 in this confirmation body. Their provider-internal protection and failure semantics are not inferred.

The selected 0xA27B0 body enters current W's lock itself. When G+0x2264 is negative it leaves and performs no snapshot allocation or storage submission. Otherwise it constructs a separate identifier-zero entity and copies the current live core with 0x3B7A0; explicitly copies DWORDs E+0x9A4 and +0x9A8; constructs a D through 0x7FBB0 from current live D; copies nested inventory through 0x83290; copies cursor count, cursor item defined fields and both gold DWORDs; and copies the current buff list through 0x7CD70. It also invokes selected World-state copy helpers for W+8/+0xC and collections at +0x10/+0x20/+0x30. It leaves the World lock before initializing a serialization context, calling 0x149760 with the entity snapshot, and requesting virtual slot-zero teardown with second argument one on that snapshot.

The World-state temporaries are later destroyed. The selected 0x149760 call receives the context and entity snapshot, with no World-state temporary argument. The serializer's full field inventory, compression transform 0x66C10, storage backend 0x672C0 and virtual teardown closure are outside this assignment's closed numerical contract. This audit does not turn temporary World copies into proof that World state is serialized by that call.

After snapshot serialization it streams the captured signed slot to a narrow key, transforms the byte buffer through 0x66C10, enters G+0x1C98, submits to G+0x2268, releases that critical section and destroys its temporaries. It does not test a storage-return status to produce a Boolean confirmation result. The confirmation body's outer World enter therefore surrounds a nested enter/leave from 0xA27B0, while the actual later storage submission inside 0xA27B0 occurs after its own leave. Imported lock and reentrancy behavior are not supplied by that static ordering.

Complete 0x7FBB0 copies the D header's defined scalar fields and invokes constructors/copy helpers for all nineteen D collections in order. It copies the two D item records by defined fields, DWORDs +0x288/+0x28C, QWORDs +0x290/+0x298/+0x2A0, both narrow strings, DWORD +0x2C8, and DWORDs +0x2F0/+0x2F4/+0x2F8. The transitive collection-copy constructors are not all closed here. In particular, some of those scalar source fields were not initialized by 0x7FF20 or filled by 0x147800. A copy instruction does not establish their value as zero or establish that 0x149760 serializes them. Untraced writers and the actual serializer subset remain explicit boundaries.

The separate entity constructed directly by confirmation in step 7 is more selective than the saving snapshot. Its direct ordered population is:

| Order | Separate destination entity | Source/effect |
|---|---|---|
| 1 | E+0x10 core | 0x3B7A0 from a current live E+0x10 |
| 2 | E+0x9F0 outer/inner inventory | 0x83290 from a newly captured current live E+0x9F0 |
| 3 | DWORD +0xA08; item +0xA0C; DWORDs +0xAAC/+0xAB0 | Cursor count, cursor defined fields, then the two gold DWORDs from that captured live E |
| 4 | List +0x980 | Copy current live buff list through 0x7CD70 unless the container receivers are identical |
| 5 | QWORDs +0xB10/+0xB18/+0xB20 | Copy this new entity's own +0x10/+0x18/+0x20 position values |
| 6 | Pointer +0x1528 | Request/construct a fresh D through 0x7FF20 |
| 7 | D+0; +4; +8; +0xC; +0x10/+0x11/+0x12 | Copy the current live D's DWORD, byte, two DWORDs and three bytes, in that order |
| 8 | D+0x18 list | Copy current live D+0x18 through 0x7CEC0 unless those container receivers are identical |
| 9 | G+0x1778 vector | Append the new entity pointer, increasing the vector end by eight bytes |

Neither the identifier E+8 nor E+0x9A4/+0x9A8 is explicitly copied in this confirmation-owned stored-character entity. Its identifier is the constructor input zero, and its +0x9A4/+0x9A8 retain their constructor words 1/0. Its core health and name are copied. Its non-core tree collections and inline +0xC38 receiver retain their own construction, rather than becoming aliases of the live entity's corresponding containers. Its D collections other than the +0x18 list retain fresh-constructor state; the live D+0xA8 home-set insertion is not separately copied into that fresh D here, even though core home coordinates are copied.

For a newly constructed empty destination, the selected inventory copy 0x83290 requests an outer allocation through 0x868F0, then 0x7C440 constructs each inner vector through 0x7E4C0. That requests inner storage and uses 0x7C510 to copy a DWORD count and defined item fields for each record. Thus the normal fresh-copy route builds separately allocated container storage; it does not transfer the live vector pointers. Generic reuse/shrink/capacity paths in the complete outer helper have their own selected copy/cleanup calls; their lower reallocation/assignment leaves are not all closed.

For a fresh buff-list destination, 0x7CD70 appends separately allocated nodes using 0x4CCB0, copying the 24-byte payload. For fresh D+0x18, 0x7CEC0 appends nodes through 0x6E600 and 0x35400. Their complete generic bodies can reuse existing destination nodes, append missing nodes or remove excess nodes; they preserve the head and adjust the QWORD count and links. The selected fresh-constructor case starts with no such reusable payload nodes.

No explicit store in 0x9EA10 replaces W+0x448 with this appended entity or deletes the retained live entity. It does not call the decoder, 0x64A90 or 0x913B0. Confirmation's stored-character copy, the saving snapshot, a decoded existing entity, and the current live entity are separate ownership cases. The selected allocations have null branches but subsequent ordinary code is not uniformly guarded against a null entity or D; this contract assumes successful ordinary allocation and does not claim a safe rollback or recoverable failure path.


## Reached post-selection helpers and explicit limits

The complete 0x107440 owner rebuilds Interface-held selection rows from the current G+0x1778 vector plus one extra row. It temporarily detaches three selected controls, clears/rebuilds the row collection, allocates each row through 0x26EA60, places the corresponding stored entity pointer or zero at that row's +0x1E8, then republishes the three selected controls. These are row-object entity references. They are not stores to W+0x448 and do not call the character decoder or reset owners.

Complete 0x107690 reads the current live class byte and constructs Interface resource/control requests for the corresponding class-dependent pair, then the four common requests. It publishes the resulting list to its selected widget and destroys the temporary list. The resource/node factories, actual display, callbacks and child lifetimes are not semantically closed. The relevant direct field contract is that this owner reads the class; it does not itself reset the live entity or assign its health, position, name, buff head or inventory pointers.

Complete 0xB2180 has an outer admission: it performs its body only when current live inventory outer begin differs from outer end. On admission it walks current D+0x18 records, supplies them to a reached World-dependent record builder and groups admitted results into six temporary collections. It then performs a five-index World-admission loop with further item construction and imported seeding/draw calls. Its complete direct call ordering ends with six 0xB2EE0 publications for indices 0 through 5, ensures at least seven descriptors in the Game-held vector at G+0x17A8, empties descriptor six by copying its begin at descriptor +0x90 to its end at +0x98, clears tree heads/counts at G+0x1938/+0x1940 and G+0x1948/+0x1950, walks current live inventory counts without a field store in that final counting loop, calls 0x280A40 and 0x280C80 on G+0x340, and releases all six temporaries.

Those are distinct Game-held collections from E+0x9F0. The selected complete 0xB2180 body has no direct E+0x180 assignment, E+0x980 clear, 0x64A90 call, 0x147800 call or 0x913B0 call. Its World-dependent item construction, 0xB2EE0 publication leaves and selected widget helpers are transitive/provider boundaries, not a newly proved universal negative-effect census. This is why the contract states exactly which loaded fields the selected bodies directly preserve, rather than claiming that all future helpers, callbacks or gameplay can never change them.

In the selected existing-character path, the decoder's reconstructed buff list survives its normal return and the loader's directly reached D-list work. There is no subsequent explicit buff clear or direct health normalization in 0x9BB20 or the existing branch of 0x9F7C0. The new-selection route is different: it explicitly resets, rebuilds inventory/equipment and assigns a newly computed health value. Confirmation alone neither decodes nor resets that live entity. Later Continue/world-entry placement, disconnect, world simulation, network updates and alternative callers are outside the selected transition closure.

## Source-only conditional field references

These references follow the described operands under their stated normal preconditions. They are not observations from an original run or a replacement engine.

| Conditional input/state | Result established by the selected operation |
|---|---|
| A valid existing E has health word 0xC0200000, representing -2.5, then receives only 0x64A90 | The reset owners do not write E+0x180; that word is retained |
| A complete record supplies health word 0x00000000 or 0xC0200000 | 0x147800 writes that exact DWORD; it performs no later health clamp/rescale, and 0x9BB20 has no direct health assignment |
| The same entity instead reaches the final health assignment in 0x913B0 | Its former health is replaced by the current 0x5FAC0 return; 0x913B0 has no direct old-health retain/clamp decision |
| The selected decoder's reused core temporary reaches 0x3B7A0 | Its copied core +0x1A8/+0x1B0/+0x1B8 source QWORDs are zero because of that caller's earlier 0x6E0 fill |
| A generic source core receives only 0x4D600, with unknown bytes at +0x1A8..+0x1BF | This constructor does not establish those bytes; later assignment cannot infer their value as zero |
| A retained E is reset and subsequently used unchanged by the new-selection intermediate 0x5FAC0 call before starter-item stores | Its empty equipment/buffs and zero flags make that particular return binary32 100 |
| A confirmation query returns UTF-16 units 0041, 0100, 0042 | Conversion produces three narrow content bytes with a zero in the second position; confirmation copies the first byte and that zero to E+0x958, then stops, despite the narrow object's stored length being three |
| A current live E has nondefault words at +0x9A4/+0x9A8 | The 0xA27B0 snapshot explicitly copies them; the distinct identifier-zero entity made directly by 0x9EA10 retains its own constructor 1/0 words |
| A complete buff sequence contains repeated payloads | The selected decoder appends them in input order; it does not suppress duplicates as its pair/triple tree insertions do |
| Input is too short even for the first version DWORD | Destructive preparation has already happened before that first bound check; no transaction rollback or uniform final malformed-input state is established |

Some selected length/allocator limits are exact constants rather than user-facing limits: outer 0x18-vector maximum count 0x0AAAAAAAAAAAAAAA; inner 0xA4-vector maximum count 0x018F9C18F9C18F9C; D+0x18 item-list maximum 0x01745D1745D1745C; buff-list maximum 0x0666666666666665; D+0x108 per-group list maximum 0x07FFFFFFFFFFFFFE; and the selected eight-byte list maximum 0x0AAAAAAAAAAAAAA9. The selected deallocation helpers also perform the original large-allocation checks at 0x1000 bytes, 32-byte alignment and stored-allocation-base distance 8 through 0x27. These are reached runtime guards, not a complete malicious-record validator. Imported nonreturning/error behavior and unwind paths remain unproved.

## Correspondence method and extent limits

The original PE section headers were used to map each RVA to raw file storage. The image identity above was checked before interpreting selected evidence. For every listed native instruction the hexadecimal encoding in the original instruction export was compared with the corresponding raw image bytes. Each function_ranges.csv owned range was checked from its inclusive first to inclusive last byte for continuous instruction coverage. No instruction mismatch or within-owned-range gap was found.

The primary owners' outer bounds also agree with independently read original PE exception-directory records at RVA 0x554000, length 188,928 bytes, containing 15,744 records. The ten primary owners with an exact-start runtime-function entry have exclusive ends matching their listed final byte plus one. The tiny 0x147750 leaf has no exact-start record in that directory; its 28-byte exported leaf body and terminating return are the bounded authority for it. Fragmented export ranges in some primary owners are shown explicitly below; the twenty one-byte inter-range gaps inside those primary PE extents were separately inspected and consist of breakpoint padding. They are not silently counted as owned exported instructions.

The original evidence files were individually selected beneath C:/Users/DieuDeGlace/Documents/decompiled cube world steam/decompiled. Complete selected C presentations are the files functions/140 plus each six-digit entry RVA plus .c listed in the complete-owner manifest. The authority instruction file is assembly.txt; original functions.csv, function_ranges.csv and calls.csv supply original export status, ranges and resolved-call metadata. No reconstruction, SDK headers, exporter implementation or project-owned implementation/test/output file was used.

Coverage comprises 101 complete supporting/primary native owners with 161 exported owned ranges, 11,512 instructions and 47,286 code bytes; the mechanically complete but selectively interpreted 0x53E80 owner with 7,943 instructions and 41,004 code bytes; ten caller windows with 199 instructions and 947 bytes; and 26 additional census-only direct calls with 130 bytes. The nonoverlapping total is 19,680 instruction encodings and 89,367 code bytes. The direct-call census checks 45 concrete callsite instructions in total, some already inside those complete owners/windows. Data tables, literals, PE metadata and any separately inspected padding are not included in that code-byte count.

Full native coverage of 0x53E80 is not a claim to a full numerical appearance specification. Its interpreted scope here is entry admission, concrete receiver/write footprint, selected original helper footprint and the distinction from health/buff reset. Likewise, complete source/body reads of supporting UI, save and post-selection owners do not close every transitive factory, serializer, backend, item-generation or widget implementation they call. The field and ordering claims above identify those boundaries expressly.

The 0x5F100 switch's six four-byte destinations at 0x5F14C and 28-byte remap at 0x5F164 were checked against the raw image. The two 0x272D90 destination tables at 0x2731B4 and 0x2731D4 contain eight and seventeen DWORD entries. The 0x53E80 tables at 0x5DEAC and 0x5DECC contain eight and 306 DWORD entries; all listed destinations correspond to original instructions. These tables are independent data correspondence, not extra function-body bytes. The exact original scalar data constants used for the health-reference description and terrain multiplication, plus the narrow num/slot/Wollay/empty and UTF-16 edit literals, were read directly from the same image.

The resolved-call census is limited to original calls.csv references whose target is one of the eleven starting owners, reconciled with the actual original call instructions. It is not a universal scan for all possible indirect, address-computed, injected or dynamically selected calls. The original instruction text contains one selected address-taking instruction for 0x9F7C0 and one for 0x9EA10, both in the bounded setup windows described above. Absence of direct calls to those callbacks is not evidence that they are unused.

The source does not establish all later writers, packet/inter-thread effects, allocation-provider internals, original imported random algorithm, string/stream locale state, floating control state, message-pump reentrancy, full lifetime synchronization, failure unwinding or behavior for corrupted pointers/containers and arithmetic-overflow records. It also does not establish every participant's use of the shown critical sections. Static store/call order and current receiver reloads are proved; a wall-clock schedule, atomic transition or lack of intervening provider activity is not.


## Complete-owner evidence manifest

Every row identifies a completely read selected original C presentation and its byte-matched native owned ranges. All corresponding C files lie in the original functions directory. For example, RVA 0x4CE80 uses functions/14004ce80.c, and RVA 0x147800 uses functions/140147800.c. Each remaining filename uses that row's preferred-base entry address in the same lower-case spelling. Range endpoints below are inclusive. PE end is exclusive and is supplied only when an original runtime-function record begins at that exact RVA. A missing exact-start entry is not filled from a neighboring record. Semantic claims remain bounded to the selected effects described in this contract.

| Entry RVA | Inclusive owned RVA ranges | Instructions | Code bytes | Exact-start PE end |
|---|---|---:|---:|---|
| 0x8FD0 | 0x8FD0..0x9201 | 173 | 562 | 0x9203 |
| 0x9CD0 | 0x9CD0..0x9D5E | 38 | 143 | No exact-start record |
| 0x9F80 | 0x9F80..0x9FB7; 0x9FB9..0x9FDD | 26 | 93 | 0x9FDE |
| 0xA100 | 0xA100..0xA137; 0xA139..0xA15D | 26 | 93 | 0xA15E |
| 0xB810 | 0xB810..0xB88D | 35 | 126 | 0xB88E |
| 0x16FB0 | 0x16FB0..0x16FC3 | 8 | 20 | No exact-start record |
| 0x16FD0 | 0x16FD0..0x16FE1 | 5 | 18 | No exact-start record |
| 0x17010 | 0x17010..0x17165 | 67 | 342 | 0x17166 |
| 0x171C0 | 0x171C0..0x171E5 | 10 | 38 | No exact-start record |
| 0x35400 | 0x35400..0x3549F | 44 | 160 | No exact-start record |
| 0x37200 | 0x37200..0x37234 | 16 | 53 | 0x37235 |
| 0x372C0 | 0x372C0..0x3738F; 0x37391..0x37396; 0x37398..0x3739D; 0x3739F..0x373A4; 0x373A6..0x373AB | 60 | 232 | 0x372D3 |
| 0x3B6B0 | 0x3B6B0..0x3B790 | 41 | 225 | 0x3B791 |
| 0x3B7A0 | 0x3B7A0..0x3BAD4 | 164 | 821 | 0x3BAD5 |
| 0x3BFF0 | 0x3BFF0..0x3C01B; 0x3C01D..0x3C042 | 21 | 82 | 0x3C043 |
| 0x3C9C0 | 0x3C9C0..0x3CA0A | 22 | 75 | 0x3C9E6 |
| 0x3E3F0 | 0x3E3F0..0x3E440 | 27 | 81 | 0x3E441 |
| 0x4CBF0 | 0x4CBF0..0x4CC0C | 8 | 29 | No exact-start record |
| 0x4CCB0 | 0x4CCB0..0x4CCEA | 16 | 59 | 0x4CCEB |
| 0x4CD30 | 0x4CD30..0x4CD76 | 19 | 71 | 0x4CD77 |
| 0x4CE70 | 0x4CE70..0x4CE7D | 5 | 14 | No exact-start record |
| 0x4CE80 | 0x4CE80..0x4D1EB | 169 | 876 | 0x4D1EC |
| 0x4D1F0 | 0x4D1F0..0x4D2F3 | 44 | 260 | No exact-start record |
| 0x4D300 | 0x4D300..0x4D5F2 | 131 | 755 | 0x4D5F3 |
| 0x4D600 | 0x4D600..0x4D79E | 71 | 415 | 0x4D79F |
| 0x4DB70 | 0x4DB70..0x4DF34 | 225 | 965 | 0x4DF35 |
| 0x4DF40 | 0x4DF40..0x4DF56 | 8 | 23 | No exact-start record |
| 0x4DF60 | 0x4DF60..0x4E062 | 73 | 259 | 0x4DFF2 |
| 0x4E070 | 0x4E070..0x4E095 | 10 | 38 | No exact-start record |
| 0x4E0E0 | 0x4E0E0..0x4E10B; 0x4E10D..0x4E132 | 21 | 82 | 0x4E133 |
| 0x4E140 | 0x4E140..0x4E16B; 0x4E16D..0x4E192 | 21 | 82 | 0x4E193 |
| 0x4E1A0 | 0x4E1A0..0x4E1CB; 0x4E1CD..0x4E1F2 | 21 | 82 | 0x4E1F3 |
| 0x4E260 | 0x4E260..0x4E297; 0x4E299..0x4E2BD | 26 | 93 | 0x4E2BE |
| 0x4E600 | 0x4E600..0x4E6C7 | 57 | 200 | 0x4E6C8 |
| 0x4E6D0 | 0x4E6D0..0x4E797 | 57 | 200 | 0x4E798 |
| 0x4E7A0 | 0x4E7A0..0x4E802 | 28 | 99 | 0x4E803 |
| 0x4E810 | 0x4E810..0x4EAC4 | 163 | 693 | 0x4E8C0 |
| 0x508E0 | 0x508E0..0x5095D | 35 | 126 | 0x5095E |
| 0x50AE0 | 0x50AE0..0x50B5D | 35 | 126 | 0x50B5E |
| 0x5F0E0 | 0x5F0E0..0x5F0F9 | 11 | 26 | No exact-start record |
| 0x5F100 | 0x5F100..0x5F148 | 22 | 73 | No exact-start record |
| 0x5F440 | 0x5F440..0x5F4BB | 38 | 124 | 0x5F46A |
| 0x5FAC0 | 0x5FAC0..0x5FC1E | 93 | 351 | 0x5FC1F |
| 0x64A90 | 0x64A90..0x64BFA | 69 | 363 | 0x64BFB |
| 0x64C00 | 0x64C00..0x64D87 | 72 | 392 | 0x64D88 |
| 0x64F30 | 0x64F30..0x65030 | 70 | 257 | 0x65031 |
| 0x6E600 | 0x6E600..0x6E643 | 19 | 68 | 0x6E644 |
| 0x6E6C0 | 0x6E6C0..0x6E707 | 22 | 72 | 0x6E708 |
| 0x6EDA0 | 0x6EDA0..0x6EE99 | 70 | 250 | 0x6EE9A |
| 0x6FDD0 | 0x6FDD0..0x6FE1A | 23 | 75 | 0x6FE1B |
| 0x737F0 | 0x737F0..0x73A2E | 175 | 575 | 0x73A30 |
| 0x740A0 | 0x740A0..0x742D1 | 173 | 562 | 0x742D3 |
| 0x745E0 | 0x745E0..0x74811 | 173 | 562 | 0x74813 |
| 0x7AD30 | 0x7AD30..0x7AD63 | 12 | 52 | 0x7AD64 |
| 0x7AD70 | 0x7AD70..0x7AD9D | 12 | 46 | 0x7AD9E |
| 0x7BFF0 | 0x7BFF0..0x7C0B3 | 54 | 196 | 0x7C0B4 |
| 0x7C280 | 0x7C280..0x7C356 | 68 | 215 | 0x7C357 |
| 0x7C440 | 0x7C440..0x7C49C | 30 | 93 | 0x7C49D |
| 0x7C510 | 0x7C510..0x7C572 | 27 | 99 | 0x7C573 |
| 0x7C7F0 | 0x7C7F0..0x7C86C | 33 | 125 | 0x7C86D |
| 0x7CD70 | 0x7CD70..0x7CEB6 | 92 | 327 | 0x7CEB7 |
| 0x7CEC0 | 0x7CEC0..0x7D006 | 92 | 327 | 0x7D007 |
| 0x7E4C0 | 0x7E4C0..0x7E596 | 55 | 215 | 0x7E597 |
| 0x7E780 | 0x7E780..0x7E910 | 104 | 401 | No exact-start record |
| 0x7FBB0 | 0x7FBB0..0x7FF19 | 189 | 874 | 0x7FF1A |
| 0x7FF20 | 0x7FF20..0x80266 | 193 | 839 | 0x80267 |
| 0x83290 | 0x83290..0x83491 | 141 | 514 | 0x83492 |
| 0x83540 | 0x83540..0x835B5 | 32 | 118 | 0x835B6 |
| 0x868F0 | 0x868F0..0x86956 | 29 | 103 | 0x86957 |
| 0x87A90 | 0x87A90..0x87ADF | 24 | 80 | 0x87AE0 |
| 0x913B0 | 0x913B0..0x91C35 | 428 | 2182 | 0x91C36 |
| 0x9AFB0 | 0x9AFB0..0x9BAC4 | 674 | 2837 | 0x9BAC5 |
| 0x9BB20 | 0x9BB20..0x9BC7A; 0x9BC7C..0x9BC8A; 0x9BC8C..0x9BC9A; 0x9BC9C..0x9BCA7; 0x9BCA9..0x9BDF1 | 160 | 718 | 0x9BDF2 |
| 0x9EA10 | 0x9EA10..0x9EC72; 0x9EC74..0x9EC82; 0x9EC84..0x9EC92; 0x9EC94..0x9EC9F; 0x9ECA1..0x9ED5E; 0x9ED60..0x9ED6E; 0x9ED70..0x9ED7E; 0x9ED80..0x9ED8B; 0x9ED8D..0x9EFED; 0x9EFEF..0x9EFFD; 0x9EFFF..0x9F00D; 0x9F00F..0x9F01A; 0x9F01C..0x9F050 | 377 | 1589 | 0x9F051 |
| 0x9F7C0 | 0x9F7C0..0x9F8E8; 0x9F8EA..0x9F8F8; 0x9F8FA..0x9F908; 0x9F90A..0x9F915; 0x9F917..0x9FD02 | 276 | 1343 | 0x9FD03 |
| 0xA17D0 | 0xA17D0..0xA1833 | 27 | 100 | 0xA1834 |
| 0xA23B0 | 0xA23B0..0xA245C | 53 | 173 | 0xA245D |
| 0xA2540 | 0xA2540..0xA262D | 64 | 238 | 0xA262E |
| 0xA27B0 | 0xA27B0..0xA2ADC; 0xA2ADE..0xA2AEC; 0xA2AEE..0xA2AFC; 0xA2AFE..0xA2B09; 0xA2B0B..0xA2BD6 | 240 | 1059 | 0xA2BD7 |
| 0xB2180 | 0xB2180..0xB29B8; 0xB29BA..0xB29BF; 0xB29C1..0xB29C6; 0xB29C8..0xB29CD; 0xB29CF..0xB29D4; 0xB29D6..0xB2C34; 0xB2C36..0xB2C50; 0xB2C52..0xB2C60; 0xB2C62..0xB2C70; 0xB2C72..0xB2C7D; 0xB2C7F..0xB2CD6; 0xB2CD8..0xB2CF2; 0xB2CF4..0xB2D02; 0xB2D04..0xB2D12; 0xB2D14..0xB2D1F; 0xB2D21..0xB2D75; 0xB2D77..0xB2D91; 0xB2D93..0xB2DA1; 0xB2DA3..0xB2DB1; 0xB2DB3..0xB2DBE; 0xB2DC0..0xB2EDE | 852 | 3403 | 0xB2EDF |
| 0xDABE0 | 0xDABE0..0xDABF8; 0xDABFA..0xDAC14; 0xDAC16..0xDAC24; 0xDAC26..0xDAC34; 0xDAC36..0xDAC41; 0xDAC43..0xDAC51 | 27 | 109 | 0xDAC52 |
| 0xE8A20 | 0xE8A20..0xE8A22 | 1 | 3 | No exact-start record |
| 0xF4420 | 0xF4420..0xF4483 | 27 | 100 | 0xF4484 |
| 0xF8410 | 0xF8410..0xF858D | 99 | 382 | 0xF858E |
| 0xF8690 | 0xF8690..0xF87C9 | 67 | 314 | 0xF87CA |
| 0x107440 | 0x107440..0x10768B | 146 | 588 | 0x10768C |
| 0x107690 | 0x107690..0x1081A9 | 681 | 2842 | 0x1081AA |
| 0x1097B0 | 0x1097B0..0x1098CA | 79 | 283 | 0x1098CB |
| 0x146AD0 | 0x146AD0..0x146B0D | 17 | 62 | 0x146B0E |
| 0x1473E0 | 0x1473E0..0x1474B6 | 68 | 215 | 0x1474B7 |
| 0x1474C0 | 0x1474C0..0x147544 | 35 | 133 | No exact-start record |
| 0x147750 | 0x147750..0x14776B | 7 | 28 | No exact-start record |
| 0x147800 | 0x147800..0x148E8F | 1531 | 5776 | 0x148E90 |
| 0x1496D0 | 0x1496D0..0x149757 | 36 | 136 | No exact-start record |
| 0x26FDA0 | 0x26FDA0..0x270BDC | 843 | 3645 | 0x270BDD |
| 0x272950 | 0x272950..0x27299A | 13 | 75 | No exact-start record |
| 0x272D90 | 0x272D90..0x2731B2 | 229 | 1059 | 0x273218 |
| 0x2ABB70 | 0x2ABB70..0x2ABB7D | 2 | 14 | No exact-start record |
| 0x2B9890 | 0x2B9890..0x2B989D | 2 | 14 | No exact-start record |
| 0x2E0B50 | 0x2E0B50..0x2E0DA7 | 158 | 600 | 0x2E0DA8 |
| 0x354C30 | 0x354C30..0x354CE4 | 50 | 181 | 0x354CE5 |

The separately bounded large appearance owner is 0x53E80..0x5DEAB inclusive, 7,943 instructions, 41,004 code bytes, with an exact-start PE end of 0x53EB2. That particular runtime-function record does not cover the entire exported logical owner; the byte-matched body extent here comes from the complete original instruction/range export. Its complete instruction range was byte-matched; only the receiver, admission and output-footprint scope is closed here.

## Bounded caller-window evidence

These are caller provenance windows, not assertions that the entire Game constructor or Interface setup is semantically closed.

| Owning entry RVA | Inclusive selected RVA range | Instructions | Code bytes |
|---|---|---:|---:|
| 0x7EA80 | 0x7EA80..0x7EAF0 | 25 | 113 |
| 0x7EA80 | 0x7EB14..0x7EB23 | 4 | 16 |
| 0x7EA80 | 0x7F450..0x7F4A3 | 21 | 84 |
| 0x7EA80 | 0x7F4C9..0x7F4D8 | 3 | 16 |
| 0x7EA80 | 0x7F4F4..0x7F666 | 73 | 371 |
| 0x7EA80 | 0x7F6D3..0x7F727 | 21 | 85 |
| 0xF9AC0 | 0xF9AC0..0xF9B43 | 25 | 132 |
| 0xF9AC0 | 0xFCBA4..0xFCBBF | 5 | 28 |
| 0xF9AC0 | 0xFCD14..0xFCD2F | 5 | 28 |
| 0xF9AC0 | 0xFCDCD..0xFCE16 | 17 | 74 |

## Resolved direct-call census for starting owners

Each cell preserves the original caller-entry/callsite association. These 45 call instructions were also checked by decoding the original relative-call displacement and comparing the resulting target with the selected original metadata. All matched. Zero direct calls does not exclude callback registration or computed dispatch.

| Target RVA | Direct sites | Original caller entry and callsite |
|---|---:|---|
| 0x4CE80 | 12 | 0x3E450 at 0x3E909; 0x7EA80 at 0x7F475; 0x7EA80 at 0x7F517; 0x9EA10 at 0x9EDBD; 0xA27B0 at 0xA2839; 0xF4490 at 0xF486A; 0x2C3710 at 0x2C448F; 0x2C3710 at 0x2C49D8; 0x2C3710 at 0x2C4B3D; 0x2E0DB0 at 0x2E11B5; 0x2E17D0 at 0x2E23E3; 0x2F4290 at 0x2F42F0 |
| 0x64A90 | 5 | 0x9F7C0 at 0x9FA2E; 0x147800 at 0x14784B; 0x2E0DB0 at 0x2E0FAB; 0x2E0DB0 at 0x2E11D0; 0x2E0DB0 at 0x2E14DB |
| 0x64C00 | 1 | 0x64A90 at 0x64AA6 |
| 0x4D600 | 6 | 0x3AA10 at 0x3AAFC; 0x3D820 at 0x3D933; 0x3F4F0 at 0x3F66D; 0x4CE80 at 0x4CEB5; 0x6EA30 at 0x6EA83; 0x147800 at 0x147978 |
| 0x3B7A0 | 10 | 0x3D820 at 0x3D93F; 0x3F4F0 at 0x3F679; 0x3F4F0 at 0x3F9CD; 0x9EA10 at 0x9EDE1; 0xA27B0 at 0xA2859; 0xF5520 at 0xF5C29; 0xF5520 at 0xF5CFD; 0x147800 at 0x147984; 0x2C3710 at 0x2C49EC; 0x2F4290 at 0x2F4338 |
| 0x147750 | 4 | 0x9BB20 at 0x9BCCF; 0xA27B0 at 0xA29C4; 0x2B98B0 at 0x2B9995; 0x2D38E0 at 0x2D3938 |
| 0x147800 | 1 | 0x9BB20 at 0x9BCDA |
| 0x9BB20 | 5 | 0x7EA80 at 0x7F488; 0x7EA80 at 0x7F71A; 0x965E0 at 0x966B4; 0x9F7C0 at 0x9F856; 0x9FDF0 at 0x9FE40 |
| 0x9F7C0 | 0 | No resolved direct-call row; selected setup address reference described above |
| 0x913B0 | 1 | 0x272D90 at 0x27317D |
| 0x9EA10 | 0 | No resolved direct-call row; selected setup address reference described above |

## Selected original export and data hashes

The following hashes identify original-side export evidence, not project-generated runtime or implementation results.

| Original export file | Bytes | SHA256 |
|---|---:|---|
| assembly.txt | 62547291 | FE07326A0F4585432A9D4E42E7CF16C570815A8FCD164DE36616095023A0740C |
| function_ranges.csv | 509127 | BC127412C5A8F21D8BA78AA53D9EB35A16AF58C1A85906DA9FF31E968E379601 |
| calls.csv | 10840369 | CF410A7DD84BBEF0E9FC4FC6FD1FF3B4903D97E9126C8F1A1D69EF05CD566DCB |
| functions.csv | 1551545 | 940E3682B9CCC68458531AC8D948A20F8B9530CDFFF5E568C5517BA91365DCEC |

| Original data RVA | Bytes | Bounded purpose | SHA256 of original bytes |
|---|---:|---|---|
| 0x5F14C | 24 | six category destinations | 1B5C7030A8A7954BCD7795707CB0CF48C0B4669E07001D877164BF1E0C4CE3DE |
| 0x5F164 | 28 | category byte remap | E259208BE5DCDAC27252BCE6F179CB46E842F94EF3F380FA47AFB95E428CA494 |
| 0x2731B4 | 32 | eight style destinations | 344DCE79A0B199684A1FBDB82A9DBAC1932451C589116F27F2EA9B95E0A97E84 |
| 0x2731D4 | 68 | seventeen option destinations | F679392FAC7A4F26AF5CD5B581B3EB28EE8560EB8793A24A8832E043D4CA4AB3 |
| 0x5DEAC | 32 | eight Data style destinations | 7D82DD7FCDC0EA95C48F25B6372AA010006319294A12D2F56F689A39B612E310 |
| 0x5DECC | 1224 | 306 appearance destinations | 5875C55AC818EEF79564D10428555887A9C96367115D1D7412E11A36C186FFB4 |

The PE extent cross-check for the primary owners uses unwind-info RVAs 0x4BC738 (0x4CE80), 0x527624 (0x64A90), 0x4BC588 (0x64C00), 0x4B3BE0 (0x4D600 and 0x3B7A0), 0x4DF000 (0x147800), 0x4C4F70 (0x9BB20), 0x4C5270 (0x9F7C0), 0x4C4DD8 (0x913B0) and 0x4C5150 (0x9EA10). This identifies the original metadata used for those bounds; unwind execution and external exception handling are not claimed.


## Exposure and hold

Only the standalone source-author policy, this isolated assignment and individually selected original executable/export evidence were used. No active AGENTS.md, WORKSPACE-MEMORY.md, CLEANROOM.md, combined history, replacement engine/exporter, tests/results, owner data/configuration/captures/outputs/assets, SDK/provider/dependency implementation or reconstruction source was opened. Earlier held handoffs were not opened or used as evidence and were not modified. No original executable was run, no additional agent was spawned, and no implementation/runtime verification feedback was requested or received.

This artifact is a standalone factual contract for the selected owners and boundaries. All saved prose is subject to full readback and integrity verification before its FINAL / STABLE / HELD report. After that report it is immutable.
