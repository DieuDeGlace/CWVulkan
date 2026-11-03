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
