# World generator: the height of a point (0x35ea0)

Part A, second file. Conventions (float order, min/max, smooth, floorf, N(…)) are those of
`worldgen-terrain.md` §0; noise is §1.4 there. "Nb" is the region neighbourhood record (§2.5 there),
"P" the place record (§3.6 there). Every noise call's scales and offsets are also listed in
`worldgen-terrain-tables.md` §5.

---

## 1. The pipeline (0x35ea0)

Input (x, y, mode). Output the ground height (float). Steps, in this order (each value feeds later
ones):

| # | Value | Routine | Inputs |
|---|---|---|---|
| 1 | Nb | 0x358e0 | x, y |
| 2 | P | 0x35ce0 | x, y |
| 3 | lake | 0x2bb7e0 | Nb |
| 4 | base | 0x2a0840 | Nb, P.kind, P.weight |
| 5 | roads | 0x2d4730 | x, y, P.kind, P.weight |
| 6 | lavaRegion | 0x2b9750 | Nb, P.kind, P.weight |
| 7 | forestRegion | 0x2d9f30 | Nb, P.kind, P.weight |
| 8 | wet | 0x2e64e0 | Nb |
| 9 | river | 0x2e5f80 | x, y, roads, lavaRegion, forestRegion, wet, lake (unused), P |
| 10 | lava | 0x2b92f0 | x, y, roads, lavaRegion, lake, P |
| 11 | swamp | 0x2d01a0 | x, y, roads, forestRegion, lake, P |
| 12 | highlands | 0x2bb310 | x, y, base, lake, river, lava, swamp, roads, P |
| 13 | dryRegion | 0x2a44a0 | Nb |
| 14 | dryness | 0x2b5960 | Nb |
| 15 | vegetation | 0x2d36f0 | x, y, dryness, highlands (unused), dryRegion |
| 16 | height | 0x2b4450 | x, y, base, highlands, river, lava, swamp, roads, vegetation, lake, P, mode |

None of these draw rand().

**Mode bits** (fourth argument): 1 skips place variants' shapes (§8.5); 2 skips the mountain rise,
the variant shapes and the highland peaks (§8.1, 8.5, 8.6); 4 adds cliff steps (§8.8); 8 skips beds
and shores (§8.7). Callers: 0 for normal ground; 4 by part B's structures; 1 by the road over a pit
(zone §5.4 step 3); the zone's cliff pass calls step 16 alone with mode 2 (zone §5.6).

Related entry points:
* **0x2ba260** (x, y): Nb at (x, y), then step 4 with kind 0 and weight 0 — "plain base height".
* **0x2ba200** (P): Nb at P's landmark point, then step 4 with P.kind and weight 1.0 — "the place's
  own height". Written PH below.
* **0x8a140** (x, y): Nb and P at (x, y), then step 4 — the base height with the place.

---

## 2. Base height (0x2a0840)

Input Nb, kind, w (weight).

1. Over the nine regions (index order 0..8): ocean = the smallest d2 among regions of biome 1;
   lakeD = the smallest d2 among regions of biome 7 (each starts at −1 and is replaced when it is < 0
   or the new d2 is strictly smaller). A region is tested for 1 first, then for 7.
2. h = 100.
3. **Ocean.** If ocean ≥ 0 and ocean ≥ nearest:
   t = (ocean − nearest) × 1.4901161193847656e-08 (= 2^−26);
   a = max(1 − t, 0) × 4; s = min(3 − a, 1);
   if 0 > s: h = −smooth(−s) × 100, else h = smooth(s) × 100. (The negation is applied to the result
   of smooth, then × 100.)
4. **Lake.** If lakeD ≥ 0 and lakeD ≥ nearest:
   t = ((lakeD − nearest) × 0.0003) × 0.0003 (float 0.0003, applied twice);
   u = max(1 − t, 0); n = N(0.0015·X + 6667, 0.0015·Y + 33322) + 1;
   h = h + n × (smooth(u) × 100).
   X, Y are Nb's x and y.
5. h = h + N(0.001·X + 247412, 0.001·Y + 8128321) × 20.
6. **Place.** kind 28: h = h − w × 50. kind 31: h = h + w × 50. Otherwise if kind ≠ 0 and w > 0 and
   1 > h: ww = min(w + w, 1); h = h + smooth(ww) × (1 − h).
7. Return h.

---

## 3. Region weights

### 3.1 The common form

For a set S of biomes and a scale k: d = the smallest d2 of regions with biome in S (−1 if none,
same replacement rule as above). If d ≥ 0 and d ≥ nearest: v = 1 − ((d − nearest) × k) × k; if
v ≤ 0 then 0; return v. Else 0. (Written max(…, 0) below: v when v > 0, else 0.)

| Value | Routine | S | k |
|---|---|---|---|
| lake | 0x2bb7e0 | {7} | 0.0004 (float 0.00039999998989515007) |
| wet | 0x2e64e0 | {8} | 0.0004 |
| dryRegion | 0x2a44a0 | {9, 13} | 0.0003 (float 0.0003000000142492354) |

### 3.2 lavaRegion (0x2b9750)

v = the common form with {13}, k = 0.0004 (0 if no such region). If v ≥ 1 return v.
Kind 18: return min(1, (w + w) + v). Kind 19: return min(1, v + w). Else v.

### 3.3 forestRegion (0x2d9f30)

v = common form with {5, 12}, k = 0.0004. If v ≥ 1 return v. Kind 26: return min(1, v + w). Else v.

### 3.4 dryness (0x2b5960)

a = common form {9, 13} k 0.0004; b = {10} k 0.0004; c = {4, 8} k 0.0004 (computed in that order).
v = N(0.0002·X + 6654, 0.0002·Y + 98883) × 0.35 (float 0.3499999940395355);
v = v + (c × 1.5 + 0.5); v = v − b × 0.4; v = v − (a + a);
if not (1 > v) v = 1; return v if 0 < v, else 0.

---

## 4. Roads (0x2d4730)

Input x, y, kind, w. Result 1 on a road, 0 away from one.

1. Offsets: a = N(0.0025·X + 2346, 0.005·Y + 64565) × 128; then
   a' = N(0.005·X + 4234, 0.01·Y + 5434545) × 32;
   b = N(0.005·X + 23432, 0.0025·Y + 2342344) × 128;
   b' = N(0.01·X + 54356, 0.005·Y + 7677) (computed in this order: a, a', b, b').
   keep = 1 − w.
   px = trunc64((a + a') × keep × 65536) + int64(x) × 65536;
   py = trunc64((b + b' × 32) × keep × 65536) + int64(y) × 65536.
2. Cell: bx = the 64-bit dots divided by 65536 **truncating toward zero** (the binary adds 0xffff to
   negatives and shifts; no floor correction), as 32-bit; cx0 = fdiv(bx, 1024); likewise cy0.
3. road = 1. For cx = cx0 − 1 … cx0 + 1 (outer), cy = cy0 − 1 … cy0 + 1 (inner):
   * A = cellPoint(cx, cy, bigCell(cx, cy)). cell = 1.
   * Four links, in this order. For each, a "hash cell" (hx, hy) (evened if that cell is big), a salt,
     and a neighbour cell (ox, oy):
     1. hash cell (cx, cy) with this cell's big flag, salt 0x81d65, neighbour (cx − 1, cy);
     2. hash cell (cx + 1, cy) with its own big flag, salt 0x81d65, neighbour (cx + 1, cy);
     3. hash cell (cx, cy) with this cell's flag, salt 0x286926, neighbour (cx, cy − 1);
     4. hash cell (cx, cy + 1) with its own flag, salt 0x286926, neighbour (cx, cy + 1).
     A link exists when H(hy × 57 + salt + hx) / 21474 is odd. Then B = cellPoint(ox, oy,
     bigCell(ox, oy)); if B ≠ A (compared x, then y), cell = cell × seg(A, B).
   * road = road × cell.
4. seg(A, B): vx = float32(px − int64(A.x) × 65536) × (1/65536), vy likewise with py, A.y;
   ex = float(B.x − A.x), ey = float(B.y − A.y) (32-bit integer differences);
   dot = ex × vx + ey × vy.
   * If not (0 < dot): d2 = vy × vy + vx × vx.
   * Else len2 = ey × ey + ex × ex. If dot ≥ len2: wx, wy are the offsets from B as for v;
     d2 = wy × wy + wx × wx. Else t = dot / len2; ay = vy − ey × t; ax = vx − ex × t;
     d2 = ax × ax + ay × ay.
   * f = 1 − min(1, d2 / (250 × 250)) (250 × 250 as a float product); f = f × f four times;
     return 1 − f.
5. Places:
   * Kind 5: if w > 9.999999747378752e-05: c = min(1, max(0, (w − 0.4) × 10)); e = 1 − c;
     n = N(0.01·X, 0.01·Y); e = e × e; inner = 1 − e; m = min(|n| + |n|, 1) (|n| is the absolute
     value taken in double and converted back); m = m × m; m = m × m; v = (1 − m) × inner;
     road = road × (1 − v).
   * Kinds 10, 13, 14, 17, 18, 19, 21, 30, 31, 32, 33 (mask 0x3c02e6400): road = 1 − (1 − road) × keep.
6. road = road × 120. If road > 0.5: road = (road − 0.5) / 119.5 × 0.5 + 0.5.
   Return max(1 − road, 0).

---

## 5. Channels: river, lava, swamp

### 5.1 Channel noise

A channel is three noise layers at one coordinate, each with its own scale and offsets
(table in the tables file §5): v = N(layer 1) × 0.5; v = v + N(layer 2); v = v + N(layer 3) × 0.1
(float 0.10000000149011612); return |v| (absolute value through double).

| Channel | Layer 1 (×0.5) | Layer 2 | Layer 3 (×0.1) |
|---|---|---|---|
| river | 0.0025, +23123, +42432 | 0.0005, +23123, +42432 | 0.01, +23123, +42432 |
| river width | 0.0025, +53455, +6547 | 0.0005, +5553445, +123321 | 0.01, +6455445, +33223 |
| lava | 0.0023, +330003, +323322 | 0.00045, +88989, +1001 | 0.01, +558587, +198743 |
| swamp | 0.0025, +5253, +809834 | 0.0005, +6664, +7567567 | 0.01, +80084, +5664 |

(Each layer is N(s·X + ox, s·Y + oy).)

**fill(v)**: v = v × 5; if v > 0.5: t = (v − 0.5) × 0.5; v = t × t × t + 0.5. f = max(1 − v, 0);
return f × f.

Helpers:
* **dries(w)** = 1 − (1 − e) where q = min(1, max(0, (w − 0.1) × 10)), e = (1 − q)²; (written as the
  game does it; the value equals e).
* **ring(r, P)**: if not (float(radius²) > P.dist2) return r. R = float(radius);
  t = |(sqrt(P.dist2) − R × 0.75) / (R × 0.25)|; keep = 1 − r; t = min(t, 1); f = 1 − t × t;
  return r + f × f × keep.
* **centreFill(r, w)**: t = (w − 0.4) / 0.6 (floats 0.4000000059604645, 0.6000000238418579);
  return r + (1 − r) × (t if t > 0 else 0).
* **roadsDry(r, road)**: c = min(1, max(0, (road − 0.4) × 10)); return (1 − c) × r.
* **centreBiome(P)**: the biome of the region 0x2ada20 gives at P's landmark point.

### 5.2 river (0x2e5f80)

1. r = 0. If 0.5 > lavaRegion and 0.5 > forestRegion:
   v = river channel; if wet > 9.999999747378752e-06: v = v × (1 − (1 − width channel) × wet);
   r = (1 − min(lavaRegion + lavaRegion, 1)) × fill(v); r = r × (1 − min(forestRegion + forestRegion, 1)).
2. If P.kind ≠ 0: kind 13 → r = centreFill(r, P.weight). Otherwise r = r × dries(P.weight); then if
   P.variant = 4 and centreBiome(P) ≠ 13, r = ring(r, P).
3. Return roadsDry(r, roads).

### 5.3 lava (0x2b92f0)

1. r = 0. If lavaRegion > 0.5: f = fill(lava channel); r = f × ((lavaRegion − 0.5) + (lavaRegion − 0.5));
   r = r × (1 − lake).
2. If P.kind ≠ 0: kind 14 → centreFill(r, P.weight). Kind 19: k = float(radius) × 0.3; k = k × k;
   if k > P.dist2: r = r + (1 − P.dist2 / k). Other kinds: r = r × dries(P.weight); then if variant 4
   and centreBiome(P) = 13, r = ring(r, P).
3. Return roadsDry(r, roads).

### 5.4 swamp (0x2d01a0)

1. r = 0. If forestRegion > 0.5: f = fill(swamp channel); r = f × ((forestRegion − 0.5) + (forestRegion − 0.5));
   r = r × (1 − lake).
2. If P.kind ≠ 0: r = r × dries(P.weight).
3. Return roadsDry(r, roads).

---

## 6. Highlands (0x2bb310)

1. n1 = N(0.0015·X + 565, 0.0015·Y + 8877); amount = max(0, (n1 + n1) + (lake + 1)).
2. m = N(0.001·X + 34234, 0.001·Y + 3245) × N(0.0015·X + 545, 0.0015·Y + 654665) (first call first).
3. c = N(0.006·X + 55545, 0.006·Y + 6566) × 0.2 + (m + m); ridge = max(1 − |c|, 0).
4. flat = 1. If P.kind ≠ 0 and P.weight > 0 (w = P.weight):
   * kinds 10 and 19: n5 = N(0.025·X + 84333, 0.025·Y + 98111) + 1; add = n5 × (w × 0.5) × (1 − ridge);
     amount = amount + (w + w); ridge = ridge + add.
   * kinds 17, 21, 28, 36 (mask 0x1010220000): nothing.
   * other kinds: s = min(1, max(0, w + w)); flat = 1 − smooth(s).
5. hb = (base + 80) × 0.05 (float 0.05000000074505806); if not (hb > 0) hb = 0.
6. If not (0 < amount): return 0.
7. a = min(1, amount).
   keep = (1 − min(lava + lava, 1)) × (1 − min(river + river, 1));
   keep = keep × (1 − min(swamp + swamp, 1)); keep = keep × (1 − min(roads + roads, 1));
   keep = keep × flat; q = 1 − keep.
   If not (hb < 1) hb = 1.
   v = (1 − smooth(q)) × smooth(a); v = v × hb × ridge.
8. If v > 0.5: t = v − 0.5; u = t × 4; if not (1 > u) u = 1; u = 0 if 0 ≥ u;
   g = cliffs(x, y, P.kind, P.weight) (§8.9); u = u × u; e = 1 − smooth(g); e = e × e;
   k = (1 − e) × (1 − u); v = (1 − k) × t + 0.5.
9. Return v.

---

## 7. Vegetation (0x2d36f0)

All in double, from float noise values widened:
v = (n1 + (1.5 − dryness)) × 0.30000001192092896, where n1 = N(0.01·X + 647646, 0.01·Y + 84373);
v = v + dryRegion; v = v + N(0.02·X + 43234, 0.02·Y + 5345345) × 0.2;
v = v + N(0.05·X + 77878, 0.05·Y + 9888) × 0.1; return float32(v).
The three noises are called in the order n1, n(0.02), n(0.05). The highlands argument is unused.

---

## 8. The final height (0x2b4450)

Inputs: x, y, base, highlands, river, lava, swamp, roads, vegetation, P, mode.
X, Y are x, y as doubles. W = P.weight.

### 8.1 Rolling and the place's shape

1. rolling = (N(0.005·X + 3363, 0.005·Y + 7874) + 1) × 25 − 10.
2. lowPlace = false; F = 1; G = 1.
3. If P.kind ≠ 0 and W > 9.999999747378752e-05:
   * PH = 0x2ba200(P) (the place's own height, §1). If 10 > PH: PH = 10, lowPlace = true.
     (The binary then skips the rest when PH < 1, which cannot happen.)
   * s = min(1, W + W); sm = smooth(s).
   * **Kind 10** (mountain): unless mode bit 2: base = W × W × 180 + base.
   * **Kind 18** (crater): R = float(radius); t = min(W + W, 1); inner = max((W − 0.5) + (W − 0.5), 0);
     F = 1 − sm; rim = t × t × 0.1 × R + base; e = (1 − inner)²; base = rim − (1 − e) × R × 0.25.
   * **Kind 19** (volcano): a = min(W / 0.85, 1); b = max((W − 0.85) / 0.15, 0) (floats
     0.8500000238418579, 0.15000000596046448); F = 1 − sm; cone = a × a × a × 180 + base;
     e = (1 − b)²; base = cone − (1 − e) × 20.
   * **Kinds 17, 21, 28, 36**: nothing.
   * **Other kinds**: F = 1 − sm; if rolling > 10: rolling = rolling + (10 − rolling) × sm.
     Kinds 8, 13, 14, 24 (mask 0x1006100): G = F; base = (PH − base) × sm + base.
     Others: base = (PH − base) × W + base.

### 8.2 Rolling ground

4. H = max(−256, base).
5. r2 = min(river + river, 1); l2 = min(lava + lava, 1); s2 = min(swamp + swamp, 1);
   d2 = min(roads + roads, 1). Wt = (1 − l2) × (1 − r2); Wt = Wt × (1 − s2).
   open = (1 − d2 × 0.5) × Wt × G.
6. hk = H × 0.05; if not (hk > 0) hk = 0.
   e = (1 − open)²; n = N(0.015·X + 4447657, 0.015·Y + 87878); e = e × e; if not (hk < 1) hk = 1.
   k = (1 − e) × hk × rolling; H = H + (n + 1) × 0.5 × k.

### 8.3 Terraces

7. q = H × 0.1; if not (1 > q) q = 1.
   d = N(0.015·X + 234234, 0.015·Y + 12233); d = d + N(0.01·X + 10943, 0.01·Y + 98433).
   v = (1 − d2) × Wt × F; v2 = 1 − (1 − v); if not (0 < q) q = 0; d = d × v2 × q.
8. If d > 0 and 0.5 > highlands:
   cmod = min(1, W + W) for kind 28, else 0; hm = max(1 − (highlands + highlands), 0).
   A step with (t, ox, oy): a = 1 − min(t × 20, 1); s = N(0.005·X + ox, 0.005·Y + oy) + cmod;
   s = s if s > 0 else 0; if not (s < 1) s = 1; H = H + (1 − a × a) × hm × 20 × s × s.
   Steps: (d, 7743, 98834); if d > 0.5: (d − 0.5, 6777, 8887); if d > 0.75: (d − 0.75, 2133, 444).
9. Hsaved = H.

### 8.4 (No section: numbering kept in step with the zone file's references.)

### 8.5 Variant shapes (skipped with mode bit 2, or bit 1, or P.kind = 0)

* **Variant 1, plateau:** R = float(radius); A = R × 0.7 (0.699999988079071); B = R × 0.5. If A × A >
  P.dist2: span = A − B; t = (sqrt(P.dist2) − B) / span; u = min(1, 1 − t); H = H + smooth(u) × span × 0.75.
* **Variant 5, walled pit** (and not lowPlace): R = float(radius); inner = trunc(R × 0.3); outer =
  trunc(R × 0.75) (integers). If float(outer²) > P.dist2:
  wall = N(0.02·X + 98453466, 0.02·Y + 8743) × 25 + 30; o = float(outer); gap = o − wall.
  If P.dist2 > gap × gap: t = (o − sqrt(P.dist2)) / wall; H = H − t × t × 100.
  Else if P.dist2 > float(inner²): t = sqrt(P.dist2) − float(inner); t = t / (the same noise
  recomputed × 25 + 30); t = min(t, 1); H = H − t × t × 100.
* **Variant 2 (spiral up), or variant 3 and not lowPlace (spiral down):** A = float(radius) × 0.6
  (0.6000000238418579). The game tests A × A > **P.weight** (not the distance; always true for
  real radii) and then:
  1. Warp offsets as 0x364e0 (terrain §3.3) via 0x2baf90: ox, oy (each noise × 32).
     dx = float32((int64(x) − cx) × 65536 + trunc64(ox × 65536)) × (1/65536), dy likewise.
     L = sqrtf(dy × dy + dx × dx); ux = dx / L; uy = dy / L; angle = acosf(ux);
     if 0 > uy: angle = float32(2π − double(angle)) (2π = 6.283185307179586 in double).
  2. Cell: c = (fdiv(x, 1024), fdiv(y, 1024)), evened if P.big. k = c.y × 57.
     start = float(R(c.x + 0x1966ab2c + k) mod 1000000, unsigned) / 1000000.0 (float); start = start + start;
     θ = float32(double(start) × π + double(angle)) (π = 3.141592653589793);
     if double(θ) > 2π: θ = float32(double(θ) − 2π).
     If H(c.x + 0x711bec9f + k) / 21474 is odd: θ = float32(2π − double(θ)).
     Kturns = float(R(c.x + 0x4e3e1c + k) mod 20 + 25) (unsigned).
     cap = min(A / Kturns × 15, 150); scale = A / 6.
  3. turns = float32(double(A) − double(θ) / 2π × double(Kturns)); turns = (turns − L) / Kturns.
  4. If turns < −0.25: no change. Otherwise:
     whole = floorf(turns) as a float; part = turns − whole − 0.75;
     s = smooth(min(1, part × 4)) if part > 0, else 0; s = s + whole;
     if s > 0: θ = float32(double(s) × 2π + double(θ));
     if 0 > turns: θ = θ × (turns × 4 + 1); if 1 > θ: θ = θ × θ.
     r = sqrtf(θ) × scale; r = min(cap, r); variant 3: H = H − r; variant 2: H = H + r.

### 8.6 Highland peaks (skipped with mode bit 2)

If highlands > 0.5: t = min(1, max(0, (highlands − 0.75) × 4)); u = (highlands − 0.5) + (highlands − 0.5);
g = (1 − t) × (t × 4); if not (1 > g) g = 1; n9 = N(0.01·X + 8757, 0.01·Y + 9087);
H = (n9 × 0.25 + 0.75) × (u × u × 100) + H; if not (0 < g) g = 0;
H = H + grain(x, y) × g × 16.

**grain(x, y)** (0x2b5670), in double: v = N(0.08·X + 432, 0.08·Y + 432) × 0.05;
v = v + N(0.04·X + 432, 0.04·Y + 432); r = float32(1 − |v|); e = 1 − r; return e × e + 0.

### 8.7 Beds and shores (skipped with mode bit 8)

* river > 0.5: s = 1 − ((river − 0.5) + (river − 0.5)); qq = (H + 5) × 0.1; if not (qq > 0) qq = 0;
  nn = (N(0.025·X + 35, 0.025·Y + 8933) + 1) × 0.5; s3 = s × s × s; if not (qq < 1) qq = 1;
  H = H − (nn × (1 − s3) × 9.5 × qq + 2).
* else lava > 0.5 or swamp > 0.5: H = H + (−2).
* else vegetation > 0.5 or roads > 0.5 or lowShore(x, y, H): H = H + (−1); if highlands > 0.5, H = H + (−1).

**lowShore(x, y, h)** (0x2b7730): false if not (h < 20); true if not (0 < h);
else (N(0.02·X + 7743, 0.02·Y + 8977) + 1) × 10 > h.

### 8.8 Cliff steps (mode bit 4 and highlands > 0.5)

g = cliffs(x, y, P.kind, W). If g > 0:
T = (N(0.01·X + 7474, 0.01·Y + 98984) + 1) × 20 + Hsaved;
slope = N(0.02·X + 8744, 0.02·Y + 904) × 0.025 + 0.06 (floats 0.02500000037252903, 0.05999999865889549);
at = H. If H > Hsaved, repeat: at = floorf(at) as a float; m = min(|(at − T) × slope|, 1);
s = smooth(1 − (1 − m × m) × g); s = s × ((H − at) × 0.1 + 0.5); if s > 0.5 stop;
at = at + (−1); if not (at > Hsaved) stop.
Return at (the cliff value replaces H).

### 8.9 cliffs(x, y, kind, w) (0x2a2c50)

peak = kind is 10 or 19. If peak and w > 0.5: 0.
a = N(0.1·X + 7007, 0.1·Y + 6000) × 0.1; a = a + N(0.01·X + 1838, 0.01·Y + 88233); a = |a| × 4;
b = N(0.01·X + 43255, 0.01·Y + 6678) × 0.25; a = min(a, 1);
c = min(1, max(0, b + (1 − a))); e = 1 − c; r = 1 − e × e × e;
if peak and w > 9.999999747378752e-05: r = r × max(1 − (w + w), 0). Return r.

### 8.10 Result

Return H (or the cliff value of §8.8).

## Errata (from the clean-room implementation, checked against the oracles)

- §6 step 7: read "v = v × hb × ridge" as v × (hb × ridge). Left to right, 8 of the 3,823 gen-oracle points are one ulp off.
