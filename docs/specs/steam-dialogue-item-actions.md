# Steam general dialogue, lore, item feedback and recipe sources

Independent clean-room behavioral specification from the native decompile,
assembly, call references and constants. No engine, tests, exporter, generated
build files or owner captures were inspected or edited; no game was executed.
All addresses are RVAs. Item records are0xa0 bytes and dialogue pages0x1b0.
This refines the older summaries in remaining-runtime-gaps §§3/5 and
lore-and-artifacts §4. It does not invent string grant commands or new awards.

## 1. Destination, append policy and speaker identity

### 1.1 Examining lore

The accepted lore-object Examine path is **0x98180 at0x9a4af**. It obtains
the existing tale script through **0x2d4290**, then compiles it into the
**existing general DialogWidget G+0x418 page vector+0x1e8**, with empty
speaker-name context. The seed is **0x2ff5c0(loreCellX,loreCellY)**. It does
not attach those pages through0xa3170 and does not create an ordinary NPC
speech bubble. The16-block lore-cell search and script construction remain
in lore-and-artifacts §§2.3/3.

After compilation, when the destination vector is nonempty, **0x9a4f0**
appends the examined16-block cell pair to the **last destination page's
+0x190 lore list**. This is after all script/dictionary page formation,
including dictionary `%` breaks. It is not the1024-block mission mark
list+0x1a0, and it is not immediate lore credit at the Use event.

Compiler **0x1694e0** does not clear the destination or reset the widget's
clock/page index. The caller here also does not do so. Ordinary dictionary
expansion may append to an existing last page or split according to its
existing word-count/page-break rules. The last-page attachment means the
last page of the resulting destination, not an assumed first new page.
Normal Use0x98180 already refuses its action route while the general
widget root is visible; that admission gate is not a compiler replace
policy. Preserve the compiler behavior even for pending/hidden contents.

Neither this caller nor the compiler assigns the general widget's full
**speaker ID+0x210**. The empty speaker-name argument and dictionary seed
are not an actor binding. Existing speaker ID is retained; its normal
construction/empty-update value is0. Empty general-widget update clears
it to0 at **0xb5827**. See steam-dialogue-panel-routing §§10–11 for the
same append/speaker distinction in specialization confirmation.

The seed helper **0x2ff5c0** has exactly the two integer coordinates as
inputs. It combines their low32-bit words as **x+57 times y modulo2^32**,
then applies the existing world-generation **R/random** transform:
100000 minus the masked native hash divided **unsigned by21474**. It
does not add a current world seed, player region or actor ID. Thus the
existing R of the wrapped coordinate combination is the exact mapping,
including negative input coordinates.

### 1.2 Receipt feedback is distinct from receipt chat

**0x9d6f0** is the received-item **chat formatter**, writing G+0x398. It
does not itself compile a general dialogue or an NPC speech bubble.
It omits its normal received-item line for category12. Other callers
may independently compile a receipt script; do not create a modal item
dialogue for every call to the chat formatter.

The receipt script contexts use `define_item` plus five description
operands serialized through **0x839c0**: **category byte, subtype i32,
modifier32, material byte, rarity byte**, in that order as decimal token
strings. They do not serialize region, attribute entries or a second
inventory grant. String item reader **0x15ca60** consumes these five
operands into an otherwise zero-initialized context item. Defining the
item changes dictionary substitution/color context, not page give fields.

The following verified receipt-script callers all target **G+0x418's
existing pages**, supply an **empty speaker-name context**, use one CRT
**rand()%10000** dictionary seed, and do not assign speaker ID+0x210 or
reset the general widget's current index/clock.

### 1.3 Locally opening a reward chest

Object type **0x16** in **0x98180** obtains its chest reward from the
loaded zone's **object-index-to-item map+0x450**, after the existing
state/used-cell tests. It copies that item; category30 is class-adapted
through **0x10a720** before feedback. At **0x9a192** it compiles
`define_item`, the five operands, and a receipt phrase into G+0x418.
Use **`you_received_the_item`** for category23/24, or gear categories3–9
or26 with rarity **greater than2**; otherwise use
**`you_received_an_item`**.

This branch adds its reward independently of script expansion: when
**0x109070(item)** returns0, add one through0x4e810 and format its receipt
chat; otherwise add that returned amount to player gold. The script is
feedback, not an opcode8 grant. The existing used-cell/chest rules remain
in zone-object-interaction §2 and lore-and-artifacts §2.

If the player is idle, or its current ability timer is **strictly greater
than0x4f220's duration**, this branch starts ability **0x7c**, resets the
ability timer, copies the **full adapted reward item** into item-in-use
**player+0x1d8**, and sets **G+0x2050=60000 ms**. That embedded item is
an actor presentation input; it is not the five-field dictionary context
and is not a second copy added to inventory.

### 1.4 Consuming local pick-up notices

Frame **0xa5a50**, in its **World+0x110 pick-up-notice** pass ending near
**0xaac9a**, resolves the recipient and adapts the notice item through
0x10a720. The general-script branch is only for the **local recipient**,
after the existing pick-up deduplication/inventory-or-currency handling.
Its eligibility is **original notice category30**, or adapted category
**23,24,29**. Ordinary gear does not enter this branch merely because it
has high rarity. Notice layout/deduplication remain in
zone-object-interaction §6; they must precede this visual feedback.

Build `define_item` plus the five adapted-item operands. Select the
definite receipt phrase for category23/24, or adapted gear3–9/26 whose
rarity is greater than2; otherwise select the indefinite phrase above.
For adapted **category24**, additionally append `new_page`, then
`define_land` with the item region's kind from0x2b9080/0x9afa0 and the
region's seed from **0x2ff5c0(item.regionX,item.regionY)**, then the
following subtype-specific phrase when one exists:

| Subtype | Dictionary phrase |
|---|---|
| 1 | description_hang_glider |
| 2 | description_boat |
| 3 | description_reins |
| 5 | description_harp |
| 7 | description_whistle |
| 8 | description_bell |
| 9 | description_treasure_spirit |
| 14,15,16,17 | description_crafting_book1,2,3,4 respectively |
| 18 | description_climbing_spikes |

Other subtypes still execute those context/page-break tokens but append
no subtype phrase in this switch. The `new_page` remains deferred under
the existing compiler rules; it does not manufacture an empty page just
because no phrase follows it.

At **0xaac9a** compile this script into G+0x418 as described above.
When the actor may change ability (idle or strictly elapsed duration),
category24 subtype1 starts **0x78**; subtype2 starts **0x79 only when
the player's previous physics flags+0x5c include water bit0x02**. Other
category24 subtypes and non-category24 receipt items start **0x7c**.
The admitted ability change copies the **full adapted item** to+0x1d8
and zeros its timer. A dry subtype2 does not start0x79 or substitute0x7c.
Within this idle/finished-ability branch the feedback timer becomes60000.
The normal received-item chat occurs separately after the notice's
recipient-specific work.

### 1.5 Feedback generated while consuming a page reward

For a **successful page give of exactly one category24 item**, the
speaker-page consumer **0xb5283** compiles a **numeric** stream into
G+0x418: opcode5, that item's five description operands through0x838a0,
then speech ID **0x85**, mapped by0x165640 to `you_received_the_item`.
This stream contains **no numeric give/take opcode**. It is feedback
after the inventory addition already happened.

The general-page counterpart **0xb5a44** instead uses the **string**
stream `define_item`, five operands, `you_received_the_item`, and appends
it to that same general widget's existing pages. Neither counterpart
forces a blanket new page/reset beyond the compiler's own rules. Both
use empty speaker-name context, one rand()%10000 seed, and retain speaker
ID. Both may start0x7c/reset its timer/copy the full awarded item to+0x1d8
when the local player is idle or its ability has strictly finished.
Their page-reward path already sets G+0x2050=60000 on successful giving.

### 1.6 Actual direct grant and object-embedded offering distinction

There is a real producer of page give fields outside the numeric
compiler: accepted **object type0 Examine**, **0x98180 at0x9a678**,
expands **`gift_of_the_goddess`** directly into G+0x418's existing pages,
with empty context and seed0. It then sets the final page's **give count
+0xd4 to1**, give-item **category+0xd8 to24**, and give-item **subtype
+0xdc to4**. Other fields retain that page's item initialization/state.
It does not issue numeric opcode8. The existing subtype4 availability
test **0x5e890(player,4)** causes this object's route to return before
offering the action in the supported build; that helper unconditionally
returns true for subtype4. Thus the direct page-field writer exists,
but its ordinary object0 admission is suppressed by this earlier gate.
Do not present a reachable new goddess grant solely from finding the
later stores.

An object's **embedded item at object+0x38**, when nonempty and its state
byte+0x28 is nonzero, is another, different route before the object-type
switch. It checks the player's required quantity, shows the offering
prompt, locally removes the quantity through0x648b0 except for object
type9, and queues ordinary interaction kind5 carrying that item. This
is an **item the object takes**, not a receipt script or an ordinary
NPC bubble. The quantity byte is object+0xd8. These semantics remain
in zone-object-interaction §2; do not reinterpret the embedded required
item as a general-widget inventory reward.

## 2. General-widget activation, effects and order

Sources are **0xb40d0 at0xb57d0–0xb61b0**, with the detailed action
branch beginning near **0xb58ac**. General-widget activation requires a
nonempty page vector, valid signed32 page index+0x204 and stored float32
clock+0x200 **exactly zero**. Root visibility and mature text display are
not additional effect gates. It reads the current page at that time;
it does not execute every compiled page at conversation creation.

The effects run in this order:

1. If signed32 **give count page+0xd4 is nonzero**, test the page+0xd8
   item through **0x50670(local player,item)**. On success, add the count
   with0x4e810, format receipt chat0x9d6f0 and set G+0x2050 to60000. Then
   perform category24/exactly-one receipt feedback from §1.5 if applicable.
2. Still **inside the nonzero-give-count branch**, if signed32 **take
   count+0x30 is nonzero**, remove that count of page item+0x34 through
   **0xa2210 at0xb5adb**. This removal runs even when step1's receive
   permission failed. A general page with give count0 does **not** remove
   its requested item through this route, even if take count is nonzero.
3. Visit **lore cells+0x190 in stored list order**. Insert each pair into
   local personal used cells+0xa8. For a lore-building cell, obtain its
   realm and insert it once into personal lore-by-realm+0x128. A new entry
   adds the realm-name and ten-per-recorded-cell percentage chat feedback
   described in lore-and-artifacts §4.1. Duplicates do not increase lore.
4. Visit **mission marks+0x1a0 in stored list order**. For each1024-block
   place-cell pair, require0x5f4c0's known/solved test to be false. Insert
   into personal known+0xb8 and map marks+0x38, set highlight+0xe8[pair]
   to **180000.0 ms**, then play sound **0x20** at G+0x1dc0, volume1.

All personal state/inventory addresses are the **local player**, including
for a guest. These page actions do not broadcast a reward to party members,
write a quest-award packet or set the solved/finished mission sets.
This general route does not iterate the numeric descriptor list+0x178
to originate feat awards.

**Two precise refinements of the old summaries:** speaker bubbles remove
their take item **before** the give branch, with no give-count prerequisite;
the general widget has the different ordering/gate above. Also the
**general widget's lore-credit loop does not separately play0x20** after
its lore chat. The speaker-bubble lore-credit branch does, at0xb55e7.
General-widget sound0x20 at **0xb5efb** belongs to a newly accepted
**mission mark**, not to each lore-cell credit. A page carrying both may
still produce that mission-mark sound. Do not impose a common unconditional
lore sound on both consumers.

After activation effects, the general widget adds elapsed time in the
precise double-add/float-store order from steam-npc-speech-rendering §8.1,
shows its root, and handles the existing mature **release** advancement.
Threshold is strictly **clock>D+100 ms**; prior Primary/Use states are
forced zero until mature. Advancing clears the clock; the next page's
effects run on the next update. Exhausting the final page clears/hides
the widget in the same update. An empty-vector update also resets its
speaker ID to0. Drawing/visibility/input routing remain specified in
steam-dialogue-panel-routing §§5–11.

There is **no independent once-only page-effects bit**. Clock exactly
zero is the actual guard. A zero elapsed update can revisit that guard;
list/set idempotence and receive permission may suppress some repeated
consequences, but the compiler/consumer does not mark the page permanently
consumed when its effects first execute. Appending pages does not itself
reset a nonzero current clock or cause earlier pages to execute again.

The normal frame calls player control **0x8ae50** before dialogue update;
its Use call is **0x98180 at0x8df69**. A newly compiled Use/Examine
general page can therefore activate during the same frame's later
**0xb40d0 at0xaa1bc**, if that page is current and its clock is zero.
Speaker-page feedback in §1.5 is also compiled before the general-widget
part of the same0xb40d0 call. By contrast, the pick-up-notice receipt
compile at0xaac9a is **after** that frame's dialogue update, so it waits
for a subsequent update. The general consumer activates only its one
current page per call; compiling more feedback into its vector does not
recursively activate or drain all newly appended pages in that call.

## 3. Numeric item-action producer boundary

The accepted numeric decoder **0x168120** retains the semantics in
remaining-runtime-gaps §5.4: opcode8 installs a five-field described item
as page give count1; opcode9 installs category12/material11 money with
its operand count; opcode10 installs the described take item and its
count. The decoder attaches to the existing last page, creating one
only if the destination is empty. These accepted-input semantics do not
establish a real native stream carrying them.

The raw native assembly contains **one identified direct call to this
decoder**, **0xb5283**, the feedback stream in §1.5. It supplies only
opcode5 and phrase0x85, not8,9,10. The separate general reward feedback
uses string compiler0x1694e0. No verified native producer of numeric
8/9/10 was found. The earlier pointer audit likewise found no stored
64-bit function pointer to0x168120. That is bounded static evidence,
not proof against every conceivable computed indirect target.

Accordingly no new numeric grant/take stream, generic string `give_item`
or `take_item` command, quest-award event, or learned-recipe action is
authorized by this decoder's existence. Verified native callers should
produce their actual receipt/display scripts and direct records above.
Actual award production for numeric8/9/10 remains unverified.

## 4. Complete bounded recipe sources and dormant tab6

### 4.1 Stored known recipes

The persistent known-recipe list is **personal data+0x18**, with64-bit
count+0x20 and0xa0-byte item payloads. Known test **0x5f440** compares
entries through full item equality0x4df60, rather than only a name or
category/subtype pair. It returns false when personal data is absent.

The verified creation/retention sources for that list are:

- **Defaults0x9afb0**, called with argument0 from normal game startup
  **0x7ea80 at0x7f72d** and the class/appearance kit rebuild
  **0x913b0 at0x91c06**. The latter first empties the previous recipe
  list; the startup call adds missing defaults to the selected player's
  existing state. The default helper itself tests each proposed item
  with0x5f440 and appends only when absent.
- **Character deserialization0x147800 at0x147fec**, restoring file field17,
  the saved recipe item count and its item records. These are real stored
  items, not regeneration from carried books. Existing serialized entries
  must be retained, including ones the current crafting UI does not list.
- **Post-load repair0x9bb20 at0x9bd5e**, after a successful blob load:
  ensure **category1/subtype7, all other fields zero**, appending at the
  tail only if the full item is not already known.
- **Character-state copy/assignment**, through0x7df80/0x74c40 and0x7cec0,
  preserving an existing recipe list, rather than awarding a new recipe.

The node constructor for these0xa0-byte item lists is **0x6e600**. Its
other audited wrapper **0xa17d0** is also used for a merchant's bought-item
history at personal+0x138, including0x94900 at0x96154; that use is **not**
a write to the recipe list+0x18. Similarly the later0x147800 history-list
read through0xa17d0 restores+0x138, not additional learned recipes.

### 4.2 Exact default proposal sequence

Default helper0x9afb0 starts with a zero item. Its argument supplies the
rarity byte; the other proposed fields are those listed below, with
modifier/region/attributes remaining zero. In order, always propose
category1 subtypes **1,2,4,10,8,11,7**.

Only for argument0, continue in this order:

1. Category28 subtypes **0,1,2,3,4,5,6**, material0.
2. Category11 subtype26, then subtype22, material0.
3. Category11 subtype9 with material28 for class4, material27 for class2,
   or material26 for class3. The class3 proposal is literally repeated;
   its second known test prevents a duplicate. Class1 has no such proposal.
4. Category11 subtype10 with materials **1,2,11,12**, in that order.

Each proposal is independently guarded by full equality. The helper draws
no RNG. The two identified direct callers both supply0; a nonzero-argument
decoder/helper capability is not evidence that a crafting book calls it
with rarity1–4. This sequence refines frontend-characters §5.6 without
turning regional generated equipment into permanently stored recipes.

### 4.3 Regional equipment and crafting-book ranks

Recipe builder **0xb2180** first takes the known-list items in stored
order and classifies them into its six source lists. It then iterates
regional **rank0,1,2,3,4**, using the **local player's current region
+0x150/+0x154** and class+0x14c. Eligibility is
**0x2b4260 →0x128400** on that region's planner:

- A missing local player returns false, even for rank0.
- Rank0 is otherwise true.
- For rank k≥1, require an entry in the planner role map whose role is
  **k+1**, and whose **1024-block place-cell pair** occurs in personal
  **solved set+0xc8**. This check does not substitute known+0xb8 or
  finished+0xd8, or inspect the inventory/used-item animation.

Each admitted rank reseeds CRT RNG to
**R(regionX+57×regionY+0xf5f9)+rank** and contributes the deterministic
class weapons, four armor pieces and two jewelry pieces detailed in
loot-crafting §2.2. Their items have that rank as rarity and that current
region; these recipes are **generated menu contents**, not inserted into
personal known recipes. The seed/decision/draw sequence, ingredient
records and station construction in loot-crafting §§1–2 remain the full
contract. Rebuilding starts from the same regional seed inputs, not an
incremental random recipe-learning event.

Role treasure producer **0x131540** makes crafting books for roles2–5:
**category24**, subtypes **14,15,16,17** respectively, with the planner's
region and other item fields zero. Its receipt description comes from
§1.4. These book/role correspondences explain the rank association, but
the actual crafting eligibility test reads the role's **solved-place
state**, not a book-use action that appends items to+0x18.

Thus the old question “how does reading the crafting book add known
recipes?” must not be resolved by inventing such an insertion. No such
recipe-list writer was identified. The exact lifecycle producing or
retaining a book place in solved+0xc8 is a **separate input/state question**;
§4.3 specifies the complete predicate used by this recipe source, without
claiming that item receipt alone writes that set. If that mission-state
producer is absent from an implementation, its native producer still
needs a separate trace rather than a substitute learned-recipe grant.

### 4.4 Tab construction and the explicit empty seventh tab

When the local inventory has at least one tab,0xb2180 first buckets the
known recipe items: category3→tab0; category1 subtypes1,2,3,12→tab4 and
other category1→tab3; category28 subtypes4,5,6→tab3 and other category28
→tab4; category11→tab5. Other known-list categories, including4–9, are
not listed through this source. Regional equipment fills tabs0–2 through
§4.3. Within a bucket the known-list contributions precede rank-generated
contributions; no arbitrary known-list sort or permanent insertion is
added by this builder.

It calls **0xb2ee0** for tabs **0,1,2,3,4,5**, in that order, to calculate
ingredient-limited makeable counts and fill those menu lists. Then at
**0xb2a80–0xb2a8e** it ensures at least seven tab containers and assigns
tab6's end pointer equal to its begin: **tab6 is explicitly emptied**.
It clears both target maps **G+0x1938 and+0x1948**, including counts,
at **0xb2a95–0xb2b34**. The subsequent inventory traversal in this
function adds no recipes or target-map entries; assembly confirms no
append/insertion in that traversal before refreshing the craft widget.

The read-side in-place crafting path0x8fbc0 accepts tab6 and those maps,
but it does not originate their contents. The audited references outside
construction are these reads and0xb2180's clearing; no supported producer
populating a seventh recipe family was identified. Therefore preserving
the real six populated families and empty seventh tab is supported;
inventing upgrade recipes merely because the dormant read-side exists
is not. A zero-tab inventory causes the normal rebuild body to be skipped,
as already specified in loot-crafting §2.

## 5. Stable scope and remaining evidence needs

The destination, append/identity policy, general activation ordering,
receipt producers and bounded recipe source contract above are stable.
No verified numeric8/9/10 producer or book-to-known-list insertion was
found. These are explicit source boundaries, not permission to synthesize
missing awards or grants.

If the solved-state producer for book places needs a follow-up oracle,
label **before/after native mission update and before recipe rebuild**;
record the current player region/class, planner role entries/place pairs,
personal solved+0xc8 and finished+0xd8, original/adapted book item and
its region, actual receipt source, and0x128400's rank0–4 results. Also
record known-list items and resulting six recipe lists in order. That
distinguishes a change of persistent mission state from merely showing
a book description or generating a menu. No original-game capture was
requested or executed by this author.
