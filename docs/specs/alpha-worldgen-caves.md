# Alpha world generation: rock formations, roads, terraces and the zone post passes

Behavioural specification of what the Cube World **Alpha** zone generator 0x5e4850 does
between the surface pass and the voxel-tree grid (code range 0x5e6b58..0x5e8b7f, plus the
ravine spawns of the surface pass), and of the two formation builders 0x5cbe00 and 0x5cb600.
Written from `Cube.exe` (SHA-256 `84a7a132…30bf`); every formula, constant and draw was
checked against the assembly (the zone generator's case bodies at 0x5e646b..0x5e699e and
the routines below were disassembled directly from the binary).

**There are no caves in this range.** The earlier specs (overview §5, terrain §5.5
"caves", vegetation §8 item 6) read 0x5cbe00 as a cave carver. It is the opposite: it
**adds** a noise-shaped mound of rock on top of the ground. Alpha's only carving is the
ravine of the surface pass (terrain spec §5.5) and the dungeon builder. The file keeps its
planned name so the links in the other specs stay valid. Corrections to earlier statements
are collected in §9.

Read first: [random](alpha-worldgen-random.md), [regions](alpha-worldgen-regions.md),
[terrain](alpha-worldgen-terrain.md), [locations](alpha-worldgen-locations.md).
The voxel-tree builder (used by the giant tree, §4.3) is specified in
[alpha-worldgen-trees.md](alpha-worldgen-trees.md).

---

## 0. Conventions

* As in the terrain spec §0: float unless marked double, N arguments double sums
  (offset + coordinate·k), trunc towards zero, x in the outer loop and y inner unless
  stated. "draw" = one `rand()` from the zone's stream (seeded with A + 65536·zy + zx,
  terrain spec §5.1).
* The zone being generated is (zx, zy), its x range [256·zx, 256·zx + 256), likewise y.
  **Lz** = location record of the zone's sub-cell (zx >> 3, zy >> 3) (locations spec);
  **E** = the zone's 16-byte table entry (kind, variant, id, level, sub-level), copied to the
  zone at +0x78 (locations spec §1.1).
* **top(x, y)** = bottom + count of the column (terrain spec §1.2), i.e. one above the
  highest stored block.
* **read(x, y, z)** = the block read rules of terrain spec §1.4: below the bottom solid grey
  (200, 200, 200, type 1); above the stored range air (255, 255, 255, 0) when z > 0 and water
  (255, 255, 255, byte 3 = 0x82) when z ≤ 0; a stored type-0 block with z ≤ 0 and bit 0x40
  clear reads as that same water.
* **write(x, y, z, b)** = the block writer of terrain spec §1.5. Consequences used below:
  writing air (type byte 0) at or above the column top does nothing; writing a block whose
  byte 3 AND 0x5f is 0 at z ≤ 0 stores water (255, 255, 255, 0x82) instead.
* **Zone-local.** Every column lookup in this range passes the zone being generated; a
  column outside that zone counts as missing (0x4347a0), so nothing here reads or writes a
  neighbouring zone. Shapes that extend past the zone edge are cut off there.
* **Colours**: Rk = rock, Cl = cliff, surface colour = terrain spec §6.6, §6.7, §6.1. A block
  built from float channels takes trunc of each channel and stores the low 8 bits.
* **home** = (8396928, 8396928) (world + 0x8000f0/f4, floats). "Within d of home" compares
  the float (x − homeX)² + (y − homeY)² with d².

---

## 1. The rock mound builder (0x5cbe00)

Inputs: column (x, y), base height z, half-sizes rx, ry, height rz (all integers), the zone.

**Shape.** For a column (X, Y) let the mound's top be
zt(X, Y) = trunc(f32(f32(N(0.01X + 4394, 0.01Y + 8974) · 20) + (float)(z + rz))).
A cell (X, Y, Z) is **inside** when u² + v² ≤ 1 with
u = f32(N(0.05Y, 0.05Z) · f32(10 / rx)) + f32((X − x) / rx),
v = f32(N(0.05X + 4374, 0.05Z + 9898) · f32(10 / ry)) + f32((Y − y) / ry)
(divisions in float; the sum is formed as v² + u²). So the cross-section is an ellipse of
half-axes about rx, ry whose outline wobbles by about ±10 blocks and changes with height;
the body is a vertical column of rock from the ground up to the noisy top zt.

**Phase 1, the guard.** For X = x − 2rx … x + 2rx (outer), Y = y − 2ry … y + 2ry (inner),
for columns that exist, for Z = zt(X, Y) down to the column bottom: if the cell is inside,
read it; if its type is not 2 (water) and its byte 3 has bit 0x80 set, **abort the whole
routine** (nothing is written). Terrain blocks never carry bit 0x80, and nothing in this
range writes such blocks before the last formation is built, so in a freshly generated
zone the guard never triggers; it protects structure blocks.

**Phase 2, the fill.** Same loops. For each existing column, b0 = its bottom; for
Z = zt(X, Y) down to b0 inclusive: read the cell; continue only if its type is 0, 2, 4, 9 or
5 and bit 0x40 is clear; then if the cell is inside:
* Z = zt(X, Y) (the column's first cell): write the **surface colour** block of
  (X, Y, Z) with the column's stored moisture and warmth (terrain §6.1: three colour bytes
  and type byte t | 0x20);
* otherwise write (Rk(X, Y, Z), type 6).

Air above the ground, water, grass, sand and earlier mesa blocks become rock; existing rock,
snow, lake bed, town ground and everything below the column bottom are left alone. A
column whose top cell is outside the ellipse gets no surface-coloured cap (its highest
inside cell is plain rock).

No draws.

---

## 2. The layered mesa builder (0x5cb600)

Same inputs and loops as §1, with these differences:

* top: zt(X, Y) = trunc(f32(f32(N(0.01X + 4394, 0.01Y + 8974) · (float)rz) + (float)(z + rz)))
  (the noise is scaled by rz instead of 20);
* guard (phase 1) inside test: u = f32(N(0.05Y, 0.05Z)·0.4) + f32((X − x)/rx),
  v = f32(N(0.05X + 4374, 0.05Z + 9898)·0.4) + f32((Y − y)/ry);
* fill (phase 2) inside test: u = f32(N(0.05Y, 0.02Z)·0.5) + f32((X − x)/rx),
  v = f32(N(0.05X + 4374, 0.02Z + 9898)·0.5) + f32((Y − y)/ry) (the vertical noise scale is
  0.02, the wobble a fixed half radius);
* fill colour below the top: s = sat(f32((float)(Z − b0 − 8) · 0.05)) (0.05 is the float
  0.05000000074505806), colour = Cl(X, Y, Z)·(1 − s) + Rk(X, Y, Z)·s per channel (the cliff
  part is computed first), type **5**. So the lowest 8 blocks are cliff-coloured and the
  colour fades to rock over the next 20.
* the top cell gets the surface colour exactly as in §1.

No draws.

---

## 3. Order of the post passes (0x5e6b58..0x5e8b7f)

After the surface pass (terrain spec §5.4–§5.6), in this order:

| § | Code | Condition | Draws |
|---|---|---|---|
| 3.1 | 0x5e6b58 | always | 0 |
| 3.2 | 0x5e6c56..0x5e6f6c | Lz type 6 | 9 to 36 |
| 3.3 | 0x5e6f72..0x5e703a | Lz type 13 or 4 with its centre in this zone | 0 |
| 4.1 | 0x5e7040..0x5e7143 | Lz type 11 with its centre in this zone | 1 |
| 4.2 | 0x5e7154..0x5e727a | otherwise, Lz type 12 with its centre in this zone | 1 + tree builder |
| 4.3 | 0x5e727a..0x5e7687 | otherwise, E.kind ∉ {1, 3, 4} | 1 + 5 or 6 per formation |
| 5.1 | 0x5e7321..0x5e7e19 | always (roads) | 0 |
| 5.2 | 0x5e7e2d..0x5e8490 | always (terraces) | 1 per terrace floor written |
| 6.1 | 0x5e8496..0x5e84fc | E.kind = 3 or 5 (dungeon) | dungeon builder (follow-up) |
| 6.2 | 0x5e84fc..0x5e888e | always (terrace boulders) | 3 per seed |
| 6.3 | 0x5e888e..0x5e8c2b | always (ravine boulders) | 3 per seed |

"Centre in this zone": the zone (blk(L.x) floor-div 256, blk(L.y) floor-div 256) of the
location centre equals (zx, zy).

### 3.1 Setup (0x5e6b58)

The height grid is freed; E is copied into the zone (+0x78..+0x87); an empty **object
list** is created (it is filled only after this range, by the camp pass and the type-10
site statics of §8, and is
consulted by the distance tests below, which therefore never reject anything here); the
zone-centre climate is read from the stored column (256zx + 128, 256zy + 128):
**mc** = its moisture, **wc** = its warmth.

### 3.2 Type 6: the 3 × 3 mound grid

For a = 0, 256, 512 (outer) and b = 0, 256, 512 (inner):

1. draw; if (draw & 3) = 0, next candidate.
2. (X, Y) = (256zx + a/3 + 42, 256zy + b/3 + 42) (integer division: offsets 42, 127, 212).
3. f = f(Lz, X << 16, Y << 16) (regions spec §7); v = 1 − f; if v ≤ 0 or v² < 0.5, next
   candidate.
4. (Object-list test: an object within 80 blocks would skip; the list is empty here.)
5. draw: rx = draw % 10 + 20; draw: ry = draw % 10 + 20; draw: rz = (draw & 15) + 20.
6. If within 60 blocks of home (squared distance < 3600), next candidate.
7. Rock mound (§1) at (X, Y, top(X, Y)) with (rx, ry, rz).

Per candidate 1 draw, or 4 when it passes steps 1–3. The whole 2048-block sub-cell of a
type-6 location runs this in each of its 64 zones; the bump test keeps the mounds near the
valley centre.

### 3.3 Types 13 and 4

When Lz is of type 13 or 4 and its centre lies in this zone, the routine searches down
from top(centre) for the first cell whose type is neither 0 nor 2. The result is never used
(dead code); nothing is written and nothing is drawn.

---

## 4. The exclusive chain: great mound, giant tree or rock pass

Exactly one of §4.1, §4.2, §4.3 can run (§4.3 only when neither of the others ran).

### 4.1 Type 11: the great mound

If Lz type = 11 and its centre is in this zone: draw; rock mound (§1) at the zone centre
(256zx + 128, 256zy + 128, top there) with rx = ry = 100 and rz = 100 + draw % 100. Its
±200-block box is cut at the zone edges (zone-local writes), so the rock is at most
256 × 256 blocks. Then the chain ends.

### 4.2 Type 12: the giant tree

Otherwise, if Lz type = 12 and its centre is in this zone: one draw (its value is unused);
voxel tree (0x5df960, trees spec) at the zone centre on top(centre) with size 80, height 80,
kind 6 (the call's arguments are (x, y, z, 80, 80, 6, zone)); then a downward search from
the column top for the first cell that is neither air nor water, whose result is unused.
The chain ends.

### 4.3 The rock pass

Otherwise, if E.kind is not 1, 3 or 4:

1. draw: n = draw & 3.
2. **Climate bonus**: n ← n + 2 if (wc > 0.6 or wc < 0.3) and (mc > 0.7 or (mc < 0.4 and
   (mc > 0.2 or wc < 0.8))). (Float constants 0.6000000238418579, 0.30000001192092896,
   0.699999988079071, 0.4000000059604645, 0.20000000298023224, 0.800000011920929; strict
   comparisons.) So 0..5 formations; extreme climates get more.
3. For each of the n formations:
   1. draw: rx = draw % 40 + 10; draw: ry = draw % 40 + 10; draw: rz = draw % 25 + 10.
   2. draw: x = 2·(128zx + rx) + draw % (4·(64 − rx)); draw: y = 2·(128zy + ry) +
      draw % (4·(64 − ry)). (So x lies in [256zx + 2rx, 256zx + 255 − 2rx]: the ±2rx box is
      inside the zone.)
   3. Skip if within 60 blocks of home.
   4. (Object-list test, 80 blocks: never rejects here.)
   5. Skip if q(x, y) > 0.6 (town-ness, terrain §3.1; locations of this zone are visible now).
   6. Skip unless 1 − 50·R(x, y) < 0, i.e. roughness R > 0.02.
   7. m = M(x, y) (mountain mask); zt = top(x, y).
   8. draw: if (draw & 1) ≠ 0 and m < 0.25: **mesa** (§2) at (x, y, zt) with (rx, ry, rz / 2)
      (integer half); otherwise **mound** (§1) at (x, y, zt) with (rx, ry, rz).

   Per formation 5 draws, plus 1 when it passes steps 3–6.

---

## 5. Roads and terraces

### 5.1 Roads (0x5e7321..0x5e7e19)

For every column (x outer, y inner):

1. s = 1 − 50·W(x, y) (W = flatness, terrain §3.2); q = q(x, y). If s < 0 (W > 0.02), next
   column. W is small along the link channels B (regions §5.2) and on the town street grid,
   so these are the **roads** between region cells and the streets.
2. h = H(x, y), and h = 0 if negative. z1 = trunc(h + 1).
3. e = 1 − (1 − s)³; zc = trunc(f32(f32((N(0.02x + 55432, 0.02y + 974) + 1)·4)
   + f32(f32(e·5) + h))) (the clearance height).
4. If read(x, y, z1) has type 0 or 2, or bit 0x40 set, next column (roads are only cut into
   ground that reaches z1).
5. **Road colour**: g = N(0.05x + 843, 0.05y + 984); C = (140 + 40g) for all three channels.
   Cobbles: with a = 234·(x / 2) and b = 234·(y / 2) (integer division towards zero),
   if N(a + 4834, b + 49434) > 0.5 then C ← C + 20·N(a, b + 49) (both noises at integer
   points, i.e. lattice values). If the region's **style** (region header + 0x14) is 1, 4 or 5:
   C = Cl(x, y, z1) instead. Final colour = Cl(x, y, z1)·(1 − q) + C·q per channel, clamped
   to [0, 255]. Write (colour, type **11**) at z1 − 1.
6. If q > 0.92: zc = top(x, y). For z = z1 … zc − 1: if read(x, y, z) has bit 0x40 clear,
   write air (255, 255, 255, 0) there (which becomes water at z ≤ 0, §0).

So outside towns a road is a type-11 strip at the base height H with a few blocks of
headroom cleared; in town centres everything above the road level is removed. Type 11
("town ground") is what the plant pass puts street lights on (vegetation §3.1).

### 5.2 Terraces and pools (0x5e7e2d..0x5e8490)

For every column (x outer, y inner):

1. s = 1 − 50·R(x, y); if s < 0 (R > 0.02), next column. If q(x, y) > 0.95, next column.
2. h = H(x, y), 0 if negative. zb = 5·(trunc(h) / 5) (integer division), the terrace level.
3. t = f32(h − zb) / 5. If t < 0.5: a = 2t. Otherwise a = 1 − 4(t − 0.5), and if a < 0:
   a = (a + 1)² − 1. (a ∈ (−1, 1].)
4. zw = zb if a ≥ 0, else zw = trunc(f32(zb − 5a)) (a little above zb).
   zlow = trunc(f32(f32(zb − 5a) + 2)).
5. **Pool**: for z = zlow … zb (inclusive; empty unless a ≥ 0.4): if read has bit 0x40 clear,
   write (0, 0, trunc(255·(1 − a) + 0·a), type 2) — a water block.
6. **Floor**: if read(x, y, zw) is not of type 2 and has bit 0x40 clear: write
   (Rk(x, y, zw), type **3**); then draw: if draw % 200 = 0, append the **boulder seed**
   (x, y, zw) to list L1 (§6.2). (When the pool reached zb the floor cell is water and is
   skipped, without a draw.)
7. **Clearing**: e = 1 − (1 − s)³; zc = trunc(f32(f32((N(0.02x + 55432, 0.02y + 974) + 1)·2)
   + f32(f32(e·5) + h))); for z = zw + 1 … zc − 1: if read has bit 0x40 clear, write air.

The effect on smooth ground is a staircase of 5-block terraces whose floors are lake-bed
blocks (type 3, which grow reeds, vegetation §3.2), with shallow pools where the terrace
edge rises (t between 0.2 and 0.5).

---

## 6. Dungeons and boulders

### 6.1 Dungeon builder calls (0x5e84c3, 0x5e84f7)

* If E.kind = 3 (a dungeon entrance zone, locations §6.1): 0x5cc510(zone, region,
  256zx + 32, 256zy + 32, E.variant, 0).
* Then, if E.kind = 5: 0x5cc510(zone, region, 256zx + 32, 256zy + 32, 3, 1). No code path of
  region preparation writes kind 5, so this call does not happen in fresh worlds (open
  question 3).

The builder (0x5cc510, about 16 KB) reads E's level and sub-level from the zone
(+0x80, +0x84), chooses a style from its fourth argument (variants 1 and 2 share a style
with one extra draw; 3, 5 and others have their own), lays out a 3 × 3 × 3 grid of rooms with
six draws per room, and then builds corridors, rooms, doors and decorations with many more
draws whose number depends on the layout and on the terrain it meets. It never re-seeds.
**Its draw count is data-dependent and needs its own specification** (follow-up
"alpha-worldgen-dungeons"); until then a zone with E.kind = 3 cannot reproduce the later
draws (boulders, camps, trees, plants).

### 6.2 Terrace boulders (list L1)

For every seed (bx, by, bz) of L1, in the order they were appended (column order of §5.2):

1. draw: ax = draw % 3 + 2; draw: ay = draw % 3 + 2; draw: az = draw % 3 + 2.
2. For X = bx − ax … bx + ax (outer), Y = by − ay … by + ay (inner):
   n = f32(N(0.05X, 0.05Y)·0.8); ux = f32((X − bx)/ax), uy = f32((Y − by)/ay),
   s2 = f32(uy² + ux²); for Z = bz + 2az down to bz − 2az:
   w = f32(f32((Z − bz)/az) + n); if f32(w² + s2) ≤ 1: if read(X, Y, Z) has bit 0x80 clear and
   bit 0x40 clear, write (Rk(X, Y, Z), type 6).

(Water read as (255, 255, 255, 0x82) has bit 0x80 set and is therefore not replaced.)

### 6.3 Ravine boulders (list L2)

For every seed of L2 (§7, case 3), in order: draw: ax = (draw & 3) + 4; draw: ay = (draw & 3)
+ 4; draw: az = draw % 6 + 4; the same shape as §6.2; write only where bit 0x80 is clear
(bit 0x40 is not tested), with type byte **0x26** (rock type 6 plus the surface flag 0x20)
and the Rk colour.

---

## 7. Ravine spawns (completes terrain spec §5.5 step 10)

Reached in the surface pass after a ravine column has been carved and its floor block
written (top > floor), with S, top, floor, P of terrain §5.5. Condition before any draw:
top − floor > 2 and (x + 3y) mod 7 = 0. Then:

1. draw; if draw % 30 ≠ 0, stop.
2. draw: case = draw & 3. Let zf = trunc(floor) and the spawn position
   (x + 0.5, y + 0.5, zf) (fx: (x << 16) + 32768, (y << 16) + 32768, zf << 16).
   * **0, glowing item.** If top > S, stop. draw; if (draw & 3) ≠ 0, stop. Build an item
     record (0x44a970) with category 11, subtype **19** (the glowing collectable,
     alpha-rendering.md §10.1), the byte at +0x0d = 9, the 16-bit count at +0x10 = 1, the
     position above, scale 0.1, and draw: rotation = f32(draw · 2π / 32767) in double
     (radians); append it to the zone's item list (zone + 0x30, 0x44d590).
   * **1, creature.** If top > S, stop. Create a creature record (0x5adb20, 0x10f0 bytes) at
     the position (+0x10, +0x18, +0x20), +0x34 = 1, +0x28 = 6, +0xf58 = 25.0; draw: k =
     draw % 10: k = 0 → kind 131, k = 1 → 133, k = 3 → draw: j = draw % 100: j = 0 → 138,
     1..3 → 137, 4..8 → 136, ≥ 9 → 135; any other k → 132. Kind at +0x2c. Append to the
     zone list at +0x18 (0x66add0).
   * **2, creature.** If top > S, stop. Creature record at the position, +0x34 = a value
     computed from the position by 0x59a9b0 (no draws), +0x28 = 1, kind +0x2c = **59**;
     append to zone + 0x18.
   * **3, boulder seed.** Append (x, y, trunc(floor + 2)) to list L2 (§6.3). No condition on S.

Draws per eligible column: 1; or 2 (cases 2, 3, or cases 0/1 with top > S); case 0: 3, or
4 when the item is made; case 1: 3, or 4 when k = 3. None of the record constructors draw.
The kinds 131..138 and 59 are creature types (creature follow-up).

---

## 8. From 0x5e8b7f to the tree grid (outline; follow-ups)

These passes follow §6.3 and precede the voxel-tree grid (vegetation §5), so their draws
must be reproduced before the tree and plant draws line up. Conditions and draw structure
were read from the assembly; the builders' internal draw counts are not specified yet.

1. **Home building** (0x5e8c2b..0x5e8d80): only if the world is **not named**
   (world + 0xa4 = 0), the zone contains the home point (trunc(home) floor-div 256) and its
   column exists: a prefab building (model entry 2123 of the world's model list
   world + 0x20, if that list is long enough) is placed by 0x5f0ce0 centred on the home
   point on top of the ground. 0x5f0ce0 draws (count: follow-up). Never runs in named worlds.
2. **Camp attempts** (0x5e8d86..0x5e8f8d): only if (zx + zy) is odd. Up to 10 tries; each
   try: draw: py = 256zy + 48 + draw % 160; draw: px = 256zx + 48 + draw % 160 (position
   px + 0.5, py + 0.5). If Lz has type 1 or 5 and the point is inside it (1 − f > 0), the try
   fails. Otherwise the camp routine 0x5ad320 tests and builds a camp (a campfire, static
   kind 65, plus NPCs); it draws internally (at least one draw for the fire's rotation when
   the roughness test passes; full count: follow-up "creatures"). On success a creature is
   created (0x58d860), appended to the object list of §3.1, and the tries stop.
3. **Boulder pass 3** (0x5e8f8d..0x5e9649): only if E.kind ∉ {1, 3, 4}. draw: n = draw % 10,
   plus 10 under the climate bonus of §4.3 step 2. Each boulder: 5 draws: ax = (draw & 7) + 3,
   ay = (draw & 7) + 3, az = (draw & 7) + 3, x = 2·(128zx + ax) + draw % (4(64 − ax)),
   y = 2·(128zy + ay) + draw % (4(64 − ay)); no further draws. Writes (outline): skipped within
   20 blocks of home, within 40 blocks of an object-list entry, or where q > 0.25; otherwise a
   noise ellipsoid of rock (type byte 0x26) on the ground unless the ground type is 11, 8 or 7.
4. **Town builder** (0x5e9655): if Lz has type 1 or 5, 0x5af170(zone, Lz) (62 KB: houses,
   walls, NPCs; draws; follow-up "structures").
5. **Type-10 site** (0x5e967f..0x5e9e7d): if E.kind = 4, static objects placed around the
   zone centre with sines and cosines; draws at 0x5e9783, 0x5e9884, 0x5e989f, 0x5e9dd5
   (follow-up).
6. Voxel-tree grid 0x5e9e7d (vegetation §5).

---

## 9. Corrections to the earlier specs

* Overview §5 table: 0x5e6c56 is a **3 × 3** grid (offsets 42, 127, 212), not 12 × 12, and it
  builds rock **mounds** with half-sizes 20..29 and height 20..35; it skips candidates within
  60 blocks of home.
* Overview §5 and regions §6.2: type 11 builds a 100 × 100 rock mound (height 100..199
  above the ground, cut to the zone), not a cavity.
* Overview §5 "Caves" paragraph and vegetation §8 item 6: 0x5cbe00 and 0x5cb600 are
  formation builders that add rock; houses are not built by them. The "objects already
  registered" list of the tree pass is the object list of §3.1, filled by the camp pass
  and the type-10 site statics (§8 items 2 and 5).
* Overview §5 rows 0x5e72a8..0x5e7dc5 and 0x5e7e2d..0x5e8435: the rock pass (§4.3), roads
  (§5.1) and terraces (§5.2); 0x5a6b60 only appends a node to a list (boulder seeds).
* Terrain spec §5.5 step 10: completed by §7 (item category 11 subtype 19, creature kinds
  131..138 and 59, boulder seeds).
* Regions spec open question 4: the link channels are roads (§5.1).

---

## 10. Test vectors (pure functions, seed-independent)

Computed from the formulas above with the noise kernel of the random spec.

* Mound top: zt(8397000, 8397100) with z = 180, rz = 25 → **211**; the same for
  X = 8397010 → 211.
* Mound inside test at (X, Y, Z) = (8397010, 8397100, 190), centre (8397000, 8397100),
  rx = 24, ry = 22: u = 0.39192742109298706, v = 0.073123499751091, u² + v² =
  0.15895414352416992 (inside).
* Mesa top: zt(8397000, 8397100) with z = 180, rz = 12 → **195**. Mesa fill test at the same
  cell and radii: u = 0.8065943717956543, v = −0.09695065766572952, sum 0.6599938869476318.
* Road colour before the town mix (style not 1/4/5):
  (8397000, 8397100): g = 0.5057623386383057, cobble noise 0.5486481785774231 > 0.5,
  second noise −0.784591555595398, C = 144.53866577148438;
  (8397002, 8397101): g = 0.509972095489502, cobble noise 0.06857702881097794 (no cobble),
  C = 160.3988800048828; (1000, 2000): C = 126.07621002197266.
* Terraces (h → zb, t, a, zlow, zw):
  12.0 → 10, 0.4000000059604645, 0.800000011920929, 8, 10 (pool z = 8..10, floor skipped);
  13.0 → 10, 0.6000000238418579, 0.5999999046325684, 9, 10;
  13.9 → 10, 0.7799999117851257, −0.22559940814971924, 13, 11 (no pool; floor at 11);
  14.6 → 10, 0.9200000762939453, −0.8976001739501953, 16, 14;
  17.2 → 15, 0.44000014662742615, 0.8800002932548523, 12, 15.
* Clearance noise N(0.02x + 55432, 0.02y + 974) at (8397000, 8397100) = 0.11888830363750458.
* Draw counts: a type-6 zone whose nine candidates all fail the first draw makes 9 draws in
  §3.2; a zone with E.kind = 0, climate without bonus and n = 0 makes exactly 1 draw in §4.3.

Seeded test vectors for a whole zone need the dungeon, camp and town builders; the
practical check is the draw-count hook of §11.

---

## 11. Checking against the running game

1. **Draw counts per pass.** Hook MSVCR110 `rand` on the generation thread and log the
   return address of each call during one zone; the call sites listed in §3–§8 (0x5e6ca0,
   0x5e6e67/7e/95, 0x5e70ec, 0x5e71f6, 0x5e72a8, 0x5e7356..0x5e75f6, 0x5e82b4, 0x5e852d..55b,
   0x5e88bf..8f5, and the ravine sites 0x5e6434, 0x5e6449, 0x5e6480, 0x5e6598, 0x5e670c,
   0x5e672c) give the per-pass counts directly; compare with a reimplementation.
2. **Blocks.** Dump a zone with a type-6 or type-11 location and a road crossing right after
   0x5e4850 returns; check the type 6 / 5 / 11 / 3 blocks and their colours against §1–§5.
3. **Visual.** Great rocks (type 11) should show straight cut faces at zone borders (§4.1);
   roads appear as grey cobbled strips sunk into the ground between regions; flat meadows
   show 5-block terraces with small pools.

---

## 12. Open questions

1. **Dungeon builder** 0x5cc510: draw count and blocks (follow-up spec). Blocks every
   later draw in kind-3 zones.
2. **Camp routine** 0x5ad320, **town builder** 0x5af170, **home building** 0x5f0ce0 and the
   type-10 statics (§8): draw counts and placements (follow-ups "structures",
   "creatures").
3. **Kind 5.** The zone generator tests E.kind = 5 twice (dungeon, §6.1) but nothing in
   region preparation writes it; search the server and the save loader for writers.
4. **Creature fields.** The meaning of creature +0x28 (6 or 1), +0x34 (1 or the 0x59a9b0
   value) and +0xf58 = 25.0, and of item byte +0x0d = 9, belongs to the creature and item
   specs.
5. **Water colour bytes.** Pool water stores (0, 0, 255·(1 − a)); the mesher ignores water
   colour (terrain §6.8), so this only matters for byte-exact block dumps.
