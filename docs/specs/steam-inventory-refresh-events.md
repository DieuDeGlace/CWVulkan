# Steam InventoryWidget refresh events and cached state

Independent native-only supplement to steam-inventory-grid-layout,
steam-inventory-slot-lifecycle4/7/8, remaining-runtime-gaps4.9/4.15
and loot-shops4.4. Sources below are Steam executable RVAs. Native
instructions were used to check omitted decompiler arguments. No
engine, tests, exporter/build, SDK, owner content or capture was read;
no game was executed. This records specific reached native routes,
not an invented observer that refreshes after every collection write.

## 1. What is cached and what each operation actually does

Available first-row-position count **A=Widget signed32+0x2a4** is cached
between calls. Constructor0x27c950 initializes it to1 and the per-tab
row-offset vector+0x288 empty. The traced InventoryWidget routines
write A in **0x280a40**, not in draw0x27d2b0, selection0x280e30,
thumb0x280c80 or thumb-move0x27fb20. Draw calculates its current C/R,
physical viewport and hover from the live collection, but does not
thereby repair A or resize/clamp the row-offset vector.

These operations must remain separate:

| Operation | Effects relevant here |
|---|---|
|0x280a40 refresh|If bound, rebuild region choices, sort/compact local canonical inventory, then calculate A and maintain row offsets. No automatic thumb or selection-frame call.|
|0x280c80 thumb|Use cached A/current offset and current frame dimensions to place/resize thumb. No collection-count calculation, canonical sort, hover reset or selection change.|
|0x280e30 selection frame|Update frame for the selected canonical pair; optional adopt-hover changes that pair. No A refresh or thumb placement.|
|0x27d2b0 draw|Read current records and stored row offsets; publish navigation/hover and immediate icons/manual text. No automatic refresh or thumb update.|

The frame's **0x103200** binds inventory G+0x330 to local creature+0x9f0,
crafting G+0x340 to Game+0x17a8 and shop G+0x350 to Game+0x1870.
Those pointer assignments do not call refresh or mark a collection
version for automatic recomputation. They run independently of panel
visibility. Ordinary Plasma animation0x335920 calls own Widget slot+0xd8;
InventoryWidget vtable0x46ef78 uses the empty0xe8a20 there. Thus that
animation callback is not a hidden every-frame inventory refresh.

**Early outcomes of0x280a40**, before adding any caller's later work:

| Condition | Native result |
|---|---|
|Bound collection+0x1e8 null|Return without region rebuild, canonical sort, A write or offset-vector change.|
|Collection nonnull|First0x2806a0, then local Game canonical sort0xb3ed0, even if hidden or a later size/tab condition fails.|
|Computed C≤0|After that work, return retaining old A and old offset-vector size/contents. No replacement C1.|
|C>0|Resize offset vector to physical backing-tab count, retaining old entries and zero-initializing additions.|
|Valid tab physically empty|Set A0 and its current row0, then return.|
|Valid physically nonempty tab|Recalculate A and clamp only current row downward, exactly grid-layout4. A region with no matching nonzero records is not the physically empty case.|
|No current valid tab/empty outer vector, after C>0|Set A1; no safe lower-bound repair for a negative tab is added.|

For **0x280c80**, collection null, thumb node null, A0 or an empty
offset vector causes return without hiding/deleting/repositioning the
old thumb. It assumes a safe current-tab index. Paired callers below
do not test whether refresh succeeded: after a C≤0 refresh return,
their thumb call can still use stale A with current dimensions; after
a physically empty refresh, A0 makes thumb do nothing. Neither a
refresh nor a thumb call resets the hover pair. Full arithmetic and
negative-boundary behavior remain steam-inventory-grid-layout2–5.

## 2. Construction, initialization, geometry and simple visibility

Constructor **0x27c950** itself stores initial state, clones/registers
controls and selects its font; it has no explicit terminal0x280a40 or
0x280c80 call. Root geometry attachment and after-children hook placement
are grid-layout7. A null binding at an early setup event remains null;
do not fabricate a temporary local-inventory binding to make it refresh.

**0x333820** initializes an own Widget through slot+0x38. For this class
that is **0x27fcc0**: run ordinary base initialization0x32b830, then
invoke its size/layout slot+0x50, **0x27fce0**. The layout hook first
requires item-frame template+0x228 nonnull. If present, it calls refresh
at **0x27fd19**, rebuilds/repositions cell frames and other controls,
then calls thumb at **0x280131**. If absent, none of that work occurs.
This hook itself does not require visible state or nonnull collection.

The normal size setter **0x32c6f0** ends by invoking own Widget slot+0x50;
**0x32c9a0** is its two-scalar wrapper. This final callback is not
conditional on an actual size difference, although propagation to
children has its separate delta checks. Therefore an explicit repeated
size request can add refreshes even when its requested dimensions did
not change. Do not collapse those calls into a single open event or
infer their absence merely because no visible geometry moved.

Changing only a root's live Display visibility is not that geometry
callback. In particular, simple movement/full-map closes, identification
Goodbye, pet-food's hide and Ring closes do not inherently refresh an
InventoryWidget. The event-specific toggle/rebuild actions below are
what supply their extra calls. The draw/animation paths above add no
universal visible-panel refresh.

## 3. Opening, closing and rebuilding panels

The verified panel routes and exact sort consequences remain
slot-lifecycle8. Their refresh/selection/thumb timing is:

| Reached event | Order |
|---|---|
|Inventory toggle0x102fe0 newly shown|Toggle visibility; select local player region; inventory refresh0x103078; thumb0x103084; hide system-menu root. Existing tab/selected slot survive.|
|Same toggle newly hidden|No refresh/thumb from this toggle.|
|Crafting toggle0x102f10, either direction|Toggle visibility; rebuild crafting0xb2180. Only when newly shown, additional crafting refresh0x102f81 then thumb0x102f92.|
|Admitted crafting rebuild0xb2180|Require nonempty local canonical outer-tab vector, rebuild recipe collections; crafting refresh0xb2bd0 then thumb0xb2bdc. No widget-visible prerequisite.|
|Panel selection0x102db0|Store requested panel, hide inventory/crafting/other auxiliary roots, then invoke the selected toggle. Inventory selection consequently takes the newly shown branch.|
|Merchant press0x98180|Show shop; open inventory via0x102db0(0); set merchant identity; shop rebuild0xb75d0.|
|Identifier press0x98180|Show identification; open inventory via0x102db0(0). No additional identification InventoryWidget refresh exists.|
|Shop rebuild0xb75d0|Rebuild its two backing tabs/current stock or buyback data; shop refresh0xb771d; thumb0xb7733, even when no merchant resolves.|

With a nonempty local tab vector and already bound crafting widget,
ordinary crafting open therefore has one refresh inside rebuild plus
one after it; close has the rebuild's one. Hidden inventory can also
be sorted by a bound crafting/shop refresh because0x280a40 always
sorts the local player's canonical collection, not the derived list.

Station Use routes in **0x98180** are distinct from the ordinary
crafting toggle. They toggle crafting visibility, select their station
tab through **0x89580** where called, then rebuild0xb2180. The direct
tab-set calls are **0x99802,0x9a7ac,0x9a8b8,0x9a98c,0x9aa60,
0x9ab2b,0x9abfc**. Most test whether the target tab differs first;
the native0x9ab2b route explicitly sets tab5 even if already selected.
For the0x9a7ac route, after rebuild, the newly shown branch additionally
refreshes crafting at0x9a7e3 then thumb0x9a7ef. A newly hidden branch
can still have the direct-set/rebuild refreshes. No common “close means
no refresh” rule replaces those actual events.

## 4. Tabs, region arrows, rows, wheel and thumb move

### 4.1 Three different tab-selection routes

**Direct setter0x89580:** write selected tab to the supplied unsigned32
value, selected pair to **(that tab,0)**, hover pair to **(−1,−1)**,
then refresh **0x895e8**, then selection frame0x895f2 with adopt-hover0.
There is **no thumb call, tab sound or shop-rebuild callback** in this
setter. It does not require the new tab to differ and does not replace
selected slot0 with the retained row's first visible slot.

**Mouse tab release0x27f980:**0x280440 registers event2 callbacks on
the tab-label Widgets. Draw0x27d2b0 sets cached hovered-tab+0x2b0 when
a label reports hover; it is initially−1 and not universally reset by
that draw. The callback reads that stored value. Require collection
nonnull, hovered tab≥0 and below physical backing-tab count, and
different from selected tab; otherwise this handler does nothing.
On admission, write selected tab, refresh **0x27f9fa**, choose selection,
selection-frame0x27fada, thumb0x27fae2, sound0x59 at0x27faf3, then
shop-only rebuild gate0x9de90 at0x27fb02. Event2 is the ordinary release
callback, not an immediate tab switch performed by the draw's hover.

**Next/previous tab0x27f830/0x2802f0:** change and wrap selected tab
using own label count+0x268, then refresh **0x27f86e/0x28031f**. Unlike
mouse selection, these handlers have no same-tab rejection when wrapping
through a one-tab set. Both then choose selection, update selection
frame, thumb **0x27f942/0x280402**, sound0x59 and shop gate0x9de90.
Controller triggers reach them under remaining-runtime-gaps4.5/4.9;
eligible panels are independently tested in inventory/crafting/shop order.

For both mouse and next/previous routes, selection becomes(−1,−1) when
the new physical tab vector is empty or its row-offset entry is absent;
otherwise it becomes **(new tab,stored row×C)**. Preserve per-tab rows;
do not reset all to0. The handler assumes native valid binding/tab
invariants around those accesses, not a safe arbitrary-null API.

**0x9de90** does nothing unless the passed Widget is exactly G+0x350.
For shop it tail-enters0xb75d0, whose final shop refresh/thumb therefore
come **after** the first pair/selection/sound above, rebuilding the new
tab's actual goods. Keep both pairs; the first precedes the shop-list
rebuild and can see its old backing contents.

### 4.2 Region and row controls

Inventory constructor0x27c950 binds matching **upbutton/downbutton**
Widgets to event2 callbacks **0x280270/0x27f7a0**, and matching
**leftbutton/rightbutton** to event2 **0x280170/0x27f6a0**. Named
descendants participate in the constructor's callback-registration
traversal; no new art or rectangular replacement button is implied.

Both region handlers request sound0x59 **first**, then search/wrap the
stored choice vector for the current region. Even with no choices or
no matching pair, previous-region finishes **refresh0x280228 then
thumb0x280230**. Next-region first explicitly sorts local inventory
at0x27f74b, then **refresh0x27f753 then thumb0x27f75b**. The refresh's
own sort is additional. Neither assigns a fresh selected-slot pair,
resets all rows or calls selection-frame0x280e30 itself.

Row next/previous first require collection nonnull and call refresh
**0x27f7b3/0x280283**. Using its newly clamped current row and A, attempt
one increment/decrement. Only an actual change then requests sound0x59,
selection frame with adopt-hover0, and thumb **0x27f816/0x2802de**.
A boundary attempt still refreshes/sorts first but adds no thumb call.
Consequently refresh alone can clamp an old row without that attempt
subsequently redrawing the thumb.

**Wheel0x97cc0** selects one target by shown root plus current GUI-tree
hit0x333e70, in priority inventory, shop, crafting; then other non-grid
fallbacks. Negative sign invokes next-row, positive previous-row, zero
does neither. WM_MOUSEWHEEL path **0x133eda** normalizes the signed high
word of wParam to−1/0/+1, so magnitude alone does not request several
rows. Controller right-Y calls the same wheel routine at **0x136b60**
with the repeat/eligibility contract of remaining-runtime-gaps4.5.
These reached row calls supply refresh; no separate wheel-level pair
is added after them.

### 4.3 Thumb movement reads the cached range

Constructor registers matching **scrollbutton** Widgets' event12 with
**0x27fb20**. GUI pointer routine **0x3418d0** dispatches event12 to the
current picked/captured node's owner Widget after its own pointer-move
virtual callback, under its normal passive/enabled callback gates.
This is the reached pointer poll path; it does not require a newly
changed coordinate to dispatch that callback.0x27fb20 additionally
requires bound collection, GUI left-button held bit0 at engine+0x1a8,
and **cached A>0**. It calls neither refresh nor0x280c80 nor a sound.

Use T=trunc32(local effective height−70), integer thumb height trunc(T/A),
and travel D=T−thumbHeight, with signed32 arithmetic. Obtain current
thumb parent-relative position via0x329f50; add this InventoryWidget's
local pointer-move delta from **0x32b0d0**, using its Y component. That
delta reads current-minus-previous GUI cursor and applies the inverse
linear XY part of this Widget's frame transform0x32ce40; it is not a
whole absolute-cursor pick or a render-frame velocity estimate.

Clamp candidate Y by first taking maximum with35.0, then minimum with
float(signed32(D+35)). When D>0, set current row to trunc32 of the
float32 calculation **float(A−1) times (candidateY−35), divided by
float(D)**, multiply before divide. When D≤0 leave row unchanged at
that stage. Then clamp row upward to0 and downward to A−1. Call
selection-frame0x280e30(adopt-hover0), query current thumb X again,
and submit that X/candidate Y through0x32b560 with byte1. Do not snap
its Y back to the discrete integer-thumb formula or resize it here.
Cached A/valid row-vector invariants remain prerequisites; no new
collection count or invalid-index repair is hidden in this handler.

## 5. Item actions and collection changes while a panel is displayed

The presence of a visible collection does not itself subscribe it to
every item mutation. The verified completion routes are:

| Event | Native refresh timing |
|---|---|
|Successful weapon equip0x94900|Inventory refresh0x95737; thumb0x95743; sound0x5b; crafting rebuild0x9576d.|
|Successful nonweapon equip|Inventory pair0x958fb/0x95907 or0x959a5/0x959b1, then equip sound; no extra crafting rebuild in that finish.|
|Unequip to canonical inventory, no shop/upgrade route|Insert old equipped item, clear its type/subtype; inventory pair0x95da7/0x95db3; sound0x5b; crafting rebuild0x95dc8.|
|Delegated direct item use0xb7740|No refresh/thumb in the helper or its ordinary inventory-down completion; quantity/ability changes can leave cached A until a later independent event.|
|Ring Use/Quick Item calls of0xb7740|Same no-refresh helper; selector caller adds none. Opening/closing selector is not an InventoryWidget refresh.|
|Admitted completed craft0x8fbc0|After ingredient consumption and insertion or target overwrite: inventory pair0x9033e/0x9034a; crafting rebuild0x90352. Carry-rejection return bypasses these.|
|Successful identification release0x965e0|After source depletion and generated-item insertion: inventory pair0x96aea/0x96af6; then sound/announcement. No independent crafting rebuild in this finish.|
|Each applicable local pickup notice in0xa5a50|After item/counter/gold application: explicit canonical sort0xaa7cd, inventory pair0xaa7d9/0xaa7e5. No inventory visibility test. Later, only if recipient is local and crafting root shown, crafting rebuild0xaad8f.|
|One upgrade palette cube0x292650|Placement/first matching source decrement only; no inventory pair, canonical sort or crafting rebuild in this helper.|
|Use-key item given to static object0x98180|Optional full-item removal, sound and queued interaction; no inventory pair/sort in that give branch.|
|Daily category22 purge0x2c34c0|Clear matching items/quantities in place; no tab erase, sort or widget pair in that purge.|

The pickup walk's missing/other recipient and previously collected
notice skip rules remain slot-lifecycle4.1. It is not a universal
per-frame refresh; multiple local applicable notices supply multiple
pairs. The later shown-crafting rebuild is separate from the earlier
inventory pair and does not imply hidden crafting always rebuilds.

Shop transaction ordering is the existing **loot-shops4.4** contract:
ordinary purchase has inventory pair, then shop rebuild, announcement,
crafting rebuild; canonical sale has shop rebuild then crafting rebuild
without an explicit inventory pair; equipped sale has shop rebuild
then equipment clear; buyback finalization has shop rebuild then
inventory pair. Their nested shop/crafting pairs still run as section3
states. Regular-stock money/carry rejection bypasses its successful
completion pair. **Resolved buyback finalization still runs with zero
successful purchases**, including nonpositive initial quantity or failed
attempts: sound0x3c, shop rebuild, then inventory pair. Do not require
a positive purchase count before that existing finalization path.

The common **left-down tail of0x94900** can adopt crafting/inventory
hover into selection and update detail/health UI. It is not another
0x280a40/0x280c80 pair; do not mistake0x280e30 for one. Data helper
insertion0x4e810, removal0x648b0 and direct quantity writes do not notify
every visible Widget automatically. Each caller's listed work matters.

## 6. Narrow original-item naming boundary

Inventory grid0x27d2b0 passes the **original live item payload**, not
the adapted display/model copy, to full name helper **0x1628b0** for
its names in modes0/2/3. Name admission, coordinates, count suffixes
and palette are grid-layout6; the following is not a replacement of
the whole dictionary/name-generation contract.

0x1628b0 first calls **0x1092b0**, which semantically copies the full
item, preserving its wrapped-type+0x18, material+0x1c, all32 attribute
records+0x1d..0x9c and count+0x9d. It also handles category22 subtypes
63/64/65/66 in that temporary: map63 to category3 with subtype from
original+0x18; map64/65/66 to category4/8/9 with subtype0. Other saved
fields remain copied. The caller's original record is not rewritten.

For effective category2, **0x1628b0's category2 branch** creates a
temporary retaining the copied item fields/attributes/count, changes
category to the low byte of wrapped-type+0x18, clears that wrapped-type
field and recursively obtains its full name, then prefixes native
literal **`Formula: `**. Do not replace this with category2's own simple
subtype lookup or discard attributes when producing the represented
item. The prefix is a native executable literal, not invented owner
dictionary text.

The helper also reads stored attribute count/material bytes on its
later material/gear path, rather than receiving an intentionally empty
attribute list. This bounded follow-up does **not** claim a complete
visible suffix/dominant-spirit rule or arbitrary corrupt-count safety.
How every such intermediate attribute tally affects final dictionary
output remains outside this timing/naming boundary. Preserve the full
source record rather than asserting every attribute changes the name.

## 7. Explicit scope limits

Direct caller inventory for0x280a40/0x280c80 was cross-checked against
the native call-reference export, including initialization/virtual
layout and callback references above. It is evidence for these reached
routes, not proof that an externally patched build has no additional
writer. The construction/character-load calls of0xb2180 at0x7f74d,
0x9efb9 and0x9f93a also reach its established crafting pair when its
outer gate passes; this document does not newly respecify all character
load/world-reset ordering or enumerate all possible indirect callbacks.

No universal mutation observer, automatic per-visible-frame refresh,
generic drag/drop, selected-slot remapping or extra redraw/sound is
inferred. A bounded native timing oracle, if needed later, should label
caller/event, Widget identity/mode/binding, shown state, selected tab,
physical tab sizes, cached A and per-tab rows before/after each actual
refresh/selection/thumb callback, plus canonical order separately. Such
an oracle must distinguish explicit data changes from subsequent layout
callbacks rather than treating one input as exactly one refresh.
