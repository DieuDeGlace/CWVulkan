# World generator, part B: structures and zone contents

Behavioural specification of how Steam Cube World fills a zone with structures and things, after
its terrain columns: the cell objects of every 16-block cell (trees, rocks, hills, houses, towns,
castles, dungeons, towers, fields, landmarks), the blocks they write, the static objects
(zone +0x60) and the flag 0x40 they leave in blocks, the models (zone +0x18), the items lying
about (zone +0x30), and the chests and crates with their rewards. Written from the decompiled
executable, its assembly (the zone generator 0x2ff6f0 has no pseudocode; its 22,520 instructions
were read directly) and the binary's constants. Routines are cited by RVA (image base
0x140000000).

This part is one of three:

| Part | File | What |
|---|---|---|
| A | `worldgen-terrain.md` | regions, biomes, places, heights, the column blocks, the world set-up and its random numbers, the light pass |
| **B** | **this file and the four below** | **structures and zone contents** |
| C | `worldgen-creatures.md` | creatures, spawn points, the region planner (0x115df0) and its roles, encounters |

Part B's files:

| File | Contents |
|---|---|
| `worldgen-structures.md` (this) | conventions, interfaces, the generator's order, the block store, the records, the zone's place, the cell objects (0x69c60), the cell loop, oracles, open questions |
| `worldgen-structures-nature.md` | trees, leaves, rocks, boulders, hills, canyons, stumps, logs, the mound, the sunken chest |
| `worldgen-structures-mason.md` | the building class (0xba9a0): frames, colour schemes, patterns, write modes and the shapes it draws |
| `worldgen-structures-buildings.md` | building bases, joins, the building builder (walls, floors, doors, windows, stairs, rooms, furniture, roofs), castles, fortresses, the wizard's tower and the other set pieces, crop fields, street lamps |
| `worldgen-structures-contents.md` | spots, the cells' chests, crates and lore objects, plants, items lying about, the final passes (flag 0x40, light and treasure lists), the daily refresh's objects |
| `worldgen-structures-tables.md` | per-type tables, object sizes, colours, the town layouts |

The realm of a place, its lore and its artifacts are in `lore-and-artifacts.md` (§4.3 the
realm, which this part calls the place's **style**; §1.4 the static object record; §2.2 the lore
object types; §5 the artifacts). Item generation from a reward record is in
`drops-and-items.md`. Neither is repeated here.

---

## 0. Conventions

* **Floats.** All arithmetic is IEEE single precision (SSE, round to nearest, no fused
  multiply-add) unless a step says "in double". Each operation is rounded to float before the
  next, in the order written; "a × b × c" means (a × b) × c. Literals written as decimals are
  the float nearest to them unless a hex pattern is given; 0.1 is 0x3dcccccd, 0.15
  0x3e19999a, 0.3 0x3e99999a, 0.35 0x3eb33333, 0.7 0x3f333333, 0.8 0x3f4ccccd, 0.85 0x3f59999a,
  1.2 0x3f99999a, 1.3 0x3fa66666, 0.499 0x3eff7cee, 0.999 0x3f7fbe77. 1/65536 is
  1.52587890625e-05 exactly. Comparisons are written as the game tests them ("unless a > b"
  differs from "if a ≤ b" for NaN); min and max follow the game's minss/maxss order: min(a, b)
  is "a < b ? a : b", max(a, b) is "a > b ? a : b".
* **Conversions.** "trunc" is conversion toward zero (cvttss2si; 32-bit unless "trunc64"). A
  value out of range gives the integer indefinite value. "floor" of a float is the game's
  truncate-and-adjust (trunc, then minus 1 when the value is negative and not whole). "Float of"
  an integer is exact for 32-bit values.
* **Integers.** 32-bit two's complement with wrap-around unless stated. "a / n" and "a mod n"
  are C's (truncating) unless written ⌊a / n⌋ or "posmod", which are floor division and the
  non-negative remainder. ⌊a / 2^k⌋ is an arithmetic shift.
* **Positions.** World positions in records are 64-bit integers in 1/65536 of a block ("dots").
  "Dots of a float f" is trunc64(f × 65536.0) computed in float. "Blocks of dots" is the floor
  (arithmetic shift by 16) unless a step says "toward zero" (division by 65536 truncating).
* **Hashes.** hash(n) and rnd(n) = 100000 − hash(n) / 21474 are the game's integer hash and its
  random number (worldgen-terrain.md). The **cell hash** H(x, y) is y × 57 + x (32-bit wrap), and
  R(x, y, k) = rnd(H(x, y) + k). Where the game tests rnd & 1, & 3 or & 7 it computes it through
  a signed multiply by a negative magic number; since 100000 is a multiple of 8 the result is
  the same as rnd & 1, & 3 or & 7, and the spec writes it so.
* **Xorshift.** X(v) is the 32-bit unsigned xorshift v ^= v << 13; v ^= v >> 17; v ^= v << 5.
  xs(n) is X(n). The **two-step** form X2(v) is v ^= v << 13; v ^= v >> 17 (no third step).
* **rand.** rand() is the C runtime's (MSVC LCG: state = state × 214013 + 2531011, result
  (state >> 16) & 0x7fff), imported through the slot at 0x42cd28, srand through 0x42cd38. It is
  one stream shared by everything in the zone generator; every draw is listed in order. A draw
  used as "rand mod n" is the C remainder (rand ≥ 0, so plain). "rand fraction × A" means
  (float of rand × A) / 32767.0, then any addend; "rand angle" means float of
  (double of rand × 6.283185307179586 / 32767.0).
* **Noise.** S(x, y) is the game's 2D simplex noise taking doubles and returning a float
  (worldgen-terrain.md). Its arguments are written as computed: "S(x × 0.05 + 9843, …)" means
  the double product and sum of the integer converted to double.
* **Colours.** Builders work in float colours where 255 is a full channel and values above 255
  are allowed. **Packing** a colour c (0xba820 and the inline copies) stores each channel as
  trunc64 of the clamp of c × 0.5 to [0, 255] (tested "0 > v → 0, else v > 255 → 255"),
  truncated to a byte.
* **Blocks.** A block is 6 bytes: r, g, b, a fourth byte (light or "glow"; 255 on lava and
  poison), type, flags. A block is **solid** when type & 0xfd is not 0 (air 0 and water 2 are
  open). Flags: 1 snow and leaves, 2 protected (built), 4 under objects 3–5, 8 floor step,
  0x10 sunk ground, 0x20 castle space, 0x40 inside a static object, 0x80 inside object 15.

## 1. Interfaces

### 1.1 What part B takes from part A (`worldgen-terrain.md`)

| What | In → out |
|---|---|
| place at a point, 0x35ce0 | (x, y) → kind, big flag, variant, radius, centre (x, y), squared distance, weight |
| zone place, 0x31a5d0 / 0x31a690 | place cell (x, y) → the cell's landmark, no distance cut-off |
| big cell 0x36160, landmark point 0x362a0, place kind 0x2b99d0, warped distance 0x364e0 | as named in part A |
| place weight 0x9bee0 | (kind, big, d²) → 0 for kind 0; else with r² = 250000 (big) or 62500: 0 when d² ≥ r², else (1 − d² / r²)² |
| region at 0x2ada20, neighbourhood 0x358e0, biome 0x2b9080 | point → region; point → the 3×3 regions; region → biome 0–13 |
| region weight of a biome set (the helper used by 0x2adeb0 and the others) | (neighbourhood, set, scale) → 0..1; the scales are 0.0004 (0x39d1b717) and 0.0003 |
| ground height 0x35ea0 | (x, y, mode) → float |
| base height 0x2a0840 | (neighbourhood, kind, weight) → float |
| roads 0x2d4730, river 0x2e5f80, cliffs 0x2a2c50 | as in part A |
| warmth 0x2d9b60, dryness 0x2b5960, lava weight 0x2b9750, forest weight 0x2d9f30, kind-27 weight 0x2abb60, snow 0x2b4320 | as in part A |
| colours: grass 0x2b40b0, rock 0x2d3150, ground 0x2ab410, surface 0x2a3e30 (0xbbfe0 at a point), buried 0x2a3140 (0x2f39e0 at a point), river bed 0xbd110 | as in part A |
| block of a column 0x2d8e00 | (x, y, z, column values) → block |
| the column values of a zone column (the 0x58-byte field) | stored per column by the column loop |
| the world's town layouts and random numbers (world +0x4a0, +0x224..+0x357) | set by 0x2b5f9a; the layouts are in tables T10 |

### 1.2 What part B takes from part C (`worldgen-creatures.md`)

| What | In → out |
|---|---|
| the region planner 0x115df0 | region → each place's role (0x93480) and level, the special town, the marked flag, the item list (+0x90) |
| the planner's item for a cell, 0x12a600 | (planner, cell) → an entry of the item list chosen by R(cx, cy, 0x89d85) mod its size, or −1 when empty |
| a role's own treasure, 0x131540 | (planner, place cell) → an item (category 0 for none) |
| the arena master of cell object 97 (`worldgen-creatures-zone.md` §5) | (cell, base height of 97) → one spawn point added, 32 draws from the cell's stream, right after the dais (buildings file §12) |
| creatures of a cell, of castles, dungeons, encounters, region roles, the daily refresh | they draw from the same rand() stream; their places in the order are given in §2 and in the contents file |

Part C takes from part B: the spot map (contents file §C1, zone +0x100/+0x110), the static
object list (encounters add their own objects), the cell objects (§6), the castle cells
(buildings file §9) and the building tests (tables T7).

### 1.3 CubeVulkan's own options

The world scale stretch and the mods' hooks plug in at part A's region layer, at the place kind
(0x2b99d0), at the planner's role (0x93480) and at the items laid down by roles and regions
(contents file §C4, §C6). They are not the game's and are not specified.

## 2. The zone generator and its order

A zone is 64 × 64 columns, zone (zx, zy) covering blocks [64 zx, 64 zx + 64) on each axis, and
4 × 4 cells of 16 blocks. Cell (cx, cy) covers [16 cx, 16 cx + 16).

0x2ae0c0 makes a zone: srand((zy << 10) + world seed + zx), the seed being world +0x244 (part
A), then the generator 0x2ff6f0, which runs these steps in order on one rand() stream:

1. **The zone's place** (§5): 0x31a5d0 for place cell (⌊64 zx / 1024⌋, ⌊64 zy / 1024⌋), rounded
   down to even for a big place; for place kind 8 the dungeon cell scan (part C); the big
   structure and anchor; the style, people race and biome; the region planner of the place's
   region (part C).
2. **The columns** (part A): for each column x then y, its values and terrain blocks. The
   overhang lights of the cliff pass draw rand() here and add model 0x59 (part A gives the
   condition; the model record is §4.2 with size the gap's height and yaw 1.0).
3. **The cell loop** (§7): every cell of the 8 × 8 around the zone builds its object.
4. **Street lamps** (buildings file §13), no draws.
5. **Spots** (contents file §C1): two draws per column pair.
6. **The cells' things** (contents file §C3): srand per cell.
7. **Plants** (contents file §C2), drawing on from step 6. At its end, without draws, the
   light list (+0x90) and the sound emitter list (+0xa8) are rebuilt from the models (contents file §C2.1).
8. **The region role's things** (0x11a820; contents file §C4 and part C).
9. **The cells' creatures and items** (0x3171a2; contents file §C5): srand per cell; part C's
   creatures interleaved with part B's items.
10. **The marked-region pass** (part C): spawn point flags, drawing on.
11. **Copies and re-stamping** (contents file §C7): the objects are copied to zone +0x78, the
    items to zone +0x48, then the items of some categories get the region of their position.
12. **Lists** 0x31d470 (light objects, treasure positions) and **marking** 0x31d700 (flag
    0x40), contents file §C7 and §C8.

The daily refresh (0x31c540) runs later, on load and on a new day (contents file §C9 and part
C).

## 3. The block store

Each zone column keeps a base height and a vector of stored blocks from it up (part A). Two
routines read and write it:

* **Lookup** 0xeb90 (x, y, z): outside the zone, or below the column's base: a solid grey block
  (200, 200, 200, 0, type 1, flags 0). At or above the top of the stored blocks: air
  (255, 255, 255, 0, 0, 0), or sea water (255, 255, 255, 0, type 2, 0) when z ≤ 0. A stored
  block of type 0 at z ≤ 0 without flag 2 reads as sea water. Otherwise the stored block.
* **Store** 0x31c1c0 (x, y, z, block): ignored outside the zone. An air block (type 0) with
  flags 0 at or above the top is not stored. When the column has no blocks the base becomes z.
  At z ≤ 0 a type-0 block without flag 2 is replaced by sea water (255, 255, 255, 0, 2, 0). Below
  the base the column grows down: the new block at z and every block between z + 1 and the old
  base − 1 is the buried colour of that height (part A, 0x2a3140 with the column's values,
  packed). Above the top the vector grows with zero blocks to z, and the block is set.

Builders call these with world coordinates; most test "inside the zone" first, so a structure
in a neighbouring cell writes only its part in this zone. Several builders refuse to overwrite a
protected block (flag 2) or blocks with flag 0x20 or 0x22; each says so.

## 4. Records

### 4.1 Static objects (zone +0x60, 0xe0 bytes)

Made from 0x2f93a0, which sets scale (+0x24) 1.0, +0x2c 0.0, owner (+0x30) 0, active byte
(+0x28) 1, the item it takes (+0x38, 0xa0 bytes) empty, item count (+0xd8) 1. The generator then
sets the type (+0x00, i32), position (+0x08, +0x10, +0x18, i64 dots, z at the object's foot),
turn (+0x20, a byte of quarter turns 0–3) and, where said, the scale and +0x2c (1000.0 for the
doors, window panes and beam objects of buildings and the wizard tower's doors). The turn byte is
not initialised by 0x2f93a0; every placement sets it or leaves it from the record's previous use
on the stack (each such case is noted).

Ways an object enters the list:

* **Keep if inside** 0x1194b0: appended (0x129b50) when x is in [64 zx·65536, (64 zx + 64)·65536)
  and y likewise; otherwise dropped.
* **Settle, then keep** 0x31b2d0 then 0x1194b0 (contents file §C8).
* A few placements append directly (0x129b50) after their own test; each says so.

**Rewards.** A reward chest's item is not in the object: 0x31c4e0 counts the list, and the item
is stored in the map at zone +0x450 under the key (count − 1), the index the chest got. The
reward item is an ordinary item record (drops-and-items.md §0.1): category +0x00, id +0x04,
modifier +0x08, region +0x0c/+0x10, rarity +0x14.

### 4.2 Models (zone +0x18, 0x40 bytes)

| Offset | Meaning |
|---|---|
| +0x00 | model type (i32) |
| +0x08, +0x10, +0x18 | position (i64 dots) |
| +0x20 | f32, a size (several placements never write it: noted) |
| +0x24 | f32, yaw in degrees (the overhang light and the lamp chain use it otherwise) |
| +0x2c, +0x30, +0x34 | f32 tint r, g, b |
| +0x38 | i32 flags: 1 gives light, 2, 4 (the plants use 2, 4 and 6) |

**Keep if inside** 0x2fb7d0 (and its inline copies): appended (0x69b30) only when x and y lie in
the zone, as for objects. Some placements append without the test; each says so.

### 4.3 Items lying about (zone +0x30, 0xe0 bytes)

Made from 0x118c20: item empty, +0xb8 −1 (i64), +0xc4 0, +0xc8 0 (8 bytes), +0xd0 −1, +0xd4 1,
+0xd8 0; the position (+0xa0, +0xa8, +0xb0) and angle (+0xc0, f32 degrees) are not initialised
(see the herb quirk, contents file §C5). The generator sets the item's category, id and the rest,
the position and the angle, and for treasures sets bit 1 of +0xc4 (the same bit
drops-and-items.md §0.4 sets for categories 23 and 24). Appended with 0x129c00 (copy through
0x3aee0); there is no inside test, every item placed is kept.

## 5. The zone's place

Computed once at the start of 0x2ff6f0 and used by many builders:

* **Place** P: 0x31a5d0 at place cell (px, py) = (⌊64 zx / 1024⌋, ⌊64 zy / 1024⌋): big flag
  0x36160 (px, py); kind 0x2b99d0 (px, py, big); when the kind is not 0, radius 500 (big) or 250,
  centre 0x362a0 (px, py) and variant 0x2abb90. With kind 0 the centre is left as it was (see
  open questions). When big, (px, py) become (2⌊px/2⌋, 2⌊py/2⌋); this is the **place cell** C1.
* **Big structure** B = 0x2aa270 (C1, kind, big) and, when B ≠ 0, **anchor** A = 0x2a9df0 (§6.2),
  else A = (0x7fffffff, 0x7fffffff).
* **Style** s = 0x2bb060 (C1, big): the place's realm, lore-and-artifacts.md §4.3.
* **People race** = 0x10cee0 (s) = 12 + xs(s + 0x7b2650) mod 11 (part C uses it).
* **Biome** b = 0x2b9080 (0x2ada20 (P's centre)): the biome of the region of the place's
  centre. Tables T9 turns (s, b) into the building culture.
* **Planner**: 0x115df0 for 0x2ada20 (P's centre) (part C).

## 6. Cell objects (0x69c60)

0x69c60 (cx, cy): the cell point Q = 0x2d8630 (cx, cy); the place at Q, 0x35ce0 (kind k, big,
centre, weight w); then 0x2d53d0 (cx, cy, Q, that place) returns the type, 0 for none. Every
builder asks it again for neighbours; it depends only on the world seed.

### 6.1 The cell point (0x2d8630)

Q = (16 cx + 4 + (R(cx, cy, 0x55bda9) & 7), 16 cy + 4 + (R(cx, cy, 0x9c373) & 7)).

### 6.2 Big structures and anchors

**0x2aa270** (x, y, kind, big): 0 unless big. With (x, y) rounded down to even, h = rnd(H(x, y) +
0x62ad0): kind 5 → 41 when h & 7 = 0; kind 9 → 40 when h & 1 = 0; kind 15 → 43 when h & 1 = 0;
kinds 42, 44, 45, 46, 47 → the kind itself; otherwise 0.

**0x2a9df0** (x, y, kind, big), in cells: rounded to even when big; p = the landmark point
0x362a0 (x, y, big). Without a big structure: (⌊p.x/16⌋, ⌊p.y/16⌋). With one: a = (8⌊p.x/128⌋ + 4,
8⌊p.y/128⌋ + 4); unless kind is 9, 42, 44, 45, 46 or 47 (mask 0xf40000000200), a.x += (R(x, y,
0x8a4d5) mod 5) × 8 − 16 and a.y += (R(x, y, 0x47940) mod 5) × 8 − 16; then each coordinate is
clamped to [64 x, 64 x + 127] (63 when not big) and likewise for y.

### 6.3 0x2d53d0: the order of decisions

c1 = (⌊16 cx / 1024⌋, ⌊16 cy / 1024⌋), rounded down to even when the place is big. The first
rule that returns decides.

**1. Big structure cells.** When B′ = 0x2aa270 (c1, k, big) ≠ 0, with anchor (ax, ay) =
0x2a9df0 (c1, k, big):
* B′ 43: the five cells (ax + i, ay), i = 4, 3, 2, 1, 0, return 71, 72, 73, 74, 75.
* B′ 42: likewise 76–80. B′ 47: 81–85.
* B′ 44 or 41: the 2×2 cells x ∈ {ax − 1, ax}, y ∈ {ay − 1, ay} return 86.
* B′ 45: those cells return 94. B′ 46: 95.
* Anything else goes on to rule 2 (the anchor stays for rule 4).

**2. A place landmark.** When k ≠ 0 and w > 0.5, m = 0x2ba990 (k, c1):
v = R(c1, 0xae13d9) & 3, pick = (R(c1, 0x5708d4) & 1) + 69; kind 10: v 1 → pick, v 2 → 65;
kind 13: v 1 → pick, v 2 → 63; kinds 14, 19, 23, 25, 28, 31: v 1 → pick; otherwise 0. When
m ≠ 0: (mx, my) = (R(c1, 0x890d5) mod 7 − 3 + ⌊centre.x/16⌋, R(c1, 0x10275) mod 7 − 3 +
⌊centre.y/16⌋). The cell (mx, my) returns m; any other cell with (cx − mx)² + (cy − my)² <
(reach(m) + 1)² (tables T6) returns 0.

**3. Towns (k = 5).** (bx, by) = (⌊cx/3⌋, ⌊cy/3⌋); t = the town block 0x2af7b0 (§6.4).
* t = 0: when cx and cy are both even, R(cx, cy, 0x723db) mod 10 = 0 and w > 0.5, return 7 when
  the warmth at Q (neighbourhood at Q, k, w) exceeds 0.75, else 6. Otherwise go on.
* t = 13: return 34 when the forest weight at Q (k, w) > 0.5, else R(bx, by, 0x4f60c1) mod 7 + 27.
* Otherwise, only when posmod(cx, 3) ≠ 2 and posmod(cy, 3) ≠ 2 (the third row and column of a
  block are streets; other cells go on), with the block corner (gx, gy) = 0x2b5760 (bx, by):
  * t = 2: the corner returns 24, other cells 23.
  * t = 11: the corner 91, others 90.
  * t = 10: the corner 92; others return 0.
  * t = 7, 8, 9: u = the cell's turn 0x2b5820 (cx, cy). t = 7 and u = 1 → 26. When u < (3, or 4
    for t = 7) or R(cx, cy, 0xfcc8) & 1: v = R(cx, cy, 0x310c07) mod 3; 0 → 26, 1 → the corner 24,
    others 22, 2 → 25. Otherwise go on.
  * any other t: the cell is a house when it is the corner; or when cx = gx and R(bx, by,
    0x45982d) mod 3 = 0; or when cy = gy (and cx ≠ gx) and R(bx, by, 0x43dcd) mod 3 = 0; or
    finally when R(bx, by, 0xf794) & 7 = 0. A house is 24 at the corner, else 23 when
    R(cx, cy, 0x12a915) mod 10 = 0, else 22. Not a house: go on.

**4. Kind 15.** (bx, by) as above. When the inner-block test 0x2b8e20 (bx, by, 15, anchor) holds
(§6.4): h = rnd(H(bx, by)). When h & 1: h ← h >> 1 (unsigned); when h mod 20 = 0 and posmod(cx, 3)
= 1 and posmod(cy, 3) = 1, return ((h / 20) & 1) + 69; when h mod 20 ≠ 0 and posmod(cx, 3) ≠ 2
and posmod(cy, 3) ≠ 2, return (R(cx, cy, 0x4d468e) & 1) + 49.

With (ax, ay) = (⌊centre.x/16⌋, ⌊centre.y/16⌋), d²(x, y) = (cx − x)² + (cy − y)², and cw(k′) =
place weight (k′, big, warped distance from the centre to (16 cx + 8, 16 cy + 8)):

**5. Kind 16.** (ax, ay) → 55; (ax, ay + 1) → 56; the eight cells (ax ± 4, ay), (ax, ay ± 4),
(ax ± 3, ay ± 3) → 54; otherwise when cw(16) > 0.5 → 0.

**6. Kinds 34, 35.** (ax, ay) → 98; d²(ax, ay) < 9 and R(cx, cy, 0x7f395) & 3 = 0 → 99.

**7. Kinds 45, 46.** |cx − anchor.x| ≤ 7 and |cy − anchor.y| < 7 → 93 (the anchor of rule 1,
or (0, 0) when there is no big structure).

**8. Kind 24.** |cx − ax| = 6 and |cy − ay| = 6 → 96; (ax − 3, ay) → 97; d²(ax, ay) ≤ 112 → 93.

**9. Kind 17.** (ax, ay) → 66. With e = 31 (big) or 15, dx = |cx − ax|, dy = |cy − ay|: when
(cx = ax or cy = ay) and dx ≤ e − 1 and dy ≤ e − 1: with m = 128 (big) or 64, vx = posmod(cx, m),
vy = posmod(cy, m), return 68 when dx = e − 1 or dy = e − 1 or vx or vy is 0 or m − 1, else 67.
Otherwise when (dx ≤ 1 or dy ≤ 1) and dx ≤ e and dy ≤ e → 0; when cw(17) > 0.5 → 0.

**10. Kind 38.** (ax, ay) → (rnd(H(c1)) & 1) + 69; d² < 9 → 0.

**11. Kind 39.** (ax, ay) → 55.

**12. Kind 37.** (ax, ay) → 57; d² < 9 → 0.

**13. Kinds 1 and 3.** (ax, ay) → 24; |cx − ax| < 2, |cy − ay| < 2 and the side pair test (below)
→ (R(cx, cy, 0x23b0d) & 1) + 22.

**14. Kind 4.** (ax, ay) → 37; the same neighbourhood and side pair test → 38 when R(cx, cy,
0x23b0d) & 1 = 1, else 35.

**Side pair test:** with g = H(c1): bx′ = (R(c1, 0x711ac) & 1) × 2 − 1 + ax, dy′ = (R(c1,
0x11b548) & 1) × 2 − 1. The cell must be (bx′, ay), (ax, ay + dy′) or (bx′, ay + dy′); with
a = R(c1, 0x6b0e8) & 1, b = R(c1, 0xb2234) & 1, true for (bx′, ay) when a, for (ax, ay + dy′) when
b, for (bx′, ay + dy′) when a or b.

**15. Kind 2.** (bx, by) = (⌊cx/3⌋, ⌊cy/3⌋), point (48 bx + 16, 48 by + 16), w′ = place weight (2,
big, warped distance to it). When w′ > 0.5: (ax, ay) → 24; within |·| < 2 of it → the side pair
test as in rule 13 (returning (R(cx, cy, 0x23b0d) & 1) + 22), and nothing else for those cells;
farther: when R(bx, by, 0x49f7f4) & 3 ≠ 0 and the roads at that point (kind 2, w′) < 0.4 →
R(bx, by, 0x4f60c1) mod 7 + 27.

**16. Kind 7.** d²(ax, ay) < 16 → 52 at (ax, ay), else 0.

**17. Kind 8** (the fortress square 0x2a2b50). size = (R(c1, 0xc0aa) & 3) + 15, turn = R(c1,
0x5d932) & 3. The cell is turned about (ax, ay): turn 1 → (x, y) = (cy − ay + ax, ax − cx + ay);
turn 2 → (ax − cy + ay, ax − cx + ay); turn 3 → (ax − cy + ay, ay + cx − ax); turn 0 → (cx, cy).
(These are the game's formulas, including turn 2's.) With half = size / 2, x0 = ax − half,
y0 = ay − half, x1 = x0 + size, y1 = y0 + size: outside → go on; corners (x < x0 + 2 and
(y < y0 + 2 or y ≥ y1 − 2)) or (x ≥ x1 − 2 and (y ≥ y1 − 2 or y < y0 + 2)) → 47; (x0, ay) → 46;
otherwise 45.

**18. Kind 12.** w′ = place weight (12, big, warped distance from the landmark point 0x362a0
(c1, big) to (16 cx + 8, 16 cy + 8)). When w′ > 0.5: unless cx mod 5 = 0 or cy mod 5 = 0 or
rnd(H(cx / 5, cy / 5)) mod 5 = 0 (C division and remainder) → 0; then at (16 cx + 8, 16 cy + 8)
when the ground height (mode 0) < 1 or the roads (12, w′) > 0.5 → 0; else 43 when (cx + 3 cy) mod
5 = 0 (C remainder), else 41.

**19. Kind 22.** (ax, ay) → 89; (ax + 1, ay) or (ax, ay + 1) → 87; (ax + 1, ay + 1) → 88.

**20. Kind 20.** (ax, ay) → 8.

**21. Kind 21.** |ax − cx| < 2 and |ay − cy| < 2 → 12 at (ax, ay), else 11.

**22. Kind 9.** (bx, by), point and w′ as in rule 15 with kind 9. When w′ > 0.5: (tx, ty) =
(8⌊centre.x/128⌋ + 4, 8⌊centre.y/128⌋ + 4). When |cx − tx| ≤ 1 and |cy − ty| ≤ 1: the cells
x ∈ {tx − 1, tx}, y ∈ {ty − 1, ty} return 39 when 0x2aa270 (c1, 9, big) ≠ 0; then tx += 1 when
R(c1, 0x8ca8d) & 1 = 0, else ty += 1; (tx, ty) → 36; otherwise 38 when R(cx, cy, 0x12a915) & 7
= 1, else 35. Farther cells: when R(bx, by, 0x49f7f4) & 3 ≠ 0 and the roads (9, w′) at the point
< 0.4 → 34.

**23. The area landmark** (one per 64 × 64 cells). (qx, qy) = (⌊cx/64⌋, ⌊cy/64⌋); its cell
0x2d3b60: (R(qx, qy, 0x24341) mod 62 + 64 qx + 1, R(qx, qy, 0x165709) mod 62 + 64 qy + 1).
Type 60 and reach 1 by default; when qx + qy is odd, type 53; otherwise by R(at the landmark
cell, 0x8ae21) mod 11: 1 → 15 (reach 2), 2 → 37 (2), 3 → 55, 4 → 69 (2), 5 → 70 (2), 6 → 13,
7 → 14, 8 → 62, 9 → 21, 10 → 100 (2), 0 → 60. At the landmark cell: type 21 becomes 18 when the
ground height (mode 0) at its centre (16 x + 8, 16 y + 8) is below 0; return the type. For type
60, a second cell (sx, qy) with sx = landmark x + 4, or landmark x − 4 when ⌊sx/64⌋ ≠ ⌊cx/64⌋:
it returns 51, and cells within |·| < 2 of it on both axes return 0. Then cells within reach of
the landmark on both axes return 0.

**24. Natural objects.** g = H(cx, cy). Let rare = 600; when k = 28 and w > 0.5 rare = 50; when
k = 29 and w > 0.5 rare = 20; when any other k ≠ 0 has w > 0.5 the rare objects are skipped.
Otherwise:
* When R(cx, cy, 0x8af1e) mod rare = 0: return 0 when the ground height at Q < 1; road = roads
  (Q, k, w); return 0 when road > 0.4 or the river at Q (road, 0, 0, 0, the place) > 0.4; return
  R(cx, cy, 0x3005f52) mod 3 + 10 when k is 28 or 29 with w > 0.5, else (R(cx, cy, 0x3005f52)
  & 1) + 10.
* Else when R(cx, cy, 0x1e2d8) mod 200 = 0: return 0 when the cliffs at Q (k, w) > 0.4, when
  k ≠ 0 with w > 0.5, when the ground height < 1, or when the roads > 0.5 or the river (road, 0,
  0, 0, place) > 0.5; else 9.

Then: return 0 when k is 14, 18 or 19 with w > 0.5; return 0 when R(cx, cy, 0x8e9a9) & 3 ≠ 0.
With n = the neighbourhood at Q:
* v = (S(cx × 0.08 + 8989, cy × 0.08 + 60466) + 1) × 0.25; v += region weight (n, biomes 3–6,
  0.0004) × 0.35 (0x2adeb0); v += S(2 cx + 0x182a8, 2 cy + 0x5cb89e) × 0.5.
* When k is 25, 26, 27 or 32 with w > 0.5: v = S(2 cx + 0x182a8, 2 cy + 0xa52de) × 0.5 + 0.75;
  otherwise return 0 when region weight (n, biomes 9 and 13, 0.0003) > 0.5.
* k = 30 with w > 0.5: v × 0.7; k = 36 with w > 0.5: v × 0.6; k = 33 with w > 0.5: return 0.
* Unless v > 0.5: 0. h = the ground height at Q; 1 > h → 0; road = roads (Q, k, w); road > 0.5
  → 0; the river (Q, road, 0, 0, region weight (n, biome 8, 0.0004), place) > 0.5 → 0.
* r = R(cx, cy, 0x49ecc). r mod 30 = 0 → 19 when R(cx, cy, 0x17b9f76) & 1, else 20.
* warm = warmth (n, k, w). warm > 0.75 and (20 > h or r & 7 = 0) → 5. warm > 0.6 and 20 > h and
  r & 3 = 0 → 5. warm > 0.8 and dryness (n) > 0.8 → 4.
* The rocky noise 0x2a0b90 (16 cx, 16 cy, warm) > 0.6 → 3: 0 when warm > 0.75; else
  S(x × 0.001 + 8473 + V(0x350), y × 0.001 + 9438 + V(0x354)) + S(65 x, 77 y) × 0.05 − 0.2 +
  (1 − warm) × 0.1, V being the world's random numbers (part A). 0.8 > warm and R(cx, cy,
  0x2f8944d) mod 10 = 0 → 3.
* When 0.7 > warm: the shrub noise 0x2adb80 (16 cx, 16 cy, warm) > 0.3 → 2 (S(x × 0.0015 +
  V(0x350), y × 0.0015 + V(0x354)) + S(37 x, 37 y) × 0.05 − 0.2 + (1 − warm) × 0.1); 0.8 > warm
  and R(cx, cy, 0x4bf00) mod 10 = 0 → 2.
* Otherwise 1.

The integer products 65 x, 77 y, 37 x and the doubled cells are 32-bit before the conversion to
double.

### 6.4 Town helpers

**Town block 0x2af7b0** (cx, cy, kind): 0 unless kind 5. (bx, by) = (⌊cx/3⌋, ⌊cy/3⌋); the place
cell (px, py) = 2⌊⌊16 cx / 1024⌋ / 2⌋ (and y), big = 0x36160 (px, py), centre c = 0x362a0 (px,
py, big); (ox, oy) = (⌊⌊c.x/16⌋/3⌋ − 2, ⌊⌊c.y/16⌋/3⌋ − 2). When ox ≤ bx < ox + 4 and oy ≤ by <
oy + 4: v = layout[rnd(H(px, py)) mod 64] entry (row by − oy, column bx − ox) (tables T10);
when v ≠ 0 return it. Then d² = warped distance from c to (48 bx + 16, 48 by + 16); r² = 250000
(big) or 62500; d² ≥ r² → 0; w = (1 − d²/r²)²; 0.5 > w or the roads (that point, 5, w) > 0.4 →
0; v = R(bx, by, 0x11b98f) mod 1000; when float of v > w × 800, return 13 when v is even, else 0;
otherwise 1.

**Block corner 0x2b5760** (bx, by): v = R(bx, by, 0x5a44a) & 3; (3 bx + (1 when v is 1 or 2),
3 by + (1 when v is 2 or 3)).

**Cell turn 0x2b5820** (cx, cy): v as above for (⌊cx/3⌋, ⌊cy/3⌋); the four cells (3bx, 3by),
(3bx + 1, 3by), (3bx + 1, 3by + 1), (3bx, 3by + 1) give (4 − v) & 3, (5 − v) & 3, (6 − v) & 3,
(7 − v) & 3; other cells −1.

**Inner block 0x2b8e20** (bx, by, kind, anchor): false when (bx, by) = (⌊anchor.x/3⌋,
⌊anchor.y/3⌋); with x = 48 bx + 16, y = 48 by + 16, its place cell (⌊x/1024⌋, ⌊y/1024⌋), big,
landmark point and warped d²: false when kind = 0 or d² ≥ r²; true when (1 − d²/r²)² ≥ 0.5.

## 7. The cell loop (0x3025b0)

For i from −2 to 5, and within it j from −2 to 5, cell (cx, cy) = (4 zx + i, 4 zy + j):

1. type = 0x69c60 (cx, cy); the castle cell = 0x2a8290 (cx, cy) (buildings file §9).
2. **Near**: with reach = T6 of the type, the cell is near unless (i < 0 and −i > reach) or
   (i ≥ 4 and i − 3 > reach) or (j < 0 and −j > reach) or (j ≥ 4 and j − 3 > reach).
3. **Type 92 inside the zone** (0 ≤ i, j < 4), before any srand of this cell: object 0x75 (117)
   with y = dots(16 cy + rand mod 8 + 4) + 32768, then x = dots(16 cx + rand mod 8 + 4) + 32768
   (y drawn first), z = trunc64((ground height (blocks toward zero of x, y; mode 0) + 1.0) ×
   65536.0), turn rand mod 4; settled without the ground test (contents file §C8), then kept if
   inside.
4. **Set pieces and buildings**, when near and (type ≠ 0 or the castle cell's kind ≠ 0):
   srand(rnd(H(cx, cy)) mod 10000000), then:
   * 60: the mound (nature file §8) at (16 cx + 8, 16 cy + 8), top base(60) − 4 (buildings file
     §1), radii 32, 32;
   * 69, 70: the tower platform (buildings file §12);
   * 97: the arena master's dais and its master (part C); 67: the aqueduct; 66: the water tower; 68: the cistern; 52: the wizard's
     tower (buildings file §11, §12);
   * then, when the castle kind ≠ 0 or the type is a building: the building builder (buildings
     file §2–§8); else when the type is 27–34 and the cell is inside the zone: the crop field
     (buildings file §13).
5. **Natural structures**, when the type ≠ 0 and the cell is near (as in 2):
   * The point p = 0x2d8630 (cx, cy), or the cell's centre (16 cx + 8, 16 cy + 8) for types 8,
     11–14 and 55. h = ground height (p, mode 4); **under water** when 0 > h, and then h = 0.
   * Size: height th = R(cx, cy, 0x2fbcc94) mod 10 + 20 (0x2d9ed0), width tw = R(cx, cy,
     0x12ce32c) mod 10 + 12 (0x2d9e70). Type 8: th = 50, tw × 3. Types 11, 13: th = 64, tw = 40.
     Types 12, 14: th = 64, tw = 80.
   * r = trunc(float of th × 1.25); skip unless p.x + r ≥ 64 zx and p.y + r ≥ 64 zy and p.x − r <
     64 zx + 64 and p.y − r < 64 zy + 64.
   * srand(rnd(H(cx, cy)) mod 10000000) (0x314a46) — for every type that got here, built or not.
   * Only types 1–15, 19, 20, 21, 37, 51, 53, 54, 55, 62, 69, 70 build (the switch at 0x314a5f);
     z = trunc(h + 1.0), zf = h + 1.0:

| Type | Built |
|---|---|
| 1, 8 | common tree (p, z, th, tw) |
| 2 | fir; 3 birch; 4 willow; 5 palm (p, z, th, tw) |
| 62 | sunken chest (nature file §9) |
| 9 | rock (p, z, rx = th, ry = th, rz = tw, flags 0) |
| 10 | hill: rocky when the highlands at p (part A, the column value) > 0.5; centre top trunc(float of tw + zf), radii 2 th, 2 th |
| 11–14 | rocky hill, top trunc(tw + zf), radii th, th |
| 15 | hill with a canyon: rocky by the highlands; open = rand mod 2 ≠ 0; a = rand angle; R = 2 th; the canyon's far end (trunc(cos a × float(4R) + float p.x), trunc(sin a × float(4R) + float p.y)) (sinf and cosf); top trunc(tw + zf), radii R, R |
| 37, 51, 53, 69, 70 | when under water: a rocky hill at top −8, radii 2 th, 2 th |
| 54, 55 | when under water the same hill; then the boulder (p, z, 8, 8, 25, flags 8) (0x314fc9) |
| 6, 7 | a tree on a pedestal (below) |
| 19 | stump (p, z, th, tw) |
| 20, 21 | along x = rand mod 2 ≠ 0; type 20: hollow = rand mod 3 = 0, radius float(rand mod 4 + 2); type 21: hollow, radius float(rand mod 4 + 3); fallen log (p, z, radius, length th, hollow, along y when not along x) |

**Tree on a pedestal (6, 7).** tz = trunc(zf + 3.0), ph = trunc(float th × 0.8); type 7: palm
(p, tz, ph, tw), type 6: common tree (p, tz, ph, tw). Then a mason with the identity frame
(mason file §1) and one colour (240, 240, 240), pattern 1: pillar (p, z, radius 6, height 3,
cap 0, mode 0), disc (p, z, radius 5, height 3, cap 2, mode 0). Then four benches (0x3155ec):
object 31 when the forest weight at p (kind 0, weight 0) > 0.5, else 30, at (p.x, p.y − 6),
(p.x + 8, p.y), (p.x, p.y + 8), (p.x − 6, p.y) with turns 0, 1, 2, 3, z = trunc64((zf + 2.0) ×
65536.0); each settled and kept (contents file §C8).

## 8. Oracles

All are read by `tests/EngineTests.cpp` from the owner's game folder (`Extract/`); none is in
the repository.

| Oracle | Format | Checks | Command |
|---|---|---|---|
| `Extract/gen-cells.txt` | text, `# cx cy object` then one line per cell: cx cy type | 0x69c60 (§6) at ~60,000 cells of the TEST world, seed 123 | `EngineTests cells` |
| `z_<x>_<y>.cwz` in a zones folder (`Extract/seed_123`, `Extract/gen_seed_123`) | "CWZ1", zone x, zone y, region biome, 0 (i32 each); then for each of 4096 columns (x-major: index lx × 64 + ly): base z (i32), run count (u32), runs of (count u16, block 6 bytes) | the blocks every structure writes, per column (part A checks the plain columns) | `EngineTests zones [n]`, `zonediff x y` |
| `z_<x>_<y>.rec` | "CWR1", then six lists, each: offset (u32), stride (u32), count (u32), the raw records: +0x18/0x40 models, +0x30/0xe0 items, +0x48/0xe0 the generator's copy of the items, +0x60/0xe0 objects, +0x78/0xe0 the generator's copy of the objects, +0x90/0x40 the light list; extractions made after ZoneExtractor learned it add a seventh, +0xa8/0x40 the sound emitters (contents file §C2.1) | objects: type (+0), position (+8), turn (+0x20), scale (+0x24); models: type, position, +0x20, +0x24; items: category (+0), id (+4), position (+0xa0), angle (+0xc0) | `EngineTests allobjects`, `allmodels`, `allitems` |
| `z_<x>_<y>.rtrace` | records of 6 bytes: caller RVA (u32), rand value (u16), every rand() of the zone's generation | the draw order to the first difference | `EngineTests rtrace x y` |
| `day.txt` | the world's day the zones were made on (135 for `gen_seed_123`) | — | `CW_DAY` |

`Extract/gen_seed_123` holds 125 zones the game generated afresh (ZoneExtractor runs 0x2ae0c0
outside the world, without saved changes) with their `.rtrace` and `.spawns` (part C), and a
`refreshed/` set after the daily refresh. Its world seed is the number after the folder name's
last underscore. In it the records hold 487 objects, 8227 models and 178 items. The extracted
world `Extract/seed_123` (7615 vanilla zones) checks the blocks over the whole world; its
records were made on day 7. `Extract/gen-oracle.txt` checks only part A's heights.

The checks: every structure block of the 7615 zones; every model's type, position and both
floats (except the unwritten sizes, open question 3); every object but the three the daily
refresh adds; every item but those of dungeon cells (kind-8 places) and the plinths.

## 9. Open questions

1. **Kind-0 place centre.** With no place, 0x31a5d0 leaves the centre unwritten; the biome
   (§5) and the planner then use whatever the stack held. CubeVulkan uses biome 0 there. Only
   buildings outside places would show it.
2. **Kind-3 castle chests' region.** The game takes the region of the zone place's centre
   (frame +0x1578, contents file §C3); CubeVulkan used the chest's position. The item records
   checked do not include the region.
3. **Unwritten model sizes.** Fruit (0x4e–0x50) and leaf (0x4d) models never write +0x20 (stack
   left-overs; CubeVulkan writes 0). Lamps' +0x20 byte in objects is never written either.
4. **Mason pattern 2** (0xbe8d0, the ground's colour) and cap kinds other than 0, 2, 6, 7, 8, 9
   (mason file) are not described; no checked zone uses them.
5. **Castle room details 11 and 12** (halls of big structure 42 and 44) use pattern 0 for their
   pieces; their ceilings are not yet exact in CubeVulkan (buildings file §8).
6. **Furniture kinds 8–10** (0x6b270, 0x6af00, 0x6c2f0) and centre pieces 1–3 (0x682a0,
   0x68370, 0x68790) belong to lists no room fills; not described.
7. **Kind-8 dungeon cells** (0x2fa7b0) are part C's; the items of those cells are not checked.
8. **Unwritten item fields.** A crate's item lying about (contents file §C3) never gets an angle,
   and the reused herb record starts with no position; both are stack left-overs (CubeVulkan
   uses 0 and a grey block). The bookshelf's books store a raw rand() as the modifier with id 0;
   CubeVulkan records them differently (the same draw count), worth re-checking against
   `allitems` with the id compared.
9. **Region-planner-driven doors** (roof kind 5's signed-multiply test on hash(H(C1) + 0x29fa4),
   buildings file §5) were read from the assembly at 0x306d3a only (no pseudocode); the
   quotient's sign handling deserves a second reading.
10. **The +0x90 list** (answered): the light list, rebuilt with the emitter list +0xa8 at the end
   of the plant pass (contents file §C2.1); checked in all 31,940 extracted zones.
11. **Model type 0x1f.** The emitter list takes type 0x1f (the sound code plays river sounds for
   it), but no generator routine was found placing a model of type 0x1f, and none occurs in the
   31,940 extracted zones. Unless a placement was missed (the generator 0x2ff6f0 has no
   pseudocode; no model-building code in it or in the other callers of 0x69b30 and 0x2fb7d0 writes the constant 0x1f as a type), the river sound never
   plays in vanilla. Likewise the sound code's cricket branch (emitter types 0..12) can never
   see such a type, since the list holds only 0x1f and 0x4d to 0x50. Oracle: a ZoneExtractor
   run that records +0xa8, searched for type 0x1f in river zones.
