// The zone generator's part B, written from docs/specs/worldgen-structures.md §2, §5 and §7.
#include "worldgen/Structures.h"

#include "worldgen/BlockColours.h"
#include "worldgen/Buildings.h"
#include "worldgen/Height.h"
#include "worldgen/Mason.h"
#include "worldgen/Nature.h"
#include "worldgen/Options.h"

#include <cmath>
#include <memory>

namespace worldgen {
namespace {

constexpr int32_t OVERHANG_LIGHT = 0x59;
constexpr uint32_t STRUCTURE_SEEDS = 10000000;

// The tree on a pedestal with four benches (cell objects 6 and 7, §7).
void pedestalTree(ZoneBuild &zone, int type, Int2 p, int32_t z, float zf, int32_t th, int32_t tw) {
    const int32_t tz = trunc32(zf + 3.0f), ph = trunc32((float)th * 0.8f);
    if (type == 7)
        palm(zone, p.x, p.y, tz, ph, tw);
    else
        commonTree(zone, p.x, p.y, tz, ph, tw);
    Mason m(zone);
    m.oneColour({240, 240, 240}, pattern::STONE);
    m.pillar(p.x, p.y, z, 6, 3, 0, 0);
    m.disc(p.x, p.y, z, 5, 3, 2, 0);
    const int bench = forestRegion(neighbourhood(p.x, p.y), 0, 0.0f) > 0.5f ? 31 : 30;
    const int64_t bz = trunc64((zf + 2.0f) * 65536.0f);
    const Int2 at[4] = {{p.x, p.y - 6}, {p.x + 8, p.y}, {p.x, p.y + 8}, {p.x - 6, p.y}};
    for (int i = 0; i < 4; i++)
        zone.settleAndKeep(makeObject(bench, dotsOf(at[i].x), dotsOf(at[i].y), bz, (uint8_t)i));
}

// The natural structure of a cell (§7 step 5).
void naturalStructure(ZoneBuild &zone, int32_t cx, int32_t cy, int type) {
    const bool centred = type == 8 || (type >= 11 && type <= 14) || type == 55;
    const Int2 p = centred ? Int2{16 * cx + 8, 16 * cy + 8} : cellPoint(cx, cy);
    float h = groundHeight(p.x, p.y, heightmode::CLIFF_STEPS);
    const bool underWater = 0.0f > h;
    if (underWater)
        h = 0.0f;
    int32_t th = cellRandom(cx, cy, 0x2fbcc94) % 10 + 20, tw = cellRandom(cx, cy, 0x12ce32c) % 10 + 12;
    if (type == 8)
        th = 50, tw *= 3;
    else if (type == 11 || type == 13)
        th = 64, tw = 40;
    else if (type == 12 || type == 14)
        th = 64, tw = 80;
    const int32_t r = trunc32((float)th * 1.25f);
    if (!(p.x + r >= zone.x0 && p.y + r >= zone.y0 && p.x - r < zone.x0 + ZONE_SIZE && p.y - r < zone.y0 + ZONE_SIZE))
        return;
    zone.seed(random(cellKey(cx, cy)) % STRUCTURE_SEEDS);
    const int32_t z = trunc32(h + 1.0f);
    const float zf = h + 1.0f;
    auto rocky = [&] { return zone.columnValuesAt(p.x, p.y).colours.highlands > 0.5f; };
    auto underWaterHill = [&] { hill(zone, p.x, p.y, -8, 2 * th, 2 * th, true); };
    switch (type) {
    case 1: case 8: commonTree(zone, p.x, p.y, z, th, tw); break;
    case 2: fir(zone, p.x, p.y, z, th, tw); break;
    case 3: birch(zone, p.x, p.y, z, th, tw); break;
    case 4: willow(zone, p.x, p.y, z, th, tw); break;
    case 5: palm(zone, p.x, p.y, z, th, tw); break;
    case 6: case 7: pedestalTree(zone, type, p, z, zf, th, tw); break;
    case 9: rock(zone, p.x, p.y, z, th, th, tw, 0); break;
    case 10: hill(zone, p.x, p.y, trunc32((float)tw + zf), 2 * th, 2 * th, rocky()); break;
    case 11: case 12: case 13: case 14: hill(zone, p.x, p.y, trunc32((float)tw + zf), th, th, true); break;
    case 15: {
        const bool isRocky = rocky();
        Canyon canyon;
        canyon.open = zone.draw() % 2 != 0;
        const float a = zone.angle();
        const int32_t R = 2 * th;
        canyon.end = {trunc32(std::cos(a) * (float)(4 * R) + (float)p.x), trunc32(std::sin(a) * (float)(4 * R) + (float)p.y)};
        hill(zone, p.x, p.y, trunc32((float)tw + zf), R, R, isRocky, &canyon);
        break;
    }
    case 19: stump(zone, p.x, p.y, z, th, tw); break;
    case 20: case 21: {
        const bool alongX = zone.draw() % 2 != 0;
        bool hollow;
        float radius;
        if (type == 20) {
            hollow = zone.draw() % 3 == 0;
            radius = (float)(zone.draw() % 4 + 2);
        } else {
            hollow = true;
            radius = (float)(zone.draw() % 4 + 3);
        }
        fallenLog(zone, p.x, p.y, z, radius, th, hollow, !alongX);
        break;
    }
    case 37: case 51: case 53: case 69: case 70:
        if (underWater)
            underWaterHill();
        break;
    case 54: case 55:
        if (underWater)
            underWaterHill();
        boulder(zone, p.x, p.y, z, 8, 8, 25, 8);
        break;
    case 62: sunkenChest(zone, cx, cy); break;
    default: break; // the others build nothing here
    }
}

// The cell loop (0x3025b0, §7).
void cellLoop(ZoneBuild &zone, BuildingRules &rules, const StructureHooks &hooks) {
    for (int i = -2; i <= 5; i++)
        for (int j = -2; j <= 5; j++) {
            const int32_t cx = 4 * zone.zx + i, cy = 4 * zone.zy + j;
            // 1, 2.
            const int type = rules.type(cx, cy);
            const CastleCell castle = rules.castleCell(cx, cy);
            const int reach = cellReach(type);
            const bool near = !((i < 0 && -i > reach) || (i >= 4 && i - 3 > reach) || (j < 0 && -j > reach) || (j >= 4 && j - 3 > reach));
            const bool inside = i >= 0 && i < 4 && j >= 0 && j < 4;
            // 3. The object beside a type-92 building.
            if (type == 92 && inside) {
                const int64_t y = dotsOf(16 * cy + zone.draw() % 8 + 4) + 32768;
                const int64_t x = dotsOf(16 * cx + zone.draw() % 8 + 4) + 32768;
                const int64_t z = trunc64((groundHeight(blocksTowardZero(x), blocksTowardZero(y), 0) + 1.0f) * 65536.0f);
                StaticObject o = makeObject(objecttype::CELL_92_OBJECT, x, y, z, (uint8_t)(zone.draw() % 4));
                zone.settle(o, false);
                zone.keepIfInside(o);
            }
            // 4. Set pieces and buildings.
            if (near && (type != 0 || castle.kind != 0)) {
                zone.seed(random(cellKey(cx, cy)) % STRUCTURE_SEEDS);
                switch (type) {
                case 60: mound(zone, 16 * cx + 8, 16 * cy + 8, rules.baseHeight(cx, cy, 60) - 4, 32, 32); break;
                case 69: case 70: towerPlatform(zone, rules, cx, cy, type); break;
                case 97:
                    arenaDais(zone, rules, cx, cy);
                    if (hooks.arenaMaster)
                        hooks.arenaMaster(zone, cx, cy, rules.baseHeight(cx, cy, 97));
                    break;
                case 67: aqueduct(zone, rules, cx, cy); break;
                case 66: waterTower(zone, rules, cx, cy); break;
                case 68: cistern(zone, rules, cx, cy); break;
                case 52: wizardTower(zone, rules, cx, cy); break;
                default: break;
                }
                if (castle.kind != 0 || isBuilding(type))
                    buildBuilding(zone, rules, i, j, cx, cy, type, castle);
                else if (type >= 27 && type <= 34 && inside)
                    cropField(zone, rules, cx, cy, type);
            }
            // 5. Natural structures.
            if (type != 0 && near)
                naturalStructure(zone, cx, cy, type);
        }
}

} // namespace

CellObjects &cellObjectsFor(const WorldValues &world) {
    // A cell's object depends on the place kinds, so the mods' world and the game's own (while a
    // hook asks for it) keep a cache each.
    struct Cache {
        std::unique_ptr<CellObjects> cells;
        int32_t seed = 0;
        const WorldValues *owner = nullptr;
        uint32_t version = 0;
    };
    thread_local Cache caches[2];
    Cache &c = caches[generatingOriginal() ? 1 : 0];
    if (!c.cells || c.seed != world.seed || c.owner != &world || c.version != optionsVersion()) {
        c.cells = std::make_unique<CellObjects>(world);
        c.seed = world.seed, c.owner = &world, c.version = optionsVersion();
    }
    return *c.cells;
}

ZonePlace zonePlace(const ZoneTerrain &terrain) {
    ZonePlace z;
    z.place = terrain.place;
    z.cell = terrain.placeCell;
    z.structure = bigStructure(z.cell.x, z.cell.y, z.place.kind, z.place.big);
    z.anchor = z.structure != 0 ? structureAnchor(z.cell.x, z.cell.y, z.place.kind, z.place.big) : NO_ANCHOR;
    z.style = placeStyle(z.cell.x, z.cell.y);
    z.people = 12 + (int)(xorshift(z.style + 0x7b2650) % 11);
    // With no place the game reads a landmark point it never wrote (open question 1): biome 0.
    z.region = regionAt(z.place.point.x, z.place.point.y);
    z.biome = z.place.kind != 0 ? regionBiome(z.region.x, z.region.y) : 0;
    z.culture = buildingCulture(z.style, z.biome);
    return z;
}

void refreshZone(ZoneContents &zone, const WorldValues &world, int day, const RefreshHook &partC) {
    // Once a day (worldgen-creatures-zone.md §4): the last refresh's creatures go, the objects come
    // back from the generator's copy.
    if (zone.spawns.day == day)
        return;
    zone.spawns.day = day;
    for (int64_t id : zone.spawns.refreshed)
        zone.spawns.points.erase(id);
    zone.spawns.refreshed.clear();
    zone.objects = zone.lists.objectsCopy;
    if (!partC)
        return;
    ZoneBuild b(zone.terrain, world, cellObjectsFor(world), false);
    b.place = zone.place;
    b.planner = zone.planner;
    b.objects = std::move(zone.objects);
    b.items = std::move(zone.items);
    b.rewards = std::move(zone.rewards);
    b.spots = zone.spots;
    b.working = zone.working;
    b.spawns = std::move(zone.spawns);
    partC(b, day);
    zone.spawns = std::move(b.spawns);
    zone.objects = std::move(b.objects);
    zone.items = std::move(b.items);
    zone.rewards = std::move(b.rewards);
    zone.working = std::move(b.working);
}

ZoneContents generateZone(const WorldValues &world, int32_t zx, int32_t zy, const StructureOptions &options) {
    ZoneContents out;
    // Steps 1 (the place) and 2 (the columns): part A, with the structure anchor.
    TerrainOptions terrainOptions;
    terrainOptions.anchor = terrainAnchor;
    terrainOptions.recordDraws = options.recordDraws;
    out.terrain = zoneTerrain(world, zx, zy, terrainOptions);
    CellObjects &cells = cellObjectsFor(world);
    BuildingRules rules(cells);
    ZoneBuild zone(out.terrain, world, cells, options.recordDraws);
    zone.place = zonePlace(out.terrain);
    if (options.hooks.planner)
        zone.planner = options.hooks.planner(zone.place);
    // The cliff pass's overhang lights (step 2) are the first models.
    for (const OverhangLight &l : out.terrain.lights) {
        Model m;
        m.type = OVERHANG_LIGHT, m.x = l.x, m.y = l.y, m.z = l.z;
        m.size = l.size, m.yaw = 1.0f, m.r = m.g = m.b = 1.0f, m.flags = 2;
        zone.models.push_back(m);
    }
    // 3, 4.
    cellLoop(zone, rules, options.hooks);
    streetLamps(zone, rules);
    // 5-7.
    findSpots(zone);
    cellThings(zone, rules);
    plants(zone);
    ModelLists modelLists7 = modelLists(zone.models); // (C2.1, the end of step 7)
    // 8. The region role's things.
    const int role = zone.planner.role;
    if (role >= 6 && role <= 13)
        roleItem(zone, role);
    else if (options.hooks.roleThings)
        options.hooks.roleThings(zone, role);
    // 9, 10.
    cellCreaturesAndItems(zone, rules, options.hooks.cells);
    if (options.hooks.markedRegion)
        options.hooks.markedRegion(zone);
    // 11, 12.
    copyAndRestamp(zone, out.lists);
    listLightsAndTreasures(zone, out.lists);
    markObjectBlocks(zone, out.lists);

    out.place = zone.place;
    out.planner = std::move(zone.planner);
    out.objects = std::move(zone.objects);
    out.models = std::move(zone.models);
    out.modelLights = std::move(modelLists7.lights);
    out.emitters = std::move(modelLists7.emitters);
    out.items = std::move(zone.items);
    out.rewards = std::move(zone.rewards);
    out.spots = std::move(zone.spots);
    out.working = std::move(zone.working);
    out.spawns = std::move(zone.spawns);
    return out;
}

} // namespace worldgen
