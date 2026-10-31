# Steam category0x17 procedural item resources

Independent native-only supplement to [item-models §3](item-models.md)
and [voxel vertex inputs](steam-voxel-vertex-inputs.md). Addresses are
Steam RVAs, relative to the live executable module base. No implementation,
tests, exporter/build, SDK, owner assets/captures or game execution were
used. Specifies native resource acquisition/owner-data capture and all
eight reached shape-family algorithms, with precision/verification
boundaries retained explicitly in§17.

## 1. Resource identity and the live cache

Category **0x17/23** is the procedural artifact category. The ordinary
item resolver **0x173b10(store,item)** normalizes the item through
0x1092b0, then dispatches this category to **0x1737b0(store,id,modifier)**.
Normalization does not change category0x17. The base resource depends
on the full signed32 subtype/ID at item+0x04 and the **full32-bit
modifier bits at+0x08**. Rarity, material, region, realm/formula word
and attribute payload do not enter this base-model key. Separate
caller-side item/attribute rendering retains its own existing contract.

The live store is reached through **Game+0x08 → World; World+0x70 →
model store**. The ordinary initialized Game pointer is also held by
the executable global pointer at **RVA0x551a80**. Existing catalog
model pointers are store+0x10/+0x18; these procedural resources are
**not appended to that catalog list** and have no corresponding
catalog index or required source CUB filename.

Store **+0x28** is a separate ordered cache with a two-word key.
Comparison treats **both words as signed32**, ID first, then modifier
(0x76440/0x172e50). Preserve the modifier's original bit pattern;
signed ordering does not reduce it modulo330/660 or otherwise discard
its high bit. Repeated lookup of the same full key returns its cached
resource without regenerating it. The lookup has no eviction path.

Native cache layout for a stable initialized store:

| Location | Content |
|---|---|
|Store+0x28|8-byte tree head/sentinel pointer|
|Store+0x30|8-byte element count|
|Node+0x00/+0x08/+0x10|Left, parent, right pointers|
|Node+0x18/+0x19|Tree color and nil/sentinel bytes|
|Node+0x20|Signed32 ID key|
|Node+0x24|Modifier32-bit key bits, compared signed|
|Node+0x28|8-byte voxel-resource pointer|

The sentinel's parent link is the root; its outer links identify
extreme nodes. Ordinary nodes have nil byte0. A cache node allocation
is **0x30 bytes** (0x1735d0). Node creation initializes value pointer
to null (0x1720e0). Data readers can enumerate existing entries without
creating keys, provided they observe a stable cache lifetime/phase.
Offsets are native data layout, not a recommendation to copy internal
pointers into an exported asset file.

## 2. Lookup ABI and generation/readiness stages

The verified native x64 interfaces are:

| Entry RVA | Arguments | Result |
|---|---|---|
|0x173b10|RCX initialized model-store pointer; RDX pointer to a full native0xa0-byte item|RAX borrowed voxel-resource pointer, or normal null outcome|
|0x1737b0|RCX initialized model-store pointer; EDX signed32 ID; R8D full modifier bits|RAX borrowed cached voxel-resource pointer|

No hidden output-vector argument is used by0x1737b0. Its generator
call **0x173844→0x2b880** passes a transient state pointer in RCX,
new resource in RDX, ID in R8D and modifier in R9D: these are integer
register arguments, despite misleading float types in abbreviated
decompilations.

On a cache miss,0x1737b0 creates a cache node/value and allocates a
**0xa8-byte resource**, calling **0x16e6b0(device,mode1)** at0x17382c
with the store's native device/renderer handle from store+0x08.
It **stores the new resource pointer into the cache before filling it**,
then calls0x2b880. That routine allocates the voxel volume, fills the
chosen shape and calls **0x170700(resource,0) at0x2bf4a** before
returning. The outer lookup returns only after that work.

Useful stable capture stages are therefore:

* **Before0x2bf4a's meshing call:** the generator has finished writing
  its raw RGB volume. The active lookup/generator inputs identify the
  full ID/modifier key.
* **After the generating0x1737b0 call returns:** the returned resource
  is the finished cached result. Its raw RGB allocation/dimensions remain available;
  0x170700 creates mesh data without freeing/replacing that RGB volume.

A nonnull cache value, nonnull RGB pointer or24³ dimensions alone is
**not** a readiness flag: those become visible before all shape writes.
No mutex or separate ready flag is established in0x1737b0. A capture
must be serialized with lookup/generation and store lifetime, such as
observation on the owning thread after the lookup returns. A generic
world/zone snapshot lock has not been shown to protect this cache.
An unsynchronized cache-hit return on another thread does not prove
that the already-published pointer's generating call has finished.

The resource is borrowed native cache storage. Copy its semantic data
while it is live; do not free it, mutate its RGB or retain a pointer
past store destruction/replacement. A cache-miss lookup also invokes
allocation/meshing/native backend work and requires the real initialized
store/device context. It is not equivalent to a read-only memory lookup
against an arbitrary reconstructed store object.

## 3. Voxel allocation, byte layout and meshing metadata

Every branch in generator **0x2b880** first requests dimensions
**24×24×24** through **0x16ff70**. That allocation is zero-filled.
There are **13,824 voxel slots and41,472 RGB bytes**; these are the
full allocation dimensions, not an occupied-vertex bounding box.

| Resource offset | Required meaning/data |
|---|---|
|+0x68|8-byte pointer to raw contiguous RGB allocation|
|+0x88/+0x8c/+0x90|Three signed32 allocation dimensions X/Y/Z, here24 each|
|+0x99|Voxel-classification policy byte; fresh resource has1|
|+0x9a|Mesh color/input mode byte; this constructor receives **1**|
|+0x9c|Signed32 corner-sampling radius, constructor value3|
|+0xa0/+0xa1/+0xa2|Additive shade/color bytes, constructor values0|

Byte address is **3 times ((z times sizeY plus y) times sizeX plus x)**.
X changes fastest, Z slowest. The three bytes are **R,G,B**, with no
per-voxel alpha/type/light bytes and no row padding. Read final raw
bytes, not the generator's float palette intermediates or GPU vertex
colors. The mesher/shader encoding remains a separate stage.

With classification byte+0x99 equal1, only RGB(0,0,0) is empty;
the special pure-red/green/blue attachment-marker exclusions apply
when that byte is0, not to this fresh procedural resource. The
classification is0x16f230. Capture actual metadata with the volume
rather than replacing it with stock-asset assumptions.

**Mode+0x9a matters.** The category0x17 constructor uses mode1, so
the existing0x170530 nonzero-mode rule applies: no stock-CUB half-RGB
step, and its fourth component2 packs as **V byte254**, not255.
See steam-voxel-vertex-inputs §2. This is not a lighting-bypass/glow
flag. Preserve the raw RGB bytes unchanged: do not apply the stock-mode
halving, gamma or vertex AO/light encoding during export. The generator's
own color scaling occurs before byte storage (e.g. §7's half-color
gradient), separately from mesh encoding. A plain RGB asset copy must
retain the mode metadata needed by the independent mesher.

Preserve the **full24³ allocation** even if occupied voxels use a
smaller box. Ordinary item resource placement/scaling uses allocation
dimensions; replacing them with cropped bounds changes centering and
size. Any occupied bounds may be additional diagnostics, not substitute
dimensions. After meshing, dirty byte+0x98 is cleared; it is not an
extra occupancy/color channel in the exported volume.

## 4. Geometry is per-pair procedural data, not just a recolor

The initial counter is existing **R(uint32(ID)+57 times modifier)**,
wrapped32-bit, as item-models3 specifies. Counter storage at transient
state+0x30 is an actual **32-bit integer**: e.g.0x2b8fc writes the
integer and0x2b907 reads it. The first four RGB float palette triplets
consume12 counter values before shape dispatch. Its subsequent
xorshift draws are shared procedural state, not a separate cosmetic
random stream.

**ID0 at0x30790 already proves geometric dependence.** Starting with
counter c after that initial palette, it chooses two radial parameters
**8 minus(X(c) modulo6)** and **8 minus(X(c+1) modulo6)**, and a height
parameter **24 minus(X(c+2) modulo12)**, where X is the existing wrapped xorshift32.
The radial values are3..8 and height13..24. These parameters feed
shape construction; they are not merely RGB assignments. The ID4
dispatch additionally draws four random parameters for0x2daf0.

Consequently one exported mask per ID with per-modifier recoloring is
not the native resource contract. Export the completed per-**full-key**
voxel data or independently implement the shape algorithm. Distinct
keys may happen to produce identical data, but cache identity remains
the full pair. Item rarity/material/realm do not supply missing shape
parameters or authorize a generic prefab replacement.

## 5. Minimal bounded owner-data export contract

A usable data record needs the exact **ID32/modifier32 key**, all three
allocation dimensions, complete ordered raw RGB allocation, and the
native classification/color-mode/radius/shade metadata above. Useful
provenance is the supported executable/RVA source and capture stage
(finished raw generation or completed lookup/meshing), plus a volume
hash/count for verification. Native object/device/tree pointers are
transient diagnostics, not persistent resource identifiers.

Enumerating cache+0x28 captures **only keys already requested**.
An absent key means not cached; it is not proof that the artifact has
no model. All32 modifier bits are meaningful to generation, so an
existing330/660-period ordinary catalog table cannot cover this case.
Needed full keys can be collected from actual item records and resolved
through the bounded native lookup when an owner-side capture is
explicitly coordinated. No native lookup or game run was performed
for this specification.

## 6. Shape-family audit boundary

The acquisition/export, allocation/mode and RNG-dependence contract
above is stable. Sections7–16 specify the shared box/color
primitive and complete ID0/1/2/3/4/5/6 and default-ID families.
No claim is made that one observed instance defines a family, and no
approximate shape substitutes for an unaudited algorithm or uncaptured key.

## 7. Shared box and raw-byte color primitive

**0x2c800** accepts the generator state, a signed integer lower XYZ
corner, three integer extents and two float RGB colors A/B. It visits
the rectangular integer voxel interval **lower inclusive, lower+extent
exclusive**, X outermost, Y next and Z innermost, each ascending.
An extent≤0 produces no writes. Out-of-allocation coordinates are
ignored; in-bounds writes replace prior bytes, including any resulting
black bytes. It does not blend with the existing voxel.

For each Z, the interpolation fraction is the float32 conversion of
**Z−lowerZ divided by extentZ**, using a float32 division. Thus the
last plane uses(extentZ−1)/extentZ, not1. For each channel, evaluate
in float32: A times(1−fraction), then B times fraction, add those two
products, then **multiply the sum by0.5**. Clamp the result to0..255,
truncate toward zero and store that byte. No intermediate conversion
to an eight-bit palette is performed. The scalar constants are:

| RVA | Float32 word/value |
|---|---|
|0x4ac980|0x3f800000 /1|
|0x4ac964|0x3f000000 /0.5|
|0x42e7d8|0x437f0000 /255|

The sphere0x2fc60 and faceted-solid0x2daf0 use the **same channel
interpolation, half scaling, clamp and truncation**, but their fraction
is float32 **(Z−centerZ+radius)/(2 times radius)**. The calls in§§9–10
use radius8; later families use their separately stated positive radii. There is no final
re-normalization based on the occupied extent. These are source voxel
bytes; mode+0x9a=1 subsequently prevents a second stock-CUB half-color
step in mesh encoding.

## 8. ID3 complete family: centered random rectangular solid

Entry **0x2ffd0**, dispatched by0x2bc7c. Let c be the counter after
the twelve initial palette draws. It consumes **three** xorshift
draws in this order:

| Dimension | Extent for the native24³ allocation |
|---|---|
|X|6 plus(X(c) unsigned modulo12)|
|Y|6 plus(X(c+1) unsigned modulo12)|
|Z|6 plus(X(c+2) unsigned modulo12)|

Each extent is6..17; counter becomes c+3 with32-bit wrap. The lower
corner in each axis is **(24−that extent)/2, integer division toward
zero**. Call §7's box primitive once with those dimensions, first
initial palette as A and second initial palette as B. The other two
palettes are unused by this family. No holes, bevels, smoothing,
extra random draws or subsequent geometry passes occur in0x2ffd0.
The ordinary0x2b880 finishing mesher then runs as in§2.

## 9. Default-ID complete family: half-voxel-centered sphere

The unsigned dispatch comparison at **0x2bc36** admits only IDs0..6
to their listed cases. Every other32-bit ID, including negative IDs,
takes **0x2be40→0x2fc60**. Its full ID still affected the initial
seed/cache key; this branch does not change the key to7.

After the initial twelve palette draws, consume three more draws.
Replace first palette A with **R=X(c+2) bitwise-and127,
G=X(c+1) bitwise-and127, B=X(c) bitwise-and127**, each converted
to float32. Counter becomes c+3. Supply a temporary second color
equal to A times0.5; the other original palettes are unused.

The sphere primitive gets integer center **(12,12,12)** and radius
**8**. It visits candidate integer coordinates center−8 through
center+8 **inclusive**, X/Y/Z ascending in that nested order. A voxel
is written exactly when the sum of the squared components of
**(voxelXYZ−integerCenter−(0.5,0.5,0.5))** is **≤64**. Native
float32 sum order is Z square plus X square, then Y square; these
bounded half-integer terms are exactly representable. The half-unit
offset is part of the predicate; it is not an integer-centered
radius test or a later model translation.

Accepted voxels use §7's radius-gradient color rule with A and
A times0.5, writing into the initially black24³ allocation. No
additional randomness, plane clipping, shell-only pass or erosion
occurs. Here the geometric mask is fixed by the branch/radius;
the key controls its color. This does not undo other families'
verified geometric RNG dependence.

## 10. ID4 complete family: random planar-faceted solid

Dispatch **0x2bc86→0x2be1c**, primitive **0x2daf0**. After the
twelve initial palette draws, take four more X draws, reduce each
by **unsigned modulo1000**, convert that nonnegative integer to
float32, and **divide by float32 1000** (word0x447a0000 at
RVA0x42e5d4; native DIVSS, not reciprocal multiplication). Counter
advances by4. The coefficients' geometric use reverses draw order:

| Coefficient | Source |
|---|---|
|XY pair|X(c+3) modulo1000, divided by1000|
|XZ pair|X(c+2) modulo1000, divided by1000|
|YZ pair|X(c+1) modulo1000, divided by1000|
|XYZ sum|X(c) modulo1000, divided by1000|

This order is confirmed by outgoing stack slots+0x38/+0x40/+0x48/
+0x50 at0x2bda8..0x2be04, then the callee's actual parameter uses.
The color A is the unchanged **first** initial palette. Temporary
B equals A times0.5; it does not select the second initial palette.

The primitive has integer center(12,12,12), radius8 and candidate
coordinates4..20 inclusive in each axis, X outermost/Y next/Z
innermost. Set x/y/z to the absolute integer coordinate distances
from this center, converted to float32. A voxel is written when
**all seven** values below are≤8:

* x, y and z individually;
* (x+y) times the XY coefficient;
* (x+z) times the XZ coefficient;
* (y+z) times the YZ coefficient;
* ((x+y)+z) times the XYZ coefficient.

The native max-selection sequence implements that inclusive test.
There is **no sphere predicate or0.5 center subtraction** in this
family. Each sum is formed before its coefficient multiplication;
the operations use float32. Accepted voxels use §7's radius-gradient
color rule with the integer signed Z distance, A and A times0.5.
No additional draw or geometry pass follows before the common mesher.

## 11. ID6 complete family: shaft, collars and faceted head

Entry **0x2f450**, dispatched at0x2be36. Every draw below uses the
same counter left after the initial twelve palette draws; call it c.
There are **18 further draws**, including one generated but unused
head palette. The following definition is used twice here and in§12:
a three-draw **warm RGB palette** beginning at d has R=180 plus
(X(d+2) unsigned modulo70), G=120 plus(X(d+1) modulo130), and
B=10 plus(X(d) modulo100). Channels become float32; counter advances3.

The ordered construction is:

1. Draw warm palettes A at c..c+2 and B at c+3..c+5.
2. Head radius is **3 plus(X(c+6) modulo3)**, hence3..5. Define
   shaft height H as **23−2 times radius**, hence17/15/13.
   Use §7's box primitive at lower(11,11,0), extent(3,3,H), colors A/B.
3. Collar count is **1 plus(X(c+7) modulo3)**, hence1..3. For each
   index i from0 through count−1, float32 f=i/count and q=1−f.
   Build a temporary color B times q plus A times f, componentwise
   in float32. Place a5×5×1 box at lower(10,10,trunc32(H times q)),
   with that same temporary color as both endpoints. The first collar
   is therefore at Z=H, not at the shaft's lowest plane. Collar boxes
   overwrite earlier voxels and do not consume extra random values.
4. Replace A with a standard RGB palette from c+8..c+10, and B from
   c+11..c+13: the standard palette uses reversed channel order and
   unsigned modulo250 plus10, as item-models §3 specifies.
   **The newly drawn B is not used for the head, but its three draws
   are still consumed.**
5. Draw four planar coefficients from c+14..c+17 exactly as§10,
   including reversed draw-to-plane association and float32 division
   by1000. Supply A and temporary A times0.5 to0x2daf0, center
   **(12,12,H+radius)** and the chosen radius. Its candidate interval,
   seven-plane predicate and gradient are§10 with that radius/center
   substituted for8/(12,12,12). It overwrites earlier shaft/collar
   voxels wherever the head is admitted.

The counter ends at c+18. No additional random draws, attachment
metadata, hollowing or post-head geometry pass occurs before meshing.
In particular, omitting the unused second head palette changes the
head's shape coefficients and is observably different.

## 12. ID5 complete family: ornament boxes followed by scalloped ring

Entry **0x2cae0**, dispatched at0x2be29. Let c again be the counter
after the initial twelve palette draws. It consumes **13** additional
draws. The construction order is important because the final ring
overwrites intersecting ornament voxels.

### 12.1 Parameters and ornament placement

The ring radius r is **9 minus(X(c) unsigned modulo3)**, hence7..9.
Ornament count n is **2 plus2 times(X(c+1) modulo3)**, hence2/4/6.
Ornament box edge s is **2 plus the low bit of X(c+2)**, hence2/3.
Draw a standard RGB palette C from c+3..c+5. Original palette colors
are replaced/unused by this family.

For each ornament index i=0..n−1, its angle is produced as follows
(precision/order is part of the contract): convert i and n to float32;
subtract **n+n** from i, divide by n in float32, then add that result
to itself in float32. Promote this doubled value to double, multiply
by double π, add double π/2, then narrow to float32 for native cosf
and sinf. The constants are **0x400921fb54442d18 atRVA0x42e758**
and **0x3ff921fb54442d18 atRVA0x42e740**. Do not replace this
with a differently reduced positive angle if byte-identical lattice
placement is required.

Multiply cosf(angle) by float32(r+1), add float32 12, then add1
as a separate float32 operation; truncate the result toward zero,
then subtract integer floor(s/2) to obtain lowerX. LowerY instead
uses sinf(angle) times float32(r+1), plus precomputed float32 **13**,
then truncation and the same integer subtraction. LowerZ is **8**.

Draw §7's s×s×s box with A=C and B=C times0.5. Then replace its
single far-corner voxel **lower+(s−1,s−1,s−1)** with a highlight:
each channel is **(C+150) times0.5**, float32 addition then
multiplication, clamped0..255 and truncated toward zero. Constant150
is word0x43160000 atRVA0x42e938. The ordinary allocation clipping
still applies to both box and highlight; do not expand/crop24³ to
fit an ornament at its boundary. No random values are consumed by
the repeated ornaments.

### 12.2 Ring footprint, variable top and gradient

After all ornaments, draw warm palettes A from c+6..c+8 and B from
c+9..c+11 using§11's definition. Let p be **2 plus(X(c+12) modulo6)**,
hence2..7; counter then ends at c+13.

Visit candidate integer X/Y from12−r through12+r inclusive, X outer
and Y inner. With x=X−12 and y=Y−12, the footprint admits exactly
those cells whose float32 **(y−0.5)² plus(x−0.5)²** is both
**≤r² and≥(r−1)²**, inclusive. It is a half-voxel-centered annulus,
not an integer-centered square or a filled disk.

For an admitted X/Y, form these two nonnegative integer distances:
the absolute value of **p minus the signed remainder of(X+12−p)
divided by2p**, and the same expression using Y. The endZ is
**8+2p minus the Y distance minus the X distance**, in that order.
Fill Z **from6 inclusive through endZ exclusive**, ascending.
The native bounded inputs make these remainder dividends positive;
the underlying operation is signed remainder, not a hash or random draw.

Each column uses §7's raw-byte color operations with A/B and fraction
**float32(Z−6) divided by float32(endZ−6)**. Thus the top written
voxel does not reach fraction1. It overwrites intersecting ornaments;
there is no final cap/ornament repair or more randomness. The common
mesher follows. The trigonometric call arguments and float32 staging
are verified; the C-runtime implementation's internal sinf/cosf
approximation has not been independently specified here. Completed
owner RGB captures remain the exact oracle for lattice-edge cases.

## 13. Additional shared solid primitives

All primitives below ignore out-of-allocation writes and overwrite
previous bytes. Channel interpolation/half scaling/clamp/truncation
is§7, with the fraction given below. They consume no RNG draws.

* **0x2f0f0, XZ annulus extruded along Y:** inputs are X/Z integer
  center, lowerY, outer radius r, Y extent e, radial thickness t,
  and colors A/B. Candidate X/Z are center−r through center+r
  inclusive. Admit the half-voxel-center squared distance
  **(Z−centerZ−0.5)²+(X−centerX−0.5)²** when it is≤r² and
  ≥(r−t)², both inclusive. Fill Y from lowerY inclusive to lowerY+e
  exclusive. Native nesting is X, Z, Y. Color fraction is
  float32(Z−centerZ+r) divided by float32(2r), not Y-based.
* **0x2d7b0, filled XZ disc extruded along Y:** same interval,
  outer-distance predicate, order and fraction, but **no inner-radius
  exclusion**.
* **0x30130, upper half of that XZ disc:** same as0x2d7b0 except
  candidate Z starts at centerZ and ends at centerZ+r inclusive.
  It is not a hollow ring; the half-unit center shift still applies.
* **0x30460, upper YZ disc extruded along X:** exchange X/Y roles
  in0x30130. Y is the outer loop, Z next, X innermost from lowerX
  inclusive to lowerX+extentX exclusive. Predicate is
  (Z−centerZ−0.5)²+(Y−centerY−0.5)²≤r²; fraction still uses Z.
* **0x2d430, filled XY disc extruded along Z:** candidate X/Y are
  center−r through center+r inclusive; admit
  (Y−centerY−0.5)²+(X−centerX−0.5)²≤r². Fill Z from lowerZ
  inclusive to lowerZ+extentZ exclusive. Nesting is X,Y,Z;
  fraction is float32(Z−lowerZ) divided by float32(extentZ).
* **0x2fb80, single voxel:** multiply supplied float RGB by0.5,
  clamp each channel0..255, truncate toward zero, then write one
  in-bounds resource voxel. It accepts the **resource**, not the
  generator-state pointer. No interpolation or extra randomness.

These finite-radius tests use float32 arithmetic. No resource-origin
translation or occupied-bound fitting is hidden inside them.

## 14. ID2 complete family: band and one or repeated settings

Entry **0x2e490**, dispatched at0x2bc6f. Let c be the post-initial-
palette counter. Retain initial first palette A and third palette C.
Replace the second palette with M: each channel is **100 plus an
unsigned modulo70** draw, R from c+2, G from c+1, B from c.
Then choose:

| Parameter | Draw/value |
|---|---|
|Radius R|6 plus(X(c+3) modulo4), hence6..9|
|Band width W|1 plus(X(c+4) modulo5), hence1..5|
|Thickness T|1 plus(X(c+5) modulo3), hence1..3|
|Band style|X(c+6) bitwise-and3|

The counter is now c+7. Define lowerY **Y0=12−floor(W/2)**.
Every band call below is0x2f0f0, centered at **X=12,Z=12**.
Unless C is stated, its colors are M and **M times0.75**;
C calls use C and C times0.75. Constant0.75 is word0x3f400000
atRVA0x42e710. Calls occur in listed order:

| Style | Ordered band calls: radius; lowerY; width; thickness; palette | Later effective radius E |
|---|---|---|
|0|R;Y0;W;T;M|R|
|1|R−1;Y0;W;T;C → R;Y0−2;2;T+1;M → R;Y0+W;2;T+1;M|R−1|
|2|R+1;Y0;W;T+1;M → R;Y0−2;2;T+1;M → R;Y0+W;2;T+1;M|R|
|3|R−1;Y0;W;T;C → R−1;Y0−2;1;T;M → R;Y0−1;1;T+1;M → R;Y0+W;1;T+1;M → R−1;Y0+W+1;1;T;M|R−1|

E is an input to the later setting; do not substitute the largest
radius drawn by that style. The first setting decision consumes
**X(c+7)'s low bit**.

### 14.1 Low bit0: one setting above the band

Choose setting radius **a=floor(W/2)+1+(X(c+8) bitwise-and1)**,
height **h=1+(X(c+9) bitwise-and1)**, and square/disc by
**X(c+10) bitwise-and1**. Counter ends c+11. Construction order:

* For square bit1: draw a constant-M box at lower
  **(12−a,11−a,13+E)**, extent **(2a+2,2a+2,1)**; then a
  constant-A box at lower **(13−a,12−a,14+E)**, extent
  **(2a,2a,h)**. Finally single-voxel helper0x2fb80 with color
  **A+150 per channel** at **(12+a,11+a,13+E+h)**.
* For disc bit0: draw0x2d430 centered X/Y=(12,12), lowerZ=13+E,
  radius a+1, extentZ1, constant M; then the same primitive at
  lowerZ=14+E, radius a, extentZ h, constant A. Finally use
  0x2fb80 with A+150 at **(trunc32(13−float32(a) times0.5),
  the same Y,13+E+h)**. The multiply and subtraction are float32.

“Constant” supplies the same color as both endpoints; the shared
primitive still applies its half scaling. Neither case uses the
original second palette, adds a spirit attribute or consumes another
random value.

### 14.2 Low bit1: repeated cubic ornaments around the XZ circle

Choose count **n=2+(X(c+8) bitwise-and3)**, hence2..5, and cube
edge **s=2+(X(c+9) unsigned modulo W)**. W≥1, so this divisor is
never zero. Counter ends c+10. Each i=0..n−1 uses§12.1's exact
float/double angle-production sequence with this n.

LowerX is **trunc32(cosf(angle) times float32(E+1) plus13.5)
minus floor(s/2)**. LowerZ substitutes sinf. LowerY is
**12−floor(s/2)**. The precomputed center13.5 is12 plus float32
**1.5** (word0x3fc00000 atRVA0x42e548); do not silently substitute
12.5 or Crown's13. Write the s³ box with A and A times0.5, then
single-voxel helper0x2fb80 with **A+150** at its far corner
lower+(s−1,s−1,s−1). Iterate ascending i; later ornaments overwrite
earlier data. There are no further RNG draws or geometry passes
before the common mesher. The same explicit CRT-trig boundary as§12
applies to exact lattice-edge comparison.

## 15. ID1 complete family: body variants followed by noise-shaped chains

Entry **0x2bf90**, dispatched at0x2bc62. Let c be the counter after
the initial palettes; retain first palette A. Choose **k=X(c)
unsigned modulo3**, **r=k+3**, **Zc=k+5=r+2**, and orientation bit
**b=X(c+1) bitwise-and1**. Replace the second palette with gray M:
R=100 plus(X(c+4) modulo50), G=100 plus(X(c+3) modulo50),
B=100 plus(X(c+2) modulo50). Choose style **X(c+5) bitwise-and3**.
Counter ends at **c+6**; the entire remaining body/chain construction
consumes no further counter values. Initial palettes three/four are
unused.

All constant-M primitives receive M for both colors. Every main
A-colored body below instead receives A and temporary A times0.5.
Sphere means0x2fc60's half-voxel-center predicate/gradient with the
stated center/radius, as§9; box/disc helpers are§§7/13. All calls
listed for a branch occur in that order.

### 15.1 Body style1

* b=1: upper-XZ half-disc0x30130 centered X=12/Z=Zc, lowerY12,
  radius r+1, extentY2, constant M; then upper-YZ half-disc0x30460
  centered Y=12/Z=Zc, lowerX12, radius r+1, extentX2, constant M.
* b=0: sphere at **(12,12,Zc+r)**, radius integer floor(r/2),
  constant M.
* Either b then draws the main sphere at **(12,12,Zc)**, radius r,
  colors A/A times0.5. No extra highlight point follows this style.

### 15.2 Body style2

* b=1: constant-M box lower **(11−r,11,Zc)**, extent
  **(2r+2,2,r+1)**, then box lower **(11,11−r,Zc)**, extent
  **(2,2r+2,r+1)**.
* b=0: let q be integer floor(3r/4). Draw constant-M box at
  **(13−q,13−q,Zc+r+2)**, extent **(2q−2,2q−2,1)**, then box
  at **(12−q,12−q,Zc+r+1)**, extent **(2q,2q,1)**.
* Either b then draws the main box lower **(12−r,12−r,2)**,
  extent **(2r,2r,2r)**, colors A/A times0.5. No highlight follows.

### 15.3 Body style3

* b=1: constant-M box at **(11−r,12,Zc)**, extent **(2r+2,1,r+1)**.
* b=0: constant-M box at **(11−r,11,1)**, extent **(2r+2,2,2r+2)**.
* Set Yg=12 for b1,13 for b0. Draw the main box lower
  **(12−r,Yg,2)**, extent **(2r,1,2r)**, colors A/A times0.5.
  Then single-voxel0x2fb80 with **A+150** at **(10+r,Yg,2r)**.

### 15.4 Body style0

* b=1: upper-XZ half-disc0x30130 centered X=12/Z=Zc, lowerY12,
  radius r+1, extentY1, constant M.
* b=0: filled-XZ disc0x2d7b0 centered X=12/Z=Zc, lowerY11,
  radius r+1, extentY2, constant M.
* Set Yg as in style3. Draw filled-XZ disc0x2d7b0 centered
  X=12/Z=Zc, lowerY=Yg, radius r, extentY1, colors A/A times0.5.
  Then single-voxel0x2fb80 with **A+150** at **(10+r,Yg,2r)**.

### 15.5 Shared final chain points: exact coordinate/precision contract

Every style next emits the chains **after** its body/highlight.
This is point sampling, not a continuous line rasterizer, mesh tube
or interpolation between adjacent generated points. Set:

* Z0=**Zc+r=2r+2**;
* remaining=24−Z0, sample count L=4 times remaining;
* spread=float32(remaining) **divided by float32 3**.

For each integer sample i from0 through L−1, compute t=float32(i)
divided by float32(L−1), then **u=t times t**, float32. Sample Z is
**float32(i) times0.25 plus float32(Z0)**, with separate float32
multiply/add. Promote u to double, subtract double0.5, add that
difference to itself in double, take absolute value by clearing the
double sign bit, then narrow to float32 v. Define q as **float32
1 minus float32(v times v)**. This double/narrow boundary is native
0x2c6c4..0x2c6e3; do not silently reshape the curve to linear t.

Noise is existing double-input/float-output **0x2e0c0**, independently
specified in [worldgen-terrain §1.4](worldgen-terrain.md). Its arguments
are, in double operation order:

* X: double(q) times0.25, plus **signed32 modifier converted to
  double**, then plus8741;
* Y: **signed32 wrapped(modifier times13) converted to double**,
  plus double(sampleZ) times0.25, then plus32992.

Both signed conversions and the32-bit multiply are explicit in
0x2c680..0x2c710. They differ from interpreting the modifier as an
unsigned real value even though the cache retains all its bits.
These offsets are double8741 atRVA0x42e928 and double32992 at
0x42e930. The noise is independent of world seed, clock or further RNG.

Let span be float32(q times spread). For jitter, first evaluate
**float32(u times0.5), then that times spread, then multiply the
result by the returned float32 noise**, each operation float32.
Native0x2c720..0x2c740 establishes this grouping; the algebraically
equivalent noise-times-u-first chain is not the instruction order.

Write two points per sample using0x2fb80 and color M, in this order:
right X is trunc32 of **(12.5 plus span) plus jitter**; left X is
trunc32 of **(12.5 minus span) plus jitter**, all float32 operations.
Both use **Y=12**, Z=trunc32(sampleZ). Thus jitter displaces both
sides in the same X direction; it is not negated on the left.
Duplicate writes replace prior bytes. The single-voxel helper supplies
the usual half-color/clamp/truncation; no connecting geometry or more
shape pass follows before the common mesher.

## 16. ID0 complete family: curved vessel, patterns and handles

Entry **0x30790**, dispatched at0x2bc55. Let c be the counter after
the initial twelve palette draws. Preserve the **fourth initial palette
D**; the other three are replaced below. Draws/parameters are:

| Source | Meaning |
|---|---|
|X(c) unsigned modulo6|R0=8 minus this remainder, hence3..8|
|X(c+1) modulo6|R2=8 minus this remainder, hence3..8|
|X(c+2) modulo12|Height H=24 minus this remainder, hence13..24|
|X(c+3) modulo3|Profile mode0/1/2|
|X(c+4) bitwise-and3|Color pattern0/1/2/3|
|c+5..c+7|New standard palette A: R from c+7, G from c+6, B from c+5, each modulo250 plus10|
|X(c+8) modulo3|Handle count0/1/2|
|X(c+9) modulo(floor(H/4)+1)|First handle Z layer S|
|X(c+10) bitwise-and1|Use D on the top two body layers|
|X(c+11) bitwise-and1|Use D on the bottom two body layers|
|X(c+12) bitwise-and1|Use D for handles|

Counter ends **c+13**. Palette B is A plus float32 **30** per
channel, not clamped at that stage (constant0x41f00000 atRVA0x42e5a8).
Palette C is the native hue-shifted derivative in§16.1. There are no
more RNG draws in the layer, pattern or handle operations.

### 16.1 Derived palette C: native mixed-precision HSV round trip

At **0x30b26/0x30b4c/0x30b51**, divide A's three float channels by
float32 **255**, actual DIVSS operations. Pass that normalized triple
through RGB→HSV **0x2e360**, add **float32 180** (word0x43340000
atRVA0x42e7d4) to returned hue,
then HSV→RGB **0x2df40** and multiply each returned float channel by
float32 255. The multiplied results are C; they are not first
converted to bytes. The shader or standard color-library hue wrapping
is not an interchangeable substitute for this sequence.

For the reached finite positive RGB inputs,0x2e360 takes min/max
of the supplied float channels, promotes selected extrema to double,
and computes delta=max−min in double. Returned V is max narrowed to
float. If delta<**double1e−5** (word0x3ee4f8b588e368f1 atRVA0x42e8d0),
it returns H=0,S=0. Otherwise S is double(delta/max) narrowed to
float. Hue branch priority is red maximum, then green maximum,
otherwise blue maximum. Form its numerator **as a float subtraction**:
G−B for red, B−R for green, R−G for blue. Promote that numerator
to double and divide by delta; add double0/2/4 respectively, then
narrow that result to float. Promote this float, multiply by double60,
narrow to float; if negative, promote/add double360/narrow again.

After the caller's float180 addition,0x2df40 returns RGB=(V,V,V)
when S≤0. Otherwise promote hue to double and, **if hue≥360, set
it to0**, rather than subtracting360. This native reset is important:
it collapses the upper half of shifted hues to sector0. Divide by
double60; sector is trunc32 of that quotient and fraction is the
quotient minus sector. With S/V promoted to double, compute three
double intermediates:

* low=(1−S) times V;
* falling=(1−S times fraction) times V;
* rising=(1−(1−fraction) times S) times V.

Sector outputs before narrowing each selected channel to float are:
0=(V,rising,low),1=(falling,V,low),2=(low,V,rising),
3=(low,falling,V),4=(rising,low,V),5=(V,low,falling).
The reached hue range produces only these sectors. This specifies
the active positive-input subset, not the helper's unreachable
negative/NaN saturation cases. Constants double2/4/60/360 are at
0x4ac9d8/0x42e8e8/0x42e8f0/0x42e908; double1 is0x4ac9b8.

### 16.2 Ordered layers, radius and hollow wall

Visit integer Z from0 through H−1. At each layer, float32
**t=float32(Z)/float32(H−1)**. First compute the base layer color L
as **A times(1−t) plus B times t**, separately per float32 channel.
This unwarped t controls color even when the profile mode differs.

The radius parameter p is t for profile0, t times t for profile1,
or **1−(1−t) times(1−t)** for profile2, all float32. Set q=1−p.
Radius is the float32 sum of two terms, preserving this grouping:
**(q times(R0 times0.5) plus p times R0) times q**, plus
**(q times R0 plus p times R2) times p**. Each multiply/add is a
separate float32 operation; do not replace it with an expanded
quadratic or evaluate it in double. Native0x30cc1..0x30cf9 fixes
this evaluation order.

Candidate X begins at trunc32((12−radius)−2), continues ascending
while float32(X)≤**(12+radius)+2**. Candidate Y has the same lower
expression, with upper limit **(radius+2)+12**, and is the inner
loop. These float32 bounds are0x30cfd..0x30d67. For a candidate,
the squared radial distance is float32 **(Y−12+0.5)² plus
(X−12+0.5)²**. The **plus0.5** places the test center at11.5;
do not reuse the minus0.5 center of§13's disc primitives.

Admit distance≤radius². At Z>0 also require distance≥(radius−1)²,
inclusive. Z0 is therefore a filled base disk; subsequent layers
are one-unit-thick annuli. The top remains open: a color-band flag
does not fill its hole. There is no post-fill erasure/erosion pass.

### 16.3 Body color patterns and overrides

For each admitted body voxel, begin with L and select:

* Pattern0: retain L.
* Pattern1: use C when **floor(Z/2) unsigned modulo3 is0**, else L.
* Pattern2: use C only when both **(floor(Z/4)+trunc32(Y/2)+
  trunc32(X/2)) is even** and **Z bitwise-and2 is0**; else L.
* Pattern3: call existing float-output/double-input noise0x2e0c0
  twice, with signed32 modifier m converted to double:
  first at **((double(Y) times0.3 plus155) plus m,
  (double(Z) times0.5 plus1003) plus m)**;
  second at **((double(X) times0.3 plus3411) plus m,
  (double(Z) times0.5 plus2131) plus m)**.
  Sum the two returned floats, clamp that float sum0..1, cube it
  using float32 multiplications, and mix each channel as
  **L plus(C−L) times the cubed weight**, float32. The two Y
  noise offsets differ; the second call's argument is not omitted
  or reused unchanged. Native0x30e11..0x30e90 verifies both.

After the pattern, replace the color by D if its top-band flag is
set and Z≥H−2; then replace by D if the bottom-band flag is set
and Z<2. These are color overrides on admitted voxels only. Multiply
the selected float RGB by0.5, clamp0..255, truncate toward zero and
store. Noise double offsets155/1003/2131/3411 are atRVA0x42e900/
0x42e910/0x42e918/0x42e920; double0.3 is0x42e8d8 and0.5 is0x4ac9a8.

### 16.4 Handles fill empty cells after that layer's body

After the body of a layer, handles run only if count>0 and
**S≤Z≤H−1**. Float32 handle fraction f is(Z−S)/(H−1−S), with
integer differences converted before division. This denominator
is positive for every native parameter combination above. Compute
g=**(0.5−f)+(0.5−f)** in float32, bulge=**(1−g times g) times5**
in float32. Right coordinate Q is trunc32 of **bulge plus
((12+0.5)+radius)**; the center sum and radius addition occur
before the final bulge addition (0x31102..0x3112d). The constant5
is float word0x40a00000 atRVA0x42e584.

Handle RGB comes from the **unpatterned base L**, or D when the
handle-color flag is set. Top/bottom body color flags do not alter
this choice. Apply the same half-color/clamp/truncation as a point
write, but write only when the candidate's existing stored RGB is
exactly(0,0,0). Nonblack body or earlier handle data is preserved.
Out-of-bounds reads behave as black for this test; out-of-bounds
writes are still ignored, so geometry is clipped to24³.

At Y=11 then12, try **X=Q, then Q−1**, at current Z. If count2,
then form **M=24−Q** and, at Y=11 then12, try **X=M−1, then M**.
This is a full-width reflection, not23−Q, and does not replace the
right handle. Each candidate independently performs the black-only
test. The zero read is native BSS storage atRVA0x55284c, not an
asset color or arbitrary uninitialized byte. Continue to the next
Z layer; there is no final handle connection/repair pass.

## 17. Verification/coverage boundary

All eight dispatch-family caller algorithms and their reached
primitive/color operations are specified above. No original-game
execution or native generated-volume comparison was performed for
this audit. The independently specified simplex noise/tables and
the explicit CRT sinf/cosf boundary remain linked dependencies;
compiled native constants/operation orders here are source evidence,
not runtime capture claims.

For owner-data verification, capture full key+metadata+RGB using§§1–5,
and label the family/branch parameters from these definitions. Useful
bounded coverage includes every ID0 profile/pattern/handle count and
clipped-handle cases, all ID1 styles/orientation bits, all ID2 styles
and setting branches, ID5 edge/count combinations, and ID6's radii/
collar counts. Preserve native completed volumes as the oracle;
do not replace branch coverage with one mask per family or silently
accept a geometric near-match when exact voxel data is available.
