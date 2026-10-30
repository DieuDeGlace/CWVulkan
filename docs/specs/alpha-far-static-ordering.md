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

Farther environment models and static objects are instead appended to
**one shared deferred vector**, then sorted at **0xb332c through0xabac0**.
The vector is not split into “all objects” and “all models”. After the
water traversal ends, the frame explicitly restores depth writes on,
clockwise culling and the WORLD pixel shader at **0xba432–0xba462**.
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

After their existing type/resource and visibility admission, compare stored
float squared distance D² with **float32(B timesB)**. **D²≤B² draws
immediately**; **D²>B² defers**. Boundaries are at
**0xb2754–0xb276a** for models and **0xb303e–0xb3051** for objects.
The upper-range/resource/frustum admission remains the existing source
route; this rule does not authorize drawing every record outside L.

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
| +0xc | Model's owning zone pointer, used by that renderer; not a meaningful required field for an object-only record |

Initializer0xabc00 clears the first two pointers. Append0xc1190 copies all
16 bytes; model append is at0xb27b1, object append at0xb308d. Within a
valid ordinary record exactly one pointer is nonzero. The dispatch tests
model pointer first, then object pointer, but that does not group separate
records by kind. Preserve source append order before sorting. The late
loop reads the owning-zone field only for a nonzero model pointer.

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
