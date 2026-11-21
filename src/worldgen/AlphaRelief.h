#pragma once
// CubeVulkan's Alpha terrain: the whole Steam world (its terrain, rivers, roads, trees, places and
// materials) with Alpha's hills added to its ground height. Not the game's: the user's add-on, as in
// their Infinite-Cubes Vulkan work. Roads, places, landmarks and channel beds keep their Steam grade
// and the shore keeps its height; the relief climbs away from each at a bounded slope.
#include "alphagen/Random.h"
#include "worldgen/Height.h"
#include "worldgen/WorldValues.h"

#include <cstdint>
#include <memory>
#include <string>

namespace worldgen {

class AlphaRelief {
  public:
    // steamSeed: the Steam world's seed; landscapeSeed: Alpha's world seed, whose offsets shape the hills.
    explicit AlphaRelief(int32_t steamSeed, uint32_t landscapeSeed = 1570745101);
    // A save and multiplayer identity for this terrain at a biome scale.
    std::string identity(double scale) const;
    // The blocks added to the Steam ground height at a column, from its height pipeline values (c.height
    // the Steam height). 0 on roads, places, landmarks and channel beds and below the shore.
    float offset(int32_t x, int32_t y, const ColumnValues &c) const;
    // False for an ordinary tree (cell types 1-5) whose crown the hills would bury in the slope beside it,
    // or that stands so close to a neighbouring tree that their crowns would merge. Zones and the map skip it.
    bool treeFits(int32_t cx, int32_t cy, int type) const;

    // The parts, for checks.
    float relief(int32_t x, int32_t y) const; // Alpha's hills, -48 .. 160, before the biome strength
    static float strength(const Neighbourhood &regions); // 1 + 0.25 mountains - 0.4 savannah
    static float liftCap(float distance);                // the highest relief this far from a flat edge
    static float smoothMin(float a, float b, float smoothing);
    float channelDistance(int32_t x, int32_t y, const ColumnValues &c) const;
    float placeDistance(int32_t x, int32_t y) const;
    float featureDistance(int32_t x, int32_t y) const;
    // Alpha's lattice noise with its cell found by floor (equal to alphagen::noise on positive coordinates,
    // continuous below zero).
    static float latticeNoise(double x, double y);

  private:
    const int32_t steamSeed;
    const uint32_t landscapeSeed;
    const WorldValues world;
    const alphagen::SeedOffsets offsets;
};

// The relief in use for generated Steam heights, or null for the Steam world as it is.
void setAlphaRelief(std::shared_ptr<const AlphaRelief>);
const AlphaRelief *alphaRelief();

} // namespace worldgen
