// Spots, plants, chests, items and the final passes, written from
// docs/specs/worldgen-structures-contents.md (sections cited as Cn).
#include "worldgen/ZoneContents.h"

#include "worldgen/BlockColours.h"
#include "worldgen/Height.h"
#include "worldgen/Options.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace worldgen {
namespace {

constexpr Block INDEX_BELOW = {200, 200, 200, 0, blocktype::SOLID, 0}; // stored index -1
constexpr Block PAST_TOP = {255, 255, 255, 0, blocktype::AIR, 0};

// A spot's position at block (x, y) and height z.
Spot spotAt(int32_t x, int32_t y, int32_t z) { return {dotsOf(x) + 32768, dotsOf(y) + 32768, dotsOf(z)}; }

// The "turn any" and the yaws of items (§C2, §C5).
float turnAny(ZoneBuild &zone) { return (float)zone.draw() * 360.0f / 32767.0f; }
float yawA(ZoneBuild &zone) { return (float)zone.draw() / 32767.0f * 360.0f; }
float turn90(ZoneBuild &zone) { return (float)(zone.draw() % 4 * 90); }

// Room above a spot (0x31ad50).
bool roomAbove(const ZoneBuild &zone, int32_t x, int32_t y, int32_t z) {
    for (int32_t dx = -2; dx <= 2; dx++)
        for (int32_t dy = -2; dy <= 2; dy++) {
            const int32_t m = std::max(std::abs(dx), std::abs(dy));
            for (int32_t k = 0; k <= 2; k++)
                if (!isOpen(zone.read(x + dx, y + dy, z + m + k)))
                    return false;
            for (int32_t k = m; k <= 7; k++)
                if (!isOpen(zone.read(x + dx, y + dy, z + k)))
                    return false;
        }
    return true;
}

// The climate at a point with the place there.
float warmthAt(int32_t x, int32_t y) {
    const Place p = placeAt(x, y);
    return warmth(neighbourhood(x, y), p.kind, p.weight);
}

GroundItem groundItem(uint8_t category, int32_t id, const Spot &s, float angle) {
    GroundItem it;
    it.item.category = category, it.item.id = id;
    it.x = s.x, it.y = s.y, it.z = s.z;
    it.angle = angle;
    return it;
}

} // namespace

// --- C1 ------------------------------------------------------------------------------------------

void findSpots(ZoneBuild &zone) {
    for (int32_t a = 0; a <= 62; a += 2)
        for (int32_t b = 0; b <= 62; b += 2) {
            const int32_t X = zone.x0 + a + zone.draw() % 2;
            const int32_t Y = zone.y0 + b + zone.draw() % 2;
            const int32_t cx = floorDiv(X, 4), cy = floorDiv(Y, 4);
            const int index = ZoneTerrain::index(X - zone.x0, Y - zone.y0);
            const TerrainColumn &col = zone.terrain.columns[(size_t)index];
            const ColumnField &f = zone.terrain.field[(size_t)index];
            const int32_t n = (int32_t)col.blocks.size();
            for (int32_t i = -1; i < n; i++) {
                const int32_t z = col.base + 1 + i;
                const Block b0 = i < 0 ? INDEX_BELOW : col.blocks[(size_t)i];
                const Block b1 = i + 1 >= n ? PAST_TOP : col.blocks[(size_t)(i + 1)];
                if (f.river > 0.5f && !isOpen(b0) && b1.type == blocktype::WATER)
                    zone.addSpot(cx, cy, 18, spotAt(X, Y, z));
                if (!isOpen(b1))
                    continue;
                zone.addSpot(cx, cy, 0, spotAt(X, Y, z));
                const uint8_t t = b0.type;
                const bool ground = t != 0 && !(b0.flags & 8) && t != 2 && t != 3 && t != 7 && t != 8 && t != 11 && t != 17 && t != 19;
                if (!ground || !roomAbove(zone, X, Y, z))
                    continue;
                if (t == 18) {
                    zone.addSpot(cx, cy, 5, spotAt(X, Y, z));
                } else if (t >= 23 && t <= 30) {
                    zone.addSpot(cx, cy, t - 16, spotAt(X, Y, z));
                } else if (b1.flags & 0x20) {
                    zone.addSpot(cx, cy, 6, spotAt(X, Y, z));
                } else if (b1.flags & 2) {
                    zone.addSpot(cx, cy, 2, spotAt(X, Y, z));
                } else if (z > 0 && !(b0.flags & 2)) {
                    zone.addSpot(cx, cy, 1, spotAt(X, Y, z));
                    if ((t == 4 || t == 5 || t == 9 || t == 10 || t == 14) && b1.type != blocktype::WATER)
                        zone.addSpot(cx, cy, 16, spotAt(X, Y, z));
                    if (t == 6)
                        zone.addSpot(cx, cy, 17, spotAt(X, Y, z));
                }
            }
        }
    zone.working = zone.spots;
}

// --- C2 ------------------------------------------------------------------------------------------

void plants(ZoneBuild &zone) {
    Model m; // one record, reused: what a plant does not set stays from the last one
    auto plain = [&](int32_t flags) { m.r = m.g = m.b = 1.0f, m.flags = flags; };
    for (int32_t X = zone.x0; X < zone.x0 + ZONE_SIZE; X++)
        for (int32_t Y = zone.y0; Y < zone.y0 + ZONE_SIZE; Y++) {
            const int index = ZoneTerrain::index(X - zone.x0, Y - zone.y0);
            const TerrainColumn &col = zone.terrain.columns[(size_t)index];
            const ColumnField &f = zone.terrain.field[(size_t)index];
            const int32_t n = (int32_t)col.blocks.size();
            if (n == 0)
                continue;
            const float noiseA = noiseAt(X, Y, 0.01, 9843, 0.01, 8437);
            const float noiseB = noiseAt(X, Y, 0.01, 34234, 0.01, 234234);
            auto put = [&](int32_t type, int32_t z) {
                m.type = type, m.x = dotsOf(X) + 32768, m.y = dotsOf(Y) + 32768, m.z = dotsOf(z);
                zone.models.push_back(m);
            };
            auto pick = [&](int32_t a, int32_t b, int32_t c, int32_t d) {
                if (noiseA > 0.0f)
                    return noiseB <= 0.0f ? b : a;
                return noiseB <= 0.5f ? d : c;
            };
            for (int32_t k = 0; k < n; k++) {
                const Block b0 = col.blocks[(size_t)k];
                const Block b1 = k + 1 < n ? col.blocks[(size_t)(k + 1)] : PAST_TOP;
                const int32_t z = col.base + k + 1;
                const uint8_t t = b0.type;
                // 1.
                if (b0.flags & 8)
                    continue;
                if (isOpen(b0) && (!(b0.flags & 0x40) || (b0.flags & 0x80)))
                    continue;
                if ((b1.flags & 0x20) && t != blocktype::GRASS)
                    continue;
                if (t == 19 || t == 2)
                    continue;
                // 2. Hanging from a cliff face.
                if (t == 0 && b1.type == 18 && zone.draw() % 20 == 0) {
                    plain(2);
                    m.yaw = turn90(zone);
                    m.size = zone.fraction(0.04f) + 0.06f;
                    m.type = 0x48, m.x = dotsOf(X) + 32768, m.y = dotsOf(Y) + 32768;
                    m.z = dotsOf(z) - trunc64(m.size * 20.0f * 65536.0f);
                    zone.models.push_back(m);
                }
                // 3.
                if (k < n - 1 && !isOpen(col.blocks[(size_t)(k + 1)]))
                    continue;
                // 4. Reeds.
                if (b1.type == 0 && t == 3 && zone.draw() % 2 != 0 && f.colours.warmth > 0.2f) {
                    if (!(absOf(noiseAt(X, Y, 0.05, 9843, 0.05, 8437)) > 0.5f))
                        continue;
                    plain(4);
                    m.yaw = turnAny(zone);
                    m.size = 0.09f;
                    put(0x23, z);
                    continue;
                }
                // 5. On lava.
                if (t == 17 && X % 16 == 0 && Y % 16 == 0 && zone.draw() % 3 != 0) {
                    m.yaw = 0.0f, m.flags = 2;
                    const float s = zone.fraction(0.5f) + 0.5f;
                    m.r = s, m.g = s * 0.5f, m.b = s * 0.2f;
                    put(0x4c, z);
                    continue;
                }
                // 6.
                if (t == 12) {
                    plain(6);
                    m.yaw = turnAny(zone);
                    m.size = 0.075f;
                    int32_t type;
                    if (noiseA > 0.0f)
                        type = simplex((double)((float)X * 0.01f + 34234.0f), (double)((float)Y * 0.01f + 234234.0f)) <= 0.0f ? 10 : 9;
                    else
                        type = noiseB <= 0.5f ? 14 : 13;
                    put(type, z);
                    continue;
                }
                // 7, 8.
                if (!(t == 4 || t == 9 || t == 10 || t == 14 || t == 18) && z > 0)
                    continue;
                if (!(absOf(noiseAt(X, Y, 0.05, 9843, 0.05, 8437)) > 0.6f))
                    continue;
                if (zone.draw() % 6 != 0)
                    continue;
                // 9. Under water.
                if (z <= 0 && !(b0.flags & 2)) {
                    if (z >= -5)
                        continue;
                    plain(2);
                    m.yaw = turn90(zone);
                    m.size = 0.1f;
                    int32_t type = 0x11;
                    if (noiseA > 0.0f) {
                        type = zone.draw() % 2 + 15;
                        if (type == 15)
                            m.size = 0.075f, m.flags = 6;
                    } else {
                        m.flags = 6;
                    }
                    m.size = m.size * ((float)zone.draw() / 32767.0f + 1.0f);
                    put(type, z);
                    continue;
                }
                // 10. On lava ground.
                if (t == 14) {
                    plain(2);
                    m.yaw = turn90(zone);
                    m.size = 0.075f;
                    const int32_t type = noiseA <= 0.0f ? 0x14 : 0x13;
                    if (type == 0x13)
                        m.flags = 6;
                    else
                        m.size = (float)((double)zone.draw() * 0.02 / 32767.0 + 0.03);
                    put(type, z);
                    continue;
                }
                // 11.
                if (t == 10)
                    continue;
                // 13. Forests.
                if (f.colours.forestRegion > 0.3f) {
                    plain(2);
                    m.yaw = turnAny(zone);
                    m.size = 0.075f;
                    const int32_t type = pick(9, 10, 13, 14);
                    if (type == 13 || type == 14)
                        m.size = zone.fraction(0.05f) + 0.05f;
                    else
                        m.flags = 6;
                    if (t == 4 || type == 9 || type == 10)
                        put(type, z);
                    continue;
                }
                // 14. Hot ground.
                if (!(f.colours.warmth < 0.8f)) {
                    if (f.colours.dryness < 0.3f) {
                        plain(6);
                        m.yaw = turnAny(zone);
                        m.size = 0.075f;
                        const int32_t type = pick(7, 8, 11, 12);
                        if (type == 11 || type == 12)
                            m.size = zone.fraction(0.05f) + 0.05f;
                        if (t == 4 || type == 7 || type == 8)
                            put(type, z);
                        continue;
                    }
                    m.r = m.g = m.b = 1.0f;
                    m.yaw = turnAny(zone);
                    m.size = 0.075f;
                    int32_t type;
                    if (noiseA > 0.0f)
                        type = noiseB > 0.0f ? 6 : 5;
                    else
                        type = noiseB <= 0.5f ? 0x16 : 0x15;
                    m.flags = type >= 4 && type <= 6 ? 6 : 2;
                    if (type == 0x15)
                        m.size = zone.fraction(0.05f) + 0.05f, m.flags |= 4;
                    else if (type == 0x16)
                        m.size = zone.fraction(0.02f) + 0.1f, m.flags |= 4;
                    if (t == 4 || type == 4 || type == 5)
                        put(type, z);
                    continue;
                }
                // 15. Grass and flowers.
                plain(2);
                m.yaw = turn90(zone);
                m.size = 0.075f;
                int32_t type;
                if (f.river > 0.43f) {
                    m.yaw = turnAny(zone);
                    m.size = 0.09f, m.flags = 4;
                    type = 0x23;
                } else {
                    if (!(noiseA > -0.1f)) {
                        const float v = noiseB;
                        type = -0.5f > v ? 0 : v <= 0.5f ? 2 : 1;
                    } else if (f.colours.dryness > 0.5f) {
                        type = noiseB > 0.0f ? 0x16 : 6;
                    } else {
                        type = noiseB <= 0.0f ? 5 : 4;
                    }
                    if ((type >= 3 && type <= 6) || type == 0x16)
                        m.flags = 6;
                    if (type == 0x16)
                        m.size = zone.fraction(0.02f) + 0.1f;
                }
                if (t == 4 || (type >= 4 && type <= 6))
                    put(type, z);
            }
        }
}

// --- C3 ------------------------------------------------------------------------------------------

void cellThings(ZoneBuild &zone, BuildingRules &rules) {
    for (int32_t cx = 4 * zone.zx; cx < 4 * zone.zx + 4; cx++)
        for (int32_t cy = 4 * zone.zy; cy < 4 * zone.zy + 4; cy++) {
            zone.seed(random(cellKey(cx, cy)));
            const CastleCell k = rules.castleCell(cx, cy);
            auto chestAt = [&](const Spot &s) {
                StaticObject chest = makeObject(objecttype::CHEST, s.x, s.y, s.z, (uint8_t)(zone.draw() % 4));
                zone.settle(chest, true);
                return chest;
            };
            auto regionOfDots = [](int64_t x, int64_t y) { return regionAt(blocksTowardZero(x), blocksTowardZero(y)); };
            // 1, 2. Castle chests.
            if (k.kind == 5) {
                const StaticObject chest = chestAt(zone.takeSpot(cx, cy, 6));
                Item reward = zone.planner.treasure;
                if (reward.category == 0) {
                    reward.category = itemcategory::GENERATED;
                    reward.modifier = (uint32_t)zone.draw();
                    reward.id = zone.draw() % 3;
                    int rarity = 4;
                    if (zone.draw() % 10 == 0)
                        rarity = 5;
                    reward.rarity = (uint8_t)std::min(rarity, 4);
                    reward.region = regionOfDots(chest.x, chest.y);
                }
                zone.keepChest(chest, reward);
            } else if (k.kind == 3) {
                const StaticObject chest = chestAt(zone.takeSpot(cx, cy, 6));
                Item reward;
                reward.category = itemcategory::GENERATED;
                reward.id = zone.draw() % 3;
                reward.modifier = (uint32_t)zone.draw();
                int rarity = (k.steps >> 2) + 1;
                if (zone.draw() % 10 == 0)
                    rarity += 1;
                reward.rarity = (uint8_t)std::min(rarity, 4);
                reward.region = regionAt(zone.place.place.point.x, zone.place.place.point.y);
                zone.keepChest(chest, reward);
            }
            // 3. Crates.
            const int t = rules.type(cx, cy);
            if (crateCell(t) && t != 21 && t != 60 && t != 62) {
                Item reward;
                reward.category = itemcategory::GENERATED;
                reward.id = zone.draw() % 3;
                reward.modifier = (uint32_t)zone.draw();
                reward.rarity = (uint8_t)(zone.draw() % 3 + 2);
                reward.region = regionAt(16 * cx, 16 * cy);
                Spot s;
                if (t == 17)
                    s = zone.takeSpot(cx, cy, zone.spotCount(cx, cy, 5) != 0 ? 5 : 0);
                else if (t == 18)
                    s = zone.takeSpot(cx, cy, 18);
                else
                    s = zone.spotInCell(cx, cy);
                if (zone.draw() % 2 != 0 || t == 18) {
                    zone.keepChest(chestAt(s), reward);
                } else {
                    GroundItem it = groundItem(reward.category, reward.id, s, 0.0f); // the angle is not written
                    it.item = reward;
                    it.flags |= PER_PLAYER;
                    zone.items.push_back(it);
                }
            }
            // 4. Lore cells.
            if (!loreCell(t))
                continue;
            const int32_t b = rules.baseHeight(cx, cy, t);
            if (t == 55) {
                StaticObject o = makeObject(objecttype::MENHIR, dotsOf(16 * cx + 8) + dotsOf(0.51f), dotsOf(16 * cy + 8) - dotsOf(0.1f),
                                            dotsOf(b + 1), 0);
                for (int n = 0; n < 8; n++) {
                    const int32_t g = floorToInt(groundHeight(blocksTowardZero(o.x), blocksTowardZero(o.y), 0));
                    o.z = dotsOf((g > 0 ? g : 0) + 2);
                    bool blocked = false;
                    for (int32_t dz = -1; dz <= 1 && !blocked; dz++)
                        for (int32_t dx = -1; dx <= 1 && !blocked; dx++) {
                            const Block k2 = zone.read(blocksOf(o.x) + dx, blocksOf(o.y), blocksOf(o.z) + dz);
                            blocked = !isOpen(k2) || (k2.flags & 0x40);
                        }
                    if (!blocked)
                        break;
                    o.y -= DOTS;
                    if (o.y < dotsOf(zone.y0))
                        break;
                }
                zone.objects.push_back(o);
                continue;
            }
            int type;
            if (t == 37)
                type = 0x82;
            else if ((xorshift(zone.place.style) & 3) == 0)
                type = 0x76;
            else
                type = 0x77 + (int)(xorshift(zone.place.style + 0xda5) % 6);
            StaticObject o = makeObject(type, dotsOf(16 * cx + 8), dotsOf(16 * cy + 8), dotsOf(b), (uint8_t)(zone.draw() % 4));
            zone.settle(o, true);
            zone.keepIfInside(o);
        }
}

// --- C4 ------------------------------------------------------------------------------------------

namespace {
// Whether a cell suits a role item (0x128cc0).
bool fits(BuildingRules &rules, int kind, int landmark, int32_t x, int32_t y) {
    int t = rules.type(x, y);
    if (landmark != 0)
        t = landmark;
    const bool plain = t == 0 || t == 1 || t == 2 || t == 3 || t == 4 || t == 5 || t == 10 || t == 11 || t == 12 || t == 99;
    const int32_t mx = 16 * x + 8, my = 16 * y + 8;
    auto ground = [&](int which) {
        const Place p = placeAt(mx, my);
        if (p.variant != 0 && !(0.8f <= p.weight))
            return false;
        const ColumnValues c = columnValues(mx, my, p, 0);
        switch (which) {
        case 0: return c.lava > 0.5f;
        case 1: return c.river > 0.5f;
        default: return !(c.river > 0.5f) && !(c.lava > 0.5f) && !(c.swamp > 0.5f) && groundHeight(mx, my, 0) >= 1.0f;
        }
    };
    switch (kind) {
    case 13: return plain && ground(1);
    case 14: case 19: return plain && ground(0);
    case 1: case 2: case 3: return t >= 22 && t <= 24;
    case 4: return t == 35 || t == 36 || t == 38;
    case 7: return t == 52;
    case 21: return t == 12;
    case 22: return t >= 87 && t <= 89;
    case 29: return t >= 10 && t <= 12;
    case 34: case 35: return t == 99;
    case 37: return t == 57;
    case 38: return t == 69 || t == 70;
    default: return plain && ground(2);
    }
}
} // namespace

void roleItem(ZoneBuild &zone, int role) {
    static constexpr int32_t IDS[8] = {1, 2, 0x12, 3, 5, 7, 8, 9};
    if (role < 6 || role > 13)
        return;
    BuildingRules rules(zone.cells);
    const Place &p = zone.place.place;
    const Int2 c1 = zone.place.cell;
    const int landmark = placeLandmark(p.kind, c1);
    const float limit = landmark != 0 ? 0.5f : 0.75f;
    std::vector<Int2> cells;
    const int32_t first = floorDiv(wrapMul(c1.x, 1024), 4), lastX = floorDiv(wrapMul(c1.x, 1024) + (p.big ? 2048 : 1024), 4);
    const int32_t firstY = floorDiv(wrapMul(c1.y, 1024), 4), lastY = floorDiv(wrapMul(c1.y, 1024) + (p.big ? 2048 : 1024), 4);
    for (int32_t x = first; x < lastX; x++)
        for (int32_t y = firstY; y < lastY; y++)
            if (limit < placeWeight(p.kind, p.big, warpedDist2(p.point, 16 * x + 8, 16 * y + 8)) && fits(rules, p.kind, landmark, x, y))
                cells.push_back({x, y});
    if (cells.empty())
        cells.push_back({floorDiv(p.point.x, 4), floorDiv(p.point.y, 4)});
    const Int2 pick = cells[(size_t)(random(cellKey(c1.x, c1.y) + 0x4192) % (uint32_t)cells.size())];
    if (!zone.inZone(16 * pick.x, 16 * pick.y))
        return;
    const Spot s = zone.spotInCell(pick.x, pick.y);
    GroundItem it = groundItem(itemcategory::SPECIAL, IDS[role - 6], s, turnAny(zone));
    // The region and the per-player flag are given by worldgen-creatures-roles.md §2 (this part's
    // spec leaves them out).
    it.item.region = regionAt(p.point.x, p.point.y);
    it.flags |= PER_PLAYER;
    if (keepsGeneratedItem(it.item.category, it.item.id))
        zone.items.push_back(it);
}

// --- C5 ------------------------------------------------------------------------------------------

void cellCreaturesAndItems(ZoneBuild &zone, BuildingRules &rules, const CellPassHooks &hooks) {
    zone.working = zone.spots;
    const Place &place = zone.place.place;
    Spot herb{0, 0, 0};
    bool herbPlaced = false; // before the first herb the record's position is undefined: a grey block
    auto belowHerb = [&]() {
        if (!herbPlaced)
            return INDEX_BELOW;
        return zone.read(blocksOf(herb.x), blocksOf(herb.y), blocksOf(herb.z - DOTS));
    };
    for (int32_t cx = 4 * zone.zx; cx < 4 * zone.zx + 4; cx++)
        for (int32_t cy = 4 * zone.zy; cy < 4 * zone.zy + 4; cy++) {
            // 1.
            const uint32_t h = random(cellKey(cx, cy));
            zone.seed(h % 32767u);
            const int t = rules.type(cx, cy);
            const int k = place.kind;
            const int32_t mx = 16 * cx + 8, my = 16 * cy + 8;
            const float w = placeWeight(k, place.big, warpedDist2(place.point, mx, my));
            bool things = true;
            // 2. Townspeople (part C).
            if (w > 0.4f) {
                if (k == 5) {
                    if (hooks.townsfolk)
                        things = hooks.townsfolk(zone, cx, cy);
                } else if (k != 8 && k != 16 && t != 88 && zone.place.cell == Int2{0, 0} && t == 24) {
                    if (hooks.firstTownPeople)
                        hooks.firstTownPeople(zone, cx, cy);
                    things = false;
                }
            }
            // 3. Camp cells.
            if (things && (t == 98 || t == 99)) {
                const int n = zone.draw() % 2 + 1;
                for (int i = 0; i < n; i++) {
                    const Spot s = zone.takeSpot(cx, cy, 1);
                    StaticObject o = makeObject(objecttype::CAMPFIRE, s.x, s.y, s.z, (uint8_t)(zone.draw() % 4));
                    if (i > 0 || t == 99) {
                        const bool low = o.z < dotsOf(25);
                        if (zone.draw() % 4 != 0) {
                            o.type = low ? 0x5f : 0x60;
                        } else if (low && zone.draw() % 3 == 0) {
                            o.type = 0x5e;
                        } else {
                            static constexpr int THINGS[6] = {0x25, 0x26, 0x27, 0x28, 0x5d, 0x1c};
                            const int r = zone.draw() % 7;
                            o.type = r < 6 ? THINGS[r] : 0x83 + zone.draw() % 2;
                        }
                    }
                    zone.keepIfInside(o);
                }
            }
            // 4. A region's boat.
            if (posMod(cx, 16) == 0 && posMod(cy, 16) == 0 && biomeAt(16 * cx, 16 * cy) == biomes::OCEAN &&
                groundHeight(mx, my, 0) < 10.0f && zone.spotCount(cx, cy, 16) != 0) {
                const Spot s = zone.takeSpot(cx, cy, 16);
                GroundItem boat = groundItem(itemcategory::SPECIAL, 2, s, turnAny(zone));
                boat.item.region = regionAt(16 * cx, 16 * cy);
                boat.flags |= PER_PLAYER;
                if (keepsGeneratedItem(boat.item.category, boat.item.id))
                    zone.items.push_back(boat);
            }
            // 5, 6. Wild groups, castle and dungeon creatures (part C).
            if ((k == 0 || w < 0.5f) && hooks.wildGroup)
                hooks.wildGroup(zone, cx, cy);
            const CastleCell castle = rules.castleCell(cx, cy);
            if (castle.kind != 0 && hooks.castleCreatures)
                hooks.castleCreatures(zone, cx, cy, castle);
            if (k == 8 && hooks.dungeonCreatures)
                hooks.dungeonCreatures(zone, cx, cy);
            // 7. Herbs.
            if (zone.spotCount(cx, cy, 16) != 0) {
                const bool tree = t >= 1 && t <= 5;
                uint8_t category = itemcategory::PLANT;
                int32_t id = belowHerb().type == blocktype::SNOW ? 0x18 : 0x16;
                if (zone.draw() % 5 == 0)
                    id = 0x1d;
                int n = zone.draw() % 40 - 38;
                bool checked = false;
                if (tree && t != 5) {
                    id = 0xf;
                    n = zone.draw() % 4;
                    if (zone.draw() % 2 != 0)
                        id = 0x24, n = 1;
                    if (t == 1) {
                        const Neighbourhood nb = neighbourhood(mx, my);
                        const Place pp = placeAt(mx, my);
                        const float warm = warmth(nb, pp.kind, pp.weight), dry = dryness(nb);
                        if (h % 10 == 0 && 0.3f < warm && warm < 0.75f && dry > 0.2f)
                            category = 1, id = 0xe, checked = true;
                        else if (h % 10 == 1 && warm > 0.6f)
                            id = 0x28, checked = true;
                    } else if (t == 4) {
                        if (h % 10 == 0)
                            id = 0x2b, checked = true;
                        else if (h % 10 == 1)
                            id = 0x29, checked = true;
                    }
                    if (checked)
                        n = zone.spotCount(cx, cy, 16) / 8 + 1;
                }
                if (!checked && (id == 0x16 || id == 0x1d) && belowHerb().type != blocktype::GRASS)
                    continue; // the rest of this cell is skipped
                for (int i = 0; i < n; i++) {
                    const float angle = yawA(zone);
                    herb = zone.takeSpot(cx, cy, 16);
                    herbPlaced = true;
                    zone.items.push_back(groundItem(category, id, herb, angle));
                }
            }
            // 8. Cold water.
            if (zone.draw() % 10 == 0 && zone.spotCount(cx, cy, 18) != 0) {
                const Spot s = zone.takeSpot(cx, cy, 18);
                if (warmthAt(blocksTowardZero(s.x), blocksTowardZero(s.y)) < 0.2f)
                    zone.items.push_back(groundItem(itemcategory::PLANT, 0x27, s, yawA(zone)));
            }
            // 9. Shells.
            if (zone.draw() % 30 == 0 && zone.spotCount(cx, cy, 5) != 0) {
                const Spot s = zone.takeSpot(cx, cy, 5);
                zone.items.push_back(groundItem(itemcategory::PLANT, 0x13, s, yawA(zone)));
            }
            // 10. Crops to pick.
            if (t == 27 || t == 32 || t == 33) {
                const int32_t id = t == 27 ? 0x10 : t == 32 ? 0x11 : 0x2a;
                const int want = zone.draw() % 5 + 4;
                const int n = std::min(zone.spotCount(cx, cy, 21), want);
                for (int i = 0; i < n; i++) {
                    const Spot s = zone.takeSpot(cx, cy, 21);
                    zone.items.push_back(groundItem(itemcategory::PLANT, id, s, turnAny(zone)));
                }
            }
            // 11. Animals (part C).
            if (hooks.animals)
                hooks.animals(zone, cx, cy);
        }
}

// --- C7, C8 --------------------------------------------------------------------------------------

void copyAndRestamp(ZoneBuild &zone, ZoneLists &lists) {
    lists.objectsCopy = zone.objects;
    lists.itemsCopy = zone.items;
    for (GroundItem &it : zone.items)
        if (restampedCategory(it.item.category))
            it.item.region = regionAt(blocksTowardZero(it.x), blocksTowardZero(it.y));
}

void listLightsAndTreasures(ZoneBuild &zone, ZoneLists &lists) {
    lists.lights.clear();
    lists.treasures.clear();
    for (const GroundItem &it : zone.items)
        if (it.item.category == itemcategory::GENERATED)
            lists.treasures.push_back({it.x, it.y, it.z});
    for (const StaticObject &o : zone.objects) {
        if (lightObject(o.type))
            lists.lights.push_back(o);
        if (o.type == objecttype::CHEST && !(zone.read(blocksOf(o.x), blocksOf(o.y), blocksOf(o.z)).flags & 0x20))
            lists.treasures.push_back({o.x, o.y, o.z});
    }
}

void markObjectBlocks(ZoneBuild &zone, ZoneLists &lists) {
    for (const auto &[x, y, z] : lists.marked) {
        Block b = zone.read(x, y, z);
        b.flags &= (uint8_t)~0x40;
        zone.store(x, y, z, b);
    }
    lists.marked.clear();
    for (const StaticObject &o : zone.objects) {
        const int t = o.type;
        if (t == 0x5f || t == 0x60 || t == 0x11 || t == 0x12 || t == 0x14 || t == 0x7d)
            continue;
        if (!o.active && ((t >= 3 && t <= 6) || (t >= 8 && t <= 14) || t == 16))
            continue;
        const ObjectSize size = objectSize(t);
        float sx = size.x * o.scale, sy = size.y * o.scale;
        const float sz = size.z * o.scale;
        if (o.turn & 1)
            std::swap(sx, sy);
        const int64_t hx = trunc64(sx * 0.499f * 65536.0f), hy = trunc64(sy * 0.499f * 65536.0f);
        const int32_t x0 = blocksOf(o.x - hx), x1 = blocksOf(o.x + hx), y0 = blocksOf(o.y - hy), y1 = blocksOf(o.y + hy);
        const int32_t z0 = blocksOf(o.z), z1 = blocksOf(o.z + trunc64(sz * 0.999f * 65536.0f));
        for (int32_t x = x0; x <= x1; x++)
            for (int32_t y = y0; y <= y1; y++)
                for (int32_t z = z0; z <= z1; z++) {
                    Block b = zone.read(x, y, z);
                    if (b.flags & 0x40)
                        continue;
                    b.flags |= t == 15 ? 0xc0 : 0x40;
                    if (t >= 3 && t <= 5)
                        b.flags |= 4;
                    zone.store(x, y, z, b);
                    lists.marked.insert({x, y, z});
                }
    }
}

// --- C9 ------------------------------------------------------------------------------------------

void campObjects(ZoneBuild &zone, int32_t cx, int32_t cy, int kind, int race) {
    auto put = [&](int type) {
        const Spot s = zone.takeSpot(cx, cy, kind);
        const StaticObject o = makeObject(type, s.x, s.y, s.z, (uint8_t)(zone.draw() % 4));
        if (zone.insideDots(o.x, o.y))
            zone.objects.push_back(o);
    };
    put(objecttype::CAMPFIRE);
    const int n = zone.draw() % 4;
    for (int i = 0; i < n; i++) {
        int type;
        switch (zone.draw() % 8) {
        case 1: type = 0x1c; break;
        case 2: type = 0x60; break;
        case 3: type = 0x28; break;
        case 4: type = 0x26; break;
        case 5: type = 0x27; break;
        case 6: type = 0x25; break;
        case 7:
            type = zone.draw() % 2 + 0x83;
            if (race == 0x2d || race == 0x2b || race == 0xa7 || race == 0xe6)
                type = 0x85;
            break;
        default: type = 0x5d; break;
        }
        put(type);
    }
}

ModelLists modelLists(const std::vector<Model> &models) {
    constexpr int32_t LIGHT_MODEL = 0x4c, FIRST_EMITTER = 0x4d, LAST_EMITTER = 0x50, WATER_EMITTER = 0x1f;
    ModelLists out;
    for (const Model &m : models) {
        if (m.type == LIGHT_MODEL || (m.flags & modelflag::LIGHT))
            out.lights.push_back(m);
        if ((m.type >= FIRST_EMITTER && m.type <= LAST_EMITTER) || m.type == WATER_EMITTER)
            out.emitters.push_back(m);
    }
    return out;
}

} // namespace worldgen
