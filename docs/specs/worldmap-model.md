# The world map's region model and its markers (0x2eb7b0)

Behavioural specification of how Steam Cube World builds one **map region**: the small voxel
model the world map draws for a square of the world at one zoom level, and the lists of
markers (places, missions, lands, flight points, castle cells, region borders) that the map
overlay reads. Written from the decompiled executable, its assembly and the binary's
constants. Routines are cited by RVA (image base 0x140000000).

The overlay that draws the markers is `map-markers.md` (0x2829c0). The lore markers' use is
`lore-and-artifacts.md` §4.5 and §5.4. Every terrain and building routine this one calls is
already specified in the world generator specs; this file says which one is called, with
which arguments and in which order, and what is done with the result. Sections are cited as
"terrain §x" (`worldgen-terrain.md`), "height §x" (`worldgen-terrain-height.md`), "zone §x"
(`worldgen-terrain-zone.md`), "structures §x" (`worldgen-structures.md`), "nature §x"
(`worldgen-structures-nature.md`), "mason §x" (`worldgen-structures-mason.md`) and "tables Tn"
(`worldgen-structures-tables.md`).

Nothing here draws rand(). The whole model is a function of the world seed (through the
generator routines), the region (rx, ry) and the zoom level L.

---

## 0. Conventions

* Float arithmetic is IEEE single precision, round to nearest, each operation rounded before
  the next, in the order written. "(double)" marks the few steps done in double.
* **fdiv(a, b)** is integer division rounding toward minus infinity (the binary's
  "(a + 1) / b − 1 for negative a"); **a / b** written plainly between integers is C
  division, truncating toward zero. The difference matters below and each use says which.
* **floor(f)** of a float: trunc32(f), minus one when f is negative and not integral.
* **pmod(a, m)** = (m + (a C-rem m)) C-rem m, the non-negative remainder.
* **R(n)** and **H(n)** are terrain §1.1–1.2; "R taken unsigned" as there.
* A **colour** is three floats on the generator's doubled scale (0…510) plus glow, type and
  flags bytes (zone §4). A **voxel** stores only three bytes. **Storing** colour c in a voxel
  (0x2fb80, and the same code inlined): each channel v = c × 0.5, then 0 if v < 0, 255 if
  v > 255, then trunc64 and keep the low 8 bits. A voxel of bytes (0, 0, 0) is **empty**; a
  stored colour that happens to round to (0, 0, 0) is empty too.
* The **column values** at a point (x, y) are the height pipeline's intermediate values
  (height §1, steps 1–15): Nb, P (the reach-gated place 0x35ce0, terrain §3.6), lake, base,
  roads, lavaRegion, forestRegion, wet, river, lava, swamp, highlands, dryRegion, dryness,
  vegetation; plus **warmth** = 0x2d9b60(Nb, P.kind, P.weight) and **kind27** =
  0x2abb60(Nb, P.kind, P.weight) (zone §4.1). **height(x, y, m)** is step 16 (0x2b4450) with
  those values and mode m; **H0(x, y)** is the full pipeline 0x35ea0 with mode 0.
* Model indices: x index i, y index j, z index k. Voxel (i, j, k) of the model is at byte
  ((sizeY × k + j) × sizeX + i) × 3 of the model's data (model +0x68; sizes at +0x88, +0x8c,
  +0x90). Writes outside the model are ignored.

---

## 1. Requests (0x2f7f50) and storage

The world map (class `WorldMap`) asks for regions around a point at zoom level L (0x2f7f50,
called from the map's update 0x83c70). Nothing happens when L is not one of the map's levels
(the level table at map +0x170, 0x68 bytes per level) or when world +0x210 is null.

* **unit** u = trunc32(2.0^L × 4.0), at least 1 (constants 0x4ac9d0, 0x4aca20): levels
  0…5 give 4, 8, 16, 32, 64, 128 blocks per map unit.
* **Region side** B = 16 × u blocks (0x5469a0 = 16 units per region).
* The point's region (cx, cy) = (fdiv(x, B), fdiv(y, B)), x and y the point's position in
  dots divided by 65536 truncating toward zero (32-bit).
* For rx = cx − 8 … cx + 7 (outer) and ry = cy − 8 … cy + 7 (inner) (0x5469a4 = 8): when the
  map's region table (map +0x188, keyed by (rx, ry, L)) has no model for it, it is a
  candidate with d = (rx − cx)² + (ry − cy)².
* The candidates are sorted by d (0x2ea0d0) and only the first 8 are kept (0x2f7de0). Those
  are built in parallel (0x2ea7e0 / 0x2f21f0 → 0x2eb7b0), one call per region.
* **Reach** r, the number of 16-block cells around a column whose objects can touch it: if
  u < 32, k = 32 / u and r = 2 when k > 1, else k; otherwise r = 1. So r = 2 for levels
  0–2 and 1 above.

Each call builds a 0x90-byte **region record** (constructor 0x2eab60) and, under the map's
lock (map +0x1a0), stores it in the region table under (rx, ry, L).

| Offset | Contents |
|---|---|
| +0x00 | the voxel model (0x1739a0 on the world's model table) |
| +0x08 | i32 **lowest** / u (C division): the model's base, in units |
| +0x10 | i32 the build's start time (timeGetTime) |
| +0x18 | list of **place** records (§6.4–6.6) |
| +0x28 | list, left empty |
| +0x38 | list of **mission** records (§6.1) |
| +0x48 | list of **land** records (§6.2) |
| +0x58 | list of **flight point** records (§6.3) |
| +0x68 | list of **castle cell** records (§6.6) |
| +0x78 | map region key → list of **border** points (§3.1) |
| +0x88 | i32 build time in ms |

Lists are appended at the end, in the order this file gives.

---

## 2. Pass 1: column tops (lines of the routine before 0x2ec1f0)

The model covers the region plus a border of 3 units on each side (0x5469a8 = 3): columns
a = −3 … 18 and b = −3 … 18, so the model is 22 × 22 columns. Column (a, b) is at world
position **X = u × a + B × rx, Y = u × b + B × ry** (whole blocks).

For a = −3 … 18 (outer), b = −3 … 18 (inner):

1. Column values at (X, Y). **h4** = height(X, Y, 4).
2. If h4 > 0 and river > 0.5: h4 = the water level 0x2e6420 (zone §4.9) with base and P.
3. **top** = floor(h4 + 0.999) (constant 0x473350); top = 0 when h4 ≤ 0 (tested on the h4
   of step 2).
4. **Extra** e = 2. For dx = −r … r (outer), dy = −r … r (inner): cell (cx, cy) =
   (fdiv(X, 16) + dx, fdiv(Y, 16) + dy) (0x2f7ed0); Q = the cell point 0x2d8630 (cx, cy)
   (structures §6.1); PQ = 0x35ce0 at Q; T = 0x2d53d0 (cx, cy, Q, PQ), the cell object
   (structures §6.3). Skip when T = 0 or its reach (tables T6, 0x25f790) is below |dx| or
   below |dy|. Otherwise e = max(e, e′) with:
   * T = 9, or T outside 1 … 15: e′ = the building height of T (tables T4, 0x25f6a0); +16 for
     T = 43 or 47; for T = 52 + 0x2d9db0 (cx, cy), where 0x2d9db0 = R(K(cx, cy) + 0x5d0ba)
     mod 40 + 100 (R unsigned, unsigned mod); +15 for T = 66; +10 for T = 68; +8 for T = 67.
   * T in 1…15 except 9: e′ = 0x2d9e70 (cx, cy) = R(K(cx, cy) + 0x12ce32c) mod 10 + 12
     (unsigned); T = 2: e′ = trunc32(float(e′) × 1.5); T = 8: e′ × 3; T = 11 or 13: 40;
     T = 12 or 14: 80. (The routine also computes 0x2d8630 (cx, cy) again, or the cell's
     centre for T = 8, 11, 12; the result is not used.)
5. **Rock spans.** If the place P is one with rock spans (0x2b8010: kinds 24, 45, 46; zone
   §3.2 step 5): SPH = 0x2ba200(P), raised to 10 when ≤ 10; the anchor A = 0x2a9df0
   (fdiv(X, 1024), fdiv(Y, 1024), P.kind, P.big) (structures §6.2), taken **without** the
   0x2aa270 gate the zone generator uses; s = the rock-span top 0x2bab80 (zone §5.8) at
   (X, Y) with P, A, h = h4 and SPH. e = max(e, s − floor(h4)).
6. **h1** = height(X, Y, 1) (the column values are recomputed; they are the same).
7. Column peak = max(trunc32(h1), top) + e.
8. Keep **lowest** = the smallest top and **highest** = the largest column peak over all
   22 × 22 columns (the first column sets both). Store **G(a, b)** = top.

Then the model is sized (0x16ff70) 22 × 22 × sizeZ with **sizeZ** = min(highest / u −
lowest / u + 8, 255) (C divisions), all voxels empty.

**Haze weight** w = (L − 2) × 0.2, then 1 if w ≥ 1, then 0 unless w > 0 (constants 0x42e700,
0x437f60). Used in §4.7.

---

## 3. Pass 2: the columns (0x2ec1f0 … 0x2f0a9a)

For a = −3 … 18 (outer) and b = −3 … 18 (inner), with X, Y as in pass 1 and voxel column
(i, j) = (a + 3, b + 3):

* t = G(a, b); **z0** = t / u − lowest / u (C divisions). z0 is a running "top" that later
  steps raise or lower; every step below reads its current value.
* **Rg** = the region of (X, Y), 0x2ada20 (terrain §2.6); **biome** = 0x2b9080(Rg).

### 3.1 Region borders

Only for a and b in 0…15: if any of the regions 0x2ada20 at (X + u, Y), (X, Y + u),
(X + u, Y + u), (X − u, Y), (X, Y − u), (X − u, Y − u), checked in that order, differs from
Rg, append the point (X, Y, t) (three i32) to the list kept for key Rg in the border map
(+0x78; 0x2ea440 finds or inserts the key). One point per column at most.

### 3.2 Column values

Column values at (X, Y), in the height pipeline's order, then: dryRegion, the unused
0x2d3c50(Nb), warmth, dryness, highlands, vegetation, kind27; **h0** = height(X, Y, 0); the
neighbours' heights **hW, hE, hS, hN** = H0 at (X − 1, Y), (X + 1, Y), (X, Y − 1), (X, Y + 1),
in that order.

**lo**, the lowest voxel to fill: lo = z0, then lowered to each neighbour's value
G(a′, b′) / u − lowest / u + 1 (C divisions), for the west neighbour (a − 1, b) when a > −2,
the south one (a, b − 1) when b > −2, the east one (a + 1, b) when a < 17, the north one
(a, b + 1) when b < 17 (min each time).

### 3.3 Water columns

A column is **water** when t < 1 or river > 0.5. Then its colour c is fixed for the whole
column:

1. d = h0, but 200 if h0 > 200. q = float(|d|) × 0.005 + 1 (the absolute value through
   double); q = 0 unless q > 0. k = float(|1 − q|) (through double); k = 1 if k > 1.
2. If t > 0: s = (river − 0.5) × 3, 1 if s ≥ 1; k = k × (s × 3 × s − (s + s) × s × s).
3. g0 = 180 − k × 100. c = (0, g0, 255), glow 0, type 2, flags 0.
4. If d > 0: f = d × 0.01; f = 0 unless f > 0; f = 1 unless f < 1.
   c.g = (220 − g0) × forestRegion × f + g0; c.b = 255 − forestRegion × f × 80.
   If also u > 32: p = the snow colour (0x2d3d90, zone §4.7) when the snow test 0x2b4320
   (X, Y, warmth, hW, hE, hS, hN) holds, else the block at height 0x2d8e00 (zone §4.2) at
   (X, Y) with z = t, h = h0, hW, hE, hS, hN, highlands, warmth, dryness, vegetation, 0,
   roads, lavaRegion, forestRegion, kind27, lava, swamp. m = (u − 32) × 0.0025, 1 if
   m ≥ 1. c.r = c.r + m × (p.r − c.r); c.g likewise; c.b = c.b + m × (p.b − c.b).

### 3.4 The fill

If the column is land, or t > 0 and u ≥ 128: z0 = z0 + 1.

For k = lo … z0 (nothing when lo > z0): the block height of voxel k is zb = t − u × (z0 − k).
* Water: store c at (i, j, k).
* Land: c = 0x2d8e00 at (X, Y) with z = zb, h = h0 and the arguments of §3.3 step 4. Then,
  only when highlands > 0.5 and u < 128: gc = cliffs 0x2a2c50 (X, Y, P.kind, P.weight)
  (height §8.9); if gc > 0:
  1. low = height(X, Y, 2). zf = float(zb).
  2. c1 = 0x2d8e00 at (X, Y) with z = floor(h0) and otherwise the same arguments.
  3. zc = trunc32(zf − 1). r = the buried colour from scratch 0x2f39e0 (X, Y, zc) (zone §4.8).
  4. α = 1 − ((h0 − zf) − 1) × 0.3, clamped to 0…1 (0 unless ≥ 0, then 1 unless ≤ 1).
     m = r + α × (c1 − r) per channel (r.x + α × (c1.x − r.x)).
  5. β = 1 − (zf − low) × 0.3, clamped to 0…1.
  6. e = 0x2f39e0 (X, Y, zc) when vegetation > 0.5 or the low shore test 0x2b7730 (X, Y, zf)
     holds (height §8.7), else the surface colour from scratch 0xbbfe0 (X, Y, zc).
  7. c.b = m.b + (e.b − m.b) × β; c.r = m.r + β × (e.r − m.r); c.g = m.g + (e.g − m.g) × β.

  Store c at (i, j, k). (Unlike the zone's cliff pass, nothing is carved here.)

### 3.5 Rock spans, snow, road pits

In this order, after the fill:

1. **Rock spans** (P with rock spans, as pass 1 step 5): SPH and A as there; bottom = 0x2ba2b0
   and top = 0x2bab80 (zone §5.8) at (X, Y) with P, A, h = h0, SPH. If top > h0:
   for k = floor(bottom) / u − lowest / u + 1 … floor(top) / u − lowest / u + 1: store the
   span colour 0x2ba5f0 (zone §5.8) at (X, Y) with **z = t** (the column top, the same for
   every voxel), P and SPH.
2. **Snow**: if roads < 0.5, the column is land, vegetation < 0.5 and the snow test 0x2b4320
   (X, Y, warmth, hW, hE, hS, hN) holds: store the snow colour at (i, j, z0).
3. **Road pit**: only when u < 2048. If 0x2ae6d0 (X, Y, P, roads) (zone §5.4 step 3) holds,
   with its outputs bottom and top: for k = floor(bottom) / u − lowest / u + 1 up to, but not
   including, floor(top) / u − lowest / u + 2: store the buried colour 0x2a3140 (zone §4.3) at
   (X, Y) with z = t, highlands, warmth, dryness, lavaRegion, forestRegion, kind27.

### 3.6 Objects near the column

For dx = −r … r (outer) and dy = −r … r (inner):

* (cx, cy) = (fdiv(X, 16) + dx, fdiv(Y, 16) + dy); p = (px, py) = 0x2d8630 (cx, cy); Pp =
  0x35ce0 at p; T = 0x2d53d0 (cx, cy, p, Pp). Skip the cell when T's reach (tables T6) is
  below |dx| or below |dy|. "**Own cell**" means dx = dy = 0.
* "**Cell centre**" is (16 cx + 8, 16 cy + 8). "**fill k1 … k2 with c**" stores c at
  (i, j, k) for each k from k1 to k2 inclusive.
* **zv(f)** converts a block height to a voxel index: trunc32(f / float(u) −
  float(lowest / u)) for a float f; for an integer n, n / u − lowest / u.

The rules below run in this order for each cell. Unless a rule says it ends the cell, the
next rules still run. Type 0 (no object) has reach 0 and matches no rule.

#### 3.6.1 Trees, T = 1 … 8 (any unit)

For T = 8, p becomes the cell centre.
1. rad = float(0x2d9ed0 (cx, cy)) × 0.75, where 0x2d9ed0 = R(K(cx, cy) + 0x2fbcc94) mod 10 +
   20 (unsigned). T = 3: rad × 0.75 again; T = 8: rad + rad.
2. d2 = float(px − X)² + float(py − Y)² (x term first). Skip the tree when rad² < d2.
3. Column values at p, then hp = height(px, py, 4) with lava and swamp passed as 0, and
   kind27 at p. Write warmth′, dryness′, forestRegion′, kind27′ for p's values.
4. f = float(0x2d9e70 (cx, cy)); th = trunc32(f + f); T = 8: th = th × 2.
5. q = d2 / rad². If warmth′ ≥ 0.7 and dryness′ < 0.3: q = 1 − (1 − q) × 0.5.
6. **Leaf span** [kb, kt]:
   * T = 2: th = trunc32(float(th) × 1.5); kb = zv(float(th / 3) + hp);
     top = (hp + float(th)) − (sqrtf(q) × 2 × float(th)) / 3.
   * else: m = q × float(th) × 0.25; kb = zv((float(th / 2) + hp) + m); top = (hp + float(th)) − m.
   * kt′ = zv(top); kb = kb + 1; kt = kt′ + 1.
   * T = 5 or 7: kb = kt′; then with ex = fdiv(px − X, u), ey = fdiv(py − Y, u): if |ex| ≠ |ey|
     and ex ≠ 0 and ey ≠ 0, kb = kt′ + kt (so no leaves in that column).
   * T = 4: n = R(57 Y + 0x714ef + X) (unsigned). If n mod 4 = 0: s = (th / u) / 2 + 1, 1 if
     s < 2; kb = kb − (n / 4) mod s (unsigned).
   * If u > 16: kb = max(kb, z0 + 1), kt = max(kt, z0 + 1).
7. **Trunk**: rt = rad × 0.2, 1 if rt ≤ 1. The column gets a trunk when d2 < rt², or when
   (X / u, Y / u) = (px / u, py / u) (C divisions). Trunk colour (200, 150, 100), (255, 255,
   255) for T = 3: store it for k = z0 + 1 … kb − 1.
8. **Leaves**: two colours A and B from the leaf routine at (px, py): T = 2 the fir colours
   0x2ae840 (with forestRegion′, kind27′); T = 3 the birch colours 0x2ae3a0 (warmth′,
   forestRegion′, kind27′); otherwise 0x2afbf0 (warmth′, dryness′, forestRegion′, kind27′)
   (nature §2), at the point p. Leaf = A × 0.5 + B × 0.5 per channel. If kt ≥ kb: for k = kb … kt store
   the leaf, and when warmth′ < 0.15 store the snow colour over it.

#### 3.6.2 The rest, only when u < 256

When u ≥ 256 the cell ends here.

**Realm** (σ, ρ): σ = ρ = 0. When T ≠ 0 and T ≥ 13: (zx, zy) = (fdiv(X, 1024),
fdiv(Y, 1024)) — the **column's** place cell, not the object's; σ = 0x2bb060 (zx, zy,
bigCell(zx, zy)) (the realm, structures §6); ρ = 0x10cee0(σ) (the realm kind). The building
tests and colours below take (σ, ρ, biome).

Then, by type:

1. **T = 52, round tower.** rr = 0x2d9e10 (cx, cy) = R(K(cx, cy) + 0x255509) mod 10 + 20
   (unsigned). dX = (double)(16 cx) + 0.5 + 8.0 − (double)X, dY likewise with cy and Y;
   d2 = trunc32(dY² + dX²) (in double). If rr² < d2 the cell ends. Otherwise:
   * top = 0x2d87f0 (cx, cy, 52) + 0x2d9db0 (cx, cy) − 2 rr (structures building base
     height; 0x2d9db0 as in pass 1); kt = zv(top).
   * If (rr − u)² ≤ d2 (the rim): kt = kt + 1, except when the battlements test 0x25e940
     (52, σ, ρ, biome) holds and fdiv(X, u) + fdiv(Y, u) is odd.
   * Fill z0 + 1 … kt with the wall colour 0x2600f0 (52, cx, cy, σ, ρ, biome) (tables T11).
   * If the height test 0x25e9a0 (52, σ, ρ, biome) holds (tables T7): kr = zv((1 −
     sqrtf(float(d2)) / float(rr)) × float(2 rr) + float(top)); fill kt … kr with the roof
     colour 0x25f840 (52, cx, cy, σ, ρ, biome) (tables T14).
   * The cell ends.
2. **T = 66.** ex = float(16 cx − X + 8), ey = float(16 cy − Y + 8) (integer differences);
   d = trunc32(ey² + ex²). Only when d ≤ 1024: kt = max(zv(0x2d87f0 (cx, cy, 66) + 10), z0);
   colour 0x2600f0 (66, …); if d ≤ 64: kt = kt + 1 and the colour is (140, 150, 180). Fill
   z0 … kt. Whatever d is, go to rule 6 next.
3. **T = 69 or 70.** rad = 0x25eca0(σ) (buildings §12; 28 for T = 70). ex, ey as in rule 2;
   when trunc32(ey² + ex²) ≤ rad²:
   * lift = 3 (T = 69) or 10 (T = 70); top = 0x2d87f0 (cx, cy, T) + lift; posts = false.
   * If u < 16: ax = |fdiv(X, u) − fdiv(16 cx + 8, u)|, ay = |fdiv(Y, u) − fdiv(16 cy + 8, u)|,
     k1 = trunc32(float(rad) × 0.7) / u, k2 = (rad − 1) / u. If (ax = k1 and ay = k1) or
     (ax = k2 and ay = 0) or (ax = 0 and ay = k2): top = top + 15; if the water line test
     0x25ec30 (T, σ, ρ, biome) holds: v = trunc32(wl(lift + 10, 10) + h0) and top =
     max(top − 15, min(top, v)); posts = true. Here **wl(a, b)** = N(0.05 X + 9076,
     0.05 Y + 8998) × float(b) + float(a) (0x2d3690, mason §3).
   * kt = zv(top) + 1; c = 0x2600f0 (T, …). For k = z0 … kt: store c, except that the top
     voxel (k = kt) when not posts, the grassy test 0x25ebc0 (T, σ, ρ, biome) holds and
     N((double)X × 0.06 + 7631, (double)Y × 0.06 + 983) > 0 takes the surface colour
     0x2a3e30 at (X, Y) with z = k (the voxel index, passed where a block height belongs),
     h = h0, warmth, dryness, 0, 0, roads, lavaRegion, forestRegion, kind27.
   Then continue with rule 4.
4. **T = 67** (only if not T = 66). If u < 32: m = 16 / u; ix = pmod(fdiv(X, u), m),
   iy = pmod(fdiv(Y, u), m). If (iy = m / 2 and the cell object 0x69c60 at (cx − 1, cy) or
   (cx + 1, cy) is 67) or (ix = m / 2 and the one at (cx, cy − 1) or (cx, cy + 1) is 67):
   store 0x2600f0 (67, …) × (float((ix + iy) C-rem 2) × 0.1 + 0.9) at (i, j, z0 + 1).
   Then go to rule 6.
5. **Other types** (T ≠ 66, 67), in this order:
   a. **Fields, T = 27 … 33**, when roads ≤ 0.5 and river ≤ 0.5: v = X when fdiv(cx, 3) +
      fdiv(cy, 3) is odd, else Y; s = (v / u) C-rem 2 (−1, 0 or 1); store the river bed
      colour 0xbd110 (X, Y, t) (zone §4.6) × (float(s) × 0.05 + 0.75) at (i, j, z0). Then if
      pmod(fdiv(X, u), 3) = 1 and pmod(fdiv(Y, u), 3) = 1 and R(57 Y + X) mod 4 ≠ 0
      (H / 21474 mod 4 ≠ 0, terrain §1.3): z0 = z0 + 1 and store (100, 200, 100) there.
   b. **T = 62**: if t > 0 and (X − 16 cx − 8)² + (Y − 16 cy − 8)² < 256: n = N((double)X ×
      0.25, (double)Y × 0.25); store 0x2f39e0 (X, Y, t) × (float(|n|) × 0.3 + 0.7) at
      (i, j, z0). Then go to rule 6.
   c. **Walls, own cell, T in {16, 26, 50, 51, 55, 57, 61, 63, 64, 65}** (mask 0x3a28c00000401
      from 16): base = 0x2d87f0 (cx, cy, T); c = 0x2600f0 (T, …); kt = zv(base + 1);
      z0 = z0 − 1; corner = false. If u < 17: m = 16 / u, ix = pmod(fdiv(X, u), m),
      iy = pmod(fdiv(Y, u), m):
      * T in {16, 50, 51, 57, 61} (mask 0x220c000000010000) and ix ∈ {0, m − 1} and
        iy ∈ {0, m − 1}: top = base + 10; if 0x25ec30 (T, …): top = min(top,
        trunc32(wl(10, 10) + h0)); kt = zv(top) + 1; corner = true.
      * T in 63 … 65 and (ix ∉ {0, m − 1} or iy ∉ {0, m − 1}): z0 = kt − 1.
      While z0 < kt: z0 = z0 + 1, store c at z0. Then, if not corner and the grassy test
      0x25ebc0 (T, …) holds and N(0.1 X + 89834, 0.1 Y + 12233) > 0.25 (double inputs):
      store the surface colour 0x2a3e30 (X, Y, z = base, h0, warmth, dryness, 0, 0, roads,
      lavaRegion, forestRegion, kind27) at (i, j, kt).
   d. **T = 55** (any cell in reach): ex = float(trunc32(16 cx + 8 − X)) − 0.5, ey =
      float(16 cy + 8 − Y) − 0.5, d = ey² + ex². Unless d > 25 and u < 9: q = d / 25;
      kt = zv((1 − q × q) × 20 + float(0x2d87f0 (cx, cy, 55))) + 2; for k = z0 … kt store the
      rock colour 0x2d3150 (zone §4.5b) at (X, Y, u × k + lowest) with warmth, lavaRegion,
      forestRegion.
      **T = 54**: ex = float(px − X) − 0.5, ey = float(py − Y) − 0.5 (the cell point, not the
      centre), d = ey² + ex². When d ≤ 64 or u > 8: q = d × 0.015625; kt = zv((1 − q × q) ×
      25 + float(0x2d87f0 (cx, cy, 54))) + 2; the same rock fill.
   T = 55 matches both c and d at its own cell; both run, c first. Then rule 6.
6. **Walls and roofs (all types reaching here).** When (the height test 0x25e9a0 (T, σ, ρ,
   biome) holds, or T is 45 … 47 (0x25eb30), or T is 40 … 43 (0x25eb40)) and it is the own
   cell and T ≠ 60:
   * base = 0x2d87f0 (cx, cy, T); bh = the building height of T (tables T4); c = 0x2600f0
     (T, …); top = bh + base; waterline = 0x25ec30 (T, …); if waterline: top = min(top,
     trunc32(wl(10, 10) + h0)). kt = zv(top).
   * If u < 17 and (T is 13, or T in {49, 53, 86, 95} (mask 0x402000000011 from 49)) and
     (ix ∉ {0, m − 1} or iy ∉ {0, m − 1}) with m = 16 / u, ix, iy as in rule 5c: kt =
     zv(base) + 1 (inside the walls only the floor shows).
   * While z0 ≤ kt: z0 = z0 + 1, store c at z0 (so z0 + 1 … kt + 1).
   * z0 = zv(bh + base) (the uncapped top).
   * **Battlements**, when u < 16 and 0x25e940 (T, …): ix = pmod(fdiv(X, u), m). If fdiv(Y, u)
     is even and ((ix = 0 and not 0x25eb60 (T, object at (cx − 1, cy))) or (ix = m − 1 and
     not 0x25eb60 (T, object at (cx + 1, cy)))): a crenel. Otherwise iy = pmod(Y / u, m) (C
     division here) and, if fdiv(X, u) is even, a crenel when (iy = 0 and not 0x25eb60 (T,
     object at (cx, cy − 1))) or (iy = m − 1 and not 0x25eb60 (T, object at (cx, cy + 1))).
     (0x25eb60 is "joins a tower", tables T7.) A crenel: z0 = z0 + 1, store c at z0.
   * **Roof**, when 0x25e9a0 (T, …) holds: joins = 0x2d86e0 (cx, cy); 0x29d170 (cx, cy, T,
     joins) adjusts them (buildings §1); style = 0x25ea50 (σ, ρ, biome) (tables T9);
     rh = 0x2d34b0 (X + u / 2 − 16 cx, Y + u / 2 − 16 cy, T, joins, style) (the roof height);
     rt = rh + bh + base; if waterline: rt = min(rt, trunc32(wl(10, 10) + h0)); kr = zv(rt) +
     1; while z0 < kr: z0 = z0 + 1, store the roof colour 0x25f840 (T, cx, cy, σ, ρ, biome).
7. **Rocks and boulders, T = 9 … 15** (T ≠ 34). For T = 11 … 14, p becomes the cell centre.
   rr = float(0x2d9ed0 (cx, cy)), rh = float(0x2d9e70 (cx, cy)); T = 11 or 13: rh = 40,
   rr = 32; T = 12 or 14: rh = 80, rr = 32. If rr² < float(px − X)² + float(py − Y)² the cell
   ends. Column values at p; hp = max(height(px, py, 0) with lava and swamp 0, 0).
   * **T = 9**: kt = zv(hp + rr) + 1; if kt < z0 the cell ends. For k = z0 … kt: zb = u × k +
     lowest; when the inside-a-rock test 0x146180 (X, Y, zb; px, py, trunc32(hp), trunc32(rr),
     trunc32(rr), trunc32(rh)) (nature §10: centre (px, py, trunc32(hp)), radii (rr, rr, rh)) holds, store the rock colour 0x2d3150 at
     (X, Y, zb) with the column's warmth, lavaRegion, forestRegion. The cell ends.
   * **T = 10 … 15**: n = N((double)X × 0.01 + 4394, (double)Y × 0.01 + 8974); kt =
     zv(n × 10 + hp + rh) + 1. For k = z0 … kt − 1, with zb = u × k + lowest: the rock colour
     at (X, Y, zb) when highlands at p > 0.5 or T is 11 … 14, else the ground colour 0x2ab410
     (zone §4.5a) at (X, Y, zb) with the column's warmth, dryness, lavaRegion, forestRegion,
     kind27. Then at kt: the snow colour when the snow test 0x2b4320 (X, Y, warmth, 0, 0, 0, 0)
     holds; otherwise, when the column's vegetation < 0.5, the surface colour 0x2a3e30
     (X, Y, z = t, h0, warmth, dryness, 0, 0, roads, lavaRegion, forestRegion, kind27). The
     cell ends.
8. **T = 34**, own cell, roads < 0.45: if pmod(fdiv(X, u), 4) = 2 and pmod(fdiv(Y, u), 4) = 2
    and R(57 Y + X) mod 10 ≠ 0 (unsigned): z0 = z0 + 1 and store (100, 100, 100) there.

All "column values" above (warmth, dryness, roads, …) are those of the column (X, Y) of §3.2
unless primed (p's). The tests 0x25e940, 0x25e9a0, 0x25ebc0, 0x25ec30 and the colours
0x2600f0, 0x25f840 take (T, σ, ρ, biome) as the zone generator's buildings do (structures
§6, tables T7, T11, T14).

### 3.7 Haze over the column

When t > 0: ks = t / u − lowest / u, plus 1 for a water column. For every k from ks to
sizeZ − 1 whose voxel v is not empty:

1. o = haze(v) (0x2f40a0) with h = trunc32(h0), the band 0 … 500, warmth and dryness:
   * f = float(h − 0) / float(500 − 0); f = 0 unless f ≥ 0; f = 1 unless f ≤ 1. s = 1 − f.
   * a = (warmth − 0.8) × (1 − dryness) × 8; a = 1 if a ≥ 1; a = 0 if a ≤ 0. ia = 1 − a.
   * kk = s × s × s × 0.8.
   * Lm = v.r × 0.0078125 × 0.299 + v.g × 0.0078125 × 0.587 + v.b × 0.0078125 × 0.114
     (each product left to right, the sum left to right).
   * o.b = ((a × 0 + ia × 140) × Lm − v.b) × kk + v.b;
     o.r = ((a × 130 + ia × 0) × Lm − v.r) × kk + v.r;
     o.g = ((a × 60 + ia × 40) × Lm − v.g) × kk + v.g.
2. The voxel becomes, per channel, trunc64(w × o + v × (1 − w)) keeping the low 8 bits — no
   clamp (w the haze weight of §2; v the old byte as a float).

At levels 0 … 2 w is 0 and the voxels keep their bytes.

---

## 4. Pass 3: shade under overhangs

For every column (i, j) (i outer, then j), walk k from sizeZ − 1 down to 0 with two flags,
seen = false and gap = false: a non-empty voxel is halved (each byte shifted right by one)
when gap is set, and sets seen; an empty voxel sets gap when seen is set. So every voxel
below the first gap under the column's top run is darkened once.

Then the model is finished for drawing (0x170700 with border 3; drawing is CubeVulkan's own)
and **lowest** / u is stored at +0x08.

---

## 5. Records

The place, mission, land and flight records are 0xc0 bytes; the list node holds them from
node +0x10 (which is how `map-markers.md` cites them: node +0x10 = record +0x00).

| Record +0x | Place (+0x18) | Mission (+0x38) | Land (+0x48) | Flight point (+0x58) |
|---|---|---|---|---|
| 0x00 | kind (i32) | landmark kind 0x2b99d0 (i32) | biome (i32) | 0 |
| 0x04, 0x08 | x, y (blocks) | x, y | x, y | x, y |
| 0x0c | z (blocks) | z | z | z |
| 0x10 | 0 | role 0x93480 (i32) | 0 | 0 |
| 0x14 | 0 | level 0x12a200 (i32) | 0 | 0 |
| 0x18 … | 0 | the place's artifact item (0x119660, 0x9e bytes copied by 0x9cd0) | 0 | 0 |

Everything not listed is zero. Castle cell records (+0x68) are 16 bytes: the 4-byte castle
word of 0x2a8290 (its first byte non-zero), x = 16 cx, y = 16 cy, then 0x2ab0a0 (cx, cy).

---

## 6. Markers (after pass 3)

"Inside" means B × rx ≤ x < B × (rx + 1) and B × ry ≤ y < B × (ry + 1).

### 6.1 Missions (u ≤ 256)

A temporary ordered set of place cells guards against duplicates. For px = fdiv(B × rx, 1024)
… fdiv(B × (rx + 1), 1024) (outer, inclusive) and py likewise (inner):
1. big = bigCell(px, py) (terrain §3.1); (qx, qy) = (px, py), each rounded down to even when
   big.
2. sel = 0x2b99d0 (qx, qy, big) (terrain §3.4). Skip when 0.
3. (x, y) = the landmark point 0x362a0 (qx, qy, big) (terrain §3.2). Skip unless inside.
4. Skip when (qx, qy) is already in the set.
5. planner = 0x115df0 for the region 0x2ada20 (x, y) (worldgen-creatures §2). role =
   0x93480 (planner, qx, qy); level = 0x12a200 (planner, qx, qy); item = 0x119660 (planner,
   qx, qy) (lore-and-artifacts §5.1). z = trunc32(H0(x, y)).
6. When role = 14 or u ≤ 64: append the mission record and insert (qx, qy) into the set.

### 6.2 Land name (u ≥ 128)

(lx, ly) = (fdiv(B × rx, 8192), fdiv(B × ry, 8192)); (x, y) = the region centre 0x2b9210
(lx, ly) (terrain §2.1). If inside: append {biome 0x2b9080 (lx, ly), x, y,
trunc32(H0(x, y))}.

### 6.3 Flight point (u ≤ 256)

(fx, fy) = (fdiv(B × rx, 2048), fdiv(B × ry, 2048)), k = 57 fy + fx;
x = fx × 2048 + (R(k + 0x474d6) AND 2047), y = fy × 2048 + (R(k + 0x181f0) AND 2047) (R as a
32-bit pattern). If inside: append {0, x, y, trunc32(H0(x, y))}.

### 6.4 Shrines (u ≤ 512)

For qx = fdiv(B × rx, 1024) … fdiv(B × (rx + 1), 1024) (outer, inclusive), qy likewise
(inner): (sx, sy) = 0x2d3b60 (qx, qy) (structures §6.3); (x, y) = (16 sx + 8, 16 sy + 8). If
inside and the cell object 0x69c60 (sx, sy) is 53: append the place {12, x, y,
0x2d87f0 (sx, sy, 0)}.

### 6.5 Lore and treasure cells (u ≤ 64)

For cx = fdiv(B × rx, 16) … fdiv(B × (rx + 1), 16) (outer, inclusive), cy likewise (inner):
(x, y) = (16 cx + 8, 16 cy + 8); if inside: the lore cell test 0x2b42e0 (cx, cy) → append
{14, x, y, 0x2d87f0 (cx, cy, 0)}; else the crate cell test 0x2b4410 (cx, cy) → append {15,
…} the same way (tables T7).

### 6.6 Castle cells and town buildings (u < 64)

n = B / 16 (C division). For di = 0 … n − 1 (outer), dj = 0 … n − 1 (inner), with
cx = fdiv(B × rx, 16) + di, cy = fdiv(B × ry, 16) + dj:
1. w = 0x2a8290 (cx, cy, 1) (the castle cell, buildings §9). If its first byte is non-zero, append the
   castle record {w, 16 cx, 16 cy, 0x2ab0a0 (cx, cy)} to +0x68.
2. Only when u ≤ 16 and the cell object 0x69c60 (cx, cy) is 22 … 26, 91 or 92:
   * the cell must be the corner of its 3 × 3 town block: 0x2b5760 (fdiv(cx, 3), fdiv(cy, 3))
     = (cx, cy) (structures §6.4), else skip;
   * (zx, zy) = (fdiv(16 cx, 1024), fdiv(16 cy, 1024)); big = bigCell(zx, zy); kind =
     0x2b99d0 (zx, zy, big); anchor = 0x2a9df0 (zx, zy, kind, big) when 0x2aa270 (zx, zy,
     kind, big) ≠ 0, else (0, 0);
   * plot = the town block 0x2af7b0 (cx, cy, kind, anchor) (structures §6.4);
   * when plot > 1 and plot ≠ 13: append the place {plot, 16 cx + 8, 16 cy + 8,
     trunc32(H0(16 cx + 8, 16 cy + 8) + 20)}.

So the place list holds shrines first, then lore and treasure cells, then the town's
buildings, each group in cell order. Place kinds 2 … 11 are the town's inn and shops (the
overlay's icons, `map-markers.md` §1.3.1), 12 shrines, 14 lore buildings, 15 treasure cells.

---

## 7. Oracles

None exists. Neither the project's `Extract/` nor `D:/SteamLibrary/steamapps/common/Cube
World/Extract/` holds a map recording (only zones, spawns, records, rtraces, creature and
item tables, and UI captures). The `ui/` captures are screenshots and cannot check bytes.

**To record** (for the user; a ZoneExtractor addition, not engine code): with the map open
or on request, for a list of (rx, ry, L) — at least levels 0 … 5 around a town, a castle
(kind 24/45/46 rock spans), a forest, a coast with rivers, snowy highlands and open sea —
either read the finished records from the map's region table (map +0x188 under the lock at
map +0x1a0, key (rx, ry, L)) or call 0x2eb7b0 directly with a request (rx, ry, d) and the
same five captured values (map, &B, &u, &r, &L). Dump per region:
* rx, ry, L, u, r; +0x08; sizes +0x88, +0x8c, +0x90 of the model and its 22 × 22 × sizeZ × 3
  voxel bytes (model +0x68);
* every record of +0x18, +0x38, +0x48, +0x58 (0xc0 bytes each) and +0x68 (16 bytes), in list
  order;
* the border map +0x78: each key (two i32) and its points.

A test would rebuild each region and compare the sizes, lowest, every voxel, and the lists in
order. Recording before 0x170700 runs is not needed: it does not change the voxel bytes.

---

## 8. Open questions

1. **Recording**: everything above is from the code alone (§7).
2. **Realm of the column**: σ and ρ come from the column's place cell (fdiv(X, 1024),
   fdiv(Y, 1024)), not the object cell's; near a place border a building can take the
   neighbouring place's colours on the map. This is what the code does.
3. **Register reuse**: the T = 34 rule's "roads < 0.45" reads a register the decompiler shows
   as several values; the assembly reloads it from the roads value at every path into that
   test (0x2efe6c), so roads it is.
4. **0x2d3c50** (Nb) is called and its result discarded; it needs no specification here.
5. The land-column cliff blend (§3.4) uses the zone's cliff colours but no carving or models,
   and its step 2 uses z = floor(h0) for every voxel of the column.
6. The span colour (§3.5 step 1) and the walls' grass (§3.6.2 rule 3) pass a constant or a
   voxel index where the zone generator passes a block height; this is what the game does.
