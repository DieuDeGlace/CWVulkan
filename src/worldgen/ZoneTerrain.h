#pragma once
// A zone's terrain (docs/specs/worldgen-terrain-zone.md §2, §3, §5): the first part of the zone
// generator 0x2ff6f0. A zone is 64 x 64 columns; for each it keeps the values the ground was made
// from (the column field) and a run of stored blocks from a base height up. The terrain lays
// down the ground, river water, snow, the rock spans of arenas and some castles, the earth under
// roads across walled pits, and cuts the overhangs of cliffs.
//
// Part B (structures) continues on the same zone and the same rand() stream; it fills the
// structure anchor through the hook below.
#include "worldgen/BlockColours.h"
#include "worldgen/Noise.h"
#include "worldgen/Places.h"
#include "worldgen/WorldValues.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace worldgen {

constexpr int ZONE_SHIFT = 6; // 64 blocks
constexpr int32_t ZONE_SIZE = 64;
constexpr int ZONE_COLUMNS = ZONE_SIZE * ZONE_SIZE;

// The anchor of a place's big structure (a castle, a dungeon), in 16-block cells, or
// NO_ANCHOR when the place has none. Part B provides it (worldgen-structures.md §6.2: the big
// structure 0x2aa270 and its anchor 0x2a9df0), given the place cell (evened for a big place),
// its kind and big flag. The terrain needs it for the rock spans of kinds 45 and 46.
constexpr Int2 NO_ANCHOR = {0x7fffffff, 0x7fffffff};
using StructureAnchor = std::function<Int2(Int2 placeCell, int kind, bool big)>;

// What a column's blocks are made from (the game's 0x58-byte column field, §3.1).
struct ColumnField {
    Place place;
    float base = 0, height = 0, lake = 0, river = 0;
    ColourInputs colours; // highlands, warmth, dryness, vegetation, roads, lavaRegion, forestRegion, kind27, lava, swamp
};

// A column's stored blocks, from `base` up.
struct TerrainColumn {
    int32_t base = 0;
    std::vector<Block> blocks;
    int32_t top() const { return base + (int32_t)blocks.size(); }
    bool empty() const { return blocks.empty(); }
};

// A light the cliff pass hangs in a tall overhang (a model in the zone's model list, zone+0x18;
// its record is part B's: type 0x59, flags 2, tint 1, second float 1).
struct OverhangLight {
    int64_t x, y, z; // 1/65536 blocks
    float size;      // the gap's height in blocks
};

struct ZoneTerrain {
    int32_t zx = 0, zy = 0;
    Place place;                       // the zone's place (its cell's, with no reach limit)
    Int2 placeCell;                    // that cell, evened when big
    Int2 anchor = NO_ANCHOR;           // the big structure's anchor
    std::vector<ColumnField> field;    // ZONE_COLUMNS, index localX * 64 + localY
    std::vector<TerrainColumn> columns; // likewise
    std::vector<OverhangLight> lights;
    CrtRandom random;                  // the zone's rand() stream, for part B to continue
    std::vector<uint16_t> draws;       // every rand() value drawn, when asked for
    SandOffsets sand;

    static int index(int32_t lx, int32_t ly) { return lx * ZONE_SIZE + ly; }
    bool contains(int32_t x, int32_t y) const {
        return floorDiv(x, ZONE_SHIFT) == zx && floorDiv(y, ZONE_SHIFT) == zy;
    }
    // The block lookup (0xeb90): solid grey outside the zone and below a column, sea water for
    // open air at or below sea level, air above.
    Block read(int32_t x, int32_t y, int32_t z) const;
    // Storing a block (0x31c1c0); the column grows down (filled with its buried colour) or up
    // (with empty blocks) to reach z.
    void store(int32_t x, int32_t y, int32_t z, Block b);
    void store(int32_t x, int32_t y, int32_t z, const BlockProps &p) { store(x, y, z, toBlock(p)); }
};

struct TerrainOptions {
    StructureAnchor anchor; // unset: no place has a big structure
    bool recordDraws = false;
};

// The terrain of zone (zx, zy) in the world given, unlit (the light pass is ZoneLight.h).
ZoneTerrain zoneTerrain(const WorldValues &world, int32_t zx, int32_t zy, const TerrainOptions &options = {});

// --- Rock spans (§5.8), for the zone and the world map ---------------------------------------

// Whether a place kind has rock spans: the arena (24) and the castles 45 and 46 (0x2b8010).
bool hasRockSpans(int kind);
// SPH: a place's own height (0x2ba200), at least 10.
float rockSpanHeight(const Place &p);
// A column's span, made of: the place's kind, the place, its structure's anchor (cells) and the
// span's base P0 (spanBaseHeight).
struct RockSpan {
    int kind;
    Place place;
    Int2 anchor;
    float spanBase;
};
// P0 at a column, from SPH and the place's variant.
float spanBaseHeight(const Place &p, float ownHeight, int32_t x, int32_t y);
int32_t spanBottom(const RockSpan &s, int32_t x, int32_t y, float h); // 0x2ba2b0
int32_t spanTop(const RockSpan &s, int32_t x, int32_t y, float h);    // 0x2bab80
BlockProps spanColour(const RockSpan &s, int32_t x, int32_t y, int32_t z, const SandOffsets &sand); // 0x2ba5f0

// A road crossing a walled pit (0x2ae6d0, §5.4 step 3): whether the column (x, y) of place p,
// with its roads weight, keeps an earth bridge, and its bottom and top.
bool roadPit(int32_t x, int32_t y, const Place &p, float roads, float &bottom, float &top);

// The sand offsets of a world.
inline SandOffsets sandOffsets(const WorldValues &world) { return {(double)world.sandX(), (double)world.sandY()}; }

} // namespace worldgen
