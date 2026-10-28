#pragma once
// The world map's model of a map region and its markers (0x2eb7b0; docs/specs/worldmap-model.md,
// sections cited as §n).
//
// The world map shows the world as small voxel models, one per map region: a square of 16 x 16
// map units of `unit` blocks each (4 at zoom level 0, doubling every level), with a border of 3
// units each way. A voxel column samples the generator every `unit` blocks: its ground, water,
// rock spans, snow, the trees, buildings, towers, walls and rocks that reach it, then a haze at
// far zoom levels and a shade under overhangs. Beside the model, the region keeps the markers the
// map overlay reads (map-markers.md): places (shrines, lore buildings, treasure cells, a town's
// inn and shops), missions, land names and flight points, and the castle cells and region
// borders that fall in it. Nothing here draws rand(): a region is a function of the world seed,
// the region and the level.
//
//   const worldgen::MapRegion m = worldgen::mapRegion(rx, ry, level);
//   const uint8_t *v = m.voxel(i, j, k); // three bytes, all 0 for an empty voxel
#include "worldgen/Regions.h"
#include "worldgen/ZoneRecords.h"

#include <array>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>

namespace worldgen {

constexpr int MAP_REGION_UNITS = 16; // map units along a region's side (0x5469a0)
constexpr int MAP_BORDER = 3;        // units of border built around it (0x5469a8)
constexpr int MAP_MODEL_SIDE = MAP_REGION_UNITS + 2 * MAP_BORDER; // 22 columns each way
constexpr int MAP_MOST_VOXELS = 255; // the model's greatest height

// Blocks per map unit at zoom level L (§1): trunc(2^L x 4), at least 1.
int mapUnit(int level);
// How many 16-block cells around a column can reach it with their objects (§1): 2 for units
// below 32 (levels 0-2), else 1.
int mapReach(int unit);

struct MapRegion {
    int32_t rx = 0, ry = 0;
    int level = 0, unit = 4, reach = 2;
    // The model: MAP_MODEL_SIDE x MAP_MODEL_SIDE columns of sizeZ voxels, three bytes each, at
    // ((sizeZ index k * side + j) * side + i) * 3; column (i, j) is world column
    // (unit * (i - 3) + 16 * unit * rx, unit * (j - 3) + 16 * unit * ry). Voxel k stands
    // (k + lowest / unit) units up. All three bytes 0 is an empty voxel.
    int side = MAP_MODEL_SIDE, sizeZ = 0;
    int32_t lowest = 0; // the lowest column top, in blocks
    std::vector<uint8_t> rgb;
    // The model's base in map units (the region record's +0x08: lowest / unit, C division).
    int32_t base() const { return lowest / unit; }
    const uint8_t *voxel(int i, int j, int k) const { return &rgb[(((size_t)k * side + j) * side + i) * 3]; }

    // A marker (§5, §6), in one of the overlay's four lists (`list`): 0 a place (+0x18), 1 a
    // mission (+0x38), 2 a land (+0x48), 3 a flight point (+0x58). `markers` holds the four lists
    // one after the other, each in the order the game appends it.
    struct Marker {
        int list = 0;
        // Record +0x00: a place's kind (2..11 a town's inn and shops, 12 a shrine, 14 a lore
        // building, 15 a treasure cell), a mission's landmark kind (its name selector), a land's
        // biome; 0 for a flight point.
        int type = 0;
        int32_t x = 0, y = 0, z = 0; // blocks
        // A mission's role (0x93480) and level (0x12a200), and the artifact its place hides
        // (category 0 for none).
        int kind = 0, level = 0;
        Item item;
        // Not in the record, for the overlay's tests: a place's or mission's place cell (the
        // block position's 1024-block cell, evened when big, map-markers.md §0.5) and its big
        // flag; a land's 8192-block cell.
        int32_t px = 0, py = 0;
        bool big = false;
    };
    std::vector<Marker> markers;

    // The castle cells of big structures in the region (§6.6, list +0x68).
    struct CastleMark {
        std::array<uint8_t, 4> cell{}; // the castle cell's four bytes (0x2a8290)
        int32_t x = 0, y = 0;          // the cell's corner, blocks
        int32_t floor = 0;             // 0x2ab0a0
    };
    std::vector<CastleMark> castles;

    // The region borders crossing the model (§3.1, map +0x78): for each world region (8192-block
    // region key), the columns (x, y, top) where a neighbouring column belongs to another one.
    std::map<std::pair<int32_t, int32_t>, std::vector<std::array<int32_t, 3>>> borders;
};

// Map region (rx, ry) at zoom level `level`.
MapRegion mapRegion(int32_t rx, int32_t ry, int level);

} // namespace worldgen
