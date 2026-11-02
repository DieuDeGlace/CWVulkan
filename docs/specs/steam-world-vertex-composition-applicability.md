# Steam WORLD vertex composition: reached draw families

Independent native-only applicability audit of the already traced shader/state
constructors, matrix setters and reached draw callers. No engine, tests,
exporter, owner data or game execution was used. Exact compiled composition,
local-position dots and normal-space arithmetic remain
[map contract §6](steam-map-world-pixel-contract.md#6-map-light-vector-and-world-vertex-precision).
This establishes where that same bytecode is bound, preserving each family's
separate geometry, controls and admission contract.

## 1. Same compiled vertex shader across reached families

Construction **0xc60d0** creates the WORLD vertex shader from embedded
blob **0x4387b0**, length **0x1860**, storing its object pointer at
**shader+0x08**. The reached shader object begins at **renderer+0x558**.

|Reached family|State/binding route|Vertex identity and pixel distinction|
|---|---|---|
|Ordinary terrain0xe5070|state0 through0xc3dc0→0xc5de0|WORLD VS+0x08 and ordinary WORLD PS+0x10, blob0x43a010|
|Ordinary queued models0xe6e60/0xe7980|states0/1 and3/4/6/7 through0xc3dc0→0xc5de0|Same WORLD VS and ordinary WORLD PS; depth-only states do not replace the VS|
|Native water0xe85e0 depth|state5 through0xc3dc0→0xc5de0|Same WORLD VS and ordinary WORLD PS, under the established no-color depth state|
|Native water0xe85e0 color|state9 through0xc3dc0→0xc5ff0|Same WORLD VS+0x08; only the PS changes to+0x28, native water blob0x43b790|
|Documented direct GUI item0xddbc0 and raw sprite0xdf230|Their reached callers select state0→0xc5de0|Same WORLD VS and ordinary WORLD PS; the helpers do not select a different VS themselves|
|Character-card helper0xdb630|Its local opaque queued consumer uses0xe6e60|Same WORLD VS, including the general consumer's explicit winding-family state choice|

These bindings use the same input layout and **0xc68c0** constant-buffer
installation. Native water's separate pixel formula does not imply a
different vertex transform. Direct GUI projections, dark/color choices
and card clipping likewise do not imply another VS under these states.

## 2. Independent matrix inputs and the triple setter

Relative to shader object S, the reached routes are:

|Input|Renderer virtual / bridge / setter|CPU float cache|GPU buffer / VS block|
|---|---|---|---|
|WORLD|+0x50 /0xc5ab0 /0xc7ab0|S+0x68|S+0x400 /b2|
|VIEW|+0x58 /0xc5a90 /0xc78b0|S+0xa8|S+0x3f8 /b1|
|Projection|+0x60 /0xc5a50 /0xc7240|S+0xe8|S+0x3f0 /b0|

Each setter handles its own **16 float32 components**, without computing
WORLD×VIEW or WORLD×VIEW×projection. Its existing same-cache-address
check can skip copy/upload. Otherwise it copies the supplied matrix and
submits its transpose: input indices **0,4,8,12**, then **1,5,9,13**,
**2,6,10,14**, **3,7,11,15**. See
[GPU coordinates §3](steam-world-gpu-coordinates.md#3-queue-composition-and-separate-gpu-matrix-uploads).

**Renderer virtual+0x68 →0xc5a30 →0xc6fc0** is a **three-input setter**,
not a precombined-WVP interface. Native instructions call **0xc7ab0 with
the WORLD pointer**, then **0xc78b0 with the VIEW pointer**, then
**0xc7240 with the projection pointer**. No matrix multiplication occurs
between those calls.

Direct item dark/main passes and raw GUI sprites use this triple bridge
with separate matrices. Character-card setup/restoration uses the same
bridges. A caller can compose an object's WORLD from pose transforms or
construct a shifted GUI projection before submission; that remains one
matrix in its corresponding slot. It does not move cross-WORLD/VIEW/
projection composition into these CPU setters.

## 3. Unchanged compiled composition and DepthOffset

The identical VS in§1 applies the compiled operations of map contract§6:

* The geometric chain is **local → WORLD → VIEW → projection**, while
  the shader constructs WORLD×VIEW and full projection coefficients
  before the relevant local-position dot products.
* View position uses the decoded local point and composed WORLD×VIEW.
  Clip position uses that local point and the composed full matrix.
  WORLD position for point-light distances is evaluated separately.
* Local homogeneous W is **1**. **DepthOffset is subtracted from the
  homogeneous/translation coefficient of composite clip Z before the
  final position dot**. It gives the established clip-Z offset for that
  input without changing view position or either normal-space input.
* Lighting uses the separately normalized WORLD upper3×3 normal; the
  view-normal varying uses the independent WORLD×VIEW path. Neither is
  an inverse-transpose construction.

No family-specific branch in this compiled VS replaces those operations
for terrain, water or GUI. This establishes applicability, not equality
of their input matrices, DepthOffset, material/effects, vertex contents or
visible outcomes. GPU dot/multiply-add/reciprocal-square-root precision
retains map§6's limits. A sequential CPU vector path or CPU double
precomposition is not proved bit-identical.

## 4. Water retains its own input timing

The depth traversal in **0xe85e0** uses **incoming shader VIEW and
projection**. Before its color traversal it selects state9 and explicitly
reinstalls **renderer+0x39c VIEW and+0x3dc projection**. Both traversals
submit their ordinary per-group WORLD translations through virtual+0x50.
They are separate inputs to the same VS, not proof that both passes must
always have identical incoming matrix contents.

Water does **not** set DepthOffset in this helper. The value retained
from earlier drawing therefore reaches the same coefficient operation
in§3. Its material, point-table, Opacity and other state rules remain
[WORLD pass controls §3](steam-world-pass-controls.md#3-water-controls-and-retained-values).
Shader identity does not supply the map's0.5 or another family's usual0
as a replacement for those retained inputs.

## 5. Bounded exclusions

No alternate precombined-WVP upload path was found in these reached
setters/bindings. This is not an exhaustive all-renderer/caller census.
Separate beam and special procedural shader paths, Plasma drawing,
untraced alternate callers, device recreation and invalid-resource
recovery remain outside scope. Procedurally generated **voxel resources
that use the WORLD bindings in§1** are not excluded merely because their
data was generated rather than file-backed.

The direct GUI result is conditional on the documented caller selecting
the reached WORLD state; these helpers do not force that state for an
arbitrary caller. Family-specific input construction, readiness, clipping,
queue ordering and style producers remain separate.
