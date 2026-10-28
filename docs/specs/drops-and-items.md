# Drops and item generation (0x2a6860, 0x2a5b70, 0x2d15d0, 0x2d0680, 0x2d0fc0, 0x2a7ca0)

Behavioural specification of Steam Cube World, written from the decompiled executable, its
assembly and the binary's constants. Routines are cited by RVA (image base 0x140000000);
constants are cited by the RVA of the value in the binary. Offsets are byte offsets into the
record named. World positions are signed 64-bit integers in 1/65536 of a block. "rand" is the
C runtime's `rand()` (0..32767), shared with the rest of the game; every draw is listed, in
order, because a port must consume exactly the same values. "r mod n" is the ordinary
remainder (rand is never negative). Vocabulary for creatures, the world, zones and blocks
follows `creature-hit.md`, `creature-upkeep.md`, `use-prompt.md` and `conversation-script.md`.

Six routines are covered:

* **Death drops, 0x2a6860 (world, creature).** Decides everything a dying creature leaves:
  a worn item, its pet-slot item, a generated weapon or armour piece, one "fixed" item
  (pet food or one of four set items), a type-15 item, coins, race resources and its whole
  inventory. Each chosen item is handed to the drop layer.
* **The drop layer, 0x2a5b70 (world, item, position, angle, scale, count, creature, owner).**
  Stamps the item with a region, may reserve it for one player (raising its rarity), finds
  the ground under the position and appends a 0xe0-byte ground item to the zone there.
* **The weapon generator, 0x2d15d0 (world, out item, rarity, class).**
* **The armour generator, 0x2d0680 (world, out item, rarity, class, plain flag).**
* **The pet-food generator, 0x2d0fc0 (world, out item, key)**, which reads the world's two
  special lists (world +0x360 and +0x378).
* **The reward routine, 0x2a7ca0 (world, position, tier).** One generated weapon or armour
  piece per nearby player, coins, and tier-bound resources.

§5 shows how they are called; §6 lists the constants.

---

## 0. Records, helpers and conventions

### 0.1 The item record (0xa0 bytes)

| Offset | Type | Meaning |
|---|---|---|
| +0x00 | u8 | **type** (0 = empty). Types met here: 1 consumable, 2 (stands for another item, see §0.5), 3 weapon, 4 chest, 5 hands, 6 feet, 7 shoulder, 8 neck, 9 ring, 11 resource, 12 coin, 15, 21 pet food, 23, 24 |
| +0x01..+0x03 | — | padding, never copied |
| +0x04 | i32 | **subtype** |
| +0x08 | u32 | **seed** (a whole rand value for generated items) |
| +0x0c, +0x10 | i32 ×2 | **region** x, y |
| +0x14 | u8 | **rarity** (0..4 in normal play; see §1.3 for values above 4) |
| +0x15..+0x17 | — | padding, never copied |
| +0x18 | i32 | for type 2 only: the type of the item it stands for (§0.5); 0 otherwise |
| +0x1c | u8 | **material** |
| +0x1d..+0x9c | 32 × 4 bytes | attribute entries (the spirit entries of `creature-hit.md`; their material byte is the fourth byte of each, i.e. +0x20 + 4k). Every routine here leaves them zero or copies them unchanged |
| +0x9d | u8 | number of attribute entries |
| +0x9e, +0x9f | — | padding, never copied |

**Copying an item** (0x35400, 0x9cd0, and the inline copies in 0x2a5b70 and 0x2a7ca0) copies
the fields above one by one and never the padding bytes. **Clearing an item** in these
routines means: type, subtype, seed, region, rarity, +0x18, material, all attribute bytes
and the count set to 0 (padding untouched).

### 0.2 Creature fields used

| Offset | Type | Use here |
|---|---|---|
| +0x10, +0x18, +0x20 | i64 ×3 | position |
| +0x60 | u8 | hostility kind: 0 player, 1 hostile, 3 friendly townsperson, 7 (a second hostile kind), others |
| +0x64 | i32 | race |
| +0x80 | u32 | appearance flags: bit 3 (0x8), bit 4 (0x10), bit 9 (0x200), bit 11 (0x800), bit 18 (0x40000) are read |
| +0x144 | i32 | level |
| +0x14c | u8 | class (1 warrior, 2 ranger, 3 mage, 4 rogue) |
| +0x1b0, +0x1b4 | i32 ×2 | a zone pair (the flight destination of `map-markers.md`; 0x7fffffff when unused). Copied verbatim into ground items |
| +0x1b8, +0x1c0 | i64 ×2 | origin (home) x, y |
| +0x318 | item | neck slot |
| +0x3b8 | item | chest slot |
| +0x458 | item | feet slot |
| +0x4f8 | item | hands slot |
| +0x598 | item | shoulder slot |
| +0x638, +0x6d8 | item ×2 | left and right weapon slots |
| +0x8b8 | item | pet slot |
| +0x968 | i64 | for players: the id stamped on items reserved for them |
| +0x9f0 .. +0x9f8 | sequence | inventory tabs; each tab a sequence of 0xa4-byte slots (i32 count, then an item) |
| +0xb88 | ordered map i64 → f32 | aggro (creature id → anger), iterated in ascending id order |

The two ring slots (+0x778, +0x818) are never dropped.

### 0.3 World fields used

| Offset | Type | Use |
|---|---|---|
| +0x08 | i32 | the day; copied into every ground item |
| +0x40 | list | all creatures, in world order (list node +0x10 holds the creature) |
| +0x190 | ordered set of zone-object addresses | zones whose ground items changed |
| +0x360 .. +0x368 | sequence of i32 | the **pet list** (creature kinds that can be pets, also the subtypes of their pet food) |
| +0x378 | ordered map i32 → sequence of i32 | the **keyed pet lists** (node key i32 at +0x20, the sequence at +0x28..+0x30) |
| +0x450 | ordered map i64 → creature | creature by id |
| +0x460 | hash map | zones by zone key (zone x low 32 bits, zone y high 32 bits); the zone object is the value's +0x18 |

A zone object keeps its **ground items** as a growing sequence of 0xe0-byte records at
+0x30 (`use-prompt.md` §0.4).

Coordinates: a position's **block** is floor(position / 65536), exact also for negatives; a
block's **zone** is floor(block / 64). Where this document says **truncated block**, the
value is position / 65536 rounded toward zero instead (it differs from the block for
negative positions that are not whole blocks). The **place cell** of a block is
floor(block / 1024).

World queries used and already described elsewhere:

* **region at (x, y)** — 0x2ada20 (`gamegen::regionAt`), deterministic, no rand.
* **big cell (cx, cy)** — 0x36160 (`gamegen::bigCell`), and **cell point (cx, cy, big)** —
  0x362a0 (`gamegen::cellPoint`): the landmark's block point in a place cell.
* **block at (bx, by, bz)** — 0xeab0/0xe9a0 (`creature-upkeep.md` §4.7). A block is six bytes;
  byte 4 is its type. A block is **open** when its type is 0 or 2 and **solid** otherwise
  (the game tests type with bit 1 masked off against zero).

### 0.4 The ground item record (0xe0 bytes)

Written by 0x2a5b70 (§1) and appended to the zone's list at zone +0x30.

| Offset | Type | Value written by 0x2a5b70 |
|---|---|---|
| +0x00 | item (0xa0) | the (possibly re-regioned, rarity-raised) item |
| +0xa0, +0xa8 | i64 ×2 | position x, y as passed |
| +0xb0 | i64 | ground height: (top open block's z) × 65536, see §1 step 7 |
| +0xb8, +0xbc | i32 ×2 | the dropping creature's +0x1b0 and +0x1b4; both 0xffffffff when there is no creature |
| +0xc0 | f32 | angle in degrees: a fresh rand × 360.0 / 32767.0 (the angle argument is overwritten) |
| +0xc4 | u8 | 2 when the item type is 23 or 24 (no automatic pick-up), else 0 |
| +0xc5..+0xc7 | — | never written (stale bytes) |
| +0xc8 | f32 | pick-up delay: float(rand mod 300 + 500), i.e. 500..799 |
| +0xcc | i32 | 0 |
| +0xd0 | i32 | world +0x08 (the day) |
| +0xd4 | i32 | the count argument (always 1 from these callers) |
| +0xd8 | i64 | owner: the reserving player's id, or 0 for anyone |

The record is copied into the zone list field by field (0x3aee0, padding and +0xc5..+0xc7
not copied; new list memory is not cleared, so those bytes are unspecified).

### 0.5 Who can use an item (0x1094d0, item, class)

Returns true or false:

1. **Type 2:** make a copy of the item whose type is its +0x18 and whose +0x18 is 0 (all
   other fields equal). If that type is 2, answer true; otherwise answer for the copy, with
   the same class.
2. **Type 3 (weapon), by subtype:**

   | Subtypes | Usable by |
   |---|---|
   | 0, 1, 2, 13, 15, 16, 17 | class 1 |
   | 6, 7, 8, 14 | class 2 |
   | 10, 11, 12 | class 3 |
   | 3, 4, 5 | class 4 |
   | 9, and every subtype ≥ 18 | anyone |

   (Jump table at 0x1096cc.)
3. **Types 4..7 (chest, hands, feet, shoulder), by material:**

   | Materials | Usable by |
   |---|---|
   | 1, 4, 5, 11, 19, 23, 29 | class 1 |
   | 20, 27, 31, 33 | class 2 |
   | 24, 26, 32, 35 | class 3 |
   | 28, 30, 34 | class 4 |
   | any other | anyone |

   (Index table 0x1096a8 into the jump table 0x109694, indexed by material − 1, 1..35.)
4. **Any other type:** true.

### 0.6 The scatter (used before every lay in §2 and §4)

Given a centre (x, y, z), three rand draws in this order:

1. r1 = rand; dy = truncate((1.0 − (float(r1) + float(r1)) / 32767.0) × 65536.0).
2. r2 = rand; dx = the same with r2.
3. r3 = rand; angle = float(r3) × 360.0 / 32767.0.

All arithmetic is single-precision float: the rand value is converted to float, doubled by
adding it to itself, divided by 32767.0, subtracted from 1.0, multiplied by 65536.0, then
truncated toward zero to a 64-bit integer. The position passed on is (x + dx, y + dy, z) —
note that the **first** draw moves y and the second moves x. The angle is divided after
multiplying. The offsets span roughly ±1 block.

### 0.7 Selection sets

* **Region set** (bit mask 0x454083fc, types below 31): types 2, 3, 4, 5, 6, 7, 8, 9, 15,
  22, 24, 26, 30.
* **Reservation set** (bit mask 0x440d3fc, types up to 26): types 2, 3, 4, 5, 6, 7, 8, 9, 12,
  14, 15, 22, 26; plus type 1 with subtype 13.
* **Rarity-raise types**: 3..9 and 26.

---

## 1. Laying a drop, 0x2a5b70

Arguments: world W, item I (read only; a working copy is made), position P (three i64),
angle A (f32), scale S (f32), count N (i32), dropping creature C (may be none), owner O (i64,
may be 0).

**S is never read.** Callers pass 1.0, 0.75, 0.8 or 3.0 (see §2, §4); a port must still
record the value if it is checked against the recording (§7), but it has no effect.

1. Make the working copy of I (§0.1 copy) and start the ground item: copy of P in
   +0xa0..+0xb0, +0xb8/+0xbc = 0xffffffff, +0xc0 = A, +0xcc = 0, +0xd8 (owner) = 0.
2. **Pick-up delay.** One rand: +0xc8 = float(rand mod 300 + 500). +0xd0 = W's day.
   If C exists, +0xb8 = C +0x1b0 and +0xbc = C +0x1b4.
3. **Region stamp.** Only when the item type is in the region set, C exists and C's
   hostility kind is not 0:
   1. Read the block at C's position (block coordinates of +0x10, +0x18, +0x20).
   2. If bit 0x20 of that block's byte 5 is **clear**: the point is (truncated block of C's
      origin x +0x1b8, truncated block of C's origin y +0x1c0).
   3. If it is **set**: take the truncated blocks of C's current x and y, their place cell
      (cx, cy) = floor(each / 1024), big = big cell (cx, cy), and the point is
      cell point (cx, cy, big).
   4. The item's region (+0x0c, +0x10) becomes region at (point).
   No rand.
4. **No-pick-up byte.** +0xd4 = N. +0xc4 = 2 if the item type is 23 or 24, else 0.
5. **Reservation.** Skipped entirely (go to step 6) when C exists and its hostility kind is 0
   (a player dropping something), or when the item is not in the reservation set.
   Otherwise:
   1. **Candidates.**
      * If C exists and its hostility kind is 1: walk C's aggro map in ascending id order
        (every entry, whatever its anger). For each id, look it up in W +0x450; keep the
        creature if it exists, its hostility kind is 0, and it can use the item (§0.5 with
        its class +0x14c). No distance test.
      * Otherwise (no C, or any kind other than 0 and 1): walk W's creature list in order;
        keep each creature whose hostility kind is 0, that can use the item, and whose
        squared distance from P is below 65536.0 (0x42e5f8), i.e. within 256 blocks. The
        distance is computed per axis as float(int64 difference) × (1/65536) (0x42e510),
        squared and summed in float.
   2. **Fallback.** If no candidate was kept, walk W's creature list again and keep every
      creature with hostility kind 0 (no usability or distance test).
   3. If there is still no candidate, **stop: nothing is laid** (no further rand, no ground
      item).
   4. One rand: the chosen player is candidate[rand mod count] (count of the candidate list,
      in the order built).
   5. If the chosen player can use the item (§0.5 again; this matters only after the
      fallback):
      * If the item type is a rarity-raise type: draw rand; if it is a multiple of 20,
        rarity +1 and draw again; up to four draws, stopping at the first that is not a
        multiple of 20 (so 1..4 draws, raising by 0..4). Rarity is a byte and wraps at 256.
        Then, if the rarity is greater than 3, set it to 4. **This clamp also lowers any
        rarity above 4 to 4**, whatever the draws were.
      * Owner (+0xd8) = O if O is not 0, else the chosen player's +0x968.
      If the chosen player cannot use it, nothing changes and the owner stays 0.
6. **Zone.** Find the zone of P's x, y (block of P, then zone of the block) in W +0x460. If
   the zone is absent, or its zone object is null, **stop: nothing is laid**, and no further
   rand is drawn.
7. **Lay.**
   1. Copy the working item into the ground item.
   2. One rand: +0xc0 = float(rand) × 360.0 / 32767.0 (replacing A).
   3. **Ground.** Let bx, by be P's x, y blocks and bz = P's z block. Repeatedly read the
      block at (bx, by, bz); while it is open, decrease bz by 1. When a solid block is
      found, +0xb0 = (bz + 1) × 65536. (If P's own block is already solid, the item sits
      on top of that block and does not climb further.)
   4. Append the ground item to the zone object's list (+0x30).
   5. Insert the zone object into W +0x190.

**Rand draws of one call:** 1 (delay) + [reservation: 1 pick + 1..4 rarity draws when
raised-type and usable] + [1 angle when laid]. A call that stops in step 5.3 draws only 1; a
call that stops in step 6 draws no angle.

---

## 2. Death drops, 0x2a6860 (world W, dead creature D)

Every item below is laid with 0x2a5b70(W, item, position, angle, scale, count 1, creature D,
owner 0), position and angle from the scatter (§0.6) centred on D's position (z unchanged).
The scale named in each step is passed through but unused.

### 2.1 Order of operations

1. **No drops** when D's hostility kind is 0 or 3, or D's flags have bit 11 (0x800). Return.
2. **Worn item.** Build the candidate list, in this order, each only if its type byte is
   not 0:
   * left weapon (+0x638), if its subtype is not 22 or 23;
   * right weapon (+0x6d8), if its subtype is not 22 or 23;
   * chest (+0x3b8), if its subtype is not 1;
   * shoulder (+0x598), hands (+0x4f8), feet (+0x458), neck (+0x318).

   If the list is not empty: one rand; if it is a multiple of 16, **or** D's flags have
   bit 9 (0x200) (the rand is drawn first either way), then one rand picks
   candidate[rand mod count]; copy it; scatter; lay with scale 1.0.
3. **Pet slot.** If +0x8b8's type is not 0: one rand; if it is a multiple of 50, scatter and
   lay the pet-slot item itself with scale 1.0. (No rand when the slot is empty.)
4. **Generated item, fixed item, type-15 item, coins.** Only for hostility kinds 1 and 7
   (§2.2). Other kinds skip to step 5.
5. **Race resources** (§2.3), for every kind that reached step 2.
6. **Inventory**, kinds 1 and 7 only: for each tab in order, for each slot in order, if the
   slot's item type is not 0 and its count is greater than 0: repeat count times — unless
   the item is type 1 with subtype 1, in which case nothing is done and no rand is drawn —
   scatter and lay the slot's item (count 1 each time) with scale 0.75.

### 2.2 Kinds 1 and 7

1. **Gate for the generated item.**
   * Kind 1: one rand; take the generated item (step 2) if it is a multiple of 20, or if
     D's flags have bit 9 or bit 18 (mask 0x40200). Otherwise go to step 3.
   * Kind 7: one rand; take the generated item if it is a multiple of 20; otherwise go to
     step 3. (Flags do not force it.)
2. **Generated item.** Clear an item. One rand: if even, the item = weapon generator
   (W, rarity = low byte of D's level +0x144, class −1); if odd, the item = armour
   generator (W, the same rarity, class −1, plain flag 0). Then, if the rarity is below 4 and
   D's flags have bit 9 or bit 18, rarity +1. Scatter; lay with scale 1.0. Continue with
   step 3.
3. **Fixed item.** One rand; if it is a multiple of 8: clear an item, one rand, and by
   rand mod 5:

   | r mod 5 | Item |
   |---|---|
   | 0 | pet food: pet-food generator (W, key 0) (§3.3) |
   | 1 | type 11, subtype 12, material 25 |
   | 2 | type 1, subtype 4 |
   | 3 | type 1, subtype 7 |
   | 4 | type 1, subtype 9 |

   All other fields 0. Scatter; lay with scale 1.0.
4. **Type-15 item.** Only when (kind is 1 and D's flags have bit 3 (0x8)) or D's flags have
   bit 4 (0x10) — tested before any draw. Then one rand; if it is a multiple of 20: an item
   of type 15, subtype 0, region 0, +0x18 0, material 0, no attributes; rarity = the low byte
   of D's level if the level is greater than 1, else 1; then one rand for the seed (+0x08 =
   the whole rand value). Scatter; lay with scale 0.75.
5. **Coins.** Kind 1 only.
   1. base = (float(level) − 1.0) × 0.1 + 1.0 (0x61000 with 0x42e528, 0x4ac980).
   2. One rand: f = base × (float(rand) / 32767.0 + 1.0).
   3. f × 10.0 if flags bit 9; then f × 20.0 if flags bit 18 (both may apply).
   4. If f > 0: lay coins while the number laid so far, as a float, is below f, at least
      one — i.e. ceil(f) coins. Each coin: type 12, subtype 0, material 11, everything else
      0; scatter; lay with scale 0.75.

   All of this is single-precision float.

### 2.3 Race resources

1. One rand: n = rand mod 3 + 1. Repeat up to n times:
   1. Clear an item; scale = 0.8 (0x42e530).
   2. By D's race:

      | Race | Item (type 11 unless empty) | Scale |
      |---|---|---|
      | 0x3a | subtype 2, material 20 | 0.8 |
      | 0x46 | subtype 18 | 0.8 |
      | 0x47 | subtype 21 | 0.8 |
      | 0x78, 0x79, 0x7c, 0x7e | one rand: even → subtype 5, material 22; odd → subtype 1, material 2 | 0.8 |
      | 0x7a | subtype 27 | 0.8 |
      | 0x7b | subtype 11, material 28 | 0.8 |
      | 0x7d | subtype 6 | 0.8 |
      | 0x7f | subtype 20 | 0.8 |
      | 0x80 | subtype 23 | 0.8 |
      | 0x83 | subtype 0, material 11 | 3.0 |
      | 0x84 | subtype 0, material 1 | 3.0 |
      | 0x85 | subtype 0, material 12 | 3.0 |
      | 0x86 | subtype 0, material 18 | 0.8 |
      | 0x87 | subtype 32, rarity 1 | 3.0 |
      | 0x88 | subtype 33, rarity 2 | 3.0 |
      | 0x89 | subtype 34, rarity 3 | 3.0 |
      | 0x8a | subtype 35, rarity 4 | 3.0 |
      | 0x8b | subtype 8, material 23 | 0.8 |
      | any other | none (type stays 0) | 0.75 |

      Unlisted fields stay 0.
   3. One rand: if it is a multiple of 8 **and** the race is 0x78..0x7f but not 0x7e, the
      item becomes type 11, subtype 37, material 0, rarity 0 (scale unchanged). Otherwise,
      if the item's type is 0, **end the whole loop** (no more iterations).
   4. Scatter; lay with the scale.

For a race with no entry this draws exactly two values (n and one test) and lays nothing.

---

## 3. Item generators

The weapon and armour generators work the same way: they build a candidate list by setting
fields of the output item and appending a copy of it, then pick one candidate with a final
rand and copy it over the output. Every candidate's seed is a whole rand value. The class
argument selects sections; any class other than 1..4 (callers pass −1, i.e. 0xffffffff)
selects every section.

### 3.1 Weapon, 0x2d15d0 (world, out, rarity, class)

Takes four arguments; callers also push a fifth byte (0), which this routine ignores.

1. Clear the output (subtype, seed, region, +0x18, attributes, count = 0), then rarity =
   the rarity argument (a byte), material = 1, type = 3.
2. **Warrior section**, when the class is not 2, 3 or 4:
   1. One rand: subtype = rand mod 3. One rand: seed. Append.
   2. Material = 1. One rand: subtype = rand mod 3 + 15. One rand: seed. Append.
   3. Subtype = 13. One rand: seed. Append.
3. **Rogue section**, when the class is not 1, 2 or 3: subtype 3, rand seed, append;
   subtype 5, rand seed, append; subtype 4, rand seed, append.
4. Material = 2 (always, even when the next section is skipped).
5. **Ranger section**, when the class is 2 or not in 1..4: subtype 6, rand seed, append;
   subtype 7, rand seed, **not appended**; subtype 8, rand seed, append. (The subtype-7
   candidate's draw happens but it is overwritten before it is appended; ranger weapons are
   only subtypes 6 and 8.)
6. **Mage section**, when the class is 3 or not in 1..4: subtype 10, rand seed, append;
   subtype 11, rand seed, append; subtype 12: one rand, material = 12 if it is even, 11 if
   odd; one rand seed; append.
7. One rand: copy candidate[rand mod count] into the output.

Candidates and draws:

| Class | Candidates (subtype/material) | Rand draws |
|---|---|---|
| 1 | 0–2/1, 15–17/1, 13/1 | 5 + 1 |
| 2 | 6/2, 8/2 | 3 + 1 |
| 3 | 10/2, 11/2, 12/11 or 12 | 4 + 1 |
| 4 | 3/1, 5/1, 4/1 | 3 + 1 |
| other | all eleven, in section order (warrior, rogue, ranger, mage) | 15 + 1 |

(The warrior's first two candidates each take a subtype draw before their seed draw.)

### 3.2 Armour, 0x2d0680 (world, out, rarity, class, plain)

All candidates have subtype 0.

1. Clear the output entirely (type 0, material 0), then rarity = the rarity argument.
2. **Warrior section**, when the class is not 2, 3 or 4:
   1. Material: if plain is 0, one rand and by rand mod 3: 1 → 29, 2 → 4, 0 → 1. If plain is
      not 0, material 1 and no draw.
   2. Type 7, rand seed, append; type 4, rand seed, append; type 5, rand seed, append;
      type 6, rand seed, append.
3. **Mage section**, when the class is 3 or not in 1..4: if plain is 0, one rand (its value
   picks 32, 35 or 26 but is then discarded); material = 26 in every case. Then type 7, rand
   seed, append; **type 7 again**, rand seed, append; type 4, rand seed, append; type 5,
   rand seed, append; type 6, rand seed, append.
4. **Ranger section**, when the class is 2 or not in 1..4: as the mage section with the
   discarded pick among 31, 33, 27 and material 27.
5. **Rogue section**, when the class is not 1, 2 or 3: as the mage section with the
   discarded pick among 30, 34, 28 and material 28.
6. **Accessories, always:** one rand, material = 11 if even, 12 if odd; type 8, rand seed,
   append. One rand, material = 11 if even, 12 if odd; type 9, rand seed, append.
7. One rand: copy candidate[rand mod count] into the output.

So the only effect of the **plain flag** is to skip the material draws (one per class
section); for the mage, ranger and rogue sections it changes only the rand count, and for the
warrior section it also fixes the material at 1. No caller in the executable passes a
non-zero plain flag (§5).

Candidates and draws (plain 0; subtract one per section with plain set):

| Class | Candidates | Rand draws |
|---|---|---|
| 1 | 4 warrior + 2 accessories = 6 | 5 + 4 + 1 |
| 2, 3, 4 | 5 + 2 = 7 | 6 + 4 + 1 |
| other | 4 + 5 + 5 + 5 + 2 = 21 | 5 + 6 + 6 + 6 + 4 + 1 |

(Note the weapon generator's subtype-12 material is 12 on an even draw, the accessories'
material is 12 on an odd draw.)

### 3.3 Pet food, 0x2d0fc0 (world, out, key)

1. Look the key up in W +0x378 (exact match).
2. One rand. If it is even **and** the key was found **and** its list is not empty, use that
   list; otherwise use the pet list W +0x360.
3. Clear the output; type = 21 (0x15).
4. One rand: subtype = list[rand mod list length].

Always two draws. The game does not guard against an empty pet list (it would divide by
zero); a port may assume it is non-empty. The death drops call it with key 0.

### 3.4 The world's special lists

Both lists belong to the world and are read only here (and, for +0x360, by the conversation
script). Their contents are game data, not derived from the seed; the port reads them from
the extracted oracle (`Extract/oracle/items.bin`, §7.2): first the pet list, then the keyed
lists. Which code fills them in the game was not traced (§8).

---

## 4. Rewards, 0x2a7ca0 (world W, position Q, tier T)

1. **Zone region:** R = region at (truncated block of Q's x, truncated block of Q's y).
2. **Gear for each nearby player.** For each creature P in W's creature list, in order, with
   hostility kind 0 and squared distance from Q below 10000.0 (0x42e5e8; per axis
   float(int64 difference of P minus Q) × 1/65536, squared and summed in float), i.e.
   within 100 blocks:
   1. One rand: even → armour generator (W, rarity T, class = P's class, plain 0); odd →
      weapon generator (W, rarity T, class = P's class).
   2. Copy the result; its region becomes R.
   3. Scatter around Q (z unchanged); lay with scale 1.0, count 1, **no creature**, owner =
      P +0x968.

   Because there is no creature, the drop layer does not re-region the item, and its
   reservation (§1 step 5) picks among all usable players within 256 blocks of the drop —
   possibly not P. The owner argument is applied only if the picked player can use the item.
3. **Coins.**
   1. c = ((float(T) − 1.0) × 0.1 + 1.0) × 10.0.
   2. One rand: k = truncate to i32((float(rand) / 32767.0 + 1.0) × c).
   3. If k > 0, lay k coins: type 12, material 11, everything else 0; scatter; scale 0.75,
      count 1, no creature, owner 0. Coins are in the reservation set, so each coin is
      reserved for one usable player within 256 blocks (anyone can use coins), with one
      pick draw each.
4. **Tier resources.** For i = 1 .. T: one rand; if it is a multiple of 8: an item of
   type 11, subtype 31 + i (32..35), rarity i, everything else 0; scatter; lay with scale
   0.75, count 1, no creature, owner 0. (Type 11 is not reserved.)

Callers pass T = min(level + 1, 4) for a dying creature (§5), so T is 1..4.

---

## 5. How the routines fit together

* **Death** (`creature-hit.md` §3.6): the apply routine 0x29dcf0 calls 0x2a6860(W, T) and
  then, when T +0x128 has bit 8 (0x100), 0x2a7ca0(W, T's position, min(T level + 1, 4)).
* **0x2a6860** calls the weapon (0x2d15d0) and armour (0x2d0680) generators with the
  creature's level as rarity and class −1, the pet-food generator (0x2d0fc0) with key 0,
  and 0x2a5b70 for every item (eight call sites).
* **0x2a7ca0** calls both generators with the tier and each player's class, and 0x2a5b70
  three ways (gear, coins, tier resources).
* **0x2a5b70** is also called by the interaction handler 0x2c3710 (three sites; among them a
  player dropping an item, interaction kind 8 — a player-kind creature, so no region stamp
  and no reservation), which also calls 0x2a7ca0 twice (object rewards). Those callers are
  not specified here.
* The generators are also called by 0x2ac6e0 (armour) and 0x2acf90 (weapons), with the local
  player's class, and by the controller routine 0x965e0 (both, class −1). Every one of these
  passes plain flag 0.
* Ground items laid here are what the use prompt (`use-prompt.md` §2.4) targets: +0xc4 bit 1
  stops automatic pick-up for types 23 and 24, +0xc8 counts down before automatic pick-up,
  +0xd8 reserves the item.

---

## 6. Constants

| RVA | Value | Use |
|---|---|---|
| 0x42e5f0 | 32767.0 | rand normaliser (scatter, angles, coin factors) |
| 0x42e5f8 | 65536.0 | scatter scale; also the reservation radius² (blocks²) in 0x2a5b70 |
| 0x42e7e4 | 360.0 | angle scale |
| 0x4ac980 | 1.0 | scatter centre, coin factor, scale of gear drops |
| 0x42e710 | 0.75 | scale: type-15 item, coins, inventory, default race item |
| 0x42e530 | 0.8 | scale: race resources |
| 0x42e580 | 3.0 | scale: some race resources |
| 0x42e528 | 0.1 | per-level coin step (0x61000) |
| 0x42e598 | 10.0 | coin factor for flag bit 9; reward coin factor |
| 0x42e5a0 | 20.0 | coin factor for flag bit 18 |
| 0x42e510 | 1/65536 | distance units → blocks |
| 0x42e5e8 | 10000.0 | reward radius² (blocks²) |
| immediate | 0x454083fc | region set (§0.7) |
| immediate | 0x440d3fc | reservation set (§0.7) |
| immediate | 300, 500 | pick-up delay range |
| 0x1096cc | jump table | weapon subtype → class (§0.5) |
| 0x1096a8 / 0x109694 | index + jump table | armour material → class (§0.5) |

Integer rolls: worn item 1 in 16; pet slot 1 in 50; generated item 1 in 20; fixed item
1 in 8 (then 1 of 5); type-15 item 1 in 20; rarity raise 1 in 20 per step; race replacement
1 in 8; tier resource 1 in 8. "Multiple of n" over 0..32767 keeps C's modulo bias (for
example 1639 of the 32768 values pass "multiple of 20").

---

## 7. Verification

### 7.1 Death drops: `Extract/oracle/drops.bin` (CDR1)

Produced by ZoneExtractor's drops request; read by `dropOracle` in `tests/EngineTests.cpp`,
which is the authority on the byte format. Each entry records one call of 0x2a6860:

* the world's day value, the dead creature's address and full record, its inventory (tabs,
  slots of 0xa4 bytes) and its aggro map;
* the world's creature list in order (address, id, hostility kind; full records for
  players, since only they are read by the reservation);
* the pet list and the keyed pet lists (as in items.bin);
* the block at the dead creature's position (for the region stamp's byte-5 test);
* every 0x2a5b70 call made, in order: the item, position, angle, scale, count, calling
  creature and owner arguments; whether the target zone was loaded; the column of blocks
  read by the ground search (top z and the blocks going down); and the ground items it
  appended (0xe0 bytes each);
* every rand value drawn during the call.

To check an implementation, run it on the recorded creature and world, feeding the recorded
rand values in order, and compare:

1. the number of drop-layer calls;
2. for each call, the item (ignoring padding bytes +0x01..+0x03, +0x15..+0x17, +0x9e,
   +0x9f), the position, angle, scale, count and owner arguments, and that the creature
   argument is the dead creature;
3. the ground items appended, in order, byte for byte, ignoring the item padding and
   +0xc5..+0xc7;
4. that exactly all recorded rand values were consumed (no more, no fewer).

Useful splits when a death fails: by hostility kind and race (race table, inventory),
by flags 0x200/0x40000 (forced gear, coin factors), by the number of players nearby
(reservation and fallback), and by unloaded zones (calls that lay nothing and draw no
angle). Rewards (§4) are not in this recording; reward kills appear only in the hit
recording (`creature-hit.md` §6.3), whose death draws must start with this routine's.

### 7.2 Generators: `Extract/oracle/items.bin` (CIG1)

Produced by ZoneExtractor's items request; read by `itemOracle` (format) and `petList`
(the first list only) in `tests/EngineTests.cpp`. Layout: the magic "CIG1"; the pet list
(u32 count, then i32 values); the keyed pet lists (u32 count; each: i32 key, u32 count,
i32 values); then entries until the end of the file, each: u8 generator (0 weapon, 1 armour,
2 pet food), i32 class (the key for pet food), u8 rarity, u8 plain flag, u32 seed, u32 number
of rand draws the game made, and the 0xa0-byte item it produced.

The game's rand was replaced by a scripted generator for these calls: state starts at the
seed (or 0x9e3779b9 if the seed is 0); each draw applies state ^= state << 13, state ^=
state >> 17, state ^= state << 5 (32-bit) and returns state & 0x7fff. Run the port's
generator with the same arguments and the same scripted draws, on an output buffer filled
with 0xcc, and require the same item (padding ignored, as above) and the same number of
draws. This checks §3 directly, including the ranger weapon's unappended draw, the
discarded armour material draws and the plain flag.

### 7.3 Coarse cross-checks

`docs/progression.md` lists observed coin counts per star level and the drop decisions
already independently checked (worn-item order and exclusions, the 1-in-16, 1-in-50 and
1-in-20 rolls); these agree with §2.

---

## 8. Open questions

1. **Bit 0x20 of block byte 5** (the region stamp, §1 step 3) — presumably marks blocks
   inside a landmark, sending drops there to the landmark's region instead of the
   creature's home region. Its meaning and who sets it were not traced; the behaviour does
   not depend on the name.
2. **Who fills W +0x360 and +0x378**, and what the keys of +0x378 mean. Only key 0 is used
   by the drops; the oracle supplies both.
3. **Creature +0x1b0/+0x1b4** is copied into every ground item (+0xb8/+0xbc); no reader of
   the ground item's copy was examined.
4. **Rarity above 4.** Generated drops take the creature's level byte as rarity. When the
   drop is reserved for a player who can use it, the clamp lowers it to 4; otherwise it
   stays as is. Whether creature levels above 4 occur in normal play was not checked.
5. **The type numbers** 15, 22, 23, 24, 26, 30 and the item named by type 1 subtype 13 are
   used here only as numbers; their names are not needed.
6. **The scale argument** of 0x2a5b70 is unused in this build; it may be a leftover.
7. **The plain flag** of the armour generator is 0 at every call site found.
8. **Empty pet list.** 0x2d0fc0 would divide by zero; not guarded.
9. **The ground search** relies on the block lookup eventually returning a solid block
   below the position inside a loaded zone; behaviour below the lowest stored block of a
   column follows the block lookup (0xe9a0/0xeb90) and was not traced further here.
