# World generator: terrain (part A)

Behavioural specification of Steam Cube World's terrain generator, written from the decompiled
executable, its assembly and the binary's constants. Routines are cited by RVA (image base
0x140000000). Part of a three-way split:

| Part | File | Covers |
|---|---|---|
| A (this) | `worldgen-terrain*.md` | building blocks, regions, places and their kinds, the height of a point, the blocks of a zone's columns, the zone seed and layout |
| B | `worldgen-structures.md` | the 16-block cells and everything built in them (trees, rocks, buildings, castles), static objects, models, plants, items |
| C | `worldgen-creatures.md` | spawn points, creatures, region roles (0x115df0) |

This part is split into four files:

| File | Contents |
|---|---|
| `worldgen-terrain.md` (this) | conventions, building blocks, regions, places, oracles, open questions |
| `worldgen-terrain-height.md` | the height of a point (0x35ea0) and every weight it uses |
| `worldgen-terrain-zone.md` | world values and the zone seed, zone layout, the column pass of the zone generator 0x2ff6f0, the block colour functions, the block store, the light pass |
| `worldgen-terrain-tables.md` | the noise permutation and gradient tables, the biome and place-kind tables, all noise call parameters |

Units: x and y are world block coordinates (signed 32-bit). "Dots" are 1/65536 of a block (signed
64-bit). z is a block height (signed 32-bit); heights are floats.

---

## 0. Conventions that matter for bit-exact results

* **Float arithmetic.** Unless a step says "in double", every operation is IEEE single precision
  (SSE, round to nearest, no fused multiply-add, no extended precision), rounded to float after each
  operation. The order of operations written is the game's order; keep it (float addition and
  multiplication are not associative). Where a formula is written as `a × b × c` it means
  `(a × b) × c`; sums likewise go left to right. Parentheses override.
* **Double arithmetic.** Noise inputs are always built in double: an integer coordinate is converted
  exactly to double, multiplied by a double scale, then a double offset is added
  (`X × scale + offset`, two roundings). The noise returns a float.
* **Float constants** written with many digits (0.30000001192092896, 0.4000000059604645 …) are the
  float nearest to the short decimal (0.3, 0.4 …); the long form is given wherever the value is used in
  double arithmetic or where confusion is possible. Where a float constant is used in double
  arithmetic it is widened exactly.
* **min and max.** `min(a, b)` means "a if a < b, else b"; `max(a, b)` means "a if a > b, else b".
  The first operand is returned only when the strict comparison holds (so the second is returned on a
  tie or when either is NaN). This is the SSE `minss`/`maxss` rule, and the game's comparisons were
  written to match it. `clamp01(v)` means `min(1, max(0, v))` evaluated in that order unless a step
  says otherwise.
* **smooth(s)** means `s × 3 × s − (s + s) × s × s`, evaluated as `((s × 3) × s) − (((s + s) × s) × s)`.
  It is used in many places; always this exact expression.
* **Truncation and floor.** `trunc(v)` is conversion toward zero (cvttss2si / cvttsd2si). 32-bit
  unless "trunc64" is written. `floorf(v)` for a float v (the game's own floor, used for all heights):
  i = trunc(v); if v < 0 and float(i) ≠ v then i − 1, else i. The result is an integer (or that
  integer as a float where the step says "as a float").
* **Integer floor division.** `fdiv(a, 2^k)` for signed 32-bit a is the mathematical floor of a / 2^k.
  The game computes it as: if a ≥ 0, a / 2^k (truncating); else ((a + 1) / 2^k) − 1 with truncating
  division. These two agree with the true floor for all 32-bit values. Used with 2 (halving),
  16, 1024 and 8192.
* **Integer arithmetic** on 32-bit values wraps modulo 2^32 (hash inputs like `y × 57 + x + salt`).
  C division and remainder of signed integers truncate toward zero; of unsigned integers are the usual
  unsigned ones. Each use below says which.
* **Comparisons** are written as the game tests them. Any ordered comparison with NaN is false.

---

## 1. Building blocks

### 1.1 The integer hash H(n)

Input: a 32-bit integer n (any bit pattern). All arithmetic unsigned 32-bit, wrapping.

1. v = n XOR (n shifted left by 13).
2. t = v × v; t = t × 60493 + 19990303; t = t × v; t = t + 1376312589.
   (The binary adds 0xd208dd0d, which differs from 1376312589 = 0x5208dd0d only in bit 31.)
3. H = t AND 0x7fffffff. Range 0 … 2^31 − 1.

### 1.2 The 0..100000 random R(n)

R(n) = 100000 − (H(n) / 21474), unsigned division (the binary uses a multiply-and-shift by
0x61a8f94d, >> 45, which is exact for every H).

Because H / 21474 reaches 100003, **R can be −1, −2 or −3** (as a signed value) for the top 0.003 % of
hashes. Each use below says whether R is taken as an unsigned 32-bit value (then those three are
4294967295, 4294967294, 4294967293) or as a signed one. This matters for bit-exactness.

Many hashes use the cell key **K(x, y) = y × 57 + x** (32-bit wrapping) plus a salt.

### 1.3 Parity of a hash ("odd hash")

Several places test whether **H(n) / 21474** (unsigned, truncating) is odd. The binary computes it
through a signed multiply by the negative magic number 0x9e5706b3 (the quotient comes out negated);
only its lowest bit is used, so the test is simply "H(n) / 21474 is odd". Since 100000 is even this is
the same as "R(n) is odd".

### 1.4 Simplex noise N(x, y) (0x2e0c0)

The game's 2D simplex noise: Gustavson's public-domain formulation with Ken Perlin's permutation.
Input two doubles, output a float. All internal arithmetic is double.

Tables (static initialisers 0x1280, 0x12e0 and 0x2b7690; values in `worldgen-terrain-tables.md` §1):
* F2 = (sqrt(3.0) − 1.0) × 0.5 (computed at start-up in double: 0.3660254037844386).
* G2 = (3.0 − sqrt(3.0)) / 6.0 (0.21132486540518713).
* P[0..511]: P[i] = PERM[i AND 255] (the 256-byte permutation at 0x470ed0, repeated).
* M[0..511]: M[i] = P[i] mod 12.
* Gradients g[0..11] (x, y) from the 3-component table at 0x530a40 (the third component is unused).

Steps:
1. x = x_in × 0.32; y = y_in × 0.32 (0.32 is a double constant).
2. s = (x + y) × F2.
3. i = trunc32(s + x); if float64(i) > s + x then i = i − 1. Likewise j from s + y.
   (Compare with the double sum, using the 32-bit integer.)
4. t = float64(i + j) × G2 (i + j as a 32-bit integer sum).
5. x0 = x − (float64(i) − t); y0 = y − (float64(j) − t).
6. If x0 > y0: (i1, j1) = (1, 0); else (0, 1).
7. x1 = (x0 − float64(i1)) + G2; y1 = (y0 − float64(j1)) + G2.
   x2 = (x0 − 1.0) + G2 × 2.0; y2 = (y0 − 1.0) + G2 × 2.0.
8. ii = i AND 255; jj = j AND 255.
   Gradient indices: c0 = M[P[jj] + ii]; c1 = M[P[jj + j1] + ii + i1]; c2 = M[P[jj + 1] + ii + 1].
9. For each corner k with (xk, yk) and gradient (gx, gy):
   a = (0.5 − xk × xk) − yk × yk. If 0 > a, its term is 0. Else a = a × a; a = a × a;
   term = ((yk × gy) + (xk × gx)) × a.
10. Result = float32(((term1 + term0) + term2) × 70.0). Note the order: corner 1 plus corner 0,
    then corner 2.

Throughout the specs, "N(a·X + b, c·Y + d)" means the noise called with
x_in = float64(X) × a + b and y_in = float64(Y) × c + d, each a double multiply then a double add.

### 1.5 The C runtime's rand()

The game uses the Microsoft CRT's rand()/srand() (import slots 0x42cd28 and 0x42cd38):
state = state × 214013 + 2531011 (unsigned 32-bit, wrapping); the value is (state >> 16) AND 0x7fff.
srand(s) sets state = s. The terrain part draws rand() only in two places: the world setup (§ zone file
1) and the cliff overhangs (zone file § 5.6).

---

## 2. Regions (8192-block cells)

### 2.1 A region's centre (0x2b9210)

For region (rx, ry), with k = K(rx, ry):
* cx = rx × 8192 + 1024 + (R(k + 0xe29e) mod 6144)
* cy = ry × 8192 + 1024 + (R(k + 0x70532) mod 6144)

R taken unsigned; mod unsigned; additions 32-bit wrapping. (0x2ad4d0 inlines the same, writing the
second salt as the first plus 0x62294.)

### 2.2 A region's biome (0x2b9080)

If (rx + 1) and (ry + 1), each taken as unsigned 32-bit, are both < 3 (the 3 × 3 regions around the
origin), the biome is 2. Otherwise v = R(K(rx, ry) + 0xb3f58) mod 50 (unsigned) and the biome is from
the table in `worldgen-terrain-tables.md` §2 (0 → 1 and 32..49 → 1, …). Biomes are 0..13.

Biome numbers that the terrain treats specially: 1 ocean, 4, 9, 10, 13 warm/dry, 5 and 12 forest,
6 and 11 cold, 7 lake, 8 (wide rivers, wet), 13 lava. Their names belong to the game's data.

### 2.3 The warp

A point (x, y) is moved by noise before regions are looked up. Both 0x358e0 and 0x2ad4d0 do it
identically:

1. a = N(0.0003·X + 8432, 0.0006·Y + 9438); a = a × 0.125 × 8192 (float, left to right).
2. b = N(0.0006·X + 567, 0.0003·Y + 1223); b = b × 0.125 × 8192.
3. px = trunc64(a × 65536) + (int64(x) × 65536); py = trunc64(b × 65536) + (int64(y) × 65536). (dots)

Then the warped region index: bx = floor(px / 65536) (exact floor of the 64-bit value, as a 32-bit
integer), rx = fdiv(bx, 8192); likewise ry.

The squared distance from the warped point to a region centre (cx, cy):
dx = float32(px − int64(cx) × 65536) × (1/65536); dy likewise (the int64 difference converted to float,
then multiplied by the float 1.52587890625e-05); d2 = dy × dy + dx × dx.

### 2.4 The region of a point, before landmarks (0x2ad4d0)

Warp (x, y). Over i = rx − 1 … rx + 1 (outer) and j = ry − 1 … ry + 1 (inner), take the region whose
d2 is smallest; the first one found wins ties (a later one replaces it only if strictly smaller). The
result is that region's (i, j).

### 2.5 The region neighbourhood (0x358e0)

Fills a 0x58-byte record used everywhere below:

| Offset | Type | Contents |
|---|---|---|
| +0x00 | i32[3][3] | biome of region (rx − 1 + a, ry − 1 + b) at index a × 3 + b |
| +0x24 | f32[3][3] | its d2, same indexing |
| +0x48, +0x4c | i32 | the point's x, y (unwarped) |
| +0x50 | f32 | the nearest d2 (starts at −1.0) |
| +0x54 | i32 | the nearest region's biome |

The loop order is a outer, b inner; the nearest is replaced when the stored nearest < 0 or the new
d2 < stored. "Nearest" below always means +0x50.

### 2.6 The region of a point (0x2ada20)

1. (cx, cy) = (fdiv(x, 1024), fdiv(y, 1024)); big = bigCell(cx, cy) (§3.1).
2. kind = placeKind(cx, cy, big) (§3.4). If kind ≠ 0:
   p = cellPoint(cx, cy, big) (§3.2); d = warpedDist2(p, x, y) (§3.3); r = 500 if big else 250;
   if d < float(r × r) and 0.25 ≤ (1 − d / float(r × r))², the result is the region of p
   (0x2ad4d0 at p).
3. Otherwise the region of (x, y) (0x2ad4d0).

The cube world map and the zone header's "region type" are the biome of this region.

**CubeVulkan's WorldScale** plugs in at the start of 0x2ad4d0 and 0x358e0 (the point is divided
before the warp, and distances are given back in full-size blocks). At scale 1 it is the game; it is
not specified here.

---

## 3. Places (1024-block cells) and their landmark kinds

A place cell (cx, cy) = (fdiv(x, 1024), fdiv(y, 1024)) may hold a landmark. Some cells are "big":
then the 2 × 2 cells sharing even corner (2 × fdiv(cx, 2), 2 × fdiv(cy, 2)) form one place, and every
hash below uses that even corner.

### 3.1 Big cells (0x36160, with 0x36210)

* hx = fdiv(cx, 2), hy = fdiv(cy, 2).
* Sevenths test (0x36210): v = hx + hy × 5; it passes when ((v rem 7) + 7) rem 7 = 1 (C signed rem).
* The cell is big when H(hy × 57 + 0x39a13d + hx) / 21474 is even (§1.3), or the sevenths test passes.

All four cells of a 2 × 2 block give the same answer.

### 3.2 The landmark point (0x362a0)

* Cell (0, 0) (exactly, before any evening): (512, 512).
* Not big: k = K(cx, cy); x = cx × 1024 + 282 + (R(k + 0x7ab2c) mod 460), y = cy × 1024 + 282 +
  (R(k + 0x271537) mod 460). R unsigned.
* Big: ex = 2 × fdiv(cx, 2), ey = 2 × fdiv(cy, 2), k = K(ex, ey); x = ex × 1024 + 532 +
  (R(k + 0x7ab2c) mod 984), y = ey × 1024 + 532 + (R(k + 0x271537) mod 984).

Callers pass the big flag; it does not have to be the cell's own (see §3.4, where some kinds look at
the point a cell would have if it were not big).

### 3.3 The warped distance to a landmark (0x364e0)

For landmark point (lx, ly) and point (x, y):
1. a = N(0.005·X + 18843, 0.01·Y + 9432) × 32.
2. b = N(0.01·X + 299, 0.005·Y + 90650) × 32.
3. dx = float32((int64(x) − lx) × 65536 + trunc64(a × 65536)) × (1/65536);
   dy = float32((int64(y) − ly) × 65536 + trunc64(b × 65536)) × (1/65536).
   (a and b already include the × 32; the products with 65536 are float.)
4. d2 = dy × dy + dx × dx.

The spiral ramp (height §8.5) uses the same two offsets through 0x2baf90.

### 3.4 The landmark kind of a cell (0x2b99d0)

Input (cx, cy, big). Returns 0 (no landmark) or a kind 1..47. "h(b)" below is the plain base height
(height file §2, 0x2ba260: no place) at the landmark point cellPoint(cx, cy, b); "biome(b)" is the
biome of the region 0x2ad4d0 gives for that point. The big flag b used is stated each time: "own" is
bigCell(cx, cy), "false" is the small-cell point, "true" the big one.

* Cell (0, 0): kind 1.
* **Not big** (the flag passed is false):
  1. r = R(K(cx, cy) + 0x2acfe1) as a **signed** 32-bit value.
  2. If r rem 4 = 0 (C signed remainder): 0.
  3. q = r / 4 (C signed, truncating; for r = −1..−3 this is 0).
  4. If q is even (bit 0 clear) and h(own) < 10: 0.
  5. m = (q / 2) rem 24 (C signed). The kind follows table §3 of the tables file, where some entries
     test heights and biomes.
* **Big**: (cx, cy) are first evened. If the sevenths test passes for them: kind 5. Otherwise
  m = (R(K(cx, cy) + 0x2acfe1) as signed) rem 19, and the kind follows table §4 of the tables file.

The "shore" rule used by several entries: when the tested height h < 0, the kind is 36 if h > −10,
else the entry's fallback (23 or 0).

**Mod hook (CubeVulkan only):** a mod may replace the returned kind; it plugs in after this routine.

### 3.5 The place variant (0x2abb90)

Input (cx, cy, kind, big); for big, (cx, cy) are evened. k = K(cx, cy).
* Kinds 2, 5, 9, 12, 15, 21, 43, 45, 46: variant 4 if R(k + 0x51c64b) (unsigned) is divisible by 10,
  else 0.
* Kinds 10, 13, 14, 17, 18, 19, 23, 25, 28, 30, 31, 33, 36, 42, 44, 47: 0.
* Every other kind: v = R(k + 0x12ce2f3) mod 10 (unsigned): 0 → 4, 1 → 1, 2 → 5, 3 → 2, else 0.

Meaning for the terrain: 1 raised plateau, 2 spiral ramp up, 3 spiral pit (never produced by this
routine, but handled by the height code), 4 water or lava ring and "now and then" variants for (B),
5 walled pit.

### 3.6 The place around a point (0x35ce0)

Fills a 0x20-byte record:

| Offset | Type | Contents |
|---|---|---|
| +0x00 | i32 | kind |
| +0x04 | u8 | big |
| +0x08 | i32 | variant |
| +0x0c | i32 | radius (500 big, 250 not) — written only when kind ≠ 0 |
| +0x10, +0x14 | i32 | landmark point (always written) |
| +0x18 | f32 | warped distance² (0 when kind is 0) |
| +0x1c | f32 | weight (0 when kind is 0) |

Steps: (cx, cy) = the point's cell; big = bigCell; point = cellPoint(cx, cy, big).
1. Reach gate: with reach = 532 if big else 282 and integer dx = x − px, dy = y − py (32-bit), if
   reach² < dx² + dy² (32-bit integer products): kind 0, variant 0, distance and weight 0. Done.
2. kind = placeKind(cx, cy, big) — note: the flag passed is big, but 0x2b99d0 is called with the
   cell's own (cx, cy). If 0: as above.
3. radius = 500 if big else 250. d = warpedDist2(point, x, y). If d < float(radius²):
   weight = (1 − d / float(radius²))², computed as f = 1 − d / R2, then f × f. Else weight = 0.
4. variant = 0x2abb90(cx, cy, kind, big).

The zone generator does not use this reach-gated version for its own columns: it uses 0x31a5d0 and
0x31a690 (zone file §3.2).

### 3.7 A place's weight from a distance (0x9bee0)

For (kind, big, d2): 0 if kind is 0; else with r = 500 or 250, (1 − d2 / float(r²))² if
d2 < float(r²), else 0. Used by (B) and (C).

### 3.8 What part B and part C take from here

| They need | From | One line |
|---|---|---|
| Is a cell big, its landmark point, kind, variant | §3.1–3.5 | (cx, cy[, big]) → bool / point / kind / variant |
| A point's place record | §3.6 | (x, y) → 0x20-byte record |
| The region of a point and its biome | §2.4, 2.6, 2.2 | (x, y) → (rx, ry); (rx, ry) → biome |
| The height of a point | height file §1 | (x, y, mode) → float |
| The plain base height of a landmark | height file §2 | (x, y) → float |
| A column's buried / surface colour from scratch | zone file §4.8 (0x2f39e0, 0xbbfe0) | (x, y, z) → block properties |
| The block store and lookup | zone file §5.2, 5.3 | (x, y, z[, block]) |
| The zone's column field | zone file §3.3 | 13 floats per column |

---

## 4. The oracles

All come from the owner's game through the ZoneExtractor mod (`tools/ZoneExtractor`), never
from code, and are compared by `tests/EngineTests.cpp`.

### 4.1 `Extract/gen-oracle.txt` (test `genMatchesGame`, command `EngineTests gen [field]`)

Written by key O in a world (namespace `genoracle`). About 3 800 points: a 21 × 21 grid around the
player (steps 37, 41), a 21 × 21 grid around the origin (steps 613, 587), a grid at the far
seed-123 extraction (−94 042 880, −93 755 584) and 2 500 pseudo-random points within ±2 000 000 blocks
of it and of the origin. One line per point; '|' is a separator:

`x y | regionX regionY | kind big variant radius cx cy dist2 weight | lake base roads w13 w125 w8 river lava swamp high a44 b59 veg | height0 height4`

| Field | Game routine | This spec |
|---|---|---|
| regionX, regionY | 0x2ada20 | §2.6 |
| kind … weight | 0x35ce0 (record zeroed first, so radius is 0 when kind is 0) | §3.6 |
| lake | 0x2bb7e0 | height §3.1 |
| base | 0x2a0840 | height §2 |
| roads | 0x2d4730 | height §4 |
| w13 | 0x2b9750 | height §3.2 |
| w125 | 0x2d9f30 | height §3.3 |
| w8 | 0x2e64e0 | height §3.1 |
| river, lava, swamp | 0x2e5f80, 0x2b92f0, 0x2d01a0 | height §5 |
| high | 0x2bb310 | height §6 |
| a44 | 0x2a44a0 | height §3.1 |
| b59 | 0x2b5960 | height §3.4 |
| veg | 0x2d36f0 | height §7 |
| height0, height4 | 0x35ea0 modes 0 and 4 | height §1, §8 |

Floats are printed with %.9g; each must equal the reimplementation's float exactly.

### 4.2 `Extract/gen-blocks.txt` (test `genBlocksMatchGame`, command `EngineTests blocks [field]`)

Same key. Header line: `# world+0x330 A world+0x334 B world+0x244 Z seed S` (the two sand-noise
offsets, the zone seed and the world seed; the test checks the first two against §1 of the zone file).
Then, for the points of gen-oracle plus a 32 × 32 square around the player:

`x y | h hx- hx+ hy- hy+ | w9b wbb | snow snowR snowG snowB | bedR bedG bedB top | ` then for depths
0, 1, 3, 10, 40 below floorf(h): `r g b c type e` (six numbers each).

| Field | Routine |
|---|---|
| h, hx−, hx+, hy−, hy+ | 0x35ea0 mode 0 at (x, y), (x∓1, y), (x, y∓1) |
| w9b | warmth 0x2d9b60 (zone §4.1) |
| wbb | kind-27 weight 0x2abb60 (zone §4.1) |
| snow | 0x2b4320 (zone §4.7) as 0/1 |
| snowR/G/B | 0x2d3d90: always 200, 280, 400 |
| bedR/G/B | 0xbd110 at floorf(h) − 1 (zone §4.6) |
| top | 0x2e6420 water level (zone §4.5) |
| r g b c type e | 0x2d8e00 (zone §4.2) at z = floorf(h) − depth, with the column's own values from 0x35ce0 and the "unused tint" input 0 |

The test only compares fields after the five heights.

### 4.3 Zone files (tests `genZonesMatchExtracted`, `EngineTests zones [n] [kind]`, `zonediff x y`)

`.cwz`, little endian: `"CWZ1"`, i32 zone x, zone y, region type (the biome of 0x2ada20 at the zone's
middle (x × 64 + 32, y × 64 + 32)), reserved; then 4096 columns in the game's order
(index = local x × 64 + local y): i32 base z, u32 run count, runs of {u16 count, 6 block bytes:
red, green, blue, glow, type, flags}. A column's runs start at its base z.

Folders:
* `Extract/seed_123/`: zones saved while playing (lit; only those within 100 000 zones of the origin
  are vanilla). The test requires every zone's region type to match and ≥ 99.9 % of plain columns
  (no structure, not inside a place) exact, ignoring light-only differences of air and water.
* `Extract/gen_seed_123/` (125 zones; select with `CW_ZONES_DIR`): zones the game generated afresh for
  `Extract/gen.request` exactly as 0x2ae0c0 does (srand, constructor 0x2f9400, generator 0x2ff6f0),
  kept out of the world. **These are unlit** (the light pass, zone §6, runs later in the game), so
  their air and water red bytes are as the generator stored them. The world seed is the number after
  the folder name's last underscore. Beside each `.cwz`: `.rec` (records, part B), `.spawns`
  (part C) and `.rtrace`.

### 4.4 `.rtrace` (command `EngineTests rtrace x y`)

Every rand() call made while generating the zone, in order: 6 bytes each, u32 caller RVA and u16 value.
For this part only the overhang draws (zone §5.6) occur before part B's cell loop; the first draws of a
zone therefore check §5.6 and the zone seed. (`EngineTests height x y` prints 0x35ea0 in modes
0, 1, 2, 4, 8, 9.)

---

## 5. Open questions

1. **R(n) negative.** CubeVulkan's current code computes the small-cell landmark's q as r >> 2
   (floor), the game as r / 4 (truncate). They differ only when R(…) is −1, −2 or −3 (about 1 cell in
   34 600): the game then always tests the height (q = 0 is even); the current code skips the test.
   No oracle point is known to hit this; the spec follows the binary (0x2b9eaf).
2. **0x2ff6f0's five empty lists.** The column pass has a block (0x30068e–0x301a0c) that walks five
   lists on the generator's stack and writes into a sixth (inserted at 0x301885, read after the
   column loop at 0x3022c3). Nothing ever adds to the five, so the block and the later pass over the
   sixth never run. Specified as "never runs" (zone §5.7); if some build fills them, it is not this one.
3. **sqrtf, acosf.** The spiral (height §8.5) uses the CRT's acosf and sqrtf; sqrtf is correctly
   rounded, acosf is the UCRT's. A reimplementation must use a bit-identical acosf (the
   current code relies on the platform's matching).
4. **The light pass** (zone §6) is summarised from 0x2a3260 and CubeVulkan's verified port; its byte
   layout of the light map (0xcb9c0) is not specified here, only its result.
5. **Static tables at run time.** F2, G2, P and M are filled by static initialisers; the binary's file
   bytes at those addresses are zero or unrelated. The values given are what the initialisers compute.
