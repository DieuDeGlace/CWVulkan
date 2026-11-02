# Steam HUD compass, context item and special-item models

Independent native-only bounded contract for the HUD submissions in
0xe2e00, their resource selection and0xee5a0 →0xe6e60 consumption.
Sources are original native decompile/binary constants, catalog
initializer0x177f50 and independent prose. No engine/tests/exporter,
owner files/data/captures, SDK/reconstruction implementation or game
execution was used. Existing specifications are unchanged.

This expands [HUD money §5](hud-money.md) and gives authoritative typed
effect meanings for this same block, including the adjacent coin. The
older placeholders `n2`, `glow`, `n4` and five-number shorthand must not
override the producer/descriptor mapping in§2 below.

## 1. Admission, inputs and submission phase

Let R be the current scene renderer, World the pointer R+0x08, P the local
creature World+0x448, g the scene's **float argument14**, and W/H the
**signed32 physical viewport dimensions R+0x41c/+0x420**. GUI scale is
already applied by the centers/scales below; they are not node-local
positions awaiting another global GUI transform.

At **0xe4138–0xe414a**, this HUD block requires **World+0x210 nonnull**
and **scene draw flag0x20 clear**. It first calls map/HUD preparation
**0xe64b0 at0xe41a2**, then submits compass, coin shadow, coin body,
optional context shadow/body and accepted special-item shadow/body pairs.
All these submissions use the same screen-model context **R+0x18**.
The subsequent Ring list loop begins at0xe48d5 and is a separate producer.

There is no additional compass/context/special test here for a Plasma
Widget being shown, menu/map-open state, pointer hit or root visibility.
This is the reached scene producer's admission, not an inference that
every possible caller always supplies flag0x20 clear. F4's independent
GUI-root visibility operation remains
[hidden-interface §1](steam-preview-hidden-interface-order.md).

**Normal Game-frame flag0x20 derivation, independently verified:** this
flag does incorporate the F4-toggled global root's own visibility. While
preparing the actual scene call, Game0xa5a50 loads **G+0x20 →engine+0x140**
at **0xad775/0xad779** and queries that node with **0x9af00 at0xad780**.
The helper reads only the node's current live Display.visibility integer,
not descendant/ancestor visibility or Widget enabled state. At
**0xad785–0xad78c**, a zero result adds0x20 to the scene mask; a nonzero
result preserves the mask without adding that bit.

Later in the same caller, own-shown tests run in this short-circuit
order: **G+0x118 title layer at0xad81d**, **G+0x170 character-creation
layer at0xad82d**, then **G+0x130 character-selection layer at0xad83d**.
Any nonzero result additionally sets0x20 at **0xad846**, independently
of whether the global root was visible. These are the frontend layers
identified by the constructor contract, not System/Options/Controls.
The intervening/later additions of0x10,0x02 and0x40 do not clear0x20.
The final mask is stored as **scene argument23 at0xad915** and supplied
to **0xe2e00 at0xad9d4**.

Thus for this normal caller, flag0x20 is set when the sampled global root
is hidden **or** any of those three frontend layers is own-shown. F4
hiding the root therefore suppresses this entire HUD submission block,
including compass and coin, through the scene mask as well as hiding
later Plasma traversal. This is a late scene-call preparation sample,
not a direct draw-mask write in the F4 key handler; a subsequent change
does not retroactively requery it inside this scene call. No broad
ordinary-UI predicate, inventory/map visibility or guessed modal gate
is substituted for these exact flag0x20 inputs.

The queue is rebuilt for the scene. Submission precedes its final mode1
opaque/faded consumption at **0xe4f10/0xe4f22**, after ordinary scene
geometry/water/effects and before Plasma traversal. It is not a Widget
draw hook inside an inventory/HUD mask or cached background.

## 2. Exact effect argument mapping; older coin labels superseded

**0xee5a0** takes context1, resource2, physical screen X3/Y4, Euler-vector
pointer5, uniform scale6, material RGBA pointer7, then **three floats**:

|Actual helper argument|Typed result|
|---|---|
|8|Glow|
|9|Shininess|
|10|DepthOffset|

For the alpha1 materials in this HUD block it also writes **pixel
Opacity1 and White0**. There is no caller-supplied separate Opacity or
White argument in this helper. Native0xee5a0 writes these values into
the ordinary opaque descriptor; its linked-list-node-relative layout is
material+0x50, Opacity+0x60, Glow+0x64, Shininess+0x68, White+0x6c and
DepthOffset+0x70. Consumer **0xe7328–0xe7381** explicitly submits material,
Shininess, Glow, Opacity, White and DepthOffset before drawing the sprite.
This traces typed controls through the actual descriptor, rather than
assigning them by visual appearance or historical parameter names.

|Submission|Material RGBA|Opacity|Glow|Shininess|White|DepthOffset|
|---|---|---|---|---|---|---|
|Compass,0xe4261|(1,1,1,1)|1|1|0|0|0|
|Coin shadow,0xe4347|(0,0,0,1)|1|0|1|0|−0.5|
|Coin body,0xe4417|(1,1,1,1)|1|0|1|0|0|
|Context shadow/body,0xe4571/0xe4629|Black/white, alpha1|1|0|Item helper below|0|−0.5 /0|
|Special shadow/body,0xe4802/0xe48aa|Black/white, alpha1|1|0|Item helper below|0|−0.5 /0|

Thus the coin's historical “glow1” description in hud-money is incorrect
under the resolved meanings: **coin Glow0/Shininess1**. Compass instead
has **Glow1/Shininess0**. The older coin position/scale contract is not
replaced by this clarification. Its catalog identity remains0xae1;
the calls above select that resource and confirm its actual effects.

Context/special Shininess is **0x10a420 on the original item**, separately
called for each shadow/body. Category0 returns0. Otherwise the first
matching rule is: material1/11/12/13/23 →1; material4/29 →0.5;
material33/35 →0.25; otherwise category23, or category24 subtype4/5/7/8/
10/18 →1; otherwise0. The complete shared interpretation is
[GUI item icons §5](steam-gui-item-icons.md#5-item-materials-shininess-producer).
It is not a rarity tint or Glow producer. Only positive Shininess selects
the established WORLD pixel ripple branch; its magnitude is not a new
ripple amplitude. Glow1 still passes through the ordinary WORLD pixel
fog/grading pipeline. Shader operations remain
[WORLD output §§1–5](steam-world-shader-output.md).

## 3. Compass owner resource, heading and float stages

The resource is catalog index **0xdae**, read through **World+0x70**,
catalog pointer-vector begin/end at+0x10/+0x18, slot **begin+0x6d70**.
At0xe41dd–0xe41f8 a vector count not greater than0xdae yields null;
otherwise use that actual slot pointer. Helper0xee5a0 accepts a null
resource by making no submission. No replacement compass is selected.

Catalog initializer **0x177f50** proves the file binding:
**0x1cd71b** references the exact narrow literal **`compass2.cub` at
0x45fbb8**; **0x1cd732 →0x6dec0** appends that literal to the initializer's
common catalog filename prefix. **0x1cd75b–0x1cd763** selects index0xdae
through0x1734c0, which returns its vector-slot address. The existing
resource is loaded through **0x16f320 at0x1cd780**, with fourth/fifth
load arguments **1,0** and the ordinary optional data-source argument.
This is owner file-backed CUB geometry, not runtime procedural compass
construction. The outer prefix configuration/container source remains
an input; this trace does not invent a different directory or alias.

The shared loader reads full signed32 X/Y/Z allocation dimensions and
raw RGB voxel data into the resource; those dimensions at **+0x88/+0x8c/
+0x90** are runtime owner inputs, not an occupied-voxel bounding box.
There is no compass-specific recoloring or geometric substitute here.
The precise shared CUB load/data contract remains
[particle resource provenance §7.1](steam-particle-bursts.md#71-verified-asset-initialization-and-extractionload-data).
Its generic loader facts apply; its cube filename/index do not.

**Heading source:** Game frame **0xad88e–0xad89c** sends current displayed
double pitch/roll/yaw at **Game+0x1d80/+0x1d88/+0x1d90** through0x295d00.
That helper narrows each double independently to float. The scene call
at0xad9d4 supplies this tuple as argument4;0xe2e00 copies its third float
into **R+0x358**. It is displayed yaw in **degrees**, not target yaw,
physical velocity, a newly reconstructed eye/target direction or GUI time.

At **0xe41ab–0xe41d2**, the compass reads that float and flips its sign
bit to negate it. Its submitted Euler vector is **(−120,0,−displayedYaw)**.
There is no modulus/wrapping or trig-based heading reconstruction before
the shared helper. This also preserves the sign-bit negation behavior
for signed zero. Native rotations subsequently multiply the float degrees
by **0.01745329238474369**, word0x3c8efa35 at0x42e520, for their cosf/sinf
arguments. Camera/angle provenance also remains
[pose invocation §2](steam-world-pose-invocation.md).

Prepare **q=float32(1024/float32(H))** earlier in the scene, with1024 at
0x43efcc. Compass scale is float32(g times **0.003000000026077032**),
then float32 times q; that literal's word is0x3b449ba6 at0x42e6dc.
Screen X first converts W to float, then subtracts float32(g times60);
screen Y is float32(g times20). Submission **0xe4261** is therefore at
**(W−60g,20g)** physical pixels. Each multiply/subtract is a separate
float32 stage; no width-normalized node coordinate is substituted.
It has one white descriptor and no offset black duplicate.

## 4. Context item source, allocation scale and shadow pair

At **0xe442f–0xe4444**, take the current player P, read its character-data
pointer **P+0x1528**, and use the **embedded original0xa0-byte item at
character-data+0x1e8**. Admission is its category byte nonzero. This caller
does not inspect a stack quantity, regional equality, equipped slot,
hovered inventory cell, Ring selection or item usability/class permission.
It does not select a substitute category from the player's class.

Resolve that original item with **0x173b10(World+0x70,item)** at0xe4455.
The selector's existing item normalization/resource rules remain
[item models §§0–3](item-models.md); that includes normalized copies of
category0x16 piece records and the separate category0x17 cached procedural
resource route. This HUD caller does not modify the original record or
construct/recolor a separate model. Shininess still uses the original
item, not the normalized temporary copy.

From the returned resource select the maximum **signed32 full allocation
X/Y/Z dimensions**, then floor that denominator to integer16, before
conversion to float. At **0xe4472–0xe4525**, scale is obtained by float32
division of **0.03500000014901161**, word0x3d0f5c29 at0x43ef3c, by the
converted denominator; multiply that result by g, then by q. This differs
from Ring's numerator-first/no-minimum16 scale. It is not an occupied
vertex extent, resource radius or direct GUI icon's size parameter.

Both draws use **(−120,0,spin)**. For each reached spin calculation, read
**World float milliseconds+0x440**, truncate toward zero to signed32,
take signed remainder modulo10000, convert the remainder to float,
multiply by **0.00009999999747378752** (word0x38d1b717 at0x4ac954), then
by float360 (0x42e7e4). Products round separately to float32. This is
World elapsed-update time, not wall-clock or GUI engine time; negative
representable values retain the signed-remainder rule. The body reads/
calculates the phase again; no independent per-item phase is introduced.

The producer submits the **same resource, scale and orientation** first
at **(230g,57g)** with black material/DepthOffset−0.5, then at
**(225g,52g)** with white material/DepthOffset0. Each pixel coordinate
uses its float constant times g. This is an opaque offset geometric
duplicate, not blurred shadow pixels or a half-alpha model. See§2 for
the actual Glow/Shininess/Opacity controls.

There is **no null-result test before the context resource's dimension
loads**. This contract's ordinary completed branch therefore assumes a
resolved valid resource. It does not establish a native fallback/skip
for an invalid nonzero-category record whose model lookup returns null.
Later GPU readiness is a different admission, described in§6.

## 5. Special-item physical-slot selection, order and placement

After the context branch, **0xe4634–0xe48b3** accesses the player's
canonical collection vector **P+0x9f0/+0x9f8**. For a valid vector with
more than one24-byte tab entry, select **tab index1**: its slot begin/end
are read from vector-begin+0x18/+0x20. Visit physical slots forward at
**0xa4-byte stride**. No sort, compaction, deduplication or aggregation
occurs in this renderer.

For each slot the tests occur in this order:

1. Quantity **signed32 slot+0** must be nonzero. Negative nonzero values
   pass this literal test; it is not a positive-count test.
2. Embedded original item at **slot+4** must have category **0x18**.
3. Its two stored signed32 region fields, item+0x0c/+0x10 (slot+0x10/
   +0x14), must exactly equal current **P+0x150/+0x154**. Compare both
   components; do not substitute camera region, distance or new generation.
4. Exclude subtypes **0x0e,0x0f,0x10,0x11**. Native subtracts14 in
   unsigned32 and accepts only a result greater than3; this is the exact
   range exclusion, not a restriction that all accepted subtypes are≤18.

Resolve accepted original items through0x173b10 at0xe46f4. As in the
context branch, the resource is immediately used for dimension loads
without a null guard. The actual category0x18 selector's supported
subtypes/resources are:

|Subtype|Catalog index|Subtype|Catalog index|
|---|---|---|---|
|1|0xda1|2|0xda2|
|3|0xda4|4|0xad6|
|5|0xad7|6|0xad8|
|7|0xad9|8|0xadb|
|9|0xc82|10|0xadc|
|11|0xadd|12|0xe43|
|13|0xab9|18|0xda3|

These fixed catalog lookups use the owner's existing resource pointers.
Excluded subtypes14–17 are not sent through this row's draw route.
Other admitted-but-unsupported subtype records have no invented fallback;
the ordinary valid-resource boundary above applies. No catalog filename
or allocation size for these entries is guessed from an item name.

The first special center's **integer X is225 when the context category
was0;325 when that context branch was taken**. For each accepted valid
item, submit black shadow at **((X+5)g,57g)**, then white body at
**(Xg,52g)**, with the context branch's same maximum-allocation/minimum16
scale and World-clock spin. Add integer50 to X after that pair. Convert
the completed integer X+5 or X to float before multiplying by g; do not
replace it with sequential float increments. Skipped slots consume no
row position. Duplicate accepted slots draw duplicate icons; stack
quantity does not multiply the number of icons.

There is no width-fit, row wrap, maximum-icon count or per-icon scissor
in this loop. A resource which is enqueued but later fails GPU readiness
has already consumed its position. A present context item reserves the
start325 even though its final pixel coverage may later be absent.

## 6. Shared transform, draw ordering and state; bounded exclusions

Each0xee5a0 descriptor starts with translation **(0,0,1)**, appends Euler
X/Y/Z rotations and uniform scale, then a local translation by **negative
half of all three signed32 allocation dimensions** through the existing
rotated/scaled basis. It centers the full owner CUB header allocation,
not its occupied AABB or a voxel-center-adjusted box. Raw voxel corners
use the established CUB units, with no extra fixed-point/world scaling.
Exact arithmetic/queued projection are
[Ring rendering §§3–4](steam-ring-selector-rendering.md).

All materials above have alpha1 and enter the **normal opaque resource
tree** in R+0x18. No opposite-winding or faded-vector submission is made
by these calls. Global opaque order is ascending **unsigned resource
pointer**, with FIFO descriptor order inside one identical-resource
group. Consequently shadow/body order for the same pointer is retained,
while source order across different resources is not a painter-order
promise. Catalog numeric order is not proved equal to heap-pointer order.

The consumer requires the ordinary resource VB/IB objects to exist and
report ready through0x16eba0 before descriptor state writes; it then draws
through0x16ecc0. Queue/ready/zero-count distinctions remain
[queued-state order §§1–2](steam-queued-world-state-order.md).

The shared final screen pass installs identity VIEW,50° perspective,
negative W/H aspect, near0.05 and far float(zone-window-count shifted6).
It retains positive view Z/clip W=view Z and0..1 depth, applying the
screen-center shift and fixed−0.5 clip-space Z shift independently of
each descriptor's **DepthOffset**. The pass supplies the fixed overlay
directional palette, white base material, finite fog range10000 and
clears point tables; mode1 does not bind world-zone point lights per icon.
The full palette/normalization/inherited sky/fog-gradient facts remain
[Ring rendering §§4–5](steam-ring-selector-rendering.md) and
[WORLD controls §2](steam-world-pass-controls.md).

These use ordinary WORLD shaders/state0: depth testing/writing enabled,
LESS_EQUAL, alpha blending disabled, normal CullMode2/FrontCounterClockwise0,
inherited main target/viewport/depth contents, no private icon depth clear
or Plasma mask. DepthOffset is the separate WORLD vertex control, not a
model-Z translation or pixel alpha. No HUD-specific spirit-attribute/cube
iteration occurs around these resource submissions; the direct0xddbc0/
0xdc390 GUI icon path must not be silently appended here.

The upstream lifecycle choosing the embedded character-data context item,
outer catalog prefix/container configuration, malformed item/resource
behavior and concurrent mutation are not closed by this bounded producer
contract. The unrelated Ring creature-preview route, general HUD text
and every other overlay producer remain separate. No owner resource
dimensions/colors or replacement artwork were invented.
