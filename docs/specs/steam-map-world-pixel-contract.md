# Steam 3D map: shared WORLD pixel contract

Independent native-only audit of **0xe64b0**, its queued consumers and
shader/state construction. No engine, tests, exporter, owner data or game
execution was used. This is a bounded correction to the older pixel
paraphrase in [worldmap rendering §3.3](worldmap-rendering.md#33-pixel-shader-0x43a010),
not a new map shader or a replacement for map model/camera generation.

## 1. Actual shader identity and passes

Shader construction **0xc60d0** creates the ordinary WORLD pixel shader
from the embedded blob at **0x43a010**, storing its object pointer at
shader+0x10. The corresponding WORLD vertex blob is **0x4387b0**, stored
at shader+0x08. Map rendering uses the same shader object at
**renderer+0x558**.

Map **0xe64b0** consumes its context at **renderer+0x268** through
**0xe6e60 /0xe7980**, with mode0. Its ordinary normal-family passes select
**state0** for opaque drawing and **states3/6** for faded depth/color.
State binding **0xc3dc0** routes each of those three states through
**0xc5de0**. That routine binds exactly the WORLD vertex/pixel objects
above and the shared constant buffers through **0xc68c0**. State3's
depth-only color-mask behavior does not select a different pixel shader.

Thus the map color passes use the full verified WORLD shader contract,
including [vertex/interface behavior §§1–2](steam-world-shader-output.md#1-vertex-outputs-and-raster-interpolation)
and [pixel stages §§3–6](steam-world-shader-output.md#3-pixel-base-color-and-the-optional-shininess-ripple).
Queue modes, culling, depth and blend states remain the existing
[pass-family contract](steam-retained-item-and-map-pass-family.md#4-native-consumer-conditions-and-pipeline-states).

## 2. Map preset versus per-descriptor and inherited inputs

After map descriptor preparation through0x2f44a0, **0xe64b0** submits:

* Material RGBA **(1,1,1,1)**.
* Front RGBA **(0.75,0.7,0.3,1)**, back **(0.5,0.5,0.5,1)**, ambient
  **(0.25,0.3,0.7,1)**, and the normalized direction of **(0.2,0.5,1)**.
  The map setup normalizes its direction, and the directional-light setter
  **0xc6ce0** normalizes the supplied direction again. These are map vertex
  lighting inputs, not a replacement pixel sky palette.
* Explicit zero point tables through **0xce580**, then fog distance
  **20000.0 at0xe66ac** through **0xc59e0 →0xc6bb0**.
* The prepared map view **map+0x58** and projection **map+0x18**, followed
  by the map context's opaque/faded consumers at **0xe67df/0xe67f1**.

The exact range/cache/setup admission remains
[fog lifetime §§1/4](steam-fog-scale-state-lifetime.md). The positive map
distance supplies the float32 reciprocal of20000; it is not permission
to omit fog. Mode0 clears points without selecting world-zone tables.

**For opaque draws and faded color passes, each consumed descriptor still
submits its own material, Opacity, Glow, Shininess, White and DepthOffset.**
The separate faded depth-only pass does not constitute another color result
or imply that it repeats all those writes. Its state/control order remains
the queued-consumer contract. The initial white material is therefore
not an override of every subsequent color record. Ordinary region-model records
from0x2f44a0 submit **white material RGBA, region fade as separate Opacity,
Glow0, Shininess0, White0 and DepthOffset0.5**. This statement is specific
to region records; creature, tombstone and border/marker descriptors retain
their independently specified producer values. Their different effects do
not change the pixel-shader identity in§1.

Map0xe64b0 does **not** supply replacements for these pixel buffers:

| Shared pixel buffer | Retained input |
|---|---|
|b0|Raw Sky1, Sky2, Fog and Sun RGBA|
|b2|Fog-gradient translation|
|b3|SunPos, including aspect and visibility|

They remain the values present on entry, normally installed by the main
scene's verified setters. Installing map view/projection does not project
the sun again or recompute the gradient. The map shader uses its map
fragment coordinates with those retained inputs. No already graded sky
image is sampled as a substitute for the raw palette reconstruction.

## 3. Exact shared pixel stages

The operation order and constants are WORLD shader output§§3–6. In
particular, the older map paraphrase must not substitute natural-base exp
or drop the second fog blend:

1. Begin with interpolated vertex RGB, including the earlier vertex
   White clamp. Apply the optional Shininess ripple when its scalar is
   positive, then the pixel saturation gain based on RGB luminance.
2. Obtain squared distance from interpolated **map view-space XYZ**.
   Multiply by the square of the submitted fog scale to obtain the
   dimensionless distance input. The fog exponent is **one minus0.75
   times the base-two exponential of negative403.9546203613281 times
   fogScale**. The coefficient is the embedded float word **0xc3c9fa31**.
3. The native power uses **log2 of the distance input, multiplication by
   that exponent, then exp2**, with the result limited to at most1.
   Apply the cubic smooth function to obtain the fog blend weight s.
4. Blend geometry toward retained **Fog** by s. Then blend that result
   toward the **full ungraded sky-with-sun target** by **s to the fourth
   power**. That target includes both raw sky gradients, the retained Fog
   contribution and the SunPos-dependent sun halo. Clamp RGB only after
   this second blend.
5. From that clamped RGB, compute luminance once. Keep red, mix green and
   blue with that luminance using **0.65/0.35**, then apply **3c²−2c³**
   componentwise. Output alpha is the descriptor's separate Opacity
   constant, not interpolated vertex alpha or material alpha.

There is no map-specific branch that discards SkySun, bypasses final
grading, or replaces exp2 by exp. A particular map point can have a small
fourth-power sky weight, but that numerical observation is not a general
zero-sky contract. It depends on the actual map-view position and the
inherited raw pixel inputs. Glow/White affect their documented earlier
stages; they do not bypass the shared fog and grading stages.

## 4. Bounds

This establishes the reached map shader objects, setter/non-setter
distinction and full pixel-stage identity. It does not add an initial
numeric value for unproved shared GPU storage, assert admission of a
skipped map callback, replace marker-specific producers, or assign exact
GPU log2/exp2/interpolation rounding beyond the compiled operations.
Render-target conversion and blending remain subsequent stages of the
existing WORLD output/pass contracts.

## 5. Mode0 resource ordering, readiness and empty geometry

This bounded extension applies [queued WORLD state §§1–2](steam-queued-world-state-order.md#1-exact-opaque-key-and-instance-ordering)
to map **0xe64b0 →0xe6e60/0xe7980**. Mode0 changes point-table handling,
not resource ordering or readiness cuts. The context is **renderer+0x268**.

### 5.1 Groups and winding families

Opaque grouping from **0xee2f0/0xee990** uses the resource object's pointer
as an **unsigned64 numeric key**. Consumers traverse groups in ascending
native address order and descriptors within a group in **FIFO enqueue
order**. They do not reorder those descriptors by distance, material,
catalog ID or backing-buffer address. Normal and opposite families are
separate: normal groups precede opposite groups regardless of their relative
pointer keys. Grouping does not span separate renderer contexts.

All seven ordinary map model callsites in **0x2f44a0** use0xee2f0 and
populate the **normal family only**, as established in
[pass-family §5](steam-retained-item-and-map-pass-family.md#5-what-the-ordinary-map-actually-populates).
The ordinary map therefore reaches normal opaque **state0**, followed by
normal faded **depth3/color6** if its faded vector is nonempty. The general
consumers retain their opposite-family support; mode0 does not convert
those descriptors to normal winding.

Native addresses and resource alias relationships are inputs. Catalog
entries yielding the same resource pointer share a group; distinct resource
objects remain distinct even if their geometry agrees. **No portable
catalog order or catalog-to-native-heap equivalence is established.** A
catalog ID alone cannot determine traversal or terminal descriptor state.

### 5.2 Opaque readiness before descriptor writes

**0xe6e60** always selects **state0** on entry. In mode0 it calls the
zero-point helper **0xce580 at0xe6ecf before inspecting the normal tree
or resource readiness**. An empty tree therefore still clears points.
Mode0 disables per-descriptor world-zone point-table selection.

For each encountered resource group, **0x16eba0 is tested once**, before
checking/visiting that group's descriptor list. It rejects a missing buffer
pointer **resource+0x70 or+0x78**, or a false readiness result from either
buffer's virtual+0x08 method. A failed group supplies **none of its
per-descriptor WORLD, material or scalar writes**. A ready group with an
empty descriptor list supplies none either. Neither case undoes the common
point clear or the already reached map setup.

Normal-tree traversal uses its begin/head relationship. The separate
opposite traversal and state1 selection require **opposite opaque
count+0x18 nonzero**, then apply the same group test and ordering. These
are valid-container facts, not recovery for inconsistent counts/links.

**0x16eba0 does not test resource vertex/triangle counts+0x80/+0x84.**
A valid header with ready buffers and zero geometry counts can still reach
descriptor setters and a draw attempt with those counts. Zero counts and
lack of pixels are therefore distinct from resource-readiness failure.
Ordinary valid header/resource lifetimes are assumed; invalid-pointer
recovery is not specified.

### 5.3 Faded setters before the draw's readiness test

In **0xe7980**, each vector's **begin!=end** condition gates its pass.
The general order is **normal depth3, opposite depth4, normal color6,
opposite color7**. Every visited vector retains append order; faded
records are not grouped or sorted by resource key. The ordinary map's
empty opposite collections leave only its normal passes.

Depth records submit **WORLD and DepthOffset**, then call **0x16ec60**.
Color records submit **WORLD, material, Shininess, Glow, Opacity, White,
DepthOffset**, then call0x16ec60. Mode0 performs no world-zone point
selection. The depth pass does not repeat all color constants or produce
an additional color result.

Only inside **0x16ec60**, after those setters, does the draw check
resource+0x70/+0x78 and both buffers' readiness. A valid-header faded
record with unavailable buffers therefore still leaves its reached
WORLD/scalar/material writes, despite producing no pixels. Zero draw
counts or hardware clipping likewise do not undo setters. Both vectors
empty means no faded pass/state/record writes, while the earlier mode0
opaque-entry point clear remains.

### 5.4 Map preset before empty or failed consumers

The preset in§2, including material, directional/ambient lighting,
explicit point clear, **F20000**, map view and projection, precedes
**0xe67df/0xe67f1**. An empty prepared context, opaque readiness failures
or undrawable faded records do not retroactively skip it. Opaque failures
preserve its reached material/scalar state; admitted faded records can
supersede corresponding constants before failing their draw readiness.
Neither route restores incoming pre-map constants.

These facts distinguish a skipped callback, valid empty context, failed
opaque group and faded record whose draw fails after setters. They do not
supply native addresses from catalog metadata, guarantee concurrent resource
stability, or define device/invalid-memory recovery.

## 6. Map light-vector and WORLD vertex precision

This bounded precision extension uses the reached CPU instructions in
**0xe64b0 /0xc6ce0** and compiled WORLD vertex blob **0x4387b0**. It
distinguishes instruction-derived arithmetic references from runtime
captures; no runtime oracle was obtained for the numbers below.

### 6.1 CPU direction production and two normalizations

At **0xe6613–0xe661b**, the map loads stored float32
**1.2899999618530273**, word **0x3fa51eb8 at0x43ef80**, then calls the
imported **sqrtf**. The stored value is the squared-length input, not an
already stored square root. The call uses the float square-root import
at0x395462, distinct from the double sqrt import at0x39545c.

It divides float32 **1.0** by the returned float. From this reciprocal,
it forms X by a separate float multiplication with **0.20000000298023224**
(word **0x3e4ccccd at0x42e700**), Y by a multiplication with **0.5**
(**0x3f000000 at0x4ac964**), and Z by retaining the reciprocal itself.
These scalar float32 operations are at **0xe6620–0xe6661**. This path
does not first calculate the squared length from three freshly assembled
components or use a double-precision normalized direction.

The call at **0xe6690** supplies that vector to renderer virtual+0xc0,
reaching **0xc6ce0**. At **0xc6d16–0xc6d84**, this setter squares X, Y
and Z separately in float32, adds **X²+Y² first, then Z²**, calls sqrtf,
divides float1 by that result, and multiplies each original component by
the resulting float reciprocal. It caches/submits that second normalized
vector as the directional-light input. Normalizing only once is not the
reached CPU operation sequence.

Under ordinary round-to-nearest float32 arithmetic, the instruction-derived
reference is:

| Value | Float32 value | Word |
|---|---|---|
|First reciprocal / first Z|0.8804509043693542|0x3f61653b|
|First X|0.17609018087387085|0x3e3450fc|
|First Y|0.4402254521846771|0x3ee1653b|
|Second squared-length sum|1.0|0x3f800000|
|Second reciprocal|1.0|0x3f800000|

The final submitted XYZ reference therefore has words
**(0x3e3450fc,0x3ee1653b,0x3f61653b)**. This is a calculation from the
identified constants and scalar instruction order, **not a measured native
frame or a general promise about alternate floating-point environments**.

### 6.2 Separate lighting and view-normal spaces

The WORLD shader decodes the packed face selector to its local face normal.
For lighting it transforms that local normal through the **WORLD upper3×3**,
computes its three-component squared norm, applies the GPU reciprocal
square root, then multiplies the transformed vector by that reciprocal.
This normalized WORLD-space normal supplies the directional, fixed-axis
and point-light dot products. It does not use the view-transformed normal
for those lighting terms.

Independently, the shader transforms the **original local normal** through
the upper3×3 of the internally composed **WORLD×VIEW**, then normalizes
that result with its own dot/reciprocal-square-root/multiply sequence.
That is the view-normal varying used by the optional pixel shininess
ripple. Neither route constructs an inverse-transpose matrix, and the
view-normal route is not a reuse of the already normalized WORLD normal.
The light-direction varying is also formed through WORLD×VIEW, without
another normalization; the ordinary pixel shader does not read it.

For ordinary region records, WORLD has a translation-only basis, so its
lighting-normal transform receives the unchanged local face axis before
normalization. Map yaw, pitch and view scale belong to VIEW. They do not
rotate the CPU direction or the fixed lighting axis before the lighting
dot products. Marker/model records with another WORLD basis still use
that actual basis. A general substitution of view-space lighting, or of
a simplified map-space matrix for those records, is not established.

### 6.3 Position composition and independent uploads

The actual inputs are the descriptor's float WORLD matrix, map view+0x58
and map projection+0x18. They remain independent GPU uploads, with the
existing native transposition described in
[GPU coordinates §3](steam-world-gpu-coordinates.md#3-queue-composition-and-separate-gpu-matrix-uploads).
The map context's verified identity base and descriptor composition remain
the pass-family contract; no additional camera-origin subtraction is
inserted by these vertex operations.

The geometric space order is **local → WORLD → VIEW → projection**.
The compiled shader constructs WORLD×VIEW coefficients first, then the
combined projection coefficients, using GPU float multiply/multiply-add
operations. Each combined component starts with its Y product, then
accumulates X, Z and the homogeneous term. It evaluates view position
from the original decoded local point and the composed WORLD×VIEW,
and clip position from that local point and the composed full matrix.
WORLD position for point-light distances is evaluated separately from
the same local point and WORLD.

Local XYZ is recovered from the normalized packed position by multiplying
by255; homogeneous W is **1**, not the face-selector input. DepthOffset
is subtracted from the homogeneous/translation coefficient of the
composite clip-Z calculation before its final position dot product.
For this W=1 input it gives the established clip-Z offset, without changing
view position or lighting-normal inputs.

Thus the established space chain does not claim a bit-identical sequence
of three CPU/vector position transformations or a CPU double-precision
combined matrix. The compiled GPU composition/dot order is a separate
precision boundary from CPU map construction and direction normalization.

### 6.4 Exact fixed axis and lighting/palette operation order

The compiled vertex shader's fixed axis is:

|Component|Exact float32 value|Word / embedded RVA|
|---|---|---|
|X|0.597022294998169|0x3f18d674 /0x439744|
|Y|0.3980148732662201|0x3ecbc89b /0x439748|
|Z|0.696526050567627|0x3f324f88 /0x43974c|

It uses these literal components directly, not a runtime normalization
of rounded replacements. The reached lighting sequence is:

1. Dot the normalized WORLD normal with the submitted directional vector;
   multiply-add using float **0.7/0.3**, then retain at least0.
2. Dot that same normal with the fixed axis; take its absolute value,
   then multiply-add using **0.5/0.5** to produce the fixed-axis weight.
3. Multiply back RGB by the fixed weight, then multiply-add front RGB
   with the directional weight into that result, saturating0..1.
4. Separately multiply input vertex V by material alpha, and multiply
   ambient RGB by the fixed weight. Multiply-add the saturated front/back
   term times V×alpha into that ambient term. Add accumulated point-light
   RGB, then saturate0..1. The map's zero point tables leave every
   positive-radius point-light branch unentered.
5. Double normalized stored vertex RGB by addition. Subtract clamped
   lighting green from ambient.w, multiply-add with **0.5/1**, then
   clamp1..2 to obtain palette saturation gain. Compute palette luminance
   with **0.299/0.587/0.114**, separately form its product with one minus
   the gain, then multiply-add gain times the doubled palette into that
   product. Clamp the boosted palette0..2.
6. For the ordinary **Glow0** path, multiply
   clamped lighting RGB by material RGB first, then by boosted palette.
   Other Glow choices and the final White clamp retain their existing
   WORLD contracts; they do not substitute view-space lighting.

Useful exact shader literal words are **0.7=0x3f333333**,
**0.3=0x3e99999a**, and **0.5=0x3f000000**. This preserves the compiled
dot, multiply, multiply-add and saturation boundaries. GPU dot reduction,
multiply-add fusion and reciprocal-square-root rounding are not proved
equivalent to a scalar CPU calculation; the instruction-derived CPU vector
reference in§6.1 does not establish those GPU results bit for bit.
