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
|1/subtype13|Set Game bytes+0x668 and+0x669 both to1; append the kind1 item interaction; store this live slot pointer at Game+0x21c0. Do **not** consume its quantity in this path. The subsequent map/interaction consumer determines the continuation.|
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
hide widget G+0x310 through its shown property at0x965a8. No inventory
cursor put-back occurs in this tail. These common effects also follow
an unusable-item sound or a category whose direct dispatch did nothing.

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
subtype13's request is not newly audited here, nor are catalog names or
new producers for otherwise unavailable item categories inferred from
these reachable consumer tests.

An additional storage limit remains explicit: category20 retains the
source-slot pointer across its old-item insertion, with no new lookup
before the later decrement. The order above is verified. This audit does
not define a portable outcome for a native insertion that reallocates
that source's slot vector and invalidates the pointer. A focused oracle
for that case would label source tab/index, size/capacity and base address
before/after insertion, original source quantity, inserted old-item
location and final live source quantity. No such game execution was
performed, and no allocator-dependent stale-pointer result is invented.
