# World generation (C): tables

Appendix of `worldgen-creatures.md`. T1–T4 are game data: their values are not kept in the
repository. The game builds them in code (0x44b40, 0x13f0d0, the race pickers); CubeVulkan
reads the values at run time from the files the extraction tool (tools/ZoneExtractor) writes
from the owner's install, under `Extract/oracle/`: spawn-table.txt, races.txt, pickers.txt and
looks.bin. Their layouts are in worldgen-creatures.md §8.3 and repeated below where they
matter. T5–T7 are short id lists and rules and are given in full.

## T1. Race levels (spawner +0x08, world+0x1b8; 0x44b40)

* **Built:** 0x44b40 makes a vector of 0x132 (306) ints, all zero, then stores a value into
  most entries one by one (indices are race ids). The values are 0 to 4.
* **Used:** a wild creature of a group whose kind is not 4 or 8 gets level = table[race]
  (zone file §2.4 step 5). Races 306 and above never occur there.
* **Read from:** spawn-table.txt, its first line: `values` followed by the 306 ints, race 0
  first.

## T2. Spawn groups (spawner +0x20, world+0x1d0; 0x44b40)

* **Built:** after the levels, 0x44b40 pushes 120 group records (0x28 bytes each) in a fixed
  order: +0x00 a vector of race ids (1 or 2 races per group, built by 0x44850 from a stack
  array), +0x18 **kind bits**, +0x1c **climate mask**, +0x20 **ground mask**. The order is the
  order the wild-group draw indexes (zone file §2.4: the fitting groups are collected in table
  order, then one is drawn with rand % count).
* **Kind bits** (how the group comes, zone file §2.4): any of 1, 2, 4, 8, 0x10.
* **Climate mask** (compared with the cell's climate bits; 0 = anywhere): 1 mild, 2 cold,
  4 forest, 8 lake, 0x10 hot, 0x20 hot desert, 0x80 culture 3, 0x100 lava, 0x200 kind-27
  places.
* **Ground mask** (0 = never drawn for animals): 1 river edge, 2 river, 4 low shore, 8 trees,
  0x10 highlands, 0x20 animal draws, 0x40 under the sea, 0x80 lava edge, 0x100 lava, 0x200
  swamp, 0x400 swamp edge, 0x800 plain ground (not animals).
* **Read from:** spawn-table.txt, one line per group after `values`:
  `<index> <kind bits> <climate mask> <ground mask> <race…>`, all decimal, index 0..119 in the
  game's order.

## T3. Sibling lists (0x13f0d0, used by variant 0x1420f0)

* **Built:** 0x13f0d0 returns, for a race, a list in a fixed order: pairs of the playable
  peoples' two sexes, a few larger families (one of four races, one of three), and one special
  selector (−99991) that yields a list of five. Every race it does not list is its own list of
  one.
* **Used:** variant(r) = list[rand % size], one draw even for a list of one (main file §3.2).
  The order of each list matters.
* **Read from:** races.txt, one line per selector: `<race> <list…>`, written for every
  selector −200..400 and for −99991 (a race with no list prints itself alone).

## T4. Race pickers

* **Built:** each picker (main file §3.3) builds its list in code, then draws
  list[rand % size] and returns variant(that) (one more draw). people-or-beasts (0x1408e0)
  takes a culture argument and builds its list in four parts: eight people races; then for
  cultures 6 and 11 three repeats of a three-race group, else two races; then for cultures 5
  and 12 three races, else one; then for cultures 9 and 10 six repeats of one race; then nine
  more races. Traveller 1 (0x141d70) draws once more before its pick (to append a second race
  one time in four).
* **Used:** the order and repeats in each list matter, because the pick is by index.
* **Read from:** the picker lists are not written out as lists by the extraction tool;
  pickers.txt records each picker run 600 times on scripted rand: `<rva hex> <seed> <draws>
  <race>`, from which the lists are recovered (the scripted rand is the xorshift32 of main file
  §8.4). The pickers are 0x141480, 0x1408e0 (called with −1), 0x142190, 0x142760, 0x13ffc0,
  0x13feb0, 0x141390, 0x142000, 0x13fa50; 0x13fda0, 0x140490 and 0x141d70 are not recorded
  (each is one to five races). An implementation without game data in the repository
  needs the lists themselves; see main file §9 item 6.

## T5. Races that carry consumables (0x65750 returns 3)

0–16, 18, 43, 44, 45, 46, 48, 49, 51, 52, 54, 76–81, 83, 84, 95, 148, 149, 167, 191, 199, 202,
203, 208, 216, 227, 228, 230, 236, 237, 252, 253, 254, 256, 259, 260, 261, 296, 304, 305.

Category 6 of the same routine (skipped by the marking pass): 73, 82, 85, 110, 225, 226, 229.

## T6. Castle and dungeon races (0x31a800)

By the structure T (the big structure 41–47 of a castle cell's place, or the place kind 8 for
dungeons):

| T | List |
|---|---|
| 8 | 0, 2, 9, 4, 11, 15, 17, 165, 85, 43, 45, 167, 230, 225, 224, 217, 59, 40, 229, 227, 226, 269, 94 |
| 0x29 | 75, 37, 96, 17, 173, 51, 229, 73 |
| 0x2a | 224, 217, 9, 59, 96, 94, 17, 47, 46, 225 |
| 0x2b | 105, 102, 104, 293, 60, 53, 151, 295, 294, 291, 290, 287, 70, 69, 54 |
| 0x2c | 37, 59, 149, 202, 106, 107, 47 |
| 0x2d | 96, 17, 77, 78, 81, 83, 72, 103, 75, 42, 7 |
| 0x2e | 96, 17, 77, 78, 81, 83, 68, 62, 75, 172, 60, 13 |
| 0x2f | 85, 225, 226, 227, 73, 229, 82, 174, 199 |
| any other | 96, 17, 165, 97, 230, 173, 269, 296, 41, 15 |

## T7. What 0x53e80 draws per race

Every race first draws c1, c2, c3, l2 = rand % 100, l3 = rand % 100 (main file §4.2). Then, in
this order (offsets into the appearance; 16-bit model ids; "colour" means bytes +2, +3, +4 =
c3, c2, c1):

| Race | Writes |
|---|---|
| 0 | colour; head (+0x18) = l2 mod 4 + 0x5a3; hair (+0x1a) = l3 mod 10 + 0x5d1 |
| 1 | colour; head l2 mod 6 + 0x5a7; hair l3 mod 11 + 0x5db |
| 2 | colour; head l2 mod 6 + 0x5ad; hair l3 mod 15 + 0x5b3; hands (+0x1c) = 0x265 + (head > 0x5af) |
| 3 | colour; head l2 mod 6 + 0x5c2; hair l3 mod 9 + 0x5c8; hands 0x265 + (head > 0x5c4) |
| 4 | colour; head l2 mod 5 + 0x97; hair l3 mod 6 + 0x9c |
| 5 | colour; head l2 mod 5 + 0xa2; hair l3 mod 6 + 0xa7 |
| 7 | colour; head l2 mod 2 + 0xae; hair l3 mod 6 + 0xb0 |
| 8 | colour; head l2 mod 5 + 0xb6; hair l3 mod 6 + 0xb0 |
| 9 | colour; head l2 mod 5 + 399; hair l3 mod 3 + 0x194 |
| 10 | colour; head l2 mod 5 + 0x197; hair l3 mod 6 + 0x19c |
| 11 | colour; head l2 mod 5 + 0x5e6; hair l3 mod 8 + 0x5f3 |
| 12 | colour; head l2 mod 5 + 0x5eb; hair l3 mod 4 + 0x5fd |
| 13 | head l2 mod 5 + 0x601; hair l3 mod 5 + 0x606 |
| 14 | head l2 mod 4 + 0x60b; hair l3 mod 4 + 0x60f |
| 15 | colour; head l2 mod 6 + 0x1a5; hair l3 mod 6 + 0x1ab |
| 16 | colour; head l2 mod 6 + 0x1b1; hair l3 mod 6 + 0x1b7 |
| 0x94 | colour; head l2 mod 3 + 0x614; hair l3 mod 3 + 0x617 |
| 0x95 | colour; head l2 mod 3 + 0x61c; hair l3 mod 3 + 0x61f |
| 0xa5 | head l2 mod 4 + 0x1c3 |
| 0xec, 0xed | hair l3 mod 4 + 0x626 |
| 0x104 | colour; hair l3 mod 8 + 0x5f3 |
| 0x105 | colour; hair l3 mod 4 + 0x5fd |
| 6 | three more draws r1, r2, r3 → colour bytes (r3, r2, r1); then head = rand mod 3 + 0xcad |
| 0x2e | three more draws → colour (r3, r2, r1) |
| 0x3a | first = (rand mod 2 == 0): head 0x184/0x180, feet (+0x1e) 0x185/0x181, chest (+0x20) 0x183/0x17f, wings (+0x26) 0x186/0x182 (first / not) |
| 0x53, 0x54 | head = rand mod 3 + base (0x118 / 0x11b); hands = 0x120 + (head ≠ base) |
| 0xe3, 0xe4 | head = rand mod 5 + (0x122 / 0x127); hands 0x12e |
| 0x76 | chest rand mod 2 + 0xb3a |
| 0x77 | head rand mod 5 + 0xb30; chest rand mod 9 + 0xb24; hands = feet = rand mod 3 + 0xb2d; tail (+0x22) rand mod 2 + 0xb35; +0x28 = rand × 0.1 / 32767 + 0.4; +0x7c = rand × 0.5 / 32767 − 1.0; +0x80 = rand / 32767 + 3.0; +0x8c = rand / 32767 + 3.0; size = (1.3, 1.3, 1.12 × 1.3) |
| 0x7c | chest 0xae9 − (rand mod 2 ≠ 0) |
| 0x8f | chest rand mod 4 + 0xb10 |
| 0x91, 0x92 | size s, s, s with s = (rand × 0.5 / 32767 + 1.2) × 0.8 |
| 0x9f | k = rand / 32767 + 6.0; size (k, k, 1.12 k) |
| 0xa1 | k = rand × 0.5 / 32767 + 4.0; size (k, k, 1.12 k) |
| 0xca, 0xcb | head rand mod 9 + (0x69 / 0x72); chest rand mod 9 + 0x7b |
| 0xcc | chest rand mod 5 + 0xa4f |
| 0xcd | chest rand mod 5 + 0xa54 |
| 0xce | one draw, unused |
| 0xcf | chest rand mod 2 + 0xa5a |
| 0xd0 | head rand mod 2 + 0x27 |
| 0xd4 | chest rand mod 2 + 0xdd7 |
| 0xe7 | chest rand mod 5 + 0xaef |
| 0xe9 | chest rand mod 6 + 0xaf8 |
| 0x121 | chest rand mod 4 + 0xaf4 |

Floats are single precision in the order written ((float)rand first). The drawn sizes of 0x77,
0x91, 0x92, 0x9f and 0xa1 are then grown by the flags (main file §4.2 step 4); the other races'
sizes are fixed data and already grown. Any race not in this table makes only the five common
draws. The table was checked against looks.bin (every race, 24 scripted calls each) and spot-checked
in 0x53e80's cases (races 0, 1, 6, 0x76, 0xcc, 0xec); the remaining cases are open item 8 of
the main file.
