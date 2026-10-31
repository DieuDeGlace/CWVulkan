#pragma once
// Regions: the world's 8192-block cells (docs/specs/worldgen-terrain.md §2). Each region has
// a centre placed at random inside it and a biome. After a noise warp, a point belongs to the
// region whose centre is nearest (a warped Voronoi diagram).
#include <cstdint>

namespace worldgen {

struct Int2 {
    int32_t x = 0, y = 0;
    bool operator==(const Int2 &o) const { return x == o.x && y == o.y; }
    bool operator!=(const Int2 &o) const { return !(*this == o); }
};

// The biome numbers the terrain treats specially (§2.2). Their names belong to the game's data.
namespace biomes {
constexpr int OCEAN = 1;
constexpr int ORIGIN = 2; // the 3 x 3 regions around the origin
constexpr int LAKE = 7;
constexpr int WET = 8; // wide rivers
constexpr int LAVA = 13;
} // namespace biomes

// A region's centre in blocks (0x2b9210).
Int2 regionCentre(int32_t rx, int32_t ry);
// A region's biome, 0..13 (0x2b9080).
int regionBiome(int32_t rx, int32_t ry);
// The region a point belongs to, before landmarks (0x2ad4d0).
Int2 regionOf(int32_t x, int32_t y);
// The region a point belongs to (0x2ada20): near a landmark, the landmark's own region.
Int2 regionAt(int32_t x, int32_t y);
// Physical fixed coordinates, with the effective block's landmark flag (native0x2ad8f0).
Int2 regionAtFixed(int64_t x, int64_t y, bool landmarkBlock);
// The biome of regionAt.
inline int biomeAt(int32_t x, int32_t y) {
    const Int2 r = regionAt(x, y);
    return regionBiome(r.x, r.y);
}

// The 3 x 3 regions around a point (0x358e0, the game's 0x58-byte record): their biomes and
// squared (warped) distances, indexed [a][b] for region (rx - 1 + a, ry - 1 + b).
struct Neighbourhood {
    int32_t biome[3][3];
    float dist2[3][3];
    int32_t x, y;             // the point, unwarped
    float nearestDist2 = -1;  // the smallest distance
    int32_t nearestBiome = 0; // and that region's biome
};
Neighbourhood neighbourhood(int32_t x, int32_t y);

// How near a point is to regions of some biomes, 0..1 (height spec §3.1): from the smallest
// distance d to such a region, 1 - ((d - nearest) * k) * k, at least 0; 0 if there is none.
float regionWeight(const Neighbourhood &n, const int *biomesIn, int count, float k);
// The smallest distance to a region of one of `biomesIn`, -1 if there is none.
float nearestOf(const Neighbourhood &n, const int *biomesIn, int count);

} // namespace worldgen
