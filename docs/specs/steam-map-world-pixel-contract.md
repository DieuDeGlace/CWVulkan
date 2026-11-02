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
