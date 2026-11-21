#include "worldgen/AlphaRelief.h"

#include "alphagen/Coordinates.h"
#include "worldgen/BuildingRules.h"
#include "worldgen/Options.h"
#include "worldgen/Structures.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <vector>

namespace worldgen {
namespace {

constexpr float SHORE_CLEARANCE = 20.0f;
constexpr float RELIEF_CENTRE = 48.0f;
constexpr float MAXIMUM_RISE = 160.0f;
// The strongest biome scale (mountains) on the highest relief.
constexpr float MAXIMUM_LIFT = MAXIMUM_RISE * 1.25f;
// Relief rises at most this much per block away from a road, place or landmark it leaves flat.
constexpr float MAXIMUM_BLEND_SLOPE = 1.1f;
// The cap starts level at the edge and reaches its full slope this many blocks out.
constexpr float BLEND_RAMP = 24.0f;
// Past this distance from a flat edge the cap no longer limits any relief.
constexpr int ANCHOR_REACH = int(MAXIMUM_LIFT / MAXIMUM_BLEND_SLOPE + BLEND_RAMP) + 1;
// Smooths the crease where the capped ramp meets the relief.
constexpr float CAP_SMOOTHING = 24.0f;
// Roads stay flat to where their weight leaves a street; the warp can squeeze distances, so 70% is trusted.
constexpr float ROAD_FLAT_DISTANCE = 29.0f;
constexpr float ROAD_DISTANCE_TRUST = 0.7f;
// Places stay flat to 81% of their radius; water or lava rings (variant 4) to their whole radius.
constexpr float PLACE_FLAT_FRACTION = 0.81f;
// Above the shore the relief may grow by this much per block the Steam ground rises.
constexpr float SHORE_RELIEF_SLOPE = 1.5f;
// Landmark footprints: rocks, the big tree and hills.
constexpr int ROCK_RADIUS = 80;
constexpr int CELL = 16, ZONE = 64, CELLS_PER_ZONE = ZONE / CELL;

std::atomic<const AlphaRelief *> current{nullptr};
std::shared_ptr<const AlphaRelief> owner;

float fadeIn(float value, float start, float end) {
    const float t = std::clamp((value - start) / (end - start), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

int32_t floorDivide(int32_t a, int32_t n) { return a >= 0 ? a / n : -((-a + n - 1) / n); }

bool ordinaryTree(int type) { return type >= 1 && type <= 5; } // common, fir, birch, willow, palm

struct Footprint {
    int32_t x, y, radius;
};

// The relief's own cell objects per thread: the shared cellObjectsFor() cache is rebuilt when another world
// asks for it, which would leave a caller's reference (the map's, a zone's) dangling mid-build.
CellObjects &reliefCells(const WorldValues &world) {
    struct Cache {
        std::unique_ptr<CellObjects> cells;
        const WorldValues *owner = nullptr;
        uint32_t version = 0;
    };
    thread_local Cache cache;
    if (!cache.cells || cache.owner != &world || cache.version != optionsVersion()) {
        cache.cells = std::make_unique<CellObjects>(world);
        cache.owner = &world, cache.version = optionsVersion();
    }
    return *cache.cells;
}

std::vector<Footprint> footprints(const WorldValues &world, int32_t zx, int32_t zy) {
    constexpr int MARGIN = ANCHOR_REACH + ROCK_RADIUS + CELL;
    constexpr int PADDING = MARGIN / CELL;
    BuildingRules rules(reliefCells(world));
    std::vector<Footprint> result;
    for (int dx = -PADDING; dx < CELLS_PER_ZONE + PADDING; ++dx)
        for (int dy = -PADDING; dy < CELLS_PER_ZONE + PADDING; ++dy) {
            const int32_t cx = zx * CELLS_PER_ZONE + dx, cy = zy * CELLS_PER_ZONE + dy;
            const int type = rules.type(cx, cy);
            if ((type == 0 || ordinaryTree(type)) && rules.castleCell(cx, cy).kind == 0)
                continue;
            int radius = (cellReach(type) + 1) * CELL;
            if (type == 9 || type == 54 || type == 55) // boulder, stones, crag
                radius = ROCK_RADIUS + CELL;
            else if (type == 8 || (type >= 10 && type <= 15)) // the big tree, hills
                radius = ROCK_RADIUS;
            result.push_back({cx * CELL + CELL / 2, cy * CELL + CELL / 2, radius});
        }
    return result;
}

// The channel noises at a block; generation reads each column's and its neighbours' many times.
struct Ridges {
    int32_t x = INT32_MIN, y = INT32_MIN;
    float wet = -1.0f;
    float river = -1.0f, lava = -1.0f, swamp = -1.0f;
};
Ridges &ridgesAt(int32_t x, int32_t y) {
    thread_local std::array<Ridges, 4096> cache;
    auto &entry = cache[(uint32_t(x) * 73856093u ^ uint32_t(y) * 19349663u) % cache.size()];
    if (entry.x != x || entry.y != y)
        entry = Ridges{x, y};
    return entry;
}
float riverAt(int32_t x, int32_t y, float wet) {
    Ridges &r = ridgesAt(x, y);
    if (r.river < 0.0f || r.wet != wet)
        r.river = riverRidge(x, y, wet), r.wet = wet;
    return r.river;
}
float lavaAt(int32_t x, int32_t y) {
    Ridges &r = ridgesAt(x, y);
    if (r.lava < 0.0f)
        r.lava = lavaRidge(x, y);
    return r.lava;
}
float swampAt(int32_t x, int32_t y) {
    Ridges &r = ridgesAt(x, y);
    if (r.swamp < 0.0f)
        r.swamp = swampRidge(x, y);
    return r.swamp;
}

constexpr int MOUNTAINS[] = {7};
constexpr int SAVANNAH[] = {10};

} // namespace

AlphaRelief::AlphaRelief(int32_t steamSeed, uint32_t landscapeSeed)
    : steamSeed(steamSeed), landscapeSeed(landscapeSeed), world(worldValues(steamSeed)),
      offsets(alphagen::makeSeedOffsets(landscapeSeed)) {}

std::string AlphaRelief::identity(double scale) const {
    return "steam-alpha-relief-v1-seed-" + std::to_string(steamSeed) + "-landscape-" + std::to_string(landscapeSeed) +
           "-scale-bits-" + std::to_string(std::bit_cast<uint64_t>(scale));
}

float AlphaRelief::latticeNoise(double x, double y) {
    constexpr double PI = 3.1415927; // Alpha's double
    const double fx = std::floor(x), fy = std::floor(y);
    const uint32_t ux = uint32_t(int32_t(fx)), uy = uint32_t(int32_t(fy));
    const auto value = [](uint32_t n) { return alphagen::latticeValue(int32_t(n)); };
    const double wx = (1.0 - std::cos((x - fx) * PI)) * 0.5;
    const double wy = (1.0 - std::cos((y - fy) * PI)) * 0.5;
    const double row = value(ux + 57u * uy) * (1.0 - wx) + value(ux + 1u + 57u * uy) * wx;
    const double nextRow = value(ux + 57u * (uy + 1u)) * (1.0 - wx) + value(ux + 1u + 57u * (uy + 1u)) * wx;
    return float(row * (1.0 - wy) + nextRow * wy);
}

float AlphaRelief::relief(int32_t x, int32_t y) const {
    const double ax = double(x) + alphacoords::HOME, ay = double(y) + alphacoords::HOME;
    const auto noise = [&](uint32_t ox, uint32_t oy, double scale) {
        return latticeNoise(offsets[ox] + ax * scale, offsets[oy] + ay * scale);
    };
    const auto weight = [&](uint32_t ox, uint32_t oy, double scale) {
        const float value = alphagen::unit(noise(ox, oy, scale));
        return value * value;
    };
    // The broad hills and roughness terms of Alpha's height.
    const float broad = (noise(0x1bc, 0x1c0, 0.0002) + 1.0f) * 100.0f * weight(0x194, 0x198, 0.0001) +
                        (noise(0x1b4, 0x1b8, 0.0002) + 1.0f) * 100.0f * weight(0x18c, 0x190, 0.0001);
    const float roughness = std::abs(noise(0x168, 0x16c, 0.001) + 0.1f * noise(0x170, 0x174, 0.01));
    const float gate = fadeIn(roughness, 0.0f, 0.25f);
    const float hills = (noise(0x1cc, 0x1d0, 0.002) + 1.0f) * 50.0f * weight(0x1a4, 0x1a8, 0.001) +
                        (noise(0x1c4, 0x1c8, 0.002) + 1.0f) * 50.0f * weight(0x19c, 0x1a0, 0.001) +
                        (noise(0x1d4, 0x1d8, 0.01) + 1.0f) * 20.0f * weight(0x1ac, 0x1b0, 0.002);
    return std::clamp(broad + gate * gate * hills - RELIEF_CENTRE, -RELIEF_CENTRE, MAXIMUM_RISE);
}

float AlphaRelief::strength(const Neighbourhood &regions) {
    // The falloffs of the Steam mountain elevation (0.0003) and of the regions (0.0004).
    const float mountains = fadeIn(regionWeight(regions, MOUNTAINS, 1, 0.0003f), 0.0f, 1.0f);
    const float savannah = fadeIn(regionWeight(regions, SAVANNAH, 1, 0.0004f), 0.0f, 1.0f);
    return 1.0f + 0.25f * mountains - 0.4f * savannah;
}

float AlphaRelief::liftCap(float distance) {
    if (distance <= 0.0f)
        return 0.0f;
    if (distance < BLEND_RAMP * 2.0f)
        return MAXIMUM_BLEND_SLOPE * distance * distance / (BLEND_RAMP * 4.0f);
    return MAXIMUM_BLEND_SLOPE * (distance - BLEND_RAMP);
}

float AlphaRelief::smoothMin(float a, float b, float smoothing) {
    const float h = std::max(smoothing - std::abs(a - b), 0.0f) / smoothing;
    return std::min(a, b) - h * h * smoothing * 0.25f;
}

float AlphaRelief::channelDistance(int32_t x, int32_t y, const ColumnValues &c) const {
    // A bed is flat where the channel's weight reaches this.
    constexpr float FLAT_WEIGHT = 0.30f;
    // The first-order estimate runs long where the noise steepens towards the bed.
    constexpr float TRUST = 0.8f;
    // About the steepest change per block of the doubled lava and forest weights at a biome border.
    constexpr float STRENGTH_GRADIENT = 0.006f;
    // Past this many blocks the relief is not limited, so the ridge is not sampled around.
    constexpr float REACH = 600.0f;
    constexpr int STEP = 2;
    float nearest = REACH;
    // strength: the channel's weight on its centre line. fading: a stand-in that keeps falling past the
    // border where the strength reaches 0, so the distance to where the channel fades in stays continuous.
    const auto measure = [&](float strength, float fading, auto ridgeAt) {
        const float fadeInDistance = (FLAT_WEIGHT - fading) / STRENGTH_GRADIENT;
        if (fadeInDistance >= nearest)
            return;
        // The ridge at which the profile times the strength reaches FLAT_WEIGHT; a fading bed narrows to its line.
        const float flat = strength > FLAT_WEIGHT ? 1.0f - std::sqrt(FLAT_WEIGHT / strength) : 0.0f;
        const float centre = ridgeAt(x, y) - flat;
        float distance = centre;
        if (centre > 0.0f) {
            // The steeper one-sided difference on each axis stays continuous across the noise's creases.
            const auto slope = [&](int dx, int dy) {
                const float before = ridgeAt(x - dx, y - dy), after = ridgeAt(x + dx, y + dy);
                return std::max(std::abs(after - centre - flat), std::abs(centre + flat - before)) / float(STEP);
            };
            // Where a channel fades its threshold moves too: add its steepest rate.
            const float tip = std::max(strength, FLAT_WEIGHT);
            const float thresholdSlope = 0.5f * std::sqrt(FLAT_WEIGHT / tip) / tip * STRENGTH_GRADIENT;
            const float gradient = std::hypot(slope(STEP, 0), slope(0, STEP)) + thresholdSlope;
            distance = centre / gradient * TRUST;
        }
        nearest = std::min(nearest, std::max(distance, fadeInDistance));
    };
    const float lavaRegion = c.lavaRegion, forest = c.forestRegion, wet = c.wet;
    measure((1.0f - std::min(lavaRegion + lavaRegion, 1.0f)) * (1.0f - std::min(forest + forest, 1.0f)),
            1.0f - 2.0f * (lavaRegion + forest), [&](int32_t px, int32_t py) { return riverAt(px, py, wet); });
    const float lava = (lavaRegion - 0.5f) * 2.0f * (1.0f - c.lake);
    measure(lava, lava, [](int32_t px, int32_t py) { return lavaAt(px, py); });
    const float swamp = (forest - 0.5f) * 2.0f * (1.0f - c.lake);
    measure(swamp, swamp, [](int32_t px, int32_t py) { return swampAt(px, py); });
    return nearest;
}

float AlphaRelief::placeDistance(int32_t x, int32_t y) const {
    struct PlaceCore {
        Int2 point;
        float core;
    };
    struct Entry {
        const AlphaRelief *owner = nullptr;
        uint32_t version = 0;
        int32_t cx = 0, cy = 0;
        std::vector<PlaceCore> places;
    };
    thread_local std::array<Entry, 16> cache;
    const int32_t cx = floorDivide(x, 1 << PLACE_SHIFT), cy = floorDivide(y, 1 << PLACE_SHIFT);
    auto &entry = cache[(uint32_t(cx) * 7u + uint32_t(cy)) % cache.size()];
    if (entry.owner != this || entry.version != optionsVersion() || entry.cx != cx || entry.cy != cy) {
        entry = {this, optionsVersion(), cx, cy, {}};
        for (int32_t i = cx - 1; i <= cx + 1; ++i)
            for (int32_t j = cy - 1; j <= cy + 1; ++j) {
                const bool big = bigCell(i, j);
                const int kind = placeKind(i, j, big);
                if (kind == 0)
                    continue;
                const float radius = float(placeRadius(big));
                const bool ring = placeVariant(i, j, kind, big) == 4;
                entry.places.push_back({cellPoint(i, j, big), ring ? radius : radius * PLACE_FLAT_FRACTION});
            }
    }
    float nearest = float(ANCHOR_REACH);
    for (const PlaceCore &place : entry.places) {
        const float gap = std::sqrt(warpedDist2(place.point, x, y)) - place.core;
        if (gap <= 0.0f)
            return 0.0f;
        nearest = std::min(nearest, gap);
    }
    return nearest;
}

float AlphaRelief::featureDistance(int32_t x, int32_t y) const {
    struct Entry {
        const AlphaRelief *owner = nullptr;
        uint32_t version = 0;
        int32_t zx = 0, zy = 0;
        std::vector<Footprint> features;
    };
    thread_local std::array<Entry, 64> cache;
    const int32_t zx = floorDivide(x, ZONE), zy = floorDivide(y, ZONE);
    auto &entry = cache[(uint32_t(zx) * 17u + uint32_t(zy)) % cache.size()];
    if (entry.owner != this || entry.version != optionsVersion() || entry.zx != zx || entry.zy != zy)
        entry = {this, optionsVersion(), zx, zy, footprints(world, zx, zy)};
    // Round footprints: boxes left straight ramps that read as sliced hillsides.
    float nearest = float(ANCHOR_REACH);
    for (const Footprint &feature : entry.features) {
        const float dx = float(std::abs(int64_t(x) - feature.x)), dy = float(std::abs(int64_t(y) - feature.y));
        const float radius = float(feature.radius);
        if (dx - radius >= nearest || dy - radius >= nearest)
            continue;
        const float gap = std::sqrt(dx * dx + dy * dy) - radius;
        if (gap <= 0.0f)
            return 0.0f;
        nearest = std::min(nearest, gap);
    }
    return nearest;
}

float AlphaRelief::offset(int32_t x, int32_t y, const ColumnValues &c) const {
    // Cheapest bounds first: the shore, then roads.
    const float height = c.height;
    float cap = std::min(SHORE_RELIEF_SLOPE * (height - SHORE_CLEARANCE),
                         liftCap((roadDistance(x, y, c.place.weight) - ROAD_FLAT_DISTANCE) * ROAD_DISTANCE_TRUST));
    if (cap <= 0.0f)
        return 0.0f;
    const float raw = relief(x, y) * strength(c.regions);
    if (raw == 0.0f)
        return 0.0f;
    const auto tighten = [&cap](float distance) {
        cap = std::min(cap, liftCap(distance));
        return cap > 0.0f;
    };
    if (!tighten(channelDistance(x, y, c)) || !tighten(placeDistance(x, y)) || !tighten(featureDistance(x, y)))
        return 0.0f;
    const float magnitude = std::max(0.0f, smoothMin(std::abs(raw), cap, std::min(CAP_SMOOTHING, cap)));
    // Keep valleys above the shore. Only the combined height is quantized, when columns are written.
    return std::max(raw < 0.0f ? -magnitude : magnitude, SHORE_CLEARANCE - height);
}

namespace {
// A tree's size numbers (structures spec §7 step 5): its spread (salt 0x2fbcc94, 20..29; the map draws the
// crown from it, birches narrower) and its height (salt 0x12ce32c, 12..21; firs half as tall again).
float crownRadius(int32_t cx, int32_t cy, int type) {
    const float radius = float(cellRandom(cx, cy, 0x2fbcc94) % 10 + 20) * 0.5f;
    return type == 3 ? radius * 0.75f : radius;
}
float riseAllowed(int32_t cx, int32_t cy, int type) {
    const float allowed = float(cellRandom(cx, cy, 0x12ce32c) % 10 + 12) * 0.8f;
    return type == 2 ? allowed * 1.5f : allowed;
}
struct TreeTest {
    const AlphaRelief *owner = nullptr;
    uint32_t version = 0;
    int32_t cx = INT32_MIN, cy = INT32_MIN;
    int type = 0;
    bool result = true;
};
template <typename Test> bool cachedTree(std::array<TreeTest, 256> &cache, const AlphaRelief *owner, int32_t cx, int32_t cy, int type, Test test) {
    auto &entry = cache[(uint32_t(cx) * 31u + uint32_t(cy)) % cache.size()];
    if (entry.owner != owner || entry.version != optionsVersion() || entry.cx != cx || entry.cy != cy || entry.type != type)
        entry = {owner, optionsVersion(), cx, cy, type, test()};
    return entry.result;
}
// Where the ground beside the trunk rises past most of the trunk, the crown sinks into the hill.
bool clearsSlope(const AlphaRelief *owner, int32_t cx, int32_t cy, int type) {
    thread_local std::array<TreeTest, 256> cache;
    return cachedTree(cache, owner, cx, cy, type, [&] {
        const Int2 p = cellPoint(cx, cy);
        const float ground = groundHeight(p.x, p.y, heightmode::CLIFF_STEPS);
        const float reach = crownRadius(cx, cy, type) * 0.9f, allowed = riseAllowed(cx, cy, type);
        constexpr float D = 0.70710678f;
        constexpr float DIRECTIONS[8][2] = {{1, 0}, {D, D}, {0, 1}, {-D, D}, {-1, 0}, {-D, -D}, {0, -1}, {D, -D}};
        for (const auto &d : DIRECTIONS)
            if (groundHeight(p.x + int32_t(std::lround(d[0] * reach)), p.y + int32_t(std::lround(d[1] * reach)), 0) - ground > allowed)
                return false;
        return true;
    });
}
} // namespace

bool AlphaRelief::treeFits(int32_t cx, int32_t cy, int type) const {
    if (!ordinaryTree(type))
        return true;
    thread_local std::array<TreeTest, 256> cache;
    return cachedTree(cache, this, cx, cy, type, [&] {
        if (!clearsSlope(this, cx, cy, type))
            return false;
        // Of two trees whose crowns would merge, the one with the higher cell hash stays (ties: the higher cell).
        const Int2 p = cellPoint(cx, cy);
        const float reach = crownRadius(cx, cy, type);
        const uint32_t priority = cellRandomUnsigned(cx, cy, 873211);
        CellObjects &cells = reliefCells(world);
        for (int32_t i = cx - 1; i <= cx + 1; ++i)
            for (int32_t j = cy - 1; j <= cy + 1; ++j) {
                if (i == cx && j == cy)
                    continue;
                const uint32_t other = cellRandomUnsigned(i, j, 873211);
                if (other < priority || (other == priority && (i < cx || (i == cx && j < cy))))
                    continue;
                const int otherType = cells.type(i, j);
                if (!ordinaryTree(otherType))
                    continue;
                const Int2 q = cellPoint(i, j);
                const float dx = float(q.x - p.x), dy = float(q.y - p.y);
                const float apart = (reach + crownRadius(i, j, otherType)) * 0.75f;
                if (dx * dx + dy * dy < apart * apart && clearsSlope(this, i, j, otherType))
                    return false;
            }
        return true;
    });
}

void setAlphaRelief(std::shared_ptr<const AlphaRelief> relief) {
    owner = std::move(relief);
    current.store(owner.get(), std::memory_order_release);
}

const AlphaRelief *alphaRelief() { return current.load(std::memory_order_acquire); }

} // namespace worldgen
