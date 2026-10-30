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
