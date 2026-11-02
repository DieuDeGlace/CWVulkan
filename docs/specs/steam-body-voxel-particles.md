# Steam death-body voxels in the shared live particle list

Independent native-only bounded contract for the ordinary death calls to
**0x90390**, followed by the existing **0xb37f0/0xe6840** consumers. No
engine/test/extractor/SDK/reconstruction or owner-capture data was read,
and no game was executed. RVAs are original Steam addresses. This closes
the death-produced **style0, collision1** path; it does not make every
producer of style0 or every other particle style complete.

## 1. Reached death pass and part calls

The Game frame's death candidate collection, unsigned-pointer set order,
prepared-byte+0x1500 gate and retained-renderer frustum/range admission
are already specified in
[steam-transient-light-producers.md§3](steam-transient-light-producers.md).
In particular this is a deduplicated per-frame death-effect set, not a
scan of every corpse every frame. Accepted candidates require the prepared
byte and0xe08c0 at0xa9084, using drawn XYZ+0xb10, half the maximum body
extent and range100. It uses the renderer state retained at that point,
before later camera/scene setup. This helper introduces no new persistent
"already fragmented" flag or per-corpse lifetime admission.

For each candidate that passes, invoke **0x90390** in the following exact
order. Model pointers and matrices are from the candidate's already
prepared render state, not a freshly recomputed death pose. Offsets below
are from the creature; matrices are float4×4 values.

| Order/call RVA | Source model pointer | Source matrix | Tint |
|---|---|---|---|
|1,0xa9110|+0xd10|+0xe30|(1,1,1,1)|
|2,0xa91c4|+0xd18|+0xe30|(byte+0x7a÷255, byte+0x7b÷255, byte+0x7c÷255,1)|
|3,0xa9244|+0xd20|+0xdf0|(1,1,1,1)|
|4,0xa92c4|+0xd30|+0xfb0|(1,1,1,1)|
|5,0xa9344|+0xd30|+0xff0|(1,1,1,1)|
|6,0xa93c4|+0xd28|+0xeb0|(1,1,1,1)|
|7,0xa9444|+0xd28|+0xef0|(1,1,1,1)|
|8,0xa94c4|+0xd48|+0x1130|(1,1,1,1)|
|9,0xa9544|+0xd48|+0x1170|(1,1,1,1)|
|10,0xa95c4|+0xd50|+0xf30|(1,1,1,1)|

Only order2 has the additional condition **creature flags+0x80 &0x400=0**.
Each division in its RGB tint is float32 division by255. All these callers
pass **style0** as the helper's final style input. The offsets above specify
the actual source binding without guessing names for otherwise untraced
part/equipment slots. Reusing a model pointer with another matrix still
performs another independent voxel visit and CRT sequence.

0x90390 returns immediately for a null source model, without voxel visits
or random draws. For nonnull models it reads raw voxel data+0x68 and full
signed32 dimensions+0x88/+0x8c/+0x90. Nonpositive dimensions produce no
visited voxels. No GPU-ready check or per-voxel camera-distance/frustum
test is added by this producer. A valid raw source allocation is required.

These calls precede ordinary **World+0xb0 burst expansion at0xaa1f0**.
Other Game actions can lie between the two phases; their random draws must
not be silently reordered merely to process all particles together.

## 2. Voxel visitation, surface test and CRT order

Visit **X ascending outermost, Y ascending middle, Z ascending innermost**,
from0 through dimension−1. The raw RGB address is still
**base+3×((z×sizeY+y)×sizeX+x)**: visitation is not the allocation's
X-fast linear order.

Use **0x16f230 with its explicit policy argument0**, for the current voxel
and each queried neighbor. This excludes the global empty RGB at0x55284c
and the exact pure attachment-marker triplets **(255,0,0),(0,255,0),(0,0,255)**.
Its result0 means occupied. The producer passes policy0 directly rather
than consulting the model's+0x99 classification byte.

For an occupied current voxel, check the six immediate neighbors in order
**X−1,X+1,Y−1,Y+1,Z−1,Z+1**, stopping at the first empty result. Out-of-
allocation queries supply the global empty triplet. If all six are occupied,
skip this interior voxel. There is one candidate per exposed occupied
voxel, not one candidate per exposed face. Empty, marker and interior
voxels consume **no producer CRT draw**.

For each exposed occupied voxel:

1. Draw **rand at0x90853**. Admit only when its signed remainder2 is0.
   An odd result ends this voxel's work after this single draw.
2. On admission, draw **rand at0x9087c** and set age to
   **−(rand remainder1000)**, giving0 through−999 for ordinary CRT results.
3. Draw angular velocity **Z,Y,X** at **0x90a4a/0x90a61/0x90a78**. Each
   component converts rand to float32, divides by32767.0f, subtracts0.5f,
   then multiplies by100.0f, with separate rounding. These are the same
   −50..+50 degrees/second bounds as the burst spin input, but there are
   no burst-style random position, velocity or angle groups here.
4. After record construction, draw **rand at0x90ba5** for temporary-list
   insertion direction, as specified in§5.

Thus rejected surface candidates consume1 draw; admitted candidates
consume6 total. There is no direct srand, random normalization, random
bounce or additional draw based on face count, part nullness or shared
cap removal. The stream is the executing Game thread's existing CRT stream.

## 3. Position and outward velocity

For each admitted integer voxel coordinate, transform **(x,y,z)** by the
full supplied part matrix, using float32 coordinates, products/additions,
one float32 reciprocal of homogeneous W, and component multiplication by
that reciprocal. There is **no voxel+0.5 offset** in this point transform.
The usual affine part matrices have W1, but the native calculation includes
the divide. Convert each normalized component to fixed by float32
multiplication by65536 followed by signed64 truncation toward zero.

For stored position X/Y, subtract **Game fixed fields+0x1df0/+0x1df8**
from these transformed fixed X/Y using low64 subtraction. Z retains its
transformed fixed value. The already-translated part matrix supplies the
actor/part location: the helper does not add the creature's drawn tuple
again or use an additional actor-velocity input to move the spawn point.

These Game fields are the last camera update's negative **look-at** X/Y,
not negative final-eye coordinates or an automatic per-actor cache restore.
The cached-origin float arguments passed by the caller are unused by this
helper. Exact argument identity, matrix retention and frame lifetime are in
[pose/body coordinate lifetime](steam-body-particle-coordinate-lifetime.md).

Compute the source allocation center from **full dimensions×0.5f** on
each axis, transformed by that same full matrix and homogeneous-divide
calculation, then converted to fixed by the same float32×65536/truncation.
This is not an occupied-voxel centroid or the integer(dim−1)/2 point.

Outward velocity is derived independently per axis from transformed voxel
fixed position minus transformed center fixed position, **before the Game
X/Y subtraction**. The exact integer/float staging at **0x909b3–0x90a36**:
take that low64 signed difference; multiply it in low64 by **131072**
(obtained by truncating float word0x48000000 at0x42e5fc); divide the signed64
product by65536 toward zero; convert that full signed64 quotient to float32;
then multiply by float32 1/65536. There is no signed32 narrowing in this
velocity conversion. In the ordinary nonoverflowing range this is outward
speed2 per second times the quantized displacement from the part center.
It is **not normalized**, and it does not inherit actor velocity or hit push.

The source point/center arithmetic groups the first X/Y products, adds Z,
then translation, all in float32 before the common reciprocal multiply.
Position and center are truncated separately before their fixed difference;
subtracting their float positions first is not the stated operation.

## 4. Initialized fragment record and retained part matrix

The helper builds the same **0xb8-byte live payload** as ordinary particles,
directly rather than calling0x7fad0. Offsets exclude the16-byte node header.

| Offset | Body-death output |
|---|---|
|+0x00|signed32 style0 from these callers|
|+0x04|signed32 negative/zero age from§2|
|+0x08/+0x10/+0x18|absolute fixed position from§3|
|+0x20/+0x28/+0x30|a second copy of that same stored fixed XYZ|
|+0x38/+0x3c/+0x40|float angles(0,0,0)|
|+0x44/+0x48/+0x4c|raw source RGB byte converted to float32, **divided by255**, then multiplied by caller tint RGB|
|+0x50|caller tint alpha times1; these callers supply1|
|+0x54/+0x58/+0x5c|outward velocity from§3|
|+0x60/+0x64/+0x68|random angular velocity from§2|
|+0x6c..+0xab|retained part matrix described below|
|+0xac|float size1|
|+0xb0|collision byte1|
|+0xb1|freeze byte0|
|+0xb2..+0xb3|not initialized/copied as fields|
|+0xb4|signed32 lifetime5000 ms|

RGB uses **division by255.0f followed by tint multiplication**, not a
stored reciprocal or a stock-CUB half-color step in the payload. Vertex
encoding for the eventual cube resource remains the separate native mesh
contract. No source face normal, source per-vertex light/AO, actor pointer
or source-model pointer is stored with each fragment.

Copy all16 floats of the supplied part matrix into the extra matrix,
but replace **translation elements12/13/14 with0**. Preserve element15
and the other linear/homogeneous elements. This retains the prepared part's
rotation, scale and possible shear by value while dropping its translation;
future actor pose changes do not update that copy. Initial Euler angles0
mean its first draw retains that part orientation, until shared motion
advances the separate random spin.

## 5. Temporary deque, live head insertion and shared cap

Each individual part call builds its own temporary particle list. For
the final direction draw, **even remainder2 appends at that temporary
list's tail**, through0x6e710 at0x90c00; **odd prepends at its head**,
through0x6e710 at0x90bd6. This is not a key sort or a single shuffle
after collecting all voxels.

After completing that part's voxel traversal, iterate its temporary list
forward and copy each payload into a **new node at the head of
Game+0x2090's existing live list**, through **0x6e710 at0x90ccc**. Increment
the shared64-bit count at+0x2098, then free the temporary nodes/head.
This is not a splice and not a separate persistent body-particle list.

Repeated head insertion reverses the temporary forward order in the final
new block. For example a temporary order A,B,C becomes live order
**C,B,A,previous live entries**. A later part call is inserted ahead of
earlier part calls; later death candidates are likewise inserted ahead
of earlier candidates' fragments. Sampling/direction draws are still
consumed in the original candidate/part/voxel generation order.

Body calls run before ordinary burst expansion, which appends to the live
tail. Both populations are already present at the later **0xb37f0 update
at0xaaed9**. Its single shared cap removes tail entries until count1000;
there is **no independent body quota or per-part cap**. Thus the body head
entries precede old live entries and this frame's appended bursts when
that cap executes. Cap discards do not undo any producer draws. Later
unrelated frame producers are not redefined by this ordering statement.

## 6. Style0 motion, visibility, rendering and retirement

For these **style0/collision1** records, the existing shared updater route
is the same gravity, first angle increment and axis-by-axis collision path
specified for collision-on particles in
[steam-particle-bursts.md§4–§5.2](steam-particle-bursts.md). It adds the
frame's integer-ms interval to age first. While age remains negative,
skip gravity, spin and position motion; **do not hide the particle**.
On the frame age becomes>=0, run the full frame interval's motion, not
only the fraction after the nominal negative-age delay elapsed.

Apply gravity30, advance angles once, then point-collide in X/Y/Z order;
only terrain types0 and2 pass. A rejected Z attempt zeroes all velocity
and spin. Cached point-step displacement remains unchanged for the rest
of that visit. There is no second angle increment on this collision-on
route. Style0 produced by an ordinary burst instead has collision0 and
uses a different branch; style tag alone does not select collision behavior.

Retirement is **age>5000**, strictly, followed by the shared queued unlink/
free phase. If the birth delay is D=rand remainder1000, this means the first
update whose accumulated integer elapsed exceeds **5000+D**, unless the
shared cap already discarded the record. Equality remains alive. No timer is
borrowed from the actor corpse, and actor erasure does not own these records.

The shared renderer **0xe6840** draws these style0 records even at negative
age, subject to its current frustum/range and resource admission. It uses
catalog **0xa46,cube.cub**, not the original part resource. Its sphere
radius input is the stored size1, **not** a bound recomputed from the retained
part matrix or actor size. Material uses the lit **Glow0/Shininess0/White0**
branch: raw fragment RGBA times frame material, with fourth component also
times max(0.2,world light÷255). It has the same opaque depth-writing/no-cull
state, inherited WORLD inputs and full-header cube centering from the
ordinary-particle spec. There is no age-based scale or opacity fade for0.

Its model order is translation, separate particle Z/Y/X Euler rotations,
uniform size1, retained translation-cleared part matrix, then the drawn
cube resource's negative half-header translation. The spin therefore
precedes that retained matrix in the composed transform. Do not replace
the retained matrix with identity or apply source-part translation twice.

## 7. Bounded limits

This establishes the reached death family and its shared list/update/draw
behavior for valid prepared parts, finite affine pose matrices, valid raw
voxel allocations and representable fixed/step arithmetic. It does not
audit new corpse/death-notice producers beyond the linked candidate-set
contract, name all native part slots by appearance, synthesize missing
owner geometry, or specify unrelated live styles. Padding remains without
deterministic expected values. No body-specific random stream, replacement
asset, permanent emitter object or separate body-fragment capacity follows
from this native path.
