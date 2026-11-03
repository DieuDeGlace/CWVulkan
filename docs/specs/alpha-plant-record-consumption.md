# Alpha small-plant record consumption

Independent native-only bounded supplement to `alpha-worldgen-vegetation.md`
§§1/3/8, `alpha-rendering.md` §11.1 and `vegetation-wind.md` §§3–4. No engine,
tests, exporter, owner assets/captures, reconstruction or game execution was
used. The Alpha client image base is **0x400000**; VA and RVA below identify
the original client. This covers ordinary table-A environment models, not
voxel trees or the table-B static-object transform path.

## 1. Record scale is one uniform voxel-to-block factor

The reached ordinary drawer is **VA0x4bd160 / RVA0xbd160**, with the record's
signed32 kind at **+0x00** selecting a resource pointer through the Alpha
table at **Game+0x800718**. The relevant record fields are:

| Offset | Type and use |
| --- | --- |
| +0x08,+0x10,+0x18 | Signed64 fixed world XYZ, 65536 units/block |
| +0x20 | **One float32 uniform scale**, blocks per model voxel |
| +0x24 | Float32 yaw in degrees |
| +0x2c,+0x30,+0x34 | Float32 RGB metadata, not additional scale components |
| +0x38 | Flags; bit 0x04 admits the sway matrix route |

Thus the earlier vegetation §1.3 wording “scale vector” does not describe this
table-A record. It must not be conflated with the separate static object's
three-component extents. Constructor **VA0x58e9f0 / RVA0x18e9f0** initializes
RGB to (1,1,1) and flags to **2**, at **VA0x58e9f0–0x58ea05**; it does not
initialize all remaining record fields. The reached producers supply position,
scale, yaw and kind separately.

The model mesher **VA0x4e7870 / RVA0xe7870** emits voxel-corner coordinates
through packed vertex helper **VA0x466650 / RVA0x66650**. Those coordinates
remain in model voxel units. The drawer applies +0x20 uniformly to all three
basis axes. There is no additional universal one-eighth, one-tenth, header-fit
or occupied-bounds normalization in this ordinary model transform. A scale
of 0.075 gives 0.075 block per voxel; a 20-voxel allocation width then spans
1.5 blocks before its yaw/sway, irrespective of how many voxels are occupied.
The owner model's actual dimensions/content were not inspected.

## 2. Full-header XY centering, with Z at the base

The drawer uses the resource's signed32 header dimensions **+0x44 X** and
**+0x48 Y**. At **VA0x4be553–0x4be56b / RVA0xbe553–0xbe56b** it converts
them to float32 and multiplies by float32 **−0.5**, constant **VA0x6fce8c**.
The corresponding local centering translation has Z equal to zero. It does
not center around half the header height or around the occupied-vertex bounds.

In the order a model vertex experiences the transform: full-header XY
centering, record yaw about Z, admitted sway, uniform record scale, then
translation to the record's position relative to the render origin. This
matches the established wind §4 transform order. Fixed X/Y first combine with
Game's matching signed64 origin offsets **+0x1d8/+0x1e0**, then become float
blocks; Z is converted from its fixed world coordinate. The origin is a render
coordinate change, not an extra plant-placement offset.

Consequently the record position denotes the **center of the full model
allocation in XY and its local Z=0 base**. Odd header sizes use their real
half-size, including a half voxel. No additional +0.5 block is inserted by
this drawer. The generated small plants already receive their own producer
offsets as described next.

## 3. Producer XY and exact water-lily Z

The reached small-plant branches in zone generator
**VA0x5e4850 / RVA0x1e4850** construct records at VA0x5ebfe9, 0x5ec1c7,
0x5ec3d2, 0x5ec68f, 0x5ec893, 0x5ecc9c and 0x5ecda1. Their checked X/Y
construction adds **0.5 block to each integer column coordinate**. This
qualifies the older vegetation §1.3 description of these plants as standing
at the block's XY corner. It is not a general placement rule for every other
table-A producer, such as signs on buildings.

The coordinate helper **VA0x468fc0 / RVA0x68fc0** multiplies the supplied
double by **−65536.0** (VA0x702ae0), truncates it to signed64, and subtracts
that fixed integer from the source coordinate. The supplied double **0.5**
(word **0x3fe0000000000000**, VA0x745de8) therefore adds exactly **32768**
fixed units. Lily X/Y use this at **VA0x5ec1f6 / RVA0x1ec1f6** and
**VA0x5ec23a / RVA0x1ec23a**.

For vegetation §3.3's lily branch, **zt already equals column bottom plus
selected block index plus one**. It is the integer cell immediately above
the selected water block, not the water block's own integer Z. This value is
prepared at **VA0x5ebd21–0x5ebd2f / RVA0x1ebd21–0x1ebd2f**.

At **VA0x5ec27e / RVA0x1ec27e**, the producer calls
**VA0x42f5a0 / RVA0x2f5a0** on the signed64 fixed representation of zt, with
double **0.1**, word **0x3fb999999999999a** at VA0x71a048. That helper:

- Multiplies this double offset by double **65536.0**, VA0x6fd588.
- Converts the double product to signed64 by the truncating helper
  **VA0x68d946 / RVA0x28d946**.
- Subtracts that integer from the original signed64 coordinate and writes
  the output, preserving the native low64 subtraction.

For this finite literal the converted offset is exactly **6553 fixed units**.
The resulting lily record Z is therefore **zt×65536−6553**, or
**zt−0.0999908447265625 blocks**. It does not add another whole block; that
one-block advancement is already in zt. It also does not first subtract 0.1
from a large floating world height and then convert that whole result.
For example, zt=10 produces fixed Z **648807**. The helper only computes the
coordinate; it does not mutate terrain, consume randomness or select a model.
The branch's existing kind31/32, scale0.09 and yaw decisions remain as in
vegetation §3.3.

## 4. Flag 4 and the actual ordinary sway phase

**VA0x4bd4fd / RVA0xbd4fd** tests record flag **0x04**. A set bit enters the
CPU matrix-shear route **VA0x4bd507–0x4be40f**. It uses the world clock at
**Game+0x8003a0**, resource header height **+0x4c** and uniform scale +0x20;
its size compensation includes **0.1 divided by (header height times scale)**.
The exact wave coefficients and matrix operations remain vegetation-wind §4.2.
The base at local Z=0 stays fixed under this shear. This consumer's verified
meaning is sway admission, not a collision-disable interpretation; other
systems' flag readers were not exhaustively audited here.

### 4.1 Actual flags produced by the checked small-plant branches

The producer operations distinguish **assignment** from **OR**. Reed kind22
overwrites the constructor's flags with **4**, at
**VA0x5ec0d3 / RVA0x1ec0d3**. The other reached sway writes OR bit4 into
the constructor value **2**, producing **6**, not 4. They do not clear the
constructor's mask0x02. Here “bit4” means mask 0x04.

| Checked producer case | Operation after constructor | Resulting flags |
| --- | --- | --- |
| Reeds, kind22 | Assign 4 | **4** |
| Underwater kinds5/7 | OR 4 at VA0x5ec54c / RVA0x1ec54c | **6** |
| Coral, kind6 | No admitted OR; retain constructor flags | **2** |
| Lava-grass, kind9 | OR 4 at VA0x5ec7d1 / RVA0x1ec7d1 | **6** |
| Hot grass/sand branch, kinds2/3/11/12 | OR 4: VA0x5ecaa4 for kinds2/3, VA0x5ecabd for kind11, VA0x5ecafc for kind12 | **6** |
| Temperate branch, kinds2/3/4/12 | OR 4 at VA0x5ed0e1 / RVA0x1ed0e1 | **6** |
| Other admitted nonsway cases in these branches, including ordinary flowers, desert flowers and thorn-plant | Retain constructor flags | **2** |
| Water lilies, kinds31/32 | Retain constructor flags | **2** |

In particular **kind4 is branch-dependent**: when selected by the checked hot
branch it retains flags2, while the temperate branch admits it to the OR and
produces flags6. Kind alone is therefore insufficient to reconstruct the
sway decision. Reed's flags4 and the other swaying records' flags6 both pass
the same consumer mask0x04 test. This establishes the actual retained flags,
without assigning an additional effect/collision meaning to mask0x02 or
extending the result to other model producers.

### 4.2 Ordinary active-list phase

The ordinary phase argument is now established more precisely than the older
wind §4.2/§9 attribution:

- The reached renderer resets a counter to **0 at VA0x4b1e08 / RVA0xb1e08**
  for each admitted cache's environment traversal.
- It visits that cache's **active environment list at cache+0x240** in stored
  order. Before examining each record, it increments the counter at
  **VA0x4b1ea0 / RVA0xb1ea0** and retains that value at VA0x4b1ea5.
  Therefore the first list entry has phase **1**, the second 2, and so forth.
- This occurs before the type/effect/resource/culling branches. Skipped,
  unsupported or separately handled entries still occupy a counter position.
  It is not a compact count of successfully drawn or sway-enabled plants.
- The near model call **VA0x4b2801 / RVA0xb2801** passes that retained counter.
  The far queue stores the same value at entry **+0x0c**, and its later call
  **VA0x4ba4fe / RVA0xba4fe** reads it back without assigning a new phase.

This supersedes the older attribution of this phase to the result of
VA0x4ec400. That result belongs to another traversal's use of the same stack
storage; it is not the phase source for the reached ordinary environment
record loop above. The consumer forms its existing **30 times phase** input
from this integer, with native 32-bit integer arithmetic before conversion.

The cache list's spatial extent, population/rebuild policy and correspondence
to the original zone plant list are **not established by this trace**. No
full-zone index, global index, stable-ID phase or modulo-12 substitution is
inferred. The established identity is one-based position in this reached
active cache list, retained when that record is deferred.

## 5. Limits

This resolves the ordinary scale/pivot, render flag4, reached phase convention
and lily-height questions, not all environment-model producers or the complete
cache lifecycle. Admission/far-pass order remains the separate Alpha scenery
contract. No owner geometry, replacement asset, collision behavior for other
consumers, degenerate zero-height/zero-scale recovery, or general malformed
coordinate conversion is supplied. Existing wind sign uncertainties are not
closed merely by identifying the flag and phase producer.
