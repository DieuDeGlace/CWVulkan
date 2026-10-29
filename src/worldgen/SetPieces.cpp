// The wizard's tower, the other set pieces, crop fields and street lamps, written from
// docs/specs/worldgen-structures-buildings.md §11-§13.
#include "worldgen/Buildings.h"

#include "worldgen/BlockColours.h"
#include "worldgen/Height.h"

#include <cmath>

namespace worldgen {
namespace {

constexpr double PI = 3.141592653589793;

// A door of the wizard's tower: object 7 that swings.
void towerDoor(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, uint8_t turn) {
    StaticObject door = makeObject(7, dotsOf(x), dotsOf(y), dotsOf(z), turn);
    door.swing = 1000.0f;
    zone.keepIfInside(door);
}

// A post of the kind drawn (0 square, 1 round, 2 capped), not grounded.
void post(Mason &m, int kind, int32_t x, int32_t y, int32_t z, int32_t r, int32_t h) {
    if (kind == 0)
        m.squarePost(x, y, z, r, h, false);
    else if (kind == 1)
        m.roundPost(x, y, z, r, h, false);
    else
        m.cappedPost(x, y, z, r, h, false);
}

} // namespace

// --- §11 -----------------------------------------------------------------------------------------

void wizardTower(ZoneBuild &zone, BuildingRules &rules, int32_t cx, int32_t cy) {
    const uint32_t s = zone.place.style;
    const int32_t base = rules.baseHeight(cx, cy, 52) + 1;
    Mason m = Mason::atCell(zone, cx, cy);
    const Colour wall = wallColour(52, cx, cy, zone.place.colours()), trim = trimColour(52, cx, cy, zone.place.colours());
    auto scheme = [&] { m.twoColours(trim, pattern::STONE, 0, wall, pattern::STONE, 20, true); };
    const int32_t r = cellRandom(cx, cy, 0x255509) % 10 + 20;
    const int32_t height = cellRandom(cx, cy, 0x5d0ba) % 40 + 100 - 2 * r;
    scheme();
    // 1. The tower.
    m.pillar(8, 8, base - 1, r, height + 1, 0, 0);
    const int32_t top = height + base;
    // 2. Its top.
    if (hasBattlements(52, s)) {
        m.disc(8, 8, top, r + 1, 6);
        m.lightDisc(8, 8, top + 2, r - 1, 4, 0);
        for (int32_t k = -(r / 5); k <= r / 5; k++) {
            const int32_t x = 7 + 4 * k;
            m.air(x, 7 - r, top + 4, 2, 2 * r + 2, 2, 0);
            m.air(7 - r, x, top + 4, 2 * r + 2, 2, 2, 0);
        }
    } else {
        m.disc(8, 8, top, r + 1, 4);
        m.lightDisc(8, 8, top + 2, r - 1, 2, 0);
    }
    // 3. A roof.
    if (keepsHeight(52, s)) {
        m.oneColour(roofColour(52, cx, cy, zone.place.colours()), pattern::STONE);
        m.cone(8, 8, top, r, 2 * r);
    }
    // 4. Inside.
    const int32_t floorH = height / 4;
    m.lightDisc(8, 8, base, r - 2, 4 * floorH - 2, 2);
    scheme();
    m.type = 0x17;
    m.disc(8, 8, base - 2, r - 2, 2, 0, 2);
    m.type = blocktype::ROCK;
    // 5. The floors.
    for (int f = 0; f < 4; f++) {
        const int32_t z = f * floorH + base;
        int variant = 0;
        if (zone.draw() % 4 == 0)
            variant = zone.draw() % 2 + 1;
        if (variant == 1)
            m.lightDisc(8, 8, z, r, floorH - 2, 2);
        const int furnish = zone.draw() % 3;
        if (furnish == 0) { // a rug
            const int32_t a = zone.draw() % 12 + 10, b = zone.draw() % 12 + 10;
            Colour c1, c2;
            c1.b = (float)(zone.draw() % 150 + 50), c1.g = (float)(zone.draw() % 150 + 50), c1.r = (float)(zone.draw() % 150 + 50);
            c2.b = (float)(zone.draw() % 100 + 150), c2.g = (float)(zone.draw() % 100 + 150), c2.r = (float)(zone.draw() % 100 + 150);
            m.type = (uint8_t)(f + 0x17);
            const int32_t ox = -(a / 2), oy = -(b / 2);
            m.oneColour(c1, pattern::PLAIN);
            m.box(ox + 8, oy + 8, z - 1, a, b, 1);
            m.oneColour(c2, pattern::PLAIN);
            m.box(ox + 9, oy + 9, z - 1, a - 2, b - 2, 1);
            m.oneColour(c1, pattern::PLAIN);
            m.box(ox + 10, oy + 10, z - 1, a - 4, b - 4, 1);
            m.type = blocktype::ROCK;
        } else if (furnish == 1) { // four posts
            scheme();
            const int kind = zone.draw() % 3;
            for (int32_t i : {0, 16})
                for (int32_t j : {0, 16}) {
                    const int32_t ri = i == 0 ? 0 : r, rj = j == 0 ? 0 : r, q = r / 4;
                    if (kind == 1)
                        m.roundPost(i, j, z, 1, floorH - 2, false); // the game passes i, j here
                    else
                        post(m, kind == 0 ? 0 : 2, ri / 2 - q + 8, rj / 2 - q + 8, z, 1, floorH - 2);
                }
        } else { // one post
            scheme();
            const int kind = zone.draw() % 3;
            post(m, kind, 8, 8, z, 3, floorH - 2);
        }
        if (f < 3) { // the stairs up
            scheme();
            const int32_t steps = floorH - 1;
            m.writeMode = writemode::ONLY_PROTECTED;
            m.type = (uint8_t)(f + 0x18);
            m.disc(8, 8, z + floorH - 2, r - 2, 2);
            m.type = blocktype::ROCK;
            const double turns = (double)f * PI;
            const float reach = (float)(r - 4);
            for (int32_t i = 0; i < steps; i++) {
                const float a = (float)((double)i * PI / (double)steps * 0.8999999761581421 + 4.869468463261368 + turns);
                m.box(trunc32(std::cos(a) * reach + 8.0f - 3.0f), trunc32(reach * std::sin(a) + 8.0f - 3.0f), i + z + 1 - 3, 6, 6, 2);
                if (i > steps - 8) {
                    const float tt = (float)r - 4.5f;
                    m.air(trunc32(tt * std::cos(a) + 8.0f - 3.0f), trunc32(tt * std::sin(a) + 8.0f - 3.0f), i + z + 1, 6, 6, 16, 0);
                }
            }
            m.writeMode = 0;
        }
        if (variant == 2) { // balconies
            scheme();
            if (f > 0) {
                m.writeMode = writemode::NOT_SOLID | writemode::NOT_PROTECTED;
                m.disc(8, 8, z - 2, r + 4, 2);
                m.writeMode = 0;
            }
            m.air(8 - r, 5, z, 2, 6, 6, 0);
            m.air(8 - r, 6, z + 6, 2, 4, 1, 0);
            m.air(r + 6, 5, z, 2, 6, 6, 0);
            m.air(r + 6, 6, z + 6, 2, 4, 1, 0);
            m.air(5, 8 - r, z, 6, 2, 6, 0);
            m.air(6, 8 - r, z + 6, 4, 2, 1, 0);
            m.air(5, r + 6, z, 6, 2, 6, 0);
            m.air(6, r + 6, z + 6, 4, 2, 1, 0);
        } else if (variant == 1) { // a ring of pillars
            scheme();
            m.writeMode = writemode::NOT_PROTECTED;
            m.disc(8, 8, z - 2, r + 3, 2);
            m.disc(8, 8, z + floorH - 2, r + 3, 2);
            m.writeMode = 0;
            for (int k = 0; k < 8; k++) {
                const float a = (float)((double)k * PI * 0.25 + 0.7853981633974483);
                m.roundPost(trunc32(std::cos(a) * (float)(r + 1) + 8.0f), trunc32(std::sin(a) * (float)(r + 1) + 8.0f), z, 2, floorH - 2,
                            false);
            }
        } else { // doors
            if (f == 0) {
                m.air(8 - r, 6, z, 2, 4, 6, 0);
                if (xorshift(s + 0x129b4) & 1)
                    m.air(8 - r, 7, z + 6, 2, 2, 1, 0);
            } else {
                m.air(8 - r, 7, z + 1, 2, 2, 4, 0);
                towerDoor(zone, 16 * cx - r + 9, 16 * cy + 8, z + 1, 1);
            }
            m.air(r + 6, 7, z + 1, 2, 2, 4, 0);
            towerDoor(zone, 16 * cx + r + 7, 16 * cy + 8, z + 1, 3);
            m.air(7, 8 - r, z + 1, 2, 2, 4, 0);
            towerDoor(zone, 16 * cx + 8, 16 * cy - r + 9, z + 1, 0);
            m.air(7, r + 6, z + 1, 2, 2, 4, 0);
            towerDoor(zone, 16 * cx + 8, 16 * cy + r + 7, z + 1, 0);
        }
    }
}

// --- §12 -----------------------------------------------------------------------------------------

void arenaDais(ZoneBuild &zone, BuildingRules &rules, int32_t cx, int32_t cy) {
    Mason m = Mason::atCell(zone, cx, cy);
    m.oneColour(wallColour(97, cx, cy, zone.place.colours()), pattern::PLAIN);
    const int32_t base = rules.baseHeight(cx, cy, 97);
    for (int32_t i = 0, size = 16; size > 8; i++, size -= 2)
        m.foundation(i, i, base + i, size, size, 1, 0, 0);
}

void waterTower(ZoneBuild &zone, BuildingRules &rules, int32_t cx, int32_t cy) {
    const int32_t base = rules.baseHeight(cx, cy, 66) + 1;
    Mason m = Mason::atCell(zone, cx, cy);
    const Colour wall = wallColour(66, cx, cy, zone.place.colours());
    m.oneColour(scaled(wall, 0.9f), pattern::PLAIN);
    m.disc(8, 8, base + 8, 32, 1);
    m.oneColour(wall, pattern::PLAIN);
    m.disc(8, 8, base + 9, 32, 1);
    const Colour stone{140, 150, 180};
    m.oneColour(stone, pattern::PLAIN);
    m.disc(8, 8, base + 10, 32, 1);
    m.disc(8, 8, base + 11, 33, 1);
    m.lightDisc(8, 8, base + 10, 31, 2, 0);
    m.air(2, -25, base + 10, 12, 66, 2, 0);
    m.air(-25, 2, base + 10, 66, 12, 2, 0);
    for (int32_t i = 0; 11.2f > (float)i; i++) {
        m.air(5 - 2 * i, -24, base + 10, 1, 66, 1, 0);
        m.air(10 + 2 * i, -24, base + 10, 1, 66, 1, 0);
        m.air(-24, 5 - 2 * i, base + 10, 66, 1, 1, 0);
        m.air(-24, 10 + 2 * i, base + 10, 66, 1, 1, 0);
    }
    for (int32_t i = 0; i < 30; i++) {
        const float add = (float)(i & 1) * 20.0f;
        m.oneColour({stone.r + add, stone.g + add, stone.b + add}, pattern::PLAIN);
        m.disc(8, 8, base - 10 + i, 8, 1);
    }
    for (int rot = 0; rot < 4; rot++) {
        Mason leg = m.child(0, 0, 0, 16, 16, rot, false);
        const Mason::World w = leg.world(8, -25, 0);
        const int32_t g = trunc32(groundHeight(w.x, w.y, 0) - 2.0f);
        const int32_t h = base - g + 10;
        leg.stripedWall(12, -25, g, 2, h);
        leg.stripedWall(4, -25, g, 2, h);
    }
}

void cistern(ZoneBuild &zone, BuildingRules &rules, int32_t cx, int32_t cy) {
    Mason m = Mason::atCell(zone, cx, cy);
    m.oneColour(wallColour(68, cx, cy, zone.place.colours()), pattern::PLAIN);
    m.foundation(0, 0, rules.baseHeight(cx, cy, 68), 16, 16, 10, 0, 0);
}

void aqueduct(ZoneBuild &zone, BuildingRules &rules, int32_t cx, int32_t cy) {
    const int32_t base = rules.baseHeight(cx, cy, 0);
    const bool alongX = rules.type(cx - 1, cy) == 67 || rules.type(cx + 1, cy) == 67;
    Mason m = Mason::atCell(zone, cx, cy);
    m.oneColour({140, 150, 180}, pattern::PLAIN);
    for (int end = 0; end < 2; end++) {
        const int32_t step = end == 0 ? -1 : 1;
        const int32_t nb = alongX ? rules.baseHeight(cx + step, cy, 0) : rules.baseHeight(cx, cy + step, 0);
        const int32_t z = base > nb ? base : nb;
        const int32_t at = end == 0 ? 0 : 15;
        if (alongX) {
            m.foundation(at, 5, z, 1, 6, 6, 0, 0);
            m.box(at, 6, z + 6, 1, 4, 1);
        } else {
            m.foundation(5, at, z, 6, 1, 6, 0, 0);
            m.box(6, at, z + 6, 4, 1, 1);
        }
    }
    m.oneColour(wallColour(67, cx, cy, zone.place.colours()), pattern::PLAIN);
    if (alongX)
        m.tube(true, 0, 7, base + 3, base + 3, 16, 2);
    else
        m.tube(false, 7, 0, base + 3, base + 3, 16, 2);
}

void towerPlatform(ZoneBuild &zone, BuildingRules &rules, int32_t cx, int32_t cy, int t) {
    const uint32_t s = zone.place.style;
    const int32_t z = rules.baseHeight(cx, cy, t) + 1;
    Mason m = Mason::atCell(zone, cx, cy);
    const Colour colA = wallColour(t, cx, cy, zone.place.colours()), colB = trimColour(t, cx, cy, zone.place.colours());
    int32_t r = (int32_t)(xorshift(s + 0x17f55) % 13) + 20;
    int32_t lift = 3;
    m.twoColours(colB, pattern::STONE, 0, colA, pattern::STONE, 20, true);
    if (t == 70) {
        lift = 10, r -= 5;
        m.spiralRing(8, 8, z - 10, r - 1, r + 6, 22);
    }
    const bool grassy = grassyFloor(t, s);
    const int32_t top = lift + z;
    m.pillar(8, 8, top + 1, r, 1, grassy ? 6 : 0, grassy ? 2 : 0);
    if (waterLineTest(t, s))
        m.waterLine = lift + 10;
    const uint32_t kind = xorshift(s + 0xdaad) % 3;
    const float rr = (float)(r - 3);
    for (int k = 0; k < 8; k++) {
        const float a = (float)((double)k * (2.0 * PI) * 0.125);
        const int32_t x = trunc32(std::cos(a) * rr + 8.0f), y = trunc32(std::sin(a) * rr + 8.0f);
        if (kind == 1)
            m.cappedPost(x, y, top, 2, 15, true);
        else if (kind == 2)
            m.roundPost(x, y, top, 2, 15, true);
        else
            m.squarePost(x, y, top, 2, 15, true);
    }
}

// --- §13 -----------------------------------------------------------------------------------------

void cropField(ZoneBuild &zone, BuildingRules &rules, int32_t cx, int32_t cy, int t) {
    const Place &place = zone.place.place;
    auto roadAt = [&](int32_t x, int32_t y) {
        const float w = placeWeight(place.kind, place.big, warpedDist2(place.point, x, y));
        return roads(x, y, place.kind, w);
    };
    int32_t x0 = 16 * cx, x1 = 16 * cx + 16, y0 = 16 * cy, y1 = 16 * cy + 16;
    static constexpr int32_t SIDE[4][2] = {{0, -1}, {-1, 0}, {0, 1}, {1, 0}};
    for (int side = 0; side < 4; side++) {
        if (zone.draw() % 4 == 0 || !fenceFreeNeighbour(rules.type(cx + SIDE[side][0], cy + SIDE[side][1])))
            continue;
        switch (side) {
        case 0: y0 += 1; break;
        case 1: x0 += 1; break;
        case 2: y1 -= 1; break;
        default: x1 -= 1; break;
        }
        zone.draw(); // drawn, not used
        const bool alongX = side == 0 || side == 2;
        for (int32_t k = 0; k < 8; k++) {
            const int32_t X = alongX ? 16 * cx + 1 + 2 * k : 16 * cx + (side == 3 ? 15 : 0);
            const int32_t Y = alongX ? 16 * cy + (side == 2 ? 15 : 0) : 16 * cy + 1 + 2 * k;
            if (roadAt(X, Y) > 0.45f)
                continue;
            const float a = alongX ? groundHeight(X - 1, Y, 0) : groundHeight(X, Y - 1, 0);
            const float b = groundHeight(X, Y, 0);
            const int32_t h = trunc32((a > b ? a : b) + 1.0f);
            const int type = t == 34 ? 0x47 + zone.draw() % 3 : 0x43 + zone.draw() % 4;
            StaticObject o = makeObject(type, dotsOf(X) + dotsOf(alongX ? 0.01f : 0.51f), dotsOf(Y) + dotsOf(alongX ? 0.51f : 0.01f),
                                        dotsOf(h), alongX ? 0 : 1);
            if (zone.read(blocksOf(o.x), blocksOf(o.y), blocksOf(o.z)).type != blocktype::WATER)
                zone.objects.push_back(o);
        }
    }
    if (t == 34) { // a farmyard
        for (int32_t a = 0; a < 2; a++)
            for (int32_t b = 0; b < 2; b++) {
                if (zone.draw() % 4 == 0)
                    continue;
                const int32_t X = (2 * cx + a) * 8 + 4, Y = (2 * cy + b) * 8 + 4;
                if (roadAt(X, Y) > 0.45f)
                    continue;
                const int32_t h = trunc32(groundHeight(X, Y, 0) + 1.0f);
                if (zone.draw() % 4 != 0) {
                    zone.objects.push_back(makeObject(0x7f + zone.draw() % 3, dotsOf(X) + 32768, dotsOf(Y) + 32768, dotsOf(h), 0)); // turn not set
                } else {
                    Model mo;
                    mo.type = 0x51;
                    mo.x = dotsOf(X) + 32768, mo.y = dotsOf(Y) + 32768, mo.z = dotsOf(h);
                    mo.yaw = (float)zone.draw() * 360.0f / 32767.0f;
                    mo.size = 0.07f;
                    mo.r = mo.g = mo.b = 1.0f, mo.flags = 2;
                    zone.models.push_back(mo);
                }
            }
        return;
    }
    const bool rows = ((cx / 3 + cy / 3) & 1) != 0;
    for (int32_t X = 16 * cx; X < 16 * cx + 16; X++)
        for (int32_t Y = 16 * cy; Y < 16 * cy + 16; Y++) {
            if (roadAt(X, Y) > 0.5f)
                continue;
            int32_t h = trunc32(groundHeight(X, Y, 0));
            const int32_t v = ((rows ? X : Y) / 2) % 2;
            const float k = (float)v * 0.05f + 0.75f;
            BlockProps bed = riverBedColour(X, Y, h, zone.terrain.sand);
            zone.store(X, Y, h, toBlock({bed.r * k, bed.g * k, bed.b * k, 0, blocktype::GROUND, 0}));
            if (X < x0 || X >= x1 || Y < y0 || Y >= y1)
                continue;
            h += 1;
            if (zone.read(X, Y, h).type != blocktype::AIR)
                continue;
            int crop = 0, big = -1;
            float scale = 0.1f;
            switch (t) {
            case 27: crop = 0x24, big = 0x74; break;
            case 28: crop = 0x26, scale = 0.2f; break;
            case 29: crop = 0x27, scale = 0.2f; break;
            case 30:
                if (zone.draw() % 10 == 0)
                    crop = 0x22, scale = 0.05f;
                else
                    crop = 0x2a, scale = 0.2f;
                break;
            case 31: crop = 0x2b, scale = 0.2f; break;
            case 32: crop = 0x25, scale = 0.15f; break;
            default: break;
            }
            const bool turned = crop != 0x26;
            const int64_t px = dotsOf(X) + 32768, py = dotsOf(Y) + 32768, pz = dotsOf(h);
            if (zone.draw() % 4 == 0) {
                zone.addSpot(cx, cy, 21, {px, py, pz});
                continue;
            }
            if (zone.draw() % 10 != 0)
                continue;
            if (zone.draw() % 12 == 0 && X % 2 == 0 && Y % 2 == 0 && big != -1) {
                zone.objects.push_back(makeObject(big, px, py, pz, (uint8_t)(zone.draw() % 4)));
            } else if (crop != 0) {
                Model mo;
                mo.type = crop;
                mo.x = px, mo.y = py, mo.z = pz;
                mo.yaw = turned ? (float)zone.draw() * 360.0f / 32767.0f : 0.0f;
                mo.size = scale;
                mo.r = mo.g = mo.b = 1.0f;
                mo.flags = crop == 0x26 || crop == 0x22 || crop == 0x2a || crop == 0x2b ? 4 : 0;
                zone.models.push_back(mo);
            }
        }
}

void streetLamps(ZoneBuild &zone, BuildingRules &rules) {
    const Place &place = zone.place.place;
    if (place.kind != 5 && place.kind != 9)
        return;
    for (int32_t a = 0; a < 4; a++)
        for (int32_t b = 0; b < 4; b++) {
            if ((a + 3 * b) % 5 != 0)
                continue;
            const int32_t X = zone.x0 + 8 + 16 * a, Y = zone.y0 + 8 + 16 * b;
            const float w = placeWeight(place.kind, place.big, warpedDist2(place.point, X, Y));
            if (0.5f > w || rules.type(floorDiv(X, 4), floorDiv(Y, 4)) != 0)
                continue;
            const float r = roads(X, Y, place.kind, w);
            if (!(r > 0.5f && 0.8f > r))
                continue;
            const Neighbourhood n = neighbourhood(X, Y);
            int type;
            if (forestRegion(n, place.kind, w) > 0.5f)
                type = 0x42;
            else if (warmth(n, place.kind, w) > 0.85f)
                type = 0x41;
            else
                type = 0x3f + (int)(random(cellKey(zone.place.cell.x, zone.place.cell.y)) & 1);
            const int64_t x = dotsOf(X) + dotsOf(0.5f), y = dotsOf(Y) + dotsOf(0.5f);
            const int64_t z = trunc64((groundHeight(blocksTowardZero(x), blocksTowardZero(y), 0) + 1.0f) * 65536.0f);
            zone.settleAndKeep(makeObject(type, x, y, z, 0)); // the turn is not set (open question)
        }
}

} // namespace worldgen
