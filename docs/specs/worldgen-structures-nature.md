# World generator, part B: natural structures

Part of `worldgen-structures.md` (conventions in its §0; the cell loop that calls these in §7).
Positions are block coordinates of the world; "the zone" is the zone being generated. "Put"
means the block store 0x31c1c0 (main file §3), which ignores columns outside the zone. "Put
unless protected" reads the block first (lookup 0xeb90) and skips it when its flags have bit 1
(protected). **Each column of a box** means every (x, y) with x0 ≤ x < x1 and y0 ≤ y < y1,
clipped to the zone first (0x146390: x0 = max(x0, 64 zx), y0 likewise, x1 = min(x1, 64 zx + 64),
y1 likewise), x outer, y inner.

---

## 1. The tree painter and its colour (0x260d50, 0x262fa0)

Trees and logs paint with a small state: a low height zL and colour cL, a high height zH and
colour cH, and a speckle switch.

**Colour at (x, y, z)** (0x262fa0):
1. With speckle on: h = X(z) + y; h ^= h << 13; h ^= h >> 17; h = ((h << 5) ^ h) + x + 0x4d2;
   h ^= h << 13; h = (h >> 17) ^ h (all unsigned 32-bit). When h & 7 = 0 the colour is
   (50, 50, 50).
2. z ≤ zL → cL. z ≥ zH → cH.
3. Else t = float(z − zL) / float(zH − zL), u = 1 − t; each channel cH × t + cL × u.

**Bark colour** (0x2668a0), two draws: t = rand / 32767.0 (float of rand / 32767.0); g =
float(rand mod 20 + 160); the colour is (t × 40 + 260, t × 40 + g, t × 30 + 60).

**Wood block**: type 7, flags 0, glow 0, the colour packed (× 0.5, clamped, truncated).
**Leaf block**: type 8 (11 when snowy), flags 1.

## 2. Pieces

**Trunk** 0x2671e0 (cx, cy, z, r0, r1, n): for k from 0 to n − 1: t = float k / float(n − 1);
r = (1 − t) × r0 + t × r1, at least 1.0 (when 1 > r, r = 1); ri = trunc r; layer h = z + k. Each
column of the box [cx − ri − 1, cx + 2 + ri) × [cy − ri − 1, cy + 2 + ri): dx = float(x − cx) +
0.5, dy likewise, d² = dy·dy + dx·dx; skip when d² > r·r. Colour c at (x, y, h). When
(r − 1)² > d²: ring = float((trunc sqrt d²) mod 2) × 0.2 + 1.3 and the block is c × ring × 0.5
packed; else c × 0.5 packed. Type 7. Put unless protected.

**Wood ball** 0x2669e0 (cx, cy, cz, r): ri = trunc r; each column of [cx − ri − 1, cx + ri + 2) ×
[cy − ri − 1, cy + ri + 2); for k from trunc(−r − 1.0) while r + 1.0 ≥ float k: dk = float k + 0.5,
dx, dy as above, d² = dy² + dx² + dk²; when d² ≤ r², put unless protected the wood block of the
colour at (x, y, cz + k).

**Branch** 0x262dc0 (x, y, z, dir, r0, r1, n): a position (px, py, pz) = dots of (x, y, z) (each
<< 16). For i from 0 to n − 1: t = float i / float(n − 1), r = (1 − t) r0 + t r1; a wood ball at
(px, py, pz) each divided by 65536 toward zero, radius r; then px += trunc64(dir.x × 65536),
py += trunc64(dir.y × 65536), pz += trunc64(t × dir.z × 65536) (products in float, left to
right). It returns the end, each coordinate the floor of dots / 65536.

**Twig** 0x2674a0 (x, y, z, dir, n): as a branch, but each step puts unless protected one wood
block at the position (toward zero), coloured at the twig's start (x, y, z).

**Leaf cloud** 0x2650e0 (cx, cy, cz, r, rz, snowy): ri = trunc r; each column of [cx − ri − 1,
cx + ri + 2) × [cy − ri − 1, cy + ri + 2): n = S(x × 2.0 / r, y × 2.0 / r) × 0.3 (the divisions in
double of the double product); for k from trunc(−rz − 2.0) while rz + 2.0 ≥ float k:
v = float k / rz + n; dx = (float(x − cx) + 0.5) / r, dy likewise; skip when dy² + dx² + v² > 1;
skip when the block there is solid; put unless protected the colour at (x, y, cz + k) × 0.5,
type 11 when snowy else 8, flags 1.

**Leaf colours** 0x2afbf0 (x, y, warm, dry, forest, k27) → (A, B). A running hash
s = X(100 y + 0x3d7) + X(100 x + 0xa8eb); "next(k)" returns X(s) and then adds k to s.
1. t = float(next(1) mod 1000) / 1000; warm palette when (warm − 0.2) × 5 > t; else when next(1)
   mod 3 = 0 (drawn only when the first test failed) also warm.
2. Not warm, dry < 0.5 (0.5 > dry): v = next(3); A.b = v mod 50, B.b = A.b + 0; A.g = v mod 150;
   A.r = v mod 55 + 200; k = float(int(next(1) mod 100 + 60)); B.r = A.r + k, B.g = A.g + k.
3. Not warm, otherwise: p = s ^ (s << 13), s += 1, p ^= p >> 17. When p & 1 (pale): v = next(3);
   A = (v mod 50 + 180, v mod 50 + 120, v mod 50 + 240); k = next(1) mod 50 + 100; B = A + k on
   all three. Else (autumn): A.g = 0; v = next(2); A.b = v mod 230; A.r = v mod 100 + 150;
   m = next(1) mod 80; w = next(1); A.r += m, A.g += m, A.b += m; k = w mod 50 + 100; B = A + k.
4. Warm: v = next(3); A.b = v mod 30; A.g = v mod 60 + 100; A.r = v mod 20; d = (0.5 − dry) × 80;
   A.r = d + A.r; when dry > 0.5, A.b = A.b − d; B.b = A.b + 0; k = next(1) mod 100 + 50;
   B.r = A.r + k, B.g = A.g + k; then A.b += 80, A.g −= 20, A.r −= 20.
5. When forest > 0.01: c = next(1) mod 80 + 140; A.g = (100 − A.g) × forest + A.g; A.r likewise
   toward 100; A.b = (c − A.b) × forest + A.b; k = next(1) mod 30 + 60; B.r and B.g toward 60,
   B.b toward k, each as (target − v) × forest + v.
6. When the snow test 0x2b4320 (x, y, warm, 0, 0, 0, 0) holds: A = B = (200, 280, 400).
7. When k27 > 0.3: A.g = (0 − A.g) × k27 + A.g; v = next(2); A.b toward v mod 200, A.r toward
   v mod 55 + 200; w = X(s); B.r toward 100, B.b toward w mod 55 + 200, B.g toward w mod 50 +
   100 (each (target − v) × k27 + v, in that order).
Unsigned remainders are converted to float through a signed int where the game does (the
"k" terms); the values are below 2^31 so the result is the same.

**Shift toward forest and kind-27 colours** (shared by the birch and the fir, s continuing):
when forest > 0.01: v = X(s), s += 1; A.g, A.r toward 100, A.b toward v mod 80 + 140; w = X(s),
s += 1; B.r, B.g toward 60, B.b toward w mod 30 + 60. When k27 > 0.3: v = X(s), s += 2; A.g
toward 0, A.b toward v mod 200, A.r toward v mod 55 + 200; w = X(s); B.r toward 100, B.b toward
w mod 55 + 200, B.g toward w mod 50 + 100.

**Birch leaf colours** 0x2ae3a0: A = (0, 180, 100), B = (255, 320, 0); when 0.15 > warm both are
(200, 280, 400); then the shift with s = X(X(y) + x).

**Fir colours** 0x2ae840: s = X(X(y) + x); v = X(s), w = X(s + 3); A = (v mod 10, v mod 50 + 100,
v mod 80 + 50), B likewise from w; then the shift with s + 6.

Every tree reads, at its point (x, y): the neighbourhood, the place there (0x35ce0) and from them
warmth, dryness, the forest weight and the kind-27 weight (part A).

## 3. The common tree (0x264670; cell objects 1, 8)

Arguments (x, y, z, h, w): h the tree's size, w its trunk height.
1. Leaf colours (A, B) of the point; bark1 = bark colour, bark2 = bark colour (2 + 2 draws).
2. Painter zL = z, zH = z + w, cL = bark2, cH = bark1. tr = float h × 0.1, at least 1 (unless
   tr > 1, tr = 1). Trunk (x, y, z, tr, tr × 0.8, w).
3. Roots: rr = tr × 0.75 (at least 1), length trunc(tr + tr); branches from (x, y, z + 1) along
   (1, 0, −1), (−1, 0, −1), (0, 1, −1), (0, −1, −1), radius rr to rr.
4. br = tr × 0.5 (at least 1). Tips: first (x, y, z + w). count = 8; 5 when h ≤ 30; 10 when
   w > 50. len = min(h / 2, 15), half = w / 2. For i from 0 to count − 1, rise starting at 0 and
   growing by half each time: a = rand angle; branch from (x, y, rise / (count − 1) + half + z)
   along (cos a, sin a, 1) (cosf, sinf), radius br to br, len steps; its end is a tip.
5. q: unless warm ≥ 0.7 and 0.3 > dry, q = rand fraction × 0.35 + 0.35; otherwise 0.15 (no draw).
6. fruit seed f = float(rand mod 100000) (kept as a float).
7. R = float h × 0.5 × 0.75, rz = R × q, snowy = 0.15 > warm.
8. For each tip e in order (the trunk's top first): painter zH = trunc(rz + e.z), zL = trunc(e.z −
   rz), cL = A, cH = B. When R > 16: five times: s = rand mod 7 + 6, then three draws a, b, c;
   cloud at (trunc(float e.x + float(2c) × 5 / 32767 − 5), trunc(float e.y + float(2b) × 5 /
   32767 − 5), trunc(float e.z + float a × 5 / 32767 − 2.5 + 1.0)), radius float s, height q × s.
   Otherwise one cloud at (e.x, e.y, e.z + 1), R, rz.
9. Then, only when (e.x, e.y) is in the zone: v = X(trunc f), then f = float(trunc f + 1). When
   v mod 10 = 0: the fruit type is X(trunc f) mod 3 + 0x4e (the game writes it as
   v′ + (0x1a − v′ / 3) × 3 in unsigned arithmetic, which is the same), then f = float(trunc f +
   1); a fruit model at dots (e.x, e.y, e.z), yaw 0, tint 1, 1, 1, flags 2, size not written;
   kept if inside. Then a leaf model 0x4d
   at the same place, yaw 0, tint A / 255.0 (each channel divided), flags 2, size not written;
   kept if inside.

## 4. The fir (0x263210; cell object 2)

1. Fir colours (A, B); bark1, bark2 (4 draws). Painter z..z + w, bark2 → bark1.
2. hh = trunc(float h × 0.5), hf = float hh; len = trunc(double hh × 1.5 + double w);
   tr = hf × 0.15, at least 1. Tapering trunk 0x266ea0 (x, y, z − 1, tr, 0.5, len): for k from 0
   to len − 1, u = 1 − float k / float(len − 1), c = 1 − u·u·u (u × u × u in that order: u ×
   (u × u)), r = (1 − c) r0 + c r1; i0 = trunc(−r); for i from i0 while r ≥ float i, j from i0
   while r ≥ float j: skip when float j × float j + float i × float i > r × r; put unless
   protected the wood colour at (x + i, y + j, z + k) × 0.5, type 7.
3. a = rand angle. Painter zL = len / 4 + z, zH = z + len, A → B. s = rand fraction × 0.2 + 0.2,
   t = 1 − s. droop = float(−hh) × 0.3. steps = len / 3; top = z + len; snowy = 0.15 > warm.
4. Thirty needle branches, i from 0 to 29: a = float(double a + 0.9424778335276408);
   q = float i × t / 30 + s; ca = cos a × hf, sa = sin a × hf; u = 1 − q, u2 = u + u. Start
   (sx, sy) = trunc64(double x × 65536.0) and y likewise, sz = trunc64(double(float len × q +
   float z) × 65536.0); end ex = sx + trunc64(ca × u2 × 65536), ey likewise with sa, ez = sz +
   trunc64(droop × u2 × 65536). For k from steps down to 1: f = float k / float steps,
   g = 1 − f; p = e + trunc64(float(s − e) × (1/65536) × g × 65536) per axis; rad = (f + f) +
   (g·tr·u + g·tr·u) with g·tr·u = g × tr × u; at least 2 (unless rad > 2, rad = 2). bx, by =
   p.x, p.y toward zero. When snowy, a needle disc at height (p.z + 65536) toward zero of the
   block (100, 140, 200, 0, 10, 1). Then a disc at bz = p.z toward zero with the colour at (bx,
   by, bz) × 0.5, type 8, flags 1. The disc 0x263120 (x, y, z, r, block): ri = trunc r; each
   column of [x − ri − 1, x + ri + 2) × [y − ri − 1, y + ri + 2) with r·r ≥ dy² + dx² (dx =
   float(bx − x), not offset by 0.5) gets the block, put unless protected.
   After the k loop, the tip: three blocks at (x, y) heights top + 1, top, top − 1, coloured at
   (x, y, max(height, top)) × 0.5, type 8, flags 0, put unless protected (repeated each of the
   30 times).
5. When rand mod 10 = 0: a fruit model 0x4e + rand mod 3 at (x, y, top − 1) (as §3 step 9 but
   kept if inside, no in-zone test before).

## 5. The birch (0x2626e0; cell object 3)

1. Painter zL = z, zH = w + z, both colours (255, 300, 300), speckle on. tr = float h × 0.05
   (at least 1). Trunk (x, y, z, tr, tr, w). Roots as the common tree's but from (x, y, z).
2. Tips: (x, y, z + w), then five twigs: half = w / 2, len = h / 3; for i from 0 to 4 with rise
   0, half, 2 half, …: a = rand angle, twig from (x, y, rise / 4 + z + half) along (cos a, sin a,
   0.5), len blocks; its end is a tip.
3. Speckle off. Birch leaf colours (A, B) from the point's warmth, forest and kind-27 weights;
   snowy = 0.15 > warm; H = float h × 0.5.
4. For each tip: r = (rand fraction × 0.25 + 0.5) × H, rz = r × 0.5; painter zH = trunc(e.z +
   rz), zL = trunc(e.z − rz), A → B; cloud (e.x, e.y, e.z + 1, r, rz, snowy). When rand mod 10
   = 0: fruit 0x4e + rand mod 3 at the tip (kept if inside).

## 6. The willow (0x263ea0; cell object 4)

1. Leaf colours (A, B). Painter zL = z, zH = top = w + z, cL = (200, 150, 80), cH = A × 0.5 per
   channel. tr = float h × 0.1 (at least 1). Trunk (x, y, z, tr, tr, w); roots from z + 1.
2. br = tr × 0.5 (at least 1). Tips: (x, y, top), then five branches: len = h / 2, half = w / 2,
   a = rand angle, from (x, y, rise / 4 + z + half) along (cos a, sin a, 1), br to br.
3. fruit seed f = rand mod 100000 (unsigned, kept as an integer). H = float h × 0.5.
4. For each tip: r = (rand fraction × 0.25 + 0.75) × H, rz = r × 0.5; painter zL = e.z − half,
   zH = trunc(e.z + rz), A → B; willow cloud (e.x, e.y, e.z + 1, r, rz) (0x263db0): as the leaf
   cloud, never snowy, block type 8 flags 1; after putting each leaf block at height z′:
   h1 = X(z′) + y, h2 = X(h1) + x; when X(h2) mod 20 = 0: len = X(h2 + 0x2e2) mod 10;
   t = X(h1 + 0x7c9) + x, t ^= t << 13, t ^= t >> 17; when t & 3 = 0, len doubles; the same leaf
   block is put unless protected at z′ − 1 − j for j < len. Then when the tip is in the zone and
   X(f) mod 10 = 0 (f += 1 after each test): fruit 0x4e + X(f) mod 3, f += 1.

## 7. The palm (0x265e70; cell object 5)

1. Leaf colours (A, B). len = trunc(float w × 1.2). Painter z..z + len, both (200, 150, 80).
   hf = float h; tr = hf × 0.1 (at least 1). a = rand angle; sa = sin a, ca = cos a;
   lean = rand / 32767.0; lx = ca × lean, ly = lean × sa.
2. Trunk, for k from −3 to len − 1: t = max(0, float k / float(len − 1)) (the max is maxss with
   0 first); r = tr − t × 0.75 × tr, at least 1; u = (1 − t) × (1 − t); bend = (1 − u) × 0.25 × hf;
   centre cx = trunc64(lx × bend × 65536) + dots(x), cy likewise with ly; rr = trunc64(r × 65536);
   the box from (cx − rr − 65536, cy − rr − 65536) to (cx + 131072 + rr, …) each toward zero
   (0x261140). In it: dx = float(dots(bx) − cx + 32768) × (1/65536), dy likewise; skip when
   dy² + dx² > r²; band = posmod(k / 2, 2) (k / 2 truncating); f = float band × 0.15 + 0.85; put
   unless protected the colour at (bx, by, z + k) × f × 0.5, type 7. After each layer the top is
   (dots(cx toward zero), dots(cy toward zero), dots(z + k)).
3. Fronds: n = rand mod 3 + 5; half = len / 2; painter zL = half + z, zH = z + len, A → B;
   reach = h / 2, hr = float reach. For i < n: ang = float(double(fi + fi) × π / double n +
   double a) (π as 3.141592653589793); fc = cos ang, fs = sin ang; droop = float(rand mod half +
   half). For j < reach: jf = float j, uu = jf / hr, u3 = uu × (uu × uu); v = float(j + 1) / hr;
   px = trunc64(fc × jf × 65536) + top.x, py likewise with fs; zA = (top.z − trunc64(droop × u3 ×
   0.5 × 65536) + 65536) toward zero; wide = (1 − u3) × 5; zB = (top.z − trunc64(v·v·v × droop ×
   0.5 × 65536) + 65536) toward zero (v × v × v, then × droop); m0 = trunc(wide × −0.5); wh =
   wide × 0.5. For zz from zB to zA, m from m0 while wh ≥ float m: bx = (px − trunc64(fs × float m
   × 65536)) toward zero, by = (trunc64(fc × float m × 65536) + py) toward zero; skip when the
   block there is solid or has flag 0x40; put unless protected the colour at (bx, by, zz) × 0.5,
   type 8, flags 0.

## 8. Stump, logs and the mound

**Stump** 0x266ad0 (cell object 19), (x, y, z, h, w): bark = bark colour (2 draws); leaf colours
(A, B); bark.r += (150 − bark.r) × k27, bark.b toward 200, bark.g toward 100 (in that order).
Painter zL = z, zH = w + z, cL = bark, cH = ((bark.r × 0.25 + A.r × 0.75) × 0.5, … g, … b).
tr = max(float h × 0.1, 1.0); rr = max(tr × 0.75, 1.0); roots from (x, y, z + 1) length
trunc(tr + tr); trunk (x, y, z, tr, tr, trunc(tr + 1.0)).

**Fallen log** 0x2651f0 along x, 0x265840 along y (cell objects 20, 21), (x, y, z, r, len,
hollow): zc = trunc(r − 2 + float z). bark = bark colour (2 draws). Painter zL = trunc(float zc −
r), zH = trunc(r + float zc), cL = ((bark.r + 20) × 0.5, (bark.g + 100) × 0.5, (bark.b + 30) ×
0.5), cH = bark. ri = trunc r. Two twigs, for side +1 then −1: unless rand mod 2 ≠ 0 skip; d1 =
rand fraction × 2 − 1, d0 = rand fraction × 2 − 1; tz = zc − 1 + rand mod 3; off = rand mod
(len − 2) − len / 2. Along x: twig from (x + 1 + off, y + ri + 1 or y − ri, tz) along (d0, side,
d1), ri blocks. Along y: from (x + ri + 1 or x − ri, y + 1 + off, tz) along (side, d0, d1). The
ends are tips. The log: each column of [x − len / 2, x + len / 2) × [y − ri − 1, y + 2 + ri)
(along x; along y the axes swap): across = by − y (bx − x along y); for bz from zc − ri − 1 to
zc + 1 + ri: da = float across − 0.5, dz = float(bz − zc) − 0.5, d² = dz² + da²; skip when d² >
r²; inside (r − 1)² > d²: hollow → air (0, 0, 0, 0, 0, 0), else ring as the trunk; the rim: the
colour × 0.5. Type 7, put unless protected. Then leaf colours (A, B) and for each tip:
q = rand / 32767.0 + 1; painter zL = trunc(e.z − q), zH = trunc(e.z + q), A → B; cloud (e, q,
q, not snowy).

**Mound** 0x146090 → 0x1448a0 (cell object 60), (x, y, z, rx, ry): each column of [x − rx,
x + rx + 1) × [y − ry, y + ry + 1), with its column values: topf = S(X × 0.01 + 4394, Y × 0.01 +
8974) × 4 + float z, top = trunc topf; a = S(X × 0.05 + 8384, Y × 0.05 + 2337) × 0.1 + float(X −
x) / float rx; b = S(X × 0.05 + 3244, Y × 0.05 + 2133) × 0.1 + float(Y − y) / float ry;
d = min(b² + a², 1.0); skip when d ≥ 1; d = d × d; n3 = float(|S(X × 0.15 + 34234, Y × 0.15 +
2424)| in double) × 0.3; m = n3 + |S(X × 0.08 + 1034, Y × 0.08 + 545)|; e = 1 − d; bottom =
trunc(((float(z − 10) − e × 10) − m × 10) × e + d × topf). For Z from top down to bottom: skip
blocks with flag 2 or 0x20. At top: when 0.15 > warmth, (200, 280, 400) type 10 flags 1; else
the surface colour (part A) with its type and flags. Below: the river bed colour at (X, Y, Z)
(0xbd110) × k with k = m × 0.5 + 0.5; grey = r × 0.3 + g × 0.59 + b × 0.11; inv = 1 / max(k,
0.001); wgt = (1 − inv) × grey; each channel × inv + wgt; type 5 flags 0. At Z = bottom the
type becomes 0 and the glow 255. Put (packed). After the top block, when snow 0x2b4320 (X, Y,
warmth, 0, 0, 0, 0): (100, 140, 200, 0, 10, 1) at top + 1.

## 9. The sunken chest (0x3158c6; cell object 62)

m = (16 cx + 8, 16 cy + 8); base = trunc(ground height (m, mode 4) − 16). For X from m.x − 16
to m.x + 16, Y likewise: t = (dx² + dy²) × 0.00390625 (float dx, dy); skip when t > 1 or the
column is outside the zone. u = (t − 0.5) × 2 (as u = t − 0.5, u += u); v = max(u, 0); k = 1 − v²;
gh = ground height (X, Y, mode 4); hgt = float base × k + (1 − k) × gh; hb = floor hgt,
g0 = floor gh. Put the buried colour at (X, Y, hb) (packed) at height hb − 1. For z from hb to
g0 + 1, except z = g0 − 1, put air (0, 0, 0, 0, 0, 0). g = g0, minus 1 unless the column's
vegetation > 0.5. Buried colour at (X, Y, g), each channel × (|S(X × 0.25, Y × 0.25)| × 0.3 +
0.7), flags |= 0x10; put it at g and at g − 1.

Then a mason at the cell (frame (16 cx, 16 cy, 0), 16 × 16, no turn; mason file §1) with
scheme (wall colour of 62, pattern 1, 0; the same, pattern 1, 20; relative): box (4, 4, base,
8, 8, 2); capped posts (0, 0), (15, 0), (15, 15), (0, 15) at base, radius 1, height 10, grounded.
The chest: object 0x16 at dots (m.x, m.y, base + 2), turn rand mod 4, settled with the ground
test, then three draws (its contents, not stored), kept if inside.

## 10. Rocks, boulders and hills

**Inside a rock** 0x146180 (X, Y, Z; x, y, z, rx, ry, rz): dz = float(Z − z);
a = S(Y × 0.02 + 3432, Z × 0.02 + 65465) × 0.25 + float(X − x) / (float rx × 0.5);
b = S(X × 0.02 + 4374, Z × 0.02 + 7665) × 0.75 + float(Y − y) / (float ry × 0.5);
c = S(X × 0.04 + 4374, Y × 0.04 + 9898) × 0.5 + max-with-0 of dz / (float rz × 0.5) (dz when
dz > 0, else 0). The vertical term is squared **twice**: q = c × c, then q × q (c to the fourth
power). Inside when 1 ≥ (b² + a²) + c⁴, the sum taken in that order in single precision (the
assembly at 0x146317–0x14635c squares c twice before adding it; Ghidra's text hides this by
reusing one variable). All three noise calls take two coordinates as written; the constants are
doubles 0.02, 3432, 65465, 4374, 7665, 0.04, 9898 and floats 0.25, 0.75, 0.5, 1.0.

**Rock** 0x146450 (cell object 9): each column of [x − rx, x + rx + 1) × [y − ry, y + ry + 1), with
its column values: for Z from trunc(the column's height) up to and including z + 2 rz (no
blocks when the start is already above it): skip blocks with flags
0x22; skip unless inside; put the rock colour (part A, 0x2d3150 with the column's warmth, lava and
forest weights) packed, type 6, the given flags.

**Boulder** 0x146560 (cell objects 54, 55), (x, y, z, rx, ry, rz, flags): top = the surface colour
at (x, y, z) with the column values of (x, y) (0xbbfe0). For X from x − rx to x + rx, Y likewise
(outer X): skip outside the zone; for Z from z − 2 rz to z + 2 rz: a = S(Y × 0.05 + 3432, Z ×
0.05 + 65465) × 0.25 + float(X − x) / float rx; b = S(X × 0.05 + 4374, Z × 0.05 + 7665) × 0.25 +
float(Y − y) / float ry; c = S(X × 0.05 + 4374, Y × 0.05 + 9898) × 0.5 + float(Z − z) / float rz;
skip unless a² + b² + c² ≤ 1; t = min(max(float(rz − z + Z) / float rz + 0.5, 0), 1); u = 1 − t;
rock colour r at (X, Y, Z); put ((r × t + u × top) × 0.5 per channel, packed), type 6, flags.

**Hill** 0x145fa0 rocky, 0x145eb0 earth (cell objects 10–14, and under-water hills), with a
canyon 0x145b30 / 0x145cf0 (15). Each column of [x − rx, x + rx + 1) × [y − ry, y + ry + 1)
(0x144fd0, 0x144260, 0x1438d0):
1. top = trunc(S(X × 0.01 + 4394, Y × 0.01 + 8974) × 10 + float z); vegetated = the column's
   vegetation > 0.5, and then top − 1. base = the column's stored base. floorz = the column's
   height − 1.
2. With a canyon to (ax, ay): dx = float(ax − x), dy = float(ay − y); px = float(dots(X) −
   dots(x)) × (1/65536), py likewise; dot = dx·px + dy·py. When 0 ≥ dot: d² = py² + px².
   Else len² = dy² + dx²; when dot ≥ len²: d² from (X, Y) to (ax, ay) the same way; else t =
   dot / len², d² = (px − t·dx)² + (py − t·dy)² (computed as py2 = py − t × dy, px2 = px − t × dx,
   d² = px2² + py2²). When 64 > d²: e = d² × 0.015625, floorz += (1 − e²) × 10.
3. For Z from top down to base: stop at a block with flags 0x22. a = S(Y × 0.05, Z × 0.05 + 2337)
   × 0.25 + float(X − x) / (float rx × 0.5); b = S(X × 0.05 + 4374, Z × 0.05 + 9898) × 0.25 +
   float(Y − y) / (float ry × 0.5); sum = b² + a²; skip unless 1 ≥ sum.
4. With a canyon and floorz > float Z: when the canyon is open and sum > 0.85, a banded wall:
   n = S(Y × 0.25 + 89003, Z × 0.25 + 10010) + S(X × 0.25, Z × 0.25); v = min(|n|, 1) × 0.3 + 0.7;
   the rock colour (rocky) or ground colour (earth, 0x2ab410 with warmth, dryness, lava, forest,
   kind-27 weights) × v × 0.5, type 5, flags 0x12. Else at Z = base the surface colour (packed);
   else protected air (0, 0, 0, 0, 0, 2). Next Z.
5. Otherwise: at Z = top and not vegetated the surface colour (its type, flags, glow); else rocky:
   rock colour, type 6; earth: ground colour, type 5; flags 0, glow 0. Put packed. At Z = top,
   when snow (X, Y, warmth, 0, 0, 0, 0): (100, 140, 200, 0, 10, 1) at top + 1.

## Errata (from the clean-room implementation)

- **Fixed (§10, Inside a rock).** Type-9 rocks came out slightly too small, mostly at the top (all 113 differing columns in the 125 gen_seed_123 zones were rock edges). Cause: the old text said "c = c × c; inside when 1 ≥ b² + a² + c²", which reads as the vertical term squared once, but the game squares it twice (c⁴; 0x146329 and 0x146335 are two `mulss xmm0, xmm0`). Since c⁴ < c² for |c| < 1, the game's rock reaches higher. The rest of 0x146180 (noise axes and offsets, divisors, the clamp of dz, the 1.0 bound) and the caller's arguments (z = trunc(h + 1), rx = ry = th, rz = tw, flags 0, from 0x314f3a) were re-checked in the assembly and are as written.
