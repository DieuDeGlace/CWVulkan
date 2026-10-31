# World generation (C): creatures, spawns and region roles

Behavioural specification of how Steam Cube World decides what lives in a zone: the region
planner (0x115df0), the creature tables, the zone's spawn points (zone+0x460) and how each one
gets its race, class, looks, gear and name, the townsfolk, the daily refresh, and how the world
later turns spawn points into creatures (residents and arena stages). Written from the
decompile (`decompiled cube world steam`, functions by RVA), the zone generator's assembly
(0x2ff6f0 does not decompile; its creature part 0x3171a2–0x318f6c was read from the
disassembly) and the binary's constants. Routines are cited by RVA (image base 0x140000000).

This part has four files:

| File | Content |
|---|---|
| `worldgen-creatures.md` (this file) | conventions, records, the region planner, the creature tables, making one spawn point (variants, looks, gear, the inventory), spots, residents, the creature record's defaults, pets and arena stages, oracles, open questions |
| `worldgen-creatures-zone.md` | the zone's creature passes in order: the arena master (made in B's cell loop), the cell pass (townsfolk, the first town, wild groups, castle and dungeon creatures, animals, and the items it interleaves), the marking pass, the daily refresh |
| `worldgen-creatures-roles.md` | what a place gets from its region role (0x11a820): role items, the set pieces of roles 19–23 and the encounters (0x1274d0) |
| `worldgen-creatures-tables.md` | appendix: how the game builds the spawn groups, race levels, variant lists and race pickers (their values are read at run time from Extract/oracle, not kept in the repo), and the short castle race lists, consumable races and looks draws |

Other parts this one leans on (interfaces only):

* **worldgen-terrain.md (A)**: the hash and random number R, the place layer (bigCell
  0x36160, placeKind 0x2b99d0, cellPoint 0x362a0, warped distance 0x364e0, placeAt 0x35ce0,
  regionAt 0x2ada20, the biome or "culture" of a region 0x2b9080), the column values of
  0x35ea0 (warmth 0x2d9b60, dryness 0x2b5960, forest 0x2d9f30, lava region 0x2b9750, lake region
  0x2bb7e0, kind-27 weight 0x2abb60, river 0x2e5f80, lava 0x2b92f0, swamp 0x2d01a0, highlands
  0x2bb310, roads 0x2d4730, ground height 0x2b4450) and the zone's columns.
  Input (x, y in blocks, mode) → those floats.
* **worldgen-structures.md (B)**: the cell objects (0x69c60 → type; 0x2d8630 → point), the
  castle cells (0x2a8290 → kind, detail, style, steps bytes), the town blocks (0x2af7b0 → a
  town cell's block role), the fortress plan (0x2a25a0, 0x2a1d10, 0x2a2b00, 0x2b8b10), the big
  structure 0x2aa270 and its anchor 0x2a9df0, the place landmark 0x2ba990, the building base
  height 0x2d87f0, static objects (zone+0x60) and how one is settled and kept (0x31b2d0,
  0x129b50, 0x1194b0), the items list (zone+0x30) and everything 0x2ff6f0 does before 0x3162b0.
  Where this part makes an object or an item it says so and gives every field it sets; the
  record layouts are B's.
* **conversation-script.md**: the xorshift X (§2.1), N and R (§2.4), the C runtime rand()
  (§2.5), the planner's lists (§1.5), the encounter of a place 0x119a00 (§5.5), the style
  helpers 0x10cee0, 0x10eb10, 0x10d310, 0x10d2e0 (§5.4.1), the sibling race 0x1420f0 (§5.6).
* **lore-and-artifacts.md**: the realm of a place 0x2bb060 (§4.3), artifacts 0x11d7b0
  (§5.1).

CubeVulkan's own options do not change anything here. The WorldScale stretch acts only
through regionAt (A). Mods hook in at the planner's role lookup 0x93480, the place kind
0x2b99d0 and the generated items (role items, region boats): they replace an answer, never a
draw.

---

## 0. Conventions

### 0.1 Numbers and random draws

* **rand()** is the C runtime generator of conversation-script §2.5: state = state × 214013
  + 2531011 (mod 2^32), result = (state >> 16) & 0x7fff. One global state per thread. The zone
  generator and everything it calls draw from it; **srand(v)** sets the state to v. "a draw"
  below always means one rand() call, made only when the text reaches it.
* **"rand % n"** is C's remainder of the non-negative draw: the game computes it either as a
  signed remainder or as an unsigned 64-bit remainder of the sign-extended draw; for the
  values 0..32767 both equal the mathematical remainder.
* **"rand % n == 0"** and **"rand % n != 0"** test that remainder.
* **N(n)** and **R(n)** are the integer hash and the random number of conversation-script §2.4
  (R(n) = 100000 − N(n) div 21474, 0..100000). **H(x, y)** means x + 57·y in 32-bit wrapping
  arithmetic, so "R(H(x, y) + k)" is R of x + 57y + k. A "seed R(…)" passed to srand is that
  value as an unsigned 32-bit number.
* **X(n)** is the xorshift of conversation-script §2.1 (n ^= n<<13; n ^= n>>17; n ^= n<<5,
  32-bit unsigned). When the game tests only bit 0 or bits 0–1 it skips the last step; the
  result is the same.
* **The signed-quotient quirk.** In many places the game computes N(n) div 21474 with a
  multiply by a negative magic constant, which yields **q = −(N(n) div 21474)**, and then tests
  q & 1 or q & 3. Since 100000 is a multiple of 4, q & 3 = R(n) & 3 and q & 1 = R(n) & 1. This
  spec writes those tests with R. Where the game adds the negative quotient to other numbers
  the text writes the sum out.
* **Floor division.** "floor(a / 2^k)" for the game's shift-with-adjustment on negative
  values. Cell of a block coordinate = floor(b / 16); place cell = floor(b / 1024); zone =
  floor(b / 64); "c1" of a zone = floor(zone·64 / 1024), and **c1 rounded** = c1 rounded down
  to even when bigCell(c1) is true.
* **Fixed point.** Positions are 64-bit integers in 1/65536 of a block ("dots"). A block
  coordinate b becomes b·65536; "+ half" adds 32768. A float f of blocks becomes
  trunc(f × 65536.0) computed in single precision (0x29be70). Dots to blocks truncates toward
  zero ((v + (v<0 ? 0xffff : 0)) >> 16) wherever the text says "rounded to zero".
* **Yaw draws.** Two float formulas appear and must be kept apart, both in single precision:
  **yawA** = (float)rand / 32767.0 × 360.0 and **yawB** = (float)rand × 360.0 / 32767.0. A
  **quarter yaw** is (float)(rand % 4) × 90.0.
* **Set iteration.** Every std::map/std::set in the game is ordered by its key compared as
  signed integers, first field first.

### 0.2 The zone's lists

| Offset | Content |
|---|---|
| zone+0x10, +0x14 | zone x, y |
| zone+0x30 | items lying about (0xe0-byte records; B) |
| zone+0x60 | static objects (0xe0-byte records; B); +0x78 is the generator's copy (see zone file §4) |
| zone+0x100 | the **spot map**: (cell x, cell y, kind) → list of positions (§5) |
| zone+0x110 | the **working spot map**, a copy of +0x100 from which spots are taken |
| zone+0x460 | the **spawn points**: map id → spawn point (0x950 bytes) |
| zone+0x480 | the **tracked list**: ids whose deaths count toward requiredDefeated (+0x490 holds the zone's count of dead tracked creatures) |
| zone+0x498 | the **mission list**: ids that must all die for the place's mission to be done |
| zone+0x4e8 | the daily refresh's creatures (a list of spawn point pointers) |
| zone+0x4f8 | the day the zone was last refreshed (−1 for a new zone) |
| zone+0x500 | the **shop spots**: positions taken by shopkeepers' counters (0x18 bytes each) |

### 0.3 The spawn point record (cube::SpawnPoint, 0x950 bytes)

Defaults are those of its constructor 0x118a60. Only fields generation or the resident pass
use are listed; every other byte is zero.

| Offset | Size | Default | Meaning |
|---|---|---|---|
| +0x00, +0x08, +0x10 | i64 ×3 | — | position in dots (z at the feet) |
| +0x18 | f32 | 0 | yaw in degrees |
| +0x1c | i32 | 1 | hostility kind (1 hostile, 3 friendly person, 4 neutral, 6, 7 animal/neutral monster, 8 pet-like) |
| +0x20 | i32 | 4 | race |
| +0x24 | i32 | 0 | level |
| +0x28 | i32 | 0 | **people**: the nation a person belongs to (realm kind 12–22, or a group code: 1, 3, 4, 5, 6, 8, 11) |
| +0x2c | i32 | −1 | (copied to the creature +0x974; never set by generation) |
| +0x30 | i32 | 0 | **style**: the realm value (0x2bb060) of the place it belongs to |
| +0x34 | u8 | 0 | class (in: the class the caller chose, 0 for "pick one"; out: 0x511e0's) |
| +0x35 | u8 | 0 | specialisation |
| +0x38 | i64 | 0 | id (given when the zone takes it, §1.2) |
| +0x40 | i64 | 0 | id of the spawn point it belongs to (a mount's rider, a guard's boss, a companion's master) |
| +0x4c | u8 | 0 | stage byte copied to the creature's +0x19c (generation never sets it) |
| +0x50 | 0xb0 | ctor 0x4d1f0 | **appearance** (0x53e80); +0x58 (appearance +8) is the **appearance flags** |
| +0x100 | 16 bytes | zero | **name** (zero-terminated; only the first town's three people have one) |
| +0x110 | 11 × 0xa0 | ctor 0x4d300 | **equipment**: slots 0 unused, 1 neck, 2 chest, 3 feet, 4 hands, 5 shoulder, 6 left weapon, 7 right weapon, 8 left ring, 9 right ring, 10 pet. Item: +0 category (u8), +4 id (i32), +8 modifier (u32), +0x14 level (u8), +0x1c material (u8) |
| +0x7f0 | vector | empty | **inventory** tabs (each a vector of 0xa4-byte stacks: +0 quantity, +4 the item); the inventory record runs to +0x8b3 (§4.5) |
| +0x8ac, +0x8b0 | i32 ×2 | 0 | the inventory's two counters (§4.5; generation never fills them) |
| +0x8b8 | i32 | 0 | **roam** bits (how it wanders: 4 stays, 0x10 roams, 0x20, 8, 0x30, 0x1030, 0x2030, 0x4000, 0x90, 0x14 …) |
| +0x8bc | u8 | 0 | **mode** (0x8a: a spider that drops from above, see §7) |
| +0x8c0 | vector of i32 | empty | abilities |
| +0x8d8 | i32 | (not set) | ability pick |
| +0x8e0 | vector of i32 | empty | second ability list |
| +0x8f8, +0x8fc | i32 ×2 | 0x7fffffff ×2 | **home**: the place cell of an encounter's people |
| +0x900 | list of (i32, i32) | empty | **area**: 16-block cells it keeps to |
| +0x910 | set of i64 | empty | **group**: ids it keeps with |
| +0x920 | i32 | 0 | **requiredDefeated** |
| +0x928 | ordered cell-tree header; count +0x930 | empty | indexed-object state prerequisite; native retained-byte quirk in [resident supplement](steam-resident-effects-prerequisites.md) (no filling generation producer identified) |
| +0x938 | set of i64 | empty | **prerequisites**: ids that must all be dead |
| +0x948 | i32 | 0 | **requiredStage** |

### 0.4 "Making" a spawn point

Most builders end the same way. "**Finish** it with looks (wild w), gear and spot S" means, in
this order: looks 0x53e80 with the race and the wild argument w (§4.2), ORing what it returns
into the flags; gear 0x511e0 with (hostility, race, level, flags, people) (§4.3); position =
the spot S. Builders that differ say so; the order of these three steps changes the draws and
is always given.

"**Add** it" means the zone takes it (§1.2).

---

## 1. Spawn point bookkeeping

### 1.1 The id (0x119530)

When the zone takes a spawn point whose id (+0x38) is 0, its id becomes
base = ((zy + 0x1ffffff) << 32 | (zx + 0x1ffffff)) << 6, as 64-bit integers (each sum is a
32-bit int sign-extended before the OR; the shifts wrap), then incremented by 1 while the zone's
spawn map already holds that id. The point is then stored under its id. In practice ids run
base, base+1, … in the order points are added, and the map's order is the order of adding.

### 1.2 The tracked and mission lists

The builders of `worldgen-creatures-roles.md` put ids into the tracked list (zone+0x480) and the
mission list (zone+0x498); some empty the tracked list first. 0x11a820 empties the mission list
before anything else. Each case is given where it happens.

---

## 2. The region planner (0x115df0)

Inputs: the world and a region (rx, ry). Output: the planner record (conversation-script §1.5
lists the fields the talk reads):

| Offset | Content |
|---|---|
| +0x08, +0x0c | rx, ry |
| +0x10 | **role map**: place cell → role (a big place keyed by its even corner) |
| +0x20 | artifact map: place cell → item (lore-and-artifacts §5.1) |
| +0x30 | help-target list; +0x48 artifact-place list (conversation-script §1.5) |
| +0x60 | **level map**: place cell → level |
| +0x70 | a pairing map used only by the role-18 pass (always empty, §2.3 step 19) |
| +0x80, +0x84 | **special town** cell, or (0x7fffffff, 0x7fffffff) |
| +0x88 | **marked** byte (1 when a demon portal exists) |
| +0x8c | **item bits** (u32): 1 role 12, 2 role 10, 4 role 11, 8 role 21 |
| +0x90 | **item list** (vector of i32): 16 per role-10 place, 15 per role 12, 14 per role 21, in role-map order |

### 2.1 Counter, culture and pool

* The counter **c** starts at R(H(rx, ry) + 0x46d7c2) (unsigned 32-bit). "Take X(c)" below
  means use X(c), then c = c + 1; "X(c + k)" reads ahead without moving c.
* **culture** = the biome of the region (0x2b9080 at (rx, ry); A).
* **Pool.** For place cell x from floor((rx·8192 − 8192) / 1024) to floor((rx + 2)·8192 /
  1024) inclusive (outer loop), and y likewise from ry (inner loop), skipping (0, 0):
  big = bigCell(x, y); kind = placeKind(x, y, big); skip kind 0; (px, py) = (x, y) rounded down to
  even when big; keep the place if regionAt(cellPoint(px, py, big)) is (rx, ry). The pool is
  an ordered set of (kind, px, py) compared as signed ints in that order, so a big place
  enters once. (The game also keeps a count of places per kind; it never affects the result.)
* "**Remove** P" erases it from the pool; "**role P = r**" and "**level P = l**" write the maps
  (overwriting).
* "**Non-town list**" = every pool place whose kind is not 5, in pool order, collected at the
  moment the step runs.
* "**Pick r from L**": if L is empty nothing happens (and c does not move); otherwise P =
  L[X(c) mod |L|], c += 1, remove P, role P = r.

### 2.2 Steps, in order

1. **Fixed kinds.** Collect the pool places of kinds 7, 16, 17, 24 (pool order), then for
   each: remove it, and
   * kind 7: role 21, level X(c) mod 3 + 2, c += 1;
   * kind 16: role 22, level (X(c) & 3) + 1, c += 1;
   * kind 17: role 23, level (X(c) & 3) + 1, c += 1;
   * kind 24: role 17, level 4, no draw.
2. **The witch.** towns = pool places of kind 5 (pool order). If |towns| ≥ 2: k22 = kind-22
   places; if k22 is not empty: P = k22[X(c) mod |k22|]; remove P; role 19; level
   X(c + 1) mod min(|towns|, 5); special town = towns[X(c + 2) mod |towns|] (its x, y);
   c += 3.
3. **Demon portal.** k37 = kind-37 places; if not empty: P = k37[X(c) mod |k37|]; remove;
   role 20; level X(c + 1) mod 5; marked = 1; c += 2.
4. **Artifact places.** A = every pool place (pool order) of kind 8, or whose big structure
   0x2aa270(x, y, kind, bigCell(x, y)) is not 0. For each in order: remove, role 14, level 4,
   make its artifact (0x11d7b0 with the counter; it moves c by 2; lore-and-artifacts §5.1).
5. **Fallback artifact**, only when A was empty: the candidate list is the first non-empty of
   (a) kind 24 places (always empty by now), (b) kinds 18, 23, 28, 30, 31, 36, (c) kinds 7, 9,
   12, 15, 18, 20, 23, 25, 28, 30, 31, (d) the non-town list. If one: P = list[X(c) mod n],
   c += 1, remove, role 14, level 4, make its artifact.
6. **Role 6.** L = places of kinds 10, 19, 21, 29; if empty, the non-town list. Pick 6 from L.
   No level.
7. **Role 7**, only if culture is not 1, 5, 12 or 13 (bit test 0x3022 for cultures ≤ 13; any
   culture > 13 also qualifies): L = kinds 13 and 23; if empty, the non-town list. Pick 7.
8. **Role 1, four times from one list.** L = the non-town list, collected **once**. For b = 0,
   1, 2, 3: if L is not empty: P = L[X(c) mod |L|], c += 1, remove P (a no-op if already
   removed), role P = 1, level P = b. The same place can be drawn twice; the later write wins.
9. **Roles 2–5.** For b = 0..3: L = the non-town list (fresh). If |L| < 8: take X(c) (c += 1)
   and skip this b when that value is odd. Then pick b + 2 from L, with level b.
10. **Role 9** if culture is not 1: pick 9 from the non-town list.
11. Pick 10, then 11, then 13, then 8, then 12, each from a fresh non-town list.
12. **Role 16.** Walk the pool in order; for each place whose kind is 18, 23, 28, 30, 31 or 36
    (mask 0x10d0840000, 0x128da0) take X(c) and keep the place if it is odd. Then for each kept
    place in order: remove, role 16, level (X(c) & 3) + 1, c += 1.
13. **Role 15.** Every remaining pool place of kind ≠ 5, in order: role 15, level c mod 4 (the
    raw counter, as a signed remainder; c is always positive here), c += 1.
14. **Role 18 (never happens).** For every place of the original pool (a copy taken after the
    pool was built) whose encounter 0x119a00 is 24, the game would pair it with the nearest
    place of role 15 or none within a squared cell distance of 16 among kinds 10, 13, 20, 25,
    28, 29, 31, 32, and make that place role 18. 0x119a00 never returns 24
    (conversation-script §5.5), so nothing happens; implementations may skip it.
15. **Lists.** item bits = 0; walk the role map in order: role 10 → bits |= 2, push 16; role 11
    → bits |= 4; role 12 → bits |= 1, push 15; role 21 → bits |= 8, push 14. Then the help-target
    and artifact-place lists (conversation-script §1.5).

### 2.3 Lookups on the planner

* **roleAt** (0x93480) and **levelAt** (0x12a200): the map value, 0 when absent.
* **Role treasure** (0x131540) of a place: roles 2–5 → a crafting book item (category 24, id
  0xe + role − 2, region = the planner's region); role 14 → the place's artifact; others none.
* **Region item of a place** (0x119800): if the item list is empty or R(H(px, py) + 0x29fa4) & 3
  is 0, the answer is 8; otherwise item list[R(H(px, py) + 0x89d85) mod |list|].
* **Door item** (0x12a600, B uses it): −1 for an empty list, else
  list[R(H(cx, cy) + 0x89d85) mod |list|].
* **Encounter** (0x119a00): conversation-script §5.5.

---

## 3. The creature tables

The game builds them once per world; CubeVulkan reads the values at run time from the
owner's install (Extract/oracle, written by ZoneExtractor: spawn-table.txt, races.txt,
pickers.txt, looks.bin; formats in §8.3), so the repository holds no game data. Tables §T1–§T4
describe how each is built, indexed and drawn from.

### 3.1 Spawn groups and race levels (0x44b40, world+0x1b0)

0x44b40 fills the spawner at world+0x1b0:

* **+0x08: the race level table** (world+0x1b8), a vector of 0x132 (306) ints, zero, then set
  one by one. A wild creature of a group whose kind is not 4 or 8 gets level table[race]
  (tables §T1; values from spawn-table.txt).
* **+0x20: the groups** (world+0x1d0), 120 records of 0x28 bytes pushed in order: +0x00 a
  vector of race ids (the types), +0x18 **kind bits** (1, 2, 4, 8, 0x10: how the group comes,
  §zone 2.4), +0x1c **climate mask** (0 = anywhere), +0x20 **ground mask** (0 = not for animal
  draws). Tables §T2 describes them; the values come from spawn-table.txt.

### 3.2 Variants (0x1420f0 over 0x13f0d0)

**variant(r)**: build r's sibling list (0x13f0d0; tables §T3, values from races.txt; a race with no list is its own
list of one), then return list[rand % size]. The draw happens even for a list of one.

### 3.3 Race pickers

Each picks list[rand % size] and returns variant(that), so it draws two numbers (tables §T4; lists recovered from pickers.txt):
people 0x141480, people-or-beasts 0x1408e0 (its list depends on a culture argument; −1 means
none), undead 0x142190, undead leaders 0x142760, beasts 0x13ffc0, beast leader 0x13feb0,
lizards 0x141390, golems 0x142000, tower folk 0x13fa50, imps 0x13fda0, people of kind 8
0x140490. **Traveller 1** (0x141d70) is different: list = (2); if rand % 4 == 0 append 9; race
= list[rand % size]; then variant(race). It draws three numbers when the second is 0, else
three as well (two plus the variant).

---

## 4. Making one creature's data

### 4.1 Style helpers

For a style s (the realm number of a place, 0x2bb060, lore-and-artifacts §4.3):
**styleOdd**(s) = X(s + 0xe6cb) odd (0x10d310); **styleEven**(s) = X(s + 0xbd8b9) even
(0x10d2e0); **styleRace**(s) = [12..22][X(s + 0x7b2650) mod 11] (0x10cee0);
**styleBoss**(s, r) = 0x10eb10 (conversation-script §5.4.1 "nationModel", with n = r);
**stylePeople**(s) = (X(s + 0x18365) & 7) + 1 (0x10cd60).

### 4.2 Looks (0x53e80)

Arguments: the race (by reference), the appearance block (0xb0 bytes), a looks record or none,
and **wild** (a byte). Generation always passes no looks record.

1. With no looks record the routine draws first, always: c1, c2, c3 = three draws; l2 =
   rand % 100; l3 = rand % 100. The colour of the record is the three bytes (c3, c2, c1) as
   written into appearance bytes +2, +3, +4 (low byte of each draw), when the race uses them.
2. It then fills the whole appearance for the race: fixed values (sizes, offsets, model ids of
   the parts, flags) and the drawn parts. The fixed values are the game's data and come from
   looks.bin (§8.3), which holds the appearance the game made for every race 0..320. What
   depends on draws is listed per race in tables §T7 (head and hair from l2 and l3, extra draws
   for some races, and sizes some races draw).
3. **Flags.** The race's flags are ORed into appearance +8 (the spawn point's +0x58), keeping
   the flags already there. With wild set, some races also get 0x8000 (looks-wild.txt holds the
   flags each race ends with when wild).
4. **Size growth.** After the race's case, the size at appearance +0xc..+0x14 is grown for
   the flags it now has: ×1.5 for 0x20000, then ×1.5 for 0x200, then ×7 for 0x40000; after each
   growth, if x > 8, all three are multiplied by (8 / x). A flag the caller had set before the
   call therefore grows a race that does not have it by itself. Products are single precision,
   factor first (k × v).
5. With a looks record (players, not generation) the race is replaced by a race from the
   record's type and sex bit; not used here.

The routine returns nothing; callers read the flags back from +0x58.

### 4.3 Gear (0x511e0)

Arguments: hostility h, race r, level L, flags f, people p, and by reference the class byte,
specialisation byte, equipment, abilities, ability pick, second abilities, inventory. It draws
in exactly this order.

**A. Start.** The equipment is reset to a fresh one (all slots empty).

**B. Consumables.** If r is in the "carries consumables" set (0x65750 returns 3; tables §T5):
n = rand % 4; if n − 1 > 0, add a stack of item (category 1, id 1) with quantity n − 1 to the
inventory (0x4e810, §4.5; category 1 goes to tab 2, so the inventory then has three tabs).

**C. By class and race** (cls = the class byte as passed):

1. cls 0x8a: chest = category 4, material 6, id rand % 5 + 2. Go to E.
2. cls in 0x80–0x87, 0x8b or 0x9b (bits 0x80008ff of cls − 0x80): chest = category 4,
   material 6, id 0. Go to E.
3. r ≠ 0x12 (18):
   * 0xfc: right = (3, id 10, material 4). 0xfd: right = (3, 10, material 0x2c).
   * 0x100: right = (3, 0xf, 4); chest material 0xb; hands category 5 material 0xb; feet
     category 6 material 0xb.
   * 0x106: chest, hands and feet material 4.
   * 0xfe: right = (3, 5, 5); shoulder (7, material 5); chest (4, 5); hands (5, 5); feet (6, 5).
   * 0xff, 0x103: nothing.
   Each of those goes to E. Any other race goes to D.
4. r = 0x12 and cls 0: chest = (4, id 0, material 6). Go to E. Otherwise D.

**D. The general case.**

1. cls 0x90: unless f & 0x20, chest = (4, id 0, material 0x18). Go to E′ (E without its first
   store; see note).
2. cls 0x8f: unless f & 0x20, chest = (4, 0, 0x2c), right = (3, id 10, material 2), right's
   modifier = a draw. Go to E.
3. cls 0x88: chest = (4, id 1, material 6); right = category 3, id 0x15. Go to E.
4. Clear left, right, shoulder, chest, feet, hands, neck (category and id 0).
5. h is 7 or 8: go to E.
6. f & 0x18, or r = 0x76: clear right and left again; go to E.
7. By race:
   * 0x75: abilities += 0x69, 0x2f, 0x65; pick = 0; second += 0x11, 0x60. Go to E′.
   * 0x6c, 0x72: right (3, 0x11, 2), left cleared. 0x6e: right (3, 5, 5), left cleared.
     0x2e: right (3, 0x11, 7), left cleared.
   * 0x73: abilities += 0x5f, 0x67; right and left (3, 4, 1). 0x6d: abilities += 0x5f, 0x67,
     100; right and left (3, 2, 7). 0x51: abilities += 0x60; right and left (3, 0xc, 0xb).
     0x52: right and left (3, 0, 1).
   * 0x2f, 0x6f, 0x70, 0x71: nothing.
   * **Staff casters** 0x2b, 0x2d, 0xa7, 0xc7 and 0xe6: for 0xe6 first abilities += 0x65,
     pick = 0, second += 0x11, 0x60, 0xa5. Then abilities += 0x2f; spec = 0 for 0xa7, else 2;
     right = category 3, id rand % 2 + 10, material 2; left cleared; class = 0. Go to E.
   * Each case of this list goes to E.
8. **Pick a class.** If the class byte is 0: class = rand % 4 + 1. Then race 0x61 forces
   class 4 and race 0xa5 forces class 1.
9. **Weapons by class:**
   * class 1: spec = rand % 2; then as 0x8d.
   * class 0x8d: k = rand % 3. k 0: right = (3, id: draw % 3 + 0xf, material 1), left cleared
     (material 0 too). k 1: right = (3, draw % 3, 1), left = (3, draw % 3, 1) (right's draw
     first). k 2: right id = draw % 3, material 1, category 3; left = (3, 0xd, 1).
   * class 2: spec = rand % 2; right = (3, rand % 3 + 6, 2).
   * class 3: spec = rand % 2; k = rand % 3: 0 → right (3, 10, 2), left cleared; 1 → right
     (3, 0xb, 2), left cleared; 2 → right and left (3, 0xc, 0xb).
   * class 4: spec = rand % 2; right = (3, rand % 3 + 3, 1); if its id is 5 the left is
     cleared (category, id, material 0), else left = (3, same id, 1).
   * class 0x8e: k = rand % 4. k 0: right = (3, draw % 3 + 0xf, material 5), left cleared.
     k 1: right = (3, id 1 if the next draw is even else 0, 5), then left the same way with a
     new draw. k 2: right (3, 5, 5). k 3: right (3, 10, 5).
   * any other class: no weapon.
10. If f & 0x20: go to E.
11. **Armour list** by the right weapon's id: 10–12 → (0x1a, 0x20, 0x23), or (0x23) when
    p = 6; 6–8 → (0x1b, 0x1f, 0x21) or (0x21); 3–5 → (0x1c, 0x1e, 0x22) or (0x22); any other →
    (1, 0x1d, 4) or (4). Then class 0x8d replaces the list by (0xb) and 0x8e by (5).
12. In this order, each material = list[rand % size]: chest (category 4), shoulder (7), hands
    (5), feet (6). Then neck = category 8, material (rand % 2 == 0) + 0xb; right ring = 9,
    (rand % 2 == 0) + 0xb; left ring = 9, (rand % 2 == 0) + 0xb.

**E. Flag weapons** (E′ starts here too; the difference between the game's two entry
points is one register reload and changes nothing):

* f & 8: right and left = category 3, id 0x16.
* f & 0x10: right = category 3, id 0x17; left = an empty item (all fields 0).
* f & 0x20: chest = category 4, id 1.

**F. Class and abilities by race** (after E, always):

| Race | Effect |
|---|---|
| 0x11f, 0x120, 0x122, 0xe2 | class 0x92; abilities += 0x88 |
| 0x11, 0x5e, 0x61 | += 0x67 |
| 0x15 | += 0x46 |
| 0x19, 0xae, 0xd7 | += 0x49 |
| 0x2e | += 0x5f; class 0x94 |
| 0x2f, 0x6f | class 0x92; += 0x7b |
| 0x45 | class 0x91; += 0x82 |
| 0x46 | class 0x93; += 0x82 |
| 0x47, 0x48 | += 0x82 |
| 0x51 | += 0x65, then += 0x88 |
| 0x55, 0x6e | += 0x88 |
| 0x52 | += 0x5f |
| 0x56 | class 0x98; spec 1 |
| 0x57 | class 0x92; += 0x82 |
| 0x58 | class 0x93; += 0x82 |
| 0x5a | class 0x92 |
| 0x68 | class 0x95 |
| 0x6c, 0x6d, 0x72 | += 0x37, 100, 0x67 |
| 0x70 | class 0x92; += 0x88, 0x2f |
| 0xaf | class 0x99; pick 0; second += 0x55 |
| 0xb0 | class 0x99; pick 0; second += 0x28, 0x25, 0x26, 0x27 |
| 0xb1 | class 0x99; pick 0; second += 0x35 |
| 0xb2 | class 0x99; pick 0; second += 0x60 |
| 0xb3 | class 0x99; pick 0; second += 0x11 |
| 0xb4 | class 0x99; pick 0; second += 0x57 |
| 200, 0xc9 | += 0x4b |
| 0xe1 | += 0x69; spec 0 |
| 0x106 | class 0x92; += 0x5f |
| 0x10d | class 0x92; += 0x37 |

**G. Flag 0x200 ability.** If f & 0x200: abilities += one of (0x65, 0x67, 0x5f, 0x62, 0x60,
0x37, 0x73, 0x71, 0x2f)[rand % 9]; if that was 0x65: pick = rand % 3, second += 0x11, 0x60.

**H. Modifiers and levels.**

1. If h = 6, the modifiers of left, right, shoulder, chest, hands, feet, neck, left ring,
   right ring become X(r + 0), X(r + 100), …, X(r + 800) in that order (no draws). Otherwise
   those nine slots, in that order, each take a draw.
2. The level byte of right, left, shoulder, chest, hands, feet, neck, left ring, right ring =
   L (low byte).
3. Again, the modifiers of right, left, shoulder, chest, hands, feet, neck, left ring, right
   ring each take a draw, in that order. (So step 1's values never survive; its draws count.)

gear.bin (§8.4) checks all of this on 30,000 scripted calls.

### 4.4 Names

Generation names only the first town's three people (zone file §2.2): spawn point +0x100
gets "Wollay", "Pixxie" and "Joschi" (seven bytes each, with the terminator; binary at
0x4360c8, 0x473904, 0x47390c). The resident pass copies the 16 name bytes to the creature
(+0x958). Everyone else has an empty name here.

### 4.5 The inventory (0x4e810, 0x5f100)

The inventory record is the same in a spawn point (+0x7f0) and a creature (+0x9f0); offsets
here are from its start:

| Offset | Content |
|---|---|
| +0x00 | the tabs: a vector of tabs, each a vector of 0xa4-byte stacks (+0 quantity i32, +4 the 0xa0-byte item) |
| +0x18 | an i32 (copied by the resident pass; generation leaves it 0) |
| +0x1c | an item (copied likewise; empty) |
| +0xbc | counter A: quantities of items of category 12 with material 11 |
| +0xc0 | counter B: quantities of items of category 14 |

**Add** (0x4e810) takes the inventory, an item, a quantity q and a tab (−1 for "pick one"):

1. If q is 0, nothing.
2. If the tab is −1 it becomes the item's **tab** (0x5f100, by category):

   | Category | Tab |
   |---|---|
   | 3–9, 15, 25, 26 | 0 |
   | 22, 24, 29 | 1 |
   | 2, 11 | 3 |
   | 20, 21 | 4 |
   | 23 | 5 |
   | any other (1, 10, 12–14, 16–19, 27, 28, …) | 2 |

3. If the item's category is 0, nothing more.
4. If there are not more tabs than the tab number, the tab vector grows to tab + 1 entries (new
   tabs are empty). This happens before step 5, so even a counted item makes the tabs.
5. Category 12 with material 11: counter A += q, done. Category 14: counter B += q, done.
6. Otherwise walk the tab's stacks in order, remembering the first stack whose quantity is 0.
   If the item is **stackable** (category 1, 10, 11, 12, 13, 22 or 28; mask 0x10403c02, 0x5f0e0)
   and the stack holds the **same item** (0x4df60: equal category, id, modifier, the i32s at
   +0xc, +0x10 and +0x18, material +0x1c, attachment count +0x9d and level +0x14, then the
   first "count" 4-byte attachments from +0x1d byte for byte): its quantity += q (or becomes −1
   when q is −1), done.
7. Past the end: if a stack with quantity 0 was seen, the item is copied into the first such
   stack and its quantity = q. Otherwise a new stack (quantity q, the item; the 0xa4 bytes
   otherwise zero) is pushed at the end of the tab.

No draws. Generation always passes q = 1 except the consumables of gear §4.3 B (q 1 or 2), and
always a fresh inventory, so in practice: the first item of a tab makes a stack of its own, a
second equal stackable item adds to it. gear.bin (§8.4) checks the tab and stack of every
consumable it records (category 1 → tab 2).

---

## 5. Spots

### 5.1 The spot map (0x3162b0, part of 0x2ff6f0)

Built once per zone after the structures (B), before the cells' things. The zone's rand()
keeps running (no srand). (If worldgen-structures.md also describes this pass, the two must
agree; it is here because only creatures and chests use it.)

For a = 0, 2, …, 62 (outer) and b = 0, 2, …, 62 (inner): the game yields the thread (a call
with no effect), then X = 64·zx + a + rand % 2, then Y = 64·zy + b + rand % 2. (cx, cy) =
(floor(X/16), floor(Y/16)). The column (X, Y) of the zone has its stored blocks from base z0
upward (B, the column record +0x3c and its block vector). Positions are (X·65536 + 32768,
Y·65536 + 32768, z·65536).

For i from −1 to n − 1 (n stored blocks): z = z0 + 1 + i; b0 = block i (index −1 means a solid
block), b1 = block i + 1 (past the top: air). "solid(b)" = type & 0xfd ≠ 0 (not air, not water).

1. If the column's river value (field +0x2c) > 0.5 and solid(b0) and b1's type is 2: add a
   **kind 18** spot at z.
2. If solid(b1): next i.
3. Add a **kind 0** spot at z.
4. If b0 is "ground" (0x31aec0: type ≠ 0, flag 8 clear, and type not in {2, 3, 7, 8, 11, 17,
   19} (types below 20 whose bit is set in 0xa098c)) and there is room above (0x31ad50, below),
   then by b0:
   * type 18 → **kind 5**;
   * type 23..30 → **kind type − 16** (7..14);
   * else b1 flag 0x20 → **kind 6**;
   * else b1 flag 2 → **kind 2**;
   * else if z > 0 and b0 flag 2 clear: **kind 1**; then also **kind 16** if b0 type is 4, 5, 9,
     10 or 14 (mask 0x4630) and b1 type is not 2; and **kind 17** if b0 type is 6.

**Room above** (0x31ad50 with step 1) at (X, Y, z): for dx, dy in −2..2, with m = max(|dx|,
|dy|): the blocks at z + m + k for k = 0, 1, 2, and at z + k for k = m..7, must all be not solid
(type & 0xfd = 0), read through the zone's block lookup (grey solid outside the zone or under a
column's stored blocks).

Then the working map (zone+0x110) becomes a copy of the spot map (0x2f9ef0). The generator
copies it again after the region roles' things (zone file §1), so the cell pass sees every spot
again.

### 5.2 Count (0x1299a0)

count(cx, cy, kind) = the number of positions in the **spot map** (+0x100, not the working
copy) under that key; 0 when absent. No draws.

### 5.3 Take a spot (0x31bd90)

take(cx, cy, kind):

1. Kind 3: the first static object of type 0x75 (117) in the zone's object list: its position
   with z + trunc((scale × base size z of type 0x75) × 65536.0) (sizes from 0x13a520 via
   0x13a960, single precision). No draw. (If none exists, continue with step 2.)
2. Look up the key in the **working map**. If the key is present and its list is empty, refill
   the list with a copy of the spot map's list for the key. If the list is not empty: i =
   rand % size; return position i and remove it (the rest keep their order).
3. Otherwise (key absent, or still empty) no draw: the zone's middle column, x = (64·zx + 32)
   ·65536 + 32768, y likewise, z = (stored base + stored count of that column)·65536.

### 5.4 Cell spot (0x31c0e0)

cellSpot(cx, cy): the kind is the first of 2, 7, 16, 1 whose count (§5.2) is not zero, else 0;
then take(cx, cy, kind).

### 5.5 Nearest spot (0x2fe700)

nearest(cx, cy, kind): if the working map has a non-empty list under the key, return (without
removing) the position with the smallest d = dy·dy + dx·dx, where dx = (float)((cx·16+8)·65536
− x) × (1/65536) and dy likewise (single precision; the first strictly smaller wins). Else
take(cx, cy, kind).

---

## 6. Region-role things, cells, refresh

See `worldgen-creatures-zone.md` (the order of the passes, the cell pass, townsfolk, wild
groups, castles, dungeons, animals, the marking pass, the daily refresh) and
`worldgen-creatures-roles.md` (0x11a820).

---

## 7. Residents (0x2e17d0) and arena stages

These run in the world update, not in generation; they define what the spawn point fields
mean.

### 7.1 Which spawn points become creatures

Unless world+0x358 is set: for every player (a creature of hostility 0), every zone whose
cell lies within ±256 blocks (0x1000000 dots, zones = floor(blocks / 64)) of the player is
listed once. The set is ordered by unsigned Zone-pointer value; process it in
that order, not coordinate or first-insertion order. Exact collection, admission,
and random-stream details are in [steam-authoritative-residents.md](steam-authoritative-residents.md).

1. Run the daily refresh (zone file §4).
2. For each spawn point in id order: skip if a creature with its id lives (world+0x450) or its
   id is in the world's dead set (world+0x20). It must be within reach of some player
   (0xf3e20: |dx| ≤ 256 and |dy| ≤ 256 blocks and dx² + dy² + dz² ≤ 256², in double, dots ×
   1/65536). Then all of:
   * **requiredDefeated** (+0x920) ≠ 0: sum, over zones (zx−8..zx+8, zy−8..zy+8), the zone's
     count of dead tracked creatures (+0x490 of a loaded zone, else the world's saved count for
     that zone); the sum must be ≥ the field.
   * **cells** (+0x928): the nonempty native tree gate is specified in
     [steam-resident-effects-prerequisites.md §§1–2](steam-resident-effects-prerequisites.md).
     It has a retained-byte quirk and is not an all-cells-cleared test. No filling
     generation producer has been identified.
   * **requiredStage** (+0x948) ≠ 0: some registered creature of class 0xa1 (an arena master),
     without an HP test, must
     have stage byte (+0x19c) equal to it.
   * **prerequisites** (+0x938): every id must be in the dead set.
3. The creature (0x15f8 bytes, 0x4ce80 with the id; its defaults in §7.3) takes from the spawn point: home
   (+0x1b8) and position (+0x10) = its position; home cell (+0x1d0) = (floor(x/16),
   floor(y/16)) of its blocks; yaw (+0x30); hostility (+0x60, a byte); race (+0x64); people
   (+0x970, byte); +0x974 = +0x2c; style (+0x978, zero-extended); level (+0x144); class and
   spec (+0x14c, +0x14d); stage (+0x19c) = +0x4c; appearance (+0x78) = +0x50; name (+0x958..
   +0x967) = +0x100..+0x10f; equipment (+0x278); inventory (+0x9f0); +0xa08 = +0x808; the item
   at +0xa0c = +0x80c; +0xaac = +0x8ac; +0xab0 = +0x8b0; roam (+0xbcc) = +0x8b8; abilities
   (+0xbd0); pick (+0xbe8); second abilities (+0xbf0); owner (+0x1a0) = +0x40; home place
   (+0x1b0, +0x1b4) = +0x8f8, +0x8fc; region (+0x150) = regionAt of its position; +0x180 =
   0x5fac0(creature); mode (+0x68) = +0x8bc; group (+0xc08) = +0x910.
4. Mode 0x8a: raw z += (rand % 50 + 80) blocks, using the executing thread's persistent
   CRT stream after any daily refresh draws; home z is retained. This pass does not seed
   or restore the stream. Incoming state is required for exact reproduction (resident
   supplement §5).
5. If appearance flag 0x4000000 is set: when the creature's region is one where a player has
   lifted the demon portal (player +0x128 bit 0x800 marks that player's region), the flag is
   cleared; otherwise hostility = 1, the size (+0x84, +0x88, +0x8c) × 1.1, and flag 0x8000 is
   cleared.
6. Add the creature.

After each zone's complete spawn-map traversal, mode0x8a bookkeeping can request one
kind9 effect. Its day-derived direction, suppression, exact record and live/outgoing
append order are specified in [steam-resident-effects-prerequisites.md §§3–6](steam-resident-effects-prerequisites.md).

(Player bit 0x8000 likewise frees the petrified, flag 0x400000, in that player's region;
this is the final World-list pass inside 0x2e17d0, including new residents, without an HP
test. See the resident supplement §6.)

### 7.2 Arena stages (in 0x2e3510)

For every creature of class 0xa1 whose stage byte (+0x19c) is not 0xff: for the zones
covering its home ± 64 blocks (0x400000 dots), each loaded zone:

1. Build a map stage → done: for every spawn point with requiredStage s ≠ 0 whose id is dead,
   done[s] = true; then for every such point whose id is not dead, done[s] = false. last =
   the largest requiredStage of the zone's points (0 if none).
2. For each stage s with done[s] true (ascending): stage byte = max(stage byte, s + 1).
3. If last ≠ 0 and last < stage byte: stage byte = 0xff (finished).

The arena's teams (roles file §3.11) carry requiredStage 1, 3, 5, 7. The only creature of class
0xa1 is the arena master of cell object 97 (zone file §5); its spawn point's stage byte (+0x4c)
is 0, so a new master starts at stage 0 and its first team (requiredStage 1) waits for nothing
but the master's presence.

### 7.3 The creature record's defaults (0x4ce80)

0x4ce80 (with 0x4d600 for the block from +0x10, the appearance constructor 0x4d1f0 and the
equipment constructor 0x4d300) makes a 0x15f8-byte creature with the given id. No draws. The
fields that matter to generation and the resident pass, with the meaning other specs give them:

| Offset | Default | Meaning |
|---|---|---|
| +0x08 | the id | id |
| +0x10 … +0x5f | 0 | position (+0x10, +0x18, +0x20) and the motion fields after it; yaw +0x30 |
| +0x60 | 3 (byte) | hostility |
| +0x64 | 0 | race |
| +0x68 | 0 (byte) | mode |
| +0x6c … +0x77 | 0 | |
| +0x78 | 0x4d1f0 | appearance (0xb0 bytes); flags at +0x80 |
| +0x128 | 0 (u16) | state flags |
| +0x12c | 0 | |
| +0x130 | −3000.0 | stun timer |
| +0x134 … +0x143 | 0 | |
| +0x144 | 1 | level (i32; +0x148 = 0) |
| +0x14c, +0x14d | 0 | class, specialisation |
| +0x150 … +0x17f | 0 | region (+0x150, +0x154) and others |
| +0x180 | 500.0 | health |
| +0x184 | 1.0 | block power |
| +0x188, +0x18c, +0x190 | 0 | mana and two more |
| +0x194 | 1.0 | ground grip |
| +0x198 | 10.0 | light radius |
| +0x19c, +0x19d | 0 | stage byte and the next |
| +0x1a0, +0x1a8 | 0 | owner, and a second creature id |
| +0x1b0, +0x1b4 | 0x7fffffff | home place |
| +0x1b8 … +0x1cf | not written | home position (the resident pass sets it) |
| +0x1d0, +0x1d4 | −1, −1 | home cell |
| +0x1d8 | empty item (0xa0 bytes, all 0) | item in use |
| +0x278 | 0x4d300 | equipment (11 slots) |
| +0x958 … +0x967 | 0 | name |
| +0x970 | 0 (byte) | people |
| +0x974, +0x978 | 0 | (+0x974 from the spawn point's +0x2c), style |
| +0x980 | empty list | buffs |
| +0x9a0 | 1.0 | stamina |
| +0x9a4 | 1.0 | (raised while a pet is wet) |
| +0x9b8, +0x9c8 | empty maps | |
| +0x9d8 | 0 | id of its pet |
| +0x9e0 | 0 | last creature hit or hit by |
| +0x9f0 | empty | inventory (§4.5: no tabs, +0xa08 = 0, item +0xa0c empty, counters +0xaac, +0xab0 = 0) |
| +0xbcc | 0 | AI flags (the spawn point's roam) |
| +0xbd0 | empty | abilities |
| +0xbe8 | 1 | ability pick / summon mode |
| +0xbf0 | empty | second abilities |
| +0xc08 | empty set | group |
| +0x1530 | 200.0 | |
| +0x1544 | 0 | (the pet bookkeeping's copy of +0x190) |
| +0x1548 | empty item | (the pet bookkeeping's copy of the pet item +0x8b8) |

**The appearance block's defaults (0x4d1f0).** The same constructor fills the spawn point's
appearance (+0x50) and the creature's (+0x78). It draws nothing and writes all 0xb0 bytes except
+5..+7, which it leaves as they were (padding; the .spawns test ignores them). Offsets are from
the start of the block; the meanings are those of creature-pose.md §2 (creature offset = block
offset + 0x78). Floats are single precision, given with their bit patterns.

| Block offset | Default | Meaning |
|---|---|---|
| +0x00, +0x01 | 0, 0 (bytes) | |
| +0x02, +0x03, +0x04 | 0xff ×3 (bytes) | hair colour (white until looks writes it) |
| +0x05 … +0x07 | not written | padding |
| +0x08 | 0 (u32) | appearance flags |
| +0x0c, +0x10, +0x14 | 1.0 ×3 (0x3f800000) | size: width, depth, height |
| +0x18 … +0x26 | −1 ×8 (i16, 0xffff) | model ids: head, hair, hands, feet, chest, tail, arm part, wings (none) |
| +0x28 | 1.01 (0x3f8147ae) | head scale |
| +0x2c | 1.0 | chest scale |
| +0x30 | 1.0 | hand scale |
| +0x34 | 0.98 (0x3f7ae148) | foot scale |
| +0x38 | 1.0 | arm-part scale |
| +0x3c | 0.95 (0x3f733333) | weapon scale |
| +0x40 | 0.8 (0x3f4ccccd) | tail scale |
| +0x44 | 1.0 | shoulder scale |
| +0x48 | 1.0 | wing scale |
| +0x4c | 0 | body pitch |
| +0x50, +0x54, +0x58 | 0, 0, 0 | resting hand turn |
| +0x5c, +0x60, +0x64 | 0, 0, 0 | foot, wing and tail pitch |
| +0x68, +0x6c, +0x70 | 0, 0, −5.0 (0xc0a00000) | chest place |
| +0x74, +0x78, +0x7c | 0, 0.5 (0x3f000000), 5.0 (0x40a00000) | head place |
| +0x80, +0x84, +0x88 | 6.0 (0x40c00000), 0, 0 | hand place |
| +0x8c, +0x90, +0x94 | 3.0 (0x40400000), 1.0, −10.5 (0xc1280000) | foot place |
| +0x98, +0x9c, +0xa0 | 0, −8.0 (0xc1000000), 2.0 (0x40000000) | tail place |
| +0xa4, +0xa8, +0xac | 0, 0, 0 | wing place |

Looks (§4.2) overwrites what each race sets; a race that leaves a field alone keeps these
values (looks.bin, §8.3, shows the result per race). Source: 0x4d1f0, 0x14004d1f0–0x14004d2f3
in the assembly (one store per field, no branches).

Everything else the constructor touches is zero or an empty container. The resident pass
(§7.1 step 3) overwrites hostility, race, mode, appearance, level, class, spec, region, health
(+0x180 = 0x5fac0), stage, owner, home place, home position and cell, equipment, name, people,
+0x974, style, inventory, AI flags, abilities, pick, second abilities and group, so a resident
keeps the defaults only for the other fields (stun timer −3000.0, block power 1.0, ground grip
1.0, light radius 10.0, stamina 1.0, and so on).

### 7.4 Pets (0x2b67b0)

0x2b67b0 (world W, owner id o, creature C) makes C into o's pet. It is called in the world
update, never by generation: by the player pet pass 0x2e0db0 (a player whose pet slot,
equipment slot 10 at +0x8b8, holds a category 20 item and who has no living pet gets a new
creature from 0x4ce80 with the next free id below the smallest, hostility 6, race = the item's
id +0x8bc, the player's position and people, added to the world before the call; the call is
made again when the item changes), and by taming (creature-abilities-ranged.md §14.2). That
pass is not part of this spec. In order:

1. srand(C's race, +0x64, as an unsigned number).
2. Looks (0x53e80, §4.2) with C's race, C's appearance (+0x78), no looks record, wild 0. Its
   flags are ORed into C's appearance flags (+0x80).
3. Gear (0x52c40 → 0x511e0, §4.3) with C's hostility byte (+0x60), race (+0x64), level
   (+0x144), appearance flags (+0x80) and people byte (+0x970), writing C's class (+0x14c),
   specialisation (+0x14d), equipment (+0x278), abilities (+0xbd0), pick (+0xbe8), second
   abilities (+0xbf0) and inventory (+0x9f0).
4. C's owner (+0x1a0) = o; C's AI flags (+0xbcc) = 0x80.
5. Look o up in the world's creature map (W+0x450, keyed by id). If found (and not null), the
   owner's pet id (+0x9d8) = C's id (+0x08).

Because of step 1 every pet of a race draws the same numbers, so a race's pet always has the
same looks and gear (for the same hostility, level, flags, people and class). The caller sets
C's race, hostility, level and flags before the call; 0x2b67b0 does not touch C's position,
health or id.

---

## 8. Oracles

All are written by tools/ZoneExtractor from the running game and checked by
tests/EngineTests.cpp. The repository holds none of them.

### 8.1 regions.txt (`EngineTests rolesoracle [show]`)

0x115df0 run for every region an extracted zone lies in. Lines:
`region <rx> <ry>`, then `map <offset hex> <x> <y> <v> <a> <b>` for every node of the maps at
+0x10, +0x20, +0x60, +0x70 (x, y the key, v the int at node +0x24, a and b the 8 bytes after),
then `special <x> <y>`, `marked <byte>`, `items <ints…>`. The test compares the role map
(offset 10), the level map (60), special, marked and the item list with the implementation's,
region by region.

### 8.2 z_<x>_<y>.spawns (`EngineTests allspawns [show]`)

"CWS1", u32 count, then per spawn point its raw 0x950 bytes followed by two int vectors (u32
count, then the ints): +0x8c0 and +0x8e0. Found in Extract/oracle (zones loaded in a session)
and in the generated sets (gen_seed_123, gen2…, genr/genb: refreshed, with day.txt; run with
CW_ZONES_DIR and CW_DAY, CW_REFRESH_DAY). The test pairs each game point with ours by position
and race, then compares hostility, level, flags, people and style, roam, mode, home, yaw,
class and spec, the appearance (bytes +5..+7 of it ignored: padding), and every equipment
slot's category, id, modifier, material and level.

### 8.3 Creature tables

* spawn-table.txt: `values <306 ints>` (the race levels), then `<index> <kind> <climate mask>
  <ground mask> <races…>` per group.
* races.txt: `<race> <list…>` for every selector −200..400 and −99991 (0x13f0d0).
* pickers.txt: `<rva> <seed> <draws> <race>`: each race picker of §3.3 run 600 times on
  scripted rand.
* looks.bin: "CWL1", then records: i32 race, u32 seed, u32 draws, i32 race after, 0xb0 bytes
  (the spawn point from +0x50) — 24 per race 0..320, made with scripted rand.
* looks-wild.txt: `<race> <draws> <flags>` with the wild argument set.

### 8.4 gear.bin (`EngineTests gearoracle <file> [show]`)

"CWG2", then 30,000 records: i32 hostility, race, level, people; u32 flags; u8 class in; u32
seed; u32 draws made; u8 class, u8 spec; 0x6e0 bytes of equipment; the abilities vector (u32
count + ints); i32 pick; the second vector; u32 tab count, then per tab u32 bytes and the raw
0xa4-byte stacks. The scripted rand is xorshift32 of the seed (13, 17, 5; a zero seed becomes
0x9e3779b9), returning state & 0x7fff. The test reruns the gear with the same draws and
compares the draw count, class, spec, every slot, both ability lists, the pick and the
inventory (tab, quantity, category, id).

### 8.5 Coverage today

CubeVulkan's current transcription (per docs/worldgen.md) matches: the region roles of every
region in regions.txt; 30,000 of 30,000 gear records; every spawn point of the 271 generated
zones (refreshed and not) field by field. The dumped zones of Extract/oracle (2299 .spawns
files today) also hold points made by the game after generation (refreshes on later days). The
implementation from this spec must do at least as well.

---

## 9. Open questions

1. **Zones without a place.** 0x31a5d0 leaves the place centre unwritten when the cell has no
   place (kind 0); 0x2ff6f0 still computes the culture and builds the planner from regionAt of
   that centre, which is stack garbage. It matters only through the planner's marked byte
   (zone file §3) and 0x11a820 (a kind-0 cell has no role, so nothing). CubeVulkan uses centre
   (0, 0); the oracle zones agree, but the game's value is undefined.
2. **The herb record's first position** (zone file §2.6) is likewise stack data on the first
   use in a zone.
3. **+0x928 cells:** the consumer in 0x2e17d0 is now traced in
   [steam-resident-effects-prerequisites.md §§1–2](steam-resident-effects-prerequisites.md).
   The former all-cells/objects-gone interpretation was incorrect: a retained-byte
   quirk makes the first cell decisive, and only indexed nonzero-state objects match.
   A filling producer remains unidentified in generation.
4. **Shoppers past the counter list** (zone file §4.2) read past the end of zone+0x500, which
   is heap memory: genr_seed_123 zone (−18, −18) has one counter and two shoppers, and the
   second stands at about (−1.2·10^14, −4.3·10^9, −49152) blocks. No value can be specified;
   implementations use the last counter, and the .spawns test cannot pair that point.
5. **Answered.** Arena masters (class 0xa1) are made by B's cell loop for cell object 97 (zone
   file §5). Spawn point +0x4c is never set by generation, so a master starts at stage 0.
6. **Game data stays out of the repo** (decided). The groups, levels, sibling lists and
   picker lists are read from Extract/oracle at run time. The extraction tool records the
   pickers only as sampled runs (pickers.txt), and three pickers (0x13fda0, 0x140490,
   0x141d70) not at all; the tool should export the picker lists directly (and CubeVulkan's
   current source hard-codes them, which this decision also covers).
7. The fixed per-race appearance values (looks.bin) are data, not specified here; only the
   draws and the rules around them are.
8. **0x53e80's per-race cases** (41 KB) were checked against looks.bin and spot-checked in the
   decompile, not read case by case; tables §T7 should be confirmed race by race.
9. **Builders read through CubeVulkan's transcription.** The planner, the cell pass, wild
   groups, castles, dungeons, gear, the refresh, the resident pass and the encounter dispatch
   were read in the decompile or assembly. The individual encounter builders (roles file §3.1
   –§3.11, §4) were confirmed by their constants, seeds, flags and list contents in the
   decompile and by the oracle's 418 matching role spawn points, but not line by line; the
   holder/guard inventory and +0x40 links found there (§3.1 step 7, §3.10 step 4) are not
   checked by any oracle (the .spawns test ignores +0x40 and the inventory).
10. **The dungeon cell D when no floor is found** (zone file §1) is a stack slot the zone
    generator never writes; its value is whatever the stack held. Only the area (+0x900) of
    dungeon floor guards uses it, and no oracle records area contents (the .spawns files hold
    only the list's pointers and size). Implementations use (0, 0).

## Errata (from the clean-room implementation, checked against the oracles)

- **Fixed (§5.1).** The ground-type list read {2, 3, 7, 8, 12, 13, 15, 17, 19}, which
  contradicts the mask 0xa098c of 0x31aec0; the mask gives {2, 3, 7, 8, 11, 17, 19}, as the
  structures spec has it.
- **Fixed (§9 question 5, zone file §5).** Cell object 97 is the arena master's dais, not a
  shrine. Its creature (class 0xa1, race variant(0x104), hostility 3, people 3, yaw 270, roam 4,
  level 0) was unspecified; gen_seed_123 zone (−152, 41) has it as the zone's first id, at
  (16cx + 8, 16cy + 8, 16), flags 0x20000, and its .rtrace opens with its 32 draws (variant,
  five looks draws, one consumables draw, four armour and three jewel draws, 18 modifier
  draws).
- **Added (§4.5, §7.3, §7.4).** The inventory 0x4e810 (gear.bin: category 1 goes to tab 2),
  the creature constructor's defaults 0x4ce80 and pet making 0x2b67b0 had no spec.
- §7.3 (Added): the animation state's size factor (+0xc38 + 0x5b8) is 1.0 after construction, not zero. All 18,300 recorded pose calls (Extract/oracle/anim*.bin, cwa2) hold 1.0 there. Zero leaves every posed part without size. This also answers creature-pose.md open question 1.
