// Copied unchanged from SteamCubeWorldMods/TerrainVariance/TerrainRules.h (MIT, DieuDeGlace)
// so CubeVulkan's Variance style uses exactly the mod's height curve.
#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

// Pure height rules for TerrainVariance (shared with its tests).
namespace terrain {
// Quintic smoothstep on [0, 1] (zero first and second derivative at the ends).
inline double smooth(double t) {
    t = std::clamp(t, 0.0, 1.0);
    return t * t * t * (t * (t * 6 - 15) + 10);
}
// Deterministic 64-bit hash (splitmix64 finaliser).
inline uint64_t hash(int64_t x, int64_t y, uint64_t seed) {
    uint64_t h = (uint64_t)x * 0x9e3779b97f4a7c15ull ^ (uint64_t)y * 0xbf58476d1ce4e5b9ull ^ seed;
    h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9ull;
    h = (h ^ (h >> 27)) * 0x94d049bb133111ebull;
    return h ^ (h >> 31);
}
// Pseudo-random value in [-1, 1) for a lattice point.
inline double value(int64_t x, int64_t y, uint64_t seed) {
    return (hash(x, y, seed) >> 11) * (2.0 / 9007199254740992.0) - 1.0;
}
// Smoothly interpolated value noise in [-1, 1].
inline double noise(double x, double y, uint64_t seed) {
    int64_t ix = (int64_t)std::floor(x), iy = (int64_t)std::floor(y);
    double tx = smooth(x - ix), ty = smooth(y - iy);
    double a = value(ix, iy, seed), b = value(ix + 1, iy, seed), c = value(ix, iy + 1, seed),
           d = value(ix + 1, iy + 1, seed);
    return (a + (b - a) * tx) * (1 - ty) + (c + (d - c) * tx) * ty;
}
inline double variation(int64_t x, int64_t y, uint64_t seed, double amplitude, double scale) {
    // Favor broad hills over smaller ripples. A smooth upward bias makes tall
    // hills more common without exceeding the existing relief bounds or clipping
    // peaks into flat plateaus. The curve remains monotonic on [-1,1].
    double n = .80 * noise(x / scale, y / scale, seed) +
               .20 * noise(x / (scale * .5), y / (scale * .5), seed ^ 0x84222325ull);
    return amplitude * (n + .06 * (1 - n * n));
}
// Height offset in whole blocks for a column.
inline int columnOffset(int64_t x, int64_t y, uint64_t seed, double amplitude, double scale) {
    return (int)std::lround(variation(x, y, seed, amplitude, scale));
}
// Whole-block change of the surface when `addedHeight` is added.
inline int surfaceOffset(double nativeHeight, double addedHeight) {
    return (int)(std::floor(nativeHeight + addedHeight) - std::floor(nativeHeight));
}
// 0 at the edges of a mountain region (1024-block regions), rising to 1 160
// blocks inside, so mountains never create cliffs at region borders.
inline double mountainRegionWeight(int64_t x, int64_t y, bool mountains) {
    if (!mountains)
        return 0;
    auto local = [](int64_t p) {
        auto r = p % 1024;
        return double(r < 0 ? r + 1024 : r);
    };
    double lx = local(x), ly = local(y);
    return smooth(std::min({lx, ly, 1024 - lx, 1024 - ly}) / 160.);
}
// Blends the surrounding relief towards a raised summit as `locationInfluence` grows.
inline double mountainRelief(double surrounding, double amplitude, double locationInfluence) {
    double weight = smooth(locationInfluence);
    double n = amplitude > 0 ? std::clamp(surrounding / amplitude, -1., 1.) : 0;
    // Rounded positive summit, blending continuously into surrounding relief.
    double summit = amplitude * (.85 + .15 * n);
    return surrounding + (summit - surrounding) * weight;
}
// World fixed-point (65536 per block) to block, rounding towards -infinity.
inline int64_t blockFromDots(int64_t dots) {
    int64_t q = dots / 65536;
    return dots < 0 && dots % 65536 ? q - 1 : q;
}
// Moves a fixed-point z by `delta` blocks.
inline int64_t shiftedDetailZ(int64_t z, int delta) {
    return z + (int64_t)delta * 65536;
}
// Protected areas retain exactly their native heights; transitions have zero end slope.
inline double protectionWeight(double distance, double protectedRadius, double blendWidth) {
    if (distance <= protectedRadius)
        return 0;
    return smooth((distance - protectedRadius) / blendWidth);
}
// 0 at sea level, reaching 1 64 blocks higher: coasts keep their native height.
inline double shoreWeight(double nativeHeight) {
    return smooth((nativeHeight - 16.0) / 64.0);
}
} // namespace terrain
