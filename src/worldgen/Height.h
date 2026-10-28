#pragma once
// The height of a point (docs/specs/worldgen-terrain-height.md): the game's 0x35ea0 and every
// weight it is built from. A point's regions and place give a base height and weights for
// lakes, lava, forest, roads, rivers, swamps and highlands; the final height rolls, terraces
// and shapes the ground with them.
#include "worldgen/Places.h"
#include "worldgen/Regions.h"

#include <cstdint>

namespace worldgen {

// Mode bits of the height (the game's fourth argument, §1).
namespace heightmode {
constexpr int NO_VARIANT_SHAPES = 1; // the road over a pit
constexpr int LOW = 2;               // no mountain rise, variant shapes or highland peaks (the cliff pass)
constexpr int CLIFF_STEPS = 4;       // structures stand on cliff steps
constexpr int NO_BEDS = 8;           // no river beds or shores
} // namespace heightmode

// --- The steps of the pipeline (§1 table), each the game routine named -------------------

// The base height (0x2a0840): ocean and lake regions, relief noise, a place's own ground.
float baseHeight(const Neighbourhood &n, int kind, float weight);
// The plain base height of a point, with no place (0x2ba260).
float plainBaseHeight(int32_t x, int32_t y);
// A place's own height: the base height at its landmark point with weight 1 (0x2ba200).
float placeHeight(const Place &p);
// The base height with the place around the point (0x8a140).
float baseHeightWithPlace(int32_t x, int32_t y);

float lakeRegion(const Neighbourhood &n);                              // 0x2bb7e0
float wetRegion(const Neighbourhood &n);                               // 0x2e64e0: biome 8
float dryRegion(const Neighbourhood &n);                               // 0x2a44a0: biomes 9, 13
float lavaRegion(const Neighbourhood &n, int kind, float weight);      // 0x2b9750
float forestRegion(const Neighbourhood &n, int kind, float weight);    // 0x2d9f30
float dryness(const Neighbourhood &n);                                 // 0x2b5960
float roads(int32_t x, int32_t y, int kind, float weight);             // 0x2d4730: 1 on a road
float river(int32_t x, int32_t y, float roads, float lavaRegion, float forestRegion, float wet, const Place &p); // 0x2e5f80
float lava(int32_t x, int32_t y, float roads, float lavaRegion, float lake, const Place &p);                    // 0x2b92f0
float swamp(int32_t x, int32_t y, float roads, float forestRegion, float lake, const Place &p);                 // 0x2d01a0
float highlands(int32_t x, int32_t y, float base, float lake, float river, float lava, float swamp, float roads,
                const Place &p);                                                                                // 0x2bb310
float vegetation(int32_t x, int32_t y, float dryness, float dryRegion);                                        // 0x2d36f0
// The cliffs factor of highlands (0x2a2c50).
float cliffs(int32_t x, int32_t y, int kind, float weight);
// Low ground at the shore (0x2b7730).
bool lowShore(int32_t x, int32_t y, float h);

// What the final height (0x2b4450) takes.
struct HeightInputs {
    float base, highlands, river, lava, swamp, roads, vegetation;
};
float finalHeight(int32_t x, int32_t y, const HeightInputs &in, const Place &p, int mode);

// --- The whole pipeline ---------------------------------------------------------------------

// Every value of the pipeline at a column, for a given place record.
struct ColumnValues {
    Neighbourhood regions;
    Place place;
    float lake = 0, base = 0, roads = 0, lavaRegion = 0, forestRegion = 0, wet = 0;
    float river = 0, lava = 0, swamp = 0, highlands = 0, dryRegion = 0, dryness = 0, vegetation = 0;
    float height = 0;
    HeightInputs heightInputs() const { return {base, highlands, river, lava, swamp, roads, vegetation}; }
};
ColumnValues columnValues(int32_t x, int32_t y, const Place &place, int mode = 0);
// The pipeline with the point's own place (0x35ea0).
inline ColumnValues columnValues(int32_t x, int32_t y, int mode = 0) { return columnValues(x, y, placeAt(x, y), mode); }
// The ground height at a column (0x35ea0).
float groundHeight(int32_t x, int32_t y, int mode = 0);

} // namespace worldgen
