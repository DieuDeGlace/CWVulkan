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

## 7. Frontend roots and same-frame input order

### 7.1 The three roots in the toggle/use gate

Interface setup**0xf9ac0** creates these as separate containers under the
GUI engine root. The interface object begins at G+0x118, so its first
pointer is also the first container pointer, not the pointer to the whole
GUI engine. Native constructor bindings and their later contents give:

|Game field|GUI entry|Container meaning|
|---|---|---|
|G+0x118|GUI[0]|title/root-menu layer; root-menu node GUI[1] and RootMenuWidget GUI[2] are constructed beneath it|
|G+0x130|GUI[3]|character-selection layer, containing the rebuilt character cards and Select/Create/delete controls|
|G+0x170|GUI[0xb]|character-creation layer, containing the appearance panel, character name and Create Character controls|

The constructor initializes the first shown and the latter two hidden.
The first is **not the in-game System menu**, Options or Controls. Those
are separate constructed widgets: System GUI[0x48], Options GUI[0xa7]
and Controls GUI[0xa8]. World selection/creation are also separate layers,
GUI[0x10]/GUI[0x1d]. These names/parent bindings follow native0xf9ac0
and the existing frontend-title2/frontend-characters2,5/frontend-options2,4,5
specifications; they do not follow a replacement engine's panel names.

The tested predicate**0x9af00** reads the passed node's **own current
Display.visibility**, nonzero meaning shown. It does not walk ancestor
visibility or equate every ordinary UI widget to one of these three roots.
Ring toggle's block additionally requires ChatWidget's active-editor
byte **G+0x398→+0x1d8 to be0**. That block does not add System/Options/
Controls/inventory/map visibility as another root test. Its0x5514ff
previous Ring-action byte updates only inside this admitted block, as
already specified in slot-lifecycle9.2.

### 7.2 Ordinary-use outer call and internal early exits

The sole direct frame call to**0x8ae50** is **0xa9b2c**. Its immediately
enclosing native tests at **0xa9ac4–0xa9b1c** require world+0x210 nonzero,
local player ability byte+0x68 neither **0x9c nor0x9d**, and all three
roots in7.1 hidden. There is no additional ordinary-UI0x1014f0, map,
System/Options/Controls or Ring-visibility test at this callsite. The
world-ready field is retained as its verified nonzero admission; no new
replacement readiness or player-health predicate is implied by its name.

Calling0x8ae50 does not mean its prompt/use branch is necessarily reached.
After its early cursor/creature-target and movement handling, before later
target resolution/prompt, **0x8bdc0–0x8be37** returns early for:

* **GUI focused pointer non-null**, returned by0x342090 from GUI engine
  +0x170. This is a different object/field from the character-creation
  container G+0x170. It is a raw retained focus-pointer test, not a new
  test of the focused Widget's current visibility/type.
* ChatWidget **active-editor+0x1d8 nonzero**.
* Native predicate**0x5ee70(player)** true and stamina float+0x9a0
  **strictly below0.0010000000474974513**, word0x3a83126f at0x42e514.
  That predicate means ability0x78, ground bit+0x5c bit0 clear,
  float+0x130≤0 and float+0x12c≤0. It is not every vehicle ability.
* Ability **0x7d** with float ability-age+0x6c **strictly below2000.0**,
  constant0x42e5d8.

These early exits precede the Use and Quick Item prior-byte stores too;
do not advance those branch-local edge histories merely because the outer
frame call was admitted. Ordinary menu visibility without a retained GUI
focus pointer is not equivalent to one of these early returns. The
separate later0x1014f0 test at **0x8ee23** is after Ring Use and Quick Item;
it must not be moved ahead of those actions.

Prompt routine**0x98180 is called at0x8df69 before selector dispatch**.
When its result is0, the selector's own shown state is tested at
**0x8e053–0x8e057**. A shown selector takes the entire selector branch:
on an admitted Use rising edge it rebuilds candidates, uses the retained
wrapped index, calls0xb7740 on the live slot and hides only for a nonempty
list. It then bypasses the ordinary no-prompt ground-pickup/vehicle branch.
That bypass also applies with **no edge or an empty candidate list**; it
does not depend on successful consumption. This is a branch-priority rule,
not an additional global movement/camera/modal block. The prior Use byte
is stored at the common continuation around0x8eb30. Prompt result1 takes
its own branch first, not this selector use. The independently later
Quick Item action4 branch still has its existing separate edge and works
whether Ring is shown or hidden; slot-lifecycle9.5 remains authoritative.

### 7.3 Ordered frame stages

The following are native ordering facts within one reached Game frame,
not a proposed reorganization of input handling:

1. Build/sort the creature target candidates, then call **0x8ae50 at
   0xa9b2c** under7.2's outer gate. Prompt/selector Use precedes independent
   Quick Item inside that routine; both see the Ring shown state and
   running index from before this frame's later toggle/selection-repeat
   work. Mutations and hides from these actions take effect immediately.
2. Later, enter the chat/front-root-gated panel-toggle block. Ring action16
   tests its rising edge, toggles the **then-current** selector shown state
   at **0xab0be–0xab0e7**, and stores its previous byte at0xab0f4. This is
   after ordinary use, not a pre-use stage.
3. Handle the separate broad panel/map movement close, then the
   **selector-specific forward/back/analog-Y close** at
   **0xab46d–0xab4b8**. The selector test is outside the toggle block:
   an earlier successful toggle can therefore be closed before any
   selector content/render work in that frame. Its exact held/threshold
   conditions remain slot-lifecycle9.2.
4. Call interface **0x103200 at0xab5ce**. Inside it, full-map G+0x668
   hides selector through **0x106152**, before that routine's selector
   left/right color-feedback handling. Consequently a frame which still
   has full map open supplies no Ring list in the later shown-only stage,
   even if Ring was toggled earlier. This map close is separate from
   0x8ae50's outer admission and its earlier actions.
5. Later rebuild all category1 candidates at **0xac4a5**, after any earlier
   item-use mutations. If nonempty, resolve the retained running index
   and update the ordinary Quick Item HUD icon/count/name through
   **0xac4aa–0xac734**, including while Ring is hidden. This stage precedes
   Ring's later repeat-driven index change.
6. Test Ring shown at **0xac756**. Hidden and shown-empty paths perform
   their existing reset/text behavior. In the shown/nonempty path, append
   all live item entries, resolve the current index and write Ring
   count/name/color through **0xac8b9–0xacb8f**. This is still **before**
   processing this frame's Ring left/right neutral-latch/repeat changes.
7. Process that repeat branch, then compute/ease displayed angle and
   radius through **0xacd9f**. Update shown brackets afterward. The scene
   call receives the resulting live-entry list and updated angle/radius,
   and renders through sections1–5. It does not regenerate either label
   after the repeat branch.

These stages settle the same-frame combinations. With an initially hidden
Ring, an admitted Quick Item press uses its old selected candidate and
hides Ring if nonempty; a later Ring toggle can then open it. An admitted
Use press with prompt0 and initially hidden Ring reaches the ordinary
pickup/vehicle fallback, before a later toggle opens Ring. If Ring was
already shown and Use consumes/hides a nonempty selection, a simultaneous
later toggle can reopen it. Each result remains subject to the specific
gates/early returns above and the later movement/map closes.

Likewise a left/right repeat in the same frame as Use/Quick Item does not
change which index those earlier actions used. The Quick HUD and Ring
labels reflect the index **before that later repeat**, while the rendered
ring can already move toward the updated index's angle. The next reached
content update observes the retained changed index. This is an observable
ordering difference; it must not be silently normalized to “selection,
then action, then all labels.”

## 8. Item text color, count mask and Quick HUD name

### 8.1 Color selection,0x10a600

The caller supplies the **original item** and the local player's current
signed32 region pair at creature+0x150/+0x154. Apply the following native
priority; every result has alpha1:

|Item category/subtype|RGBA|
|---|---|
|23 or24|(1,0.6,0.25,1)|
|21|(0.7,1,0.25,1)|
|0|(1,1,1,1)|
|22, or category1 subtype13|(0.5,1,1,1)|
|13|(0.5,0.1,1,1)|
|2,3,4,5,6,7,8,9,15,26,30|palette below, using regional rarity0x109720|
|all other categories|palette below, using original unsigned rarity byte item+0x14|

Fixed decimal fractions denote the native stored float32 constants:
0.6 word0x3f19999a,0.25 word0x3e800000,0.7 word0x3f333333,
0.5 word0x3f000000 and0.1 word0x3dcccccd. The regional category set
is mask **0x440083fc** with category<31; it is not every nonempty item.
In particular **ordinary category1 Ring candidates use their original
rarity without regional degradation**, except subtype13's fixed cyan.
Passing a player region to this helper does not make all Ring colors
region-dependent.

Regional helper**0x109720** normally returns the original unsigned rarity
byte only when item region+0x0c/+0x10 equals the supplied pair, otherwise
**signed32−1**. There is one preceding extended-area exception: categories
**3 through9 or26**, whose unsigned modifier+0x08 is divisible by
**(unsigned rarity+1) times10**, retain rarity when the maximum absolute
X/Y region difference is **strictly below2**. Equality of the regions
still succeeds outside that exceptional case. The native coordinate
subtractions/absolute-value operations wrap at32 bits, and the maximum/
below2 comparisons are signed32; no wider distance or overflow clamp is
introduced by this helper. It does not alter the original item rarity.

Palette helper**0x64680** clamps its signed32 input to **[−1,4]** before
selecting these literal float32 colors:

|Clamped value|RGBA|Fractional float words|
|---|---|---|
|−1|(0.7,0.7,0.7,1)|0x3f333333|
|0|(1,1,1,1)|none|
|1|(0,1,0,1)|none|
|2|(0.3,0.45,1,1)|0x3e99999a,0x3ee66666|
|3|(0.55,0.2,1,1)|0x3f0ccccd,0x3e4ccccd|
|4|(1,1,0,1)|none|

Thus an input below−1 is gray, an input above4 is yellow, and an unsigned
item rarity255 passed by the ordinary branch becomes yellow. The helper's
syntactic switch-default red path is unreachable for an ordinary signed32
argument after its clamp; do not expose it as an out-of-range fallback.
The fixed-category overrides in the first table bypass that palette.

### 8.2 Count and name paths

The ordinary Quick Item HUD's count and `info` text are first cleared
each reached frame, before candidate build0xac4a5. A selected count is
written only when **0x5f0e0** accepts its original category. That helper
tests category<29 with mask **0x10403c02**, exactly
**{1,10,11,12,13,22,28}**. Category18 is not in that set. The displayed
count is the selected slot's **signed32 quantity**, as ordinary decimal,
including1 or negative nonzero values; no multiplication sign, minimum2
rule or absolute-value conversion is added. A nonaccepted category leaves
the already-cleared count blank. The shown Ring count applies the same
predicate; actual Ring candidates are category1 and therefore qualify.

The Quick HUD's `info` producer at **0xac627–0xac6f3** does **not** call
the full item-name helper0x1628b0. It resolves the dictionary key from
the original category/subtype using **0x16af30**, then expands that key's
**`default`** phrase with **0x16be20** and an empty substitution/context
string. It supplies neither original modifier, rarity, material, attributes
nor a renamed/full-item record to that name lookup. This bounded producer
does not itself submit0x10a600 as a Quick HUD text-color override.

The shown Ring `name` producer at **0xaca95** does call **0x1628b0 on
the full original item**, then applies0x10a600 at0xacb5c/0xacb75. However,
that full-name helper has an explicit basic-name branch for category1
(also15 and19): after its item adaptation, it performs the **same
category/subtype `default` expansion with empty context**. The adaptation
0x1092b0 only rewrites specified category22 cases and does not rewrite
category1. Consequently the two native name paths are distinct calls,
but **for an actual category1 Ring candidate their naming rule is the
same**; the Ring does not add rarity/material/generated-name decorations
to these candidates. Other categories can use the full helper's richer
rules, but are not admitted by Ring's candidate builder. Do not infer a
new renamed-item string field or a special Ring renaming algorithm.

## 9. Exact combined open/close sound predicate

Game wrapper**0x9ae70** is a tail call to **0x1014f0(G+0x118)**;
its12 native bytes do not add a sound-specific or edit-focus test.
0x1014f0 returns true when any of the following is true, in this native
short-circuit order. “Visible Widget root” means the Widget's node at
**widget+0x178**, reading that node's **own current Display.visibility**;
it is not an ancestor/root-visible test.

|Order|Predicate|
|---|---|
|1–5|Visible frontend container G+0x118 title, G+0x130 character select, G+0x170 character creation, G+0x198 world select, G+0x200 world creation, respectively|
|6|Full-map byte **G+0x668 nonzero**, including ordinary map mode|
|7–10|Visible Widget root for G+0x330 inventory, G+0x340 crafting, G+0x388 multiplayer, G+0x350 shop, respectively|
|11|Menu-open byte **G+0x420 nonzero**, separately from SystemWidget's root|
|12–14|Visible Widget root for G+0x370 upgrade, G+0x368 weapon customization, G+0x360 identification, respectively|
|15|ChatWidget **G+0x398, active-editor byte+0x1d8 nonzero**|
|16–19|Visible Widget root for G+0x378 answers, G+0x358 System menu, G+0x658 Controls, G+0x650 Options, respectively|

It does **not** separately test the GUI focus pointer, Controls waiting
index, Ring selector, ordinary NPC speech-bubble vectorG+0x400 or general
DialogWidgetG+0x418. A focused editor can matter to ordinary use or chat
admission without being an extra condition of this predicate. Conversely
System, Options and Controls visibility explicitly count here even though
they are not the three frontend roots in Ring's toggle/use gate7.1.

The sound sample at **0xad78f–0xad7e6** combines this result with the
selector nodeG+0x310's own shown state. Compare that single combined
boolean to previous byte **0x551560**: false→true requests **sound0x56**,
true→false requests **sound0x57**, unchanged requests neither. The call
is0xa0420 at the current listenerG+0x1dc0, with volume/pitch1 and the
existing final byte1. Store the combined current boolean back to0x551560
later in the same reached stage, before scene submission. No independent
Ring sound is emitted at its toggle or hide call.

This sampling is **after** the use/toggle/movement/map-close, Quick HUD,
Ring label, repeat and radius/angle stages in7.3. A temporary open then
close within the same frame does not itself create two sound edges.
Another counted UI can hold the combined boolean true when Ring closes,
suppressing0x57; its own subsequent closing may produce that falling edge.
Ring opening while another counted UI is already true likewise produces
no new0x56. Hiding the global drawing root alone is not a substitute for
clearing the own-visibility/state predicates listed above.

## 10. Left/right feedback changes one live Display multiplier

In interface0x103200, each arrow lookup **0x332190** returns the first
matching node named **`left`** or **`right`** in the selector's depth-first
child-list order, considering the passed root before its descendants.
It does not collect all same-named descendants. The feedback writes at
**0x1061f7/0x106289** address **that node's Display pointer+0x68**, then
its attribute **Display+0x180**. This is **Display.fillColor**, not the
shape pointer at node+0x58 or an array of vertex colors. Native Display
constructor**0x3736c0** registers the fill RGBA attribute at+0x180;
loader**0x382130** supplies its owner data. Stroke has a separate attribute.

Setter**0xf88f0** replaces exactly four float words in the attribute's
**currently selected frame element**: signed32 frame index at attribute
+0x3c,16-byte RGBA elements beginning at the data pointer attribute+0x70.
It does not replace every stored/keyed frame, edit a named sequence,
iterate vertices or rewrite any shape data. Nor does it recurse into
children, change their Displays or write Display.strokeColor. It is an
immediate assignment to that one live RGBA, even if its value is already
equal; this helper contains no clone/copy-on-write, changed-flag or mesh
rebuild call. The Ring arrow callers add no such recursive update.

The assigned value is idle **(1,1,1,1)** or held **(1,0,0.2,1)**, with
float0.2 word0x3e4ccccd; the action/analog thresholds and shown admission
remain slot-lifecycle9.4. Because this is a **multiplier**, idle white
retains the owner's vertex/texture colors and held color modulates them;
it does not flatten a colored owner mesh to one solid RGBA.

Drawing**0x3913d0** reads the node's current Display.fillColor at
**Display+0x1f0 plus16 times signed32 index at Display+0x1bc**, the same
element selected above. It multiplies that RGBA componentwise with the
current pass tint before submitting the draw-color constant. This own-node
Display multiplier is used by the shared drawing path for fill and any
stroke/extrusion geometry submitted through it; the field's name does
**not** restrict the effect to a fill-only region or rewrite the separate
stroke-color attribute. It does not multiply a parent Display down into
independently drawn child nodes. If owner nodes share the same Display
object, they naturally observe that same changed attribute; that aliasing
is distinct from a recursive child-color override and is not removed by
this feedback setter. Numeric owner geometry/colors and sharing are not
inferred here.
