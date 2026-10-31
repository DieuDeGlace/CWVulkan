# Steam retained item image and WORLD pass-family selection

Stable bounded native-only confirmation/addendum to steam-gui-item-icons
§9, steam-inventory-slot-lifecycle §6 and worldmap-rendering. Sources:
0x27d2b0,0xddbc0,0xee2f0,0xee990,0xee5a0,0xeef30,0xe2e00,
0xe64b0,0x2f44a0,0xe6e60,0xe7980 and0xc3150/0xc3dc0, with
the native binary constants. No engine, tests, exporter, build, SDK,
owner assets/captures or reconstruction were inspected; no game was run.

## 1. The retained image is an embedded player record

The image caller **0x27d2b0 at0x27f543** reads the current local creature
through **Game+8→World+0x448**, not from the widget's bound collection.
The source is the embedded **164-byte retained cursor stack**:
signed32 quantity at **creature+0xa08**, original160-byte item at
**creature+0xa0c**. It is not the hovered inventory slot, selected quick
item, Ring candidate, an equipped item under the pointer, or a newly
copied draw-local stack.

Admission is **quantity!=0**, including negative quantities. There is
no caller category!=0, class-usability, positive-count, hovered-cell,
pointer-inside-widget, physical-held-button or last-used-device gate.
The original embedded item address is passed directly to0xddbc0.
Resource selection/missing-resource behavior stays in that shared helper;
in particular category0 has its existing catalog0xcd1 fallback, not a new
caller filter. Failure to draw a resource does not empty the source stack.

The known source writers and lifetime are already bounded in
[slot lifecycle §§6.1–6.5](steam-inventory-slot-lifecycle.md): constructor
0x4ce80 initializes empty;0x83540 moves/copies an existing cursor along
with inventory, and the identified class-kit/character-read inputs are
fresh empty/default records;0x9ea10,0xa27b0 and0x2e17d0 copy an existing
source record. The character file's ordinary tab fields do not serialize
this extra cursor stack.0x8ae50 can queue a one-item outside-GUI drop and
deplete it, under its already specified held-edge and GUI-capture gates.

**There is still no verified reachable ordinary input producer that
changes an empty retained cursor to nonempty.** These confirmed display
and depletion consumers do not authorize drag grabbing, splitting,
put-back, swapping or an invented save field. Existing-copy producers
are not an origin for the copied nonempty value. The precise first-write
oracle still needed for that question remains slot-lifecycle §6.4.

The cursor belongs to the creature/inventory, not the image. Hiding a
panel, skipping its callback, or resetting renderer queues does not clear
it. This callback samples the current record and draws immediately;
it neither owns another retained copy nor leaves a queued item pointer
for a later frame. General tab reset0x64a90's documented non-reset of
this record remains distinct from character replacement with a fresh
empty inventory.

## 2. Callback admission, exact image inputs and layer

This common callback serves Inventory, Shop and Crafting-list instances.
It returns early for a **null bound collection at widget+0x1e8**, before
the cursor tail. With a nonnull collection, a selected tab at or above
the current tab count skips grid drawing but still reaches the common
tail. Modes0/2/3 do not select different cursor records or suppress
this tail. Multiple admitted instances can therefore draw the same
current player cursor, each at its own point in GUI traversal.

InventoryWidget construction0x27c950 does not set Widget flag0x200.
Its ordinary own callback consequently runs **after its root shape and
children**, subject to the existing ancestor/own visibility, clipping,
blur/cache and callback admission. Within0x27d2b0, the cursor tail follows
the cell icons, manual GUI text/decorations and tab-control state work.
It is not a global cursor layer appended after every menu or sibling.
Later admitted GUI siblings continue drawing afterward. See
[grid layout §7](steam-inventory-grid-layout.md).

At **0x27f550–0x27f559**, select renderer **state0** again. The image
call **0x27f5fa** supplies:

* Center X/Y: the **current GUI engine float32 pointer at+0x180/+0x184**,
  directly. No integer screen truncation, frame-origin addition, widget
  M00, extra GUI scale or physical/fixed64 player/map coordinate enters
  this center.
* Euler degrees: **(225,0,spin)**. Take GUI engine **signed32 absolute
  clock+0x194**, signed remainder10000, convert that remainder to float32,
  multiply by stored float32 **0.0001** (word0x38d1b717,0x4ac954), then
  multiply by float32 **360** (0x42e7e4). Keep the two float products.
  Do not substitute elapsed-frame time, world time or positive-normalized
  modulo for a negative signed clock.
* Size: literal float32 **0.05000000074505806**, word0x3d4ccccd at0x42e524.
  No hover enlargement or category9 caller-size multiplier is added here.
* Caller screen-depth translation **0**, dark-offset pass enabled **1**.

The shared helper's full-header centering, category-dependent rotations/
divisor,45-degree shifted projection, resource readiness and optional
attribute cubes remain [GUI item icons §§1–6](steam-gui-item-icons.md).
The caller does not replace them with the queued Ring transform. Its dark
and colored passes remain immediate WORLD draws under state0; inherited
black-pass constants and explicit colored-pass resets are §§8.1–8.3 there.
No cursor quantity label follows the image.

At **0x27f5ff–0x27f606**, restore Plasma through the **GUI engine's**
virtual+0x80 and return. That GUI pipeline restore is different from a
renderer virtual+0x80 call. It does not clear depth or reset retained WORLD
constants. Ordinary target/clip admission remains the existing GUI icon
contract, including its separate offscreen-layer cases.

## 3. Queue context versus winding family versus opacity

Let R denote the native renderer passed to0xe2e00/0xe64b0. These are
different full descriptor contexts within it:

|R-relative context|Use|
|---|---|
|+0x138|Ordinary world context|
|+0x1d0|Second world context, including the documented zone-model flag0x08 route|
|+0x268|3D world-map context|
|+0x18|Late screen-overlay context, including queued Ring images|

**The second world context is not an opposite-winding list.** Every
context independently contains both winding families. Relative to a
chosen context, normal opaque tree head/count are **+0/+8**, opposite
opaque tree head/count **+0x10/+0x18**, normal faded vector
**begin+0x20/end+0x28/capacity+0x30**, and opposite faded vector
**begin+0x38/end+0x40/capacity+0x48**.

The actual family selection is explicit in the producer:
**0xee2f0 writes the normal family;0xee990 writes the opposite family**.
Both require separate pixel opacity>0. Within the already chosen family,
opacity **at least1** inserts into its opaque resource-keyed tree;
**strictly between0 and1** appends its faded vector. The helper does not
clamp opacity down to1 before that test. This is not material alpha or
an automatic switch based on the matrix determinant, map mode, GUI scale,
sprite category, White, Glow or GPU readiness.

**0xee5a0**, used by the queued screen-sprite path, likewise writes the
normal family. Its separate material-alpha→pixel-opacity conversion is
steam-ring-selector-rendering §3. Direct0xddbc0 item images bypass these
queues and retain the state explicitly chosen by their caller.

## 4. Native consumer conditions and pipeline states

**0xe6e60** selects state0, consumes the normal opaque tree, then selects
state1/consumes the opposite tree only when its **count+0x18 is nonzero**.
Each opaque resource must pass readiness **0x16eba0**; that checks the
resource's vertex/index buffer pointers+0x70/+0x78 and their readiness
methods. Readiness rejects drawing; it does not transfer a descriptor
between winding families. Resource groups traverse ascending unsigned
resource-pointer keys; list entries inside each group preserve append
order.

**0xe7980** conditionally runs each nonempty vector in this order:
normal depth-only state3, opposite depth-only state4, normal color state6,
opposite color state7. Vector nonempty means its own begin!=end. A vector
does not select another family when empty, and these vectors are not
distance-sorted by this consumer. Their0x16ec60 resource draws retain
the separate ordinary buffer-readiness check.

The mode bits passed to these consumers do not select winding: bit0
adds the existing screen-center projection shift; bit1 enables the
existing transformed-header-center world point-table lookup. In
particular map mode0 versus screen-overlay mode1 is not the normal/
opposite distinction.

Native state creation **0xc3150** and selection **0xc3dc0** establish:

|State|Winding family|Color writes/blending|
|---|---|---|
|0|Normal|RGBA, no blend|
|1|Opposite|RGBA, no blend|
|3|Normal|No color writes, depth-only|
|4|Opposite|No color writes, depth-only|
|6|Normal|RGBA, source-alpha/inverse-source-alpha add|
|7|Opposite|RGBA, source-alpha/inverse-source-alpha add|

All six bind the **same enabled depth test, writes enabled, LESS_EQUAL,
stencil disabled** (descriptor initial words1,1,4 at0x438740). Color-pass
depth writes are not disabled for faded models. Normal raster uses solid
fill3, **CullMode2=FRONT, FrontCounterClockwise0**; opposite keeps
CullMode2 and changes **FrontCounterClockwise to1**. Both retain depth
clipping on and scissoring off. Do not replace opposite winding with a
different native CullMode value. State3/4 have color mask0; state6/7
have mask15 and alpha-channel one/zero add. The ordinary WORLD shader
pair is bound through0xc5de0 for all six; neither family selects water
or Plasma shaders. These numeric D3D states require the replacement
API's independently correct projection/winding adaptation.

### 4.1 Surviving screen orientation under normal state0

This is the orientation implied by the verified **FrontCounterClockwise0,
CullMode2=FRONT** state, not a replacement-API enum prescription or a
runtime image capture. Use native render-target coordinates: **X increases
right, Y increases down**, with the ordinary viewport's top-left(0,0)
and positive width/height from **0x135310**. The native map-to-screen
projection convention likewise negates NDC Y; see map-markers §0.7.

For a nondegenerate triangle after its complete projection, perspective
divide and viewport mapping, **clockwise on that displayed screen is
front-facing and is culled**. **Counterclockwise on that displayed screen
is back-facing and survives this cull test**. Depth/clipping/coverage
remain separate tests; surviving culling alone does not promise a pixel.

For an explicit sign convention, the signed doubled area is
**(Bx−Ax)(Cy−Ay)−(By−Ay)(Cx−Ax)** in those Y-down screen coordinates.
Positive is clockwise and rejected by state0; negative is counterclockwise
and retained. Thus supplied screen vertices **(10,10),(20,10),(10,20)**
are rejected; **(10,10),(10,20),(20,10)** survive the cull test. These are
arithmetic examples, not owner geometry or observed captures. Zero-area
triangles are outside this nondegenerate orientation statement.

If instead expressing the same projected triangle in native NDC with
**X right, Y up**, the viewport's Y reflection reverses the algebraic
area sign: normal state0 retains **positive** signed area in that
coordinate convention. State1's opposite FrontCounterClockwise1 setting
reverses these admissions. States3/6 share state0's winding;4/7 share1's.

The native negative projection aspect is already part of the complete
projection used for this result. Any model/view reflections and any
replacement viewport/projection Y conversion must be included before
comparing final orientation. An unprojected voxel face's index order or
its normal does not by itself establish that final sign. This statement
adds no automatic map/GUI family switch or guessed replacement cull rule.

## 5. What the ordinary map actually populates

At **0xe30d6**, scene0xe2e00 resets the entire **R+0x268** map context
through **0xeef30**. This empties both trees and both vectors, resets its
screen center, and writes **identity** to its context matrix+0x50..+0x8f.
The ordinary map wrapper0xe64b0 and content producer0x2f44a0 do not
replace that base matrix.0xee2f0 reads/precomposes it without modifying
it. Thus identity here is a verified ordinary-call-path fact, not a
matrix guessed from a screenshot.

All seven queued model callsites in **0x2f44a0** use0xee2f0:
**0x2f4d8d,0x2f5780,0x2f5b59,0x2f6db6,0x2f6e27,0x2f7aa2,
0x2f7c2c**. These cover terrain/castles/border and the later marker/route
model submissions already specified in the map family documents. They
do not use0xee990, expand into a full actor-renderer submission, or
write the map context's opposite collections.

The ordinary map therefore consumes **normal opaque state0**, then
**normal depth state3 and normal faded color state6 when its normal
faded vector is nonempty**. States1/4/7 belong to the general consumer
but are not selected by an ordinary map's empty opposite collections.
Both minimap/full-map use this same family choice. Border pulse0..0.8
enters the normal faded vector; the separate terrain-fade threshold
selects normal opaque versus normal faded terrain, not another winding.

The wrapper calls0xe6e60 and0xe7980 at **0xe67df/0xe67f1**, with mode0,
after current map camera/lighting setup and before later GUI drawing.
It adds no map-private depth clear. Its renderer virtual+0x80 call at
**0xe66ac** resolves to **0xc59e0**, the fog-range setter forwarding to
0xc6bb0; it supplies the existing map fog range20000. It must not be
misread as the GUI engine's identically numbered Plasma restore slot
or as a depth clear. The wrapper's state binds and queue consumers also
do not clear depth. The normal frame clear and inherited target contract
remain GUI item icons §8 and steam-world-shader-output.

## 6. Remaining limits

The retained cursor's positive empty→nonempty input origin is genuinely
unverified; the already specified first-write oracle is required to
close it. This supplement does not enumerate all full-actor body-piece
decisions that deliberately call0xee990: those are separate producers,
not a missing selection inside the audited map or retained-image paths.
Owner sprite geometry and actual GUI sibling/artwork data remain owner
inputs. No new global image layer, cursor input action, automatic cull
reclassification or replacement resource is inferred.
