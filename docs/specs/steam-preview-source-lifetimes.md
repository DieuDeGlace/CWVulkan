# Steam ordinary information preview: source producers and lifetimes

Independent native-only supplement to the existing
[preview selection/placement contract](steam-voxel-widget-information-preview.md).
That document's sections2/4 give the complete selector priority and
section7 gives comparison identity, actual formatter inputs, Shift and
size timing. Those facts are not duplicated here. Sources are native
0x2829c0,0xb6b00,0x2b8fd0,0x8ae50,0xa5a50,0x93e60,
0xd3170 and0xcc650, plus the existing independent map/Ring/loot prose.
All addresses are Steam RVAs. No engine, tests, tools/exporter/build,
SDK, owner assets/data/captures or game execution were used.

## 1. MapOverlay's embedded item is a draw-produced semantic copy

**MapOverlayWidget G+0x380**, constructor0x282170, owns a semantic
item member at **+0x1b0**, followed by its separate hovered-lore cell
pair **+0x250/+0x254**. Construction makes the item semantically empty
and the cell pair **(INT32_MAX,INT32_MAX)**. The existing owner-data /
marker acquisition, projection, sorting and complete hit rules are
[map-markers part1](map-markers.md); this is the information-record
handoff from that producer.

In reached draw **0x2829c0**, collection and sorting precede hover
output reset. The semantic empty-item assignment at **0x285b41**
clears +0x1b0 before the hover pass, and the separate cell pair is
reset to the sentinel. This is semantic item assignment0x9cd0, not
a newly required160-byte padding comparison or raw-memory clear.

The hover pass requires **Game+0x668 nonzero** and **Game+0x1db0
zero**. In shrine-choice mode+0x669, only shrine-target-eligible
markers are considered. Its strict half-rectangle and nearest-pointer
distance selection, sorted traversal and finite ties remain
map-markers1.7. On an admitted new-best marker with **flagC+0x82**:

* A nonnull marker **+0x88 item pointer whose category byte is nonzero**
  is copied through **0x9cd0 at0x285e31** into Overlay+0x1b0.
* Without that item, flagD+0x83 can write the lore-cell pair instead.
  It does not turn the marker icon/category/mission level into an item.

One concrete pointer producer is the collected mission entry from the
**region mission list+0x38**, with inline semantic item at **entry+0x28**.
Collection puts that address in marker+0x88 when its category byte is
nonzero; see map-markers1.3.2, particularly step13. This is world mission
metadata, not the player's canonical inventory or a new quest-award
transaction. The present audit does not regenerate those item contents
from a marker kind, icon or rarity. It also does not claim that mission
metadata has no other upstream producers.

The published overlay member is a **copy**, so it does not borrow the
temporary marker vector or depend on that temporary vector surviving
the draw. The preview later borrows the stable address **Overlay+0x1b0**
as its original item. The semantic member contains no quantity prefix;
its quantity input must not be inferred from an inventory stack.

Reset occurs once before traversal, not on every new-best hit. Later
closer markers overwrite only outputs their own flags actually write.
For example, a later marker without an admitted item pointer does not
implicitly clear an earlier item copy. The independent lore/location
sentinel fields and their draw precedence remain preview section4.

When an overlay draw reaches the reset but the map is closed, dragged,
or has no admitted item marker, it leaves the item empty. A skipped
draw does **not** execute that reset. Selector0xb6b00 itself neither
clears this member nor requires the overlay root currently shown:
its first ordinary source test is just that member's category. These
are draw/selector lifetimes, not a new eager clear on every logical
map-toggle event. The selector's borrowed pointer does not freeze the
member contents against a subsequent native draw updating that member.

## 2. Ground preview borrows a current World record resolved by tuple

The current ground-target key is the **three signed32 values**
**Game+0x1e70/+0x1e74/+0x1e78**: zone X,zone Y,index. The reached
target-reset stage of0xa5a50 sets it to **(INT32_MAX,INT32_MAX,0)**;
the copy store is **0xa67c8**. It is not a retained ground-item pointer.

The actual player target/Use pass **0x8ae50**, called at **0xa9b2c**
when its enclosing call admission holds, can populate this tuple before
the preview selector at **0xa9d22**. Its ground target producers and
proximity/crosshair/line-of-sight/reservation/automatic-pickup rules are
[use-prompt2.4–2.5](use-prompt.md), with ground-record behavior in
[loot-pickup](loot-pickup.md). Native writes include the crosshair
selection at **0x8dc67** and nearby fallback at **0x8dde7**. These
select original loaded-zone record identity, not a class-adapted copy.
The target producer's validity test0x2b8140 requires category!=0 and
quantity+0xd4!=0; negative nonzero quantity is not excluded by that
particular test. Its remaining eligibility rules remain the existing
targeting contract.

The ordinary prompt-owned continuation can also clear the key. One
explicit early case is **0x98180 returning true with Ring hidden**:
**0x8df99–0x8dfd4** writes all three fields to INT32_MAX, before its
later object-use continuation. This differs from the frame reset's
index0; the first-zone sentinel still denotes absence. Other Use /
prompt continuation details retain their existing source contract;
do not infer that queueing a pickup is immediate removal of the World
record or permanent capture of a preview pointer.

At **0xb6b6d**, preview selection resolves the current tuple through
**0x2b8fd0** only if its first field is not INT32_MAX. The resolver:

* Looks up the existing zone key in **World+0x460**, without creating a
  zone; requires an existing nonnull zone pointer.
* Requires index>=0 and index less than the current vector length
  derived from **zone+0x30 begin / +0x38 end**, stride **0xe0**.
* Returns **begin+index×0xe0**. The semantic item starts at offset0
  of this record. Missing zone or invalid index returns null.

This resolver does not repeat the target producer's category, quantity,
reservation, carry or distance tests. Thus a still-in-bounds tuple can
resolve a zero/changed record if those contents changed; a new blanket
quantity0 preview-hide predicate is not supplied by this resolver.
The selector captures its signed32 **quantity+0xd4** as the scalar
input described in preview section4.

If ground fallback supplies the selected preview, **+0x1f0 original
item** and **+0x1e8 ground origin** point to the same World record
base. They are borrowed, not independently allocated/refcounted item
copies. The original Item160-byte payload, position, count and reserved
owner ID+0xd8 retain the ground-record layout. A higher-priority ordinary
UI source changes the selected item and sets preview+0x1e8 null, as the
existing selector contract specifies. The scalar count captured earlier
is not reset by those later source choices, and the current formatter
does not read its supplied quantity argument (preview7.2).

The origin record is not a reference player. It lets item-detail
formatting inspect reservation/ownership metadata. The actual local
player used for selected-item adaptation and stats remains
**World+0x448**. No source-record mutation is performed by the preview
adaptation calls. A native World-vector mutation can invalidate a
borrowed address; this handoff provides no promise that it survives
zone unload, erase, reallocation or replacement. Each subsequent
selector pass resolves current tuple/storage again.

The later scene call **0xe2e00 at0xad9d4** receives the same target
tuple. In its ordinary raw-scene path, **0xd3170** uses that tuple for
selected-ground comparison through **0xcc650**, whose32-byte body
compares the three integer fields without writing either input. That
is a consumer for highlighting the already selected record, not a
second ground-target producer or a replacement UI-pointer pick.

## 3. Ring preview borrows a freshly resolved canonical candidate

When **Ring G+0x310 is shown**, selector0xb6b00 bypasses the ordinary
MapOverlay/inventory/equipment/crafting/shop/Voxel hover chain and
calls candidate builder **0x93e60 at0xb6bc7**. The exact builder and
stored-index rules are [slot-lifecycle9.1](steam-inventory-slot-lifecycle.md).
It freshly scans canonical tabs/slots in stored order, admitting
**quantity!=0 and category1**, including negative nonzero quantities.
It does not sort/compact canonical storage or add region/class/usability
filters. This list is not the broad Quick HUD count-mask family.

For a nonempty candidate list, lookup uses **Game signed32+0x20c0**
wrapped to the current count and resolves that tab/slot's **original
live item payload at slot+4**. This path does not clone, equip, consume
or transfer the item into the inventory cursor. It does not overwrite
the stored running index with its wrapped lookup value. The negative
adjustment includes native32-bit negation at **0xb6dc9**; the bounded
nonnegative-modulo contract and INT32_MIN behavior remain the existing
Ring specification rather than an inferred new reset.

With no candidates, this Ring branch retains the already resolved
ground fallback, if any; it does not try the skipped ordinary hover
chain. With neither candidate nor ground fallback, the selected item
pointer is null. Location/lore sentinel overrides can still show their
separate preview modes, as the existing selection contract states.

Unlike the ordinary hover-success branches, the Ring branch does not
set the selector's UI-source-success flag. Therefore its final
**Preview+0x1e8 can retain the earlier ground-origin pointer even when
+0x1f0 now points to a Ring canonical item**. Its scalar count likewise
retains the selector's earlier default1 or ground-record count, not the
Ring stack quantity. Those fields have these independent native sources;
do not force them to describe a single copied stack. The formatter's
ignored quantity argument and the real Ring/Quick counters remain
separate facts.

Closing Ring is a visibility change, not destruction of a retained
preview item or a normalization of+0x20c0. On the next selector pass,
hidden Ring permits the ordinary priority chain again; pointer/count /
comparison fields are recomputed through that pass. The exact same-frame
ordering of Use, preview selection, later Ring toggle/close, HUD labels
and render queue remains [Ring-rendering7.3](steam-ring-selector-rendering.md).
The preview must not move its source sampling to the later label or draw
stage merely to make them use one synthetic snapshot.

## 4. Bounded completion

Map semantic-copy production, current ground-tuple resolution, Ring
canonical borrowing, clearing boundaries and scalar/origin coupling
above are stable. Original-equipment comparison and draw/size timing
are completed by preview section7. The full item formatter's remaining
stat rows/deltas/effect paragraphs remain explicitly untraced there;
this supplement does not invent them or upstream quest awards.
