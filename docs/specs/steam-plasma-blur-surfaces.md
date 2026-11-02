# Steam Plasma blur traversal and concrete surface behavior

Independent native-only bounded contract from traversal **0x330b20**, bounds
**0x332ba0/0x332fd0**, layer push/pop **0x33c110/0x33e1e0**, projection
**0x38ed10/0x390050**, concrete surface vtable **0x480718**, surface
**0x392610/0x392200/0x392280/0x3925c0**, and GUI entry **0x33e170**.
No engine, tests, exporter, owner captures or game execution was used.
This preserves the reached concrete backend rather than inferring a filter
from generic virtual-method names. Cache policy remains
[surface caches](steam-plasma-surface-caches.md); sampler provenance and
conditional public API defaults remain [clip layers §6.1](steam-plasma-clip-layers.md#61-bounded-native-provenance-versus-the-public-null-sampler-default).

## 1. Admission and projected radius

Visibility0 and disabled node flag4 reject drawing before this branch.
Blur requires **engine+0x288 bit0x04**, **live Display.flags bit0x01**,
an ordered positive live blur-radius value, and **traversal flag0x08 clear**.
Children receive the traversal flags with0x08 removed, so disabling blur
for this node does not permanently disable blur in descendants.

A selected cache hit bypasses the actual blur/shape/child/own-callback
work as already specified. A cache redraw can surround this branch.
The following contract assumes the admitted branch runs, valid nonnull
surfaces, valid bounds and ordinary finite intermediate values.

At **0x330f7b–0x331009**, the routine obtains the current GUI matrix and,
when an own shape exists, allows that shape's virtual+0x70 operation to
adjust the matrix. Call the resulting float matrix M, indexed in the
existing16-component CPU storage. The blur radius calculation is separate
from the bounds helper's own expansion rules:

* Square **M[5] and M[1]**, add those float32 squares, then call sqrtf.
* Square **M[4] and M[0]**, add those float32 squares, then call sqrtf.
* Add the first root to the second in float32, multiply by **0.5**, then
  multiply by the live radius and divide by stored float32 **1.399999976158142**,
  word **0x3fb33333 at0x4328bc**. Call the result r.

The scalar instructions are **0x331020–0x33109b**. Their matrix-component
pairs must not be exchanged for another convenient row/column norm rule.
Set the generic pass radius c to r and reduction q to1. Only when **r>8**,
set c to stored float **8.0**, word **0x41000000 at0x4aca30**, and q to
the float division **8/r**. The positive live-radius test does not itself
prove r>0 for a degenerate matrix; invalid/degenerate output behavior is
outside the ordinary finite nondegenerate scope.

Traversal flag1 retains its own shape-bounds test0x3339c0. With blur and
no clipping, the branch can still run when that own-shape test rejects;
it omits the own-shape draw but can process children. With simultaneous
clipping and that rejection, the layer work is skipped. Own callback and
cache admission retain their separate existing rules.

## 2. Bounds, allocation and reduction projection

The branch obtains screen bounds from **0x332ba0**, then subtracts r from
both minimum components and adds r to both maximum components. Call this
expanded minimum L and its float width/height E. These are live geometry
queries, not a rounded Widget frame. The helper already applies its own
Display blur/shape/child bounds rules through0x332fd0; this extra r expansion
must not be assumed the only blur-related growth or replaced by raw shape
bounds. Per-shape override/bounds behavior remains an input to that helper.

Outer push **0x33116e →0x33c110** passes **q** to the layer/projection
wrapper. The underlying pool surface A still has its fullscreen allocation
and full-surface viewport; **q does not resize its texture**. The ordinary
clip pool/storage/resize limits remain [clip layers §§1–2](steam-plasma-clip-layers.md).

For this push, the layer rectangle is origin0 and current GUI width/height.
The true-flag branch of **0x38ed10** saves projection, binds/clears the
surface, and builds the reduced projection as follows:

* From current GUI WORLD translation X/Y, multiply by q and subtract each
  product's signed32 truncation converted back to float. If q equals1,
  use0 for both fractional corrections.
* Compute float32 **(1−q)×0.5**, then multiply that factor by each
  rectangle dimension. Subtract this result from the corresponding near
  edge; its far edge remains one full dimension beyond that near edge.
* Build the ordinary orthographic projection for those X/Y edges,
  with +Y downward, near **−1** and far **10000000** (word0x4b189680
  at0x4804f0). Translate by the negative fractional corrections, then
  scale the X/Y basis by q.

These are float32 construction operations, not allocation dimensions.
For ordinary affine points they center the reduced drawing in a full-sized
target, with the specified fractional registration correction. They are
not evidence for a smaller render target or a different sample count.

Generic bounds-operation requests extend L by a further−2r and E by
**2+4r**; concrete surface virtual+0x40 is a **no-op**, so these requests
do not partially clear or resize a target. Every actual surface begin
clears its full target as specified below.

## 3. Initial shape/child composition and workspace

With blur alone, the admitted own shape is drawn into A, then children
are visited in list order. With simultaneous clipping, **0x331215** first
pushes another pool layer with scale **1**, draws children there, pops it,
then draws the own shape/mask into outer A. Thus the inner clip is not
silently given q as its projection scale. Nested child blur follows its
own recursive traversal.

The engine's separate workspace surface B is at **engine+0x68**. The
first use creates it through the same concrete factory and requests
current GUI width/height. On later GUI entries, **0x33e170** resizes an
existing B to current width/height before root drawing. Engine destruction
0x33a440 destroys it. This is distinct from the retained pool A entries,
whose broader resize/reset provenance remains the clip contract's limit.

A and B use fullscreen **single-sample R8G8B8A8_UNORM** storage, null
depth on begin, and the ordinary target/viewport save/restore. There is
no separate small-radius or reduced-resolution texture allocation here.
Direct B/A surface binds during the following copies do not independently
rebuild the GUI projection; the outer reduced projection remains current
until the engine's corresponding layer pop.

## 4. Four generic passes and the actual copy operation

After preparing A, the routine ends A's direct surface binding, restoring
the parent target while retaining the outer layer's projection state.
It begins B, makes another no-op bounds request, then ends B. This begin
still clears B fully, even before the actual ping-pong copies.

The loop has two outer iterations, divisors **2 then1**. In each iteration
it performs **A→B, then B→A**. The generic source operation is surface
virtual **+0x30**, with destination L, size E, nominal radius **c/divisor**,
an alternating direction boolean, and reduction q. Every destination is
begun before its copy and ended afterward. Thus there are **four copy
attempts**, with a full destination clear on every begin; they are not
four additive draws into uncleared images.

Concrete vtable0x480718 maps this operation to **0x3925c0**. That wrapper
discards the nominal radius, direction and reduction arguments, replaces
source origin with **(0,0)** and copy scale with **1**, then calls ordinary
**0x392280**. Destination L and size E still pass through. It does not
select a Gaussian shader or supply radius-dependent sample offsets/weights.

The reached copy restores the Plasma pipeline, enables t1 object texturing,
sets mask0 and all texture adjustment parameters to1, disables Widget
deformation, and uses the current GUI projection. Its color is the source
surface RGBA, normally initialized white. UV origin is0; UV extent is
E divided by the source's actual texture dimensions. It uses the ordinary
Plasma sample/color/blend formula in [clip layers §§4–7](steam-plasma-clip-layers.md).
The inherited sampler, not a name such as horizontal/vertical blur,
determines interpolation/addressing. The four passes retain the outer
reduced projection; wrapper substitution of copy scale1 does not undo it.

## 5. Final rebind clear and final source selection

After all four copies, **0x331476–0x331486** obtains the saved active pool
surface **A** and begins it again through virtual+0x10. In this concrete
backend **0x392610 unconditionally clears its full color target to
transparent black**. No bypass of that clear based on blur direction,
surface byte+8 or “restore previous binding” intent appears in that method.
There is no subsequent shape or ping-pong draw restoring A's copied image
before the next pop.

The call **0x331497 →0x33e1e0** then ends A, restores the parent target/
viewport and the saved GUI projection, retains A as mask source and
decrements the layer index. Final composition uses **A, not B**.
It calls ordinary surface virtual+0x28 at **0x3314e0**, with destination
**L**, destination size **E+2** componentwise, source origin **L** and
copy scale **q**. There is no integer rounding of those bounds in this
blur path.

For that positive q copy,0x392280 calculates each fractional correction
from the current GUI WORLD translation times q, minus its signed32
truncation; q=1 forces corrections0. Before normalization by actual
texture dimensions, compute the center factor **(1−q)×0.5**, multiply by
the texture dimension, then subtract the fractional correction from that
center term. Add that difference to **q×sourceOrigin**, then divide by
the texture dimension. Each UV extent is the requested destination
dimension times q, then divided by its texture dimension. Preserve these
float32 multiplication/subtraction/addition boundaries. The destination
rectangle still uses its requested L/E+2; q controls source registration/
extent. The helper temporarily uses identity GUI WORLD for the rectangle,
keeps the restored parent projection, and uses ordinary scale-copy color,
texture parameters, blend and position-bias behavior.

The cleared source contents are therefore an explicit native result,
not a retained four-pass blur image. When the final texture sample is
transparent black, texture opacity1/mask0 yields transparent black before
blending. This does **not** assert blanket invisible pixels for every
possible out-of-range sample, border/sampler state, invalid resource or
external writer. It also does not erase the own Widget hook: its earlier/
later callback positions are reached after this blur composition and can
draw independently into the restored target (or a surrounding cache).

## 6. Bounded result and unresolved inputs

This closes the reached pass count, allocation-versus-projection distinction,
generic arguments versus concrete copy consumption, final clear and selected
source. It does not authorize a replacement Gaussian, separable filter or
assumed radius-weighted averaging. The generic algorithms and this backend's
ordinary rectangle copies are different factual layers.

The supplied GUI matrix after any shape adjustment, valid screen bounds,
broader engine-option/radius producers, sampler bindings, external pool
resize/reset paths, invalid/degenerate/nonfinite cases and exact GPU
sampling/rounding remain explicit inputs or limits. Cache-hit/redraw and
ownership behavior remain the cache contract. No original game was executed
to replace these limits with an appearance claim.
