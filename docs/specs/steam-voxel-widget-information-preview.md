# Steam customization: ordinary information preview and palette coordinates

Independent native-only follow-up to
[the VoxelWidget edit view](steam-weapon-customization-edit-view.md) and
[presentation/input](steam-voxel-widget-presentation-input.md). Sources:
0xb6b00,0x94260,0x94490,0x94580,0x94600,0x94750,0x94190,
0x28c0a0,0x28c290,0x292990 and native constants/instructions. Addresses
are Steam RVAs. No engine, tests, exporter/build, SDK, owner assets or
captures were inspected, and no game was executed.

## 1. Which information panel this selects, and when

The ordinary information panel historically called **InfoWidget** in
these prose specifications is the native **PreviewWidget**, constructor
**0x28c0a0**, draw **0x28c290**, vtable0x46fb78. RTTI locator0x4b0a48 /
type descriptor0x54b8a8 establish that native class name. It is
**G+0x3c0 / Interface+0x2a8 / GUI[0x55]**, not the inventory details
root G+0x338 or formula details G+0x348.

Interface setup **0xf9ac0**, with constructor call **0xffebe**, takes
the owner node **`preview`**, reparents it under the GUI engine root
through0x3347f0, attaches this wrapper and requests **300×300**. The
size constant is float300 at0x42e7e0. Preserve the owner's bind/pivot /
geometry and ordinary initialization; this is not a new blackwidget or
a rectangle to reconstruct from the request. The constructor stores its
Game reference at **+0x228** and selects owner font `resource1.dat`.
Its callback/flags contract remains gui-item-icons16.3.

The selector is **0xb6b00**, called by the Game frame at **0xa9d22**.
It runs before the later ordinary panel movement-close/interface update
and before the subsequent draw-produced hover refresh. It does not run
inside VoxelWidget's click helper. Its item pointers remain borrowed
references; it does not freeze the pointed-to member contents into a
new item snapshot for the later preview draw.

## 2. Ordinary hover priority, including the VoxelWidget palette

When **Ring selector G+0x310 is hidden**, first successful source wins
in this order. A failure proceeds to the next row unless otherwise
specified. These are the complete ordinary source tests in0xb6b00,
not a priority based on which overlapping panel was opened last.

|Priority|Original item source and admission|Placement anchor Widget recorded by selector|
|---|---|---|
|1|**MapOverlayWidget G+0x380 embedded item+0x1b0**, when its category byte is nonzero;0xb6f30–0xb6f45. The selector adds no separate overlay shown/hit test around this record.|None|
|2|Canonical Inventory hover through **0x94600 at0xb6f52**: Inventory shown, current frame hit, valid stored hover tab/slot, quantity!=0. The selected pointer is the original live slot's item payload.|Inventory G+0x330|
|3|Equipment hover through **0x94260 at0xb6fa6**: Inventory shown and one of its equipment SpriteWidget subtrees hit. Require a nonnull pointer and nonzero category here.|Inventory G+0x330|
|4|Crafting hover through **0x94490 at0xb6fc6**: Crafting shown, frame hit, valid stored hover pair, source quantity>=0 and category!=0. Quantity0 is admitted by this resolver.|Crafting G+0x340|
|5|Shop offer hover through **0x94750 at0xb6fe1**: Shop shown, frame hit and valid stored offer hover pair. This resolver returns the original offer payload; the selector adds no source-count/category test.|Shop G+0x350|
|6|Formula-detail / Voxel palette resolver **0x94580 at0xb6ffc**, detailed below.|**Crafting G+0x340**, even when the returned source belongs to VoxelWidget|
|7|The later0x94190 call at0xb7017.|No effective source in the audited original image; see below|

The higher-priority canonical/equipment/crafting/shop cases can therefore
win while customization is shown. This selector does not reuse the
editor click tail's palette-before-thumbnail choice. Slot identity,
frame hit and cached grid hover rules remain the independent inventory
specifications; they are not fresh click/ray picks here.

Resolver **0x94580** first checks whether **FormulaDetailsWidget
G+0x348** is shown. If so, it returns its draw-produced semantic hover
item **+0x2ec** only when that item's category!=0; otherwise it returns
null **without trying VoxelWidget**. This branch is0x94580–0x945b5.
Consequently a shown formula-details panel with no hovered item still
prevents the Voxel palette fallback within this helper.

Only when formula details are hidden does it test VoxelWidget G+0x368.
Require its current root shown property nonzero, then require its
stored palette-hover index **+0x2d8>=0**. Return the member-item address
**VoxelWidget+0x2dc**; otherwise null. This is
**0x945b6–0x945f2**. It adds no current pointer hit, palette-vector bounds,
quantity, category or selected-palette/carry condition. The draw loop's
existing reset/copy rules supply the cached item/index; a negative index
makes the retained old item unavailable to this resolver.

On a nonnull result, the caller explicitly records **G+0x340 as the
anchor at0xb700b**, not G+0x368. The later placement queries therefore
use the Crafting Widget's geometry even when Crafting itself is hidden
and the information source is the Voxel palette. Do not substitute the
customization root as a more intuitive anchor.

**0x94190 is not a fallback for VoxelWidget+0x214.** Its native body
checks identification selection fields but unconditionally clears its
return register at **0x9424f** and returns null at0x94251. The caller
tests that return. This statement is confirmed in instructions, rather
than inferred from the abbreviated decompile or a function name.

When Ring is shown, its separate source-resolution branch bypasses the
whole ordinary hover chain above, including0x94580. Its selected canonical
quick-item and world fallback retain the Ring specification. If no
ordinary UI source wins, the earlier resolved current ground-item fallback
can supply the item; this is not a new VoxelWidget source.

## 3. Draw-produced +0x214 is not the ordinary preview fallback

VoxelWidget draw **0x292990** clears and produces the semantic member
item **+0x214** as edit-view sections5–7 specify: selected/carry feedback
before the raw pick, and a picked attribute can overwrite it. This is
separate storage from the palette-hover member **+0x2dc**.

The complete ordinary preview-selection path **0xb6b00→0x94580** above
does **not** read +0x214, use it when palette hover is negative, or show
the information panel merely because a candidate/carried/picked cube
made that member nonempty. Its confirmed Voxel source is +0x2dc only.
No other consumer of this specific +0x214 member has been verified by
this bounded follow-up. This preserves the native written state without
inventing an information-window trigger or claiming that every possible
indirect alias in other paths is absent.

## 4. Preview state, original records and local-player adaptation

The selector's shared assignment **0xb7034–0xb717d** makes its own root
shown if the chosen original item pointer is nonnull, hidden otherwise.
It stores that borrowed **original item pointer at Preview+0x1f0**,
clears the comparison pointer **+0x210**, and resets its auxiliary
itemless-entry indices **+0x200/+0x208 to−1**.

UI hover choices in section2 set **Preview+0x1e8 to null**. That field
is a separate **ground-item origin record pointer**, not the local-player
reference and not a semantic item copy. At the beginning of0xb6b00,
the current ground selection at G+0x1e70 can be resolved by **0x2b8fd0**
against World. The fallback retains that record pointer for ownership
information; UI choices suppress it.

Quantity **Preview+0x1f8** starts from1 for this selection pass. A
successfully resolved ground fallback earlier in the pass replaces it
with that record's signed32 quantity+0xd4. The later UI source selection
does not reset this scalar to the inventory/shop/palette source quantity.
In the ordinary no-ground-fallback palette case the scalar is therefore1,
not the palette's aggregated available quantity. The manual palette count
is the distinct presentation/input section1 text.

The same assignment copies the map renderer's pair **(G+0x10)+0x150/
+0x154** into Preview+0x218/+0x21c, and MapOverlay's **+0x250/+0x254**
into Preview+0x220/+0x224. If either source X is not **INT32_MAX**, it
forces this root shown even without an item. Draw0x28c290 gives the
first location pair precedence, then the overlay/lore pair; ordinary
item presentation requires **both X fields equal INT32_MAX**. These
are sentinel-input checks, not a newly inferred full-map-visible gate.

For an original item, the selector constructs a temporary adapted
record with **0x10a720**, passing the **current local player at
World+0x448**. It uses that adapted category for equipment-comparison
selection; it does not replace +0x1f0 with the temporary. Draw
**0x28c290** independently calls0x10a720 again with +0x1f0 and the same
current local player, then submits the adapted copy to0xddbc0 at0x28c752
and the main item-detail draw0xb7b90. This is different from inventory
cell/palette icon calls that submit their original semantic copy without
this player adaptation. Gui-item-icons11.1 retains the exact preview
icon pose/size/depth/dark-pass contract.

Comparison selection starts empty. It compares candidate equipment's
original category with the temporary adapted selected category, excluding
the selected original address itself. The ordinary candidate order is
weapon+0x6d8, weapon+0x638, then equipment **+0x8b8,+0x598,+0x3b8,
+0x4f8,+0x458,+0x318**, then ring+0x818 and ring+0x778, all on the
current local player. Matching later independent equipment cases replace
earlier ones. Both weapon-address cases exclude selection of either
weapon address; both ring cases similarly exclude either ring address.
The second weapon/ring case can replace the first when physical Shift
byte **G+0x1a97** is nonzero or no comparison has yet been selected.
For adapted subtype13, the first weapon case additionally requires its
candidate to be a category3 subtype in the existing two-handed mask
0x878de0 with subtype<=23. These tests are **0xb7194–0xb73ef**.

The stored comparison **+0x210 remains an original equipped-item
pointer**. Draw passes that original record, not a selector-owned adapted
copy, to its subsequent “Currently equipped:” detail section. The current
local player remains the reference for the invoked stat/adaptation
helpers; +0x1e8 must not be misread as an alternate player for Voxel
palette details. This addendum does not duplicate all item-detail text
formulas in0xb7b90.

## 5. Placement uses native point-and-offset calls

After selection, **0xb73ef** queries ordinary UI0x1014f0. With UI true
and **Preview+0x1e8 null**—the normal Voxel palette hover case—first
place through **0x334700 at0xb7447** using current cached cursor XY
as the anchor point and offset **(30,30)**. These are point-and-offset
arguments, not a frame-origin moveTo or a new viewport clamp.

When the source has an anchor Widget from section2, then use its
**world frame origin0x32cf50** and **world frame size0x32d010**. Form
float32 **originX + float32(sizeX×0.5)** and compare to cursor X.
Equality belongs to the right-side placement at **0xb7499**.

* If cursor X is strictly left of that midpoint, query the source's
  **0x32c340 parent-relative frame position**, subtract30 from X,
  then subtract Preview's **local effective width0x32a1d0**, each in
  float32. Use anchor **(0,current cursor Y)** and offset
  **(that completed X,30)**. Source0xb749b–0xb74e2.
* Otherwise query the same source0x32c340 position, add the X returned
  by its **world frame size0x32d010**, then add30 in float32. Use
  anchor **(0,current cursor Y)** and offset **(that completed X,30)**.
  Source0xb74e7–0xb7530.

The mixed query bases are native: do not silently replace them with a
single rectangle/AABB in another coordinate system. Plasma's documented
query/bind/pivot behavior and0x334700 solve still apply. For a Voxel
palette, the source Widget in these calls is **Crafting G+0x340**.
With no anchor Widget, the initial cursor+(30,30) placement is retained.

When ordinary UI is false **or +0x1e8 is nonnull**, the alternate call
uses anchor **(signed-integer viewport half-width, float(viewport height)
minus Preview parent-relative height0x32ad10)**, and offset
**(210,−23)**. Source0xb7532–0xb7584. Constants are
0x437ff0=210 and0x438038=−23; they are not a guessed half-panel-width.
All final placements call0x334700. Current height is read before the
later preview draw's own text-dependent resizing; no fresh layout/draw
is forced by the selector.

## 6. Exact palette left/top evaluation stages

In Voxel draw0x292990, let P be the already projected root-pivot XY from
the reciprocal-first homogeneous calculation. For a nonempty palette,
**0x294801** first adds float20 to P.x in float32 and **0x294806** first
adds float40 to P.y in float32. These two rounded base values are kept
for the loop.

For each nonnegative index, compute column and row with the existing
eight-column integer division/remainder. Compute **45×column as an
integer**, convert that result to float32 at **0x29485f**, then add
the rounded X base in float32 at **0x294862**, then truncate toward
zero to signed32 at **0x294867**. Y likewise computes integer45×row,
converts at **0x294874**, adds the rounded Y base at **0x294877**,
and truncates at **0x29487b**.

Thus the operation is sequential **projected P plus20/40 first, then
converted integer pitch**, with a float32 rounding at each addition.
It is not integer(20+45×column) or integer(40+45×row) converted once
and added to P. The icon-center integer+20 step and half-open40-pixel
hover test remain presentation/input section2. No extra GUI-scale
multiplier is introduced at these loop additions.

These selector, placement and arithmetic facts are stable. The unverified
consumer scope of +0x214 remains explicit in section3; no owner geometry
or replacement information-panel art is inferred here.

## 7. Stable follow-up: comparison draw, message-derived Shift and sizing

This section closes the ordinary comparison-call and size lifecycle. It
does not claim that every category-specific row of0xb7b90 is specified.

### 7.1 Live sources and Shift are not copied comparison snapshots

The original equipment-comparison ordering remains section4. The selector
clears **+0x210 on every pass at0xb70c9**, before considering the current
source's adapted category. Its tests exclude by **original address
identity**, not full item equality. A semantically equal item in a
different canonical slot is not excluded merely for being equal. Both
selected item and equipped comparison remain borrowed pointers; no
independent retained item record is constructed for +0x210.

The Shift source is **Game+0x1a97**, in the256-byte virtual-key state
array beginning+0x1a87. Constructor **0x7ea80** initializes that array
to zero. Physical key-down **0x96e70** writes1 at+0x1a87+key before its
key-specific handling; key-up **0x97870** writes0 there. Window message
dispatch **0x133c70** supplies the low8 virtual-key code. Shift is
virtual key **0x10**, not a configured logical action or a newly queried
device state in the preview selector.

The second-weapon decision reads this byte at **0xb720f**, and the
second-ring decision reads it at **0xb739c**. Each uses the then-current
stored byte; there is no separate Shift snapshot, press edge, or compare
mode latched by0xb6b00. Their current comparison-null tests still apply
independently. No preview show/hide transition clears the virtual-key
array or creates a different meaning for this byte.

### 7.2 Exact formatter submissions and ignored quantity/flag inputs

In the ordinary item branch of draw0x28c290, the main icon precedes the
main text. After restoring GUI rendering, invoke **0xb7b90 at0x28c7c6**
with the **adapted selected item** from section4 and these inputs:

|Input|Main item call|Equipped comparison call at0x28c918|
|---|---|---|
|Item record|Temporary local-player-adapted selected item|Original equipped record at +0x210, without a new caller adaptation|
|Quantity argument|Preview signed32+0x1f8|1|
|Start X|14|14|
|Start Y|25|Truncation-to-signed32 of the completed comparison start Y in7.3|
|Formatter size scalar|1|0.75|
|Wrapping-width argument|Signed32 260|Signed32 260|
|Name-heading enable|1|1|
|Class-mismatch-warning enable|1|0|
|Twelfth argument byte|0|Preview+0x1fc|
|Ground-origin record|Preview+0x1e8|Null|
|Comparison/difference item|Preview+0x210|Null|

The quantity argument is **not read by this original0xb7b90 body**.
The twelfth argument byte is likewise **not read**. Their argument
slots and all body reads were checked in instructions, not assumed
from the decompiler's parameter labels. In particular, the ordinary
formatter does not produce a stack count, hide its text for count0, or
change comparison styling merely because those supplied arguments
differ. This does not remove the real quantity fields from their source
records or the separately rendered grid/Ring/palette counters.

Preview+0x1fc starts0 in0x28c0a0 and is not reset by the selector;
forwarding it at0x28c8cb–0x28c8d2 does not establish an active formatter
feature. The name and class-warning enable bytes, by contrast, are
actually tested at **0xb7e92** and **0xb89c6** respectively.

Formatter entry selects owner `resource1.dat`, sets line-spacing1,
base font **12×size scalar**, stroke3, black outline RGBA(0,0,0,1),
text flags **0x10**, and wrapping width **float32(signed32 width
argument)**. The wrapping-width conversion is0xb7e6e–0xb7e7c. Stroke3
is not multiplied by the size scalar in that base setup. Name enabled
selects font **15×size scalar**, stroke4, and the existing regional
rarity color0x10a600 using the current local player's region pair.
These settings are0xb7dde–0xb7ee3. They are GUI text-helper inputs,
with the normal Widget transform/font scaling, not fixed physical pixels.

The heading name uses **0x1628b0** on the supplied item. For category3..9
or26 it appends literal `+` when the item's unsigned seed+8 is divisible
by **(rarity byte+0x14+1)×10**. No quantity prefix/suffix is introduced
here. The local player's class drives the admitted main class-warning
block: if0x1094d0 rejects it, the formatter lists each admitting class
in order **`Warrior `,`Ranger `,`Mage `,`Rogue `**. The comparison call's
class-warning enable0 suppresses that block. Other category rows and
effect text remain the explicitly bounded formatter scope below.

### 7.3 Comparison heading and actual returned text endpoint

The first formatter returns its completed local XY endpoint. Let Y be
that returned float32 Y. At **0x28c7cb–0x28c807**, first resize the
preview using current local effective width from0x32a1d0 and requested
height **max(Y,200)** through0x32c9e0, with its final flag1. This does
not replace the endpoint Y with200.

If +0x210 is nonnull, add20 to the endpoint in float32 and draw literal
**`Currently equipped:`** through0x269210 at **0x28c8a6**, at local
**X14,Y+20**. It explicitly sets font12, stroke2 and main white
RGBA(1,1,1,1), retaining the ordinary black outline and current wrapping
settings. Add another20 to that same endpoint in float32, truncate the
completed value to signed32 at **0x28c8bd**, and use that integer as
the comparison formatter's start Y. The two20 additions are separate;
the second call does not start after the minimum200-height frame edge.

After the second formatter, copy its returned endpoint and resize again
at **0x28c95e**, using a **fresh**0x32a1d0 width query and
height **max(comparison end Y,200)**. No second item icon is submitted
for the comparison. The comparison uses its original equipment record,
quantity argument1 and size scalar0.75 exactly as7.2 specifies.

### 7.4 Which size the earlier selector observes

The resizing helper **0x32c9e0** is not interchangeable with the direct
constructor request0x32c9a0. It constructs the accumulated matrix of the
preview root's **parent**, multiplies each supplied width/height by that
matrix's first float **M00**, then forwards the resulting pair and flag
to the ordinary frame-size setter **0x32c6f0**. This is the native scalar
operation, not an AABB calculation or a second full parent inverse.
Owner minimum/bind/deformation behavior remains the shared Widget
contract. Each of the two reached resizes performs its own width query;
do not silently cache one earlier width across both calls.

The frame's earlier selector0xb6b00 positions using the Widget geometry
that exists **before this later draw**. Its0x32ad10 height query therefore
sees the last actually stored effective size, transformed through the
normal parent-relative query. There is no separate preview-height cache
or fresh formatter measurement in that selector. The draw's final
reached resize remains in the ordinary Widget for later queries until
another size writer/normal owner update changes it.

The selector does not reset frame dimensions when its source clears or
the comparison pointer becomes null. Hidden/skipped item draws do not
run these two item resizes. On a subsequent admitted item draw without
a comparison, the main resize alone can shrink the previous comparison
height, **after** that frame's earlier placement used the older geometry.
Other location/lore draw branches retain their separate size writers;
this item-only contract does not claim that they leave height unchanged.

### 7.5 Remaining formatter boundary

Source selection, comparison record identity, the actual argument roles,
quantity/unused-byte behavior, heading placement and frame-size timing
above are stable. The complete category-specific stat rows, comparison
delta formatting and effect-description paragraphs in0xb7b90 are not
newly specified by this checkpoint. They must not be replaced by invented
generic text on the strength of this narrower call contract.
