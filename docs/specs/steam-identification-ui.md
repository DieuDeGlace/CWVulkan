# Steam identification widget appearance and placement

Independent native behavioral prose. Sources are the executable, decompile,
assembly and prior independent plasma/item specifications. No engine, tests,
exporter, build files or owner captures were inspected; no game was run.
Addresses are RVAs. Business actions remain loot-shops §5; controller points
and release hit tests are consolidated in steam-controller-handoff §1.

## 1. Owner templates and construction

Interface constructor **0xf9ac0**, constructor call **0x10055e**, creates
**IdentificationWidget**, size0x208, installed at **G+0x360**
(interface+0x248). It clones the owner **gui.plx `blackwidget`** template
under the main GUI parent node **G+0x220**. That clone becomes the widget
root **+0x178**. It obtains the owner **`itemframe`** template separately.
The identification constructor **0x277060** clones that itemframe under
the new blackwidget root and stores its node at widget **+0x200**.
The item's frame widget is node+0x70. Transform/display copies are made
independent so this instance's geometry/state can change separately.

Use the owner template tree, drawing shapes, fills, inner bounds,
deformation, visibility and child order. Native does not replace the
blackwidget with a newly styled rectangle or create separate graphical
footer-button templates. The Identify and Goodbye controls are drawn
text plus footer hit regions. The root is resized through **0x32c9a0**
to **350×285 GUI frame units**; the effective minimum/deformation rules
remain plasma-format-and-scene §2.7. It is initially hidden through
**0xa3a20**. Opening changes that same root's visibility.

Initial selected inventory tab/index **+0x1e8/+0x1ec** are both−1;
additional signed32 gate word **+0x1f0** initializes1. It is not a
verified mouse-press latch; see §6. Font lookup binds
**`resource1.dat`**. There is no different owner identification-specific
window asset or embedded title shape requested by this constructor.

The itemframe retains **its owner-defined dimensions**; the identification
constructor does not replace its width/height with a hardcoded square.
Layout callback **0x277e60** sets its parent-relative frame position to
**((identification effective width−itemframe parent-relative width)/2,
50)** through0x32b560. The width is from0x32da30 and identification's
effective width from0x32a1d0. Thus bind/deformation and ancestor scale
continue to participate; do not center using an arbitrary icon bitmap.

## 2. Placement beside inventory and GUI scaling

Interface layout **0x101bd0**, specifically the call at **0x10269a**,
positions this root with the shared **0x334700 /0x334510** point-and-offset
placement operation. In the layout's viewport coordinate system:

- X anchor: viewport width's **signed integer half**, converted to
  float, plus **0.5 times ordinary inventory's parent-relative width**.
- Y anchor: viewport height's **signed integer half**, converted to
  float, minus **ordinary inventory's parent-relative height**.
- Additional local offset: **(10,−50)**.

The ordinary inventory here is **G+0x330 /interface+0x218**.
Widths/heights come from0x32da30/0x32ad10; this uses inventory's current
parent-relative dimensions, not identification's350×285 dimensions.
Integer halves precede float conversion, so odd viewport sizes retain
the native one-half truncation. The additional10/−50 are the separate
offset argument, not additions guessed in physical pixels.

The shared placement helper maps the supplied anchor through the
parent/ancestor inverse placement before applying its local offset
and updating root translation. Preserve that operation and the existing
GUI scale/parent frame system. It is not equivalent in all scaled or
transformed trees to simply writing these numbers as node translation.
No independently fitted center-screen modal position is introduced.

## 3. Draw order and typography

Draw **0x277250** starts by resolving the selected item using0x277c00.
It sets **font size12, outline radius3, black outline RGBA(0,0,0,1)**
and text flags0. The font is resource1.dat. These use the ordinary
two-pass scalable GUI text contract **0x269210**, with native baselines,
GUI scale and glyph metrics; see plasma-shapes-and-text §§4/7.1.

After the owner tree has provided the panel/slot artwork, this callback's
manual content order is:

1. If the selected item resolves: its rotating3D icon, then its ordinary
   item name/details block.
2. Literal title **`Identification`**.
3. Literal footer **`Identify`**, then its navigation point.
4. Literal footer **`Goodbye!`**, then its navigation point.
5. If the item resolves: the cost label and value.

All caption/footer/cost positions below are **widget-local GUI units**.
The text draw's return position remains the native pen position in that
space, rather than a second separately estimated text width.

| Text | Origin/baseline | Flags / color |
|---|---|---|
| Identification |(15,25) | Left aligned; RGBA(1,0.8,0.5,1) |
| Identify |(W/3,H−20) | Horizontally centered, flag1; colors below |
| Goodbye! |(2W/3,H−20) | Horizontally centered, flag1; colors below |
| COST: |(15,trunc32(H−50)) | Left aligned, flags0; white |
| decimal cost followed by ` GOLD ` | Returned pen after COST:, same truncated baseline | Left aligned; RGBA(1,0.9,0,1) |

Caption and footer font size is12 and outline3. With a resolved item,
the shared detail routine temporarily enables word wrapping for its
item text; the title follows that routine without a separate flags reset
until the footer sets flag1. Its short title still starts at(15,25).
Price explicitly resets left-aligned flags0. The full cost phrase is
split into two color runs; there is a space after the colon and a space
before/after GOLD. No currency icon or affordability-red value is drawn
by this path.

### 3.1 Footer visual states

**Identify with no valid selected item:** RGBA **(0.7,0.7,0.7,1)**,
constant0x46ce40. It still registers a navigation point. With a valid
item it is white **(1,1,1,1)**, or cyan **(0,1,1,1)** while strict hit
test0x277d70 is true. Goodbye uses white/cyan according to0x277cd0,
independent of whether an item resolves. Hover color therefore follows
the strict footer hit region, not the text's ink bounds.

The draw does not choose an additional pressed color or displaced
caption from+0x1f0, and it does not gray Identify solely
because the player lacks gold. Affordability belongs to release admission.
Cost is shown **only with a resolved selected item**; empty state draws
neither `COST: 0` nor an invented placeholder instruction/name/icon.
The itemframe's owner artwork itself remains part of the tree.

## 4. Item icon and item-information inputs

The selected item is **the existing original category15 record**, with
its inventory stack required nonzero. Native does not identify/generate
a result merely to preview it. The selection points into the live
inventory through widget tab/index, rather than a cloned item record.

The icon goes through the ordinary native GUI item renderer **0xddbc0**,
using its ordinary item-sprite selector **0x173b10**.
Its screen center is the itemframe's **world frame origin0x32cf50 plus
half its world frame size0x32d010**, both queried on itemframe node+0x70.
These preserve the copied frame's bind geometry, effective dimensions
and full current placement. Submitted icon scale is **float32(0.06)
times the composed identification-root matrix's X scale**. The renderer
then applies its ordinary header-dimension/GUI projection normalization;
0.06 is not a replacement CUB world scale or a fixed-pixel width.

Submitted Euler degrees are **(225,0,phase)**, where phase is the GUI
engine's signed32 clock at **widget engine+0x194**, signed remainder10000,
converted to float, multiplied by stored float **0.0001**, then by360,
with those two float multiplications in order. It uses no world clock.
The ordinary icon routine's additional-depth input is0 and its extra
dark icon-pass option is enabled. Native restores the GUI engine's
ordinary drawing context immediately afterward. The precise shared icon
projection, CUB surface drawing and icon shader remain that renderer's
contract; this specifies the identification-specific inputs.

The shared information draw **0xb7b90** receives this widget and item,
starting local origin **(15,180)**, scale1, integer wrap width300, name
enabled, class-warning disabled, and no comparison/equipped-item context.
It draws the **0x1628b0 ordinary item-name result**, size15 outline4,
then advances from the returned last baseline by24. Body style is
size12 outline3, line spacing1 and word-wrap flag0x10 at width300.

Category15 participates in the existing item region-description mask.
Its next body run uses the ordinary dictionary land-description `named`
phrase, generated from the item's **original region XY**, the region's
0x2b9080 kind, and deterministic region name seeded by the existing
R(x+57y) generator. It does not substitute the current player's region
or a prospective identified item's name/region. The next body baseline
starts at204 when the name occupied one line; wrapped names advance from
their actual returned last baseline instead.

Item name color **0x10a600 /0x109720 /0x64680** uses effective rarity.
For category15, original item region must equal the player's current
region to retain its rarity; otherwise effective rarity is−1. The colors
are:−1 gray(0.7,0.7,0.7,1),0 white,1 green(0,1,0,1),2 blue(0.3,0.45,1,1),
3 purple(0.55,0.2,1,1),4 yellow(1,1,0,1), with higher values clamped4.
The region-description line instead uses **(1,0.9,0.5,1)** when effective
rarity equals the original rarity byte, otherwise **(0.75,0.75,0.75,1)**.
The remaining optional shared item-stat lines retain their ordinary
item-calculator predicates; no identification-specific speculative
equipment stats or comparison columns are added.

## 5. Data-owned appearance and bounded unknowns

The constructor/frame/slot placement, draw inputs, positions, phases and
colors above are verified and stable. The actual blackwidget/itemframe
outline, fill, bind/inner rectangles and design dimensions are **owner
gui.plx data**, not hardcoded coordinates in0x277250. They must be read
with the existing independent plasma format/shape specifications. This
pass did not inspect an owner asset/capture or invent its numeric paths.
No runtime screenshot oracle or additional general icon-renderer audit
was performed. The known native template binding and frame rules are
enough to avoid treating that data-owned artwork as unspecified geometry.

## 6. Exact down/up gate, selection binding and clearing

**Correction to the older loot-shops §5's inferred “pressed” wording:**
global **0x5514d7** is an **identification-was-visible-at-down snapshot**,
not a physical held-button bit. At entry to mouse-down **0x94900**,
after0x101020, it stores whether this identification root's live visibility
is nonzero. This occurs **for every button number**, before the later
button-specific branches/early returns. It is overwritten on the next
down event, and the identification up branch does not clear it.

Mouse-up **0x965e0** enters identification actions only when **the root
is currently visible AND that snapshot is nonzero AND released button
number is0**. A panel first opened after a down event that observed it
hidden cannot consume that opening release. It does **not** require
the pointer to have been over Identify on down, or a separately invented
left-physical-held flag at up. Releasing over Identify after pressing
elsewhere while the panel was already visible passes this visibility
gate, subject to the ordinary item/price gates.

Identify further requires its current strict hover test, price≤gold,
**signed32 widget+0x1f0 exactly1**, valid stored tab/index, nonzero live
stack count and category15 through0x277e30. Constructor0x277060 initializes
that word1. **0x94900 does not set/reset it as a mouse-press state**;
neither the selection branch nor identification/Goodbye up changes it.
The verified contract is that additional word comparison, not an assumed
press-capture state machine. Goodbye uses its current strict hover test
inside the same shown/snapshot/button0 outer gate, without requiring
price, valid item or+0x1f0=1, then hides identification and inventory.

### 6.1 Inventory selection happens on down and stays an index pair

After down events with button number **0 or1**, **0x94600** returns the
ordinary inventory's current hover **tab+0x270 /slot+0x274** only when
inventory is visible, its frame hit test0x32b0c0 passes, both indices
are valid and that live stack count is nonzero. Otherwise it returns
**(−1,−1)**. With a returned valid pair and identification currently
visible, **0x94900 at0x95033** copies those two indices to identification
**+0x1e8/+0x1ec** and leaves the ordinary equip/sell path for that pair.

This pair assignment **does not filter category15**. Clicking another
nonempty inventory item can bind its indices while the identification
resolver returns null, showing an empty/disabled visual state. The resolver
**0x277c00** rechecks bounds/count/category15 against the live inventory
on each use/draw. It stores neither an item copy nor a stable item ID.

No identified slot-clear action is attached to clicking the decorative
itemframe. Clicking an empty/noninventory location does not assign the
helper's(−1,−1) result into identification: that down goes through other
ordinary routes, leaving the identification pair alone. **Goodbye,
reopening through class0x83's0x98180 Talk branch, and successful identify
also do not reset the pair.** Opening sets visibility and opens inventory
via0x102db0; it does not reconstruct the widget.

Success decrements the referenced stack; if it drops below1, native clears
quantity/type/subtype. It leaves the stored tab/index unchanged while
adding the new result and rebuilding inventory displays. With more
category15 items still in that slot, the same binding remains usable for
the next release. With an empty or changed-type slot the resolver becomes
null. If the same indices later contain another nonzero category15 stack,
the existing pair resolves to that new live content; there is no native
item-identity snapshot check to prevent that rebinding.

## 7. Exact category15 region-description dictionary call

The shared item information draw **0xb7b90** expands this line through
**0x16be20**, using **Game's dictionary object G+0x50**. It is a plain
string lookup/expansion, not a dialogue script compiled into G+0x418 or
an NPC bubble. Its dictionary node is selected by **0x16b210**, using
**0x2b9080(World,item.regionX,item.regionY)**. The exact node IDs are:

| Region kind | Dictionary node ID |
|---|---|
|0|Plains|
|1|Ocean|
|2|Hills|
|3|Woodlands|
|4|Jungle|
|5|DarkWoods|
|6|SnowForest|
|7|Mountains|
|8|Wetlands|
|9|Desert|
|10|Savannah|
|11|Snowlands|
|12|Deadlands|
|13|Firelands|

There is no `lands:` prefix added by this helper. Other region kinds
produce the empty node ID, rather than an invented Plains fallback.
Node definitions remain the loaded owner dictionary data at its node
map **dictionary+0x98**.

The fourth0x16be20 input is literal **`named`**. That helper constructs
a fresh qualifier set containing literal **`#named`**, and a fresh string
substitution map containing **literal key `@`** mapped to the generated
proper region name below. Its other expansion context is empty/default;
it does not bind `@land`, a speaker, the hovered NPC or an item award.
It calls **0x16a790(dictionary,outString,nodeID,qualifierSet,stringMap,0)**,
so the top-level dictionary selection seed is **0**, not the region hash.
Native grammar expansion/flattening then produces the string drawn at
the body baseline. The owner grammar decides the name/terrain-word order
and punctuation. It is not a manual concatenation of display biome name
and proper name. The resulting plain string receives the single body-line
RGBA in §4; embedded word colors are not individually submitted here.

### 7.1 Proper-name input and all relevant generic fragments

First compute **s=R(uint32(regionX)+57 times uint32(regionY))**, wrapping
32-bit arithmetic, with the existing0x2ff5c0/region R function. Then call
the deterministic name generator **0x611d0(out,s,−1)**. Its−1 argument
selects the generic30-prefix/30-suffix family, **not the terrain-kind
number**. Within that family prefix index is **R(s−57) modulo30**;
suffix index is **R(57 times s+0x3acaa6) modulo30**, all wrapped32-bit.
It concatenates the selected prefix then suffix with **no separator**
through0x4cc10. Prefix case is data; there is no additional case-normalizer.

The generic family is initialized inside0x611d0 at the runtime string
records **0x54e2c0+32i** and **0x54e690+32i** respectively. Each is an
MSVC32-byte **UTF-16LE wide string**: union of inline16-byte characters or
pointer at+0, unsigned64 length at+0x10, unsigned64 capacity at+0x18.
Capacity≤7 uses the inline characters, otherwise follow the pointer.
Those runtime records are filled lazily; their on-disk zero-initialized
storage is not a string table.

These fragments are **owner game data**, not replacement vocabulary to
embed in the engine. For an owner mapped image of the supported Steam
build, read the following literal RVAs and exact **UTF-16 code-unit
lengths**, excluding NUL. Source references are the0x611d0 initializers
through0x3d4f0 or0x41d80; a repeated RVA intentionally shares data. PE file
reads must translate RVA through section headers rather than use RVA as a
file offset. A different build needs its own verified address manifest.

|Index|Prefix literal RVA|Length|Suffix literal RVA|Length|
|---|---|---|---|---|
|0|0x431d60|3|0x431e58|2|
|1|0x431d68|3|0x431e60|2|
|2|0x431d70|2|0x431e68|2|
|3|0x431d78|2|0x431e70|3|
|4|0x431d80|3|0x431e78|3|
|5|0x431d88|2|0x431e80|3|
|6|0x431d90|3|0x431e88|3|
|7|0x431d98|2|0x431e90|3|
|8|0x431da0|3|0x431e98|3|
|9|0x431da8|3|0x431ea0|3|
|10|0x431db0|2|0x431ea8|3|
|11|0x431db8|3|0x431eb0|3|
|12|0x431dc0|2|0x431eb8|3|
|13|0x431dc8|3|0x431ec0|4|
|14|0x431dd0|4|0x431ed0|3|
|15|0x431de0|4|0x431ed8|2|
|16|0x431dec|2|0x431eb0|3|
|17|0x431df8|4|0x431ee0|4|
|18|0x431e08|3|0x431e70|3|
|19|0x431e10|3|0x431ef0|3|
|20|0x431e18|3|0x431ef8|3|
|21|0x430e28|1|0x431f00|3|
|22|0x431e20|2|0x431e80|3|
|23|0x431e28|3|0x431f08|3|
|24|0x431e30|3|0x431f10|3|
|25|0x431e38|2|0x431f18|3|
|26|0x431e40|2|0x431f20|3|
|27|0x431dc8|3|0x431f28|4|
|28|0x431e48|3|0x431f38|3|
|29|0x431e50|2|0x431f40|3|

The name uses deterministic R evaluations. The grammar uses its existing
local xorshift/counter selection with seed0. If its top-level node has a
variant vector, xorshift(0)=0 chooses variant0 and recursive expansion
continues with its normal incremented local seed; do not force every
nested choice to index0. The region-name hash is only an input to0x611d0,
not a new seed for0x16a790 or the game's global random state.

Repeated draws therefore consume no crafting/drop/identify CRT draws.
An absent dictionary node yields empty expanded text; the code does not
fall back to a fabricated English region sentence. This lookup and its
temporary qualifier/substitution data are rebuilt by the shared detail
draw, rather than writing persistent dialogue pages or marks.

## 8. Shared icon projection and material

The independent [shared GUI item-icon contract](steam-gui-item-icons.md)
now specifies0xddbc0, including the header-centered transform, negative-aspect
45° projection, optional black offset pass, main DepthOffset, fixed palette,
Shininess producer and weapon spirit cubes. Use §4 here for identification's
actual center/scale/Euler arguments and that shared contract for their3D
meaning. No icon-renderer behavior remains intentionally inferred from the
identification text layout.
