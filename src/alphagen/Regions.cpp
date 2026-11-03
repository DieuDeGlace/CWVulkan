// Adapted from AlphaVulkan (DieuDeGlace), GPL-3.0-only; see docs/alpha-generation.md.
#include "alphagen/Regions.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <mutex>

namespace alphagen {
namespace {

constexpr int32_t GRID = 1024; // regions per side
constexpr float TWO_POW_MINUS_16 = 1.f / 65536.f;

bool inWorld(int32_t rx, int32_t ry) {
    return rx >= 0 && ry >= 0 && rx < GRID && ry < GRID;
}
uint32_t keyOf(int32_t rx, int32_t ry) {
    return (uint32_t)rx * GRID + (uint32_t)ry;
}

// §2.2's value forms. Each makes its draw(s) when called, in float or double as the spec says.
float temperateMoisture(Random &r) {
    return (float)r.draw() * 0.4f / 32767.f + 0.3f;
}
float temperateWarmth(Random &r) {
    return (float)(r.draw() * 0.4 / 32767.0 + 0.4000000059604645);
}
float high(Random &r) {
    return (float)r.draw() * 0.1f / 32767.f + 0.9f;
} // 0.9 .. 1
float low(Random &r) {
    return (float)r.draw() * 0.1f / 32767.f;
} // 0 .. 0.1

// §5.1: squared distance from p to the segment a-b, in double, returned as float.
float segmentDistance(glm::dvec2 a, glm::dvec2 b, glm::dvec2 p) {
    const glm::dvec2 v = b - a;
    const double q = glm::dot(v, v);
    auto squared = [](glm::dvec2 d) { return glm::dot(d, d); };
    if (q < 9.999999682655225e-21)
        return (float)squared(p - a);
    const double t = glm::dot(p - a, v) / q;
    if (t <= 0)
        return (float)squared(p - a);
    if (t >= 1)
        return (float)squared(p - b);
    return (float)squared(p - (a + t * v));
}

} // namespace

World::World(uint32_t seed) : worldSeed(seed), o(makeSeedOffsets(seed)) {
}

const RegionCell *World::cell(int32_t rx, int32_t ry) const {
    if (!inWorld(rx, ry))
        return nullptr;
    std::shared_lock lock(cellsLock);
    const auto it = cells.find(keyOf(rx, ry));
    return it == cells.end() ? nullptr : it->second.get();
}

const RegionCell *World::makeCell(int32_t rx, int32_t ry) const {
    if (!inWorld(rx, ry))
        return nullptr;
    if (const RegionCell *existing = cell(rx, ry))
        return existing;

    auto c = std::make_unique<RegionCell>();
    const uint32_t a = (uint32_t)A();
    Random r((uint32_t)rx + 0x108au + 1024u * (uint32_t)ry + 3u * a);
    c->id = (int32_t)(1024u * (uint32_t)ry + (uint32_t)rx + a);
    const int32_t homeRegion = (HOME >> 8) >> 6;
    const bool home = rx == homeRegion && ry == homeRegion;

    // Steps 3-5: the climate draws.
    const int r1 = r.draw();
    if ((r1 & 1) == 0 || home) {
        c->moisture = temperateMoisture(r);
        c->warmth = temperateWarmth(r);
        if (!home && r.draw() % 10 == 0) { // a fire region
            c->fire = true;
            c->warmth = (float)r.draw() * 0.2f / 32767.f + 0.8f;
            c->moisture = (float)r.draw() * 0.5f / 32767.f;
        }
    } else {
        c->moisture = (r.draw() & 1) ? high(r) : low(r);
        c->warmth = (r.draw() & 1) ? high(r) : low(r);
    }
    // Step 6: the point, y drawn first; step 7: snapped to a sub-cell centre.
    if (home) {
        c->x = c->y = HOME;
    } else {
        c->y = REGION * ry + r.draw() % 15360 + 512;
        c->x = REGION * rx + 512 + r.draw() % 15360;
    }
    c->x = floorDiv(c->x, SUBCELL) * SUBCELL + SUBCELL / 2;
    c->y = floorDiv(c->y, SUBCELL) * SUBCELL + SUBCELL / 2;

    // Step 8: elevation from noise of the region index alone.
    const double Ad = (double)A();
    const float n1 = noise(1.4 * rx + Ad, 1.4 * ry + Ad + 843.0);
    const float f = (n1 + 1.f) * 100.f - 70.f;
    const float n2 = noise(4.0 * rx + Ad, 4.0 * ry + Ad + 843.0);
    c->elevation = (int32_t)(f + n2 * 30.f);

    // Step 9: sea cells.
    if (c->elevation <= 0) {
        c->fire = false;
        if (home) {
            c->elevation = r.draw() % 50 + 20; // the home region is always land
        } else {
            c->elevation = std::max(c->elevation - 100, -100);
            if (r.draw() & 1) {
                c->moisture = temperateMoisture(r);
                c->warmth = temperateWarmth(r);
            } else {
                c->moisture = high(r);
                c->warmth = high(r);
            }
        }
    }

    std::unique_lock lock(cellsLock);
    auto &slot = cells[keyOf(rx, ry)];
    if (!slot) // (another thread may have made it meanwhile; the result is the same)
        slot = std::move(c);
    return slot.get();
}

void World::makeCells(int32_t rx, int32_t ry) const {
    for (int32_t i = -2; i <= 2; i++)
        for (int32_t j = -2; j <= 2; j++)
            makeCell(rx + i, ry + j);
}

void World::prepareRegion(int32_t rx, int32_t ry, const std::function<float(int32_t, int32_t)> &height) const {
    if (!inWorld(rx, ry))
        return;
    std::shared_ptr<RegionSlot> slot;
    {
        std::unique_lock lock(regionsLock);
        auto &entry = regions[keyOf(rx, ry)];
        if (!entry)
            entry = std::make_shared<RegionSlot>();
        slot = entry;
    }
    std::call_once(slot->once, [&] {
        slot->region = buildRegion(rx, ry, height);
        slot->ready = slot->region.get(); // §2.2: visible only now
    });
}

const Region *World::region(int32_t rx, int32_t ry) const {
    if (!inWorld(rx, ry))
        return nullptr;
    std::shared_lock lock(regionsLock);
    const auto it = regions.find(keyOf(rx, ry));
    return it == regions.end() ? nullptr : it->second->ready.load();
}

const ZoneEntry *World::zoneEntry(int32_t zx, int32_t zy) const {
    const Region *r = region(zx >> 6, zy >> 6);
    return r ? &r->zones[64 * (zx & 63) + (zy & 63)] : nullptr;
}

glm::vec2 World::displacement(int32_t x, int32_t y) const {
    const float dx = noise(y * 0.0005, 3423.0) * 3.f * 256.f;
    const float dy = noise(x * 0.0005, 23421.0) * 3.f * 256.f;
    return {dx, dy};
}

void World::displaced(int32_t x, int32_t y, int32_t &px, int32_t &py) const {
    const glm::vec2 d = displacement(x, y);
    px = (int32_t)((float)x + d.x);
    py = (int32_t)((float)y + d.y);
}

int32_t World::squaredDistance(const RegionCell &c, int32_t px, int32_t py) {
    const float ex = (float)(((int64_t)c.x << 16) - ((int64_t)px << 16)) * TWO_POW_MINUS_16;
    const float ey = (float)(((int64_t)c.y << 16) - ((int64_t)py << 16)) * TWO_POW_MINUS_16;
    return (int32_t)(ey * ey + ex * ex);
}

bool World::ownerIndex(int32_t x, int32_t y, int32_t &oi, int32_t &oj) const {
    int32_t px, py;
    displaced(x, y, px, py);
    bool found = false;
    int32_t best = 0;
    for (int32_t i = floorDiv(x - REGION, REGION); i <= floorDiv(x + REGION, REGION); i++)
        for (int32_t j = floorDiv(y - REGION, REGION); j <= floorDiv(y + REGION, REGION); j++) {
            const RegionCell *c = cell(i, j);
            if (!c)
                continue;
            const int32_t d = squaredDistance(*c, px, py);
            if (!found || d < best) { // on a tie the first found stays
                found = true;
                best = d;
                oi = i;
                oj = j;
            }
        }
    return found;
}

bool World::landOwnsZone(int32_t lx, int32_t ly, int32_t zx, int32_t zy) const {
    const RegionCell *own = cell(lx, ly);
    if (!own)
        return false;
    int32_t px, py;
    displaced(zx * 256, zy * 256, px, py);
    const int32_t d0 = squaredDistance(*own, px, py);
    for (int32_t i = lx - 1; i <= lx + 1; i++)
        for (int32_t j = ly - 1; j <= ly + 1; j++)
            if (const RegionCell *c = (i == lx && j == ly) ? nullptr : cell(i, j))
                if (squaredDistance(*c, px, py) < d0)
                    return false;
    return true;
}

const RegionCell *World::owner(int32_t x, int32_t y) const {
    int32_t i, j;
    return ownerIndex(x, y, i, j) ? cell(i, j) : nullptr;
}

template <class Value> float World::blend(int32_t x, int32_t y, float fallback, Value value) const {
    int32_t px, py;
    displaced(x, y, px, py);
    const int32_t i0 = floorDiv(x - REGION, REGION), i1 = floorDiv(x + REGION, REGION);
    const int32_t j0 = floorDiv(y - REGION, REGION), j1 = floorDiv(y + REGION, REGION);
    // Nearest pass: any missing cell gives the default at once.
    const RegionCell *window[3][3];
    int32_t distances[3][3];
    int32_t nearest = std::numeric_limits<int32_t>::max();
    for (int32_t i = i0; i <= i1; i++)
        for (int32_t j = j0; j <= j1; j++) {
            const RegionCell *c = cell(i, j);
            if (!c)
                return fallback;
            window[i - i0][j - j0] = c;
            distances[i - i0][j - j0] = squaredDistance(*c, px, py);
            nearest = std::min(nearest, distances[i - i0][j - j0]);
        }
    // Blend pass: weights fall from 1 at the nearest cell to 0 at 2e6 further.
    float weights = 0, sum = 0;
    for (int32_t i = i0; i <= i1; i++)
        for (int32_t j = j0; j <= j1; j++) {
            const float t = std::min((float)(distances[i - i0][j - j0] - nearest) * 4.999999987e-07f, 1.f);
            const float w = 1.f - t;
            weights += w;
            sum += value(*window[i - i0][j - j0]) * w;
        }
    return weights > 0 ? sum / weights : fallback;
}

float World::warmth(int32_t x, int32_t y) const {
    return blend(x, y, 0.5f, [](const RegionCell &c) { return c.warmth; });
}

float World::moisture(int32_t x, int32_t y) const {
    float m = blend(x, y, 0.5f, [](const RegionCell &c) { return c.moisture; });
    // The oasis rule: dry ground near a type-3 location is wetter.
    if (m < 0.2f)
        if (const Location *L = location(x, y); L && L->type == 3) {
            const float v = 1.f - locationDistance(*L, x, y);
            const float b = v > 0 ? v * v : 0.f;
            m = std::min(m + 0.3f * b, 1.f);
        }
    return m;
}

float World::seaShare(int32_t x, int32_t y) const {
    // Pre-scan: a missing cell, or no sea cell at all, gives 0 without blending.
    bool any = false;
    for (int32_t i = floorDiv(x - REGION, REGION); i <= floorDiv(x + REGION, REGION); i++)
        for (int32_t j = floorDiv(y - REGION, REGION); j <= floorDiv(y + REGION, REGION); j++) {
            const RegionCell *c = cell(i, j);
            if (!c)
                return 0;
            any = any || c->elevation < 0;
        }
    return any ? blend(x, y, 0.f, [](const RegionCell &c) { return c.elevation < 0 ? 1.f : 0.f; }) : 0.f;
}

float World::fireShare(int32_t x, int32_t y) const {
    bool any = false;
    for (int32_t i = floorDiv(x - REGION, REGION); i <= floorDiv(x + REGION, REGION); i++)
        for (int32_t j = floorDiv(y - REGION, REGION); j <= floorDiv(y + REGION, REGION); j++) {
            const RegionCell *c = cell(i, j);
            if (!c)
                return 0;
            any = any || c->fire;
        }
    return any ? blend(x, y, 0.f, [](const RegionCell &c) { return c.fire ? 1.f : 0.f; }) : 0.f;
}

glm::dvec2 World::channelNoise(int32_t x, int32_t y) const {
    auto n = [&](uint32_t ox, uint32_t oy, double k) { return (double)noise((double)o[ox] + k * x, (double)o[oy] + k * y); };
    return {0.1 * n(0x204, 0x208, 0.01) + n(0x1fc, 0x200, 0.0005), 0.1 * n(0x214, 0x218, 0.01) + n(0x20c, 0x210, 0.0005)};
}

float World::channel(int32_t x, int32_t y) const {
    int32_t ci, cj;
    if (!ownerIndex(x, y, ci, cj))
        return 1;
    const RegionCell *own = cell(ci, cj);
    if (!own)
        return 1;
    const glm::dvec2 sums = channelNoise(x, y);
    const glm::dvec2 p(x + 500.0 * sums.x, y + 500.0 * sums.y);
    const glm::dvec2 from(own->x, own->y);
    bool found = false;
    float best = 0;
    const int32_t neighbours[4][2] = {{ci - 1, cj}, {ci + 1, cj}, {ci, cj - 1}, {ci, cj + 1}};
    for (const auto &n : neighbours)
        if (const RegionCell *other = cell(n[0], n[1])) {
            const float s = segmentDistance(from, glm::dvec2(other->x, other->y), p);
            if (!found || s < best) {
                found = true;
                best = s;
            }
        }
    return found ? best * 0.0002f : 1.f;
}

glm::dvec2 World::grid(int32_t x, int32_t y) const {
    constexpr double K = 6.103515625e-5; // 1 / 16384
    const glm::dvec2 sums = channelNoise(x, y);
    return {x * K + 500.0 * K * sums.x, y * K + 500.0 * K * sums.y};
}

const Location *World::location(int32_t x, int32_t y) const {
    // §6.3: the record of the column's 2048-block sub-cell.
    const int32_t lx = floorDiv(x, SUBCELL), ly = floorDiv(y, SUBCELL);
    if (lx < 0 || ly < 0 || lx >= 8192 || ly >= 8192)
        return nullptr;
    const Region *r = region(lx >> 3, ly >> 3);
    if (!r)
        return nullptr;
    const Location &L = r->records[8 * (lx & 7) + (ly & 7)];
    return L.type ? &L : nullptr;
}

float World::locationDistance(const Location &L, int32_t x, int32_t y) const {
    return locationDistanceFx(L, (int64_t)x << 16, (int64_t)y << 16);
}

float World::locationDistanceFx(const Location &L, int64_t X, int64_t Y) const {
    if (L.radius < 0.001f)
        return 0;
    float ex, ey;
    if (L.type == 11 || L.type == 12 || L.type == 14) {
        ex = (float)(X - L.x) * TWO_POW_MINUS_16;
        ey = (float)(Y - L.y) * TWO_POW_MINUS_16;
        return (ex * ex + ey * ey) / (L.radius * L.radius);
    }
    // The distorted rim (§7 steps 1-5).
    const int64_t Tx = (int64_t)((double)X * -0.0025), Ty = (int64_t)((double)Y * -0.0025);
    const double a = (double)(0x80AD580000LL - Tx) / 65536.0, b = (double)(0x1617D0000LL - Ty) / 65536.0;
    const float k = L.type == 13 ? 100.f : 256.f;
    const int64_t DX = (int64_t)(noise(a, b) * k * 65536.f) - L.x + X;
    const double a2 = (double)(0xD5F0000LL - Tx) / 65536.0, b2 = (double)(0x700000LL - Ty) / 65536.0;
    const int64_t DY = (int64_t)(noise(a2, b2) * k * 65536.f) - L.y + Y;
    ex = (float)DX * TWO_POW_MINUS_16;
    ey = (float)DY * TWO_POW_MINUS_16;
    return (ey * ey + ex * ex) / (L.radius * L.radius);
}

} // namespace alphagen
