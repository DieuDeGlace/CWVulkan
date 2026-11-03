# Alpha world generation: region preparation and location records

Behavioural specification of how Cube World **Alpha** fills a region object when it is
first needed: the region header, the 8 × 8 **location records** that the terrain consults
(towns, lakes, valleys, dungeon entrances, giant trees …), and the 64 × 64 **per-zone table**
that the zone generator copies into every zone. Written from `Cube.exe`
(SHA-256 `84a7a132…30bf`): region preparation 0x5da280 (9088 bytes; server counterpart
**server** 0x50e080, same size, not compared instruction by instruction) and its helpers
0x5a4710, 0x5c3ac0, 0x5c3bb0, 0x5ee4b0, 0x5f4d10, 0x5c2270 / 0x5c24f0 and the type-10 helper
0x5c3ef0. Every statement was checked against the assembly; the pseudocode was only a
reading aid.

Read first: [random](alpha-worldgen-random.md) (rand, A, N), [regions](alpha-worldgen-regions.md)
(cells, owner, blends, §6–§7 location lookup and f(L, x, y)) and
[terrain](alpha-worldgen-terrain.md) (H). This document replaces regions spec §6.4 and
answers regions spec open questions 1–3 and terrain spec open question 4 (§11).

---

## 0. Conventions

* As in the random spec §0 (f32, trunc, float vs double, floor_div, C remainder).
* **fx** = 64-bit fixed point, 1/65536 block. **trunc64(v)** converts a float to a 64-bit
  integer **towards zero** (the CRT routine 0x68d946 used here truncates; this answers
  regions spec open question 3). **blk(X)** = the 64-bit fx value X divided by 65536 with the
  C rule (towards zero); all positions here are positive, so it is floor(X / 65536).
* **draw** = one `rand()` call. Draws are listed in the order they are made and only when
  their branch is reached. "draw & 3", "draw % 10" as in the random spec §1.2.
* (rx, ry): the region being prepared. **C** = its own region cell (rx, ry) (regions spec §2).
  **Slot (i, j)**: location sub-cell i = 0..7 along x, j = 0..7 along y; its origin is
  (ox, oy) = (16384·rx + 2048·i, 16384·ry + 2048·j); record index 8·i + j.
* **owner(x, y)** = the region cell returned by 0x477e10 for block column (x, y) (regions
  spec §3.4). "owned by C" compares cell identity.
* **H(x, y)** = base height of the terrain spec §4, with **no** location visible (§2.2).
* **Named world**: the world's name string (world + 0x94, length at world + 0xa4) is not
  empty. In normal play (single player save, or a client that received the server's world
  name) the world is named. All test vectors assume a named world.

---

## 1. The region object (0x15a28 bytes; constructor 0x5c3ac0)

| Offset | Size | Meaning | Initial value |
|---|---|---|---|
| +0x00 | 4 | (type information) | — |
| +0x08 | 1 | flag | 1 |
| +0x0c | int32 | **tier**: distance class from the home region (§3) | 1 |
| +0x10 | int32 | **h10**: a 0..4 draw for far regions (§3; use not traced) | 0 |
| +0x14 | int32 | **style**: 0..3, or 4..5 in hot regions (§3); copied to the town | 0 |
| +0x18 | 64 × 64 × 16 | **per-zone table** (§1.1) | see §1.1 |
| +0x10018 | 64 × 64 × 4 | zone pointers (terrain spec §1.1) | null |
| +0x14018 | 64 × 0x68 | **location records** (§1.2), record 8·i + j | see §1.2 |
| +0x15a18 | 1 | "stored mission data loaded" flag (§9) | 0 |
| +0x15a1c | 4 | pointer to an auxiliary container (0x46d4b0) | — |
| +0x15a20 | 4 | — | 0 |

### 1.1 Per-zone table entry (16 bytes, at region + 0x18 + 16·(64·zxl + zyl))

zxl = zx mod 64 and zyl = zy mod 64 are the zone's indices inside its region. The zone
generator copies the whole entry into the zone at +0x78..+0x87 (terrain spec §5.7).

| Offset | Size | Meaning | Initial |
|---|---|---|---|
| +0 | byte | **kind**: 0 none, 1 town quarter, 3 dungeon entrance, 4 type-10 site (5 is tested by the zone generator but never written, see caves spec §6) | 0 |
| +1 | byte | **variant**: town quarter 1..4; dungeon style 0..5 | 0 |
| +2, +3 | — | unused here | 0 |
| +4 | int32 | **id** (dungeon or type-10 site id) | 0 |
| +8 | int32 | **level** | 1 |
| +0xc | byte | **sub-level** (dungeons) | 0 |
| +0xd..+0xf | — | unused here | 0 |

### 1.2 Location record (0x68 bytes; constructor 0x5c3bb0)

Every field starts at 0 except **level (+0x24) = 1**.

| Offset | Type | Meaning |
|---|---|---|
| +0x00 | int64 | centre x, fx |
| +0x08 | int64 | centre y, fx |
| +0x10 | float | radius r, blocks |
| +0x14 | float | ground height h |
| +0x18 | int32 | type (0 = none) |
| +0x1c | int32 | **variant** (type 1: region style; type 3: 0..2; type 5: climate class; type 14: dungeon variant) |
| +0x20 | int32 | **serial** (a running number per region) or **id** (type 1: the cell id; type 10: random id) |
| +0x24 | int32 | **level** |
| +0x28 | int32 | **sub-level** (types placed by the generic switch, §6.3) |
| +0x2c..+0x53 | — | mission data, only filled from stored data (§9) |
| +0x54..+0x67 | — | monster data, only filled from stored data (§9) |

---

## 2. Order of region preparation

### 2.1 Steps (0x5da280)

Called for region (rx, ry) with 0 ≤ rx, ry < 1024. If world + 0xbc already holds the
region, nothing happens (the existing region is returned). Otherwise:

1. Create region cells (rx + i, ry + j) for i = −2..2 (outer), j = −2..2 (inner).
2. `srand(A + 1024·ry + rx)`.
3. Construct the region object (§1).
4. Header and level range (§3).
5. Phase A: scan the 64 slots, place candidates, build the candidate list (§4).
6. Phase B: sort the list (§5).
7. Phase C: assign types to the 32 best candidates (§6).
8. Phase D: up to five type-10 sites from the rest (§7).
9. Phase E: copy record levels into the per-zone table (§8).
10. Phase F: height samples whose results are thrown away (§8.1).
11. Phase G: choose the four town quarters (§8.2).
12. Phase H: read stored mission and monster data (§9; single player only, no draws).
13. **Store the region in world + 0xbc** (instruction 0x5dc628, the last thing done).

### 2.2 When the region becomes visible (answers regions spec open question 2)

The region pointer is written to world + 0xbc only at step 13, after every height,
owner and climate evaluation of the preparation. While a region is prepared, location
lookups (regions spec §6.3) that fall into it return "no region", so:

* every H(x, y) computed during preparation is the **location-free** height (no town
  flattening, no island rule, no basins, no spires);
* the moisture blend's oasis rule (regions spec §4) never applies during preparation;
* all positions sampled during preparation lie inside the region being prepared, so the
  locations of already-prepared neighbouring regions are never consulted either.

Region cells needed by these evaluations (the 3 × 3 window around any column of the
region) always exist, because step 1 creates the 5 × 5 block around the region.

---

## 3. Header and level range

1. **tier** (0x5a4710): 1 if (rx, ry) = (512, 512); otherwise, with dx = 512 − rx and
   dy = 512 − ry converted to float, s = f32(f32(dy²) + f32(dx²)),
   d = f32(√s) (double square root of s, rounded to float), and
   tier = 2 − trunc(f32(d · (−0.75))). So tier = 2 + trunc(0.75·d): 2 for direct
   neighbours, 3 at d = √2 or 2, 5 at d = 4 …
2. **h10** = 0; if tier > 4: draw, h10 = draw % 5.
3. draw: **style** = draw & 3.
4. draw: **serial** = draw % 10000 (the running serial number, §6).
5. If C's warmth (cell + 0x10) > 0.81 (float constant 0x71dd18 = 0.8100000023841858):
   draw: style = 4 + (draw & 1).
6. **Level range** of C (0x5ee4b0), evaluated in this order, later rules overriding:
   start (min, max) = (1, 10); warmth < 0.2 → (10, 20); moisture < 0.2 and warmth > 0.8 →
   (15, 25); moisture > 0.8 and warmth > 0.8 → (10, 20); fire flag = 1 → (20, 30).
   (0.2 and 0.8 are the float constants 0.20000000298023224 and 0.800000011920929; all
   comparisons are strict.) Only **minLevel** is used (§4.2); the maximum is unused.

Draws so far: 2 or 3 (+1 when tier > 4).

---

## 4. Phase A: placing candidates in the 64 slots

Let (cxs, cys) = ((C.x >> 11) & 7, (C.y >> 11) & 7), the slot that contains C's point
(cell points are always slot centres, regions spec §2.2 step 7).

For i = 0..7 (outer), j = 0..7 (inner), record R = record 8·i + j:

### 4.1 The cell slot of a named world: the town (type 1)

If the world is named and (i, j) = (cxs, cys):

1. R.type = 1; R.variant (+0x1c) = style; R.id (+0x20) = C's cell id (cell + 0x14).
2. draw: R.r = float(draw % 200 + 512) (512..711).
3. R.centre = (C.x << 16, C.y << 16).
4. Clamps, in this order (each compares 64-bit fx values; with r ≤ 711 and the cell point at
   the slot centre none of them can trigger, but they are part of the routine):
   if R.x − trunc64(r·65536) − 2²⁴ < ox << 16: R.x = trunc64(f32(f32(f32(ox) + r) + 256)·65536);
   the same for y with oy; if R.x + trunc64(r·65536) + 2²⁴ > (ox + 2048) << 16:
   R.x = trunc64(f32(f32(f32(ox + 2048) − r) − 256)·65536); the same for y.
5. R.h = H(blk(R.x), blk(R.y)); if R.h < 0 then R.h = 0.
6. Nothing else: the town is **not** put on the candidate list, keeps level 1 and
   sub-level 0.

(One draw.) If the world is not named, the cell slot is treated like any other slot.

### 4.2 Every other slot

1. If owner(ox + 1024, oy + 1024) is not C: **skip the slot** (no draws; the record keeps all
   its initial values, including centre (0, 0)).
2. draw: R.r = float((draw & 255) + 512) (512..767).
3. **Placement** with radius r (the same procedure is reused in §6.3 case 3). With
   r256 = f32(r + 256) and span = f32(2048 − f32(r256·2)) (2..512):
   * draw: R.y = (oy << 16) + trunc64(f32(r·65536)) + 2²⁴
     + trunc64(f32(f32(f32(draw·span) / 32767)·65536));
   * draw: R.x = (ox << 16) + trunc64(f32(r256·65536))
     + trunc64(f32(f32(f32(draw·span) / 32767)·65536)).
   The **y draw comes first.** (r·65536 + 2²⁴ equals r256·65536, so both axes place the
   centre at origin + r + 256 + u·span with u = draw/32767 in float: the disc plus a
   256-block margin lies inside the slot.)
4. R.h = H(blk(R.x), blk(R.y)) (not clamped here). R.level (+0x24) = minLevel.
5. If owner(blk(R.x), blk(R.y)) is not C: stop here for this slot (the record keeps its
   centre, radius, h and level, but its type stays 0 and it is not a candidate).
6. **Candidate key**: dx = f32(cxs − i), dy = f32(cys − j), d = f32(√f32(f32(dy²) + f32(dx²)))
   (double square root, rounded); draw: key = f32(f32(f32(f32(draw·4) / 32767) + d) − 2).
   Append the entry (key, i, j) to the candidate list (0x5f4d10 is a plain vector append).

Per slot: 0 draws (skipped), 3 (centre not owned), or 4 (candidate). The key is the slot's
distance in slots from the cell's slot plus a random 0..4, minus 2.

---

## 5. Phase B: sorting the candidates

The list is sorted by key, **ascending** (0x5c2270, the C++ library sort with the
comparison "key less than key"; the slot numbers do not take part). With all keys distinct
the result is simply ascending order. Keys can only tie when both the slot distance and the
draw coincide; see open question 1 for what happens then.

At most 63 slots can be candidates, so the list has 0..63 entries.

---

## 6. Phase C: ranked assignment

A counter c runs from 0 to 63. At each step, if the list is empty the phase ends; otherwise
if c is **odd** nothing happens; if c is even the **first** entry (smallest key) is removed
and assigned. So at most 32 candidates are assigned, with ranks c = 0, 2, 4, … 62.

For the removed entry (key, i, j), record R = record 8·i + j:

1. If R.h < 0: R.h = 0.
2. **Level** (0x4462f0 then truncation): t = f32(c · 0.015625) (exact c/64),
   level = trunc(f32(f32(f32(f32(1 / f32(1 − t)) − 1)·20) + 1)), all float. R.level = level.
   Values for c = 0, 2, 4 … 62: 1, 1, 2, 3, 3, 4, 5, 6, 7, 8, 10, 11, 13, 14, 16, 18, 21, 23,
   26, 30, 34, 39, 45, 52, 61, 72, 87, 109, 141, 194, 301, 621.
3. If (c / 2) is odd (c = 2, 6, 10, …): **dungeon entrance**, §6.1. Otherwise (c = 0, 4,
   8, …): the **generic switch**, §6.2.

### 6.1 Type 14 (dungeon entrance; 16 of the 32 ranks)

1. R.type = 14; R.r = 150.
2. Snap the centre to the centre of its zone: for each axis, X = blk(R.x),
   X′ = floor(X / 256)·256 + 128, R.x = X′ << 16 (same for y). h is **not** recomputed.
3. Zone entry E for zone (zxl, zyl) = ((X′ >> 8) & 63, (Y′ >> 8) & 63):
   E.kind = 3; draw: E.variant = draw & 3.
4. Climate at the zone centre (zx_c, zy_c) = ((64·rx + zxl)·256 + 128, (64·ry + zyl)·256 + 128)
   (= the snapped centre): if warmth(zx_c, zy_c) > 0.8 (regions spec §4 blend) then
   E.variant = 4 + (1 if moisture(zx_c, zy_c) ≤ 0.8 else 0). (No draw.)
5. draw: E.id = draw % 10000000 + 1 (= draw + 1).
6. E.level = R.level.
7. R.serial (+0x20) = serial; draw: serial ← serial + 1 + draw % 50.
8. R.variant (+0x1c) = E.variant.
9. **Sub-level rule** (shared with §6.2) into E.sub-level (+0xc): level < 5 → 0, no draw;
   level < 10 → draw & 1; level < 15 → draw % 3; level < 19 → draw % 3 + 1; otherwise
   (draw & 3) + 1.

Draws: 3 + (1 if level ≥ 5).

### 6.2 Generic switch (16 of the 32 ranks)

draw: s = draw & 7. "Sea" below means C's elevation (cell + 0x18) is negative.

| s | Type | Extra work, in order |
|---|---|---|
| 0 | 2 | — |
| 1 | 3 | draw: R.variant = draw % 3 |
| 2 | 4, or **15** if sea | — |
| 3 | 5 | R.variant from C's climate (below); draw: R.r = float((draw & 255) + 256) (256..511); then the **placement** of §4.2 step 3 again in the record's own slot (y draw, then x draw) with the new r. h is **not** recomputed (it stays the height at the old centre). |
| 4 | 6, or 15 if sea | — |
| 5 | 7, or 15 if sea | — |
| 6 | 11 | R.r = 128; snap the centre to its zone centre as in §6.1 step 2 |
| 7 | 12 | R.r = 128; snap the centre to its zone centre |

Type-5 variant: if C.warmth > 0.8: 3 if C.moisture > 0.8, 2 if C.moisture < 0.2, else 0;
if C.warmth ≤ 0.8: 0.

Then, for every s:

1. Sub-level rule of §6.1 step 9 into **R.sub-level (+0x28)**.
2. R.serial (+0x20) = serial; draw: serial ← serial + 1 + draw % 50.

Draws: 1 (switch) + (1 for s = 1, 3 for s = 3) + (1 if level ≥ 5) + 1.

### 6.3 What the types mean (from their uses)

| Type | Made by | Behaviour it drives | Probable role |
|---|---|---|---|
| 1 | §4.1, one per region (named world) | town flattening and streets (terrain §3); town quarters (§8.2); town builder 0x5af170 (caves spec §8) | town |
| 2 | s = 0 | flatness and roughness bumps, mountain-mask override | hill / plateau |
| 3 | s = 1 | moisture boost in dry climates; tree bonus | oasis |
| 4 / 15 | s = 2 | lake basin and fill; 15 in sea regions | lake (15: sea feature, island rule only) |
| 5 | s = 3 | mountain-mask hole; town builder 0x5af170 runs in its zones too (caves spec §8) | village / camp site (open) |
| 6, 7 / 15 | s = 4, 5 | valley; type 6 also gets the 3 × 3 rock-mound grid (caves spec §3.2) | valley |
| 10 | §7 | round mountain-mask hole (§11.1); static objects in its zone (caves spec §8) | clearing / site (open) |
| 11 | s = 6 | a 100 × 100 rock mound at its zone centre (caves spec §4.1) | great rock |
| 12 | s = 7 | a giant voxel tree at its zone centre (vegetation spec §6) | giant tree |
| 13 | **never created** by 0x5da280 | spire (terrain §4.3) | unused in fresh worlds (open question 3) |
| 14 | §6.1 | dungeon builder in its zone (zone kind 3) | dungeon entrance |

---

## 7. Phase D: type-10 sites

After phase C, the candidates still on the list (ranks were only taken from the front, so
these are the ones with the largest keys) get up to five type-10 sites. Counter n = 0;
while the list is not empty:

1. draw: k = draw mod (list length) (unsigned remainder); remove entry k (the order of the
   remaining entries is kept).
2. R.type = 10; draw: R.id (+0x20) = draw % 10000000 + 1.
3. Zone entry for the zone of R's centre ((blk(R.x) >> 8) & 63, (blk(R.y) >> 8) & 63):
   kind = 4, id = R.id.
4. n ← n + 1; stop when n = 5.

R keeps the radius (512..767), the unclamped h and level = minLevel from phase A. A list
of ≤ 32 candidates is used up in phase C and gives no type-10 site.

---

## 8. Phase E: levels in the per-zone table

For each record in index order 0..63, the zone entry of the zone that contains its centre
((blk(x) >> 8) & 63, (blk(y) >> 8) & 63) gets **level = record level**. Later records
overwrite earlier ones. Records that were never placed have centre (0, 0) and level 1 and so
write level 1 into the region's local zone (0, 0). This is the only place where the table's
level of a type-0 or type-2..13 zone is set; dungeon entries were already given their level
in §6.1, and phase E writes the same value again.

### 8.1 Phase F: discarded height samples

For zone (a, b), a = 0..63 (outer), b = 0..63 (inner), the routine evaluates H at the four
zone corners (16384·rx + 256a, 16384·ry + 256b), (+256, +0), (+0, +256), (+256, +256) and
throws the results away; zones with (64·rx + a + 5·(64·ry + b)) mod 13 = 0 are collected
in a list that is never used. H draws nothing and changes nothing, so a reimplementation
**skips this phase** (it costs 16384 height evaluations in the original).

### 8.2 Phase G: town quarters

For every record of type 1 (at most one, the town of §4.1), in index order:

1. For a = 0..7 (outer), b = 0..7 (inner): the zone (8i + a, 8j + b) of the town's slot,
   centre (X, Y) = (16384·rx + 2048·i + 256·a + 128, 16384·ry + 2048·j + 256·b + 128);
   f = f(L, X << 16, Y << 16) (regions spec §7, distorted, k = 256); v = 1 − f;
   w = v² if v > 0 else 0. Append (w, 8i + a, 8j + b).
2. Sort the 64 entries by w, **descending** (0x5c24f0).
3. The first four entries' zones get kind = 1 and variant = 1, 2, 3, 4 in that order.

(The radius is at least 512, so at least four zones always have w > 0; see open question 1
for exact ties.) These four zones are the town's quarters; kind 1 also makes the zone
generator skip its rock and boulder passes (caves spec §4.3, §8).

---

## 9. Phase H: stored mission and monster data (single player)

If the world flag at world + 0xb4 is 0 (set from the world constructor's second argument,
0x58eb00) and the world is named, then for every slot the routine opens two stored records
named from "mission" or "monster" and the sub-cell coordinates (lx = 8·rx + i, ly = 8·ry + j)
and, when present, reads them into the record's mission (+0x2c) or monster (+0x54) area;
a successful mission read sets region + 0x15a18 = 1. No draws, no effect on terrain. The
format belongs to a save-file follow-up (overview §7 item 6).

---

## 10. How the rest of the generator reads these records

* Location lookup by column: regions spec §6.3 (record 8·(lx mod 8) + (ly mod 8)).
* Zone generator (terrain spec §5, caves spec): **Lz** = the record of the zone's sub-cell
  (zx >> 3, zy >> 3), and the zone's copied table entry (zone + 0x78..0x87).
* Heights: terrain spec §4.3 location rules (types 1, 4, 6, 7, 13, island rule for sea
  owners), shaping fields §3 (types 1, 2, 4, 5, 6, 7, 10, 13), colours §6.1 (type 4).
* The level, sub-level and variant feed the dungeon builder, the town builder and creature
  levels (follow-up specs).

---

## 11. Answers to open questions of the other worldgen specs

### 11.1 Terrain spec open question 4: the type-10 helper 0x5c3ef0

0x5c3ef0 is not a segment routine: it subtracts two fx points (a 2 × int64 vector
difference). In the mountain mask (terrain §3.4 step 8) the record is the first operand, so
(dX, dY) = (L.x − (x << 16), L.y − (y << 16)): the offset from the column to the location
**centre**, without noise. Then ex = f32(f32((float)dX)·2⁻¹⁶), ey likewise (the 64-bit
integers are converted to float with round-to-nearest), d = f32(f32(ey²) + f32(ex²)) / f32(r²)
(float), and if d < 1: M ← (1 − (1 − d)²)·M. A type-10 site is therefore a round hole of
radius r (512..767 blocks) in the mountain cover.

### 11.2 Regions spec open questions

1. Location creation: this document.
2. Visibility during preparation: §2.2.
3. The float → int64 conversions truncate towards zero (0x68d946).
4. Partly: the link channels B are **roads** – the zone generator lays town-ground blocks
   along them where the flatness W < 0.02 (caves spec §5.2). Type 2, 5, 10, 15 meanings
   remain inferences (§6.3).

### 11.3 Corrections to earlier statements

* Regions spec §6.4: types 1 and 13 are not chosen by the draw & 7 switch; type 1 comes
  from the cell slot (§4.1), type 13 is never created; type 14 has r = 150 and is the
  dungeon entrance; type 11/12 snap to the **zone** centre (256-block grid), not a block
  centre.
* Regions spec §6.2: type 10 is a round hole around its centre (§11.1), not a segment.

---

## 12. Test vectors (seed 123, A = 440, named world)

Computed from this document and the earlier specs by an independent spec-side model; the
model reproduces every earlier test vector (random, noise, cells, displacement, blends).
Positions, radii, types and all integer fields depend only on draws, owners and blends;
**h** depends on the location-free H of the terrain spec §4 and may differ in the last bits
if an implementation evaluates H's float expressions in another order (open question 4).

### 12.1 Home region (512, 512)

Own cell C: point (8397824, 8397824), m 0.4915097653865814, w 0.5601367354393005, e 74,
id 525240; cell slot (cxs, cys) = (4, 4).

* Header: tier 1, h10 0 (no draw), style 3, serial start 274, no hot-region draw;
  level range (1, 10), minLevel 1.
* Slots skipped by the slot-centre owner test (records left at their initial values):
  0–4, 8–10, 16, 48–50, 56–63.
* Candidates: 43. Sorted order (i, j): (3,4) (5,6) (6,5) (5,5) (4,2) (3,6) (5,3) (3,1) (4,3)
  (4,6) (2,2) (2,5) (3,3) (6,6) (4,5) (2,1) (2,6) (5,7) (0,5) (5,4) (1,6) (3,5) (3,7) (6,3) (4,1)
  (5,2) (0,6) (6,7) (4,0) (6,4) (1,3) (2,4) (1,4) (1,7) (2,7) (3,2) (2,3) (5,0) (4,7) (3,0) (1,5)
  (5,1) (0,7). First keys: −0.396343946, 0.372302532, 0.51989007, 0.572452068, 0.891994953,
  0.939336538.
* Total draws after `srand`: **308**; serial after phase C: 1084.

Placed records (fx positions exactly; r and h as floats; "rank" is c):

| # | slot | type | x (fx) | y (fx) | r | h | +0x1c | +0x20 | +0x24 | +0x28 | note |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 5 | (0,5) | 12 | 549814534144 | 550485622784 | 128 | 142.8507537841797 | 0 | 764 | 26 | 2 | rank 36 |
| 6 | (0,6) | 4 | 549822681812 | 550628568518 | 706 | 190.33050537109375 | 0 | 901 | 87 | 3 | rank 52 |
| 7 | (0,7) | 0 | 549822854785 | 550762547407 | 766 | 213.23287963867188 | 0 | 0 | 1 | 0 | candidate, unused |
| 11 | (1,3) | 11 | 549948751872 | 550233964544 | 128 | 165.2716064453125 | 0 | 1041 | 301 | 4 | rank 60 |
| 12 | (1,4) | 10 | 549956491734 | 550360442409 | 729 | 152.62640380859375 | 0 | 25782 | 1 | 0 | type-10 pick 1 |
| 13 | (1,5) | 10 | 549955967888 | 550489560732 | 674 | 177.1772918701172 | 0 | 12662 | 1 | 0 | type-10 pick 2 |
| 14 | (1,6) | 3 | 549943273190 | 550633487754 | 547 | 195.61947631835938 | 0 | 806 | 34 | 3 | rank 40 |
| 15 | (1,7) | 0 | 549956948520 | 550761992408 | 761 | 191.81500244140625 | 0 | 0 | 1 | 0 | candidate, unused |
| 17 | (2,1) | 14 | 550082969600 | 549965529088 | 150 | 196.05026245117188 | 0 | 692 | 18 | 0 | rank 30 |
| 18 | (2,2) | 5 | 550086636472 | 550079029984 | 316 | 237.089599609375 | 0 | 574 | 10 | 0 | rank 20 |
| 19 | (2,3) | 10 | 550089908911 | 550224554716 | 726 | 187.2955322265625 | 0 | 30966 | 1 | 0 | type-10 pick 5 |
| 20 | (2,4) | 14 | 550082969600 | 550351405056 | 150 | 187.16929626464844 | 1 | 1044 | 621 | 0 | rank 62 |
| 21 | (2,5) | 14 | 550082969600 | 550502400000 | 150 | 243.83181762695312 | 0 | 607 | 11 | 0 | rank 22 |
| 22 | (2,6) | 3 | 550091341031 | 550632924695 | 692 | 203.23983764648438 | 2 | 711 | 21 | 3 | rank 32 |
| 23 | (2,7) | 10 | 550090584724 | 550763306878 | 750 | 133.84512329101562 | 0 | 2163 | 1 | 0 | type-10 pick 3 |
| 24 | (3,0) | 0 | 550233329554 | 549818607246 | 565 | 165.4136199951172 | 0 | 0 | 1 | 0 | candidate, unused |
| 25 | (3,1) | 14 | 550233964544 | 549948751872 | 150 | 165.40859985351562 | 3 | 493 | 6 | 0 | rank 14 |
| 26 | (3,2) | 0 | 550228166557 | 550091699876 | 687 | 191.66400146484375 | 0 | 0 | 1 | 0 | candidate, unused |
| 27 | (3,3) | 2 | 550230947402 | 550231415428 | 639 | 196.22561645507812 | 0 | 641 | 13 | 1 | rank 24 |
| 28 | (3,4) | 5 | 550227107042 | 550359015900 | 398 | 179.47958374023438 | 0 | 274 | 1 | 0 | rank 0 |
| 29 | (3,5) | 14 | 550233964544 | 550485622784 | 150 | 243.8876953125 | 0 | 811 | 39 | 0 | rank 42 |
| 30 | (3,6) | 14 | 550217187328 | 550636617728 | 150 | 221.93838500976562 | 2 | 429 | 4 | 0 | rank 10 |
| 31 | (3,7) | 6 | 550225547753 | 550761699771 | 735 | 127.34442138671875 | 0 | 845 | 45 | 4 | rank 44 |
| 32 | (4,0) | 12 | 550351405056 | 549814534144 | 128 | 184.8523406982422 | 0 | 945 | 141 | 2 | rank 56 |
| 33 | (4,1) | 5 | 550352738079 | 549942862354 | 453 | 220.9300537109375 | 0 | 860 | 61 | 2 | rank 48 |
| 34 | (4,2) | 5 | 550366058756 | 550104592852 | 299 | 191.91867065429688 | 0 | 383 | 3 | 0 | rank 8 |
| 35 | (4,3) | 2 | 550359484770 | 550220183367 | 674 | 181.4418487548828 | 0 | 495 | 7 | 0 | rank 16 |
| 36 | (4,4) | 1 | 550359793664 | 550359793664 | 516 | 187.8577880859375 | 3 | 525240 | 1 | 0 | town (cell slot) |
| 37 | (4,5) | 2 | 550344015312 | 550493442725 | 513 | 171.46815490722656 | 0 | 670 | 16 | 1 | rank 28 |
| 38 | (4,6) | 14 | 550351405056 | 550619840512 | 150 | 211.27719116210938 | 1 | 537 | 8 | 0 | rank 18 |
| 39 | (4,7) | 10 | 550355533142 | 550765662762 | 644 | 188.21343994140625 | 0 | 2053 | 1 | 0 | type-10 pick 4 |
| 40 | (5,0) | 0 | 550493444248 | 549823061230 | 759 | 263.40020751953125 | 0 | 0 | 1 | 0 | candidate, unused |
| 41 | (5,1) | 0 | 550494821146 | 549959548503 | 723 | 282.70849609375 | 0 | 0 | 1 | 0 | candidate, unused |
| 42 | (5,2) | 14 | 550502400000 | 550082969600 | 150 | 231.79689025878906 | 1 | 889 | 72 | 0 | rank 50 |
| 43 | (5,3) | 5 | 550494512706 | 550215683752 | 341 | 228.041259765625 | 0 | 458 | 5 | 0 | rank 12 |
| 44 | (5,4) | 14 | 550502400000 | 550368182272 | 150 | 211.44168090820312 | 1 | 796 | 30 | 0 | rank 38 |
| 45 | (5,5) | 14 | 550485622784 | 550502400000 | 150 | 156.45974731445312 | 2 | 347 | 3 | 0 | rank 6 |
| 46 | (5,6) | 14 | 550485622784 | 550619840512 | 150 | 200.17022705078125 | 1 | 309 | 1 | 0 | rank 2 |
| 47 | (5,7) | 14 | 550485622784 | 550754058240 | 150 | 287.55731201171875 | 2 | 751 | 23 | 0 | rank 34 |
| 51 | (6,3) | 14 | 550636617728 | 550217187328 | 150 | 101.46678161621094 | 3 | 848 | 52 | 0 | rank 46 |
| 52 | (6,4) | 14 | 550636617728 | 550368182272 | 150 | 186.68209838867188 | 1 | 994 | 194 | 0 | rank 58 |
| 53 | (6,5) | 6 | 550628858773 | 550494084720 | 667 | 170.71372985839844 | 0 | 320 | 2 | 0 | rank 4 |
| 54 | (6,6) | 14 | 550636617728 | 550619840512 | 150 | 153.9163360595703 | 2 | 669 | 14 | 0 | rank 26 |
| 55 | (6,7) | 14 | 550636617728 | 550754058240 | 150 | 245.3907470703125 | 2 | 909 | 109 | 0 | rank 54 |

Type-10 picks (phase D), in order: record 12 (index 0 of 11 remaining), 13 (index 7 of 10),
23 (index 1 of 9), 39 (index 4 of 8), 19 (index 2 of 7).

Per-zone table entries that differ from the initial values (zone given as (zxl, zyl)):

| Zone | kind | variant | id | level | sub-level |
|---|---|---|---|---|---|
| (19,12) | 3 | 0 | 25357 | 18 | 1 |
| (19,35) | 3 | 1 | 26548 | 621 | 4 |
| (19,44) | 3 | 0 | 11270 | 11 | 1 |
| (27,52) | 3 | 2 | 164 | 4 | 0 |
| (28,11) | 3 | 3 | 9362 | 6 | 0 |
| (28,43) | 3 | 0 | 6335 | 39 | 2 |
| (35,51) | 3 | 1 | 13185 | 8 | 1 |
| (43,44) | 3 | 2 | 21796 | 3 | 0 |
| (43,51) | 3 | 1 | 4712 | 1 | 0 |
| (43,59) | 3 | 2 | 26139 | 23 | 1 |
| (44,19) | 3 | 1 | 21120 | 72 | 1 |
| (44,36) | 3 | 1 | 23550 | 30 | 1 |
| (52,27) | 3 | 3 | 4074 | 52 | 4 |
| (52,36) | 3 | 1 | 10655 | 194 | 3 |
| (52,51) | 3 | 2 | 20439 | 14 | 1 |
| (52,59) | 3 | 2 | 31418 | 109 | 4 |
| (11,36) | 4 | 0 | 25782 | 1 | 0 |
| (11,43) | 4 | 0 | 12662 | 1 | 0 |
| (19,27) | 4 | 0 | 30966 | 1 | 0 |
| (19,60) | 4 | 0 | 2163 | 1 | 0 |
| (35,60) | 4 | 0 | 2053 | 1 | 0 |
| (36,35) | 1 | 1 | 0 | 1 | 0 |
| (36,36) | 1 | 2 | 0 | 1 | 0 |
| (35,35) | 1 | 3 | 0 | 1 | 0 |
| (35,36) | 1 | 4 | 0 | 1 | 0 |

Levels only (kind 0): (3,43) 26, (3,52) 87, (11,28) 301, (11,52) 34, (19,19) 10, (19,52) 21,
(27,59) 45, (28,28) 13, (35,3) 141, (35,11) 61, (35,27) 7, (35,43) 16, (36,20) 3, (44,27) 5,
(52,44) 2. Town-quarter weights w: 0.6970832347869873 (36,35), 0.6452403664588928 (36,36),
0.5470340251922607 (35,35), 0.48573052883148193 (35,36).

### 12.2 Sea region (513, 512)

C: point (8408064, 8395776), e −100 (sea), w 0.7547471523284912. Header: tier 2, style 2,
serial start 1022, minLevel 1; 244 draws; 32 candidates (so no type-10 site); every placed
record has h clamped to 0 except record 24 (centre owner differs: type 0,
h −82.16587829589844). Lakes and valleys become type 15. The town is record 11, slot (1,3),
centre (8408064, 8395776) blocks, r 634, h 0, variant 2, quarters (12,28) (12,27) (12,29)
(11,27). First records:

| # | slot | type | x (fx) | y (fx) | r | +0x1c | +0x20 | +0x24 | +0x28 |
|---|---|---|---|---|---|---|---|---|---|
| 0 | (0,0) | 15 | 550895279956 | 549833322960 | 585 | 0 | 1332 | 16 | 2 |
| 1 | (0,1) | 14 | 550905053184 | 549948751872 | 150 | 0 | 1170 | 4 | 0 |
| 2 | (0,2) | 15 | 550904796350 | 550098876084 | 619 | 0 | 1240 | 7 | 1 |
| 3 | (0,3) | 2 | 550892451175 | 550222133006 | 684 | 0 | 1061 | 2 | 0 |
| 6 | (0,6) | 11 | 550888275968 | 550619840512 | 128 | 0 | 1695 | 141 | 4 |
| 8 | (1,0) | 12 | 551039270912 | 549831311360 | 128 | 0 | 1293 | 13 | 1 |

### 12.3 Unnamed world

With an empty world name the home region's cell slot (4, 4) becomes an ordinary candidate
(key −1.888302 sorts first), there is no town and no town quarters, and the whole ranking
shifts (e.g. record 5 becomes type 3, record 6 type 12). The draw count is unchanged by
coincidence (308).

---

## 13. Checking against the running game

1. Region records: after the home region exists, read world + 0xbc[rx·1024 + ry] and dump
   region + 0x0c..0x17, the 64 records at +0x14018 (0x68 bytes each) and the table at +0x18;
   compare with §12.
2. Draw count: hook MSVCR110 `srand`/`rand` on the generation thread; between
   `srand(A + 1024·ry + rx)` and the next `srand` there must be exactly as many draws as the
   reimplementation makes (308 for the home region of seed 123, named world).
3. Visibility: set a breakpoint on 0x5c5e20 while 0x5da280 is active and confirm that
   world + 0xbc for the region is still null (§2.2).

---

## 14. Open questions

1. **Ties in the two sorts.** The candidate sort (§5, up to 63 entries) and the town-quarter
   sort (§8.2, 64 entries) are the C++ library's unstable sort (introsort: insertion sort up
   to 32 elements, partitioning above). With distinct keys the order is the plain sorted
   order. A tie needs two slots at the same slot distance with the same 15-bit draw, or two
   zones with exactly equal weights; it was not seen in the test regions. If bit-exact
   handling of ties is wanted, the MSVC 2012 `std::sort` partitioning must be reproduced;
   verify by dumping a region whose candidate keys tie.
2. **Server.** **server** 0x50e080 is assumed identical; a byte comparison would confirm it.
   It matters for which side the world name test (§4.1) sees.
3. **Type 13.** No code path in 0x5da280 writes type 13 (spire). Stored mission data or
   another routine might; search the server for writes of 13 to record + 0x18.
4. **h precision.** The record height is H at an integer column; H's evaluation order is
   fixed by the terrain spec, but smooth() and the long sums leave room for last-bit
   differences. Compare h of a few records against a dump; positions are unaffected.
5. Meanings of h10 (header + 0x10), the type-5 variant, and record + 0x2c..+0x67 belong to the
   structure, creature and save follow-ups.
