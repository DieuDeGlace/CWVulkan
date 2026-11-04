# Steam derived vehicle/special-item selection

## 1. Scope, objects and stored item identity

This bounded native-only contract describes the item consumed by the ordinary Use fallback, its World-step producer and directly reached availability queries. Sources are original Steam routines **0x7ff20, 0x2da6c0, 0x5e890, 0x4df60, 0x2b84d0, 0xeab0/0xe9a0, 0x8ae50, 0x98180, 0xb7740, 0x147800 and0x149760**, plus the independent prose linked below. No engine, tests, exporter, SDK/reconstruction implementation, owner outputs or game execution was used.

Addresses are RVAs in the original AMD64 `cubeworld.original.exe.unpacked.exe`, preferred base **0x140000000**, SHA-256 **7af18cb28c8bf848f355287428c6390b2ea2a682ca466190f5eb9859c5d6f244**.

Let P be the local creature **World+0x448**, and D its CharacterData pointer **P+0x1528**. The relevant selection is an **embedded0xa0-byte original-item value at D+0x148**. Its category byte is D+0x148, subtype signed32 word D+0x14c, modifier D+0x150, region pair D+0x154/+0x158, rarity byte D+0x15c, full32-bit formula/seed word D+0x160, material byte D+0x164,32 four-byte attribute slots D+0x165…+0x1e4, and attribute-count byte D+0x1e5. It has no stack quantity or live canonical-slot index attached to it. The neighboring item D+0x1e8 and its count D+0x288 are the separate [HUD context cache](steam-hud-context-item-lifecycle.md), not this selection.

The player's **active/held item P+0x1d8** is another original-item value. It is not D+0x148 and is not an equipped inventory slot. The ordinary producer below generates the D item from conditions; it does not copy the first matching inventory item, the currently held item or a saved vehicle choice into it.

## 2. Construction, rebuild admission and timing

CharacterData constructor **0x7ff20**, beginning this item's writes at **0x80185**, initializes its semantic item fields to empty: category/subtype/modifier/region/rarity/full formula word/material/count0 and all32 attribute slots0. Item-local padding **+0x01…+0x03, +0x15…+0x17 and+0x9e…+0x9f** is not made meaningful by that initialization. This is not a guarantee of160 zero bytes. Ordinary Game construction creates D through0x7ff20 and attaches it to P as already specified in the HUD-context lifetime note.

The selection rebuild is inside the ordinary World step **0x2da6c0**, in the local-player branch admitted at **0x2da911–0x2da93d**. It requires a nonnull P, nonempty loaded World name **World+0x210**, and current ability other than **0x98,0x9c,0x9d**. The native mask specifically admits0x99. There is no additional positive-dt, HP-positive, authority-byte, UI-visibility or inventory-open test in this slice. When the outer branch is skipped, it does not clear this item merely because a Use consumer later reads it.

Within the admitted slice, availability-driven cancellation at **0x2daca5–0x2dad1d** occurs first:

|Current ability|Cancellation condition|Write|
|---|---|---|
|0x76|Availability0x5e890(P,3) is false|Ability byte becomes0|
|0x78|P+0x5c bit0 is set, and availability(P,1) is false|Ability byte becomes0|
|0x79|Availability(P,2) is false|Ability byte becomes0|

These cancellations do **not** reset P+0x6c. The ability/current age read by the subsequent rebuild is therefore the then-current state after those byte writes.

The rebuild then semantically clears D+0x148 again, including all32 attribute records, at **0x2dad1d–0x2daddd**, before testing selection eligibility. A failed inner eligibility test thus leaves an empty item rather than retaining the previous selection. Eligibility is:

- Dodge timer **float32 P+0x12c equals0**, with an ordered comparison; either signed zero is equal, NaN is not admitted.
- Retain float32 ability age **P+0x6c**, obtain the **whole ability duration from0x4f220(P)**, convert its signed32 result to float32, and require **age strictly greater than duration**. Equality does not pass. This is not the effect-threshold helper0x4ead0.
- The then-current ability must not be **0x7a**.

This producer runs **before the per-creature physics/ability/interaction/upkeep traversal** at **0x2db674–0x2db6d4**. That traversal's calls are physics0x2bb8e0, ability0x2c5250, interaction0x2c3710, then upkeep0x2de4c0. Consequently the selection can describe state from before that step's later movement/ability updates.

In the ordinary client Game frame the complete World batch, including the final residual or called zero-ms step, precedes drawn-position processing and local control **0xa9b2c→0x8ae50**. The prompt call **0x8df69→0x98180** and its fallback come afterward, once in that local-control invocation. There is no additional World rebuild after starting a vehicle in that same frame. See [steam-gameplay-frame-order.md §2](steam-gameplay-frame-order.md#2-ordered-stages-in-one-ordinary-frame) and [steam-world-creature-visitation.md §2](steam-world-creature-visitation.md#2-per-substep-visits-inside0x2da6c0). A wanted/loaded-world mismatch that skips the World batch is not an implicit selection refresh.

## 3. Generated record and exact subtype priority

After the semantic clear and eligibility above, **all emitted selections have category0x18**. The producer assigns only that category and a subtype. **Modifier, both region words, rarity, full formula/seed word, material, attribute count and all attributes stay0.** In particular, the generated item's region is **(0,0)** even when availability was established from an original inventory item belonging to the player's nonzero current region. No original item's modifiers, attributes, quantity or identity are retained.

First, an already current ability **0x78** provisionally writes subtype1 at **0x2dae34/0x2dae49**; an already current ability **0x79** provisionally writes subtype2 at **0x2dae60/0x2dae75**. Other abilities leave the cleared item provisional-empty. These provisional selections can be overwritten by the following priority chain. A failing later test does not clear a provisional selection.

The five special-item candidates are attempted in this exact order. In each row the availability query runs first, then the object probe(s), then the flags test. Stop at the first successful row and emit that subtype:

|Priority|Availability query subtype|Nearby static-object probe0x2b84d0: kind, require-state byte|Additional P+0x5c test|Emitted subtype / write range|
|---:|---:|---|---|---|
|1|7|kind1, false|Low two bits are not both0|7;0x2dae7f–0x2daeda|
|2|13|kind9, false|Same|13;0x2daedf–0x2daf3a|
|3|4|kind0, true|Same|4;0x2daf3f–0x2daf98|
|4|8|kind2, false; if false, kind15, false|Same|8;0x2daf9d–0x2db00b|
|5|5|kind16, true|Same|5;0x2db010–0x2db06c|

Subtype4 availability is unconditionally true in0x5e890; its object/state/flags requirements still apply. The second probe in priority4 is skipped when its first probe succeeds. These kind numbers are actual static-object record kinds, not vehicle subtype aliases.

If none succeeds, test **boat subtype2** at **0x2db071–0x2db0e7**: availability(P,2) must be true; the low-byte flags must satisfy **bit1 set and bit2 clear**, equivalently the native low-three-bit mask result is2 under mask6. The producer queries the effective block at P's physical fixed XYZ through0xeab0 and requires **block flags byte+5 bit0x20 clear**. Success emits subtype2 and ends selection.

Otherwise test **glider subtype1** at **0x2db0ec–0x2db181**: availability(P,1) true; P+0x5c low two bits both0; current ability not0x53; and either ability0 or retained age **at least** the float32-converted whole duration returned by another0x4f220 call. This later non-strict test remains a separate reached query from the earlier strict eligibility test. The effective block at P's physical XYZ must again have flags bit0x20 clear. Success emits subtype1.

If all tests fail, leave the provisional item unchanged: empty for an ordinary unselected ability, or the subtype1/2 preset for current0x78/0x79. Thus the environmental0x20 tests do not retroactively erase an active ability's provisional selection. The reached producer emits **only subtypes1,2,4,5,7,8,13**. It does not generate subtype3, even though one fallback cancellation condition mentions it.

## 4. Availability0x5e890: exact inventory and equality contract

The helper receives P and a signed32 requested subtype. **Subtype4 immediately returns true**, without inventory, region or quantity inspection. Every other subtype constructs a semantic original-item template with:

- Category0x18 and the requested subtype.
- Modifier0; region pair copied from the current cached **P+0x150/+0x154**.
- Rarity0, full32-bit formula/seed word0, material0, attribute count0, all32 attribute records0.

It does not recompute a region from physical coordinates, use the generated D item's zero region or use the active held item's region. It scans **all physical inventory tabs in order**, then every physical slot in each tab. P+0x9f0/+0x9f8 delimit the tab array, whose elements are0x18 bytes; each slot is0xa4 bytes, with signed32 quantity followed by the0xa0-byte original item.

For each slot, compare the item's value against the template through **0x4df60**. This is the proved field equality, not category/subtype-only matching and not a160-byte memcmp. It compares category byte+0, subtype word+4, modifier+8, region words+0x0c/+0x10, full32-bit word+0x18, material byte+0x1c, attribute-count byte+0x9d and rarity byte+0x14. Equal nonzero attribute counts cause the four bytes of each active attribute slot to be compared. With this template's count0, attribute payload bytes are not compared. Item padding is not compared. A slot with stale unused attributes can therefore match; an otherwise similar item with a different modifier/rarity/full word/material/count does not.

On every equality match add the slot's **signed32 quantity** to a signed32 accumulator, using native32-bit word addition. There is no preliminary quantity-positive filter, early first-match return or count saturation. The result is true only when the final signed32 sum is **strictly positive**. Matching negative quantities can cancel positive quantities, and the native sum wraps rather than widening to an unlimited total. Zero quantities contribute0. This is an availability calculation, not a selection of any particular slot or transfer of its item payload.

## 5. Spatial/flag inputs to selection

### 5.1 Nearby-object probe0x2b84d0

The probe uses the World's current local P, an exact signed32 object kind and a byte telling whether nonzero object state is required. The reached calls/arguments are fixed in section3; they must not be replaced with a generic nearby-interaction or creature probe.

It visits loaded zones intersecting P's **physical X/Y ±8 blocks**, with X outer and Y inner, inclusive bounds, then their current ordinary object vectors **Zone+0x60…+0x68**, stride0xe0, in list order. Missing/null zones contribute no objects. Bounds come from the fixed64 coordinate after adding/subtracting **0x80000**, not a float distance rounded to a tile count.

For each fixed bound the native block conversion adds65535 only for a negative value, arithmetic-shifts by16 and narrows to signed32; for a negative original it decrements that block word when reconstructing it as signed64 times65536 does not equal the original fixed value. It then obtains the64-block zone index using signed32 negative reconstruction: for a negative block use signed division of block+1 by64 toward zero, followed by−1; nonnegative blocks use ordinary division by64. These are ordinary floor conversions in the representable block domain, with narrowing before the negative correction; they are not full64-bit floor functions outside that domain.

An object qualifies only when its signed32 kind+0 equals the requested kind, and, when the require-state input is true, its **byte+0x28 is nonzero**. Its fixed64 position is at+8/+0x10/+0x18. Subtract P's physical XYZ in64-bit word arithmetic, interpret each difference as signed64, convert each separately to float32, and multiply by exact float32 **1/65536** at0x42e510. Square each scaled float; add X² and Y², then add Z², rounding in float32 at each operation. Require the resulting squared distance **at most64.0**, word0x42800000 at0x42e5b4. This arithmetic/comparison is at **0x2b87ed–0x2b882e**. The first qualifying object yields true; exhausting the window yields false. No object render-resource, mesh-readiness, frustum or UI admission is added to this query.

### 5.2 Effective block at physical XYZ

Boat/glider tests pass P+0x10/+0x18/+0x20 directly to **0xeab0**, which converts each fixed64 component to the narrowed signed32 block coordinate with the negative correction above, then obtains a six-byte effective record through **0xe9a0 →0xeb90**. Only returned flags byte+5 bit0x20 is used here. It is not a collision-solidity or light-byte test, and is not evaluated at a drawn/interpolated creature position.

Missing loaded zones cause0xe9a0 to copy the shared fallback block at **0x54dcb0…0x54dcb5**; a loaded zone reaches its effective accessor. This selection slice does not initialize that fallback or override its flags. The general stored/effective-column rules remain [steam-zone-terrain-storage.md §5](steam-zone-terrain-storage.md#5-stored-versus-effective-reads-and-error-boundaries); the fallback's initialization/later external writers are not newly closed here. No guessed all-zero flags are supplied for a missing zone.

## 6. Ordinary Use read, copy and start/cancel

The vehicle fallback is **in0x8ae50 after0x8df69 calls0x98180**, not inside the prompt routine itself. It requires that prompt return0; the Ring selector G+0x310 is hidden; the ground-item target sentinel G+0x1e70 says none; and the general DialogWidget G+0x418 is hidden. A shown Ring selector suppresses this fallback even if it has no usable entries or there is no Use edge. The caller's normal local-control admission remains [frame-order §3](steam-gameplay-frame-order.md#3-local-control-caller-gate-and-target-lifetime).

If D+0x148 category is nonzero, copy it through0x9cd0 to the use-button icon's item at **(G+0x3e8)+0x1f8**. Display Cancel when current ability0x78 matches subtype1 or ability0x79 matches subtype2; otherwise obtain the category/subtype short name through0x16af30 and format it with0x16be20's default style. Clear the ground target as specified in use-prompt§4.3. This display path does not reread an inventory slot for original metadata.

On the Use rising edge, using **G+0x1a7b versus previous byte0x5515a3**, the exact cancellation conditions are:

- Current ability0x78 and selected subtype1.
- Current ability0x79 and selected subtype2.
- Current ability0x76 and selected subtype3, with the float32 movement-intent squared length **strictly below0.001**, word0x3a83126f at0x42e514. Square P+0x40/+0x44/+0x48 separately; add X² and Y², then Z², in float32. No square root or normalization is used.
- Current ability0x7a and selected category0.

Cancellation writes only ability0; it does not clear the selection, active item or old ability age. Otherwise, if the selected category is nonzero, copy the semantic selected value through **0x9cd0 to P+0x1d8**, set ability **0x78 for subtype1**, **0x79 for subtype2**, or **0x89 for any other subtype**, then set float32 age P+0x6c to positive zero. The copied value is the synthetic D record, including its zero region and other zero semantic fields, not an original matching inventory record.

This fallback does not consume an inventory quantity, write an equipped slot, change D's selection or perform a new availability/region/stun/whole-duration check. It has no additional health test at this cut; the actual outer/prompt gates remain relevant. It tests nonzero category rather than independently insisting on category0x18, although the proved producer emits0x18. Downstream ability0x89 effects use P+0x1d8 and have their separate [special-item contract](creature-abilities-ranged.md#16-using-a-special-item-0x89-0x2cce4c); they do not establish a saved D selection.

Pet riding0x76 is a **separate reached pet-key path** after this Use continuation. At **0x8eb71–0x8ec4f** it requires its edge/whole-duration admission, resolves P's pet ID+0x9d8, tests strict physical distance² below36 and availability(P,3), then enters0x76; when already0x76 its admitted key cancels instead. It does not source riding from D+0x148 or copy that D item into the held record. The complete pet/call continuation remains [player-control.md §4.8](player-control.md#48-use-quick-item-pet-and-riding). Other creature/mount0x80 routes are not redefined as selected-vehicle actions by this note. No ordinary producer of selected subtype3 is established here.

## 7. Direct original-item use and save/import boundaries

The shared item-use helper **0xb7740's category0x18 branch at0xb7a5b–0xb7b56**, reached by ordinary inventory dispatch, behaves differently from the generated-selection fallback. Require the stun timer not positive, and require the **original item's signed32 region pair+0x0c/+0x10 exactly equal P+0x150/+0x154**. Region failure requests sound0x34 and leaves the item/ability unchanged. On admission it first copies the original item to P+0x1d8. Subtype1 toggles0x78, subtype2 toggles0x79; toggling off leaves old age intact. Toggling on or every other subtype starts the corresponding0x78/0x79/0x89 with age0. It does not decrement source quantity, reset the dodge timer or assign D+0x148. See [slot-lifecycle §7.6](steam-inventory-slot-lifecycle.md#76-other-categories-dispatched-to-the-shared-item-use-helper).

Although Ring/Quick also call0xb7740, their established0x93e60 enumerator supplies category1 slots. Sharing the helper is not evidence that those paths choose category0x18 vehicles or write this D cache. Ordinary inventory use of an original regional vehicle can therefore leave a different held-item payload from the synthetic Use fallback while neither records a persistent D selection.

The original version11 character writer **0x149760** saves original inventory/equipment values and the established personal fields in [native-saves.md §3.3](native-saves.md#33-order-of-the-fields). It does **not** save D+0x148 or the active held item P+0x1d8 as an equipped-vehicle choice. The reader **0x147800** replaces CharacterData through fresh0x7ff20 calls at **0x14788b/0x147cf8**, and this embedded item starts semantically empty. Imported inventory items supply future availability-query inputs; no field in that reached sequence restores this cache from a selected tab/index or raw vehicle item. The similarly numbered P+0x148 saved experience word is a different owner/field, not D+0x148.

This bounds the ordinary startup, admitted World rebuild, prompt fallback and direct original-item-use paths. It does not supply an exhaustive external/mod writer, whole-CharacterData clone, alternate archive/network or malformed-input contract. Cached P region, flags, whole-duration/stat helpers, effective-block fallback state and downstream special-item ability effects retain their established producers or the explicit limits above. A skipped rebuild retains its actual previous cache; no invented first-inventory-item or active-held-item fallback is part of this native contract.
