# Steam map border and information-HUD consumers

Independent native-only follow-up to worldmap-rendering §6 and
steam-map-region-history. Sources are the Steam native code and binary
constants, especially0x2f44a0,0xe64b0,0xee2f0,0xe6e60,0xe7980,
0x103200 and0x2e2e80. No engine, tests, exporter, build, SDK, owner
assets/captures or reconstruction were inspected; no game was executed.
This is a stable bounded consumer contract, not a replacement asset.

## 1. Which region the 3D border uses

WorldMap is **Game+0x10**. Its signed32 region pairs have distinct roles:
**+0x118/+0x11c** is the consumed hovered region; **+0x120/+0x124** is
the forced region. An X component of **0x7fffffff** is the invalid
sentinel tested by these consumers. Do not merge these pairs with
the fixed64 map cursor at+0x100/+0x108/+0x110.

The late Interface0x103200 history-label rectangle writes the forced
pair before current3D map submission. A successful history-label hover
copies the selected saved region; the alternative writes the sentinel
pair. The exact rectangle and source lifetime remain
[map/history §§2–5](steam-map-region-history.md). F4 does not replace
that normal binding phase with a draw callback.

In **0x2f44a0**, the first visible-tile traversal admits a cached
region only when its record pointer and that record's model pointer
are both nonnull. For each admitted tile, queue the region, perform
the ordinary ray pick when the half-radius flag is clear, then apply
the forced pair, then consider that tile's castle-cell overlay.
Successful ray picks replace the hovered pair at
**0x2f528e/0x2f5298**. A later successful tile visit can replace the
earlier pair; this assignment is not a retained nearest-hit-region
reduction across the tile traversal.

The force test and copy are **0x2f52d0–0x2f52ec**: if forced X is
not the sentinel, copy both forced components into the hovered pair.
This is inside the admitted cached-region branch and outside the
half-radius ray-pick condition. It is not an unconditional assignment
at map-pass entry. With no admitted region visit, that copy does not
occur. No Y-component sentinel test is added here.

Neither wrapper0x2f3d00 nor0x2f44a0 clears the hovered pair at entry:
0x2f3d00 only enters the map critical section. The cursor and player-label
collection have their separate entry resets. The later ordinary
**MapOverlay draw0x2829c0 resets the hovered pair**, then can replace
it through its flat-marker hover logic. Consequently the current3D
border precedes current flat-marker hover; absent a fresh ray/force
write it can consume the retained preceding Overlay result. Skipped
Overlay draws do not manufacture a reset. See
[map rendering phases §§1–4](steam-map-rendering-phases.md) for full-map,
minimap, dragging, Control/half-radius and ancestor/F4 admission.

## 2. Border lists, readiness and traversal

After the entire first tile traversal, **0x2f57e8** admits the border
pass when hovered X is not the sentinel. It visits the same visible
tile rectangle, X outer and Y inner. Each interval is tile-center minus
radius through tile-center plus radius minus1, inclusive; radius is
the current stored8, or4 with the half-radius flag. The centers come
from drawn fixed XY converted to whole blocks toward zero, narrowed
to signed32, then signed division by the tile width **16 times u**
toward zero. These are cache tile coordinates, not region-history keys.

Here **u** is the integer map voxel unit produced by0x2f44a0 from its
normalized zoom layer: float32 powf(2,float32(layer)), multiply by
float32 4, truncate to signed32, and raise to1 if smaller. Existing
worldmap-rendering §4 supplies the layer clamp and camera inputs.

For each tile, **0x2f5887–0x2f589c** again requires the cache record
and its model pointer. Search that record's **+0x78 border map** for
the exact hovered signed32 region pair; absent key or empty point list
produces no cubes. Point lists are visited in their stored forward
order. A list node has next/previous pointers+0/+8 and signed32
whole-block XYZ at **+0x10/+0x14/+0x18**.

These points are the generated **(X,Y,t)** records of
[worldmap-model §3.1](worldmap-model.md), with one point at most per
eligible column. In particular, t is the producer's stored whole-block
height, not a final occupied-vertex AABB, a screen coordinate, or a
height snapped again by this consumer.

The border admission does **not** require the region's fade to equal1,
does not multiply border opacity by region fade, and does not repeat
the local-player effective-block bit0x20 or personal castle-cell-map
checks. Those belong to castle-cell drawing in the first traversal.
A newly ready model whose terrain opacity is still0 can already
contribute border cubes. The border does not itself build a missing
region resource.

## 3. Actual model and local transform

For every point, **0x2f59c0–0x2f59e3** fetches the ordinary catalog
of **World+0x70**, requires its pointer-vector count to exceed0xa46,
and selects catalog entry **0xa46**. Native catalog initialization
**0x177f50,0x19c48b–0x19c4f0** identifies that entry as the file-backed
owner resource **cube.cub**. See
[VoxelWidget presentation §3](steam-voxel-widget-presentation-input.md)
for the filename/index provenance. The filename alone does not establish
its dimensions, occupied voxels or RGB; use the owner's actual resource.
There is no procedural replacement cube in this path. An absent catalog
entry yields a null local resource, but this branch subsequently reads
its dimensions without a protective null-resource return.

Start an identity model matrix. Per point axis, sign-extend the stored
signed32 coordinate to64 bits and multiply by65536. Subtract the
corresponding **drawn map center fixed64** supplied to0x2f44a0. Convert
that signed64 difference directly to float32, multiply by float32
**1/65536**, then multiply by the float32 reciprocal of u. That
reciprocal is obtained by **double1.0 divided by double(u), then narrowed
to float32**, at0x2f59a0–0x2f59b5. Each multiplication has its own
float32 result. Translate by these three map-model coordinates through
**0x2b7a0 at0x2f5abe**. Do not snap the point or narrow the difference
back to signed32 before conversion.

The uniform scale is a float32 **2.0 divided by this draw's viewScale**,
at **0x2f5998–0x2f599c**. Apply it on all three axes through
**0x2b580 at0x2f5ad3**. This is not another division by u: position has
already changed from world blocks to map-model units, whereas cube
allocation dimensions are scaled by2/viewScale. Caller viewScale retains
the full-map/minimap and GUI-scale production in worldmap-rendering §4;
do not introduce an additional GUI multiplier here.

Then read signed32 allocation dimensions **resource+0x88/+0x8c/+0x90**,
convert each to float32, multiply each by **−0.5f**, and apply that
local translation through **0x2b7a0 at0x2f5b0f**. It acts through the
already scaled basis, leaving the allocation center at the point.
It does not use occupied-voxel bounds, a bottom pivot, or a raw unscaled
addition to the translation column. This local matrix has no extra
heading or billboard rotation. The normal map view/projection supplies
the shared camera transformation afterward.

## 4. Pulse, material and actual queued pass order

The pulse reads **World float32+0x440**, not calendar+0x0c, GUI clock,
wall time or an integer clock cast. Its direct producer **0x2e2e80**
adds that routine's float32 elapsed-ms argument to the stored float32
accumulator. No independent border timer is advanced by drawing.

At **0x2f57fd–0x2f581b**, multiply that float by float32
**0.006000000052154064** (word **0x3bc49ba6**, data0x43ef34), pass the
float32 result to **cosf at0x2f580d**, add float32 1, then multiply by
float32 **0.4000000059604645** (word **0x3ecccccd**, data0x42e52c).
Round each arithmetic result to float32. No integer time truncation,
phase modulo, extra terrain-fade multiplication or double phase is
inserted. Evaluate it once for this border pass and reuse the opacity
for all its points. The previously written multiplier0.5 was incorrect.

Thus for finite ordinary clock input the opacity ranges from0 to the
float32 value **0.800000011920929**, not1. Submit at
**0x2f5b14–0x2f5b59** with material RGBA **(1,1,1,1)**, **Glow0,
Shininess0, White0**, and shader **DepthOffset1.600000023841858**
(word0x3fcccccd, data0x4328c0). White material is not the shader White
effect. No separate pulse scales the material alpha or vertex RGB.

Shared queue helper **0xee2f0** rejects nonpositive opacity; exact0
therefore emits no descriptor. Every positive border pulse is below1
and appends a **normal-cull faded descriptor**, not the opaque resource
tree or the opposite-cull vector. It uses the shared context-matrix
composition and descriptor contract in steam-world-pass-controls §2.

Submission order is important: the first tile traversal queues each
ready terrain model and its admitted castle cells; the second traversal
appends the border points; only afterward come the creature/player/tomb
and route families. Do not reinterpret that producer order as one
unbatched GPU sequence. **0xe64b0** binds the current stored map view/
projection and map lighting, clears point tables through0xce580, then
calls **0xe6e60 followed by0xe7980**, both with mode0.

The former consumes the opaque resource-grouped trees. The latter's
general order is normal depth-only3, opposite depth-only4, normal faded
color6, opposite faded color7, each only for its corresponding nonempty
vector. The ordinary map populates only normal collections, so it
actually selects state0 and conditional3/6, not1/4/7; see
[pass-family §§3–5](steam-retained-item-and-map-pass-family.md). Border descriptors
participate in the normal depth-only iteration and later normal blended
color iteration, in their forward vector order; there is no additional
border distance sort. The normal faded vector can also contain other
map families. Its depth/color state contract is the existing
[world pass controls §2](steam-world-pass-controls.md), not a new overlay
blend mode. Mode0 does not add per-model world point-light table lookup.
Ordinary flat MapOverlay labels/icons and the information HUD draw later
through the GUI traversal. The map has no separate animated water pass.

## 5. Information HUD ZOOM text

The ordinary information-line producer is **Interface0x103200**.
After its coordinate/time/temperature/humidity parts, it appends the
literal **`  ZOOM 1:`**, with two leading spaces, then a decimal signed
integer. At **0x105757–0x10577a**, read signed32 **Game+0x1e04**, convert
it to float32, evaluate **powf(2.0f, that float)** at0x10576d, multiply
the returned float by float32 **4.0**, and truncate that float toward
zero to signed32 with the conversion at0x10577a. This is N in1:N.
The constants are data0x4ac9d0=2 and0x4aca20=4.

The text calculation does not reuse a ray-picked position, the map
scale factor, GUI scale, region scaling, viewport dimensions or a
world-distance measurement. It does not add the map model producer's
minimum-unit/layer clamp around this independent read. Supported normal
levels give **L0→1:4, L3→1:32, L4→1:64, L6→1:256, L8→1:1024**.
Mapped zoom and wheel have their different existing limits; see
map/history §6 rather than imposing the wheel limit on this label.

The complete line, not only its ZOOM suffix, is assigned recursively
through **0x334c50 at0x1057b6** to **Interface+0x1b0 = Game+0x2c8**.
This code-created text node is constructed through0x1008a0 in0xf9ac0
under ordinary game-UI root Game+0x220. The string producer has no
full-map-only or text-node-shown condition: it is reached during the
normal Interface binding call even for minimap or hidden ancestors.
Normal GUI hierarchy/visibility still controls whether that text draws.

## 6. Bounded limits

This follow-up establishes the consumer transform and the owner resource
identity, not owner cube geometry or a substitute asset. Internal CRT
cosf/powf implementations are not independently reconstructed. It does
not alter the previously bounded history-node lifetime, empty-selection
or missing-resource behavior, or invent a fresh Overlay hover when its
draw is skipped. Exact movement-close targets are now enumerated in
[map/history §7.4.1](steam-map-region-history.md).
