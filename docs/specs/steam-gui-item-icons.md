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
The D3D raster descriptor has solid fill3, cull mode2(**D3D11_CULL_FRONT**), FrontCounterClockwise0
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
at+0x1d+4i translates a copy of the same model matrix in its existing local
basis, as detailed below; material is the fourth,unsigned byte. It draws owner
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

### 6.1 Attribute translation basis and arithmetic

**0xdc390, from0xdc420**, copies the original weapon model anew for each
attribute. It leaves the first three four-float basis vectors unchanged.
It forms the fourth vector by multiplying the first basis vector by signed
attribute X, the second by signed Y and the third by signed Z, summing those
contributions, then adding the original fourth vector. This applies to all
four components, including the homogeneous component. Consequently the
attribute displacement follows the weapon's existing rotations and scales:
it is the local post-translation of that model, not a raw addition of XYZ
to its translation column, nor a screen/view-space displacement. The model
already includes the full-header centering described in §2; this branch adds
no new cube-centering subtraction.

The operation is **inline in0xdc390**, not a call to0x2b7a0. Native scalar
SSE first converts each signed byte to float exactly; for each component it
multiplies the three basis components separately, adds X contribution to Y,
adds Z, then adds the original translation component. Each multiplication
and addition rounds to float32. In particular, do not use the abbreviated
decompile's apparent translation-first grouping to establish rounding order.
The copied matrix is submitted by renderer matrix setter0xc5a30, then owner
cube sprite0xa46 is drawn by0x16ec60. Every attribute starts from the original
weapon matrix rather than the previous attribute's translated copy.

## 7. Scope and verification boundary

The shared projection/model/lighting/pass rules above and the identification
call inputs are stable. The ordinary item→owner-sprite selector remains its
existing separate asset-mapping contract; this document does not replace
unavailable assets or list every category/subtype mapping. GPU byte output
still requires the independently specified packed CUB vertex data, native
world shader, normal target conversion and caller's inherited sky/gradient
constants. No original-game screenshot or new runtime capture was performed.

## 8. Ordinary interface entry, depth, clipping and inherited state

### 8.1 Depth is retained through the ordinary interface draw

The ordinary admitted application frame **0x134420** clears its main color
target at**0x134499**, then clears **both depth and stencil** at**0x1344c4**
to depth**1**, stencil**0**, before calling Game0xa5a50. This is a frame
clear. The late scene interface entry **0xc4af0** passes traversal flags0
to GUI engine**0x33e170**. Neither that entry, identification draw**0x277250**,
nor shared icon**0xddbc0** adds a depth/stencil clear. There is also no clear
between the offset black sprite, main colored sprite and attribute cubes.

Identification explicitly selects renderer **state0** before its icon call.
**0xc3dc0**, with states constructed by**0xc3150**, binds depth enabled,
depth writes enabled, **LESS_EQUAL**, stencil disabled. Its rasterizer uses
**FillMode3**, **CullMode2(D3D11_CULL_FRONT)** and
**FrontCounterClockwise0**, with depth clipping enabled and
**scissoring disabled**. Raster descriptor constants are at**0x438710**,
**0x4386f0** and**0x4386f8**; the depth descriptor starts at**0x438740**.
State selection binds shaders/input layout and
existing WORLD constant buffers through**0xc5de0/0xc68c0**; it does not clear
depth or initialize shader effects. Both item passes use this ordinary
depth state and can write the currently bound depth target.

The numeric descriptor is authoritative. In its ten-word field order it is
**3,2,0,0,0,0,1,0,0,0**: fill, cull, front-CCW, depth bias, float depth-bias
clamp, float slope bias, depth clip, scissor, multisample and antialiased
line.0xc3150 creates this first rasterizer at renderer**+0x9d0**, before
changing the local descriptor for the separate+0x9d8/+0x9e0 objects.
State0 selects that first object through0xc3dc0. With native clockwise-front
classification, CullMode2 rejects the **front** class; the earlier wording
“back-face culling” in this subsection was an incorrect enum label, not a
different descriptor or an instruction to substitute CullMode3. Replacement
API winding must still account for the native projection convention.
The bounded [screen-orientation contract](steam-retained-item-and-map-pass-family.md)
§4.1 spells this out: in native X-right/Y-down render-target coordinates,
state0 retains counterclockwise displayed triangles, whose signed
algebraic area is negative. This is a final projected-screen test, not
an instruction to reuse a local index-order label across APIs.

Therefore, on the normal main-target route, the item draw loads the depth
contents left by earlier scene/overlay draws. Rendering Plasma backgrounds
does not imply fresh depth: Plasma's own depth state disables depth testing
(**0x38fb90**, descriptor**0x480520**), and restoring it does not clear the
buffer. A replacement must not infer a per-icon clear or a depth-disabled
icon draw merely from this being GUI work. Offscreen Plasma layers have a
different target case described below.

### 8.2 Viewport, node admission and Plasma clipping

Main target/viewport setup**0x135310** binds the window-sized color and depth
targets and a viewport with top-left**(0,0)**, float window width/height and
depth interval**[0,1]**. The icon routine does **not** install a viewport or
scissor rectangle matching its slot. Its screen-center projection uses the
full renderer dimensions as in §3. GUI draw-entry helper**0x38ea80** prepares
identity GUI model and orthographic GUI projection; it does not issue a GPU
viewport/scissor/target change or clear.

Ordinary GUI root traversal**0x330b20**, called with flags0, skips an entire
subtree for live Display.visibility0 or node flag0x04. The optional bounds
test**0x3339c0** belongs to traversal flag0x01, which this ordinary entry
does not supply. Identification's callback checks for a resolvable item,
but adds no slot-rectangle, viewport-intersection or 3D-frustum admission
test around0xddbc0. Hardware projection/raster clipping still applies.

Plasma clipping is its layer/mask composition, not a scissor rectangle
automatically inherited by the WORLD shader. Plasma's own rasterizer also
has scissoring disabled (**0x38fb90**, descriptor**0x480500/0x480510**).
The ordinary clip/blur/cache traversal remains plasma-format-and-scene
§2.3.1: descendants can be drawn into an offscreen layer and composited
through the ancestor's shape. Thus a WORLD icon emitted during that
descendant traversal can be clipped by the later **composition**; this does
not mean WORLD consumes Plasma's mask-mode constants. A widget's own draw
hook runs after its own clip-layer children have been composed, as follows
from0x330b20; do not invent an extra own-frame rectangle clip for the hook.

For a bound Plasma render surface, **0x392610** first saves the prior color
target, depth target and viewport, binds its own color target with a **null
depth target**, installs viewport(0,0,surface width,surface height,0,1), and
clears that color target to transparent black. Its counterpart**0x392200**
restores the saved color/depth targets and viewport. This is not a new
private depth buffer for the icon.0xddbc0 itself preserves whichever
target/viewport the caller's traversal has made current. Cache reuse can
also skip rerunning a widget callback until its surface is redrawn.

### 8.3 WORLD inputs before the optional black draw

Immediately before the late queued screen overlays, scene**0xe2e00**
establishes the fixed overlay palette and resets Opacity1, White0,
DepthOffset0, Shininess0, Glow0, material white, fog range10000 and all
point-light tables to zero; see steam-world-pass-controls §4. It then
consumes the opaque and faded screen-overlay lists with**0xe6e60** and
**0xe7980**, mode1, **before** entering Plasma. These consumers submit each
descriptor's material, Shininess, Glow, opacity, White and depth offset;
the initializer is not repeated after the last descriptor. Sky/Fog/Sun
RGBA, SunPos and fog-gradient translation retain the frame's inputs.

At identification entry, state0 rebinds the WORLD buffers without resetting
their values.0xddbc0 first writes Opacity1 and fog range10000; its optional
black draw additionally writes the shifted view/model/projection, black
material and DepthOffset0. **It has not yet written Glow, Shininess, White,
the directional palette, ambient or point tables.** Those inputs are the
current WORLD values left by preceding draws. GUI pipeline restoration
does not replace them with defaults. Normal overlay setup already cleared
points, but it is not a point-table clear performed by the black-pass code.

The following colored pass performs all explicit resets and fixed-light
writes in §4.2, including zeroing points. Consequently a later icon can
inherit the previous icon's retained Shininess and effect values for its
black pass; attribute cubes additionally leave their last material and
matrix in the WORLD buffers. Positive inherited Shininess still selects
the WORLD pixel ripple, and inherited White still reaches its native color
clamp. Black material alone is not authority to disable those shader stages,
fog or grading. No independent sky/SunPos/gradient override is added here.

This establishes the initialization boundary and known writers, not a claim
that every possible GUI callback is incapable of writing WORLD constants.
The exact final effect values at a particular icon depend on the preceding
submitted descriptors and callbacks. No new runtime state capture was made.

### 8.4 Relative drawing order and restoration

In0xe2e00 the main world terrain/models, water and scene effects precede the
late GUI work. The 3D world-map route**0xe64b0**, when selected, also completes
before the final screen-overlay initialization/consumption and Plasma entry.
It builds its map descriptors through**0x2f44a0** and consumes them with
0xe6e60/0xe7980 before returning. Map-label Plasma widgets remain ordinary
tree draws; this is not a claim that every map label precedes every icon.

GUI icons are **immediate draws inside widget callbacks**, not one global
3D layer after all GUI text. In ordinary0x330b20 traversal the node's own
shape is drawn first; widget flag0x200 selects its draw hook before children,
otherwise the hook follows children. Children are visited in their live
list order, with clipping/blur/cache composition as above. Therefore the
copied owner tree and widget flags determine whether other nodes' art/text
precedes or follows an icon. This audit does not substitute an assumed fixed
inventory/identification sibling order for that tree.

Within identification callback**0x277250**, the icon precedes item detail
text and the manual heading/footer/price text. Immediately after0xddbc0 it
calls GUI engine**0x38e800** to restore Plasma's shaders, input layout,
constant buffers, triangle-list topology, rasterizer, blend and depth state.
That function does **not** restore WORLD constant values, viewport,
render targets, depth contents or a prior set of WORLD matrices. Plasma
shape drawing also calls this pipeline restore through**0x3913d0**. Thus
later 2D draws resume their own pipeline, while the next direct WORLD icon
still starts with the retained WORLD values documented above.

## 9. InventoryWidget cell and retained-cursor callers

### 9.1 Source records and cell admission

The common draw **0x27d2b0**, shared by inventory, crafting-list and shop
instances, reads the collection pointer at widget**+0x1e8**. Its outer
vector contains24-byte tab descriptors; each tab contains ordered
**0xa4-byte stack records**, signed32 quantity first and the original
160-byte item at**+4**. The selected signed32 tab is**+0x2b4**. A null
collection returns before grid/cursor drawing. A selected tab at or above
the tab count skips the grid branch. The maintained nonnegative-tab case
is the supported input here; do not add a new sorting or item adaptation
step to this renderer.

The live cell dimensions are signed32 **width+0x2a8, height+0x2ac** and
per-tab first-row indices are the signed32 array at**+0x288**. Constructor
**0x27c950** defaults dimensions to40,40; when the cell-template widget
at+0x228 exists, it replaces them with truncated queries**0x32da30** and
**0x32ad10** on that template's widget. These are live dimension inputs,
not an instruction to assume40-pixel cells for every owner template.

From the widget width query**0x32a1d0**, subtract float10, divide by float
(cell width+5), and truncate to signed32 to obtain the column count. From
height query**0x329e00**, subtract float40, divide by float(cell height+5),
truncate, then multiply by columns for visible capacity. The first slot is
selected-tab first-row times columns. The ordinary rectangular loop covers
that first slot through first+capacity, exclusive; usable record indices
are limited to the tab's physical slot count.

For mode**+0x280=0**, tabs0/1 also stop the usable record interval at the
first nonzero-quantity stack whose original item region pair**+0x0c/+0x10**
differs from widget's selected region pair**+0x1f0/+0x1f4**. Empty holes
do not terminate this scan. It starts at the **first visible physical slot**,
not at slot0 or a compacted matching-region start. The complete interval,
hover/reset, navigation, distinct scrollbar count/geometry and text rules
are **steam-inventory-grid-layout1–7**. This presumes the independently specified
canonical ordering; it does not search arbitrary later records for matches.
For each usable index, ordinary icon admission is **quantity!=0**. Modes
**2/3**, crafting-list/shop, admit their usable records even at quantity0.
There is no additional category!=0, positive-quantity, class-usability or
region-adaptation filter at this call. The original item pointer passed at
**0x27dac4** is the tab base plus slot stride plus4. Any eventual resource
absence is handled by the shared selector/draw path, not a substituted icon.

### 9.2 Cell centers and caller size

The widget frame origin is queried by**0x32cf50(widget)**. Separately,
the caller forms its root matrix from the node's parent's world matrix
**0x100fa0** (identity if absent), multiplied by the root transform's local
matrix**+0x2a0** (transform pointer at node+0x60).
Use the resulting **M00**, not its absolute value, vector length, or a
separate global GUI-scale value. With slot position relative to the first
visible slot, integer column/row offsets are respectively
**10+(cell width+5) times column** and
**40+(cell height+5) times row**. Convert each to float, multiply by M00,
add the corresponding queried origin component, truncate toward zero to
signed32, then convert back to float. These are the cell's screen left/top.

For the submitted icon center, add to **both** left and top the float
product of M00 and the signed integer **cell height divided by2**, with
division truncating toward zero. In particular, the horizontal icon center
also uses half the **height**, not half the width. The draw receives these
float sums, not the independently truncated navigation-point coordinates.

Base size is float **M00 times0.03**, word**0x3cf5c28f** at**0x42e6ec**.
Within the usable record interval, compare the GUI engine's float cursor
**+0x180/+0x184** against the half-open cell rectangle: left/top inclusive,
right/bottom exclusive, with extents float(cell width/height) times M00.
Hover stores the tab/index at widget**+0x270/+0x274** and multiplies base
size by float **1.5**, word**0x3fc00000**. This is the current pointer hover,
not the click-selected pair+0x278/+0x27c. Category**9** subsequently
multiplies that size by float **0.75**, word**0x3f400000** at**0x42e710**.
Preserve those separate multiplications. Quantity does not multiply size
or create multiple3D copies of the item.

### 9.3 Time, draw arguments and ordering

Each admitted cell uses Euler **(225,0,spin)** degrees. Read the GUI
engine's **signed32 millisecond clock+0x194**; take its signed remainder
modulo10000, convert to float, multiply float **0.0001**
(**0x38d1b717**, data**0x4ac954**), then float **360** (data**0x42e7e4**).
This is the same phase for every cell in the draw, with no item seed,
per-cell offset, hover spin speed, world time or timeGetTime substitution.
Pass screen-depth translation**0**, dark-pass byte**1**, and the size from
§9.2. Native argument loads before**0x27dac4** independently confirm that
the float centers survive the integer navigation-point recording.

The common caller selects renderer state0 once before its grid loop and
issues all cell icons immediately in increasing slot order, row-major.
There is no GUI restore between two consecutive cell icons. After the
loop it restores Plasma through GUI virtual+0x80, then draws its manual
heading, cell labels/quantities/availability and other GUI decoration.
Those text passes do not turn a stack into repeated3D geometry. Thus
shader values left by one cell's colored pass can reach the next cell's
black pass exactly as §8.3 describes.

The late retained-player-cursor draw is a separate call at**0x27f5fa**:
it selects state0 again, uses the current GUI cursor directly as center,
the same Euler/time formula, literal size**0.05**, screen-depth**0**,
dark**1**, then restores Plasma. Its player record/admission and lack of a
cursor quantity label remain steam-inventory-slot-lifecycle §6.2. Reaching
this common tail is not a producer that moves a cell into the cursor.
The native-only [retained-item and pass-family supplement](steam-retained-item-and-map-pass-family.md)
confirms its exact record/lifetime, null-binding/invalid-tab admission,
after-children layer and explicit state0 selection, including the still
unverified positive cursor producer.

## 10. SpriteWidget: equipment and quick-button item images

### 10.1 Binding, admission and center

Constructor **0x290c40** creates the native SpriteWidget draw owner,
sets optional raw sprite pointer**+0x1e8** to null, initializes embedded
160-byte original item**+0x1f8** to semantic empty, stores Game
pointer**+0x298**, and sets widget
flag**0x200**. Its draw hook **0x290da0** therefore runs **before child
nodes**, after the root's own shape, under ordinary traversal §8.4. Owner
child frame/count artwork can draw afterward. The callback is not a
global after-text item layer.

If both embedded item category byte+0x1f8 is0 and raw pointer+0x1e8 is
null, it draws nothing. If the category is nonzero it selects the embedded
item branch, even if a raw pointer also exists. No adjacent stack quantity
is required. The item is passed directly, not through0x10a720 adaptation.
State0 is selected before drawing, and Plasma is restored afterward.

Center uses the SpriteWidget **itself**: query its two-component frame size
with**0x32d010**, multiply each component by float0.5, query frame origin
with**0x32cf50**, then add the matching half-size to each origin component.
The root matrix is also formed by0x100fa0(parent) times the root transform's
local matrix+0x2a0 (transform pointer at node+0x60),
but the item branch does not apply another M00 factor to these centers or
to its size. It queries frame size again and passes size **queried width
times float0.0006**, word**0x3a1d4952** at**0x4705d0**. It uses the width
query, not height, occupied-model bounds, or a guessed40-pixel cell.

The item call **0x290f92→0xddbc0** uses the same **(225,0,GUI spin)**
formula as §9.3, screen-depth**0**, dark-pass byte**1**. This callback adds
no pointer-hover enlargement, category9 size adjustment, stack-count
multiplier, disabled tint, class tint or separate rarity material to those
arguments. Shared0xddbc0 still applies its own category-specific rules.
Child GUI frame/count states are separate from the WORLD item's inputs.

### 10.2 Equipment and selected quick item sources

GUI construction **0xf9ac0** creates ten equipment SpriteWidgets, cloning
owner nodes **equipmentboxright** for even indices and
**equipmentboxleft** for odd indices, and storing them in **G+0x3c8**'s
ordered widget vector. No replacement asset is specified here. During
**0x103200**, full original equipment items are copied by**0x9cd0** into
the respective widgets' embedded+0x1f8 records. The source creature
offsets in vector order are:

| Index | Original local-creature item |
|---|---|
|0|+0x638|
|1|+0x6d8|
|2|+0x778|
|3|+0x818|
|4|+0x318|
|5|+0x598|
|6|+0x3b8|
|7|+0x4f8|
|8|+0x458|
|9|+0x8b8|

These are copied item values, not pointers borrowed into the equipped
records. The draw callback does not test source quantity or source-slot
hover; its admission remains the embedded category/raw-pointer rule above.
The existing equipment frame/label update can therefore remain a separate
Plasma concern.

The same constructor also attaches SpriteWidgets to the **sprite** child
of each of four cloned **quickbutton** nodes. The first is **G+0x3e0**.
The reached Game frame path**0xa5a50** first clears that first widget's
embedded item to semantic empty. Later it builds the personal category1,
nonzero-quantity candidate list through**0x93e60**, selects the current
wrapped selector index**G+0x20c0**, resolves that canonical tab/index, and
copies the original item at stack+4 into this widget's+0x1f8 by0x9cd0.
This selection update is not conditional on the Ring Menu being visible;
candidate construction/wrapping remain slot-lifecycle §9. A nonempty
candidate uses the same SpriteWidget item draw above. No candidates leave
its embedded item empty.

For that first quick-button count label, the same frame branch uses
**0x5f0e0**'s category mask **0x10403c02** below category29 (categories
1,10,11,12,13,22,28) before submitting the selected stack quantity to its
separate **count** child. This does not supply another3D scale or opacity.
The complete producers of the other three quick-button image bindings
are not established by this bounded caller audit.

### 10.3 Raw sprite fallback and remaining caller scope

When embedded category is0 but raw sprite pointer+0x1e8 is nonnull,
0x290da0 instead calls **0xdf230** with that resource directly, the same
center/Euler clock, and size **queried frame width times float0.0005,
then divided by float maximum full sprite header X/Y/Z dimensions**.
The stored multiplier is **0x3a03126f** at**0x43ef2c**; preserve multiply
then divide. Its final float argument0 is consumed by the WORLD shader
DepthOffset setter, not0xddbc0's screen-depth translation. There is no
0xddbc0 dark-pass-byte argument on this different helper. Its complete
helper contract is §14; do not route raw sprites through the item selector
or assume they have the item's sizing adjustments. The remaining seven
direct0xddbc0 sites are specified separately in §§11–13. Outstanding
producer/geometry scope is retained explicitly in §15.

## 11. Information preview and pointer-bound upgrade panel

For the following root-matrix callers, **M** means0x100fa0 on the root
node's parent, or identity when absent, multiplied by the root transform's
local matrix+0x2a0 (transform pointer at node+0x60). **S** is its first
float M00. The root's live pivot is the two-float value at transform+0x278
selected by signed index+0x244. Its projected position **P** applies M to
that XY point at Z0: calculate homogeneous W, take float1/W and multiply
each transformed XY numerator by that reciprocal. This is not a frame-origin
query or an extra item-centered viewport.

### 11.1 Item-information preview0x28c752

Draw hook**0x28c290** reaches this icon only when signed32 fields
**+0x218 and+0x220 both equal INT32_MAX**, and original item pointer
**+0x1f0** is nonnull. Other branches display location/lore information;
their presence is not admission for an item icon. It constructs a temporary
item through**0x10a720(original item, output, local creature)**, using the
independently specified player/item adaptation.0xddbc0 receives this
**adapted copy**, rather than the original pointer used by inventory cells.
The comparison item pointer+0x210 contributes subsequent detail text, not
a second0xddbc0 icon at this site.

Select renderer state0. Query parent-relative frame width by**0x32da30**
twice. Center X is the second queried width times float**0.75**, then P.x;
center Y is the first queried width times float**0.3**, then P.y. Size is
**S times float0.075** (word**0x3d99999a**, data**0x42e6f4**). These calls
use neither half frame height nor0x32cf50. The order of additions and
multiplications is float32; preserve the reciprocal-first pivot projection.

At**0x28c752**, pass Euler(225,0,GUI spin) from §9.3, screen-depth
translation **−0.05**, word**0xbd4ccccd** at**0x470090**, and dark byte**1**.
That negative depth is0xddbc0's projection translation, distinct from its
later main-pass shader DepthOffset0.1. There is no pointer-hover size
multiplier in this image branch. Restore Plasma immediately afterward,
then render item/comparison details and subsequent manual text. The main
icon does not inherit a new rarity tint from the surrounding information
frame.

### 11.2 Upgrade panelG+0x370, call0x267b58

Constructor**0x267780** and draw hook**0x267960** retain a borrowed original
item pointer at widget**+0x1e8**. The reached slot/equipment selection paths
in**0x94900** assign this pointer when G+0x370 is visible and shop handling
does not take precedence, accepting categories**3 through9, or26**. One
route points at an equipped item; the other points at original canonical
stack+4. It is not an owned item copy or the retained cursor item. This
caller audit does not expand those selection paths into new input actions.

A null pointer skips the icon/details branch. Otherwise the caller first
requires**0x173b10** to resolve that original item to a resource. If it
resolves, select state0 and use the stored **itemframe** node at+0x1f8:
query its widget at node+0x70 by**0x32d010** and**0x32cf50**, then add each
half-size to the matching frame-origin component. This is the stored icon
frame, not the whole upgrade panel or the **rightarrow** control cloned
separately at+0x200.

Size is **S times float0.06**, word**0x3d75c28f** at**0x432d68**. The call
at**0x267b58** passes the borrowed original item, Euler(225,0,GUI spin),
screen-depth**0**, dark**1**. No icon hover/count scale is added. Restore
Plasma immediately after the sprite, before the original/prospective item
detail text and upgrade-button presentation. When resource resolution
fails, the later non-icon panel logic can still run; that is not authority
to fabricate an unavailable item resource.

## 12. Crafting detail result and required-item images

Both direct calls belong to recipe-detail draw**0x26c2d0**. The selected
result is the widget's embedded original160-byte item**+0x1f8**. It first
resolves that item with0x173b10 and returns on a null resource, before
either result or required-item images. It queries frame origin with
**0x32cf50(widget)** and forms root M/S as §11.

### 12.1 Result call0x26c58d

Center X is float**0.5 times parent-relative width0x32da30**, then queried
origin X. Center Y is float**S times110**, then origin Y (data**0x42e7c4**).
Default size is **S times0.1** (word**0x3dcccccd**, data**0x42e528**).
For its own local-pointer hover test,0x32b180 supplies pointer coordinates:
require X strictly between effective width0x32a1d0/2 minus60 and plus60,
and Y strictly between**50 and170**. In that case size becomes
**S times0.15**, word**0x3e19999a** at**0x42e6fc**, and the result is
copied into the current detail-hover item+0x2ec. These are strict edges,
not the half-open ingredient rectangles below.

The supplied Euler vector is the live three-float field **+0x1e8**, not
the common225-degree vector. Constructor**0x26bf70** initializes it to
**(−120,0,0)**. Each reached draw first converts GUI engine **signed32
last-step milliseconds+0x190** to float, multiplies by stored float
**0.02** (word**0x3ca3d70a**, data**0x42e6e8**), adds the old float Z
angle+0x1f0 and stores it. It uses no remainder10000 or angle clamp here;
hidden/skipped draws do not advance this draw-local angle. The exact
contract is the live Euler vector plus this update, without assuming no
other caller could assign the vector.

Select state0 and draw the original result item with screen-depth**0**,
**dark byte0**. This result image has no offset black sprite. Keep the
WORLD pipeline active for the following required-item loop; there is no
Plasma restore between the result and those images.

### 12.2 Required-item loop0x26c7b9

The vector at widget**+0x2a0/+0x2a8** uses**0xa4-byte records whose item is
at offset0 and signed32 required quantity at+0xa0**. This differs from
inventory's quantity-first stack layout. Every vector entry is drawn in
stored order, directly from its original item; availability/required
station/count do not suppress its image or navigation point.

Columns are signed truncation of **(parent-relative width0x32da30−10)/45**,
even though horizontal spacing is**70**. Begin with Y base equal to queried
origin Y plus **(effective height0x329e00−130) times S**. For entry i,
left is truncated toward zero from **S times(20+70 times column)+origin X**;
top is separately truncated from **S times(10+45 times row)+Y base**.
The integer column/remainder and row/quotient derive from i/columns. Add
float **20 times S** to each truncated left/top for the actual draw centers.
The Y-base addition and later row addition are separate float operations.
The navigation point truncates those final centers again; its integer
coordinates are not substituted for the float draw arguments.

Hover compares the global GUI cursor against the half-open **40 times S**
square at that left/top. Normal size is **S times0.025**, word**0x3ccccccd**
at**0x46d654**; hover size is **S times0.0375**, word**0x3d19999a** at
**0x46d658**. Hover stores the required-item index at+0x2e8 and copies the
item to+0x2ec. It does not change Euler, depth or resource selection.
At**0x26c7b9**, use Euler(225,0,GUI spin), screen-depth**0**, dark**1**.
After all required items and their center registration, restore Plasma
once, then draw result description, requirement quantities/availability
and craft-button UI. No stack quantity produces repeated3D copies.

## 13. Weapon customization's three direct item-image sites

Draw**0x292990** first submits its manual heading, then selects state0,
forms M/S and root-pivot projection P as §11. Its two equipped thumbnails
use original local-creature records directly; they do not apply0x10a720.
They use Euler(225,0,GUI spin), screen-depth**−0.05** and dark**1**.

### 13.1 Equipped thumbnails0x292eaa and0x29307b

Let W/H be queries**0x32da30/0x32ad10** on the customization widget.
The first thumbnail is creature item**+0x6d8**, admitted only when its
category is nonzero. Center is **(W+P.x)−60 times S**, **(H+P.y)−80 times S**;
size is **S times0.075**. If both absolute float cursor-center differences
are strictly less than **50 times S**, multiply size by float**1.25**
(word**0x3fa00000**, data**0x42e71c**) and set hovered-weapon index+0x2c4
to0. This is an open square, not a circular-distance test. Native widens
the already float-subtracted differences to double for sign-bit clearing
and narrows before the float comparisons; it does not subtract in double.
Submit that original item at**0x292eaa**.

If that first thumbnail was admitted, the second thumbnail's horizontal
offset is signed32 truncation of **S times100**, converted back to float;
otherwise offset0. The second source is creature item**+0x638**, again
requiring nonzero category. Its X is the first center X minus this offset,
and its Y is unchanged. It has the same base size/strict hover rule,
sets hovered-weapon index1 and calls0xddbc0 at**0x29307b**. This means the
remaining weapon uses the first position when the other category is empty.
There is no quantity or class-usability admission here. The callback keeps
WORLD active between these thumbnail calls.

The selected-weapon field+0x2c0 identifies0 for+0x6d8,1 for+0x638; its
fallback checks choose the available nonempty record. A selected missing
record skips the subsequent editing/palette image path. The intermediate
interactive weapon/voxel preview is a separate raw-resource draw, not
another direct0xddbc0 site; its complete edit-view geometry is outside
this image-caller audit. Do not infer that no intervening WORLD state is
written before the palette loop.

### 13.2 Material palette source and0x29498a

Each reached draw resets the temporary stack vector at**+0x1f8** to empty.
With a selected equipped weapon, it builds its palette from all canonical
inventory tabs. The first possible entry is semantic-default category**11**,
subtype**10**, with material byte+0x1c copied from the selected weapon.
Its quantity is the signed32 sum of quantities whose original items
compare equal to that candidate through**0x4df60**; append if the sum is
nonzero. Then for material byte values**128 through131 inclusive**, sum all
canonical quantities with category11, subtype**14**, matching material byte,
and append a semantic-default such item when the sum is nonzero. Those
subtype14 sums do not add full-item equality on the other item fields.
Order is that optional subtype10 entry followed by ascending material.
This is a temporary display aggregation, not an inventory grant/removal.

This range corrects the former0-through131 wording: native initialization
is128 at0x293432 and the bound is132 at0x29361d. Full default-record,
signed accumulation and field-only aggregation versus full-equality
consumption facts are **steam-weapon-customization-palette1–3**. The
broader edit-view module remains incomplete until its separate audit.

The vector uses the inventory-style**quantity-first0xa4** layout. For each
palette image, copy the original item at stack+4 into a local item with
**0x35400**, then pass that copy to0xddbc0. There is no player adaptation
step. Reproject the root's live pivot for palette origin. Left/top are
truncated from **P.x+20+45 times(i mod8)** and
**P.y+40+45 times trunc(i/8)**. These offsets/pitches are physical float
pixel additions here, **without multiplication by S**. Center is each
integer left/top plus integer20, then float conversion, as confirmed by
the argument loads before**0x29498a**.

The current GUI cursor inside the half-open40-pixel square stores hovered
palette index+0x2d8 and copies the item to+0x2dc. Size is literal float
**0.15** when current-hovered or equal to selected palette index+0x210,
otherwise literal float**0.1**; neither size is multiplied by S. Pass
Euler(225,0,GUI spin), screen-depth**0**, dark**1**. The palette draws in
vector order with no Plasma restore between entries. Afterward the
callback restores Plasma for the remaining manual upgrade/count labels;
its common no-selected-weapon exit also restores Plasma.

## 14. Complete raw sprite helper0xdf230

The direct caller index identifies SpriteWidget**0x29106f** as the raw
helper's caller.0xdf230 accepts renderer, already-resolved sprite resource,
screen XY, Euler-degrees pointer, uniform size and shader DepthOffset.
A null resource does nothing. There is no item/category/quantity selector,
attribute attachment, automatic substitute resource or internal clock.
The caller's state0 and ordinary resource readiness remain applicable.

Build the model from **T(0,0,1)**, append X/Y/Z Euler rotations through
0x2b360/0x2b470/0x13050, uniform caller size, then uniform **1024/H**.
Finally center all three full signed32 header dimensions at+0x88/+0x8c/+0x90
by their negative halves in that existing basis. The two uniform scales
are distinct float operations. This helper has **no additional max-D
division, short-Z padding, category4 multiplier or category9 rotations**;
SpriteWidget already supplies its raw max-D divisor in §10.3.

Use identity view and the same45-degree literal projection entries and
negative aspect as §3, translating it in NDC by the supplied screen XY,
with projection-translation Z**0**. The helper's final scalar input instead
goes to the WORLD shader **DepthOffset** setter. It makes one colored
resource draw, not an item-style black-plus-colored pair.

Before that draw, explicitly submit **Opacity1, White0, Shininess0, Glow0,
material(1,1,1,1), fog range10000**, the supplied shader DepthOffset, and
clear all point tables with0xce580. Its palette differs from the shared
item palette: front**(1,1,0.8,1)** at**0x43f160**, back and ambient both
**(0.3,0.6,1,1)** at**0x43f180**. Prepare float a=1/sqrtf(stored float1.73,
word**0x3fdd70a4**, data**0x43ef84**), then direction
**(float0.3 times a,float0.8 times a,−a)**. Native transforms this direction
through the inverse of the just-prepared identity view, then the palette
setter normalizes it again. Float0.3/0.8 use words**0x3e99999a/0x3f4ccccd**.
There is no current-day front/back/ambient substitution.

The WORLD sky/fog/sun RGBA, SunPos and fog-gradient translation remain
retained; finite fog, vertex/pixel color handling and final grading still
apply.0xdf230 installs no viewport, target, scissor or depth clear, and
does not restore WORLD constants afterward. SpriteWidget restores the
Plasma pipeline after returning. A later item black pass can therefore
inherit this raw helper's palette/effect values, rather than receiving an
automatic return to the usual item palette.

## 15. Remaining source scope

All eleven direct0xddbc0 call sites in the supported native call index now
have caller-input contracts here: identification §1/8, inventory/cursor §9,
SpriteWidget item §10, preview/upgrade §11, crafting §12 and customization
§13. This is the direct-call index, not a proof about every possible
indirect reference or injected code.

The first quick-button item producer is established in §10.2. The other
three SpriteWidgets are constructed at the **sprite** children of their
quickbuttons, stored at **G+0x3e8/+0x3f0/+0x3f8**, with the same empty item
and null raw-pointer initialization. This bounded native trace has not
established subsequent nonempty item/raw-resource producers for those
three fields, or closed every alias/indirect path that could reach them.
**Do not infer class/ultimate/skill images from their positions, and do
not assert they are permanently unused from the missing direct writers.**
Their exact later producer mapping remains unresolved. The separate main
weapon-customization edit-view geometry also remains outside §13's direct
item-image contract. More generally, this audit has not established a
subsequent nonnull writer of SpriteWidget's raw resource field+0x1e8:
the complete0xdf230 fallback contract alone does not prove its reachability
for a particular shipped equipment/quick-button instance. No game execution, owner assets/captures, SDK or
replacement implementation were used to fill these boundaries.

Independent follow-up **steam-weapon-customization-edit-view1–8** now
specifies the separate VoxelWidget raw main image, root/input/pose,
projection, picking and attribute overlays, plus bounded AdaptionWidget
facts. It does not close the extra SpriteWidget producer questions above.

## 16. Shared GUI step, detail hover coordinates and callback placement

Native follow-up sources are0xa5a50,0x103200,0x349cf0,0x339b70,
0x26bf70,0x28c0a0,0x26c2d0,0x32b180,0x100ee0 and0x32cf50.
No implementation, owner content or capture was inspected.

### 16.1 Producer of recipe-detail step+0x190

The ordinary Game frame first caps its supplied **double elapsed wall
milliseconds to1000.0**, with no lower clamp, at0xa5afc–0xa5b0a.
The high-resolution nanosecond producer/cap/interruption contract is
steam-scene-lighting5.1–5.2. At **0xa7e6f–0xa7e74**, narrow that capped
double to float32 and retain it. The Interface update call **0xab5ce**
receives that float;0x103200 preserves it without another cap or
timestamp subtraction. At **0x106b97**, truncate this float toward zero
to signed32, and pass that integer to0x349cf0 at0x106ba7.

**0x349cf0** first adds this signed32 duration with32-bit integer
arithmetic to GUI clock+0x194, stores it at GUI engine+0x190, then clears
engine field+0x20 and advances its root with0x3358e0. GUI
constructor0x339b70 initializes+0x190/+0x194 to0. These fields are not
narrowed absolute timestamps, not the raw uncapped wall interval, not
world calendar time and not a sum of independently truncated simulation
substeps. Normal finite nonnegative input gives an integer in0..1000;
fractional ms are discarded **after** the float32 narrowing. Preserve
that boundary near whole-millisecond rounding cases.

This update occurs once on each reached ordinary Interface update,
regardless of recipe-detail or InfoWidget visibility. Hiding that one
widget does not freeze the GUI's last-step field or total clock. If its
draw is skipped, its recipe Euler Z does not advance; a later admitted
draw consumes the **latest one step**, not accumulated hidden time. If
the encompassing frame/update is not called, there is no new write by
this producer; the existing Game-frame elapsed cap governs the next
valid frame. No independent widget pause/catch-up clock is inserted.

In **0x26c2d0**, the original result's resource query0x173b10 and null
return precede the Z-angle addition described in12.1. Therefore a missing
result resource also leaves that stored angle unchanged. On a resource-
admitted draw, the Z update precedes the strict result-hover test and
the0x26c58d image submission. There is no draw-local1000-ms clamp in
addition to the producer's cap.

### 16.2 Exact recipe local-pointer query0x32b180

Read cached GUI cursor floats at engine+0x180/+0x184. Obtain the
Widget's **screen frame origin O through0x32cf50**. Independently,
**0x100ee0 on the Widget's root node** builds its inverse-world matrix I:
start from the root transform's cached inverse+0x2e0, or identity if no
transform, and compose ancestor inverses recursively. This is the
inverse of that node's transform hierarchy, including the transforms'
normal pivot/rotation/scale effects. It does not obtain a shape AABB or
replace that hierarchy with an absolute GUI-scale scalar.

Subtract O.x/O.y from the cached cursor components in float32. Apply
only I's **2×2 linear XY part** to that displacement: local X is the
rounded X displacement times I[0], plus rounded Y displacement times
I[4]; local Y is I[1] times the X displacement, plus I[5] times the Y
displacement. These indices use the16-float column-major layout of the
existing Plasma matrix spec. Products and the final additions use
float32. No inverse-matrix translation term or homogeneous divide is
performed here, and the resulting coordinates are not integer-snapped.

The origin query remains the actual Widget frame-origin query: its
bind/frame construction must already be respected. The pointer helper
does **not** separately multiply by the Widget bind matrix, undo its
nine-slice deformation, or subtract the root pivot again. In particular,
“inverse the node's absolute point” and “inverse the deformed drawing”
are not substitutes for this frame-origin subtraction plus linear
inverse operation. Use these outputs for12.1's strict X/Y comparisons.

### 16.3 FormulaDetailsWidget and InfoWidget own draw hooks

Constructors **0x26bf70** and **0x28c0a0** both call0x268a70→0x328ff0.
Base Widget flags+0x150 start at0, and neither derived constructor sets
bit0x200. Their own vtables are **0x46d3c0** and **0x46fb78**, with
draw slot+8 pointing to **0x26c2d0** and **0x28c290**, respectively.
Attaching a copied owner root transfers its old raw Widget geometry,
not its old flags/settings; see steam-inventory-grid-layout7 and
steam-character-list-input-and-frame-init5.

Consequently **0x330b20** places both native manual hooks **after their
root's child traversal** on the usual admitted uncached path, just as
the flags0 InventoryWidget. Own shape/background draws earlier; the
native immediate3D calls and subsequent text occur inside their hook,
not in a later global icon layer. Visibility, excluded-node, cached-
surface, clip and blur rules still decide whether/how that traversal
reaches a callback. This is a constructor/ordinary-route fact, not an
assertion that runtime code can never change a Widget flag.
