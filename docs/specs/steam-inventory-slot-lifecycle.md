# Steam canonical inventory slots and identification indices

Independent native-only behavioral supplement. Sources:0x147800,
0x913b0,0x4e810,0x5f100,0x5f0e0,0x648b0,0xa2210,0x965e0,
0x94900,0xb3ed0 and their helpers. No engine, tests, exporter, build or
owner data inspected; no game execution. This concerns actual creature
inventory storage, separate from derived crafting/shop collections.

## 1. Canonical structure and loading

Creature **+0x9f0/+0x9f8/+0xa00** is the begin/end/capacity triple for
an ordered vector of tabs. Each tab is a24-byte begin/end/capacity triple
for **164-byte slots**: signed32 quantity+0, item160-byte payload+4.
The item type byte is slot+4, subtype unsigned32 is slot+8, seed slot+12,
region signed32 pair slot+16/+20. A zero quantity is an empty slot even
when other bytes retain prior item data. Nonzero, including−1, is the
occupied test in insertion/refresh; some removal paths separately require
strictly positive quantities. Do not infer occupied solely from item type.

Character reader **0x147800** first replaces the inventory with six empty
tabs. At saved field14 it reads the saved signed32 tab count, resizes the
actual tab vector to that count through0x64f30, then for each tab reads
its signed32 slot count, resizes through0xa2540 and copies **count×164
bytes directly** from the well-formed blob into that tab's storage.
This preserves **stored tab membership, order, empty slots and quantities**.
The reader does not feed each item through insertion0x4e810, recategorize
it from its current type, collapse duplicates or fill holes at this step.
Six is the initial reset/default, not a reason to discard other well-formed
saved tab counts. Truncated/malformed-count policy remains native-saves.

Class/appearance kit rebuild **0x913b0**, unlike loading an existing blob,
empties the previous inventory, resizes to six tabs, and initially makes
tabs0/1/2 length10 each, tabs3/4 length30 each, tab5 length0. New slots
are quantity0/default item through0x7c7f0; allocator capacity is not another
visible slot. It then adds its actual starter items through the ordinary
insertion helper. These preallocated lengths are not permanent minimums:
refresh compacts them as §4. Exact starter content remains the existing
class/kit specs; this is no extra item grant.

## 2. Insertion0x4e810

Inputs are the inventory address, item, signed32 quantity, and optional
explicit tab. Quantity0 or type0 does nothing. Tab−1 derives the default
from **0x5f100**, using the type byte only:

| Canonical tab | Types |
|---|---|
|0|3..9,15,25,26|
|1|22,24,29|
|2|Every other type not assigned by this table|
|3|2,11|
|4|20,21|
|5|23|

The helper first grows the tab vector if necessary to include the chosen
tab, retaining earlier tabs. It then handles type12/material11 by adding
quantity to **inventory+0xbc =creature+0xaac gold**, without creating a
slot. Type14 instead adds quantity to **inventory+0xc0 =creature+0xab0**,
also without a slot. This does not assign an unverified English name to
the second counter. Those shortcuts occur after the tab-growth decision.

For ordinary items, scan the selected tab in increasing slot order.
Remember the **first quantity0 slot**, but continue looking for an equal
stackable item. The exact stackable type set from **0x5f0e0** is
**{1,10,11,12,13,22,28}**; other types do not merge even when their items
compare equal. Equality is the complete semantic item comparison0x4df60
already specified in steam-dialogue-item-actions §4.1.1, not category/subtype
alone and not a160-byte padding memcmp.

An equal stackable item wins immediately, **before remembered-empty reuse**.
The comparison has no existing-quantity>0 prerequisite. Requested quantity−1
sets that stack quantity to−1; other nonzero requests add signed32 quantity.
It does not clear/rewrite that stack's item. There is no max-stack split
or automatic sort in this helper.

If no equal stackable item was found, overwrite the first remembered
quantity0 slot's item and set its quantity to the request. If there was
no empty slot, **append one slot at the tab's end** through0x61020. That
helper grows capacity if needed while retaining all existing slot indices;
the new payload is a semantic item copy. It does not insert at a category-
sorted position, compact other tabs or shift earlier slots. Equality for
nonstackable gear never turns it into a merged stack.

Example: an earlier empty slot2 and a later equal stackable slot5 cause
the quantity to merge into5, leaving2 empty. Two identical unidentified
type15 items are nonstackable and occupy separate slots, with first-empty
reuse before append. Existing saved copies stay distinct until an actual
native operation changes them; loading does not reinvoke these rules.

## 3. Removal and emptied-item contents

### 3.1 General page take0xa2210

This helper accepts the inventory address, full item and nonzero quantity.
Type0 does nothing. Type12/material11 subtracts from gold and clamps that
counter upward to0 if negative; no tab slot is removed for coins.
Otherwise it walks **tabs then slots in increasing canonical order**,
testing complete item equality and **quantity>0**. A sufficient partial
stack is decremented and returns. If requested quantity consumes a whole
stack, it sets quantity0 and copies a **default empty item**, subtracts
that consumed amount from the request, then continues until satisfied
or no matching stack remains. This does not erase the vector element,
shift subsequent indices, consolidate the surviving stacks or sort.
Its counter shortcut is coins, not an inferred removal shortcut for the
separate type14 insertion counter.

### 3.2 Creature removal0x648b0

The creature-wide removal helper also traverses all tabs/slots in order,
using complete item equality. It subtracts the lesser of request and
current stack quantity, reducing the outstanding request accordingly;
when the stack becomes exactly0 it copies a default empty item. It has
no explicit positive-quantity guard and does not stop the traversal at
request0. For ordinary positive quantities this consumes matching stacks
in order without shifting indices. It supplies no special infinite-stack
protection. Do not replace it with a vector erase or an item-type search.

### 3.3 Direct UI consumption differs from default clearing

The identification commit in **0x965e0**, after copying the slotted type15
item, decrements its live quantity. At a result below1 it writes **quantity0,
item type0, subtype0 only**; other item fields can remain stale. The same
three-field emptying pattern occurs in **0x94900** for its drop/equipment/
sell consumption paths. These slots remain in the vectors until refresh.
Consequently a model that treats any nonzero old item payload as an occupied
slot, or erases a slot immediately at quantity0, changes native behavior.

For identification specifically, the new result is then inserted through
§2, followed by ordinary inventory refresh0x280a40. The insertion can reuse
an earlier hole instead of the just-consumed slot; the subsequent sort can
move the result again. Native does **not** promise the result remains at
the selected unidentified item's former index.

### 3.4 Removal call-site distinctions for the canonical bridge

These verified consumers do **not** share one general remove operation:

- **One upgrade palette cube,0x292650:** first call weapon attribute
  placer0x108b90 at0x292743, then walk every canonical tab/slot in order.
  The first slot with quantity!=0 and full item equality to the selected
  palette payload (0x4df60 at0x2927a5) loses1; below1, clear only
  quantity/category/subtype and return. This is an inline decrement,
  not0x648b0 or0xa2210. Negative quantities have no protection. The
  palette index/count and weapon-count<32 gate precede it; the palette
  entry's own quantity is not retested. No rollback of placement occurs
  if no canonical match remains. Consumption is not conditional on the
  attribute placer appending: it can replace an existing material, or
  reject a new coordinate at the one-handed16-cube limit while the
  widget's looser<32 gate admitted the call.0x292650 itself calls no
  inventory sort/refresh. Full placement context is loot-crafting §4.
- **Use-key giving to a static object,0x98180:** call **0x648b0 at
  0x99bbb**, with the object's full required item O+0x38 and its
  **unsigned byte count O+0xd8**, zero-extended to signed32. The call
  occurs even for count0, unless the **object's type is9**, which skips
  removal. This is not ring item category9. The earlier full-equality
  quantity-sum admission0x5e780 at0x99a69 applies to type9 as well.
  Count0 can pass with an empty aggregate and no matching slot. The
  removal helper has no request0 early return: matching quantity0 slots
  can still have their stale item payload replaced by the semantic
  default. Its no-positive-quantity guard/continued traversal remain
  §3.2, including effects on unusual negative stacks. After optional
  removal, request sound0x3e at0x99bc8, then append kind5 at0x99c1a with
  target zone/index and a copy of the **object's** item. No inventory
  sort/refresh is called. The host object action does not remove it
  again. See use-prompt §3.7.1 and zone-object-interaction §2.6.
- **Daily category22 purge,0x2c34c0:** for non-null local creature
  World+0x448, scan **all canonical tabs/slots**, including saved extra
  tabs. For each category22 item, regardless of quantity, replace all
  meaningful item fields/attributes/count with the semantic default,
  then write quantity0. Type test0x2c3560; quantity clears last at
  0x2c3613. This is inline clearing, not either removal helper. It does
  not visit retained cursor/equipment, erase slots, resize tabs or
  sort/refresh them. Surrounding world-reset timing is
  remaining-runtime-gaps §6; later independent refreshes can compact
  these holes.

## 4. Refresh sorts and compacts canonical storage

**0xb3ed0** receives Game, fetches the local creature's **actual+0x9f0
inventory** and sorts its tab vectors **in place**. It is not an alternate
display-order list. Source calls are frame0xa5a50 at **0xaa7cd** after
pickup application, next-region0x27f6a0 at **0x27f74b**, and widget refresh
**0x280a40 at0x280a6d**. That widget refresh invokes it whenever its bound
inventory pointer+0x1e8 is non-null, before computing current row limits;
even refreshing a derived shop/crafting widget can therefore sort the
player's canonical inventory. The routine does not accept the widget's
bound collection as the collection to sort.

Order within nonempty slots:

| Tab | Precedence, in order |
|---|---|
|0, helper0x7b880 /comparator0x85770|Region equal to ordinary InventoryWidget G+0x330's selected pair+0x1f0/+0x1f4 first; then usable-by-local-player first through0x50640; then **descending float item power0x109c50(item,localPlayer,0)**|
|1, helper0x7b700 /comparator0x83d90|Same selected-region equality first; then unsigned type byte ascending; then unsigned32 subtype ascending|
|2 and every later tab, helper0x7b580|Unsigned type byte ascending, then unsigned32 subtype ascending|

The usability/power functions are stat-rules §§0.5/1.4; power uses its
actual local-player/reference-region inputs, not an invented selected-UI-
region override. No quantity, rarity, seed, full equality or name tie key
is appended to these comparisons. Sorting does not consume CRT random
draws through these pure seed/power queries.

All three predicates test **right quantity0→true before left quantity0→false**.
Thus nonempty precedes empty; **two empty arguments also return true**.
Preserve this native empty case if reproducing its partitioning; silently
normalizing it to a strict stable-empty comparator can change larger-list
tie permutations. Negative nonzero quantities enter the occupied branch.

The three sort drivers have the native MSVC mechanics independently
specified in steam-controller-handoff §4, with **stride164**, initial
partition budget=slot count, insertion threshold≤32 and the same pivot/
partition/heap mechanics. Tab0's insertion/partition helpers are0x75560/
0x78d60, tab1's0x75230/0x78140, others'0x74fb0/0x774a0. For nonempty
equal keys, small-list insertion preserves existing order; larger lists
are not guaranteed stable. Do not append a canonical-index tie breaker.

After those sorts,0xb3ed0 visits each tab, counts its **quantity!=0**
slots, and resizes the tab to that count through **0xa2540**. This removes
the sorted empty tail, not an independent stable filtering before sorting.
It can shorten a vector and invalidate an old slot index. It does not
merge equal live stacks, transfer items between tabs, or reclassify a
saved item to the tab currently suggested by0x5f100. A saved type15 in a
nonstandard tab remains in that tab through this refresh, ordered by that
tab's comparator.

### 4.1 Pickup-frame call frequency

The call **0xaa7cd** in frame0xa5a50 is **not unconditional once per
frame**. It is inside the ordered walk of pickup notices at
World+0x110, after resolving the notice's recipient creature and adapting
the item through0x10a720. It is reached only when that resolved creature
is the current local player. A missing recipient or a different recipient
does not reach this inventory refresh. With notice flags bit0x02 set,
the per-character position-key lookup at personal+0x88 additionally
skips the remainder of an already collected notice, including refresh.

For each local, non-skipped notice, first apply its item/counter/gold
branch, then call0xb3ed0 at0xaa7cd, then ordinary InventoryWidget
0x280a40 at0xaa7d9, then0x280c80 at0xaa7e5. The refresh is reached
even for a notice whose application only changed a special counter or
gold, and is not conditioned on the amount being positive or on an
insertion helper actually appending a slot. It precedes that notice's
receipt script/used-cell/announcement processing. This matches the
existing pickup notice contract in loot-pickup §4.

There is **no inventory-widget visibility test** around these calls.
Zero local applicable notices means no0xaa7cd call; N applicable local
notices cause N such calls in list order, not one batched call after the
walk.0x280a40 itself also calls0xb3ed0 when its bound collection pointer
+0x1e8 is non-null, as §4 already specifies; that invocation likewise
does not require its widget to be shown. Thus one pickup can cause the
explicit sort and the widget refresh's additional sort. Do not replace
this event-specific order with a universal every-frame compaction.

## 5. Consequences for stored identification indices and scope

Identification stores **only tab/index**, not an item ID or copy; see
steam-identification-ui §§3/6. The insertion/removal helpers above have
no identification-state remap, and0xb3ed0/0x280a40 do not rewrite that
widget's stored pair to follow a moved item. Each later draw/use resolves
the pair against current canonical storage, with its existing bounds,
quantity and type15 tests. A refresh can make it refer to another live
item, to a non-type15 item, or beyond the shortened tab. It is not correct
to retain selection by item identity on the widget's behalf.

The raw reader stage in §1 and post-load/UI-refresh stage in §4 are
different: caller rebuilds can sort after load, but that is not permission
to drop saved records or reconstruct the inventory during deserialization.
Likewise preserving a hole immediately after a direct removal does not
mean it survives a later refresh. Canonical indices are stable across a
simple append/reuse/quantity decrement only to the extent those operations
do not themselves invoke their caller's refresh.

This pass establishes the shared storage helpers, reader/kit initialization,
all recognized direct0xb3ed0 callers, and identification/direct-consumption
paths. It is **not a complete inventory mouse drag/cursor-item state-machine
audit**, nor a proof against every unrecognized inlined writer. Such a
writer would need its own native source evidence before assigning a new
generic drag reorder or compaction event. Derived shop/buyback/crafting
collections retain their own rebuild lifecycles and are not serialized as
these canonical player tabs.

## 6. Retained cursor stack: consumers exist, no verified grab producer

### 6.1 Record and initialization/copy boundary

There is a distinct164-byte cursor stack at **inventory+0x18**:
**creature+0xa08 signed32 quantity**, **+0xa0c160-byte item**. This is
not the InventoryWidget's hover/selected tab/index, an equipment slot,
or an additional serialized tab. Constructor **0x4ce80 at0x4cf67** sets
quantity0 and a default item. Shared inventory move **0x83540** preserves
the cursor record as well as moving tabs; its two identified callers,
class-kit reset0x913b0 and character reader0x147800, supply their freshly
empty/default inventory. Character save field14 stores tab slots, and
fields15/16 store the two counters; it does **not** serialize this cursor
stack in between them. Normal character loading therefore does not
restore a dragged item from a hidden extra file field.

Other identified quantity writers preserve an existing record:
**0x9ea10 at0x9ee0a** character copy,
**0xa27b0 at0xa28ec** character-state copy, and
**0x2e17d0 at0x2e258e** resident/spawn-point copy from spawn+0x808
quantity/+0x80c item. Generated spawn inventory leaves that record empty,
as worldgen-creatures §4.5 already establishes. A copy of a nonempty
input could retain it; none of these copies is an observed UI grab origin.

### 6.2 Actual display consumer

The common InventoryWidget draw **0x27d2b0 at0x27f543** checks the local
player's quantity!=0, independently of its bound shop/crafting collection.
When nonzero, it selects renderer mode0 through0xc3dc0 and draws that
player cursor item through shared **0xddbc0 at0x27f5fa**, with:

- Center=current GUI engine float cursor **+0x180/+0x184**, in pixels;
  no additional GUI-scale multiplier here.
- Euler **(225,0,spin)** degrees. Spin uses signed32 GUI clock+0x194
  signed remainder10000, then float conversion, multiplication by stored
  **0.0001** (word0x38d1b717 at0x4ac954), then multiplication by **360**
  (0x42e7e4), preserving both float operations.
- Size **0.05** (word0x3d4ccccd at0x42e524), screen-depth translation
  **0**, dark-offset pass enabled1.

Afterward it resumes ordinary GUI rendering through GUI vtable+0x80.
The renderer mode switch is not an invented depth-buffer clear. Exact
sprite/projection/material/shadow rules remain steam-gui-item-icons.
This tail does not draw a separate cursor quantity label. It does not
move an inventory item into that cursor record merely by hovering or
drawing it. Each visible instance reaching this common draw tail uses
the same local-player record, rather than a separate cursor per panel.

### 6.3 Outside-GUI left press drops one retained cursor item

Input update **0x8ae50**, at its early cursor branch around0x8af70–
0x8b100, observes GUI engine **held-button bit1** and retained prior
byte **0x5515a0**. On a fresh rising left-held state, while cursor
quantity!=0, ordinary inventory G+0x330 is visible, and **0x33f1e0
returns no GUI hit/capture**, it copies the cursor item into the same
deferred **one-item drop-intent family** used by0x94900's middle-button
drop. This is a queued creature interaction, not a new direct full-stack
ground-item placement. It then decrements cursor quantity by1; below1
it writes quantity0, item type0 and subtype0, retaining other stale item
fields just like direct UI emptying in §3.3.

The prior left-held byte is updated after this test even when quantity0,
inventory hidden or a GUI hit prevents dropping. Holding left outside
does not drop again every frame. The ordinary enclosing0x8ae50 admission
still applies; this is not a global mouse-event hook bypassing chat/game
control gates. A GUI release is not a cursor put-back operation here.

Carry admission **0x50670** also includes this cursor quantity when its
full item equals the candidate, then adds matching ordinary tab stacks;
the existing caps remain use-prompt's 'May carry more' section. It does
not create/populate the cursor. Shared removal §3 scans tabs, not this
extra cursor stack.

### 6.4 Why there is no invented drag state machine

The traced ordinary slot interactions **do not populate this record**:
InventoryWidget selection **0x280e30** copies hover tab/index to selected
tab/index and updates its selection frame. **0x94900** handles left/right
inventory/equipment use, shop/sell operations and middle-button one-item
drop directly; **0x965e0** supplies release actions including identify.
Their recognized cursor-count writes do not contain a new grabbed stack.
The widget's actual vtable0x46ef78 binds the generic frame mouse down/up
handlers, not a hidden inventory stack-transfer callback. Its row/scroll/
region handlers change the states already specified in the controller
handoff; they do not split/swap/assign cursor items.

A native binary direct-displacement writer audit corroborates the
constructor, depletion and existing-state copies above; other+0xa08
matches are local stack or renderer fields rather than this Creature
record. The identified aliasing inventory move0x83540 is classified in
§6.1. **No verified reachable Steam UI producer for quantity0→nonzero
was found.** Therefore no pickup-whole-stack, Shift-half-stack, swap-on-
drop, put-back-on-close or generic drag reorder is authorized by these
otherwise retained consumers. Calling it dormant/legacy is a reasonable
inference, **not a formal proof against every unrecognized inlined or
indirect writer**. Ordinary direct click actions still have their existing
native behavior; absence of a verified drag is not absence of inventory use.

If a complete positive origin is required, the precise bounded evidence
needed is the **first empty→nonempty write to the current local creature's
signed32+0xa08**, together with current164-byte cursor stack, prior/new
canonical tab/index and slot bytes, input phase/button/modifiers, and
native write call stack/RVAs. A previously copied nonempty cursor is not
an origin: trace its source. This audit performs no game launch and
does not substitute a guessed modern drag interface for that evidence.

### 6.5 Follow-up writer/alias audit: still no verified grab origin

The native-only follow-up rechecked direct writes, address-taking and
inventory-pointer aliases against the same Steam binary. It still finds
**no verified ordinary input route that fills an empty retained cursor
stack**. The positive cursor writes remain precisely those in §6.1:
construction/default reset input, copies of an already present cursor
record, and §6.3's depletion. This is a bounded finding, not a proof
against every possible indirect writer or a new grant/drag action.

In particular, the general tab insertion0x4e810 never assigns inventory
quantity+0x18 or item+0x1c. It reuses/merges/appends within its chosen tab,
or updates its two special counters at inventory+0xbc/+0xc0. Its default
tab argument−1 invokes the category selector; it does not mean “cursor”.
Likewise tab accessor0x83b70 addresses a24-byte tab header in the tab
vector, not this retained stack. The generation wrapper0x52c40's
inventory argument to0x511e0 leads to ordinary0x4e810 insertion, not a
hidden cursor grab. No modifier test in these helpers supplies a split
operation on the retained stack.

Inventory move0x83540 explicitly copies cursor quantity at0x83588 and
item through0x9cd0 at0x8358b **after moving the tab vector**. It does not
clear the source cursor quantity/item as part of that move. The two
identified reset/read callers still provide their fresh empty record;
copying a deliberately nonempty input would transfer that already existing
state, without identifying where it originated. The separately recognized
clone0x9ea10 and character-state copy0xa27b0 also source the current
player's cursor; their writes at0x9ee0a/0xa28ec do not extract a tab slot.

One narrower reset distinction is verified:0x64a90 destroys/empties the
tab vector and makes its end equal begin at0x64b93, but does **not** reset
the retained cursor quantity/item. Its base-state reset0x64c00 touches
the earlier state/equipment area, not this cursor. This fact is specific
to that routine; it does not override the empty cursor supplied by the
class-kit or saved-character inventory replacement in §6.1.

### 6.6 Three different meanings of selection and “cursor item”

The old wording in **loot-shops §4.2**, “item on the mouse cursor”, must
not be read as creature+0xa08/+0xa0c. The positive native resolver for
that branch is **0x94260**, called at **0x95c1d**. It requires ordinary
inventory G+0x330 visible, hit-tests the equipment widget entries in
G+0x3c8 with0x333e70, and returns a pointer to the first hit equipped
160-byte item in this order:

| Equipment widget entry | Item address relative to local creature |
|---|---|
|0|+0x638|
|1|+0x6d8|
|2|+0x778|
|3|+0x818|
|4|+0x318|
|5|+0x598|
|6|+0x3b8|
|7|+0x4f8|
|8|+0x458|
|9|+0x8b8|

This record has no adjacent tab-slot quantity and is not copied into the
retained cursor by this resolver. A miss returns null. Numeric addresses
above establish the source identity without guessing equipment names.

The ordinary inventory-slot resolver **0x94600** instead requires the
inventory visible and pointer within its frame through0x32b0c0. It reads
hover tab/slot+0x270/+0x274, checks current canonical tab and slot bounds
and requires current quantity!=0. It returns that live signed32 pair or
(−1,−1); it does not copy an item or quantity. For button0/1 down,
0x94900 first asks this resolver at0x95013. If a valid inventory slot
exists it takes the slot's direct use/sell/identification route. Only
without that slot does the equipment-under-pointer resolver above run;
if it returns no nonempty item, the handler may resolve a shop offer.
Earlier map/other input branches and their native early returns still
apply; this precedence describes the reached inventory action branch.

The equipment branch with neither shop nor customization shown inserts
one equipped item into canonical tabs through0x4e810, then clears the
equipped item's type/subtype and refreshes the inventory. With a shop
shown it sells **that equipped item**: credit sell price, append a
quantity1 buyback entry, retain at most ten entries by removing oldest,
rebuild shop, and clear the equipped type/subtype. Source0x95c1a onward;
sell-price call0x95c73, buyback append0x95cf7, old-entry removal0x95d0d,
shop rebuild0x95d15. No player tab quantity is decremented merely for
selling this equipped source; no retained cursor quantity/item is written.
The separate valid-tab-slot sale does decrement its own live slot by1.
It must not be implemented by first assigning the retained cursor stack.

Lastly, **0x280e30** is a widget selection-frame operation. With its
copy-hover boolean true and hover slot>=0, it copies hover tab/slot to
selected tab/slot+0x278/+0x27c and requests sound0x58, then adjusts the
selection frame. With false it only refreshes that frame. It neither
removes a canonical item nor fills the retained cursor.0x94900 calls it
at0x96260 for the visible crafting collection G+0x340 on left down;
that crafting choice is not a cursor grab. The distinct live index
binding used by identification remains steam-identification-ui §6.

### 6.7 Input phases and actual one-item depletion

Physical messages reach Game down0x94900/up0x965e0 through0x133c70:
left WM0x201/0x202 maps to button0, right0x204/0x205 to1, middle
0x207/0x208 to2. Controller A/X synthesis calls those same Game handlers
with0/1, subject to the separately specified routing/edge/200-ms rules;
it has no additional inventory-stack transfer callback.

At down,0x94900 calls interface0x101020 at0x94953 **before its inventory
business actions**. The interface invokes GUI left-down0x340ec0,
right-down0x3415f0 or middle-down0x341300. Left-down sets GUI held bit1
at engine+0x1a8 before hit/capture/widget callbacks. Thus an equipment or
tab action is a down action, not a generic mouse-up drop. At release,
interface0x101120 is called at0x96767 after the earlier frontend actions
but before identification and later release actions. Its left-up0x3410e0
clears held bit1 before those GUI up callbacks; the other button helpers
are0x341780/0x3414a0. This routing does not itself move an item between
tab storage and the retained cursor.

The two verified one-item drop sources remain different:

- **Middle down**, in0x94900 at0x94ea6 onward, resolves the current live
  inventory slot with0x94600, copies one item's payload into the queued
  drop-intent record, then decrements **that canonical slot**. Quantity
  below1 becomes0 and only its item type/subtype are cleared. Types22,
  23,24 are excluded. When player state byte+0x68 is0x56, a full-equal
  item at player+0x1d8 is also excluded. No retained cursor is populated.
- **Left held rising during input update0x8ae50**, under §6.3's inventory
  visibility/no-GUI-hit gates, copies the already retained cursor item to
  the queue and decrements **cursor quantity**, not a canonical slot.
  Exact quantity write is0x8b0d3; below1 clearing is0x8b0e2–0x8b0f0.
  There is no−1/infinite-quantity exception in either direct decrement.

The latter is a held-state edge observed by the admitted input update,
not a second unconditional WM-left-down consumer. A down and up both
completed before that update leave the held bit clear and do not trigger
this retained-stack branch. Release has no verified put-back/slot-swap
operation. A refresh after other direct actions can still sort/compact
canonical tabs as §4 describes; that is separate from cursor movement.

These added source/phase distinctions close the misleading equipment/
hover/crafting interpretations. The positive empty→nonempty retained
cursor producer remains unverified. The bounded first-write oracle
described in §6.4 is still the specific missing evidence, and no original
game execution was performed to obtain it.

## 7. Canonical-slot direct use/equip on mouse down0x94900

This section covers the reached ordinary canonical-slot branch of the
Steam handler, after resolver0x94600. Shop sale, identification,
customization and middle-button dropping retain their separate routes.
It does not introduce a grabbed stack. All addresses below are Steam RVAs.

### 7.1 Common admission, item copies and insertion semantics

The ordinary branch is reached for **button0 left or1 right down**, with
the current nonnegative live canonical tab/slot pair resolved at0x95013.
Inventory selection is by current hover indices, not the crafting
selection pair or a saved item identity. Earlier event/interface handling
in §6.7 already ran. Identification shown intercepts the pair; shop
shown takes its sale branch; customization shown takes its own item
binding branch. The following equip/use rules apply after those routes
have been excluded. The stored source tab does not itself determine an
equipment destination; the item's category does.

The resolver reads the widget's stored hover pair and independently
checks its current frame hit; it does not rerun the grid's cell-picking
calculation inside this mouse event. Preserve the producer/consumer
distinction instead of deriving a new slot from the click coordinates
in0x94600 itself.

The player/item usability test **0x50640 at0x95315** occurs before the
category dispatch and its repeated source-count check. For creature
kind byte+0x60!=0 it returns true. Kind0 calls0x1094d0 with the item and
class byte+0x14c. Failure requests **sound0x35**, then rejoins the
ordinary handler tail, without consuming/equipping that slot. There is
no additional level, region, health-positive, stamina or carry-cap test
in this reached equip admission. The class/material table is
stat-rules §1.3; it applies to the incoming item, not each displaced item.
The subsequent quantity==0 check also skips the dispatch. Resolver
admission and this check use nonzero, not strictly positive, quantities.

Every incoming copy below is a **semantic item copy**, not a reduction
to category/subtype and not a required160-byte padding comparison.
Construction0x35400 and assignment0x9cd0 preserve category, subtype,
modifier, region pair, rarity, wrapped-type field, material, all32
attribute entries and the trailing attribute count. Semantic padding is
not assigned by these helpers. “Default empty item” below means all
those meaningful fields zero, including all attribute entries/count.

“Return an old item” means call **0x4e810 with quantity1 and default
tab−1**, using the item's actual category to select its destination tab.
It is not a direct swap into the selected slot. Type0 is ignored;
otherwise the ordinary merge/first-hole/append rules in §2 apply, with
no0x50670 carry-cap check added for the displaced item. Therefore an
already freed source hole can be reused if it lies in the chosen tab;
the same hole cannot be used before its count becomes0. Saved source
items in nonstandard tabs do not force displaced equipment into that tab.

“Consume the source” below means decrement that live canonical slot's
signed32 quantity by1. If the result is below1, write quantity0 and only
item category0/subtype0; retain its other old fields. There is no−1
infinite-stack exception in these direct decrements.

### 7.2 Nonweapon equipment: exact destination and operation order

After the common gate, these categories are handled identically for left
and right down, except rings:

| Incoming category | Old item returned | Incoming destination | Ordering |
|---|---|---|---|
|4|Chest+0x3b8|+0x3b8|Copy incoming, consume source, return old, assign incoming|
|5|Hands+0x4f8|+0x4f8|Same|
|6|Feet+0x458|+0x458|Same|
|7|Shoulder+0x598|+0x598|Same|
|8|Neck+0x318|+0x318|Same|
|9, left0|Ring+0x818|+0x818|Same|
|9, right1|Ring+0x778|+0x778|Same|
|20 (0x14)|+0x8b8|+0x8b8|Copy incoming, return old, assign incoming, **then consume source**|
|26 (0x1a)|**Chest+0x3b8**|**Head+0x278**|Copy incoming, consume source, return chest, assign head|

The category26 mismatch is verified in native instructions, not an
inferred intended swap:0x958be selects old+0x3b8, insertion is at
0x958d4, and0x958e4 selects destination+0x278 for assignment0x958ef.
The chest is not cleared by that return, and the old head item is not
returned by this branch. Preserve this observed behavior separately
from a desired normal equipment swap. Category20's return/assignment
before source depletion is likewise explicit at0x957a9/0x957c4 before
the quantity write0x957cd. Other armor/ring branches are0x95933–0x95bf5.

Each successful row then calls ordinary inventory refresh **0x280a40**,
followed by scrollbar/layout refresh **0x280c80**, then **sound0x5b**.
For20/26 the calls are0x958fb,0x95907,0x95929; for4–9 the shared finish
uses refresh0x959a5/0x959b1 and sound wrapper0xa03f0 at0x96226. Refresh can sort
and compact the actual canonical tabs as §4 specifies. These nonweapon
finishes do not themselves call the extra crafting rebuild0xb2180 used
by the weapon path. Later common left-click widget handling still runs.

### 7.3 Category21 starts pet-food presentation without immediate depletion

Category21 (0x15) has no left/right distinction. Admit when current
ability byte+0x68 is0, or current float32 ability age+0x6c is **strictly
greater** than the current ability duration returned by0x4f220, converted
to float32 for the comparison. Equality does not pass this second test.
Source0x957e7–0x95889.

On admission, copy the selected item to player+0x1d8, set ability0x56,
set ability age0 and hide ordinary InventoryWidget G+0x330. This branch
does **not** decrement/clear the source slot, return equipment, invoke
the ordinary equip refresh pair or request equip sound0x5b. A rejected
attempt also reaches the common handler tail. The subsequent ability/
pet interaction lifecycle remains its separate producer/consumer path;
starting this presentation is not an immediate grant or generic item use.

### 7.4 Weapon selection input is raw Shift, not a rebound action

For the weapon table below, **W1=player+0x638** and **W2=+0x6d8**.
The relevant Shift input is byte **G+0x1a97**. Its producer is the raw
virtual-key array at G+0x1a87:0x96e70 sets array[WM key low byte]=1,
0x97870 clears it, and0x133c70 routes WM_KEYDOWN/UP through those
handlers. Thus index0x10 is **VK_SHIFT**, independently of the rebound
Class Skill binding or controller action2. Constructor0x7ea80 zeros
the256-byte array. The key-down write precedes that handler's later
chat/menu actions. No Shift splitting or cursor interaction follows
from this modifier.

Raw Shift affects the ordinary weapon choices below; it does not change
the ring destinations or the other nonweapon table rows.

### 7.5 Category3 weapon routing, in native decision order

First copy the incoming weapon, then consume its canonical source as
defined in §7.1. Both operations precede **every** old-weapon insertion
in this section. The incoming item is retained in a temporary semantic
copy even if source depletion cleared its original category/subtype.

For these tests, “2H” means **category3, unsigned subtype<24, and subtype
in {5,6,7,8,10,11,15,16,17,18,23}**, mask0x878de0. It is an exact
predicate, not a size or owner-asset inference. Test incoming subtypes
13/14 first, then6/7, before the remaining 2H/ordinary cases. The table
preserves insertion, assignment and clearing order; a clear replaces the
whole semantic equipped item with the default empty item from §7.1.

| Incoming subtype / input | Exact old-item and equipment sequence after source depletion |
|---|---|
|13 or14, either button, either Shift state|If W2 is2H, return W2 then clear W2. Next return W1, then assign incoming to W1. A non-2H W2 is retained.|
|6 or7, left0, either Shift state|Return W1, assign incoming to W1, then return W2 and clear W2 unconditionally.|
|6 or7, right1, either Shift state|Return W2, assign incoming to W2, then return W1 and clear W1 unconditionally.|
|Any other2H incoming, either button, either Shift state|Return W2, assign incoming to W2, then return W1 and clear W1 unconditionally.|
|Other incoming, left0 and Shift clear|Return W2, assign incoming to W2. W1 is retained, without an old-W1 2H rejection/clear.|
|Other incoming, right1 or left0 with Shift held|Return W1. If current W2 is2H, return W2 then clear W2. Finally assign incoming to W1. A non-2H W2 is retained.|

Source is **0x95345–0x95772**. The subtype13/14 unsigned subtract13/<2
test is0x95362–0x95368; the left subtype6/7 subtract6/<2 test is
0x9537c–0x95382, with its corresponding right-button branch later;
0x95518/0x95528 join the remaining incoming-2H choice and the W2
assignment path. The asymmetry in the last two rows is real: ordinary
left without Shift does not normalize a previously 2H W1 before
equipping W2. No special insertion into the original source slot is
performed; each “return” invokes the ordinary canonical insertion helper.
The order of multiple returns can therefore determine hole reuse and
append order before later sorting.

All successful weapon rows finish **inventory refresh0x280a40 at0x95737**,
then **scrollbar/layout refresh0x280c80 at0x95743**, then request
**sound0x5b at0x95765**, then rebuild crafting collections through
**0xb2180 at0x9576d**. They rejoin the common left-widget/tail processing
at0x9622b. No retained cursor quantity/item is used for a weapon swap.

### 7.6 Other categories dispatched to the shared item-use helper

After the equipment/category21 cases, the caller's exact dispatch mask
is **0x19000002 with category<29**: categories **1,24,27,28**.
It enters the World critical section+0x3c8 through0x2abb70, calls
**0xb7740 with the live canonical slot pointer**, then leaves through
0x2b9890. Calls are0x95bfc/0x95c07/0x95c10. The incoming button0/1
does not select different behavior inside this helper.

The caller mask and helper mask are distinct. The helper's first family
admits1,22,28; the inventory-down caller above does **not** dispatch22.
Category27 reaches the helper but none of its effect families, so it
does not consume or change the item here. Do not infer type22 inventory
click use merely because another caller can invoke0xb7740 for it.

For the reached categories, require float stun timer player+0x130≤0
before the following helper effects. This is a stun test, not an
ability-finished, health-positive or carry-cap gate:

| Category/subtype | Helper's immediate changes |
|---|---|
|1/subtype9|Append a kind1 queued interaction carrying the incoming item, then consume that canonical source by1. This is the existing bomb producer in skills-and-interactions §2.3, not immediate host damage.|
|1/subtype13|Set Game bytes+0x668 and+0x669 both to1; append the kind1 item interaction; store this live slot pointer at Game+0x21c0. Do **not** consume its quantity in this path. The continuation is independently specified in steam-item-map-selection.md.|
|1/subtypes1,2,3,12|Copy the item to active-item field player+0x1d8; set ability0x54, ability age0, dodge timer+0x12c=0; consume source by1.|
|Other1 subtypes|Same active-item/age/dodge/count operations, with ability0x55.|
|28 (0x1c), any subtype|Same operations, with ability0x54.|

These are0xb7740–0xb7a5b. The source count's below1 clear again writes
only quantity/category/subtype, retaining its other stale fields. The
queued interaction's item is copied before this clearing. For the
subtype13 map case, storing the live pointer is not assigning the
retained cursor and is not a quantity reservation/decrement. No generic
click-time relocation of that slot is established by the pointer store.

**Category24 (0x18)** has its own helper branch at0xb7a5b–0xb7b56,
also requiring stun≤0. Require the item's signed32 region pair+0x0c/
+0x10 equal the player's current region+0x150/+0x154. A mismatch
requests **sound0x34 at0xb7b51**, leaving the item/ability unchanged.
On a match, first copy the item to active-item field+0x1d8, then:

- Subtype1: if current ability is0x78, set ability0 without clearing its
  old age. Otherwise set0x78 and age0.
- Subtype2: the same toggle using ability0x79.
- Every other subtype: set ability0x89 and age0, even if0x89 was already
  current.

This category24 branch does not decrement the source quantity or reset
the dodge timer. No equip refresh pair or equip sound0x5b is requested
by these delegated caller/helper paths. Later normal ability updates,
interaction consumers, visibility changes and their sounds are separate.
The direct caller does not hide ordinary inventory for these delegated
categories; category21's explicit hide in §7.3 is a distinct path.

All remaining categories take no item-specific action after the common
class/count gates; they still reach the common handler tail. This does
not assert that they have no use from other input routes.

### 7.7 Sound arguments, common health adjustment and final UI work

The0x35 rejection and0x5b equip requests described above use the listener/
camera fixed position **G+0x1dc0**, pitch1 and volume1 through0xa0420;
wrapper0xa03f0 supplies the same defaults. The callers also write an
extra final byte1 in the next argument position, which0xa0420 does not
read; it is not established here as a notice or queued-interaction flag. They
are not emitted at the selected item's world position. The category24
region error0x34 uses the same listener position/defaults. This section
specifies direct requests; later independent state/ability sound events
retain their separate update order.

Before inventory actions,0x94900 saved float maximum health from
**0x5fac0 at0x94e8c**, after the earlier GUI down dispatch. All the reached
success, failure and no-item-action paths above eventually finish at
**0x9651f**, after any ordinary left-click crafting/customization widget
processing beginning0x9622b. Thus it is incorrect to add a blanket
“other panel visible” exclusion to this direct branch merely to avoid
the later widget processing.

At0x9652a query maximum health again. Divide that new float32 maximum
by the saved old float32 maximum, then multiply the result by the
player's current float32 health+0x180, with **float32 division before
float32 multiplication**; store it. Query maximum health a second time
at0x96559 and cap stored health to that maximum only when the maximum
is smaller. Source0x9653a–0x96579. There is no added lower-zero clamp
or special old-maximum-zero branch in this handler. This preserves the
health fraction for ordinary finite positive maximum-health inputs; it
is not an additive equipment-health delta or an unconditional full heal.

Then clear ChatWidget G+0x398's active-editor byte+0x1d8 at0x96588 and
hide the quick usable-item selector G+0x310 through its shown property at0x965a8. No inventory
cursor put-back occurs in this tail. These common effects also follow
an unusable-item sound or a category whose direct dispatch did nothing.

**G+0x310 identity:** interface setup0xf9ac0 looks up the owner GUI node
named **`selector`**, stores it at interface+0x1f8 =Game+0x310,
reparents it under the dynamically created game-UI container G+0x220,
and initializes it hidden. Its pointer is distinct from the ordinary
InventoryWidget G+0x330. The configured **Ring Menu action16**
(default Tab) toggles its shown property on its rising edge in0xa5a50;
the held byte is G+0x1a80. Frame display of this selector fills its
`name`/`count` fields from usable-item choices, including the empty text
“You don't have any usable items.” Interface0x103200 updates its
`left`/`right` child feedback and hides it when the full map is open.
This identifies a quick usable-item selection menu;0x965a8 closes that
selector, rather than clearing/closing the canonical inventory grid.

### 7.8 Verification boundary

The admitted canonical-slot consumer, category/button decisions,
raw-Shift producer, semantic copies, operation order, displaced-item
insertion/count clearing, refreshes, direct sounds and shared health/UI
tail are verified in the native Steam code/instructions. No game was
executed. The inventory retained-cursor producer remains the separate
unverified question in §6.4; none of these direct actions supplies one.
The full ability-duration helper used by category21 is independently
specified in effect-objects §2.2, with existing ability update contracts
governing the later effects. The next map consumer after category1/
subtype13's request is now independently specified in
steam-item-map-selection.md. Catalog names and new producers for
otherwise unavailable item categories are not inferred from these
reachable consumer tests.

An additional storage limit remains explicit: category20 retains the
source-slot pointer across its old-item insertion, with no new lookup
before the later decrement. The order above is verified. This audit does
not define a portable outcome for a native insertion that reallocates
that source's slot vector and invalidates the pointer. A focused oracle
for that case would label source tab/index, size/capacity and base address
before/after insertion, original source quantity, inserted old-item
location and final live source quantity. No such game execution was
performed, and no allocator-dependent stale-pointer result is invented.

## 8. Auxiliary panel binding, opening and canonical sorts

The bounded follow-up **steam-inventory-refresh-events** supplies the
separate cached A/row/selection/thumb lifecycle and verified event/callback
order; simple binding or collection mutation is not an automatic refresh.

This section concerns explicit calls on the traced panel routes; geometry
callbacks and later transactions remain separate events. Addresses below
refer to Steam. The common refresh0x280a40 first requires non-null widget
+0x1e8, builds the region-choice list through0x2806a0, then sorts/compacts
the **local creature's canonical inventory** through0xb3ed0 at0x280a6d.
The widget may display crafting or shop collections; the sort still
targets the local creature. Visibility is not tested.0x280c80 only
updates scrollbar geometry; it does not call the canonical sort.

**Binding:** constructor0x27c950 initializes the InventoryWidget backing
pointer+0x1e8 to null. The normal interface update0x103200 assigns:

| Widget | Bound collection at widget+0x1e8 |
|---|---|
| Inventory G+0x330 | current local creature+0x9f0 |
| Crafting G+0x340 | Game+0x17a8 |
| Shop G+0x350 | Game+0x1870 |

These are direct pointer assignments in that update, independent of panel
visibility. The assignments themselves do not call refresh/sort or reset
canonical indices. They are not a copy/reclassification of inventory.
Identification G+0x360 instead stores its live canonical tab/index pair,
as specified in steam-identification-ui §6; it is not another bound
InventoryWidget collection.

**Inventory toggle0x102fe0:** toggle G+0x330 root visibility first. Only
on the newly shown branch copy local creature region+0x150/+0x154 to
widget selected region+0x1f0/+0x1f4, call0x280a40 at0x103078 and
0x280c80, then hide G+0x358's root. Closing by this toggle has no sort.
The selected tab and canonical selection are not reset by this routine.

**Crafting toggle0x102f10:** toggle G+0x340 root, then call0xb2180
whether newly shown or newly hidden. If the root is now shown, call an
additional0x280a40 at0x102f81 and0x280c80.0xb2180's outer gate is a
nonempty local canonical **outer tab vector**, not widget visibility or
the presence of positive item quantities. If admitted, it rebuilds its
six recipe collections in index order0 through5 via0xb2ee0, then calls
G+0x340 refresh0x280a40 at0xb2bd0 followed by0x280c80. Consequently,
with a nonempty tab vector and a bound crafting widget, this toggle sorts
once on close and twice on open. Retain those separate refreshes; larger
equal-key canonical orders need not be stable across repeated sorts.

**Panel selection0x102db0:** store requested selection at Interface+0x30c,
hide G+0x330, G+0x340 and G+0x388 roots, then dispatch selection0 to the
inventory toggle or1 to the crafting toggle. Thus selection0 necessarily
takes the newly shown inventory branch; it is not an extra inventory
refresh before that branch. Selection2 is a different auxiliary panel
and is outside this bounded inventory/crafting contract.

**Shop/identification opening from0x98180:** a reached merchant press
sets shop G+0x350 shown, calls0x102db0(0), stores merchant ID/class at
Game+0x1e98/+0x1ea0, then calls0xb75d0. That rebuild ends in shop
0x280a40/0x280c80, so the ordinarily bound route sorts once through
opening inventory and once through rebuilding shop. The identifier
class0x83 press sets G+0x360 shown then calls0x102db0(0); it adds the
inventory-opening sort but no independent identification refresh/sort.
Identification Goodbye hides both identification and inventory roots,
retaining its selection (steam-identification-ui §6). It does not
itself sort.

Simple root visibility changes do not call0xb3ed0. In particular the
movement/full-map close paths which only hide roots are not a substitute
for either opening/rebuild route above. Transaction-specific nested
refreshes remain loot-shops §4.4; a shop mode/category callback can rebuild
shop and thereby sort even though a mere root hide does not. Widget
size/layout callback0x27fce0 also calls0x280a40 when its item-frame
template+0x228 is non-null, then0x280c80 after rebuilding its hit frames;
it adds a sort if the widget is bound, including while hidden. Do not
generalize the toggle counts into a count for every surrounding layout
or transaction event.

## 9. Ring Menu selector: construction, content and input

### 9.1 Owner node and live candidate identity

The selector is the owner gui.plx node **`selector`**, installed by
0xf9ac0 at Game+0x310/Interface+0x1f8, reparented under game UI
G+0x220, then hidden. It is not constructed by0x27c950 and has no
InventoryWidget backing-collection bind, tab selection or mouse-hover
slot pair. Game constructor0x7ea80 initializes the signed32 selection
index **G+0x20c0=0**, displayed float angle **G+0x1e8c=0** and float
radius **G+0x1e90=0**. There is no item transfer into creature+a08/a0c.
This setup does not register separate native down/up callbacks on selector
or its arrow children. Normal interface layout0x101bd0, at0x102409,
places the node with0x334700: the target point is
**(float(trunc32(layout width/2)),float(trunc32(layout height/2)))**,
where the division is signed integer division toward zero before float
conversion; the additional offset is **(0,−200.0)** in parent/frame
units.0x334510 transforms the target point into parent coordinates,
adds that offset and subtracts the node's owner-defined pivot before
storing position. Thus the target uses the physical layout midpoint;
the−200 offset is transformed by its GUI parent, not independently
subtracted from physical screen Y after that conversion. Preserve the
owner selector subtree, geometry, text style and pivot.

Candidate builder **0x93e60** scans every canonical tab, then every slot,
in current stored order. It appends a pair of **signed32 tab/index**
precisely when **quantity!=0 and item category==1**. Negative nonzero
quantities qualify. It does not test subtype, region, class, usability,
stun, equipment or the retained cursor item. In particular category28,
although directly consumable elsewhere, is absent. The returned vector
has eight-byte pairs and is freshly rebuilt by each caller; it neither
sorts nor compacts canonical storage.

The selection is a retained signed32 running index, not a retained item
or canonical pair. Nonempty lists select by wrapping it to the current
candidate count. The stored running index is not rewritten with that
wrapped value. Sorting, insertion/removal and changes in candidate count
can therefore change the selected live item without resetting the index.
For positive candidate counts up to signed32 maximum, the result is
mathematical nonnegative modulo for **every signed32 index**, including
−2147483648. The native negative adjustment first negates the index in
**32 bits**, sign-extends that result, takes its unsigned64 quotient by
the count, adds one to that quotient, multiplies by count, adds the
original sign-extended index, then takes unsigned64 remainder by count
(instructions0x8e09b–0x8e0d0 and0xac90e–0xac94b). The intermediate
machine negation at−2147483648 wraps, but this final remainder still
has the stated nonnegative-modulo result for that bounded count range.
Do not introduce a new UI reset for negative indices or use undefined
signed32 negation to implement their mathematical normalization.

### 9.2 Toggle, visibility and close ordering

Ring Menu is action16: state byte **G+0x1a80**, previous byte **0x5514ff**,
default Tab scan0x0f/device0. XInput supplies its ordinary action16
state from right-stick press; this is not a new mouse-widget event.
In frame0xa5a50, when chat editor G+0x398 byte+0x1d8 is inactive and
the roots at G+0x118, G+0x130 and G+0x170 are hidden, a rising action16
edge toggles selector shown state. The prior byte updates within that
same admitted block, even without an edge; it is not updated by this
block while one of those gates prevents entry. No canonical refresh,
sort, candidate-index reset or default selection is performed by toggle.

Later in that frame a shown selector is hidden if forward action5 or
back action6 is held, or left analog Y is **strictly>0.25 or<−0.25**.
Chat activity does not add a gate to this selector-specific close test.
The earlier broad movement-close test for ordinary panels is separate.
The subsequent interface update **0x103200** also hides selector whenever
full-map Game+0x668 is true; a selector opened during that frame can
therefore close before its content/update/render stage. The selector
alone is absent from ordinary-UI predicate0x1014f0; do not globally
disable ordinary movement/camera merely because Ring Menu is shown.

The common0x94900 inventory-down tail closes selector after its shared
health adjustment and chat-editor deactivation (§7.7), including a
reached button0 or1 down. This is distinct from choosing an item in
InventoryWidget G+0x330. Quick-use closes are specified below. Ordinary
visibility changes do not sort, consume or put back an item.
Open/close sounds are sampled later in0xa5a50, not requested directly
by the selector toggle: the observed state is ordinary-UI0x9ae70
**OR selector shown**. Its rising/falling edges request0x56/0x57 at
the listener. Another open ordinary panel can keep that combined state
true while selector closes, so selector hide alone does not imply0x57.

### 9.3 Selected text and render-list production

Frame **0xa5a50 at0xac4a5** rebuilds the candidates even while selector
is hidden. With any candidates, it resolves the current wrapped index
and updates the ordinary quick-item HUD's selected item/count/name.
This does not equip or copy that item into the retained cursor record.
The subsequent selector-specific update is at0xac7xx–0xacdxx.

While shown and empty, child **`count`** is cleared, **`name`** becomes
the literal **“You don't have any usable items.”**, and that name's RGBA
becomes white. No per-item render entry is produced. While shown and
nonempty, resolve the current index **before** handling this frame's
left/right changes. `count` is the signed decimal slot quantity only
when0x5f0e0 marks its item stackable; otherwise it is empty. `name` is
the full item name from0x1628b0, colored by0x10a600 using the item's
region and the local player's current region. Do not substitute a
plain category label or a new color scheme.

Also append one16-byte scene input per candidate, in candidate order:
first pointer null, second pointer the live canonical **item payload**
(slot+4). These are temporary draw references, not semantic item copies
or inventory transfers. The scene call0xe2e00 receives this list along
with the selector radius and displayed angle. All owner nodes named
**`brackets`** under selector are shown exactly when this list is
nonempty, hidden otherwise. Hidden selector produces no such draw list.
Changing the running index later in the frame does not retroactively
change the name/count selected earlier in that update.

### 9.4 Left/right neutral latch, repeat and animation

Only a **shown, nonempty** selector enters its selection-repeat branch.
Left means action7 held **or left analog X<0**; right means action8 held
**or left analog X>0**. Selection itself has no additional0.2/0.25 analog
threshold. Global byte **0x551554** is the neutral-seen latch, and
signed32 **0x551558/0x55155c** are independent left/right repeat
countdowns in milliseconds. Their static initial values are zero.

A hidden selector clears the neutral latch and zeros displayed angle
and radius, but does not reset the running selection index or repeat
countdowns. A shown **empty** selector zeros angle/radius without
resetting that latch or advancing either countdown. Therefore holding
left/right through opening does not immediately select: a shown,
nonempty update with neither direction held must first set the latch.
An empty interval while still shown retains whatever latch state it
already had.

Inside an admitted selection update, a released direction first has its
own countdown set0. Once the neutral latch is set, each held direction
whose countdown is≤0 changes G+0x20c0 by one (left−1, right+1), resets
its countdown to **200**, and requests sound **0x58**, volume/pitch1,
at the current listener. Left is handled before right; both can execute
and both can sound in the same frame. There is no initial400-ms period
and no multi-step catch-up loop. A newly reset200 is reduced in the same
update. When latched, or when both directions are neutral (which sets
the latch), each countdown is promoted to double, subtracts the
frame's **capped double elapsed ms**, truncates toward zero to signed32,
then clamps negatives to0. If a direction remains held while the latch
is clear, neither countdown is decremented. This is frame timing,
distinct from successful-XInput-poll repeat clocks. The Game frame
upper cap at0xa5afc is **1000.0 ms**; it is not newly capped to200 here.

After the possible index changes, target angle in degrees is the
float32 product **float(index)×−360.0**, divided in float32 by the
candidate/render-entry count. If radius≤0, or absolute displayed-minus-
target difference≥**270.0**, set displayed angle directly to target;
otherwise ease via0x16ee0/0xa7a0 using capped double elapsed and
coefficient float **0x3c23d70a** (approximately0.01). A zero radius
also explicitly forces the angle to target. Then ease radius toward
the float32 product **GUI scale G+0x664×200.0**, using that same helper,
elapsed and coefficient. These stored floats, rather than an instant
wrapped angle/index, are supplied to the scene.

Interface0x103200 separately colors owner children **`left`** and
**`right`** for held feedback. Its analog thresholds are **X<−0.2** and
**X>0.2**, respectively, in addition to the action7/8 bytes. This
feedback threshold differs from the any-nonzero selector movement test.
It writes the first matching child's live Display.fillColor entry through0xf88f0: idle
**RGBA(1,1,1,1)**; held **RGBA(1,0,0.2,1)**, where0.2 is float word
**0x3e4ccccd**. Calls0x1061ba/0x1061f7 handle left and
0x10624c/0x106289 handle right. The child drawing geometry and text
geometry remain owner GUI data; these are color overrides on that
node's Display multiplier, not newly drawn arrow geometry or a recursive
vertex-color rewrite. Exact frame-slot and drawing effects are
steam-ring-selector-rendering10.

### 9.5 Use and Quick Item are separate rising-edge routes

In admitted target/use update **0x8ae50**, call prompt routine0x98180
first. When it returns0 and selector is shown, rising action11
**Use/Interact** (G+0x1a7b versus prior byte0x5515a3) rebuilds0x93e60.
If nonempty, resolve the current running index, call **0xb7740 on that
live slot**, then hide selector even if the helper did nothing, such as
while stunned. If empty, leave selector shown. This shown-selector
branch suppresses later ground pickup/vehicle fallbacks in the
no-prompt route, including frames without an action11 edge. A prompt
return1 takes its own earlier branch instead. The previous action11
byte is updated later in0x8ae50 whichever of those branches ran.

Independently, later in0x8ae50, rising **action4 Quick Item**
(G+0x1a74 versus0x5515a5; default Q scan0x10/device0) rebuilds the same
category1 candidates. With a nonempty list, resolve the retained index,
call0xb7740 on that live slot, then hide selector. This does **not**
require selector shown, and is not contingent on0x98180 returning0.
The action4 previous byte updates after that branch. Both action11 and
action4 can be rising in one update; their branches rebuild separately
and can act successively on the changed live inventory. This code is
before0x8ae50's later ordinary-UI predicate branch; do not add a new
0x1014f0 test inside it. The outer admission conditions for calling
0x8ae50 remain use-prompt §1.

The selector caller adds no inventory refresh, canonical sort, equip
sound or generic cursor action. Category1 quantity/clearing and the
subtype13 exception are the exact0xb7740 rules already in §7.6 and
steam-item-map-selection. In particular closing the selector does not
prove a quantity was consumed. This section closes input/data/visibility
behavior; the complete0xee5a0 queued sprite-descriptor transform and
later sprite draw-state path are not newly claimed here.
