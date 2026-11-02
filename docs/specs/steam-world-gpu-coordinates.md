# Steam ordinary WORLD coordinates at the GPU boundary

Stable bounded native-only supplement, 2026-11-01. Sources: normal scene
**0xe2e00**, collection **0xd3170**, pose **0x17ab0**, object drawing
**0xe0ac0**, terrain drawing **0xe5070**, enqueue **0xee2f0/0xee990**,
consumer **0xe6e60** and the directly reached matrix setters. No game,
implementation, tests, extractor, SDK, owner data or captures were used.
This covers the ordinary main3D WORLD passes before screen overlays.

## 1. Established camera contracts retained

Camera construction and its precision remain
[gameplay camera §§7.1–7.3](steam-gameplay-camera.md), including the float
rotation/projection matrices and the shaken origin-relative view. Argument
addresses, final-eye versus look-at identity and cached lifetimes remain
[pose/body coordinate lifetime](steam-body-particle-coordinate-lifetime.md).
The current-scene CPU/collection/GPU-upload order remains
[world pose §2](steam-world-pose-invocation.md).

The relevant installed fields are:

| Renderer field | Meaning on this route |
| --- | --- |
| +0x338/+0x340/+0x348 | Signed64 final eye XYZ; CPU distance anchor |
| +0x428/+0x430 | Signed64 offsets equal to negative fixed look-at X/Y |
| +0x39c | Float4×4 shaken, origin-relative view copied from Game+0x1f24 |
| +0x3dc | Float4×4 projection copied from Game+0x1f64 for these WORLD passes |

There is no matching Z-origin offset. The view already includes the
relative negative eye and shake. Its translation is formed from the
appropriate fixed differences before float conversion, as established in
camera§7.2. The final eye is not substituted for the XY look-at origin.
Scene0xe2e00 installs the GPU view/projection after0xd3170 and point-light
collection, before terrain/ordinary model draws. Its later screen-overlay
camera changes are a separate phase.

## 2. Fixed positions become float model translations

**DOT = float1/65536**, exact word **0x37800000**, including the copy at
**0x42e510**. The reached operations below add fixed integers **before**
the signed64-to-float conversion. Native conversion is direct signed64
**CVTSI2SS**, followed by a float multiply by DOT. Absolute world position
and the origin are not separately rounded to floats and then subtracted.

| Producer | Translation boundary |
| --- | --- |
| Ordinary posed creature0x17ab0 | Add drawn fixed X/Y at creature+0xb10/+0xb18 to the supplied signed64 origin values, in low64 arithmetic; convert each sum to float and multiply by DOT. The normal caller supplies pointers to Renderer+0x428/+0x430. |
| Ordinary environment-model branch in0xd3170 | Add each record's fixed X/Y+8/+0x10 to Renderer+0x428/+0x430 before float×DOT. Fixed record Z+0x18 converts separately to float×DOT. The ordinary model translation precedes its existing scale, animation and centering operations. |
| Generic static-object/door placement in0xe0ac0 | Add the object's fixed X/Y+8/+0x10 to the same renderer offsets before float×DOT. Fixed Z+0x18 converts to float×DOT before the existing type-specific float translation/rotation/scale stack.0xd3170 reaches this helper at0xd9244. |

The pose's exact Z fixed offsets, truncations and subsequent part matrices
remain [creature pose §8.2](creature-pose.md#82-the-base-matrix-and-the-body-frame).
They do not introduce a Z camera-origin member. The static object's full
allocation centering, size factors, hinge and sliding rules remain
[object transforms §4](worldgen-parity-gaps.md#4-verified-static-object-scale-centering-and-doorwindow-pivots).
This supplement does not replace those transforms with an AABB fit.

### 2.1 Terrain uses zone origin plus a vertical band base

Ordinary terrain0xe5070 constructs its model matrix from identity. For
each horizontal axis, read signed32 zone coordinate **Zone+0x10/+0x14**,
shift left6 **in32 bits**, then sign-extend that result to64 bits and
shift left16. Add the corresponding signed64 renderer origin offset in
low64 arithmetic. Convert that sum directly to float, then multiply by
DOT. Native X/Y construction is visible at **0xe51d2–0xe5212**.

Thus the zone-coordinate×64 narrowing occurs **before** fixed64 promotion.
An unbounded64-bit zone-coordinate×64 product is not this boundary.
The ordinary finite signed32 shift behavior is the native low32 result,
including its sign when promoted.

Z translation is instead the mesh group's **signed32 vertical band base
at+0x5c**, converted to float block units. It is not a fixed65536-unit
field and receives neither DOT nor a camera-Z subtraction. The resulting
matrix has identity basis and this XYZ translation. The repeated matrix
construction for the directly reached terrain face groups uses the same
zone origins and group base.

The group vertex buffer contains local corner coordinates relative to the
zone's XY origin and that group's vertical band. The existing250-block
band representation is [voxel vertex inputs §1](steam-voxel-vertex-inputs.md#1-shared-packed-vertex-input).
The complete band-partition producer is not newly audited here; the draw's
direct input and conversion are the signed32 group+0x5c above.

## 3. Queue composition and separate GPU matrix uploads

Before normal collection, scene0xe2e00 resets its world context
**Renderer+0x138** through **0xeef30 at0xe30be**. That reset restores the
context base matrix to identity and its screen-center fields to0 as well
as clearing prior queues. Enqueue0xee2f0/0xee990 combines that float context
base with the supplied float model matrix. It performs no second fixed
origin subtraction. The opaque descriptor retains its float4×4 at list
node+0x10; consumer0xe6e60 passes it to renderer virtual+0x50.

Let **S=Renderer+0x558**, the shader-state object reached by these wrappers:

| Matrix | Renderer virtual / wrapper / setter | CPU float cache | GPU VS constant block |
| --- | --- | --- | --- |
| WORLD | +0x50 /0xc5ab0 /0xc7ab0 | S+0x68 | b2, buffer at S+0x400 |
| View | +0x58 /0xc5a90 /0xc78b0 | S+0xa8 | b1, buffer at S+0x3f8 |
| Projection | +0x60 /0xc5a50 /0xc7240 | S+0xe8 | b0, buffer at S+0x3f0 |

Each setter copies the supplied16 float components into its respective
CPU cache when the input is a different address. Its GPU upload rearranges
those components into the transpose: the first four uploaded floats are
CPU elements **0,4,8,12**, then **1,5,9,13**, then **2,6,10,14**, then
**3,7,11,15**. These are independent float matrix uploads/bindings through
0xc68c0, not a CPU-combined double WORLD-view-projection product.

An ordinary model draw selects the appropriate stored model matrix;
terrain supplies the matrix from§2.1. GPU upload does not reinterpret
either translation as fixed units, add the final eye or subtract another
render origin. These setters change shader matrix state, independently
of the renderer's separately stored fixed eye/origin fields.

## 4. Vertex space and shader composition

The existing packed vertex contract is eight bytes: local XYZ corners,
normal index, then RGBA/V bytes. Both input groups are R8G8B8A8_UNORM.
The WORLD VS at **0x4387b0** multiplies normalized position XYZ by255 to
recover the local corners. POSITION.w is the normal selector, not the
homogeneous position W; point transformation uses homogeneous1.

CUB corners remain allocation-local voxel coordinates, with their pose/
instance centering and voxel-to-block scale already in WORLD. Terrain
corners are zone/band-local as above. There is no per-vertex fixed64 world
position supplied to this shader and no additional DOT applied to the
decoded corner merely because the instance translation came from fixed
coordinates.

The shader transforms the local point by **WORLD, then view, then
projection**, with these float matrices. WORLD first produces the rebased
block-space point; view produces eye-relative/view coordinates. The normal
spaces, clip DepthOffset, homogeneous divide/raster interface and pixel
processing remain [WORLD shader output §1](steam-world-shader-output.md#1-vertex-outputs-and-raster-interpolation).
No replacement of the shaken view by the full-world inverse is implied.

## 5. Arithmetic reference and explicit limits

Arithmetic reference, not a runtime capture: look-at X fixed
**1099511627776** (block16777216), hence origin **−1099511627776**, and
actor/object X fixed **1099511644160** produce integer relative X16384,
then float translation **0.25 blocks**. A terrain zone X262144 produces
signed32 block origin16777216, hence relative matrix X0 with that same
look-at origin. This labels the origin/conversion boundary only.

This closes the directly reached ordinary model/terrain upload path and
its spaces. It does not newly specify every articulated-object/effect
branch, mesh-band partition algorithm, water, beams, particles, map/GUI
preview cameras or cross-GPU last-bit execution. Their separate existing
contracts retain their scope. CPU frustum/distance tests also retain their
own exact inputs and do not become GPU world-matrix transformations.
