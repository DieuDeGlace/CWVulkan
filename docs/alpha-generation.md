# Alpha terrain generation

The requested default is Alpha terrain and biomes with Steam town and dungeon
layouts retained. Steam generation remains a separate mode. Verified Alpha-only
locations may be added alongside those Steam layouts. This hybrid is a project
extension; it must distinguish original Alpha terrain from Steam structure edits.

Implementation is in progress. The current default has **not** been switched:
`Terrain.Style=Alpha` remains the older approximate relief option, and
`World.Source=Extracted` still reads captured Steam terrain. Selecting a different
name for that profile would not implement Alpha generation.

The new `alphagen` core reuses the sister AlphaVulkan project's original
GPL-3.0-only clean-room code (DieuDeGlace). AlphaVulkan's README documents the same
specification/implementation separation used here. Random, region, location and
column code is namespaced separately from Steam. The surface implementation stops
at the independently named surface boundary and retains its padded height grid,
random state and ravine boulder candidates. It contains no original game code,
models, textures, saves or captured columns.

The accompanying `alpha-worldgen-*.md` documents are the sister project's independent
behavioral contracts. Their later locations and caves completions supersede older
open-question summaries. Read [the bounded parity assessment](specs/alpha-terrain-generation-boundaries.md)
before treating a height result, surface snapshot or full zone as equivalent.
Formula vectors check implementation against those contracts, not a running-game
oracle. Native surface, seam, precision and world-edge comparisons remain necessary.

Remaining integration includes later terrain passes, coordinate/zone conversion,
Steam structure placement on the new terrain, collision/water/lighting queries,
mode-specific caches/saves/network settings and the final default selection.
The original Steam mode must keep its existing generator and data interpretation.

TerrainVariance lives in the separately licensed SteamCubeWorldMods repository.
Its current wrapper adds offsets to native Steam height queries and translates
stored columns and object anchors after generation. Compatible height changes can
be shared, but translating existing Steam columns alone does not recreate Alpha's
ravine intervals, biome choices or full voxel layout. Full parity must not be
claimed for that transformation without additional implementation and evidence.

The owner's Steam install contains `sprites/Sphinx01.cub`. Alpha's independent
structure specification lists `sphinx01.cub` as a desert-ruin building type.
Its verified placement and role, rather than a guessed separate dungeon, are the
starting point for that addition. Required content stays in the owner's install.


The first shared bridge is the original MIT-licensed `AlphaCoordinates.h` utility in
TerrainVariance, copied byte-for-byte as `alphagen/Coordinates.h` under the existing
SteamCubeWorldMods MIT notice. Explicit integer anchors preserve absolute Alpha
noise inputs. Coverage splits Steam tiles across Alpha parents and clips finite
edges without periodic wrapping. It is not yet a runtime generation switch.
Portable mod coordinate/rule checks and the engine's Alpha checks cover this step;
the native DLL/placement suite requires the mod's separate clang-cl toolchain.


`SurfaceStore` is the next bridge: an immutable seed/name/frame context caches Alpha
parent stages with a bounded ready-entry LRU and coalesces simultaneous requests for
the same parent. Generation runs outside the cache lock. Column leases retain their
parent across eviction and store destruction; published voxel storage is const.
Effective block reads and solid-versus-water surface reads use the Alpha column
rules. Cache capacity bounds retained entries, not caller-held leases or in-flight
work. This store still supplies only the named initial surface stage and is not yet
connected to playable generation or the default configuration.


`SteamBridge` converts that initial surface into read-only Steam-sized column
storage. The 4096 columns retain their bases, extents and RGB values across up to
four Alpha parents; source XY indexing is transposed explicitly. An admission mask
keeps missing finite-world columns distinct from empty native columns. Dry-air
flags are translated rather than copied. Alpha glowing material 13 maps to Steam
light 20, with full emission as an explicit hybrid policy, instead of Steam road 13.
Wood maps explicitly to Steam type 7/flags 0 and leaves to type 8/flags 1,
retaining Alpha RGB rather than repacking working colors. Unknown types are rejected. This is a data
representation adapter, not Steam structure placement or a new runtime mode.

Steam also retains `obelisk.cub`, `castle-arc.cub`, `temple-arc.cub`,
`antique-building1.cub` through `antique-building4.cub`, and `dungeon-test01.cub` /
`dungeon-test02.cub`. File presence and names establish content availability, not
whether Steam uses them or cut them. Owner assets remain outside source and
development packages.


`generateRelief` now continues the same initial-surface work through the rock
formations, road cuts, five-block terraces and shallow pools, stopping before
Alpha dungeon calls. The mutable working zone is moved into immutable output;
there is no duplicate full-zone snapshot during that continuation. The output
retains the random stream and ordered terrace/ravine boulder seeds. Mounds and
mesas add material; a protected non-water block aborts an entire formation before
any writes. Boulder writers are implemented and checked independently, but their
later generation passes are not run at this boundary.

`ReliefStore` uses the same parent-cache synchronization as `SurfaceStore`, with
separate typed stage ownership. The column adapter accepts both stores without
reinterpreting block records. Failed loads propagate and remain retryable, and
aliasing column leases retain the owning stage through eviction/store destruction.
Giant-tree centers now invoke the voxel-tree builder at the specified point in
the same stream. Its other tree kinds share moss, dry-air and zone-clipping rules. This remains a data
consumer integration, not a complete playable Alpha world or default switch.
Extra ruin structures, including the Sphinx addition, are deferred at the user's
request while terrain work continues.


The tree builders reuse the sister project's GPL implementation under the independent
Alpha tree contract. Palms extend its incomplete stamping path using only the
independent mode0 placement contract: owner `palm-leaf.cub` and
`palm-leaf-diagonal.cub` models, ordered height draws for models actually present,
quarter-turn rotation without mirroring, internal dry/keep air and protected-solid
exclusion. The owner loader requires both valid files; no replacement art is made.
Raw model bytes remain owner data and are absent from source/development packages.
The independent worked tree example checks 57 draws, 332 wood cells, 961 leaf cells
and final bark/leaf RGB values. Synthetic palm models check all four rotations,
material flags and model-dependent draw counts. Tree-grid placement, later terrain
passes and runtime/default integration still follow separately.
