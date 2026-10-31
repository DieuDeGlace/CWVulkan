# Steam weapon customization: panel, pose, raw edit view and picking

Independent native-only behavioral facts. Primary sources:0xf9ac0,
0x101bd0,0x292380,0x292990,0x292650,0x294c50/70/a0/c0,
0x97880,0xa5a50,0xdf0b0,0x16f230 and0x109960. The separate
stable palette checkpoint is **steam-weapon-customization-palette**;
direct thumbnail/palette image contracts remain steam-gui-item-icons13.
No engine, tests, exporter/build, SDK, owner assets or captures were
read, and no game was executed. Fields are byte offsets; addresses RVAs.

## 1. Owner scene and persistent state

Native RTTI identifies **VoxelWidget**: vtable0x470ae0, locator0x4b0f78,
type descriptor0x54ba08. Its constructor **0x292380** is installed at
**Game+0x368 / Interface+0x250** by0xf9ac0. Interface clones owner
gui.plx **blackwidget** under the main game UI node **G+0x220**, attaches
this custom wrapper and requests **800×800** through0x32c9a0, then hides
its root through0xa3a20. Constructor selects owner **resource1.dat**.

Preserve the owner root/descendants, transforms, visibility, drawing,
bind/minimum/deformation and ordinary initialization. The replacement
root Widget receives the old cloned root's raw geometry through
0x334410/0x329c70, not an art-AABB fit. It retains fresh Widget flags0;
0x292380 does not set0x200. Consequently its **0x292990 draw hook runs
after children** in an ordinary admitted uncached0x330b20 traversal.
Its initialization hook is base0x32b830; size hook and per-animation
slot+0xd8 are empty0xe8a20. No extra SpriteWidget is constructed as the
main edit image: this hook draws a resolved raw resource directly.

Interface layout **0x101bd0 at0x10264a** places this root through
**0x334700**, using anchor **(viewport signed-integer half-width minus
half the customization parent-relative width, viewport signed-integer
half-height minus the ordinary inventory parent-relative height)**,
with separate offset **(0,−50)**. Width/height queries are0x32da30/
0x32ad10 and include their normal binding/ancestor behavior. The helper
is the established point-and-offset placement, not a frame-origin
moveTo. GUI scale/viewport coordinate basis is the same shared layout
contract as steam-identification-ui2; do not infer an800-pixel screen
rectangle from the size request or insert a new rotation/centering pose.

Important persistent fields:

| Widget offset | Meaning / constructor value |
|---|---|
|+0x1e8/+0x1ec/+0x1f0|float X/Y/Z edit Euler degrees, **(−90,0,90)**|
|+0x1f8/+0x200/+0x208|temporary quantity-first palette vector, initially empty|
|+0x210|signed selected palette index,−1|
|+0x214|semantic item used for current edit/detail feedback, initially empty|
|+0x2b8|borrowed Game pointer|
|+0x2c0|selected equipped record index,0|
|+0x2c4|hovered equipped thumbnail index,−1|
|+0x2c8/+0x2cc/+0x2d0|signed32 picked/placement XYZ,0|
|+0x2d4|byte hit-valid flag,0|
|+0x2d8|signed hovered palette index,−1|
|+0x2dc|semantic hovered-palette item, initially empty|
|+0x37c|byte carrying an existing upgrade cube,0|
|+0x380/+0x384/+0x388|remembered carried cube XYZ, written on pickup|
|+0x38c|remembered carried material, written on pickup|

Carried-position/material fields are not newly guaranteed initialized
until their producing pickup. Do not use them with the carried flag0.
The names “carried cube” and “selected palette” describe this edit
state, not the player's general inventory cursor stack.

## 2. Opening, closing and click order

The verified opening/toggle producer is **0x98180 object type0x68**, with
the existing not-owned-by-local-player admission and reached Use press.
It publishes native prompt **Customization Bench**; on the press, first
**0x294c50** sets selected palette−1 and carrying0, then toggle this
same root's visibility. The reset happens on either direction. It does
not reset Euler pose, selected equipped index or copy a new weapon.
Clearing carrying does not itself remove/refund an attribute.

The common movement-close block in **0xa5a50** also hides G+0x368 and
G+0x370 among its panel roots. It is admitted unless map+0x668 and
Game+0x1958 are both nonzero. Within it, close when any held movement
action byte **G+0x1a75..0x1a78** is nonzero while chat editing is off,
or when left-movement vector length0x2b630(G+0x1d6c) is **strictly
greater than float0.01**, word0x3c23d70b at0x437f74. The stick clause
is outside that chat test. These simple hides do not invoke0x294c50
or reset the edit pose/carry state themselves. Other generic menu
closing routes are not newly exhaustively enumerated here.

Selection0 means current local creature item **+0x6d8**;1 means **+0x638**.
**0x294cc0** returns that live address only while category!=0. It neither
copies the item nor adds a category3/class gate. In draw0x292990, retain
a valid nonempty selected record; otherwise the nonempty thumbnails'
fallback chooses0 first or1 if needed. Both empty skip the edit/palette
path. Hovered-thumbnail index resets−1 at the reached draw's thumbnail
stage; direct image geometry and hover order are gui-item-icons13.1.

On the **reached common left-down tail of0x94900**, after its ordinary
inventory/crafting work, require customization shown and0x294cc0 nonnull.
The priority is: cached hovered palette index≥0 → **0x294c70**; otherwise
cached hovered thumbnail index≥0 → **0x294ca0**; otherwise **0x292650**.
These are cached draw-produced fields, not fresh picking by the click.
Earlier handler early returns still apply; this is not an unconditional
global down handler. There is no corresponding0x965e0 customization
commit or new release latch in this path.

0x294c70 toggles selected palette between the hovered index and−1,
and requests sound0x58.0x294ca0 copies hovered thumbnail to selected
equipped index and requests0x58. Neither clears carrying or resets pose.
Switching weapons does not retain an independent old-weapon pointer
with the carried coordinates; subsequent operations resolve the current
selected equipped record again.

Attribute placement/pickup/move/drop in0x292650 is loot-crafting4 and
slot-lifecycle3.4, with palette's stricter consumption rules in the
separate palette spec. For its no-hit/no-weapon branch, the GUI query
**0x33f1e0** returns current captured node engine+0x168 when nonnull;
otherwise it performs GUI-tree hit0x333f60(root+0x140,cursor+0x180,flags0).
It is a node query, not merely “another Widget is under pointer.” A
null result plus carrying invokes old-coordinate removal before clearing
selection/carry. Otherwise that branch just clears selection/carry.
Native assumes a still-valid selected weapon for that removal; unusual
weapon disappearance does not establish a safe null-item deletion API.

## 3. Pose producers: persistent edit angles, not thumbnail spin

**0x97880** first routes ordinary GUI pointer polling through0x101160.
After its binding-capture return gate, customization shown plus physical
GUI **right button held** (engine+0x1a8 bit1) changes the edit pose using
cached current-minus-previous GUI cursor floats. X angle subtracts Y
delta times float(cameraSpeed G+0x1bac), then times float0.005; Z angle
adds X delta times that speed, then times0.005. The operations and
stored additions/subtractions are float32. This branch does not use
invertY, frame dt, a drag-start latch, angle wrapping or pitch clamp.
There is no extra “pointer hits the edit image” predicate here.

The Game frame's right-stick pass instead adds to these X/Z fields
while the panel is shown: X receives the cubic rightY term and Z the
cubic rightX term, under the exact0.4-degree/ms/target-distance scaling
and capped-double clock of **steam-gameplay-camera3.2**. Cube and rate
product are float before widening; add in double to the old float and
narrow back. Y angle has no writer in these two pose routes. Constructor
values survive bench reset/toggle. Thumbnail/palette auto-spin continues
to use GUI total clock as previously specified, independently of this
main pose.

## 4. Main edit image: model, projection and physical center

After selected-weapon and palette preparation, resolve the original
equipped item with **0x173b10 at0x293770**. Null skips raw image/picking
and continues the palette image path. In particular it does not execute
the later hit-valid reset; old hit/coordinate state is not implicitly
cleared by that missing-resource branch.

For a valid sprite use **full signed32 allocated dimensions** X/Y/Z at
resource+0x88/+0x8c/+0x90. Let D be their signed maximum. At0x2938d0–
0x293989, build the main model from **translation(0,0,1)**, followed by
stored X/Y/Z degree rotations, uniform scale **float0.35** (word
0x3eb33333 at0x4328a8), then separate uniform scale **float1/float(D)**,
then negative half-dimension local translation. This centers the full
allocation, not occupied vertices. There is no16-unit floor, automatic
item-category multiplier, added1024/window-height factor, root M00
scale, plant sway or GUI spin in this main model.

The view is identity. The base projection is native **45-degree vertical
LH perspective, negative aspect W/H, clip Z0..1**. It computes float
f=1/tanf(stored floatπ/8 at0x43ef54); X diagonal is−f/(floatW/floatH),
Y diagonal f; Z terms are the literal float words **0x3f800347** and
**0xbdccd20b** at0x43ef74/0x43efe0, with W=positive view Z. They are
the usual near0.1/far1000 values but the stored words, not a different
recomputed projection, are authoritative. Native uses the GUI engine's
physical window dimensions+0x1c4/+0x1c8, not an edit-rectangle viewport.

Reproject the root's live pivot through its parent-world×local matrix,
using float reciprocal homogeneous W, exactly gui-item-icons11's P.
The intended physical screen center C is **P.x+0.5×queried parent-
relative widget width**, **P.y+0.6×queried parent-relative widget height**,
with float0.6 at0x42e708. The queries are0x32da30/0x32ad10. This is not
screen frame origin plus half its height.

At0x293a82–0x293ae1, compute clip translation X by subtracting float
windowWidth×0.5 from C.x, dividing by float windowWidth and doubling
by addition. For Y subtract float windowHeight×0.5 from C.y, divide
by float windowHeight and multiply by−2. Z translation is0. Prepend
that clip translation to the base projection. No local viewport,
widget-sized depth clear or widget rectangle scissor is installed by
this construction or by raw draw helper0xdf0b0. Existing caller/Plasma
target, clipping/layer and depth context remain gui-item-icons8.

## 5. Object-space pointer ray and native voxel traversal

Picking **0x293b94–0x294215** inverts the actual composed projection×
identity-view×main-model matrix. From cached GUI cursor floats make
NDC X by cursorX/float(windowWidth), doubling by addition, then minus1;
NDC Y is1 minus twice cursorY/float(windowHeight), again divide before
doubling. Inverse-transform **(ndcX,ndcY,0,1)** and **(ndcX,ndcY,1,1)**;
each point uses float1/W before multiplying XYZ numerators. The first
is the object-space ray origin; normalize second-minus-first through
float squared sum/sqrt/reciprocal to get direction. There is no separate
Widget-frame or shape-hit admission before this ray construction.

At0x293ec0–0x293ef8, initial integer cell coordinates truncate the origin
component toward zero and then subtract1 whenever that original float
component is negative. Thus an exact negative integer also subtracts1;
do not silently replace this with a mathematically different floor.
Start traveled ray distance0, face-normal tuple0, and explicitly clear
hit-valid+0x2d4 at0x293f0e.

Each visited cell first tests the base sprite's raw RGB volume. Its
address is resource+0x68 plus **3×((z×sizeY+y)×sizeX+x)**; outside the
full allocation uses the global empty RGB reference **0x55284c** instead.
**0x16f230(...,0)** treats equality to that empty reference as empty,
and also the exact RGB marker triples **(255,0,0),(0,255,0),(0,0,255)**.
Other RGB triples count as a hit. The reference lies in the image's
zero-filled data area; this audit does not assert absence of all possible
later indirect writes to that global. This is the sprite-empty/marker
predicate, not a terrain block-type/collision test.

Then visit original weapon attribute entries in stored order. A carried
entry at the remembered XYZ is skipped by comparing the three coordinate
bytes. Any other attribute at the current cell's **low8 coordinate bytes**
also establishes a hit, even outside the base allocation. It writes
detail item+0x214 to category11/subtype14/material=that attribute material,
rarity2; the rest retains this draw's semantic-default clear. Attributes
are considered before deciding to stop, even when the base voxel hit.

If no hit, advance to the nearest next cell-boundary crossing. Boundary
distances use current ray position origin+direction×traveledDistance,
and division by that axis's direction. A positive direction uses the
cell's upper boundary; zero directions are skipped. Start candidate
distance10 and X as tie choice; only a **strictly smaller** Y then Z
candidate replaces it, so equal crossings prefer X, then Y, then Z.
Advance only that cell axis by its direction's sign and set outward
face normal opposite the step. Add that boundary distance to traveled
distance. Continue only while distance **less than1000**; reaching1000
ends before testing the next cell. The constants10 and1000 are at
0x42e598/0x42e5d4. This is native cell traversal, not picking triangle
ink or the dark thumbnail's enlarged geometry.

At a hit or distance termination, publish the current signed32 cell
tuple to+0x2c8/+0x2cc/+0x2d0. Hit flag remains false on distance
termination. The following base raw image draw at **0x294258** uses the
main model, identity view, shifted projection and white RGBA material.

## 6. Candidate and existing attribute overlays

The base image is drawn first. If the **previously stored palette hover
index+0x2d8 is negative** and either carrying or current selected palette
is valid, submit a candidate cube. Only later, at0x294741, does this draw
reset/recompute palette hover for its palette images. Do not replace
that old-hover gate with the newly calculated result of the later loop.

Copy the main model for the candidate. With hit-valid, first add the
outward face normal to the **stored**+0x2c8 tuple, then post-translate
the copy by those coordinates in the main model's basis. This tuple
mutation is observable by the subsequent attribute highlight and click
consumer. Without hit, translate by **ray origin plus normalized
direction×100**, float100 at0x42e5bc; hit flag stays false. Candidate
material is carried material when carrying, otherwise selected palette
material. Use original catalog resource **0xa46** if present; null
resource makes the raw helper do nothing, not synthesize new geometry.

Then render original attributes in stored order, skipping carried-
coordinate matches. Each begins from a fresh main model and receives
local translation by its **signed8** XYZ through0x2b7a0. Use that same
owner catalog resource0xa46. When hit-valid and its coordinate bytes
equal the current stored picked/placement tuple, append **translate
(0.5,0.5,0.5), uniform scale1.1, translate(−0.5,−0.5,−0.5)**. Use pulse
factor **1+0.2×cosf(float(GUI signed32 clock+0x194)×0.01)**, with the
multiply/cos/multiply/add in float32, to scale the white input RGBA.
Other attributes receive white RGBA. This is not a separate glow pass.

Every candidate/attribute color goes through **0x109960** with its
material and a literal float fourth argument0. For this reached case:

| Material | Base RGB |
|---|---|
|1|(0.7,0.7,0.7)|
|2|(0.4,0.3,0.2)|
|3/default|(0.5,0.5,0.5)|
|5|(0.1,0.1,0.1)|
|7|(0.9,0.9,0.9)|
|11|(1,0.7,0.2)|
|12|(0.8,0.8,0.85)|
|128|(1,0.5,0.1)|
|129|(0.3,1,0.5)|
|130|(0.3,0.5,1)|
|131|(0.8,0.8,1)|

For ordinary materials multiply base RGB by input RGB and use input
alpha. For128–131 the helper's special branch uses the listed fixed
RGB and input alpha when fourth argument0: it does **not** multiply
those RGB by the pulse input. A highlighted spirit attribute therefore
retains that special material rule. Do not substitute a generic
rarity tint or introduce an invented Glow bit.

## 7. Raw draw helper state and overall draw order

**0xdf0b0** accepts renderer, resolved resource, caller model, view,
projection and material RGBA. Null resource returns immediately. It
submits those matrices and explicitly sets **Opacity1, Glow0,
Shininess0, White0, DepthOffset0, supplied material, fog range10000**,
then clears point-light tables with0xce580. No item lookup/adaptation,
black duplicate, allocation recentering or automatic attribute attachment
occurs inside this helper.

Its fixed palette input is front **(1.1,1.1,0.9,0)** at0x43f010,
back **(0.5,0.6,0.8,1)** at0x43f150, ambient **(0.4,0.4,0.4,1)** at
0x43f0d0, with direction **(0.2,−0.4,−1)/sqrtf(float1.2)** before the
normal renderer palette setter. It does not reset WORLD sky/fog/sun
RGBA, SunPos or fog-gradient translation. The normal WORLD shader and
its material-alpha/color rules still apply; this is not an unlit flat
GUI material merely because White/Glow are0.

The hook's reached order is manual heading; select renderer state0;
equipped thumbnails; selected-weapon fallback; palette aggregation and
edit feedback preparation; raw resource/model/picking; base raw image;
candidate cube if eligible; existing attribute cubes; palette item
images; restore Plasma; manual upgrade-cap/count feedback and palette
quantity labels. It retains WORLD between the raw resource/overlays
and palette item helpers. Each raw call uses the same helper palette
and resets its own listed constants. The next shared item black pass
can inherit those retained WORLD values as gui-item-icons8 describes.
The whole hook remains inside its after-children owner-tree callback.

The detail item+0x214 is semantically cleared at the draw's start. Before
the main pick, a negative prior palette-hover index allows selected
palette item copy into it, or constructs carried category11/subtype10
when material<128, otherwise subtype14. Picked attributes can overwrite
it as section5 states. This is distinct from the later hovered-palette
copy+0x2dc. Selection index≥rebuilt palette size is clamped to size−1;
there is no general repair of more-negative selected values. Raw
resource failure skips picking/hit reset but still reaches the palette
hover reset/images, preserving that stage distinction.

## 8. Bounded separate G+0x370 panel facts and remaining limits

This is a different native **AdaptionWidget**, RTTI vtable0x46cc70,
locator0x4afc10, descriptor0x54b420; its literal heading is **Upgrade**.
Constructor **0x267780** is installed at Game+0x370 / Interface+0x258.
It clones blackwidget under G+0x220, requests **500×450**, hides it, and
clones owner **itemframe** and **rightarrow** under that root, with
independent transformation/display copies. Widget+0x1e8 starts as a
null borrowed item pointer; Game pointer+0x1f0, itemframe+0x1f8 and
arrow+0x200. It retains flags0/after-children draw0x267960. Size hook
**0x2687f0** places itemframe at((local effective width−itemframe parent-
relative width)/2,50), arrow at(local effective width/2−10,230).
Layout **0x1026ea** places the root beside inventory using the same
anchor/offset as identification: half viewport width plus half inventory
parent-relative width, half viewport height minus inventory parent-
relative height, separate(10,−50).

With this root already shown and shop not taking priority, reached
inventory/equipment down routes in0x94900 assign the borrowed **original
item address** for categories3–9 or26, rather than changing equipped
selection of G+0x368. Canonical source is live slot+4; equipped source
is the resolved current equipment record. Existing source precedence/
guards are slot-lifecycle6/7. No stable item identity/remap is supplied
if canonical sorting/insertion later invalidates a borrowed address.

Draw0x267960 shows/hides the arrow according to whether item pointer
is nonnull, renders the established gui-item-icons11.2 image and
ordinary item details/comparison, and draws manual **Upgrade/Goodbye!**
footer labels and cost. Left/footer predicate **0x268670** requires
nonnull item, price **trunc32(float32(0x109d30(item)))≤local gold**, then
strict local pointer **0<X<effectiveWidth×0.5×S** and
**(effectiveHeight−30)×S<Y<effectiveHeight×S**. Right/footer predicate
**0x268540** instead requires **effectiveWidth×0.5×S<X<effectiveWidth×S**
and the same Y interval. Here S is root parent-world×local M00; pointer
XY is exactly0x32b180, as gui-item-icons16.2. No positive-price gate
or blanket root-frame rectangle replaces these strict native tests.

On reached shown-panel **left release0x965e0 at0x96db7**, a true left
predicate invokes **0x267930 at0x96dc7**. Native0x267930's complete44-byte
body only returns0 for null item or obtains0x109d30, converts integer
price to float32 then truncates to integer, and returns. The caller
does not use that return to charge gold or alter the item. Do **not**
invent an upgrade transaction for this button. The subsequent right
predicate0x96dd3 hides only this root when true. These release branches
have no matching identification-style pressed-state gate.

The actual normal-UI **show producer for G+0x370 remains unverified**
by this bounded audit. Construction/hide, selection consumers and
footer appearance do not prove a reachable opening action. No general
assertion about all indirect writers is made. Exact remaining caption/
footer typography and all parent input early-return combinations are
not exhaustively restated here. Missing extra quick-button/raw
SpriteWidget producers remain gui-item-icons15; this direct VoxelWidget
raw edit path does not close those separate alias/producer questions.
