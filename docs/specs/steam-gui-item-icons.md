# Steam shared 3D GUI item-icon renderer

Independent prose from native **0xddbc0**, **0xdc390**, matrix helpers,
renderer vtable0x4385e8/setters and constant data. No engine, tests, exporter,
build files or owner captures were inspected; no game was run. The world
shader, CUB vertex inputs and target conversion are independently specified
in steam-world-shader-output.md and steam-voxel-vertex-inputs.md.
Identification-specific center, scale and Euler inputs are steam-identification-ui §4.

## 1. Inputs, sprite source and render state

The shared routine receives renderer, original item, **screen center X/Y
in pixels**, Euler-degree vector, float size input, float screen-depth
translation, and a byte enabling an offset dark pass. Screen center is
already transformed by the caller's GUI/frame geometry. Do not multiply
it by GUI scale again. Renderer viewport width/height are signed32 at
**+0x41c/+0x420**, converted to float for this routine.

Sprite selection is the existing item selector **0x173b10** on the
renderer owner's sprite collection. For category0, it instead uses sprite
slot **0xcd1**, returning without drawing if that slot is unavailable.
For other items a null selected sprite also draws nothing. This is not
an instruction to invent a replacement image or voxel model for a missing
owner asset. Draws use that sprite's native voxel vertex/index buffers
through **0x16ec60**, subject to their ordinary readiness tests.

The identification caller selects renderer **state0** before0xddbc0.
State0 through **0xc3dc0**, created by0xc3150, uses the ordinary world
shader pair, enabled depth test/write with **LESS_EQUAL**, disabled alpha
blending, full RGBA write mask and normal native solid raster state.
The D3D raster descriptor has solid fill3, cull mode2, FrontCounterClockwise0
and depth clipping enabled. Adapt winding to the replacement API's projection
conventions instead of assuming an unflipped perspective matrix.0xddbc0
does not clear depth between icons/passes or replace it with a GUI depth-off
shader. The caller restores its ordinary Plasma GUI drawing context after
the item draw; that is separate from these3D state inputs.

## 2. Header-based centering and model matrix

Read the selected sprite's **signed32 full allocation dimensions**
**X+0x88,Y+0x8c,Z+0x90**. Do not substitute occupied-vertex bounds.
Let D initially be the largest of those3 dimensions. If Z≤3, add4 toD.
For category4, replace D by **trunc32(float32(D) times1.5)** after that
small-Z adjustment. Other categories do not take this extra divisor scale.

Start model matrix with **translation(0,0,1)**. Append rotations, on the
right in the existing column-vector convention, **Rx(Euler.x),Ry(Euler.y),
Rz(Euler.z)** using the float degree/trig matrix helpers. Category9 then
additionally appends **Rx(90),Rz(−90)**, in that order. The routine does
not replace the supplied angles by a gameplay camera orientation.

Append **three successive uniform scales**, preserving each float step:

1. Caller float size input.
2. Float **1024 /float(viewport height)**.
3. Float **1 /float(D)**.

Finally append translation **(−0.5X,−0.5Y,−0.5Z)**, each dimension first
converted to float and multiplied by stored−0.5. It is a full-header
corner-to-center translation after those rotations/scales, not a pivot
chosen from visible occupied voxels. Thus the final model relationship is
**T(0,0,1) Rx Ry Rz [ring rotations] S(size) S(1024/H) S(1/D) T(−dimensions/2)**.
Do not merge those successive scale multiplications when exact float
transforms are required. This model matrix is shared by dark and main passes.

## 3. Identity view and shifted45° projection

The normal icon view is **identity**, independent of gameplay eye, camera
angle, render origin, camera zoom and gameplay FOV. Build a float perspective
matrix Q with:

- h=**1/tanf(float0.39269909262657166)**, the stored half-angle(22.5°).
- Aspect is **−(float(W)/float(H))**, then Q00=**h/aspect**, Q11=h.
- Q22=stored **float1.000100016593933**, word **0x3f800347**.
- Q23=stored **float−0.10001000016927719**, word **0xbdccd20b**.
- Q32=1, all other entries0, includingQ33.

These depth words correspond to the routine's nominal near0.1/far1000
projection, but are **literal inputs** here. Preserve them rather than
recomputing two nominal distances and accepting changed rounding. Aspect
is negative; native positive view Z has clip W=view Z and depth0..1.

Compute float nx by subtracting W times0.5 from screen X, dividing byW,
then **adding that result to itself**. Compute ny by subtracting H times0.5
from screen Y, dividing byH, then multiplying stored−2. The submitted
projection is **T(nx,ny,caller screen-depth translation) timesQ**. This
translation is in clip/homogeneous space: it shifts final NDC X/Y by
nx/ny without requiring every voxel to share a view depth. Its Z input
adds to NDC depth and is distinct from the shader DepthOffset below.
Identification supplies this screen-depth input0.

## 4. Pass order and exact shader inputs

The renderer vtable's relevant setters resolve to the existing world
shader contract: matrices **0xc5a30→0xc6fc0**, opacity
**0xc59a0→0xc6790**, DepthOffset **0xc59c0→0xc6a80**,
fog range **0xc59e0→0xc6bb0**, Glow **0xc59f0→0xc6c50**,
material **0xc5a20→0xc6f20**, White **0xc5aa0→0xc7a20**,
Shininess **0xc5a60→0xc73b0**, light palette **0xc5a10→0xc6ce0**.

At entry set **pixel opacity1** and **fog range10000**. Do not use a
special fog-disabled or ungraded icon fragment shader: these are the same
world vertex/pixel shaders, including their input encoding, White clamp,
palette saturation, optional ripple, finite fog and final grade. Ordinary
sprite raw RGB is not independently normalized/doubled a second time here.

### 4.1 Optional offset black pass

When enabled, first draw the same sprite/model with view translated by
**(−0.1 times the caller size input,−0.1 times that size,0)**. The multiply
is float. This is a view-space offset, not negative model scale, an enlarged
outline or another generated silhouette mesh. The model retains the positive
scales built in §2. Under this projection it offsets toward screen down/right.

Submit the shifted projection from §3 unchanged, material **(0,0,0,1)**
and shader DepthOffset0, then draw the sprite. Opacity remains1. This first
pass occurs **before** the main-pass clearing/lighting writes below, so
other shader constants not explicitly set yet retain their previous values.
Normal native overlay setup has White0; do not claim the function forcibly
clears White before this first draw. The black material makes this a dark
pass under that native state, with the same finite fog/grade still present.

### 4.2 Main colored pass

Restore identity view and submit the same model/projection. Then set:

| Parameter | Value |
|---|---|
| White |0 |
| DepthOffset | **float0.1 if dark pass enabled**, otherwise0 |
| Glow |0 |
| Shininess | **0x10a420(original item)**, §5 |
| Pixel opacity |1 from entry |
| Material RGBA |(1,1,1,1) |
| Point lights | All16 position/radius and color records zeroed through0xce580 /0xc7010 |
| Directional front RGBA |(0.7,0.7,0.2,1), constant0x43f0a0 |
| Directional back RGBA |(0.5,0.5,0.5,1), constant0x43f0f0 |
| Ambient RGBA |(0.3,0.3,0.8,1), constant0x43f140 |

Direction is supplied as follows: **a=1/sqrtf(stored float2.1599998474121094)**,
then float vector **(a,a times0.4,−a)**. Palette setter0xc6ce0 subsequently
normalizes that float vector again using its float squared sum/sqrt/reciprocal.
Thus its geometric direction is proportional to **(1,0.4,−1)**, not the
title-overlay's(1,1,−1). Preserve the caller's float preparation and setter's
normalization when exact constants matter. No scene point-light zone binding,
day/night palette or per-item rarity tint replaces these explicit inputs.

Draw the selected sprite after those writes. DepthOffset is subtracted
from **clip Z**, without changing clip W; its0.1 is not the NDC-depth
translation argument and is not a physical translation of the icon.

The function does not write the shader's sky/fog RGBA, SunPos or
fogGradientTranslation in this path; those remain the current renderer's
values. Fog range10000 makes their contribution very small at this local
model depth, but not an explicit zero branch. Positive Shininess still
uses the existing world shader ripple and its inherited gradient input.

## 5. Item material's Shininess producer

**0x10a420** returns0 for category0. For nonempty items, apply these in order:

| Condition | Returned float |
|---|---|
| Material1,11,12,13 or23 |1 |
| Otherwise material4 or29 |0.5 |
| Otherwise material33 or35 |0.25 |
| Otherwise category23, or category24 subtype4/5/7/8/10/18 |1 |
| Otherwise |0 |

This is the shader's **Shininess input**, not Glow or material-alpha.
As established in the shader output spec, only its positivity selects
the ripple branch; its numeric magnitude is not a ripple amplitude.
Ordinary category15/material0 has0. It is not made emissive or recolored
by rarity merely because it is displayed for identification.

## 6. Weapon spirit cubes after the main sprite

Finally **0xdc390** appends geometry only for **category3** with nonzero
attribute count+0x9d. In stored order, each attribute's **signed8 XYZ**
at+0x1d+4i offsets a copy of the same model matrix's translation by those
voxel coordinates; material is the fourth,unsigned byte. It draws owner
sprite **0xa46,cube.cub**, a1×1×1 white voxel, without recentering that cube.
The cube therefore has one weapon voxel's size. View/projection, depth
offset, opacity, Glow/White, cleared points and icon palette remain those
of the main pass. There is no additional dark pass for spirit cubes.

Each cube's material RGBA is **0x109960(material,white tint,extra scalar0)**,
whose complete material-color table is creature-pose-items §9.1. This
changes material RGB/alpha only; it does not apply a new rarity color or
particle effect. Category15 has no attachment branch even if an owner
record contains attribute data. Draws/constant setters are immediate;
this routine does not restore the prior3D shader constants afterward.

## 7. Scope and verification boundary

The shared projection/model/lighting/pass rules above and the identification
call inputs are stable. The ordinary item→owner-sprite selector remains its
existing separate asset-mapping contract; this document does not replace
unavailable assets or list every category/subtype mapping. GPU byte output
still requires the independently specified packed CUB vertex data, native
world shader, normal target conversion and caller's inherited sky/gradient
constants. No original-game screenshot or new runtime capture was performed.
