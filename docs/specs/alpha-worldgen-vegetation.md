# Alpha world generation: vegetation, plants and ground items

Behavioural specification of the living things the Cube World **Alpha** zone generator
places on its terrain: **plant sprites** (grass, flowers, reeds, water lilies, underwater
plants — small voxel models from a fixed kind table), **ground items** (collectable
category-11 items lying on grass and snow), **street lights** in towns, and **voxel trees**
written into the terrain. Written from `Cube.exe` (SHA-256 `84a7a132…30bf`), mainly the zone
generator 0x5e4850 and the start-up model tables. Read the [terrain spec](alpha-worldgen-terrain.md)
first: this pass runs on the finished columns and their block types.

Status: the plant-sprite pass (§3) is specified to the draw; the tree pass (§5) is specified
to the decisions but the tree **builder** is a follow-up; ground items (§4) need the item
spec for their meaning.

---

## 0. Conventions

As in the terrain spec §0. "draw" is one `rand()` call (random spec §1); every draw listed
is made in the order listed, and only when its branch is reached. Rotations are in degrees
about +Z. Scales are model scale factors (a model voxel times the scale is its size in
blocks; open question 5).

---

## 1. Model tables (start-up, 0x459c40 at 0x462563..0x4636xx; registry 0x4ee230)

The model registry assigns each `.cub` model an index. Two kind tables map generator
kinds to registry indices.

### 1.1 Plant sprite kinds (table A)

| Kind | Model | Kind | Model |
|---:|---|---:|---|
| 0 | flowers2.cub | 27 | desert-flower01.cub |
| 1 | flowers.cub | 28 | desert-flower02.cub |
| 2 | grass.cub | 29 | wheat.cub |
| 3 | grass2.cub | 30 | corn.cub |
| 4 | grass3.cub | 31 | water-lily01.cub |
| 5 | underwater-plant.cub | 32 | water-lily02.cub |
| 6 | coral.cub | 33–40 | shop and inn signs |
| 7 | alga.cub | 41 | ivy.cub |
| 8 | lava-flower.cub | 42, 43 | wall-roses-red / -white.cub |
| 9 | lava-grass.cub | 44–47 | inca-art1..4.cub |
| 10 | thorn-plant.cub | 48 | crest1.cub |
| 11 | echinacea2.cub | 49 | wall-skull.cub |
| 12 | leaf.cub | 50 | castle-chain.cub |
| 13 | lantern02.cub | 51–54 | torch-read/-green/-blue/-yellow.cub |
| 14 | torch.cub | 55 | liana.cub |
| 15, 16 | stone.cub, stone2.cub | 56 | chandelier.cub |
| 17 | christmas-tree.cub | 57, 58 | cobwebs.cub, cobwebs2.cub |
| 19 | tendril.cub | 20 | tulips-colorful.cub |
| 21 | cornflower.cub | 22 | reed.cub |
| 23 | pumpkin-leaves.cub | 24 | pineapple-leaves.cub |
| 25 | sunflower.cub | 26 | bean-tendril.cub |

(Kind 18 has no entry in the table.)

### 1.2 Static object kinds (table B, excerpt relevant here)

50 = street-light02.cub, 51 = street-light01.cub, 65 = campfire.cub (the rest of table B —
doors, furniture, fences, workbenches — belongs to structures). Kinds 50 and 51 are the
light sources of alpha-rendering.md §10.1.

### 1.3 Records

* **Plant sprite** (built by 0x58e9f0, appended to the zone's plant list by 0x4862e0):
  position (x, y, z) as fixed point (block coordinates, the plant stands at the block
  corner (x, y) on top of the block below z), scale vector, rotation, kind, a flags word
  (bit 2, value 4, set for the kinds noted below; meaning open question 1).
* **Static object** (0x58eaa0 record, inserted with 0x5a0e00): kind, position, scale
  vector, rotation index (0..3, quarter turns).
* **Ground item** (0x148-byte record built by 0x44a970, appended with 0x44d590): item
  category byte, subtype byte, rotation, scale, a count and a flag (§4).

---

## 2. The pass and its order

After the terrain passes and the structure passes (overview spec §5), the zone generator
walks all columns, x in the outer loop and y inner (0x5ebc60..0x5ec5e8), and for each
column walks its stored blocks from the bottom (index i = 0 .. count − 1):

* If i < count − 1 and block i + 1 is solid (type neither 0 nor 2), skip this i.
* Otherwise let **here** = type of block i, **above** = type of block i + 1 (for the last
  stored block, the implicit block above, terrain spec §1.4), and **zt** = bottom + i + 1,
  the z of the cell above block i.
* Cases (§3, §4, §6) are decided on (here, above).

So every exposed top surface in the column (including ravine floors, overhang tops and
lake beds) is considered, bottom to top.

---

## 3. Plant sprites

### 3.1 Street lights (here = 11, above = 0)

If q(x, y) > 0.75 (town-ness, terrain spec §3.1) and (90y + x) mod 470 = 0: draw; if
draw & 15 = 0 and the 7 cells z = zt … zt + 6 are all non-solid: place a static object of
kind **51** (street-light01) if warmth(x, y) > 0.8, else kind **50** (street-light02),
scale (1, 1, 8) for 51 and (2, 2, 8) for 50, at (x, y, zt), rotation index = draw & 3.

### 3.2 Reeds (here = 3 lake bed, above = 0)

If warmth > 0.2 and |N(f32(0.05x + 9843), f32(0.05y + 8437))| > 0.5 (arguments computed in
**float**, then widened), draw; if draw & 7 = 0: plant kind **22** (reed) at (x, y, zt),
scale 0.09 (bits 0x3db851ec), rotation = draw · 360 / 32767 (float).

### 3.3 Water lilies (here = 2 water, above = 0)

If warmth > 0.2, zt > 0 and |N(f32(0.05x + 24234), f32(0.05y + 53565))| > 0.7: draw; if
draw mod 10 = 0: plant at (x, y, zt adjusted by 0x42f5a0, open question 2), scale 0.09,
rotation = draw · 360 / 32767, kind = **31 + (draw & 1)**.

### 3.4 Ground cover (here ∈ {4, 9, 10, 12}, above = 0 or water)

Gate: |N(f32(0.05x + 9843), f32(0.05y + 8437))| > 0.6, then draw; continue only if
draw & 7 = 0. Then:

* **Underwater** (zt ≤ −5): plant at (x, y, zt), rotation = (draw & 3)·90, scale 0.1;
  nz = N(0.01x + 9843, 0.01y + 8437) (double arguments). If nz ≤ 0: kind **7** (alga),
  flag 4. Else draw: kind = **5 + (draw & 1)**; kind 5 (underwater-plant) gets scale 0.075
  and flag 4; kind 6 (coral) keeps 0.1. Finally scale ← scale·(draw / 32767 + 1) (float).
  Columns with −5 < zt < 1 get nothing.
* **Fire ground** (here = 12, zt ≥ 1): plant, rotation = (draw & 3)·90, scale 0.075;
  kind = **9** (lava-grass, flag 4) if N(0.01x + 9843, 0.01y + 8437) > 0, else **10**
  (thorn-plant) with scale = f32(draw · 0.02 / 32767 + 0.03) (double).
* **Snow** (here = 10, zt ≥ 1): no plant; go to the ground-item step (§4).
* **Grass and sand** (here ∈ {4, 9}, zt ≥ 1), by the column's warmth wm and moisture mo:
  * **Hot** (wm > 0.75):
    * mo ≤ 0.25: draw; if draw mod 100 = 0: plant, rotation = (draw & 3)·90, scale 0.075,
      kind = **28 − (1 if (draw & 1) ≠ 0 else 0)** (desert flowers 27/28); placed.
    * mo > 0.25: plant, rotation = draw · 360 / 32767, scale 0.075. If
      N(0.01x + 9843, 0.01y + 8437) > 0: kind = **4** if N(f32(0.01x + 34234),
      f32(0.01y + 234234)) > 0 else **3**; otherwise kind = **12** if
      N(0.01x + 34234, 0.01y + 234234) ≤ 0.5 else **11**. Kinds 2, 3 get flag 4; kind 11
      flag 4 and scale = draw · 0.05 / 32767 + 0.05; kind 12 flag 4 and scale =
      draw · 0.02 / 32767 + 0.1 (float). Placed if here = 4, or kind ∈ {2, 3}.
  * **Temperate** (wm ≤ 0.75): plant, rotation = (draw & 3)·90, scale 0.075. With
    nA = N(0.01x + 9843, 0.01y + 8437):
    * mo > 0.5: nA > 0 → kind **2** if N(f32(0.01x + 34234), f32(0.01y + 234234)) ≤ 0, else
      **3**; nA ≤ 0 → kind **1** if N(0.01x + 34234, 0.01y + 234234) ≤ 0, else **0**.
    * mo ≤ 0.5: nA > 0 → kind **12** if the float-argument noise > 0, else **4**; nA ≤ 0 →
      kind 1 or 0 as above.
    * Kinds 2, 3, 4, 12 get flag 4; kind 12 scale = draw · 0.02 / 32767 + 0.1.
      Placed if here = 4, or kind ∈ {2, 3, 4}.

The two "N(0.01x + 34234, …)" forms differ: one computes its arguments in float (written
f32), the other in double; this changes the value at large x, y and must be kept.

---

## 4. Ground items (after §3.4 for here = 4 or 10)

Only columns that passed the §3.4 gate (noise test and draw & 7 = 0) with zt ≥ 1 reach
this step; for here = 4 it follows the plant decision whether or not a plant was placed.

* here = 4: draw; if draw mod 150 = 0: item record with category **11**, rotation =
  (draw / 32767)·360, then draw & 1: 0 → subtype **15**; 1 → subtype **22** with scale 0.1;
  count 1, flag byte 2; appended to the zone's item list.
* here = 10: draw; if draw mod 300 = 0: category 11, rotation draw, then a draw whose value
  always selects subtype **24** with scale 0.1; count 1, flag 2.

Category 11 items with these subtypes are collectables lying in the world (an item of
category 11 subtype 19 glows, alpha-rendering.md §10.1). Their names come from the item
tables (follow-up "items" spec).

---

## 5. Voxel trees (grid pass, 0x5e9e7d..0x5ea7c6)

Trees are not models: a builder (0x5df960, 20099 bytes, follow-up spec) writes trunk and
crown voxels into the zone. The placement runs on a 14 × 14 grid of 18-block cells, before
the plant pass. For i = 0..13 (outer, x) and j = 0..13 (inner, y):

1. Base gx = 256zx + 18i, gy = 256zy + 8 + 18j; sample point (gx + 8, gy), that is
   (256zx + 18i + 8, 256zy + 18j + 8) (0x5e9ef7, 0x5e9f5b..0x5e9f69: moisture is read at
   (outer base + 8, inner value)).
2. Oasis bonus b = bump(f(Lz, gx + 8, gy)) if the zone's location Lz has type 3, else 0.
3. mo = moisture(gx + 8, gy).
4. draw: crown size s = trunc((draw mod 5) + 2·mo + 6 + 4b) (float sum).
5. draw: height t = trunc((draw · 8 / 32767 + 6b + 8)·(0.5·mo + 1)) (float).
6. Position x = gx + s, y = gy − 8 + s, that is **x = 256zx + 18i + s, y = 256zy + 18j + s**
   (both axes get the same +s; the +8 of gy belongs only to the sample point of steps 1–3).
   Skip the cell if the position is within 20 blocks of the home point
   ((x − 8396928)² + (y − 8396928)² < 400, float).

   **Correction:** (0x5ea030..0x5ea04f, checked in the assembly) the y position is the inner
   loop's sample y (which already holds 256zy + 8 + 18j) plus −8 plus s: `ADD ECX,-0x8;
   ADD ECX,EDX(s)`. The earlier text read the constant as −2, which put every grid tree
   **6 blocks too far towards +y**. The x side (0x5ea06f..0x5ea075: outer base 256zx + 18i
   plus s, with no +8) was already right. Loop roles were re-checked too: the outer counter
   (0x5ea7ce, base 256zx = zx shl 8 from 0x5e49ee) is x, the inner counter (0x5ea7b3, base
   256zy + 8 from 0x5e49da / 0x5e9e89) is y, and both step by 18.
7. Clamp x ≤ 256zx + 256 − s/2 and y ≤ 256zy + 256 − s/2 (integer s/2).
8. Skip if any object already registered for this zone by the structure passes lies within
   40 blocks (squared distance < 1600).
9. **Tree kind** k: n = N(O294 + 0.001x, O298 + 0.001y); draw: k = 5 if (draw & 1) ≠ 0
   else 0. If n ≤ 0.3: draw; if draw mod 10 = 0, k = 1. Else: draw; if draw mod 10 ≠ 0,
   k = 1. Then draw; if draw mod 10 = 0, k = 2.
10. wm = warmth(x, y). If wm > 0.8 and mo > 0.7: draw: k = 4 + ((draw & 1) ≠ 0); draw: if
    draw & 3 = 0, k = 3.
11. Size rules by warmth:
    * 0.3 ≤ wm < 0.7: n2 = N(0.001x + 8473, 0.001y + 9438); if n2 > 0.8: draw; if
      draw mod 5 ≠ 0, k = 2.
    * wm < 0.3: if wm > 0.2: n2 as above; if n2 > 0.6: draw; if draw mod 5 ≠ 0, k = 2.
      Otherwise k = 1.
    * k = 2: t ← t + draw mod (t / 2). k = 1: s ← min(s, trunc(0.5t)), s ≥ 1.
12. Keep the crown inside the zone: if x + s > 256zx + 256, x = 256zx + 256 − s; same for y.
13. **Density** p = min(sat(1.2·N(O1f4 + 0.008x, O1f8 + 0.008y)) + 0.2·mo² + 0.02, 1) + b
    (0x5a5bc0 plus the bonus). draw; continue if draw / 32767 ≤ p.
14. Ground: from the column top − 1 downwards to the first block that is neither air nor
    water; if that z ≥ 0, the tree stands at z + 1 if the ground type is 4 (grass), 10
    (snow), or 9 (sand) with z + 1 < 3. Build tree (x, y, z + 1) with size s (or a default
    when s < 1), height t and kind k (exact argument list: open question 3).

The draws in this pass precede all plant-sprite draws of §3 in the zone's stream.

---

## 6. Other placements in the same zone

* **Location type 12**: when the location's centre lies in this zone, one discarded draw,
  then a **giant tree** (builder 0x5df960 with size 80, height 80, kind 6) at the zone
  centre (256zx + 128, 256zy + 128) on the column top (0x5e7231). This happens in the
  structure passes, before the tree grid.
* **Ravine spawns** (terrain spec §5.5) add item records and creatures inside ravines.
* Lakes get reeds through §3.2 because their beds are type 3.

---

## 7. Checking against the running game

* Dump a freshly loaded zone's plant list (zone+0x24) and item list; compare kinds,
  positions and rotations with §3–§4 for a reimplemented zone (needs the same terrain).
* The plant density can be checked visually without exact terrain: grass patches follow
  the |N(0.05x + 9843, 0.05y + 8437)| > 0.6 pattern (patchy clumps about 20 blocks wide), and
  only one column in eight inside those patches carries a plant.
* Lilies appear only on raised lakes (water blocks above z = 0), never on the sea.

---

## 8. Open questions

1. Meaning of plant flag 4 (perhaps "sway"/"no collision") and how the renderer uses
   the scale field of plant sprites.
2. 0x42f5a0 applied to the lily height (probably "+ 1 − a small offset" so the lily floats
   on the surface).
3. Tree builder 0x5df960: argument list (x, y, z, size, height, kind, zone), its four
   initial draws, per-kind shapes (kind 3 sets colours (240, 180, 120)-like bark and leaf
   values, kind 5 scales the crown by 0.7), voxel colours. Follow-up spec.
   See also [alpha-worldgen-trees.md](alpha-worldgen-trees.md), which specifies the builder (§1.2 there notes a kind-3 override in step 14).
4. Whether the ground-item subtypes 15/22/24 are herbs, mushrooms or similar.
5. Model scale convention (the .cub voxel size × scale) — shared with the clouds spec.
6. The "objects already registered" list consulted by the tree pass is filled by the
   structure passes (houses 0x5cbe00, 0x5cb600); its exact contents need the structures
   spec.

See also: question 6 — 0x5cbe00/0x5cb600 are rock-formation builders, and the object list is filled by the camp and type-10 passes, see [alpha-worldgen-caves.md](alpha-worldgen-caves.md) §3.1, §8.
