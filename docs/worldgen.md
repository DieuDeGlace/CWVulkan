# World generation

CubeVulkan reimplements Cube World's world generator, so the whole world exists without the game running, as OpenMW does for Morrowind. The work goes layer by layer, and each layer is checked value for value against the game itself.

**Source:**
- The supported Steam build (5d93ab0d), read from the running game's code (read-only).
- The executable stays SteamStub-protected on disk; nothing is decrypted.
- The noise is Stefan Gustavson's public-domain 2D simplex noise with Ken Perlin's permutation table, as the game uses it. Everything else is written from the game's behaviour.

## Status

**2026-10-29 placement audit:** the loader now keeps live model/object lists
separate from ground-item, backup and derived-light lists. Object sizes use the
type extents and owner's voxel-X dimension; captured full-byte rotations,
door hinges and gate/window slide extents are retained. Crop and furnishing
tint/flag corrections preserve random-draw order. All 50,116 initialized model
records in 789 overlapping seed-123 snapshots now match in list order, including
size/angle, tint and flags. The full engine test suite passes; a deep read of
31,151 exported zones finds no invalid terrain records, but still reports a
missing owner `stone-stool` model. These are bounded checks: the historical
coverage table below is not proof of all seeds, cultures or dungeons. Live
object animation synchronization, light-beam rendering, model lighting and the
consumed undefined lamp-turn bytes remain open in the
[parity audit](specs/worldgen-parity-gaps.md) and [roadmap](roadmap.md).

The generator is now `src/worldgen/` (namespace `worldgen`), rebuilt clean-room from
`docs/specs/worldgen-*.md` (see `docs/CLEANROOM.md`); the engine uses it through
`worldgen/Generator.h`, and CubeVulkan's own options are in `worldgen/Options.h`. The routine
names in the table below are those of the earlier port, which is gone; the checks hold for the new
code (`EngineTests wgterrain`, `wgstructures`, `wgcreatures`, and the commands named below).

| Layer | Game functions | CubeVulkan | Checked |
|---|---|---|---|
| Regions, biomes, places, height | 0x35ea0 and 38 callees | `src/worldgen/Regions`, `Places`, `Height` | Exact at 2882 points (all 25 intermediate values) |
| Blocks of a column (type, colour) | 0x2d8e00, 0xba820 and callees | `blockAt`, `toBlock` | Exact: 40 values at 3906 points (`EngineTests blocks`) |
| Zone columns (terrain, river water, snow, cliffs, light) | 0x2ff6f0's column loop and cliff pass, 0x2a3260 | `field`, `terrainColumn`, `unlitZone`, `litZone` | 99.95%+ of plain columns over all 7615 vanilla zones |
| Cell objects (what stands in each 16-block cell) | 0x69c60 / 0x2d53d0, 0x25f790, 0x2d8630 | `cellObject`, `cellReach`, `cellObjectPoint` | Exact at 60,000 cells (`EngineTests cells`) |
| Trees, rocks, hills, logs, pedestals | 0x2ff6f0's cell loop (switch at 0x314a5f) | `buildStructures` | Every structure block of the first 300 extracted zones (1,339,827) |
| Place buildings (towns, castles, fortresses) | 0x3063c3 to 0x3148d1 and the building class 0xba9a0 | `buildBuilding`, `buildRoof`, `Furnisher` | Every one of the 24,362,466 structure blocks of the 7615 vanilla zones (`EngineTests zones 7615`) |
| Static objects (furniture, doors, window panes, fences, lamps) | 0x2fc270, 0x69860, the wall section 0x30bd50, 0x305115, 0x315fd8; marked by 0x31d700 | `roomObjects`, `cropField`, `streetLamps`, `markObjects`, `zoneObjects` | 3412 of the 3415 objects in the extracted zones' records (`EngineTests allobjects`); the other 3 come from the daily refresh (below) |
| Models (plants, fruit, lamps, signs, furniture models) | 0x3191b0 (plants), the tree builders, 0x2fb7d0 pushes in the house builder, 0x69860 | `plants`, `fruit`, `putModel`, `zoneModels` | Every model of the extracted zones' records (`EngineTests allmodels`); fruit sizes are uninitialised memory in the game |
| A region's roles (castles, special towns, dungeons; their doors and treasure) | 0x115df0 (its last pass, role 18, never applies) | `makeRegionRoles`, `regionRoles` | Doors and castle treasure match where checked |
| Spawn spots, chests, crates, cell 98/99 things | 0x3162b0, 0x316780, 0x31bd90, part of 0x3171a2 | `findSpots`, `takeSpot`, `cellThings`, `cellCreatures` | Match where checked |
| Creatures (spawn points, zone+0x460) | 0x3171a2 to 0x318e0f; gear 0x511e0, groups 0x47e20 and 0x2fdf90, castles 0x2fb850 and 0x31a800 | `makeGear` (worldgen/Gear.cpp), `townsfolk`, `wildGroup`, `castleCreatures`, `animals` | Gear: all 30,000 oracle samples (`EngineTests gearoracle`). Spawn points of the 188 dumped zones (`EngineTests allspawns`): animals 52/52, wild groups 498/500 placed, townspeople 24/31 in place and the rest moved by the game after generation (below). In the generated sets (271 zones, refreshed and not) every spawn point matches field by field: door buildings (0x31abb0) give their cell a kind-4 spot at the ground floor's middle (0x307a87), which their keepers take |
| Dungeon creatures (kind-8 places) | 0x2fa7b0 | `dungeonCreatures` | Their cells' items all match |
| Region roles (0x115df0) | roles, levels (+0x60), special town, marked flag, items | `makeRegionRoles` | Identical to the game's for the 5 regions of the extracted zones (`EngineTests rolesoracle`, Extract/oracle/regions.txt) |
| Region-role things | 0x11a820: roles 6-13 an item (type 24); 19-23 their own set pieces; others an encounter (0x1274d0, kind by 0x119a00, partly by the world's day) | `regionRoleThings`, `roleItem`, `encounterKind`; encounters `guardedChest` (0), `lonelyCreature` (1), `bossCamp` (2-8), `questCamp` (9-16), `groupCamp` (17-23), `arenaTeams` (27), `bossWithGuards` (30-32); roles `roleCamp` (19), `bossWithGuards` (20), `towerPeople` (21), `roleKeepers` (22), `roleNests` (23) | All spawn points of 247 generated zones match the game field by field (418), with their objects and items. Encounters 24 (0x123210) and 26 (0x1224c0) are unreachable: 0x119a00 never gives 0x18, so 0x115df0's last pass pairs no place and role 18 never occurs |
| Cell objects 97 (shrine), 66-68 (water tower, aqueduct, cistern), 54/55 boulders | 0x303315, 0x302ce7, 0x303575, 0x303244; 0x146560 | `shrine`, `waterTower`, `aqueduct`, `cistern`, `boulder`; mason `tube` (0xc1610, 0xc19b0), `stripedWall` (0xbd590) | All generated zones holding them match block for block |
| Kind-7 places (wizard tower, cell object 52) | 0x3039df-0x304d98 | `wizardTower` | The 8 extracted tower zones match |
| Roads over walled pits (variant-5 places) | 0x2ae6d0 in the column loop | `terrainInto` | An earth bridge from the unshaped height (mode 1) down; the 8 tower zones' "causeway" |
| Rock spans of arenas (24) and rock places (45, 46) | 0x2b8010, 0x2ba2b0, 0x2bab80, 0x2ba5f0 | `rockSpanTop/Bottom/Colour` | Generated zones of kinds 24 and 46 match (101,000 columns) |
| Daily refresh of a zone | 0x31c540, run when a zone is loaded and when the day changes: srand(rnd(x + 57y + 0x4373e1) + day); shoppers (hostility 3) at the shops' counters; when (3y + day + x) % 5 == 0, one time in five, travellers camping on a random cell of the zone (a lone traveller 0x130, townspeople in towns, else people by the place's style), with a campfire (0x53) and up to three of their things one time in four. No blocks change | `refreshZone`, `setRefreshDay` (`CW_REFRESH_DAY` for the tests; the engine's Game world uses the game's day) | The 271 refreshed oracle zones (day 135): every shopper and camp, with its creatures and objects, matches; one shopper of a one-counter shop the game places from past the counter list's end (garbage) |

## How the height is made (0x35ea0)

**Building blocks:**
- **Hash:** `(n·(n²·60493 + 19990303) + 1376312589) & 0x7fffffff`, with `n ^= n << 13` first. `rnd(n) = 100000 − hash(n)/21474`.
- **Noise:** simplex noise at 0.32 × the coordinates.

**Regions** are 8192-block cells:
- Each has a random centre (`regionCentre`) and a biome 0–13 (`biome`, from `rnd % 50`). The 3×3 regions around the origin are biome 2.
- A point belongs to the nearest centre after a noise warp of up to ±1024 blocks (`regionOf`). Near a landmark, it takes the landmark's region (`regionAt`).

**Places** are 1024-block cells:
- A cell is "big" (`bigCell`) by a hash parity or a mod-7 pattern. Big cells pair up into 2048-block cells.
- The landmark point is `cellPoint`. Its kind (`placeKind`) uses the same numbers as the dialogue's location types: 1 House … 47 Vault. Some kinds depend on the ground height and biome there.
- Its influence (`placeAt`) fades over 250 blocks, or 500 for big places.

**The column** (`worldgen::columnValues`) adds, in order:
- the base height: 100, eased down near oceans (biome 1) and lakes (7), ±20 noise, plus the place's shape;
- roads between neighbouring places;
- rivers, lava and swamps: thin channels of three-scale noise in their biomes, with lake and lava-lake places and moat rings;
- highlands (ridged noise);
- dryness and vegetation;
- the final height (`0x2b4450`): mountain, crater and volcano shapes, rolling ground, terraces, plateaus, walled pits, spiral ramps, peaks, river beds, and optional cliff steps.

**Mode bits:** the game's fourth argument. 1 skips place shapes, 2 also skips highland peaks, 4 adds cliff steps, 8 skips beds.

**Exactness:**
- Every float operation is in the game's order. The C runtime's `sqrtf`, `acosf` and `powf` are the same ones the game imports.
- Rounding helpers follow the game's own: floor of negative coordinates, truncation of dots, and `minss`/`maxss` operand order.

## Checking it

1. In the game, with ZoneExtractor, load a character and press **O**. This writes `Extract/gen-oracle.txt`: every intermediate value of the game's height at ~2900 points (around the player, the origin, and up to ±2,000,000 blocks out).
2. Run `EngineTests`; its `genMatchesGame` test compares them.
3. `EngineTests gen [field]` prints the differences.

**Zones the game generates on request.** To check places that the extracted world lacks, the game itself can generate chosen zones:
1. Write `Extract/gen.request` (lines `x y`, zone coordinates; `EngineTests encounters <r> <day>` lists every place within r regions with its role, encounter and the zone it is built in).
2. In a game session (`build/game_session.sh start`), load the TEST character. ZoneExtractor makes each zone the way 0x2ae0c0 does (rand() seeded with `y*1024 + seed + x`, 0x2f9400, the generator 0x2ff6f0), two per frame, outside the world and without its saved changes. It writes `Extract/oracle/gen/z_x_y.cwz/.rec/.spawns`, a `.rtrace` of every rand() call (caller RVA and value), and `day.txt`.
   Each zone is then given the game's daily refresh for the world's day (0x31c540, as on loading) and written again to `Extract/oracle/gen/refreshed/` (with its own `.rtrace`); compare those with `CW_REFRESH_DAY=<day>` set.
3. Copy them into a folder named like `gen_seed_123` (with the oracle's creature tables beside them) and run the usual comparisons on it: `CW_ZONES_DIR=<folder> CW_DAY=<day> EngineTests zones|zonediff|allobjects|allspawns|allitems`, plus `EngineTests rtrace x y` (our rand() values against the game's, to the first difference).
Fresh zones have no light yet; `zonediff` ignores light-only differences.

## How the blocks are made (0x2d8e00)

The zone generator keeps 13 values per column (its 0x58-byte field): the height, warmth (0x2d9b60), dryness, highlands, the lava, forest and kind-27 place weights, the lake region, vegetation, river, lava, swamp and roads. `blockAt(x, y, z, field)` then gives a block's colour and type:

- **Lava and poison:** inside lava (type 17) and swamp (type 19) channels, glowing (byte 3 = 255).
- **Buried:** below the floor of the column and all four neighbours, and everywhere on vegetation, roads (type 13) and low shores: ground (type 5, 0x2ab410) or, in highlands, rock (type 6, 0x2d3150).
- **Surface:** grass (type 4, 0x2b40b0), browned by dryness and tinted in forests and kind-27 places. In lava regions it turns into lava ground (type 14).
- **Steep sides:** blended from the surface towards the buried colour, by the steepest drop or rise to a neighbour.
- **Ground:** brown, turning to sand below z 100 and where it is warm and dry, and to pale rock where it is cold. Below z 100 its noise is offset by two numbers kept in the game's world (world+0x330/0x334: 10211, 5983). These were read from the running game and match the seed-123 extraction; whether they depend on the seed is not known yet.
- **Rivers:** a wet bed (type 3) one block under the ground, then water (type 2) up to the river's level (0x2e6420). Red is 255 on water, which is the level the game's water keeps for a full block.
- **Snow:** one block (type 10, flag 1) on cold, level ground (0x2b4320).

A stored block is half the float colour, clamped and truncated (0xba820).

The zone's columns use the place of their own 1024-block cell (0x31a5d0/0x31a690), with no distance cut-off; their neighbours' heights come from `groundHeight`.

In the engine, `[World] Source=Game` streams zones from `terrainZone` through the same loader as extracted zones (`ExtractedWorld::openGenerated`). Water above the sea now has a surface drawn.

**Checking:**
1. Key **O** also writes `Extract/gen-blocks.txt`.
2. `EngineTests blocks` compares the block functions.
3. `EngineTests zones [n]` compares whole generated columns with the first n extracted zones. It counts apart the columns the game built on (trees, buildings) and those inside places.

## How the structures are made (the cell loop)

After its columns, the zone generator walks the 8x8 16-block cells around the zone (its own 4x4 and two more on each side, those whose object reaches in). Each cell's object comes from `cellObject`. Before building, the generator calls `srand(rnd(y*57 + x) % 10000000)`, so every structure draws its own random numbers.

| Object | Builder | What it is |
|---|---|---|
| 1, 8 | 0x264670 | common tree (8: 50 high, three times as wide, at the cell's centre) |
| 2 | 0x263210 | fir: tapering trunk, a spiral of 30 drooping needle branches, snow in the cold |
| 3 | 0x2626e0 | birch: speckled white trunk, five twigs, round leaves |
| 4 | 0x263ea0 | willow: leaves with hanging strands |
| 5 | 0x265e70 | palm: banded leaning trunk, drooping fronds |
| 6, 7 | inline | common tree or palm on a round white stone pedestal (building class) |
| 9 | 0x146450 | rock |
| 10 to 14 | 0x145fa0 / 0x145eb0 | rock or earth hill, by highlands |
| 15 | 0x145b30 / 0x145cf0 | hill with a canyon |
| 19 | 0x266ad0 | stump |
| 20, 21 | 0x2651f0 / 0x265840 | fallen log along x or y, hollow or not |
| 54, 55 | inline | rock, on a hill when under water |

A tree's size is `rnd(cell + 0x2fbcc94) % 10 + 20` high and `rnd(cell + 0x12ce32c) % 10 + 12` wide. It stands at the cell's point on the ground (`groundHeight` mode 4, at least 0), one block up. Structures write through the game's block store (0x31c1c0): air is not stored above a column, air under the sea becomes water, and a column grows down with its buried colour. Blocks with flag 2 are protected from most builders.

**Checking:** `EngineTests zones [n]` counts the structure blocks (types the terrain never makes) and the differing blocks by the object of their cell. `EngineTests objects x y` lists the objects around a column.

## Static objects and flag 0x40

Furniture, doors, window panes and the like are static objects (zone+0x60, 0xe0 bytes each: type, position in 1/65536 blocks, quarter turns, scale). When a zone is done, 0x31d700 sets flag 0x40 on every block inside each object's box (0x80 too for object 15, 4 for objects 3 to 5), storing air blocks where needed. Box sizes come from 0x13a520 (`objectSize`), times the object's scale; an odd turn swaps width and depth.

| Where | Game | What |
|---|---|---|
| Room furniture | 0x675c0 sets up four lists by castle or building type; 0x69860 places | Pieces built against the walls (posts, bookshelf, fireplace, window box), models on and under walls, a lamp on a chain, a hanging cone |
| Room objects | 0x2fc270 | By the town block (`townBlock`): a table and chair, a bed with things round it, or counters; then up to three things along each wall, settled by 0x31b2d0 |
| Walls | 0x30bd50 to 0x3124e4 | Door, window pane (6 or 7), window box (60 to 62), things outside on the ground |
| Beams | 0x30bb40 | The beam's object, scaled to the beam |
| Crop fields | 0x305115 | Fence posts where a field meets neither a field nor a building; crops |
| Cell 92 | 0x3026b9 | Object 117, placed with rand() before the cell's srand() |
| Towns | 0x315fd8 | Street lamps on road edges |

| Region roles | 0x115df0 | A region is 8x8 1024-block cells. Its places get roles from an xorshift counter seeded by `rnd(region hash + 0x46d7c2)`; roles 10, 12 and 21 give the item list (16, 15, 14) that some doors (roofs 4 and 5, buildings 76, 81, 100) and castle rooms of detail 5 take their object from (0x12a600) |
| Spawn spots | 0x3162b0 | Every other column, a block or so off by rand() (no srand): kinds 0 (open ground), 18 (water), and 1, 2, 5, 6, 7-14, 16, 17 by the blocks below and above and room around. 0x31bd90 hands spots out once each, drawn with rand(); without one, the top of the zone's middle column |
| The zone's cells | 0x316780 (srand per cell) | Chests in castle rooms (kinds 3 and 5) on kind-6 spots, crates by cells 14, 15, 17, 18 and 100, and objects 118-124 or 130 by cells 13, 37, 69, 70 |
| Cells 98, 99 | 0x317e7b (srand(rnd % 32767) per cell) | One or two things on kind-1 spots |

A few game quirks the port keeps: on wall 1 the first outdoor spot of a one-window wall turns its object like wall 0's (turn 3, the game's copy at 0x30df67); the tree pass re-seeds rand() for every object near enough (0x314a46), built or not, so a house's leftover draws never reach a later cell's type-92 object.

## Models (zone+0x18)

Models are 0x40-byte records: type, position, two floats (+0x20 size, +0x24 yaw in degrees), a tint (+0x2c) and flags (+0x38; 1 = gives light, 2 = default). The engine names them through the owner's `Extract/sprite-groups.txt` (environment table).

| Where | Game | What |
|---|---|---|
| Plants | 0x3191b0, after the cells' things, on the zone's rand() | On each block something may grow on: grass, flowers, bushes and reeds by warmth, dryness, forest and two noises (0.01 scale), water plants under the sea, hanging plants under overhangs, mushrooms on type-17 blocks. One record on the stack is reused, so a branch that leaves a field keeps the last value |
| Overhang light | 0x302153 (cliff pass) | Model 0x59 as tall as the gap below, with the glowing block |
| Tree fruit and leaves | birch, fir, willow, common tree | Fruit 0x4e-0x50 one tip in ten (birch and fir by rand(), willow and common tree by xorshift of a rand() seed); a leaf model 0x4d tinted leafA/255 at each common-tree tip |
| Ceiling lamps | 0x3122c8 | Model 0x17 (0x1a for building kind 4) under each floor's ceiling in attached houses |
| Door lamps and shop signs | 0x30d21a and the other walls' copies | Lamp 0x18 (0x19) a block outside over the door of attached houses; a shop sign 0x2e-0x35 by the town block's role, 1.5 blocks past the door's side |
| Wall plants | the outdoor spots | 0x36-0x38, kept only inside the zone (0x2fb7d0) |
| Crops | 0x305115 | Crop models in fields |

## Items lying about (zone+0x30)

After cells 98 and 99, each cell (srand(rnd(cell) % 32767)) gets items (0xe0-byte records: type byte, kind at +4, position at +0xa0, angle at +0xc0), in `cellCreatures`:

| Game | What |
|---|---|
| 0x318068 | Item 0x18/2 on a kind-16 spot of every 16th cell (x and y multiples of 16) in regions of culture 1 below 10 blocks |
| 0x31832b | Herbs (0xb: 0x16, 0x18 by snow, 0x1d one in five: `rand() % 5`) one cell in forty; under trees 0xf or 0x24, and by the cell hash berries (1/0xe, 0xb/0x28) under common trees and 0x29/0x2b under willows. Quirk: the record's position is left from the last herb, and herbs 0x16/0x1d are only placed (and the cell's later items and creatures only made) when the block under that old position is type 4 |
| 0x3186a9 | 0xb/0x27 on a water spot one cell in ten where it is cold |
| 0x3187f9 | 0xb/0x13 on a kind-5 spot one cell in thirty |
| 0x3188b3 | Crops to pick in fields 27, 32 and 33 (0x10, 0x11, 0x2a) on kind-21 spots |

Every item made matches the extracted records (`EngineTests allitems`: 5473 of the 5569, none extra). The cells of kind-8 places (0x2fa7b0, not made yet) are skipped; the missing items are theirs and the plinths' (type 30).

## Creatures (zone+0x460)

Spawn points (0x950-byte cube::SpawnPoint records) come from the cell loop and draw the same rand() stream as the items, so both have to be made. The game's creature tables are data, so they are not in this repository: tools/ZoneExtractor's oracle reads them from the owner's game into `Extract/oracle` (touch `Extract/oracle.request` while in a world):

| File | What |
|---|---|
| `spawn-table.txt` | The 120 creature groups of world+0x1d0 (built by 0x44b40): kind bits (+0x18), climate mask (+0x1c), ground mask (+0x20), creature types; and the per-race levels (world+0x1b8) |
| `races.txt` | 0x13f0d0's variant lists, from which 0x1420f0 picks with one draw |
| `looks.bin` | 0x53e80 on scripted rand(): per race, its draws (5 for most, up to 13) and appearance flags; also the full appearance, for the renderer later |
| `gear.bin` | 30,000 calls of 0x511e0 on scripted rand(), the test for `spawnGear` |
| `z_<x>_<y>.spawns` | the loaded zones' spawn points, the test for `zoneSpawns` |

The pseudocode of the user's Ghidra export (Documents/decompiled cube world steam) is a reading aid only: it drops stores (0x5227d's weapon and chest writes by appearance flags 8, 0x10, 0x20 were missing) and loses arguments; the oracle and the assembly decide.

| Where | What |
|---|---|
| 0x317290 | Town cells (place kind 5): shopkeepers by the town block's role (classes 0x80-0x87, 0x9a for role 10 plus a 0x12f helper raised by its height), one in ten houses' people (0x8a), the town hall's mayor (0x9b, cell centre), one in twenty other cells (even cx+cy) a passer-by or a traveller (hostility 4). Race: a variant of the culture's people (jump table at 0x319130); appearance flag 0x400000 when the place is the region's special town |
| 0x317c54 | The first town's three set people (0x101, 0x102, 0x13) |
| 0x3181aa, 0x2fdf90, 0x47e20, 0x31b910 | Wild groups (one cell in 128 off roads): a group that suits the climate and ground, its kind (1, 2, 4, 8, 0x10: size and level), kind 8 with a camp (objects 0x53, 0x5e-0x60...) |
| 0x2fb850, 0x31a800, 0x10eb10 | Castle bosses (castle cell kinds 2, 4 and 6), each reseeding rand() (so the cell's later items follow the new stream) |
| 0x3189d6 | Animals (one cell in 64: by lava, cold, dryness), and one cell in 16 another animal or a wild group of animals |

The game's `(hash / 21474) & 3` goes through a signed multiply by the negative magic number: it is `-(hash / 21474) & 3`, that is `rnd(n) & 3` (100000 being a multiple of 4); `& 1` is the same either way. Encounter kinds of places without a treasure add the world's day (world+8) to their hash, so they change from day to day (`setWorldDay`; the extracted zones were made on day 7, as the camps show).

Found by the comparisons: herbs 0x1d are `rand() % 5` (not 10); spawn points with no spot of their kind take the zone's middle column (0x31bd90), and the game moves those of townspeople to their cell's centre later (not generation). Not yet: kind-8 places (0x2fa7b0), the region-role spawns (0x11a820), the appearance flag 0x8000 that 0x53e80 adds for wild creatures (its fourth argument), the last pass of 0x115df0 (role 18).

## World scale

`[World] Scale` (1 to 4, default 1 = the game). It scales the region layer: points are pulled toward the origin by the scale before regions are looked up (`regionOf`, `neighbourhood`), and region distances are given back in full-size blocks. As a result, biomes, landmasses, oceans, their blends and the region-sized noise all widen. Places, roads, rivers, terraces and blocks keep the game's sizes.

At 1.5, a 300,000-block square holds 2.14x fewer regions than at 1 (`worldScaleWidensRegions`). At 1, nothing changes.

Unlike the BigRegions mod, which only renames which region a point belongs to, this also widens the land itself.

Found with the generated zones: place kind 4's cells are objects 0x23-0x26 but 0x25 (`(t - 0x23) & ~3`, not `t & ~3`); 0x140490's travellers are {2, 9, 0, 13, 7} (the 0 is a register); 0x1241b0 keeps max(n / 8, 8) of its shuffled cells (growing the list with cell 0 0); crop fields add kind-21 spots one time in four (zone+0x100 is the spot map), which the crops to pick then take; houses of building kind 6 blend their colours relative to the ground. The world seed of a zones folder is the number after its last underscore.
Not yet: castle room detail codes 11 and 12 (the halls of big structure 42 and the like: their pieces use a pattern-0 scheme, so their ceilings show as a few lighter blocks), the mason's ground pattern (2, 0xbe8d0). The lamp objects' +0x20 byte is never written by the game (stack left-overs): 0 on its worker threads, as ours.
