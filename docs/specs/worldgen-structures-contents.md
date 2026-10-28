# World generator, part B: spots, objects, models and items of a zone

Part of `worldgen-structures.md` (conventions §0; records §4; the generator's order §2). The
creatures that share these steps' rand() stream are part C's (`worldgen-creatures.md`); here
each such call is named at its place in the order, with the number of draws left to part C.

---

## C1. Spots (zone +0x100, +0x110)

A spot is a position (x, y, z in dots) in a map keyed by (cell x, cell y, kind). Spots are added
by the building builder (kind 4, buildings file §4), by crop fields (kind 21, buildings file
§13) and by 0x3162b0 below; zone +0x110 is a working copy from which spots are given out.

**Finding spots** 0x3162b0, after the street lamps: for a from 0 to 62 step 2, b from 0 to 62
step 2 (a outer): X = 64 zx + a + rand mod 2, then Y = 64 zy + b + rand mod 2 (two draws); cell
(⌊X/16⌋, ⌊Y/16⌋); the column of (X, Y) with its values; base position (dots(X) + 32768, dots(Y) +
32768). For each stored index i from −1 to n − 1 (n stored blocks; index −1 reads as a solid
block of type 1, indices ≥ n as air, 0xe770), z = base + 1 + i, b0 = block i, b1 = block i + 1:
1. The column's river value > 0.5, b0 solid and b1 water (type 2) → kind 18 at z.
2. When b1 is solid, next i.
3. Kind 0 at z.
4. ground = b0's type ≠ 0, no flag 8, and its type not in {2, 3, 7, 8, 11, 17, 19} (mask
   0xa098c). Unless ground and there is room above (below), next i.
5. Type 18 → kind 5. Types 23–30 → kind (type − 16). Else b1 flag 0x20 → kind 6. Else b1 flag 2
   → kind 2. Else when z > 0 and b0 has no flag 2: kind 1; and kind 16 when b0's type is 4, 5,
   9, 10 or 14 (mask 0x4630) and b1 is not water; and kind 17 when b0's type is 6.

**Room above** 0x31ad50 (X, Y, z): for dx, dy in −2..2 (dx outer): m = max(|dx|, |dy|); false when
any of the blocks (X + dx, Y + dy, z + m + k), k = 0..2, is solid, or any of (…, z + k), k from m
to 7, is solid. True otherwise.

After finding, the working copy (+0x110) is set to the map (0x2f9ef0); it is set again at the
start of step 9 (C5).

**Taking a spot** 0x31bd90 (cx, cy, kind):
1. Kind 3: the first static object of type 0x75 in the list: its (x, y, z + trunc64(size z (T8)
   × scale × 65536)); no draw.
2. When the working copy has the key: when its list is empty it is refilled from the map's list
   for the key. When the list is not empty: i = rand mod count; the spot i is removed and
   returned.
3. Otherwise (no key, or nothing to refill): the top of the zone's middle column, (dots(64 zx +
   32) + 32768, dots(64 zy + 32) + 32768, dots(top of its stored blocks)); no draw.

**Count** 0x1299a0: the number of spots of the key in the map (+0x100), 0 when absent.

**A spot in a cell** 0x31c0e0: the first kind with a non-zero count of 2, 7, 16, 1, else 0;
taken.

## C2. Plants (0x3191b0, step 7)

For X from 64 zx to 64 zx + 63, Y inner, with the column's values, for each stored index k
from 0 to n − 1 (b0 = block k, b1 = block k + 1 (air past the top), z = base + k + 1). One model
record on the stack is reused for every plant; a field a branch does not set keeps its value
from the previous plant (the size starts at 0). "Plain (flags)" sets the tint to (1, 1, 1) and
the flags. "Turn 90" = float(rand mod 4 × 90); "turn any" = float of rand × 360 / 32767 (float
rand × 360.0, then / 32767.0). noiseA = S(X × 0.01 + 9843, Y × 0.01 + 8437), noiseB = S(X ×
0.01 + 34234, Y × 0.01 + 234234). "Put type" writes type and position (dots(X) + 32768, dots(Y)
+ 32768, dots(z)) and appends the record (no inside test). Per block:

1. Skip when b0 has flag 8; skip when b0 is open and (no flag 0x40 or flag 0x80); skip when b1
   has flag 0x20 and b0's type is not 4; skip types 19 and 2.
2. **Hanging** (b0 type 0, b1 type 18) when rand mod 20 = 0: plain (2); yaw turn 90; size = rand
   fraction × 0.04 + 0.06; type 0x48 at z = dots(z) − trunc64(size × 20 × 65536); append.
3. Skip when k < n − 1 and block k + 1 is solid.
4. b1 type 0, b0 type 3, rand mod 2 ≠ 0 (drawn only for such blocks) and warmth > 0.2: v =
   S(X × 0.05 + 9843, Y × 0.05 + 8437); unless |v| > 0.5 next block; plain (4), yaw turn any,
   size 0.09, put 0x23; next block.
5. b0 type 17 at X mod 16 = 0 and Y mod 16 = 0 (C remainder) and rand mod 3 ≠ 0: yaw 0, flags 2;
   s = rand fraction × 0.5 + 0.5; tint (s, s × 0.5, s × 0.2); put 0x4c (the size kept); next.
6. b0 type 12: plain (6), turn any, size 0.075; noiseA > 0 → 9 unless S(double(float X × 0.01 +
   34234), double(float Y × 0.01 + 234234)) ≤ 0 → 10 (this noise is computed in float first);
   else noiseB ≤ 0.5 → 14, else 13; put; next.
7. Unless b0's type is 4, 9, 10, 14 or 18 (mask 0x44610), skip when z > 0.
8. Unless |S(X × 0.05 + 9843, Y × 0.05 + 8437)| > 0.6 next; then unless rand mod 6 = 0 next.
9. **Under water** (z ≤ 0 and b0 without flag 2): skip when z ≥ −5. plain (2), turn 90; size
   0.1, type 0x11; noiseA > 0 → type = rand mod 2 + 15, and type 15 → size 0.075, flags 6; else
   flags 6. size × (rand / 32767 + 1); put; next.
10. b0 type 14: plain (2), turn 90, size 0.075; type 0x14 when noiseA ≤ 0, else 0x13; 0x13 →
    flags 6; 0x14 → size = float(double rand × 0.02 / 32767 + 0.03); put; next.
11. b0 type 10: next.
12. "Pick (a, b, c, d)": noiseA > 0 → (b when noiseB ≤ 0, else a); else (d when noiseB ≤ 0.5,
    else c).
13. Forest weight > 0.3: plain (2), turn any, size 0.075; type = pick (9, 10, 13, 14); 13 or 14 →
    size = rand fraction × 0.05 + 0.05; else flags 6; put when b0 is type 4 or the type is 9 or
    10; next.
14. Unless warmth < 0.8 (hot):
    * dryness < 0.3: plain (6), turn any, size 0.075; type = pick (7, 8, 11, 12); 11 or 12 → size
      = rand fraction × 0.05 + 0.05; put when b0 is type 4 or the type is 7 or 8; next.
    * unless dryness < 0.1: tint (1, 1, 1), turn any, size 0.075; noiseA > 0 → (6 when noiseB > 0,
      else 5); else (0x16 when noiseB ≤ 0.5, else 0x15); flags 6 for types 4–6, else 2; 0x15 →
      size = rand fraction × 0.05 + 0.05, flags |= 4; 0x16 → size = rand fraction × 0.02 + 0.1,
      flags |= 4; put when b0 is type 4 or the type is 4 or 5; next.
    * (dryness < 0.1 here cannot happen after the first test; the game would draw rand mod 100
      and on 0 put 0x28 or 0x29 by rand mod 2 with plain (2), turn 90, size 0.075.)
15. Otherwise: plain (2), turn 90, size 0.075. River value > 0.43: turn any, size 0.09, flags 4,
    type 0x23. Else: unless noiseA > −0.1: v = noiseB; type 0 when −0.5 > v, 2 when v ≤ 0.5, else
    1 (0x31b8b0); else dryness > 0.5 → (0x16 when noiseB > 0, else 6); else (5 when noiseB ≤ 0,
    else 4); types 3–6 or 0x16 → flags 6, and 0x16 → size = rand fraction × 0.02 + 0.1. Put when
    b0 is type 4 or the type is 4–6.

### C2.1 The light list and the sound emitter list (end of 0x3191b0)

After the last column, still inside 0x3191b0 (0x14031a53b–0x14031a5a7), the plant pass rebuilds
two lists of 0x40-byte model records from the zone's whole model list (+0x18). It draws no
rand(), so it does not move the draw order.

1. Both lists are emptied (end set back to start; the memory is kept): the **light list** at zone
   +0x90 (start +0x90, end +0x98) and the **emitter list** at zone +0xa8 (start +0xa8, end +0xb0).
2. Every model of +0x18 is visited once, in list order (first to last). For each model, first the
   light test, then the emitter test:
   * **Light**: type 0x4c, or bit 0 of the flags (+0x38) set → appended to +0x90.
   * **Emitter**: type 0x4d, 0x4e, 0x4f or 0x50 (type − 0x4d ≤ 3 as unsigned), or type 0x1f →
     appended to +0xa8.

   A model can enter both lists. Both appends go through 0x69b30, which copies the record's
   fields one by one: type (+0x00, 4 bytes), position (+0x08, +0x10, +0x18, 8 bytes each) and
   the seven 4-byte fields +0x20 to +0x38 (size, yaw, +0x28, tint r, g, b, flags). Bytes +0x04..
   +0x07 and +0x3c..+0x3f are not copied; they are whatever the list's memory held.

So an emitter record is exactly a model record (main file §4.2): the sound code (`sound.md` §4)
reads its type (+0x00) and position (+0x08 to +0x18). What fills the list in practice:

| Type | Model | Where it is placed | Sound use (`sound.md` §4) |
|---|---|---|---|
| 0x4d | leaf | the common tree's leaf clumps (nature file §3, 0x264670); also centre piece 0x68790, which no room uses | the falling-leaf particle, 1 in 16 per second |
| 0x4e, 0x4f, 0x50 | fruit (0x4e + a value mod 3) | the common tree, fir, birch and willow (nature file §3–§6: 0x264670, 0x263210, 0x2626e0, 0x263ea0) | birds by day; 0x4e owls by night |
| 0x1f | — | no placement found in the generator (main file, open question 11) | river sounds |

Only models present when the plant pass ends are in the list. Models added later (steps 8 to 12)
would not be; in the checked zones none of the light models are (see the evidence below).
CubeVulkan's own options do not touch this step.

**Evidence.**
* The assembly at 0x14031a53b–0x14031a5a7 is the whole step (two list resets, one loop with the
  two tests); calls.csv lists the two appends (0x14031a581 to +0x90, 0x14031a59b to +0xa8) as
  the only 0x69b30 calls on a zone's +0xa8; no other routine was found writing zone +0xa8
  (the sound code 0xaf110 only reads it).
* **Draw order.** The plant pass's last rand() call site is 0x14031a3d3 (return 0x31a3d9). Over
  all 789 `.rtrace` files of `gen_seed_123`, `gen2_seed_123`, `genb_seed_123` and
  `genr_seed_123`, no draw has a return address between 0x31a3d9 and the end of 0x3191b0
  (0x31a5c2); the draw after the plant pass's last one comes from step 8 or 9 (for example
  0x115c36, 0x11ac42, 0x141341, 0x3181f8). This is what a draw-free pass between step 7 and
  step 8 gives.
* **The sibling list.** The existing `.rec` files hold +0x18 and +0x90 (but not +0xa8). In all
  31,940 zones of `seed_123` and the `gen*_seed_123` sets, +0x90 equals "the models of +0x18 that
  are type 0x4c or have flag bit 0", in order and count, with every copied field equal (only the
  uncopied bytes +0x04..+0x07 and +0x3c..+0x3f differ). The same rule on the same models
  predicts 51,029 leaf (0x4d), 2,416 type-0x4e, 2,119 type-0x4f and 2,245 type-0x50 emitters,
  and none of type 0x1f (no model of type 0x1f occurs in any of these zones).

**Oracle for a future extraction.** ZoneExtractor now writes a seventh list into `.rec`
(offset 0xa8, stride 0x40, then the records, the same layout as the others). New extractions
check this step when each zone's +0xa8 records equal, in order and count, the models of the
zone's +0x18 list whose type is 0x4d to 0x50 or 0x1f, comparing type, position and the fields
+0x20 to +0x38 (not +0x04 and +0x3c). For the emitter list to tell more than +0x90 already
does, a few zones should hold trees with fruit (woods and forests) and, if one can be found, a
model of type 0x1f.

## C3. The cells' things (0x316780, step 6)

For each cell of the zone, cx from 4 zx to 4 zx + 3, cy inner: srand(rnd(H(cx, cy))) (0x2ff5c0;
no modulus). K = the castle cell (buildings file §9). "Chest at a spot": object 0x16 at the spot,
turn rand mod 4, settled with the ground test (C8; the result is not checked).

1. **K kind 5**: chest at a spot taken of kind 6. The reward: the role's own treasure 0x131540
   (planner, C1) (part C; a crafting book or artifact, lore-and-artifacts.md §5). When it has
   category 0: category 30; modifier = rand; id = rand mod 3; rarity 4; a further draw r with
   r mod 10 = 0 would raise it to 5, then capped at 4; region = 0x2ada20 (the chest's x, y toward
   zero). Kept if inside; when kept, the reward is stored under the chest's index (main file
   §4.1).
2. **K kind 3**: chest at a spot of kind 6; reward category 30, id = rand mod 3, modifier = rand,
   rarity = (K's steps >> 2) + 1, + 1 when rand mod 10 = 0, at most 4; region = 0x2ada20 (the
   zone place's centre, §5 of the main file). Kept if inside, reward stored.
3. t = type (cx, cy). **Crates** when t is a crate cell (T7) other than 21, 60, 62 (the
   generator masks those out): the reward item: category 30, id = rand mod 3, modifier = rand,
   rarity = rand mod 3 + 2, region = 0x2ada20 (16 cx, 16 cy). The spot: t = 17 → kind 5 when its
   count is non-zero, else 0; t = 18 → kind 18; else a spot in the cell (C1). Then rand mod 2 ≠ 0
   (as "rand & 0x80000001" signed remainder) or t = 18: a chest at that spot (turn rand mod 4,
   settled), kept if inside with the reward stored. Else an item lying about: the reward at the
   spot, bit 1 of +0xc4 set, angle not written (left from the record).
4. **Lore cells** (T7: 13, 37, 55, 69, 70): b = base (cx, cy, t) (buildings file §1).
   * t = 55: object 0x7d at (dots(16 cx + 8) + dots(0.51), dots(16 cy + 8) − dots(0.1), dots(b +
     1)); up to 8 times: z = dots(max(floor of ground height (x toward zero, y toward zero, mode
     0), 0) + 2); blocked when any block at (x + dx, y, z + dz) for dz = −1..1 (outer), dx =
     −1..1 is solid or has flag 0x40 (0x31abf0); unless blocked stop; y −= 65536; stop when y <
     dots(64 zy). Appended directly. Next cell.
   * Otherwise type 0x82 for 37; else X(style) & 3 = 0 → 0x76, else 0x77 + X(style + 0xda5) mod 6.
     Turn rand mod 4; at (dots(16 cx + 8), dots(16 cy + 8), dots(b)); settled with the ground
     test; kept if inside.

## C4. The region role's things (0x11a820, step 8)

The role of the zone's place cell C1 in its planner (part C). Role 0: nothing. Roles 6–13: an
item (below). Roles 19–23 and the encounters: part C (their objects too: campfires, chests,
braziers; worldgen-creatures.md).

**Role item.** p = the place of C1 (kind, big, centre, hash g = H(C1)). landmark = 0x2ba990
(kind, C1) (main file §6.3 rule 2); limit = 0.5 with a landmark, else 0.75. The place's cells:
for x from ⌊1024 C1.x / 16⌋ to ⌊(1024 C1.x + 2048 when big, else 1024) / 16⌋ − 1, y likewise
(x outer), those with limit < place weight (kind, big, warped distance from the centre to (16 x +
8, 16 y + 8)) that fit (0x128cc0); none → the centre's cell (⌊centre/16⌋). pick = cells[rnd(g +
0x4192) mod count]. Nothing unless the pick is in the zone. The item: category 24, id by role
6–13: 1, 2, 0x12, 3, 5, 7, 8, 9; at a spot in the pick cell (C1); angle = rand × 360 / 32767.

**Fits** 0x128cc0 (kind, x, y): t = type (x, y); with a landmark: t = landmark. Kind 13: plain
cell and on a river; kinds 14, 19: plain and on lava; otherwise 0x128aa0: kinds 1–3: t 22–24;
4: t 35, 36, 38 (35–38 but 37); 7: 52; 21: 12; 22: 87–89; 29: 10–12; 34, 35: 99; 37: 57; 38: 69,
70; any other: plain and on open dry ground. "Plain": t in {0, 1, 2, 3, 4, 5, 10, 11, 12} (mask
0x1c3f) or 99. Ground tests (0x128970, 0x128fd0, 0x128dc0) at (16 x + 8, 16 y + 8), with the place
there: false when its variant ≠ 0 and not 0.8 ≤ weight; lava: the column's lava value > 0.5;
river: its river value > 0.5; open dry: none of river, lava, swamp > 0.5 and the ground height
(mode 0) ≥ 1.

## C5. The cells' creatures and items (0x3171a2, step 9)

The working spots are reset. One item record on the stack is reused for the herbs (its position
starts unset: the first herb check reads the block at an undefined position, which the game's
memory makes a grey solid block of type 1). For each cell, cx outer, cy inner:

1. h = rnd(H(cx, cy)); srand(h mod 32767). t = type; k = the zone place's kind; w = place weight
   (k, big, warped distance from its centre to (16 cx + 8, 16 cy + 8)). things = true.
2. When w > 0.4: k = 5 → the townspeople of the cell (part C, 0x317290), which returns whether
   things stay true. Else when k is not 8 or 16, t ≠ 88, C1 = (0, 0) and t = 24: the first
   town's three people (part C, 0x317c54), things = false.
3. **Cells 98, 99** (when things): n = rand mod 2 + 1; for each: a spot of kind 1; object 0x53;
   turn rand mod 4; for the second or any of cell 99: low = z < dots(25); when rand mod 4 ≠ 0:
   0x5f when low else 0x60; else when low and rand mod 3 = 0: 0x5e; else r = rand mod 7: 0x25,
   0x26, 0x27, 0x28, 0x5d, 0x1c for 0–5, 6 → 0x83 + rand mod 2. Kept if inside.
4. **A region's boat** (0x318068): cx and cy multiples of 16 (⌊·/16⌋ × 16), biome of the region at
   (16 cx, 16 cy) is 1, ground height (16 cx + 8, 16 cy + 8, mode 0) < 10 and the kind-16 count is
   non-zero: a spot of kind 16; angle = rand × 360 / 32767; item category 24, id 2.
5. **Wild groups** (part C, from 0x3181aa): when k = 0 or w < 0.5: rand mod 128 = 0 and roads
   (cell centre, k, w) < 0.25 and the area landmark cell (main file §6.3 rule 23) at squared
   distance > 16 → part C's group.
6. **Castle creatures** when K's kind ≠ 0; **dungeon creatures** when k = 8 (part C; some srand).
7. **Herbs** (0x31832b), when the kind-16 count is non-zero: tree = t in 1–5; category 0xb;
   the block under the reused record's position (blocks of x, y, z − 65536) decides id 0x18 when
   it is snow (type 10), else 0x16. When rand mod 5 = 0, id 0x1d. n = rand mod 40 − 38 (so 1 one
   time in forty, else ≤ 0). When tree and t ≠ 5: id 0xf, n = rand mod 4; when rand mod 2 ≠ 0, id
   0x24 and n = 1. Then t = 1: warm, dry at the cell centre; h mod 10 = 0, 0.3 < warm < 0.75 and
   dry > 0.2 → category 1, id 0xe (checked); else h mod 10 = 1 and warm > 0.6 → id 0x28 (checked).
   t = 4: h mod 10 = 0 → 0x2b, = 1 → 0x29 (checked). Checked → n = count / 8 + 1. Unless checked,
   when the id is 0x16 or 0x1d: the block under the reused position must be type 4, else the rest
   of this cell is skipped (the next cell begins). For n times: angle = rand / 32767 × 360; a spot
   of kind 16 (it becomes the reused position); the item there.
8. **Cold water** (0x3186a9): rand mod 10 = 0 and the kind-18 count non-zero: a spot of kind 18;
   when warmth at its (x, y toward zero) < 0.2: item 0xb/0x27, angle rand / 32767 × 360.
9. (0x3187f9) rand mod 30 = 0 and kind-5 count non-zero: a spot of kind 5; item 0xb/0x13, angle
   rand / 32767 × 360.
10. **Crops to pick** (0x3188b3), t 27, 32, 33: id 0x10, 0x11, 0x2a; want = rand mod 5 + 4; n =
    min(count of kind 21, want); each: a spot of kind 21, item 0xb/id, angle rand × 360 / 32767.
11. Animals (part C, 0x3189d6).

Items here are category 0xb (and 1 for the berries) with no other fields set; positions are the
spots; they are appended (no inside test).

## C6. Other items

* Plinth items: buildings file §8. Role items: C4. Crate items: C3.
* **Books** (bookshelves, buildings file §6.1 kind 4): category 0x13, id 0, the modifier a raw
  rand value.

## C7. Copies, re-stamping and lists (steps 11–12)

1. Objects (+0x60) are copied to +0x78 (0x29baf0) and items (+0x30) to +0x48 (0x29bcb0): the
   generator's state, which the daily refresh restores.
2. Each item in +0x30 whose category is in T7's re-stamp set gets the region 0x2ada20 (x, y toward
   zero) written to +0x0c/+0x10.
3. 0x31d470: the list at +0xc8 is reset to +0xc0's start (emptied) and the list at +0x4b8 cleared.
   Every item of category 30 adds its position to +0x4b8; every object of a light type (T7) is
   appended to +0xc0; every chest (0x16) whose block (floor of its position) has no flag 0x20
   adds its position to +0x4b8.

## C8. Settling and marking

**Settle** 0x31b2d0 (object, ground test): false when (x, y) blocks are outside the zone. Size
(T8) × scale; odd turn swaps x, y. e = trunc64(655.3599853515625) = 655; hx = trunc64(sx × 0.5 ×
65536), hy likewise; box x0 = floor((e − hx + x)/65536), x1 = floor((hx − e + x)/65536), y0, y1
likewise. solid (z) = any block of the box at height z is solid. Up to 20 times while not solid
(z toward zero), z −= 65536; up to 20 times while solid, z += 65536. With the ground test: false
when any block of the box at (z − 65536) toward zero is open. False when the block at the floor
of the position is water (type 2), or the one below (z − 65536) is type 3, 17 or 19. Otherwise z
= floor(z / 65536) × 65536; true. The object keeps its moved height even when false.

"Settled and kept" = settle with the ground test, then, when true and inside, append (no second
inside test result is needed).

**Marking** 0x31d700 (step 12): first every block position in the set at zone +0xe8 has flag 0x40
cleared (stored back). Then for each object: skip types 0x5f, 0x60, 0x11, 0x12, 0x14, 0x7d; skip
when its active byte (+0x28) is 0 and the type is one of 3–6, 8–14, 16 (mask 0x17f78). Size =
0x13a960 (T8 × scale); odd turn swaps x and y. hx = trunc64(sx × 0.499 × 65536), hy likewise;
x0 = floor((x − hx)/65536), x1 = floor((x + hx)/65536), y0, y1; z0 = floor(z / 65536), z1 =
floor((z + trunc64(sz × 0.999 × 65536)) / 65536). For every block in [x0, x1] × [y0, y1] × [z0,
z1] (x outer, then y, then z) without flag 0x40: flags |= 0x40 (0xc0 for object 15), |= 4 for
objects 3–5; stored (0x31c1c0) and its position added to the set at +0xe8.

## C9. The daily refresh's objects (0x31c540)

Part C specifies the refresh (its seed, shoppers and travellers). Its objects: after the
travellers, when their camp has a fire: "put (type)" takes a spot of the camp's kind in the camp
cell, turn rand mod 4, appended directly when the position is inside the zone. First 0x53 (the
campfire), then n = rand mod 4 times: by rand mod 8: 1 → 0x1c, 2 → 0x60, 3 → 0x28, 4 → 0x26, 5 →
0x27, 6 → 0x25, 7 → rand mod 2 + 0x83 (0x85 when the race is 0x2d, 0x2b, 0xa7 or 0xe6), 0 → 0x5d.
Before it the objects are restored from +0x78.

## Errata (from the clean-room implementation)

- C4: the role item's region and per-player flag are not given here; take them from `worldgen-creatures-roles.md` §2.
- **Fixed (part C, `worldgen-creatures-roles.md` §3.1 "Where its draws fall"; `worldgen-creatures-zone.md` §1 step 4).** In 2 of the 125 gen_seed_123 zones (−171 198 and −183 89) the game makes a spot take, then creature looks and gear, right after the spot map. It is not the planner (planners draw no rand()). It is step 8, the region role's things (C4): role 1 places whose encounter is a quest camp (0x11f280). In those two zones the cells' things (C3) and the plants (C2) draw nothing, so step 8 directly follows the spot map. The camp reseeds with srand(R(ph + 0x9a11)) and then draws the camp object's cellSpot take, the giver's looks and gear, and then, when the layout does not hold, per guard: variant, looks, gear, cellSpot. The traces match this draw for draw (first values 25167 and 1406).
- The street lamp's turn is unset here; the game shows 48 (a stack leftover). Implementations write 0.
