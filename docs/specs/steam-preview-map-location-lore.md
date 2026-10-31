# Steam PreviewWidget map coordinates, flight target and lore

Independent native-only supplement to [map markers](map-markers.md),
[world-map records](worldmap-model.md), [lore/artifacts](lore-and-artifacts.md),
[preview item drawing](steam-voxel-widget-information-preview.md) and
[hidden-interface ordering](steam-preview-hidden-interface-order.md).
Steam RVAs below identify native sources. No implementation, tests,
exporter/build, SDK, owner assets/captures or game execution were used.

## 1. Coordinate meanings, producers and sampling

The two Preview pairs are not interchangeable generic map positions:

| Preview fields | Copied source | Coordinate meaning |
|---|---|---|
|+0x218/+0x21c|Map renderer G+0x10, +0x150/+0x154|Signed32 **block XY flight target**|
|+0x220/+0x224|MapOverlay G+0x380, +0x250/+0x254|Signed32 **16-block cell XY of a lore marker**|

Both copies occur in selector **0xb6b00**, called at **0xa9d22**.
The sentinel is **INT32_MAX** in X; normal producers reset both axes.
The selector uses a non-sentinel X from either pair to show Preview
even when its separately selected item pointer is null. It copies
the scalar words; these are not pointers into mutable map output.

Initialization is consistent with that sentinel: WorldMap constructor
**0x2eaa00** sets its flight pair to MAX/MAX; Overlay constructor
**0x282170** sets its cell pair to MAX/MAX; Preview constructor
**0x28c0a0** sets both copied pairs to MAX/MAX.0x2eaa00 does not
explicitly initialize the separate byte+0x158. Its meaningful producer
is the accompanying flagB hover write below, not an assumed default
derived from whether an arbitrary unselected point is a flight station.

The producing ordinary draw is **MapOverlay0x2829c0**, after the
current frame's selector. Its entry resets map flight target+0x150/
+0x154, shrine target+0x15c/+0x160 and hovered land-map cell+0x118/
+0x11c to INT32_MAX pairs. Collection and sorting follow. After sorting,
it resets its embedded item+0x1b0 and own cell+0x250/+0x254.
The latter reset is around **0x285b41**.

Hover selection then requires **full map G+0x668 nonzero and drag
G+0x1db0 zero**. In restricted map mode+0x669, only flagA records
participate. Projection/admission, scale, hit rectangles, sorted order
and strict nearer-distance tie behavior are exactly map-markers
§§1.3–1.7. A newly nearest admitted hit writes only the fields its
flags govern; earlier outputs from another hit are not cleared again.

For the flight pair, marker **flagB at+0x81** copies its signed32
block X/Y at+0x60/+0x64 into map+0x150/+0x154. It also writes map's
**byte+0x158 to marker flagE==0**. Natural flight-point records
(map-markers1.3.4) have B/C/E and are collected only in map flight mode
**map+0x164**. Other-player records gain B during that mode, with E
still0 (map-markers1.4.1); their block destination handling remains
that section. Shrine flagA instead writes+0x15c/+0x160 and does not
itself create the Preview flight pair.

Flight mode itself starts0 in0x2eaa00. The admitted flight-master
use in **0x98180, NPC role0x9a**, sets both full-map G+0x668 and
map+0x164 to1. The early Game-frame stage clears map+0x164 at
**0xa6fe9** when G+0x668 is0, before ordinary use0xa9b2c and preview
selection0xa9d22. A later admitted use can therefore open flight mode
before selection without producing a new hover pair until GUI drawing.
Ordinary toggle0x1031e0 changes only+0x668. These input-state writers
do not themselves populate/reset the copied flight coordinates.

For the lore-cell pair, **flagC at+0x82** first tests marker item
pointer+0x88. A nonnull pointer with nonzero category semantically
copies its item into Overlay+0x1b0. Otherwise, **flagD at+0x83**
copies floor(blockX/16), floor(blockY/16) into Overlay+0x250/+0x254.
That floor uses signed coordinates, not truncation toward zero.
The ordinary place-record producer sets C/D for **plot kind14 lore
buildings**, after its existing used-cell admission; these are not
role14 artifact missions. Marker creation and the used-cell set remain
map-markers1.3.1 and lore-and-artifacts4.5.

The flagB pair store is **0x285de7**, C-item copy **0x285e31** and
D-cell pair store **0x285e97**;0x2829c0 is the source for all
decisions above. Its reached minimap/full-map-closed or dragged draw
still performs the resets but skips hover selection. Hidden/skipped
drawing retains prior outputs. There is no reset or new producing draw
between the usual previous Overlay draw and selector0xa9d22; the next
Overlay draw is later in GUI rendering. The exact F4, traversal and
borrowed-item/scalar timing is the hidden-interface supplement.

**Map+0x158 remains a live separate input.** The selector does not
copy it into Preview. Flight text later reads the map's then-current
byte, while its target XY comes from the earlier scalar copy. The
visited draw's entry resets do not reset this byte; a reached flagB
hit updates it. This temporal distinction also applies when current
draw hover changes after the earlier selection.

## 2. Draw precedence and item suppression

Every reached **Preview draw0x28c290** first hides its four named
children **`star1`..`star4`** through their own live visibility values.
It then chooses these branches, in this priority:

1. Flight X+0x218 differs from INT32_MAX: call **0x28cc30**. There is
   no simultaneous lore or ordinary-item draw in this branch.
2. Otherwise lore-cell X+0x220 differs from INT32_MAX: call
   **0x28e640**. Its internal lore-building validation can reject
   the cell, but the caller does not fall through to an item.
3. Otherwise, nonnull selected-item pointer+0x1f0 reaches the existing
   adapted ordinary-item/comparison branch, described in preview §§7–8.
4. With no item, nonnegative+0x200 reaches0x28d320; otherwise
   nonnegative+0x208 reaches0x28db50. The ordinary selector resets
   +0x200 and+0x208 to−1 on each pass. These additional detail-widget
   producers/text contracts are not extended by this map supplement.

Only X is the sentinel gate. Flight/lore are selected without testing
their Y sentinel, item category/count, the current map-open byte or
whether the prior producing marker is still hit. Their sources are
the copied coordinates, not a new cursor raycast. Any selected item
and previously computed equipment comparison remain stored even when
one of these coordinate branches suppresses their rendering.

These hooks retain the established **after-background/children**
callback timing. A reached branch's late resize affects subsequent
geometry queries, not the already submitted background of this draw.
Sources:0x28c290,0xb6b00; item preview7.4/7.6.

## 3. Exact marker item payload provenance

The ordinary marker+0x88 producer in0x2829c0 borrows a nonempty item
from the region model's **mission-list node+0x28**. The mission value
record begins at node+0x10, so this is **record+0x18**, not a pointer
to a separately generated display-only icon item. Node+0x40 is that
same item's+0x18 realm/formula word. The mission record is0xc0 bytes;
worldmap-model §5 and §6.1 give its other fields and generation gates.

Ordinary generator **0x2eb7b0** obtains that item from planner
**0x119660(planner,placeX,placeY)** and copies it through0x9cd0 before
appending the mission value. Planner artifacts are made by **0x11d7b0**:
category23; subtype X(counter) modulo7; modifier X(counter+1); item
region pair equal to the normalized **place XY**, not biome-region XY;
rarity0; item+0x18 equal to that place's realm. Material+0x1c is0,
all32 four-byte attributes at+0x1d..+0x9c are0 and attribute count+0x9d
is0. The constructor explicitly initializes those fields; these are
verified generator outputs, not defaults invented from the icon.
Counter ordering and realm generation remain lore-and-artifacts §5.1.
0x119660 returns its semantic empty singleton when no artifact exists.

Hover copying does **not** require category23, regenerate the planner
item or replace fields from mission level/name. It copies the actual
nonzero-category inline item. **0x9cd0** transfers category+0x00;
the full32-bit words+0x04,+0x08,+0x0c,+0x10,+0x18; rarity byte+0x14;
material byte+0x1c; **all128 attribute payload bytes+0x1d..+0x9c,
including unused entries**; and count byte+0x9d. It does not copy padding
+0x01..03,+0x15..17,+0x9e..9f. Thus the old shorthand“0x9e copied”
describes the covered span, not a contiguous raw memcpy160.

If a supplied mission record contains other semantic field/attribute
values, this consumer preserves them. The proven ordinary producer's
zero-attribute artifacts are not permission to reconstruct an arbitrary
160-byte supplied item by category/subtype while discarding its payload.
This subsection identifies the ordinary native producer and consuming
copy; it does not invent a separate native quest-award/raw-item producer.

## 4. Flight-point text and late geometry

This is **0x28cc30**, selected solely by §2's non-sentinel flight X.
It does not resolve a place/mission name, look up a region record or
retest full-map/flight-mode/marker admission. Its target is the copied
signed32 block pair Preview+0x218/+0x21c. Current player data comes
from World+0x448. Map byte+0x158 is read live, as §1 distinguishes.

### 4.1 Exact drawing state, rows and height

Use Preview's current font resource, initialized to owner
**`resource1.dat`** by0x28c0a0. The flight helper selects **font12**,
**outline3**, text flags **0x10**, black outline **(0,0,0,1)**.
These are GUI text units under the ordinary Widget/GUI transform.
It initially sets wrap width to queried local effective width W minus20,
but **replaces this with W minus30 before the first submitted text**.
It briefly sets font15 during an unused target/home-pair comparison,
then restores12 at **0x28cd20**; the actual heading is font12.

This helper does not reset character or line spacing. Preview's
constructor initializes line spacing4 via0x2691a0. The ordinary item
formatter sets it to1; the static lore-word helper sets it to2.
Those reached writers persist until another writer changes the Widget.
Character spacing starts0 in base0x268a70 and these branches retain it.

All text uses0x269210. Initial pen is **(14,25)**. Submitted rows are:

| Row | Text | RGBA | Y used |
|---|---|---|---|
|Heading|Literal`Flight point`|(0.5,1,1,1)|25|
|Distance|`Distance: ` plus §4.2's distance string|(1,1,1,1)|Heading's returned pen Y plus30|
|Unknown purchasable target, insufficient money|`Cost: ` plus signed decimal price plus` gold`|(1,0,0,1)|Distance's returned pen Y plus30|
|Unknown purchasable target, enough money|`Purchase for: ` plus signed decimal price plus` gold`|(1,0.9,0,1)|Distance's returned pen Y plus30|
|Other admitted target|`Flight time: ` plus signed decimal seconds plus` seconds`|(1,1,1,1)|Distance's returned pen Y plus30|

Exactly one of the last three rows occurs. Branch predicates and the
price/time inputs are below. All rows retain font12 and outline3;
line wrapping uses the submitted width W−30 and inherited line spacing.
The30-unit Y additions are separate float32 additions to each returned
text Y; wrapped earlier rows therefore move later rows.

After a price/purchase row, request height **that row's returned Y
plus30**. After a flight-time row, first add30 to its returned Y,
then add30 again for the requested height. Query current effective
width again and call **0x32c9e0(width,height,1)**. There is **no200-unit
minimum** in this branch. Its parent-M00 size conversion and the
already-drawn background timing remain preview-item7.4/7.6.

Arithmetic reference with single-line returned Ys, identity parent
and no extra minimum constraint: heading25, distance55, final row85;
requested height **115 for either price row**, **145 for flight time**.
This is derived layout arithmetic, not a runtime capture.
Colors:0x4381d0 cyan,0x42e8c0 white,0x438050 red,0x46ce30 gold.
Font/style and resize source:0x28cc30..0x28d30b.
Text calls: heading0x28cda9, distance0x28ce83, unaffordable cost
0x28cf9b, purchase0x28d058, time0x28d248; resize0x28d2de.

### 4.2 Displayed horizontal distance and units

Distance uses **current physical player fixed X/Y**, not camera,
drawn position, Z or an already truncated player block coordinate.
For each axis, first form the signed64 fixed difference between
**int64(targetBlock) times65536** and the player's fixed coordinate.
Convert that difference to double, then multiply by exact double
**2^-16**, constant0x43ef58. Square each double, add X square plus
Y square, take double square root, multiply by double **0.5** at
0x4ac9a8, then truncate to signed32. This integer is the displayed
distance in metres passed to **0x28c9c0**.

That formatter uses these signed integer boundaries; divisions truncate
toward zero. For ordinary nonnegative distance n:

| n | Output |
|---|---|
|Below10|n followed by` m`|
|10..499|n truncated down to a multiple of10, then` m`|
|500..999|n truncated down to a multiple of100, then` m`|
|1000..9999|Integer thousands; append literal`.` and the hundreds digit only when that digit is nonzero; then` km`|
|10000 or more|Integer thousands with no fractional digit, then` km`|

Examples:9→`9 m`,19→`10 m`,599→`500 m`,1099→`1 km`,
1199→`1.1 km`,9999→`9.9 km`,10999→`10 km`.
Suffix constants0x435fcc/0x435fd8 are literal` m`/` km`;
decimal point0x4300c8 is literal`.`. Source:0x28cc30 and0x28c9c0.

### 4.3 Price-versus-time gate and the displayed price

Read the known-flight-point predicate **0x5f310(player,targetXY)**,
membership in personal-data+0x28 (false if personal data is null).
Show price/purchase text exactly when this is **false AND live
map+0x158 is0**. Otherwise show flight time. The map flag is the
inverse of the selected flagB marker's flagE; it is not a snapshot
of target type inside Preview.

Price is **0x2adce0(World,targetXY)**, returned as signed32. Compare
player's signed32 money+0xaac with it: strictly less chooses red
`Cost: `; equality or greater chooses gold`Purchase for: `.
This draw only quotes text and changes geometry; it does not buy a
point, remove money, insert into the known set or start flight.

The price helper starts with double horizontal distance from the
player to target. Here, unlike §4.2, it first converts each **player
fixed coordinate to double and multiplies by2^-16**, then subtracts
the target's double block coordinate. For every known flight point,
calculate signed32 wrapped point-minus-target X/Y differences,
convert to double, obtain their Euclidean norm, and multiply by
double0.5. Keep it only if strictly less than the current best.
Thus the comparison is between full player distance and **half**
known-point distance, not one uniformly halved minimum.

Bound the resulting best to at least0, truncate double to signed32,
divide that signed integer by100, then choose at least100. Finally
convert this integer to float32 and back to signed32 before returning;
that round-trip is explicit at **0x2ade78..0x2ade7f** and can change
large integers. There is no separate high-price cap. Known-set order
does not change an equal numeric minimum; equality does not replace
it. Source:0x2adce0 and its Preview call **0x28cf17**.

### 4.4 Flight-time calculation and in-progress subtraction

Normally convert current player fixed XYZ to signed whole blocks by
signed64 division by65536 **toward zero**, then narrow each to signed32.
Call **0x2adc50(World,originXYZ,targetXY)**. Z and World are unused
in this helper. It subtracts target from origin in signed32 wrapped
XY, converts differences to double, takes horizontal Euclidean norm,
multiplies by double **2.5** at0x472208, truncates to signed32 and
clamps that integer to **10000..30000 ms**. This is the same duration
helper as creature-abilities-ranged's native0x9c flight contract.

If current ability is **0x9c AND destination block pair P+0x1b0/
+0x1b4 equals the Preview target**, instead recalculate the full
duration from flight-start fixed XYZ **P+0x1b8/+0x1c0/+0x1c8**,
with the same toward-zero block conversion/narrowing. Convert full
integer ms to float32, subtract float32 ability age **P+0x6c**, then
truncate the float result to signed32. There is **no remaining-time
clamp after subtraction**. A different current destination keeps the
ordinary current-position duration.

Divide the final signed ms by1000 toward zero and insert that integer
in the time row. No fractional seconds or pluralization branch is
selected. The distance row still uses the current physical position,
even when the time row uses flight-start duration minus age.
Source:0x28cc30's time continuation around0x28d073..0x28d244;
0x2adc50. Ordinary numeric conversions above assume their integer
intermediates fit; no pre-conversion floating clamp is substituted.

## 5. Lore-cell text, static styled words and late geometry

### 5.1 Validation, realm and percentage inputs

This is **0x28e640**, entered with the copied signed32 cell pair
Preview+0x220/+0x224. First call **0x2b42e0(World,cellX,cellY)**,
at0x28e68b. It accepts cell-object kinds **13,37,55,69,70** from
0x69c60, as already specified in lore-and-artifacts2.3. On rejection,
the helper returns before any of its text/state/size work. Its caller
still suppresses the item branch; it does not retry source selection.

For an admitted cell, calculate its block coordinates with signed32
wrapped multiplication by16, then floor-divide those signed results
by1024 to get place XY. In the ordinary non-overflow range this is
floor(cell/64). Query bigCell0x36160, then realm0x2bb060(World,placeXY,
big), preserving the existing lore-and-artifacts4.3 realm rules.
The header is **0x16c3e0(Game dictionary,0x10cee0(realm),realm)**:
the owner's `realmname` expansion, not a cell/NPC/mission label.
Realm calls are0x28e793/0x28e7bd.

Look up this unsigned realm ID in local player's personal-data lore
map+0x128. A missing key is **created with an empty cell set**.
For ordinary count k, display **10 times k**, without a100% cap.
Native arithmetic takes the count's low32 bits, multiplies by10 with
32-bit wrap and treats the result as signed32 for decimal formatting
and comparison. Color is grey if that result<100, otherwise green.
This lookup/insertion does not insert the hovered cell into the set,
credit lore or issue a progress sound.

### 5.2 Fixed headers and tale compilation

Use the Preview's current font resource (`resource1.dat` from its
constructor). Set **font12, outline3, flags0x10, black outline** and
wrap width **queried effective W−20**. Character/line spacing is not
reset here; §4.1 identifies the ordinary persistent writers.

| Submission | Origin | RGBA |
|---|---|---|
|Realm-name expansion|(14,25)|(1,0.8,0.5,1),0x438110|
|Literal`Lore ` plus signed decimal percentage plus`%`|(14,49)|Below100:(0.75,0.75,0.75,1),0x438150; otherwise(0.6,1,0.25,1),0x4700a0|

The header positions are **fixed**, not subsequent returned text Ys.
Wrapping the realm header or percentage does not move the next row
or body origin. Header text calls are **0x28e7e3/0x28e91c**;
49 is float constant0x470088.

Then select white, construct the cell's tale with
**0x2d4290(World,out,cellX,cellY)** at0x28e9c6, and compile its
temporary script through **0x1694e0** at0x28ea5f. The compiler's
selection seed is existing **R(uint32(cellX)+57 times uint32(cellY))**,
with wrapped32-bit arithmetic; its additional string is empty.
Tale generation, realm/ruler context and dictionary grammar remain
lore-and-artifacts §3 and the conversation/script specifications.
This is not a replacement generic NPC sentence.

Render the **entire compiled page vector** through **0x269590** at
**0x28eab7**, starting at **(14,73)** with available width equal to
a fresh effective-width query minus30.73 is constant0x47008c.
There is no DialogWidget, speaker ID, clock-based reveal or selected
page index in this path. The temporary compiled pages and input tale
are destroyed after the draw; no persistent G+0x418 pages are appended.

### 5.3 Exact static-word layout0x269590

The helper first sets Widget text flags+0x1e4 to0, **line spacing
+0x1bc to2**, and native automatic wrap width+0x1c0 to0. It **retains
outline radius3 at+0x1b4**, requested font12 at+0x1b0 and character
spacing at+0x1b8. The2 is line spacing, not a replacement outline.
It uses manual word placement below instead of plain-string wrapping.

Each word's width is the float32 difference **maximum ink X minus
minimum ink X**, then truncation toward zero to signed32. Measurement
at **0x26973b** uses0x352110 with the same current-GUI-matrix,
scalable-font, **zero-outline ink-box** inputs as0x269050; see
steam-dialogue-panel-routing §10. Here the stored line spacing is2.
The normal GUI raster sizing and normalization remain that shared
contract; this is neither final pen advance nor width plus two outlines.
There is **no apostrophe-specific extra width** in this helper.
A missing font gives zero ink bounds, before the spacing rules.

For each page, reset pen X to starting X and treat its first word as
having no preceding inter-word gap. Process its stored words in order,
including empty words. The following constants are **fixed GUI units**,
not multiplied by font size or overall Widget scale at this layer:

* A one-character word exactly in **`. - , ; ! ? : / )`** is the
  narrow-gap/non-wrapping punctuation group. The checks are exact
  UTF16 string equality and length1, not a Unicode punctuation class.
* Except for the first word or a word immediately following literal
  **`-`**, add a preceding gap: **3** for that punctuation group,
  otherwise **12**. Following literal`-` gets no gap.
* After this gap, a word outside the punctuation group wraps only
  when **penX plus float32(truncated width) is strictly greater than
  startingX plus availableWidth**. On wrap, first add stored line
  spacing2 to Y, then font size12, and reset X to startingX.
  The test also applies to an over-wide first word; no separate
  first-column exemption is present. Punctuation never takes this wrap.
* Copy the word's stored float32 RGBA (word-list node+0x30..+0x3f)
  into the TextWidget fill, then submit its complete stored text at
  current X/Y through **0x269210 at0x269bfe**. Advance X by the
  truncated ink width converted back to float32. Returned text pen
  coordinates are not used for this manual advancement.
* If that word is exactly **`/` or `(`**, additionally add **−9** to
  pen X afterward. `(` is not in the narrow-gap group above. These
  are explicit constants, not a glyph-width-dependent adjustment.

After **each page**, including the last and an empty page, advance Y
by float32 **(stored line spacing plus font size) times1.5**. In this
caller that is21. Resetting X for the next page is separate from this
advance. An empty word remains a normal non-punctuation word with its
usual gap/wrap behavior and zero width when measurement yields zero.
The helper reads no word shake byte, uses no reveal-prefix clock and
does not execute page lore/mark/give/take actions. Styled word colors
come from the compiled dictionary words, not the preceding white fill.

At return, truncate starting X and accumulated Y separately to signed32,
then convert them to float32 for the returned vector. Thus output X
is the original starting X, not the last or maximum line width.
The last word's fill and the helper's line spacing2/flags0/wrap0
remain in the TextWidget; no post-layout state restore occurs.
Punctuation constants:0x4300c8,0x4300d0,0x430ed0,0x430e88,
0x437ddc,0x431820,0x4311b4,0x430ed4,0x437e88; opening-parenthesis
constant0x46d048,−9 at0x42e854. Source:0x269590..0x269dd2.

### 5.4 Final size and artifact distinction

Take that returned, already integer-truncated Y, add float32 **30**,
query current effective width again and call **0x32c9e0(width,height,1)**
at **0x28eae4**. No200-unit minimum, comparison block or item icon is
added by this lore branch. Existing parent-M00 sizing and late
background/children behavior remain preview-item7.4/7.6.

Arithmetic reference: one compiled page whose words all fit one line
starts at73, adds the final page spacing21 and returns94; requested
height is124 before parent sizing/minimum constraints. Each manual
wrap adds14. Zero compiled pages return73 and request103. These are
derived ordinary-font layout examples, not native runtime captures.

A **plot-kind14 lore building** supplies this cell branch. An admitted
known **role14 artifact mission** instead carries its planner item;
with no coordinate branch taking precedence it uses the ordinary
item/3D-icon formatter, including that formatter's realm/effect text.
Its item+0x18 realm does not create a lore-cell preview or a lore award.
The collection-time100%-lore artifact reveal remains the distinct
map-markers1.3.2/lore-and-artifacts5.4 behavior; it is not performed
by this static tale text consumer.

## 6. Bounded completion

Coordinate production/sampling, item payload transfer, branch priority,
ordinary flight text and static lore text/layout above are stable.
Existing world generation, full item-name resolution, dictionary word
production and generic font rasterization remain their independent
contracts. The two additional+0x200/+0x208 detail fallbacks in §2 now
have a [separate consumer/provenance supplement](steam-preview-skill-specialization-fallbacks.md);
their affirmative live producers remain unverified. Numeric examples assume ordinary finite input
and fitting conversion intermediates; no malformed-value formatting
or unrelated gameplay action is inferred from these draw routines.
