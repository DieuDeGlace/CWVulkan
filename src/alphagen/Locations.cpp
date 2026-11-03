// Adapted from AlphaVulkan (DieuDeGlace), GPL-3.0-only; see docs/alpha-generation.md.
// Region preparation (docs/specs/alpha-worldgen-locations.md): the region header, the 8 x 8
// location records and the 64 x 64 per-zone table, in the order and with the draws the spec
// gives. Section numbers point at the locations spec.
#include "alphagen/Regions.h"
#include <algorithm>
#include <cmath>

namespace alphagen {
namespace {

float f(double v) {
    return (float)v;
}
int64_t blk(int64_t fx) {
    return fx / 65536;
} // towards zero; positions are positive here

// §6.1 step 9: the sub-level from the level.
int32_t subLevelOf(int32_t level, Random &r) {
    if (level < 5)
        return 0;
    if (level < 10)
        return r.draw() & 1;
    if (level < 15)
        return r.draw() % 3;
    if (level < 19)
        return r.draw() % 3 + 1;
    return (r.draw() & 3) + 1;
}

// §4.2 step 3: a centre for a disc of radius r inside slot origin (ox, oy), y drawn first.
void place(Location &L, float r, int32_t ox, int32_t oy, Random &random) {
    const float r256 = f(r + 256.f);
    const float span = f(2048.f - f(r256 * 2.f));
    auto offset = [&] { return (int64_t)f(f(f((float)random.draw() * span) / 32767.f) * 65536.f); };
    L.y = ((int64_t)oy << 16) + (int64_t)f(r * 65536.f) + (1LL << 24) + offset();
    L.x = ((int64_t)ox << 16) + (int64_t)f(r256 * 65536.f) + offset();
}

// §6.1 step 2: the centre of the zone it lies in.
void snapToZone(Location &L) {
    L.x = ((blk(L.x) / 256) * 256 + 128) << 16;
    L.y = ((blk(L.y) / 256) * 256 + 128) << 16;
}

ZoneEntry &entryAt(Region &region, int64_t fx, int64_t fy) {
    return region.zones[64 * ((blk(fx) >> 8) & 63) + ((blk(fy) >> 8) & 63)];
}

} // namespace

std::unique_ptr<Region> World::buildRegion(int32_t rx, int32_t ry, const std::function<float(int32_t, int32_t)> &H) const {
    // §2.1 steps 1-3.
    makeCells(rx, ry);
    const RegionCell *C = cell(rx, ry);
    Random random((uint32_t)A() + 1024u * (uint32_t)ry + (uint32_t)rx);
    auto region = std::make_unique<Region>();
    if (!C) {
        region->draws = random.count();
        return region;
    }

    // §3: header and level range.
    if (rx == 512 && ry == 512) {
        region->tier = 1;
    } else {
        const float dx = (float)(512 - rx), dy = (float)(512 - ry);
        const float d = f(std::sqrt((double)f(dy * dy + dx * dx)));
        region->tier = 2 - (int32_t)f(d * -0.75f);
    }
    if (region->tier > 4)
        region->h10 = random.draw() % 5;
    region->style = random.draw() & 3;
    int32_t serial = random.draw() % 10000;
    if (C->warmth > 0.8100000023841858f)
        region->style = 4 + (random.draw() & 1);
    int32_t minLevel = 1;
    if (C->warmth < 0.2f)
        minLevel = 10;
    if (C->moisture < 0.2f && C->warmth > 0.8f)
        minLevel = 15;
    if (C->moisture > 0.8f && C->warmth > 0.8f)
        minLevel = 10;
    if (C->fire)
        minLevel = 20;

    // §4: phase A, the slots.
    struct Candidate {
        float key;
        int32_t i, j;
    };
    std::vector<Candidate> candidates;
    const int32_t cxs = (C->x >> 11) & 7, cys = (C->y >> 11) & 7;
    for (int32_t i = 0; i < 8; i++)
        for (int32_t j = 0; j < 8; j++) {
            Location &R = region->records[8 * i + j];
            const int32_t ox = REGION * rx + SUBCELL * i, oy = REGION * ry + SUBCELL * j;
            if (named && i == cxs && j == cys) { // §4.1: the town
                R.type = 1;
                R.variant = region->style;
                R.serial = C->id;
                R.radius = (float)(random.draw() % 200 + 512);
                R.x = (int64_t)C->x << 16;
                R.y = (int64_t)C->y << 16;
                const int64_t rFx = (int64_t)(R.radius * 65536.f);
                if (R.x - rFx - (1LL << 24) < ((int64_t)ox << 16))
                    R.x = (int64_t)(f(f(f((float)ox + R.radius) + 256.f)) * 65536.f);
                if (R.y - rFx - (1LL << 24) < ((int64_t)oy << 16))
                    R.y = (int64_t)(f(f(f((float)oy + R.radius) + 256.f)) * 65536.f);
                if (R.x + rFx + (1LL << 24) > ((int64_t)(ox + SUBCELL) << 16))
                    R.x = (int64_t)(f(f(f((float)(ox + SUBCELL) - R.radius) - 256.f)) * 65536.f);
                if (R.y + rFx + (1LL << 24) > ((int64_t)(oy + SUBCELL) << 16))
                    R.y = (int64_t)(f(f(f((float)(oy + SUBCELL) - R.radius) - 256.f)) * 65536.f);
                R.height = std::max(H((int32_t)blk(R.x), (int32_t)blk(R.y)), 0.f);
                continue;
            }
            if (owner(ox + 1024, oy + 1024) != C)
                continue;
            R.radius = (float)((random.draw() & 255) + 512);
            place(R, R.radius, ox, oy, random);
            R.height = H((int32_t)blk(R.x), (int32_t)blk(R.y));
            R.level = minLevel;
            if (owner((int32_t)blk(R.x), (int32_t)blk(R.y)) != C)
                continue;
            const float dx = (float)(cxs - i), dy = (float)(cys - j);
            const float d = f(std::sqrt((double)f(f(dy * dy) + f(dx * dx))));
            const float key = f(f(f(f((float)random.draw() * 4.f) / 32767.f) + d) - 2.f);
            candidates.push_back({key, i, j});
        }

    // §5: phase B, ascending by key.
    std::sort(candidates.begin(), candidates.end(), [](const Candidate &a, const Candidate &b) { return a.key < b.key; });

    // §6: phase C, the ranked assignment.
    const bool sea = C->elevation < 0;
    for (int32_t c = 0; c < 64 && !candidates.empty(); c++) {
        if (c & 1)
            continue;
        const Candidate taken = candidates.front();
        candidates.erase(candidates.begin());
        Location &R = region->records[8 * taken.i + taken.j];
        if (R.height < 0)
            R.height = 0;
        const float t = f(c * 0.015625f);
        R.level = (int32_t)f(f(f(f(1.f / f(1.f - t)) - 1.f) * 20.f) + 1.f);
        if ((c / 2) & 1) {
            // §6.1: a dungeon entrance.
            R.type = 14;
            R.radius = 150;
            snapToZone(R);
            ZoneEntry &E = entryAt(*region, R.x, R.y);
            E.kind = 3;
            E.variant = (uint8_t)(random.draw() & 3);
            const int32_t zx = (int32_t)blk(R.x), zy = (int32_t)blk(R.y);
            if (warmth(zx, zy) > 0.8f)
                E.variant = (uint8_t)(4 + (moisture(zx, zy) <= 0.8f ? 1 : 0));
            E.id = random.draw() % 10000000 + 1;
            E.level = R.level;
            R.serial = serial;
            serial += 1 + random.draw() % 50;
            R.variant = E.variant;
            E.subLevel = (uint8_t)subLevelOf(R.level, random);
            continue;
        }
        // §6.2: the generic switch.
        switch (random.draw() & 7) {
        case 0:
            R.type = 2;
            break;
        case 1:
            R.type = 3;
            R.variant = random.draw() % 3;
            break;
        case 2:
            R.type = sea ? 15 : 4;
            break;
        case 3:
            R.type = 5;
            R.variant = C->warmth > 0.8f ? (C->moisture > 0.8f ? 3 : (C->moisture < 0.2f ? 2 : 0)) : 0;
            R.radius = (float)((random.draw() & 255) + 256);
            place(R, R.radius, REGION * rx + SUBCELL * taken.i, REGION * ry + SUBCELL * taken.j, random);
            break;
        case 4:
            R.type = sea ? 15 : 6;
            break;
        case 5:
            R.type = sea ? 15 : 7;
            break;
        case 6:
            R.type = 11;
            R.radius = 128;
            snapToZone(R);
            break;
        case 7:
            R.type = 12;
            R.radius = 128;
            snapToZone(R);
            break;
        }
        R.subLevel = subLevelOf(R.level, random);
        R.serial = serial;
        serial += 1 + random.draw() % 50;
    }

    // §7: phase D, up to five type-10 sites from what is left.
    for (int n = 0; n < 5 && !candidates.empty(); n++) {
        const uint32_t k = (uint32_t)random.draw() % (uint32_t)candidates.size();
        const Candidate taken = candidates[k];
        candidates.erase(candidates.begin() + k);
        Location &R = region->records[8 * taken.i + taken.j];
        R.type = 10;
        R.serial = random.draw() % 10000000 + 1;
        ZoneEntry &E = entryAt(*region, R.x, R.y);
        E.kind = 4;
        E.id = R.serial;
    }

    // §8: phase E, levels into the table (phase F's discarded heights are skipped).
    for (const Location &R : region->records)
        entryAt(*region, R.x, R.y).level = R.level;

    // §8.2: phase G, the town's four quarters.
    for (int32_t i = 0; i < 8; i++)
        for (int32_t j = 0; j < 8; j++) {
            const Location &R = region->records[8 * i + j];
            if (R.type != 1)
                continue;
            struct Quarter {
                float w;
                int32_t zxl, zyl;
            };
            std::vector<Quarter> quarters;
            for (int32_t a = 0; a < 8; a++)
                for (int32_t b = 0; b < 8; b++) {
                    const int32_t X = REGION * rx + SUBCELL * i + 256 * a + 128;
                    const int32_t Y = REGION * ry + SUBCELL * j + 256 * b + 128;
                    const float v = 1.f - locationDistance(R, X, Y);
                    quarters.push_back({v > 0 ? v * v : 0.f, 8 * i + a, 8 * j + b});
                }
            std::sort(quarters.begin(), quarters.end(), [](const Quarter &p, const Quarter &q) { return p.w > q.w; });
            for (int q = 0; q < 4; q++) {
                ZoneEntry &E = region->zones[64 * quarters[q].zxl + quarters[q].zyl];
                E.kind = 1;
                E.variant = (uint8_t)(q + 1);
            }
        }
    region->draws = random.count();
    return region;
}

} // namespace alphagen
