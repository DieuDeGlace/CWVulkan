// Regions, written from docs/specs/worldgen-terrain.md §2 (biome table: worldgen-terrain-tables.md §2).
#include "worldgen/Regions.h"

#include "worldgen/Noise.h"
#include "worldgen/Options.h"
#include "worldgen/Places.h"

#include <cmath>

namespace worldgen {
namespace {

constexpr int REGION_SHIFT = 13; // 8192 blocks
constexpr int32_t REGION_SIZE = 8192;
constexpr int32_t CENTRE_MARGIN = 1024; // a centre lies 1024..7167 blocks into its region
constexpr uint32_t CENTRE_SPREAD = 6144;
constexpr uint32_t CENTRE_X_SALT = 0xe29e;
constexpr uint32_t CENTRE_Y_SALT = 0x70532;
constexpr uint32_t BIOME_SALT = 0xb3f58;
constexpr float DOT = 1.0f / 65536.0f; // 1.52587890625e-05

// Biome by R mod 50 (tables §2).
constexpr int BIOME_BY_DRAW[50] = {1, 2, 2, 2, 2, 0, 0, 0, 9, 9,  10, 10, 4, 4, 13, 11, 11, 6, 6, 3, 3, 3, 3, 8, 8,
                                   8, 7, 7, 12, 12, 5, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
static_assert(BIOME_BY_DRAW[31] == 5 && BIOME_BY_DRAW[49] == 1, "the biome table has 50 entries");

// A point moved by noise before regions are looked up (§2.3), in dots.
struct Warped {
    int64_t x, y;
};
Warped warp(int32_t x, int32_t y) {
    float a = noiseAt(x, y, 0.0003, 8432, 0.0006, 9438);
    a = a * 0.125f * 8192.0f;
    float b = noiseAt(x, y, 0.0006, 567, 0.0003, 1223);
    b = b * 0.125f * 8192.0f;
    return {trunc64(a * 65536.0f) + (int64_t)x * 65536, trunc64(b * 65536.0f) + (int64_t)y * 65536};
}

// The squared distance from a warped point to a region's centre (§2.3).
float dist2To(const Warped &p, Int2 centre) {
    const float dx = (float)(p.x - (int64_t)centre.x * 65536) * DOT;
    const float dy = (float)(p.y - (int64_t)centre.y * 65536) * DOT;
    return dy * dy + dx * dx;
}

Int2 warpedRegion(const Warped &p) {
    return {floorDiv(floorDots(p.x), REGION_SHIFT), floorDiv(floorDots(p.y), REGION_SHIFT)};
}

// CubeVulkan's world scale (Options.h): the point pulled toward the origin before the region
// lookups, and the squared distances they find given back in full-size blocks. At scale 1 the
// point and distances are the game's own.
Int2 scaledPoint(int32_t x, int32_t y, double scale) {
    if (scale == 1.0)
        return {x, y};
    return {(int32_t)std::floor((double)x / scale), (int32_t)std::floor((double)y / scale)};
}
float fullSize(float dist2, double scale) { return scale == 1.0 ? dist2 : (float)((double)dist2 * scale * scale); }

} // namespace

Int2 regionCentre(int32_t rx, int32_t ry) {
    const uint32_t k = cellKey(rx, ry);
    return {(int32_t)((uint32_t)rx * REGION_SIZE + CENTRE_MARGIN + random(k + CENTRE_X_SALT) % CENTRE_SPREAD),
            (int32_t)((uint32_t)ry * REGION_SIZE + CENTRE_MARGIN + random(k + CENTRE_Y_SALT) % CENTRE_SPREAD)};
}

int regionBiome(int32_t rx, int32_t ry) {
    if ((uint32_t)rx + 1u < 3u && (uint32_t)ry + 1u < 3u)
        return biomes::ORIGIN;
    return BIOME_BY_DRAW[random(cellKey(rx, ry) + BIOME_SALT) % 50u];
}

Int2 regionOf(int32_t x, int32_t y) {
    const Int2 s = scaledPoint(x, y, worldScale());
    const Warped p = warp(s.x, s.y);
    const Int2 home = warpedRegion(p);
    Int2 best;
    float bestDist2 = 0;
    bool first = true;
    for (int32_t i = home.x - 1; i <= home.x + 1; i++)
        for (int32_t j = home.y - 1; j <= home.y + 1; j++) {
            const float d = dist2To(p, regionCentre(i, j));
            if (first || d < bestDist2)
                best = {i, j}, bestDist2 = d, first = false;
        }
    return best;
}

Int2 regionAt(int32_t x, int32_t y) {
    const int32_t cx = floorDiv(x, PLACE_SHIFT), cy = floorDiv(y, PLACE_SHIFT);
    const bool big = bigCell(cx, cy);
    if (placeKind(cx, cy, big) != 0) {
        const Int2 point = cellPoint(cx, cy, big);
        const float d = warpedDist2(point, x, y);
        const int32_t r = placeRadius(big);
        const float r2 = (float)(r * r);
        if (d < r2) {
            const float f = 1.0f - d / r2;
            if (0.25f <= f * f)
                return regionOf(point.x, point.y);
        }
    }
    return regionOf(x, y);
}

Neighbourhood neighbourhood(int32_t x, int32_t y) {
    Neighbourhood n;
    const double scale = worldScale();
    const Int2 s = scaledPoint(x, y, scale);
    const Warped p = warp(s.x, s.y);
    const Int2 home = warpedRegion(p);
    n.x = x, n.y = y;
    n.nearestDist2 = -1.0f;
    n.nearestBiome = 0;
    for (int a = 0; a < 3; a++)
        for (int b = 0; b < 3; b++) {
            const int32_t rx = home.x - 1 + a, ry = home.y - 1 + b;
            const float d = fullSize(dist2To(p, regionCentre(rx, ry)), scale);
            const int biome = regionBiome(rx, ry);
            n.biome[a][b] = biome;
            n.dist2[a][b] = d;
            if (n.nearestDist2 < 0.0f || d < n.nearestDist2)
                n.nearestDist2 = d, n.nearestBiome = biome;
        }
    return n;
}

float nearestOf(const Neighbourhood &n, const int *biomesIn, int count) {
    float best = -1.0f;
    for (int a = 0; a < 3; a++)
        for (int b = 0; b < 3; b++)
            for (int k = 0; k < count; k++)
                if (n.biome[a][b] == biomesIn[k]) {
                    if (best < 0.0f || n.dist2[a][b] < best)
                        best = n.dist2[a][b];
                    break;
                }
    return best;
}

float regionWeight(const Neighbourhood &n, const int *biomesIn, int count, float k) {
    const float d = nearestOf(n, biomesIn, count);
    if (d >= 0.0f && d >= n.nearestDist2) {
        const float v = 1.0f - ((d - n.nearestDist2) * k) * k;
        return v > 0.0f ? v : 0.0f;
    }
    return 0.0f;
}

} // namespace worldgen
