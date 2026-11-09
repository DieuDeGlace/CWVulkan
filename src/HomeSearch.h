#pragma once
// Bounded engine driver for the held native home-search contract. The native loop is unbounded.
#include "NativeSaves.h"
#include "worldgen/Height.h"
#include <functional>

namespace homesearch {
using Cell = nativesaves::Cell;
enum class Weight { LavaRegion, ForestRegion, LakeRegion, DryRegion, WetRegion };
enum class Channel { River, Lava, Swamp };
struct Terrain {
    worldgen::Place place;
    float lavaRegion = 0, forestRegion = 0, lake = 0, dryRegion = 0, wet = 0, roads = 0;
};
struct World {
    // Coarse grid -> home cell; home cells are sixteen blocks wide.
    std::function<Cell(int32_t, int32_t)> areaCell;
    std::function<int(int32_t, int32_t)> cellObject; // home coordinates
    std::function<float(int32_t, int32_t)> ground;   // mode-zero height at integer blocks
    std::function<Cell(int32_t, int32_t)> region;    // block coordinates ->8192-grid pair
    std::function<int(int32_t, int32_t)> regionType;
    std::function<worldgen::Neighbourhood(int32_t, int32_t)> neighbours;
    std::function<worldgen::Place(int32_t, int32_t)> place;
    std::function<float(Weight, const worldgen::Neighbourhood &, const worldgen::Place &)> weight;
    std::function<float(int32_t, int32_t, const worldgen::Place &)> roads;
    std::function<float(Channel, int32_t, int32_t, const Terrain &)> channel;
    std::function<Cell(int32_t, int32_t)> excludedPlace; // earlier regional pair -> descriptor+80 pair
    // Explicit captured/Alpha coverage policy. Empty for ordinary native Steam.
    std::function<bool(Cell)> playable;
    bool ready() const;
};
World nativeWorld();
bool fits(const World &, Cell home);
bool find(const World &, const std::function<int()> &random, Cell coarseOrigin, int radiusFrom, int radiusSpread,
          int maxAttempts, Cell &out);
Cell block(Cell home);
std::array<int64_t, 3> position(Cell home, float height);
} // namespace homesearch
