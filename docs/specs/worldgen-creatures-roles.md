# World generation (C): what a place gets from its region role (0x11a820)

Part of `worldgen-creatures.md` (conventions in its §0, planner in §2, spots in §5, the race
pickers in §3.3 and tables §T4). 0x11a820 runs once per zone, after the plants and before the
cell pass (zone file §1), with the zone's planner. It draws from the zone's rand() where the
plants left it; every builder below that reseeds says so.

---

## 1. Entry

1. Empty the zone's mission list (zone+0x498).
2. **P** = the zone's place cell: c1 of the zone, rounded to even when big (bigCell(c1)). kind =
   placeKind(P, big); role = roleAt(P) (0x93480); lvl = levelAt(P) (0x12a200); centre =
   cellPoint(P, big); ph = H(P.x, P.y). All builders use these.
3. role 0: nothing. Roles 6–13: **role item** (§2). Role 21: **tower people** (§4.1). Role 19:
   **witch's hut** (§4.2). Role 20: **boss with guards** (§3.10) with boss 0xaf, guard race 0xe3,
   people 4. Role 22: **brazier keepers** (§4.3). Role 23: **spider nests** (§4.4). Any other
   role: **encounter** (§3).

### 1.1 Shared helpers

* **inZone(c)**: the cell c is one of the zone's 4×4 cells.
* **Place cells(limit, fits)**: for x from floor(1024·P.x / 16) to floor((1024·P.x + E) / 16) − 1
  (outer) and y likewise, E = 0x800 when big else 0x400: keep (x, y) when the place weight at
  (16x + 8, 16y + 8) is greater than limit and fits(x, y). The weight is computed inline as in
  zone file §2 step 3 (kind 0 → 0; else (1 − d / r²)² for d < r², r = 250 or 500). If none is
  kept, the list is the single centre cell (floor(centre.x / 16), floor(centre.y / 16)).
* **Plain cell** (object type t): t ≤ 12 with bit t of 0x1c3f set (0–5, 10, 11, 12), or t = 99.
* **Ground tests** at (16x + 8, 16y + 8) with the place there (0x128970, 0x128fd0, 0x128dc0):
  false if the place's variant is not 0 and its weight is below 0.8; then *lava* (lava > 0.5),
  *river* (river > 0.5), or *open* (river, lava and swamp all ≤ 0.5, and the ground height
  (mode 0) ≥ 1.0). (A provides the column values.)
* **Fits the kind** (0x128aa0), by the place kind and the cell's object t: 1, 2, 3 → t in
  0x16..0x18; 4 → t in 0x23..0x26 but not 0x25; 7 → 0x34; 0x15 → 0xc; 0x16 → 0x57..0x59;
  0x1d → 10..12; 0x22, 0x23 → 99; 0x25 → 0x39; 0x26 → 0x45, 0x46; any other kind → plain cell and
  open.
* **Fits the role** (0x128cc0, with the place's landmark L from 0x2ba990): L ≠ 0 → t = L;
  kind 13 → plain and river; kinds 14, 19 → plain and lava; else fits the kind.
* **Fits a group** (0x128c10): kinds 0x15, 0x1d → t in 10..12; 0x22, 0x23 → t = 0x62; else
  plain and open.
* **Camp cells** = place cells(0.5, plain and open).
* **Kind cells** = place cells(0.5, fits the kind).
* **Shuffle** (0x115b80): for i = 1 .. n − 1: m = i + 1; draw r until r / m < 32767 / m or
  32767 mod m = m − 1 (integer division; most draws pass); swap element i with element r mod m.
* **Treasure** = the role treasure of P (main file §2.3); "carrying it" adds it to the
  inventory with quantity 1 (0x4e810; the tab follows the item's category) when it exists.

---

## 2. Role items (roles 6–13)

1. L = the landmark of the place (0x2ba990; B). limit = 0.5 if L ≠ 0, else 0.75.
2. cells = place cells(limit, fits the role with L). pick = cells[R(ph + 0x4192) mod n].
3. If inZone(pick): an item (0x118c20, then): type 0x18; kind by role 6..13 = 1, 2, 0x12, 3,
   5, 7, 8, 9; region (+0xc) = regionAt(centre); per-player flag (+0xc4 |= 2); position =
   cellSpot(pick); angle = yawB; push to the items (a mod hook may drop it after the draws).

---

## 3. Encounters (0x1274d0)

srand(R(ph + 0x21559a6e)). e = the encounter 0x119a00(P) (conversation-script §5.5, with the
world's day). Then by e, as in the table. Only the quest camps (e 9–16) choose a **giver**:
0x131 if role is 1, else a people pick (0x141480, two draws). That choice is made inside their
cases, first, before any other pick of the case (for e 9, before the people-or-beasts pick of
the group race); no other encounter draws it.

| e | What |
|---|---|
| 0 | guarded chest (§3.2) |
| 1 | lonely creature (§3.9) |
| 2 | boss camp (§3.4): boss = R(H(u.x, u.y) + 0x1c52c9) mod 3 + 0xef with u = the zone's c1 **not** rounded; races (0xec); people 6 |
| 3 | boss camp: 0xaf; races (227, 85, 226, 225); people 4 |
| 4 | boss camp: 0xb2; (0x60, 0xe6); people 5 |
| 5 | boss camp: 0xb3; (17, 173, 166, 165, 0xe6); people 5 |
| 6 | boss camp: 0xb0; (37, 40, 38, 39); people 0 |
| 7 | boss camp: 0xb4; (0x57); people 0 |
| 8 | boss camp: 0xb1; (0x35); people 0 |
| 9 | quest camp (§3.1): giver, then group race = people-or-beasts(−1), group people 0 |
| 10 / 11 | quest camp: giver; group 0x2e / 0x2d; people 0 |
| 12 / 13 / 14 | quest camp: giver; group 0x11 / 0x10d / 0xe6; people 5 |
| 15 / 16 | quest camp: giver; group 0xe3 / 0xe2; people 4 |
| 17 | group camp (§3.5): leader 0x6b, flags 0x40000, races (0x6b), people 0 |
| 18 | group camp: r = max(lvl − 1, 0) + 0x25; leader r, flags 0x40000, races (r), people 0 |
| 19 | a = undead, b = undead, leader = undead leader (picks in that order); flags 0x200; (a, b); people 5 |
| 20 | a, b = beasts, leader = beast leader; 0x200; (a, b); people 4 |
| 21 | leader = variant(0xec); 0x200; (0xec); people 6 |
| 22 | a, b, leader = lizards; 0x40000; (a, b); people 0 |
| 23 | a, b, leader = golems; 0x40000; (a, b); people 0 |
| 27 | arena teams (§3.11) |
| 28, 29 | nothing (0x1223f0 only looks the place up) |
| 30 | leader = undead; boss with guards (§3.10) as leader: guard race 0xe6, people 5 |
| 31 | boss with guards: boss (R(H(P) + 0x1d7d7d) & 1) + 0xb2; guard 0xe6; people 5 |
| 32 | leader = beasts; boss with guards as leader: guard 0xe3, people 4 |
| 24, 26 | unreachable (0x123210, 0x1224c0) |

### 3.1 Quest camp (0x11f280)

Arguments: giver race, giver class 0, giver people 0, group race G, group people Gp.

1. bigUnaligned = bigCell(P) (true for any big place). layouts = (0, 3); if Gp is 4 or 5:
   (4, 2); if G is 0x2d, 0x2e, 0x11, 0xa5, 0x60, 0xe2 or 0x10d: (1). layout =
   layouts[R(ph + 0xe2ef) mod size]. **holds** = the layout is 0, 2 or 4.
2. srand(R(ph + 0x9a11)). The chest item: category 0x16, id ((P.y mod 64)·64 + (P.x mod 64))
   mod 20 (positive remainders), region (+0xc) = regionAt(centre), level (+0x14) =
   min(lvl + 1, 4). (No draws.)
3. pick = kind cells[R(ph + 0x4192) mod n]; others = place cells(0.5, not pick and fits a
   group).
4. A **guard at cell c** is: hostility 1; level lvl; race = variant(G); people Gp; roam 0x30;
   flags as given; looks; gear; position = cellSpot(c).
5. If inZone(pick):
   * object type = (R(ph + 0xeb2a) & 3) + 0x54 for layout 0, else (–, 0x58, 0x5a, 0x5b, 0x59)
     [layout]; when holds, the object's item (+0x38) is the chest item; position =
     cellSpot(pick); turn = R(ph + 0x326a0) & 3; settle it (0x31b2d0, not on the ground; B).
   * The giver: hostility 3, level lvl, race and class and people as given, roam 0x30, flags
     0x3000, home P; looks; gear; position = the object's; add; then it carries the treasure.
   * The object's scale = (giver's appearance +0xc) / 0.8; put the object (0x1194b0).
   * Not holds: n = (R(ph + 0x5d14e) & 3) + 5 guards at pick with flags 0x2000, each with
     area += pick and home P, added. Stop.
6. Else, not holds: stop.
7. Holds: small = not bigUnaligned. pick2 = others[R(ph + 0x19dbe) mod n]. If inZone(pick2): a
   holder = a guard at pick2 with flags 0x2200 that carries the chest item (after its spot),
   added; then, when small, n = R(ph + 0x5d14e) mod 3 + 4 more guards at pick2 (flags 0x2000),
   added.
8. small: stop. Big places: srand(R(ph + 0x9a11)) again; rest = others shuffled; keep the
   first max(min(n, 32), n / 8) (n = |others|); for each kept cell c inside the zone, in order,
   a guard is made but never added: hostility 1, level lvl, race = variant(G), people Gp, roam
   0x30, flags 0x2000 (set before the looks, so the looks and gear see 0x2000); looks; gear;
   position = cellSpot(c) (a take, which uses up the spot). All its draws happen.

**Where its draws fall.** The camp runs inside step 8 of the structures order
(`worldgen-structures.md` §2: after the spot map, the cells' things C3 and the plants C2, before
the cell pass), from the srand of step 2. Nothing before the object's spot draws, so the first
draws are: the object's cellSpot (one take, 0x31bd90, when the cell has spots of a counted kind),
then the giver's looks (0x53e80) and gear (0x511e0) (step 5; the giver has no variant draw);
then, when not holds, each guard's variant (0x1420f0), looks, gear and cellSpot, in that order.
In zones whose cells' things and plants draw nothing, these draws come straight after the spot
map's draws. Example (gen_seed_123): zone (−183, 89), P (−12, 5), ph 273: the camp object's take
and the giver (race 0x131, role 1) only; zone (−171, 198), P (−11, 12), ph 673: the take, the
giver, then 7 guards ((R(ph + 0x5d14e) & 3) + 5). The first draw after the reseed is the take:
srand(R(273 + 0x9a11)) gives 25167, srand(R(673 + 0x9a11)) gives 1406, as in the traces. These
are not the planner: building a planner (main file §2) uses only the hashes R and X, never
rand().

### 3.2 Guarded chest (0x11d970, e 0)

1. srand(R(ph + 0x9a11)); race = people-or-beasts(−1).
2. item: category 0x16; id = ((P.y mod 64)·64 + (P.x mod 64)) mod 20 (positive remainders);
   level (+0x14) = min(lvl + 1, 4).
3. pick = kind cells[R(ph + 0x4192) mod n]; not inZone → stop.
4. Object 0x15 at cellSpot(pick); turn R(ph + 0x326a0) & 3; settled (not on the ground); put.
5. n = (R(ph + 0x5d14e) & 3) + 5. For i: hostility 1, level lvl, race = variant(race), roam 0x30,
   flags 0x2000, home P; i = 0 also flags |= 0x200 and carries the item (quantity 1); looks;
   gear; position = cellSpot(pick); area += pick; add.

### 3.3 Camp guards

A **camp guard** of race list Rs, people p, class k, flags f at cell c: hostility 1; level lvl;
race = variant(Rs[rand % size]); people p; roam 0x30; flags f | 0x2000; class k; home P; looks;
gear; then if the race is 0xec or 0xed and rand % 2 ≠ 0: position = take(c, kind 0) and mode
0x8a (it drops from above when it appears, main file §7.1); else position = cellSpot(c).

A **camp traveller** at cell c (with the camp's people p): hostility 3; level lvl; p 4 or 5 →
race = traveller 1, class 0x8d, people 1; else race = kind-8 people (0x140490), class 0x8f,
people 8; roam 0x2030; flags 0x2000; looks; gear; cellSpot(c); add. Then a pet when (people 1:
rand % 4 == 0; people 8: rand % 8 == 0): hostility 4, level lvl, flags 0x2000; people 1 → race
0xd1, roam 0x90, +0x40 = the traveller's id; people 8 → race 0x107, roam 0x10; looks; gear;
cellSpot(c); add.

### 3.4 Boss camp (0x126160, e 2–8)

Arguments: boss race B, race list Rs, people p.

1. srand(R(ph + 0x9a11)). pick = kind cells[rand % n]. open = camp cells, shuffled.
2. n = |open|; r5 = rand % 5 + 1; r8 = rand % 5 + 8; count = min(r8, max(1, max(n / 20,
   min(r5, n / 2)))).
3. Empty the tracked list (zone+0x480).
4. If inZone(pick), the **host**: hostility 3, level lvl; race = the ruler of the place
   (0x2ba810, §3.12) when it has one (class 0x8a for kinds 1–3), else as a camp traveller's race
   and class and people (traveller 1 when p is 4 or 5, else kind-8 people); flags 0x3000; roam
   0x2030; position = cellSpot(pick); looks; gear; carrying the treasure; add.
5. guards = B is 0xb2, or B is outside 0xaf..0xb4.
6. For i = 0..n−1: if i ≥ count and rand % 12 ≠ 0, skip. c = open[i]; not inZone → skip.
   srand(R(H(c) + 0x4f2f90)). If i < count, a boss: hostility 7, level lvl, race B, flags
   0x2000, home P; position = cellSpot(c); yaw = quarter yaw; looks; gear; add; its id goes to
   the tracked list. Then, if guards, a camp guard (Rs, p, class 0, flags 0) at c, added.

### 3.5 Group camp (0x1241b0, e 17–23)

Arguments: leader race Lr, leader flags Lf, race list Rs, people p.

1. srand(R(ph + 0x9a11)). pick = kind cells[rand % n]. open = camp cells, shuffled, then
   resized to max(1, max(|open| / 8, 8)) (growing pads with cell (0, 0)).
2. Empty the tracked list.
3. If inZone(pick), the **leader**: hostility 1, level lvl, race Lr, people p, flags Lf | 0x3000,
   roam 0x30, requiredDefeated = max(1, min(20, |open| / 2)) with |open| the size **after** the
   resize of step 1 (so at least 4); position = cellSpot(pick); if it
   has a treasure: carrying it and flags |= 0x20000; looks; gear; add. Without a treasure its id
   goes to the mission list.
4. For each cell c of open in inZone: srand(R(H(c) + 0x4f2f90)); a camp guard (Rs, p, class 0,
   flags 0) at c, added, its id tracked; then if rand % 10 == 0, a camp traveller at c.

### 3.9 Lonely creature (0x125720, e 1)

1. srand(R(ph + 0x9a11)).
2. Race list (0x1292f0): kinds 0x17 and 0x24 → (0x6b); else (0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71,
   0x72, 0x73, 0x73, 0x74, 0x75, 0x76, 0x77); race = list[R(H(P) + 0x2b3a706) mod size].
3. cells = the centre cell alone for kind 0x18, else kind cells. pick = cells[R(ph + 0x4192) mod
   n]; not inZone → stop.
4. hostility 1, level lvl, race = variant(race), roam 0x30, flags 0x42000, home P; carrying the
   treasure; looks; gear; position = cellSpot(pick); add. Without a treasure its id goes to the
   mission list.

### 3.10 Boss with guards (0x121770 for a boss, 0x120bc0 for a leader)

Arguments: boss race B, guard race Gr, people p, leader (true for e 30 and 32).

1. srand(R(ph + 0x9a11)); one people-or-beasts(−1) pick, dropped.
2. pick = kind cells[R(ph + 0x4192) mod n]; not inZone → stop.
3. The boss: hostility 1 if leader else 7; level lvl; race = variant(B); as leader: people p,
   roam 0x10; flags 0x42000 when B is 0xaf and not leader, else 0x2000; home P; position =
   nearest(pick, kind 0) (main file §5.5) when B is 0xaf and not leader, else cellSpot(pick); if
   it has a treasure: carrying it, and as leader flags |= 0x20200; looks; gear; add; its id goes
   to the mission list.
4. n = rand % 4 + 3 guards: hostility 1, level lvl, race = variant(Gr), people p, roam 0x4000,
   flags 0x2000, home P; position = cellSpot(pick); +0x40 = the boss's id; looks; gear; add.

### 3.11 Arena teams (0x11cd80, e 27)

1. srand(R(ph + 0x9a11)). middle = the centre cell; not inZone → stop.
2. previous = empty. For team t = 0..3:
   * size = R(0x452·t + 0x2fdd8 + H(P)) mod 3 + 1; r = a draw (always made).
   * Kind = 0 for t = 3, else r mod 10. flags 0x200, people 0, style 0; by kind:
     1 → races (people-or-beasts(−1)); 2 → (69, 70, 71, 72, 0x11f, 0x122, 0x123); 3 → (102, 104,
     105, 103); 4 → (0x28); 5 → (97, 230, 15, 165, 17, 269, 96, 41, 0xe6, 0xad, 0xa6), people 5;
     6 → (226, 85, 73, 229, 0xe1, 0xe3), people 4; 7 → (0x104), people 3; 8 → style = the zone's
     style σ, races (styleBoss(σ, a draw)), people styleRace(σ), flags 0x100200 if styleEven(σ)
     is false; 9 → (0xec, 0xed, 0x106), people 3; 0 → (108, 109, 114, 115, 119, 117, 118, 111,
     0x71, 0x70, 0x74), flags 0x40000 **instead of** 0x200 (the team's flags are 0x40000
     alone), size 1.
   * keep = size; if size > 1 and rand % 2 ≠ 0: size ×= 2, keep = 1.
   * For i = 0..size−1: when i ≥ keep, clear 0x200 from the flags (for this and later members).
     hostility 1; level t + 1; race = variant(races[rand % size]); roam 0x10; flags | 0x2000;
     home P; prerequisites = previous; people; style; requiredStage 2t + 1; looks; gear;
     position = cellSpot(middle); add; for t = 3, i = 0: carrying the treasure, and its id goes to
     the mission list.
   * previous = this team's ids.

### 3.12 The ruler of a place (0x2ba810)

v = R(H(x, y) + 0x8e736) (the place cell): kinds 1–3 → (0, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12,
13, 14, 15, 16)[v & 0xf]; kind 7 → 0xa7 if v is odd, else 0x2b; kind 22 → 0x2d; others none.

---

## 4. Set pieces of roles 19–23

### 4.1 Role 21: tower people

1. For each zone cell (x outer) whose object is 0x34: floors = 4 (0x2d9da0); remember the cell
   (tx, ty).
   * The ruler: hostility 1; race = the place's ruler (§3.12; class 0x8a for kinds 1–3); yaw =
     quarter yaw; level lvl; flags |= 0x2200; home P; looks; gear; position = take(cell,
     floors + 6); add.
   * Its pet: hostility 8, race 0x121; quarter yaw; level lvl; flags |= 0x2000; +0x40 = the
     ruler's id; home P; looks; gear; take(cell, floors + 6); add; its id goes to the mission
     list.
2. No such cell: stop. srand(R(H(P.x + 0x2317, P.y + 0x2ab6))) (0x2ff5c0).
3. A dweller kind per floor i = 0..floors−1 by rand % 8: 1 beasts; 2 undead; 3 golems; 4 race
   rand % 4 + 0x25; 5 tower folk; 6 lizards; 7 people-or-beasts(−1) with flags 0x100000;
   0 people-or-beasts(−1).
4. low = min(lvl, 1). For cx = tx−1..tx+1, cy = ty−1..ty+1, floor i = 0..floors−1, except the
   tower cell's top floor: if rand % 2 == 0 or count(c, i + 7) = 0, skip; else hostility 1, the
   floor's race, quarter yaw, level (lvl − low)·i / (floors − 1) + low (integer), flags | 0x2000,
   roam 0x30; looks; gear; take(c, i + 7); add.

### 4.2 Role 19: the witch's hut

1. If kind is 0x16: cells = every cell of (mx − 4 .. mx + 3) × (my − 4 .. my + 3) around the
   centre cell whose object is 0x58. If any: pick = cells[rand % n]; if inZone(pick): object 0x58
   at nearest(pick, 2), turn R(H(P) + 0x326a0) & 3, settled (not on the ground), put.
2. The leader (0x11e7f0, race 0x2d, people 0, class 0, flags 0): srand(R(ph + 0x9a11)); pick =
   kind cells[R(ph + 0x4192) mod n]; not inZone → stop; hostility 1, people, class, level lvl,
   race = variant(0x2d), roam 0x30, flags 0x2200, home P; carrying the treasure; looks; gear;
   cellSpot(pick); area += pick; add; without a treasure its id goes to the mission list.

### 4.3 Role 22: brazier keepers

For each zone cell (x outer) whose object is 0x38: item = (category 0x16, id 0x49, level lvl).
A keeper: hostility 1; race = people-or-beasts(−1); class 1; people 0xb; yaw = quarter yaw;
level lvl; area += the cell; flags |= 0x102200; roam 0x30; carrying the item; looks; gear;
cellSpot(cell); add. Then an object 0x7e holding the item (+0x38): turn rand % 4; position =
cellSpot(cell); settled on the ground; put.

### 4.4 Role 23: spider nests

1. Over the place's cells (as in §1.1, x outer): object 0x44 → cisterns; 0x42 → towers; any
   other with place weight > 0.5 → open. open shuffled, then resized to max(1, max(|open| / 8,
   8)).
2. A **spider** at c with level l and flags f: hostility 1, level l, race = variant(0xec),
   people 6, flags f, home P; position = cellSpot(c); roam 0x10; looks; gear.
3. A **pet** of race q at c: hostility 8, level lvl, race q, flags 0x42000, home P; position =
   nearest(c, 0); quarter yaw; +0x40 = the id of the point added before it; looks; gear; add;
   0xef's id goes to the mission list, 0xf0's to the tracked list.
4. A **lower spider** is a spider whose level is lower = max(lvl − rand % 2, 0), drawn for each
   lower spider as its first draw (0x11bc79 for cisterns, 0x11bf7d for open cells), before its
   variant. There is no draw for the place as a whole.
5. Each cistern in the zone: a lower spider (flags 0x2200), added; then a pet 0xf0.
6. Each open cell in the zone: a lower spider (flags 0x2000), added.
7. Each tower in the zone: a spider (lvl, 0x2200), added, with requiredDefeated = |cisterns|;
   then a pet 0xef; then if rand % 2 ≠ 0: hostility 1, level lvl, race 0x106, people 6, flags
   0x42000, home P, position cellSpot(tower), roam 0x10, requiredDefeated |cisterns|; looks;
   gear; add; and the spider's +0x40 = this one's id.

## Errata (from the clean-room implementation, checked against the oracles)

- **Fixed (§3).** The giver's people pick was written as made before every encounter. 0x1274d0
  makes it only inside the quest camp cases (e 9–16). Evidence: gen_seed_123 zone (85, −149),
  P (4, −10), role 5, e 22: after srand(R(ph + 0x21559a6e)) the .rtrace's first draws are
  11002 and 16644 at 0x14144b and 0x142148, the first lizard pick (0x141390) and its variant,
  with no people pick before them.
- **Fixed (§4.4).** "lower" is drawn once per lower spider, as its first draw (before its
  variant), not once per place. Evidence: gen2_seed_123 zone (−261, −55): after the shuffle,
  each open-cell spider starts with a draw at 0x11bf7d, then its variant (0x142148), its spot
  and its looks and gear.
- **Settled (§3.1 step 8).** The big camp's guards that are made but not added have flags
  0x2000 (0x11f280: set before the looks), and each takes its spot after the gear.
- **Settled (§3.5 step 3).** requiredDefeated uses the size of open after the resize.
- **Settled (§3.11).** Team kind 0's flags are 0x40000 in place of 0x200, not added to it.
