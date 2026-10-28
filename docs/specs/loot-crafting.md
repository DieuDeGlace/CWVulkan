# Loot: crafting and weapon upgrades (0x2aec30, 0xb2180, 0xb2ee0, 0xb03d0, 0x8fbc0, 0x292650, 0x108b90)

Behavioural specification of Steam Cube World, written from the decompiled executable, its
assembly and the binary's constants. Routines are cited by RVA (image base 0x140000000).
Item fields are those of `drops-and-items.md` §0.1: type (+0x00), subtype (+0x04), modifier
(+0x08), region (+0x0c, +0x10), rarity (+0x14), material (+0x1c), the attribute entries
(+0x1d, four bytes each: x, y, z, material) and their count (+0x9d). The inventory record and
its add are `worldgen-creatures.md` §4.5; "same item" is the equality of
`zone-object-interaction.md` §0.7; the known-recipe list and the default recipes are
`frontend-characters.md` §5.6 and `native-saves.md` item 17. "rand" is the C runtime's
`rand()`; N, R and H are the integer hash, its random number and the place hash of
`worldgen-creatures.md` §0.1. "2H" is the two-handed weapon set of `stat-rules.md` §0.3
(subtypes 5, 6, 7, 8, 10, 11, 15, 16, 17, 18, 23; mask 0x878de0).

Covered: the recipe of an item (what it takes and where it is made), the crafting lists (the
known recipes plus the region's generated gear for each unlocked rank), the craft action and
its extra-yield roll, and placing upgrade cubes on a weapon.

---

## 1. The recipe of an item (0x2aec30)

Input: a recipe item Q (a 0xa0-byte item). Output: a **recipe** record: the result item
(0xa0 bytes), a **station** number (i32 at record +0xa0) and an **ingredient list** (record
+0xa8, a growing list of 0xa4-byte entries: an item, then an i32 quantity at entry +0xa0).
It always reports success. No rand.

1. Clear the record (result all zero, station 0, no ingredients).
2. If Q's type is 0x1d: the record stays **empty** (0x2af6b0 clears it again; the result's
   type is 0). Done.
3. The result is a copy of Q.
4. Ingredients by Q's type and subtype (tables below). Every ingredient is an item of
   **type 11** with modifier 0 and region 0; its subtype, material and rarity are given as
   (subtype, material, rarity) with omitted fields 0. r = Q's rarity, m = Q's material.
   Quantities are integers.
5. The **tier ingredient** (§1.7), for every Q whose type is neither 1 nor 0x1c.

### 1.1 Type 1 (consumables)

| Subtype | Ingredients, in order | Station |
|---|---|---|
| 1 | (0x16) ×1; then by r: r = 1 → (0x1c, rarity 1) ×1, r = 2 → (0x1d, rarity 2) ×1, r = 3 → (0x1e, rarity 3) ×1, r = 4 → (0x19, rarity 4) ×1, other r → none; then (0x1a) ×1 | 0 |
| 2 | (0x17) ×1, (0x0c, material 0x19) ×1 | 0 |
| 3 | (0x1d, rarity 1) ×1, (0x1a) ×1 | 0 |
| 4 | (0x14) ×4 | 9 |
| 7 | (0x1b) ×1 | 0 |
| 8 | (0x0f) ×1, (0x15) ×1 | 9 |
| 10 | (0x11) ×1 | 0 |
| 11 | (0x10) ×1 | 9 |
| 12 | (0x16) ×1, (0x19, rarity 1) ×1, (0x1a) ×1 | 0 |
| any other | none | 0 |

### 1.2 Type 3 (weapons)

| Subtype | Ingredients | Station |
|---|---|---|
| 0, 1, 2, 3, 13 | (10, material 1) × 5(r + 1) | 4 |
| 4 | (10, material 1) × 4(r + 1), then (9, material 0x1c) × 2(r + 1) | 4 |
| 5, 15, 16, 17 | (10, material 1) × 10(r + 1) | 4 |
| 6 | (10, material 2) × 10(r + 1), then (9, material 0x1b) × (r + 1) | 8 |
| 7 | (10, material 1) × 2(r + 1), then (10, material 2) × 8(r + 1), then (9, material 0x1b) × (r + 1) | 8 |
| 8, 10, 11 | (10, material 2) × 10(r + 1) | 8 |
| 12 | (10, material m) × 5(r + 1) | 4 |
| 9, 14, 18 and above | none | 0 |

### 1.3 Armour (types 4–7)

Let k = 10(r + 1) for type 4 (chest), 5(r + 1) for types 5 and 6 (hands, feet), 6(r + 1) for
type 7 (shoulder). Then:

* m = 1: (10, material 1) × k; station 4.
* any other m: (9, material m) × k; station 6.

### 1.4 Types 8 and 9 (neck, ring)

(10, material m) × 5(r + 1); station 0.

### 1.5 Type 11 (resources made from resources)

| Subtype | Ingredients | Station |
|---|---|---|
| 9 | by m: 0x1a → (6) ×1; 0x1b → (5, material 0x16) ×1; 0x1c → (0x0b, material 0x1c) ×1; any other m → none | 5 when an ingredient was added, else 0 |
| 10 | m = 2 → (1, material 2) ×1, station 7; any other m → (0, material m) ×1, station 3 | |
| 0x16 | (0x18) ×1 | 9 |
| 0x1a | (0x0c, material 0x19) ×1 | 1 |
| any other | none | 0 |

### 1.6 Type 0x1c

Every recipe of this type ends with (0x1a) ×1.

| Subtype | Ingredients | Station |
|---|---|---|
| 0 | (0x16) × 5(r + 1), (0x14) × (r + 1), (0x1a) ×1 | 0 |
| 1 | (0x0f) × 5(r + 1), (0x24) × 2(r + 1), (0x1a) ×1 | 0 |
| 2 | (0x0f) × 5(r + 1), (0x25) × 2(r + 1), (0x1a) ×1 | 0 |
| 3 | (0x13) × 5(r + 1), (0x1d) × 2(r + 1), (0x1a) ×1 | 0 |
| 4 | (0x26) ×2, (0x27) ×2, (0x28) ×3, (0x1a) ×1 | 0 |
| 5 | (0x26) ×1, (0x2b) ×3, (0x1a) ×1 | 9 |
| 6 | (0x29) ×2, (0x2a) ×2, (0x1a) ×1 | 0 |
| any other | none | 0 |

### 1.7 The tier ingredient

For a result whose type is not 1 and not 0x1c and whose rarity t is 1, 2, 3 or 4: one more
ingredient (0x1f + t, rarity t) — subtypes 0x20 to 0x23, the items of the tier-resource shop
(`loot-shops.md` §2.3) and of the reward's tier resources (`drops-and-items.md` §4 step 4).
Quantity 4 when the result is type 4, or type 3 with a 2H subtype; otherwise 2. Rarity 0 (or
above 4) adds nothing.

The station numbers are only compared with the workstation the player stands at (not traced
here, open question 3); the map's crafting markers are `map-markers.md` §1.3.2.

---

## 2. The crafting lists (0xb2180)

Rebuilt whenever the inventory changes (after buying, crafting, picking up, loading). Only
when the local player's inventory has tabs.

### 2.1 Known recipes

For each item K in the player's known-recipe list (player data +0x18), in list order: take
its recipe (§1) and append it to a tab by K's type and subtype:

| K | Tab |
|---|---|
| type 3 | 0 (weapons) |
| type 1, subtype 1, 2, 3 or 12 | 4 |
| type 1, any other subtype | 3 |
| type 0x1c, subtype 4, 5 or 6 | 3 |
| type 0x1c, any other subtype | 4 |
| type 11 | 5 |
| any other type | not listed |

(Known recipes of types 4–9 are never listed; gear of those types reaches the lists only
through §2.2.)

### 2.2 Rank gear of the region

Let (rx, ry) be the local player's current region (creature +0x150, +0x154). For rank
k = 0, 1, 2, 3, 4:

1. **Unlocked?** (0x2b4260 → 0x128400, on the region planner of (rx, ry)): rank 0 always;
   rank k ≥ 1 when the planner's role map has a place whose role is k + 1 (roles 2–5, the
   crafting-book places of `worldgen-creatures.md` §2.3) that is in the player's solved set
   (player data +0xc8). False when there is no local player. Skip the rank if not unlocked.
2. srand(R(H(rx, ry) + 0xf5f9) + k) — i.e. srand of 100000 − N(rx + 57·ry + 0xf5f9) div 21474
   + k. (The routine also seeds once before the loop with k = 0; that seed is always replaced
   before a draw.)
3. A base item: type 3, rarity k, region (rx, ry), everything else 0. The items below set
   subtype, material and modifier on it in turn; "make" means: modifier = one rand, then the
   recipe of the item (§1) is appended to the named tab.
4. **Weapons**, tab 0, by the player's class (creature +0x14c):
   * class 1: material 1; subtype = rand mod 3, make; subtype 13, make; subtype = rand mod 3
     + 15, make.
   * class 2: material 2; subtype 6, make; 7, make; 8, make.
   * class 3: material 2; subtype 11, make; 10, make; subtype 12 with material 11 when one
     rand is even, 12 when odd, make.
   * class 4: material 1; subtype 3, make; 4, make; 5, make.
   (A subtype draw comes before its modifier draw.)
5. **Armour**, tab 1: material by class (1 → 1, 2 → 0x1b, 3 → 0x1a, 4 → 0x1c; any other class
   keeps the weapon's material), subtype 0; type 4, make; type 7, make; type 5, make; type 6,
   make.
6. **Jewellery**, tab 2: type 8 with material 11 when one rand is even, 12 when odd, make;
   type 9 the same.

Draws per unlocked rank: class 1: 2 + 1 + 2 (weapons) + 4 + 4 = 13; classes 2 and 4: 3 + 4 + 4
= 11; class 3: 4 + 4 + 4 = 12.

So each region offers, per unlocked rank, three weapons of the player's class, four armour
pieces and two jewellery pieces, fixed by the region (the same every day and for every
character of that class) and bound to it.

### 2.3 Filling the tabs (0xb2ee0)

Tabs 0–5 of the crafting window's list (controller +0x17a8) get the six lists in order. For
each recipe: **makeable** = the smallest, over its ingredients, of (the total quantity of
stacks in the whole inventory that are the same item as the ingredient) div (the ingredient's
quantity); 0 for a recipe with no ingredients. The tab entry is (makeable, result). If the
entry is the one selected in the list widget (controller +0x340: tab +0x278, index +0x27c),
the recipe (result, station, ingredients) and its (tab, index) are copied to the crafting
button widget (controller +0x348: +0x1f8 result, +0x298 station, +0x2a0 ingredients, +0x2b8
tab and index).

Then the window's list is made to have 7 tabs, **tab 6 is emptied**, and the two lookup maps
used for tab 6 (controller +0x1938: tab-6 index → equipped item; +0x1948: tab-6 index →
inventory tab and slot) are cleared. Nothing here fills tab 6 (open question 2).

---

## 3. Crafting (0xb03d0 each frame, 0x8fbc0 per item)

### 3.1 Holding the button (0xb03d0, given the frame time dt in ms)

State on the controller: progress (+0x20cc, float), speed (+0x20d0, float), the recipe being
made (+0x20d8 result, +0x2178 station, +0x2180 ingredients) and its (tab, index) (+0x2198,
+0x219c).

1. If the crafting window is hidden: progress = −1.0, the recipe is cleared and (tab, index)
   = (−1, −1). Done.
2. Otherwise, if the button widget is not ready (widget +0x38c is 0) or the input's held flag
   (controller +0x20 → +0x1a8, bit 0) is clear: speed = 0.001 (immediate 0x3a83126f).
   Otherwise, if progress < 0: progress = 0; copy the selected recipe and its (tab, index)
   from the button widget; if the result's type is in the region set
   (`drops-and-items.md` §0.7), its region = the player's current region (+0x150, +0x154).
3. If progress ≥ 0: progress += float(dt) × speed. When progress reaches 1.0: speed ×= 1.2
   (0x42e718), progress = −1.0, speed is capped at 0.005 (0x42e518), and the item is made
   (§3.2).
4. The widget shows the progress (0x26e940).

So the first item takes 1000 ms of holding; each further item while held is 1.2 times faster,
down to 200 ms. Releasing resets the speed but (as written) does not cancel a started item;
see open question 4.

### 3.2 Making one (0x8fbc0)

Let C be the recipe's result (a copy) and T the target, if any:

1. **Target in place.** If the recipe's tab is 6, the window's list has more than 6 tabs and
   the index is valid in tab 6: the target is the item that tab-6 index refers to, first
   through the equipped-item map (+0x1938), else through the inventory map (+0x1948: tab and
   slot of the player's inventory), and only if it is the same item as tab 6's entry. With a
   target, step 2 is skipped.
2. **Room.** If the player may not carry more of C (0x50670): chat "You can't carry more of
   these items." and sound 0x34 at the player; done (nothing consumed).
3. **Consume.** For each ingredient (quantity q), walk the inventory tabs in order and their
   stacks in order. For a stack that is the same item as the ingredient:
   * if q is smaller than the stack's quantity: the stack loses q, and the walk continues
     with the **next tab**;
   * otherwise the stack is emptied (quantity 0, item cleared) and the walk continues with the
     next stack.
   q is **never reduced** by an emptied stack (the game subtracts the stack's quantity after
   setting it to 0). With the usual single stack per resource this takes exactly q; with
   several equal stacks it takes every stack up to the first one larger than q in each tab.
   No check here that enough was held (the button is only ready when it is, open question 3).
4. Sound 0x5c at the listener, volume 1.0.
5. **Extra yield.** If C's type is 11 and its subtype is not 0x1a: one rand; if it is a
   multiple of 5, extra = 1, then one more rand; if a multiple of 5, extra = 2, then one more;
   if a multiple of 5, extra = 3. (1, 2 or 3 draws; each further step only after a success.)
   When extra ≥ 1, sound 0x5d at the listener. Otherwise extra = 0 and no draw.
6. **Deliver.** Without a target: add extra + 1 of C to the inventory (tab by category) and
   announce it (0x9d6f0). With a target: the target is overwritten by C field by field
   (type, subtype, modifier, region, rarity, +0x18, material, all attribute entries and their
   count); extra is ignored.
7. Refresh the windows and rebuild the crafting lists (§2).

C's region was set in §3.1 step 2, so crafted gear is bound to the region the player stood in
when the item started, not to the recipe's region. For rank gear (§2.2) both are the same
unless the player crossed into another region while crafting.

---

## 4. Upgrade cubes on a weapon (0x292650, 0x108b90, 0x10a0e0)

The item record's attribute entries are cubes placed on a weapon's voxel grid: (x, y, z) and a
material each. Their number u is what the weapon damage reads as its upgrade count
(`stat-rules.md` §1.4: the exponent gains u × 0.5 / 16, or / 32 for 2H).

**Place** (0x108b90: item, x, y, z, material): if an entry with the same (x, y, z) exists, its
material becomes the new one. Otherwise, if the count is below the cap (32 for a type-3 item
with a 2H subtype, else 16), append (x, y, z, material) and add one to the count.

**Remove** (0x10a0e0: item, x, y, z): the first entry with that (x, y, z) is removed, later
entries move down by one, the count drops by one.

**The upgrade widget** (0x292650, run from the click handler 0x94900): it edits the player's
right weapon (widget +0x2c0 = 0) or left weapon (+0x2c0 = 1), when that slot is not empty.
The widget keeps a palette (+0x1f8: 0xa4-byte entries of quantity and item), whose items are
type 11 subtype 10 with a material (the material cubes, filled by the widget's drawing routine
0x292990 from the player's inventory), a selected palette index (+0x210), the voxel under the
pointer (+0x2c8, +0x2cc, +0x2d0) and a carried cube (+0x37c flag, +0x380..+0x388 position,
+0x38c material).

Each call, in this order:

* **No voxel or no weapon** (the widget's flag +0x2d4 is 0 — read here as "no voxel of the
  weapon is under the pointer" — or the slot is empty): if a cube is carried and the UI's
  query 0x33f1e0 returns none (presumably: the pointer is over no other widget), the carried
  cube is removed from the weapon. Then the palette index becomes −1 and nothing is carried.
  Done.
* **Placing from the palette** (the palette index is valid and the weapon has fewer than 32
  entries): place a cube of the palette entry's material at the pointed voxel, then take one
  of that palette item from the inventory: the first stack (tab order, slot order) that is the
  same item loses one; at 0 its quantity, type and subtype become 0. (With 32 or more entries
  nothing happens.)
* **Moving a carried cube** (no valid palette index, a cube carried): remove it from its old
  position and place it at the pointed voxel with its material; nothing carried.
* **Picking a placed cube up** (no valid palette index, nothing carried): if an entry sits at
  the pointed voxel, remember it as carried (its position and material). Nothing changes yet.

Placing a cube takes the cube item; moving costs nothing; dropping a carried cube outside the
weapon destroys it (its item is not returned). What calls this routine per frame or per click
inside 0x94900 was not separated (open question 5).

---

## 5. Constants

| RVA / where | Value | Use |
|---|---|---|
| immediate 0x3a83126f | 0.001 | starting craft speed (progress per ms) |
| 0x42e718 | 1.2 | speed factor after each item |
| 0x42e518 | 0.005 | speed cap (200 ms per item) |
| 0x4ac980 | 1.0 | progress needed |
| immediate | 0xf5f9 | rank-gear seed offset |
| immediate | 5 | extra-yield odds (1 in 5, up to three times) |
| immediate 0x878de0 | 2H set | tier quantity 4, cube cap 32 |
| immediate | 16 / 32 | cube cap, one-handed / 2H |

Sounds: 0x5c craft, 0x5d extra yield, 0x34 refusal (`sound.md`).

---

## 6. Verification

Nothing here has a recording. Proposed ZoneExtractor requests (`loot-overview.md` §5):

* **recipes.request** — call 0x2aec30 on scripted items: every type 0..0x1d × subtypes 0..47 ×
  rarity 0..5 × materials {0, 1, 2, 0x16, 0x19, 0x1a, 0x1b, 0x1c, 11, 12, 0x1d}; write the item
  and the record (result, station, every ingredient with its quantity). No rand is involved;
  the table above must match entry for entry.
* **craftlists.request** — call 0xb2180 for the loaded character and dump tabs 0–6 (makeable
  and the result of each entry), the player's region, class, solved set and known recipes,
  and every rand drawn. Repeat in a few regions and for each class.
* **craft recording** — hook 0x8fbc0: the recipe, the inventory before and after, the rand
  values, and what was added (or which target was overwritten).

## 7. Open questions

1. **Recipe sources.** Which items reach the known-recipe list besides the defaults (crafting
   books of roles 2–5 are carried by quest givers and guards, `worldgen-creatures-roles.md`;
   how reading or using one adds recipes was not traced).
2. **Tab 6** of the crafting window and its two maps (the in-place target of §3.2 step 1): no
   routine seen here fills them.
3. **When the button is ready** (widget +0x38c): presumably makeable > 0 and the right station
   nearby; not traced. Station numbers 0–9 are only compared, not named.
4. **Releasing the button** sets the speed back to 0.001 but leaves a started item's progress
   running, so it completes. Whether another routine resets the progress on release was not
   checked.
5. **The palette materials** of the upgrade widget (which materials 0x292990 lists, and in
   which order) and the widget's 32-entry check versus the 16-entry cap of one-handed weapons
   (the place routine refuses silently, but the cube item was already taken) — confirm with a
   recording.
6. **Known recipes of types 4–9** are skipped by §2.1. Whether the game ever stores such
   recipes was not checked.
