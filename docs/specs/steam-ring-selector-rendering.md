# Steam Ring Menu queued item rendering

Independent native-only follow-up to steam-inventory-slot-lifecycle9.
Sources:0xe2e00,0xee5a0,0xe6e60,0xe7980,0xeef30,0x173b10,
0x10a420,0x16eba0/0x16ecc0 and the original executable's constants.
No engine, tests, exporter/build output, SDK, owner assets or captures
were inspected, and no game was executed. This document specifies the
Ring Menu's **item** entries; the same scene input's alternate creature
entry is explicitly outside this bounded contract.

## 1. Source list and admission

Game frame0xa5a50 supplies scene0xe2e00 with its temporary vector of
16-byte entries, displayed radius **G+0x1e90**, displayed angle in
degrees **G+0x1e8c**, and GUI scale **G+0x664**. Construction, canonical
tab/index identity, live item references and shown/empty rules are
steam-inventory-slot-lifecycle9. Every Ring Menu item entry has first
pointer0 and second pointer equal to the canonical slot's item payload,
not its quantity. No copying into a cursor/equipment item occurs.

The scene's list loop is **0xe48d5–0xe4d60**. It uses the input vector's
original count N and visits indices i from0 upward, in vector order.
The item continuation is **0xe4c64–0xe4d27**: with first pointer0, require
the second item pointer nonzero, resolve it with **0x173b10(world+0x70,
original item)** and skip submission if the resource is null. There is
no category0 fallback sprite here. Resource-selection semantics remain
item-models. A missing item/resource leaves a hole at that index: it
does not reduce N or redistribute the other centers. Both-null entries
also produce nothing. A nonzero first pointer instead takes the separate
creature-preview route, not an item route or a Ring Menu candidate.

This scene branch tests the list, not the selector's frame/pivot or
another GUI hit/clip/visibility query. No additional per-item frustum,
screen-rectangle, region, quantity or usability check surrounds the
submission. The caller's established shown/list rules determine whether
the Ring Menu supplies entries; do not invent another admission from
the behavior of direct GUI item icons.

## 2. Centers, index angle, scale and spin

Let W/H be scene signed32 dimensions at renderer **+0x41c/+0x420**,
g the submitted GUI scale, r the submitted displayed radius and a the
submitted displayed angle in degrees. These are scene inputs, not the
current selector node's queried origin or dimensions.

For each original index, calculate the ring phase in **double**: multiply
double(i) by double2π at **0x43ef98**, divide by double(N), add
double(a) times doubleπ at **0x42e758** divided by double180 at
**0x42e778**, then subtract doubleπ/2 at **0x42e740**. The a-to-radians
part is calculated once for the loop. Narrow the final phase to float
for **cosf/sinf**. Native instructions are **0xe4986–0xe4a38**.

Screen X is float(cosine times r), plus float(W) times float0.5.
Screen Y is float(sine times r), plus float(H) times float0.5. Each
product/add rounds to float32. The halves are float multiplications,
**not integer W/2 or H/2**. Radius already contains the caller's scale;
there is no further g multiplication on it here. The selected candidate
at its settled target angle lies at the top of the ring. The−200 offset
used to place the owner selector subtree is not independently subtracted
from these centers; its relation to the settled200×g radius comes from
the caller's separate animation, not an extra scene translation.

The item scale numerator is calculated in this float32 order:
**1024 divided by float(H), then times g, then times0.09**. The stored
constants are **1024 at0x43efcc** and **0.09000000357627869**, word
**0x3db851ec at0x43ef44**. Divide that numerator by float of the maximum
**signed32 full resource allocation X/Y/Z dimensions**, resource
**+0x88/+0x8c/+0x90**. The dimensions are selected as integers before
conversion. There is **no minimum16 floor**, occupied-voxel AABB or
extra per-category scale. The inspected caller has no special zero-size
division guard. Submission is **0xe4d27**.

The submitted Euler angles are **(225°,0°,spin)**. The first two float
words come from **0xe4910**, whose eight-byte initialization contains
float225 followed by float0. Spin is derived from **world float
milliseconds+0x440**, not GUI elapsed time or timeGetTime. Convert that
float to signed32 by truncation toward zero, take its signed remainder
modulo10000, convert the remainder to float, multiply by stored float
**0.00009999999747378752**, word **0x38d1b717 at0x4ac954**, then by
float360 at **0x42e7e4**. Both multiplications round to float32.
The item uses this single spin; the nearby doubled-spin value belongs
to the alternate creature-preview branch. See **0xe48fe–0xe4978**.
Changing selection affects placement through the smoothed displayed a;
it does not give the selected item a separate size, spin or material.

## 3. Queued transform and descriptor,0xee5a0

The helper receives the resolved resource, screen XY, Euler vector,
uniform scale, white material **(1,1,1,1)**, **Glow0**, **Shininess from
0x10a420(original live item)** and **DepthOffset0**. That item-material
helper's complete rule remains steam-gui-item-icons5; it is Shininess,
not Glow or an item-color multiplier. There is no submitted rarity
material tint or offset black duplicate.

At **0xee5d8–0xee655**, start with identity linear part and translation
**(0,0,1)**, append native Euler rotations in **X then Y then Z order**
through0x2b360,0x2b470,0x13050, then apply the submitted scale to all
three basis columns through0x2b580. Each rotation converts its float
degrees by multiplication with float **0.01745329238474369**, word
**0x3c8efa35 at0x42e520**, before cosf/sinf. These are the existing native
rotation conventions; their axis signs are not to be changed merely
because the consuming projection has negative aspect.

Then center the **full allocation**: convert the three signed header
dimensions to float and multiply each by float−0.5 at **0x42e80c**;
transform that offset through the already rotated/scaled basis and add
it to translation. This is a local offset through that basis, not raw
translation-column subtraction of unscaled halves. No occupied-vertex
center/header margin is substituted. For finite affine inputs, this is
the transform described by translationZ1, then X/Y/Z rotations, scale,
then a local negative half-allocation translation. The centering
instructions are **0xee65a–0xee76e**; their output is the queued model
matrix, independent of its pixel-center projection offset.

Material alpha1 selects the **first opaque resource-keyed tree** in
screen queue **renderer+0x18**. The descriptor stores the matrix,
submitted white RGBA, pixel Opacity1, Glow0, the item's Shininess,
White0, DepthOffset0 and its screen center. The record layout consumed
from its linked-list node is:

|Node-relative offset|Field|
|---|---|
|+0x10..+0x4f|float4×4 model matrix|
|+0x50..+0x5f|material RGBA|
|+0x60|pixel Opacity|
|+0x64|Glow|
|+0x68|Shininess|
|+0x6c|White|
|+0x70|DepthOffset|
|+0x74/+0x78|screen center X/Y|

The tree groups by **unsigned resource-pointer value**, not candidate
index, angle, distance, material or screen depth. Its consumer traverses
ascending keys. Within one resource group,0xed7f0 appends descriptors at
the list tail, preserving submission order, including two candidates
which share a sprite. Thus candidate order determines placement but is
not the global opaque draw order. The queue is cleared/rebuilt for each
scene at **0xe2e00→0xeef30(renderer+0x18)**, not retained as inventory
state across frames.

For completeness,0xee5a0's general alpha<1 route appends a120-byte faded
record into the queue's first faded vector: material RGB unchanged with
material alpha1, original alpha used as pixel Opacity, same effects and
center. It does not use the opposite-winding tree/vector. **Ring Menu's
verified white-alpha1 caller does not take that route.** Its animation
radius is not an opacity and introduces no fade-vector submission.

## 4. Queued draw projection, state and ordering

After producing these descriptors, scene setup **0xe4d65–0xe4f22** installs
identity view and a **50° perspective** projection via0xa0340. FOV is
float **0.8726646900177002 at0x437f98**, aspect is negative float(W)
divided by float(H), near is float0.05 and far is float of the signed32
zone-window count+0x438 shifted left6. This is native positive-view-Z,
**clip W=view Z, depth0..1**. It is the screen-overlay camera, not the
direct0xddbc0 item icon's45° camera or gameplay view.

Consumers **0xe6e60(queue+0x18,mode1)** and then
**0xe7980(queue+0x18,mode1)** run at **0xe4f10/0xe4f22**. Mode1 shifts
the base projection per descriptor with screen-center NDC X/Y and
**NDC depth−0.5**, before applying it to the model. X offset is
the float ratio (screenX−float(W)×0.5)/float(W), then added to itself;
Y offset is (screenY−float(H)×0.5)/float(H), times float−2 at0x42e60c.
The shift is on the left of the base projection in the native matrix
convention. It is a clip-space/homogeneous translation, not adding screen
pixels to the model matrix or applying−0.5 to model view Z. Each draw
uses its own submitted XY; the physical-pixel centers are not multiplied
by GUI scale again.

The opaque consumer selects **renderer state0** and ordinary WORLD
shaders, not the water shader or Plasma shader. It clears point tables
when mode bit2 is absent; mode1 therefore **does not select world-zone
point lights**. Per draw, the consumer submits model/projection,
material, Shininess, Glow, pixel Opacity, White and DepthOffset, then
draws the resource's normal voxel buffers. **0x16eba0** admits a resource
group only when its vertex and index buffer objects+0x70/+0x78 exist
and report ready;0x16ecc0 performs its bound indexed draw. A null or
unready resource group is not an alternate icon fallback.

State0 is depth testing and depth writing enabled, **LESS_EQUAL**,
stencil disabled, solid fill, alpha blending disabled and RGBA writes.
Its raster descriptor is **CullMode2(FRONT), FrontCounterClockwise0**,
depth clip enabled and scissor disabled; see0xc3dc0/0xc3150 and
steam-world-pass-controls2. Overlay setup/consumption does not clear
the inherited depth target or allocate a private depth buffer, viewport
or slot scissor. Main target/viewport and frame clear remain
steam-gui-item-icons8.1–8.2. Do not replace the verified state with
depth-disabled GUI drawing. The general faded consumer has its existing
depth-only then blended color phases; this is shared machinery, not an
extra pass of each Ring item.

These queued overlays occur **after terrain, actors, ordinary scenery,
water and later scene effects**, and **before Plasma root traversal**
through renderer virtual+0x10→0xc4af0. The Ring selector's owner
background/brackets/text and other GUI children are consequently drawn
later in ordinary GUI order. Item submission is not a widget callback
interleaved between that subtree's background and text. It is not
automatically clipped/composited by a selector node's Plasma mask layer.

## 5. WORLD lighting/color inputs and attribute boundary

Overlay preparation explicitly writes **white material, Opacity1,
White0, DepthOffset0, Shininess0, Glow0**, fog range **10000** and zero
point lights. It supplies fixed front **(0.7,0.7,0.2,1)**, back
**(0.5,0.5,0.5,1)** and ambient **(0.3,0.3,0.8,1)**, from
**0x43f0a0/0x43f0f0/0x43f140**. Prepare float reciprocal sqrtf(3),
direction (a,a,−a), then the directional setter normalizes it again.
Full frame and per-descriptor setter behavior is
steam-world-pass-controls4. The Ring descriptor then replaces Shininess
with its item value while retaining its explicit Glow0/White0/white
material/Opacity1/DepthOffset0.

**Sky1/Sky2/Fog/Sun RGBA, projected SunPos and fog-gradient translation
remain the frame's existing values.** Changing to identity view and the
overlay projection does not recompute or discard them. The ordinary
WORLD lighting, vertex/pixel saturation, grading, possible positive-
Shininess ripple and finite10000 fog apply as in steam-world-shader-output.
Neither being a selector item nor its material alpha1 bypasses these
stages or implies fullbright. Preserve their distinction from the direct
GUI icon's different fixed direction/palette and optional black pass.

The inspected Ring item branch makes **one0xee5a0 submission per resolved
item**. It does not iterate the original item's spirit attributes,
submit cube sprite0xa46, or repeat the direct0xddbc0 attribute-placement
pass. The queued helper/consumer only knows its resolved resource,
transform and material descriptor; it does not recover an original item
and append spirit cubes later. Thus the direct icon's attribute overlay
must not be added to this Ring path merely because both display items.

## 6. Remaining bounds

The item source/placement/transform/queue/pass contract above is stable.
It deliberately does not specify the alternate first-pointer creature
preview branch or arbitrary other0xee5a0 caller policies. The owner
selector subtree's pivot, shapes, style and text remain owner data under
the existing UI specification. No numeric drawing bounds, artwork or
cross-edition sprite correspondence was inferred or supplied here.
