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
