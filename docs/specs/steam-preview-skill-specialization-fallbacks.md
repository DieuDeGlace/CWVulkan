# Steam PreviewWidget skill/specialization detail fallbacks

Independent native-only supplement to
[Preview item rendering](steam-voxel-widget-information-preview.md) and
[map/flight/lore precedence](steam-preview-map-location-lore.md).
Sources:0x28c0a0,0xb6b00,0x28c290,0x28d320,0x28db50,0x16c5e0,
0x167c80 and native virtual tables/instructions. No implementation,
tests, exporter/build, SDK, owner assets/captures or game execution.

## 1. Verified field lifecycle; positive producers remain unverified

Preview constructor **0x28c0a0** initializes:

| Field | Initial value | Ordinary selector0xb6b00 |
|---|---|---|
|Signed32+0x200, skill lookup ID|−1, store0x28c175|Writes−1 every reached selector|
|Signed32+0x204, positive-details gate|0, store0x28c17f|Retains it|
|Signed32+0x208, specialization index|−1, store0x28c185|Writes−1 every reached selector|

The selector is0xa9d22, before later GUI rendering. Its normal own
Preview visibility decision is based on its item/map sources, not
on a new skill/spec hover route. The fallback draw routines below
neither set their admission fields nor show their own root. They do
not clear the fields after use either.

**No affirmative live producer is verified for this Preview instance.**
The reviewed constructor, ordinary selector, Interface update, UI
draw/input paths and direct instance-pointer accesses did not identify
a user event assigning a nonnegative skill/spec field. Primary virtual
entries of Preview at0x46fb78 through slot0x150 match TextWidget at
0x46ced8 except destructor and draw:0x28c250/0x28c290. There is no
Preview-specific hover/input virtual override in that table.

The native direct-call index identifies one call to each text helper,
both from0x28c290. These findings establish the consumers and known
negative writers; they do **not** establish an exhaustive proof against
every possible aliased/data-driven write. A normal skill-tree button,
specialization-answer hover or other activation route must therefore
remain **unverified**, rather than being supplied from the labels.
On the verified constructor/ordinary-selector paths, these two gates
remain negative.

## 2. Reached caller arguments and priority

After hiding`star1`..`star4`, Preview0x28c290 tests flight coordinates,
then lore coordinates, then the selected-item pointer, as the map
supplement specifies. Only with both coordinate X sentinels and a
null item does it test these fallbacks:

* **+0x200≥0:** call **0x28d320 at0x28c56c** with the stored signed32
  skill ID+0x200, stored signed32 gate+0x204, integer **X14,Y25**,
  scalar **1.0**, integer available width **275**.
* Otherwise, **+0x208≥0:** call **0x28db50 at0x28c5bf** with the
  **current local player's class byte P+0x14c**, integer **X14,Y25**,
  scalar **1.0**, integer available width **275**. The helper reads
  the specialization index from Preview+0x208 itself.

The scalar argument is unread by both text helpers; it does not scale
their fonts, gaps, wrap increments or width. GUI scale remains the
ordinary external Widget/Plasma transform.275 is a fixed caller input,
not a query of current frame width minus padding.

If the first reached helper produces no pages, its caller does not
retry the specialization branch. Neither branch draws an ordinary
item icon, a comparison heading or a new class/skill model. Root
visibility and after-background/children callback timing retain the
ordinary Preview contracts.

## 3. Skill-ID lookup and ordered owner dictionary requests

**0x28d320** obtains a string key through
**0x16c5e0(Game dictionary,out,skillID)**. The dictionary has a
signed32-ID-to-UTF16-string map at **dictionary+0x18**; its tree node
key is+0x20 and value string+0x28. Lookup is signed, and missing ID
returns an empty string without creating a map entry. These mapped
strings are required loaded dictionary data, not a new hard-coded
display-name table or a CUB model catalog.

Create one initially empty local expansion context and one empty
temporary compiled page vector. Submit the following literal dictionary
phrase IDs to **0x167c80**, in order, using that same context/vector:

1. **`description:` concatenated with the mapped string key.**
2. Only when the supplied+0x204 argument is strictly positive:
   **`skill:level`**.
3. Under that same positive gate:
   **`details:` concatenated with the mapped string key.**

Every request uses selection seed **0** and an empty additional string.
This caller adds no numeric level, player, item or speaker substitution
to the local context. In particular, the numeric gate's magnitude is
not formatted or inserted here:1 and100 both admit the same requests.
The context is initially empty and reused, not reconstructed between
the three requests; normal dictionary expansion semantics still apply.

Requests append through normal0x167c80 behavior, not an unconditional
new paragraph per request. Its existing last-page **more-than10 stored
words** rule and owner script page breaks determine the actual compiled
pages; see steam-dialogue-panel-routing §11's phrase-append contract.
A missing mapped key still makes literal`description:` and, if admitted,
`details:` requests. It does not select an invented generic skill name.
Sources:0x16c5e0,0x28d320,0x167c80.

## 4. Specialization phrase selection

**0x28db50** creates an initially empty local expansion context and
temporary compiled page vector, then chooses one literal phrase ID:

| Current class byte | Preview+0x208 | Phrase ID |
|---|---|---|
|1 Warrior|0|`description:SpecializationBerserker`|
|1 Warrior|1|`description:SpecializationGuardian`|
|2 Ranger|0|`description:SpecializationSniper`|
|2 Ranger|1|`description:SpecializationScout`|
|3 Mage|0|`description:SpecializationFire`|
|3 Mage|1|`description:SpecializationWater`|
|4 Rogue|0|`description:SpecializationAssassin`|
|4 Rogue|1|`description:SpecializationNinja`|

Other class/index combinations issue no description request. The
chosen phrase is expanded through0x167c80 with seed0 and an empty
additional string. English description bodies, word colors and any
explicit page breaks come from the owner's loaded dictionary. There
is no native literal replacement paragraph, stat-row calculation or
automatic selection of a specialization in this draw.

## 5. Shared fallback word layout and final size

The two helpers implement the same layout, distinct from static
lore's0x269590. Use the Preview's current font resource, initialized
to`resource1.dat` by its constructor. They do not reload another
font. If the compiled page vector is nonempty, select **outline3,
black outline RGBA(0,0,0,1), text flags0**. Character spacing,
stored line spacing and stored wrap width are retained. Native
automatic word wrap is disabled by flags0; the following integer
pen algorithm performs wrapping against the supplied width275.

First page font is **12**, every subsequent page font is **10**,
including after an empty first page. For each page start X is14;
initial Y is25 and subsequent pages continue the accumulated Y.
At the start of each page its first word has no preceding gap.
Stored words, including empty words, retain their order.

Measure a word through **0x269050**. Subtract min ink X from max
ink X in float32, then truncate that difference toward zero to
signed32. The current GUI matrix/raster-scaled, normalized, zero-outline
ink measurement is steam-dialogue-panel-routing §10. This is not pen
advance, and there is no apostrophe-specific extra width here.

* Exact one-character **`. - , ; ! ? : / )`** is the narrow-gap,
  non-wrapping punctuation group. Other strings are outside it.
* Except for page-first words and words immediately after literal`-`,
  add integer gap **3** for that group, otherwise **12**. Following
  literal`-` receives no gap.
* After adding the gap, a non-punctuation word wraps when integer
  **penX plus truncated word width > startX plus275**, strictly.
  The over-wide first word is not exempt. On wrap, convert current
  Y to float32, add it to the float32 sum **current page font size
  plus4**, truncate to signed32 Y, then reset X to14. Thus ordinary
  first-page wraps add16; subsequent-page wraps add14. Punctuation
  does not take this wrap.
* Copy the word's stored RGBA at list-node+0x30..+0x3f to text fill.
  Submit its complete text at the integer pen converted to float32
  through0x269210. Advance integer X by the truncated ink width;
  returned text pen coordinates are ignored.
* Exactly **`/` or `(`** subtracts integer9 from X afterward.
  `(` is outside the narrow-gap group. At the next word only literal
  `-` suppresses the normal preceding gap.

After **every page**, including the last or an empty page, reset X
for the next page and add integer **18** to Y. These page advances
are not lore's21 or a line-height/size multiplier. No reveal clock,
word shake, page-effect execution or click-to-advance state is used.
The temporary expansion/pages are freed after drawing.

Finally query current effective frame width and call
**0x32c9e0(width,float32(final integer Y),1)**. There is **no additional
bottom30,200-unit minimum or comparison resize**. With no compiled
pages the style block is skipped, but this resize still requests
initial height25. One unwrapped page requests43; two request61.
Each first-page wrap adds16 and each later-page wrap adds14 in the
ordinary range. These are derived layout references, not game captures.

The last actually selected page font, last word fill and flags0 remain
in the Widget. Neither helper resets line spacing or stored wrap
width. The ordinary parent-M00 size conversion and earlier background
paint remain the existing Preview sizing/callback contract.
Sources:0x28d320..0x28db42,0x28db50..0x28e63b; constants12 at0x42e59c,
10 at0x42e598,4 at0x42e584,3 at0x42e580; UTF16 equality0x89470.

## 6. Implementation/evidence boundary

The two callee contracts, exact callers, field initializers/resets and
dictionary request/layout rules above are stable. **No native user-input
producer or affirmative field write has been established.** This is a
consumer specification, not authorization to add a skill/spec hover
route or to populate these fields from a guessed UI element.

The missing evidence is an affirmative writer RVA for the live
G+0x3c0 instance's+0x200/+0x204/+0x208, with its event/admission and
own visibility handling, or a bounded native write trace identifying
that producer. No such runtime trace was run for this audit. Ordinary
finite/fitting numeric layout inputs are covered; malformed arithmetic
and untraced aliases remain outside the claim.
