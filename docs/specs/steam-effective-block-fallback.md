# Steam effective-block fallback initialization and lifetime

## 1. Scope and established result

This bounded native-only note resolves the shared-global initialization limit in [selected vehicle §5.2](steam-selected-vehicle.md#52-effective-block-at-physical-xyz) and [Zone terrain storage §5](steam-zone-terrain-storage.md#5-stored-versus-effective-reads-and-error-boundaries). It confirms the older missing-zone value in [creature upkeep §4.7](creature-upkeep.md#47-blocks-and-light). No earlier note is modified.

The supported original AMD64 image is `cubeworld.original.exe.unpacked.exe`, preferred base **0x140000000**, SHA-256 **7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244**. All addresses below are RVAs. Evidence is the original PE section data, original decoded instructions including unnamed startup routines, initializer-pointer data and reached lookup routines. No engine, tests, exporter, SDK/reconstruction implementation, owner outputs/probes or game execution was used.

After successful ordinary native startup, the six bytes at **0x54dcb0…0x54dcb5** are **c8 c8 c8 00 01 00**: RGB200/200/200, auxiliary/light byte0, type1 and flags0. **Flags byte5 bit0x20 is clear.** This is an explicitly initialized native fallback, not a guessed all-zero or type-air block.

## 2. PE bytes versus reached startup initialization

The three effective defaults have distinct six-byte storage locations spaced eight bytes apart. Their enclosing writable initialized-data section has RVA **0x530000**, virtual size **0x23c40**, raw-file offset **0x52dc00** and raw size **0x1de00**. All three locations lie within its file-backed raw extent, not merely the section's zero-filled virtual tail.

|Effective default|Storage RVA|File offset in this exact image|Six bytes before dynamic initialization|Six bytes after ordinary initialization|
|---|---|---|---|---|
|Missing/invalid/below-column solid|0x54dcb0|0x54b8b0|00 00 00 00 00 00|c8 c8 c8 00 01 00|
|Above-span positive-Z air|0x54dcb8|0x54b8b8|00 00 00 00 00 00|ff ff ff 00 00 00|
|Implicit sea-level water|0x54dcc0|0x54b8c0|00 00 00 00 00 00|ff ff ff 00 02 00|

The two bytes between consecutive six-byte records are not additional block fields. Byte3 is the auxiliary/light byte, not alpha; byte4 is type and byte5 is flags. Reading the on-disk zeros alone does not describe ordinary post-startup World queries.

The native startup routines and their concrete writes are:

|Routine RVA|Destination|Direct write RVAs and coverage|Result|
|---|---|---|---|
|0x10e0|0x54dcb8|0x10ec writes the first2 bytes;0x10f3 writes the following4|White air, including type0/flags0|
|0x1120|0x54dcc0|0x112c writes the first2 bytes;0x1133 writes the following4|White water, including type2/flags0|
|0x1100|0x54dcb0|0x110c writes the first2 bytes;0x1113 writes the following4|Gray solid, including type1/flags0|

Each initializer explicitly covers **all six bytes**, including byte5. Its flags0 result does not depend solely on the PE's original zero value. These short routines were decoded in the native instruction listing but had no enclosing named function in the original function-decompile inventory. Searching only named-function source assignments misses them.

The ordinary startup routine **0x393884** passes initializer range **0x42cdc0…0x42db90** to imported **_initterm at0x393908**, after successful **_initterm_e at0x3938e7**. That range contains the following eight-byte function-pointer entries, in increasing table order:

|Pointer-slot RVA|Initializer RVA|
|---|---|
|0x42cdf8|0x10e0, air|
|0x42ce00|0x1120, water|
|0x42ce08|0x1100, gray solid|

The same startup routine calls the game's ordinary entry **0x1355a0 at0x393996** afterward. Thus completed ordinary CRT initialization precedes that game's World/Game construction and ordinary queries. Constructor-RVA order is not the table's execution order. Failed or bypassed native startup is not assigned an invented initialized-state guarantee.

## 3. Getter identity, copy semantics and direct-writer census

**0xe9a0** receives signed32 block coordinates and looks up the containing loaded zone in the passed World's zone map. If the entry is missing, or its Zone pointer is null, it copies the six bytes **0x54dcb0…0x54dcb5** into the caller's output record, at **0xea5c–0xea99**, and returns the caller's output address. The supplied Z does not select air/water on this missing-zone branch: the gray-solid default is used for any Z.

The fixed-coordinate wrapper **0xeab0** performs its established coordinate conversions and reaches0xe9a0. It does not own another fallback or initialize these bytes. A present zone reaches **0xeb90**, whose distinct choices are in section4. The output is a **copy**; these routines do not lend the global record as a mutable block pointer, insert a physical block, extend a column or publish light/mesh state by returning it.

The original decoded-instruction census, including direct references whose memory widths overlap these records, establishes the following direct writers:

- Gray default0x54dcb0…+5: **0x110c and0x1113 only**.
- Air default0x54dcb8…+5: **0x10ec and0x10f3 only**.
- Water default0x54dcc0…+5: **0x112c and0x1133 only**.

Other decoded direct references to these three records are byte reads by0xe9a0/0xeb90. No additional direct World constructor, Zone constructor, world/character reset or ordinary physical-block setter write to them was found in that census. No direct address-taking reference or stored absolute64-bit pointer to any of their three base addresses was found in the checked image. The process-image storage is shared by calls on different World instances; it is not a World member, Zone allocation or thread-local block value. The section remains writable, so this is a native lifetime finding rather than a claim that hardware makes the bytes immutable.

This census covers decoded original native direct references and the checked image data. It is not a proof about every arbitrary computed address, undiscovered instruction path, invalid output alias, injected module or external memory write. No universal mod/external-writer exclusion or synchronization guarantee is asserted. Within the ordinary reached startup/query path, there is no per-World reinitialization or reset value replacing the three defaults.

## 4. Separate effective and raw-column fallback paths

The following distinctions are necessary: a physical column's relative-index default is not the shared World missing-zone default.

|Reached path|Record/source|Returned bytes after its own initialization/writes|
|---|---|---|
|World0xe9a0: missing map entry or null Zone|0x54dcb0|c8 c8 c8 00 01 00|
|Zone0xeb90: horizontal coordinate outside the Zone, or Z below the column's stored base|0x54dcb0|c8 c8 c8 00 01 00|
|Zone0xeb90: Z above the stored span and greater than0|0x54dcb8|ff ff ff 00 00 00|
|Zone0xeb90: above-span Z at most0; or stored type0 at Z at most0 with stored flags bit0x02 clear|0x54dcc0|ff ff ff 00 02 00|
|Raw column0xe770: negative relative index|Separate0x54dcd8…0x54dcdd|00 00 00 00 01 00|
|Raw column0xe770: nonnegative relative index at/above count|Separate0x54dce4…0x54dce9|00 00 00 00 00 00|

In0xeb90 the below-base test precedes above-span/sea handling, so a below-base Z does not become water merely because Z is nonpositive. A stored in-span block normally retains its bytes; the special stored-air sea conversion above is bypassed when stored flags bit0x02 is set. These are read substitutions; they do not change stored columns. General bounds, narrowing and column layout remain the existing Zone terrain-storage contract.

The two raw-column records are **separate guarded statics**, initialized on their respective0xe770 branches, with guards at **0x54dce0** and **0x54dcec**. The negative-index record is zero-initialized at **0xe939/0xe943**, then its six bytes are assigned to black type1 on the negative path through **0xe7bb/0xe7d1**. The above-span record is zero-initialized at **0xe976/0xe980** and is assigned six zero bytes on its branch through **0xe86f/0xe885**. These writes do not overlap0x54dcb0,0x54dcb8 or0x54dcc0. The raw reader also copies into caller storage; its static records are not returned as column ownership. Their guarded initialization is distinct from the three CRT initializer-table callbacks.

All five returned fallback forms have **flags byte0** on these reached ordinary paths. Their RGB/type values differ. In particular, the black raw-below-range solid and gray effective/missing-zone solid must not be substituted for each other when color/type provenance matters. These defaults do not establish a terrain-light-solver publication value or change the separately specified missing-zone light-accessor rule.

### 4.1 Empty physical column and retained baseZ

For an admitted horizontal coordinate and a valid column record, **an empty physical vector does not suppress the below-base branch**. At **0xec09–0xec19**,0xeb90 first loads the signed32 **column+0x3c baseZ** and compares requested signed32 Z against it. A smaller Z immediately takes the gray-solid copy at0xec1b. It has not yet read the physical vector's begin/end pointers or tested its count.

Only a Z at least baseZ reaches **0xec71–0xec95**, which subtracts the vector's begin from end, obtains its six-byte record count, adds the narrowed count to baseZ and tests whether Z is strictly below that top. With equal begin/end, including an empty vector with retained allocation, the count is0 and top equals baseZ. No Z in this second branch is then inside the stored span. **0xec97–0xec99** chooses the remaining default from whether requested Z is positive.

|Empty vector case|Returned effective record|
|---|---|
|Z below baseZ, regardless of whether Z is positive, zero or negative|Gray solid0x54dcb0: c8 c8 c8 00 01 00|
|Z equals baseZ, and Z is positive|White air0x54dcb8: ff ff ff 00 00 00|
|Z equals baseZ, and Z is zero or negative|White water0x54dcc0: ff ff ff 00 02 00|
|Z above baseZ, and Z is positive|White air0x54dcb8|
|Z above baseZ, and Z is zero or negative|White water0x54dcc0|

These cases do not call the in-span raw reader0xe770, and no live/nonzero-count gate precedes the below-base comparison. In particular, an empty column with a positive retained baseZ returns gray solid for Z0 when0 is below that base; it does not immediately choose sea water merely because its vector is empty.

The base comes from **the actual column+0x3c field**, not a vector-derived or cached-height replacement. Ordinary column constructor **0x6cbd0**, at **0x6cbdc–0x6cbe7**, explicitly sets that field to0 and the begin/end/capacity pointers to null. Thus a fresh constructed empty column returns gray solid below0, white water at0 and white air above0. The separate cached float fields+0x08…+0x38 remain uninitialized by that constructor and are not read for these decisions; see [Zone terrain storage §3](steam-zone-terrain-storage.md#3-in-place-column-array-and-initialization).0xeb90 does not reset baseZ when it sees an empty vector. The provenance of an arbitrary later nonzero base in an empty column remains an input from its actual producer/lifecycle, not an invented empty-column initialization rule.

## 5. Consequence and limits for the prior open input

For the ordinary selected-vehicle0xeab0 lookup, a missing/null loaded zone supplies **type1 with flags0**, so its flags-bit0x20 test is clear. This is established by the reached static initialization and lookup copy, without assuming an all-zero missing block or inspecting a runtime capture. A loaded zone may supply actual stored/effective flags instead, under its own accessor rules; this note does not erase them.

The note closes the ordinary original-image startup value and direct-writer uncertainty. It does not define pre-startup injected queries, modified executables, arbitrary concurrent corruption, alternate builds, every raw-getter concurrency domain or restoration behavior outside these directly reached helpers. No earlier finalized spec or implementation file was changed.
