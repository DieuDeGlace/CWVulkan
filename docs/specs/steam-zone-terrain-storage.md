# Steam Zone: primitive stored terrain view

Independent native-only specification. Only original native evidence and existing
independent prose were read. No SDK, engine, exporter, test, sister implementation,
reconstruction, owner output/capture, probe or game execution was used. This is a
bounded read-only data contract, not a complete Zone class, serializer, mutation
implementation or whole-zone snapshot guarantee.

## 1. Image, owner and coordinate identity

Addresses are RVAs in the analyzed original unpacked AMD64 image
`cubeworld.original.exe.unpacked.exe`, preferred base **0x140000000**, SHA-256
`7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244`.
This identifies one analyzed image, not all distributions or modified builds.

The reached Zone allocation is **0x58518 bytes** in **0x2ae0c0**. Constructor
**0x2f9400** establishes the following identity fields:

| Zone offset | Established content |
| --- | --- |
| +0x00 | Eight-byte native table pointer, set to table 0x4738c0 |
| +0x08 | Eight-byte pointer to the supplied owning World |
| +0x10 | Signed32 Zone X coordinate |
| +0x14 | Signed32 Zone Y coordinate |

One ordinary Zone spans **64 by 64 whole-block columns**. In the ordinary
nonoverflowing coordinate domain, its block X origin is Zone X times 64 and its
block Y origin is Zone Y times 64. Local column coordinates are 0 through 63.
For example Zone(-1,2) covers whole-block X −64 through −1 and Y 128 through 191.
These header coordinates are not 65536-unit fixed positions or region indices.

Native effective reader **0xeb90** validates global signed32 block X/Y against
that Zone's horizontal bounds before obtaining an in-place column. Its origin,
extent and local-coordinate arithmetic use signed32 operations. This contract
does not extend ordinary geometric bounds across overflowing signed32 products
or additions. The related neighborhood lookup **0xdfb50** returns an original
column pointer from available Zone pointers, or null when its neighborhood/Zone/
horizontal admission fails; it does not allocate a missing column or return a
copy of its records.

The loaded-container pointer and World lifetime remain separate boundaries;
see [loaded Zone enumeration](steam-loaded-zone-container.md). A readable table
pointer or coordinate pair alone does not retain the Zone.

## 2. Region/theme header: established boundary and unresolved storage

**This bounded audit does not establish a stored inline region/theme header or
a single Zone biome-field offset.** The identity fields above must not be
relabeled as region coordinates, region type or theme. No global absence of such
metadata elsewhere is inferred.

Native point-region query **0x2ada20** produces a signed32 region-coordinate pair
from a World and whole-block XY point; its exact output/argument distinction is
in [the independent region-query ABI](steam-region-query-abi.md). Biome query
**0x2b9080** consumes region coordinates and returns a biome value. These are
World/point-derived query results, not evidence of an inline Zone field or a
unique sampling point for a per-Zone summary. A derived region/theme label and
the raw Zone header are therefore distinct data provenance.

Constructor 0x2f9400 also writes **0xffffffff to the 32-bit field at +0x4f8**.
Its region/theme meaning is **unproved here**; that constructor write is not
sufficient to assign it such a name. Other unlisted header members likewise
remain outside the supplied view. Existing independent generation contracts
retain their own region/place/climate meanings without inventing a new stored
header layout.

## 3. In-place column array and initialization

The Zone contains **4096 in-place columns beginning at +0x518**, each **0x58 bytes**.
Columns are ordered **local X outside, local Y inside**, both increasing 0..63.
Advancing local Y advances 0x58 bytes; advancing local X advances 0x1600 bytes.
The column array occupies 0x58000 bytes and ends at Zone+0x58518, matching the
observed Zone allocation extent. It is not an array of column pointers.

Constructor 0x2f9400 invokes the native vector-construction iterator on this
array with count 0x1000, stride 0x58 and column constructor **0x6cbd0**. That small
constructor was checked in native disassembly: it sets the column's table pointer,
sets **baseZ+0x3c to 0**, and sets all three vector pointers **+0x40/+0x48/+0x50
to null**. It does **not initialize the cached float fields +0x08..+0x38**.
They cannot be assigned arbitrary zero/default values merely because a Zone
allocation has completed construction.

| Column offset | Established storage |
| --- | --- |
| +0x00 | Eight-byte native table pointer, table 0x432e78 on construction |
| +0x3c | Signed32 base Z of the stored block sequence |
| +0x40 | Eight-byte pointer to the first live stored block |
| +0x48 | Eight-byte pointer immediately after the live stored blocks |
| +0x50 | Eight-byte pointer immediately after the block allocation's capacity |

These vector pointers do not describe the complete column object allocation.
Capacity is not a live block count, surface height or valid data span. Native
readers derive live count from the beginning/live-end byte span divided by six;
**0xe770** and **0xeb90** narrow that count to signed32 for their comparisons.
Malformed spans, misalignment, overflow or corrupt pointers receive no recovery
contract here.

## 4. Live block span and exact six-byte record

For ordinary nonoverflowing values, stored index 0 has whole-block Z equal to
baseZ, then indices increase upward one block at a time. The stored interval is
baseZ inclusive through baseZ plus live count exclusive. It need not start at
zero, include sea level or end at the cached surface height. Record size is
**exactly six consecutive bytes**:

| Record byte | Established meaning |
| --- | --- |
| 0 | Red byte; also receives solved light values for stored air/water on the reached solver path |
| 1 | Green byte |
| 2 | Blue byte |
| 3 | Auxiliary-light byte; distinct from flags and from a per-draw shader Glow control |
| 4 | Stored material-type byte; an accessor can return a substituted type value |
| 5 | Flags byte |

All six are byte fields; no per-record alpha, padding or eight-byte element
alignment is established. Within a stored vector they are **raw stored records**.
Record bytes must not be reordered because a historical label called byte 3 glow.
The native channel 0 reader **0xda830** uses byte 3 as auxiliary light and byte 5
as flags, as established in
[block-light publication §3](steam-block-light-publication.md#3-exact-channel0-query).

Known separate flags include **bit 0x02**, which participates in the below-sea
air/water substitution rule, and **bit 0x40**, whose reached static-object
marking/restoration provenance is in
[static-object block flags](steam-static-object-block-flags.md). This does not
assign meanings to every flag bit or equate type classification with collision
solidity, opacity or light emission.

Constructor-empty columns have null beginning/end/capacity and no live records.
More generally, equal live beginning/end denotes no stored records; this condition
alone does not supply records for all unrepresented heights or make capacity
bytes live terrain. The base of an empty column is not a universal world-height
origin.

## 5. Stored versus effective reads and error boundaries

The relative stored-column copy helper **0xe770** copies six bytes for an admitted
nonnegative index below live count. Its explicit negative-index fallback is
**0,0,0,0,1,0**, and its at/above-count fallback is **0,0,0,0,0,0**. These are
returned records, not inserted cells or extensions of the vector. This relative
helper is not interchangeable with a global XYZ Zone query.

Effective Zone reader **0xeb90** first enforces the horizontal Zone bounds. For
a valid column it distinguishes below-base, in-range and above-live-span Z. Its
out-of-Zone/below-base, above-span and sea cases use their reached global fallback
records. An in-range stored type 0 at Z at or below zero is replaced by the sea
result unless flags bit 0x02 prevents that substitution. No replacement is written back by this
read. The complete substitution/setter interaction remains in
[the full block setter](steam-full-block-setter.md) and
[block-light publication](steam-block-light-publication.md).

Consequently a raw empty column, an accessor's synthesized air/water/below-range
result and a physically stored six-byte record are different observations.
This note does not create missing terrain, normalize flags, manufacture implicit
records in capacity storage or assign new numeric fallback-global contents from
unverified image bytes. Accessors supply copies/results rather than a lifetime
extension for a previously saved vector pointer.

## 6. Cached per-column generation inputs

The following float32 cache fields are independently established in
[terrain-zone layout §3.1](worldgen-terrain-zone.md):

| Column offset | Cached generation input |
| --- | --- |
| +0x08 | Height |
| +0x0c | Warmth |
| +0x10 | Dryness |
| +0x14 | Highlands |
| +0x18 | Lava-region weight |
| +0x1c | Forest-region weight |
| +0x20 | Kind27 weight |
| +0x24 | Lake |
| +0x28 | Vegetation |
| +0x2c | River |
| +0x30 | Lava |
| +0x34 | Swamp |
| +0x38 | Roads |

Ordinary generation **0x2ff6f0** supplies these caches; column construction does
not. They are not interchangeable with a stored end Z, occupied bounds, a
single region/theme header or a guarantee of the post-edit surface. For example,
full setter **0x31c1c0** uses the cached highlands/climate values when filling
new buried gaps. The cache's continued presence is not proof that a fresh
regeneration occurred after each block edit.

## 7. Mutation, light publication and lifetime limits

The full setter **0x31c1c0**, using six-byte put **0xa36e0**, can change baseZ,
shift existing records, extend the vector and replace/reallocate its storage.
Existing-block writes can also change bytes without changing the vector header.
Their complete admission, sea substitution and fill behavior is already bounded
in the full-setter contract. Those helpers do not provide an internal whole-Zone
snapshot or a general locking/lifetime guarantee.

The reached light solve **0x2a3260** writes solved red bytes into the actual stored
type 0/type 2 records through 0xa36e0, retaining the other five bytes read from each
record. That happens before later terrain geometry and GPU-group publication.
It is not a separate immutable gameplay-light volume. The reached writeback is
not atomic across the Zone, and it occurs after the neighborhood's World+0x3f0
capture lock has been released and before the later World+0x3c8 mesh-installation
lock. Exact sites/order remain in
[block-light publication §§1–2](steam-block-light-publication.md).
Holding a mesh-list lock must therefore not be relabeled as proof that all column
RGB/type/flag storage is frozen or corresponds to one completed generation phase.

World+0x3f0 container protection, World+0x3c8 mesh publication and Zone lifetime
are separate demonstrated boundaries. Their reached paths are described in
[loaded Zone lifetime](steam-loaded-zone-container.md) and
[terrain buffer resources](steam-terrain-buffer-resources.md). This note supplies
no universal lock pair that freezes every physical edit, solver write, reset,
destruction or caller. Reading only a column/vector header under a lock and
later traversing its allocation unlocked does not establish coherence or retain
the allocation.

Zone cleanup **0x2f9940**, reached through **0x2fa3b0**, invokes the column
destruction iterator on all 4096 entries, stride 0x58, using **0x6cc40**. Column
cleanup releases the block allocation and clears its three vector pointers.
The outer Zone allocation is also conditionally released. Saved numerical Zone,
column and block pointers acquire no ownership or post-destruction validity.

Unresolved limits are the single stored region/theme-header mapping, unlisted
header/flag semantics, every alternate writer and lifecycle route, malformed-data
recovery, overflow domains and complete concurrent snapshot/phase guarantees.
No runtime-capture oracle, serializer layout, complete class or full-world parity
claim follows from these primitive data facts.
