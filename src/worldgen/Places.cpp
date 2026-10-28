// Places and their landmark kinds, written from docs/specs/worldgen-terrain.md §3 and the kind
// tables in worldgen-terrain-tables.md §3 and §4.
#include "worldgen/Places.h"

#include "worldgen/Height.h"
#include "worldgen/Noise.h"
#include "worldgen/Options.h"

namespace worldgen {
namespace {

constexpr uint32_t BIG_SALT = 0x39a13d;
constexpr uint32_t POINT_X_SALT = 0x7ab2c;
constexpr uint32_t POINT_Y_SALT = 0x271537;
constexpr uint32_t KIND_SALT = 0x2acfe1;
constexpr uint32_t RING_SALT = 0x51c64b;
constexpr uint32_t VARIANT_SALT = 0x12ce2f3;
constexpr int32_t SMALL_MARGIN = 282, SMALL_SPREAD = 460; // a small landmark lies 282..741 into its cell
constexpr int32_t BIG_MARGIN = 532, BIG_SPREAD = 984;     // a big one 532..1515 into its 2 x 2 cells
constexpr float DOT = 1.0f / 65536.0f;

// The "sevenths" test (0x36210) on a half-resolution cell: (hx + 5 hy) mod 7 = 1.
bool sevenths(int32_t hx, int32_t hy) {
    const int32_t v = (int32_t)((uint32_t)hx + (uint32_t)hy * 5u);
    return ((v % 7) + 7) % 7 == 1;
}

// The even corner of a big place.
int32_t evened(int32_t c) { return 2 * floorDiv(c, 1); }

// The ground the landmark kind tables test: the plain base height and the region's biome at a
// cell's landmark point, for a given big flag.
struct LandmarkGround {
    int32_t cx, cy;
    float height(bool big) const {
        const Int2 p = cellPoint(cx, cy, big);
        return plainBaseHeight(p.x, p.y);
    }
    int biome(bool big) const {
        const Int2 p = cellPoint(cx, cy, big);
        const Int2 r = regionOf(p.x, p.y);
        return regionBiome(r.x, r.y);
    }
};

// The "shore" rule (§3.4): just below sea level a shore landmark (36), deeper the fallback.
int shore(float h, int fallback) { return h > -10.0f ? 36 : fallback; }

// A small cell's kind from its table row m (tables §3).
int smallKind(int m, const LandmarkGround &g, bool own) {
    switch (m) {
    case 0: return 7;
    case 1: return 17;
    case 2: return g.biome(false) == biomes::LAVA ? 21 : 20;
    case 3: return 21;
    case 4: return 22;
    case 5: return 1;
    case 6: return 9;
    case 7: {
        const float h = g.height(false);
        return h >= 0.0f ? 18 : shore(h, 0);
    }
    case 8: {
        const float h = g.height(own);
        if (h >= 0.0f)
            return g.biome(own) == biomes::LAVA ? 14 : 13;
        return shore(h, 0);
    }
    case 9: {
        const float h = g.height(false);
        return h >= 0.0f ? 14 : shore(h, 0);
    }
    case 10: return 2;
    case 11: return 16;
    case 12: return 29;
    case 13: return 4;
    case 14: {
        const float h = g.height(false);
        return h >= 0.0f ? 30 : shore(h, 23);
    }
    case 15: {
        const int b = g.biome(own);
        if (b < 3 || b > 6 || b == 4)
            return 32;
        const float h = g.height(false);
        return h >= 0.0f ? 33 : shore(h, 23);
    }
    case 16: return 32;
    case 17: {
        const float h = g.height(false);
        return h >= 0.0f ? 31 : shore(h, 23);
    }
    case 18: return 34;
    case 19: return 15;
    case 20: return 9;
    case 21: return 38;
    case 22: return 37;
    case 23: return 39;
    default: return 0;
    }
}

// A big place's kind from its table row m (tables §4); the cell is already evened.
int bigKind(int m, const LandmarkGround &g, bool own) {
    switch (m) {
    case 0: return 10;
    case 1: {
        const int b = g.biome(own);
        return b == 4 || b == 10 ? 46 : b == 9 ? 45 : 8;
    }
    case 2: return 24;
    case 3: return 15;
    case 4: return 42;
    case 5: return 44;
    case 6: return 47;
    case 7: return 19;
    case 8: return 21;
    case 9: return g.height(true) < 10.0f ? 0 : 14;
    case 10:
        if (g.height(own) >= 10.0f)
            return g.biome(own) == biomes::LAVA ? 14 : 13;
        return 0;
    case 11: return 9;
    case 12: {
        const int b = g.biome(own);
        if (b < 3 || b > 6)
            return 25;
        const float h = g.height(true);
        return h >= 0.0f ? 33 : shore(h, 23);
    }
    case 13: return g.height(true) < 50.0f ? 0 : 28;
    case 14: return 29;
    case 15: {
        const float h = g.height(true);
        return h >= 0.0f ? 30 : shore(h, 23);
    }
    case 16: {
        const float h = g.height(true);
        return h >= 0.0f ? 31 : shore(h, 23);
    }
    case 17: return 26;
    case 18: return 27;
    default: return 0; // a negative row (R wrapped below zero)
    }
}

int computePlaceKind(int32_t cx, int32_t cy, bool big) {
    if (cx == 0 && cy == 0)
        return 1; // the starting place
    if (!big) {
        const int32_t r = (int32_t)random(cellKey(cx, cy) + KIND_SALT);
        if (r % 4 == 0)
            return 0;
        const int32_t q = r / 4; // truncating, as the game (0x2b9eaf): 0 for r = -1..-3
        const bool own = bigCell(cx, cy);
        const LandmarkGround g{cx, cy};
        if ((q & 1) == 0 && g.height(own) < 10.0f)
            return 0;
        return smallKind((q / 2) % 24, g, own);
    }
    cx = evened(cx), cy = evened(cy);
    if (sevenths(floorDiv(cx, 1), floorDiv(cy, 1)))
        return 5;
    const int32_t m = (int32_t)random(cellKey(cx, cy) + KIND_SALT) % 19;
    return bigKind(m, LandmarkGround{cx, cy}, bigCell(cx, cy));
}

// Landmark kinds are asked for again and again (every column's place, the region of a point,
// roads), and finding some takes the ground's height. A small cache per thread keeps the last
// answers; a cell's kind depends on nothing but its coordinates (and the world scale).
struct KindCache {
    struct Entry {
        int32_t cx, cy;
        int8_t big = -1, kind = 0;
    };
    Entry entries[256];
    uint32_t version = 0;
    int lookup(int32_t cx, int32_t cy, bool big) {
        if (version != optionsVersion()) {
            for (Entry &e : entries)
                e.big = -1;
            version = optionsVersion();
        }
        Entry &e = entries[(cellKey(cx, cy) * 2u + (big ? 1u : 0u)) & 255u];
        if (e.big != (big ? 1 : 0) || e.cx != cx || e.cy != cy)
            e = {cx, cy, (int8_t)(big ? 1 : 0), (int8_t)computePlaceKind(cx, cy, big)};
        return e.kind;
    }
};
thread_local KindCache kindCache;

// The kinds that have only "now and then" variants (4 one time in ten), and those with none.
bool ringOnlyKind(int kind) {
    switch (kind) {
    case 2: case 5: case 9: case 12: case 15: case 21: case 43: case 45: case 46: return true;
    default: return false;
    }
}
bool plainKind(int kind) {
    switch (kind) {
    case 10: case 13: case 14: case 17: case 18: case 19: case 23: case 25: case 28: case 30: case 31: case 33:
    case 36: case 42: case 44: case 47: return true;
    default: return false;
    }
}

} // namespace

bool bigCell(int32_t cx, int32_t cy) {
    const int32_t hx = floorDiv(cx, 1), hy = floorDiv(cy, 1);
    return !oddHash((uint32_t)hy * 57u + BIG_SALT + (uint32_t)hx) || sevenths(hx, hy);
}

Int2 cellPoint(int32_t cx, int32_t cy, bool big) {
    if (cx == 0 && cy == 0)
        return {512, 512};
    if (!big) {
        const uint32_t k = cellKey(cx, cy);
        return {(int32_t)((uint32_t)cx * 1024u + SMALL_MARGIN + random(k + POINT_X_SALT) % SMALL_SPREAD),
                (int32_t)((uint32_t)cy * 1024u + SMALL_MARGIN + random(k + POINT_Y_SALT) % SMALL_SPREAD)};
    }
    const int32_t ex = evened(cx), ey = evened(cy);
    const uint32_t k = cellKey(ex, ey);
    return {(int32_t)((uint32_t)ex * 1024u + BIG_MARGIN + random(k + POINT_X_SALT) % BIG_SPREAD),
            (int32_t)((uint32_t)ey * 1024u + BIG_MARGIN + random(k + POINT_Y_SALT) % BIG_SPREAD)};
}

LandmarkWarp landmarkWarp(int32_t x, int32_t y) {
    return {noiseAt(x, y, 0.005, 18843, 0.01, 9432) * 32.0f, noiseAt(x, y, 0.01, 299, 0.005, 90650) * 32.0f};
}

float warpedDist2(Int2 landmark, int32_t x, int32_t y) {
    const LandmarkWarp w = landmarkWarp(x, y);
    const float dx = (float)(((int64_t)x - landmark.x) * 65536 + trunc64(w.x * 65536.0f)) * DOT;
    const float dy = (float)(((int64_t)y - landmark.y) * 65536 + trunc64(w.y * 65536.0f)) * DOT;
    return dy * dy + dx * dx;
}

int placeKind(int32_t cx, int32_t cy, bool big) {
    const int kind = kindCache.lookup(cx, cy, big);
    return modHooksSet() ? hookedPlaceKind(cx, cy, big, kind) : kind;
}

int placeVariant(int32_t cx, int32_t cy, int kind, bool big) {
    if (big)
        cx = evened(cx), cy = evened(cy);
    const uint32_t k = cellKey(cx, cy);
    if (ringOnlyKind(kind))
        return random(k + RING_SALT) % 10u == 0 ? 4 : 0;
    if (plainKind(kind))
        return 0;
    switch (random(k + VARIANT_SALT) % 10u) {
    case 0: return 4;
    case 1: return 1;
    case 2: return 5;
    case 3: return 2;
    default: return 0;
    }
}

float placeWeight(int kind, bool big, float d2) {
    if (kind == 0)
        return 0.0f;
    const int32_t r = placeRadius(big);
    const float r2 = (float)(r * r);
    if (!(d2 < r2))
        return 0.0f;
    const float f = 1.0f - d2 / r2;
    return f * f;
}

bool samePlace(const Place &a, const Place &b) {
    if (a.kind == 0 || b.kind == 0)
        return a.kind == b.kind;
    return a.kind == b.kind && a.big == b.big && a.variant == b.variant && a.radius == b.radius && a.point == b.point &&
           a.dist2 == b.dist2 && a.weight == b.weight;
}

Place placeAt(int32_t x, int32_t y) {
    Place p;
    const int32_t cx = floorDiv(x, PLACE_SHIFT), cy = floorDiv(y, PLACE_SHIFT);
    p.big = bigCell(cx, cy);
    p.point = cellPoint(cx, cy, p.big);
    // Beyond reach of the landmark (unwarped), no place.
    const int32_t reach = p.big ? BIG_MARGIN : SMALL_MARGIN;
    const int32_t dx = x - p.point.x, dy = y - p.point.y;
    if (reach * reach < dx * dx + dy * dy)
        return p;
    p.kind = placeKind(cx, cy, p.big);
    if (p.kind == 0)
        return p;
    p.radius = placeRadius(p.big);
    p.dist2 = warpedDist2(p.point, x, y);
    p.weight = placeWeight(p.kind, p.big, p.dist2);
    p.variant = placeVariant(cx, cy, p.kind, p.big);
    return p;
}

Place cellPlace(int32_t cx, int32_t cy) {
    Place p;
    p.big = bigCell(cx, cy);
    p.kind = placeKind(cx, cy, p.big);
    if (p.kind == 0)
        return p;
    p.radius = placeRadius(p.big);
    p.point = cellPoint(cx, cy, p.big);
    p.variant = placeVariant(cx, cy, p.kind, p.big);
    return p;
}

Place placeForColumn(const Place &cell, int32_t x, int32_t y) {
    Place p = cell;
    if (p.kind == 0) {
        p.dist2 = 0, p.weight = 0;
        return p;
    }
    p.dist2 = warpedDist2(p.point, x, y);
    p.weight = placeWeight(p.kind, p.big, p.dist2);
    return p;
}

} // namespace worldgen
