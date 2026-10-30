# Loot: shops, buying, selling and identifying (0xb75d0, 0x2acf90, 0x2ac6e0, 0x2acbd0, 0x2acdd0, 0x94900, 0x965e0)

Behavioural specification of Steam Cube World, written from the decompiled executable, its
assembly and the binary's constants. Routines are cited by RVA (image base 0x140000000).
Offsets are byte offsets into the record named. Item fields, the item generators, the
"same item" test and the inventory add are those of `drops-and-items.md` (§0.1, §3) and
`worldgen-creatures.md` §4.5; prices are `stat-rules.md` §2.7. "rand" is the C runtime's
`rand()` (0..32767), shared with the whole game; "r mod n" is the ordinary remainder.

This file covers the stock a shopkeeper offers, what buying, selling and buying back do, and
the identify window that turns a type-15 item into gear. How the shop is opened (talking to
a townsperson of class 0x80, 0x81, 0x82 or 0x9c) is `use-prompt.md` §3.6.2.

---

## 1. Filling the shop window (0xb75d0, also 0x9de90)

Both routines do the same thing; 0xb75d0 runs when the shop opens and after every purchase or
sale, 0x9de90 when the shop window asks for a refresh.

1. Empty the shop list (controller +0x1870, an inventory-like list of tabs; see
   `worldgen-creatures.md` §4.5 for the record) and give it two tabs.
2. **Buy-back mode.** If the shop window's mode (window +0x2b4) is1,
   **tab1** becomes a newest-first copy of the controller's buy-back list
   (+0x21b0, §4.3); tab0 remains empty and nothing below is done. Native
   0xb75d0 appends to the24-byte tab header at collection begin+0x18.
3. Otherwise look up the shopkeeper by the id kept at controller +0x1e98. If it does not
   exist, the list stays empty. Otherwise, by the class kept at controller +0x1ea0:

   | Class | Stock routine | Arguments |
   |---|---|---|
   | 0x81 | weapons, 0x2acf90 (§2.2) | the world, the list, the shopkeeper creature |
   | 0x82 | armour, 0x2ac6e0 (§2.1) | the world, the list, the shopkeeper creature |
   | 0x9c | tier resources, 0x2acbd0 (§2.3) | the world, the list, the shopkeeper's id |
   | any other (0x80) | general goods, 0x2acdd0 (§2.4) | the world, the list, the shopkeeper's id |

   (The decompiler drops the third argument at three of these calls; the assembly at 0xb76c4
   passes the creature for 0x81/0x82 and the address of the id for the others.)
4. Make sure the list still has two tabs, then refresh the window (0x280a40, 0x280c80).

Everything a stock routine offers goes into **tab 0** (the add is called with tab 0, not
"pick one"). A quantity of −1 means "as many as wanted" (`worldgen-creatures.md` §4.5 step 6).
A quantity of 0 adds nothing.

### 1.1 The day, the shop key and what was bought

* **Day**: world +0x08.
* **Shop key**: the shopkeeper's id (creature +0x08, a 64-bit value). Seeds use its low 32
  bits; the bought list uses the whole value.
* **Bought count** (0x2a0c80, world, item, key): 0 when there is no local player (world
  +0x448). Otherwise look the key up in the local player's **bought list** (player data
  +0x138, an ordered map from shop key to a list of items, `native-saves.md` item 29); if
  absent, 0; otherwise the number of items in that list that are the same item
  (`drops-and-items.md` / `zone-object-interaction.md` §0.7 equality) as the one asked about.
  No rand.

### 1.2 The shop tier (0x2bb290)

The highest rarity the gear shops reach. 0 when there is no local player. Otherwise build the
region planner of the local player's current region (creature +0x150, +0x154; planner
0x115df0, `worldgen-creatures.md` §2) and count its role map entries (0x129190) whose role is
1 and whose place is in the player's **solved** mission set (player data +0xc8,
`conversation-script.md` §1.2). The tier is that count, at most 4. The planner draws no rand.

So a region's gear shops sell rarity 0 only until the first role-1 mission place of the
region is solved, and one more rarity per solved one, up to rarity 4.

---

## 2. The four stock routines

### 2.1 Armour shop (0x2ac6e0; class 0x82)

1. Empty the list. T = the shop tier (§1.2), taken **before** seeding.
2. srand(low 32 bits of the shopkeeper's id + day × 13 + 0x10ec), in 32-bit wrap-around
   arithmetic.
3. For rarity r = 0, 1, …, T (inclusive):
   1. One rand: n = (that rand) mod 4 + 1 (the game masks with 3; rand is never negative).
   2. n times:
      1. The armour generator (`drops-and-items.md` §3.2) with rarity r, class = the local
         player's class (creature +0x14c of world +0x448), plain flag 0.
      2. Its region (+0x0c, +0x10) = region at (truncated block of the shopkeeper's home x
         +0x1b8, truncated block of its home y +0x1c0) (0x2ada20; "truncated block" as in
         `drops-and-items.md` §0.3). No rand.
      3. One rand: its modifier (+0x08) = that whole value (replacing the generator's).
      4. Append it to a working list.
4. Sort the working list by rarity, ascending (0x299090, comparing byte +0x14). The list is
   generated in rarity order and has at most 20 entries, which the sort handles by insertion
   (stable), so **the order does not change**.
5. For each item in order: quantity = max(1 − bought count (§1.1), 0); add it to tab 0 with
   that quantity.

### 2.2 Weapon shop (0x2acf90; class 0x81)

As §2.1, with three differences: the seed adds 0x156b6 instead of 0x10ec; n = rand mod 2 + 1;
the weapon generator (§3.1 of `drops-and-items.md`, rarity r, the local player's class)
replaces the armour generator.

Draws per rarity: 1 (n) + n × (generator draws + 1). With class 1 the weapon generator makes
6 draws, class 2: 4, class 3: 5, class 4: 4 (`drops-and-items.md` §3.1 table); the armour
generator 10 for class 1 and 11 for classes 2–4 (§3.2).

### 2.3 Tier-resource shop (0x2acbd0; class 0x9c)

1. srand(day × 13 + low 32 bits of the shopkeeper's id). Empty the list.
2. Four items, each type 11 with every other field 0 except as stated, in this order:

   | Item | Quantity |
   |---|---|
   | subtype 0x20, rarity 1 | max(rand mod 5 + 3 − bought, 0) |
   | subtype 0x21, rarity 2 | max(rand mod 5 + 2 − bought, 0) |
   | subtype 0x22, rarity 3 | max(rand mod 5 + 1 − bought, 0) |
   | subtype 0x23, rarity 4 | max(rand mod 5 − bought, 0) |

   For each: one rand is drawn first, then the bought count is taken.

### 2.4 General goods (0x2acdd0; class 0x80 and any other class reaching 0xb75d0)

1. srand(day × 13 + low 32 bits of the shopkeeper's id). Empty the list.
2. In order, every other field 0:
   1. type 11, subtype 0x0c, material 0x19 — quantity −1 (unlimited);
   2. type 11, subtype 0x26 — unlimited;
   3. pet food: type 0x15, subtype by one rand mod 6: 0x22, 0x23, 0x13, 0x1a, 0x1e, 0x57 —
      quantity max(1 − bought, 0);
   4. type 1, subtype 9 — unlimited.

One rand in all. The pet-food subtypes are creature races (`drops-and-items.md` §3.3).

---

## 3. Prices

The buying price is the item price (0x109d30) and the selling price is 0x10a390, both in
`stat-rules.md` §2.7. Gold is the inventory's coin counter (creature +0xaac, which is
inventory +0xbc, `worldgen-creatures.md` §4.5).

---

## 4. Buying, selling and buying back (the click handler 0x94900)

Only the parts that create or move items are given. Messages go to the chat (0x26bf10) in the
interface's warning colour; sounds are interface sounds at the player (`sound.md`).

### 4.1 Buying from tab 0

When the player clicks an offered item I in shop mode 0:

1. If gold < price(I): "Not enough money." and sound 0x34; nothing else.
2. If the player may not carry more of I (0x50670): "You can't carry more of these items."
   and sound 0x34; nothing else.
3. Add one I to the player's inventory (tab by category, `worldgen-creatures.md` §4.5).
4. Gold −= price(I).
5. Append a copy of I to the bought list under this shop's key (player data +0x138; the
   entry for the key is created if absent).
6. Sound 0x3c; refresh the windows; refill the shop (§1), so the bought item's quantity drops
   by one (to 0 for gear, which then disappears); announce the item (0x9d6f0, a chat line);
   rebuild the crafting lists (0xb2180, `loot-crafting.md` §2).

The bought list is what keeps a purchase from coming back the same day. Because the stock is
re-rolled from the day, gear bought on one day does not affect later days' stock unless the
same generated item recurs (same modifier and region).

### 4.2 Selling

The older “item on the mouse cursor” label for this branch was misleading.
On left/right **down**, when no live inventory slot resolves,0x94260
can resolve a nonempty **equipped item under the pointer**, C. With
the shop visible, that equipped-item branch sells C. It does not read
or populate the retained cursor stack at creature+0xa08/+0xa0c.
The resolver order and exact source addresses are independently specified
in **steam-inventory-slot-lifecycle §6.6**; canonical tab-slot sales are
a separate source branch.

1. Gold += sell price(C).
2. Sound 0x3d at volume 0.75 (0x42e710).
3. Append (count 1, a copy of C) to the controller's buy-back list (+0x21b0). If the list now
   has more than 10 entries, remove its **first** (oldest) entry (0xa0720).
4. Refill the shop (§1). Clear that equipped item's type and subtype.

### 4.3 Buying back (mode 1)

In mode1 **tab1** lists the buy-back entries (§1 step2). A reached left/right
down resolves the displayed row's newest-first ordinal to the underlying
list node. One click attempts the **entire positive quantity in that
node**, not just one unit: for each attempt, check0x50670 carry admission;
if admitted and gold is at least the selling price0x10a390, insert one
through0x4e810, subtract the selling price and announce one through
0x9d6f0. A failed affordability check makes no purchase for that attempt
but does not break the loop or show the regular-stock money error.
A carry failure shows its warning/sound0x34 and exits the attempts.

The successful-purchase counter is signed32 integer, initialized0 at
0x95e46 and incremented at0x95ef1. After attempts, subtract **only that
counter** from the node quantity at0x95fb6. Unlink/free the node only
when its resulting quantity is exactly0; do not erase a partially bought
node. Nonpositive initial quantities make no attempts but still reach
this finalization. Once a matching node has been resolved, the final
sound0x3c and refresh path below run even if the successful counter is0.
There is no extra crafting rebuild call on this buyback finish.

### 4.4 Source precedence and exact refresh ordering

The reached inventory/shop branch is **button0 or1 down**, not mouse-up.
Native0x94900 first tries the live canonical inventory hover pair through
0x94600; if valid, it takes identification/sale/direct-use routing rather
than selecting a shop offer behind it. With no valid inventory slot,
try equipped item under pointer through0x94260. Only if that is null or
category0 does0x94750 resolve a shop offer. The ten equipped addresses
and resolver guards are steam-inventory-slot-lifecycle §6.6.

Shop resolver0x94750 requires shop shown, frame hit, and valid stored
hover tab/slot within Game+0x1870's current collection. It returns the
payload pointer; it adds no quantity>0 test, so an unlimited stock count−1
is not rejected. Ordinary rendering normally supplies occupied hover
rows. Do not introduce a new source count decrement to implement a
purchase: the bought-item list and shop rebuild govern stock depletion.

A canonical-slot sale is allowed only for source tabs **other than1
and5**, regardless of item category or a saved item's usual category tab.
It copies the item, decrements that live quantity by1, clears only
quantity/category/subtype below1, credits selling price, requests sound
0x3d at player position with volume0.75/pitch1, appends a **separate
quantity1** buyback node and clips oldest entries past ten. No full-item
removal helper or cursor transfer is used. Tabs1/5 instead request0x34
at the listener and do not sell. Equipped sales have no tab quantity to
decrement; their equipped type/subtype clear after rebuilding the shop.
Both append paths retain separate nodes rather than merging equal sales.
These sale branches add no class-usability, carry-cap or positive-price
gate. The canonical source's nonzero-count admission is the resolver's
existing test; a negative nonzero count has no infinite-stack exemption
from its direct decrement/below1 clear.

After the preceding data mutations, the direct native call order is:

| Action | Direct completion calls, in order |
|---|---|
|Regular stock purchase|sound0x3c at player; ordinary inventory0x280a40/0x280c80; shop rebuild0xb75d0; announce0x9d6f0; crafting rebuild0xb2180. Sources0x9617d–0x961be.|
|Canonical-slot sale|shop rebuild0xb75d0 at0x95251; crafting rebuild0xb2180 at0x95259. Its sound/buyback changes precede these. No explicit G+0x330 refresh pair in this finish.|
|Equipped-item sale|shop rebuild0xb75d0 at0x95d15, then equipped type/subtype clear. No explicit ordinary inventory or crafting rebuild in this finish.|
|Buyback finalization|sound0x3c at player at0x96011; shop rebuild0xb75d0 at0x96019; ordinary inventory0x280a40/0x280c80 at0x96025/0x96031.|

For regular stock, insertion and gold subtraction precede the bought-item
list append, which precedes the table's completion calls. The two
price/carry error branches do not execute that successful completion.
The common later left-widget and health/UI tail of0x94900 remains
steam-inventory-slot-lifecycle §7.7.

Nested refreshes matter:0xb75d0 always ends with the **shop** G+0x350
0x280a40/0x280c80 pair.0xb2180, when the local inventory has a nonempty
outer tab vector, ends its rebuild with the **crafting list** G+0x340
pair; this is not a widget-visibility condition. Each bound0x280a40 can
sort/compact the **local canonical inventory** again under slot-lifecycle
§4, even when that widget is hidden. Preserve the action's direct and
nested order rather than replacing it with one universal refresh or
one unconditional per-frame sort. None of these sales/purchases writes
the retained cursor stack.

Regular-stock code retains the resolved offer pointer across shop rebuild
for the following announcement. This pass specifies the native order;
it does not invent a portable announcement payload for allocator-dependent
pointer invalidation by that rebuild. A bounded oracle for that edge
would label the original offer, rebuilt collection/base and announced
item separately. No game was run for such an oracle.

---

## 5. Identifying a type-15 item (the window at controller +0x360; 0x965e0)

Talking to a townsperson of class 0x83 opens this window (`use-prompt.md` §3.6.2). The
player puts an item in its slot (window +0x1e8 = inventory tab, +0x1ec = index in the tab).

**Price** (0x277bc0): with an item in the slot, max(price(item) / 2, 1) in integer division;
0 without one. A type-15 item of rarity r costs 2^r (its base price is 0.2 × 5 = 1.0, so its
price is 2^r), hence max(2^(r−1), 1).

When all of these hold on a frame (0x965e0, with its second argument 0):

* the window is shown and the game's global input flag at 0x1405514d7 is set;
* the pointer is over the window's button (0x277d70);
* price ≤ gold, and the window's button state (+0x1f0) is 1 (pressed);
* the slot's tab and index are valid and the stack's quantity is not 0;
* the slotted item's type is 0x0f (0x277e30);

then:

1. Gold −= price.
2. Copy the slotted item (S). The stack's quantity −= 1; if it is now below 1, the stack's
   quantity, type byte and subtype become 0.
3. One rand: if even, the weapon generator (rarity = S's rarity, class −1); if odd, the armour
   generator (rarity = S's rarity, class −1, plain 0).
4. If the result's type is in the region set (`drops-and-items.md` §0.7), its region = S's
   region.
5. Add one of it to the player's inventory (tab by category). Refresh; play sound 0x31 at the
   listener (volume 1.0); announce it (0x9d6f0).

So a type-15 drop (`drops-and-items.md` §2.2 step 4: rarity = the dead creature's level, a
random seed) becomes a random weapon or armour piece **of any class**, of the same rarity,
bound to the region where the type-15 item dropped. The type-15 item's own seed is not used.

Draws: 1 + the generator's (16 for a weapon, 28 for armour with class −1).

When the window's close test (0x277cd0) passes, the window and the inventory window are
hidden.

---

## 6. Constants

| RVA / where | Value | Use |
|---|---|---|
| immediate in 0x2ac6e0 | 0x10ec | armour shop seed offset |
| immediate in 0x2acf90 | 0x156b6 | weapon shop seed offset |
| immediate | 13 | day factor in every shop seed |
| immediate | 3 / 1 | armour / weapon per-rarity count masks |
| 0x42e710 | 0.75 | volume of the selling sound |
| immediate | 10 | buy-back list length |
| immediate | 0x0f | the type the identify window takes |

Shop classes: 0x80 general, 0x81 weapons, 0x82 armour, 0x83 identify window, 0x9c tier
resources. Sounds: 0x34 refusal, 0x3c bought, 0x3d sold, 0x31 identified.

---

## 7. Verification

No shop has been recorded. Oracles to add to tools/ZoneExtractor (see `loot-overview.md` §5):

* **shop.request**: for each loaded townsperson of class 0x80, 0x81, 0x82 or 0x9c, call 0xb75d0's
  stock routine for today and the next N days (setting world +0x08 for each call, as the talk
  export does), with the game's rand recorded; dump the shopkeeper id, home, day, the tier
  (0x2bb290), the local player's class and bought list, and the resulting list (quantity and
  0xa0 bytes per stack). The port, fed the same draws, must give the same lists byte for byte
  (padding ignored).
* **identify**: hook the generator call inside 0x965e0 and record S, the price, gold before and
  after, every rand and the item added.

## 8. Open questions

1. **The general shop's goods** (type 11 subtypes 0x0c/material 0x19 and 0x26, type 1 subtype
   9) and the tier resources' names are not needed by the rules; the extracted item names give
   them.
2. **Buy-back details** are resolved by the native follow-up in §4.3/§4.4,
   including tab1, bulk attempts, partial node retention and refresh order.
3. **Identification input** is resolved in steam-identification-ui §6:
   0x5514d7 snapshots shown state at down, and button0 release commits
   under the live visibility/slot/footer gates. It is not a physical
   held-button latch or a generic “button was pressed” condition.
4. **Shop window mode values** other than 0 (stock) and 1 (buy-back) were not seen.
