# Alpha world generation: voxel trees

Behavioural specification of the Cube World **Alpha** voxel-tree builder: the routine that
writes a tree's trunk, roots, branches and leaf crowns as blocks into a zone. Written from
`Cube.exe` (SHA-256 `84a7a132…30bf`): the builder 0x5df960 (20099 bytes; its decompile is
unreliable in places, so every formula here was read from the assembly and the binary
constants), the leaf-blob routine 0x5d7f60, the ball and disc writers 0x59cbf0 / 0x59c890 and
the moss share 0x5efdb0. Read the [random spec](alpha-worldgen-random.md) (draws, N, precision
words), the [terrain spec](alpha-worldgen-terrain.md) (block format §1.3, reads §1.4, writes
§1.5, colours §6) and the [vegetation spec](alpha-worldgen-vegetation.md) §5–§6 (where trees are
placed) first.

**Server.** The server contains the same routines: builder **server** 0x513760, leaf blob
**server** 0x50bd60, ball **server** 0x4d4820, disc **server** 0x4d44c0, moss **server**
0x523b90. The builder and the leaf blob were compared instruction by instruction with the
client: they are identical except for absolute addresses, and every constant they read has the
same value in both binaries.

Status: kinds 0, 1, 2, 4, 5 and 6 are specified to the voxel and to the draw. Kind 3 (palm) is
complete except for the palm-leaf models, which are stamped by the structure routine 0x5f0ce0
(open question 1).

---

## 0. Conventions

* As in the random spec §0 and terrain spec §0. **f32(…)** rounds to single precision;
  "[float]" after a formula means every operation in it rounds to single, "[double]" means
  double; where they mix, the f32 marks say where rounding happens. trunc truncates towards
  zero. "a div b" is C integer division (towards zero); "a mod b" the C remainder.
* **D** is one draw (`rand()`), taken at the point where it is written. Every draw listed is
  made exactly once when its step is reached, in the order written. The builder makes no
  `srand` call; it continues the zone's stream (random spec §2.3).
* Arguments: **x, y** block column of the tree, **z** the first cell above the ground,
  **s** size, **t** height, **k** kind (0–6), **Z** the zone being generated, with zone indices
  (zx, zy). **X0 = x + 0.5**, **Y0 = y + 0.5** (the builder holds them as fixed point
  x·65536 + 32768; as doubles they are exact).
* **w** = warmth and **mo** = moisture of column (x, y) (column +0x08 / +0x04 when the column is in
  Z, otherwise the blends 0x5c4dd0 / 0x5c4800; regions spec §4). The builder reads w once at the
  start and mo only where stated.
* **π** in angle formulas is the double 3.141592653589793 (0x6fce08), **2π** the double
  6.283185307179586 (0x702a78); sin and cos are the CRT double functions applied to the
  float angle widened to double, results rounded to float.
* Colours are three floats on a 0..255 scale; **bytes(c)** truncates each channel to an integer
  (no clamping unless "clamp" is written; clamp = each channel limited to [0, 255]).
* Names of the tree kinds are inferences from the shapes (§7).

---

## 1. Callers and arguments

### 1.1 The builder

The builder is called as a member of the world object with seven arguments, in this order:
**x, y, z, s, t, k, Z**, all 32-bit integers except Z (the zone). It returns nothing; its
effects are block writes into Z (§2), plant records appended to Z (§5) and draws.

### 1.2 Call sites

| Caller | Address | Arguments |
|---|---|---|
| zone generator, tree grid | 0x5ea7ae | (x, y, ground + 1, s or 1, t, kind, Z) with x = 256zx + 18i + s, y = 256zy + 18j + s |
| zone generator, location type 12 | 0x5e7231 | (256zx + 128, 256zy + 128, column top, 80, 80, 6, Z) |
| town builder 0x5af170 | 0x5bc200 | (x, y, h + 1, D mod 6 + 7, D′ and 7 + 13, kind, Z) |

* **Tree grid** (vegetation spec §5): s and t are the grid's crown size and height, s replaced
  by 1 when s < 1. The kind is the grid's kind with one more rule that vegetation §5 step 14
  does not list: after the ground search, if warmth(x, y) > 0.8 **and** moisture(x, y) < 0.2,
  the kind becomes **3** (0x5ea6c3..0x5ea75e; both read from the column, or the blends). The
  ground-type test then follows as written there (types 4, 10, or 9 with z + 1 < 3).
* **Giant tree**: z is the column's bottom + count at the zone centre (the first cell above
  the column's stored blocks), after one discarded draw (vegetation spec §6).
* **Town builder**: two draws are made in argument order from right to left: first D′ (height
  = (D′ and 7) + 13), then D (size = D mod 6 + 7). The kind is 3 if warmth(x, y) > 0.8, else 5.
  The call happens only when the builder's value h is greater than 1 (h belongs to the
  structures follow-up).
* **Correction (grid position, 0x5ea030):** the grid passes x = 256zx + 18i + s and
  y = 256zy + 18j + s; vegetation spec §5 step 6 used to give y 6 blocks too large (see the
  correction there). Re-checked at the same time against the assembly, with no error found:
  argument order at 0x5ea7a0..0x5ea7ae (x is pushed last, so it is the first argument);
  X0 = x + 0.5 (0x5df9eb, the constant at 0x702ad8 is −32768 and is subtracted); roots
  (0x5e1332..0x5e1479), round-crown branches (0x5e4122..0x5e4224, 0x5e4411..0x5e4482), the
  layered crown and its clumps (0x5e22b2..0x5e23b2, 0x5e25fa..0x5e2699, 0x5e2936..0x5e29a3)
  and the conifer spokes (0x5e39a5..0x5e3a89) all put cos on x and sin on y with + signs and
  offsets from X0/Y0 (or x/y for the conifer); the zone clamps use zone+0x60 for x and +0x64
  for y; the flare offsets (0x5e1673..0x5e18e9) are symmetric about x + 0.5, y + 0.5; the
  leaf blob (0x5d7f60) loops x outer, y inner, centred on +0.5. None of these biases the
  canopy towards either side.
* Server: the corresponding calls are at **server** 0x51e58e (grid), 0x51b011 (giant tree)
  and 0x4ef970 (town builder **server** 0x4e28e0).

---

## 2. Blocks written and write rules

### 2.1 Block types

| Byte 3 | Type | Written by |
|---|---|---|
| 0x27 | **7 wood**, flag 0x20 | trunks (all kinds but 6), roots, base flare, branches |
| 0x07 | **7 wood**, no flag | the trunk of kind 6 only |
| 0x28 | **8 leaves**, flag 0x20 | leaf blobs, vines, conifer discs, conifer snow caps, conifer top |

**Alpha's leaf type is 8.** Leaf sway that selects blocks by type should select type 8; note that
the snow caps of conifers (§4.10.4) are also type 8. Palm leaves come from models (§4.8.3); their
block type is open question 1. The terrain spec's type table (§1.3) should gain rows 7 (wood) and
8 (leaves).

### 2.2 Reading before writing

Every voxel step of the builder and its helpers first **reads** the target cell exactly as
terrain spec §1.4 describes, restricted to zone Z: a cell whose column is outside Z reads as
the implicit solid grey block. If the block read has bit 0x40 ("dry air", carved overhang
space, terrain §2.3) the cell is **skipped**; otherwise it is written. Only stored blocks can
carry 0x40, so trees overwrite air, water, terrain, other trees' blocks and their own earlier
blocks alike, and never fill overhangs or ravine air that the surface pass marked dry.

### 2.3 Writing

Writes go through the generator's block write (0x44e7a0, terrain spec §1.5) with zone Z:

* A cell whose column is **outside Z is not written** (the column lookup fails); the read
  before it still happened and any draw tied to it is still made. Trees are therefore clipped
  to their own zone; the grid, the kind-5 crown and the round crown move their crowns inward
  for this reason.
* Tree blocks never have type 0, so the "water below sea level" substitution of §1.5 never
  applies; writing below a column's bottom fills the gap with rock exactly as §1.5 says (roots
  of the giant tree can do this); the keep marker (bit 7) behaves as terrain §1.3 says.
* Later writes replace earlier ones. The order of the steps in §4 is therefore part of the
  result (for example the crown replaces the top of the trunk).

---

## 3. Shared helpers

### 3.1 Moss share m(i, j, k) (0x5efdb0)

* n1 = N(f32(f32(0.2·j) + 534), f32(f32(0.2·k) + 13)) (arguments computed in float);
* n2 = N(0.05·i + 4343, 0.1·k + 84734) (double);
* f = f32(f32(f32(n1·0.1) + n2)·0.7) + 0.2 [float];
* m = f32(f·mo(i, j)), limited to [0, 1], where mo(i, j) is the moisture of column (i, j).

Moss blends a colour c towards **(50, 120, 60)**: channel c ← f32(f32((T − c)·m) + c) with
T = 50, 120, 60 [float], then bytes.

Test vectors (mo = 1 / mo = 0.5): m(0, 0, 0) = 0.6660673022270203 / 0.33303365111351013;
m(10, 20, 30) = 0.07561349868774414 / 0.03780674934387207; m(100, 200, 50) =
0.2500426769256592 / 0.1250213384628296; m(5000, 7000, 12) = 0.23285818099975586 /
0.11642909049987793; m(900, 1900, 55) = 0 (f < 0).

### 3.2 Ball (0x59cbf0)

Ball(cx, cy, cz, r, colour bytes c, type byte, centred, moss): integer centre, float radius r.

* Centre C = (cx, cy, cz), plus 0.5 on each axis when **centred** (all tree uses are centred).
* Cells i from trunc(f32(f32(cx − r) − 1)) while i ≤ f32(f32(cx + r) + 1) (float compare);
  j and k likewise with cy and cz. Loop order: i outer, then j, then k, all ascending.
* A cell is inside when ((j − Cy)² + (i − Cx)²) + (k − Cz)² ≤ f32(r·r) (sums in double, in
  that order).
* Inside and not dry air (§2.2): the colour is c, or with **moss** the moss blend of c (§3.1)
  at (i, j, k), where (T − c) uses c as a float. Written with the given type byte.

### 3.3 Disc (0x59c890)

Disc(cx, cy, z, r, c, type byte, centred): the same in one layer z: (j − Cy)² + (i − Cx)² ≤
f32(r·r), loops i then j ascending, centre + 0.5 when centred. The tree uses it without moss.

### 3.4 Leaf blob (0x5d7f60)

Blob(x, y, z, rH, rV, Ca, Cb, k): integer centre, float radii, two float colours, the tree kind.

1. Centre (cx, cy, cz) = (x + 0.5, y + 0.5, z + 0.5) [double].
2. **D1, D2** (two draws), used as noise offsets.
3. Columns: i from trunc((cx − rH) − 1) while i ≤ (rH + cx) + 1, and inside that j from
   trunc((cy − rH) − 1) while j ≤ (rH + cy) + 1 [double].
4. Per column: **off** = f32(0.5·N(0.04i + D1, 0.04j + D2) + f32(0.3·N(0.1i + D1, 0.1j + D2)))
   (noise arguments double; the 0.5 product and the sum in double, the 0.3 product in float);
   H = ((j − cy)/rH)² + ((i − cx)/rH)² [double]. The noise is evaluated for every column.
5. Cells k from trunc((rV + cz) + 1) **downwards** while k ≥ (cz − rV) − 1:
   v = ((k − cz)/rV)·1.7999999523162842 + off [double] (the constant is float 1.8 widened);
   inside when v² + H ≤ 1.
6. Inside and not dry air: τ = f32((v + 1)·0.5) limited to [0, 1];
   colour = clamp(f32(f32(1 − τ)·Ca) + f32(Cb·τ)), bytes; write with byte 0x28.
7. **Vines (kind 4 only)**, right after that write: D; if D mod 20 = 0: D: n = D mod 10;
   D: if (D and 3) = 0, n ← 2n. Then for the n cells below (k − 1, k − 2, …, k − n) in the same
   column: if not dry air, write leaves with τ′ = f32((((k′ − cz)/rV)·1.8 + off + 1)·0.5)
   [double inside, as in 5–6] limited to [0, 1], colour as in 6. Vines ignore the blob shape
   and the ground.

The shape is an ellipsoid of horizontal radius rH and vertical half-height rV/1.8, shifted up
or down per column by the noise (|off| ≤ 0.8), coloured Ca at its bottom and Cb at its top.

---

## 4. The builder, step by step (0x5df960)

### 4.1 Start

rA = rB = f32(s·0.25) (trunk radius at the bottom and at the top). Bark bases
gB = f32(f32(w·50) + 120), bB = f32(f32(w·100) + 50).

### 4.2 The four initial draws: bark colours

**D1, D2, D3, D4**:
**A** = (240, f32(D1 mod 80 + gB), f32(D2 mod 50 + bB)) (bark at the trunk's foot),
**B** = (240, f32(D3 mod 80 + gB), f32(D4 mod 50 + bB)) (bark at the trunk's top).

### 4.3 Per-kind adjustments (no draws)

| Kind | Change |
|---|---|
| 5 | t ← trunc(f32(t·0.7)); s ← min(trunc(f32(s·0.2)), 3); if 8s > t then t ← 8s. The new s and t are used from here on; rA, rB keep the values of §4.1. |
| 1 | rA ← f32(rA·0.5); t ← 2t; rB ← 0.1 (raised to 0.3 below) |
| 6 | rB ← f32(rA·0.5) |
| 3 | rA = rB = 2; A = (240, 180, 120); B = (220, 100, 50) |
| 2 | rA = rB = 1.5; A = B = (255, 255, 255) |
| 0, 4 | none |

Then, for every kind except 2 and 3: rA ← max(rA, 0.3), rB ← max(rB, 0.3).

### 4.4 Leaf palette (draws D5 …)

v = f32(f32(w − 0.5)·10) with the unclamped w; flag **F** = 0; then w is limited to [0, 1]
(all later uses of w are the limited value).

**D5**: the tree gets **green** leaves if f32(D5/32767) < v, or k = 4, or k = 3. Otherwise mo
is read and:

| Case | Draws | L1 (lower leaf colour) | d | L2 (upper leaf colour) | F |
|---|---|---|---|---|---|
| green | D6–D9 | (D6 mod 20, D7 mod 155 + 50, D8 mod 100) | D9 mod 100 + 50 | clamp(L1 + (d, d, 0)) | 0 |
| mo > 0.5, "autumn" | D6–D9 | (D6 mod 55 + 200, D7 mod 100, D8 mod 50) | D9 mod 100 + 50 | clamp(L1 + (d, d, 0)) | 1 |
| mo ≤ 0.5, D6 even, "blossom" | D6–D9 | (D7 mod 55 + 200, 0, D8 mod 200) | D9 mod 100 + 100 | clamp(L1 + (d, d, d)) | 1 |
| mo ≤ 0.5, D6 odd | D6–D10 | (D7 mod 20, D8 mod 100 + 120, D9 mod 100) | D10 mod 150 + 100 | clamp(L1 + (d, d, 0)) | 0 |

("D6 even" means D6 and 1 = 0.) **Kind 2** then overrides: L1 = (150, 255, 0),
L2 = (244, 255, 0), F = 0 (its palette draws were still made). F marks trees that get
leaf-particle records (§5).

### 4.5 Cold tint (w < 0.2)

c = min(1 − f32(f32(w − 0.1)/0.1), 1), q = 1 − c [float], S = snow(x, y) (terrain §6.5):
L1 ← f32(c·S) + f32(q·L1), L2 ← f32(c·S) + f32(q·L2),
A ← (f32(c·120) + f32(q·A.r), f32(c·120) + f32(q·A.g), f32(c·80) + f32(q·A.b)) [float,
per channel]. B is unchanged.

### 4.6 Lean (kind 3 only)

D: jx = f32(f32(0.5 − f32(D/32767))·t); D: jy likewise. Then jx ← 0 if
f32(f32(x + jx) − s) < 256zx or f32(s + f32(x + jx)) ≥ 256zx + 256 (float compares); jy
likewise with y and zy. Other kinds: jx = jy = 0.

### 4.7 Trunk

Columns i from trunc(f32(f32(x − rA) − 1)) while i ≤ f32(f32(x + rA) + 1) (float), j likewise
with y; cells k from **z + t down to z − 2**. For each cell:

1. u = f32((k − z + 2)/f32(t + 2)) (0 at k = z − 2, 1 at the top), u2 = f32(u·u);
   radius r = f32(f32((1 − u2)·rA) + f32(u2·rB)) [float].
2. Target column: (i, j); for kind 3 (trunc(f32(f32(jx·u2) + i)), trunc(f32(f32(jy·u2) + j)))
   (the trunk leans quadratically towards (x + jx, y + jy)).
3. Shape [double, r widened]:
   * r > 0.8 and k = 6: ex = f32(N(0.025j, 0.025k)·f32(5/r)) + (i − X0)/r,
     ey = f32(N(0.025i, 0.025k + 8473)·f32(5/r)) + (j − Y0)/r (noise products in float,
     f32(5/r) a float division; the giant trunk wobbles by up to 5 blocks);
   * r > 0.8, other kinds: ex = (i − X0)/r, ey = (j − Y0)/r;
   * r ≤ 0.8: ex = (i − x)/r, ey = (j − y)/r (thin trunks are centred on the column, so they
     are one column wide).
   Inside when ey² + ex² ≤ 1. The bounds use rA only.
4. Inside and the target cell not dry air: colour c = f32(A·f32(1 − u)) + f32(B·u)
   [per channel]. **Kind 2**: D; if D mod 8 = 0, c = (50, 50, 50) (birch spots; one draw per
   such cell, including cells outside Z). Moss blend (§3.1) at the target cell, bytes, write at
   the target cell with byte 0x07 for kind 6, 0x27 otherwise.

### 4.8 Base: roots, flare or palm leaves

#### 4.8.1 Roots (kind 6)

D: nR = (D and 3) + 5. D: φ = f32(D·2π/32767) [double]. g = f32(rA·0.75).
For root ρ = 0 … nR − 1:

* a = f32(ρ·π·2/nR + φ) [double, in that order]; sa = f32(sin a), ca = f32(cos a).
* P = (X0 + f32(f32(ca·rA)·0.5), Y0 + f32(f32(sa·rA)·0.5), z) [double].
* Steps dX = f32(f32(ca·g)·0.1), dY = f32(f32(sa·g)·0.1), sink limit e = f32(g·0.05).
* For q = 0, 7, 14, …, 140 (21 steps): Ball(trunc PX, trunc PY, trunc PZ, radius
  10 − q div 20, bytes(A), 0x27, centred, moss); PX += dX; PY += dY; D;
  PZ −= f32(f32(D·e)/32767).

The roots run outwards in nR evenly spaced directions, shrinking from radius 10 to 3 and
sinking a random amount per step.

#### 4.8.2 Base flare (kinds 0, 2, 4, 5)

o = −1. For zc = z, then z − 1: r = f32(f32((4 − z + zc)·rA)/5) (0.8·rA, then 0.6·rA);
o ← f32(o + r); four balls of radius r, colour bytes(A), byte 0x27, centred, moss, at
(trunc(f32(x − o)), y, zc), (trunc(f32(x + 1 + o)), y, zc), (x, trunc(f32(y − o)), zc),
(x, trunc(f32(y + 1 + o)), zc), in that order. No draws.

#### 4.8.3 Palm leaves (kind 3)

Crown column Xc = (x·65536 + 32768 + trunc(f32(jx·65536))) div 65536, Yc likewise with y and jy
(64-bit integers). The world's model table (W+0x20) holds palm-leaf.cub at index 2300 and
palm-leaf-diagonal.cub at index 2301 (registered at 0x515aaf / 0x515af9); a model's x and y
sizes are its +0x44 and +0x48 fields (sx, sy).

* If model 2300 exists: four stamps of it, for rotations 0, 1, 2, 3 at
  (Xc − sx div 2, Yc + 1), (Xc + 1, Yc − sx div 2), (Xc − sx div 2, Yc − sy), (Xc − sy, Yc − sx div 2).
* Then, if model 2301 exists (sizes dx, dy): four stamps at rotations 0–3 at
  (Xc + 1, Yc + 1), (Xc + 1, Yc − dy), (Xc − dy, Yc − dy), (Xc − dy, Yc + 1).
* Each stamp first makes one draw D and uses height zp = z + t − 5 + D mod 3. The stamp is the
  structure routine 0x5f0ce0 called with (model, position (x, y, zp), rotation, 0x28, 0, Z, 0,
  four zero integers, 0); its behaviour belongs to the structures follow-up (open question 1).

### 4.9 Marker draw (all kinds)

D; if D mod 10 = 0 and w > 0.2: append a **kind 61** record (§5) at fixed point
(x·65536 + 32768, y·65536 + 32768, (z + t)·65536), with t after §4.3.

### 4.10 Crowns

Kind 3 has no further step; the other kinds continue as follows.

#### 4.10.1 Round crown (kinds 0, 2, 4)

1. D: nb = D mod 5 + 4. D: φ = f32(D·2π/32767). D: sf = f32(f32(D·0.8)/32767) + 0.6 [float].
   D: rC = f32((f32(f32(D·1.25)/32767) + 2)·rA) (the + 2 and the product in double).
2. **Main blob**: Blob(x, y, z + t, rC, f32(rC·sf), L1, L2, k).
3. For each branch b = 0 … nb − 1:
   1. D: h = f32(f32(f32(D·0.5)/32767) + 0.5); zb = f32(f32(h·t) + z).
   2. a = f32(b·π·2/nb + φ); sa, ca. P = (X0 + f32(f32(ca·rA)·0.5), Y0 + f32(f32(sa·rA)·0.5), zb)
      [double]. Branch colour cb = f32(A·f32(1 − h)) + f32(B·h).
   3. g = f32(rA·0.75); six steps: if k is 0 (not 2 or 4) and rA > 1.5, Ball(trunc PX,
      trunc PY, trunc PZ, 2, bytes(cb), 0x27, centred, moss); then PX += f32(f32(ca·g)·0.5),
      PY += f32(f32(sa·g)·0.5); D; PZ += f32(f32(D·f32(g·0.5))/32767).
   4. D: rr = f32((f32(f32(D·1.25)/32767) + 2)·rA).
   5. Keep the blob in the zone [double compares]: if (PX − rr) − 1 < 256zx, PX =
      f32(f32(256zx + rr) + 1); same for PY with zy; then if (rr + PX) + 1 ≥ 256zx + 256, PX =
      f32(f32(256zx + 256 − rr) − 2); same for PY.
   6. If b > 4: PZ += f32(f32(sf·rr)·0.5).
   7. e = f32(f32(sf·rr)·0.25); Blob(trunc PX, trunc PY, trunc(e + PZ), rr, f32(sf·rr), L1, L2, k).
   8. If F: append a **kind 60** record at (trunc(PX·65536), trunc(PY·65536),
      trunc((PZ + e)·65536)) (64-bit truncations of doubles) with colour L1/255.

#### 4.10.2 Layered crown (kind 5; s and t as changed in §4.3)

1. n_L = trunc(f32((t div 4 + 6) + f32(s·0.5))) layers.
2. D, D (both discarded). D: φ0 = f32((D and 7)·π/4) (π/4 = 0.7853981633974483, double).
3. For layer i = 0 … n_L − 1 (with acc = i·t):
   1. If i is odd: D; if D mod 3 ≠ 0 the layer is skipped.
   2. hf = f32(f32(i·0.6)/(n_L − 1)) + 0.6 [float]; zl = f32(f32(t·hf) + z).
   3. a = f32(i·π·0.25 + φ0) [double]; sa, ca. Q = (X0 + f32(ca·rA), Y0 + f32(sa·rA), zl).
   4. Branch colour col = clamp(f32(A·f32(1 − hf)) + f32(B·hf)).
   5. D: n = t div 2 + D mod (t div 2) + (acc div 4) div n_L.
   6. fr = f32(f32(f32(s·8)·f32(1 − hf)) + s); rx = f32(fr·ca), ry = f32(fr·sa).
   7. For j = 0 … 2n (2n ≥ 0 always):
      * e = 1 − (1 − j/(2n))² [float].
      * Fixed-point point PZ = trunc((f32(j·0.5) + zl)·65536), PY = trunc((f32(e·ry) + Q.y)·65536),
        PX = trunc((f32(e·rx) + Q.x)·65536) (doubles truncated to 64-bit); the block containing it
        is (bx, by, bz) = (PX, PY, PZ) div 65536.
      * If the cell (bx, by, bz) **and** the cell above are both not dry air: write bytes(col)
        with 0x27 at (bx, by, bz + 1), then at (bx, by, bz). Then: if j = 2n, leaf clump;
        else if j mod 4 = 0: D, and if D mod 3 = 0, leaf clump. (If either cell is dry air,
        nothing of this bullet happens, draws included.)
      * **Leaf clump**: D: rH = f32(f32(f32(f32(D·f32(e·0.75))/32767) + 0.25)·f32(n·0.5)) + 2.
        D: rV = f32(f32(f32(D·0.5)/32767) + 0.5)·rH. Centre CY = PY +
        trunc(f32(f32(f32(rH·sa)·0.25)·65536)), CX = PX + trunc(f32(f32(f32(rH·ca)·0.25)·65536)),
        RH = trunc(f32(rH·65536)). Zone clamps, in this order: if CX − RH − 65536 < 256zx·65536,
        CX = trunc(f32(f32(f32(256zx + rH) + 1)·65536)); same for CY with zy; if
        CX + RH + 65536 ≥ (256zx + 256)·65536, CX = trunc(f32(f32(f32(256zx + 256 − rH) − 2)·65536));
        same for CY. Blob(CX div 65536, CY div 65536, bz + 1, rH, rV, L1, L2, 5). If F: a kind
        60 record at (CX, CY, PZ + 65536) with colour L1/255.

Each kept layer is one wooden branch, two voxels thick, running from the trunk outwards and
upwards along a quarter-ellipse; successive layers turn by 45° and rise from 0.6t to 1.2t.

#### 4.10.3 Giant crown (kind 6)

1. D: nB = D mod 5 + 8. D: φ = f32(D·2π/32767). D, D (discarded). g = f32(rA·0.75).
2. For branch b = 0 … nB − 1:
   1. D: h = f32(f32(f32(D·0.5)/32767) + 0.5); zb = f32(f32(t·h) + z).
   2. a = f32(b·π·2/nB + φ); sa, ca; a2 = f32(a + π/2) (π/2 = 1.5707963267948966, double);
      s2 = f32(sin a2), c2 = f32(cos a2).
   3. P = (X0 + f32(f32(ca·rA)·0.5), Y0 + f32(f32(sa·rA)·0.5), zb); cb = f32(A·f32(1 − h)) +
      f32(B·h) (not clamped).
   4. For q = 0, 5, 10, …, 200 with step number n = q div 5 (41 steps):
      * ρ = f32(6.5 − f32(q/40)); Ball(trunc PX, trunc PY, trunc PZ, ρ, bytes(cb), 0x27,
        centred, moss).
      * If n > 10, five rounds, each of two halves:
        first half: D; if D mod 120 < n − 10: D: v1 = (D and 3) + 4; D: h1 = (D and 3) + 4;
        D: cz = trunc((D mod M1) + (PZ − 3)); D: cy = trunc(((D mod 20) + PY − 10) + f32(s2·4));
        D: cx = trunc(((D mod 20) + PX − 10) + f32(c2·4)); Blob(cx, cy, cz, h1, v1, L1, L2, 6),
        with M1 = 2 − trunc(f32(ρ·−2)) [sums in double, left to right];
        second half: the same with M2 = (1 − trunc(f32(ρ·−2)))·2 and the s2, c2 terms
        subtracted instead of added.
      * If n > 10 and F: a kind 60 record at (trunc(PX·65536), trunc(PY·65536), trunc(PZ·65536))
        with colour L1/255.
      * PX += f32(f32(ca·g)·0.075); PY += f32(f32(sa·g)·0.075); PZ += f32(f32(f32(g·0.1)·n)/40).

The branches start half a trunk radius out, thin from 6.5 to 1.5, curve upwards more and more,
and from step 11 on throw leaf clumps (radius 4–7) to both sides with rising probability.

#### 4.10.4 Conifer crown (kind 1; t doubled in §4.3; L1, L2 and F unused)

1. D: φ = f32(D·2π/32767).
2. D: p = D mod 100; D: q = D mod 50 + 100; D: r = D mod 100; **C1** = (r, q, p).
   D, D, D likewise: **C2** = (r′, q′, p′).
3. R = s, droop = f32(−s·0.3), segs = t div 3.
4. For spoke n = 0 … 29:
   1. φ ← f32(φ + 0.9424778335276408) (0.3π, added in double).
   2. v = f32(f32(f32(n·0.7)/30) + 0.3); cr = f32(f32(cos φ)·R), sr = f32(f32(sin φ)·R);
      fV = f32(f32(1 − v)·2); hz = f32(f32(v·t) + z).
   3. End point E = (x + f32(fV·cr), y + f32(fV·sr), hz + f32(fV·droop)) [double sums].
   4. For m = segs down to 1: f = f32(m/segs), gm = f32(1 − f);
      P = (f·E.x + gm·x, f·E.y + gm·y, f·E.z + gm·hz) [double].
      * If w < 0.3: Disc(trunc PX, trunc PY, trunc(PZ + 1), 2, bytes(snow(x, y)), 0x28, centred).
      * Disc(trunc PX, trunc PY, trunc PZ, 2, bytes(f32(C1·gm) + f32(C2·f)), 0x28, centred).
   5. Write bytes(C1) with 0x28 at (x, y, z + t + 1), then at (x, y, z + t).

Thirty radius-2 leaf discs per spoke line run from the trunk at height v·t (rising with n) out
to a drooping tip; low spokes are long and high spokes short, which makes a cone.

---

## 5. Records added to the zone

The builder appends 0x3c-byte records to the zone's list at **zone + 0x04** (the same list the
plant pass of vegetation spec §3 appends to through 0x4862e0; open question 4) using 0x5c03f0.

| Offset | Content in tree records |
|---|---|
| +0x00 | kind: **60** (0x3c) or **61** (0x3d) |
| +0x08, +0x10, +0x18 | x, y, z as 64-bit fixed point (1/65536 block) |
| +0x20 … +0x2b | not written by the builder (stale stack contents; an implementation may store 0) |
| +0x2c, +0x30, +0x34 | three floats: L1/255 per channel (kind 60), or 1, 1, 1 (kind 61) |
| +0x38 | integer 2 |

Neither kind has a model in the plant-sprite table, so neither is drawn as a sprite. The
client's per-frame world routine 0x4ac260 handles them: kind 60 (around 0x4b1eb4) now and then
emits small falling particles near the record, apparently in its colour (falling leaves and
petals of autumn and blossom trees); kind 61 (around 0x4acae6, 0x4acf5d) emits something only
between 06:00 and 21:00 world time (open question 3). These particle draws use the render
thread's random state, not the generator's.

No light records are made.

---

## 6. Draw order summary

| Step | Draws |
|---|---|
| §4.2 bark | 4 |
| §4.4 palette | 1 + 4 (green, autumn, blossom) or 1 + 5 (odd-D6 green) |
| §4.6 lean (k = 3) | 2 |
| §4.7 trunk (k = 2) | 1 per trunk cell that passes the shape and dry-air tests |
| §4.8.1 roots (k = 6) | 2 + 21 per root |
| §4.8.3 palm (k = 3) | 1 per stamp made (up to 8), plus whatever the stamp routine draws (open question 1) |
| §4.9 marker | 1 |
| §4.10.1 round | 4; main blob 2; per branch 1 + 6 + 1 + blob 2 |
| §4.10.2 layered | 3; per layer: 1 if odd, then 1 if built; per branch cell: see §4.10.2 7; per clump 2 + blob 2 |
| §4.10.3 giant | 4; per branch 1; per step with n > 10: 10 tests, 5 + blob 2 per success |
| §4.10.4 conifer | 7 |
| every blob, kind 4 | per written cell: 1, and 2 more when it hangs a vine |

The plant pass of vegetation spec §3 continues the stream after the last tree of the grid.

---

## 7. What the kinds look like (inferred)

| Kind | Placed by | Shape |
|---|---|---|
| 0 | grid | round broadleaf: trunk radius s/4, height t, root flare, big leaf blob and 4–8 short wooden branches with leaf blobs |
| 1 | grid (cold) | conifer: one-column trunk 2t tall (base radius s/8 down to 0.3), cone of drooping disc layers in dark green (C1, C2), snow caps where w < 0.3 |
| 2 | grid | birch: white trunk radius 1.5 with grey spots, yellow-green leaves, leaf blobs without branch wood |
| 3 | grid (hot and dry), towns (hot) | palm: radius-2 trunk leaning up to t/2, sandy bark (240, 180, 120) → (220, 100, 50), palm-leaf models |
| 4 | grid (hot and wet) | jungle tree: like 0 but always green, no branch wood, vines hanging from the leaf blobs |
| 5 | grid, towns | layered tree: trunk 0.7t tall, spiral of horizontal branches with leaf clumps |
| 6 | location type 12 | giant tree: trunk radius 20 tapering to 10, noise-distorted, 5–8 sinking roots, 8–12 rising branches with many leaf clumps |

Leaf colour families (§4.4): warm climates green; cool and wet autumn red-orange-yellow; cool and
dry either pink-to-white blossom or green. Moss (§3.1) greens the bark in moist places.

---

## 8. Worked example (test vectors)

Computed from this spec's formulas (not from a game dump). Setting: the generator state is
`srand(123)` with no draw made yet; zone Z = (0, 0); every cell touched is plain air (no dry
air); w = 0.5 at (100, 200) and mo = 0.45 in every column. Call: x = 100, y = 200, z = 50,
s = 8, t = 10, k = 0.

* Bark: D1–D4 = 440, 19053, 23075, 13104 → A = (240, 185, 103), B = (240, 180, 104);
  rA = rB = 2.
* Palette: D5 = 32363, v = 0 → not green; mo ≤ 0.5; D6 = 3265 (odd); D7–D10 = 30749, 32678,
  9760, 28064 → L1 = (9, 198, 60), d = 114, L2 = (123, 255, 60); F = 0.
* Trunk: 156 cells = 12 columns × z 48 … 60, written first at (99, 200, 60), (99, 200, 59), … .
  Column (100, 200) from k = 60 down to 48 is written with bytes (222, 174, 99), (222, 174, 99),
  (220, 174, 99), (218, 174, 98), (216, 173, 98), (214, 173, 97), (214, 173, 97),
  (214, 174, 97), (216, 175, 97), (218, 176, 98), (218, 176, 98), (217, 176, 98),
  (214, 176, 97), all with byte 0x27 (the shading comes from moss).
* Flare: radii 1.6 and 1.2, offsets o = 0.6 and 1.8 (balls centred on x 99, 101, y 199, 201
  at z 50, then x 98, 102, y 198, 202 at z 49).
* Marker draw: no record. Crown: nb = 4, φ = 0.8590554594993591, sf = 0.6943144202232361,
  rC = 6.321619033813477; main blob at (100, 200, 60).
* Branch ends (b, h, a, final P, rr, blob centre):
  (0, 0.982116162776947, 0.8590554594993591, (104.092342, 204.664743, 61.644465),
  4.94477653503418, (104, 204, 62));
  (1, 0.9562822580337524, 2.429851770401001, (96.335257, 204.092342, 61.388461),
  4.171742916107178, (96, 204, 62));
  (2, 0.8698538541793823, 4.000648021697998, (96.907658, 196.335257, 61.549396),
  5.749092102050781, (96, 196, 62));
  (3, 0.5567949414253235, 5.571444511413574, (104.664743, 196.907659, 58.135142),
  5.030381202697754, (104, 196, 59)).
* Totals: 57 draws (the next draw is 27840); 2244 writes; 1293 distinct cells, of which
  961 are type 8 and 332 type 7. Final cells: (100, 200, 50) = (218, 177, 98) wood,
  (100, 200, 58) = (220, 173, 98) wood, (100, 200, 61) = (222, 174, 98) wood (a branch ball),
  (104, 204, 62) = (53, 220, 60) leaves, (100, 200, 65) not written. The last three writes are
  leaves at (109, 197, 59) = (47, 217, 60), (109, 198, 60) = (76, 231, 60),
  (109, 198, 59) = (47, 217, 60).

Leaf blob on its own: generator state `srand(1)`, zone (0, 0), all air, Blob(100, 100, 20, 3, 2,
Ca = (0, 100, 0), Cb = (100, 200, 50), kind 0): 46 cells in x 98 … 103, y 98 … 103, 31 at z 20 and
15 at z 21; (100, 100, 20) = (39, 139, 19), (100, 100, 21) = (84, 184, 42),
(98, 100, 20) = (41, 141, 20), (102, 101, 20) = (40, 140, 20), (100, 103, 20) = (37, 137, 18);
(100, 100, 19) not written.

Ball: radius 2 centred covers 32 cells; the same ball not centred covers 33.

---

## 9. Checking against the running game

* With a 32-bit Alpha mod DLL: snapshot a zone's columns in an area, call 0x5df960 (thiscall,
  ECX = W, seven stack arguments as §1.1) on a test zone with a chosen `srand` state, and diff
  the blocks, the zone + 0x04 list and the final `rand` state against a reimplementation. Do
  this per kind; kind 3 also exercises 0x5f0ce0.
* 0x5d7f60, 0x59cbf0 and 0x5efdb0 can be called alone with the §3 and §8 inputs.
* Hooking `rand` during one zone's tree grid gives the draw count per tree; with the grid's
  draws (vegetation §5) this checks §6.
* Visual: Alpha forests show light orange-brown trunks with moss-green shading, round green
  crowns made of noisy blobs, white birches with grey spots, dark green snow-capped conifer cones
  in cold regions, red-orange or pink crowns in cool regions, palms in hot deserts and towns.

---

## 10. Open questions

1. **Palm-leaf models (kind 3).** The stamp routine 0x5f0ce0 (15724 bytes, 33 `rand` call
   sites) writes the palm-leaf models. Which of its draws run for these calls (structure kind
   argument 0), the block type of the stamped voxels (whether the 0x28 argument makes them
   type 8, or the model supplies it) and their colours belong to the structures follow-up.
   Verify by hooking `rand` and the block write while a palm is built.
2. **Rounding of the 64-bit conversions.** 0x68d946 is taken to truncate towards zero like the
   other conversions (regions spec open question 3); the kind-5 crown and the records depend
   on it.
3. **Meaning of records 60 and 61.** Their handling in 0x4ac260 was only skimmed: kind 60
   appears to emit falling-leaf particles in the record's colour, kind 61 a daytime effect at
   the crown top (perhaps birds or insects). This affects rendering only.
4. **Zone lists.** The tree records and the plant pass use the list at zone + 0x04, while the
   vegetation spec names zone + 0x24 as the plant list; one of the two descriptions needs a
   check (dump both lists after a zone loads).
5. **Town calls.** The value h that gates the town builder's call (§1.2) and its x, y come from
   0x5af170 (structures follow-up).
6. **Implicit block bytes.** The ball and disc writers use their own copies of the implicit
   blocks (0x76b7cc..0x76b7d4) rather than the terrain reader's; they are assumed to have the
   same bytes (bit 0x40 clear). A memory dump of both triples after start-up settles it.
