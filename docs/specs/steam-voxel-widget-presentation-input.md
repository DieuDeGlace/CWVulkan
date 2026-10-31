# Steam VoxelWidget presentation and input addendum

Native-only clean-room supplement to
[the edit-view contract](steam-weapon-customization-edit-view.md) and
[the palette contract](steam-weapon-customization-palette.md). This covers
the manual text and input/visibility consumers needed to present the existing
weapon-customization window. It does not specify replacement art. Sources
are the Steam executable's instructions, constants and decompile; addresses
are RVAs with image base 0x140000000. G is Game. VoxelWidget is G+0x368,
Interface+0x250. It is distinct from the Upgrade/AdaptionWidget at G+0x370.

## 1. Manual text submissions

Draw callback **0x292990** selects the following text settings before its
heading: font size **12**, stroke width **3**, outline RGBA **(0,0,0,1)**,
and flags **0**. The constructor's font is the owner font `resource1.dat`
(0x292380). These are caller GUI font units, not independently fixed physical
pixels. Text is submitted through the ordinary Widget helper **0x269210**;
retain its normal frame/font-scale/text-transform behavior. The coordinates
below are that helper's local input positions, not manually projected screen
positions or glyph-ink bounds.

|Text|Local input position|Flags|Main RGBA|
|---|---|---|---|
|`Weapon Customization`|X=15, Y=25|0|(1,0.8,0.5,1)|
|`Upgrades ` followed by used count, `/`, cap, LF|X=20, Y=current local effective height minus20|0|(1,1,1,1)|
|Each palette record's signed quantity, followed by LF|X=60+45×column, Y=80+45×row|2, right aligned|(1,1,1,1)|

The heading is a literal20-code-unit string, not a dictionary lookup. Its
submission is **0x292ad0**. The float size/stroke constants are **0x42e59c=12**
and **0x42e580=3**; heading X/Y constants are **0x42e7a4=15** and
**0x42e5a4=25**. Heading RGBA is **0x438110**: the green component is stored
float32 0.800000011920929. Black is **0x42e660**, white **0x42e8c0**.

Immediately after the heading, **0x292990** changes its stored text color to
white. It does not change that color, font size, outline color or stroke for
the remaining manual text. The count submission **0x294a65** occurs after
the raw edit image/attributes and palette item icons, with GUI rendering
restored. Its height input comes from **0x329e00**, followed by float32
subtraction of20. It uses the currently selected original equipped record,
not the semantic thumbnail item. The used count is the unsigned byte
**item+0x9d**; it is formatted as unsigned decimal. The cap is formatted as
signed decimal. **0x109c20**, called at **0x2949dc**, returns32 only for
category3 and subtype in **{5,6,7,8,10,11,15,16,17,18,23}**; all other
records return16. It checks subtype<=23 before the bit-mask test
(mask **0x878de0**). The counter is not clamped to the cap for presentation.
The slash has no surrounding spaces: a typical string is `Upgrades 3/16`
followed by LF. Helper **0x7d2f0** appends LF and flushes the stream; the
newline is part of the submitted string. No selected equipped record means
this count line is skipped; the earlier heading still executes. A missing
selected raw model alone does not skip the count.

After that branch, flags change to2 at **0x294ab7**. Each current palette
record is then formatted from its leading **signed32 quantity**; there is
no positive-quantity filter, absolute value or infinity substitution here.
For palette index i>=0, column is i modulo8 and row is its integer quotient
by8. The X/Y expressions in the table are evaluated as integer arithmetic,
then converted to float32 for **0x269210** at **0x294b6d**. Unlike the icons
in section2, these positions do not manually add the projected root pivot,
snap to screen pixels or multiply the45-unit pitch by GUI scale. They pass
through the shared text helper. Font size12, stroke3 and black outline
remain in effect for the quantities. No footer button or extra instructional
sentence is emitted by these manual text submissions.

## 2. Palette cells and candidate-color input

This makes the presentation boundary of gui-item-icons13.2 explicit. In
**0x292990**, the palette image loop begins after **0x294741**, after the
main edit image and candidate/attribute overlays. It resets the current
palette-hover index **+0x2d8 to−1** there. Let P be the root's live pivot
projected by its current parent-world/local transform with homogeneous
division, as specified by the edit-view contract. For index i>=0:

* Physical left is truncation toward zero to signed32 of
  **P.x+20+45×(i modulo8)**.
* Physical top is truncation toward zero to signed32 of
  **P.y+40+45×(integer quotient i/8)**.
* The image center passed to **0xddbc0** is **float(left+20),float(top+20)**:
  integer addition precedes conversion. The half-size is20 physical units.

The offset/pitch in these screen calculations has no additional GUI-scale
multiplier. In particular, the quantity positions in section1 cannot be
obtained by reusing these snapped icon-center coordinates.

The exact sequential float32 evaluation, and the separate ordinary
information panel's hover priority/placement/adaptation, are clarified in
[the information-preview follow-up](steam-voxel-widget-information-preview.md).

The hover test reads the cached GUI cursor floats **engine+0x180/+0x184**
and uses **left<=X<left+40, top<=Y<top+40**. It is a direct rectangle test,
not an additional GUI-tree hit/capture query. It does not require held/down/
release input or positive quantity. A hit writes the index and copies that
palette record's semantic item into **+0x2dc**. On no hit, the index remains
−1; this loop does not clear the old +0x2dc item. Selected index **+0x210**
or current hover selects icon size0.15; ordinary size is0.1. Existing
gui-item-icons13.2 supplies its Euler/spin/depth/dark-pass inputs. Later
click consumers use the stored indices, separately from this draw-time test.

The candidate overlay's material helper **0x109960** is called at
**0x29441b** with input RGBA **(1,1,1,1)** and fourth float argument **0**.
White is loaded at **0x294370**. Carrying chooses material byte **+0x38c**;
otherwise it uses the selected palette record's material. This is input to
the existing material-color table, not a demand that the rendered cube be
white. Its output and the different pulsing input for an already present
hovered attribute remain exactly edit-view section6. Candidate admission
uses the previously stored hover index; the later reset/test above must not
be moved ahead of that admission.

## 3. What catalog index0xa46 identifies

This mapping is established by native catalog initialization, not inferred
from the appearance of an owner asset. Within initializer **0x177f50**,
**0x19c48b** references the narrow filename **`cube.cub`** at **0x455b50**.
The initializer constructs its resource filename at **0x19c4a2**, selects
catalog index **0xa46** at **0x19c4cb–0x19c4d3**, then loads that resource
with **0x16f320** at **0x19c4f0**. Thus this is a file-backed catalog model,
not a procedural primitive created by the edit draw callback. The same
index also serves category10 in ordinary item lookup **0x173b10**.

The edit callback fetches catalog pointer0xa46 only when the pointer-vector
count is at least0xa47; otherwise the pointer supplied to the raw draw is
null. It reuses that resource for candidates and existing attributes.
Native metadata establishes the filename and ID here. This audit does
**not** derive its voxel dimensions, occupied voxels or raw RGB from that
name; those belong to the owner's file and are not a license to synthesize
a substitute asset. Apply the previously specified raw local transform to
the actual resource.

## 4. VoxelWidget alone selects ordinary UI routing

Visibility of **G+0x368** is an explicit positive input to **0x1014f0**.
It therefore makes the ordinary UI predicate true without Inventory,
Crafting, System Menu or any other panel being shown. The complete
predicate is independently listed in ring-selector-rendering section9.
This is input routing, not a simulation pause or a blanket movement freeze.

**0x97880** first sends ordinary UI pointer processing through **0x101160**.
The return immediately following that processing tests **G+0x1958**, the
last-used-pad byte. It is not a controls-binding-capture predicate. When
that byte is clear, held physical right mouse button rotates the edit
pose as specified in edit-view section3, without requiring an image hit.
Ordinary gameplay mouse-look is still possible only through the UI
exception: physical left button held and **0x33f1e0 returns null**.
Captured/hit GUI excludes this exception; rebinding an attack key does not
change that physical-button condition. The exact yaw/pitch arithmetic is
gameplay-camera3.1. Controller UI routing and the frame right-stick preview
rotation remain the existing controller and edit-view contracts.

The full ordinary UI result also reaches player actions **0x8ae50** at
**0x8ee23**. True jumps past late normal/strong/class attack and stationary
dodge handling, regardless of pointer hit or capture. Logical mapped held
bytes are still sampled. The existing earlier movement/jump/ultimate and
ordinary run/walk stages retain their separate gates; moving dodge is
disallowed by the UI test in **0x9c050**. The exact edge-state and stage
distinctions already specified in dialogue-panel-routing section12 apply
unchanged when VoxelWidget, rather than Answers, supplies the true result.
Movement can subsequently dismiss the window in the shared frame close
pass; that later dismissal does not retroactively enable an earlier
attack-stage decision. The shared movement-close conditions and retained
edit state remain edit-view section2.

## 5. Inventory B, Crafting, System Menu and controller B differ

The frame's logical Inventory/Crafting/System toggles are reached from
**0xa5a50** when the actual chat editor byte **(G+0x398)+0x1d8** is zero
and frontend roots **G+0x118/+0x130/+0x170** are all hidden. Those roots
are title, character selection and character creation respectively.
VoxelWidget visibility is not an additional exclusion. The toggles use
their logical binding's rising edge, whose previous state is stored in
this admitted block.

|Input route|Verified effect on VoxelWidget visibility|
|---|---|
|Inventory action17, default keyboard B|**0x102fe0** toggles Inventory G+0x330. Opening Inventory refreshes its region/grid/thumb and hides System G+0x358. It does not hide/toggle G+0x368.|
|Crafting action18, default C|**0x102f10** toggles Crafting G+0x340, rebuilds crafting and performs its documented shown-state refresh. It does not hide/toggle G+0x368.|
|System Menu action19, default O / pad Start|**0x103150** toggles System G+0x358; opening System hides Inventory G+0x330. It does not hide/toggle G+0x368.|
|Controller B, mask0x2000, while the ordinary UI predicate is true|The successful-poll shared close batch explicitly hides G+0x368 at **0x1379f3–0x137a13**.|

The controller B branch is a held-state test at **0x1378f8**, not a new
Widget-specific release handler. It sets the common B-consumed byte
**0x551cbc=1** at **0x137905**, preventing that held-through-close press
from becoming an ordinary gameplay jump. The shared batch performs
visibility writes; it does not run the bench's reset routine, refund a
carried cube, clear carry/selection, reset Euler angles, or clear the
palette-hover record. Its other panel writes remain the shared controller
contract, not a policy that every possible UI root must close.

Escape is a separate native key route: **0x96e70** toggles **G+0x420**.
That key's direct branch is not the Inventory B toggle or a direct
VoxelWidget close. This bounded addendum does not redefine every consumer
of that separate flag. Returning to the title through System Menu also
has its established broader close/reset path; it must not be conflated
with merely showing System. No exclusive-modal policy follows from the
three ordinary panel toggles above.

## 6. Scope and remaining unknowns

These text, catalog mapping, hover and listed control/visibility facts are
stable. Owner art, exact file geometry and resulting glyph ink remain
owner data/shared-renderer inputs. This addendum does not establish every
possible callback that can show/hide the window or a new binding for the
unreached Upgrade-panel show path. It does not infer drag/drop, undo,
automatic putback or pointer-confinement behavior beyond the existing
native input contracts.
