# World generator, part B: the building class (the "mason")

Part of `worldgen-structures.md` (conventions in its §0). Buildings, castles and set pieces draw
through one class, made by 0xba9a0 (identity frame) or 0xba8c0 → 0xbfa30 (a frame at a cell).
It draws in its own integer coordinates (local x, y, z) mapped to the world by a 4 × 4 integer
matrix, and colours blocks from a scheme of up to three colours that blend with height, each
with a pattern. This file calls an instance a **mason**.

---

## 1. State and frame

| Field | Offset | Initial value |
|---|---|---|
| scheme: colours c0, c1, c2 (3 floats each) | +0x10 | (255, 255, 255) each |
| scheme: patterns p0, p1, p2 | | 0 |
| scheme: heights u0, u1, u2 | | 0 |
| scheme: a parameter (used by patterns 4–7) | | 0 |
| scheme: relative flag | | false |
| cap colours | +0x54, +0x60 | (255, 255, 255) |
| cap pattern | +0x6c | 0 |
| write mode | +0x70 | 0 |
| water line | +0x74 | 0 |
| block type | +0x78 | 6 |
| matrix M (column-major, 16 ints) | +0x7c | identity |

The scheme is 0x41 bytes at +0x10 and is copied whole where the builders copy it (0x2fa150,
0x31c470).

World position of local (x, y, z): X = M0·x + M4·y + M8·z + M12; Y = M1·x + M5·y + M9·z + M13;
Z = M2·x + M6·y + M10·z + M14. "The column (x, y) is in the zone" uses X and Y with z = 0.

**Frame at a cell** 0xba8c0 (x, y, z, sx, sy, rot, mirror):
1. Translate: M12 += M0·x + M4·y + M8·z, and likewise M13, M14, M15 with rows 1, 2, 3.
2. r = rot mod 4 (C remainder). r = 1: add (sy − 1) × column 1 to column 3 (M12..M15 += M4..M7 ×
   (sy − 1)). r = 2: add (sx − 1) × column 0, then (sy − 1) × column 1. r = 3: (sx − 1) × column 0.
3. Turn (0xbd4a0) by rot: (c, s) = (1, 0) for rot mod 4 = 0, (0, −1) for 1, (−1, 0) for 2,
   (0, 1) for 3; for each row i: a = M[i], b = M[4 + i]; M[i] = b·s + a·c; M[4 + i] = b·c − a·s.
4. When mirrored: add (sy − 1 when rot is odd, else sx − 1) × column 0 to column 3, then negate
   column 0.

Every builder that says "a mason at the cell" uses frame (16 cx, 16 cy, 0, 16, 16, 0, false).

**A child mason** 0xbaa80 → 0xbf930 (x, y, z, sx, sy, rot, mirror): a new mason with frame
(x, y, z, sx, sy, rot, mirror) built as above from the identity; it takes the parent's scheme
with u0, u1, u2 each reduced by float z, fresh caps and modes; its matrix becomes parent × child
(0xbabb0): result[col][row] = Σk parent[k][row] × child[col][k], k = 0..3.

**Facing** 0xc15d0 (d): when M0 is 1 or −1: d when M5 = 1, else (d + 2) mod 4. Otherwise
(d + 3) mod 4 when M4 = 1, else (d + 1) mod 4.

## 2. Schemes

**One colour** 0xbebe0 (c, p): c0 = c1 = c2 = c, p0 = p1 = p2 = p, u0 = u1 = u2 = 0.
**Two colours** 0xbf8c0 (low, pl, ul, high, ph, uh, relative): c0 = low, p0 = pl, u0 = ul;
c1 = c2 = high, p1 = p2 = ph, u1 = u2 = uh; the relative flag.
Some builders set the three slots directly; each says which.

**Colour at local (x, y, z)** 0xbb9e0:
1. h = float z; when relative, h −= the ground height at the world (X, Y) of (x, y) with z = 0,
   mode 8 (part A).
2. When u0 ≥ h: pattern p0 of c0.
3. Else when u1 > h: when u0 ≥ u1, pattern p1 of c1; else f = (h − u0) / (u1 − u0), the blend of
   (p1 of c1) × f + (p0 of c0) × (1 − f), channel by channel (a·f + b·g, g = 1 − f).
4. Else when c2 is exactly (0, 0, 0): pattern p1 of c1.
5. Else unless u2 > h, or when u1 ≥ u2: pattern p2 of c2.
6. Else f = (h − u1) / (u2 − u1): the blend of p2 of c2 and p1 of c1 as in 3.

The patterned colour is computed for each slot used (both, for a blend) before blending.

**Patterns** 0xc1140 (c, pattern, local x, y, z):
* 0: c.
* 1 (0xbb660, stone with light patches): world (X, Y, Z); sx = ((Z + X) / 2) × 154 + Z × 53,
  sy = ((Z + Y) / 2) × 154 + Z × 53 (C division by 2, unsigned multiply-add); n = S(sx, sy);
  when n > 0.7: k = (n − 0.7) / 0.3 × 0.1 + 1, the colour × k; else c.
* 3: the river bed colour (0xbd110) at the world position.
* 4: n = S(x × 77, param × 97) × 0.15 + 1 (local x; integer products, then double); c × n.
* 5 (0xc2460): S(y × 77, param × 97). 6 (0xc2510): S(z × 77, param × 97). 7 (0xc23b0):
  S(x × 77 + param, y × 97 + param). Each × 0.15 + 1, times c.
* 2 (0xbe8d0, the ground's colour): open question 4 of the main file.

**Block** (type t, flags): the colour at (x, y, z) packed (× 0.5, clamped, truncated), glow 0.

## 3. Writing

**Put** 0xbf600 (local x, y, z, block): world (X, Y, Z).
1. Water line > 0 (0x2d3690): w = S(X × 0.05 + 9076, Y × 0.05 + 8998) × 10 + float(water line)
   + the ground height (X, Y, mode 8); skip when float Z > w.
2. Write mode ≠ 0: read the world block g; mode bit 1: skip when g is solid; bit 2: skip when g
   has flag 2; bit 4: skip unless g has flag 2.
3. Store (main file §3).

**Get** 0xbc340: the lookup at the world position.

**Ground below** (used by foundations and pillars): from local z down, the first z whose block
is solid and has no flag 1.

**Caps** 0xbd330 (x, y, z, mode) — whether a column's top block is a cap: mode 1 → no; mode 2 →
S(X × 0.06 + 7631, Y × 0.06 + 983) > 0; mode 3 → k = double(Z × 53); S(double((Z + X) / 2 × 2) ×
0.1 + k, double((Z + Y) / 2 × 2) × 0.1 + k) > −0.3; any other mode → yes.

**Cap block** 0xbec30 (x, y, z, kind, flags):
* 2: the grass colour at (X, Y) (0x2b40b0) packed, type 4.
* 8: v = 127 when (y / 4 + x / 4) is odd (local, C division), else 50; (v, v, v), type 6.
* 0, 7: the mason's block of its own type.
* 9: when the block at (x, y, z) and the one above are both solid: the colour packed, type 6;
  otherwise as 6.
* 6: the surface colour at the world (X, Y, Z) with that column's values (0xbbfe0), packed,
  flags |= the given flags, stored directly (no put rules).
* others: open question 4.

**Column** (x, y, from, z, h, capKind, capMode, body type): capped = caps (x, y, z + h − 1,
capMode); end = z + h when capped, else z + h − 1; for bz from `from` to end − 1: at end − 1 when
capped the cap block with flags 2, else the block of the body type with flags 2.

## 4. Shapes

All loop x outer, y inner, z innermost, and skip columns outside the zone where noted "(in zone)".

| Shape | RVA | Arguments | What |
|---|---|---|---|
| box | 0xbb400 | x0, y0, z0, sx, sy, sz | (in zone) every block of the box: the block of the mason's type, flags 2 |
| foundation | 0xc1d60 | x0, y0, z, sx, sy, h, capKind, capMode | (in zone) each column: column (x, y, ground below z, z, h, capKind, capMode, the mason's type) |
| pillar | 0xbcc90 | x0, y0, z, r, h, capKind, capMode | (in zone) for x in [x0 − r, x0 + r], y likewise: dx = float(x − x0) + 0.5, dy likewise; skip when dx² + dy² ≥ float(r²); column from the ground below z, body type 6 |
| disc | 0xbc5b0 | x0, y0, z, r, h, capKind, capMode | as the pillar, but the column starts at z and uses the mason's type |
| cone | 0xbbcd0 | x0, y0, z, r, h | (in zone) as the disc's test with d² < float(r²); top = trunc((1 − sqrtf d² / float r) × float h + float(z + 1)); Z from z to top − 1: the block of the mason's type, flags 0 |
| liquid | 0xc2180, 0xbd7c0, 0xbe4d0 | x0, y0, z0, sx, sy, sz, flags, kind | (in zone) each open block of the box: kind 1 water (0x7f, 0x7f, 0x7f, 0, 2, flags); 2 lava: n = S(X × 0.1, Y × 0.1) + S(X × 0.05, Y × 0.05), packed (255, n × 50 + 50, 20), glow 255, type 17; 3 poison: n = S(X × 0.06, Y × 0.06) × 20 + 120, packed (30, n, 40), glow 0, type 19 |
| air | 0xbc400 | x0, y0, z0, sx, sy, sz, flags | (in zone) put (0, 0, 0, 0, 0, flags | 2) |
| light disc | 0xbc990 | x0, y0, z, r, h, flags | (in zone) disc test with d² < float(r²); n = max((S(X × 0.05, Y × 0.05) × 1.5 + 0.5) × 127.5, 0) (X, Y the world position at z = 0); v = trunc n; Z from z to z + h − 1: put (0, 0, 0, v, 0, flags) |
| tube | 0xc1610 along x, 0xc19b0 along y | x0, y0, z0, z1, len, r | along a from x0 (or y0) to + len − 1, rise starting at 0 and growing by z1 − z0: zc = z0 + rise / len; across b in [c − r, c + r] (c the other start coordinate); (in zone) Z in [zc − r, zc + r]: dz = float(Z − zc) − 0.5, db = float(b − c) − 0.5; when dz² + db² ≤ float(r²): f = float(a mod 2) × 0.1 + 0.9; the colour × f packed, the mason's type, flags 0 |
| hanging cone | 0xc0cd0 | x, y, z, r, h | Z from z − h to z + 1: rr = float(Z − z + h) / float h × float r; X from trunc(float x − rr − 1.5) while float X ≤ float x + rr + 1.5, Y likewise; (in zone) a = S(Y′ × 0.1, Z × 0.1) × 1.5 + float(X − x) with Y′ the world y, b = S(X′ × 0.1, Z × 0.1) × 1.5 + float(Y − y) with X′ the world x; when a² + b² ≤ rr²: block type 6, flags 0x22 |
| striped wall | 0xbd590 | x, y, z, w, h | for j < w, X = x − w / 2 + j, Z from z to z + h − 1: f = float(Z mod 2) × 0.1 + 0.9; the colour at (X, y, Z) × f packed, type 16, flags 0 |
| square post | 0xbade0 | x, y, z, r, h, grounded | x0 = x − r, y0 = y − r, s = 2r + 2: grounded → foundation (x0 − 1, y0 − 1, z, s, s, 2, 0, 0), else box (x0 − 1, y0 − 1, z, s, s, 2); box (x0, y0, z + 2, 2r, 2r, h − 2); box (x0 − 1, y0 − 1, z + h − 2, s, s, 2) |
| capped post | 0xbaca0 | as above | the first two as the square post; box (x0 − 1, y0 − 1, z + h − 3, s, s, 3); box (x0 − 2, y0 − 2, z + h − 1, 2r + 4, 2r + 4, 1) |
| round post | 0xbeac0 | x, y, z, r, h, grounded | grounded → pillar (x, y, z, r + 1, 2, 0, 0), else disc (same); disc (x, y, z + 2, r, h − 2, 0, 0); disc (x, y, z + h − 3, r + 1, 3, 0, 0); disc (x, y, z + h − 1, r + 2, 1, 0, 0) |
| battlements | 0xbb170 along y, 0xbaef0 along x | x0, y0, z, len, w, h | for i < len, skipping i with (i / w) mod 2 = 1; k < w: the column (x0 + k, y0 + i) along y or (x0 + i, y0 + k) along x; (in zone) Z from z to z + h − 1: the colour packed, type 6, flags 0 |
| ramp | 0xc0430 along x, 0xc0880 along y | x0, y0, z, sx, sy, a, b, capKind, capMode | (in zone) each column: g = z − 1, down while the block is open, then g + 1; t = x − x0 (or y − y0), n = sx (or sy); top = z + t × (b − a) / n + a (C division); capped = caps (x, y, top − 1, capMode); end = top or top − 1; Z from g to end − 1: the cap (flags 2) at end − 1 when capped, else the block of the mason's type, flags 2 |
| spiral ring | 0xc0030 | x0, y0, z, rIn, rOut, h | x, y in [·0 − rOut, ·0 + rOut]; (in zone) fx = float(x − x0) − 0.5, fy likewise; d² = fy² + fx²; skip when d² ≥ float(rOut²) or float(rIn²) > d²; a = acosf(fx / sqrtf d²); g = z, down until a solid block without flag 1; top = trunc(double(float h × a) / π + double z); Z from g to top − 1: the colour packed, the mason's type, flags 0 |
