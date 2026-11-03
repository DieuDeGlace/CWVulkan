# Alpha world generation: terrain, blocks and colours

Behavioural specification of how Cube World **Alpha** turns the climate fields and
locations of the [regions spec](alpha-worldgen-regions.md) into the voxel columns of a
zone: the shaping fields (town-ness, flatness, roughness, mountain mask), the base height
function, the per-column surface pass, ravines and lake basins, the block format and every
block colour rule. Written from `Cube.exe` (SHA-256 `84a7a132…30bf`), mainly the zone
generator 0x5e4850 (37430 bytes; its decompile only succeeded on a retry, so formulas here
were checked against the assembly) and the routines it calls. Read the
[random spec](alpha-worldgen-random.md) first (N, A, offsets O…, precision rules).

---

## 0. Conventions

* As in the random spec §0. All formulas below are **float** unless marked double; "N(…)"
  arguments are always double sums of the form (double)offset + coordinate·k, with the
  product in double. Integer → float conversions are exact for block coordinates below 2²⁴.
* (x, y) is a block column, z a block height. +Z is up. **Sea level is z = 0**: water
  fills z ≤ 0 (§2.3).
* Names: **bump(f)** = max(1 − f, 0)²; **f(L, x, y)** is the location distance of the
  regions spec §7 evaluated at (x << 16, y << 16); **L(x, y)** is the location record of the
  column's 2048-block sub-cell (regions spec §6.3), possibly absent; "type" is its +0x18.
* **Climate of a column**: moist(x, y), warm(x, y), fire(x, y) are the blends of the
  regions spec §4. Inside a zone being generated, these are read from the column record;
  outside, computed; the values are identical.

---

## 1. Records

### 1.1 Zone (200 bytes, constructor 0x6240f0)

A zone is 256 × 256 columns; zone (zx, zy) covers x in [256·zx, 256·zx + 256), y likewise,
with 0 ≤ zx, zy < 65536. Zones are found through their region (regions spec §1): the
region object holds 64 × 64 zone pointers at region + 0x10018 + 4·(64·(zx mod 64) +
(zy mod 64)) (lookup 0x434d10).

| Offset | Meaning |
|---|---|
| +0x18 | list used for static objects (vegetation spec) |
| +0x24 | list used for plant sprites (vegetation spec) |
| +0x60, +0x64 | zx, zy |
| +0x78 … +0x87 | 16 bytes copied from the region's per-zone table (regions spec §6.1) |
| +0xa8 | pointer to 65536 column records |

### 1.2 Column (32 bytes, at zone+0xa8 + 32·((x mod 256) + 256·(y mod 256)); lookup 0x4347a0)

| Offset | Type | Meaning |
|---|---|---|
| +0x04 | float | moisture of this column |
| +0x08 | float | warmth |
| +0x0c | float | fire share |
| +0x10 | int32 | **bottom**: z of the first stored block |
| +0x14 | int32 | surface z − 8 (use not traced) |
| +0x18 | pointer | stored blocks, 4 bytes each, from z = bottom upwards |
| +0x1c | int32 | **count** of stored blocks |

### 1.3 Block (4 bytes)

Bytes 0, 1, 2 are R, G, B (0..255). Byte 3: bits 0–4 **type**, bit 5 (0x20) set on blocks
coloured by the surface-colour routine, bit 6 (0x40) "dry air" (see §2.3), bit 7 (0x80) a
"keep" marker: when the generator writes a block over a stored block whose bit 7 is set,
it only writes if the new type is non-zero and then sets bit 7 on the result; otherwise it
writes plainly. In a freshly generated zone the stored blocks start zeroed, so plain
writes happen.

Block types used by the generator (names inferred from use):

| Type | Use |
|---|---|
| 0 | air |
| 1 | default solid (implicit fill, grey 200, 200, 200) |
| 2 | water |
| 3 | lake bed |
| 4 | grass / ordinary ground (the surface routine's default) |
| 6 | rock, cliff, sub-surface |
| 9 | sand |
| 10 | snow |
| 11 | town ground (set by structure code, not traced here) |
| 12 | fire ground |
| 13 | glowing (light 255 in propagation, alpha-rendering.md §9) |

### 1.4 Implicit blocks (reading a block, 0x42f7e0)

* No column (zone missing, or outside the world): solid grey (200, 200, 200), type 1.
* z < bottom: solid grey type 1. (The mesher draws these with the rock colour,
  alpha-rendering.md §9.5.)
* z ≥ bottom + count: air (255, 255, 255, type 0) if z > 0, **water** (255, 255, 255, byte 3 =
  0x82) if z ≤ 0.
* Stored block whose type is 0, z ≤ 0 and bit 0x40 clear: read as water.

### 1.5 Writing a block (0x44e7a0)

Writing block b at (x, y, z) into a zone: nothing happens if the column is missing, or if
b is air (type byte 0) at or above the column top. If z ≤ 0 and (byte 3 AND 0x5f) = 0, b
becomes water (255, 255, 255, 0x82). The column storage is extended as needed (0x44e700);
if the write is below the old bottom, the gap between z + 1 and the old bottom − 1 is filled
with rock-coloured blocks Rk(x, y, z′) (§6.6), type 6.

---

## 2. Water, sea level and the shoreline

### 2.1 Sea level

There is no stored sea: water is every non-solid cell at z ≤ 0 (§1.4). Where the terrain
surface lies below z = 0 the column is under the sea. Cells carved by ravines (§5.5) at
z < 1 are written as explicit water blocks.

### 2.2 Lakes

Location type 4 lowers the ground into a basin (§4.3) and the zone pass fills it with
water blocks up to trunc(h_L − 25) (§5.6).

### 2.3 Dry air

A carved cell above sea level but below the original surface (inside an overhang) is
written as air with bit 0x40, which keeps it air even at z ≤ 0.

---

## 3. Shaping fields

### 3.1 Town-ness q(x, y) (0x5989d0)

q = bump(f(L, x, y)) if L(x, y) has type 1, else 0.

### 3.2 Flatness W(x, y) (0x5f9f70)

1. w = B(x, y) (link channels, regions spec §5.2); g = G(x, y) (regions spec §5.3).
2. If q > 0: a = min(3q, 1), a′ = 1 − (1 − a²)²; for each of g.x then g.y:
   c = f32(cos(g·360)·a′ + 1) (cos of the double g·360, i.e. 360 **radians** per grid unit,
   a period of about 286 blocks; product with a′ in double); w ← min(w, c). This lays a
   street grid into towns.
3. If q > 0.65: w ← w · max((0.7 − q) / 0.05000000074505806, 0) (double, rounded).
4. If L(x, y) has type 2 or 4: w ← w + 2·bump(f(L, x, y)).

### 3.3 Roughness R(x, y) (0x5f9340)

1. v = |f32(N(O170 + 0.01x, O174 + 0.01y)·0.1) + N(O168 + 0.001x, O16c + 0.001y)|.
2. v ← v · ((N(0.001x, 0.001y) + 1)·0.1 + 0.8) (no offsets in this noise).
3. t = 1 − 0.75·B(x, y); if t > 0: v ← v + 0.05·t².
4. L types 1, 2, 4, 13: v ← v + bump(f); types 6, 7: v ← v + 0.5·bump(f).
5. v ← v + fire(x, y). Result v.

### 3.4 Mountain mask M(x, y) (0x5effa0)

1. a = N(0.01x, 0.01y); a′ = 1 − a².
2. b = f32(N(O1dc + 0.01x + 843, O1e0 + 0.01y + 984)·0.1);
   c = N(0.0025x + O1dc, 0.0025y + O1e0).
3. M = 1 − |b + c|·((1 − a′²)·1.3 + 2).
4. M ← M · (0.4·N(0.005x + 94, 0.005y + 874) + 0.6).
5. If L(x, y) has type 6 or 7, with f = f(L, x, y):
   s = 0 if M ≥ 1; 1 − M if M + 0.5 > 1; else 0.5.
   * f ≤ 0.36: k = min((1 − f/0.36)·1.5, 1); M ← (M + s)·(1 − k²) (+ k²·0).
   * 0.36 < f < 1: r = f32(√f) (double square root); g = (r − 0.6)/0.4; M ← (1 − g²)²·s + M.
6. If M < 0: M = 0 and go to step 8.
7. M ← M · smooth(min(2R(x, y), 1)); M ← M · smooth(min(W(x, y), 1));
   M ← M · (1 − smooth(min(2q, 1)))³.
8. Location adjustments (if L exists):
   * type 2: **return** smooth(M) + bump(f) (no further step);
   * type 10 with r > 0: (dX, dY) = offset from the location's segment (0x5c3ef0, open
     question 4); d = (dX² + dY²)·2⁻³² / r² (components converted from fx to float blocks);
     if d < 1: M ← M·(1 − (1 − d)²);
   * types 4, 5: f ≤ 0.25 → M = 0; 0.25 < f < 1 → g = (f32(√f) − 0.5)·2,
     M ← (1 − (1 − g²)²)·M.
9. Return smooth(M).

M is 0 on plains and near 1 on mountain ridges; it lifts the surface (§5.3) and decides
rock cover.

---

## 4. Base height H(x, y) (0x5c5e20; server counterpart 0x4f9b70)

### 4.1 Large-scale terms

1. s1 = unit(N(O18c + 0.0001x, O190 + 0.0001y))², s2 = unit(N(O194 + 0.0001x, O198 + 0.0001y))²,
   s3 = unit(N(O19c + 0.001x, O1a0 + 0.001y))², s4 = unit(N(O1a4 + 0.001x, O1a8 + 0.001y))²,
   s5 = unit(N(O1ac + 0.002x, O1b0 + 0.002y))², where unit(n) = (n + 1)·0.5.
2. g = smooth(min(4R(x, y), 1))²; m3 = g·s3, m4 = g·s4, m5 = g·s5.

### 4.2 Elevation blend

1. (dx, dy) = D(x, y) (regions spec §3.1). Fixed-point sample:
   Px = (x << 16) + trunc64(f32(dx·65536)), Py = (y << 16) + trunc64(f32(dy·65536)).
   (Unlike the climate blends, the shift keeps its fraction.)
2. Window as in the regions spec §3.2; **missing cells are skipped**.
   For each cell: ex = f32((float)((c.x << 16) − Px))·2⁻¹⁶, ey likewise, d² = f32(ey² + ex²)
   (float, **not** truncated). d0² = the smallest (first strictly smaller wins).
3. For each cell in scan order: t = min((d² − d0²)·5.000000058e-08, 1) (float constant
   0x71dcd8), wc = (1 − t)². Σw += wc; Σe += (float)c.e·wc; if c.e > 0: Σl += wc.
4. **E** = Σe/Σw (blended elevation), **Lnd** = Σl/Σw (land share). If no cell was found:
   E = 0, Lnd = 1.

### 4.3 Assembly

1. n6 = N(O1bc + 0.0002x, O1c0 + 0.0002y), n7 = N(O1b4 + 0.0002x, O1b8 + 0.0002y);
   H = ((n6 + 1)·100·s2 + (n7 + 1)·100·s1)·Lnd + E.
2. If q > 0.5: t = min((q − 0.5)·2, 1); m5 ← m5·(1 − smooth(t)).
3. w = W(x, y); c = min(max(w, 0.02)·2, 1); u = 1 − c; g′ = 1 − u⁴;
   A = (0.1g′ + 0.9)·m3, Bm = (0.1g′ + 0.9)·m4, C = (0.5g′ + 0.5)·m5.
4. If L(x, y) has type 1: k = 1 − 0.5·bump(f); A, Bm, C ← ·k.
5. H ← ((N(O1cc + 0.002x, O1d0 + 0.002y) + 1)·50·Bm + (N(O1c4 + 0.002x, O1c8 + 0.002y) + 1)·50·A)
   + (N(O1d4 + 0.01x, O1d8 + 0.01y) + 1)·20·C + H, summed in that order.
6. **Location rules**, if L(x, y) exists with type ≠ 0, in this order:
   * **Island.** Let O = owner cell of the location centre (0x477e10 at the centre ÷ 65536,
     truncating). If O exists, O.e < 0 and type ≠ 11:
     ax = 0.0025x, ay = 0.0025y (double);
     DX = (x << 16) + trunc64(f32(f32(N(ax + 8432984, ay + 90493)·100)·65536)) − L.x;
     DY = trunc64(f32(f32(f32(N(ax + 3423, ay + 112)·100) + (float)y)·65536)) − L.y
     (note: y is added in float here, x in 64-bit integers above);
     ρ = min((ex² + ey²)/(r + 256)², 1) with ex, ey = f32((float)D…)·2⁻¹⁶;
     if 1 − ρ > 0: t = min((1 − ρ)·1.1, 1); H ← H − smooth(t)·(float)O.e. Sea locations
     therefore raise land out of the water by up to −O.e (≤ 100) blocks.
   * **Type 4 (lake basin)**, f = f(L, x, y), h_L = L.h: f ≤ 0.25 → u = (1 − 4f)²,
     H = (1 − u)·(h_L − 25) + u·(h_L − 50); 0.25 < f < 1 → s = f32(√f),
     v = (1 − ((s − 0.5)·2)²)², H ← ((h_L − 25) − H)·v + H.
   * **Types 6, 7 (valley)**: f ≤ 0.25 → u = (1 − 4f)², H = (1 − u)·(H + 10) + u·(H − 30);
     0.25 < f < 1 → H ← v·10 + H (v as above); then in all cases H ← max(H, E).
   * **Type 13 (spire)**: f ≤ 0.01 → result H + 150; 0.01 < f < 1 → w = (f32(√f) − 0.1)/0.9,
     u = 1 − w², result H + 150·u⁴; otherwise H. (Only the result, not H, changes here.)
7. Return H (or the type-13 result).

H is a float height in blocks; values near 0 are the coast.

---

## 5. Generating a zone (0x5e4850; server counterpart 0x518630)

### 5.1 Order

For zone (zx, zy) not yet present:

1. Prepare regions (zx >> 6) + i, (zy >> 6) + j for i = −1..1 (outer), j = −1..1 (inner)
   (regions spec §6.1).
2. Create the zone record; `srand(A + 65536·zy + zx)`; set +0x60/+0x64.
3. Compute (not used further in the parts specified here) the owner of the zone centre
   (256zx + 128, 256zy + 128) and trunc(G) there; note the zone's own location
   **Lz** = location of sub-cell (zx >> 3, zy >> 3).
4. Climate pass (§5.2). 5. Base-height grid (§5.3). 6. Surface pass (§5.4–§5.6), which also
   makes the ravine spawn draws (§5.5). 7. Post passes: structures, vegetation, creatures,
   stored modifications, light (overview spec §5).

### 5.2 Climate pass

For x = 256zx … 256zx + 255 (outer), y = 256zy … 256zy + 255 (inner): column +0x04 =
moisture, +0x08 = warmth, +0x0c = fire share (regions spec §4).

### 5.3 Base-height grid

A 257 × 257 float grid h[i][j] = H(256zx + i, 256zy + j) for i, j = 0..256 inclusive, x in
the outer loop (so the extra row and column come from the neighbouring zones' ground).

### 5.4 Surface pass (per column)

x in the outer loop, y inner, over the 256 × 256 columns. With h = h(x, y):

1. **Steep** = |h − h(x+1, y)| > 0.3 or |h − h(x, y+1)| > 0.3 (differences in float,
   magnitude through double).
2. P = M(x, y). (The pass counts columns with P > 0.25; open question 3.)
3. **Ridge pattern** T:
   v1 = 1 − |N(0.04x + 432, 0.04y + 432) + 0.05·N(0.08x + 432, 0.08y + 432)| (sum in double),
   v2 = 1 − |0.05·N(0.08x + 4234, 0.08y + 234) + N(0.05x + 423432, 0.05y + 54352)|,
   T = (1 − v2³)² + ((1 − v1³)² + 0).
4. If Lz has type 6 or 13: k = min(2·bump(f(Lz, x, y)), 1), k′ = 1 − (1 − k²)²,
   m = sat(1.3·N(0.01x + 985, 0.01y + 98584) + 1), P′ = smooth(m)·k′·0.4 + P; else P′ = P.
5. G = P′·T; **cover** = smooth(sat(1.25·G)) (rock cover).
6. **Surface** S = f32(f32((N(0.01x + 34432, 0.01y + 8992) + 1.5)·60)·P) + 8G + h
   ((n + 1.5)·60 in double; note P, not P′); zs = trunc(S).
7. **Colour**: c = surface colour(x, y, zs) with type t (§6.1). If cover > 0.5, t = 6.
   c′ = trunc(c·(1 − cover) + Rk(x, y, zs)·cover) per channel (c as byte values).
8. Column bottom = zs; the pass tracks the zone's lowest and highest zs.
9. r8 = min(8P′, 1).
10. **Blocks**: block at zs: if steep, (c′, t); else
    trunc(Cl(x, y, zs)·(1 − r8) + (c′ + Rk(x, y, zs))·(0.5·r8)) with type 6 (§6.7).
    Block at zs + 1: (c′, t). So the ground's top face is at z = zs + 2, and in flat ground
    the block under the top one is cliff-coloured (visible only on steps, where the steep
    rule colours it like the top instead).
11. If t = 4, a grass counter is incremented (used later, open question 3).
12. Column +0x14 = zs − 8.

### 5.5 Ravines (same column, after §5.4)

1. Lr = (N(0.01x + 435, 0.01y + 847) + 1)·20 + h. If S ≤ Lr, skip to §5.6.
2. v = 1 − |N(0.005x + O178, 0.005y + O17c) + N(O178 + 0.01x, O17c + 0.01y)|·4; if v < 0,
   skip to the next column (§5.6 is skipped too).
3. u = 1 − v²; w1 = 1 − u²; w2 = 1 − u⁴.
4. t = sat((10·N(0.03x + 7635, 0.03y + 123847) + ((S − Lr) − 20))/10); k = 1 − (1 − t²)²;
   a7 = (1 − t)⁷.
5. **top** = ((20P + 4)·w1 + Lr)·(1 − a7) + (S + 2)·a7;
   mfl = 6·T·P + Lr; **floor** = (1 − w2)·S + w2·mfl + 2.
6. If mfl < S − 2: for i = 0, 1 (z = zs + i): read the block (§1.4) and write
   (b·k + Rk(x, y, trunc(i + S))·(1 − k) per channel, keeping b's type byte).
7. If top ≤ floor, skip to the next column.
8. Carve: for z = trunc(floor) while z < top: write water (0, 0, 0, type 2) if z < 1, else
   air (0, 0, 0, 0x40 if top < S else 0).
9. Floor block at zf = trunc(floor − 1): c2 = surface colour(x, y, zf); type 6 if
   cover > 0.5 or k > 0.5; c3 = trunc(c2·(1 − k) + (Rk(x, y, zf) + Cl(x, y, zf))·(0.4k));
   c4 = trunc(c3·(1 − cover) + Rk(x, y, zf)·cover); write c4 at zf.
10. **Ravine spawns**: if top − floor > 2 and (x + 3y) mod 7 = 0: draw; if draw mod 30 ≠ 0
    stop; else draw & 3 selects: 0 → (if top ≤ S) a draw & 3 = 0 adds a tree-like record
    (vegetation spec §4) with a random rotation draw; 1 → (if top ≤ S) a creature (object
    0x5adb20) whose kind is chosen by draw mod 10 (and draw mod 100 for one case); 2 →
    (if top ≤ S) a creature of kind 59; 3 → an entry from 0x5a6b60 at (x, y, trunc(floor + 2)).
    These draws consume the zone's random stream (see open question 2).

### 5.6 Lake fill (same column)

If Lz has type 4 and f(Lz, x, y) < 0.25: for z = trunc(Lz.h − 25) downwards while
z ≥ column bottom: if the block there reads as air or water, write water
(255, 255, 255, 0x82); otherwise write (Rk(x, y, z), type 3).

### 5.7 After the column passes

The height grid is freed; the zone copies its 16-byte entry from the region table; then
the post passes run (overview spec §5). The last generation step re-computes block light
for the whole zone (0x59a0e0 over the zone's columns), as alpha-rendering.md §9.2
specifies.

---

## 6. Colours

All colour routines return three floats (R, G, B) on a 0..255 scale. "clamp" means: each
channel below 0 becomes 0, above 255 becomes 255. Bytes are trunc of the floats.

### 6.1 Surface colour (0x5c56e0), with moisture mo and warmth wm of the column

Start: type t = 4, colour C = grass(x, y) (§6.2), rk = sat((10R(x, y) − 0.3)·1.5).

1. **Hot rock.** If wm > 0.8: a = min((wm − 0.8)/0.1, 1); n = sat(N(0.03x, 0.03y));
   b = (1 − (1 − n²)²)·0.8·a; C ← C·(1 − b) + Rk(x, y, z)·b.
2. **Desert.** If mo < 0.2 and wm > 0.75: t = 9; d = sat((1 − mo/0.2)·(wm − 0.75)·4·10);
   if L(x, y) has type 4: g = sat(f(L, x, y) − 0.5), d ← d·(1 − (1 − g²)²); d ← d·rk;
   if d > 1 then d = 1, else if d < 0.5 then t = 4; C ← C·(1 − d) + sand(x, y)·d.
3. **Beach.** g = sat(N(0.01x + 854, 0.01y + 985) + (15 − z)/10 − 0.5); g′ = 1 − (1 − g²)²;
   if wm < 0.2: g′ ← g′·(wm/0.2). If g′ > 0: C ← C·(1 − g′) + beach(x, y)·g′, t = 9.
4. **Snow.** If wm < 0.3: k = min(1 − (wm − 0.29)/0.01, 1); t = 10;
   C ← C·(1 − k) + snow(x, y)·k.
5. **Fire.** F = fire(x, y); if F > 0: C ← C·(1 − F) + Rk(x, y, z)·F; if F > 0.5, t = 12.
6. Output bytes trunc(C), type byte t OR 0x20.

### 6.2 Grass (0x5ee540)

a = unit(N(0.04x, 0.04y)); p = 120·N(0.005x + 45645, 0.005y + 456456);
k = 80·N(0.02x + 89648, 0.02y + 1649) + 1; e = 30·N(0.005x + 342, 0.005y + 23423);
R = (k·a + 200(1 − a)) − max(p, 0); G = (120a + 230(1 − a)) + e; B = (k·a + k(1 − a)) + p;
clamp. (Sums in the order written.) Alpha's grass is therefore yellow-green with little blue.

Test vectors: grass(0, 0) = (109.60842895507812, 207.68994140625, 0);
grass(8396928, 8396928) = (163.84413146972656, 224.4915313720703, 0).

### 6.3 Sand (0x5f9bc0)

a80 = 80·unit(N(O274 + 0.03x, O278 + 0.03y)); b = unit(N(O27c + 0.003x, O280 + 0.003y));
g = 40·N(0.01x + 493, 0.01y + 789) + 200;
R = ((1 − b)·255 + 255b) + a80; G = ((1 − b)·g + 150b) + a80; B = ((1 − b)·100 + 50b) + a80;
clamp.

### 6.4 Beach (0x5c4520)

a80 = 80·unit(N(O274 + 0.03x, O278 + 0.03y)); b = unit(N(O27c + 0.01x, O280 + 0.01y));
R = ((1 − b)·240 + 240b) + a80; G = ((1 − b)·240 + 180b) + a80; B = ((1 − b)·100 + 50b) + a80;
clamp. (Shares two noises with sand, at a finer second scale.)

### 6.5 Snow (0x5f9e50)

a = unit(N(0.04x, 0.04y)); R = 190a + 100(1 − a); G = 220a + 180(1 − a); B = 255a + 255(1 − a);
clamp. Test vectors: snow(0, 0) = (132.31939697265625, 194.3641815185547, 255);
snow(1000, 2000) = (138.51609802246094, 197.11827087402344, 255).

### 6.6 Rock Rk(x, y, z) (0x5f9620)

1. Strata v = unit((N(0.1y + 98984, 0.4z) + N(0.1x, 0.4z))·0.5) (z·0.4 in double).
2. Base by warmth wm: default (120, 120, 130). If wm < 0.5: t = 2wm, base =
   (10(1 − t) + 160t, 20(1 − t) + 160t, 50(1 − t) + 170t). Else if wm > 0.75:
   t = 4(wm − 0.75); n = N(0.001·t + 6544, 0.001y + 123) (the first argument really uses t);
   base = (120(1 − t) + 200t, 120(1 − t) + (20n + 130)t, 130(1 − t) + 100t).
3. C = base + 160v per channel, and B += 10·(N(O22c + 0.005x, O230 + 0.005y) + 1); clamp.
4. **Fire tint** F = fire(x, y) > 0: s = min(0.1·N(0.05x, 0.05y) + N(0.02x, 0.02y), 1);
   u5 = (1 − |s|)⁵; R ← R(1 − F) + (200u5 + 10)F; G ← G(1 − F) + ((N(0.1x, 0.1y) + 1)·u5·50 + 10)F;
   B ← B(1 − F) + 10F. (Not clamped again.)

### 6.7 Cliff Cl(x, y, z) (0x5c7140)

1. v = unit((N(0.1y + 98984, 0.4z + 8437) + N(0.1x, 0.4z))·0.5).
2. R = 60v + (20·N(O24c + 0.01x, O250 + 0.01y) + 180) + 20·N(O25c + 0.01x, O260 + 0.01y);
   G = 60v + (20·N(O254 + 0.01x, O258 + 0.01y) + 100) + 20·N(O264 + 0.01x, O268 + 0.01y);
   B = (0·v + 60) + 20·N(O26c + 0.01x, O270 + 0.01y); clamp. (Noise evaluation order:
   O25c, O264, O26c, O24c, O254.)
3. If wm > 0.75: t = 4(wm − 0.75); C ← C(1 − t) + Rk(x, y, z)·t·0.9.
4. If wm < 0.2: s = 1 − 4wm; C ← C·(1 − s) + (Rk(x, y, z) + snow(x, y))·(0.5s).
5. If fire F > 0: C ← C(1 − F) + Rk(x, y, z)·F.

### 6.8 Water and implicit colours

Water blocks carry (255, 255, 255) or (0, 0, 0) and are coloured by the mesher from their
light (alpha-rendering.md §9.5). Blocks below a column are drawn by the mesher from
0x5f9620 = Rk (same reference).

---

## 7. Checking against the running game

* **Colours**: with a 32-bit DLL call 0x5ee540, 0x5f9e50 (no world needed: cdecl-like with
  an output pointer, x, y) at the test inputs; call 0x5f9620 / 0x5c7140 / 0x5c56e0 on the
  live world W for a few columns and compare.
* **Heights**: call 0x5c5e20 (thiscall, ECX = W, x, y, zone or 0) and 0x5effa0 for columns
  around the player; compare with a reimplementation fed with the dumped region cells and
  location records (until the locations spec exists).
* **Blocks**: dump a generated zone's columns (zone+0xa8; 32-byte records, then the block
  arrays) right after load; compare bottom, count and the two surface blocks per column.
* Visual: Alpha terrain shows yellow-green grass, grey-brown cliffs with horizontal strata
  (the 0.4z noise), deep narrow ravines with water at their bottom below sea level, flat
  towns with a regular street pattern, and islands raised around sea locations.

---

## 8. Open questions

1. **Locations** (regions spec open question 1) feed H, M, R, W, q and the colours; until
   they are specified only location-free areas can be reproduced exactly.
2. **Ravine spawns and later passes** draw from the zone's random stream; the exact
   number of draws in the creature and 0x5a6b60 branches was not traced, so every later
   draw in the zone (vegetation) depends on completing that trace.
3. The use of the counters (columns with P > 0.25, grass columns), the min/max zs and
   column +0x14 was not traced (likely structure placement).
4. The type-10 segment helper 0x5c3ef0 (mountain mask) was not traced.
5. The double-precision √ used in §3.4, §4.3 is the CRT `sqrt` (0x68d9c8); assumed
   correctly rounded.
6. Writes that meet bit-7 "keep" blocks only occur when structures run before terrain
   writes; the generator order makes this impossible in a fresh zone as far as traced.

See also: questions 1 and 4 in [alpha-worldgen-locations.md](alpha-worldgen-locations.md) (§11.1 for 0x5c3ef0); question 2 (ravine spawns and post-pass draws) in [alpha-worldgen-caves.md](alpha-worldgen-caves.md) §3–§8.
