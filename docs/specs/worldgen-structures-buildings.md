# World generator, part B: buildings, castles and set pieces

Part of `worldgen-structures.md` (conventions §0; the zone's place, style s, big structure B,
anchor and planner §5; the cell loop §7). The mason, its schemes and shapes are in
`worldgen-structures-mason.md`; per-type tables (T1–T15) in `worldgen-structures-tables.md`.
"type(x, y)" is the cell object 0x69c60 of cell (x, y); "building" is a type whose floor height
(T3) is above 0; "c" the building culture (T9) of the zone's place.

---

## 1. Helpers of buildings

**Base height** 0x2d87f0 (cx, cy, t); t = 0 means type(cx, cy):
* t in 45–47 or 52: the place cell (⌊16 cx/1024⌋, ⌊16 cy/1024⌋), its landmark point p (big from
  0x36160): trunc(ground height (p, mode 0) + 1.0).
* t in 40–43: the highest floor(ground height (mode 0)) of the 3 × 3 points (32⌊cx/2⌋ + 16 i,
  32⌊cy/2⌋ + 16 j), i, j = 0..2 (starting from 0).
* m = (16 cx + 8, 16 cy + 8). t = 60: trunc(max(ground height (m), 0) + 80); t = 13:
  … + 44; t = 14: … + 80. t ≤ 12: trunc(ground height (m)).
* The place at m (0x35ce0). t = 94 or 95: b = the base height 0x2a0840 (neighbourhood at the
  place's centre, kind, weight 1.0); 95 → trunc(b + 100 − 24), 94 → trunc(b + 1).
* t = 86: the ground height at (16 cx + 16 when type(cx + 1, cy) = 86 else 16 cx, likewise y),
  trunc(+ 2).
* Otherwise the column at m (part A): when its river value > 0.5: f = floor(the river's water
  level 0x2e6420 (its base, the place) + 1); v = f / 2 × 2 + 1; max(v, 0). Else h = ground height
  (m); m′ = the largest of the heights at (16 cx + 16, 16 cy + 16), (16 cx, 16 cy + 16),
  (16 cx + 16, 16 cy), (16 cx, 16 cy) (taken in that order with "a > b ? a : b"); h = max(h, m′)
  (when h < m′); floor(max(h + 1, 0) × 0.5 × 2 + 2 + 0).

**Floor offset** 0x2d82c0 (cx, cy): t = type; t 45 or 46: the fortress cell code (§10) of the
cell; 0, 1 or 4 → 0; otherwise trunc((S(⌊cx/4⌋ × 1.5, ⌊cy/4⌋ × 1.5) + 1) × 5). t 72–75 → −10,
−20, −30, −40 in order; 77–80 and 82–85 likewise; others 0.

**Floor exists** 0x2b8b10 (cx, cy, f): t = type; false when f ≥ most floors (T5); true unless t
is 45; for 45 true when the fortress cell code ≠ 0.

**Stairs** 0x2d8790 (cx, cy, f): t 47 → 2 when f > 0, else 0; t 39, 86, 94, 95 → 2 at f = 0, 1
when f > 0, else 0; others 1 when f > 0, else 0.

**Same building** 0x2b8a60 (a, b): type(a) = type(b) and base(a) + height(type(a)) = base(b) +
height(type(b)) (T4). **Joins** 0x2d86e0: bit 1 west (cx − 1), 2 east, 4 south (cy − 1), 8 north,
set when the neighbour is the same building.

**Ridge sides** 0x29d170 (cx, cy, t, joins): unchanged unless t = 25 or roof kind 1, 3, 4, 7 or 8.
w, e, s, n = the four join bits. When w ≠ e: when s ≠ n and R(cx, cy, 0x44b2d3) & 1, s = n = true;
otherwise w = e = true and, when s ≠ n, s = n = true. Else when s ≠ n: s = n = true. When none is
set: R(cx, cy, 0x2f887fb) & 1 → w = e = true, else s = n = true. The result is joins with these
bits added.

**Roof height** 0x2d34b0 (local x, y, t, ridge, sub) over the walls' top:
1. t = 95 → 2. Roof kind 1 with sub = 5: 2 ≤ x, y < 14 → sub − 3, else 3.
2. dx = x − 8, dy = y − 8; ay = trunc(|float dy + 0.5|), ax likewise; c = max(ax, ay).
3. t = 38 or sub = 6: c as is. t 40, 43, 47: c = −8, then c = max(c, dx) for ridge bit 1,
   max(c, 7 − x) for bit 2, max(c, dy) for bit 4, max(c, 7 − y) for bit 8. Others: when (bit 1
   and x < 8) or (bit 2 and x ≥ 8), c = min(c, ay); when (bit 4 and y < 8) or (bit 8 and y ≥ 8),
   c = min(c, ax).
4. h = 8 − c; sub = 4 → h × 2. Then × 2 for t 38, 43, 47, 81, 91, 96, 100 or roof kind 8; else
   / 2 (C) for t 25, 76, roof kind 3 or sub = 6.

## 2. The building builder: set-up (0x3063c3)

Called for a cell (i, j relative to the zone, cx, cy, type t, castle cell K; §9) when the castle
kind ≠ 0 or t is a building. Values:

* base = base height (cx, cy, t); height = building height (T4), + 2 unless "keeps its height"
  (T7) for (t, s).
* M = a mason at the cell. When i ≥ 0 and the water line test (T7) holds, its water line = 10.
* colA = wall colour (T11), colB = trim colour (T12). roof = roof kind (T2); sub = c when roof
  = 1, else −1.
* overhang: roof 1 → by sub: 1 → 2, 0 or 2 → 1, 3 or 4 → 2, 5 → 0, 6 → 4, others 1; roof 3, 6,
  10 → 4; roof 4, 9 → 2; others 1.
* deep roof: roof 1 → sub = 4; else roof 3, 4, 6 or 10.
* Scheme (two colours, relative): when i ≥ 0 and t = 25: (colB, 0, 0; colA + (50, 50, 50), 0,
  10). Else when i ≥ 0 and (roof 7 or sub 2 or 3): (colB, 6, 0; colA, 6, 10). Else when i ≥ 0,
  sub 6 and t ≠ 26: (colB, 7, 0; colA, 7, 10). Else when i ≥ 0 and roof 8: ((colB.r × 0.8,
  colB.g × 1.0, colB.b × 0.8), 1, 0; colA, 1, 10). Else when i ≥ 0 and roof 5: (colB, 1, 0;
  colA, 1, 25). Otherwise (colB, 1, 0; colA, 1, 10). S1 = this scheme.

**Foundations** (when floor height > 0):
* t 63–65: one colour colA pattern 0; z = base − 2 (+ 3 for 63 and 64); e = type(cx + 1, cy) in
  63–65, n likewise north; foundation (0, 0, z, 2, 2, 1, 0, 0); unless e: (14, 0, …); unless n:
  (0, 14, …); unless e or n: (14, 14, …). Then one colour colA with pattern rand mod 2 + 4; the
  mason's type 23; box (0, 0, base − 1, 16, 16, 1). (The type stays 23 for what follows.)
* roof 3, 6, 10 or t 16, 26, 50, 51, 55, 57, 58, 59, 61: foundation (0, 0, base − 1, 16, 16, 1,
  0, 0).
* otherwise: top = floor offset + base; for each of the four neighbours (W, E, S, N) that is a
  building: top = min(top, base (neighbour) + floor offset (neighbour)); foundation (0, 0, top,
  16, 16, height − top + base, 0, 0).

S3 = the scheme with every colour × 0.75. **Sign draw**: when roof = 4, one rand (kept for the
door, §6); otherwise none.

## 3. Frame and battlements

**Timber frame** when (sub 0 or 2, or roof 7 or 8) and t keeps its height: save the scheme;
sub 0 → one colour, the timber colour (T13), pattern 0; sub 2 → two colours (0.5 × (255,
255, 255) + 0.5 × colB, 0, 0; 0.5 × white + 0.5 × colA, 0, 0; absolute); roof 8 → (60, 50, 30),
pattern 0; else colA × 0.85, pattern 0. S3 becomes this scheme. When floor height F > 0: for k
from 0 to height / F, box (0, 0, base − 1 + k F, 16, 16, 1). Corner posts: jW, jS, jE, jN =
"attached" (T7) of the four neighbours' types; bW = base (cx − 1, cy, 0), bS = base (cx, cy − 1,
0). z = base, min(bW, base) when jW, then min(bS, base) when jS; box (0, 0, z − 1, 1, 1, height −
z + base). Unless jE: z = min(bS, base) when jS else base; box (15, 0, z − 1, 1, 1, height − z +
base). Unless jN: z = min(bW, base) when jW else base; box (0, 15, …). Unless jE or jN: box (15,
15, base − 1, 1, 1, height). The saved scheme comes back.

**Battlements** (0x3073fb): o = overhang, len = 16 + 2o, z = base + height − o. For each side,
unless "joins a tower" (t, the neighbour's type) and only when "has battlements" (T7): west: box
(−o, −o, z, 2, len, 3) then battlements along y (−o, −o, z + 3, len, 2, 2); east: box (o + 14, −o,
…), battlements along y at x = o + 14; south: box (−o, −o, z, len, 2, 3), battlements along x
(−o, −o, z + 3, len, 2, 2); north: at y = o + 14.

When the castle kind is 0 and t is not a building, go to §8 (roof) directly.

## 4. Floors

floors = height (T4) / F when F > 0, else 0. The eight neighbours in the game's order W, E, S,
N, SW, SE, NW, NE: offsets (−1, 0), (1, 0), (0, −1), (0, 1), (−1, −1), (1, −1), (−1, 1), (1, 1);
their types nT[d] and bases nB[d] = base (neighbour, 0).

**Door side.** has a door (T7). When t has a door: sides = the bits 1, 2, 4, 8 of W, E, S, N
whose neighbour is not a building, in that order; and a kind-4 spot is added (contents file §C1)
for cell (cx, cy): (dots(16 cx + 8), dots(16 cy + 8), dots(base + floor offset)) (0x307a87).
When sides is not empty, doorSide = doorPick = sides[rand mod count]; else doorPick = −1 and
doorSide = (1, 2, 4, 8)[rand mod 4].

castleZ = castle floor (§10) when K's kind ≠ 0, else 0; castleTop = castleZ + rise (T15);
castleBottom = castle block top (§10) when K's kind ≠ 0.

For f from −1 (castle level, only when K's kind ≠ 0) or 0 to floors − 1; skip f ≥ 0 when the
floor does not exist:

1. level = f < 0. stairs = stairs (cx, cy, f). z0 = f F + base + floor offset; depth0 = 0.
   Level: z0 = castleZ, depth0 = room depth (T15) of K's kind, floorColour = first castle colour,
   floorColour2 = second. Otherwise both are the wall colour (T11).
2. Draws: a = rand mod 150 + 50, b = …, c = …; tint = (c, b, a); then a, b, c = rand mod 100 +
   150 each; border = (c, b, a).
3. carpet: when not level, floors = 1, roof 1, 8 or 9 and t ≠ 26: carpet = rand mod 4 = 0 (a
   draw only then). carpetW = rand mod 5 + 8, carpetD = rand mod 5 + 8 (always drawn).
4. floorPattern = 0 when level and B = 42, else 1.
5. Neighbours. Not level: has[d] = floor exists (neighbour, f); nz[d] = nB[d] + floor offset
   (neighbour) + F(nT[d]) × f. When t = 25 or roof 9, for W, E, S, N: has = true and nz = base
   (neighbour, t) + floor offset + F(nT[d]) × f. top0 = base + (f + 1) F; nTop[d] = F(nT[d]) ×
   (f + 1) + nz[d], nBottom[d] = nz[d]; bottom0 = z0. Level: nc[d] = castle cell of the
   neighbour; has = its kind ≠ 0; nz = castle floor; nTop = nz + its rise; nDepth = room depth
   of its kind; nBottom = castle block top − nDepth. top0 = castleTop; bottom0 = castleBottom −
   depth0.
6. floorMin = bottom0, maxTop = top0, lowered and raised by every neighbour that has a floor.
   inner box: x0 = 0 when has W else 1, x1 = 16 when has E else 15, y0, y1 likewise with S, N.
7. doorWidth = 16, wall = 2, dome = false. Not level: roof 1, 2 or 8 → doorWidth 10; roof 7 →
   8 unless t is 78 or 79; t = 25 → doorWidth 14, wall 1; else t 13, 16, 26, 49, 50, 51, 53, 57,
   60, 61, 63, 64, 65, 86, 94, 95 or roof 9 → doorWidth 16, wall 0. Level with K's style byte 1
   → wall 1, dome.
   special = t is 39, 86, 94 or 95. "In the door" (v): (16 − doorWidth) / 2 ≤ v < (doorWidth +
   16) / 2.
8. The 3 × 3 cells around, k = (x − cx + 1) × 3 + (y − cy + 1): their types, castle cells (the
   cell itself: K), floors (z0) and tops (top0); neighbour d maps to k as (W 1, E 7, S 3, N 5,
   SW 0, SE 6, NW 2, NE 8).
9. Floor border: when level and K's detail = 7: ring[d] = the neighbours' details; when not
   level, no carpet, t = 45 and the fortress floor code (§10) of (cx, cy, f) = 7: ring[d] = the
   neighbours' codes.

**Per column** lx from 0 to 15, ly from 0 to 15:

a. pillar (a corner open to the outside): lx < wall and ly < wall and not has SW; or lx ≥ 16 −
   wall and ((ly < wall and not has SE) or (ly ≥ 16 − wall and not has NE)); or lx < wall,
   ly ≥ 16 − wall and not has NW.
b. floorZ = z0; ceiling = castleTop − 1 when level, else (f + 1) F − 1 + base.
c. For each side in order W (along ly, depth lx), E (along ly, depth 15 − lx, far), S (along lx,
   depth ly), N (along lx, depth 15 − ly, far): when it has no floor: depth < wall → pillar.
   Otherwise: door = in the door (along); dz = z0 − nz[d], k = 2 when dz ≤ 5 else 1; when door
   and depth + (1 when far) ≤ dz × k and depth0 = 0 and nDepth[d] = 0 and not special:
   floorZ = min(floorZ, z0 − (dz × k − depth) / k). When depth < wall: unless door, when z0 >
   nz[d] or t = 25, **skip the column**; when F(nT[d]) ≠ 0, ceiling = min(ceiling, F(nT[d]) ×
   (f + 1) + nB[d] − 1).
d. **Walls.** When not level and (t does not keep its height, or (t is roofed and the column is
   outside the inner box)): one colour, the wall colour, pattern floorPattern. Otherwise:
   * Level: blend over the castle cells within 16 blocks: wx = 16 cx + lx, wy likewise; for the
     16-block cells gx from ⌊(wx − 16)/16⌋ to ⌊(wx + 16)/16⌋, gy likewise: dx = min(wx + 16,
     16 gx + 16) − max(wx − 16, 16 gx), dy likewise; skip unless both > 0; skip cells (among
     the 3 × 3) whose castle kind is 0; wgt = (double(dy × dx) × 0.00390625)² (in double); its
     light colour (T15) adds with wgt to L and wgt to lw, or, without a light, the first castle
     colour adds with wgt × 0.01; cw += wgt; zs += z × wgt, ts += top × wgt; types 72–74 add
     their wall colour (T11 at (gx, gy)) to both A and B; others add the second castle colour
     to B and the first to A (all in double). When cw > 0.0001: zz = zs / cw, tt = ts / cw,
     c1 = A / cw, c2 = B / cw (to float); else c1, c2 = the cell's own castle colours. c0 = L /
     lw when lw > 0, else the cell's light colour, else its first castle colour.
   * Not level: c0 = c1 = c2 = the wall colour; zz = z0, tt = top0.
   * Slots: c0 at zz − 10, c1 at zz, c2 at tt, all with floorPattern, absolute. Level: box (lx,
     ly, floorMin, 1, 1, maxTop − floorMin + 1). Not level: write mode 2, box (lx, ly, floorMin,
     1, 1, top0 − floorMin + 1), mode 0.
e. When pillar, next column.
f. **Stairs** (stairs = 1, f > 0): h = (16 − F) / 2. Door side 2 and lx ≤ 4, or door side 1 and
   lx ≥ 11: when h ≤ ly < 16 − h: floorZ = F (f − 1) − h + floor offset + ly + base; put the
   block of type 6, flags 2 at floorZ − 2. Door side 8 and ly ≤ 4, or 4 and ly ≥ 11: the same
   with lx.
g. light = trunc(max((S(wx × 0.05, wy × 0.05) × 1.5 + 0.5) × 127.5, 0)) (the max as "v > 0 ? v :
   0").
h. Colour: attached house (T7): not level: roof ≠ 8 → one colour, timber, pattern posmod(cx +
   cy, 2) + 4; roof 8 → floorColour, pattern 1. Not attached and not level: two colours (colB,
   1, 0; floorColour, 1, 10; relative).
i. floorFlags = 0x22 when level, else 2. fz = floorZ, or castleBottom − depth0 when depth0 ≠ 0.
   floorType = f + 23 when f > 0, else 6. "Lay (z, colour, type, flags)" packs with glow =
   light.
   * fz < floorZ: lay (fz − 1, the colour at (lx, ly, fz − 1), 6, floorFlags | 8).
   * Else unless (not level and t is 75, 80 or 85): lay (fz − 1, colour, floorType, floorFlags);
     with a carpet: x0 = (16 − carpetW) / 2, x1 = (carpetW + 16) / 2, y0, y1 likewise with
     carpetD; inside: edge = ((lx = x0 + 1 or x1 − 2) and y0 + 1 ≤ ly < y1 − 1) or ((ly = y0 + 1
     or y1 − 2) and x0 + 1 ≤ lx < x1 − 1); lay border or tint there. Else with a floor border:
     r = border (lx, ly, ring) (§4.1); r = 2 → (200, 200, 100), r = 1 → (200, 50, 100).
j. Dome: tt −= double((S(wx × 0.1 + 808348, wy × 0.1 + 8553) + 1) × 3); e = 1 and lowered (min) by
   lx × 0.125 when W is open and lx < 8, (15 − lx) × 0.125 when E open and lx ≥ 8, ly × 0.125
   (S), (15 − ly) × 0.125 (N), and for the corners SW (lx, ly < 8) max(ly × 0.125, lx × 0.125),
   SE (lx ≥ 8, ly < 8) max(ly k, (15 − lx) k), NE max((15 − ly) k, (15 − lx) k), NW max((15 −
   ly) k, lx k) — in that order; t = min(1 − e, 1); t = t × (t × t); ceiling = floor(double(1 −
   t) × (tt − 1) + double t × zz).
k. Air: for z from fz to ceiling − 2: put (0, 0, 0, light, 0, floorFlags).
l. Grass: grassy = (level: K's style 2 or B 43 or 44; else f = 0 and "grassy floor" (T7)). When
   grassy, fz = floorZ and S(wx × 0.1 + 89834, wy × 0.1 + 12233) > 0.25: the grass colour at
   (wx, wy), glow light, type 4, floorFlags, at fz.
m. When level or t keeps its height: lay (ceiling − 1, colour, 6, 2) (glow light).

After the columns:

* **Liquids** (level, 0x30b255), g = castleBottom: kind 10 → water liquid (0, 0, bottom0, 16, 16,
  g − bottom0 − 2, flags 0x22); kind 11 → foundation (0, 0, g − 5, 16, 16, 1, 0, 0), lava (0, 0,
  g − 4, 16, 16, 1, 0x22), then every block of the cell from g − 3 to g − 1 is read and put back
  with glow 255; kind 12 → the foundation and poison (0, 0, g − 4, 16, 16, 1, 0x22).
* **Spiral stairs** (stairs = 2, 0x30b429): bottom = (f − 1) F + base, rise = F; sx = 0 when
  stairs (cx − 1, cy, f) = 2 else 8, sy likewise with (cx, cy − 1); + 8 when the east (north)
  neighbour's stairs are 2. r = 1, w = 5; unless sx = sy = 8, r = 5, w = 7. When special and
  f = 0: slots (first castle colour, 1, castleZ; second, 1, castleTop; floorColour, 1, z0 − 1),
  absolute; box (0, 0, castleTop − 1, 16, 16, z0 − castleTop); bottom = castleZ, rise = z0 −
  castleZ. Otherwise one colour floorColour, pattern 1. steps = rise − 1, turn = 2r + 1; z =
  bottom + 1; for q′ from steps down to 0 (z += 1 each): q = q′ / turn, pos = q′ mod turn; at
  q′ = steps, q = (q′ − 1) / turn and pos = (q′ − 1) mod turn + 1. By q mod 4: 0 → (pos − r −
  w + sx, sy − r − w); 1 → (sx + r, pos − r − w + sy); 2 → (sx − pos + r, sy + r); 3 → (sx −
  r − w, sy − pos + r). air (x, y, z, w, w, 10, 0); box (x, y, z − 2, w, w, 2). Then box (sx −
  r, sy − r, bottom, 2r, 2r, rise).
* **The beam and its object** (0x30b7ae): code = K's detail when level; the fortress floor code
  when t = 45; else 0. Object kind: code 2 → 8, 3 → 9, 4 → 10, 5 → the planner's item for the
  cell (0x12a600); otherwise none (also none when ≤ 0). down = 0; while down < 30 and the block
  at the world (16 cx + 8, 16 cy + 8, z0 − 1 − down) is open, down += 1. Two colours
  (floorColour, 1, z0; floorColour2, 1, top0; absolute). run = 16 − 2 wall, zb = z0 − down +
  run, hb = top0 − z0 + down − run. along x = has S and has N: box (wall, 7, zb, run, 2, hb);
  else box (7, wall, zb, 2, run, hb). The object: that kind at dots (16 cx + 8, 16 cy + 8, z0 −
  down), turn 0 along x else 1, scale float run, +0x2c 1000.0; kept if inside.

### 4.1 The floor border (castle detail 7, fortress code 7)

Inputs: local (x, y) and the eight neighbours' codes W, E, S, N, SW, SE, NW, NE. "Seven" means
the code 7. The result is 0 (nothing), 1 or 2.

**Drawn** when any holds: 3 ≤ x < 13 and 3 ≤ y < 13; x < 3, W seven and 3 ≤ y ≤ 12; x ≥ 13,
E seven and 3 ≤ y < 13; y < 3, S seven and 3 ≤ x ≤ 12; y ≥ 13, N seven and 3 ≤ x < 13; x < 3,
y < 3 and SW, W, S all seven; x ≥ 13, y < 3 and SE, E, S seven; x < 3, y ≥ 13 and NW, W, N
seven; x ≥ 13, y ≥ 13 and NE, E, N seven. Not drawn → 0.

**Bright** (2 instead of 1), tested as this sequence of steps, each either answering or going
to a named step:
1. When W is seven and (y ≥ 3 or SW = W): when not (y < 13 or NW is seven) go to 2, else go to 3.
2. When x = 4 and (y ≥ x or S seven) and (y < 12 or N seven): bright.
3. When E is seven and (y ≥ 3 or SE = E): when not (y < 13 or NE seven) go to 4, else go to 5.
4. When x = 11: when y ≥ 4 or S seven: bright when y < 12 or N seven (else continue with 5);
   otherwise go to 6.
5. When S is seven and (x ≥ 3 or SW = S) and (x < 13 or SE seven): go to 7.
6. When y = 4 and (x ≥ y or W seven) and (x < 12 or E seven): bright.
7. When N is seven and (x ≥ 3 or NW = N) and (x < 13 or NE seven): go to 9.
8. When y = 11 and (x ≥ 4 or W seven) and (x < 12 or E seven): bright.
9. Unless SW is seven: when x = 4: y > x → go to 10; S seven → bright. When y = 4, x ≤ y and W
   seven → bright.
10. Unless SE is seven: when x = 11: y > 4 → go to 11; S seven → bright. When y = 4, x ≥ 11 and
   E seven → bright.
11. Unless NW is seven: when x = 4: y < 11 → go to 12; N seven → bright. When y = 11, x ≤ 4 and W
   seven → bright.
12. Unless NE is seven: when x = 11: y < x → not bright; N seven → bright. When y = 11, x ≥ y
   and E seven → bright.
13. Not bright.
Steps that neither answer nor jump fall through to the next.

## 5. Walls, doors and windows (0x30bd50; not on the castle level)

Scheme S3. openings = t is not 25 or 26 and roof is 1, 2, 4, 5 or 8. Furniture list (roof 1
only): 38, 39, 37, 40, 74, 28, 30 (7 entries); none for other roofs. Wall turns (W, E, S, N) =
3, 1, 0, 2; wall angles 270, 90, 0, 180.

**Outdoor spot** (wall d, along, modelAlong, turn override −1):
1. Unless rand & 1, nothing. xWall = d < 2 (the wall runs along y). mid = dots(16 (cy when xWall
   else cx) + 8); edge = dots(16 (cx or cy) + 16 when d is odd, else + 0); out = +1 for odd d,
   else −1. "Ground (x, y)" = dots(trunc-toward-zero floor of the ground height (x toward zero,
   y toward zero, mode 0) + 1).
2. When rand & 1: when there is no furniture list, nothing. Object = list[rand mod 7]; 74 →
   74 + rand mod 9. across = edge + out × dots(0.5) + out × dots(size y (T8) × 0.5); a = mid ±
   dots(|along|); position (across, a) for an x wall, (a, across) otherwise; z = ground; turn =
   the override, else the wall turn; settled and kept (contents file §C8).
3. Else a model 0x36 + rand mod 3 at across = edge + out × dots(0.5), a = mid ± dots(|modelAlong|),
   z = ground, size rand fraction × 0.05 + 0.15, yaw the wall angle; kept if inside.

**The door's object** (0x306d3a): 5 when sub = 4 or roof = 4, else 4. Roof 5: 8; then unless
hash(H(C1) + 0x10581) is odd (0x31a720): q = (hash(H(C1) + 0x29fa4) as a signed int, times
0x9e5706b3 signed, high 32 bits) >> 13, plus its sign bit; when q & 3 ≠ 0 and the planner's item
list is not empty, the list entry R(C1, 0x89d85) mod its size. Roof 4: when the sign draw mod 4
= 0 and the planner's role of C1 is neither 12 nor 10, the planner's item for (cx, cy). Roof 3,
6, 9, 10: none. Then t 76, 81, 100: none, and the planner's item for (cx, cy) when t = 100 or
hash(H(C1) + 0x134cb8) is even (0x31a770).

doorH = 10 for roof 5; 6 for roof 3, 4, 6, 9, 10 or t 76, 81, 100; else 4. windowH = roof 1 → 4
when sub = 4 else 2; roof 3–6 or 10 → 4; else 2. Flower boxes = roof 1 and sub not 4 or 6.

**For each wall** d = 0..3 (W, E, S, N), skipping a wall whose neighbour is a building, and all
walls unless openings or the type has a door:
1. front = f = 0, t has a door, and doorPick = (1 << d). w = rand mod 3 (windows).
2. front: when doorH > 4 or w = 0, w = 1; furnish. Else when w > 0: furnish. Else (no windows):
   with a timber frame: scheme S3, box at (0, 8), (15, 8), (8, 0), (8, 15) for W, E, S, N with
   size (1, 1, F) from z0; on the ground floor two outdoor spots (d, −3.51, −3.51), (d, 3.51,
   3.51).
3. When f = 0, furnish, openings, and not (front and doorH > 4): w = 1: outdoor spots (d, −3.51,
   −3.51, turn 3 when d = 1 — the game's copy for that wall turns it like wall 0's) and (d,
   3.51, 3.51); w = 2: one spot (d, 0.01 when d is odd else −0.01, 0.01).
4. For k < w: centre c = 16 / w / 2 + k × 16 / w.
   * **Door** (front and k = 0): h = doorH, at = c − h / 2. W: box (0, at − 1, z0, wall, h + 2,
     h + 1), air (0, at, z0, wall, h, h, 0), ramp along x (−16, at − 1, z0 + 1, 16, h + 2, a =
     −16, b = 0, cap 0, mode 0). E: box (16 − wall, at − 1, z0, …), air (16 − wall, at, …),
     ramp along x (16, at − 1, z0, 16, h + 2, 0, −16). S: box (at − 1, 0, z0, h + 2, wall, h + 1),
     air (at, 0, z0, h, wall, h), ramp along y (at − 1, −16, z0 + 1, h + 2, 16, −16, 0). N: box
     (at − 1, 16 − wall, …), air, ramp along y (at − 1, 16, z0, h + 2, 16, 0, −16).
     The door object when its type ≥ 0: in wall d at c, 0.5 inside (the opening position: across
     = edge + dots(0.5) for even d, edge − dots(0.5) for odd; along = dots(16 (cy or cx) + c)),
     z = dots(z0), turn the wall turn, scale = float doorH / size x (T8) of the door type, +0x2c
     1000.0; kept if inside.
     Outside (0x30d21a): out = dots(16 (cx or cy) + 16 when odd d, else + 0) ± 65536 (+ for odd);
     rowStart = dots(16 (cy or cx)). An attached house gets a lamp model 0x18 (0x19 when sub = 4)
     at along rowStart + dots(c), height z0 + doorH, yaw the wall angle, tint (1, 0.9, 0.4) ((0.5,
     1, 0.4) for 0x19), size 0.1, flags 1, kept if inside. When the town block (main file §6.4)
     of the cell is 2–9: a sign model (0x2e, 0x2f, 0x31, 0x30, 0x32, 0x33, 0x34, 0x35)[block −
     2], tint (1, 0.9, 0.4), flags 2, along rowStart + (odd d: dots(c + doorH / 2) + 98304, else
     dots(c − doorH / 2) − 98304), height z0 − 1 + doorH, yaw the wall angle; when the block
     there (blocks of the position) is solid, the across coordinate moves 65536 further out (+
     for odd d); size 0.06666667; kept if inside.
   * **Window** (otherwise, when openings): W: box (0, c − 2, z0, wall, 4, windowH + 2), air (0,
     c − 1, z0 + 1, wall, 2, windowH, 0); E at 16 − wall; S: box (c − 2, 0, z0, 4, wall, windowH
     + 2), air (c − 1, 0, z0 + 1, 2, wall, windowH); N at 16 − wall. The pane: object 7 when
     windowH = 4, else 6, in wall d at c, 0.5 inside, z0 + 1, +0x2c 1000.0, kept if inside. With
     flower boxes: object 0x3c + rand mod 3 at c, 0.5 outside, z0, kept if inside.

**Ceiling lamp** (0x3122c8), inside the zone, for attached houses and roof 9: lamp = rand mod 4
= 0 (always drawn) or t has a door or (town block 2 and f = 0); and t ≠ 88: model 0x17 (0x1a with
tint (0.5, 1, 0.4) for culture 4) at (dots(16 cx + 8) + 32768, dots(16 cy + 8) + 32768,
dots(base + (f + 1) F − 3)); up to four times: when the block above it is solid, write it (size
0.1, flags 1, yaw 0; kept... appended without the inside test) and stop; else z += 65536.

## 6. Room contents (0x3124e8)

Only when the cell is inside the zone, not (special and level), floor offset > −32, and (level:
K's detail not 2–5; else t is a building). code = K's detail (level), the fortress floor code (t
= 45), else 0. light = floorColour, or the castle light colour when level and the kind has one.

### 6.1 Furniture (0x675c0 sets up, 0x69860 places)

A furnisher with the cell's mason, colours c0 = floorColour, c1 = floorColour2, the light colour,
and four lists (wall pieces, centre pieces, side models, ceiling models):
* Level, K's style 2: wall = {2, 5} when detail = 0; side += 0 when K's kind = 1; centre = {4}.
* Level, other styles, by B: 40: wall {4, 3, 5} (detail 0), side {4, 3}; 41: wall {6, 7, 0, 5},
  side {4}; 42: side {4}; 43: wall {1}, side += 0 when kind 1, ceiling {0}; 44: ceiling {0},
  centre {0}; 45: wall {4, 1, 5}; 46: wall {4, 2, 5}, ceiling {0}; 47: wall {4, 0, 5}. Then side
  += 1 when kind 1, and centre += 4. (The wall lists only when detail = 0.)
* Not level: t 35–38: side {4, 1}, wall {2, 4}; t 45: ceiling {1}, side {4, 1}, wall {0}.
Then furnish (x0 = wall, y0 = wall, z = z0, sx = sy = 2 (8 − wall), h = top0 − z0 − 2), below.

"Taken" block: solid, or flag 0x40 without 0x80. "Pick (list)" = list[rand mod size].

**Furnish** 0x69860 (x0, y0, z, sx, sy, h), top = z + h, four times in order (wall S, N, W, E):
rand mod 2 ≠ 0 → side model, else wall spot:
1. side model (x0 + sx / 2, y0, z, side 0) or wall spot (x0 + sx / 2, y0, z, top, 0, not
   mirrored);
2. side model (x0 + sx / 2, y0 + sy − 1, z, 2) or wall spot (x0 + sx / 2 − 1, y0 + sy − 1, …, 2,
   mirrored);
3. side model (x0, y0 + sy / 2, z, 1) or wall spot (x0, y0 + sy / 2, …, 1, not mirrored);
4. side model (x0 + sx − 1, y0 + sy / 2, z, 3) or wall spot (x0 + sx − 1, y0 + sy / 2 − 1, …, 3,
   mirrored).
Then for x from x0 to x0 + sx − 1, y inner: x mod 5 = 3 and y mod 5 = 3 and rand mod 5 = 0 →
ceiling model (x, y, top); x mod 5 = 1 and y mod 5 = 1 and rand mod 5 = 0 → centre piece (x, y,
top, z). (The rand is drawn only when the position test passes.)

**Side model** 0x6a9a0 (x, y, z, side): nothing when the side list is empty or rand mod 2 ≠ 0.
A child mason C (x, y, 0, 1, 1, side, no mirror). kind = pick (side). kind 1 or 3–5: z += 2 +
rand mod 4; kind 2: z += 2. py = 0; up to 10 times while the block at C (0, py − 1, z) is open,
py −= 1; up to 10 times while C (0, py, z) is taken, py += 1. Fail when any of C (dx, py − 1, zz),
dx = −1..1, zz = z..z + 4 is open; fail unless C (0, py − 1, z) is solid with flag 2 and C (0, py,
z), C (0, py, z + 1) are open. Position (dots(C world x of (0, py, z)) + 32768, likewise y, dots(z)
+ 655). By kind: 0 → type 0x55 + rand mod 4, k = 0.2, s = rand fraction × 0.6 + 1; 1 → 0x43, k
0.0625, s 1.5; 2 → 0x3d, k 0.04, s 3; 3 → 0x3e, 0.0625, 1.5; 4 → 0x3f, 0.0625, 1.5; 5 → 0x44 +
rand mod 4, k 1, s = rand fraction × 0.1 + 0.1. yaw = float(facing (0) × 90); t′ = s − 1; x +=
trunc64(t′ × float(C.M4) × 0.5 × 65536), y += trunc64(t′ × float(C.M5) × 0.5 × 65536); a model
with size k × s, appended without the inside test.

**Wall spot** 0x6bef0 (x, y, z, top, side, mirror): nothing when the wall list is empty. C =
child (x, y, 0, 1, 1, side, mirror). Fail unless every C (dx, −1, zz), dx = −2..2, zz from z + 1
to top − 1, is solid with flag 2. Then from z up: t = taken (the cell mason's block at (x, y,
z)); when z ≥ top, fail; when not taken, stop; else z += 1. The piece pick (wall) is built in C's
frame at that z.

**Wall pieces** (frame C, the wall behind at y = −1); unless kind 7 the slots are set first:
light at z − 20, c0 at z, c1 at top, all pattern 1, absolute (0xbf840).
* 0 (0x6a600): foundation (−1, 0, z, 4, 2, 1, 0, 0); box (0, 0, z + 1, 2, 1, top − z − 2); box
  (−1, 0, top − 1, 4, 2, 1).
* 1 (0x6b5c0): foundation (−2, 0, z, 6, 2, 1); foundation (−1, 2, z, 4, 1, 1); box (−1, 0, z +
  1, 4, 1, top − z − 2); box (0, 1, z + 1, 2, 1, top − z − 2); box (−2, 0, top − 1, 6, 2, 1);
  box (−1, 2, top − 1, 4, 1, 1).
* 2 (0x6a370): foundation (−1, 0, z − 1, 4, 2, 1); box (−2, 0, z, 6, 3, 1); box (−1, 0, z + 1, 4,
  2, 1); box (0, 0, z + 2, 2, 1, top − z − 2); box (−1, 0, top − 5, 4, 2, 1); box (0, 1, top − 3,
  2, 1, 1); box (0, 1, top − 2, 2, 2, 1); box (0, 1, top − 1, 2, 9, 1).
* 3 (0x6a7a0): foundation (−1, 0, z, 4, 2, 1); box (0, 0, z + 1, 2, 1, top − z − 4); box (−1, 0,
  top − 4, 4, 2, 2); box (−2, 0, top − 2, 6, 3, 1); box (−4, 0, top − 1, 10, 4, 1).
* 4 bookshelf (0x69ce0): box (−4, 0, z − 1, 9, 1, 1); h = top − z; box (−4, 0, z, 9, 1, h); box
  (−3, 1, z, 7, 1, h); zz = z + 1; h / 5 times (zz += 5 each): when rand mod 2 ≠ 0 skip; air
  (−2, 0, zz, 5, 2, 3, 0); when rand mod 3 ≠ 0 skip; a = rand mod 2 (the local x), b = rand
  mod 2 (the local y); (wx, wy, wz) = the world position of C (a, b, zz); angle = float(rand mod
  4 × 90); a book, an item lying about (main file §4.3, its other fields as 0x118c20 sets them):
  category 0x13, id 0, modifier = rand (the raw value), at (trunc64((double wx + 0.5) ×
  65536.0), likewise y, dots(wz)), angle; appended.
* 5 (0x6a140): foundation (0, 0, z − 1, 2, 2, 1); box (0, 0, z, 2, 2, top − z − 5); box (−1, 0,
  top − 5, 4, 2, 2); box (−2, 0, top − 3, 6, 2, 1); box (−3, 0, top − 2, 8, 2, 1); box (−5, 0,
  top − 1, 12, 2, 1).
* 6 fireplace (0x6b7f0): foundation (−4, 0, z − 1, 9, 3, 1); box (−4, 0, z, 9, 3, 4); box (−3, 0,
  z + 4, 7, 3, 1); box (−2, 0, z + 5, 5, 3, 1); air (−3, 0, z, 7, 3, 3, 0); air (−2, 0, z + 3, 5,
  3, 1, 0); two colours ((80, 120, 80), 0, z; (80, 80, 80), 0, z + 4; absolute); box (x, 1, z, 1,
  1, 4) for x = −2, 0, 2; one colour (1, 1, 1), 0; box (−3, 0, z, 7, 1, 4).
* 7 window box (0x6bb20): e = z + 1 + rand mod 4; two colours ((80, 120, 40), 0, e; (80, 120,
  130), 0, e + 4); box (−1, 0, e, 3, 3, 1); box (−2, 0, e + 1, 5, 3, 3); box (−1, 0, e + 4, 3, 3,
  1); air (−1, 0, e + 1, 3, 3, 3, 0); one colour (1, 1, 1); box (−1, −1, e + 1, 3, 1, 3); two
  colours (light, 0, 0; (60, 150, 20), 0, e); when rand mod 3 = 0 stop; box (−1, 0, e + 1, 3, 2,
  1); when rand mod 3 = 0 stop; b = rand mod 3 − 1; box (b, 2, e + 1, 1, 1, 1); r = rand mod 10;
  box (b, 3, e + 2 − r, 1, 1, r).
* 8–10: open question 6 of the main file.

**Ceiling model** 0x68510 (x, y, t): nothing when the ceiling list is empty or rand mod 2 ≠ 0.
Up to 10 times while the cell mason's block at (x, y, t − 1) is taken, t −= 1. kind = pick;
kind 1 and rand mod 20 ≠ 0 → nothing. X, Y = dots of the world position + 32768, Z = dots + 655.
yaw = float(rand mod 4 × 90). Kind 0: s = rand fraction × 0.06 + 0.08; Z −= trunc64(s × 20 ×
65536); model 0x48 size s. Kind 1: model 0x49 at Z − 0x40000, size 0.15. Appended without the
inside test.

**Centre piece** 0x696b0 (x, y, top, z0): nothing when the centre list is empty; kind = pick; 0 →
cone; 4 → lamp; others: open question 6.

**Lamp on a chain** 0x68b00 (x, y, t, z0): nothing unless rand mod 8 = 0. x += rand mod 5 − 2,
y += rand mod 5 − 2. Up to 10 times while (x, y, t − 1) is taken, t −= 1. inside (a, b): p =
S(double(x + a) × 0.3, 0) + float a, q = S(double(y + b) × 0.3, 0) + float b; true unless q² + p² >
1, or when float b² + float a² ≤ 1. Fail when any (a, b) in −2..2 is inside and (x + a, y + b, t +
2) is not taken. Then for each inside (a, b): put (0x7f, 0x7f, 0x7f, 255, 0, 0x22) at (x + a, y +
b, t); when the blocks at height t + 1 at (x + a, y + b) and its four side neighbours are all
solid, put (0x7f, 0x7f, 0x7f, 255, type 0x14, 0x22) at t + 1. The chain: model 0x59 at
(dots(wx) + 32112, dots(wy) + 32112, dots(wz) − 67502) of the world (x, y, t + 2), size float(wz −
z0 − 1), yaw 0.7; appended without the inside test.

**Hanging cone** 0x690b0 (x, y, t): r = float rand; r = (r + r) / 32767 + 3; h = trunc(rand
fraction × 10 + r). offset (X, Y, z) = (S(Y × 0.1, z × 0.1) × 1.5 + float(X − x), S(X × 0.1,
z × 0.1 + 843) × 1.5 + float(Y − y)) (local X, Y). Fail when any (X, Y) from trunc(float x − r −
1.5) while float X ≤ float x + r + 1.5 (likewise Y) with offset (X, Y, t + 1) inside r² has
(X, Y, t + 2) not taken. Two colours (c0, 0, t − h; c1, 1, t). For Z from t − h to t + 1: rr =
float(Z − (t − h)) / float h × r; the same box with rr; where a² + b² ≤ rr², the block of type 6,
flags 0x22.

### 6.2 Room objects (0x2fc270)

Arguments: the cell, z0, t, f. kind′ = B when f < 0, else the place kind. k = 0 when f < 0, else
the town block of the cell. When f ≥ 0 and (roof 3, or t 26, 86 or 95): nothing.

**Mode** (f ≥ 0 only; level rooms have mode 0): hall = t 24 or 100, or t − 36 in the set {0, 1,
10, 35, 40, 45, 53, 55, 56} (mask 0x1a0210800000403). hall and (k 2–6 or 11) → 1; k 7–9 or 0 → 0;
k 2 → 2 + (1 when f ≠ 0); k 11 → 2 when rand mod 2 ≠ 0, else 0; otherwise: list the floors of the
3 × 3 cells of the town block (bx = 3⌊cx/3⌋, by likewise; a, b = 0..2, a outer; for each floor fl
below most floors (T5) of that cell's type that exists): ground floors (fl = 0) in one list,
upper in another. chosen = upper[rand mod size] when there are upper floors, else ground[rand
mod size] when any, else none; mode 3 when chosen is (cx, cy, f), else 2 when rand mod 4 = 0,
else 0.

Objects (Z0 = dots(z0); the cell's centre (dots(16 cx + 8), dots(16 cy + 8))):
* Mode 3: object 0x20 at the centre + dots(rand mod 5 − 2) on x, then on y; turn rand mod 4;
  settled and kept. Then object 0x21 at that position moved by turn: 0 → x += sway, y += 65536;
  1 → x −= 65536, y += sway; 2 → x += sway, y −= 65536; 3 → x += 65536, y += sway; sway = −1.75
  or +1.75 blocks in dots (trunc64(±1.75 × 65536.0) in double) by rand mod 2 ≠ 0 → negative;
  settled and kept.
* Mode 2: object 0x18 at the centre + dots(rand mod 3 − 1) on x then y, turn rand mod 4, settled
  and kept (its position is the one after settling). Then four sides with d = 163840 (2.5
  blocks): (0, −d), (0, d), (−d, 0), (d, 0), turns 2, 0, 1, 3: when rand mod 4 = 0 skip; object
  0x1c at the bed's position plus the offset, its z, settled and kept.
* Mode 1: object 0x2c at (16 cx + 8, 16 cy + 5), (16 cx + 8, 16 cy + 11), (16 cx + 5, 16 cy + 8),
  (16 cx + 11, 16 cy + 8) with turns 2, 0, 1, 3, z Z0, settled and kept.

**Along the walls.** list = {28, 45, 42, 74, 40, 38, 39, 37, 30, 31}; full = roofed (T7);
spacing 3. f < 0: full = true; by kind′ − 40: 0 → {51, 28, 27, 90}; 1 → {28, 90, 31, 38, 39, 37};
3, 5, 6 → {54, 27, 74}; 7 → {48, 27, 31, 90, 89}; others → {48, 28, 37, 38, 40, 39, 90}. f ≥ 0:
kind′ = 8 → {48, 28, 31, 90, 74}; else roof 4 → {51, 28, 27, 31}; then when not full: {74} and
spacing 5. "Taken" here: solid or flag 0x40. edge = 98304 (1.5 blocks).

For wall 0 (south, y low), 1 (north), 2 (west, x low), 3 (east), and i = −1, 0, 1:
off = −spacing + (i + 1) spacing; skip when rand mod 3 ≠ 0 or (not full and i = 0). alongX = wall
< 2; sign = +1 for walls 0, 2, else −1. a = dots(16 (cx when alongX, else cy) + 8 + off);
b = dots(16 (cy or cx)) + edge − (0 when full, else 65536) for sign +1, or dots(16 (cy or cx) +
16) − edge + (0 or 65536) for sign −1. Up to 4 times while the block at (a, b, Z0) (x = a, y = b
along x; swapped otherwise) is taken, b += sign × 65536. type = list[rand mod size]; b += sign ×
trunc64(double size y (T8) × 0.5 × 65536.0); skip when (a, b, Z0) is taken. open = any of (a +
dots(d), b − sign × edge, Z0 + dots(c)) for c = 0..4, d = −1..1 (c outer) not taken; skip when
open and full. turn = (2, 0, 1, 3)[wall]. Types 45, 48, 51, 54 → + rand mod 3; type 0x4a →
+ rand mod 9. Position (a, b) or (b, a), z Z0; settled and kept.

### 6.3 Pieces by the room's code

Slots for these: light at z0 − depth0, floorColour at z0, floorColour2 at top0, all with the
given pattern, absolute.
* code 8: slots pattern 1; foundation (4, 4, z0 − 1, 8, 8, 1, 0, 0).
* code 1: slots pattern 1; z = top0 − 2; up to 10 times: when the block at (8, 8, z − 1) is open
  and (no flag 0x40 or flag 0x80), stop; else z −= 1. K's style 2 or B 40: round post (8, 8, z0,
  2, z − z0, grounded); else capped post (8, 8, z0, 2, z − z0, grounded), and for B 42 or 44 box
  (4, 4, z, 8, 8, 4).
* code 12 (0x31273b): slots pattern 0; h = rand mod 5 + 8, r = rand mod 4 + 3; the mason's hanging
  cone (8, 8, top0 − 2, r, h).
* code 11 (0x312802): one colour (150, 120, 50), pattern 0; z as for code 1; foundation (7, 7, z0,
  2, 2, z − z0); alongX = rand mod 2 ≠ 0: box (0, 7, z − 2, 16, 2, 2), else box (7, 0, z − 2, 2,
  16, 2). When rand mod 4 = 0: along = rand mod 2 × 8 + 4; a lamp model 0x17 at x = dots(16 cx) +
  dots(along) along x, else dots(16 cx + 8); y = dots(16 cy + 8) along x, else dots(16 cy) +
  dots(along); z = dots(z − 3); size 0.1, yaw 0, flags 1; kept if inside.

The order in the building: furniture (6.1), room objects (6.2), then 6.3.

## 7. Corner posts (0x312dd7)

When (roof 3, 4, 6, 9 or 10, or t 16, or sub 1, 3, 4 or 6) and t ≠ 26: kind = (roof 1, 9 or 10:
3 when sub 3 or 6, else 0; otherwise xs(s + 0xdaad) mod 3). Two colours (colB, 1, 0; colA, 1,
10; relative); kind 3 → one colour colA × 0.8, pattern 0. Write mode 3 unless t is 13, 16, 49,
50, 51, 53, 60, 86, 94, 95, 97 or roof 9. For the corners (0, 0), (16, 0), (0, 16), (16, 16)
with their three neighbours (W or E, S or N, the diagonal, in that order): lo = base, hi = height
(T4) + base; for each neighbour that is a building, lo = min(lo, its base), hi = max(hi, its
height + base). kind 0: square post (x, y, lo − 1, 1, hi − lo − 1, grounded); 1: capped post; 2:
round post; 3: foundation (x − 1, y − 1, lo − 1, 2, 2, hi − lo + 1, 0, 0). Write mode back to 0.

## 8. Roof and extras (0x313913..0x3148d1)

**Roof**, when t keeps its height: joins; x from (0 when joins W, else −overhang) to (16 when
joins E, else 16 + overhang), y likewise; ridge = ridge sides. One colour, the roof colour (T14),
pattern 0; S5 = that scheme. Write mode 3. sub 1: scheme S1; box (−3, −3, base + height − 2, 22,
22, 2); box (−1, −1, base + height − 3, 18, 18, 1). sub 3: one colour colA × 0.8, pattern 0; box
(−1, −1, base + height − 2, 18, 18, 2). For each x, y (x outer): h = roof height (x, y); when
(x, y) is within 0..15 or the roof is deep: scheme S1; for k from (−2 when deep, else 0) to h − 1
the block of type 6, flags 0 at base + height + k. Scheme S5; top = height + h; the block at base
+ top; when h exceeds any of the roof heights at (x ± 1, y), (x, y ± 1) by more than 1, top = h + 1
+ height and the block there too. When the snow test (wx, wy, warmth at (wx, wy) with the place
there, base, base, base, base) holds (base as floats): (100, 140, 200, 0, 10, 1) at base + 1 +
top. Type 88: write mode 0; two colours (colA, 1, base + height; colA × 0.5, 1, base + height +
25; absolute); box (5, 5, base + height − 3, 6, 6, 28); air (6, 6, base + height − 3, 4, 4, 28,
0). Write mode 0.

Then two colours (colB, 1, 0; colA, 1, 10; relative). Inside the zone: t 53, 14, 16, 60 → box (4,
4, base (cx, cy, t), 8, 8, 2). t 57, 61 (inside or not): a new mason at the cell, two colours
(wall colour, 1, 0; the same, 1, 20; relative); square posts at (0, 0), (15, 0), (15, 15), (0,
15), base (cx, cy, t), radius 1, height 10, grounded.

**Objects on plinths** (0x31402f), inside the zone, b = base (cx, cy, t): "middle (type, z)" is an
object at (dots(16 cx + 8), dots(16 cy + 8), dots(z)) with turn rand mod 4, kept if inside.
t 53 → middle (0, b + 2); t 14, 16, 60 → an item lying about (0x3141c4): category 30, angle =
rand / 32767 × 360, id = rand mod 2, rarity rand mod 2 + 3, modifier rnd((cx + 8883) + (cy +
983049823) × 57) (32-bit), no region, bit 1 of +0xc4 set, at (dots(16 cx + 8), dots(16 cy + 8),
dots(b + 2)); t 51 → middle (1, b); t 61 → middle (2, b).

Only the objects on plinths need the cell inside the zone: for a cell outside it the game skips
them and goes straight on to the town squares, which every cell of the building builder reaches,
inside the zone or not (the plinth branches at 0x31403c, 0x3142a6, 0x3144d2 jump to 0x3146ca
when the cell is outside).

**Town squares** (0x3146ca), for every cell of the building builder, inside the zone or not:
when the town block (of the zone's place kind) is 7–9: u = the cell turn; y = dots(16 cy + 4 +
rand mod 8) + dots(0.5), then x likewise (y drawn first); z = dots(base); turn rand mod 4; type
by (block, u): 7 → (0, 0x62, 0x63, 0x68)[u]; 8 → (0, 0x66, 0x67, 0); 9 → (0, 0x65, 0x64, 0); 0
for u = −1. When the type ≠ 0: settled with the ground test (result ignored), then kept if
inside.

## 9. Castle cells (0x2a8290)

Returns four bytes: kind, detail, style, steps. Kind 0 nothing; 8 a passage through a room
wall; 9–12 room middles; 1–6 floors and room features.

1. type(cx, cy) 39, 86, 94 or 95 → kind 1, the rest 0.
2. c1 = the place cell, even when big; k = 0x2b99d0; B′ = 0x2aa270 (c1, k, big); 0 → nothing.
3. **Direction**: m = the landmark point; best = max(m.x − (c1.x << 11), 0); dir = 2, b2 = best;
   d = m.y − (c1.y << 11); when d > best, b2 = d, else dir = 0; e = (c1.x + 2) × 1024 − m.x; when e > b2, dir = 1, b2 = e; f = (c1.y + 2) × 1024 − m.y; when f > b2,
   dir = 3.
4. The room square (rx, ry) = (⌊cx/8⌋, ⌊cy/8⌋); anchor a = 0x2a9df0 (c1, k, big); the anchor room
   (⌊a.x/8⌋, ⌊a.y/8⌋). The room record = room (rx, ry) (§9.1); none → nothing.
5. With g = H(rx, ry) and R3(k′) = rnd(g + k′) mod 3: a0 = R3(0x4e6289), a1 = R3(0x21d4a), a2 =
   R3(0x11a43f), a3 = R3(0x14f747); lx = posmod(cx, 8), ly = posmod(cy, 8). Doorways: door(lo, o1,
   hi, o2, o3) = max(lo, R3(o1)) + 1 + rnd(g + o3) mod (7 − max(hi, R3(o2)) − (max(lo, R3(o1)) +
   1)). W = door(a2, 0x11a43e, a3, 0x14f746, 0x7bf18); S = door(a0, 0x4e6250, a1, 0x21d11,
   0x14018); E = door(a2, 0x11a440, a3, 0x14f748, 0x7bf19); N = door(a0, 0x4e62c2, a1, 0x21d83,
   0x14051). (The modulus is unsigned.)
6. In the anchor's room: bx = posmod(a.x, 8), by = posmod(a.y, 8); a0 = 0 when bx ≤ a0 + 1; a1 = 0
   when bx ≥ 6 − a1; a2 = 0 when by ≤ a2 + 1; a3 = 0 when by ≥ 6 − a3.
7. **Passage** (neighbour room (nx, ny), recSides, nbSides, wall coordinate, c1 coordinate, at
   edge): nb = room (nx, ny); false when nb has no type, when |nb.steps − rec.steps| ≥ 2, when
   bit rec.side of recSides or bit nb.side of nbSides is clear, or when 2⌊⌊wall / 1024⌋ / 2⌋
   differs from the c1 coordinate. Then: not at edge → (8, 0, rec.b2, rec.steps); rec.b2 = 2 and nb.b2 ≠ 2 → (8, 5,
   nb.b2, nb.steps); rec.type 4, 6 or 8 and nb.steps > rec.steps → (8, 2, rec.b2, rec.steps);
   rec.type 2 and nb.steps > rec.steps → (8, 6, …); else (8, 0, …). True.
8. Walls: lx ≤ a0 → (ly = W and passage (rx − 1, ry, 0b11, 0b101, 128 rx − 128, c1.x, lx = 0)) or
   nothing. Else ly ≤ a2 → (lx = S and passage (rx, ry − 1, 0b1001, 0b10001, 128 ry − 128, c1.y,
   ly = 0)). Else lx ≥ 7 − a1 → (ly = E and passage (rx + 1, ry, 0b101, 0b11, 128 (rx + 1), c1.x,
   lx = 7)). Else ly ≥ 7 − a3 → (lx = N and passage (rx, ry + 1, 0b10001, 0b1001, 128 (ry + 1),
   c1.y, ly = 7)).
9. Inside: mx = (a0 − a1 + 8) / 2, my = (a2 − a3 + 8) / 2; floorKind = 1 when R(cx, cy, 0x7dbcb9a)
   mod 3 ≠ 0, else 6. deco = 0; unless type(cx, cy) is 39, 75, 80, 85, 86, 94 or 95: h =
   rnd(H(cx / 2, cy / 2) + 0x7a667) (C division); rec.flags & 1 and R(cx, cy, 0x468acd) mod 3 = 0
   → 1; else rec.flags & 2 and h even → 7; else B′ 42 and h even → 11; else B′ 44 and h even → 12.
   special = (floorKind, deco, rec.b2, rec.steps).
10. rec.type 2 or 3 and (lx, ly) = (a0 + 1, a2 + 1) → (1, 14, b2, steps).
11. rec.type 9–12: special for lx ≤ mx with ly = W and a room at (rx − 1, ry); lx ≥ mx with ly = E
    and a room east; ly ≤ my with lx = S and a room south; ly ≥ my with lx = N and a room north;
    the centre (mx, my). Else (type, 0, b2, steps) with type 9 → 9, 11 → 11, 10 → 10, 12 → 12.
12. rec.type 13–15: special for (a0 + 1, W) with a room west; (6 − a1 ≥ mx, E) east; (S, a2 + 1)
    south; (N, 6 − a3 ≥ my) north. Else (9 for 13, 11 for 14, 12 for 15; detail 8 when R(cx, cy,
    0x54bb5) & 3 ≠ 0, else 0; b2, steps).
13. Others, centre = (mx, my), keep7 = 7 when deco = 7 else 0: type 4: centre (2, keep7), else
    (1, deco); 5: (3, keep7) / (1, deco); 6: (4, keep7) / (floorKind, deco); 7: (5, keep7) / (1,
    deco); 8: (6, deco); any other: (floorKind, deco); with b2 and steps.

### 9.1 Rooms (0x2aa450, 0x2a04e0) and the corridor

A big structure is a corridor of 128-block rooms from the anchor room (ax, ay) out in direction
dir (0 −x, 1 +x, 2 −y, 3 +y), with side rooms.

**Wobble**: cx(t) = ay when t = ax, else trunc(S(double(t / 3) × 0.5, 10984) × 3 + float ay)
(0x2aad70; t / 3 C division); cy(t) = ax when t = ay, else trunc(S(double(t / 3) × 0.5, 76643) ×
3 + float ax) (0x2aad10).

**On the corridor** 0x2b7de0 (x, y): dir 0: x ≤ ax, a = cx(x), b = cx(x − 1), v = y; dir 1: x ≥ ax,
cx(x), cx(x + 1), v = y; dir 2: y ≤ ay, cy(y), cy(y − 1), v = x; dir 3: y ≥ ay, cy(y), cy(y + 1),
v = x; true when v lies between a and b inclusive.

**Steps** 0x2aa880 (x, y): dir 0 (x ≤ ax, else 0): n = ax − x; for i = 0..n, t = ax − i: cur =
(ay when t = ax, else cx(t)), prev = (ay when t − 1 = ax, else cx(t − 1)), at i = n prev = y;
sum += |prev − cur| + 1. dir 1 (x ≥ ax): for t = ax..x: cur as before, next = (ay when t + 1 = ax,
else cx(t + 1)); d = y − cur at t = x, else next − cur; sum += |d| + 1. dirs 2, 3 the same with y
and cy.

**Corridor end** 0x2b8310 (in = 0) and 0x2b88a0 (in = 1): dir 0: x = ⌊1024 c1.x / 128⌋ + in and
y = (ay when x − 1 = ax, else cx(x − 1)); dir 2: y = ⌊1024 c1.y / 128⌋ + in and x = cy(y − 1);
dir 1: x = ⌊(1024 c1.x + 0x7ff) / 128⌋ − in and y = cx(x + 1); dir 3: y = ⌊(1024 c1.y + 0x7ff) /
128⌋ − in and x = cy(y + 1).

**Corridor room** 0x2a04e0 (x, y) → (type, flags, b2, side, steps), all 0 when not on the
corridor: type 1; b2 = 1 when B′ is 42 or 44; steps; steps 1 → type 2, 2 → 3; else end (in 1) → 6;
end (in 0) → 7; else unless (x, y) is the anchor room: v = R(x, y, 0x6c9c4) & 7, v ≤ 5 → type
(9, 10, 13, 14, 8, 4)[v]; flags = 4 when R(x, y, 0xab97d) & 1. Then flags |= 1 when R(x, y,
0x11a0595) & 1; flags |= 2 when B′ is not 41, 43, 44 (42 counts as not small) and R(x, y,
0xe0fae) & 1.

**Room** 0x2aa450 (x, y): the corridor room when it has a type. Else nothing unless R(x, y,
0x71504) mod 5 = 0. The four neighbours' corridor rooms (W = 1, E = 2, S = 3, N = 4); those with a
type, in order, are the candidates; none → nothing. pick = candidates[R(x, y, 0x55260) mod
count]; side = pick; steps = that neighbour's; flags = 1; type 5 when R(x, y, 0x57b071) mod 3 ≠
0, else 4; b2 = 2 for type 5, else 1 when B′ 42 or 44, else 0; flags = 3 when B′ not small and
R(x, y, 0xe0fae) & 1.

## 10. Castle levels and fortresses

**Castle block top** 0x2aadd0 (cx, cy): c1 the place cell (not rounded), big, kind, anchor a. The
room square (bx, by) = (⌊cx/8⌋, ⌊cy/8⌋). When it is the anchor's square and kind is not 45 or
46: base (a, 0) − 40. Else X0 = 128 bx, Y0 = 128 by, X1 = X0 + 128, Y1 = Y0 + 128: h0 = ground
height ((X1 + X0) / 2, (Y1 + Y0) / 2, mode 3), h1 (X1, Y1, 3), h2 (X0, Y1, 3), h3 (X1, Y0, 3),
h4 (X0, Y0, mode 2); m = h1 when h1 ≤ h0 else h0; m = m when h2 > m else h2; likewise h3;
trunc(min(m, h4) − 40) with min as "m < h4 ? m : h4".

**Castle floor** 0x2ab0a0 (cx, cy): t = block top; lx = posmod(cx, 8), ly likewise; the first of
lx = 0 (west), ly = 0 (south), lx = 7 (east), ly = 7 (north) picks the neighbour cell; with one,
t = (max(block top (neighbour), t) + t) / 2. Then c1 (rounded when big), B′; none → t. The room
of (⌊cx/8⌋, ⌊cy/8⌋) (direction and anchor room as in §9, anchor room = (⌊m.x/128⌋, ⌊m.y/128⌋)
from the landmark point here); when its flags have bit 4: t + R(cx / 3, cy / 3, 0x7b7e8) mod 6 (C
division).

**Fortress** (place kind 8): F.c1 = 2⌊⌊16 cx/1024⌋/2⌋ (and y); g = H(F.c1); size = (R(F.c1,
0xc0aa) & 3) + 15; turn = R(F.c1, 0x5d932) & 3; middle (ax, ay) = (⌊p.x/16⌋, ⌊p.y/16⌋) of the
landmark point.

**Fortress cell code** 0x2a25a0 (cx, cy): (x, y) turned as in main file §6.3 rule 17; half =
size / 2; x0 = ax − half, y0, x1 = x0 + size, y1. h = H(2⌊⌊16 x/1024⌋/2⌋, …y) (of the turned
cell); r = R(h, 0x44a753) mod (size − 8) − half + 4 (unsigned modulus). mark: by R(h, 0x4d9848) &
3: 0 → 2 when x = ax − r and y0 + 1 ≤ y ≤ y0 + 3; 1 → … y1 − 4 ≤ y ≤ y1 − 2; 2 → y = ay − r and
x0 + 1 ≤ x ≤ x0 + 3; 3 → y = ay − r and x1 − 4 ≤ x ≤ x1 − 2. Then by R(h, 0x311e6) & 3: 0 → 1 when
x = ax and y0 + 4 ≤ y ≤ y0 + 5; 1 → x = ax, y1 − 6 ≤ y ≤ y1 − 5; 2 → y = ay, x0 + 4 ≤ x ≤ x0 + 5;
3 → y = ay, x1 − 6 ≤ x ≤ x1 − 5 (otherwise mark stays). gapped (v) = 0 when (3x + y) mod 5 = 0 (C)
and rnd(H(x, y)) & 1, else v. When x0 + 1 < x < x1 − 2 and y0 + 1 < y < y1 − 2: when x0 + 3 ≤ x ≤
x1 − 4 and y0 + 3 ≤ y ≤ y1 − 4 and not (x > x0 + 4, y > y0 + 4, x < x1 − 5, y < y1 − 5): mark
non-zero → 3, else gapped (3). When x0 + 6 ≤ x ≤ x1 − 7 and y0 + 6 ≤ y ≤ y1 − 7: 4. Else mark.
Outside: mark → 2, else gapped (2).

**Fortress style** 0x2b6870: R(g, 0x55a05c) mod 3: 1 → 4, 2 → 3, 0 → 2.

**Fortress floor code** 0x2a1d10 (cx, cy, f): type 47: 0 when f ≤ 0 or cx or cy is odd (C
remainder); 13 when the style ≠ 3; else by R(g, 0x100d7) & 3: 0 → 13 when cx < ax and cy < ay; 1
→ cx ≥ ax, cy < ay; 2 → cx ≥ ax, cy ≥ ay; 3 → cx < ax, cy ≥ ay; else 0. Other types: 0 unless the
floor exists; room = the cell code; nextTo (v) = any of the four neighbours' codes is v. room 4 →
0 at the middle, else 1 when R(cx, cy, 0x4f690) & 7 ≠ 0, else 0; room 1 next to 4 → 5; room 2
next to 3 → the style; R(⌊cx/3⌋, ⌊cy/3⌋, 0x559dbf) & 7 = 0 → 7; R(cx, cy, 0x4f690) & 7 = 1 → 1;
R(cx, cy, 0x9b61c) & 7 ≠ 0 → 0; room 2 and R(g, 0x55a05c) mod 3 = 0 → 13; else 0.

## 11. The wizard's tower (0x3039df; cell object 52)

base = base (cx, cy, 52) + 1; a mason at the cell; wall, trim colours; scheme() = two colours
(trim, 1, 0; wall, 1, 20; relative). r = R(cx, cy, 0x255509) mod 10 + 20; height = R(cx, cy,
0x5d0ba) mod 40 + 100 − 2r.
1. Pillar (8, 8, base − 1, r, height + 1, 0, 0). top = height + base.
2. Has battlements (T7): disc (8, 8, top, r + 1, 6); light disc (8, 8, top + 2, r − 1, 4, 0); for k
   from −(r / 5) to r / 5 with x = 7 + 4k: air (x, 7 − r, top + 4, 2, 2r + 2, 2, 0), air (7 − r,
   x, top + 4, 2r + 2, 2, 2, 0). Else disc (8, 8, top, r + 1, 4); light disc (8, 8, top + 2, r −
   1, 2, 0).
3. Keeps its height: one colour, the roof colour, pattern 1; cone (8, 8, top, r, 2r).
4. floors = 4, floorH = height / 4. Light disc (8, 8, base, r − 2, 4 floorH − 2, 2). scheme();
   type 0x17; disc (8, 8, base − 2, r − 2, 2, cap 0, mode 2); type 6.
5. For f = 0..3, z = f floorH + base:
   * variant 0; when rand mod 4 = 0: variant = rand mod 2 + 1; variant 1 → light disc (8, 8, z,
     r, floorH − 2, 2).
   * furnish = rand mod 3. 0, a rug: a = rand mod 12 + 10, b = rand mod 12 + 10; c1's blue, green,
     red drawn in that order as rand mod 150 + 50 each; c2's likewise as rand mod 100 + 150; type f +
     0x17; ox = −(a / 2), oy = −(b / 2); colour c1 box (ox + 8, oy + 8, z − 1, a, b, 1); c2 box (ox
     + 9, oy + 9, z − 1, a − 2, b − 2, 1); c1 box (ox + 10, oy + 10, z − 1, a − 4, b − 4, 1);
     type 6. 1, four posts: scheme(); k = rand mod 3; for i = 0, 16 and j = 0, 16 (ri = 0, r; rj
     = 0, r): q = r / 4; k 0 → square post (ri / 2 − q + 8, rj / 2 − q + 8, z, 1, floorH − 2, not
     grounded); 1 → round post (i, j, z, 1, floorH − 2) (the game passes i, j here); 2 → capped
     post as 0. 2, one post: scheme(); k = rand mod 3; square / round / capped post (8, 8, z, 3,
     floorH − 2, not grounded).
   * f < 3: scheme(); steps = floorH − 1; write mode 4; type f + 0x18; disc (8, 8, z + floorH − 2,
     r − 2, 2); type 6; turns = double f × π; reach = float(r − 4); for i < steps: a = float(double
     i × π / double steps × 0.8999999761581421 + 4.869468463261368 + turns); box (trunc(cos a ×
     reach + 8 − 3), trunc(reach × sin a + 8 − 3), i + z + 1 − 3, 6, 6, 2); when i > steps − 8:
     tt = float r − 4.5, air (trunc(tt cos a + 5), trunc(tt sin a + 5), i + z + 1, 6, 6, 16, 0)
     (computed as + 8 − 3). Write mode 0.
   * variant 2: scheme(); f > 0 → write mode 3, disc (8, 8, z − 2, r + 4, 2), mode 0; air (8 − r,
     5, z, 2, 6, 6), (8 − r, 6, z + 6, 2, 4, 1), (r + 6, 5, z, 2, 6, 6), (r + 6, 6, z + 6, 2, 4,
     1), (5, 8 − r, z, 6, 2, 6), (6, 8 − r, z + 6, 4, 2, 1), (5, r + 6, z, 6, 2, 6), (6, r + 6,
     z + 6, 4, 2, 1).
   * variant 1: scheme(); write mode 2; discs (8, 8, z − 2, r + 3, 2) and (8, 8, z + floorH − 2,
     r + 3, 2); mode 0; for k < 8: a = float(double k × π × 0.25 + 0.7853981633974483); round post
     (trunc(cos a × (r + 1) + 8), trunc(sin a × (r + 1) + 8), z, 2, floorH − 2, not grounded).
   * variant 0: doors (object 7 at dots(x, y, z + 1), the given turn, +0x2c 1000.0, kept if
     inside): f = 0 → air (8 − r, 6, z, 2, 4, 6), and when xs(s + 0x129b4) & 1, air (8 − r, 7,
     z + 6, 2, 2, 1); f > 0 → air (8 − r, 7, z + 1, 2, 2, 4), door (16 cx − r + 9, 16 cy + 8, 1).
     Then air (r + 6, 7, z + 1, 2, 2, 4), door (16 cx + r + 7, 16 cy + 8, 3); air (7, 8 − r, z +
     1, 2, 2, 4), door (16 cx + 8, 16 cy − r + 9, 0); air (7, r + 6, z + 1, 2, 2, 4), door (16 cx
     + 8, 16 cy + r + 7, 0).

## 12. Other set pieces

**Arena master's dais** (0x303315; 97, the arena of place kind 24): mason at the cell, one
colour the wall colour (T11), pattern 0; for i = 0.. with size 16, 14, 12, 10 (while size > 8):
foundation (i, i, base (cx, cy, 97) + i, size, size, 1, 0, 0). No draws. Then the **arena
master** (class 0xa1) is made and added, standing on the dais at z = base (cx, cy, 97) + 4: part C,
`worldgen-creatures-zone.md` §5 (input the cell and its base; output one spawn point; 32 draws
from this cell's stream).

**Water tower** (0x302ce7; 66): base = base (cx, cy, 66) + 1; wall = wall colour; one colour
wall × 0.9 (0x4e070), disc (8, 8, base + 8, 32, 1); wall: disc (8, 8, base + 9, 32, 1); stone =
(140, 150, 180): disc (8, 8, base + 10, 32, 1), disc (8, 8, base + 11, 33, 1); light disc (8, 8,
base + 10, 31, 2, 0); air (2, −25, base + 10, 12, 66, 2, 0), air (−25, 2, base + 10, 66, 12, 2,
0); for i while 11.2 > float i: air (5 − 2i, −24, base + 10, 1, 66, 1, 0), air (10 + 2i, −24, …),
air (−24, 5 − 2i, base + 10, 66, 1, 1, 0), air (−24, 10 + 2i, …). Trunk: for i < 30, one colour
stone + float(i & 1) × 20 on each channel, disc (8, 8, base − 10 + i, 8, 1). Legs: for rot = 0..3,
a child (0, 0, 0, 16, 16, rot, no mirror); g = trunc(ground height (its world (8, −25), mode 0)
− 2); h = base − g + 10; striped wall (12, −25, g, 2, h); striped wall (4, −25, g, 2, h).

**Cistern** (0x303244; 68): mason at the cell, wall colour, pattern 0; foundation (0, 0, base
(cx, cy, 68), 16, 16, 10, 0, 0).

**Aqueduct** (0x303575; 67): base = base (cx, cy, 0); alongX = type(cx ± 1, cy) is 67; one colour
(140, 150, 180). For end 0 then 1: z = max(base, base of the neighbour at −1 or +1 along the axis,
type 0); at = 0 or 15; along x: foundation (at, 5, z, 1, 6, 6, 0, 0), box (at, 6, z + 6, 1, 4, 1);
along y: foundation (5, at, z, 6, 1, 6), box (6, at, z + 6, 4, 1, 1). Then the wall colour of 67,
pattern 0; tube along x (0, 7, base + 3, base + 3, 16, 2) or along y (7, 0, …).

**Tower platform** (0x3029aa; 69, 70): z = base (cx, cy, t) + 1; mason at the cell; colA, colB;
r = xs(s + 0x17f55) mod 13 + 20 (0x25eca0); lift = 3; two colours (colB, 1, 0; colA, 1, 20;
relative). t = 70: lift = 10, r −= 5, spiral ring (8, 8, z − 10, r − 1, r + 6, 22). grassy =
grassy floor (T7); top = lift + z; pillar (8, 8, top + 1, r, 1, cap 6 and mode 2 when grassy,
else 0, 0). Water line test → water line = lift + 10. kind = xs(s + 0xdaad) mod 3; rr = float(r −
3); for k < 8: a = float(double k × 2π × 0.125); post at (trunc(cos a × rr + 8), trunc(sin a × rr
+ 8)) (y from sin), top, radius 2, height 15, grounded: kind 1 capped, 2 round, else square.

## 13. Crop fields and street lamps

**Crop field** (0x305115; 27–34, inside the zone). roadAt (X, Y) = roads (X, Y, the zone place's
kind, its weight at (X, Y) by warped distance). Bounds x0 = 16 cx, x1 = 16 cx + 16, y0, y1.
Fences, sides 0 (y low, neighbour (0, −1)), 1 (x low, (−1, 0)), 2 (y high, (0, 1)), 3 (x high,
(1, 0)): skip when rand mod 4 = 0 or the neighbour's type is not fence-free (T7); shrink the bound
on that side by one; draw one rand (unused); for k < 8: along x (sides 0, 2): X = 16 cx + 1 + 2k,
Y = 16 cy (+ 15 for side 2); along y: X = 16 cx (+ 15 for side 3), Y = 16 cy + 1 + 2k. Skip when
roadAt > 0.45. h = trunc(max(ground height at (X − 1, Y) or (X, Y − 1) along y, ground height at
(X, Y)) + 1) (max as "a > b ? a : b", both mode 0). Object: 34 → 0x47 + rand mod 3, else 0x43 +
rand mod 4; x = dots(X) + dots(0.01 along x, 0.51 along y), y = dots(Y) + dots(0.51 along x, 0.01
along y); z dots(h); turn 0 along x else 1; appended directly unless the block at its position is
water.

Type 34: for a, b in 0..1 (a outer): skip when rand mod 4 = 0; X = (2 cx + a) × 8 + 4, Y likewise;
skip when roadAt > 0.45; h = trunc(ground height + 1); when rand mod 4 ≠ 0: object 0x7f + rand mod 3
at (dots(X) + 32768, dots(Y) + 32768, dots(h)), turn left unset, appended directly; else a model
0x51 there, yaw rand × 360 / 32767 (float of rand × 360.0, divided), size 0.07, appended directly.
Done.

Types 27–33: rows = (cx / 3 + cy / 3) & 1 (C division). For X in the cell, Y inner: skip when
roadAt > 0.5; h = trunc(ground height (X, Y, mode 0)); v = ((rows ? X : Y) / 2) mod 2 (C); k =
float v × 0.05 + 0.75; put the river bed colour at (X, Y, h) × k, packed, type 5, flags 0, at h.
Skip outside the shrunk bounds. h += 1; skip unless the block at h is air (type 0). crop, big =
−1, scale 0.1, turned = true; 27: crop 0x24, big 0x74; 28: 0x26, 0.2; 29: 0x27, 0.2; 30: rand mod
10 = 0 → 0x22, 0.05, else 0x2a, 0.2; 31: 0x2b, 0.2; 32: 0x25, 0.15; 33: none. Crop 0x26 is not
turned. When rand mod 4 = 0: a kind-21 spot (contents file §C1) at (dots(X) + 32768, dots(Y) +
32768, dots(h)); next. When rand mod 10 ≠ 0, next. When rand mod 12 = 0 and X, Y even and big ≠
−1: object big at that position, turn rand mod 4, appended directly. Else when crop ≠ 0: a model
crop, yaw rand × 360 / 32767 when turned else 0, size scale, appended directly.

**Street lamps** (0x315fd8; place kinds 5 and 9): for a, b = 0..3 (a outer) with (a + 3b) mod 5
= 0: X = 64 zx + 8 + 16a, Y = 64 zy + 8 + 16b; w = place weight (kind, big, warped distance from
the place centre to (X, Y)); skip when 0.5 > w or type(⌊X/16⌋, ⌊Y/16⌋) ≠ 0; r = roads (X, Y,
kind, w); skip unless r > 0.5 and 0.8 > r. n = the neighbourhood at (X, Y): forest weight (kind,
w) > 0.5 → object 0x42; else warmth > 0.85 → 0x41; else 0x3f + (rnd(H(C1)) & 1). Position
(dots(X) + dots(0.5), dots(Y) + dots(0.5)), z = trunc64((ground height (x toward zero, y toward
zero, mode 0) + 1) × 65536); settled and kept.

## Errata (from the clean-room implementation, checked against the oracles)

- **Fixed (§8, town squares).** The old text put the town squares after "inside the zone" and
  read as if they needed the cell inside. They run for every cell of the building builder;
  only the plinth objects need the cell inside. Evidence: genb_seed_123 zones (−15, −17),
  (−15, −18), (−16, −17), (−16, −18), (−16, −20), (−17, −19) draw at 0x314704, 0x314771 and
  0x314809 for building cells outside the zone (in (−16, −20) the first is cell (−65, −78), type
  25, i = −1: its floor draws, then the three square draws, then the next building).
- **Fixed (§12).** Cell object 97 is the arena master's dais, not a shrine; its creature is
  specified in part C (`worldgen-creatures-zone.md` §5), and it draws 32 numbers, not three.
- §4 step d: the wall box is drawn only in the "Otherwise" branch. Columns of the first branch (types that don't keep their height, and the outside columns of roofed types) are already filled by the foundation. Read this way, every building column matches the .cwz.
- §6, side model kind 1 (type 0x43): the game gives it tint (0.6, 0.5, 0.2) and flags 1, so it goes into the light list (+0x90); this spec gives no tint or flags. Seen in 7 of 125 gen_seed_123 zones and 29 seed_123 records. To check in the decompile.
- Type 0x49 models: in 4 seed_123 zones ((−2002,−237), (−2002,−238), (−2003,−238), (−2004,−236)) the game's type-0x49 models have flags 1 and appear in +0x90, but no spec gives that model a light flag. To check in the decompile.
