# Steam scene lighting and mesh visibility cache

Independent clean-room specification from the owner's Steam native decompile,
assembly and embedded constants. This author reads no engine or test sources,
edits only this prose, and has not run the game. Addresses are RVAs. Fixed
positions are signed64 with65536 units per block. Numeric model/object IDs are
hexadecimal when prefixed0x. This continues `worldgen-parity-gaps.md` §9;
new findings here do not turn mesh-time results into generation fields.

## 1. Cached zone mesh fields, readiness and capture coherence

Sources: zone constructor **0x2f9400**, mesh-cache constructor **0xc25c0**,
mesh job **0xe9530**, publication worker **0x842d0**, and renderer **0xd3170**.

| Zone offset | Native type | Meaning |
| --- | --- | --- |
| +0x1c8 | pointer | Active mesh-group linked-list sentinel; not a group pointer/count itself |
| +0x1d0 | **unsigned64 element count** | Number of active mesh groups;0xd3732 tests this whole qword for nonzero before model/beam zone visibility |
| +0x1d8 | pointer | Pending mesh-group linked-list sentinel |
| +0x1e0 | unsigned64 element count | Number of pending groups awaiting publication |
| +0x1e8,+0x1f0,+0x1f8 | three signed64 | Cached minimum XYZ, fixed units |
| +0x200,+0x208,+0x210 | three signed64 | Cached maximum XYZ, fixed units |
| +0x218,+0x220,+0x228 | three signed64 | Cached center XYZ, fixed units |
| +0x230 | float32 | Cached radius, blocks |
| +0x234 | float32 | Per-frame squared camera distance to cached center, blocks² |
| +0x238 |32-bit field | Initialized0 by the mesh-cache constructor; no readiness interpretation assigned here |
| +0x23c | byte | Mesh dirty/request flag; not an initialized-bounds bit |
| +0x444 | signed32 | Weighted mesh triangle cost; initialized0, then mesh job sums group+0x50 +2×group+0x48 +group+0x44 (§2.6); include under the same mesh lock in a cache-metadata capture |

**Construction:**0x2f9400 calls0xc25c0 at zone+0x1c0. That helper constructs
two empty linked lists and sets both64-bit counts0. It initializes+0x238,
the dirty byte and temporary-light count, but **does not initialize the
min/max/center/radius fields**. Consequently freshly generated zones cannot
be assumed to have zero or meaningful cached bounds. A record exporter can
preserve raw bytes, but must not label these values as initialized generation
data or validate them against a generator's guessed bounds.

**Mesh installation:**0xe9530 constructs GPU-resource wrappers and installs
the new bounds/center/radius while holding the world's critical section at
**world+0x3c8**. Its lock/unlock helpers are **0x2abb70 /0x2b9890**. The
minimum/maximum stores are **0xec804–0xec82b**. The same locked portion clears
and rebuilds the **pending** group list at+0x1d8/+0x1e0 and clears dirty+0x23c.
It does not synchronously replace the active group list at+0x1c8/+0x1d0.

**Publication:** after the meshing tasks complete,0x842d0 prepares pending
resources, then replaces the active list from the pending list while holding
that same world+0x3c8 critical section. It subsequently destroys the old
active resources and empties pending nodes. Thus an already rendered zone can
temporarily have **new bounds and pending groups alongside old active groups**.
A nonzero active count is the renderer's readiness test; it does not identify
which rebuild produced a cached bound. Pending count>0 can identify a mesh-built
but not yet published first mesh. There is no separate initialized-bounds
flag in these audited routines. A completed empty mesh need not leave either
count nonzero, so the counters are not a universal historical completion log.

**Capture rule:** a coherent read of the nine signed64 values and active count
needs the native **world+0x3c8** critical section, together with the caller's
normal zone-pointer/lifetime protection. The separate world+0x3f0 critical
section, entered by0x2abb80 and left by0x2b98a0, guards other world/zone-map
operations; it is not a substitute for+0x3c8 here. This spec has not inspected
the implementer's existing snapshot locks. If they already hold+0x3c8 around
the field read, no additional mesh-field lock or guessed validity flag is
required by this trace. Exporting active count, pending count, dirty byte and
a labeled capture phase preserves more information than a single ready bool.

### 1.1 Exact bounds prepass and band/group count

0xe9530's bounds prepass follows light solving and model light sampling. It
visits the target zone's **x ascending outer, y ascending inner**, each over
the64 integer columns. The cached column accessor is0xdfb50; effective block
queries use0xda660/0xeb90, including implicit sea-water behavior. This is not
a scan only of stored solid block centers.

Start lowest exposed Z=0, upper tracker=0 and an unseen-exposed-block state.
For each available target column, preserve its own exclusive stored end
**base+stored block count**. Find the smallest stored base among that column
and its available **west,east,south,north** neighbors. Before scanning Z, if
the **north(y+1)** neighbor is available, raise the upper tracker to at least
that neighbor's exclusive stored end. This north-end update is literal native
behavior; it is not performed for each of the other three neighbors.

Visit Z ascending from **smallest base−1** through **own exclusive end−1**.
An effective non-air block qualifies when at least one of its six neighbors
is air(type0), or when the current block is not water(type2) and a neighbor
is water. The tested order is west,east,south,north,down,up; later tests can
be skipped once the block qualifies. A water block next to water alone does
not qualify. Types/flags are obtained from the effective getter, not by treating
the current raw column's unallocated positions as plain air.

For the **first qualifying block in the whole prepass**, assign both lowest
exposed Z and upper tracker to its Z and mark the exposed state seen. This
assignment can replace an upper value raised by the preceding north-end
update. For every later qualifying block, lower lowest exposed Z if necessary
and raise upper tracker if necessary. North-end updates for subsequent columns
still occur before their Z scan. Preserve this order rather than taking a
single unordered maximum over all stored ends and exposed blocks.

At the end, raise upper tracker to **at least1**. Cached block-coordinate
minimum is **(64×zoneX,64×zoneY,lowest exposed Z)**. Cached maximum is
**(64×zoneX+65,64×zoneY+65,max(0,upper tracker+1))**. Convert all components
to fixed signed64 by multiplying by65536. The extra horizontal extent is
**65**, not64. No ordinary model or beam extent is included.

The cached center is the midpoint of each fixed minimum/maximum pair, using
integer division toward zero. These bounds are integer blocks, so half-block
centers are exactly representable in fixed units. For the cached radius,
convert each center-minus-maximum fixed difference to float32 blocks, square
and sum the three components, take sqrtf, then multiply by **float1.5**.
The result at+0x230 is not just the box's unmultiplied half diagonal.

The prepass defines **N=trunc((upper tracker−lowest exposed Z)/250)+1** height
bands, with group base heights **lowest exposed Z+250×band index**. The mesh
uses250-block bands. Shared vertex and per-surface index arrays are built per
band; water, solid and the extra surface categories share that band's vertex
buffer. A pending mesh-group record is created **only for a band whose shared
vertex-buffer resource is non-null**, i.e. it has generated vertices. Its base
height is stored at group+0x5c. Empty intermediate bands do not contribute
groups merely because they are counted in N.

Thus the pending count is the number of nonempty generated vertex-buffer
bands, **not N unconditionally**, not a pointer and not a triangle count.
Publication copies that sequence into the active list; the active64-bit count
then equals the published nonempty-band count. Source: prepass **0xe9530**,
pending group construction in its final locked portion, and publication
**0x842d0** using list helpers0x7dc20/0x7cc20. The full face-writing rules and
light bytes remain in the existing terrain/water specs; their geometry can
affect whether a particular band is empty.

The point-light collector and complete palette are specified below, with
additional verification inputs collected in §4.

### 1.2 Exact early-distance arithmetic versus float fade distance

**0xe0680** (zone-box horizontal gate) and **0xe08c0** (ordinary model/light
early center gate) use **signed64 integer distance arithmetic**. Subtract
the fixed coordinates in64 bits and square each axis difference in64 bits.
For **each square separately**, divide by65536 with signed truncation toward
zero, then add those reduced integers. Native instructions implement that
division by adding65535 when the signed square is negative, then arithmetic
right-shifting by16; it is not an unsigned logical shift. Multiplications,
additions and subtraction retain their64-bit machine results.

Do not sum the squares first and shift once. For example differences200 and
200 fixed units give reduced terms0 and0, sum0; reducing their80000 sum would
instead give1.0xe0680 uses only X/Y.0xe08c0 uses X/Y/Z.

The radius threshold is **float32(float32(R×R)×65536)**, converted toward zero
to **signed64**. The comparisons are signed, and retain equality: rejection
occurs only when the integer distance sum is greater than that threshold.
The native conversion instruction gives the signed64 indefinite result for
an invalid/out-of-range conversion; this is not an unsigned64 comparison.
Source instructions: **0xe0705–0xe0746** and **0xe08f4–0xe095c**.

In contrast, the subsequent **cached zone-center3D gate is float32**:
**0xd382e–0xd38ac** subtracts the signed64 fixed coordinates, converts each
difference to float, multiplies each by float1/65536, squares each resulting
block component, and adds X²+Y² then Z². It retains a zone only when that
value is **strictly below float32((renderer+0x49c+64)²)**. It also stores the
float squared distance at zone+0x234. It does not use the per-square integer
reduction from the earlier horizontal gate.

The beam raw-position gate and0xce300 fade likewise use float32 block
components and squared sums, not the early integer reduction. Ordinary
models use the raised center and radius from the previous audit §9.6.
For light ranking in §3.1, start with the integer-reduced three-axis sum,
then convert it to float and multiply by1/65536 to express the ranking key
in blocks². These related computations deliberately have different rounding.

### 1.3 Native CPU clip matrix and six plane equations

Source **0xdf8f0** copies renderer's projection matrix+0x3dc, multiplies it
by view matrix+0x39c through0x173e0, and extracts planes. Treat these as
**column-major matrices acting on column vectors**: combined matrix is
**M=P×V**. Let its rows be X,Y,Z,W (each a four-coefficient vector), so a
point's native homogeneous clip coordinates are those four row dot products.
The planes stored from renderer+0x43c, at16-byte strides, are exactly:

| Index | Equation coefficients | Zone box0xe0680 | Point/sphere0xe08c0 |
| --- | --- | --- | --- |
|0|W+X|Test|Test |
|1|W−X|Test|Test |
|2|W+Y|Test|Test |
|3|W−Y|**Skip**|Test |
|4|W+Z|Test|Test |
|5|W−Z|Test|Test |

**Do not normalize these planes.** The native code writes raw row sums;
the sphere routine adds its supplied radius directly to the resulting dot
product without dividing by the normal's length. Plane3 is the positive
clip-Y/upper-screen boundary for this projection, not the far boundary.

**Projection source0xa0340, normal camera caller0xadcd0:** view-forward depth
is positive. Let h=1/tanf(fovy/2), n=0.05, f=1920, and
**aspect=−float(viewportWidth)/float(viewportHeight)** (the minus is explicit
at0xaf097). The nonzero column-major entries, in row,column notation, are
**P00=h/aspect**, **P11=h**, **P22=f/(f−n)**,
**P23=−(n×f)/(f−n)** and **P32=1**; P33=0 and all other entries0.
Thus native GPU depth is **zero-to-one**, with clip-W=view-Z. Native X
projection scale is negative and native Y scale positive. The CPU near
equation nevertheless remains **W+Z**, not Z. It admits view-Z as low as
**n×f/(2f−n)** on the central ray, approximately half the GPU near distance.
This discrepancy is verified behavior; do not silently substitute the GPU
near equation or extract from a Vulkan matrix carrying a Y flip.

The normal-camera fovy global float at0x533930 starts at float
**0.8726646900177002** (50 degrees).0xadcd0 eases it toward that target
with0xa7a0(frameMS,0.002) when local-player u16+0x128 bit0x400 is clear;
when set, the target is float **0.5235987901687622** (30 degrees) and the
coefficient rate0.01. This is an additional camera-state input when checking
planes. The projection helper uses the **current eased value**, not the
target. It uses no projection Y flip or CPU depth remapping.

**Point coordinates for these equations:** before float conversion add
renderer signed64+0x428 to world fixed X and+0x430 to world fixed Y, then
multiply each converted float by1/65536. Z converts directly from world
fixed units. The view matrix includes the matching render-origin-adjusted
translation. Supplying a full world-eye view while also applying this origin
would count it twice. A precise oracle should preserve both matrices, the
two fixed origin offsets, the six raw planes and current fovy.

For a zone box, choose the maximum coordinate on axes whose plane normal
coefficient is>0 and the minimum otherwise. Reject if **Y term +X term
+Z term +constant<0**, float32 operations, for any of the five enabled
planes. The early horizontal distance test remains separate (§1.2).
For0xe08c0 test its point against **all six** raw planes, adding the supplied
sphere radius after the constant, rejecting strictly negative results.
Zero lies on the accepted boundary. No epsilon, sphere-normal rescaling or
extra near/far plane appears in either native routine.

## 2. Distinct clock-derived lighting inputs

Sources: frame **0xe2e00**, palette target provider **0x2b0400**, smoothing
helper **0xa7a0**, and directional-light upload **0xc6ce0**. World+0x0c is a
float32 time-of-day input in milliseconds; the period constant is86400000.
There are **two different day curves**; their values must not be substituted
for one another.

### 2.1 Renderer+0x52c, used by point-light decisions

0xe2e00 forms x=2×time/86400000−1, then forms **2×(1−x²)⁴** using powf and
float32 arithmetic. It clamps that result to **[0.05,1]** and assigns it to
renderer+0x52c. Later in the same frame, before model collection and0xce5f0,
it obtains the camera location's regional value from **0x2d9f30**, and multiplies
the assigned value by **1−0.75×that regional value**. This final factor is the
collector's input; it is not clamped back to0.05 afterward. Regional metadata
and the place-kind/fraction inputs come from0x358e0/0x35ce0 at the camera XY.
The regional helper's own definition remains in the terrain specification;
its concrete palette/collector use is verified here.

This scalar is recalculated each frame, rather than exponentially interpolated.
It affects admission/color of the0x17–0x19 model lights and other collector
branches specified later. It is **not shader ambient.w**.

### 2.2 Base palette day coordinate and ambient.w

The target provider0x2b0400 instead uses **A=(1−|x|³)⁴**, with the same x above.
It does not apply the frame's factor2 or [0.05,1] clamp to this coordinate.
After its other palette calculations it explicitly assigns the ambient
target's fourth component to **A**. Later special overrides, including the
checked effect0x13 transition, can change that target component. Therefore
ambient.w's normal time-only target is this A, not renderer+0x52c or a UI/
shadow scalar. Its rendered value is also subject to frame smoothing.

### 2.3 Submitted sun direction

In0xe2e00, let a=(2×time)×π/86400000. The native sequence adds the two time
floats first, promotes that sum for the multiplication/division by the double
constants, then converts the resulting angle to float. It forms
**(sin(a),−0.4000000059604645×sin(0.5×a),−cos(a))** and normalizes it.
0xc6ce0 normalizes the supplied direction again before upload. This is a
time-driven direction; it is not derived from camera yaw, target or eye.

### 2.4 Frame material and palette output roles

Normal frame material at renderer+0x530 is RGBA**(1,1,1,1)**. When the effective
world block at the camera is water(type2), it becomes **(0.5,0.5,1,1)**.
These are renderer-wide geometry material components; model record RGB does
not replace them. A normal non-flag1 model scales this material alpha by its
mesh-time float light/255 as in the earlier audit.

The seven target outputs of0x2b0400, in argument order, are bound to the frame
as follows. All are four float components; their environment/time-color
construction is completed in §§2.5–2.8 and the referenced sky spec.

| Provider output argument | Frame destination | Verified use |
| --- | --- | --- |
|3|renderer+0x4ac|First sky color |
|4|renderer+0x4bc|Second sky color |
|5|renderer+0x4cc|Fog RGBA, with interior override |
|6|renderer+0x4dc|Sun color RGBA |
|7|renderer+0x50c|Directional front RGBA |
|8|renderer+0x51c|Directional back RGBA |
|9|renderer+0x4fc|Ambient RGBA; provider overwrites its W with A before later overrides |

0xc6ce0 uploads front/back to the world vertex shader Light buffer and forwards
ambient to its separate Ambient buffer. The palette constants are embedded
native data used by0x2b0400, including the vector region around **0x472700**;
they are not replacement values authored by the engine. Their exact table
roles and environmental blends are specified below. This table corrects an
earlier draft's swapped output5/output7 destinations: the frame call's fifth
argument is its local+0x210 fog target; seventh is its local+0x240 front target.
The field/shader names themselves, including front+0x50c, are unchanged.

### 2.5 Complete back-light target and the normal RGBA smoothing coefficient

The back-light target is a complete, relatively small part of0x2b0400's
palette. With A from §2.2, the three embedded RGBA colors are:

| Role | RGBA | Native data RVA |
| --- | --- | --- |
| Low-day-coordinate endpoint | (0.05,0.05,0.3,1) |0x4727f0 |
| Middle endpoint | (0.25,0.25,0.25,1) |0x4727d0 |
| High endpoint | (0.5,0.5,0.5,1) |0x43f0f0 |

For A≤0.5 interpolate low→middle with weight **2A**. For A>0.5 interpolate
middle→high with weight **1−(1−2×(A−0.5))⁴**. Use componentwise float32
operations; this upper branch computes the fourth power by successive
multiplications. These back-light endpoints are not changed by the ordinary
region/height/underground color calculations earlier in the provider.
The camera-underwater correction at the provider's end changes fog and the
first sky color, not this back target.

The later **local-player effect0x13** override interpolates back toward
**(0.05,0.1,1,1)** and ambient toward **(0.01,0.05,0.2,1)**. Its weight is
**min(1,(60000−effect remaining-ms)/3000)**. The checked instructions impose
that upper bound without a second lower bound. This explains how ambient.w's
normal A target can be changed to1. The local player is world+0x448, and
0x5e3a0 obtains the effect; its remaining duration is float at effect+0x08.

Front, back and ambient targets are **not copied immediately** into their
three renderer RGBA destinations in §2.4. They share the following coefficient,
from0xa7a0 called with the renderer's float frame-duration input in ms and
rate **float0.0010000000474974513**. Let N be trunc(double(frame-ms)×100).
For N≤0 the coefficient is0. Otherwise begin a double-precision accumulator
at0. For j=1 through N in order, move that accumulator toward **j/N** by the
factor **double(rate)×0.02**; return the final accumulator rounded to float32.
Each RGBA channel then moves from previous toward target by that coefficient,
in float32. Do not replace it by frameSeconds, a constant per-frame factor or
the usual single-step exponential expression if bitwise matching is intended.

For checks, frame inputs1/16/33ms give coefficients respectively
**0.0010093337623402476 /0.015840688720345497 /0.03229581564664841**, float words
**0x3a844b9f /0x3c81c455 /0x3d04489e**. The constructor0xcbd10 initializes
front to **(0.7,0.7,0.3,1)**, back to **(0.5,0.5,0.5,1)** and ambient to
**(0.3,0.3,0.7,1)**. Thus even ambient.w has previous state; its startup value
is1 rather than an immediate evaluation of the clock curve.

### 2.6 Adaptive range+0x4a4 and the two derived scene radii

Source **0xecda0**, called during0xe2e00 before scene collection. Let N be
renderer signed32+0x438 and camera be fixed XYZ+0x338/+0x340/+0x348.
Find the camera's zone by mathematical floor through blocks then64-block
zones. Iterate the inclusive square cameraZone±N, X outer/Y inner.

Start a float squared-distance bound **B=float(N×64)²**. Whenever a zone is
absent, its pointer is null, or its active mesh-group count+0x1d0 is zero,
consider the **lower XY corner (zoneX×64,zoneY×64)**, not its center. Compute
camera-relative fixed differences, promote to double, multiply by double
1/65536, square/add in double. If that result is strictly smaller than B,
round it to float and replace B. The horizontal corner choice is native;
do not expand this into a nearest-box or center-distance rule.

Make a second pass through the same square. Accept nonnull zones having
nonzero active group count, passing0xe0680 with radius100000, whose
cached-center **horizontal** distance² computed in double is≤B. Append
an 8-byte pair of float distance² and signed32 **zone+0x444 mesh cost**.
Sort by ascending float distance² through0xcad50. Accumulate those signed
costs into signed64. The budget is **signed64(N)×N×12288**. At the first
entry bringing the sum≥budget, replace B with the **smaller of this entry's
distance² and the preceding entry's distance²**, where the preceding value
starts at0. Thus a first-entry budget crossing selects0. If the budget never
crosses, retain the missing-zone/window B; do not substitute the last entry.
Equal keys have no stable secondary ordering.

The mesh job0xe9530 writes+0x444 under the bounds/pending-list mesh lock:
reset0, then for every actually created band group add **group+0x50 +
2×group+0x48 + group+0x44**, all signed32 triangle counts. Each such field
is that index list's uint32 index count divided by3. The+0x48 category is
water, consumed by0xe85e0. Counts+0x54/+0x58 and other groups of faces do
not enter this cost. This is a weighted triangle cost, not the active group
count, a vertex count, a volume, or an arbitrary guessed distance heuristic.
Like cached bounds it can describe a completed pending mesh before its
resource groups become active (§1).

The candidate range is **max(sqrtf(B)−float44.8,float0.001)**. Let F be true
if the block at the local player's **physical position** has both flag bits
0x02 and0x20 in byte+5 and the color supplied by0x2aa140 has any nonzero
RGB component. Let E mean local-player effect0x13 exists.

- With local ability byte+0x68=0x9c and timer+0x6c>1000 ms, ease the current
  +0x4a4 toward **max(candidate,64)** using0xa7a0(frameMS,0.005).
- Otherwise, if neither F nor E is true, or candidate≤256: snap+0x4a4 to
  candidate when candidate≤current; when candidate>current, ease toward it
  using0xa7a0(frameMS,0.0005).
- Otherwise ease toward256 using that same0.0005 coefficient. In this branch
  the target is256 even when the candidate is greater.

All easing updates use current+(target−current)×coefficient in float32.
The constructor0xcbd10 initializes+0x4a4 to0. Separately this routine eases
renderer+0x4a8 toward1 with coefficient0xa7a0(frameMS,0.05); it is not used
as a substitute for+0x4a4 or ambient.w.

For reproducibility,0x2aa140 uses local-player XY converted from fixed to
integer blocks **toward zero**, then double x=0.01×blockX,y=0.01×blockY.
RGB before normalization is respectively **127+100×noise(x+9434,y+123123)**,
**127+100×noise(x+7658,y+3256)**, **127+100×noise(x+6577,y+74765)**, where
noise is0x2e0c0 and each returned sample/color operation is float32.
0xecda0 clears renderer+0x4ec RGBA to0 each call; for the flagged block it
writes RGB divided by255 and leaves alpha0. The nonzero test uses the
unnormalized three floats. This RGB is a separate scene/fog target input.

After this update,0xe2e00 derives **+0x49c=min(+0x4a4,float(N×64)×0.2,200)**
and **+0x4a0=min(+0x4a4,float(N×64)×0.5,256)**. The two ranges are not
synonyms; §3 names the precise one consumed by each source.

### 2.6.1 Native face categories and exact pre-merge cost

The weighted count's categories are fully identified in **0xe9530**.
Their group index-buffer/triangle-count fields are:

| Native terrain category | Buffer / count | Contribution per emitted quad |
|---|---|---|
| Ordinary nonzero effective types except water2 and lava17; includes wet3, types10/11, poison19 and light20 | +0x10 / +0x44 |2 triangles, cost2 |
| Water2 ordinary faces and wet3's additional top film | +0x18 / +0x48 |2 triangles, cost4 |
| Lava17(0x11) ordinary opaque faces | +0x28 / +0x50 |2 triangles, cost2 |

These are effective accessor classifications, not collision-solidity
classes. The ordinary six face tests expose a face against effective
air0, or additionally against water2 when the current type is not2;
water's reciprocal ordinary face therefore exists only against air.
For wet3 with air immediately above, both its solid top and the separate
water film contribute: **2+4=6 weighted cost**. Water immediately above
exposes the ordinary solid top without adding that wet film. See
steam-voxel-vertex-inputs §§8.1–8.3 for full face/film color and normal
contracts; lava remains a separate index category even though it is opaque.

Each accepted ordinary face appends **four new8-byte vertices and six
32-bit indices** through **0xe2d00 /0xcd7d0**. The RGB-dependent diagonal
chooses the split, not the triangle count. Native0xe9530 does **not**
greedily merge adjacent coplanar faces, deduplicate equal vertices, or
remove an indexed quad because its packed color/light matches a neighbor.
Counts must reflect these native accepted face events before any
replacement renderer merging, vertex sharing or index compression.

The explicit coarse-water exception is the previously specified whole-zone
optimization: when its effective-block/light conditions pass, it emits
**one64×64 quad at Z1**, four vertices plus six indices through
**0xcdea0**, into the water list. It suppresses the ordinary type2 face
processing for **all water blocks at Z0**, not merely those blocks' top
face tests. That optimized quad contributes **2 water triangles /cost4**.
Other heights/types still follow their normal face tests. Preserve the
existing light-solve/effective-accessor stage order for that optimization;
do not approximate its decision from raw unlit stored terrain.

Every block's faces go to its native250-block Z band. After construction,
each category count is the number of its **32-bit CPU indices divided
by3**. Choosing16-bit GPU indices when that shared vertex count is below
65536 only converts each index width and preserves the count. It is not
a geometry optimization. The auxiliary terrain categories **+0x4c,
+0x54,+0x58** receive no indices in this producer and remain0; none is
an extra ordinary face contribution. The final mesh job includes only
actually created nonempty shared-vertex-buffer groups and accumulates
**+0x44 +2×+0x48 +0x50**, as already stated above, at zone+0x444.
Sprite/model/object draw geometry contributes nothing to this terrain
cost. Cache publication and mesh-stage labels remain §1.

### 2.6.2 Equal-distance ordering can affect the selected threshold

In **0xecda0**, accepted zone pairs are appended in the window scan's
**ascending X outer /ascending Y inner** order. Each pair contains a
float32 horizontal distance² key and a signed32 triangle-cost word.
The cost is carried bit-for-bit with the key; it is neither a float cost
nor a secondary sort key. Sort **0xcad50** begins with budget equal to
pair count, comparing the distance key only.

For the finite keys produced from the cached fixed centers, this sort
has the exact partition/insertion/heap decisions described in
**steam-controller-handoff §4.1–4.3**, using the pair's float32 key in
place of the inventory Chebyshev key. In particular **≤32** uses stable
insertion **0xc86a0**; **>32** partitions through **0xc9550**;
sample selection **0xc82d0 /0xc8e80** uses the three-sample case through
41 entries and the nine-sample case from42; budget reduction uses the
same two integer truncations. Heap fallback **0xc8b50 /0xca8f0**,
with repair **0xca310**, selects the right child on equal keys and stops
upward insertion on equality. There is no stable region-coordinate or
cost tie-break above32. This includes distances that become equal only
after float32 rounding.

Ordering matters because the first budget-crossing entry chooses the
**smaller of its key and the immediately preceding key**, with previous
key initialized0. A crossing at the first member of a tied group can
therefore select the previous, lower-distance group; crossing later in
the tied group selects that tied distance instead. For an arithmetic
illustration, N3 gives budget110592. A prefix cost110572 is20 short.
For two equal-key records of costs30 and6, putting30 first crosses at
the group's first entry; putting6 first crosses at its second. With a
strictly lower preceding key, those select different squared bounds.
This is an illustrative calculation, not a claimed native capture.
Thus preserving only sorted distances while arbitrarily permuting their
costs is insufficient for exact adaptive-range parity.

### 2.7 Complete front and ambient palette targets

The following is the remaining directional/ambient construction in
**0x2b0400**. It corrects the earlier draft's front/fog argument confusion:
the front is output7, and is **not** replaced by sky2 when underwater.
Existing `sky-and-clouds.md` §B.3 covers sky1/sky2/fog/sun; those four are
different provider outputs.

Obtain neighbourhood, place kind/weight/big flag, dryness D, ground height H,
lava weight V and forest weight F at camera XY with0x358e0/0x35ce0,
0x2b5960/0x35ea0(mode0)/0x2b9750/0x2d9f30. Their existing definitions are in
`worldgen-terrain-height.md`. Convert fixed camera XY to integer blocks
**toward zero** for these calls. If H<20, multiply D by **max(H/20,0)**.
V and F are the unscaled regional/place weights. Let P be the place weight
only when kind=0x1b, otherwise0.

Maintain three RGBA endpoint pairs named low, middle and high. Their initial
values are:

| Endpoint | Front RGBA and data RVA | Ambient RGBA and data RVA |
| --- | --- | --- |
|Low|(0.04,0.05,0.1,1),0x4726e0|(0.05,0.075,0.1,1),0x4726f0 |
|Middle|(0.65,0.6,0.15,1),0x472750|(0.15,0.2,0.75,1),0x4728e0 |
|High|(0.75,0.7,0.3,1),0x43f0c0|(0.25,0.3,0.7,1),0x43f130 |

The following endpoint modifications are ordered and componentwise float32.
Where a weighted blend is named, evaluate **target×w + old×(1−w)**; the dry
blend explicitly uses **old+(target−old)×w** instead. Keep all four channels
through these stages, even though ambient alpha is finally overwritten.

1. **Camera block flag0x20.** Query effective block0xeab0 at camera fixed XYZ.
   If byte+5 has this bit, compute big-structure kind0x2aa270 from the place
   kind/big flag and camera cell XY. For this cell computation, first truncate
   fixed XY to blocks toward zero, then **floor-divide those blocks by1024**.
   Choose ambient-high **C=(0.1,0.3,0.4,1)** from0x472840 for structure kind
   0x2b or0x2c; otherwise **C=(0.25,0.1,0.5,1)** from0x472880. Replace
   ambient-high by C; ambient-middle by **C×0.9**; front-high by
   **(1.5,1.5,1.5,1)−C**; front-middle by **(1,1,1,1)−C×0.9**.
   Low endpoints stay unchanged.
2. **D<0.3**, whether or not that block flag was set: move ambient-high toward
   **(0.3,0.3,0.2,1)**,0x4727b0, using weight **1−D/0.3** and the dry blend
   above. Replace front-high by **(1,1,1,1)−the resulting ambient-high**.
   Other endpoints are unchanged. In particular, the flagged high-front
   initial subtraction uses1.5 for RGB, while this dry recomputation uses1.
3. **V>0.001:** weighted-blend each endpoint toward the following target by V:

   | Endpoint | Front target, RVA | Ambient target, RVA |
   | --- | --- | --- |
   |Low|(1,0.8,0.1,1),0x46f5a0|(0.1,0.05,0,1),0x472680 |
   |Middle|(1,1,0,1),0x43f080|(0.3,0.2,0.1,1),0x472710 |
   |High|(1.1,1,0.1,1),0x472730|(0.4,0.3,0.2,1),0x4727c0 |

4. **F>0.001:** front-middle toward **(0.2,0.4,0.4,1)**,0x472850;
   front-high toward **(0.2,0.2,0.9,1)**,0x472930. Ambient middle/high both
   toward **(0.2,0.2,0.3,1)**,0x472800. Front-low remains unchanged.
   Ambient-low has **two successive weighted blends by the same F**:
   first toward **(0.1,0.1,0.2,1)**,0x472780; then the resulting value
   toward **(0,0.01,0.01,1)**,0x4726b0. Retain both operations; this is
   not a single blend to either endpoint. The second blend is verified at
   **0x2b34ad–0x2b351d**.
5. **P>0.001:** front-middle/high toward **(0.9,0.7,0.6,1)**,0x4728c0;
   ambient-middle/high toward **(0.3,0.5,0.5,1)**,0x4728a0, by P. Low
   endpoints stay unchanged.

Finally use the common day coordinate A from §2.2. For A≤0.5, the output
is **low×(1−2A)+middle×2A**. For A>0.5, let
**w=1−(1−2×(A−0.5))⁴**, successive float multiplications; output
**high×w+middle×(1−w)**. Overwrite ambient alpha with **A**, regardless of
the alpha produced by those endpoint computations. Back uses its own
unmodified three endpoints in §2.5. Far-flight0x2b8050 does not replace
these front/ambient endpoint sets; its earlier modifications affect other
provider outputs.

Camera water changes fog and sky1 to sky2, **not these lighting targets**.
Then local-player effect0x13 uses §2.5's weight: front **and back** move toward
**(0.05,0.1,1,1)**; ambient toward **(0.01,0.05,0.2,1)**. The renderer applies
the common §2.5 smoothing to all four channels of these three final targets.
Normal scene shader uploads and model mesh-light material composition remain
as independently specified in `worldgen-parity-gaps.md` §9.

### 2.8 Other palette outputs, far-flight completion and frame transitions

The sky/fog/sun key tables and ordinary ordered environment RGB modifiers
are already specified in **`sky-and-clouds.md` §B.3**, verified against the
same0x2b0400 here. Use that document together with the argument mapping in
§2.4 and the following completions, rather than substituting arbitrary frame
colors. These are additional precise facts, not an assumption that every
channel has alpha1 after every modifier.

**Place-kind0x1b alpha exception:** when P>0.001, the dusk-B sky1 key's RGB
has the documented target(0.8,0.2,1), but its alpha becomes
**P×P+oldAlpha×(1−P)**. It is not a blend toward constant alpha1. This occurs
before the time-key interpolation, so partially weighted places can produce
sky1/fog alpha values other than1.

**Far-flight modifier0x2b8050:** the helper is true exactly when a local
player exists with ability0x9c, the double horizontal distance from physical
player XY to signed32 block-coordinate anchor+0x1b0/+0x1b4 is **>1000**, and
the double horizontal distance from fixed XY+0x1b8/+0x1c0 to that anchor is
**>16384**. Fixed differences are converted using double1/65536, then
sqrt of the double sum of squares. Height is not involved. This helper is
different from §2.6's ability-timer flight branch.

When it is true, after all ordinary environment/place key modifiers and
before time interpolation:

- Copy the already modified sky1 **day,dusk-B,night** keys to their matching
  fog keys. Fog dusk-A is retained.
- Replace sky1 **day,dusk-B,night** with **(0,0,0,1)**. Sky1 dusk-A is retained.
- Replace sky2 **day,dusk-B** with **(0,0,0,1)**. Sky2 dusk-A and its black
  night endpoint are retained.

The native assignments in0x2b0400 therefore change more keys than the earlier
sky spec's summary “day sky1 becomes black.” Sun and the lighting targets in
§§2.5/2.7 are not replaced by this branch. Apply the ordinary time interpolation
after these changes. Camera-water replacement and effect0x13 then occur in
the order already specified. Fog and sky1 move toward(0.9,1,1,1) for effect13,
sky2 toward(0,0,0,1); sun is not changed by that provider effect branch.

**Frame transition0xe2e00:** front/back/ambient always receive §2.5's rate0.001
easing first. If the freshly computed renderer+0x4ec **has any nonzero RGBA
component**, sky1+0x4ac,sky2+0x4bc and fog+0x4cc each ease toward that whole
RGBA with0xa7a0(frameMS,0.0005), via0xcb5c0; sun+0x4dc eases toward zero RGBA
with rate0.002. Otherwise sky1/sky2 copy their provider outputs directly,
fog eases toward output5 at rate0.0005, and sun eases toward output6 at
rate0.002. This is a comparison with a zero vector, **not a comparison with
the previous frame's target**.0xecda0 resets+0x4ec to zero each call (§2.6).
The interior override does not replace front/back/ambient after their normal
update, and its alpha0 must not be reused as ambient.w.

## 3. Point-light collector: common records and selection

Sources: collector **0xce5f0**, append **0xe2bd0**, sorting
**0xcb7f0 /0xcaff0 /0xc88b0**, and GPU upload **0xc7010**.

### 3.1 Collected record and source ordering

The frame vector is renderer **+0x318 begin/+0x320 end/+0x328 capacity** and
is cleared before collection each frame. Each record has stride**0x30**:

| Record offset | Type | Meaning |
| --- | --- | --- |
|0x00,0x08,0x10|signed64|World fixed XYZ light position |
|0x18|float32|Radius in blocks |
|0x1c,0x20,0x24|float32|RGB intensity/color |
|0x28|float32|Ranking value: squared camera distance in blocks² |
|0x2c|padding|Not copied by the append helper; no meaning assigned |

The distance key is obtained by squaring each fixed camera-to-light difference,
dividing each square by65536 with signed truncation, summing those integers,
then converting the sum to float32 and multiplying by float1/65536. This
differs slightly from computing a float Euclidean square first. Each source
stores the key for its actual emitted position.

Collection order is: renderer's accepted-zone vector **+0x300/+0x308**, each
zone's glowing ground items then model-light auxiliary list then object-light
auxiliary list; then the world's projectile/effect list at world+0x60; then
the frame-supplied transient light list; then the world's creature list
at world+0x40. Specific source gates and positions are specified below as
verified. The zone vector is populated by0xd3170 after its zone-frustum gate,
not simply by walking all generated zones.

### 3.2 Ranking, affected zones and shader limit

After all source branches, sort collected records by **ascending stored
squared-camera-distance key**, not brightness, radius, player distance or
per-zone distance. The native introsort is not guaranteed stable for equal
keys; this trace assigns no extra source-ID or type tie-breaker.

For each sorted light, extend fixed X/Y by its float radius×65536 truncated
to signed64. Floor the two inclusive endpoints to zone coordinates using
64-block zones, including negative-coordinate flooring. Visit that inclusive
zone rectangle with X outer/Y inner. Every existing loaded zone in this
rectangle is eligible; there is no additional sphere-versus-zone intersection
or brightness-rank test here. Up to **16** records are retained for each zone,
in the already sorted order. Later records affecting a full zone are skipped.
Only XY extent determines affected zones; a light is not assigned to vertical
250-block bands separately.

Per-zone count is signed32 at **+0x440**. Its position/radius table is
**+0x240**,16 float4 entries; colors are **+0x340**,16 float4 entries. Convert
each light's fixed XYZ to float blocks after adding renderer fixed origin
translation+0x428/+0x430 to X/Y. Position.w is radius; color.w is0. These
entries are per-frame renderer data, separate from all generation records.
0xd3170 clears the count and both tables for the meshed zones it considers
before testing their frustum, so unused slots in those current zones are0.

**0xc7010 compares/caches all16 entries but uploads only the first10** positions/
radii and first10 colors. The ordinary vertex shader also loops only those
10. Retaining16 in the CPU zone tables is therefore native collector behavior,
not a claim that all16 illuminate a draw. Terrain/water draws bind their zone
tables. Ordinary world model draws bind the tables selected from the model's
transformed center location; missing-zone fallback tables are zero. The shader
radial/normal attenuation remains `worldgen-parity-gaps.md` §9.3. Helper
**0xce580** binds all-zero tables for draw paths that explicitly request it.

### 3.2.1 Exact per-draw sprite center and zone-table binding

Sources: opaque draw consumer **0xe6e60**, including the center/conversion
sequence **0xe6f78–0xe70e8**; faded color consumer **0xe7980**; descriptor
append **0xee2f0/0xee990**; matrix composition **0x173e0**; scenery submission
and creature-pose call in **0xd3170**; per-part submissions in **0x17ab0**.

**The local center is half the full sprite voxel-array dimensions.** Read
signed32 dimensions from the selected sprite at **+0x88,+0x8c,+0x90**,
convert each to float32 and multiply by float0.5. Thus the center is
**(sizeX/2,sizeY/2,sizeZ/2,1)** in that sprite's local voxel coordinates,
including half-voxel values for odd dimensions. It is not the center of
occupied voxels, emitted vertices, an occupied-vertex AABB, the creature's
overall extent, or another separately stored center/pivot field. Hidden
markers and empty padding do not change this calculation unless the sprite's
dimension fields themselves change.

Transform that center by the **final per-descriptor model/world matrix**:
the same matrix that this descriptor uploads for drawing.0xee2f0 and the
mirrored0xee990 construct it from the queue base matrix at queue+0x50 and
the submitted transform. With column vectors it is **queueBase×submitted**,
so the submitted transform acts first. The lookup does not use the view or
projection matrix, screen coordinates, or the shader's clip depth offset.

For ordinary zone scenery, the submitted matrix built by0xd3170 already
contains its record placement, size, yaw, sprite centering and any selected
flag0x04 sway-table transform or flag0x10/0x20 pivot rotation. These affect
the center used for light binding through the final matrix, just as they
affect the drawn geometry. The consumer does not substitute an undeformed
record foot position or redo a separate unswayed bounds calculation. The
ordinary world vertex shader has no additional plant-wind deformation stage
outside that matrix path.

**Coordinate arithmetic:** retain float32 rounding through this sequence.
For the center's homogeneous W, multiply each half dimension by its
corresponding fourth-row matrix coefficient, add the X and Y products,
then the Z product, then the translation/W coefficient. Compute **1/W once
by float32 division**. For each X/Y numerator, likewise form its separate
dimension-times-matrix products, add X and Y products first, then Z, then
translation. Multiply each numerator by that rounded reciprocal, then by
float32 **65536**. There is no fused multiply-add or double accumulator in
the traced CPU sequence. Only transformed X/Y and W are needed for the zone
lookup; local Z still affects X/Y when the world matrix tilts the sprite.

Convert each scaled X/Y float to **signed64 toward zero**. **After** this
conversion subtract the matching signed64 renderer origin translation,
**+0x428 for X / +0x430 for Y**. This restores the world fixed coordinate
from the matrix's render-origin-adjusted coordinate. Then floor from fixed
units to whole blocks, including negative fractions, and floor-divide the
block coordinate by64 to obtain the signed32 zone key. The native arithmetic
uses signed32 whole-block/zone intermediates in its normal coordinate domain;
there is no nearest-zone rounding and no use of the actor's raw position.

Do not replace these stages by direct floating floor(center/64). For example,
with origin0 a transformed X of **−1/131072 block** scales to−0.5 fixed unit
and truncates to0, hence zoneX0. A transformed X of **−1/65536 block** scales
to−1 exactly and yields whole block−1, hence zoneX−1. Flooring the float
before its fixed conversion would give a different result in the first case.

The world-zone-map lookup requires an existing entry and non-null zone
pointer. It does not additionally require that selected zone to have a mesh,
pass a frustum test, or equal the zone containing the object's record foot.
If the entry/pointer is missing, supply an all-zero16-entry table for both
position/radius and color. Otherwise bind **that zone's +0x240/+0x340 tables**
through0xc7010, retaining §3.2's16 cached/10 uploaded distinction.

**This is per drawable descriptor, not once per actor or shared sprite.**
The original scene0xd3170 passes queue **renderer+0x138** into creature pose
**0x17ab0**. Body parts and held models are submitted there through0xee2f0,
or through0xee990 for the mirrored winding path; the calls in0x17ab0 include
the main per-part range beginning **0x25b63**. Each uses that part's own
sprite dimensions and its complete pose/scale/centering transform. A head,
hand, weapon and body from one creature can therefore select different
zone tables at a zone boundary. They do not inherit a single root-creature
zone binding. Reused instances of one sprite also perform the lookup for
their separate stored transforms.

The normal scene calls both opaque and faded consumers with draw flag2
set.0xe6e60 applies the selection to both ordinary/mirrored opaque lists;
0xe7980 applies it to both winding lists' **color passes**. The latter's
depth-only prepasses do not repeat the lookup. Ordinary scenery in the
flag0x08 queue renderer+0x1d0 uses the same consumer rule. The lookup is
not skipped merely because a descriptor has Glow1.

This selection is gated by **consumer drawFlags&2**, not by a model-record
flag. When that consumer bit is clear,0xe6e60 instead calls0xce580 to bind
zero light tables;0xe7980 skips per-descriptor zone selection. The normal
overlay caller clears lights before its overlay draws. Terrain and water
keep their mesh-containing-zone table binding, rather than applying this
sprite-center rule to each quad.

A bounded distinguishing case is sprite dimensions **(8,4,6)**, occupied
geometry only within local[0,1]³, and an otherwise identity matrix translated
by **(61.5,0,0)** blocks, origin0. Its full-header center transforms to
**(65.5,2,3)** and selects zone(1,0); an occupied-AABB center would wrongly
select zone(0,0). A per-part actor case should likewise preserve each
sprite's dimensions and final matrix, rather than only the actor root.

### 3.3 Model-light auxiliary records, complete branch

The collector reads **zone+0x90/+0x98**, stride0x40, rather than submitting
the ordinary model list a second time. Position, tint and mesh-written+0x28
are from this auxiliary copy. Model null markers can therefore emit lights
without an ordinary voxel asset.

For a type**0x4c** marker, require raw-position squared distance strictly
below **(renderer+0x4a4+32)²** in float32. This branch does not use an
individual frustum/sphere test or require flag1. Its emitted position is the
raw record XYZ and its RGB is the raw record tint. Its radius is
**30+2×cos(10×N+0.01×world clock)**. Here world clock is float ms at world+0x440;
N is existing double-input simplex noise **0x2e0c0**, called with each fixed
X/Y first multiplied by signed integer10, converted to double, then multiplied
by double1/65536. The noise result, phase arithmetic and cosf/radius are float32.
Once such a marker is admitted, the collector **breaks out of this zone's
entire model-light-list loop**. An out-of-range marker does not cause that
break. The subsequent object-light list is still processed.

Every other model light requires **flag0x01** and an **0xe08c0** visibility test
at its raw position, sphere radius**16**, render range **renderer+0x4a4**.
The missing argument in the abbreviated decompiler call is resolved by the
assembly load of the16.0 constant at0xce69a. For types
**0x17/0x18/0x19**, form U=(float+0x28×renderer+0x52c)/255. Admit only U≤0.4
and multiply its RGB tint by **1−U**. Other flag1 types have no such U gate or
color multiplier. All these model lights use raw record position and radius
**15+0.5×cos(10×N+0.01×world clock)** with the same N as above. There is no
extra distance fade on these RGB values, and no ordinary model midpoint/Z
extent offset. The radii are30±2 and15±0.5, not a2π phase or a15±5 flicker.

### 3.4 Ground-item lights, complete branch

Before the model-light list, each accepted zone's **live item list+0x30/+0x38**
is scanned in order, stride0xe0, but only when its cached float squared-center
distance+0x234 is strictly below **(renderer+0x49c+64)²**. Require item's
signed32 timer+0xcc **equal0**, and a nonzero glow color from **0x1098d0**:

| Item category / id | Glow RGB |
| --- | --- |
|0x0b /0x13 |(0.1,0.3,0.3) |
|0x0e /any |(0.1,0.3,0.5) |
|0x13 /0 |(0.6,0.6,0.2) |
|0x13 /1 |(0,0.6,0.2) |
|Otherwise |(0,0,0), omitted |

Also require visibility/ownership predicate **0x2b8140**. For these glow
categories it requires nonzero quantity+0xd4, and, when a local player exists,
owner ID+0xd8 equal0 or that local player's ID+0x968. The other learned/lore
category cases in this predicate do not apply to these glow categories.

Light position is item fixed position **+0xa0/+0xa8/+0xb0 with Z+3 blocks**.
Use0xe08c0 there with sphere radius**16** and render range **renderer+0x49c**.
If accepted, append radius**16**, and multiply all three glow RGB components
by **0xce300(position,renderer+0x49c)**. This branch uses its Z+3 position for
both rank and fade. It is not the ordinary model's scaled-height midpoint.
There is no day-factor gate on these lights.

#### 3.4.1 Cached center-distance producer and exact fade arithmetic

The cached **zone+0x234** value is a **float32 full XYZ squared distance in
blocks²**, produced by **0xd3170 at 0xd3804–0xd388f**. The position is the
zone's signed64 fixed mesh-cache center **+0x218/+0x220/+0x228**. The camera
reference is renderer's signed64 fixed XYZ **+0x338/+0x340/+0x348**. Its X
pointer is established at **0xd34dd**. These are world-coordinate values;
this calculation does not add the render-origin translation+0x428/+0x430,
multiply a view matrix, or substitute the player's position or a ground
item's position. It includes the center's Z distance, not only horizontal
distance, and does not subtract the cached radius+0x230.

For each axis, subtract camera fixed coordinate from center fixed coordinate
in signed64 integer arithmetic **before** either value converts to float.
Convert that difference directly to float32, then multiply by the exact
float **1/65536**, constant word **0x37800000** at **0x42e510**. Square the
three resulting float32 block components separately, add **X²+Y² first**,
then add **Z²**, rounding at each float32 operation. Store that final value
at +0x234, at **0xd388f**. There is no block-floor conversion, signed32
narrowing, integer-square reduction/shift or fused multiply-add here.

The store follows the nonzero active-group-count and 0xe0680 zone checks,
and insertion into the renderer's accepted-zone list+0x300. It precedes the
strict center-distance gate for that zone's model records. The ground-light
collector reads this stored value; it does not recalculate it from the
item position. At **0xce713–0xce737**, it loads the float32 range from
renderer+0x49c, **adds the float32 constant 64.0** (word 0x42800000 at
0x42e5b4), squares that
rounded sum in float32, and admits the item-list scan only when the cached
distance is **strictly below** that squared threshold. Equality fails; an
unordered comparison fails as well. The added 64 belongs to this containing-
zone gate, not the item fade or its individual point/sphere test.

The item fade is literally **the same 0xce300 helper used by ordinary models**
in `worldgen-parity-gaps.md` §9.6, with different sampled positions. Its
ground-light call at **0xce945** receives the range loaded at **0xce93c**:
**L=renderer+0x49c**, with no +64 and no addition of the light's 16-block radius.
The sampled fixed point is item+0xa0/+0xa8/+0xb0, with exactly **196608 fixed
units (3 blocks)** added to Z in integer arithmetic. Ordinary scenery's
asset-height midpoint does not apply to this item call.

Inside 0xce300, the full XYZ sampled-point-minus-camera differences use the
same signed64-subtract, direct float32 conversion, multiply by1/65536,
separate squares, **X²+Y² then Z²** sequence described above; sources
**0xce30f–0xce377**. This fade distance is separate from both the cached
zone-center distance and the collector's integer-reduced light-ranking key.

The helper's ordered finite-input fade arithmetic is:

- Form the full-opacity squared threshold by multiplying **L by
  0.809999942779541015625**, rounding to float32, then multiplying by **L**
  again and rounding. Its constant is **word 0x3f4f5c28 at 0x43ef70**. This is
  the actual multiplication order, not a newly evaluated square of 0.9×L.
  At or below that threshold, return exactly 1.
- Above that threshold, take the float32 square root of the squared distance.
  Separately form **L×0.89999997615814208984375** and
  **L×0.100000001490116119384765625**, each rounded to float32; constants
  **word 0x3f666666 at 0x42e534** and **word 0x3dcccccd at 0x42e528**.
  Subtract the first product from the square root, divide that result by
  the second product, then subtract that quotient from float 1.0. Each step
  is float32 and preserves that order. Finally take the maximum with 0.
  There is no subsequent upper clamp in this helper.

The threshold path is **0xce364–0xce37e**, and the square-root/ramp/clamp
path is **0xce380–0xce3b5**. There is **no explicit zero-range branch or safe
denominator replacement** in 0xce300; no additional special case is assigned
here. Ground-light RGB components are multiplied individually by its result
at **0xce968–0xce98c**, after which the light is appended. This call site does
not add the ordinary descriptor queue's separate nonpositive-fade rejection.

### 3.5 Static object-light auxiliary records, complete branch

After model lights, scan **zone+0xc0/+0xc8**, stride**0xe0**, in list order.
This is the **object-light auxiliary copy**, rebuilt by0x31d470 from light
types in the live+0x60 object list; it is not every live object. Its selected
types are **0,0x10,0x3f,0x40,0x41,0x42,0x53,0x70,0x71,0x72,0x7e**.
The list production is already in `worldgen-structures-contents.md` §C7.

Let P be object fixed XYZ at+0x08/+0x10/+0x18. Let h be the Z extent returned
by **0x13a960**: the type-size table **0x13a520**, multiplied by record's
float scale+0x24. This is the object's physical extent, not an arbitrary
replacement asset's voxel height. A height offset h or h+1 below is multiplied
by float65536 and truncated to signed64 before addition to P.z.

Every table row uses **0xe08c0**, render range **renderer+0x4a4**, all six
planes. The table distinguishes its culling test point/radius from its
emission point/radius. Day gate means **renderer+0x52c strictly below0.4**.
None of these object branches adds noise, clock flicker, a distance color
fade or a record tint multiplier.

| Object type | Culling point / sphere radius | Emission point / light radius | RGB | Additional gate |
| --- | --- | --- | --- | --- |
|0x3f,0x40|P.z+h /50|P.z+h /32|(1.7,1.5,0.8)|Day gate |
|0x42|P.z+h /20|P.z+h /20|(0.3,2,0.5)|Day gate |
|0x53|P /32|P.z+h+1 /32|(1.5,1,0.4)|None |
|0|P /32|P.z+h+1 /32|(0.25,0.3,0.5)|Used-cell membership |
|0x7e|P /32|P.z+h+1 /32|(1.5,1,0.4)|Used-cell membership |
|0x41|P /32|P.z+h /20|(1.2,0.8,0.4)|Day gate |
|0x70,0x71|P /32|P.z+h /20|(0.6,0.7,1.3)|None |
|0x72|P /32|P.z+h /20|(1.2,1,0.5)|Day gate |
|0x10|P /32|P.z+h /20|(1.2,1,0.5)|None |

X/Y remain P.x/P.y in every row. “P.z+h” denotes that shifted XYZ point,
not a scalar position. The order is exactly the table order while processing
one object; matching type cases do not cause other objects to be skipped.

For types0 and0x7e, derive the16-block cell pair using the exact signed-width
and conversion order in §3.5.1 below. Require
that pair to be **present** in the local player's personal used-cell set
**personal state+0xa8**, reached through world+0x448 then player+0x1528.
Neither the object's live open/closed state nor its animation timer replaces
this membership test. The branch assumes that local-player/personal-state
context exists, as does the native lookup. These conditional lights still
rank by their emitted, height-shifted position.

#### 3.5.1 Used-cell key widths and fixed-coordinate conversion order

Verified in collector **0xce5f0**, type 0 coordinate paths
**0xcf446–0xcf4f2**, and type 0x7e paths **0xcf7ea–0xcf896**. Both branches
perform the same integer conversion independently for object fixed X at
**+0x08** and fixed Y at **+0x10**. Z and the subsequent emission-height
offset do not participate in this key. No floating-point conversion occurs.

The lookup key is exactly **8 bytes: signed 32-bit cell X followed by signed 32-bit
cell Y**. The personal-state **+0xa8** set compares X first, then Y, using
signed 32-bit comparisons. Native tree nodes hold those two key fields at
**node+0x1c/+0x20**; the comparison walks two 4-byte elements and stops at
node+0x24. This is not a pair of signed 64-bit cells or a comparison of a packed
unsigned 64-bit number. The type 0 lookup is **0xcf560–0xcf652**, with the
corresponding type 0x7e lookup **0xcf900–0xcf9f2**. The earlier personal-state
pointer chain and requirement for a present key remain as stated in §3.5.

For each signed 64-bit fixed coordinate, the observed conversion order is:

1. Divide the signed 64-bit fixed value by **65536**, truncating toward zero.
   Native integer arithmetic adds 65535 to negative inputs before the
   arithmetic 16-bit right shift. The full quotient is still 64-bit here;
   the fixed coordinate itself has not first been narrowed to 32 bits.
2. Retain the quotient's low 32 bits and interpret them as a **signed 32-bit block
   coordinate**. This narrowing precedes the negative exactness test and
   the division by 16.
3. If the **original signed 64-bit fixed value is negative**, sign-extend the
   narrowed signed 32-bit block coordinate back to signed 64-bit and multiply that
   value by 65536. Compare this reconstructed fixed value with the original
   signed 64-bit coordinate. If they differ, subtract 1 from the **32-bit block
   coordinate**, retaining 32-bit wraparound. Otherwise keep it unchanged.
   For nonnegative original coordinates, this correction is skipped.
4. Floor the resulting **signed 32-bit block coordinate divided by 16** to obtain
   the signed 32-bit cell coordinate. Native negative handling gives the
   mathematical floor here, rather than truncation toward zero. The final
   cell component lies in **[−134217728,134217727]**.

The negative reconstruction test is specifically visible at
**0xcf45e–0xcf46a** for type 0 X, **0xcf4b7–0xcf4c3** for type 0 Y,
**0xcf802–0xcf80e** for type 0x7e X, and **0xcf85b–0xcf867** for type 0x7e Y.
The narrowing is therefore material even before the cell division. For
coordinates whose block quotient fits signed 32-bit, this is the familiar
fixed-to-block floor followed by block-to-cell floor. Outside that range,
the narrowed reconstruction can differ even for an exact negative whole
block. Do not replace the complete sequence with a signed 64-bit floor directly
to 16-block cells or defer all narrowing until after cell division.

Arithmetic reference examples below apply to either component. These are
derived from the verified integer operations, not new runtime captures:

| Signed 64-bit fixed input | Block after narrowing and correction | Signed 32-bit cell |
| --- | --- | --- |
|−1|−1|−1|
|−1048576 (exactly−16 blocks)|−16|−1|
|−1048577|−17|−2|
|140737488355328 (exactly 2147483648 blocks)|−2147483648|−134217728|
|281474976710656 (exactly 4294967296 blocks)|0|0|
|−281474976710656 (exactly−4294967296 blocks)|−1|−1|

In the final row, the truncated 64-bit block quotient narrows to 0, but the
original negative fixed value is not equal to reconstructed 0; the native
32-bit decrement therefore produces −1. This distinguishes the observed
sequence from merely flooring in 64 bits and then narrowing the block result.

### 3.6 World projectile/effect and frame-transient sources

After all accepted-zone sources, walk the linked list at **world+0x60**.
For a list node whose payload starts at node+0x10, accept effect kind3 or4
at **payload+0x68**. Its fixed position is payload+0x08/+0x10/+0x18 and the
color-driving scalar is float **payload+0x54**. Test that position using
0xe08c0, sphere radius16, render range renderer+0x49c. Emit radius16 and
RGB **(0.5×scalar,0.5×scalar,0.2×scalar)**. There is no day gate or extra fade.
These are record-relative offsets, not the list-node-relative offsets.

Next process the caller-supplied transient-light list, in linked-list order.
Its payload format is three signed64 XYZ at0/+8/+0x10, float RGB at
**+0x18/+0x1c/+0x20**, float radius at **+0x24**, signed32 elapsed ms at
**+0x28**, signed32 lifetime ms at **+0x2c**. The collector's second argument
is this list, separate from its output vector. Test position with0xe08c0 using
the record's own radius and render range renderer+0x49c. Emit that radius,
the same position, and RGB multiplied by **max(0,1−float(elapsed)/float(lifetime))**.
There is no additional color-distance fade. The duration fields are converted
to float separately before division; no zero-lifetime replacement was found.

The persistent caller list's actual hit/death producers, exact record fields,
scene handoff, first-frame aging and strict-expiration order are specified in
`steam-transient-light-producers.md`. Rendering a particle does not implicitly
add an entry to this separate light list.

### 3.7 Living-creature lanterns and the first fixed-color sources

Finally walk **world+0x40**, preserving linked-list order. For every creature
with **HP float+0x180 >0**, evaluate the following tests independently and in
the listed order. These basic creature sources do **not** add an individual
frustum/distance test or use renderer+0x52c. They are not limited to the list
of actors whose geometry was visible in the current frame. Each successful
test appends a separate light; equal-position lights are not merged.

Define a shared lamp anchor using **drawn fixed position+0xb10/+0xb18/+0xb20**,
raw creature **yaw float+0x30 degrees**, body Z extent **float+0x8c**, and
vertical drop/squash **float+0x990**. Rotate local point
**(0.5,1,0.3×body-Z−drop)** around Z by that yaw, using float32 radians-per-degree
and sinf/cosf through0x13050. Multiply each resulting component by float65536,
truncate each to signed64, and add to drawn position. The yaw is+0x30,
**not displayed angle+0xb3c**. This anchor should not be approximated by a
generic actor-center light or an unrotated vertical-only head offset.

| Test, in order | Emitted position | Radius | RGB |
| --- | --- | --- | --- |
| Either equipped-hand item id at+0x63c or+0x6dc equals0x14 |Shared lamp anchor|Creature float+0x198|(1.2,1,0.5) |
| Creature flags+0x128 bit0x01 set |Shared lamp anchor|Creature float+0x198|(0.8,0.7,0.4) |
| Effect type9 exists, lookup0x5e3a0 |Shared lamp anchor|16|(0.6,0.4,0) |
| Race signed32+0x64 equals0xb9 |Shared lamp anchor|32|(0.6,0.4,0) |

The hand-id test is an OR and emits one light even when both hands match.
The toggle/effect/race tests can each add another. No item-category guard
accompanies the id0x14 hand test in this collector. The radius+0x198 is the
existing creature light-radius stat produced by physics/rules, not a hardcoded
ten-block substitute. These first anchor paths are at **0xd058d onward** in
0xce5f0. The blue0 in the last two rows is an initialized upper half of a
64-bit store, not an unwritten stack value.

### 3.8 Remaining creature race/state lights

These branches follow §3.7 in the same living-creature loop, independently
in table order, without additional frustum/day/distance gates. Race is signed32
at+0x64; flags below are the u32 at+0x80. Define **drawn-drop** as drawn
fixed position with Z increased by **trunc64(−float(drop)×65536)**. Define
**physical-drop-raised** similarly from physical position+0x10/+0x18/+0x20,
then add exactly **98304 fixed units (1.5 blocks)** to Z.

| Condition | Position | Radius | RGB |
| --- | --- | --- | --- |
|Race0x75|drawn-drop|16|(0,0.4,0.8) |
|Race0xdd and byte+0x19c=0|drawn-drop|16|(0.8,0.6,0.1) |
|Race0xde and byte+0x19c=0|drawn-drop|16|(0,0.4,0.8) |
|Race0xdf and byte+0x19c=0|drawn-drop|16|(0,0.8,0.3) |
|Race0x10e and byte+0x19c=0|drawn-drop|20|(0,0.8,0.3) |
|Flags0x100000 set OR effect0x13 exists|drawn-drop|16|(0.05,0.2,0.3) |
|Flags0x4000000 set|drawn-drop|16|(0.25,0.1,0.1) |
|Flags0x200 set|drawn-drop|16|(0.25,0.1,0.5) |
|Race0x116|drawn-drop, also Z+trunc64(float(body-Z×0.5×65536))|16|(0.5,0.5,0.3) |
|Race0x108–0x10b inclusive and byte+0x19c=0|drawn-drop|20|RGB bytes+0x7a/+0x7b/+0x7c divided by255 then each×0.5 |
|Race0x10c and byte+0x19c nonzero|drawn-drop|20|(0.1,0.2,0.4) |
|Race0x103|drawn-drop|16|(0,0.25,0.5) |
|Race0x121|physical-drop-raised|16|(0.3,0.5,0.6) |
|Race0x87|physical-drop-raised|16|(0.1,0.5,0.1) |
|Race0x88|physical-drop-raised|16|(0.1,0.1,0.5) |
|Race0x89|physical-drop-raised|16|(0.5,0.1,0.2) |
|Race0x8a|physical-drop-raised|16|(0.8,0.8,0.6) |

The0x116 height and negative-drop conversions are **separate truncations**;
do not combine them into one rounded float offset. The last five rows use
physical position, whereas most rows use drawn position. These distinctions
are explicit native reads, not author-selected visual approximations.

### 3.9 Creature ability light sources

After the race/state rows, inspect ability byte+0x68 and ability timer float
+0x6c. Existing duration helpers are **D=0x4f220(C)** and
**W=0x4ead0(C,−1)**, specified in the ability/rules specs.

**First ability light:** admit abilities **0x1d,0x25,0x68–0x6b** when timer<D,
or abilities **0x26,0x27,0x2c,0x2d,0x2f,0x60,0x61,0x62,0x65** when timer<W.
Its position is drawn position plus separately truncated negative drop in Z,
plus the float local vector **(0,1,0)** rotated by **displayed yaw+0xb3c**,
each rotated component converted to fixed as in §3.7. Emit radius20 and
RGB **(0.3,0.2,0.1)×(1+0.1×cos(2×timer×π/D))**. The phase doubles timer in
float, promotes for double π multiplication/division by integer D, converts
the angle to float, then calls cosf. **D is the phase denominator even when
W was the admission duration.** There is no additional visibility test here.

**Second ability light:** admit abilities **0x1f–0x22,0x86,0x87** while timer<D,
then test **physical position plus the float offset vector+0xb4c** through
0xe08c0, sphere radius**4**, render range **renderer+0x4a0**. Convert that
offset to fixed per component before addition. If accepted, emit radius20
at physical position with separately truncated negative drop; additionally
add that+0xb4c offset for every admitted ability **except0x87**. Its RGB is
**(0.3,0.2,0.1)×(1+0.1×cos(timer×0.01))**, float32 phase/cosf. The test point
and emitted point intentionally differ for ability0x87. This is an ability
ID, not the similarly numbered race in §3.8.

These two tests are independent of the earlier lights. Renderer+0x4a0 is a
distinct frame radius:0xe2e00 sets it to **min(renderer+0x4a4,
32×renderer zone-window count+0x438,256)**. The ordinary model/item fade radius
+0x49c instead uses **min(renderer+0x4a4,12.8×that count,200)**, computing
count×64 then multiplying by float0.2. Do not substitute one for the other.

### 3.10 Equipment spirit-material lights

Last in the living-creature loop, form **q=float(signed32 C+0x70) /
float(0x5f9e0(C))**, using the existing integer combo-threshold helper. Only
when q is nonzero proceed. For each item below, use **0x109330** to obtain
its glow RGBA and require its fourth component>0. That fourth component is
an admission test; the color multiplier below uses q and RGB, not that alpha.

| Item test, in order | Light position | Radius | RGB multiplier after q |
| --- | --- | --- | --- |
|Category at C+0x6d8 equals3|Float3 C+0x11f4, converted to fixed, then subtract fixed C+0x1508/+0x1510 from X/Y|10|0.3 |
|Category at C+0x638 equals3|Float3 C+0x1374, converted to fixed, then subtract those same fixed X/Y values|10|0.5 |
|Category at C+0x598 nonzero|Physical position|10|0.1 |
|Category at C+0x3b8 nonzero|Physical position|10|0.1 |

Thus each light RGB is **(glow RGB×q)×the listed factor**, in that order.
The float3 fields in the first two rows are pose-produced render positions;
their stored render-origin translations are removed to recover world fixed
coordinates. They are not interchangeable with an arbitrary root/head anchor.
These branches add no additional frustum/day/distance test or color fade.

**0x109330 color source:** inspect the item's byte count at+0x9d and its
material-code bytes at **+0x20+4×index**. Include only material codes≥0x80.
Each included code contributes the following RGBA; sum in item order and
multiply the total by float **1/included count**. With none, return all zeros.
This follows0x109960 with base RGBA1 and scalar1:

| Material code | RGBA contribution |
| --- | --- |
|0x80|(1.5,0.75,0.15,3) |
|0x81|(0.45,1.5,0.75,3) |
|0x82|(0.45,0.75,1.5,3) |
|0x83|(1.2,1.2,1.5,3) |
|Other code≥0x80|(0.5,0.5,0.5,1) |

For exact float work, the0x80–0x83 RGB is the base hue respectively
**(1,0.5,0.1)/(0.3,1,0.5)/(0.3,0.5,1)/(0.8,0.8,1)** multiplied by float1.5,
rather than separately authored rounded decimals. Ordinary lower material
codes do not enter the average. This completes the collector's source-branch
inventory; all resulting records go through the same sort/zone limits in §3.2.

## 4. Pending work and precise additional oracle inputs

- The collector source inventory/ranking/zone selection and cached bounds/group
  prepass are now specified above. A useful verification recording should
  preserve the collected records **before and after sorting**, each source's
  kind/owner identity, zone+0x444 cost, selected per-zone16 slots, shader10 slots,
  camera fixed XYZ, N and previous/current+0x4a4, derived+0x49c/+0x4a0,
  and+0x52c. Label raw generation, completed mesh job and published mesh
  separately; do not treat the latter two as interchangeable phases.
- All seven palette targets are now covered by §§2.1–2.8 and the referenced
  sky/fog/sun tables. For verification preserve the provider's camera fixed position, time,
  sampled region record, place kind/fraction, all seven target RGBA outputs and
  frame previous/current RGBA values, frame time input, renderer+0x52c and sun
  direction. Capture ambient.w as a fourth component, not an inferred channel.

No new runtime recording has been requested or performed by this spec author.

## 5. Frame-time producer, interruptions and camera-coordinate precision

This is the follow-up on the actual input to the easing coefficients above.
Sources: main loop **0x1355a0**, frame wrapper **0x134420**, game frame
**0xa5a50** (longer-budget decompile plus assembly), camera/FOV **0xadcd0**,
renderer **0xe2e00** and palette provider **0x2b0400**. No original game was
run for this trace.

### 5.1 Wall-clock source and the1000-ms cap

0x134420 obtains the standard-library high-resolution performance frequency
and counter. It converts the counter to **signed64 integer nanoseconds**:
whole seconds×1000000000 plus the integer division of remaining ticks×
1000000000 by frequency. Subtract the previously stored nanoseconds at
global **0x551cb0** to obtain a signed64 elapsed interval, then store the
**current full timestamp immediately**, before running the game frame.
The supplied game-frame double milliseconds is computed as
**(double(elapsedNanoseconds)/1000000000.0)×1000.0** in that order.

**Caller identity/fraction clarification:** native0x134536–0x134558 performs
the double conversion/divide/multiply and then **directly calls0xa5a50**.
There is no intervening0xf5520 call, whole-millisecond truncation,
fractional-ms carry balance or zero-interval branch. Global0x551cb0 stores
the full previous nanosecond timestamp, not a fractional remainder. The
separate0xf5520 host tick is called by host thread0x83e60 with timeGetTime
integer differences and updates a different World object. This corrects
the older ordinary-frame attribution in creature-physics §1; detailed
native call/step examples are gameplay camera §7.2.3.

At game-frame entry0xa5a50, **0xa5afc–0xa5b0a** caps this double interval to
**1000.0 ms**. The native code adds no lower clamp. For the ordinary finite
nonnegative performance-clock intervals this is min(elapsedMS,1000). The
1000 double constant is at0x437fd8. Excess elapsed time is **discarded**:
the previous timestamp was already replaced with the complete current
timestamp, so a long interruption does not leave a multi-frame catch-up
debt in this frame-time producer.

The renderer's argument7 to0xe2e00 is **float32(cappedDoubleMS)**, preserved
through the game frame; renderer also stores this float at+0x330. The scene
RGBA easing, adaptive-range transitions and other renderer calls to0xa7a0
therefore promote that rounded float to double for the coefficient helper.
This is elapsed render/game-frame time, **not a consumed world-update
substep, simulation remainder, time-of-day delta or speed-scaled clock**.
World update may consume its own substeps, but their remaining duration does
not replace this renderer argument.

Rechecked at **all four0xa7a0 call sites in0xecda0**: the+0x4a8 easing,
both ordinary range-transition branches and the flight/glide range branch
all use **double(float32(cappedDoubleMS))**.0xe2e00 passes that same
float scene argument to0xecda0 at0xe3096; its saved float is promoted
afresh at each coefficient call. None recovers the original capped Game
double, truncates to integer ms, or substitutes a simulation step. The
retained-double Game camera/FOV call described next is a different boundary.

**Sky eased-palette clock uses the same boundary.** Normal sky1/sky2/fog/
sun transitions and their current-block override path in0xe2e00 likewise
use **double(float32(cappedDoubleMS))**, not the original Game double.
For the normal path the float scene argument is explicitly promoted at
**0xe3696**; the override path reloads the same float argument and
promotes its low lane at **0xe37cc–0xe37dc**. RGBA helper **0xcb5c0**
receives that already-promoted **double as its third argument** and
forwards it unchanged to0xa7a0 at **0xcb621**, with its fourth float
rate argument. The sky calls at0xe3781/0xe379c and override calls at
0xe3884/0xe389f do not introduce a separate timestamp or another integer
truncation. Direct fog/sun coefficient calls in those branches use the
same promoted scene duration. Thus the 'frameMS' in §2.7's sky easing
contract denotes this rounded scene duration; keep the Game camera/FOV
double contract below separate.

**FOV precision distinction:** the call to0xadcd0 at **0xaaf02** passes the
capped interval still as **double**, without passing through the renderer's
float conversion.0xadcd0 retains that double for its0xa7a0 calls, including the
FOV transition in §1.3. Thus scene easing and FOV use the same capped wall
interval, but not necessarily the same floating representation. Near a
hundredth-millisecond boundary, **trunc(doubleMS×100)** can differ from
**trunc(double(float32(doubleMS))×100)**. Preserve this distinction when
comparing exact helper coefficients; do not silently round the FOV input to
the renderer's float merely because both ultimately update float fields.

### 5.2 First call, focus, delays and skipped calls

The previous high-resolution timestamp is a function-local static initialized
on the **first valid0x134420 invocation** with its own performance-counter
sample; the routine immediately samples again for the first elapsed interval.
Consequently the first interval is the short time between those two samples,
not elapsed time since application startup and not a fixed16-ms bootstrap.
Subsequent valid calls use the saved timestamp. This static is not reset by
changing a palette target, opening a menu or changing worlds in the traced
producer.

0x134420 performs its update only while the global renderer/device-related
pointer0x551a90 and Game pointer0x551a80 are nonnull. When that outer guard
skips the routine, it does not update the previous timestamp there. The next
valid call therefore measures the accumulated performance-clock interval and
the game-frame1000-ms cap still applies.

The main loop's inactive/not-foreground route at **0x137b83** sleeps16 ms,
then joins the normal call to0x134420 at **0x137b8e**. It does **not** replace
the frame interval with zero or bypass that call. The traced producer has
no separate minimized-window elapsed-time reset. If a window/message/device
operation blocks frame calls, its resulting long clock interval is handled
by the same cap, rather than a special pause-time subtraction. This statement
does not assume how a particular OS clock counts suspend; it specifies what
the native producer does with the interval that the clock supplies.

0x134420's later optional frame-rate sleep and presentation happen after the
game update. Its timeGetTime calls measure that pacing budget; they are not
the elapsed-time source passed to0xa5a50. Such sleeps, presentation waits and
ordinary background-loop delays are included in the next high-resolution
interval. Menus/input eligibility don't zero the scene interval: even if a
world clock remains unchanged, the renderer can continue easing toward its
unchanged target while frames continue.

### 5.3 Palette camera XY: confirmed truncation, separate block reads

The statement in §2.7 is confirmed at **every direct camera XY region/height
call in0x2b0400**:

- **0x358e0 neighbourhood:** camera fixed X/Y divided by65536 with signed
  truncation toward zero.
- **0x35ce0 place-at-point:** the same truncated block coordinates.
- **0x35ea0 ground height, mode0:** the same truncated block coordinates.

The subsequent warmth0x2d9b60,dryness0x2b5960,lava0x2b9750 and forest0x2d9f30
calls consume the resulting neighbourhood/place records, not a new fractional
camera XY. The interior big-structure query similarly starts with truncated
camera block XY and then floor-divides those integers by1024, as §2.7 states.
For example fixed X=−1 (−1/65536 block) becomes integer block0 for these
queries, whereas fixed X=−65536 becomes−1. Do not replace this with mathematical
floor of the original fixed coordinate.

There is a deliberate separate path: the provider's effective-world-block
reads0xeab0 for the interior flag and final water test receive the **original
fixed camera XYZ**. They are not supplied the truncated region-query inputs.
The far-flight helper0x2b8050 consumes its own player/anchor fields (§2.8),
not a fractional camera-height query. Capturing raw camera fixed XYZ plus
the actual truncated region/height query XY makes this distinction testable.

## 6. Frame-input adapters: zone window, adaptive range and creature radius

### 6.1 Producer and units of renderer+0x438

This is a **signed32 horizontal zone-window radius N**, measured in
**64-block zones**. It is not a total number of visible zones, a block
distance, a mesh-group count or the CPU-performance setting. The square
visited by the scene/adaptive-range passes has inclusive zone coordinates
**cameraZoneX−N through cameraZoneX+N**, and likewise for Y: nominally
**(2N+1) squared zone positions** before availability/culling tests.

The normal scene caller is **0xa5a50 at0xad863–0xad8b3**. Read signed32
**Game+0x1b9c**, the `renderDistance` setting, multiply by30 with32-bit
integer arithmetic, then divide by100 with signed truncation toward zero.
If that quotient is **less than4**, supply **3**; otherwise supply the
quotient. This value is **argument11 of0xe2e00**, called at0xad9d4. The
scene copies it directly to renderer+0x438 at **0xe2fe1**, before updating
the adaptive range. There is no additional upper/lower clamp in that
scene assignment.

The settings default is **50**, producing **N=15**, hence a31-by31
candidate square. The normal options UI permits5 through100, in steps
of5, producing N=3 through30;5 and10 both produce3,15 produces4,100
produces30. The file loader **0x13aa50** reads the recognized setting as
an integer without applying the UI arrow bounds. Thus the UI range is
not an extra clamp on manually supplied configuration values. The caller
retains its quotient-below4 fallback even for such inputs. Settings
defaults/application are otherwise specified in frontend-options §1.

Base renderer constructor **0xcbd10**, store **0xcbea8**, initializes
+0x438 to0. That construction value is not the gameplay setting default:
the first scene call replaces it with its supplied argument. A replacement
renderer should use the current scene's caller-derived N, not keep0 or
replace N with a constant draw-distance guess.

### 6.2 Meaning, persistence, reset and derived ranges

The float32 at **renderer+0x4a4** is the **current adaptive scene range in
blocks**, produced by **0xecda0** exactly as §2.6 describes: loaded/published
mesh availability, their cached geometry/cost, camera/player state and its
previous value determine it. It is not a direct copy of a percentage or
an independent graphics setting, and is not intrinsically the constant256.
The normal Render Distance setting influences it through N, the window,
initial squared-distance bound and triangle budget; CPU-performance is
not an additional scalar in these formulas.

The constructor initializes +0x4a4 to **0**. Explicit reset helper
**0xe8a30** writes **0** to that field only. Its identified normal-frame
caller at **0xad724–0xad752** invokes it when the requested world identity
differs from the current world: requested seed **Game+0x206c** differs
from **World+0x220**, or requested name **Game+0x2070** differs from
**World+0x200** through comparison0x6ddf0. This is before scene submission.
The reset does not set N or either derived range. There is no reset of
+0x4a4 merely because a new frame starts or Render Distance is changed in
the traced caller/scene path; preserve the previous value for §2.6's
asymmetric snap/easing rules.

On **each0xe2e00 call**, it first installs the supplied camera, matrices,
N and frame inputs, then calls **0xecda0 at0xe3096**. After that update,
it overwrites the two derived float32 radii at **0xe330d /0xe333c**,
before scene/model/light collection. The exact finite-value arithmetic is:

- Form **N times64 in signed32**, implemented by shifting its32-bit word
  left6. Convert that completed signed32 result to float32 once.
- For **+0x49c**, multiply that float by stored float32 **0.2**
  (word **0x3e4ccccd**, RVA0x42e700), select the smaller of this product
  and current+0x4a4, then the smaller of that result and **200**.
- For **+0x4a0**, multiply the same converted N-times64 float by exact
  float32 **0.5** (0x4ac964), select the smaller of this product and
  current+0x4a4, then the smaller of that result and **256**.

Keep the multiplication order; do not first replace the first formula by
a multiplication of N by a separately rounded12.8 constant. These fields
are overwritten directly after adaptive updating, without a second ease
or a positive minimum clamp. At default N=15 their setting-derived limits
are192 and256 blocks, respectively; adaptive range can reduce either.
The64-block margins in particular source gates are added later exactly
where §§3.3–3.10 specify them, not to these stored radii generally.

The candidate floor0.001 and special ability's target minimum64 in §2.6
are **target rules inside0xecda0**, not a universal post-update clamp on
+0x4a4. In particular the constructor/reset zero participates in the
first upward easing step. The flagged-block/effect branch's256 target
is also conditional; it does not justify imposing a global256 ceiling
on +0x4a4. Use §2.6's complete rules rather than treating the field name
as a maximum user-configured draw-distance constant.

### 6.3 Physics/rules producer of creature light-radius+0x198

The collector reads an existing **float32 radius in blocks**; it does not
calculate the stat itself. Creature base-data constructor **0x4d600**, at
**0x4d6e7**, initializes this field to **10.0**. Its subobject begins at
creature+0x10, so its local+0x188 store is creature+0x198.

Physics **0x2bb8e0**, in the grip/lamp stage, calls **0x5f6e0 at0x2bbda3**
and stores the returned float at **0x2bbda8**. This stage writes the radius
only when either:

- This creature pointer equals **World+0x448**, the local player; or
- The creature hostility byte **+0x60 is nonzero** and the remote-client
  byte **World+0x358 is zero**.

Those are the exact authority conditions. It is not every nonlocal
creature on every client, nor every hostility-zero player on an
authoritative world. The update does not additionally require a carried
lamp, lamp-toggle flag, positive emitted RGB, nearby camera, frame-light
selection, swimming or a particular ability. Outside that authority
gate this physics stage retains the stored radius. Remote state has its
own float field34 for+0x198, as documented in netcode-packets; the
collector must retain/consume that state rather than recomputing all
remote creatures from local counters every render frame.

Rule **0x5f6e0** chooses a signed32 integer input: for hostility+0x60
**zero**, use artifact skill counter **+0xb74**; otherwise use
**10 times level+0x144**, with32-bit integer multiplication. Convert this
integer to float32. Divide it by20, add1, divide1 by that result,
subtract the quotient from1, multiply by12, then add8. Every listed
operation is float32, rounded before the next; there is no fused
operation, explicit result clamp, percentage scaling or smoothing here.
Do not replace this evaluation by the real-number-equivalent n/(n+20)
expression when exact float agreement matters. Stored constants are1 at
0x4ac980,20 at0x42e5a0,12 at0x42e59c and8 at0x4aca30. Inputs0,10,20
give radii8,12,14 blocks, respectively. The constructor's10 is therefore
a startup/default value, not the zero-artifact rule result.

For the local player, **0x66850**, called from world update0x2da6c0,
rebuilds +0xb74 along with the other six artifact counters from the
category23 stacks in inventory tab5: each qualifying stack contributes
one counter increment according to unsigned item-modifier modulo7;
remainder6 contributes to+0xb74. Stack count does not multiply the
increment. This provenance and physics ordering are already specified
in lore-and-artifacts §§5.2–5.3 and creature-physics §3.2.

These traces resolve the normal frame-setting adapter and light-radius
producer. They do not replace missing mesh readiness/cost/bounds with
synthetic loaded-zone data: an exact adaptive-range oracle still needs
the labeled mesh-stage and camera inputs listed in §4. No game execution
or new owner capture was performed for this clarification.
