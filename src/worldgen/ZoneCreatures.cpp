// The zone's creature passes, written from docs/specs/worldgen-creatures-zone.md (sections cited
// as §n) and worldgen-creatures.md.
#include "worldgen/ZoneCreatures.h"

#include "worldgen/CreatureBuilder.h"
#include "worldgen/Height.h"
#include "worldgen/Residents.h"
#include "worldgen/BlockColours.h"

#include <algorithm>
#include <cmath>

namespace worldgen {
namespace {

using namespace looksflag;

constexpr int32_t NO_RACE = -1;

// --- §2.1 Townsfolk --------------------------------------------------------------------------

// The townspeople's race by the region's culture 1..13 (0x319130), 2 for any other.
int32_t townRace(int culture) {
    static constexpr int32_t RACES[13] = {0xca, 2, 0, 0xd, 0xf, 0, 9, 0xd, 7, 0xb, 2, 0xf, 4};
    return culture >= 1 && culture <= 13 ? RACES[culture - 1] : 2;
}

bool townsfolk(CreatureBuilder &b, int32_t cx, int32_t cy) {
    ZoneBuild &zone = b.zone;
    const uint32_t s = zone.place.style;
    const uint32_t special = b.plan() && b.plan()->specialTown == zone.place.cell ? SPECIAL_TOWN : 0;
    const int32_t people = !styleOdd(s) && styleEven(s) ? styleRace(s) : 0;
    const int block = townBlock(zone.world, cx, cy, 5);
    if (block == 2) {
        const Spot sp = b.take(cx, cy, 2);
        zone.spawns.shopSpots.push_back({sp.x, sp.y, sp.z});
    }
    const int32_t base = townRace(zone.place.biome);
    auto townsperson = [&](uint8_t cls) {
        SpawnPoint p;
        p.hostility = hostility::FRIENDLY;
        p.race = b.variant(base);
        p.gear.cls = cls;
        p.people = people;
        p.style = s;
        return p;
    };
    const int t = b.rules.type(cx, cy);
    // 7. Houses with a door.
    if (isHouse(t) && hasDoor(t)) {
        static constexpr uint8_t KEEPERS[9] = {0x84, 0x80, 0x82, 0x81, 0x83, 0x85, 0x86, 0x87, 0x9a};
        if (block == 0)
            return true;
        uint8_t cls = 0x8a;
        int kind = 4;
        if (block >= 2 && block <= 10) {
            cls = KEEPERS[block - 2];
            kind = block == 10 ? 1 : 4;
        } else if (b.rand() % 10 != 0) {
            return true;
        }
        SpawnPoint p = townsperson(cls);
        p.yaw = b.yawA();
        p.addFlags(special);
        b.looks(p);
        b.gear(p, 0);
        b.place(p, b.take(cx, cy, kind));
        p.roam = 4;
        p.yaw = b.yawB();
        b.add(p);
        if (block == 10) { // the keeper's helper
            SpawnPoint h;
            h.hostility = hostility::NEUTRAL;
            h.race = 0x12f;
            h.yaw = b.yawA();
            h.addFlags(special);
            b.looks(h);
            b.gear(h, 0);
            b.place(h, b.take(cx, cy, 3));
            h.z += trunc64((h.appearance.height() * 0.5f + 0.5f) * 65536.0f);
            h.roam = 4;
            h.yaw = b.yawB();
            b.add(h);
        }
        return true;
    }
    // 8. The town hall.
    if (t == 0x5b) {
        SpawnPoint p = townsperson(0x9b);
        p.yaw = b.yawA();
        p.addFlags(special);
        b.looks(p);
        b.gear(p, 3);
        p.x = dotsOf(16 * cx + 8), p.y = dotsOf(16 * cy + 8), p.z = dotsOf(b.rules.baseHeight(cx, cy, 0x5b));
        p.roam = 4;
        p.yaw = b.yawB();
        b.add(p);
        return false;
    }
    // 9. Anyone else, now and then.
    if (b.rand() % 20 != 0 || posMod(cx + cy, 2) != 0)
        return true;
    SpawnPoint p;
    if (b.rand() % 10 != 0) {
        p = townsperson(0x8a);
        p.addFlags(special);
        p.roam = 0x10;
        if (b.rand() % 2 != 0)
            p.roam |= 0x20;
        if (b.rand() % 2 != 0)
            p.roam |= 8;
        if (b.rand() % 2 != 0)
            p.roam |= 4;
    } else {
        static constexpr int32_t TRAVELLERS[8] = {32, 30, 33, 20, 26, 19, 56, 34};
        p.hostility = hostility::NEUTRAL;
        p.race = TRAVELLERS[b.rand() % 8];
        p.roam = 0x10;
    }
    p.yaw = b.yawA();
    b.looks(p);
    b.gear(p, 0);
    b.place(p, b.take(cx, cy, 1));
    b.add(p);
    return true;
}

// --- §2.2 The first town's people ------------------------------------------------------------

void firstTownPeople(CreatureBuilder &b, int32_t cx, int32_t cy) {
    struct Person {
        int32_t race;
        const char *name;
        uint32_t flags, roam;
    };
    static constexpr Person PEOPLE[3] = {{0x101, "Wollay", 0x2000, 0x30}, {0x102, "Pixxie", 0x2000, 0x30}, {0x13, "Joschi", 0x102000, 0x10}};
    for (const Person &person : PEOPLE) {
        SpawnPoint p;
        p.hostility = hostility::FRIENDLY;
        p.race = person.race;
        std::snprintf(p.name, sizeof p.name, "%s", person.name);
        b.looks(p);
        p.addFlags(person.flags);
        b.place(p, b.take(cx, cy, 2));
        p.roam = person.roam;
        b.add(p);
    }
}

// --- §5 The arena master ---------------------------------------------------------------------

// The arena's master (class 0xa1), standing in the middle of his dais; the arena's teams wait on
// his stage byte (worldgen-creatures.md §7.2).
constexpr int32_t ARENA_MASTER_RACE = 0x104;
constexpr int32_t ARENA_MASTER_PEOPLE = 3;
constexpr int32_t DAIS_HEIGHT = 4;

void arenaMaster(CreatureBuilder &b, int32_t cx, int32_t cy, int32_t base) {
    SpawnPoint p;
    p.hostility = hostility::FRIENDLY;
    p.race = b.variant(ARENA_MASTER_RACE);
    p.gear.cls = ARENA_MASTER_CLASS;
    p.people = ARENA_MASTER_PEOPLE;
    p.yaw = 270.0f;
    b.looks(p);
    b.gear(p);
    p.x = dotsOf(16 * cx + 8), p.y = dotsOf(16 * cy + 8), p.z = dotsOf(base + DAIS_HEIGHT);
    p.roam = 4;
    b.add(p);
}

// --- §2.4 Wild groups ------------------------------------------------------------------------

const SpawnGroup *chooseGroup(CreatureBuilder &b, int32_t cx, int32_t cy, bool animals) {
    const int32_t x = 16 * cx + 8, y = 16 * cy + 8;
    const int culture = biomeAt(x, y);
    const int object = b.rules.type(cx, cy);
    const ColumnValues c = columnValues(x, y);
    const int kind = c.place.kind;
    const float w = c.place.weight;
    const float F = forestRegion(c.regions, kind, w), T = warmth(c.regions, kind, w), D = c.dryness, K = c.lake;
    const float V = c.lavaRegion, W = kind27Weight(kind, w), Rv = c.river, Lv = c.lava, Sw = c.swamp, G = c.highlands, Z = c.height;
    uint32_t env = F > 0.5f ? 4 : 0;
    if (T < 0.2f)
        env |= 2;
    bool mild = T >= 0.2f && F <= 0.5f;
    if (D < 0.1f && T > 0.9f)
        env |= 0x10, mild = false;
    if (D > 0.8f && T > 0.9f)
        env |= 0x20, mild = false;
    if (D < 0.4f && T > 0.8f)
        env |= 0x10;
    if (K > 0.5f)
        env |= 8, mild = false;
    if (V > 0.5f)
        env = 0x100, mild = false;
    if (culture == 3)
        env |= 0x80, mild = false;
    if (W > 0.5f)
        env |= 0x200, mild = false;
    if (mild)
        env |= 1;
    uint32_t ground = object >= 1 && object <= 4 ? 8 : 0;
    if (Z > 0.0f && Z < 10.0f)
        ground |= 4;
    if (Z <= 0.0f)
        ground |= 0x40;
    if (Rv > 0.25f && Rv < 0.5f)
        ground |= 1;
    if (Rv > 0.5f)
        ground |= 2;
    if (Lv > 0.25f && Lv < 0.5f)
        ground |= 0x80;
    if (Lv > 0.5f)
        ground |= 0x100;
    if (Sw > 0.25f && Sw < 0.5f)
        ground |= 0x400;
    if (Sw > 0.5f)
        ground |= 0x200;
    if (G > 0.75f)
        ground |= 0x10;
    if (!animals && ground == 0)
        ground = 0x800;
    if (animals)
        ground |= 0x20;
    std::vector<const SpawnGroup *> fitting;
    for (const SpawnGroup &g : b.tables.groups())
        if ((g.climate == 0 || (env & g.climate)) && ((!animals && g.ground == 0) || (g.ground & ground)) && !g.races.empty())
            fitting.push_back(&g);
    if (fitting.empty())
        return nullptr;
    return fitting[(size_t)(b.rand() % (int)fitting.size())];
}

void wildGroup(CreatureBuilder &b, int32_t cx, int32_t cy, bool animals) {
    const SpawnGroup *group = chooseGroup(b, cx, cy, animals);
    if (!group)
        return;
    const int S = animals ? 5 : 1;
    if (b.count(cx, cy, S) == 0)
        return;
    // 2. How it comes.
    std::vector<uint32_t> kinds;
    for (uint32_t k : {1u, 2u, 4u, 8u, 0x10u})
        if (group->kinds & k)
            kinds.push_back(k);
    const uint32_t k = kinds.empty() ? 0 : kinds[(size_t)(b.rand() % (int)kinds.size())];
    // 3, 4.
    int n = 1;
    if (k == 2 || k == 4 || k == 8)
        n = std::min(b.rand() % 3 + 3, b.count(cx, cy, S));
    else if (k == 0x10)
        n = std::min(b.rand() % 8 + 6, b.count(cx, cy, S));
    const int odds = k == 8 ? 2 : k == 4 ? 8 : 20;
    const int32_t sharedLevel = b.rand() % 5;
    // 5.
    for (int i = 0; i < n; i++) {
        SpawnPoint p;
        p.race = group->races[(size_t)(b.rand() % (int)group->races.size())];
        p.level = k == 4 || k == 8 ? sharedLevel : b.tables.raceLevel(p.race);
        if (p.race == 0xaf || (p.race >= 0xb1 && p.race <= 0xb4))
            p.hostility = hostility::ANIMAL;
        else if (i == 0 && b.rand() % odds == 0)
            p.addFlags(RESIDENT | STRONG);
        b.looks(p, true);
        b.gear(p);
        b.place(p, b.take(cx, cy, S));
        p.roam = k == 8 ? 0x30 : 0x10;
        const int64_t rider = b.add(p);
        if (p.race == 0x10e) { // its mount
            SpawnPoint m;
            m.race = 0xa5;
            m.level = p.level;
            b.looks(m, true);
            b.gear(m);
            m.x = p.x, m.y = p.y, m.z = p.z;
            m.appearance.setFlags(p.flags());
            m.owner = rider;
            b.add(m);
        }
    }
    // 6. A camp.
    if (k != 8)
        return;
    const int m = std::min(b.count(cx, cy, S), 6);
    for (int j = 0; j < m; j++) {
        const Spot s = b.take(cx, cy, S);
        StaticObject o = makeObject(objecttype::CAMPFIRE, s.x, s.y, s.z, (uint8_t)(b.rand() % 4));
        if (j > 0) {
            if (b.rand() % 4 == 0) {
                static constexpr int32_t THINGS[6] = {0x25, 0x26, 0x27, 0x28, 0x5d, 0x1c};
                if (o.z < 0x190000 && b.rand() % 3 == 0)
                    o.type = 0x5e;
                else
                    o.type = THINGS[b.rand() % 6];
            } else {
                o.type = o.z > 0x18ffff ? 0x60 : 0x5f;
            }
        }
        b.zone.keepIfInside(o);
    }
}

// The test before a wild group in the cell pass (from 0x3181aa).
void wildGroupTest(CreatureBuilder &b, int32_t cx, int32_t cy) {
    if (b.rand() % 128 != 0)
        return;
    const Place &place = b.zone.place.place;
    const int32_t mx = 16 * cx + 8, my = 16 * cy + 8;
    const float w2 = placeWeight(place.kind, place.big, warpedDist2(place.point, mx, my));
    if (!(roads(mx, my, place.kind, w2) < 0.25f))
        return;
    const Int2 q = areaLandmark(floorDiv(cx, 6), floorDiv(cy, 6));
    const int64_t dx = (int64_t)q.x - cx, dy = (int64_t)q.y - cy;
    if (dx * dx + dy * dy > 16)
        wildGroup(b, cx, cy, false);
}

// --- §2.5 Castles ----------------------------------------------------------------------------

void castleCreature(CreatureBuilder &b, int32_t cx, int32_t cy, CastleCell k) {
    // 1.
    int32_t px = floorDiv(16 * cx, PLACE_SHIFT), py = floorDiv(16 * cy, PLACE_SHIFT);
    const bool big = bigCell(px, py);
    if (big)
        px = 2 * floorDiv(px, 1), py = 2 * floorDiv(py, 1);
    const int kind = placeKind(px, py, big);
    const int T = bigStructure(px, py, kind, big);
    // 2-4.
    if (b.count(cx, cy, 6) == 0)
        return;
    SpawnPoint p;
    const int32_t ax = floorDiv(cx, 3) * 8, ay = floorDiv(cy, 3) * 8;
    for (int32_t x = ax; x < ax + 8; x++)
        for (int32_t y = ay; y < ay + 8; y++) {
            const CastleCell c = b.rules.castleCell(x, y);
            if (c.kind != 0 && c.detail == 2)
                p.area.push_back({x, y});
        }
    p.level = std::min((k.steps >> 2) + 1, 4);
    const uint32_t sigma = placeStyle(px, py);
    // 5.
    switch (k.kind) {
    case 2:
        p.race = 0xe2;
        p.addFlags(RESIDENT | STRONG);
        b.seed(random(cellKey(cx, cy) + 0x53735));
        b.castleRace(p, T, sigma);
        b.seed(random(cellKey(cx, cy) + 0x8043e));
        p.race = b.variant(p.race);
        p.roam = 4;
        break;
    case 4: // the end boss
        p.race = 0xe3;
        p.addFlags(BIG | RESIDENT | STRONG);
        b.seed(random(cellKey(px, py) + 0x20d2d));
        b.castleRace(p, T, sigma);
        b.seed(random(cellKey(cx, py) + 0x1191ac));
        p.race = b.variant(p.race);
        p.roam = 4;
        break;
    case 6: {
        b.seed(random(cellKey(cx, cy)) + 665325);
        bool up = false;
        if (b.rand() % 8 == 0) {
            p.level = std::min(p.level + 1, 4);
            up = b.rand() % 2 != 0;
        }
        b.seed(random(cellKey(px, py)) + random(cellKey(floorDiv(cx, 3), floorDiv(cy, 3))) + xorshift((uint32_t)p.level) + (up ? 1u : 0u));
        b.castleRace(p, T, sigma);
        b.seed(random(cellKey(cx, cy) + 0x5a48));
        p.race = b.variant(p.race);
        p.addFlags(RESIDENT);
        p.roam = up ? 0x10 : 4;
        break;
    }
    default: return;
    }
    b.looks(p);
    b.gear(p);
    b.place(p, b.take(cx, cy, 6));
    // 6.
    p.yaw = b.yawB();
    const float height = p.appearance.height();
    if (height > 6.0f) {
        const float m = 6.0f / height;
        for (size_t at : {Appearance::SCALE, Appearance::SCALE + 8, Appearance::SCALE + 4})
            p.appearance.set(at, m * p.appearance.get<float>(at));
    }
    b.add(p);
}

// --- §2.6 Dungeons ---------------------------------------------------------------------------

// The dungeon's floor F and its cell D (§1), once per zone.
struct DungeonFloor {
    int floor = 0;
    Int2 cell; // unset (0, 0) when no floor is found
};

DungeonFloor dungeonFloor(CreatureBuilder &b) {
    thread_local Int2 zoneOf{0x7fffffff, 0};
    thread_local DungeonFloor cached;
    if (zoneOf == Int2{b.zone.zx, b.zone.zy})
        return cached;
    DungeonFloor f;
    const Place &place = b.zone.place.place;
    const int32_t ax = floorDiv(place.point.x, 4), ay = floorDiv(place.point.y, 4);
    const int32_t h = fortressSquare(b.zone.place.cell, place.point).size / 2;
    for (int32_t x = ax - h; x < ax + h; x++)
        for (int32_t y = ay - h; y < ay + h; y++) {
            const int t = b.rules.type(x, y);
            if (t < 45 || t > 47)
                continue;
            const int code = b.rules.fortressFloorCode(x, y, 0);
            if (code >= 2 && code <= 4)
                f.floor = code, f.cell = {x, y};
        }
    zoneOf = {b.zone.zx, b.zone.zy};
    cached = f;
    return f;
}

void dungeonCreatures(CreatureBuilder &b, int32_t cx, int32_t cy) {
    ZoneBuild &zone = b.zone;
    const RegionPlan *plan = b.plan();
    if (!plan)
        return;
    const DungeonFloor F = dungeonFloor(b);
    const Int2 P = zone.place.cell;
    const uint32_t ph = cellKey(P.x, P.y);
    const int lvl = plan->levelAt(P.x, P.y);
    const uint32_t sigma = placeStyle(P.x, P.y);
    constexpr int DUNGEON = 8;
    const int object = b.rules.type(cx, cy);
    const bool fortressCell = object >= 45 && object <= 47;
    const uint32_t ch = cellKey(cx, cy);
    // The guards at a gate.
    if ((random(ph + 0x10581) & 1) || plan->regionItem(P.x, P.y) == 8) {
        if (!fortressCell) {
            Int2 gate;
            bool found = false;
            for (Int2 n : {Int2{cx - 1, cy}, Int2{cx + 1, cy}, Int2{cx, cy - 1}, Int2{cx, cy + 1}})
                if (!found && b.rules.type(n.x, n.y) == 0x2e)
                    gate = n, found = true;
            if (!found)
                return;
            const int n = (random(ph + 0x11c135) & 1) ? 1 : (int)(random(ph + 0x3c77a) & 3) + 3;
            b.seed(random(ph) + 9003);
            for (int i = 0; i < n; i++) {
                SpawnPoint p;
                p.race = b.variant(2);
                p.area.push_back(gate);
                p.level = std::max(lvl - 1 + b.rand() % 2, 0);
                b.seed(random(ph + 0x2fdf5d));
                b.castleRace(p, DUNGEON, sigma);
                p.addFlags(RESIDENT | (n == 1 ? STRONG : 0));
                b.seed(random(ch + 0x93ac8) + (uint32_t)i);
                p.race = b.variant(p.race);
                b.looks(p);
                b.gear(p);
                b.place(p, b.take(cx, cy, 0));
                p.roam = 0x14;
                p.yaw = b.yawB();
                b.add(p);
            }
            return;
        }
    }
    if (!fortressCell)
        return;
    // 1.
    int level = lvl;
    if (b.rules.fortressCellCode(cx, cy) == 2)
        level = std::max(level - 1, 0);
    // 2. The boss in the middle.
    const Place &place = zone.place.place;
    if (cx == floorDiv(place.point.x, 4) && cy == floorDiv(place.point.y, 4)) {
        SpawnPoint p;
        p.addFlags(BIG | RESIDENT | STRONG);
        b.seed(random(ph + 0x1d5dde1));
        b.castleRace(p, DUNGEON, sigma);
        p.level = level;
        p.roam = 0x10;
        const Item treasure = plan->roleTreasure(P.x, P.y);
        if (treasure.category != 0)
            p.gear.inventory.add(treasure, 1);
        b.seed(random(ch + 0x1260b4));
        p.race = b.variant(p.race);
        b.looks(p);
        b.gear(p);
        b.place(p, b.take(cx, cy, 2));
        b.add(p);
        return;
    }
    // 3. Floor guards.
    for (int i = 0; i < 2; i++) {
        const int S = i == 0 ? 2 : 8;
        if (b.rules.fortressFloorCode(cx, cy, i) != 13 || b.count(cx, cy, S) == 0)
            continue;
        SpawnPoint p;
        p.addFlags(RESIDENT);
        b.seed(random(ph + 0x2ffdd));
        if (F.floor == 3) {
            b.castleRace(p, DUNGEON, sigma);
            p.addFlags(STRONG);
            p.roam = 0x10;
            Item key;
            key.category = 0x16;
            key.id = (posMod(P.y, 64) * 64 + posMod(P.x, 64)) % 20;
            key.region = regionAt(place.point.x, place.point.y);
            p.gear.inventory.add(key, 1);
            p.level = level;
        } else if (F.floor == 4) {
            p.hostility = hostility::ANIMAL;
            p.race = 0xd6;
            p.level = level;
            p.area.push_back(F.cell);
        } else {
            b.castleRace(p, DUNGEON, sigma);
            p.addFlags(BIG);
            p.level = level;
            p.roam = 0x10;
            p.area.push_back(F.cell);
        }
        b.seed(random(ch + 0x1260b4) + (uint32_t)i);
        p.race = b.variant(p.race);
        b.looks(p);
        b.gear(p);
        b.place(p, b.take(cx, cy, S));
        p.yaw = b.quarterYaw();
        b.add(p);
    }
    // 4. A group.
    if (b.count(cx, cy, 2) == 0 || !b.rules.floorExists(cx, cy, 0))
        return;
    b.seed(random(ch) + 665325);
    if (b.rand() % 6 != 0)
        return;
    const int n = b.rand() % 4 + 1;
    for (int i = 0; i < n; i++) {
        SpawnPoint p;
        b.seed(random(ch + 0x305d5) + (uint32_t)i);
        p.level = std::max(level - 1 + b.rand() % 2, 0);
        bool up = false;
        if (b.rand() % 8 == 0)
            p.level = std::min(p.level + 1, 4), up = true;
        b.seed(xorshift((uint32_t)p.level) + random(ph) + (up ? 1u : 0u));
        b.castleRace(p, DUNGEON, sigma);
        b.seed(random(ch + 0x127fd) + (uint32_t)i);
        p.race = b.variant(p.race);
        p.addFlags(RESIDENT);
        b.looks(p);
        b.gear(p);
        b.place(p, b.take(cx, cy, 2));
        p.roam = up ? 0x10 : 4;
        p.yaw = b.yawB();
        b.add(p);
    }
}

// --- §2.8 Animals ----------------------------------------------------------------------------

void animals(CreatureBuilder &b, int32_t cx, int32_t cy) {
    if (b.rand() % 64 == 0 && b.count(cx, cy, 16) != 0) {
        SpawnPoint p;
        p.hostility = hostility::ANIMAL;
        b.place(p, b.take(cx, cy, 16));
        const int32_t X = blocksTowardZero(p.x), Y = blocksTowardZero(p.y);
        const Neighbourhood n = neighbourhood(X, Y);
        const Place place = placeAt(X, Y);
        const float warm = warmth(n, place.kind, place.weight);
        if (lavaRegion(n, place.kind, place.weight) > 0.5f) {
            p.race = b.rand() % 2 == 0 ? 0x7e : 0x82;
        } else if (warm < 0.25f) {
            p.race = b.rand() % 2 == 0 ? 0x79 : 0x7a;
        } else if (dryness(n) < 0.2f) {
            p.race = 0x7c;
            if (b.rand() % 2 != 0 && warm > 0.8f)
                p.race = 0x80;
        } else {
            static constexpr int32_t RACES[3] = {0x78, 0x7b, 0x7d};
            p.race = RACES[b.rand() % 3];
            if (b.rand() % 10 == 0)
                p.race = 0x7f;
        }
        b.looks(p);
        b.add(p);
    }
    if (b.rand() % 16 == 0 && b.count(cx, cy, 5) != 0) {
        if (b.rand() % 4 == 0) {
            wildGroup(b, cx, cy, true);
            return;
        }
        SpawnPoint p;
        p.hostility = hostility::ANIMAL;
        p.race = 0x84;
        // Three in four keep the base race and draw nothing more for it; the rest take
        // 0x83 or 0x85 and then walk the rarer chain 0x87..0x8a until a draw stops it.
        if (b.rand() % 4 == 0) {
            p.race = b.rand() % 2 != 0 ? 0x83 : 0x85;
            for (int32_t r : {0x87, 0x88, 0x89, 0x8a}) {
                if (b.rand() % 2 != 0)
                    break;
                p.race = r;
            }
        }
        b.looks(p);
        b.place(p, b.take(cx, cy, 5));
        b.add(p);
    }
}

// --- §3 The marking pass ---------------------------------------------------------------------

void markedRegion(CreatureBuilder &b) {
    if (!b.plan() || !b.plan()->marked)
        return;
    for (auto &[id, p] : b.zone.spawns.points) {
        if (p.hostility != hostility::HOSTILE && p.hostility != hostility::FRIENDLY)
            continue;
        if (p.hostility == hostility::FRIENDLY && (p.flags() & RESIDENT))
            continue;
        if (markingExempt(p.race))
            continue;
        if ((p.flags() & (GIANT | STRONG)) || b.rand() % 50 == 0)
            p.addFlags(DEMON_MARKED);
    }
}

// --- §4 The daily refresh --------------------------------------------------------------------

void refresh(CreatureBuilder &b, int32_t day) {
    ZoneBuild &zone = b.zone;
    ZoneSpawns &spawns = zone.spawns;
    // 4.2 Shoppers.
    b.seed(random(cellKey(zone.zx, zone.zy) + 0x4373e1) + (uint32_t)day);
    const int shops = (int)spawns.shopSpots.size();
    if (shops > 0) {
        const int count = std::max(b.rand() % (shops + 1), 2);
        for (int i = 0; i < count; i++) {
            SpawnPoint p;
            p.hostility = hostility::FRIENDLY;
            p.level = b.rand() % 5;
            p.race = b.variant(b.pick(Picker::PeopleOrBeasts));
            p.roam = 0x20;
            p.gear.cls = (uint8_t)(b.rand() % 4 + 1);
            b.looks(p);
            b.gear(p);
            // Past the end of the list the game reads beyond it (open question 4): the last spot.
            const ZoneSpawns::ShopSpot &s = spawns.shopSpots[(size_t)std::min(i, shops - 1)];
            p.x = s.x, p.y = s.y, p.z = s.z;
            spawns.refreshed.push_back(b.add(p));
        }
    }
    // 4.3 The travellers' camp.
    if (posMod(3 * zone.zy + day + zone.zx, 5) != 0 || b.rand() % 5 != 0)
        return;
    int n = b.rand() % 4 + 1;
    const int32_t level = b.rand() % 5;
    b.rand(); // drawn and not used
    bool fire = b.rand() % 4 == 0;
    const int32_t cx = floorDiv(64 * zone.zx, 4) + b.rand() % 4;
    const int32_t cy = floorDiv(64 * zone.zy, 4) + b.rand() % 4;
    int32_t race = b.pick(Picker::PeopleOrBeasts, biomeAt(16 * cx + 8, 16 * cy + 8));
    const int S = b.count(cx, cy, 5) != 0 ? 5 : 1;
    if (b.count(cx, cy, S) == 0)
        return;
    int32_t host = hostility::FRIENDLY, people = 0;
    uint8_t cls = 0;
    uint32_t style = 0, extra = 0;
    if (b.rand() % 4 == 0) {
        race = 0x130, cls = 0x9c, n = 1;
    } else if (zone.place.place.kind == 5) {
        fire = false;
    } else {
        const uint32_t sigma = placeStyle(floorDiv(16 * cx, PLACE_SHIFT), floorDiv(16 * cy, PLACE_SHIFT));
        if (b.rand() % 4 == 0) {
            people = stylePeople(sigma);
            host = people >= 2 && people <= 6 ? hostility::HOSTILE : hostility::FRIENDLY;
            switch (people) {
            case 1: cls = 0x8d, race = b.traveller(); break;
            case 2: cls = 0x8e, race = b.traveller(); break;
            case 3: race = b.pick(Picker::Imps); break;
            case 4: race = b.pick(Picker::BeastLeader); break;
            case 5: race = b.pick(Picker::UndeadLeader); break;
            case 6: race = 0xec; break;
            case 8: cls = 0x8f, race = b.pick(Picker::KindEightPeople); break;
            default: break;
            }
        } else {
            race = styleBoss(sigma, (uint32_t)b.rand());
            people = styleRace(sigma);
            style = sigma;
            host = styleOdd(sigma) ? hostility::HOSTILE : hostility::FRIENDLY;
            if (!styleEven(sigma))
                extra = FOREIGN_STYLE;
        }
    }
    std::set<int64_t> travellers;
    for (int i = 0; i < n; i++) {
        SpawnPoint p;
        p.hostility = host;
        p.level = level;
        p.race = b.variant(race);
        p.roam = fire ? 0x30 : 0x1030;
        p.gear.cls = cls;
        p.people = people;
        p.style = style;
        p.addFlags(extra);
        b.looks(p);
        b.gear(p);
        b.place(p, b.take(cx, cy, S));
        const int64_t id = b.add(p);
        spawns.refreshed.push_back(id);
        travellers.insert(id);
    }
    for (int64_t id : spawns.refreshed)
        if (SpawnPoint *p = spawns.find(id))
            p->group = travellers;
    if (fire)
        campObjects(zone, cx, cy, S, race);
}

} // namespace

PlannerFacts plannerFacts(const ZonePlace &place) {
    // The plan of the region of the place's landmark point; (0, 0) for a zone without a place
    // (worldgen-creatures.md open question 1).
    PlannerFacts f;
    f.plan = regionPlan(place.region.x, place.region.y);
    f.role = f.plan->roleAt(place.cell.x, place.cell.y);
    f.items = f.plan->items;
    f.treasure = f.plan->roleTreasure(place.cell.x, place.cell.y);
    return f;
}

StructureHooks creatureHooks(const CreatureTables &tables, int32_t day) {
    StructureHooks h;
    const CreatureTables *t = &tables;
    h.planner = plannerFacts;
    h.arenaMaster = [t, day](ZoneBuild &zone, int32_t cx, int32_t cy, int32_t base) {
        CreatureBuilder b(zone, *t, day);
        arenaMaster(b, cx, cy, base);
    };
    h.roleThings = [t, day](ZoneBuild &zone, int role) {
        CreatureBuilder b(zone, *t, day);
        roleThings(b, role);
    };
    h.cells.townsfolk = [t, day](ZoneBuild &zone, int32_t cx, int32_t cy) {
        CreatureBuilder b(zone, *t, day);
        return townsfolk(b, cx, cy);
    };
    h.cells.firstTownPeople = [t, day](ZoneBuild &zone, int32_t cx, int32_t cy) {
        CreatureBuilder b(zone, *t, day);
        firstTownPeople(b, cx, cy);
    };
    h.cells.wildGroup = [t, day](ZoneBuild &zone, int32_t cx, int32_t cy) {
        CreatureBuilder b(zone, *t, day);
        wildGroupTest(b, cx, cy);
    };
    h.cells.castleCreatures = [t, day](ZoneBuild &zone, int32_t cx, int32_t cy, CastleCell k) {
        CreatureBuilder b(zone, *t, day);
        castleCreature(b, cx, cy, k);
    };
    h.cells.dungeonCreatures = [t, day](ZoneBuild &zone, int32_t cx, int32_t cy) {
        CreatureBuilder b(zone, *t, day);
        dungeonCreatures(b, cx, cy);
    };
    h.cells.animals = [t, day](ZoneBuild &zone, int32_t cx, int32_t cy) {
        CreatureBuilder b(zone, *t, day);
        animals(b, cx, cy);
    };
    h.markedRegion = [t, day](ZoneBuild &zone) {
        CreatureBuilder b(zone, *t, day);
        markedRegion(b);
    };
    return h;
}

RefreshHook creatureRefresh(const CreatureTables &tables) {
    const CreatureTables *t = &tables;
    return [t](ZoneBuild &zone, int day) {
        CreatureBuilder b(zone, *t, day);
        refresh(b, day);
    };
}

ZoneContents generateZoneWithCreatures(const WorldValues &world, const CreatureTables &tables, int32_t day, int32_t zx, int32_t zy,
                                       bool recordDraws) {
    StructureOptions options;
    options.hooks = creatureHooks(tables, day);
    options.recordDraws = recordDraws;
    return generateZone(world, zx, zy, options);
}

void refreshZoneCreatures(ZoneContents &zone, const WorldValues &world, const CreatureTables &tables, int32_t day) {
    refreshZone(zone, world, day, creatureRefresh(tables));
}

} // namespace worldgen
