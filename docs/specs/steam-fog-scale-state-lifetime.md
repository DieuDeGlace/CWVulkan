# Steam WORLD fog-scale state lifetime

Independent native-only behavioral audit of the fog-range setter, construction
and directly reached ordinary scene, map and GUI callers. No engine, tests,
exporter, owner data or game execution was used. The pixel fog formula remains
[WORLD shader output §5](steam-world-shader-output.md#5-exact-ordinary-world-fog-curve).
Point-table state has a separate lifetime in
[point-table lifetime §§1–5](steam-point-table-state-lifetime.md).

## 1. Exact setter and shared storage

Renderer virtual+0x80 reaches **0xc59e0**, which adds **0x558** to the
renderer pointer and forwards to **0xc6bb0**. This is the same concrete
WORLD shader object used by ordinary world, overlay, map and the reached
direct GUI model paths. Relative to that shader object:

| Field | Meaning |
|---|---|
|+0x3d4|CPU float32 comparison cache of the reciprocal fog range|
|+0x448|Pointer to the16-byte GPU fog-scale constant buffer|

Binding **0xc68c0** installs this buffer at **pixel constant-buffer slot1**.
Binding the buffer or selecting another draw pipeline does not change its
contents. Projection/view/WORLD changes, material setters and point-table
clear0xce580 likewise do not reset this fog cache or buffer.

The input to0xc6bb0 is a **float32 distance F**, not a reciprocal. The
native comparison at **0xc6bc9–0xc6bcc** admits only ordered **F>0**:

* Positive finite F is admitted.
* Positive infinity is admitted.
* Positive zero, negative zero, negative finite F and negative infinity
  return without changing the CPU cache or GPU buffer.
* A NaN input is unordered and follows that same no-change return.

For an admitted input, **0xc6bd6** performs a scalar float32 division of
literal **1.0** by F. This is the stored1.0 word0x3f800000 at **0x4ac980**,
not a double reciprocal followed by narrowing. The resulting float scale
is compared with shader+0x3d4 at **0xc6bda–0xc6be3**. An ordered equal
comparison returns without uploading; an unequal or unordered comparison
updates the cache and submits **float4(scale,0,0,0)** at **0xc6c2f**.
The reciprocal itself is not clamped or tested again for positivity.

In particular, **F=positive infinity produces scale=positive zero** and
can upload that zero. A nonpositive F does not request zero. Positive
finite inputs whose native division overflows may submit positive infinity;
no additional finite-range fallback appears in this setter. This describes
the reached scalar operations, without assigning alternate floating-point
environment settings or exception handling.

## 2. Construction and scene-range reset

Shader construction **0xc5ac0 →0xc60d0** creates the16-byte buffer at
**0xc647e–0xc6496**, with **no initial-data pointer**. After creation,
**0xc6514 →0xc6550** initializes the CPU cache+0x3d4 to **-1.0**, word
0xbf800000. That cache initialization does not upload a fog value.

Consequently this constructor trace proves neither an initial zero GPU
scale nor an initial GPU scale of-1. No initial GPU-data oracle was
obtained. Until an actual admitted setter establishes contents, their
numeric value remains unproved. For a freshly initialized cache, the
first admitted positive input's nonnegative reciprocal differs from-1
and therefore reaches the upload, assuming the ordinary valid resource
and update path.

The adaptive scene distance is a different field, **renderer+0x4a4**.
Its base constructor0xcbd10 initializes it to0. Explicit scene-range
reset **0xe8a30**, reached on the documented world-identity change, writes
that renderer field to0; it does **not** write shader+0x3d4, recreate the
fog buffer or call the fog setter. The next ordinary scene still applies
its normal range producer and setter admission. A resulting F=0 leaves
the prior shader scale intact rather than resetting it. See
[scene lighting §6.2](steam-scene-lighting.md#62-meaning-persistence-reset-and-derived-ranges).

## 3. Ordinary scene, sky, stars and mandatory overlay setup

The meaningful reached order inside **0xe2e00** is:

1. After adaptive range update and normal frame setup, **0xe3a40** supplies
   the current float32 **renderer+0x4a4** to the fog setter. With effective
   camera block type **2**, its preceding selection uses **150.0**
   (word0x43160000 at **0x42e938**) when the range is at least150; otherwise
   it passes the range. For ordinary finite inputs this is **min(F,150)**.
   The native ordered-comparison branches pass a NaN range onward as NaN,
   which the setter rejects. Positive infinity selects150 in this
   underwater branch; outside it, positive infinity reaches the setter
   unchanged and produces zero scale.
2. The optional early sky quad **0xe3ad4 →0xc5120** does **not** supply a
   fog range. Its independently documented indeterminate point-table
   submission does not reset this fog buffer. Main terrain, queued world
   models, effects, particles and water do not implicitly reset fog merely
   by selecting their pipelines or binding their other constants.
3. In the reached optional night/far-flight star branch, **0xe3d99** supplies
   **100000.0**, word0x47c35000 at **0x43800c**. This is after the ordinary
   world/water work, within the sky-enabled branch; an unentered star
   branch does not perform this write. The existing sky/cloud ordering
   and night/far-flight admission remain [sky B.2](sky-and-clouds.md#b2-order-of-the-sky-draws-frame-0x1400e2e00).
4. The late screen-overlay preparation **unconditionally supplies10000.0
   at0xe4e86**, word0x461c4000 at **0x42e5e8**. This precedes the point
   clear0xe4e8f and mode1 overlay consumers0xe4f10/0xe4f22, then the GUI
   traversal. It is not gated by a nonempty or ready overlay model queue.

Thus a completed ordinary scene reaches a known positive-range setter
for its late overlay even if its earlier world-range call was rejected.
That late setting is not automatically restored to the earlier world
range afterward.

## 4. Reached map, card and direct GUI setters

These callbacks use the same renderer+0x558 shader cache and GPU buffer
from§1. Their setters describe **reached setup**, not an instruction to
run them for hidden, skipped or early-returning callbacks.

| Caller | Fog input and reached order |
|---|---|
|3D map0xe64b0|**0xe66ac** supplies **20000.0**, word0x469c4000 at **0x42e5ec**, after its point clear and before the map-context opaque/faded consumers|
|Character-card preview0xdb630|Its admitted local preview setup supplies **10000.0**, before its explicit point clear and local resource-tree drawing|
|Shared GUI item0xddbc0|Its admitted image setup supplies **10000.0** before the optional dark-offset pass; the main colored pass's point clear occurs later|
|Raw GUI sprite0xdf230|Its reached main model setup supplies **10000.0**, immediately before its explicit point clear|

**Map cut:**0xe64b0 first prepares/submits the map context through0x2f44a0.
Its20000 setter is after that preparation but before resource-tree/vector
consumption at0xe67df/0xe67f1. There is no intervening empty-context or
resource-readiness condition around the setter. An empty prepared context
or exclusively undrawable queued resources therefore still reaches it.

**Card cut:** the admitted **nonnull-character** own callback0x26eba0
calls0xdb630 at0x26f22e. After its local pose preparation,0xdb630 reaches
the10000 setup without testing that the pose produced an opaque part or
that any queued part is GPU-ready. An empty local context, parts confined
to its unconsumed faded queue, or later opaque-resource readiness failures
do not skip that setter. The null New character card does not call0xdb630.
Ancestor visibility/cache/callback admission remains the
[card contract §§1–2](steam-character-card-preview.md#1-record-and-callback-ownership).

**Shared item cut:**0xddbc0 does not reach its setter for a null item
pointer or a null chosen model resource. For category0, its special
fallback additionally returns when the catalog has fewer than **0xcd2**
entries; otherwise it selects catalog slot **0xcd1**, still requiring a
nonnull resource. Other categories use the returned0x173b10 resource.
Once the nonnull selected resource enters image setup, the10000 setter
precedes both optional dark and main draw attempts. No successful GPU
geometry draw is required for it. Thus a valid resource header with no
drawable buffers can change fog while producing no image.

**Raw sprite cut:**0xdf230's null resource skips its whole image setup.
A nonnull resource reaches its10000 setter before the direct draw attempt;
later absence of drawable buffers/pixels does not retrospectively suppress
that write. These cuts do not define recovery for invalid resource pointers.

Those calls do not restore the previous fog range or reciprocal on return.
An unentered callback or a return preceding its setter preserves whatever
fog state was already present; resource/draw readiness must not be treated
as proof that every earlier setup write was skipped. Plasma pipeline
restoration is not restoration of this WORLD shader's old fog constant.

At entry to a subsequent ordinary scene, the last actually reached shared
fog setter determines the incoming numeric state. Its rejected F<=0 or
NaN world-range call preserves that state, which may come from a late
10000 overlay, a20000 map, or a later admitted GUI model setup. It does
not necessarily retain the last normal-world range, and does not select
zero merely because the adaptive range was reset to0.

## 5. Correction and limits

This audit confirms WORLD shader output§5's nonpositive-input retention
and **supersedes the former pass-controls§1 phrase “otherwise0.”** The
zero result from positive infinity is a distinct admitted reciprocal
calculation, not evidence for a nonpositive-input zeroing branch.

No source-independent numeric initial GPU value, failed-update behavior,
concurrent access protocol, untraced alternate renderer/callback path or
device-recreation contents is established here. The listed callers and
their reached ordering do not assert an exhaustive census of all future
or external shader users. Their shared state and no-restore behavior are
separate from the fog pixel formula and from point-light collection.
