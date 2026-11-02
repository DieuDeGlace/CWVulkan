# Steam HUD context-item source and lifetime

Independent native-only bounded contract for the embedded item at
CharacterData+0x1e8 and its companion count+0x288. Sources are constructors
0x7ff20/0x7ea80, character reader/writer0x147800/0x149760, World step
0x2da6c0, copy0x9cd0, quantity helper0x119950, equality0x4df60 and interface
0x103200. No engine/tests/exporter, owner files/data/captures, SDK or
reconstruction implementation, game execution or subagents were used.
Only this new document is written; existing specifications are unchanged.

## 1. Identity, defaults and by-value storage

Let P be **World+0x448**, and D the character-data pointer **P+0x1528**.
The HUD source **D+0x1e8** is an embedded0xa0-byte item record, followed
by the **signed32 total D+0x288**. It is not a borrowed canonical-slot
pointer, tab/index pair, Ring selection, equipment reference or inventory
cursor item. Its ordinary producer is the nearby-record World-step scan
below, not a choice made by the screen-model renderer.

Fresh CharacterData constructor **0x7ff20** initializes the item's
semantic fields to empty: category0; subtype/modifier/region words0;
rarity0; full formula word0; material0; all32 four-byte attribute records0;
attribute-count0. Relevant writes are **0x801c9–0x8020d**. The final
eight-byte zero at D+0x288 sets the total to0 and the adjacent tombstone
timer to float0. No tab/index association is initialized alongside it.

This is a semantic item clear, **not a guarantee of160 zero bytes**.
Item padding+0x01..+0x03,+0x15..+0x17,+0x9e..+0x9f is not made meaningful
by these writes. Ordinary Game construction0x7ea80 allocates0x300 bytes,
calls this constructor at **0x7f627**, and assigns the resulting D to its
local creature before registering that creature as World+0x448.

The record remains part of D's allocation. Canonical-slot relocation,
sorting or removal does not move this embedded copy. Replacing/freeing
D invalidates that allocation, not an invented retained slot reference.
An admitted rebuild can replace its contents every World substep.

## 2. Rebuild admission and clearing phase

In World step **0x2da6c0**, the context slice is inside the local-player
branch admitted at **0x2da911–0x2da93d**. It requires:

- Nonnull World+0x448 local creature.
- Nonzero **World+0x210**, the64-bit length of loaded World name+0x200.
- Current **ability byte P+0x68** is not0x98,0x9c or0x9d. Native's byte
  mask has an explicit0x99 allowance;0x99 is admitted. This is an ability
  gate, not a race/class test.

There is no additional authority-byte World+0x358, HP-positive,
UI-visible, positive-dt, Ring-open or current-region test on this slice.
It follows the earlier prelude/pet work and local0x66850 call, and occurs
before the later ordinary World creature-update traversal. Other earlier
work in the same admitted branch need not be repeated as a context rule.

At **0x2db18b–0x2db24d**, replace D+0x1e8's meaningful item fields with
the semantic empty item and write **D+0x288=0**. This occurs before the
nearby-zone scan, even if that scan subsequently finds no candidate.
Clearing is not conditional on whether the previous item equals a new
one, and does not preserve an earlier candidate across admitted rebuilds.

When the outer admission fails, this slice is skipped: it does not
clear/refresh the old item/count. A matching Game frame normally runs
successive World substeps, including its final zero-ms residual, before
local input and interface/scene submission. Each admitted substep can
rebuild; the final reached result is what later HUD consumers read.
Skipping the World batch leaves the prior result until another actual
writer/replacement. The established batch ordering is
[Game frame order §§1–2](steam-gameplay-frame-order.md).

## 3. Loaded-zone scan, candidates and exact overwrite order

At **0x2db254–0x2db32d**, derive the center zone from P's **physical
signed64 fixed X/Y at+0x10/+0x18**, independently. The inline conversion
matches the narrow-block-first operation in
[trap coordinates §2](steam-world-traps.md#2-verified-coordinate-conversion-including-out-of-range-negative-inputs),
using final division by64:

1. Divide fixed by65536 toward zero as signed64, then narrow the quotient
   to signed32 block coordinates.
2. For an originally negative fixed value, compare it with the
   sign-extended narrowed block multiplied by65536; if unequal,
   decrement the block with32-bit wrapping.
3. Floor-divide that signed32 block by64. Form the neighboring zone
   bounds only afterward.

Visit **centerX−1 through centerX+1 inclusive, X outer ascending**, and
for each X **centerY−1 through centerY+1 inclusive, Y inner ascending**.
At0x2db360–0x2db434, look up each key in the World's **current loaded-zone
map** and skip a missing/null Zone. There is no zone allocation/request
or ordering by Zone-pointer value here. The coordinate walk determines
visit order, even when a map's internal hash order differs.

For each found Zone, perform these two passes in this order:

|Pass|Admission and copied source|
|---|---|
|Ground vector **Zone+0x30 begin/+0x38 end**, forward0xe0-byte records,0x2db43a–0x2db473|Only ground item's category byte at record+0 is **0x16**. Copy that record's leading original item into D+0x1e8 through **0x9cd0 at0x2db464**.|
|Live static-object vector **Zone+0x60 begin/+0x68 end**, forward0xe0-byte records,0x2db475–0x2db4b5|Require embedded requirement item's category **object+0x38 nonzero**, then **object state byte+0x28 nonzero**. Copy the original item at **object+0x38** through **0x9cd0 at0x2db4a6**. No object-type filter is added.|

Every accepted record **overwrites** the same embedded item. There is no
early exit, nearest-distance comparison, aim/crosshair selection,
line-of-sight/range-within-zone test, quantity availability check or
explicit candidate-region comparison in these passes. Ground stack
quantity/owner/timer and static object item-count+0xd8 are not used to
choose this copied record or its total. The static pass follows all
ground records of that zone, before the next zone.

Therefore **the last eligible record in this exact walk wins**, before
inventory availability is tested. In particular an unavailable late
requirement can suppress an earlier available candidate; no earlier
candidate is retried. Empty/unloaded zones contribute no write. No
inventory sort, source-object mutation, item consumption, sound or CRT
draw is performed by this bounded selection/copy slice.

Copy0x9cd0 preserves the source's semantic fields, including full subtype,
modifier, region pair, rarity, full32-bit formula word, material, all32
stored attributes and attribute count. It does not copy item padding.
There is no player-class adaptation, region reassignment or synthetic
defaulting of a selected source's other meaningful item fields. The HUD
later resolves a model from this original copy under its separate item
normalization rules; it does not turn this selection into a slot index.

## 4. Companion quantity, equality and final rejection

After completing the whole coordinate scan, **0x2db4f8–0x2db519** calls
**0x119950(P+0x9f0,D+0x1e8)** and stores the returned signed32 total at
D+0x288. The helper derives a **category-selected canonical tab through
0x5f100**, not a saved selected index and not a scan of all tabs. Categories
0x16/0x18 use tab1. A static object's other requirement category uses the
same established category-to-tab rules in the canonical inventory contract.
If that tab is absent the result is0.

Within the selected tab, visit every physical0xa4-byte slot forward,
compare its item to the copied context item using **0x4df60**, and add
the slot's **signed32 quantity** for each equal item. There is no quantity
nonzero/positive guard before equality and no early exit after a match.
Addition is32-bit native addition; negative values participate and an
overflowing total wraps rather than being clamped. Reordering equal
records does not attach an identity/index to D+0x1e8.

Full equality compares category; subtype/modifier words; both region
words; full formula word+0x18; material; rarity; attribute count; and each
of the **active count's** four attribute bytes. It ignores item padding
and inactive attribute records beyond that count. It is not a160-byte
memcmp or category/subtype-only availability test. The established field
contract is [dialogue item actions §4.1](steam-dialogue-item-actions.md).
Malformed attribute counts outside the ordinary record capacity are not
assigned a replacement rule here.

At **0x2db52d–0x2db576**, if the stored **signed total is less than1**,
copy the semantic empty item into D+0x1e8 again. **The just-computed total
is retained**; this second item clear does not write it back to0. Thus a
negative summed quantity leaves an empty context item and negative total.
If total is at least1 the selected original item remains. No fallback
to another candidate, quantity normalization or inventory mutation occurs.

Region relevance is consequently **full equality against inventory**,
not a separate equality with P's current region. A matching regional
inventory record can satisfy the copied candidate even when that region
is not P's. No matches elsewhere in another canonical tab are counted.
If no candidate was copied, the helper still receives the semantic empty
item; malformed positive quantities on empty records do not manufacture
a nonzero category or authorize an invented candidate.

## 5. Model/count consumers and Ring/Quick/UI distinction

The screen-model consumer **0xe442f–0xe4444** tests only the embedded
category byte. Its model/shadow/effects and special-row space reservation
remain [HUD models §4](steam-hud-model-overlays.md). The positive-inventory
condition is supplied upstream by§4, not rechecked by that renderer.

**Count text's exact reached target:** startup0xf9ac0 names an outlined
text subtree **`contextitem`**, creates it through0x1008a0 at **0xfcf1d**
and stores it at **Interface+0x208 at0xfcf22**, equivalently **Game+0x320**.
Its existing font24/outline5/placement are HUD money§4; this supplement
specifies the data update rather than new text geometry.

In ordinary Interface **0x103200**, **0x104fe7–0x105014** reads the current
P→D and starts from a freshly empty output string. Only **D+0x288>1,
signed**, appends the ordinary signed decimal integer through the wide
stream. **0x105036 →0x334c50** applies the resulting string to every
admitted TextShape in that stored `contextitem` subtree. No `x` prefix,
category name, region, quantity clamp or replacement zero is appended.
Totals1,0 and negative totals yield the empty string. Consequently the
ordinary total<1 item-clear path also leaves empty count text, while a
single available item keeps its model but has no numeral.

That subtree helper excludes node flag0x04, rather than hidden nodes;
it assigns only differing live strings, with no rebuild or direct
keyable/node/Widget dirty write of its own. Equal decimal/empty text does
not acquire a new invalidation rule. Its precise behavior remains
[cache invalidation §4](steam-plasma-cache-invalidation.md).

This top-level interface update has **no context-item category check,
own-node shown check, global-root/F4 check or positive-dt gate**. It does
not show/hide that named node according to item/count state. It updates
the stored text even while its drawing ancestor is hidden; actual GUI
visibility/traversal still decides whether pixels appear. Malformed or
stale state with category0 and total>1 would still send that decimal text
through this literal consumer. No implied category/count consistency
repair belongs to the text setter.

The ordinary Game call **0xab5ce →0x103200** follows the earlier completed
World-step batch and precedes scene submission. It reads the current
record/count; it does not recompute nearby selection or available quantity.
Inventory actions later than the last context rebuild can therefore await
the next actual rebuild, rather than trigger a speculative same-frame one.

Ring/Quick Item and ordinary inventory hover/use have their established
separate sources. This producer reads **nearby ground/static records plus
the category-selected quantity sum**, and never reads their selected
tab/index, hover pair or quick-item pointer. Those actions may alter the
world/inventory for a later scan, but no direct index association, cursor
handoff or context selection from their current UI highlight is established.

## 6. Save/reset/copy boundaries and explicit limits

The ordinary character reader **0x147800** resets the target creature,
destroys/frees its old CharacterData when present, allocates0x300 bytes
and constructs fresh data through **0x7ff20 at0x14788b**. After reading
the name it again replaces that data through a fresh constructor at
**0x147cf8**, then fills the serialized personal fields. Both constructions
give this context item/count the semantic empty/0 defaults above.

The reached version11 character writer **0x149760** serializes the
established field sequence in [native saves §3.3](native-saves.md).
It does **not** serialize D+0x1e8 or D+0x288, and this reader has no field
that restores them from a selected inventory index or raw context item.
Reload therefore does not preserve a cached nearby-context choice; it
starts empty until a subsequent admitted World step supplies one.

Do not generalize that replacement into every creature reset:
**0x64a90 itself does not replace D or clear these two fields**. In the
character-load route it is the caller's subsequent free/new construction
that does so. The selected item is ordinary embedded value data; no
separately allocated model/slot ownership is transferred by0x9cd0.

No dedicated network field/message transferring this selected cache was
established in this bounded trace, and none is invented. Canonical items
and loaded nearby records have their own existing save/network sources;
that does not make the derived context cache itself a transmitted field.
An exhaustive network/whole-CharacterData clone or external-writer census,
all World replacement callers, malformed containers/pointers, concurrent
mutation and invalid resource fallback remain outside this contract.
