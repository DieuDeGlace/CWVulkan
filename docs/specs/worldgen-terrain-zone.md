# World generator: the blocks of a zone's columns

Part A, third file. Conventions are those of `worldgen-terrain.md` §0; heights and weights come from
`worldgen-terrain-height.md`. A zone is 64 × 64 columns; zone (zx, zy) covers x = zx × 64 … zx × 64 + 63
and likewise y.

---

## 1. World values and the zone seed (0x2b5f20)

When a world is set up with its seed S (stored at world+0x220), 0x2b5f20:
1. srand(S).
2. 64 times: makes a 16-entry list [2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 0, 0, 0, 0, 0] and shuffles it
   with the MSVC std::shuffle (0x298d40) drawing on rand(), then appends it to world+0x4a0 (the 64
   town layouts, used by part B). The shuffle, for position i = 1 … 15 with n = i + 1:
   draw r = rand() (one draw: 15 bits always cover n − 1); accept it unless
   (r / n ≥ 32767 / n and 32767 mod n ≠ n − 1), else draw again; then swap entries i and r mod n
   (unsigned arithmetic).
3. Then 77 more draws, each stored as rand() mod 100000 (signed 32-bit) in this order:
   world+0x244 (the **zone seed**), +0x224 … +0x230 (4), +0x248 … +0x344 (64, contiguous, in rising
   address), +0x234 … +0x240 (4), +0x348 … +0x354 (4).

For seed 123 the zone seed is 15376 and world+0x330, +0x334 are 10211 and 5983 (checked by the
gen-blocks header). The terrain uses only +0x244, +0x330 and +0x334.

## 2. Making a zone (0x2ae0c0)

When a zone is first needed: srand(zy × 1024 + zoneSeed + zx) (32-bit wrapping), then allocate the
0x58518-byte zone, construct it (0x2f9400: stores the world at +0x8, zone x at +0x10, zone y at
+0x14; draws nothing), then run the generator 0x2ff6f0. All of a zone's generation shares this one
rand() stream.

## 3. Zone layout and the generator's prologue

### 3.1 The column field (zone+0x518, 4096 × 0x58 bytes)

Column (lx, ly) (local 0..63) is entry lx × 64 + ly. 0x518 + 4096 × 0x58 = 0x58518, the zone size.

| Entry offset | Type | Contents |
|---|---|---|
| +0x08 | f32 | height (§5.1 step 16) |
| +0x0c | f32 | warmth (§4.1) |
| +0x10 | f32 | dryness |
| +0x14 | f32 | highlands |
| +0x18 | f32 | lavaRegion |
| +0x1c | f32 | forestRegion |
| +0x20 | f32 | kind-27 weight (§4.1) |
| +0x24 | f32 | lake |
| +0x28 | f32 | vegetation |
| +0x2c | f32 | river |
| +0x30 | f32 | lava |
| +0x34 | f32 | swamp |
| +0x38 | f32 | roads |
| +0x3c | i32 | base z of the stored blocks |
| +0x40 | vector | stored blocks, 6 bytes each (red, green, blue, glow, type, flags), from base z up |

### 3.2 The zone's place

1. (pcx, pcy) = (fdiv(zx × 64, 1024), fdiv(zy × 64, 1024)).
2. **0x31a5d0** fills a place record (terrain §3.6 layout) for that cell **without the reach gate**:
   big = bigCell; kind = placeKind(pcx, pcy, big); if kind is 0, variant 0 (radius and point left
   unset); else radius 500/250, point = cellPoint(pcx, pcy, big), variant = 0x2abb90.
3. If big, (pcx, pcy) are evened for what follows.
4. Part B's prologue work happens here (kind-8 structures, cell objects 0x69c60; the big structure
   0x2aa270 and its anchor 0x2a9df0; the realm 0x2bb060; the region roles 0x115df0). None draws rand().
   The terrain keeps: the **anchor** — 0x2a9df0's result when 0x2aa270 is non-zero, else
   (0x7fffffff, 0x7fffffff) — interface in `worldgen-structures.md` (big structure and anchor:
   (cell, kind, big) → structure number, anchor in 16-block cells).
5. **Rock-span height** (only when the kind is 24, 45 or 46, test 0x2b8010): SPH = 0x2ba200(place);
   if SPH ≤ 10 then SPH = 10. Otherwise SPH is 0 (unused).

For each column, **0x31a690** completes the record at (x, y): if kind is 0, distance and weight 0;
else d = warpedDist2(point, x, y), weight = (1 − d / float(radius²))² if d < float(radius²), else 0.
This is the place every column of the zone uses (no reach gate).

### 3.3 Column order

Columns are visited x outer (zx × 64 … +63), y inner. (The game calls Sleep(0) once per x row and once
before each cliff pass; no effect.) Everything in §5 happens for one column before the next.

---

## 4. Block properties and colours

A **block property** is three float colours (r, g, b), a glow byte, a type byte and a flags byte.
Types the terrain makes: 0 air, 2 water, 3 wet ground, 4 grass, 5 ground, 6 rock, 10 snow, 13 road,
14 lava ground, 17 lava, 18 cliff face, 19 poison (swamp), 20 (light block, only in the dead pass).

**Storing a property (0xba820):** each colour c → c × 0.5; if 0 > c then 0, else if c > 255 then 255;
byte = trunc64(c) (low 8 bits). Glow, type, flags are copied.

### 4.1 Warmth (0x2d9b60) and the kind-27 weight (0x2abb60)

Warmth (Nb, kind, w): warm = region weight {4, 9, 10, 13} k 0.0004; cold = {6, 11} k 0.0004
(height §3.1 form). Kind 19: warm = warm + w; cold = cold × (1 − w). Kinds 14 and 18:
warm = warm + w × 0.25; cold = cold × (1 − w).
v = N(0.0002·X + 4234234, 0.0002·Y + 55543) × 0.35; v = v + ((warm + warm) + 0.5); v = v − cold;
if not (1 > v) v = 1; return v if 0 < v, else 0. (X, Y are Nb's.)

Kind-27 weight: w when kind = 27, else 0.

### 4.2 The block at height z (0x2d8e00)

Inputs: x, y, z, h (the column's height), the four neighbours' heights west (x − 1), east (x + 1),
south (y − 1), north (y + 1), highlands, warmth, dryness, vegetation, an unused tint T (callers
always pass 0), roads, lavaRegion, forestRegion, kind27, lava, swamp.

1. **Lava** (lava > 0.5): r = 255, g = (N(0.1·X, 0.1·Y) + N(0.05·X, 0.05·Y)) × 50 + 50, b = 20;
   glow 255, type 17, flags 0.
2. **Poison** (else swamp > 0.5): r = 30, g = N(0.06·X, 0.06·Y) × 20 + 120, b = 40; glow 255, type 19.
3. fz = floorf(h). **Buried** when z < fz and z < floorf(west), z < floorf(east), z < floorf(south),
   z < floorf(north) (tested in that order).
4. If buried, or vegetation > 0.5, or roads > 0.5, or lowShore(x, y, h) (height §8.7) — tested in that
   order after "buried" — the result is the **buried colour** (§4.3); if T > 0 each colour moves
   toward 128 (c + (128 − c) × T; never with T = 0); if roads > 0.5 the type becomes 13.
5. Otherwise: dW = west − h, dE = east − h, dS = south − h, dN = north − h;
   lo = min(dW, 0), hi = max(dW, 0); lo = min(dE, lo), hi = max(dE, hi); then dS, then dN likewise
   (each as min(new, previous)).
   top = the **surface colour** (§4.4).
   If (z < fz and −1 > lo) or hi > 1:
     steep = −lo if (z < fz and −1 > lo), else hi;
     t = (steep − 1) × 3; t = min(t, 1) then max(0, …) (as max(0, min(1, …)));
     side = buried colour; colour = side × t + top × (1 − t) per channel (1 − t computed once);
     glow from top; type and flags from side if t > 0.5, else from top.
   Else the result is top.

### 4.3 Buried colour (0x2a3140)

Inputs x, y, z, highlands, warmth, dryness, lavaRegion, forestRegion, kind27.
highlands > 0.5: rock colour (§4.5b) with (warmth, lavaRegion, forestRegion), type 6; else ground
colour (§4.5a), type 5. Glow 0, flags 0.

### 4.4 Surface colour (0x2a3e30)

1. Grass (0x2b40b0): a = (N(0.005·X + 343, 0.005·Y + 9233) + 1) × 0.5; a = a × a;
   b = (N(0.02·X + 6654, 0.02·Y + 778) + 1) × 0.5; ib = 1 − b;
   g = b × 260 + ib × (200 − a × 50); r = (220 − a × 100) × b + ib × (80 − a × 60);
   bl = b × 30 + ib × (a × 80 + 40); colour (r + 0, g + 0, bl + 0). Type 4, glow 0, flags 0.
2. d = (0.5 − dryness) × 80; r = r + d; if dryness > 0.5: b = b − d.
3. If forestRegion > 0.001 (0.0010000000474974513), with f = forestRegion:
   p = (N(0.01·X, 0.01·Y) + 1) × 30; q = (N(0.006·X + 4343, 0.006·Y + 983) + 1) × 30; inv = 1 − f;
   newR = ((p + 128) − q) × f + inv × r; newB = (max(q, p) + 128) × f + inv × b;
   g = inv × g + f × 128; r = newR; b = newB.
4. If kind27 > 0.001, k = kind27: p = (N(0.01·X, 0.01·Y) + 1) × 20; q = (N(0.006·X + 4343,
   0.006·Y + 983) + 1) × 100; inv = 1 − k; R' = inv × r, G' = inv × g, B' = inv × b;
   g = G' + k × 60; r = R' + ((p + 128) − q × 0.5) × k; b = B' + ((p × 0.5 + 128) + q) × k.
5. If lavaRegion > 0.001, l = lavaRegion: il = 1 − l; R' = il × r, G' = il × g, B' = il × b;
   K = rock colour(x, y, z, warmth, l, forestRegion); r = K.r × l + R'; g = G' + K.g × l;
   b = K.b × l + B'; if l > 0.5 the type becomes 14.

### 4.5 Ground and rock colours

**(a) Ground (0x2ab410)** inputs x, y, z, warmth, dryness, lavaRegion, forestRegion, kind27:
1. t = (N(0.015·X + 1883, 0.015·Y + 98984) + 1) × 0.5; b = t × 0 + 10; g = t × 150 + 160;
   r = t × 150 + 260 (computed b, g, r in this order).
2. k = N(0.01·X + 431, 0.01·Y + 873) × 30 + 30; r = k + r; g = k + g; b = k + b.
3. sand() = (N(V330 + 0.03·X, V334 + 0.03·Y) + 1) × 0.5 × 70, where V330, V334 are world+0x330,
   +0x334 as doubles and the noise input is V + X × 0.03 (double).
4. If z < 100: s = max(0, float(z) / 100); s = s × s; q = sand(); inv = 1 − s;
   r = inv × ((q + 340) − r) + r; g = inv × ((q + 250) − g) + g; b = inv × ((q + 100) − b) + b.
5. m = min(1 − warmth, dryness). If 0.3 > m: w = min(1, m / 0.3); R' = r × w, G' = g × w, B' = b × w;
   q = sand(); iw = 1 − w; g = iw × (q + 250) + G'; r = iw × (q + 340) + R'; b = iw × (q + 100) + B'.
6. If 0.2 > warmth: cw = max(0, 1 − warmth × 5); i = 1 − cw; R' = r × i, G' = g × i, B' = b × i;
   K = rock colour(x, y, z, warmth, lavaRegion, forestRegion); cw = cw × 0.5;
   r = (K.r + 200) × cw + R'; g = (K.g + 280) × cw + G'; b = (K.b + 400) × cw + B'.
7. If forestRegion > 0.001, f = forestRegion: p = (N(0.01·X, 0.01·Y) + 1) × 20,
   q = (N(0.006·X + 4343, 0.006·Y + 983) + 1) × 20; inv = 1 − f; G' = inv × g, R' = inv × r, B' = inv × b;
   b = ((p + 128) + q) × f + B'; r = ((p × 0.5 + 128) − q × 0.5) × f + R'; g = (128 − q × 0.5) × f + G'.
8. If kind27 > 0.001, k: p = (N(0.01·X, 0.01·Y) + 1) × 100; q = (N(0.006·X + 4343, 0.006·Y + 983) + 1) × 50;
   inv = 1 − k; R' = r × inv, G' = g × inv, B' = b × inv; g = k × 128 + G';
   r = ((p + 128) − q × 0.5) × k + R'; b = ((p × 0.5 + 128) + q) × k + B'.
9. If lavaRegion > 0.001, l: il = 1 − l; R' = r × il, …; K = rock colour(…); r = K.r × l + R';
   g = K.g × l + G'; b = K.b × l + B'.

**(b) Rock (0x2d3150)** inputs x, y, z, warmth, lavaRegion, forestRegion. Here Z = float64(z) × 0.4,
Xd = X × 0.1, Yd = Y × 0.1 (doubles).
1. v = N(Yd + 98984, Z); v = v + N(Xd, Z); v = (v × 0.5 + 1) × 0.5.
2. r = g = b = 210. If not (0.5 > warmth) and warmth > 0.6: t = min(1, (warmth − 0.6) / 0.3);
   r = t × 110 + 210; g = t × 10 + 210; b = t × −130 + 210.
3. v = v × 0.3; r = v × r + r; g = v × g + g; b = v × b + b.
4. If forestRegion > 0.001, f: k = 1 − f × 0.25; m = f × 0.15; r = r × k, g = g × k, b = b × k;
   r = (1 − m) × r; g = k × g (so g is multiplied by k twice).
5. If lavaRegion > 0.001, l: s = N(0.04·X + 8773, 0.04·Y + 99808) × 0.5; s = s + N(0.02·X, 0.02·Y);
   u = 1 − |min(1, s)|; il = 1 − l; u5 = u × (((u × u) × u) × u); r, g, b each × il;
   glowR = u5 × 300 + 30; n = N(Xd, Yd) + 1; r = glowR × l + r; g = ((n × u5) × 50 + 30) × l + g;
   b = l × 30 + b.

### 4.6 River bed colour (0xbd110)

At (x, y, z), from scratch: Nb at (x, y); the cell's place with **no reach gate**: big = bigCell,
kind = placeKind(cell, big), w = (1 − d/R²)² when kind ≠ 0 and d = warpedDist2(cellPoint, x, y) < R²
(R = 500/250), else 0. Colour = ground colour with warmth(Nb, kind, w), dryness(Nb),
lavaRegion(Nb, kind, w), forestRegion(Nb, kind, w), kind27(kind, w). Type 3, glow 0, flags 0.

### 4.7 Snow (0x2b4320, colour 0x2d3d90)

Snow lies at (x, y) with warmth and the four neighbour heights when: 0.3 > warmth, and
|west − east| < 2 and |south − north| < 2 (differences in float, absolute value via double); then
true if 0.1 > warmth, else true when N(0.05·X + 8411122, 0.05·Y + 884646) × 0.05 + 0.2 > warmth.
The snow block is colour (200, 280, 400), glow 0, type 10, flags 1.

### 4.8 Colours "from scratch" (0x2f39e0, 0xbbfe0)

Used by the cliff pass and by part B. Both recompute a column's values at (x, y) through the
reach-gated place 0x35ce0 and the same routines as the height pipeline (height §1), then:
* 0x2f39e0 (x, y, z): the buried colour (§4.3) with those values.
* 0xbbfe0 (x, y, z): the surface colour (§4.4) with those values.

### 4.9 Water level (0x2e6420)

Inputs base (the column's base height) and the place P.
If kind is 0 or one of 10, 17, 19, 21, 28, 36 (mask 0x10102a0400): base.
w = P.weight; kinds 8, 13, 14: s = min(1, max(0, w + w)); w = smooth(s).
If w > 9.999999747378752e-05: ph = max(0x2ba200(P), 10); base = base + (ph − base) × w. Return base.

---

## 5. The column pass of the zone generator (0x2ff6f0, 0x2ffc00–0x302294)

### 5.1 The field (per column)

1. Nb = 0x358e0(x, y); place = §3.2 record completed at (x, y).
2. base = 0x2a0840(Nb, kind, weight); dryRegion = 0x2a44a0; warmth = §4.1; dryness; lake;
   lavaRegion; forestRegion; kind27; roads(x, y, kind, weight); wet; river; lava; swamp;
   highlands(x, y, base, …); vegetation(x, y, dryness, highlands, dryRegion);
   height = 0x2b4450(…, mode 0). (Call order as listed; all as in the height file but with this
   place record.) The values go into the field (§3.1).

### 5.2 Storing a block (0x31c1c0)

Store(x, y, z, block) into the zone's column:
* Ignored outside the zone.
* Ignored when the block's type and flags are both 0 and z ≥ base + count (no air is stored above a
  column).
* If the column is empty, base = z.
* If z < 1, type 0 and not (flags AND 2): the block is replaced by sea water (255, 255, 255, glow 0,
  type 2, flags 0).
* Written at z. If z < base the column grows down: base becomes z, and every height between z + 1 and
  the old base − 1 is filled with the buried colour (§4.3) from the column's field values (highlands,
  warmth, dryness, lavaRegion, forestRegion, kind27), stored as §4. If z ≥ top the column grows up,
  the gap filled with zero blocks.

### 5.3 Reading a block (0xeb90)

Outside the zone, or below base: solid grey (200, 200, 200, glow 0, type 1, flags 0). Within the
stored range: the block, except that an air block (type 0) at z ≤ 0 without flag 2 reads as sea water.
Above the top: air (255, 255, 255, 0, 0, 0) if z > 0, else sea water.

### 5.4 The column's blocks

Let h = height, fz = floorf(h).

1. **River** (river > 0.5 and h > −5): bed colour (§4.6) at fz − 1, type 3, stored at fz − 1. Then
   top = floorf(waterTop(base, place)) − 1 (§4.9); for z = fz … top − 1 store water: the **property**
   (255.0, 255.0, 255.0, glow 0, type 2, flags 0) (0x3000e0), converted by 0xba820 (§4) to the
   **bytes (127, 127, 127, 0, 2, 0)** (255 × 0.5 = 127.5, truncated). (No snow, no rock span, no
   neighbour heights.) This is the only water the terrain stores in a column; sea water (§5.2, §5.3)
   is the byte block (255, 255, 255, 0, 2, 0) and is not converted.
2. **Otherwise:**
   a. Neighbour heights: west, east, south, north = 0x35ea0 mode 0 at (x − 1, y), (x + 1, y),
      (x, y − 1), (x, y + 1) — the full reach-gated pipeline, not the zone's place.
   b. Store 0x2d8e00 at z = fz (§4.2, T = 0).
   c. **Snow**: if not (vegetation > 0.5), not (roads > 0.5), h > 0, and snow (§4.7) holds: store the
      snow block at fz + 1; remember "snowed".
   d. **Rock span** (kinds 24, 45, 46): lo = spanBottom, hi = spanTop (§5.8); for z = lo … hi − 1 store
      the span rock colour (§5.8) with glow 0, type 6, flags 0.
3. **Road over a walled pit** (0x2ae6d0, both branches): when roads ≥ 0.5, the zone place's variant is
   5 and P.dist2 ≤ float(radius²): t = (sqrtf(dist2) / float(radius) − 0.5) × 5; t = t × t; t = min(t, 1)
   (as "1 if t > 1"); k = min(1 − (1 − roads) / (t × 0.75 + 0.25), 1). If 0.5 ≤ k: top = 0x35ea0 at (x, y)
   with mode 1; bottom = (top − 1) − (t × 40 + 20) × (k − 0.5). For z = floorf(bottom) … floorf(top)
   (inclusive) store the buried colour (§4.3, field values).
4. **(Never runs)** — §5.7.
5. **Cliff pass** — §5.6.

### 5.6 The cliff pass (0x301a0c–0x302254)

When highlands > 0.5 and g = cliffs(x, y, kind, weight) > 0 (zone place; height §8.9):
1. low = 0x2b4450 with this column's field values and place, mode 2.
   T = (N(0.01·X + 7474, 0.01·Y + 98984) + 1) × 20 + low. bot = floorf(low).
2. old = read (§5.3) at (x, y, fz); r2, g2, b2 = float(old byte) doubled (r + r).
3. slope = N(0.02·X + 8744, 0.02·Y + 904) × 0.025 + 0.06.
4. solidBelow = true; gapBase = bot.
5. For z = bot + 1 … fz + 2, with zf = float(z):
   d = h − zf; depth = d × 0.1 + 0.5; m = min(|(zf − T) × slope|, 1); t = 1 − (1 − m × m) × g;
   v = smooth(t) × depth.
   * If 0.5 < v (solid): if not solidBelow and (x + 3y) rem 5 = 0 (32-bit, C signed rem; computed
     x + 2y + y): draw **rand()**; if it is divisible by 500 and fz + 1 − z > 16: add a model to the
     zone's model list (zone+0x18; record type 0x59, position (x × 65536 + 32768, y × 65536 + 32768,
     (z + 1) × 65536), size float(z − gapBase), second float 1.0, tint (1, 1, 1), flags 2 — format
     belongs to part B) and store at z the block (0, 0, 0, glow 255, type 0, flags 0).
     Then solidBelow = true.
   * Else (open): store the zero block (0, 0, 0, 0, 0, 0) at z. If solidBelow was true (first open
     block of a gap): solidBelow = false; gapBase = z − 1; colour the block below (z − 1):
     c = 0x2f39e0 at (x, y, z − 1) (§4.8); type = 18;
     a = min(1, max(0, 1 − d × 0.3)); if a > 0.5 type = old's type;
     c.r = c.r + (r2 − c.r) × a, likewise g, b;
     u = min(1, max(0, 1 − (zf − low) × 0.3));
     e = (vegetation > 0.5 or lowShore(x, y, zf)) ? 0x2f39e0 : 0xbbfe0, at (x, y, z − 1);
     if u > 0.5 type = e's type; c.r = c.r + (e.r − c.r) × u, likewise g; c.b = c.b + u × (e.b − c.b);
     store (c, glow 0, type, flags 0) at z − 1. If snowed and type ≠ 18: store the snow block at z.
   (At 0x301bfb an alternative glowing air block is prepared under a condition fz + 1 < fz that never
   holds; the open block is always zero.)

### 5.7 Passes that never run

Between the column's blocks and the cliff pass (0x30068e–0x301a0c) the generator walks five lists
created empty in its prologue and never filled; the block is skipped. Inside it, entries would be
added to a sixth list, which the generator walks after the column loop (0x3022c3–0x3024ae) to place
light blocks (type 20). Both never run.

After the column loop the cell loop of part B begins (`worldgen-structures.md`).

### 5.8 Rock spans (0x2ba2b0, 0x2bab80, 0x2ba5f0)

For kinds 24 (arena), 45, 46. P is the zone place; anchor (ax, ay) from §3.2 (16-block cells);
h the column's height.

**Base** P0: variant 2: c = cell of (x, y) (evened if big), turns = float(R(c.y × 57 + 0x4e3e1c + c.x)
mod 20 + 25); P0 = SPH + min(150, float(radius) × 0.6 / turns × 15). Variant 1: P0 = SPH +
(float(radius) × 0.7 − float(radius) × 0.5) × 0.75. Else P0 = SPH.

**Bottom** (0x2ba2b0):
* kind 45, |x − ax × 16| < 8 and y < (ay + 1) × 16: trunc(P0 + 15).
* kind 24: (qx, qy) = (fdiv(P.x, 16), fdiv(P.y, 16)); dx = qx × 16 − x + 8, dy = qy × 16 − y + 8,
  d = dx² + dy² (integers). If 0xe0f < d < 0x4731: d > 0x4623 → trunc(P0 + 30); d > 0x4518 →
  trunc(P0 + 29); else if fdiv(x, 16) = qx or fdiv(y, 16) = qy → floorf(P0) + 12.
* otherwise floorf(h).

**Top** (0x2bab80):
* kind 24: (cx, cy) = (fdiv(P.x, 16) × 16 + 8, fdiv(P.y, 16) × 16 + 8); d = (cx − x)² + (cy − y)².
  d ≥ 0x4731 → floorf(h) − 1. d < 0xe10 → trunc(P0 + 1). d > 0x3fff → trunc(P0 + 40).
  ax' = |x − cx|, ay' = |y − cy|. If both > 19: s = max(sqrtf(float(d)) × 0.5 − 30, 0);
  result trunc(float((trunc(s) / 2) × 2 + 2) + P0 + 1). Else s = max(float(max(ax', ay')) × 0.5 − 30, 0);
  trunc(s + P0 + 1).
* kinds 45, 46: dx = |x − ax × 16|, dy = |y − ay × 16|, m = max(dx, dy). Kind 46: m = max(m, 24); if
  dx > 12 and dy > 12, m = (m / 6) × 6 − 3. v = (P0 + 100) − float(m); result floorf(h if v ≤ h else v).

**Colour** (0x2ba5f0): kind 24 and (x − (qx × 16 + 8))² + (y − qy × 16 − 8)² < 0xe10: the sand colour
(0x2d3830) (q + 340, q + 250, q + 100) with q = sand() of §4.5a. Otherwise base (180, 170, b) with
b = 120 for kind 24, 90 for 46; kind 45 uses (260, 250, 130). n = N(((x + z) / 2) × 154 + z × 53,
((y + z) / 2) × 154 + z × 53) (integers, C division, used directly as the noise inputs); if n > 0.7:
n = (n − 0.7) / 0.3 × 0.1 + 1; each channel × n.

---

## 6. The light pass (0x2a3260; after generation)

Not part of 0x2ff6f0: the game lights a zone later (called from 0xe9530 with a margin of 8 columns,
reading the neighbouring zones). Freshly generated oracle zones are unlit. The result is the red byte
of air (type 0) and water (type 2) blocks — "open" blocks, type AND 0xfd = 0.

Over the zone plus 8 columns each side (a light map per column, one byte per stored block; below a
column's base it is dark (0), above its top open sky (255), outside the area 255):
1. **Sky**, per column top down: light starts at 255, sky = true. A type-20 block sets sky = true,
   light = its glow. A solid block gets the current light, sets sky = false and then
   light = min(light, its glow). An open block gets 255 if sky, else max(light, its glow).
2. **Spread**, margin − 3 = 5 rounds, over the area less one column each side: every open block not
   already 255 takes max over its six neighbours of max(neighbour, 25), times 85 / 100 (integer); the
   round's results are written back after the whole area is done.
3. The zone's own open blocks get the light as their red byte.

---

## 7. Where CubeVulkan's own options plug in

WorldScale changes only the region lookups (terrain §2.6). Mod hooks replace the place kind
(terrain §3.4) and part B/C items. Neither is the game's and neither is specified here.

## Errata (from the clean-room implementation, checked against the oracles)

- §6, the light pass: below a glowing open block (an overhang light), the running light becomes max(light, glow) and keeps that value for the open blocks under it. It is not a one-block bump; read literally, the overhang columns come out at 21 where the game has up to 255.
- **Settled (§5.4 step 1, §6), 2026-10-27.** Lit-zone water is (light, 127, 127) because the generator
  stores river water as the float property 255.0 per channel, which the storing routine 0xba820 halves
  and truncates (its scale 0.5 at 0x1404ac964, its cap 255.0 at 0x14042e7d8) to (127, 127, 127). The
  light pass (§6) then replaces only the red byte (it rewrites the block with 0xa36e0, copying the
  other five bytes unchanged), and the zone save (ZoneExtractor's .cwz, a byte copy of the columns)
  changes nothing. Evidence: 0x3000e0–0x300121 builds (255.0, 255.0, 255.0, 0, 2, 0) and converts it
  with 0xba820; in the first 300 zones of Extract/seed_123 every stored water block (742,455) has
  green = blue = 127, glow 0, flags 0, its red being the light (e.g. zone (−10, −10), column 320:
  bed type 3 at z 83, water (255, 127, 127, 0, 2, 0) at z 84). The gen_seed_123 zones hold no water
  block (no rivers there), so no unlit oracle shows the 127 red byte directly. Implementations store
  river water as (127, 127, 127, 0, 2, 0); the earlier reading "(255, 255, 255)" gave the property,
  not the stored bytes.
