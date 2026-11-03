# Alpha world generation: region cells, climate fields and locations

Behavioural specification of the large-scale layer of the Cube World **Alpha** world: the
1024 × 1024 grid of **region cells** (one jittered control point per 16384-block cell
carrying moisture, warmth, elevation and a fire flag), the noise-displaced nearest-cell and
distance-weighted blends that turn those points into smooth climate fields, the "link
channels" between neighbouring cells, and the 8 × 8 grid of **location records** inside
each region that the terrain consults. Written from `Cube.exe` (SHA-256 `84a7a132…30bf`).
Read the [random spec](alpha-worldgen-random.md) first for N(x, y), A and the offsets.

This document resolves **open question 1 of [alpha-rendering.md](alpha-rendering.md)**
(field meanings in §2.1, exact blends in §4).

---

## 0. Conventions

As in the random spec (§0 there): f32, trunc, floor_div, float/double precision, smooth.
Positions:

* **Block coordinates** are integers; the world spans 0 ≤ x, y < 2²⁴ (16777216) blocks.
* **Fixed point** positions are 64-bit integers in 1/65536 block ("fx" below); block → fx
  is a 64-bit shift left by 16.
* **Region index** (rx, ry) = (floor_div(x, 16384), floor_div(y, 16384)), 0..1023.
* **Location sub-cell index** (lx, ly) = (floor_div(x, 2048), floor_div(y, 2048)),
  0..8191; it lies in region (lx >> 3, ly >> 3) at slot (lx mod 8, ly mod 8).
* "Cell" in this document always means region cell; "location" means location record.

---

## 1. Grids in the world object

| Grid | Where (W = world) | Size | Entry |
|---|---|---|---|
| Region objects | W+0xbc | 1024 × 1024 pointers, index rx·1024 + ry | region (64 × 64 zones, §6) |
| Region cells | W+0x4000bc | 1024 × 1024 pointers, index rx·1024 + ry | cell record (§2) |

A missing pointer is null. Both are cleared at world initialisation.

---

## 2. Region cells

### 2.1 Record (0x1c bytes)

| Offset | Type | Meaning (name used in these specs) |
|---|---|---|
| +0x00 | int32 | point x, blocks |
| +0x04 | int32 | point y, blocks |
| +0x08 | byte | **fire flag** (1 = fire region) |
| +0x0c | float | **moisture** m, 0..1 |
| +0x10 | float | **warmth** w, 0..1 |
| +0x14 | int32 | cell id = 1024·ry + rx + A |
| +0x18 | int32 | **elevation** e; negative = sea cell (−100..−1), positive = land |

These are exactly the fields the palette uses (alpha-rendering.md §3.1): +0x0c is
moisture, +0x10 warmth, +0x18 < 0 marks sea, byte +0x08 = 1 marks fire. The names there
are confirmed by how the generator creates and uses them (§2.2, §4, terrain spec).

### 2.2 Creation (0x5d7a70; server counterpart 0x50b870, same size)

A cell is created the first time it is requested (only by region preparation, §6.1), and
never changes afterwards. Out-of-range indices give no cell. Steps, with every draw in
order:

1. `srand(rx + 0x108a + 1024·ry + 3A)`; id = 1024·ry + rx + A; fire = 0.
2. **home** = (rx, ry) is the region of the home point: rx = (trunc(8396928.0) >> 8) >> 6
   and likewise for y (with the code's floor rounding); for the fixed home point this is
   region (512, 512).
3. Draw r1.
4. **Temperate branch** if (r1 & 1) = 0 **or** home:
   * m = f32(f32(f32(draw · 0.4f) / 32767f) + 0.3f) (float), range 0.3..0.7;
   * w = f32(draw · 0.4 / 32767.0 + 0.4000000059604645) (double, then rounded), 0.4..0.8;
   * if home: point = (trunc(8396928.0), trunc(8396928.0)) and go to step 6;
   * draw r2; if r2 mod 10 = 0 (**fire region**): fire = 1;
     w = f32(f32(f32(draw · 0.2f) / 32767f) + 0.8f) (0.8..1.0);
     m = f32(f32(draw · 0.5f) / 32767f) (0..0.5).
5. **Extreme branch** otherwise (r1 odd, not home):
   * draw r3; if r3 odd: m = f32(f32(f32(draw · 0.1f) / 32767f) + 0.9f), else
     m = f32(f32(draw · 0.1f) / 32767f);
   * draw r4; if r4 odd: w = f32(f32(f32(draw · 0.1f) / 32767f) + 0.9f), else
     w = f32(f32(draw · 0.1f) / 32767f).
   So extreme cells are dry or wet, and cold or hot, at the ends of the range.
6. **Point** (not for home): py = 16384·ry + (draw mod 15360) + 512, then
   px = 16384·rx + 512 + (draw mod 15360) — the y draw comes **first**.
7. **Snap**: x ← floor(x / 2048)·2048 + 1024, y likewise (code: add 2047 if negative, clear
   the low 11 bits, add 1024). Every cell point is therefore the centre of one of its
   region's 8 × 8 location sub-cells.
8. **Elevation**: with Ad = (double)A,
   n1 = N(1.4·rx + Ad, 1.4·ry + Ad + 843), f = f32(f32(f32(n1 + 1) · 100) − 70),
   n2 = N(4·rx + Ad, 4·ry + Ad + 843), e = trunc(f32(f + f32(n2 · 30)))
   (the products 1.4·rx and 4·rx are double; 843 is a double constant).
9. If e ≤ 0 (**sea**):
   * fire = 0;
   * if home: e = (draw mod 50) + 20 (the home region is always land) and stop;
   * e = max(e − 100, −100);
   * draw r5; if r5 odd: m and w are re-drawn as in the temperate branch (m float 0.3..0.7,
     then w double 0.4..0.8); if even: m = 0.9 + …, w = 0.9 + … (float, as in step 5's
     "odd" forms; m first).
10. Store the cell.

Note that the elevation noise depends only on (rx, ry) and A, not on the draws, so whether a
cell is sea is known before its draws are interpreted.

### 2.3 Test vectors (S = 123, so A = 440)

| Cell | x, y | m | w | e | fire |
|---|---|---|---|---|---|
| (512, 512) home | 8397824, 8397824 | 0.4915097653865814 | 0.5601367354393005 | 74 | 0 |
| (513, 512) | 8408064, 8395776 | 0.5406934022903442 | 0.7547471523284912 | −100 | 0 |
| (512, 513) | 8403968, 8412160 | 0.9370158910751343 | 0.9595354795455933 | −100 | 0 |
| (511, 511) | 8377344, 8381440 | 0.4052278399467468 | 0.6399243474006653 | 56 | 0 |
| (510, 512) | 8371200, 8397824 | 0.9375194311141968 | 0.9097689390182495 | −100 | 0 |
| (514, 514) | 8426496, 8428544 | 0.664085865020752 | 0.40054935216903687 | 45 | 0 |
| (509, 509) | 8342528, 8354816 | 0.9998779296875 | 0.09554429352283478 | 58 | 0 |

(The home point 8396928 snaps to 8397824.)

---

## 3. Displacement and the owning cell

### 3.1 Sample displacement D(x, y) (0x5eefa0)

For a block column (x, y):

* dx = f32(f32(N(y·0.0005, 3423) · 3) · 256)
* dy = f32(f32(N(x·0.0005, 23421) · 3) · 256)

(x·0.0005 in double; 3423 and 23421 are double constants; the x shift is driven by y and
the y shift by x.) |dx|, |dy| < 768 blocks. Test: D(8396928, 8396928) =
(402.813232421875, 92.43751525878906).

### 3.2 Search window

The candidate cells for column (x, y) are all (i, j) with
floor_div(x − 16384, 16384) ≤ i ≤ floor_div(x + 16384, 16384) and the same for j with y:
the 3 × 3 cells around the column's own region cell. The window uses the **undisplaced**
column; only distances use the displaced point.

### 3.3 Integer squared distance d²

The displaced point is p = (trunc(f32((float)x + dx)), trunc(f32((float)y + dy))) — the sum
is made in float, so above 2²³ blocks it is rounded to whole blocks before truncation.
For a cell c: ex = f32(f32((float)((c.x << 16) − (p.x << 16))) · 2⁻¹⁶) (64-bit difference,
converted to float), likewise ey; **d²(c) = trunc(f32(f32(ey²) + f32(ex²)))**.

### 3.4 Owner (0x477e10; index form 0x5cade0)

The owner of (x, y) is the candidate with the smallest d²; on a tie the first one found
wins, scanning i (x) in the outer loop and j (y) in the inner loop, both ascending.
Missing or out-of-range cells are skipped. 0x5cade0 returns the owner's (i, j) (or
(−1, −1)); 0x477e10 returns the cell. The owner is the column's **region-cell biome**:
its elevation sign decides sea or land in several rules.

(0x477e10 computes the same d² through 0x5eeee0 with x87 conversion; the result is the same
float truncated to an integer.)

---

## 4. The climate blends (resolves alpha-rendering.md open question 1)

All four take a block column (x, y), use the window and d² of §3, and run two passes:

1. **Nearest pass.** If any of the 9 candidate cells is missing or out of range, return the
   **default** at once. Otherwise d0² = the smallest d².
2. **Blend pass.** For each candidate in the same order: t = f32((float)(d²(c) − d0²) ·
   4.999999987e-07) (integer difference converted to float, times the float constant at
   **0x71dcdc** = 5·10⁻⁷), t = min(t, 1), weight wc = 1 − t (float). Accumulate
   Σw += wc and Σv += value(c)·wc in float, in scan order.
3. If Σw > 0 the result is f32(Σv / Σw), else the default.

Weights fall from 1 at the nearest cell to 0 at d² − d0² ≥ 2·10⁶, so a blend is only
mixed within roughly 100 blocks either side of a cell boundary (the boundary itself is
wobbled by the displacement of §3.1).

| Field | Routine | value(c) | Default | Extra rule |
|---|---|---|---|---|
| **Warmth** | 0x5c4dd0 | c.w (+0x10) | 0.5 | — |
| **Moisture** | 0x5c4800 | c.m (+0x0c) | 0.5 | oasis rule below |
| **Sea share** | 0x5f0720 | 1 if c.e < 0 else 0 | 0 | pre-scan |
| **Fire share** | 0x5ef040 | 1 if c.fire = 1 else 0 | 0 | pre-scan |

* **Pre-scan** (sea and fire shares): before the nearest pass, scan the window; if a cell is
  missing return 0; if no cell has the property return 0 without blending. (When a cell
  with the property is found, the rest of that row is not checked for missing cells; the
  blend pass then returns 0 if one is missing, so the result is the same.)
* **Oasis rule** (moisture only, after a successful blend with result m < 0.2): look up the
  location of the column (§6.3); if it exists and its type is 3, let f = the location's
  normalised distance at (x, y) (§7, using x << 16, y << 16), v = 1 − f, b = v² if v > 0 else
  0; m ← min(m + 0.3·b, 1).

Inside a zone the generator stores moisture, warmth and fire share per column (column
+0x04, +0x08, +0x0c, see the terrain spec) and later formulas read the stored value when
the column exists, otherwise they call the routine; the values are identical.

**Test vectors (S = 123, cells of §2.3 and their neighbours present):**

| Column | warmth | moisture |
|---|---|---|
| (8396928, 8396928) | 0.5601367354393005 (pure home cell) | 0.4915097653865814 |
| (8402400, 8396800) | 0.6633611917495728 | 0.5175975561141968 |
| (8402440, 8396800) | 0.6896752715110779 | 0.5242478847503662 |
| (8402480, 8396800) | 0.733677864074707 | 0.5353685617446899 |
| (8402520, 8396800) | 0.7547471523284912 (pure cell (513, 512)) | 0.5406934022903442 |

---

## 5. Link channels

### 5.1 Segment distance (0x5ef5a0)

For points A, B, P (doubles): with v = B − A and q = |v|²: if q < 9.999999682655225e-21,
return |P − A|²; else t = ((P − A)·v)/q; t ≤ 0 → |P − A|²; t ≥ 1 → |P − B|²; otherwise
|P − (A + t·v)|². Computed in double, returned as float.

### 5.2 Channel distance B(x, y) (0x5eea60)

1. (ci, cj) = owner index of (x, y) (§3.4). If none, or that cell is missing, B = 1.
2. Distorted point P (double):
   P.x = x + 500·(0.1·N(O204 + 0.01x, O208 + 0.01y) + N(O1fc + 0.0005x, O200 + 0.0005y)),
   P.y = y + 500·(0.1·N(O214 + 0.01x, O218 + 0.01y) + N(O20c + 0.0005x, O210 + 0.0005y)).
   (Each N is converted to double before the 0.1 multiply and the sum.)
3. For the neighbours (ci−1, cj), (ci+1, cj), (ci, cj−1), (ci, cj+1) in that order, if
   present: s = segment distance from the owner's point to the neighbour's point, at P
   (points as doubles). Keep the smallest (the first value found always replaces the
   initial "none").
4. If none was found, B = 1; else **B = f32(s_min · 0.0002)** (float constant 0x71dce0).

So B grows with the squared distance from the straight lines joining each cell point to
its four grid neighbours; B = 1 at about 70.7 blocks. The terrain uses small B to flatten
and roughen the ground along these lines. Whether these lines are meant as rivers, paths
or valleys is not established (they are not filled with water by the code traced so far).

### 5.3 Grid coordinates G(x, y) (0x59fc90)

G = P · 6.103515625·10⁻⁵ (= 1/16384), computed as x·K + 500·K·(…) in double with the same
four noises as §5.2. Used for the town street pattern (terrain spec §3.2). The zone
generator also stores trunc(G) of the zone centre.

---

## 6. Regions and location records

### 6.1 Region preparation (0x5da280; server counterpart 0x50e080)

A region (rx, ry) covers 64 × 64 zones (16384 × 16384 blocks). It is prepared once, before
any of its zones (or its neighbours' zones) is generated:

1. Create region cells (rx + i, ry + j) for i = −2..2 (outer), j = −2..2 (inner).
2. `srand(A + 1024·ry + rx)`.
3. Allocate the region object (0x15a28 bytes, constructor 0x5c3ac0).
4. Fill its content plan: header fields (+0x0c from 0x5a4710(rx, ry), +0x10, +0x14 from
   draws), then the **8 × 8 location records** (§6.2), then further plans (towns, dungeons,
   a 64 × 64 per-zone table at +0x18 with 16-byte entries). This step is a long routine
   (9088 bytes) and is **only outlined here** (§6.4); a dedicated spec is needed (open
   question 1).
5. Store the region in W+0xbc.

### 6.2 Location record (0x68 bytes, at region + 0x14018 + 0x68·(8·sx + sy))

(sx, sy) = (lx mod 8, ly mod 8). Fields established from their uses:

| Offset | Type | Meaning |
|---|---|---|
| +0x00 | int64 | centre x, fx |
| +0x08 | int64 | centre y, fx |
| +0x10 | float | **radius** r, blocks |
| +0x14 | float | **ground height** h (from the height function at the centre, clamped ≥ 0 for some types) |
| +0x18 | int32 | **type** (0 = none) |
| +0x1c, +0x20, +0x24, +0x28 | various | type-dependent (variant, ids, flags) |

Types seen in terrain rules (role inferred from the rules, names are this spec's):

| Type | Terrain effect (terrain spec) | Probable role |
|---|---|---|
| 1 | flattens detail, street grid, roughness bump | town |
| 2 | channel width, mountain mask override, roughness bump | (open) |
| 3 | moisture boost when dry (§4) | oasis |
| 4 | basin to h − 25 / h − 50, lake fill, flatness | lake |
| 5 | mountain mask hole | (open) |
| 6, 7 | ravine valley to −30 / ridge +10, never below the cell blend; type 6 also gets a grid of carved caves (overview §5) and extra mountain cover | valley |
| 10 | mountain mask hole around a segment (0x5c3ef0) | (open) |
| 11 | plain distance; a 100 × 100 × (100..199) cavity carved at the centre zone (overview §5) | great cavity |
| 12 | plain distance; a giant voxel tree at the centre zone (vegetation spec §6) | giant tree |
| 14 | plain distance | (open) |
| 13 | spire up to +150 | peak |
| 15 | chosen instead of 4 or 6 in sea cells | (open) |

### 6.3 Location lookup (0x487da0)

For a sub-cell (lx, ly) with 0 ≤ lx, ly < 8192: the region (lx·8 >> 6, ly·8 >> 6) — i.e.
(lx >> 3, ly >> 3) — must exist; the record is at slot (lx mod 8, ly mod 8). A column's
location is the one of (floor_div(x, 2048), floor_div(y, 2048)). The record always exists
when the region does; type 0 means no location.

### 6.4 What is known about location creation

From the outline of 0x5da280: for each of the 64 slots, a position is chosen inside the
slot (with draws scaled by 256 + (draw & 255) and by a range reduced by the radius), the
height h is taken from the terrain height function at the centre (clamped to ≥ 0 where
the slot's owner is land), and a type is chosen from a draw & 7 switch: 0 → 2, 1 → 3
(with a variant draw % 3), 2 → 4 (15 if the slot's cell is sea), 3 → 5 (variant from the
cell's moisture and warmth), 4 → 6 (15 in sea), 5 → 7 (15 in sea), 6 → 11 (r = 128, centre
snapped to a block centre), 7 → 12 (r = 128). Separate passes create type 14 (r = 150) and
type 10 entries and fill the per-zone table. The exact draw order, the conditions that skip
a slot, and the assignment of types 1 and 13 are **not specified yet** (open question 1).
Because the height function reads these records, the terrain is only bit-exact once this
is specified.

---

## 7. Normalised location distance f(L, x, y) (0x5f8e10)

Inputs: a location L and a position (X, Y) in fx (callers pass x << 16, y << 16).

* If L.r < 0.001: f = 0.
* Types 11, 12, 14: f = (ex² + ey²) / r² with ex = f32((float)(X − L.x)) · 2⁻¹⁶ (float),
  likewise ey, all float. (No distortion.)
* All other types (the distorted boundary):
  1. Tx = trunc64(f64(X) · (−0.0025)), Ty = trunc64(f64(Y) · (−0.0025)) (int64, via x87;
     truncation assumed).
  2. a = (0x80AD580000 − Tx)·2⁻¹⁶ and b = (0x1617D0000 − Ty)·2⁻¹⁶ in double; this is
     8432984 + 0.0025·x and 90493 + 0.0025·y.
  3. k = 100 for type 13, 256 for other types.
     DX = trunc64(f32(N(a, b) · k · 65536)) − L.x + X (int64), ex = f32((float)DX) · 2⁻¹⁶.
  4. a′ = (0xD5F0000 − Tx)·2⁻¹⁶ = 3423 + 0.0025x, b′ = (0x700000 − Ty)·2⁻¹⁶ = 112 + 0.0025y;
     DY = trunc64(f32(N(a′, b′) · k · 65536)) − L.y + Y, ey likewise.
  5. f = f32(f32(ey²) + f32(ex²)) / f32(r²) (float; ey² is added to ex²).

f is 0 at the (shifted) centre and 1 on the wobbly rim. **bump(f)** = max(1 − f, 0)² is the
common falloff.

---

## 8. Checking against the running game

* Region cells: read W+0x4000bc[rx·1024 + ry] after the first zones have loaded and dump
  the 0x1c-byte records around the home region; compare with §2.3 for the session's seed.
* Blends: call 0x5c4dd0 / 0x5c4800 / 0x5f0720 / 0x5ef040 (thiscall, ECX = W, two int
  arguments, float in ST0) at the test columns of §4.
* B: call 0x5eea60 (thiscall, ECX = W, x, y) on a line between two cell points; values
  should be near 0 on the segment and reach 1 about 71 blocks away.
* Locations: dump region + 0x14018 (64 × 0x68 bytes) for the home region; this is the data
  the locations follow-up spec must reproduce.

---

## 9. Open questions

1. **Location creation** (0x5da280, 0x5a4710, 0x5c3ac0, 0x5f4d10): full draw order, type
   1/13 assignment, radius and height rules, the per-zone table at region+0x18, and the
   type-10 segment helper 0x5c3ef0. Needed for bit-exact terrain.
2. **Region storage time.** It is assumed the region object is stored in W+0xbc only at
   the end of preparation, so that lookups of its own locations during preparation see
   nothing; if it is stored earlier, heights computed during preparation change.
3. The int64 conversions (0x68d946, used for float → fx) are assumed to truncate towards
   zero; this matters at exact multiples.
4. The meaning of B (rivers? paths?) and of location types 2, 5, 10, 15.

See also: questions 1–3 and part of 4 are answered in [alpha-worldgen-locations.md](alpha-worldgen-locations.md) (§2.2, §11); roads along B in [alpha-worldgen-caves.md](alpha-worldgen-caves.md) §5.1.
