# Alpha world generation: dungeons, camps, boulders, sites and towns

Behavioural specification of the structure builders that the Cube World **Alpha** zone
generator 0x5e4850 runs after its terrain passes and before the voxel-tree grid: the dungeon
builder 0x5cc510, the camp attempts and camp routine 0x5ad320, boulder pass 3, the type-10
site statics, the town builder 0x5af170 and the home building 0x5f0ce0. Written from
`Cube.exe` (SHA-256 `84a7a132…30bf`); the decompiler output was used as a reading aid and the
draw sites, constants and argument lists were checked against the assembly where stated.

This document continues [alpha-worldgen-caves.md](alpha-worldgen-caves.md) §6.1 and §8.
Read first: [random](alpha-worldgen-random.md) (rand, N, f32 rules), [terrain](alpha-worldgen-terrain.md)
(columns, block format, read/write rules, colours), [locations](alpha-worldgen-locations.md)
(zone entry E, location records), [vegetation](alpha-worldgen-vegetation.md) (static objects,
model tables), [trees](alpha-worldgen-trees.md) (voxel-tree builder).

**Status.** The builders are very large (the dungeon builder is 9,454 instructions, the town
builder 15,821) and many of their draws depend on the terrain they meet and on prefab model
data. The dungeon builder (§1) and the prefab placer (§6.2) are specified to the draw except
for the item generators they call; the town builder (§5) is specified to the draw throughout:
plot survey, special plots, plinths, markets, gardens, modular houses with their modules,
roofs, floors, furniture and outdoor goods, the special buildings, plaza trees and benches,
the animals of open plots, the town residents and vendors and the ruin creatures. What a
placement draws still depends on the marker voxels of each model (§6.2) and on the terrain.
§8 collects what remains open. Sections were written in the order dungeon, camps,
boulder pass 3, type-10 site, town, home building.

---

## 0. Conventions

* As in the caves spec §0 (f32, trunc, double sums in N arguments, x outer / y inner, draw =
  one `rand()` from the zone's stream, zone-local column lookups, read/write rules).
* **draw & 1 = 0** etc. are written plainly; the code's sign fix-ups never trigger (draws are
  non-negative).
* "**even(d)**" means a draw whose low bit is 0.
* Byte colours are written (R, G, B). A colour assembled from three draws d1, d2, d3 made in
  that order is written with the draw that lands in each channel, e.g.
  (d3 % 50 + 20, d2 % 50 + 20, d1 % 50 + 20) means the **third** draw is red.
* **Model slot k**: entry k of the model list at world + 0x20. The list is filled at start-up
  by the model registry 0x4ee230, which loads each `.cub` key from `data1.db` into a fixed slot
  (`PUSH name … PUSH k … CALL 0x468c60`); slot names below come from that routine
  (assumption: world + 0x20 is that registry's vector; open question 1).
* **Creature record**: 0x10f0-byte object built by 0x5adb20 (caves spec §7); fields named by
  offset. **Static object**: record built by 0x58eaa0 and inserted with 0x5a0e00 (vegetation
  spec §1.3). **Item record**: 0x148 bytes, 0x44a970, appended with 0x44d590.

---

## 1. The dungeon builder (0x5cc510)

### 1.1 Call and inputs

Called by the zone generator (caves spec §6.1) as builder(zone, world, x0, y0, v, flag):

* E.kind = 3: (x0, y0) = (256zx + 32, 256zy + 32), **v** = E.variant (0..5), flag = 0;
* E.kind = 5: v = 3, flag = 1 (never happens in fresh worlds).

It reads the zone's level **lv** = zone + 0x80 and sub-level **sl** = byte zone + 0x84
(locations spec §1.1). It never re-seeds. The dungeon is a grid of 22 × 22 × 22 **cells**,
each 10 × 10 × 10 blocks, so it spans blocks x0 … x0 + 219 (the zone columns 32 … 251) and
likewise in y. All block writes go through the block writer (zone-local).

### 1.2 Style setup (no draws except variants 1 and 2)

The builder fills two creature-kind tables: a single-kind list **K1** and a list of kind
groups **K2** (each group a short list of creature kinds). By v (jump table 0x5d5f90):

| v | K1 | K2 groups | Draws |
|---|---|---|---|
| 0, 4 (and any other) | 11, 12 | {46, 19} | 0 |
| 1, 2 | 15, 16 | {15, 16, 96}, then one more group by draw | **1**: d % 3 = 1 → {94}, = 2 → {17}, = 0 → {97} |
| 3 | 2, 3 | {2, 3, 19} | 0 |
| 5 | 78, 77 | {17, 81}, {62, 30} | 0 |

(The second group of v = 1, 2 is a one-kind group appended after the first.) The kinds are
creature types. Each K2 group is really a pair (leaders, escorts); §1.10 gives the exact
split and the use.

### 1.3 Room parameters (162 draws)

There are 27 **rooms**, one per position (a, b, c) of a 3 × 3 × 3 grid, index
r = 9a + 3b + c. Each room has four flag bytes **present**, **marked**, **quiet**, **fixed**
(all 0 to start) and six integers p0 … p5. For a = 0..2 (outer), b = 0..2, c = 0..2 (inner),
six draws:

1. p5 = d % 3;
2. p4 = d % 3 − 1;
3. p3 = d % 3 − 1;
4. p2 = d % 3 − 1;
5. p1 = d & 1;
6. p0 = d & 1.

### 1.4 Room selection and links

A **link** is a pair of grid positions (a, b, c) → (a′, b′, c′). Exactly one of these tables
applies; flags not mentioned stay 0. "present iff even(d)" consumes one draw.

**v = 2, flag = 0** (draws: 1 discarded, then 2):

1. One draw, value unused.
2. Present: rooms 11, 16, 25, 22, 19, 7; then room 4 present iff even(d); rooms 0, 3
   present; room 6 present iff even(d); room 12 present and marked.
3. Links, in order: (1,0,2)→(1,2,1), (1,2,1)→(0,2,1), (1,2,1)→(2,2,1), (0,2,1)→(0,1,1),
   (2,2,1)→(2,1,1), (2,1,1)→(2,0,1), (2,0,1)→(0,0,0), (0,0,0)→(0,1,0), (0,1,0)→(0,2,0),
   (0,1,0)→(1,1,0).

**v = 0, 1, 3, flag = 0** (draws: 1 + 3): draw s = d % 3; room 10 present; then

* s = 1: room 19 present iff even(d); rooms 13, 4 present; room 1 present iff even(d);
  rooms 7, 16 present; room 25 present and marked; room 22 present iff even(d).
  Links: (1,1,1)→(1,0,1), (2,0,1)→(1,0,1), (0,1,1)→(1,1,1), (0,0,1)→(0,1,1),
  (0,2,1)→(0,1,1), (1,2,1)→(0,2,1), (2,2,1)→(1,2,1), (2,1,1)→(2,2,1).
* s = 2: room 19 present iff even(d); rooms 1, 4 present; room 7 present iff even(d);
  rooms 13, 22 present; room 25 present iff even(d); room 16 present and marked.
  Links: (0,0,1)→(1,0,1), (2,0,1)→(1,0,1), (2,1,1)→(2,0,1), (0,1,1)→(0,0,1),
  (1,1,1)→(0,1,1), (0,2,1)→(0,1,1), (1,2,1)→(1,1,1), (2,1,1)→(1,1,1), (2,2,1)→(2,1,1).
* s = 0: room 19 present iff even(d); rooms 1, 4 present; room 7 present iff even(d);
  room 13 present; room 16 present iff even(d); room 22 present; room 25 present and
  marked. Links: (0,0,1)→(1,0,1), (2,0,1)→(1,0,1), (0,1,1)→(0,0,1), (1,1,1)→(0,1,1),
  (0,2,1)→(0,1,1), (1,2,1)→(1,1,1), (2,1,1)→(1,1,1), (2,2,1)→(2,1,1).

**v = 4, 5, flag = 0** (draws: 1 discarded, then 2): room 14 present, quiet and fixed;
rooms 16, 25, 22, 19, 7 present; room 4 present iff even(d); rooms 0, 3 present; room 6
present iff even(d); room 12 present and marked. Links: (1,1,2)→(1,2,1), (1,2,1)→(0,2,1),
(1,2,1)→(2,2,1), (0,2,1)→(0,1,1), (2,2,1)→(2,1,1), (2,1,1)→(2,0,1), (2,0,1)→(0,0,0),
(0,0,0)→(0,1,0), (0,1,0)→(0,2,0), (0,1,0)→(1,1,0).

**flag = 1** (any v; no draws): rooms 13, 16, 4, 22, 1, 19 present. Links: (1,1,1)→(1,2,1),
(1,1,1)→(0,1,1), (1,1,1)→(2,1,1), (0,1,1)→(0,0,1), (2,1,1)→(2,0,1).

Room indices used: 0 (0,0,0), 1 (0,0,1), 3 (0,1,0), 4 (0,1,1), 6 (0,2,0), 7 (0,2,1),
10 (1,0,1), 11 (1,0,2), 12 (1,1,0), 13 (1,1,1), 14 (1,1,2), 16 (1,2,1), 19 (2,0,1),
22 (2,1,1), 25 (2,2,1).

### 1.5 The cell grid and room carving

The **grid** (object 0x5c38c0, 22 × 22 × 22 two-byte cells, all zero) has a rotation
**rot** (0..3) and a mirror flag **mir**, both 0 while rooms and corridors are carved. Cell
byte 0 is the cell **kind**; byte 1 holds flags. Kinds: 0 solid/outside, 2 **pillar**,
3 **open**, 4 **entrance**. Flag bits: 1 **stair** (vertical corridor step), 2 **floor
patch**, 4 **mark**, 0x100 in the 16-bit cell = flag 1, 0x200 = flag 2, 0x400 = flag 4
(as tested in §1.10).

**View transform** (0x5fa3c0, used by every cell access): a logical cell (a, b, c) is stored
at (A, B, c) with rot 0: (a, b); rot 1: (21 − b, a); rot 2: (21 − a, 21 − b);
rot 3: (b, 21 − a); then if mir: B ← 21 − B. Accesses outside 0..21 read a scratch cell of
kind 1 and writes to them are lost.

For a = 0..2 (outer), b, c = 0..2, each **present** room:

1. If fixed: p0 = p1 = p2 = 0 and (p3, p4, p5) = (0, 0, 1).
2. Room box: position P = (7a + 2 + p0, 7b + 2 + p1, 7c + 3 + p2), size
   S = (p3 + 3, p4 + 3, p5 + 1).
3. If neither marked nor fixed: draw; **pit** = (d % 3 = 0). Otherwise pit = false, no draw.
4. **Room builder** 0x5ef8f0(P, S, pit):
   1. every cell of the box becomes kind 3;
   2. draw; if odd: draw w = d % Sx + 1; draw h = d % Sy + 1; u = P.x, v = P.y; if Sx − w > 0:
      draw, u += d % (Sx − w + 1); if Sy − h > 0: draw, v += d % (Sy − h + 1); the cells
      (u … u + w − 1, v … v + h − 1, P.z) get flag 2;
   3. if pit and Sx > 2 and Sy > 2: draw w = d % (Sx − 2) + 1; draw h = d % (Sy − 2) + 1;
      u = P.x + 1, v = P.y + 1; if Sx − w > 1: draw, u += d % (Sx − w − 1); if Sy − h > 1:
      draw, v += d % (Sy − h − 1); the cells (u … u + w − 1, v … v + h − 1,
      P.z … P.z + Sz − 1) become kind 2.
5. If marked: the cell (P.x + Sx/2, P.y + Sy/2, P.z + Sz/2) gets flag 4 (integer halves).
   (Checked at 0x5ce16d..0x5ce1ae: this is the mark only. It sets a flag bit and does not
   change the cell's kind. The entrance does **not** use this formula; see §1.7 step 1.)

### 1.6 Corridors (0x5c52a0)

For each link in table order whose two rooms are both present, with room A the link's first
position and B its second (boxes PA, SA and PB, SB as in §1.5):

* If the x ranges overlap (PB.x < PA.x + SA.x and PA.x < PB.x + SB.x): m = SB.x/2 + PB.x
  clamped to [PA.x, PA.x + SA.x − 1]. The corridor runs along y at x = m:
  * if PB.y < PA.y: line from (m, PB.y + SB.y, PB.z) to (m, PA.y, PA.z); else line from
    (m, PA.y + SA.y − 1, PA.z) to (m, PB.y − 1, PB.z);
  * draw: if odd and PA.x < m and PB.x < m: a second line one cell lower in x (m − 1), same
    ends;
  * draw: if odd and m < PA.x + SA.x − 1 and m < PB.x + SB.x − 1: a third line at m + 1.
* Otherwise, if the y ranges overlap: the same with x and y exchanged (m from PB.y, SB.y;
  lines along x; the second line at m − 1, the third at m + 1, with the same two draws).
* Otherwise nothing and **no draws**.

So each link between present rooms costs 2 draws if the rooms overlap in x or y, else 0.
A **line** (0x5ef6d0) from F to T: n = max(|Δx|, |Δy|, |Δz|); if n > 0, for t = 0 … n the cell
F + (Δ·t)/n (C integer division per axis) becomes kind 3; when the line climbs (z above the
previous step's z) the cell above also becomes 3 and the current cell gets flag 1; when it
descends, the cell below becomes 3 and gets flag 1.

### 1.7 Entrance, orientation and palette draws

1. The first link's first room gets an **entrance**. This happens after all corridors are
   carved. With that room's box P, S from §1.5 (after the fixed-room reset), the cell
   **(P.x, P.y − 1, P.z)** becomes kind 4, whatever was there before. That cell is one step
   outside the room's low-y wall, level with its floor layer, at the room's low-x corner. It is
   not the room centre.
   **Correction (checked at 0x5ce3f3..0x5ce4de):** an earlier draft said the entrance used the
   centre cell from §1.5 step 5. The code instead builds x = 7a + 2 + p0, y = 7b + 1 + p1,
   z = 7c + 3 + p2 from the link's first position (a, b, c) and that room's record. No size
   term goes in. The grid write goes through the view transform with rot = mir = 0.
2. draw: rot = d & 3 (provisional); draw: mir = even(d).
3. Shell palette: draw d1, d2, d3 → **c1** = (d3 % 50 + 20, d2 % 50 + 20, d1 % 50 + 20).
4. draw: e = d % 50; if v = 3: draw: e = d % 50 + 100.
5. **c2** = three draws (d % 50 + 50 + e each, third draw red); **c3** = three more, same rule.
6. Defaults: glow rate **g** = 0.005 (float 0x3ba3d70a), a float triple (0.6, 0.5, 0.1), a
   style flag 0. Then by v:
   * v = 1, 2: style flag 1, triple (0, 0.5, 0.1); c2 = draws d1, d2, d3 →
     (d3 % 20 + 50, d2 % 50 + 50, d1 % 50 + 50); c3 the same with three more draws;
     g = 0.0015;
   * v = 4: c2 = (d3 % 50 + 100, d2 % 50 + 80, d1 % 50 + 20); c3 likewise; g = 0.0025;
   * v = 5: c2 = (255, 210, 130), c3 = (200, 180, 80) (no draws).
7. **Arch model**: slot 2545 (`temple-arc.cub`) if v = 4 or 5, otherwise slot 2544
   (`castle-arc.cub`); none if the list is shorter. Used in §1.10.

Draws in steps 2–6: 2 + 3 + 1 + [v = 3] + 6 + 6·[v ∈ {1, 2, 4}].

### 1.8 Raised floors (v ∈ {0, 1, 2, 3})

For each logical column (i, j), i = 0..21 (outer), j = 0..21 (inner) (the loop bounds swap
with rot's low bit but the grid is square): w = 1; if v ≠ 2: draw; if d % 6 = 0: draw,
w = d % 3 + 2. Then from c = 21 down to 0, find the first cell whose kind is neither 4 nor 0;
the cells c + 1 … min(c + w, 21) above it become kind 2 (pillar). (Cells are read and
written through the view with the provisional rot and mir.)

**Correction / confirmation (checked at 0x5ce946..0x5ce9da):** the rule above is exact.
The scan skips kind 4 and kind 0 cells and stops at the first cell of any other kind
(including the out-of-range scratch kind 1, which cannot occur for c = 0..21). If no such
cell exists, the column gets no writes. The write loop stores kind 2 into every cell from
c + 1 to min(c + w, 21) without testing the old kind, so an entrance in that range **is**
overwritten. That is rare for the real entrance cell of §1.7 step 1. The cell lies outside
the room box, and in every link table of §1.4 the room directly below the first link's
first room (same a, b, layer c − 1) is not present. So nothing sits under the entrance
except corridor cells that happen to cross its column. The entrance is overwritten only when
the first non-0, non-4 cell f in its column satisfies f < P.z ≤ f + w. Keep the overwrite,
since the original does it; §1.9 then falls back as described there.

Draws: 0 for v = 2; otherwise 484 plus one per column whose first draw is a multiple of 6.

### 1.9 Fitting the dungeon to the ground (no draws)

For k = 0..3: rot ← (rot + 1) & 3; find the entrance cell in logical coordinates (scan
c = 21 down to 0, a = 0..21, b = 0..21; the last kind-4 cell found wins); with
(X, Y) = (x0 + 10a, y0 + 10b): if that column is missing, **the builder stops** (nothing
more is written; the records made so far stay). g0 = top(X, Y), lowered while the block at
g0 is air, plus 1 (the height just above the ground); **wet** = some block among
(X + i, Y + j, g0 − 5 + k), i, j, k = 0..9, has type 2 or 3 (water or lake bed).
score = g0 + 100·wet − 10c. The first k with the smallest score wins (strict comparison);
its rot is kept and **base** = its score. Then: base += 50 if v ∈ {4, 5}; base −= 100 if the
winner was wet; if base + 10c < 0: base = −10c. Dungeon cell (a, b, c) occupies blocks
(x0 + 10a … + 9, y0 + 10b … + 9, base + 10c … + 9).

The mirror flag keeps the value of §1.7 step 2.

**Correction (checked at 0x5cea5f..0x5cee13):**

* **No entrance found.** At the start of every k, the found (a, b, c) is reset to
  (0, 0, 0) (0x5ceaad..0x5ceabd). If the scan finds no kind-4 cell, that (0, 0, 0) is used
  as is. The builder does not stop or skip that k. It scores the corner column (x0 + 5,
  y0 + 5) with c = 0 like any other.
* **Scan bounds.** a runs to the view-dependent x size and b to the view-dependent y size
  (0x5f9e10 / 0x5f9e30; both 22 here). c is the outer loop, from 21 down. A later hit
  overwrites an earlier one, so the winner is the kind-4 cell with the lowest c, then the
  highest a, then the highest b.
* **Sample column.** The height is read at the cell's centre column (X + 5, Y + 5) with
  (X, Y) = (x0 + 10a, y0 + 10b). The builder stops only if that column is missing (0x5cebce).
  The lowering loop tests the blocks of that column. The wet test still covers
  (X + i, Y + j, g0 − 5 + k), i, j, k = 0..9, starting at the cell corner.
* **Choosing the winner.** A k replaces the best when no best exists yet (the stored best
  rot is −1) or when its score is strictly smaller. The winner's wet flag and c are kept for
  the base adjustments. The grid's rot is then set to the best rot, or 0 if it is negative.
* **No missed writers.** Between the grid's construction (0x5cdfd5) and this pass, the
  only grid writes are these: the room builder (§1.5 step 4), the mark (§1.5 step 5), the
  corridors (§1.6), the entrance (§1.7 step 1) and the raised floors (§1.8). Every other cell
  access in that range is a read, and the palette steps make no cell accesses at all.

### 1.10 Building pass

Everything after the fit (§1.9) up to the end of the builder. Read from the assembly of
0x5ceeb3..0x5d5f3a; the few places taken from the decompiler output only are marked
*inferred*. Notation for this section:

* **cell** (a, b, c) is always a *logical* cell, read through the view of §1.5 with the final
  rot and mir; **kind(a, b, c)** is its kind byte, **flags(a, b, c)** its flag byte (bit 1
  stair, bit 2 floor patch, bit 4 mark). Logical cells outside 0..21 read kind 1.
* X = x0 + 10a, Y = y0 + 10b, Z = base + 10c (the cell's lowest block corner).
* **closed(k)** means k ∈ {0, 1, 4} (used by the roof and wall tests of §1.10.2);
  **solid(k)** means k ∈ {0, 1, 2} (used by the decoration tests, 0x5eea40).
* **block solid** means the block read with the implicit rules has a type other than 0 and 2
  (0x43b480).
* lv = zone level, sl = sub-level (§1.1); g = glow rate, c1 … c5 the colours of §1.7 and
  §1.10.1; K1, K2 as below; **flag** = the builder's last argument (0 in fresh worlds).
* "→ fx" means the block coordinate times 65536 (64-bit); a float coordinate f becomes
  trunc64(f32(f · 65536)) (0x42c580), a double d becomes trunc64(d · 65536) (0x459c00).

**K1 and K2, exactly** (corrects the table of §1.2). K1 is a list of creature kinds; K2 is a
list of **groups**, each group a pair (leaders, escorts) of kind lists:

| v | K1 | K2 |
|---|---|---|
| 0, 4 (and any other) | 11, 12 | ({46}, {19}) |
| 1, 2 | 15, 16 | ({15, 16}, {96}), then ({k}, {}) with k drawn as in §1.2 |
| 3 | 2, 3 | ({2, 3}, {19}) |
| 5 | 78, 77 | ({17, 81}, {62, 30}) |

#### 1.10.1 Palette after the fit

Six draws give **c4** = (d3 % 180 + 50, d2 % 180 + 50, d1 % 180 + 50) and **c5** likewise;
if v ∈ {0, 3}: one more draw, value unused.

#### 1.10.2 Helpers used by the pass (none of them re-seeds)

1. **Box filler** 0x5df600: box(x, y, z, size (sx, sy, sz), colour C, rate g, flags f). It
   makes **3 + sx·sy·sz draws** when sx, sy, sz are all positive and **exactly 3** otherwise
   (the table draws happen first, the cell loops then run zero times). Table: a 23-entry
   table t with t[0] = d % 20, t[8] = d % 20, t[16] = d % 20 (three draws, in that order) and
   zeros elsewhere. Cells: i = 0..sx − 1 (outer), j = 0..sy − 1, k = sz − 1 down to 0
   (inner), one draw per cell **whether or not the column exists**. For the cell
   (x + i, y + j, z + k):
   * v = t[((7k + i)/2 + 7·((j + 7k)/2)) mod 23] (integer halves);
   * m = moss(x + i, y + j, z + k), with moss(X, Y, Z) = clamp to [0, 1] of
     f32(mo · f32(f32(f32(N(f32(0.2Y + 534), f32(0.2Z + 13)) · 0.1) + N(0.05X + 4343, 0.1Z + 84734)) · 0.7 + 0.2)),
     mo = the column's stored moisture (or the moisture blend if the column is missing);
     the clamp returns 0 when the product is negative and 1 when it exceeds 1;
   * colour = ((float)(C.R + v), m·(120 − (float)(C.G + v)) + (float)(C.G + v), (float)(C.B + v)),
     each channel clamped to [0, 255] and truncated;
   * the cell's draw r gives type byte f | 0x46 (rock, structure bit) if r/32767 > g
     (float division), else f | 0x4d (glowing type 13, structure bit);
   * the block is written (block writer, so missing columns ignore it).
2. **Carve** 0x59aba0: carve(x, y, z, size (sx, sy, sz), rough). For i = 0..sx − 1 (outer),
   j = 0..sy − 1, k = sz − 1 down to 0: write (0, 0, 0) with type byte **0xC0** (air, dry,
   keep) at (x + i, y + j, z + k). If rough, after each such write, for each face the cell
   touches, in the order i = 0 (cell x − 1), j = 0 (y − 1), i = sx − 1 (x + 1), j = sy − 1
   (y + 1): if N(0.1·Y′ + 13·X′, 0.2·Z′) > 0.5, with (X′, Y′, Z′) the carved cell, the
   neighbouring block outside the face is also written as (0, 0, 0, 0xC0). (The noise
   arguments of the first face were checked; the other three faces use the same constants
   and are taken to use the same formula, *inferred*.) No draws.
3. **Plain fill** 0x5cb550: fill(x, y, z, size, colour C): every cell (same loop order as the
   box filler) gets (C, type byte 0x46). No draws.
4. **Wall furniture** 0x5f6e20: furniture(P (fx position), side s, style t) returns a static
   object:
   * draw; if d % 50 = 0: kind **10** (`chest-base02`), scale (1.5, 1, 1), rotation
     (s + 2) & 3, and P moved half a block away from the wall: s = 0: y − 0.5, s = 1:
     x + 0.5, s = 2: y + 0.5, s = 3: x − 0.5 (±32768 fx). 1 draw.
   * otherwise draw k = d % 3:
     * k = 0, shelf: kind by t: t = 3 → 32 + d % 3 (`shelf1..3`), t = 4 → 38 + d % 3
       (`stone-shelf`), t = 5 → 41 + d % 3 (`sandstone-shelf`), any other t → 35 + d % 3
       (`castle-shelf`); scale (2, 1, 1.57); rotation s; no offset. 3 draws.
     * k = 1, table: kind 13 if t = 4, 14 if t = 5, else 12; scale (3, 3, 1); rotation s;
       draw: o = trunc64(f32(f32(d/32767 + 1) · 65536)) (one to two blocks); P moved by o
       away from the wall: s = 0: y − o, s = 1: x + o, s = 2: y + o, s = 3: x − o. 3 draws.
     * k = 2, stool: kind 15 if t = 4, 17 if t = 5, else 16; scale (1, 1, 0.5); rotation s;
       draw: o = trunc64(f32(f32(d/32767) · 65536)) (zero to one block), same directions.
       3 draws.
5. **Torch sprite** 0x5f8960: torch(style t, P, angle A) returns a plant-sprite record
   (vegetation spec §1.3; record layout of trees spec §5): scale fields (+0x2c … +0x34)
   (1, 1, 1), +0x38 = 2, +0x20 = 0.0625, +0x24 = A (degrees), position P, then by t:
   * t = 0: draw; even → kind **50** (`castle-chain`) with z raised by 2 blocks;
     odd → the lamp below;
   * t = 1, 2: draw k = d % 3: 0 → 50, 1 → 49 (`wall-skull`), 2 → 52 (`torch-green`) with
     +0x38 = 1 and tint (0, 0.5, 0.1);
   * t = 3: kind 48 (`crest1`), no draw;
   * t = 4: draw; odd → draw: kind 44 + (d & 3) (`inca-art1..4`); even → the lamp;
   * t = 5: the lamp, no draw;
   * the **lamp**: kind 54 (`torch-yellow`), +0x38 = 1, tint (0.8, 0.7, 0.2).
6. **Rarity** 0x5f8530: rarity(r, force): q = d % (r + 1); then draw, +1 if d % 100 = 0;
   draw, +1 if d % 1000 = 0; draw, +1 if d % 10000 = 0; if force: q = r + 1; result
   min(q, 4). Always 4 draws.
7. **Spawner inventory** 0x5c9f90(creature + 0xf6c, lv, sl): the inventory is reset to **10**
   item records (0x11c bytes) and each is filled by 0x5f8750(lv), in order:
   * draw; if odd: **coins**: draw r; F = f32(pow(2, f32(f32(f32(r · 2)/32767) + 1) ·
     f32(lv · 0.25)))) (pow in double, result rounded to float); draw d; item = category
     12, subtype 0, byte +0x0d = 10, count +0x10 = d % (2·trunc(F) + 2). 3 draws.
   * if even: count +0x10 = lv; draw; if d % 6 = 0: draw k = d % 5: 0 → (category 1,
     subtype 1); 1 → coins as above but subtype 4 and +0x0d = 12 (2 more draws);
     2 → coins with subtype 0 and +0x0d = 12 (2 more draws); 3 → (1, 7); 4 → (1, 5).
     If d % 6 ≠ 0: draw; if odd: category 18, subtype 0, +0x04 = d % 3 (one more draw),
     +0x0d = 0; if even: the record stays empty (category 0).
   So 2 to 5 draws per record; 20 to 50 for the inventory.
8. **Item generators** 0x5f7a60 (111 draw sites) and 0x5f6d50 build random equipment; they
   belong to an items specification and are **not** specified here (open question 2).

#### 1.10.3 Shell, walls and roofs (per column)

For each logical column (a, b), a = 0..21 (outer), b = 0..21 (inner):

1. **m** = the lowest column top (bottom + count) over (X + i, Y + j), i, j = 0..9 (no
   draws; the columns lie inside the zone).
2. found = false, ent = false. For c = 21 down to 0, with k = kind(a, b, c), skipping k = 0:
   1. If found is false: **Htop** = base + 10(c + 1), minus 10 when k = 4.
   2. If k = 4: ent = true, found = true, no box (any entrance cell, top or not).
   3. Otherwise, if found is false (this is the topmost cell) and base + 10c + 5 > m (the
      ground lies below the middle of the cell):
      * v = 2: box(X, Y, Z + 11, (10, 10, 3), c2, 0, 0);
      * v ∈ {0, 1, 3}: **parapets**, for the sides −x, +x, −y, +y in that order, each only
        if closed(kind) holds for the neighbour on that side at level c **and** at level
        c + 1. With Zr = Z + 10:
        * −x: box(X − 3, Y − 3, Zr, (2, 16, 3), c2, 0, 0); then for t = 0..7, q = Y − 2 + 2t:
          if the block (X − 3, q, Zr + 3) reads air or water, write (0, 0, 0) with type byte
          0x40 there; box(X − 3, q − 1, Zr + 3, (1, 1, 1), c2, 0, 0).
        * +x: box(X + 11, Y − 3, Zr, (2, 16, 3)); for t = 0..7, q = Y − 1 + 2t: the same test
          and air at (X + 12, q, Zr + 3); box(X + 12, q − 1, Zr + 3, (1, 1, 1)).
        * −y: box(X − 3, Y − 3, Zr, (16, 2, 3)); for t, q = X − 2 + 2t: test and air at
          (q, Y − 3, Zr + 3); box(q − 1, Y − 3, Zr + 3, (1, 1, 1)).
        * +y: box(X − 3, Y + 11, Zr, (16, 2, 3)); for t, q = X − 1 + 2t: test and air at
          (q, Y + 12, Zr + 3); box(q − 1, Y + 12, Zr + 3, (1, 1, 1)) (*inferred* from the
          +x side; the −x, +x and −y sides were read).
        All colour c2, rate 0, flags 0. Each parapet costs 3 + 96 + 8·4 = 131 draws.
      * v = 3 also (after the parapets): if closed(kind) holds for all four neighbours at
        level c: **stepped roof**: box(X − 2, Y − 2, Z + 10, (14, 14, 4), c1, 0, 0), then for
        t = 0..6: box(X − 2 + t, Y − 2 + t, Z + 14 + 2t, (14 − 2t, 14 − 2t, 2), c1, 0, 0).
        (The roof flag is the byte the builder sets for v = 3, *inferred* from the
        decompiler output.)
      * The builder then probes the centre column (X + 5, Y + 5) for its ground height; the
        value is not used (no effect, no draws).
   4. Unless step 2 applied: the **shell**: box(X − 2, Y − 2, Z − 2, (14, 14, 14), c2, g, 0) —
      **2,747 draws**. Every non-zero, non-entrance cell gets one (the outline's statement
      that the topmost cell is skipped was wrong).
   5. found = true.
3. After the c loop, if found or v ∈ {4, 5}, and the column (X, Y) exists:
   **Bmin** = the lowest column bottom over (X − 2 … X + 11, Y − 2 … Y + 11), starting from
   the bottom of (X, Y) and ignoring missing columns. Then by v:
   * v ∈ {0, 1, 2, 3} (only reached when found): **foundation**:
     box(X − 2, Y − 2, Bmin, (14, 14, Htop − Bmin), c2, g, f) with f = 0x80 if ent else 0.
   * v = 4: **pyramid step** (every column, found or not), §1.10.4.
   * v = 5: nothing here.

#### 1.10.4 The v = 4 pyramid (temple)

Let (ae, be, ce) be the entrance cell of §1.9 (logical, final rot), and for the column
(a, b): m = max(|a − 10|, |b − 10|), e = 10 − m, minus 1 more if a = b = 10;
**H** = base + 10·(ce − 5 + e), plus 2 if |a − 10| ≤ 1 and |b − 10| ≤ 1.

1. box(X − 2, Y − 2, Bmin, (14, 14, H − Bmin − 4), c2, g, 0).
2. If H − Bmin > 0:
   1. box(X − 1, Y − 1, H − 4, (12, 12, 3), c2, 0, 0); box(X, Y, H − 1, (10, 10, 3), c2, 0, 0).
   2. **Grass skin**: for i = −2..11 (outer), j = −2..11: z = H + 2, minus 6 if i or j is −2
      or 11, otherwise minus 3 if i or j is −1 or 10. If N(0.02·(X + i), 0.02·(Y + j)) > 0
      (double arguments) and the block (X + i, Y + j, z) reads air or water: write the
      terrain surface block (terrain spec §6.1, 0x5c56e0) for (X + i, Y + j, z), whose two
      climate arguments come from the blends 0x5c8350 and 0x5c8310 at that column (which is
      moisture and which warmth was not checked). No draws.
3. **Stairs** on the two centre lines (k = 0..9, each a box with colour c2 and rate g):
   * a = 10 and |b − 10| > 1, b < 10: for k: box(X − 2, Y − 1 + k, H, (14, 1, k + 2));
     then for k: box(X − 2, Y − 1 + k, H, (2, 1, k + 4)); then for k:
     box(X + 10, Y − 1 + k, H, (2, 1, k + 4)).
   * a = 10, b > 10 + 1: the same three runs with y = Y + 10 − k.
   * b = 10 and |a − 10| > 1, a < 10: box(X − 1 + k, Y − 2, H, (1, 14, k + 2)); then
     box(X − 1 + k, Y − 2, H, (1, 2, k + 4)); then box(X − 1 + k, Y + 10, H, (1, 2, k + 4)).
   * b = 10, a > 11: the same with x = X + 10 − k.

#### 1.10.5 The v = 5 mound

After the column loop of §1.10.3, if v = 5: for x = x0 … x0 + 219 (outer), y = y0 … y0 + 219
(inner), if the column exists: with its bottom **bot** and d = max(|y − (y0 + 105)|,
|x − (x0 + 105)|): box(x, y, bot, (1, 1, base + 10ce + 59 − d − bot), c2, g, 0). This is a
square pyramid of slope 1 (the outline's "cleared floor" was wrong). Each column costs
3 + max(0, base + 10ce + 59 − d − bot) draws. The builder then probes the centre column for
its ground height without using the value.

#### 1.10.6 Interior pass

For a = 0..21 (outer), b = 0..21, c = 0..21 (inner), only cells with k = kind(a, b, c) ∈
{3, 4}. In order:

1. **Exit marker**: if k = 4, append a 0x140-byte marker record of type 5 with position
   (X + 5, Y + 5, Z) (fx) to the zone list at zone + 0x48 (0x5f4ba0). No draws.
2. **Carve** the cell: carve(X, Y, Z, (10, 10, 10), rough = v ∈ {1, 2, 4}).
3. **Windows** (v ∈ {0, 1, 3}, k ≠ 4, stair flag clear): four sides in the order −y, +y,
   −x, +x. Each side: draw; if odd **and** the neighbour on that side (level c) has kind 0
   **and** the test block is not solid:
   carve(cx, cy, Z + 2, (2, 2, 4), not rough) and a static object of kind **4**
   (`castle-window`), scale (2, 0.2, 4), at the window position (fx), inserted with
   0x5a0e00:

   | Side | Test block | Carve corner (cx, cy) | Window position | Rotation |
   |---|---|---|---|---|
   | −y | (X + 5, Y − 3, Z + 2) | (X + 4, Y − 2) | (X + 5, Y − 1, Z + 2) | 0 |
   | +y | (X + 5, Y + 13, Z + 2) | (X + 4, Y + 10) | (X + 5, Y + 11, Z + 2) | 2 |
   | −x | (X − 3, Y + 5, Z + 2) | (X − 2, Y + 4) | (X − 1, Y + 5, Z + 2) | 3 |
   | +x | (X + 13, Y + 5, Z + 2) | (X + 10, Y + 4) | (X + 11, Y + 5, Z + 2) | 1 |

   1 draw per side.
4. **Stairs** (any v, stair flag set): with c3 and rate g, eleven boxes t = 0..10 for the
   first case that applies:
   * kind(a + 1, b, c) ≠ 3, kind(a − 1, b, c) = 3, kind(a + 1, b, c + 1) = 3:
     box(X − 1 + t, Y, Z − 1 + t, (11 − t, 10, 1));
   * kind(a − 1, b, c) ≠ 3, kind(a + 1, b, c) = 3, kind(a − 1, b, c + 1) = 3:
     box(X, Y, Z − 1 + t, (11 − t, 10, 1));
   * kind(a, b + 1, c) ≠ 3, kind(a, b − 1, c) = 3, kind(a, b + 1, c + 1) = 3:
     box(X, Y − 1 + t, Z − 1 + t, (10, 11 − t, 1));
   * kind(a, b − 1, c) ≠ 3, kind(a, b + 1, c) = 3, kind(a, b − 1, c + 1) = 3:
     box(X, Y, Z − 1 + t, (10, 11 − t, 1)).
   The four tests are made one after the other, and every case that holds builds its
   staircase (normally only one holds). A staircase costs 33 + 10·66 = **693 draws**.
5. If v = 4 and k = 4: go to the next cell.
6. If kind(a, b, c − 1) = 3 (open below): go to step 8.
7. **Floor and furnishing** (the cell stands on something):
   1. Floor: box(X, Y, Z, (10, 10, 1), c3, g, 0) (303 draws).
   2. Carpet (v ∈ {0, 3} and floor-patch flag): x1 = X, x2 = X + 10, y1 = Y, y2 = Y + 10;
      if the cell at a − 1 lacks the patch flag: x1 += 2 (and draw the −x border); if a + 1
      lacks it: x2 −= 2 (+x border); if b − 1 lacks it: y1 += 2 (−y border); if b + 1 lacks
      it: y2 −= 2 (+y border). fill(x1, y1, Z, (x2 − x1, y2 − y1, 1), c4); then the borders
      in the order −x, +x, −y, +y: fill(x1, y1, Z, (1, y2 − y1, 1), c5),
      fill(x2 − 1, y1, Z, (1, y2 − y1, 1), c5), fill(x1, y1, Z, (x2 − x1, 1, 1), c5),
      fill(x1, y2 − 1, Z, (x2 − x1, 1, 1), c5). No draws.
   3. If k = 4: go to step 9. If the stair flag is set: go to step 8.
   4. **Trap** (flag = 0 only): draw; if d % 40 = 0: static kind **7** (`spike-trap`) at
      (X + 5, Y + 5, Z + 1), scale (10, 10, 10), draw: rotation d & 3, draw: the field
      +0x34 of the static record = d % 4000; insert (0x5a0e00); go to step 8.
   5. n0 = the number of statics in the zone vector now.
   6. **Ceiling decorations**, only if solid(kind(a, b, c + 1)):
      * draw; if d % 3 = 0 and v ∈ {2, 4}: a **liana** sprite (kind 55, constructor
        defaults 0x58e9f0: scale fields 1, +0x38 = 2): draw: +0x24 = f32(f32(d/32767)·360);
        draw: s = f32(f32(d·0.04)/32767 + 0.08), +0x20 = s; z = f32((float)(Z + 10) − 20s);
        draw: y = Y + 2 + d % 5; draw: x = X + 2 + d % 5; position (x, y, z) → fx; append to
        the zone's sprite list (zone + 0x04, 0x4862e0).
      * if v ∈ {1, 2}: **cobwebs** on the four sides −x, +x, +y, −y. Each side: draw; if
        d % 3 = 0 and solid(kind of the neighbour at level c): sprite: +0x24 = 0, 180, 270,
        90 by side; draw: s = f32(f32(d·0.05)/32767 + 0.08), +0x20 = s; z = Z + 10 − 20s;
        draw: q = 2 + d % 5; position −x: (X + 10s, Y + q, z); +x: (X + 10 − 10s, Y + q, z);
        +y: (X + q, Y + 10 − 10s, z); −y: (X + q, Y + 10s, z) (float sums → fx); draw: kind
        57 + (d & 1) (`cobwebs`, `cobwebs2`); append to zone + 0x04.
   7. **Wall decorations**, four sides in the order −x, +x, −y, +y. Each side: draw; if
      d % 3 = 0 and solid(kind of the neighbour at level c):
      1. draw; if odd: **torch**: position as in the table (one draw d & 3 for the
         along-wall coordinate); if the wall block (one block beyond the torch towards the
         wall) is solid: torch(v, P, A) (§1.10.2 item 5, its own draws) appended to
         zone + 0x04.
      2. draw; if d & 7 ≠ 0: **wall furniture**: position as in the table (a fresh d & 3
         draw); furniture(P, s, t = byte zone + 0x79, i.e. v) inserted with 0x5a0e00.
         If d & 7 = 0: **guard creature**: record 0x5adb20 with +0x28 = 6, +0x2c = **143**,
         +0x34 = lv, +0xf58 = 10.0, +0x08 = 150.0, position as in the table (a fresh d & 3
         draw) at +0x10; inventory 0x5c9f90(lv, sl) (§1.10.2 item 7); append to zone + 0x18
         (0x66add0).

      | Side | Torch P, angle A | Furniture P, side s | Creature position |
      |---|---|---|---|
      | −x | (X + 0.5, Y + 3 + r, Z + 2), 270 | (X + 0.5, Y + 3 + r, Z + 1), s = 1 | (X + 1, Y + 3 + r, Z + 1) |
      | +x | (X + 9.5, Y + 3 + r, Z + 2), 90 | (X + 9.5, Y + 3 + r, Z + 1), s = 3 | (X + 9, Y + 3 + r, Z + 1) |
      | −y | (X + 3 + r, Y + 0.5, Z + 2), 0 | (X + 3 + r, Y + 0.5, Z + 1), s = 2 | (X + 3 + r, Y + 1, Z + 1) |
      | +y | (X + 3 + r, Y + 9.5, Z + 2), 180 | (X + 3 + r, Y + 9.5, Z + 1), s = 0 | (X + 3 + r, Y + 9, Z + 1) |

      (r = the d & 3 draw of that object. Torches hang one block higher than furniture
      and guards. The torch test block is the one at the torch position moved one block
      towards the wall, by floor division of the fx coordinates.)
   8. **Loot**, for every static added since n0, in vector order:
      * kind 10 (chest): draw: n = (d & 3) + 1; n items from 0x5f7a60(lv, sl + 1, −1)
        appended to the chest's content list (+0x48); then the chest's byte +0x30 = 2.
      * kinds 12, 13, 35, 36, 37 (tables and castle shelves): for u = 0, 1, … while
        u < scale x, and inside it w = 0, 1, … while w < scale y (float comparisons):
        draw; if d % 10 = 0: a ground item (0x44a970) with scale +0x134 = 0.06 and +0x138 = 1:
        draw: if d % 6 ≠ 0: draw k = d & 3: 0 → (category 11, subtype 26); 1 → (18, subtype =
        the style flag of §1.7 step 6), +0x04 = d % 3 (draw); 2 → (11, 9), +0x0d = d % 3 + 25
        (draw); 3 → (1, 7). If d % 6 = 0: draw k = d % 6: 0 → (1, 1); 1 → (1, 4); 2 → (1, 5);
        3 → coins as in §1.10.2 item 7 with lv (2 draws), +0x0d = 10; 4 → 0x5f7a60(lv, sl);
        5 → 0x5f6d50(lv, sl). Position = the static's position + fx of
        (u − sx·0.5 + 0.5, w − sy·0.5 + 0.5, sz) (floats, sx, sy, sz its scale); draw:
        rotation +0x130 = f32(d·360/32767) (degrees); append to the zone item list
        (zone + 0x30, 0x44d590).
8. **Arches and chandelier**, only if k ≠ 4 and kind(a, b, c + 1) ≠ 3:
   * if the arch model exists (§1.7 step 7) and the block (X, Y, Z + 10) is solid: for the
     sides −x, +x, −y, +y whose neighbour (level c) is not open (kind ≠ 3): place the arch
     with the prefab placer (§6.2) at (X, Y, Z), rotation 0, 2, 3, 1 respectively (the +y
     value 1 is *inferred*), type byte 0x46, **mode 0**, the zone, no fill-down, no crop, no
     output list. Mode 0 interprets no markers, so arches cost **no draws**.
   * v = 3: draw; if d % 10 = 0: **chandelier** sprite kind 56 at
     (X + 5.5, Y + 5.5, Z + 10 − 2.7) (z: double, then → fx), +0x20 = 0.1, +0x24 = 0,
     +0x38 = 1, tint = the float triple of §1.7 step 6; append to zone + 0x04.
9. **Boss** (mark flag set): creature record with position
   (X + 4.5, Y + 4.5, Z + 1) (floats → fx), +0x28 = 1, +0x7a |= 0x1000, +0x34 = max(lv, 1),
   byte +0x58 = sl; draw: kind = K1[d mod |K1|] (unsigned division); one item
   0x5f7a60(+0x34, rarity(sl, force) (4 draws), −1) appended to the inventory (+0xf6c);
   +0x7a |= 0x200; a marker record of type 6 (position, kind, creature index) to
   zone + 0x48; draw: n = d & 3 items (category 1, subtype 1, count field = level) appended;
   +0x08 = 150.0, +0x10e8 = 1; append to zone + 0x18.

#### 1.10.7 Central shafts (v = 4 and v = 5)

After the interior pass. Let C = logical cell (10, 10), Xc = x0 + 100, Yc = y0 + 100,
**Ze** = base + 10(ce + 1) (top of the entrance cell's level).

* v = 4: Zt = base + 10ce + 44. box(Xc − 2, Yc − 2, Zt, (14, 14, 16), c2, g, 0);
  carve(Xc, Yc − 2, Zt, (10, 14, 10)); carve(Xc − 2, Yc, Zt, (14, 10, 10));
  carve(Xc, Yc, Ze, (10, 10, Zt − Ze)).
* v = 5: Zt = base + 10ce + 38. box(Xc − 2, Yc − 2, Zt, (14, 14, 14), c2, g, 0);
  carve(Xc + 2, Yc − 32, Zt, (6, 74, 6)); carve(Xc − 32, Yc + 2, Zt, (74, 6, 6));
  carve(Xc, Yc, Ze, (10, 10, Zt − Ze + 6)) (corridor offsets read from the address
  arithmetic; *inferred* in detail).
* Both: **pillar** box(Xc + 4, Yc + 4, Ze − 30, (2, 2, n), c2, g, 0) with n = Zt − Ze + 30;
  then if n > 1, a **spiral**: for z = Ze − 29, Ze − 28, … while z − Ze + 30 < n, with
  step u = z − Ze + 30 (1, 2, …), one box(…, c2, g, 0) chosen by u & 7:

  | u & 7 | Corner | Size |
  |---|---|---|
  | 0 | (Xc, Yc, z) | (5, 5, 1) |
  | 1 | (Xc + 5, Yc, z) | (1, 5, 1) |
  | 2 | (Xc + 6, Yc, z) | (5, 5, 1) |
  | 3 | (Xc + 6, Yc + 5, z) | (5, 1, 1) |
  | 4 | (Xc + 6, Yc + 6, z) | (5, 5, 1) |
  | 5 | (Xc + 5, Yc + 6, z) | (1, 5, 1) |
  | 6 | (Xc, Yc + 6, z) | (5, 5, 1) |
  | 7 | (Xc, Yc + 5, z) | (5, 1, 1) |

  (v = 4 read; v = 5 uses its own jump table at 0x5d6018 with the same box sizes, positions
  taken to be the same, *inferred*.)

#### 1.10.8 Erosion (v ∈ {1, 2}, no draws)

For the grid columns a = 0..21 (outer), b = 0..21, then x = X … X + 9 (outer),
y = Y … Y + 9: e = f32(N(0.1x, 0.1y)·2) + f32(N(0.05x, 0.05y)·8) (float sum); if e > 0,
e = 0. With top = bottom + count of the column and h = trunc(f32((float)top + e)): for
z = top down to h: if the block at (x, y, z) has bit 0x40 set and bit 0x80 clear, write the
implicit-air block (255, 255, 255, type 0) there.

#### 1.10.9 Room spawns

For rooms (a, b, c) of §1.3, a outer, c inner, each **present and not quiet**, with P and S
of §1.5 (after the fixed override). Its corner cells Q0 = P, Q1 = (P.x + S.x − 1, P.y, P.z),
Q2 = (P.x + S.x − 1, P.y + S.y − 1, P.z), Q3 = (P.x, P.y + S.y − 1, P.z) are converted from
storage to logical coordinates (0x5fa440: if mir, B ← 21 − B; then rot 1: (A, B) →
(B, 21 − A); rot 2: (21 − A, 21 − B); rot 3: (21 − B, A)). A creature "at Q" stands at
(x0 + 10Q.x + 5, y0 + 10Q.y + 5, base + 10Q.z + 1). Every creature: record 0x5adb20,
+0x34 = lv, AI helper objects (no draws), appended to zone + 0x18. **kind rule**: if
flag ≠ 0: +0x28 = 3, draw: kind = d & 1; else +0x28 = 1, +0x7a |= 0x1000, draw: kind from
the list named, byte +0x58 = sl.

1. **Group** (S.x·S.y > 6 and K2 non-empty): draw: G = K2[d mod |K2|]; if G's leaders are
   empty, skip to 2. Draw: k = d & 3; the leader at Qk, kind rule with G.leaders; draw:
   route = d % 3 (patrol waypoints over the corners, no draws); +0x08 = 150.0; draw:
   n = d & 1 items (category 1, subtype 1, level) appended. Then, if flag = 0 and G.escorts
   is non-empty: draw: n = d % 3 + 1 escorts, each at the leader's position with
   +0x28 = 1, +0x7a |= 0x1000, draw: kind = G.escorts[d mod |G.escorts|], +0x58 = the
   leader's byte.
2. **Rings** (K1 non-empty and the room not marked): draw: G = (d & 1) + 1; draw: k = d & 3;
   if S.x·S.y > 7: G += 1. For g = 0..G − 1: draw: m = (d & 1) + 2; corner Q = Q_((k + g) & 3);
   for j = 0..m − 1: θ = f32(2j·π/m) (π the double 3.141592653589793, division in double);
   a creature at (x0 + 10Q.x + 5 + f32(2cos θ), y0 + 10Q.y + 5 + f32(2 sin θ), base +
   10Q.z + 1) (x and y as float sums → fx; cos/sin in double of the float θ, rounded to
   float), kind rule with K1, then draw: n = d & 1 items (1, 1) appended.
3. **Centre** (K1 non-empty, room not marked, S.x > 2 and S.y > 2): draw; if odd: the cell
   Qm = (Q0 + Q2)/2 (component-wise C division); if kind(Qm) = 3: draw: m = (d & 1) + 2; a
   ring of m creatures around Qm exactly as in step 2.

### 1.11 Draw summary

From the start of the builder to the end of §1.9:
1 (v ∈ {1, 2}) + 162 + link-table draws (§1.4: 3 for v = 2 or 4/5, 4 for v ∈ {0, 1, 3},
0 for flag = 1) + room draws (§1.5: one per present room that is neither marked nor fixed,
plus the room builder's 1, 3–5 and 0 or 2–4 draws) + 2 per overlapping link (§1.6) +
12 to 18 (§1.7) + §1.8.

Then, in order: 6 or 7 (§1.10.1); per column of §1.10.3 the boxes listed (2,747 per shell,
3 + 196·h per foundation, 131 per parapet, 303 for the v = 2 roof); §1.10.4 or §1.10.5; per
interior cell of §1.10.6 the window, stair, floor (303), trap, ceiling, wall and loot draws;
§1.10.7; §1.10.9. The only terrain-dependent counts are the box heights that use Bmin or
column bottoms (§1.10.3 step 3, §1.10.4 step 1, §1.10.5), the window and torch tests, and
the parapet air tests (which do not change the count). The only data-dependent counts are
the item generators 0x5f7a60 and 0x5f6d50 (open question 2); a dungeon whose rooms hold no
chest, no table loot with d % 6 = 0 and k ∈ {4, 5}, and no boss is fully specified.

### 1.12 Test vectors

* View transform, rot 1, mir 1: logical (3, 5, c) → storage (21 − 5, 21 − 3) = (16, 18).
  Inverse (0x5fa440), rot 1, mir 1: storage (16, 18) → mirror B: (16, 3) → (3, 21 − 16) =
  (3, 5).
* Room builder draw count: S = (3, 3, 2), pit = false, first draw even → 1 draw; first draw
  odd with w = 3, h = 3 → 3 draws; w = 1, h = 2 → 5 draws.
* Box filler: size (14, 14, 14) → 2,747 draws; size (10, 10, 3) → 303; (10, 10, 1) → 103;
  (14, 14, 0) or (14, 14, −5) → 3.
* Box filler table from the stream of srand(123) (440, 19053, 23075, …): t[0] = 0,
  t[8] = 13, t[16] = 15.
* moss(8397000, 8397100, 190) with mo = 0.5: first noise N(1679954, 51) =
  0.8264924883842468, second N(424193, 84753) = −0.035708021372556686, inner value
  0.23285886645317078, result **0.11642943322658539**.
* moss(8396950, 8396990, 40) with mo = 0.75: noises −0.7148255705833435 and
  0.05057518184185028, inner 0.18536484241485596, result **0.13902363181114197**.
* Staircase: 11 boxes of sizes 11·10 … 1·10 → 33 + 660 = 693 draws.
* Parapet: 3 + 2·16·3 + 8·(3 + 1) = 131 draws.
* Wall furniture with the srand(123) stream, side 1, style 0: 440 % 50 = 40 (no chest),
  19053 % 3 = 0 (shelf), 23075 % 3 = 2 → kind 37 (`castle-shelf3`), scale (2, 1, 1.57),
  rotation 1; 3 draws.
* Spawner item with the srand(123) stream and lv = 5: 440 even → count 5; 19053 % 6 = 3
  (≠ 0); 23075 odd → category 18; 13104 % 3 = 0 → +0x04 = 0. 4 draws.
* Coins with lv = 5 and r = 19053: exponent f32(f32(38106/32767) + 1)·1.25 = 2.703673,
  F = 6.514583, modulus 2·6 + 2 = 14; with d = 23075 the count is 3.
* Rarity(2, false) with the srand(123) stream: 440 % 3 = 2; 19053 % 100 = 53, 23075 % 1000 =
  75, 13104 % 10000 = 3104 → result 2 (4 draws).
* Table offset (k = 1 furniture) for d = 19053: trunc64(f32(1.5814691 · 65536)) = 103643
  (1.58 blocks).
* Ring positions, m = 2: offsets (2, 0) and (−2, −1.7484555e−07); m = 3: (2, 0),
  (−1.0000001, 1.7320508), (−0.9999998, −1.7320509) (float values of 2cos θ, 2 sin θ).
* v = 4 pyramid heights with base = 100, ce = 5: column (10, 10): e = 9, H = 190 + 2 = 192;
  (9, 10): m = 1, e = 9, H = 192; (8, 10): m = 2, H = 180; (0, 0): m = 10, H = 100.
* v = 5 mound with base = 100, ce = 5: the column at the centre (x0 + 105, y0 + 105) reaches
  z = 209 (top block 208); a column 40 blocks away along x tops out at 168.

---

## 2. Camps (0x5e8d86..0x5e8f8d and 0x5ad320)

### 2.1 The attempts (zone generator)

Only when (zx + zy) is odd. Up to 10 tries; for each try:

1. draw: py = 256zy + 48 + d % 160; draw: px = 256zx + 48 + d % 160 (**y first**).
2. Position in fx: P = (px·65536 + 32768, py·65536 + 32768, 0) (the double −32768.0 at
   0x702ad8 is truncated and subtracted).
3. If Lz has type 1 or 5: v = 1 − f(Lz, P) (0x5f8e10, the location distance of the regions
   spec §7 at the fx point); if v > 0 and v² > 0, the try **fails** (no camp call, no
   further draws in this try).
4. Otherwise, if the column (px, py) exists: P.z = top(px, py)·65536.
5. Call the camp routine (§2.2). If it returns true: append an **object-list node** holding
   P (built by 0x58d860; the list of caves spec §3.1) and **stop trying**. If it returns
   false, go to the next try.

Draws per try: 2, plus the camp routine's 0, 1 or 9 (§2.2).

### 2.2 The camp routine (0x5ad320)

Input: the zone and the fx position P (x, y, z as int64). Let (bx, by) = (P.x / 65536,
P.y / 65536) (64-bit division towards zero).

1. **Roughness test**: R = R(bx, by) (terrain spec §3.3); if 1 − 50·R ≥ 0, return **false**
   (0 draws). So camps need R > 0.02 (rough ground). (50.0 is the float at 0x6fcd7c; the
   comparison is float, 1 − 50R against 0.0.)
2. **Campfire**: a static object of kind **65** (`campfire.cub`), scale (2.4, 2.4, 0.5)
   (float bits 0x4019999a, 0x4019999a, 0x3f000000); draw: rotation index = d & 3.
3. For i = 0..2 (outer, x) and j = 0..2 (inner, y): place the fire at
   (P.x + 65536i, P.y + 65536j, P.z) and run the **settle test** (§2.3) with the
   floor check on. The first position that passes is kept: the fire is inserted into
   the zone's static-object vector (zone + 0x0c, via 0x5a0e00) at the settled height. If none
   of the nine passes, return **false** (1 draw in total).
4. **Ring**: for i ∈ {0, 7} (outer, x) and j ∈ {0, 7} (inner, y), at
   (P.x + 65536i − 229376, P.y + 65536j − 229376, P.z) (that is ±3.5 blocks around the
   **original** P, not around the shifted fire): draw: k = d & 3 selects

   | k | Kind | Model | Scale |
   |---|---|---|---|
   | 0 | 66 | `tent.cub` | (4, 4, 3) |
   | 1 | 16 | `stool.cub` | (1, 1, 0.5) |
   | 2 | 12 | `table.cub` | (3, 3, 1) |
   | 3 | 69 | `wood-mat.cub` | (2, 2, 0.1) |

   then draw: rotation index = d & 3. If the settle test (§2.3, floor check on) passes, the
   object is appended to the same static-object vector (0x188-byte records) at the settled
   height; otherwise it is dropped. The draws happen either way.
5. Return **true**.

Draws: 0 (step 1 fails), 1 (no fire position), or 9 (camp built).

Static-object record fields used here (0x188 bytes, constructor 0x58eaa0): +0x00 kind,
+0x08 x, +0x10 y, +0x18 z (int64 fx), +0x20 rotation index, +0x24/+0x28/+0x2c scale
(floats), +0x30 a byte set to 1; the rest keeps the constructor's defaults.

The object-list node of §2.1 is what later passes test distances against (boulder pass 3,
40 blocks; tree grid, 40 blocks; the formation passes of the caves spec run earlier and see
an empty list).

### 2.3 The settle test (0x5f4da0)

Input: a static-object record, the zone and a "floor check" flag (1 in every call here).

1. Footprint: (sx, sy) = scale x, y, swapped when the rotation index is odd;
   hx = trunc64(f32(f32(sx·0.5)·65536)), hy likewise with sy. Columns
   X0 = (x − hx)/65536 … X1 = (x + hx)/65536 and Y0 … Y1 likewise (64-bit, towards zero),
   all inclusive.
2. **Drop**: up to 50 times: with zc = z / 65536, if every footprint column reads air or
   water at zc (read rules of terrain §1.4: a missing column or a cell below the bottom is
   solid), lower z by 65536; stop at the first height where some footprint cell is solid.
3. **Rise**: up to 50 times: if some footprint cell at z / 65536 is solid (type neither 0
   nor 2), raise z by 65536; otherwise stop.
4. If z ≤ 0 (as a 64-bit fx value), fail.
5. Floor check: every footprint cell at (z − 65536)/65536 must be solid; otherwise fail.
6. Pass if the cell containing the position (floor division of x, y, z by 65536, 0x42f860)
   is **not water** (type ≠ 2).

The test writes the settled z back into the record. It makes no draws.

### 2.4 Test vectors

* Camp try position: zx = 32800, zy = 32801 (odd sum), draws 1000 then 2000:
  py = 8397056 + 48 + 1000 % 160 = 8397144; px = 8396800 + 48 + 2000 % 160 = 8396928;
  P = (8396928.5, 8397144.5) blocks.
* Ring offsets: −3.5 and +3.5 blocks (229376 / 65536 = 3.5).
* Footprint half-widths: tent (4, 4) → hx = 131072 (2 blocks); table (3, 3) → 98304;
  campfire (2.4, 2.4) → trunc64(f32(1.2·65536)) = 78643.

---

## 3. Boulder pass 3 (0x5e8f8d..0x5e9649)

Only if E.kind ∉ {1, 3, 4} (otherwise no draws).

1. draw: n = d % 10. **Climate bonus** exactly as caves spec §4.3 step 2 with the zone-centre
   climate (wc, mc): n ← n + 10 if (wc > 0.6 or wc < 0.3) and (mc > 0.7 or (mc < 0.4 and
   (mc > 0.2 or wc < 0.8))). If n ≤ 0, stop.
2. For each of the n boulders:
   1. draw: ax = (d & 7) + 3; draw: ay = (d & 7) + 3; draw: az = (d & 7) + 3 (3..10 each);
   2. draw: x = 2·(128zx + ax) + d % (4·(64 − ax)); draw: y = 2·(128zy + ay) +
      d % (4·(64 − ay)). (So x ∈ [256zx + 2ax, 256zx + 255 − 2ax].)
   3. **Home**: skip if f32(f32((float)y − homeY)² + f32((float)x − homeX)²) < 400 (within
      20 blocks; x and y are converted to float and the float home coordinates subtracted).
   4. **Objects**: for each object-list node (x_o, y_o fx): ey = f32(f32((y·65536) − y_o)
      · 2⁻¹⁶), ex likewise (the 64-bit differences are rounded to float first); skip if
      ey² + ex² < 1600 (within 40 blocks).
   5. **Town**: skip if q(x, y) > 0.25 (terrain spec §3.1).
   6. **Ground**: G = the height just above the highest stored block of column (x, y) that
      is neither air nor water (scan down from the top; G = the column bottom if every
      stored block is air or water).
   7. Skip if the block at (x, y, G − 1) has type 11, 8 or 7, or has bit 0x40 set.
   8. **Shape**: for X = x − ax … x + ax (outer), u = f32((float)(X − x)/(float)ax); for
      Y = y − ay … y + ay: v = f32((float)(Y − y)/(float)ay), s2 = f32(v² + u²),
      k = f32(N(0.05X, 0.05Y)·0.8) (double arguments, no offsets); for Z = G + 2az down to
      G − 2az: w = f32(f32((float)(Z − G)/(float)az) + k); if f32(w² + s2) ≤ 1 and the
      block at (X, Y, Z) has bit 0x40 clear: write (Rk(X, Y, Z), type byte **0x26**).

Draws: 1 + 5 per boulder; nothing inside the shape draws. Unlike caves §6.2 and §6.3 the
centre is on the ground surface G (not one block below) and bit 0x80 is not tested. A
boulder whose centre column is town ground is skipped, but its shape can still reach
neighbouring town ground.

Test vectors:

* k at (8397000, 8397100): N(419850, 419855) = −0.4843904376029968, k = −0.38751235604286194.
* x range for zx = 32800, ax = 3: 2·(128·32800 + 3) + d % 244 = 8396806 + (0..243).
* Draw count: n = 0 without the bonus → exactly 1 draw; with the bonus and n = 0 →
  1 + 5·10 = 51.

---

## 4. The type-10 site (0x5e967f..0x5e9e7d)

Runs after the town builder call (§5) and only if E.kind = 4 (a type-10 site, locations
spec §7).

1. (cx, cy) = (256zx + 128, 256zy + 128); T = top(cx, cy).
2. Append an object-list node at (cx·65536, cy·65536, T·65536).
3. draw: n = d % 3 + 6 (6..8 **pillars**).
4. For k = 0 … n − 1:
   1. a = f32((double)(2k)·π / (double)n) (π = the double 3.141592653589793 at 0x6fce08,
      computed in double, rounded to float); s = f32(sin(a)), c = f32(cos(a)) (CRT sin/cos
      in double of the float a).
   2. Centre (px, py) = (cx + trunc(f32(c·25)), cy + trunc(f32(s·25))); zp = top(px, py) + 4
      (T + 4 if that column is missing).
   3. draw: a1 = (d & 3) + 3; draw: a2 = (d & 3) + 3.
   4. For X = px − 2a1 … px + 2a1 (outer), Y = py − 2a2 … py + 2a2:
      ny = f32(N(0.05Y, 0.05Y)·0.3) (**both** noise arguments are 0.05Y, in double);
      for Z = zp + 20 down to zp − 20: w = f32((float)(Z − zp)/10),
      v = f32(f32((float)(Y − py)/(float)a2) + ny),
      u = f32(f32(N(0.05X, 0.05Z)·0.3) + f32((float)(X − px)/(float)a1)) (double noise
      arguments); if f32(f32(u² + v²) + w²) ≤ 1 and the block at (X, Y, Z) has bit 0x40
      clear: write (Rk(X, Y, Z), type **6**).
5. **Runestone**: a static object of kind **45** (`runestone.cub`) at
   (cx·65536 + 229376, cy·65536 + 229376, T·65536) (3.5 blocks off the centre in x and y).
   Settle without footprint: while the cell at its position (0x42f860, floor division) reads
   air or water, lower z by 65536; then while it reads solid, raise z by 65536 (unbounded).
   draw: rotation index = d & 3; scale (4, 4, 5); insert into the zone's static-object
   vector (zone + 0x0c, 0x5a0e00).

Draws: 1 + 2n + 1 = 14, 16 or 18.

So a type-10 site is a ring of six to eight rough rock pillars (half-width a1 = 3..6,
half-depth a2 = 3..6, half-height 10, centred 4 blocks above the local ground) at radius 25
around the zone centre, with a runestone near the middle: a standing-stone circle.

Test vectors (pillar offsets (trunc(25c), trunc(25s)) for k = 0 … n − 1):

* n = 6: (25, 0), (12, 21), (−12, 21), (−25, 0), (−12, −21), (12, −21);
* n = 7: (25, 0), (15, 19), (−5, 24), (−22, 10), (−22, −10), (−5, −24), (15, −19);
* n = 8: (25, 0), (17, 17), (0, 25), (−17, 17), (−25, 0), (−17, −17), (0, −25), (17, −17)
  (for k = 2 and 4 the float angle gives cos and sin of about −4.4e−8 and −8.7e−8, which
  truncate to 0).
* Inside test at (X, Y, Z) = (8397060, 8397095, 205) for a pillar at (8397058, 8397096),
  zp = 200, a1 = 4, a2 = 5: N(0.05Y, 0.05Y) = 0.2656570374965668, N(0.05X, 0.05Z) =
  −0.6990781426429749, u = 0.29027652740478516, v = −0.12030288577079773, w = 0.5,
  sum = 0.34873324632644653 (inside).

---

## 5. The town builder (0x5af170)

### 5.1 Call and inputs

Called by the zone generator for **every** zone whose sub-cell location Lz has type 1 (town)
or type 5 (ruins), before the type-10 statics: 0x5af170(zone, Lz) (0x5e9655..0x5e967f). So
all 64 zones of a town's or ruin's 2048-block sub-cell run it, not only the four town
quarters; zones far from the centre usually end early (§5.4). Inputs used:

* Lz.type (+0x18), Lz.variant (+0x1c: the region style 0..5 for a town, the climate class
  0, 2 or 3 for type 5), Lz.level (+0x24), Lz centre (+0x00, +0x08);
* the zone's quarter number Q = byte zone + 0x79 (E.variant: 1..4 in the four quarter zones
  of a town, 0 elsewhere; locations spec §8.2);
* w(X, Y) = **0x5fa4c0** = max(1 − f(Lz, X, Y), 0)² at an fx point (f as regions spec §7);
  this is q of terrain §3.1 evaluated at fx positions.

A list of creature kinds is filled first (no draws): 34, 30, 19, 26, 33 (the animals of
open plots, §5.5.10).

### 5.2 The plot survey

g = 5 plots per side for type 1, 4 for type 5; plot size s = 256 / g (51 or 64, integer).
Plot (i, j), i = 0..g − 1 outer (x), j = 0..g − 1 inner (y), has origin
(ox, oy) = (256zx + 256i/g, 256zy + 256j/g) (integer division) and record index
r = i + g·j. Each record has: **lo**, **hi** (lowest and highest column top), **inner**,
**kind**, **sub**, **face** and **wc** (centre weight).

For each plot, for a = 0..s − 1 (outer, x), b = 0..s − 1 (inner, y), column
(X, Y) = (ox + a, oy + b):

1. wv = w(X·65536, Y·65536); **busy** ← busy or wv > 0.1;
2. **bad** ← bad or top(X, Y) < 1 (top = bottom + count), and bad ← bad or the column stores
   any block of type 2 (water) or 3 (lake bed);
3. if 7 < a < s − 7, 7 < b < s − 7 and wv < 0.65 (float 0.6499999761581421): **road** ← road
   or the column stores a block of type 11;
4. h = bottom + count − 1 (z of the highest stored block): lo = min, hi = max over the plot;
5. if 7 < a < s − 8, 7 < b < s − 8 and inner ≠ 0: inner = h (inner starts at −10000, so it
   ends as h of the last such column unless some such h was 0).

Then, for the same plot, before the next plot's columns are surveyed:

6. draw: face = d & 3;
7. wc = w at the plot centre ((ox + s/2)·65536, (oy + s/2)·65536);
8. kind: if not bad and busy: if road: kind = 7; else draw; kind = 2 if
   d/32767 < wc + 0.25 (float), else 0; then if hi − lo > 16: kind = 0. Otherwise kind = 0.

Draws: g² (face) + one per plot that is busy, not bad and has no road.

Correction (0x5af60d..0x5af723, rechecked against the assembly): the face draw and the kind
draw sit at the end of the plot's own pass, so the stream order is face(plot 0),
[kind(plot 0)], face(plot 1), [kind(plot 1)], … in plot order (i outer, j inner); there is
no second loop. (The survey itself makes no draws, so this only matters if an
implementation draws all faces first.) The **hi − lo > 16** test belongs to the no-road
branch only: a road plot (kind 7) jumps past it and stays kind 7 however steep it is
(0x5af6bb writes 7 and jumps to 0x5af723). Constants confirmed in the binary: busy
0x6fcb7c = f32 0.1 (wv > 0.1, strict), road 0x71dc28 = f32 0.65 (wv < 0.65, strict),
0x6fd338 = 32767.0, 0x745d98 = 0.25; the kind test is f32(wc + 0.25) > f32(d)/32767
(strict, so equality gives kind 0). Bounds rechecked: road test a > 7, b > 7, a < s − 7,
b < s − 7; inner test a ≥ 8, b ≥ 8, a < s − 8, b < s − 8; bad = (bottom + count ≤ 0) or any
stored block with (type byte & 0x1f) ∈ {2, 3}; road = some stored block with
(type & 0x1f) = 11; h = bottom + count − 1; the plot's first column sets lo = hi = h.
Record index r = i + g·j (stride 0x1c per i, 0x1c·g per j).

### 5.3 Facing (no draws)

For i = 0..g − 1 (outer), j = 0..g − 1 (inner): up to three times: the neighbour in
direction face & 3 is 1: (i − 1, j), 2: (i, j + 1), 3: (i + 1, j), 0: (i, j − 1); if it lies
inside the grid and its kind is not 2, stop; otherwise face ← face + 1. (Houses turn their
front to a non-house plot, which is usually a street or garden.)

### 5.4 Gardens and special plots

1. **Gardens** (type 1 only): for each record in index order: if kind = 0 and hi − lo < 16:
   draw; if even(d) and wc > 0: kind = 6.
2. List **B** = indices of plots with kind **2 only**, in index order. If B is empty, the
   builder skips to its end (no further draws, nothing written). Otherwise B is sorted by
   wc **ascending** (MSVC insertion sort for ≤ 32 entries: stable, ties keep index order).
   "Pop" below takes the **last** entry (highest wc); "pick" draws d and removes entry
   d mod |B| (unsigned), keeping the order of the rest.

   Correction (0x5af980..0x5afa3a): the index loop fills **two** lists, kind-2 plots into
   B (EBP − 0x5d1c) and kind-7 plots into a second list (EBP − 0x5db4). The second list is
   never read again (it is only destroyed at 0x5bf386). The empty test (0x5afa38) and every
   pop and pick below use B alone, so street plots (kind 7) are never turned into markets,
   wells, inns, class houses, towers or kind 0, never count towards |B| (which changes every
   pick's d mod |B| and the |B| > 1 / |B| > 3 gates), and a zone whose busy plots are all
   kind 7 ends here with no further draws. Sort rechecked (0x5ae540 → insertion sort
   0x5ae140 for ≤ 32 entries): an element moves left only while the earlier wc is strictly
   greater, so ascending and stable. Pick rechecked: rand(), unsigned DIV by |B|, memmove of
   the tail; pop: the last entry.
3. **Type 1**, by Q:
   * Q = 1: if |B| > 1: pop → kind 9 (market).
   * Q = 3: four picks (each only if B is non-empty) → kinds 10, 11, 12, 13.
   * Q = 2: three picks (each only if B is non-empty) → kinds 14, 16, 15.
   * Q = 3 and |B| > 1: pop → kind 3 (tower).
   * Q = 0 or 3, and |B| > 1: pop → sub = 1.
   * Q = 1: up to four pops (while B is non-empty) → sub = 2, 3, 4, 5.
   * Q = 0: pick → kind 5 (well).
   * Then six times: if |B| > 3: pick → kind 0.

   Checked (0x5afa68..0x5aff3d): the order and gates above are right. Q is the byte at
   zone + 0x79, which the zone generator copies from the per-zone table entry's byte +1
   (0x5e6ba1: the 16-byte entry goes to zone + 0x78), i.e. the town-quarter variant 1..4 of
   locations spec §8.2 and 0 in every other zone. **Q = 4** has no branch of its own: a
   Q = 4 quarter runs only the six "pick → kind 0" steps. The Q = 1 market gate compares the
   count taken right after the sort (equal to |B| at that point); the Q = 0 well pick has no
   emptiness test (B is non-empty there).
4. **Type 5**:
   * If the location centre's zone is this zone (blk(L.x) >> 8 = zx and blk(L.y) >> 8 = zy)
     and the original |B| > 1: pop → kind 20.
   * Twice: if |B| > 3: pick → kind 0.
   * Lz.variant = 0: twice: if |B| > 3: pick → kind 18, then draw: face = d & 3.
   * Lz.variant = 2: draw; if odd and |B| > 3: pick → kind 17, draw: face = d & 3. Then
     draw; if odd and |B| > 3: pick → kind 19, draw: face = d & 3.

Plot kinds and what they become (§5.5):

| Kind | Made by | Building | Prefab slot (model key) |
|---|---|---|---|
| 0 | default | nothing | — |
| 2 | survey | modular house (§5.5.4) | style set, below |
| 3 | Q = 3 | tower | 2177 `frame-tower.cub` |
| 4 | never written (§8 item 4) | landing place (dead code) | 2183 `landingplace.cub` |
| 5 | Q = 0 | well | 2178 `building-stone-well.cub` |
| 6 | gardens | field with crops, scarecrow, fences | plant kinds 23, 25, 26, 29, 30, 21 |
| 7 | survey (plot holds road blocks) | nothing (only §5.5.10); Correction (0x5af9ce): not in list B | — |
| 8 | never written (§8 item 4) | stepped stone monument (dead code) | — |
| 9 | Q = 1 | market (stands, crates, barrels) | static kinds 21..26 |
| 10..16 | Q = 3, 2 | class and craft houses | 2303 `building-warrior`, 2304 `-ranger`, 2305 `-rogue`, 2306 `-mage`, 2307 `-smithy`, 2309 `-weavingmill` (15), 2308 `-carpentersshop` (16) |
| 17 | type 5, desert | sphinx | 2564 `sphinx01.cub` |
| 18, 20 | type 5 | antique building | 2124 + (d & 1): `antique-building1/2.cub` |
| 19 | type 5, desert | obelisk | 2565 `obelisk.cub` |

House module sets (slots of `<set>-base`, `-floor`, `-floor-stairs`, `-wall`,
`-wall-window`, `-wall-door`, `-wall-indoor`, `-wall-balcony`, `-wall-lamp`, `-roof1..3`,
`-arc`, and the entrance stairs): type 1 by Lz.variant: 0 `framework` (2189..2201),
1 `wood` (2287..2299), 2 `whitewood` (2215..2227), 3 `stone` (2202..2214), 4 `clay`
(2228..2240, stairs 2241 `clay-entrance-stairs`), 5 `whiteclay` (2243..2255, stairs 2242);
other sets use 2185 `entrance-stairs.cub`. Type 5 by Lz.variant: 2 `desertruin`
(2276..2286), 3 `jungleruin` (2265..2275), otherwise `antiqueruin` (2256..2264); the ruin
sets have fewer wall pieces and repeat `-wall` (or `-wall-door`) in the missing roles. The
full role table is §5.5.4 step 1.

### 5.5 Building pass

Read from 0x5b02c8..0x5bf2c7 (decompiler output as a guide; argument lists and constants
checked in the assembly where stated). Notation: plot (i, j) with origin (ox, oy) and size
s; the plot record (0x1c bytes, §5.2) holds **lo** (+0x00), **hi** (+0x04), **inner**
(+0x08), **kind** (+0x0c), **sub** (+0x10), **face** (+0x14), **wc** (+0x18);
**cx** = ox + s/2, **cy** = oy + s/2 (integer halves); top(x, y) = bottom + count of the
column. Statics are 0x188-byte records inserted with 0x5a0e00 into zone + 0x0c; "settled"
means the settle test of §2.3 with the floor check, the object being inserted only if it
passes. Sprites go to zone + 0x04 (0x4862e0), creatures to zone + 0x18 (0x66add0), ground
items to zone + 0x30 (0x44d590). The box filler is §1.10.2 item 1, the prefab placer §6.2.

**Plot loop.** i = 0..g − 1 (outer), j = 0..g − 1 (inner). Each plot runs only the block of
its kind; in the code the blocks follow each other in the order 8, 9, 6, 2, **open-plot
creatures** (§5.5.10, every plot whose kind is not 2), 3, 4, 10..16, 5, 17, 19, 20, 18. So a
plot of kind 8, 9 or 6 first builds its own block and then runs §5.5.10; a plot of kind 3,
5 or 10..20 runs §5.5.10 before its prefab; kinds 0 and 7 run only §5.5.10. The street and
tree pass (§5.5.8) and the NPC pass (§5.5.9) follow the plot loop.

Two more containers appear below: the zone's **house list** (zone + 0x88, the house grids of
§5.5.4 in build order) and the zone's creature list (zone + 0x18). "Lowest open z" of a
column means: start at the column's bottom (+0x10) and step up while the block there
(implicit read rules) is solid.

#### 5.5.1 Kind 8: stepped plinth with a statue (dead code)

No plot ever gets kind 8 (§8 item 4); the block is described for completeness.

1. T = top(cx, cy) + 1.
2. For n = 4..23: box(cx − 2 − 2(n − 4), cy − 2 − 2(n − 4), T − (n − 4), (n, n, 1),
   colour (220, 220, 220), rate 0, flags 0). (The corner moves two blocks per step while the
   size grows by one, as in the code.)
3. A static of kind **0** (`goddess2`) at (cx, cy, T) (fx), scale (3, 3, 12); its z is raised
   one block at a time while the block at its position (floor division) is solid; draw:
   rotation d & 3; insert (no settle test).

Draws: 20·3 + Σ n² (n = 4..23) + 1 = 60 + 4,310 + 1 = **4,371**.

#### 5.5.2 Kind 9: market

1. If the column (cx, cy) exists, the point (cx, cy, top) is appended (0x674ee0) to the
   builder's **market list**, which the residents' day schedules of §5.5.9 visit; no draws.
2. Twenty slots, in this order. Each slot: draw; if d % 5 = 0 the slot stays empty (1 draw).
   Otherwise one draw q = d % 3 for the position, then the object (stands 0x5bff10, goods
   0x5bfd70, with their own draws) at z = hi, settled and inserted:

   | Slots | Object | Positions (x, y) | Rotation |
   |---|---|---|---|
   | 1, 2, 3 | stand | (cx − 7, oy + 6 + q), (cx, oy + 6 + q), (cx + 7, oy + 6 + q) | 2 |
   | 4, 5, 6 | stand | (cx − 7, oy + s − 6 − q), (cx, …), (cx + 7, …) | 0 |
   | 7, 8, 9 | stand | (ox + s − 6 − q, cy − 7), (…, cy), (…, cy + 7) | 3 |
   | 10, 11, 12 | stand | (ox + 6 + q, cy − 7), (…, cy), (…, cy + 7) | 1 |
   | 13, 14 | goods | (cx − 3, oy + 6 + q), (cx + 3, oy + 6 + q) | 2 |
   | 15, 16 | goods | (cx − 3, oy + s − 6 − q), (cx + 3, …) | 0 |
   | 17, 18 | goods | (ox + s − 6 − q, cy − 3), (…, cy + 3) | 3 |
   | 19, 20 | goods | (ox + 6 + q, cy − 3), (…, cy + 3) | 1 |

3. **Stand** (0x5bff10): draw: kind 21 + d % 3 (`market-stand1..3`), scale (3.5, 2, 3).
4. **Goods** (0x5bfd70): draw k = d & 3: 0 → barrel 24, scale (2, 2, 2); 1 → crate 25,
   draw: f = f32(f32(f32(d·0.5)/32767 + 1)·1.5), scale (f, f, f); 2 → open crate 26, draw:
   e = f32(f32(d·0.5)/32767 + 1), scale (1.5e, 1.5e, 0.75e); 3 → sack 27, scale
   (1.5, 1.5, 1.4).

Draws per slot: 1 (empty), 3 (stand), 3 or 4 (goods).

#### 5.5.3 Kind 6: garden

1. draw: o = d & 1 (stripe orientation); draw: crop = d % 6, which fixes the plant kind P,
   the ground-item subtype I (0 = none), the sprite flag F (+0x38) and the sprite size S
   (+0x20):

   | crop | P | I | F | S |
   |---|---|---|---|---|
   | 0 | 23 `pumpkin-leaves` | 16 | 0 | 0.1 |
   | 1 | 25 `sunflower` | 0 | 4 | 0.2 |
   | 2 | 26 `bean-tendril` | 0 | 0 | 0.2 |
   | 3 | per column: draw; d % 10 = 0 → 21 `cornflower` (S = 0.05), else 29 `wheat` (S = 0.2) | 0 | 4 | see P |
   | 4 | 30 `corn` | 0 | 4 | 0.2 |
   | 5 | 24 `pineapple-leaves` | 17 | 0 | 0.15 |

   For crop 3 the cornflower/wheat draw is made at the start of **every** column of the plot
   (s² draws), before the column is examined.
2. For u = 0..s − 1 (outer, x = ox + u), w = 0..s − 1 (inner, y = oy + w): if the column has
   stored blocks and its top stored block has type 4 (grass):
   1. The top block is rewritten with type byte **5** (field), no draws: with
      zt = bottom + count − 1 and the stripe selector sel = (u >> 1) & 1 when o = 0,
      (w >> 1) & 1 when o = 1 (as a float 0 or 1), the colour is
      C = Cl(x, y, zt)·0.7 + (10, 10, 0)·sel per channel (Cl = the cliff colour of terrain
      spec §6.7 at the top block, 0x5c7140; f32 products and sums, the 0.7 being
      f32 0.699999988), clamped to [0, 255], bytes by truncation. The write goes through the
      column's set-block (0x44e700, index count − 1), which keeps bit 0x80 of the old type
      byte when it was set. So every other pair of rows across the stripe direction is 10
      brighter in red and green.
   2. If u = s/2 and w = s/2: a creature, +0x28 = 6, kind **140**, +0x34 = the level from
      0x59a9b0 at (x, y), position (x + 0.5, y + 0.5, top) (fx); draw: facing
      +0x54 = (d & 3)·90.0.
   3. Otherwise: draw; if d % 10 = 0: draw k = d & 3; if k ≠ 0 or I = 0: a sprite of kind P
      (constructor defaults 0x58e9f0, then +0x38 = F), +0x24 = 0 if P = 25, else (draw)
      f32(d·360/32767); position (x + 0.5, y + 0.5, top); +0x20 = S. If k = 0 and I ≠ 0: a
      ground item of category 11, subtype I, at (x + 0.5, y + 0.5, top); draw: rotation
      +0x130 = f32(d·360/32767); scale +0x134 = 0.1; +0x138 = 2.
3. **Fences**, two runs. Run 1: draw; if odd: for k = 0..s/2 − 1: draw; if d % 10 ≠ 0 and the
   column (ox + 2k + 1, oy) has stored blocks whose top stored block has a type other than
   0: draw: static kind 52 + (d & 3) (`fence01..04`) at (ox + 2k + 1, oy + 0.5, top),
   rotation 0, scale (2, 1, 1.5), inserted without a settle test. Run 2: the same with the
   columns (ox, oy + 2k + 1), position (ox, oy + 2k + 1.5, top) (the code adds the same
   (0, 0.5, 0) offset) and rotation 1.

Draws: 2 + [s² for crop 3] + per grass column 1 to 3 (the centre column: 1) + per fence
run 1 + (s/2)·(1 or 2) when active.

#### 5.5.4 Kind 2: modular houses

Read from the assembly 0x5b24cf..0x5ba2a0 (the decompiler output garbles the placer
argument lists and misses the jump-table cases). A house is a grid of module cells; each
cell becomes a few prefab placements (§6.2) of 13 × 13 × 7-block modules.

**Placement shorthand.** "Place M (r, T, mode, F)" calls the placer with model M, rotation r,
type byte T, that mode, the zone, fill-down F, crop (0, 0, 0, 0) unless stated and no output
list. "At the cell centre" means origin X0 = O.x + 7 + 13a − ⌊sx/2⌋, Y0 = O.y + 7 + 13b −
⌊sy/2⌋ with sx, sy the **unrotated** sizes of the model being placed (model + 0x44, + 0x48;
halves by integer division) and (a, b) the cell's view coordinates (step 2).

1. **Module set** (no draws). Fourteen model slots, by Lz.type and Lz.variant:

   | Set | stairs S | base B | floor F | floor-stairs FS | wall W | window WW | door WD | indoor WI | balcony WB | lamp WL | roof1..3 R1..R3 | arc A |
   |---|---|---|---|---|---|---|---|---|---|---|---|---|
   | type 1, variant 0 or > 5 `framework` | 2185 | 2189 | 2190 | 2191 | 2192 | 2193 | 2194 | 2195 | 2196 | 2197 | 2198..2200 | 2201 |
   | type 1, 1 `wood` | 2185 | 2287 | 2288 | 2289 | 2290 | 2291 | 2292 | 2293 | 2294 | 2295 | 2296..2298 | 2299 |
   | type 1, 2 `whitewood` | 2185 | 2215 | 2216 | 2217 | 2218 | 2219 | 2220 | 2221 | 2222 | 2223 | 2224..2226 | 2227 |
   | type 1, 3 `stone` | 2185 | 2202 | 2203 | 2204 | 2205 | 2206 | 2207 | 2208 | 2209 | 2210 | 2211..2213 | 2214 |
   | type 1, 4 `clay` | 2241 | 2228 | 2229 | 2230 | 2231 | 2232 | 2233 | 2234 | 2235 | 2236 | 2237..2239 | 2240 |
   | type 1, 5 `whiteclay` | 2242 | 2243 | 2244 | 2245 | 2246 | 2247 | 2248 | 2249 | 2250 | 2251 | 2252..2254 | 2255 |
   | type 5, variant 2 `desertruin` | 2276 | 2277 | 2278 | 2279 | 2280 | 2280 | 2281 | 2282 | 2281 | 2281 | 2283..2285 | 2286 |
   | type 5, variant 3 `jungleruin` | 2265 | 2266 | 2267 | 2268 | 2269 | 2269 | 2270 | 2271 | 2269 | 2269 | 2272..2274 | 2275 |
   | type 5, other `antiqueruin` | 2256 | 2257 | 2258 | 2259 | 2260 | 2260 | 2260 | 2260 | 2260 | 2260 | 2261..2263 | 2264 |

   Slot names (registry 0x4ee230): `<set>-base`, `-floor`, `-floor-stairs`, `-wall`,
   `-wall-window`, `-wall-door`, `-wall-indoor`, `-wall-balcony`, `-wall-lamp`, `-roof1..3`,
   `-arc`; 2185 `entrance-stairs`, 2241 `clay-entrance-stairs`, 2242
   `whiteclay-entrance-stairs`, 2256/2265/2276 `<ruin>-entrance-stairs`.

2. **The grid** (0x5aeb40; a 0x74-byte `cube::House`). 3 × 3 × 4 cells of 12 bytes:
   **role** (+0), **r** (+1, rotation), **entrance** flag (+2), **stairs** flag (+3),
   **height** (+4, int32), **room** (+8). Grid fields: **face** (+0x04), **mirror** (+0x08),
   **origin O** (+0x0c, three int32), **sub** (+0x60, = the plot's sub), and point lists
   **spawn** (+0x24), **entrance** (+0x30), **counter** (+0x3c), **seats** (+0x48),
   **beds** (+0x54). Everything starts at zero.
   Roles: 0 none, 1 storey, 2 ground floor, 3 roof, 5 arc.
   Every cell access goes through 0x5a5b40, which maps **view** coordinates (a, b, z) to
   **storage** (x, y, z): face 0: (a, b); 1: (2 − b, a); 2: (2 − a, 2 − b); 3: (b, 2 − a);
   then, if mirror ≠ 0, y ← 2 − y. An access outside 0..2 × 0..2 × 0..3 returns a shared
   dummy cell whose role reads 0. The layout (step 3) is written while face = mirror = 0, so
   it is in storage coordinates; all later steps loop over view coordinates, which turns and
   mirrors the layout with the plot.

3. **Layout** (0x5b2e04..0x5b381a). Cells are written as "(x, y, z) = role", followed by
   "room k" (+8 = k) or "stairs" (+3 = 1) when set; listed in write order.

   * **sub ∈ {2, 3, 4, 5}** (the shops of Q = 1), no draws: (0,0,0)=2, (0,1,0)=2,
     (1,1,0)=2, (1,0,0)=2, (0,0,1)=1, (0,1,1)=1 room 3, (1,1,1)=1, (1,0,1)=1, (0,0,2)=3,
     (0,1,2)=3, (1,1,2)=3, (1,0,2)=3. A 2 × 2 house, one storey above the ground floor.
   * **sub = 1** (the inn), 1 draw: (0,0,0)=2; (0,0,1)=1 room 2; (0,0,2)=1 stairs;
     (0,0,3)=3; (1,0,0)=2; (1,0,1)=1 room 2; (1,0,2)=1 room 1; (1,0,3)=3; (2,0,0)=2;
     (2,0,1)=1 room 2; (2,0,2)=1 room 1; (2,0,3)=3; (0,1,0)=2; (0,1,1)=1 room 2; (0,1,2)=3;
     **draw**: odd → (2,1,0)=2, (2,1,1)=1 room 2, (2,1,2)=3; then (1,1,0)=2; (1,1,1)=1
     room 3; (1,1,2)=3; (1,2,0)=2; (1,2,1)=1; (1,2,2)=3.
   * **sub = 0** (ordinary house): **draw** k = d % 3.
     * **k = 0** (two towers), 4 draws in all: (0,0,0)=2; (0,0,1)=1; (0,0,2)=1 room 1
       stairs; (0,0,3)=3; (2,0,0)=2; (2,0,1)=1; (2,0,2)=1 stairs; (2,0,3)=3; (0,1,0)=2;
       (0,1,1)=1; **draw**: even → (0,1,2)=1, (0,1,3)=3; odd → (0,1,2)=3. **Draw**: odd →
       (2,1,0)=2, (2,1,1)=1, (2,1,2)=3. **Draw**: even → (1,0,0)=2, (1,0,1)=1, (1,0,2)=3;
       odd → (1,0,1)=5 (an arc between the towers; (1,0,0) stays empty).
     * **k = 1** (L-shape), 3 draws: (0,0,0)=2; (0,0,1)=1; (0,0,2)=3; (1,0,0)=2; (1,0,1)=1;
       (1,0,2)=3; (0,1,0)=2; (0,1,1)=1 room 1; (0,1,2)=3; (1,1,0)=2; (1,1,1)=1; **draw**:
       odd → (1,1,2)=1 stairs, (1,1,3)=3; even → (1,1,2)=3. **Draw**: odd → (0,2,0)=2,
       (0,2,1)=1, (0,2,2)=3.
     * **k = 2** (bar with wings), 4 draws: **draw**: even → (0,0,0)=2; (0,0,1)=1;
       (0,0,2)=1 stairs; (0,0,3)=3; (1,0,0)=2; (1,0,1)=1; (1,0,2)=1; (1,0,3)=3; (2,0,0)=2;
       (2,0,1)=1 room 1; (2,0,2)=1; (2,0,3)=3 — odd → (0,0,0)=2; (0,0,1)=1; (0,0,2)=3;
       (1,0,0)=2; (1,0,1)=1 room 1; (1,0,2)=3; (2,0,0)=2; (2,0,1)=1; (2,0,2)=3. Then
       **draw**: odd → (0,1,0)=2, (0,1,1)=1, (0,1,2)=3. **Draw**: odd → (2,1,0)=2,
       (2,1,1)=1, (2,1,2)=3. Then (1,1,0)=2, (1,1,1)=1, (1,1,2)=3.

   Then face = the plot's face (§5.3, §5.4); **draw**: mirror = 1 if d & 1 = 0, else 0. The
   call 0x5aea70 that follows only clears a local vector (no wall roles are derived; the
   wall choice is made per side in step 10).

4. **Storey heights** (no draws). For a = 0..2 (outer), b = 0..2: a cell (a, b, 1) is
   **eligible** when its role is 1, the cell (a, b, 2) has stairs 0 and its room is not 3.
   For an eligible cell, height = the largest top (bottom + count) of the existing columns
   (ox + 7 + 13a + u, oy + 7 + 13b + v), u = 0..12 outer, v = 0..12 (start 0);
   **M** = the largest height (start 0).

5. **Entrance** (0 or 1 draw). The eligible cells with height = M, in (a, b) order, form a
   list; if it is not empty: **draw**; the cell (a, b, 1) of entry d mod n (unsigned) gets
   entrance = 1.

6. **Rotations.** For a, b, z (z = 0..3 inner), every cell including empty ones: role 5 →
   r = face & 1 (no draw); otherwise **draw**: r = d & 3. That is 36 draws, one fewer per arc.

7. **Origin** (no draws). h0 = 0 and "first" = true; for z = 3 down to 0 (outer), a, b: if
   the cell has entrance = 1, for the 15 × 15 columns (ox + 6 + 13a + u, oy + 6 + 13b + v),
   u = 0..14 outer: for each existing column, v = top − M; if first or v > h0: h0 = v, first
   = false. **O** = (ox + 7, oy + 7, M + h0 − 6). O is stored in the grid and the grid is
   appended to the zone's house list (zone + 0x88).

8. **Road under the house** (type 1, variants other than 4 and 5; no draws). Rectangle:
   x0 = O.x − 2, y0 = O.y − 2; x1 = max(O.x − 2, the largest O.x + 13a + 13 over the cells
   (a, b, 0) with role ≠ 0) + 2; y1 likewise with b.
   *Test pass* (x = x0..x1 outer, y = y0..y1): for each existing column, z = its top stored
   block; while the block at z is not solid, z ← z − 1. If that block's type is not 11
   (road), or z < O.z, the step ends without writing.
   *Write pass* (same order, same z search): if the found block's type is 11: C = its colour
   as floats; on the border (x ∈ {x0, x1} or y ∈ {y0, y1}) C = (140 + 40n)·0.7 in every
   channel with n = N(0.05x + 843, 0.05y + 984) (f32: 40·n, + 140, × 0.7); then bytes
   trunc(C) with type 11 are written at z + 1 … O.z + 5 with the block writer 0x44e7a0.
   (So paving happens only when the whole rectangle already is road at or above O.z.)

9. **House mode** Mh = sub + 1 (jump table 0x5bf404: sub 1 → 2, 2 → 3, 3 → 4, 4 → 5,
   5 → 6, otherwise 1). It is passed only to doors and entrance stairs (step 10), which is
   how the sign markers of §6.2 (modes 2..6: inn, shop, weapon shop, armour shop,
   identifier) reach the inn and the four shops.

10. **Bases, arcs and walls** (placer calls 0x5b4bd7 … 0x5b5f2d). For a, b, z (z inner), by
    role:
    * **5**: place A (r = cell r, T 6, mode 1, F 0) at the cell centre, Z0 = O.z + 7z.
    * **2**: place B (cell r, T 6, mode 1, **F 1**) at the cell centre, Z0 = O.z + 7z.
    * **1**: the four sides in the order **−y (rotation 0), +y (2), −x (1), +x (3)**. N = the
      neighbour cell on that side (same z), N′ = the cell below N (z − 1):
      * N role 0 (open side):
        * if this cell has entrance = 1, or N′'s role is neither 0 nor 3: place WD
          (side rotation, T 6, mode **Mh**, F 0) at the cell centre, Z0 = O.z + 7z. Then,
          only if entrance = 1: place S (side rotation, T 6, mode Mh, **F 1**) at
          Z0 = O.z + 7z − 4, centred 13 blocks out: −y: Y0 = O.y − 6 + 13b − ⌊sy/2⌋;
          +y: Y0 = O.y + 20 + 13b − ⌊sy/2⌋; −x: X0 = O.x − 6 + 13a − ⌊sx/2⌋;
          +x: X0 = O.x + 20 + 13a − ⌊sx/2⌋ (the other coordinate as at the cell centre).
        * otherwise (N′ empty or roof): z = 1: **draw**; d % 6 = 0 → WL, else WW. z ≥ 2:
          **draw**; d % 6 = 0 → WB, else WW. z ≤ 0: WW without a draw. Placed with the side
          rotation, T 6, mode 1, F 0, at the cell centre, Z0 = O.z + 7z.
      * N role 1: nothing (open interior).
      * N any other role: place W (side rotation, T 6, mode 1, F 0) at the cell centre,
        Z0 = O.z + 7z.
    * roles 0 and 3: nothing here.

11. **Roofs** (0x5b6748, 0x5b67f0). For a, b, z: role 3: crop (e0, e1, e2, e3) = 1 for each
    of the neighbours −x, −y, +x, +y (same z) whose role is not 0, else 0. Two halves: the
    first with rotation r (the cell's), the second with (r + 2) & 3. Each half uses R1 unless
    its "front" neighbour is a roof: first half r = 0 → −x, 1 → +y, 2 → +x, 3 → −y; second
    half the opposite side. If that neighbour has role 3, the half uses **R3** when
    (neighbour r) & 1 = r & 1 (the ridge continues) and **R2** otherwise. Both halves:
    T **14**, mode 1, F 0, the crop above, at the cell centre (with the half's own model
    sizes), Z0 = O.z + 7z.

12. **Floors and carpets** (0x5b6c11 … 0x5b6d09). For a, b, z:
    * role 3 with role 1 directly below (z − 1): place F (cell r, T 6, mode 1, F 0) at the
      cell centre, Z0 = O.z + 7z + 1 − sz (sz = model + 0x4c), i.e. the slab's top at O.z + 7z.
    * role 1 with stairs = 1: place FS the same way.
    * role 1, stairs = 0: if type 1: **draw**; if d is even and the cell above (z + 1) has
      stairs = 0: **draw**: carpet slot 2186 + d % 3 (`carpet1..3`); place it with T 6,
      mode 1, F 0 and rotation = **draw** d & 3 (drawn after the model is chosen, before the
      placement), at the cell centre with Z0 = O.z + 7z + 1 − sz(carpet). Then, for both
      types, place F as above.

13. **Indoor walls** (0x5b6ef2 … 0x5b71bd). For a, b, z: role 1 and room 1: for the
    neighbours **−y (0), −x (1), +y (2), +x (3)** in that order: neighbour role 1 → place WI
    (that rotation, T 6, mode 1, F 0) at the cell centre, Z0 = O.z + 7z.

14. **Room clearing and points** (no draws). For a, b, z: role 1: for u = 0..13 (outer),
    v = 0..13, w = 5 down to 0: the block at (O.x + 13a + u, O.y + 13b + v, O.z + 7z + w) is
    read; if its type byte lacks 0x40 it is rewritten (block writer) with its own colour and
    type byte **0x40** (protected air). Then, if the cell below (z − 1) has role 2, the point
    Pc = (O.x + 7 + 13a, O.y + 7 + 13b, O.z + 7z + 1) is appended to the house's entrance
    list if entrance = 1, to the counter list if room = 3, and always to the spawn list.

15. **Lanterns and furniture** (type 1 only; 0x5b75ca … 0x5b9746). For a, b, z:
    1. Role 1 and (a + b) odd: a sprite of kind **13** (`lantern02`) at
       (O.x + 7 + 13a, O.y + 7 + 13b, O.z + 7z + 5.3) (fx; the 5.3 added as trunc(5.3·65536)),
       +0x20 = 0.075, +0x24 = 0, +0x38 = 1, tint +0x2c = (0.6, 0.5, 0.4), added to zone + 0x04
       (0x4862e0). No draws.
    2. Role 1 and the cell above (z + 1) has stairs = 0: furniture with floor level
       Zf = O.z + 7z + 1. "Supported" below means: inserted into zone + 0x0c (0x5a0e00) only
       if the block at (x, y, Zf − 0.5) (fx, floor division) is solid; otherwise discarded.
       Statics are built with 0x58eaa0; rotation +0x20, scale +0x24..+0x2c.
       1. **Wall pieces** (helper **W** below, each result supported), for each side whose
          neighbour (same z) is not a storey (role ≠ 1), in this order:
          * −x: **draw**, d % 3 ≠ 0 → W(O.x + 13a + 1.5, O.y + 13b + 4.5, rotation 1);
            **draw**, d % 3 ≠ 0 → W(O.x + 13a + 1.5, O.y + 13b + 9.5, 1);
          * +x: the same with x = O.x + 13a + 12.5 and rotation 3;
          * −y: W(O.x + 13a + 4.5, O.y + 13b + 1.5, rotation 2) with **no** gate draw;
          * +y: **draw**-gated W(O.x + 13a + 4.5, O.y + 13b + 12.5, 0), then **draw**-gated
            W(O.x + 13a + 9.5, O.y + 13b + 12.5, 0).
          (All positions fx with z = Zf; "+ 4.5" etc. are added as doubles × 65536.)
       2. **By room** (switch on +8):
          * **1, bedroom**: four beds, kind **19** (`bed`), scale (2, 3, 1), each with a
            **draw** for its rotation d & 3, at (O.x + 13a + 4, O.y + 13b + 4),
            (+10, +4), (+4, +10), (+10, +10); each supported.
          * **2, dining room**: table, kind **12** (`table`), scale (3, 3, 1), at the centre
            (O.x + 7 + 13a, O.y + 7 + 13b), **draw**: rotation d & 3; then four stools,
            kind **16** (`stool`), scale (1, 1, 0.5): (O.x + 13a + 4.5, centre y) rotation 1,
            (O.x + 13a + 9.5, centre y) 3, (centre x, O.y + 13b + 4.5) 2,
            (centre x, O.y + 13b + 9.5) 0. All supported. 1 draw.
          * **3, counter room**: four counters, kind **31** (`counter`), scale (4, 1, 1):
            (O.x + 13a + 3.5, centre y) rotation 3, (O.x + 13a + 10.5, centre y) 1,
            (centre x, O.y + 13b + 3.5) 0, (centre x, O.y + 13b + 10.5) 2; supported; no
            draws.
          * **other**: **draw**; d % 5 = 0 → a table (kind 12, (3, 3, 1)) at the centre with
            a **draw** for its rotation d & 3, supported.

    **Helper W** (0x5bf7c0)(P, r, room): returns a static at P with rotation r; draws in
    order:
    * **d1**: d1 % 50 = 0 → kind **10** (`chest-base02`), scale (1.2, 0.8, 0.8), P moved
      half a block into the room: r 0: y − 0.5; r 1: x + 0.5; r 2: y + 0.5; r 3: x − 0.5
      (±32768 fx). 1 draw.
    * else if room = 1: **draw**: odd → kind **20** (`bedtable`), (1, 1, 1); even → **draw**:
      kind **32 + d % 3** (`shelf1..3`), scale (2, 1, 1.57). 2 or 3 draws.
    * else **draw** d % 6:
      0: **draw**: shelf 32 + d % 3, (2, 1, 1.57);
      1: kind **18** (`bench`), (3, 1, 0.4);
      2: kind **16** (`stool`), (1, 1, 0.5); **draw**: o = trunc(f32(f32(d)/32767)·65536);
         r 0: y − o; r 1: x + o; r 2: y + o; r 3: x − o;
      3: kind **30** (`desktop`), (3, 1, 1); shift 13107 fx (trunc of 13107.2) with the
         signs of case 2;
      4: kind **29** (`cupboard`), (2.5, 1, 3); shift 19660 fx, same signs;
      5: **draw**: kind **56 + d % 9** (`flower-vase1..9`), (1, 1, 1).
      2 or 3 draws.
    (Static kind names: the kind → model map in 0x4ee230, e.g. 10 → 2490 `chest-base02`,
    18 → 2498 `bench`, 28 → 2509 `shelter`, 29 → 2501 `cupboard`, 30 → 2510 `desktop`,
    31 → 2511 `counter`, 56..64 → 2322..2330.) The chest gets no items (its item list is
    empty).

16. **Seats and beds** (no draws). Every static appended to zone + 0x0c since the house began:
    kind 16 or 18 → (zx, zy, static index) to the seats list; kind 19 → to the beds list.

17. **Outdoor goods** (type 1 only; 0x5b9845 … 0x5ba274). For a, b: if (a, b, 0) has role 2,
    the sides −x, +x, −y, +y in that order; for a side whose neighbour (a ± 1 or b ± 1, z 0)
    has role 0, two slots, each: **draw**; d % 6 = 0 → helper **G** at the slot, settled
    (§2.3 with the floor check) and inserted. Slots (z = O.z + 1, fx):
    −x: (O.x + 13a − 1.5, O.y + 13b + 4), then (…, O.y + 13b + 10), rotation 3;
    +x: (O.x + 13a + 15.5, same y values), rotation 1;
    −y: (O.x + 13a + 4, O.y + 13b − 1.5), then (O.x + 13a + 10, …), rotation 0;
    +y: (same x values, O.y + 13b + 15.5), rotation 2.

    **Helper G** (0x5bf5b0)(P, r): **draw** d % 7: 0 → kind 24 (`barrel`), (2, 2, 2);
    1 → kind 25 (`crate`), **draw**: f = f32(f32(f32(d·0.5)/32767 + 1)·1.5), (f, f, f);
    2 → kind 26 (`open-crate`), **draw**: e = f32(f32(d·0.5)/32767 + 1),
    (1.5e, 1.5e, 0.75e); 3 → kind 27 (`sack`), (1.5, 1.5, 1.4); 4 → kind 18 (`bench`),
    (3, 1, 0.4); 5 → kind 16 (`stool`), (1, 1, 0.5); 6 → kind 28 (`shelter`), (3, 3, 2.5).
    Rotation r. 1 or 2 draws.

**Draw order of one house**: layout (§3: 0, 1, 3 or 4) → mirror (1) → entrance (0 or 1) →
rotations (36 − arcs) → step 10 (per open storey side at z ≥ 1 without a door: 1; plus the
placer's marker draws of every module, §6.2, in placement order) → roofs (markers) → floors
(type 1: 1 per stair-less storey, + 2 for a carpet, + markers) → indoor walls (markers) →
furniture (type 1) → outdoor goods (type 1). Everything else is deterministic.

#### 5.5.5 Special buildings (kinds 3, 5, 10..20)

One prefab each, type byte 6, **fill-down on**, no crop, no output list, the zone, rotation
= face:

| Kind | Model slot | Position (x, y, z) | Mode |
|---|---|---|---|
| 3 tower | 2177 `frame-tower` | (cx − 16, cy − 16, hi − 7) | 7 |
| 5 well | 2178 `building-stone-well` | (cx − sx/2, cy − sy/2, hi), z lowered by one while the block there is not solid | 8 |
| 10 warrior | 2303 | (cx − 16, cy − 16, Hm − 7) | 9 |
| 11 ranger | 2304 | as kind 10 | 10 |
| 12 rogue | 2305 | as kind 10 | 11 |
| 13 mage | 2306 | as kind 10 | 12 |
| 14 smithy | 2307 | as kind 10 | 13 |
| 15 weaving mill | 2309 | as kind 10 | 15 |
| 16 carpenter | 2308 | as kind 10 | 14 |
| 17 sphinx | 2564 | (cx − sx′/2, cy − sy′/2, hi), sx′, sy′ the model sizes swapped when face is odd | 0 |
| 19 obelisk | 2565 | as kind 17 | 0 |
| 18, 20 antique building | 2124 + (d & 1), one draw first | as kind 17 | 0 |

(sx, sy = the model's x and y sizes.) **Class and craft houses** (0x5bac15..0x5bae58): the
model is 2303 by default and, by the case index kind − 11 (jump table 0x5bf440, cases
0..5), 2304, 2305, 2306, 2307, 2309, 2308; the mode is 9 by default and 10, 11, 12, 13, 15,
14 for the same cases (confirmed in the assembly). **Hm** = the largest top (bottom +
count, start 0) of the existing columns (cx − 16 + u, cy − 16 + v) for u = 0..sx − 1
(outer), v = 0..sy − 1, with the **unrotated** model sizes. No draws besides the placer's.

Kinds 17 and 19 then add a ground item from 0x5f77b0 (one draw, value d % 200) to zone +
0x30. Mode 0 interprets no markers and mode 8 only turns blue voxels into water, so kinds 5
and 17..20 cost 0 draws besides the model draw of 18/20 and the item draw of 17/19; modes 7
and 9..15 make the marker draws of §6.2 (NPC markers, craft stations).

**Kind 4** (landing place, dead code: no plot ever gets kind 4, §8 item 4): slot 2183 at
(cx − 16, cy − 16, hi − 5), mode 7, fill-down; then, if any of the eight neighbouring
regions (zx >> 6 ± 1, zy >> 6 ± 1) has +0x18 > 0, an airship object (0x466510) and a
creature (+0x28 = 3, +0x30 = 0x88, kind 4, level Lz.level) are created. Not specified
further.

#### 5.5.6 Rotation and mirroring

Buildings turn by quarter turns only, through the placer's rotation (§6.2): the plot's
face (§5.3) for single prefabs, per-cell and per-side rotations for house modules. The
house grid's face and mirror flag (§5.5.4 steps 2 and 3, one draw for the mirror) act on
every cell access, so they turn and mirror the house layout; the placer itself never
mirrors.

#### 5.5.7 Walls and doors

Town houses get their walls, doors and windows from the wall modules of §5.5.4 step 10 and
the module markers (§6.2 markers 1 and 2: static kinds 1 `door`, 2 `big-door` in mode 7,
3 `window`). This routine builds no separate town wall or gate (no wall builder is called;
`gate` and the `castle-*` models are used elsewhere).

#### 5.5.8 Plaza trees and benches (0x5bbad8..0x5bc8d6)

After the plot loop. For each plot in plot-loop order (i outer, j inner) whose **final**
kind is 0 or 7: flag Fp = 0; then four points, x offset p outer, y offset q inner, each
from {0, h} with h = (256 / g) / 2 (25 for g = 5, 32 for g = 4) and the quarter
k = s / 4 (12 or 16): **X = ox + k + p**, **Y = oy + k + q**. For each point:

1. w = q(X, Y) at fx positions (0x5fa4c0, §5.1); if w < 0.72 (double compare) the point is
   skipped (no draws).
2. H = the lowest open z of column (X, Y).
3. **Type 1 only**: if the block at (X, Y, H − 1) is not of type 11 (road), the point is
   skipped (nothing below happens, Fp unchanged). Otherwise the **plaza disc**:
   * Pass 1: x′ = X − 8 .. X + 8 (outer), y′ = Y − 8 .. Y + 8: with
     r² = (X + 0.5 − x′)² + (Y + 0.5 − y′)² (doubles), if r² ≤ 64: H′ = lowest open z of
     (x′, y′); H = max(H, H′).
   * Pass 2, same order: if r² ≤ 64: H′ = lowest open z of (x′, y′); mo, wm = moisture and
     warmth of (x′, y′) (0x5c4800, 0x5c4dd0); for z = H′ − 1 .. H: if r² < 49: the block is
     the terrain surface colour (terrain spec §6.1, 0x5c56e0) evaluated at the **centre
     column** (X, Y, H) with these mo and wm, with its own type byte (no draw); otherwise
     **draw**: k = f32(f32(f32(d)·0.1f)/32767 + 0.75) and the block is
     (trunc(St(x′, y′, z)·k), type 6). Written with the block writer 0x44e7a0 at (x′, y′, z).
     So the disc of radius 8 is filled up to height H, a paving stone ring outside radius 7
     (one draw per ring block).
4. Fp = 1. **Tree**: m = the warmth (+0x08) of column (X, Y) (0x5c8350; the blend 0x5c4dd0
   when the column is missing); tree kind = 3 if m > 0.8, else 5. If H > 1: **draw** d1,
   **draw** d2, then the tree builder 0x5df960(X, Y, H + 1, s = d2 % 6 + 7,
   t = (d1 & 7) + 13, kind, zone) with its own draws (trees spec §4).
5. **Type 1 only**: four benches; each: **draw**; if d & 7 ≠ 0: a static of kind **18**
   (`bench`), scale (3, 0.4, 0.4), at z = H and (X + 0.5, Y − 7.5) rotation 0;
   (X + 0.5, Y + 9.5) rotation 2; (X − 7.5, Y + 0.5) rotation 3; (X + 9.5, Y + 0.5)
   rotation 1 (in this order); settled (§2.3 with the floor check) and inserted.

After the four points, if Fp = 1: the point (ox + s/2, oy + s/2, lowest open z of that
column) is appended to the **plaza list** (§5.5.9).

**St(x, y, z)** (0x5fa170, stone colour): v = ((N(0.01y + 98984, 0.3z + 8437) +
N(0.01x, 0.3z))·0.5 + 1)·0.5 (sum and products f32, noise arguments double, evaluated in
this order); base = 200v + 50; R = base + 20·N(O234 + 0.005x, O238 + 0.005y),
G = base + 20·N(O23c + 0.005x, O240 + 0.005y), B = base + 20·N(O244 + 0.005x, O248 + 0.005y)
(O… = the world noise offsets at world + 0x800234 …, as in terrain spec §6.7); clamp to
[0, 255].

Draws per point: 0 when skipped; otherwise (type 1) one per ring block, then 2 + the tree's
own when H > 1, then 4 (+0 per bench; benches make no draws of their own).

#### 5.5.9 NPCs (0x5bc8d6..0x5bf34d)

After §5.5.8. From the zone's house list (zone + 0x88, build order) two lists are made: the
**inns** (sub 1) and the **shops** (sub 2..5). Creatures are 0x10f0-byte records (0x5adb20),
appended to zone + 0x18; positions from integer points are converted to fx exactly
(0x5a6a70). "AI objects" are helper objects built with 0x4dc2d0, 0x42c550(20.0),
0x4c8130, 0x58cfa0(2.0), 0x4dfab0, 0x4db100, 0x4dbf10 and stored at +0x109c; with 0x5bf4f0
the creature's own position is recorded as its home point. None of them draws.

**Town (Lz.type 1).**

1. **Points of interest** (no draws): the 8 × 8 location records of the region
   (zx >> 6, zy >> 6) (0x434a90; records of 0x68 bytes from + 0x14018) — every record whose
   type (+0x18) is neither 1 nor 10, and of type 0 only when the lookup 0x4a6ad0 at the
   record's position finds a non-empty entry — are appended, with their position, to the
   POI list (0x5ae000, 0x469000; contents *inferred*).
2. **Level band** (lo, hi) = (1, 20); if the region record (0x47faa0) exists, 0x5ee4b0 sets
   it: (1, 10); if +0x10 < 0.2: (10, 20); then if +0x0c < 0.2 and +0x10 > 0.8: (15, 25); if
   +0x0c > 0.8 and +0x10 > 0.8: (10, 20); if byte +0x08 = 1: (20, 30). The band is passed to
   the vendor routines, which do not read it (each takes three stack arguments, `RET 0xC`,
   and uses only the creature).
3. **Per house**, in list order; a house whose spawn list is empty is skipped:
   * **sub 1 (inn)**: only if its counter list is not empty: **draw**: position =
     counter[d mod n] (unsigned); +0x28 = 3; **draw**: kind +0x2c = (d & 1) + 2; +0x34 =
     Lz.level; +0x30 = **0x84**; AI objects; appended. 2 draws.
   * **sub 2, 3, 4, 5 (shops)**: the same two draws with +0x30 = **0x80, 0x81, 0x82, 0x83**,
     then the vendor routine fills the creature's inventory (+0xf6c): sub 2 → **V1**
     (0x5c9b30), sub 4 → **V2** (0x5c8390), sub 3 and sub 5 → **V3** (0x5ca0a0).
   * **sub 0**: **draw**: n = (d & 1) + 1 residents. For each:
     1. **draw**: position = spawn[d mod |spawn|]; +0x28 = 3, +0x30 = 0x88; **draw**: kind
        = (d & 1) + 2; +0x34 = Lz.level.
     2. If the POI list is not empty: **draw**; if even: +0x60 = (x, y, 0) of POI entry
        c mod |POI| (c = a running counter of the zone, starting at 0, incremented on use;
        no draw), +0x5c = 4.
     3. **Draw**: d % 10 = 0 → +0x5c = 3.
     4. **Day schedule** at +0x10a0 (0x5bf470 appends (point fx, time); times in
        milliseconds; "+t" below means T ← (time of the last entry) + t):
        * (own position, 0); **draw**: T = (d % 180 + 420)·60000;
        * plaza list not empty: **draw**: (plaza[d mod n], T); **draw**: + (d % 60 + 60) min;
        * market list not empty: **draw**: (market[d mod n], T); **draw**: + (d % 180 + 60) min;
        * shops not empty: **draw**: shop = shops[d mod n]; if its entrance list is not
          empty: **draw**: (entrance[d mod m], T); **draw**: + (d % 20) min;
        * plaza not empty: as above (3 draws if the list is not empty);
        * inns not empty: **draw**: inn; if its entrance list is not empty: **draw**: point;
          **draw**: + (d % 180 + 60) min;
        * shops not empty: **draw**: shop; if its entrance list is not empty: **draw**:
          point; **draw**: + (d % 30) min; then again **draw**: shop; if not empty:
          **draw**: point; **draw**: + (d % 30) min;
        * plaza not empty: as above;
        * own house's entrance list not empty: **one draw whose value is unused**; then
          (own position, T); **draw**: + (d % 60 + 60) min;
        * finally (own position, T + 1,800,000) (30 minutes later), no draw.
        ("min" = × 60000.)
     5. AI objects; appended.

**Vendor stock** (V1, V2, V3), 30 rounds k = 0..29. Each round starts with a level:
L0 = trunc(f(k/30)), L1 = trunc(f((k + 0.99999)/30)) with f(t) = (1/(1 − t) − 1)·20 + 1
(0x4462f0; all f32), **draw**: L = L0 + d % (L1 − L0 + 1), at least 1; round 0 uses L = 1.
Items are appended to +0xf6c (0x46ebe0); an item record has category (+0), subtype (+1),
seed (+0x04), product (+0x08), rarity (+0x0c), material (+0x0d), +0x0e and 16-bit level
(+0x10) (items spec §2). "Rarity draws" are: **draw** r = (d % 100 = 0); **draw**: if
d % 1000 = 0, r + 1; **draw**: if d % 10000 = 0, r + 1; r = min(r, 4).

* **V1** (general shop). Round 0 only, before the rest: items (11, 12) material 24 level 1;
  (24, 0) material 1 level 1; for i = 1..4: **draw**; d % (10i) = 0 → (24, 0) rarity i,
  material 1, level 1; then (23, 0) rarity 0 material 2; (23, 1) material 2; then
  (20, X) material 0 level 1 with **draw** X = (34, 35, 19, 26, 30, 87)[d % 6]. Every round:
  **draw** m = d % 3 recipes; each: **draw** r = d & 1; **draw** d % 100 = 0 → r + 1;
  **draw** d % 1000 = 0 → r + 1; **draw** d % 10000 = 0 → r + 1; r = min(r, 4); record
  cleared (level 1); **draw**: even → armour(L, r, −1) (24 draws), odd → weapon(L, r, −1)
  (16 draws) (items spec §2); the result is copied, product = its category, category = 2;
  appended. Then **draw**: odd → item (1, 7) level L.
* **V3** (weapon shop and identifier). Per round after the level draw: eleven candidates of
  category 3 with +0x0e = 1 and level L, each ending with its rarity draws and a **seed
  draw** (+0x04 = the full draw value), in this order: subtype **draw** d % 3 then an unused
  draw (material 1); subtype **draw** d % 3 + 15, unused draw (material 1); then subtypes
  13, 3, 5, 4 (material 1), 6 (material 2), each with one unused draw first; then **five
  unused draws**; subtypes 8, 10, 11 (material 2) with one unused draw each; subtype 12:
  **draw**: material **12 if odd, 11 if even**, then one unused draw. Each candidate costs
  5 draws (6 for the first two and the last) — 63 in all. Then **draw**: p = d % 3 + 1
  picks; each, while candidates remain: **draw**: index d mod (count) (unsigned); that
  candidate is appended and removed. Per round 1 + 63 + 1 + p draws.
* **V2** (armour shop). Per round after the level draw: candidates with subtype 0, level L,
  +0x0e = 1, each with one unused draw, the three rarity draws and a seed draw (5 draws):
  material 1: categories 7, 4, 5, 6; material 25: 7, 7, 4, 5, 6; material 26: the same
  five; material 27: the same five; then category 8 and category 9, each preceded by a
  **draw**: material 12 if odd, 11 if even (6 draws each). 21 candidates, 107 draws; then
  the picks as in V3. Per round 1 + 107 + 1 + p draws.

**Ruins (Lz.type 5).**

1. **Kind lists** (no draws): a list K of creature kinds and a list GL of groups, each
   group a pair of kind lists (a, b), by Lz.variant: 0: K = (41, 17, 96, 96), GL =
   ((41, 17 | 59, 37, 38, 40, 39)); 1: K = (9, 10), GL = ((9, 10 | 22, 55)); 2: K = (83, 84),
   GL = ((83, 84 | 53, 60), (81 | —)); 3: K = (76), GL = ((76 | 58, 60), (81 | —));
   4: K = (15, 16), GL = ((15, 16 | 96), (X | —)) with X = 94, 17, 97 for Lz.serial (+0x20)
   % 3 = 1, 2, 0; other variants: K = (11, 12), GL = ((46 | 19)).
2. **Per plot** (plot-loop order):
   * kind 20 (the antique building): creature at (cx, cy, z + 1), z = the first open z at
     or above column +0x14 (surface − 8); **draw**: facing +0x54 = f32(f32(d)·360)/32767;
     +0x34 = Lz.level; +0x28 = 1; **draw**: kind = K[d mod |K|]; +0x58 = Lz sub-level byte
     (+0x28); +0x7a |= 0x1000 | 0x200; +0x10e8 = 1; AI objects; appended. 2 draws.
   * other kinds ≠ 2 with wc > 0.2: points as in §5.5.10 (p outer, q inner ∈ {0, s});
     each: **draw**; d % 5 = 0 and the column exists: creature at the point, z as above;
     **draw** facing; +0x34 = Lz.level; +0x28 = 1; +0x7a |= 0x1000; **draw**: kind =
     K[d mod |K|]; +0x58 as above; appended (no AI objects).
3. **Per house** (house list order) with a non-empty spawn list; P = a copy of it:
   * **draw**: m = d % 3 rings; for each, while P is not empty: **draw**: centre =
     P[d mod |P|], removed from P; **draw**: n = d % 3 + 2; member t = 0..n − 1:
     a = f32(2tπ/n) (double, then f32); position = centre (fx) + (trunc(f32(f32(cos a)·2)·
     65536), trunc(f32(f32(sin a)·2)·65536), 65536) (cos/sin the precise SSE2 routines of
     MSVCR110); facing +0x54 = f32((a/π)·180 + 90); +0x34 = Lz.level; +0x28 = 1;
     +0x7a |= 0x1000; **draw**: kind = K[d mod |K|]; +0x58 as above; AI objects; **draw**:
     e = d & 1 items: an item record (category 1, subtype 1, level = +0x34; a potion,
     *inferred*) added e times to the inventory (+0xf6c); appended.
   * if GL is not empty: **draw**: m = d & 1 groups; for each, while P is not empty:
     **draw**: point = P[d mod |P|], removed; **draw**: group = GL[d mod |GL|]; if its list
     a is not empty: creature at point + (0, 0, 1); **draw** facing (as in 2);
     +0x34 = Lz.level; +0x28 = 1; +0x7a |= 0x1000; **draw**: kind = a[d mod |a|]; +0x58;
     AI objects with the home point; if the zone's house list is not empty: **draw**: a
     house; if its spawn list is not empty: **draw**: one of its points is added to the AI's
     route; **draw**: e = d & 1 items as above; appended. (The group's list b is not
     read here.)

#### 5.5.10 Creatures on open plots (in the plot loop, every kind except 2)

0x5ba2a6..0x5ba555. If the plot's wc > 0.2 (float compare): for p ∈ {0, s} (outer),
q ∈ {0, s} (inner): **draw**; if d & 7 = 0: X = ox + s/4 + p/2, Y = oy + s/4 + q/2 (C
divisions); if the column exists: z = the first open z at or above column +0x14
(surface − 8); a creature at (X, Y, z + 1) (fx); **draw**: facing +0x54 =
f32(f32(d)·360)/32767; **draw**: kind +0x2c = (34, 30, 19, 26, 33)[d mod 5] (§5.1);
**draw**: level +0x34 = lo + d % (hi − lo + 1) with (lo, hi) from 0x444020: (3, 6) for
kinds 19, 26, 30, (2, 4) for 33 and 34; +0x28 = 5; +0x58 = Lz sub-level byte (+0x28);
appended to zone + 0x18. Draws: 4 per plot, + 3 per creature. (Checked 0x5ba29b..0x5ba2f4:
the gate is kind ≠ 2 and wc > f32 0.2 (0x6fcdbc), strict. The plot loop's per-plot calls
before the kind-2 test, 0x5c4800 and 0x5c4dd0 at 0x5b2478/0x5b24b1, make no draws, and all
plot kinds are fixed before the plot loop starts, so nothing in §5.5 can change a role.)

None of these steps re-seeds.

### 5.6 Test vectors

* Plot origins for g = 5, zx = 32800: ox = 8396800 + {0, 51, 102, 153, 204}; size 51; the
  last plot ends at 8396800 + 254, so column 255 of the zone belongs to no plot.
* Plot origins for g = 4: + {0, 64, 128, 192}, size 64.
* Draw count of a zone where no column has w > 0.1 (all plots kind 0, list B empty, the
  builder stops at §5.4 step 2): type 5: exactly g² = 16; type 1: 25 + one garden draw per
  plot with hi − lo < 16.
* Kind 8 plinth (dead code, never reached): 4,371 draws; the first box is (cx − 2, cy − 2, T,
  (4, 4, 1)), the last (cx − 40, cy − 40, T − 19, (23, 23, 1)).
* Market in plot (2, 2) of a type-1 zone with zx = 32800, zy = 32801 (g = 5, s = 51):
  ox = 8396902, oy = 8397158, cx = 8396927, cy = 8397183. Slot 1 with q = 1 stands at
  (8396920, 8397165, hi), slot 7 with q = 1 at (8396946, 8397176, hi), slot 19 with q = 0
  at (8396908, 8397180, hi).
* Market slot draws with the srand(123) stream starting at slot 1: 440 % 5 = 0 → slot 1
  empty (1 draw); slot 2: 19053 % 5 = 3 → placed, q = 23075 % 3 = 2 → (cx, oy + 8), stand
  kind 21 + 13104 % 3 = 21 (`market-stand1`); slot 3 starts with 32363.
* Garden, s = 51, crop 3: 2 + 2,601 (the per-column cornflower/wheat draws) + the per-grass
  column draws + the fence draws.
* Goods scale: crate with d = 16383: f = f32(f32(8191.5/32767 + 1)·1.5) = 1.8749886

House vectors (srand(123) stream 440, 19053, 23075, 13104, 32363, 3265, 30749, 32678, 9760,
28064, 20429, 750, 4480, … taken from the start of the step named):

* **Ordinary house layout** (sub 0): k = 440 % 3 = 2; 19053 odd → the two-storey bar
  (0..2, 0, 0) = 2, (·, 0, 1) = 1 with room 1 at (1, 0, 1), (·, 0, 2) = 3; 23075 odd → wing
  (0, 1, 0..2) = 2, 1, 3; 13104 even → no wing at (2, 1); then (1, 1, 0..2) = 2, 1, 3;
  mirror draw 32363 odd → mirror = 0. 5 draws; storey cells (0,0,1), (1,0,1), (2,0,1),
  (0,1,1), (1,1,1); roofs at z = 2.
* **Inn layout** (sub 1): 440 even → no (2, 1) column; mirror 19053 odd → 0. 2 draws.
* **Rotations** starting at 440 in view order (a, b, z inner): (0,0,0) r = 440 & 3 = 0,
  (0,0,1) 19053 & 3 = 1, (0,0,2) 23075 & 3 = 3, (0,0,3) 13104 & 3 = 0, (0,1,0)
  32363 & 3 = 3, …
* **Cell centre** in the plot (2, 2) of the market vector (ox = 8396902, oy = 8397158):
  O.x = 8396909, O.y = 8397165; a module of sx = sy = 13 in cell (1, 1) has X0 = O.x + 20 −
  6 = 8396923, Y0 = 8397179; its −y entrance stairs (say sy = 9) have Y0 = O.y − 6 + 13 −
  4 = 8397168.
* **Helper W**, room 0: 440 % 50 = 40 → 19053 % 6 = 3 → kind 30 (`desktop`), (3, 1, 1),
  shifted 13107 fx; 2 draws. Room 1: 19053 odd → kind 20 (`bedtable`); 2 draws. Starting at
  23075 (room 0): 23075 % 50 = 25 → 13104 % 6 = 0 → 32363 % 3 = 2 → kind 34 (`shelf3`); 3
  draws.
* **Helper G**: 440 % 7 = 6 → kind 28 (`shelter`); starting at 23075: 23075 % 7 = 3 → kind
  27 (`sack`). Market/G crate with d = 440: f = 1.5100712.
* **Open-plot animal** (§5.5.10) starting at 440: 440 & 7 = 0 → creature; facing
  f32(f32(19053)·360)/32767 = 209.3289; kind (34, 30, 19, 26, 33)[23075 % 5 = 0] = 34;
  level 2 + 13104 % 3 = 2; next point 32363 & 7 = 3 → none.
* **Resident schedule**: first time draw d = 440 → T = (440 % 180 + 420)·60000 =
  30,000,000 ms.
* **Vendor levels** (L0, L1) for rounds k = 0..29: (1,1) (1,2) (2,3) (3,4) (4,4) (5,5)
  (6,7) (7,8) (8,9) (9,10) (11,12) (12,14) (14,16) (16,18) (18,20) (21,23) (23,27) (27,30)
  (31,35) (35,40) (41,47) (47,55) (56,66) (66,80) (81,100) (100,130) (131,180) (180,280)
  (281,580) (580,67108844) — in the last round L = 580 + d.
* **V1 round 0** starting at 440: L draw 440 (then L = 1); rarity tests 19053 % 10 = 3,
  23075 % 20 = 15, 13104 % 30 = 24, 32363 % 40 = 3 → no extra (24, 0) items; pet subtype
  (34, 35, 19, 26, 30, 87)[3265 % 6 = 1] = 35; m = 30749 % 3 = 2; first recipe:
  r = 32678 & 1 = 0, 9760 % 100 ≠ 0, 28064 % 1000 ≠ 0, 20429 % 10000 ≠ 0 → r = 0;
  750 even → armour(1, 0, −1) begins with 4480.
* **Ruin ring offsets** (fx, added to the centre): n = 3: t = 0: (131072, 0), facing 90;
  t = 1: (−65536, 113511), 210; t = 2: (−65535, −113511), 330. n = 2: (131072, 0), 90;
  (−131072, 0), 270. n = 4: (131072, 0), (0, 131072), (−131072, 0), (0, −131072); facings
  90, 180, 270, 360.

---

## 6. The home building and the prefab placer (0x5e8c2b..0x5e8d80, 0x5f0ce0)

### 6.1 Home building (unnamed worlds only)

Runs only if the world name is empty (world + 0xa4 = 0) and the zone contains the home point:
zx = trunc(homeX) floor-div 256 and zy = trunc(homeY) floor-div 256, and the column
(trunc(homeX), trunc(homeY)) exists. Named worlds — every normal single-player and
multiplayer world — never run it, so it does not affect their draw stream.

The model is slot 2123 (`character-platform.cub`; none if the list is shorter). It is placed
by the prefab placer at (trunc(homeX) − sx/2, trunc(homeY) − sy/2, top of the home column),
with sx, sy the model's x and y sizes (model + 0x44, + 0x48, integer halves), rotation 0,
type byte 6, **mode 0**, the zone, fill-down on, no crop and no output list (pushes at
0x5e8d4e..0x5e8d6d; the earlier reading "mode 6" was the type byte). Mode 0 interprets no
markers, so the home building makes **no draws**.

### 6.2 The prefab placer (0x5f0ce0)

Read from the assembly (0x5f0ce0..0x5f4ae3) and its helpers 0x5cb3b0 and 0x5ee0f0.

**Call.** placer(world, model, origin (X0, Y0, Z0) blocks, rotation r, type byte T, mode M,
zone, fill-down F, crop rectangle (e0, e1, e2, e3), output list). r is taken as r & 3
(callers pass 0..3). The model has sizes sx, sy, sz (model + 0x44, + 0x48, + 0x4c) and
voxel colours (3 bytes each, model + 0x30, index (k·sy + j)·sx + i); the colour (0, 0, 0)
means **empty**. Reads outside the model return empty.

**Zone clip.** If a zone is given: with (wx, wy) = (sx, sy), swapped when r is odd, the
placer returns at once (no writes, no draws) if X0 + wx ≤ 256zx, Y0 + wy ≤ 256zy,
X0 ≥ 256zx + 256 or Y0 ≥ 256zy + 256.

**Warmth.** wm = the warmth of the column (X0, Y0) (+0x08), or the warmth blend 0x5c4dd0 at
(X0, Y0) when that column is missing. Read once.

**Rotation.** Model voxel (i, j, k) lands on the block

| r | X | Y |
|---|---|---|
| 0 | X0 + i | Y0 + j |
| 1 | X0 + j | Y0 + sx − 1 − i |
| 2 | X0 + sx − 1 − i | Y0 + sy − 1 − j |
| 3 | X0 + sy − 1 − j | Y0 + i |

and Z = Z0 + k. There is no mirroring.

**Crop.** The crop entries cut whole model columns, named in world directions
(e0 = −x, e1 = −y, e2 = +x, e3 = +y): i runs over [i0, sx − i1) and j over [j0, sy − j1)
with (i0, i1, j0, j1) = (e0, e2, e1, e3) for r = 0, (e3, e1, e0, e2) for r = 1,
(e2, e0, e3, e1) for r = 2, (e1, e3, e2, e0) for r = 3.

**Order.** i ascending (outer), j ascending, then k from sz − 1 down to 0 (inner). Draws
happen only at marker voxels, in this order.

**Per column (i, j):**

1. zlo = the lowest and zhi = the highest k with a non-empty voxel (zlo = sz, zhi = 0 when
   the column is empty). "Inside" below means zlo < k < zhi.
2. **Fill-down** (F set): if voxel (i, j, 0) is non-empty and the target column (X, Y)
   exists: for z = Z0 − 1, Z0 − 2, … while z > the column's bottom: if the block at
   (X, Y, z) (implicit read rules) is solid (type ∉ {0, 2}), stop; otherwise write the
   colour of voxel (i, j, 0) with type byte **6** there.
3. For each k (from the top): let C be the voxel colour and E the block now at the target.
   * C empty: if inside, write (255, 255, 255) with type byte **0xC0** (air, dry, keep);
     otherwise nothing.
   * C non-empty and E has bit 0x40 set and E is solid: skip (structure blocks already
     present win).
   * C non-empty, M ∉ {1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15}: the **default
     write** below.
   * Otherwise the **marker table** below, first match by exact colour; a colour that
     matches no marker gets the default write.

**Default write.** If M = 8 and C = (0, 0, 255): write (255, 255, 255) with type byte **2**
(water). Otherwise write C with type byte T | 0x40.

Every block write of the placer (default, air, marker air) also appends a node (position,
block) to the output list when one is given; this has no effect on draws.

**Blobs** (markers 1, 2 and 10). A voxel (i, j, k) of colour C is a blob **origin** only if
none of (i − 1, j, k), (i, j − 1, k), (i, j, k − 1) has colour C. From the origin the blob
runs along +x while the colour stays C (row j, k), along +y (column i, k) and along +z
(i, j); this gives the model box [i, i′] × [j, j′] × [k, k′]. Both corners are rotated with
the table above but without the origin (X0, Y0), sorted per axis, and the maxima get + 1. The
static then gets: x = (min.x + max.x)·32768 + X0·65536, y likewise, z = (min.z + Z0)·65536;
rotation index 0 if (max.x − min.x) > (max.y − min.y), else 1; scale (long side, short side,
max.z − min.z) as floats; byte +0x30 = 1. Non-origin voxels of the blob make no object.

**Marker table.** P = the block position (X, Y, Z) as fx; "+(a, b, c)" adds a, b, c blocks
(as 65536·a, rounded as stated). "Air" = write (0, 0, 0) with type byte **0x40**; "air if
inside" = only when zlo < k < zhi; "untouched" = nothing is written.

| # | Colour | Effect | Draws |
|---|---|---|---|
| 1 | (255, 0, 0) | Door: at a blob origin, a static of kind **1** (`door`), or **2** (`big-door`) when M = 7, from the blob; appended directly to the zone's static vector (zone + 0x0c). Always air. | 0 |
| 2 | (0, 255, 0) | Window: at a blob origin, a static of kind **3** (`window`) from the blob, inserted with 0x5a0e00. Always air. | 0 |
| 3 | (0, 127, 127) | Wall plant: if wm ≤ 0.5: untouched. If the block now at the target is solid: untouched. Otherwise draw; if d % 5 = 0: draw: a sprite of kind **41 + d % 3** (`ivy`, `wall-roses-red`, `wall-roses-white`) at P + (32964, 32964, 196) fx, +0x20 = 0.15, +0x24 = −90r, scale fields 1, +0x38 = 0, added to zone + 0x04 with 0x5c03f0. Then air. | 1 or 2 |
| 4 | (255, 127, 0) | Bed: static **19** (`bed`) at P + (0.5, 0.5, 0), rotation 0, scale (2, 3, 1); static **20** (`bedtable`) at P + (2.25, 0.5, 0), rotation 0, scale (1, 1, 1); both inserted. Air if inside. (The rotation r is not applied to these two.) | 0 |
| 5 | (255, 127, 127) | Shelf: draw: static **32 + d % 3** (`shelf1..3`) at P + (0.5, 0.5, 0), rotation r, scale (2, 3, 1). Air if inside. | 1 |
| 6 | (127, 127, 0) | Table set: draw: static **12** (`table`) at P + (0.5, 0.5, 0), rotation d & 3, scale (3, 3, 1); draw: static **16** (`stool`) at P + (2.7, 0.5, 0) (x offset trunc64(176947.2)), rotation d & 3, scale (1, 1, 0.5); the same stool again at P + (−1.7, 0.5, 0) (−111411) with the same rotation; then one more draw whose value is unused. Air if inside. | 3 |
| 7 | (255, 255, 0) | Lantern: sprite **13** (`lantern02`) at P + (0.5, 0.5, 0), +0x20 = 0.1, +0x24 = 0, tint (0.6, 0.5, 0.4), +0x38 = 1, to zone + 0x04 (0x4862e0). Air if inside. | 0 |
| 8 | (63, 0, 0) | Crest: sprite **48** (`crest1`) at P + (0.5, 0.5, 0.5), +0x20 = 0.065, +0x24 = −90r, scale fields 1, +0x38 = 0. Air if inside. | 0 |
| 9 | (0, 0, 255) | Sign: by M: 2 → **33** `inn-sign`, 3 → 34 `shop-sign`, 4 → 35 `weapon-shop-sign`, 5 → 36 `armor-shop-sign`, 6 → 37 `identifier-sign`, 13 → 38 `smithy-sign`, 14 → 39 `carpentersshop-sign`, 15 → 40 `weavingmill-sign`; other modes no sign. Sprite at P + (0.5, 0.5, −0.5), +0x20 = 0.1, +0x24 = (−1 − r)·90, scale fields 1, +0x38 = 0. Air if inside. | 0 |
| 10 | (255, 0, 255) | Flower box: if wm ≤ 0.5: untouched. Otherwise at a blob origin: draw: static **47 + d % 3** (`flower-box01..03`) from the blob, inserted. Air if inside. | 0 or 1 |
| 11 | (0, 255, 255) | M = 13: static **71** `furnace`, scale (3, 3, 6); M = 14: **75** `sawbench`, (1.5, 1.5, 1); M = 15: **73** `spinningwheel`, (1.5, 1.5, 1); each at P + (0.5, 0.5, 0), rotation index −r (the negative integer is stored), air if inside. Any other M: a creature, +0x28 = 6, kind **141** if M = 10 else **142**, position P + (0.5, 0.5, 0.5), facing +0x54 = (2 − r)·90, +0x34 = level from 0x59a9b0 at (X, Y), appended to zone + 0x18; always air. | 0 |
| 12 | (0, 0, 127) | M = 13: static **77** `customization-bench`, scale (1.8, 1.8, 1.2), at P + (0.5, 0.5, 0), rotation −r; always air. Other M: untouched. | 0 |
| 13 | (0, 255, 127) | M = 13: static **72** `anvil` (1, 1, 1); M = 14: **76** `workbench` (1.8, 1.8, 1.2); M = 15: **74** `loom` (1.5, 1.5, 3); at P + (0.5, 0.5, 0), rotation −r; air if inside. Other M: untouched. | 0 |
| 14 | (0, 127, 0) | Vase: draw: static **56 + d % 9** (`flower-vase1..9`) at P + (0.5, 0.5, 0); draw: rotation d & 3; scale 0.5 on all axes if the kind is below 60, else 1; inserted. Air if inside. | 2 |
| 15 | (127, 0, 127) | NPC: creature with +0x28 = 3; draw: kind +0x2c = d & 3; facing +0x54 = (2 − r)·90; position P + (0.5, 0.5, 0.5); equipment slot +0x350 = (category 4, subtype 0), material +0x35d = 6; +0x34 = level from 0x59a9b0; then the mode table below; AI helper objects (no draws); appended to zone + 0x18. Air if inside. | 1 + mode table |

**NPC marker by mode.** Equipment slots are item records inside the creature: weapon
+0x8c8, chest +0x698, gloves +0x350, shoulder +0x468, boots +0x580 (names *inferred* from
the category numbers 3, 7, 4, 6, 5); in each slot byte +0 = category, +1 = subtype,
+0x0c = modifier (d % 5), +0x0d = material, 16-bit +0x10 = level. Draws are listed in order.

| M | Byte +0x30 | Fields and draws |
|---|---|---|
| 6 | 0x83 | none |
| 7 | 0x89 | +0x34 = 20; draw: kind = (d & 1) + 2 |
| 9 | 1 | +0x7a = 0x40, +0x34 = 20, kind = 2; weapon (3, d % 3 + 15) (draw), weapon modifier d % 5 (draw), level 20, material 1; chest cat 7, modifier d % 5 (draw), level 20, material 1; gloves cat 4, modifier (draw), material 1; shoulder cat 6, modifier (draw), material 1; boots cat 5, modifier (draw), material 1 — 6 draws |
| 10 | 2 | as 9 but weapon subtype (d & 1) + 6 (draw), weapon material 2, other materials 26 — 6 draws |
| 11 | 4 | as 9 but weapon (3, 5) with no subtype draw, weapon material 1, other materials 27 — 5 draws |
| 12 | 3 | as 9 but weapon (3, 10), weapon material 2, other materials 25 — 5 draws |
| 13, 14, 15 | 0x85, 0x86, 0x87 | none |
| 1..5 | unchanged | none |

**Modes used by the callers**: 0 (dungeon arches, sphinx, obelisk, antique buildings, the
home platform: no markers at all), 1 (house modules), 7 (tower, landing place), 8 (well:
blue voxels become water), 9..15 (class and craft houses). Modes 2..6 are the house mode
Mh = sub + 1 of §5.5.4 step 9, passed only to the door module and the entrance stairs of
the inn (sub 1 → 2, `inn-sign`) and the four shops of Q = 1 (sub 2..5 → 3..6: shop, weapon
shop, armour shop, identifier signs); ordinary houses use mode 1 there too.

**Draw count of one placement** = Σ over marker voxels in placement order of the "Draws"
column (rows 3, 5, 6, 10, 14, 15). Everything else is deterministic. A model's draw count
therefore depends only on its marker voxels, on wm (rows 3 and 10), on the block at the
target of each row-3 voxel, and on the structure blocks already present (the skip rule).

Positions given as "+0.5" are 32768 fx; P + (0.5, 0.5, −0.5) uses the doubles 0.5, 0.5,
−0.5 times 65536; the creature positions use the float triple (0.5, 0.5, 0.5) times 65536
(0x42c460).

---

## 7. Draw order of the whole range (zone generator, after the ravine boulders)

In order, for one zone (each line is skipped entirely when its condition fails):

| § | Pass | Condition | Draws |
|---|---|---|---|
| 6.1 | home building | unnamed world, zone holds the home point | 0 (mode 0) |
| 2 | camp attempts | (zx + zy) odd | 11 to 38 when a camp is built (each failed try 2 or 3, the successful one 11); 20 to 30 when none is |
| 3 | boulder pass 3 | E.kind ∉ {1, 3, 4} | 1 + 5n, n = 0..19 |
| 5 | town builder | Lz type 1 or 5 | g² + … (§5.2–§5.5, fully specified; placements add the marker draws of their models, §6.2) |
| 4 | type-10 site | E.kind = 4 | 14, 16 or 18 |
| — | voxel-tree grid | always | vegetation spec §5 |

The dungeon builder (§1) runs earlier, at caves spec §6.1, before the terrace and ravine
boulders.

Exactly reproducible today: every zone whose Lz is not of type 1 or 5 (camps, boulder pass
3, type-10 sites and the home building are fully specified), including kind-3 (dungeon)
zones as long as the dungeon's item generators are not reached (§1.11). Town and ruin
zones are specified to the draw as well (§5); reproducing them needs, besides this
document, the marker voxels of every placed model (§6.2, counted from the extracted
`.cub` files), the settle test (§2.3), the tree builder (trees spec) and the armour and
weapon generators (items spec §2) for the vendors.

---

## 8. Open questions

1. **Model list.** The slot numbers come from the registry 0x4ee230 (`PUSH key … PUSH slot …
   CALL 0x468c60`). That world + 0x20 is this registry's vector is inferred from the bounds
   checks (2123, 2544, 2545 all lie inside its 2569 slots) and the plausible names; confirm
   by dumping world + 0x20 at run time and comparing a few slot pointers with the registry.
2. **Item generators** — closed: see [alpha-items-generator.md](alpha-items-generator.md), which
   also corrects the calls (0x5f7a60 takes only (level, rarity cap); the −1 pushed before it
   belongs to the next call). Former text: 0x5f7a60 (111 draw sites) and 0x5f6d50: used by dungeon chests,
   table loot (d % 6 cases 4 and 5) and the boss (§1.10.6), and by the town's class
   houses and NPCs. They need an items specification; until then a dungeon's draw count
   is exact only up to the first call.
3. **Dungeon details still inferred**: the +y parapet offsets and the v = 3 roof flag
   (§1.10.3), the +y arch rotation (§1.10.6 step 8), the v = 5 corridor and spiral
   positions (§1.10.7), and the carve noise on the three later faces (§1.10.2 item 2).
   A draw-count hook (caves spec §11) on one kind-3 zone of each variant is the quickest
   check: every box costs 3 + its volume.
4. **Kinds 4 and 8 of the town plots** — closed: they are dead code. The plot list is a
   local vector of 0x5af170 (EBP − 0x5d28, cleared at entry, built by the survey); every
   write of a plot kind in the routine was listed from the assembly (0x5af6bb … 0x5b0230:
   7, 2, 0, 6, 9, 10..16, 3, 5, 0, 20, 0, 17, 19, 18) and none writes 4 or 8, and no other
   routine can reach the vector. The plinth (§5.5.1) and the landing place (§5.5.5) are
   never built.
5. **Town pass** — closed (§5.5.3–§5.5.10): (a) the sub-0 layouts are §5.5.4 step 3;
   (b) module choice, rotations, crops and positions are steps 10–13 and the furniture
   steps 15–17; (c) the garden blend is §5.5.3 step 2.1 (0x451510 is a vector scale,
   0x5c7140 the cliff colour); (d) plaza radius 8 (r² ≤ 64) with the inner radius 7
   (r² < 49), benches and trees in §5.5.8; (e) class houses at (cx − 16, cy − 16, Hm − 7),
   case index kind − 11 (§5.5.5); (f) the NPC pass is §5.5.9 (0x5a6a70 is only the
   int → fx conversion, not an appearance call; 0x5ee4b0 is the level band) plus the
   open-plot animals of §5.5.10, which the outline had missed; (g) modes 2..6 are the inn
   and shop doors (§5.5.4 step 9). Still *inferred* or unexplored within §5: the meaning of
   the creature fields +0x28, +0x30, +0x5c, +0x60, +0x7a bits and +0x10e8; the AI helper
   objects (no draws); the POI filter for location type 0 (0x4a6ad0); the item category
   names (category 1 subtype 1 "potion", category 20 pets); the effect of the kind-4
   airship code (dead).
6. **Prefab placer**: the marker table (§6.2) is complete; its draw count depends on the
   model data, so the marker voxels of each `.cub` used (house modules, class houses,
   tower, landing place) must be counted from the extracted models.
7. **Creature and item fields** set here (kinds 140, 141, 142, 143, the +0x28 values 1, 3,
   6, +0x30, +0x58, +0x7a, +0xf58, +0x10e8, the equipment slot offsets and categories of
   the NPC marker, the item categories of §1.10.2 and §1.10.6) belong to the creature and
   item specs; the names given for the equipment slots are *inferred*.
8. **Server.** The server's zone generator (**server** 0x518630) is assumed to call the same
   builders; a comparison of the server's camp and boulder code would confirm that clients
   and server stay in step.
9. **Unstable sort ties** in §5.4: the insertion sort is stable, but a list longer than 32
   (impossible here, g² ≤ 25) would switch to the unstable introsort.
10. **Model-dependent draws of the town.** Every house module, carpet, roof half and class
    house runs the placer with markers (§6.2); a zone's exact draw count needs the marker
    voxel list of each model in the order the placer visits them (item 6). The modules of a
    set use mode 1, so only markers 3, 5, 6, 10, 14 and 15 draw.
11. **Order-sensitive helpers to confirm with a `rand` hook**: the unused draws of V3/V2
    (one per candidate and five before subtype 8 of V3), the unused draw before a
    resident's last home visit (§5.5.9), and the −y wall piece of §5.5.4 step 15 having no
    gate draw. All were read from the assembly; a hook on one shop zone would confirm them.

---

## 9. Checking against the running game

1. Hook MSVCR110 `rand` (caves spec §11) and log return addresses during one zone of each
   kind: a camp zone (sites 0x5e8ddb, 0x5e8e16 and 0x5ad405, 0x5ad633, 0x5ad6fe), a boulder
   zone (0x5e8fb5, 0x5e9056..0x5e90d3), a type-10 zone (0x5e9783, 0x5e9884, 0x5e989f,
   0x5e9dd5); the counts must match §2–§4 exactly.
2. Dump the zone's static-object vector (zone + 0x0c, 0x188-byte records) after generation:
   campfire, ring objects and runestone positions, kinds, scales and rotations must match.
3. Dungeon: hook `rand` during one kind-3 zone of each v and compare the count between the
   end of §1.9 and the zone's next pass with §1.11 (every box filler call is 3 + its volume;
   a staircase 693; a shell 2,747). Dump zone + 0x0c (windows kind 4, traps kind 7, wall
   furniture, chests), zone + 0x04 (torches, lianas, cobwebs, chandeliers), zone + 0x18
   (kinds 143, K1/K2 kinds) and zone + 0x30 (table loot).
4. Placer: place one house module in a test zone with a hook on `rand` and on 0x5a0e00 and
   0x4862e0; each marker voxel must produce the object and the draws of §6.2.
5. Visual: camps appear only on rough ground (R > 0.02) in every other zone, with a fire and
   up to four tents, tables, stools or mats; type-10 sites are stone circles of 6–8 pillars
   of radius 25 with a runestone; towns of style 0..5 use framework, wood, whitewood, stone,
   clay or whiteclay houses; type-5 locations are antique, desert or jungle ruins.
