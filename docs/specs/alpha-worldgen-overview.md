# Alpha world generation: overview

Entry point to the behavioural specification of how Cube World **Alpha** generates its
world from a seed, written from the Alpha client `Cube.exe` (SHA-256 `84a7a132…30bf`) and
server `Server.exe` (SHA-256 `1ffbcbce…4fb1`) decompiles, their assembly and binary
constants (`disassembled alpha cube world/`). It is the specification side of a clean-room
port: an AlphaVulkan implementer should be able to rebuild the generator from these
documents alone.

| Document | Covers |
|---|---|
| this overview | where generation runs, units, records, the order of everything, follow-ups |
| [alpha-worldgen-random.md](alpha-worldgen-random.md) | `rand`, seeds, the 77 seed offsets, the noise kernel N(x, y), test vectors |
| [alpha-worldgen-regions.md](alpha-worldgen-regions.md) | region cells, displacement, owner search, climate blends, link channels, location records |
| [alpha-worldgen-terrain.md](alpha-worldgen-terrain.md) | shaping fields, base height, the zone surface pass, ravines, lakes, blocks, all colours |
| [alpha-worldgen-vegetation.md](alpha-worldgen-vegetation.md) | plant sprites, ground items, street lights, voxel-tree placement |
| [alpha-worldgen-clouds.md](alpha-worldgen-clouds.md) | cloud placement (completes alpha-rendering.md §11.4) |

Related: [alpha-rendering.md](alpha-rendering.md) (lighting, meshing, block light — §9 is
referenced, not repeated) and [alpha-data-stores.md](alpha-data-stores.md) (assets).

---

## 0. Conventions

* Routines are cited by address in `Cube.exe` (image base 0x400000), or marked **server**
  for `Server.exe` (also base 0x400000; separate address space).
* Precision and integer conventions: random spec §0. Float vs double is stated wherever it
  changes the result; following it is required for bit-exactness.
* Names of fields and types are this spec's descriptive names, chosen from how the code
  uses them; they are not the original developers' names.
* Statements about meaning ("lake", "town") are inferences from behaviour and are marked
  as such where they matter.

---

## 1. Where generation runs

* **Both executables generate.** The client builds every zone it needs locally from the
  world seed; the server contains the same world class and generator (apparent
  counterparts with equal sizes and constants: world initialiser **server** 0x4d83a0,
  noise **server** 0x4d5d30, base height **server** 0x4f9b70, region cell **server**
  0x50b870, region preparation **server** 0x50e080, zone generator **server** 0x518630).
  The two were not compared instruction by instruction (open question 1).
* **The seed comes from the server.** The client starts with `timeGetTime()` as seed, then
  takes the server's seed and world name when they arrive (0x46f620) and re-initialises the
  world with them (0x46afab); random spec §2.1. In single player the client's own local
  server supplies it.
* **Streaming.** The world-update routine 0x46a8a0 (apparently run on its own thread,
  random spec open question 1) looks, for each interest point it tracks, at the zones up to
  3 zone indices away in x and y, and generates **one** missing zone per pass: the one whose
  centre is closest to the reference position, if that squared distance is below
  589824 (768 blocks). Generation of a zone is deterministic and independent of the order
  in which zones are generated, as far as traced (open question 2).

---

## 2. Units and grids

| Unit | Size | Index range | Where |
|---|---|---|---|
| World position | 1/65536 block, 64-bit integer ("fx") | — | entities, locations |
| Block (column x, y; height z) | 1 | x, y in 0..2²⁴−1; z any int | terrain |
| Chunk (render only) | 32 × 32 columns | — | alpha-rendering.md §9 |
| Zone | 256 × 256 columns | 0..65535 | terrain spec §1.1 |
| Location sub-cell | 2048 × 2048 blocks (8 × 8 zones) | 0..8191 | regions spec §6 |
| Region | 16384 × 16384 blocks (64 × 64 zones) | 0..1023 | regions spec §1 |
| Region cell | one control point per region | 0..1023 | regions spec §2 |

* +Z is up; **sea level is z = 0** (every non-solid cell at z ≤ 0 is water).
* The **home point** (spawn) is (8396928, 8396928) blocks, in region (512, 512); its region
  cell is always temperate land (regions spec §2.2).
* World object W = game object G + 0x2e4 (as in alpha-rendering.md). W+0xbc: region
  pointers (1024 × 1024); W+0x4000bc: region-cell pointers (1024 × 1024); W+0x800164: seed;
  W+0x800168..0x800298: seed offsets; W+0x8000f0/f4: home point (floats).

---

## 3. Data records (summary; layouts in the topic specs)

* **Region cell** (0x1c bytes): point, fire flag, moisture, warmth, id, elevation.
* **Region** (0x15a28 bytes): 64 × 64 zone pointers (+0x10018), 8 × 8 location records
  (+0x14018, 0x68 bytes each), a 64 × 64 table of 16-byte per-zone entries (+0x18), header.
* **Zone** (200 bytes): indices, object lists, pointer to 65536 columns.
* **Column** (32 bytes): climate (moisture, warmth, fire), bottom z, block count, blocks.
* **Block** (4 bytes): R, G, B, type byte (type in the low 5 bits, flags 0x20/0x40/0x80).

---

## 4. Order of generation from a seed

1. **World initialisation** (0x5a52e0): `srand(seed)`, 77 draws into the seed offsets
   (random spec §2.2); grids cleared; home point set.
2. **Zone request** (§1). For zone (zx, zy):
3. **Region preparation** for the 3 × 3 regions around the zone's region, each of which
   first creates the 5 × 5 region cells around it, then re-seeds and fills its location
   records and plans (regions spec §6.1).
4. **Zone generation** (0x5e4850), re-seeded with A + 65536·zy + zx (terrain spec §5):
   1. climate pass (per column: moisture, warmth, fire share);
   2. base-height grid 257 × 257 (terrain spec §4);
   3. surface pass with ravines and lake fill (terrain spec §5.4–§5.6);
   4. post passes (§5 below), including voxel trees and plant sprites;
   5. stored modifications for this zone are read (save file name built from the world
      name and "zone", 0x5edd7c..0x5ede60; single player);
   6. block light of the whole zone (0x59a0e0 → alpha-rendering.md §9.2);
   7. the zone is inserted into its region.
5. **Per frame** (not stored): clouds (clouds spec), sky palette from the camera's climate
   (alpha-rendering.md §3).

---

## 5. Post passes of the zone generator (overview; detail in follow-up specs)

In code order, after the surface pass. All consume the zone's random stream, so a 1:1
port needs every one of them before the vegetation draws line up (terrain spec open
question 2).

| Code range | Condition | What it does (behaviour seen) | Spec |
|---|---|---|---|
| 0x5e6b58 | always | free height grid; copy the zone's 16-byte entry from the region table | terrain §5.7 |
| 0x5e6c56..0x5e6f34 | zone location type 6 | 12 × 12 candidate grid (64-block steps from the zone origin, positions 256zx + i/3 + 42 …): draw & 3 ≠ 0, bump ≥ 0.5, no earlier one within 80 blocks, ≥ 60 blocks from home → carve an ellipsoidal noise-edged **cave** with radii 20..29, 20..29, 20..35 (0x5cbe00) at the column top | caves (follow-up) |
| 0x5e6fff..0x5e7040 | zone location type 13 or 4 with its centre in this zone | find the top solid z at the centre (used by a later feature) | follow-up |
| 0x5e7040..0x5e7154 | type 11, centre in zone | one draw; cavity 0x5cbe00 of 100 × 100 × (100 + draw mod 100) at the zone centre | caves |
| 0x5e7154..0x5e727a | type 12, centre in zone | one draw; **giant voxel tree** (size 80) at the zone centre | vegetation §6 |
| 0x5e72a8..0x5e7dc5 | (several draws) | uses q, R, M, W, H and the cliff colour; writes voxels; calls 0x5cb600 and 0x5cbe00 — likely rock formations or mountain caves | follow-up |
| 0x5e7e2d..0x5e8435 | — | uses R, q, H, rock colour; voxel writes; 0x5a6b60 entries | follow-up |
| 0x5e84c3, 0x5e84f7 | — | **dungeon** builder 0x5cc510 (layout grid, connectors; see the decompile's PLACEMENT guide) | dungeons (follow-up) |
| 0x5e852d..0x5e8b7f | — | draws, noise, rock-coloured voxel writes (veins or boulders) | follow-up |
| 0x5e8d7b | — | 0x5f0ce0 (15724 bytes) | towns? (follow-up) |
| 0x5e8ddb..0x5e9672 | — | NPC/creature creation (0x5ad320, 0x58d860) and the 61916-byte builder 0x5af170 | towns/castles (follow-up) |
| 0x5e9783..0x5e9e4c | — | sin/cos placements of static objects (0x58eaa0, 0x5a0e00) | follow-up |
| 0x5e9e7d..0x5ea7c6 | always | **voxel-tree grid** 14 × 14 | vegetation §5 |
| 0x5ea7f4..0x5eb8xx | — | more static objects and creatures (0x5adb20, 0x66add0) | creatures (follow-up) |
| 0x5ebc60..0x5ec5e8 | always | **plant sprites, street lights, ground items** | vegetation §2–§4 |
| 0x5ec5f9..0x5ed86d | — | finalising object and creature lists | follow-up |

**Caves.** Alpha has no separate cave noise: caves are the ravines of the surface pass
(terrain spec §5.5), the carved ellipsoids above, and dungeons. The ellipsoid carver
0x5cbe00 (x, y, z, rx, ry, rz) tests each cell against a noise-distorted ellipsoid (noises at
0.01 and 0.05 scales) and writes air above sea level / water below; specifying it is part
of the caves follow-up.

**Structures and creatures** hook in only after the terrain: none of them changes the
height function. They do change blocks (and later passes see those blocks), and they draw
from the zone's random stream.

---

## 6. What a first implementation can reproduce

With the topic specs as they stand, an implementation can reproduce exactly: the seed
offsets, the noise kernel, all region cells, the four climate blends, the link-channel
distance, the base height and surface of columns whose 2048-block sub-cell has **no
location** and whose zone's location is not of types 4, 6, 13, all surface colours and
block layouts there, and the cloud placement. Everything that depends on the location
records, and the vegetation draws (which follow the unspecified post passes in the random
stream), needs the follow-up specs below. A practical order for AlphaVulkan: implement
§4 steps 1–4.3 and the colours, compare columns against dumps from the running game,
then add the follow-ups one by one.

---

## 7. Follow-up specifications needed

1. **alpha-worldgen-locations**: region preparation 0x5da280 in full (location records,
   per-zone table, town and dungeon plans; helpers 0x5a4710, 0x5c3ac0, 0x5f4d10, 0x5c3ef0).
2. **alpha-worldgen-caves**: the ellipsoid carver 0x5cbe00 and 0x5cb600, and the rock
   passes 0x5e72a8..0x5e8b7f.
3. **alpha-worldgen-trees**: the voxel-tree builder 0x5df960 (kinds 0–6).
4. **alpha-worldgen-structures**: towns, houses and castles (0x5f0ce0, 0x5af170, static
   objects, signs, street furniture), dungeons (0x5cc510, 0x5c38c0, 0x5ef6d0, 0x5df600).
5. **alpha-worldgen-creatures**: creature and NPC spawns in zones (0x5adb20, 0x66add0,
   0x5ad320, 0x58d860) and in ravines.
6. **alpha-zone-saves**: the per-zone save file and how stored edits are applied.

---

## 8. Verifying against the running game

The user's 32-bit Alpha mod DLLs can call the generator's routines in-process and dump
its records; no screenshots are needed for most checks:

1. After a world loads, dump W+0x800164..0x800298 (seed and offsets) and compare with the
   random spec for that seed.
2. Dump region cells (W+0x4000bc) and the home region's location records; these are both
   test data for the regions spec and the input the locations follow-up must reproduce.
3. Call the leaf routines (noise 0x4c0ef0, blends 0x5c4dd0/0x5c4800/0x5f0720/0x5ef040,
   colours 0x5ee540/0x5f9e50/0x5f9620/0x5c7140, height 0x5c5e20, mask 0x5effa0) at chosen
   inputs and diff against a reimplementation.
4. Dump a fresh zone's columns (bottom, count, blocks) and object lists right after
   generation (hook the return of 0x5e4850) for a zone without locations.
5. Hook MSVCR110 `srand`/`rand` around one zone's generation and log the draw count per
   phase: this pins down the unspecified post passes' consumption.

---

## 9. Open questions

1. Client and server generators are assumed identical; a byte comparison of the routine
   pairs listed in §1 would confirm it.
2. Order independence: structure builders may write into neighbouring zones; if so, the
   generation order of §1 would matter. Not traced.
3. Whether the client also generates zones the server never asks for (e.g. for the map),
   and whether the server streams zones by its players' positions in the same way.
4. All open questions of the topic specs, chiefly: location creation (regions spec Q1),
   post-pass draw counts (terrain spec Q2), the tree builder (vegetation spec Q3), and the
   cloud coverage sampling (clouds spec Q1).

See also: [alpha-worldgen-locations.md](alpha-worldgen-locations.md) (region preparation, location records, per-zone table) and [alpha-worldgen-caves.md](alpha-worldgen-caves.md) (post passes 0x5e6b58..0x5e8b7f, ravine spawns, corrections to §5 of this overview).
