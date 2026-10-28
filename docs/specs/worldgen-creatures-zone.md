# World generation (C): a zone's creature passes

Part of `worldgen-creatures.md` (read its §0 first: rand, R, H, X, yawA/yawB, "finish", "add",
the spawn point record, spots). Everything here is in the zone generator 0x2ff6f0 (read from
its disassembly, 0x3162b0–0x318f6c) and the routines it calls, plus the daily refresh
0x31c540.

---

## 1. Where the passes run

One creature is made inside the structures: B's cell loop (`worldgen-structures.md` §7 step 4)
makes the **arena master** of cell object 97 right after its dais (§5). It is the only spawn
point made before the spot map, so in a zone that has one it takes the zone's first id.

After the structures (B), in this order:

1. The spot map (0x3162b0, main file §5.1), then the working copy (0x2f9ef0).
2. The cells' own things (0x316780: chests, crates; B).
3. The plants (0x3191b0; B).
4. **The region role's things** (0x11a820; `worldgen-creatures-roles.md`). It draws from the
   zone's rand() where the plants left it, until it reseeds. Most encounters reseed before
   their first draw (roles file §3), so in a .rtrace this step shows as a break in the rand()
   chain right after the plants' (or, when steps 2 and 3 draw nothing, the spot map's) last
   draw; e.g. a quest camp starts with a spot take, then the giver's looks and gear (roles file
   §3.1, "Where its draws fall").
5. The working spot map is copied from the spot map **again** (0x2f9ef0), so every spot is
   available to the cell pass, including those taken in steps 2 and 4.
6. **The cell pass** (§2).
7. **The marking pass** (§3).
8. B's end: the object list is copied to zone+0x78, the items to zone+0x48, every item for which
   0x31ac70 holds gets its region (+0xc, +0x10) = regionAt of its position rounded to zero, the
   lights and doors are listed (0x31d470) and the object boxes marked (0x31d700).

The daily refresh (§4) runs later, when the world first touches the zone.

Zone context computed by 0x2ff6f0 before step 1 (B computes it; this part reads it):

* **c1** = (floor(64·zx / 1024), floor(64·zy / 1024)); the **place** = 0x31a5d0(c1): kind,
  big, variant, radius, centre (for kind 0 the centre is left unwritten; main file §9.1). If
  big, c1 is rounded down to even.
* **style** s = 0x2bb060(c1, big) (the realm; lore-and-artifacts §4.3).
* **culture** = biome of regionAt(centre) (0x2b9080).
* **planner** = 0x115df0 for regionAt(centre) (main file §2). Its marked byte drives §3.
* **dungeon floor** (kind 8 only): F = 0 and cell D = unset. With (ax, ay) = (floor(centre.x /
  16), floor(centre.y / 16)) and h = floor(0x2a2b00(c1) / 2) (a signed halving that truncates
  toward zero), for x from ax − h to ax + h − 1 (outer) and y from ay − h to ay + h − 1: if the
  cell object at (x, y) is 45, 46 or 47 (0x25eb30) and the fortress floor 0x2a1d10(x, y, 0) is
  2, 3 or 4, then F = that floor and D = (x, y) (the last one found wins). (B owns 0x2a2b00 and
  0x2a1d10.) When no cell qualifies, D is never written: the game's value is whatever its stack
  slot held (main file §9.10). Only the floor guards' area (§2.6) reads it; implementations use
  (0, 0).

---

## 2. The cell pass (0x3171a2)

For cx from floor(64·zx / 16) to floor((64·zx + 64) / 16) − 1 (outer) and cy likewise (inner),
the zone's 4×4 cells:

1. h = R(H(cx, cy)); **srand(h mod 32767)**.
2. type = the cell object at (cx, cy) (0x69c60; B). (mx, my) = (16·cx + 8, 16·cy + 8).
3. w = the place weight (0x9bee0) of the zone's place at (mx, my): kind 0 gives 0; else with d =
   the warped squared distance (0x364e0) from the centre and r² = 62500 (250000 when big),
   w = (1 − d / r²)² when d < r², else 0 (single precision).
4. "things" = true. If w > 0.4:
   * place kind 5: **townsfolk** (§2.1); it may set things to false.
   * otherwise, if the kind is neither 8 nor 16, the type is not 0x58, c1 is (0, 0) and the
     type is 24: **the first town's people** (§2.2); things = false.
5. If things and type is 98 or 99: **campfire things** (§2.3).
6. **Region boat** (0x318068): if cx mod 16 = 0 and cy mod 16 = 0 (positive remainders), the
   biome of regionAt(16·cx, 16·cy) is 1, the ground height (0x35ea0, mode 0) at (mx, my) is
   below 10, and count(cx, cy, 16) ≠ 0: an item of type 0x18, kind 2: region = regionAt(16·cx,
   16·cy), per-player flag (+0xc4 |= 2), position = take(cx, cy, 16), angle (+0xc0) = yawB; push
   it to the items (a mod hook may drop it after the draws).
7. **Wild group** (after 0x3181aa): w2 = the place weight at (mx, my) again. If the place kind
   is 0 or w2 < 0.5: if rand % 128 == 0 and the roads value 0x2d4730(mx, my, kind, w2) < 0.25
   (A): q = the area point 0x2d3b60(floor(cx / 64), floor(cy / 64)) (A/B: one point per 64×64
   cells); if (q.x − cx)² + (q.y − cy)² > 16: **wild group** (§2.4) with animals = false.
8. **Castle**: if the castle cell 0x2a8290(cx, cy) has kind ≠ 0: **castle creature** (§2.5).
9. **Dungeon**: if the zone's place kind is 8: **dungeon creatures** (§2.6) with F and D.
10. **Herbs and other items** (§2.7). A cell whose herb test fails skips straight to step 1 of
    the next cell (no water item, no animals).
11. **Animals** (§2.8).

### 2.1 Townsfolk (0x317290)

Per cell:

1. special = 0x400000 if the planner's special town is c1 (0x31acd0), else 0.
2. people = styleRace(s) if styleOdd(s) is false and styleEven(s) is true, else 0.
3. role = the town block role of the cell (0x2af7b0(cx, cy, 5, …); B).
4. If role is 2: take(cx, cy, 2) and append it to the shop spots (zone+0x500).
5. base race by culture 1..13 = (0xca, 2, 0, 0xd, 0xf, 0, 9, 0xd, 7, 0xb, 2, 0xf, 4) (jump
   table 0x319130); any other culture: 2.
6. A "**townsperson** of class k": hostility 3, race = variant(base), class k, people, style s.
7. **House with a door** (type 22–26 or 0x5c (0x2b8030), and type 24, 36, 37, 46, 71, 76, 81,
   89, 92 or 100 (0x31abb0)):
   * role 0: nothing; things stay true.
   * role 2..10: class from (0x84, 0x80, 0x82, 0x81, 0x83, 0x85, 0x86, 0x87, 0x9a)[role − 2];
     spot kind 1 for role 10, else 4; make one.
   * any other role: class 0x8a, spot kind 4; make one only if rand % 10 == 0.
   * Making one: a townsperson of that class; yaw = yawA; flags |= special; looks (not wild);
     gear with level 0; position = take(cx, cy, spot kind); roam 4; yaw = yawB (again); add.
   * Role 10 also gets the keeper's helper after it: hostility 4, race 0x12f; yaw = yawA;
     flags |= special; looks; gear (level 0, people 0); position = take(cx, cy, 3); z +=
     trunc((height × 0.5 + 0.5) × 65536.0) with height = appearance +0x14 (spawn point +0x64);
     roam 4; yaw = yawB; add.
8. **Town hall** (type 0x5b; the game also prints "GUILD HALL CENTER" to its console): a
   townsperson of class 0x9b; yaw = yawA; flags |= special; looks; gear with **level 3** (the
   spawn point's level stays 0); position = ((16·cx + 8)·65536, (16·cy + 8)·65536, base·65536)
   with base = the building base height 0x2d87f0(cx, cy, 0x5b) (B) (no half-block offsets);
   roam 4; yaw = yawB; add. Things = false.
9. **Any other cell:** if rand % 20 ≠ 0, or (cx + cy) mod 2 ≠ 0 (positive remainder), nothing.
   Otherwise a new point:
   * rand % 10 ≠ 0: a townsperson of class 0x8a; flags |= special; roam 0x10; then three draws:
     rand % 2 ≠ 0 → roam |= 0x20; rand % 2 ≠ 0 → |= 8; rand % 2 ≠ 0 → |= 4.
   * else a traveller: hostility 4; race = (32, 30, 33, 20, 26, 19, 56, 34)[rand % 8] (no
     variant; binary 0x473a70 and 0x473a90); roam 0x10.
   * Then yaw = yawA; looks; gear (level 0); position = take(cx, cy, 1); add.

### 2.2 The first town's people (0x317c54)

Three points, no gear, no yaw draw, each: hostility 3; race; name (main file §4.4); looks (not
wild); flags |= extra; position = take(cx, cy, 2); roam; add.

| # | Race | Name | Extra flags | Roam |
|---|---|---|---|---|
| 1 | 0x101 | Wollay | 0x2000 | 0x30 |
| 2 | 0x102 | Pixxie | 0x2000 | 0x30 |
| 3 | 0x13 | Joschi | 0x102000 | 0x10 |

### 2.3 Campfire things of cells 98 and 99 (0x317e7b)

n = rand % 2 + 1. For k = 0..n−1 a static object (B's record; default scale 1): type 0x53;
position = take(cx, cy, 1); turn = rand % 4. Then, if k > 0 or the type of the cell is 99:
low = (z < 25·65536) (0x2fa350);
* rand % 4 ≠ 0 → type 0x5f if low, else 0x60;
* else if low and rand % 3 == 0 → 0x5e;
* else r = rand % 7: r 0..5 → (0x25, 0x26, 0x27, 0x28, 0x5d, 0x1c)[r]; r 6 → 0x83 + rand % 2.

Each is put with 0x1194b0 (kept where it is if inside the zone; B).

### 2.4 Wild groups (0x2fdf90, 0x47e20, 0x31b910)

Arguments: the cell and **animals** (a bool).

**Choosing a group (0x47e20).** At (x, y) = (16·cx + 8, 16·cy + 8):

* culture = biome of regionAt(x, y); object = the cell object (0x2d53d0 with the place at
  the cell point) — ground = 8 if it is 1..4, else 0.
* The column values at (x, y) with the place at (x, y) (A): forest F, warmth T, dryness D,
  lake region K, lava region V, kind-27 weight W, river Rv, lava Lv, swamp Sw, highlands G,
  height Z (0x2b4450, mode 0).
* climate: env = 4 if F > 0.5 else 0; if T < 0.2: env |= 2. mild = (T ≥ 0.2 and F ≤ 0.5).
  D < 0.1 and T > 0.9 → env |= 0x10, mild = false. D > 0.8 and T > 0.9 → env |= 0x20, mild =
  false. D < 0.4 and T > 0.8 → env |= 0x10. K > 0.5 → env |= 8, mild = false. V > 0.5 → env =
  0x100 (replaces), mild = false. culture 3 → env |= 0x80, mild = false. W > 0.5 → env |= 0x200,
  mild = false. Finally mild → env |= 1.
* ground: 0 < Z < 10 → |= 4; Z ≤ 0 → |= 0x40; 0.25 < Rv < 0.5 → |= 1; Rv > 0.5 → |= 2;
  0.25 < Lv < 0.5 → |= 0x80; Lv > 0.5 → |= 0x100; 0.25 < Sw < 0.5 → |= 0x400; Sw > 0.5 →
  |= 0x200; G > 0.75 → |= 0x10. Not animals: if ground is 0 it becomes 0x800. Animals:
  ground |= 0x20.
* Fitting groups, in table order: (climate mask 0 or env & mask ≠ 0) and ((not animals and
  ground mask 0) or ground mask & ground ≠ 0) and a non-empty race list. None → no group.
  Else the group = fitting[rand % count].

Comparisons are single precision with the constants 0.5, 0.2, 0.1, 0.9, 0.8, 0.4, 10, 0.25, 0.75.

**Making the group (0x2fdf90).**

1. No group → stop. spot kind S = 5 for animals, else 1. count(cx, cy, S) = 0 → stop.
2. k = one of the group's kind bits 1, 2, 4, 8, 0x10 that are set, in that order, as a list,
   picked with rand % size (0x31b910; 0 when none is set, no draw).
3. n = 1; k 2, 4 or 8: n = min(rand % 3 + 3, count(S)); k 0x10: n = min(rand % 8 + 6, count(S)).
4. leader odds = 2 for k 8, 8 for k 4, 20 otherwise. shared level = rand % 5.
5. For i = 0..n−1: hostility 1; race = the group's races[rand % size]; level = shared level for
   k 4 or 8, else the race level table (main file §3.1). If the race is 0xaf or 0xb1..0xb4:
   hostility 7; otherwise if i = 0 and rand % odds == 0: flags |= 0x2200. Looks with **wild
   set**; gear; position = take(cx, cy, S); roam = 0x30 for k 8, else 0x10; add.
   * If the race is 0x10e, its mount follows: a new point (hostility 1, the default), race 0xa5,
     level = the rider's level; looks with wild set; gear; then position = the rider's, flags =
     the rider's (replacing what the looks gave), +0x40 = the rider's id; add.
6. k 8 also makes a **camp**: m = min(count(cx, cy, S), 6) (count read now, after the group took
   its spots). For j = 0..m−1: object type 0x53, position = take(cx, cy, S), turn rand % 4; if
   j > 0: rand % 4 == 0 → (z < 0x190000 and rand % 3 == 0 → 0x5e; else (0x25, 0x26, 0x27,
   0x28, 0x5d, 0x1c)[rand % 6]); otherwise type = 0x5f, or 0x60 when z > 0x18ffff. Kept (0x129b50)
   only when its x and y lie inside the zone.

### 2.5 Castle creatures (0x2fb850 with 0x31a800)

For castle cell (cx, cy) with castle bytes (kind, detail, style, steps):

1. Its place: (px, py) = (floor(16·cx / 1024), floor(16·cy / 1024)), rounded to even when big;
   kind and big at that cell; structure T = 0x2aa270(px, py, kind, big).
2. count(cx, cy, 6) = 0 → stop.
3. **area** = every cell (x, y) of the 8×8 block (floor(cx / 8)·8 .. +7, x outer) whose castle
   kind is not 0 and detail is 2.
4. level = min((steps >> 2) + 1, 4). style σ = 0x2bb060(px, py).
5. By castle kind (anything else: stop):
   * **2**: hostility 1, race 0xe2, flags |= 0x2200, level. srand(R(H(cx, cy) + 0x53735));
     **castle race** (below); srand(R(H(cx, cy) + 0x8043e)); race = variant(race); looks (not
     wild); gear; position = take(cx, cy, 6); roam 4.
   * **4** (the game prints "END BOSS?"): hostility 1, race 0xe3, flags |= 0x22200, level.
     srand(R(H(px, py) + 0x20d2d)); castle race; srand(R(cx + 57·py + 0x1191ac)) (note: the cell's
     x with the place's y); variant; looks; gear; take(cx, cy, 6); roam 4.
   * **6**: hostility 1, level. srand(765325 − N(H(cx, cy)) div 21474) (0xbad8d). up = false; if
     rand % 8 == 0: level = min(level + 1, 4), up = (rand % 2 ≠ 0). seed = R(H(px, py)) +
     R(H(floor(cx / 8), floor(cy / 8))) + X(level) (all wrapping), + 1 when up; srand(seed);
     castle race; srand(R(H(cx, cy) + 0x5a48)); variant; flags |= 0x2000; looks; gear;
     take(cx, cy, 6); roam 0x10 when up, else 4.
6. yaw = yawB; area → +0x900. If appearance +0x14 > 6.0: k = 6.0 / that value; appearance
   +0xc, +0x14 and +0x10 are each multiplied by k (k × v). Add.

**Castle race (0x31a800)** for structure T, style σ, and the point:

1. The list for T (tables §T6).
2. If styleOdd(σ) and rand % 8 == 0 — or the list is empty (never) — the **style** path: race =
   styleBoss(σ, rand); people = styleRace(σ); style = σ; if styleEven(σ) is false, flags |=
   0x100000.
3. Otherwise race = list[rand % size].
4. race = variant(race).

### 2.6 Dungeon creatures (0x2fa7b0)

For a cell of a kind-8 place, with F and D (§1). (px, py) = the place cell of the cell
(rounded for big); ph = H(px, py); lvl = levelAt(px, py); σ = 0x2bb060(px, py); T = the
structure (0x2aa270 with the place kind; for kind 8 that is the kind-8 list of tables §T6,
since 0x31a800 is given the place kind 8 directly).

**Guards path.** If R(ph + 0x10581) is odd, or the region item of the place (main file §2.3) is
8:

* If the cell's object is 45..47, go to the fortress path.
* gate = the first of the neighbours (cx−1, cy), (cx+1, cy), (cx, cy−1), (cx, cy+1) whose object
  is 0x2e; none → stop.
* n = 1 if R(ph + 0x11c135) is odd, else (R(ph + 0x3c77a) & 3) + 3 (0x2c30b0).
* srand(109003 − N(ph) div 21474) (0x1a9cb).
* For i = 0..n−1: hostility 1; race = variant(2) (one draw; replaced below); area += gate;
  level = max(lvl − 1 + rand % 2, 0); srand(R(ph + 0x2fdf5d)); castle race (T = 8); flags |=
  0x2000, and |= 0x200 when n = 1; srand(R(H(cx, cy) + 0x93ac8) + i); race = variant(race);
  looks; gear; position = take(cx, cy, 0); roam 0x14; yaw = yawB; add.
* Stop.

**Fortress path** (also when the guards test fails), only if the cell object is 45..47:

1. level = lvl; if the fortress room 0x2a25a0(cx, cy, …) is 2: level = max(level − 1, 0).
2. **Boss** if (cx, cy) = (floor(centre.x / 16), floor(centre.y / 16)) of the place: flags |=
   0x22200; hostility 1; srand(R(ph + 0x1d5dde1)); castle race; level; roam 0x10; the role
   treasure of the place (main file §2.3), if any, into the inventory (quantity 1);
   srand(R(H(cx, cy) + 0x1260b4)); variant; looks; gear; position = take(cx, cy, 2); add (no
   yaw). Stop.
3. **Floor guards**, for i = 0, 1: spot kind S = 2 for i 0, 8 for i 1. Only if the fortress floor
   0x2a1d10(cx, cy, i) is 13 and count(cx, cy, S) ≠ 0: flags |= 0x2000; srand(R(ph + 0x2ffdd));
   * F = 3: hostility 1; castle race; flags |= 0x200; roam 0x10; an item into the inventory:
     category 0x16, id = ((py mod 64)·64 + (px mod 64)) mod 20 with positive remainders,
     region (+0xc) = regionAt(centre); level.
   * F = 4: hostility 7; race 0xd6; level; area += D.
   * else: hostility 1; castle race; flags |= 0x20000; level; roam 0x10; area += D.
   Then srand(R(H(cx, cy) + 0x1260b4) + i); variant; looks; gear; position = take(cx, cy, S); yaw =
   quarter yaw; add.
4. **A group**, if count(cx, cy, 2) ≠ 0 and the floor exists 0x2b8b10(cx, cy, 0):
   srand(765325 − N(H(cx, cy)) div 21474); if rand % 6 ≠ 0 stop; n = rand % 4 + 1. For i =
   0..n−1: hostility 1; srand(R(H(cx, cy) + 0x305d5) + i); level = max(level − 1 + rand % 2, 0)
   (level from step 1); up = false; if rand % 8 == 0: level = min(that + 1, 4), up = true;
   srand(X(level) − N(ph) div 21474 + (100001 if up else 100000)); castle race;
   srand(R(H(cx, cy) + 0x127fd) + i); variant; flags |= 0x2000; looks; gear; position =
   take(cx, cy, 2); roam 0x10 if up else 4; yaw = yawB; add.

### 2.7 Herbs and items (0x31832b, 0x3186a9, 0x3187f9, 0x3188b3)

These make items (zone+0x30, B's records: type byte, kind +4, position +0xa0, angle +0xc0).
They are here because they share the cell's rand() with the creatures.

**Herbs**, if count(cx, cy, 16) ≠ 0:

1. tree = the cell object is 1..5. The herb record is reset (0x118c20) **without** touching
   its position (+0xa0), which keeps the position of the last herb placed in this zone (stack
   data before the first; main file §9.2). Type 0xb, kind 0x16.
2. below = the block at (x >> 16, y >> 16, (z − 65536) >> 16) of that kept position (0x2fea10).
   kind = 0x18 if below's type is 10, else 0x16. If rand % 5 == 0: kind = 0x1d.
   n = rand % 40 − 38.
3. If tree and the object is not 5: kind = 0xf; n = rand % 4; if rand % 2 ≠ 0: kind = 0x24,
   n = 1. Then, with h of step 1 of the cell pass:
   * object 1: T, D = warmth and dryness at (mx, my) (with the place there). If h mod 10 = 0 and
     0.3 < T < 0.75 and D > 0.2: type 1, kind 0xe → *checked*. Else if h mod 10 = 1 and T > 0.6:
     kind 0x28 → checked.
   * object 4: h mod 10 = 0 → kind 0x2b, checked; h mod 10 = 1 → kind 0x29, checked.
   * checked: n = count(cx, cy, 16) / 8 + 1.
4. If not checked and kind is 0x16 or 0x1d: read below again as in step 2; if its type is not 4,
   **end this cell** (go to the next cell).
5. For n times (none when n ≤ 0): angle = yawA; position = take(cx, cy, 16), which also becomes
   the kept position; push.

**Water**: if rand % 10 == 0 and count(cx, cy, 18) ≠ 0: p = take(cx, cy, 18); at its blocks
rounded to zero, if warmth < 0.2: an item type 0xb, kind 0x27, position p, angle yawA.

**Shells** (kind-5 spots): if rand % 30 == 0 and count(cx, cy, 5) ≠ 0: position = take(cx, cy, 5);
angle yawA; type 0xb, kind 0x13.

**Crops**: object 27 → kind 0x10, 32 → 0x11, 33 → 0x2a; others nothing. c = count(cx, cy, 21);
w = rand % 5 + 4; n = min(c, w). For each: position = take(cx, cy, 21); angle yawB; type 0xb.

### 2.8 Animals (0x3189d6)

1. If rand % 64 == 0 and count(cx, cy, 16) ≠ 0: hostility 7; position = take(cx, cy, 16). At
   (X, Y) = its blocks rounded to zero, with the neighbourhood and place there:
   * lava region > 0.5: race 0x7e if rand % 2 == 0, else 0x82;
   * else warmth < 0.25: 0x79 if rand % 2 == 0, else 0x7a;
   * else dryness < 0.2: 0x7c; then if rand % 2 ≠ 0 and warmth > 0.8: 0x80;
   * else r = rand % 3: 0x78, 0x7b, 0x7d; then if rand % 10 == 0: 0x7f.
   Looks (not wild); add. No gear, no yaw.
2. If rand % 16 == 0 and count(cx, cy, 5) ≠ 0:
   * rand % 4 == 0: a **wild group** with animals = true. Done.
   * else hostility 7, race 0x84, and the race chain:
     - draw rand % 4. If it is not 0, the race stays 0x84 and the chain ends here (no more
       draws).
     - Only when it is 0: race = 0x83 if rand % 2 ≠ 0, else 0x85. Then, still inside this
       branch, for r in 0x87, 0x88, 0x89, 0x8a in turn: draw rand % 2; if it is not 0 stop
       (the race keeps its last value), else race = r.

     So the race costs 1 draw (0x84, 3 in 4), or 2 + k draws with k = 1..4 chain draws. Then
     looks (not wild); position = take(cx, cy, 5); add. Expected shares: 0x84 3/4, 0x83 or
     0x85 1/8, 0x87–0x8a 1/8 (0x87 1/16, 0x88 1/32, …).

     Evidence (errata 2026-10-27): the assembly at 0x318ce3 jumps straight to the looks call
     (0x318da1) when rand % 4 ≠ 0, skipping the whole chain (0x318cff–0x318d9a). The 41 shore
     animals (hostility 7, race 0x83–0x8a) of Extract/oracle/*.spawns are 32 × 0x84, 5 × 0x83
     or 0x85, 4 × 0x87–0x8a (zones (−15, 8), (−16, 26), (−2, 14), (12, −7)), as these shares
     predict; zone (−16, 9) keeps 0x84. The one in the generated sets, gen2/genb/genr zone
     (−335, −11) race 0x88 at (−21396, −648, 109), has the draws in its .rtrace (record 2861 on):
     rand % 16 = 0 (call site 0x318c57), % 4 = 2 (0x318c8c: no wild group), % 4 = 0 (0x318ce9),
     % 2 = 1 (0x83), then 0, 0, 1 (0x87, 0x88, stop).

---

## 3. The marking pass (0x318e28)

Only if the planner's marked byte is set. For every spawn point of the zone in id order:
skip unless hostility is 1 or 3; skip hostility 3 with flag 0x2000; skip races of category 6
(0x65750: 0x49, 0x52, 0x55, 0x6e, 0xe1, 0xe2, 0xe5). Then if flags & 0x40200 ≠ 0, or else
rand % 50 == 0 (a draw only in that case): flags |= 0x4000000. The draws continue the last
cell's stream.

---

## 4. The daily refresh (0x31c540)

Runs when the world's day (world+8) differs from the zone's (zone+0x4f8); then the zone's day
becomes the world's.

### 4.1 Undo the last refresh

Every spawn point of the refresh list (zone+0x4e8) is removed from the spawn map and freed;
the list is emptied. The object list is restored from zone+0x78; if it differs from what it
was, the zone is queued for a rebuild of its object data (world+0x1a0; zone+0xfa = 1). No
blocks change.

### 4.2 Shoppers

srand(R(H(zx, zy) + 0x4373e1) + day). If the shop spot list (n entries) is not empty:
count = rand % (n + 1), at least 2. For i = 0..count−1: hostility 3; level = rand % 5; race =
variant(people-or-beasts(−1)) (four draws); roam 0x20; class = rand % 4 + 1; looks; gear;
position = shop spot i mod count (that is, spot i); add; append to the refresh list. The count
can exceed n only when n = 1 (count is then 2): the second shopper's spot lies past the end of
the list, and the game reads heap memory there (genr_seed_123 zone (−18, −18): that shopper
stands at a nonsense position; main file §9.4). Implementations give it the last spot.

### 4.3 Travellers' camp

1. Only if (3·zy + day + zx) mod 5 = 0 (positive remainder), then only if rand % 5 == 0.
2. n = rand % 4 + 1; level = rand % 5; one draw dropped; fire = (rand % 4 == 0).
3. (cx, cy) = (floor(64·zx / 16) + rand % 4, floor(64·zy / 16) + rand % 4).
4. race = people-or-beasts(biome of regionAt(16·cx + 8, 16·cy + 8)) (variant included).
5. S = 5 if count(cx, cy, 5) ≠ 0, else 1; if count(cx, cy, S) = 0 stop.
6. hostility 3, class 0, people 0, style 0, extra flags 0. If rand % 4 == 0: race 0x130, class
   0x9c, n = 1. Otherwise, with the zone's place (at c1 unrounded):
   * place kind 5: fire = false (the town's people of step 4).
   * else σ = 0x2bb060 of the cell's place cell (floor(16·cx / 1024), floor(16·cy / 1024)); if
     rand % 4 == 0: people = stylePeople(σ); hostility 1 if people is 2..6, else 3; by people:
     1 → class 0x8d, race = traveller 1; 2 → class 0x8e, traveller 1; 3 → imps (0x13fda0);
     4 → beast leader; 5 → undead leader; 6 → race 0xec; 8 → class 0x8f, kind-8 people
     (0x140490); others keep step 4's race.
     Else: race = styleBoss(σ, rand); people = styleRace(σ); style = σ; hostility 1 if
     styleOdd(σ), else 3; extra flags 0x100000 if styleEven(σ) is false.
7. For i = 0..n−1: hostility; level; race = variant(race); roam 0x30 with a fire, 0x1030
   without; class; people; style; flags |= extra; looks; gear; position = take(cx, cy, S); add;
   append to the refresh list; put its id in the group set.
8. Every point of the refresh list (shoppers included) gets the group set (+0x910) =
   all the travellers' ids.
9. With a fire: object 0x53 at take(cx, cy, S), turn rand % 4, kept only inside the zone. Then
   for k = rand % 4 times: r = rand % 8: 1 → 0x1c, 2 → 0x60, 3 → 0x28, 4 → 0x26, 5 → 0x27,
   6 → 0x25, 7 → 0x83 + rand % 2 (0x85 instead when the race is 0x2b, 0x2d, 0xa7 or 0xe6),
   0 → 0x5d; position = take(cx, cy, S); turn rand % 4; kept only inside the zone.

---

## 5. The arena master (0x303315, in B's cell loop)

Cell object 97 is the dais of a kind-24 place (the arena; `worldgen-structures.md` §6.3 rule 8
puts it at (ax − 3, ay)). B's cell loop builds the dais (`worldgen-structures-buildings.md`
§12) for a near cell of type 97, after the cell's srand(rnd(H(cx, cy)) mod 10000000), and the
same routine then makes the master, drawing on from that srand (the dais itself draws nothing).
Interface with B: input the cell (cx, cy) and the base height b = 0x2d87f0(cx, cy, 97) (B's
building base height); output one spawn point added to the zone. Type 97 has reach 0
(`worldgen-structures-tables.md` T6), so only the zone that contains the cell builds it and
makes its master. In this order:

1. A new spawn point (constructor defaults, main file §0.3).
2. hostility 3.
3. race = variant(0x104) (one draw, main file §3.2).
4. class byte = 0xa1 (the arena master's class: gear picks no class for it, and the arena stages
   of main file §7.2 look for it). people = 3. yaw = 270.0. Level stays 0, style 0, flags 0.
5. Looks (0x53e80) with the race, not wild (main file §4.2); for race 0x104 that is the five
   looks draws and flags 0x20000.
6. Gear (0x511e0) with hostility 3, the race, level 0, the flags from step 5 and people 3 (main
   file §4.3). For race 0x104 and class 0xa1 that is: the consumables draw (B); the general case
   D with no class pick (the class byte is not 0) and no weapon; armour list (1, 0x1d, 4) (the
   right weapon is empty and people is not 6), so four armour draws (chest, shoulder, hands,
   feet) and three jewel draws (neck, right ring, left ring); then the 9 + 9 modifier draws of
   H. The class stays 0xa1.
7. Position = ((16cx + 8)·65536, (16cy + 8)·65536, (b + 4)·65536): the middle of the cell on
   top of the four-step dais, with no half-block offsets.
8. roam = 4. Everything else keeps its default: home (0x7fffffff, 0x7fffffff), no area, group,
   prerequisites or requirements, stage byte (+0x4c) 0.
9. Add it (main file §1.1).

So the master costs 32 draws (1 + 5 + 1 + 4 + 3 + 18). Oracle: gen_seed_123 zone (−152, 41)
(and the same zone in gen2, genb, genr): its first spawn point id (base + 0) is hostility 3, race
0x104, people 3, class 0xa1, yaw 270, flags 0x20000, roam 4, level 0, at (−9720, 2648, 16)
blocks = (16·(−608) + 8, 16·165 + 8, 12 + 4); chest, shoulder, hands and feet materials 0x1d,
1, 0x1d, 0x1d; neck (category 8) material 12, right ring 11, left ring 12; and the zone's
.rtrace opens with those 32 draws (0x142148, 0x53f27 … 0x53fb4, 0x512ae, 0x52025 … 0x520f1,
0x528d5 … 0x529db) before the spot map's first draw (0x3162d2). The arena's teams in that zone
(roles file §3.11) carry requiredStage 1, 3, 5, 7.

## Errata (from the clean-room implementation, checked against the oracles)

- **Fixed (§5, new).** The arena master of cell object 97 was unspecified (it was called a
  shrine keeper in part B); evidence as in §5.
- **Settled (§4.2).** Shoppers past the end of the counter list: the game reads heap memory
  (genr_seed_123 zone (−18, −18)); implementations use the last counter.
- **Settled (§1).** The dungeon cell D when no floor is found is unwritten stack data;
  implementations use (0, 0).
- **Fixed (§2.8 step 2, 2026-10-27).** The 0x87–0x8a chain runs only inside the rand % 4 == 0
  branch; when that draw is not 0 the shore animal stays 0x84 and draws nothing more for its
  race. Evidence in §2.8.
