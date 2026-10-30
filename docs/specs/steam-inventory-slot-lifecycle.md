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
