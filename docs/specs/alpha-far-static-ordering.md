# Alpha near/far static model and object ordering

Independent original Alpha client audit from the native-analysis Alpha
Cube.exe/decompile/assembly. **All addresses below are RVAs relative to
Alpha image base0x400000**, unlike older alpha-rendering headings which
often give full VAs. No engine, tests, exporter, build or owner captures
were inspected, and no game was executed.

## 1. Two stages around water

Frame **0xac260** draws eligible near scenery immediately during its
zone traversal: environment models through **0xbd160 at0xb2801**,
static objects through **0xbe760 at0xb3164**. These calls occur after
ordinary terrain and **before water**. The latter routine selects the
static-object catalog, not an independently identified ground-item list.
Do not create a third ground-item pass from the older shorthand “items”.
These two audited families do not exhaust all content in the scene;
§8 explicitly retains the separate World-cell list+0x30.

Farther environment models and static objects are instead appended to
**one shared deferred vector**, then sorted at **0xb332c through0xabac0**.
The vector is not split into “all objects” and “all models”. After the
water traversal ends, the frame explicitly restores depth writes on,
clockwise culling and the WORLD pixel shader at **0xba432–0xba462**.
The explicit device states there are **ZWRITEENABLE, id14, value1** at
0xba445 and **CULLMODE, id22, value2 (clockwise)** at0xba45a, followed
by WORLD pixel-shader bind0x47d10. This is a state restoration without
clearing the depth contents left by terrain/near scenery/water.
It then walks that sorted vector forward, dispatching each model at
**0xba4fe** or object at **0xba54a**. Creature draws begin afterward,
at the already specified0xba5d0 route. No depth clear separates water,
the deferred vector and creatures.

Consequently far faded geometry behind the nearest already drawn water
surface can fail depth testing. Near geometry instead exists before water
color blending. Moving all static geometry after water, or drawing the far
list before water merely because it uses the WORLD shader, changes this
observable order. Existing ordinary world alpha blending remains enabled;
the object/model helpers set their own submitted pixel Alpha.

## 2. Range inputs and the immediate/deferred decision

Let **R=Game float+0x1d0**, the render radius from alpha-rendering §7.1,
and **F=Game float+0x1d4**, its adaptive fog range. Retain the separate
float32 stages, with native constants0x2fcdbc=0.2,
0x2fcdc0=0.3,0x2fd31c=0.9 and0x302a94=22.4:

| Family | Fade-end/range input L | Full-opacity input B |
|---|---|---|
| Environment model | **float32(R times0.2)** | **float32(L times0.9)** |
| Static object | **float32(float32(F+22.4) times0.3)** | **float32(L times0.9)** |

These producers are **0xad9d1–0xada04**, **0xac4e0–0xac50c** and
**0xb06f3–0xb0766**. Do not replace B by a single differently rounded
0.18R or0.27(F+22.4) operation. Models and objects intentionally have
different L producers.

The rechecked binary float words are:

| Constant RVA | Word | Exact float value |
|---|---|---|
|0x2fcdbc|0x3e4ccccd|0.20000000298023224|
|0x2fcdc0|0x3e99999a|0.30000001192092896|
|0x2fd31c|0x3f666666|0.8999999761581421|
|0x302a94|0x41b33333|22.399999618530273|

After their existing type/resource and visibility admission, compare stored
float squared distance D² with **float32(B timesB)**. **D²≤B² draws
immediately**; **D²>B² defers**. Boundaries are at
**0xb2754–0xb276a** for models and **0xb303e–0xb3051** for objects.
The upper-range/resource/frustum admission remains the existing source
route; this rule does not authorize drawing every record outside L.
The native comparisons are the direct stored-float **COMISS followed by
JBE** at0xb2767/0xb276a and0xb304e/0xb3051. For finite generated values,
equality belongs to the immediate branch. No epsilon, sqrt comparison,
integer distance quantization or secondary threshold occurs at this split.

For the distance key, use full XYZ fixed differences from camera fixed
XYZ **Game+0x140/+0x148/+0x150**, then each completed difference's
float32 conversion times float **1/65536** through0x2c4a0. Squared norm
**0x24860** performs float X²,Y²,Z², then **(X²+Y²)+Z²** and stores the
float result. This is neither horizontal distance nor distance rounded to
whole blocks.

Environment-model distance uses its record fixed XYZ at+8/+0x10/+0x18
with an extra Z offset: convert signed32 sprite header Z+0x4c to float,
multiply0.5, then the model record's float scale+0x20, then65536 in
float through0x2c580, truncate toward zero to signed64 and add to the
record fixed Z. This is the record's vertical center, not an occupied
voxel AABB center. The distance construction is **0xb26c1–0xb274b**.
Static-object distance at **0xb300e–0xb303a** uses its fixed record XYZ
directly, without that environment-model height adjustment. Its prior
sphere-admission center is a separate input and is not the distance key.

The draw helpers receive that captured D²,L,B even in the later pass;
they do not remeasure camera distance after water. Pixel Alpha is1 when
D²≤B². Otherwise widen the float D² for the native double sqrt helper,
narrow the result to float, then float **1−((sqrtDistance−B)/(L−B))**.
Native calls/narrowing are at **0xbe64c–0xbe66f** and
**0xc0419–0xc043c**. The verified helper does not add a new final CPU
clamp or special L=B fallback around this expression. Its material/light
inputs remain separate from this pixel Alpha.

## 3. Mixed deferred records and sort/ties

Each deferred record is **16 bytes**:

| Offset | Field |
|---|---|
| +0 |32-bit environment-model record pointer, or0 |
| +4 |32-bit static-object record pointer, or0 |
| +8 | float32 squared distance captured above |
| +0xc | Signed32 one-based ordinal within the active cache's environment list, used by model animation; not a meaningful required field for an object-only record |

Initializer0xabc00 clears the first two pointers. Append0xc1190 copies all
16 bytes; model append is at0xb27b1, object append at0xb308d. Within a
valid ordinary record exactly one pointer is nonzero. The dispatch tests
model pointer first, then object pointer, but that does not group separate
records by kind. Preserve source append order before sorting. The late
loop reads the ordinal only for a nonzero model pointer.

The former description of+0xc as an owning-zone pointer was incorrect.
The cache's environment-list counter starts at zero, increments before
each record's checks at0xb1ea0–0xb1ea5, and is stored at deferred+0xc
at0xb2787–0xb27a1. Rejected records and special-effect type0x3c still
consume an ordinal. Near0xb27bb–0xb2801 and far0xba4d6–0xba4fe
pass that same integer to0xbd160; its animation phase reads the integer
and multiplies it by30. It is not dereferenced as a zone. The ordinal
restarts for each cache and is unrelated to the globally sorted position.

This is **one frame-wide sort after collection**, not a separate sort per
zone or a merge of independently sorted model/object vectors. At
**0xb329a–0xb32d4** the enclosing World-cell coordinate scan finishes its
remaining coordinates; **0xb32fc–0xb332c** then passes the shared vector's
full begin/end into0xabac0, which derives element count and initial sort
budget from the byte span divided by16. Consequently a farther
object can precede a nearer model, and finite ties in a small combined
vector retain that combined input order across families/zones. The record
kind does not define a water-side partition beyond the distance decision
in §2.

Sort **0xabac0→0xaa490** compares **only D²**, largest first. There is
no pointer, model/object-kind, zone or type secondary key. Its control
structure and whole-entry movements are the same independently specified
MSVC order mechanics in steam-controller-handoff §4, with **the strict
float comparison reversed and16-byte entries**:

- Insertion sort for partitions≤32 is stable for finite equal keys.
- Larger partitions use the same in-place three-way equal band, shorter
  partition recursion and budget update half+floor(half/2).
- Pivot samples are first/middle/last for N≤41; N≥42 uses the same
  four-triple median-of-nine sequence. Individual triple comparisons move
  equal keys only indirectly through later partition movements.
- Heap fallback is a **minimum heap** for this descending result: choose
  the smaller child; equal children choose the **right** child. Creation's
  final upward repair stops at the original parent/subheap top; extraction
  repairs from root0.

Alpha sources for those order facts are **0xa9470** insertion,
**0xa9b50/0xa9e60** samples, **0xaaeb0** partition,
**0xa98d0** heap creation, **0xa9110** repair and **0xaa7c0** extraction.
Thus equal-distance records in a larger scene need not retain insertion
order; a replacement stable sort is not the exact native contract. This
specification covers finite generated distances, not a fabricated ordering
for corrupted NaN coordinates.

## 4. Verification boundary

Stage order, thresholds, distance keys, mixed record layout, opacity
arithmetic and finite sort mechanics above are independently verified.
No new Alpha runtime oracle was run. A useful bounded draw oracle would
label the original collection order, model/object family, fixed record
XYZ, model scale/header Z when relevant, R,F,D²,L,B and final sorted
ordinal, with a label indicating **near-before-water** or
**deferred-after-water**. A screenshot alone cannot establish tie order
or whether a missing submerged far object failed the retained depth test.

### 4.1 Bounded threshold reference and remaining admission scope

For an arithmetic-only input **R=640**, the environment-model path yields
**L=128**, **B=115.19999694824219**, **B²=13271.0390625** with the native
float stages. For records that already passed their independent admission:

| Stored D² | Native route |
|---|---|
|13271.0380859375, preceding float|Immediate, before water|
|13271.0390625, exact threshold|Immediate, before water|
|13271.0400390625, following float|Shared deferred vector, after water|

These are scalar references derived from the binary words, **not runtime
captures, native defaults or claims that all such records pass frustum/
resource admission**. They distinguish the strict deferred comparison and
the separately rounded0.2/0.9 stages from a single0.18 multiply.

The subsequent admission audit in §§5–8 closes the previously outstanding
cache-window, family lookup and range/frustum decisions. These precede
the distance split in §2. Their centers, arithmetic and ordering remain
separate inputs; in particular the shared range test is inclusive.

## 5. Shared fixed-point admission tests

All addresses in these additions remain **Alpha RVAs**, relative to
image base0x400000. The frame is0xac260. Fixed coordinates are signed64
with65536 units per block. The camera position is Game+0x140/+0x148/+0x150;
the horizontal scene origin is the signed64 pair Game+0x1d8/+0x1e0.
These are distinct from the physical player position used by §8.

### 5.1 Sphere test0x7f760

Inputs are a fixed XYZ point, float32 sphere radius and float32 range.
The radius affects the frustum test only. It is **not added to the range**.
This helper is called for environment models at0xb26b4 and static objects
at0xb2df0.

First subtract the point from the fixed camera, component by component.
For each of X,Y,Z, form the signed64 square, then divide that individual
product by65536 with signed division truncating toward zero. Sum the
three signed64 results. Helper0x3ac20 establishes this order: the native
operation is not a division or shift of the already summed squares.
Separately square the range in float32, multiply that float32 result by
65536 in float32, and truncate the resulting value to signed64. Reject
only when the fixed sum is **greater** than this integer threshold;
equality passes. Native comparison is at0x7f812–0x7f820. There is no
floating D² replacement for this early gate and no saturation of the
integer intermediates. These rules describe finite ordinary coordinates
whose intermediate products fit the native signed64 representation.

Only after range admission, add the fixed scene origin to point X/Y;
leave Z unchanged. Convert each completed signed64 coordinate to
float32, then multiply each by float32 1/65536. All three resulting block
coordinates are computed before testing planes. For each of the **six**
stored planes at Game+0x1000fa4, stride16, evaluate its X/Y/Z products in
float32, add X product to Y product, then Z product, then plane constant,
then sphere radius, with a float32 result at every stage. Reject a plane
only when the result is strictly negative. A sphere touching a plane
passes. The helper does not omit an upper screen plane. Plane consumption
is0x7f826–0x7f907; this section takes the stored planes as input rather
than deriving a different camera convention.

The scale constants are float32 65536 at0x2fce7c and float32 1/65536
at0x2fcd94. The range threshold's float-to-signed64 conversion is the
truncating helper0x28d946, not float-to-signed32.

### 5.2 Cached-box test0x7f3c0

Inputs are fixed minimum/maximum XYZ and a float32 range. This test's
range center is calculated from the supplied extrema, independently of
the separately stored cache center used for sorting.

Add minimum and maximum in signed64 for each axis. Multiply each sum by
the fixed half32768, then divide by65536 with signed truncation, through
0x2c900. For supported coordinate magnitudes this is the midpoint rounded
toward zero in fixed units. Compare **horizontal X/Y distance only** from
this midpoint to the fixed camera: divide each signed64 squared
difference by65536 before summing. The integer threshold and inclusive
comparison are the same float32 range-square, float32 multiply65536,
truncate-signed64 sequence as §5.1. Z is excluded from this range sum.
Source0x7f3d3–0x7f536; the fixed half originates from double32768
at0x302a88.

For the frustum part, add scene-origin X/Y to each endpoint before
converting signed64 to float32 and multiplying by float32 1/65536. For
each of all six stored planes choose maximum for an axis whose plane
coefficient is strictly positive, otherwise minimum. Multiply the selected
coordinates by their coefficients in float32; add the **Y product and X
product**, then Z product, then plane constant, rounding each stage to
float32. Reject only a strictly negative result. Thus a box touching a
plane passes. Source0x7f53c–0x7f727. This is the positive-vertex box test,
not a sphere around the cache center or a test of all eight corners.

## 6. Render-cache collection and environment-model admission

### 6.1 Stored window and readiness transfer

The cache window has signed32 origin X/Y at Game+0x2ac/+0x2b0 and
signed32 side count N at+0x2dc. Its array pointer is+0x2e0 and each
element is0x268 bytes. The frame visits X in increasing order from origin
X through origin X+N **exclusive**, with Y as the inner increasing loop
over the corresponding exclusive interval. Coordinates outside
0≤X,Y<0x80000 are skipped. The ring-array slot is
**(Y remainder N)×N+(X remainder N)**. Admitted coordinates are
nonnegative, so these remainders are nonnegative. Sources0xac4e8–0xac586
and0xad442–0xad472. These are **32-block render chunks**, as independently
confirmed by the cache-coordinate left-five-bit transform in the terrain
draw. They are not the256-block World cells in §8.

Readiness is not inferred merely from a loaded World cell. Before the
coordinate traversal,0xac475–0xac4d8 scans all N² array elements in
linear storage order. It builds a frame-local count by the element's
32-bit key at+0x70, incrementing only for a nonzero key whose byte+0x74
is nonzero. The meaning/producers of this opaque key and byte are not
newly traced here; their exact consumption is verified.

For each visited slot, the lookup of its key in that count defaults to
zero. When the count is zero, lock the slot's critical section+0x250.
If its pending mesh-group list+0x10 has nonzero count at+0x14, release the
old active GPU group resources, assign the pending group pointers to the
active list+0x08, clear the pending group-list nodes/count, and assign
pending environment records from list+0x248 to active list+0x240 in their
stored order. This is a **pending-to-active replacement**, not clearing
the active scenery without replacement. The record-list assignment does
not itself erase its source list. Unlock afterward. Source0xac58a–0xac652;
group assignment0xab940 and environment assignment0x570a0. A nonzero
count for that key suppresses this transfer for the frame, preserving
the previous active data.

Then lock+0x250 for the ordinary cache admission work. Require active
mesh-group count at+0x0c to be nonzero and stored signed32 coordinate pair
at+0x18/+0x1c to equal the requested window coordinate. A stale ring slot
with a different coordinate is excluded even if it contains groups.
Call the box test of §5.2 with cached fixed minimum+0x20,
maximum+0x38 and range **float32(A+32)**, where **A=float32(F+22.4)**
and F is Game+0x1d4. The32 is float32 at0x302aa0. Only a passing cache
pointer is appended to the frame's candidate vector. Source0xac658–0xac6de.

For that candidate calculate its sort key+0x6c from the cached fixed
center+0x50 minus the fixed camera: use all three axes and the per-product
fixed dot/division of0x3ac20, convert the resulting signed64 sum to
float32, then multiply by float32 1/65536. This yields a squared distance
in block units. **Do not substitute the floating per-record norm in §2**,
or the horizontal gate in §6.2. Source0xac6e3–0xac737. Release the lock
after the remaining cache prepass at0xad42d–0xad434.

### 6.2 Candidate ordering and coarse model gate

When Game byte+0x8006e6 is nonzero,0xb1af4–0xb1b04 empties the candidate
vector before drawing; that same byte also suppresses §8's World-cell
scan. This audit preserves its exact gate without assigning an untraced
UI/mode meaning to the byte.

Sort the candidate pointers **ascending only by cache float32+0x6c**
at0xb1b48, through0xaba70→0xaa260. For≤32 candidates, insertion0xa9320
retains order for finite equal keys. Larger lists use the same native
in-place introsort mechanics described in §3, with four-byte cache
pointers, ascending comparisons and a maximum-heap fallback; do not add
a coordinate/pointer tie key or assume global stability. The cache sort
is separate from the later descending mixed-record sort.

Visit that sorted vector in order, locking each cache+0x250. Recheck that
its active mesh-group list+0x08 is nonempty; otherwise skip its entire
environment-model pass. After its ordinary terrain-group work, apply a
second coarse gate to environment records: convert the fixed difference
between cached center+0x50 and camera to float32 block X/Y, square each in
float32 and add in float32. Compare that horizontal squared distance to
**float32(float32(L+32)×float32(L+32))**, where model L=float32(R×0.2)
from §2. Reject only greater; equality passes. This is a floating coarse
gate, unlike §5.2's fixed midpoint gate. Sources0xb1bb4–0xb1be1 and
0xb1dde–0xb1e54; pair extraction0x58de0→0x11df0, norm0x24830.

### 6.3 Individual environment records

Iterate the active environment **list** at cache+0x240 in stored order.
Type is signed32 at record+0. Type0x3c takes a separate effect path
0xb1eb4–0xb212a, which does not enter the ordinary model draw/far vector.
For the ordinary path require nonnegative type less than the size of the
catalog at Game+0x800718; exclude type0x12; require the selected sprite
pointer to be nonzero. These checks are0xb25af–0xb25ea. They are not a
universal test for all kinds of scene records.

These are **Alpha environment-record type IDs and Alpha catalog indices**,
not Steam model IDs or an asserted cross-edition asset correspondence.
The record comes from the active Alpha cache list+0x240, and the sprite
comes from the Alpha client's own environment catalog+0x800718. This
audit does not identify catalog-entry asset names or establish a mapping
to another edition's records. The geometric rules can be implemented
independently, but a type exclusion requires this catalog provenance;
do not apply numeric0x12/0x3c to unverified Steam records.

For a valid sprite, read signed32 header dimensions X at+0x44 and Z
at+0x4c, and record float32 scale+0x20. Sphere radius is
**float32(float32(max(header X,header Z))×scale)**: Y is unused and
there is no extra one-block padding. Sphere point starts at the record's
fixed XYZ+0x08/+0x10/+0x18. Raise Z by the truncating fixed conversion
of **float32(float32(float32(header Z)×0.5)×scale)**; the conversion
multiplies by65536 in float32 before truncating to signed64, through
0x2c580. Call §5.1 with this point, radius and the model's L. Source
0xb25f0–0xb26bb; header accessors0x24810/0x7b5f0. This raised center
also supplies that model's later floating D² in §2.

Only after these gates does the model reach the established near/far
decision. Near rendering or deferred append rejoins the environment
list loop at0xb212f. Finish all sorted caches and their model lists
before the independent World-cell object pass begins at0xb21c7.
Unlock each cache on leaving its pass at0xb2169. Therefore, before the
mixed sort, **all deferred ordinary environment models precede all
deferred static objects**, with order within the first family inherited
from the ascending cache sort and then its stored model-list order.

## 7. Individual static-object admission

The World-cell traversal in §8 supplies the static-object list at each
cell+0x0c. Visit that list in stored order. Require signed32 type+0≥0
and less than the size of the catalog at Game+0x800724. Unlike the model
path, the collection code has **no selected-sprite-pointer null check**
and no type0x12 exclusion. The object draw helper0xbe760 subsequently
selects its resource from this distinct catalog. Do not import the
environment catalog or its extra rejection conditions. Source
0xb2c95–0xb2d17; size query0x87f50.

Likewise these are **Alpha static-object record/catalog indices**. Their
provenance is the Alpha World cell's list+0x0c and the separate Alpha
catalog+0x800724. Numeric object6 below is an Alpha ID; no Steam object6
correspondence or replacement asset mapping has been verified here.

Read the object's own float32 extents X/Y/Z at+0x24/+0x28/+0x2c.
Sphere radius is **float32(max(X,Y,Z)+1)**, where one is the float32
constant at0x345dc0. It does not use sprite-header dimensions or the
environment-model scale rule. Start the sphere point at fixed object
XYZ+0x08/+0x10/+0x18. Raise Z by the signed64 truncation of the result
of promoting extent Z to double, multiplying by double0.5, then by
double65536. Double half is at0x345de8; helper0x59c00 supplies the
double65536 multiply from0x2fd588 before truncating. Call §5.1 with
that point, radius and object L=float32(A×0.3). Source0xb2d17–0xb2df7.

After successful admission, type6 with nonzero byte+0x30 takes an
additional effect path at0xb2dfd–0xb3009 before continuing to the
ordinary distance decision. It is not a rejection and does not substitute
another point for the ordinary object's distance key. Exact rendering
of that additional effect remains outside this admission audit.

The object's subsequent D² is measured from its **raw fixed record
position**, with no half-extent elevation, as already specified in §2.
Consequently a raised object center can determine admission while its
raw position determines near/far routing and the mixed sort. It is
incorrect to share the model's raised-distance convention here.

## 8. Independent World-cell bounds and combined append order

The World is embedded at Game+0x2e4. This scan is centered on the
local-player pointer at Game+0x8006d0 and its physical fixed position
at player+0x10. It is **not centered on the camera** and does not iterate
the sorted render-cache vector.

For each physical fixed X/Y, first divide signed64 by65536 truncating
toward zero, then take the low32 bits as a signed32 block coordinate
through0x2f570. Convert that signed32 value to float32. To get each
minimum cell coordinate subtract A in float32, multiply by float32
1/256, then truncate to signed32; for each maximum use float32 addition
of A before the same multiply/truncation. Here A=float32(F+22.4), not
object L, model L or an integer ceil radius. The exact 1/256 constant is
at0x302a2c. Source0xb075c–0xb0827. Narrowing to the block integer occurs
**before** all float arithmetic, not after adding a fixed-point radius.

Scan increasing X as the outer loop, increasing Y as the inner loop,
including **both minimum and maximum**. Source0xb21d4–0xb2214 and
0xb329a–0xb32d4. Byte Game+0x8006e6 suppresses this entire scan when
nonzero. Each coordinate queries existing cells with0x34d10; it does
not create/load cells merely for this render pass. That query rejects
negative coordinates and coordinates≥65536, looks up the region for
X/64,Y/64, and returns null if the region or selected cell is absent.
Within a present region, the pointer array starts at+0x10018, with
four-byte entries indexed by **(X remainder64)×64+(Y remainder64)**.
Thus the scan order is X-major/Y-inner even though the render ring's
storage in §6 is Y-major. The queried cells cover256 blocks per axis.

For each present cell, the frame first handles its separate list+0x30,
then its static-object list+0x0c; list+0x30 must not be mistaken for the
cache's environment-model list or the object catalog. The ordinary
environment-model cache pass has already finished globally. Static
objects that pass §7 append to the shared far vector in World-cell scan
order and then object-list order. Once this scan ends, the mixed
descending sort and after-water pass in §§1–3 run.

### 8.1 Remaining bounded inputs and verification

This addition verifies the consumer's complete stored cache-window
bounds/order, readiness-transfer condition, active-data requirement,
resource/type exclusions and ordinary model/object range/frustum gates.
It does not newly trace who changes cache-window origin/count, the opaque
dependency key+0x70 and byte+0x74, scene-suppression byte+0x8006e6, or
the full auxiliary effects for model0x3c/object6. Those remain explicitly
separate producer/effect questions, not reasons to invent extra ordinary
admission predicates. Stored plane production and cache extrema/center
production also remain inputs to this bounded consumer contract.

No game was run. A bounded native draw/cache oracle could add requested
render-chunk coordinate, ring ordinal, stored coordinate tag, active and
pending group counts, dependency key/count, box-test range/result,
cache+0x6c, coarse horizontal distance, and per-record sphere point,
radius, range/result to §4's already named fields. For objects, retain
the physical-player-derived World-cell bounds separately from the
camera and the raised admission point separately from raw-key position.
