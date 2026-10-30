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

For types0 and0x7e, derive the16-block cell pair by flooring each fixed X/Y
to integer blocks, then flooring those coordinates divided by16. Require
that pair to be **present** in the local player's personal used-cell set
**personal state+0xa8**, reached through world+0x448 then player+0x1528.
Neither the object's live open/closed state nor its animation timer replaces
this membership test. The branch assumes that local-player/personal-state
context exists, as does the native lookup. These conditional lights still
rank by their emitted, height-shifted position.

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
