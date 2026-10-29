// The world generator as the engine uses it (Generator.h).
#include "worldgen/Generator.h"

#include "worldgen/BlockColours.h"

#include <atomic>
#include <list>
#include <mutex>

namespace worldgen {
namespace {

// --- The world's settings --------------------------------------------------------------------

struct Settings {
    WorldValues world = worldValues(0);
    std::atomic<int32_t> day{0};
    std::atomic<int32_t> refreshDay{-1};
    bool tablesLoaded = false;
};
Settings settings;
// Changes with every setting, so cached zones made under other settings are made again.
std::atomic<uint32_t> settingsVersion{1};

// --- The zone cache --------------------------------------------------------------------------

struct CachedZone {
    int32_t zx, zy;
    uint32_t settings, options;
    bool original;
    std::shared_ptr<const ZoneContents> zone;
};
std::mutex cacheLock;
std::list<CachedZone> cache; // the most recently used first
constexpr size_t CACHED_ZONES = 96; // a lit zone reads its 8 neighbours; the near zones fit

ZoneContents makeZone(int32_t zx, int32_t zy, bool recordDraws) {
    const WorldValues &world = settings.world;
    if (!settings.tablesLoaded) {
        // Without the creature tables: the structures and the planner's role items only.
        StructureOptions options;
        options.hooks.planner = plannerFacts;
        options.recordDraws = recordDraws;
        return generateZone(world, zx, zy, options);
    }
    return generateZoneWithCreatures(world, creatureTables(), settings.day, zx, zy, recordDraws);
}

constexpr int SPECIAL_ITEM = 24;  // the category of role items and boats
constexpr int BOAT_ID = 2;
constexpr int ROLE_ITEM_IDS[8] = {1, 2, 0x12, 3, 5, 7, 8, 9}; // roles 6..13 (contents file §C4)

} // namespace

// --- The world -------------------------------------------------------------------------------------

void setWorldSeed(int32_t seed) {
    if (seed == settings.world.seed)
        return;
    settings.world = worldValues(seed);
    settingsVersion++;
}
const WorldValues &currentWorld() { return settings.world; }

void setWorldDay(int32_t day) {
    if (settings.day.exchange(day) != day)
        settingsVersion++;
}
int32_t worldDay() { return settings.day; }

void setRefreshDay(int32_t day) {
    if (settings.refreshDay.exchange(day) != day)
        settingsVersion++;
}

bool loadCreatureTables(const std::filesystem::path &oracleDir, std::string *why) {
    settings.tablesLoaded = sharedCreatureTables().load(oracleDir, why);
    settingsVersion++;
    return settings.tablesLoaded;
}
bool creatureTablesLoaded() { return settings.tablesLoaded; }
const CreatureTables &creatureTables() { return sharedCreatureTables(); }

// --- Cells and columns -----------------------------------------------------------------------------

int cellObject(int32_t cx, int32_t cy) { return cellObjectsFor(settings.world).type(cx, cy); }

CastleCell castleCell(int32_t cx, int32_t cy) { return BuildingRules(cellObjectsFor(settings.world)).castleCell(cx, cy); }

int32_t cellHeight(int32_t cx, int32_t cy, int type) { return BuildingRules(cellObjectsFor(settings.world)).baseHeight(cx, cy, type); }

ColumnField columnField(int32_t x, int32_t y) {
    const Place cell = cellPlace(floorDiv(x, PLACE_SHIFT), floorDiv(y, PLACE_SHIFT));
    const ColumnValues c = columnValues(x, y, placeForColumn(cell, x, y), 0);
    ColumnField f;
    f.place = c.place;
    f.base = c.base;
    f.height = c.height;
    f.lake = c.lake;
    f.river = c.river;
    f.colours = colourInputs(c);
    return f;
}

// --- Regions and places --------------------------------------------------------------------------

RegionRoles regionRoles(Int2 region) {
    const std::shared_ptr<const RegionPlan> plan = regionPlan(region.x, region.y);
    RegionRoles out;
    for (const auto &[cell, role] : plan->roles)
        if (const int hooked = plan->roleAt(cell.first, cell.second); hooked != 0)
            out.roles.push_back({cell.first, cell.second, hooked});
    for (const auto &[cell, level] : plan->levels)
        out.levels.push_back({cell.first, cell.second, level});
    out.special = plan->specialTown;
    out.marked = plan->marked;
    out.items = plan->items;
    return out;
}

int plannerRole(Int2 region, int32_t px, int32_t py) { return regionPlan(region.x, region.y)->roleAt(px, py); }
int plannerLevel(Int2 region, int32_t px, int32_t py) { return regionPlan(region.x, region.y)->levelAt(px, py); }
int plannerEncounter(Int2 region, int32_t px, int32_t py, int32_t day) { return regionPlan(region.x, region.y)->encounter(px, py, day); }

PlaceEncounter placeEncounter(int32_t px, int32_t py) {
    PlaceEncounter e;
    e.big = bigCell(px, py);
    if (e.big)
        px = 2 * floorDiv(px, 1), py = 2 * floorDiv(py, 1);
    e.kind = placeKind(px, py, e.big);
    if (e.kind == 0)
        return e;
    e.point = cellPoint(px, py, e.big);
    const Int2 region = regionAt(e.point.x, e.point.y);
    e.role = plannerRole(region, px, py);
    e.encounter = e.role >= role::WITCH && e.role <= role::SPIDERS ? 100 + e.role : plannerEncounter(region, px, py, settings.day);
    e.zx = floorDiv(e.point.x, ZONE_SHIFT), e.zy = floorDiv(e.point.y, ZONE_SHIFT);
    return e;
}

// --- Zones -----------------------------------------------------------------------------------------

std::shared_ptr<const ZoneContents> zone(int32_t zx, int32_t zy) {
    const uint32_t version = settingsVersion.load(), options = optionsVersion();
    const bool original = generatingOriginal();
    {
        std::lock_guard<std::mutex> hold(cacheLock);
        for (auto it = cache.begin(); it != cache.end(); ++it)
            if (it->zx == zx && it->zy == zy && it->settings == version && it->options == options && it->original == original) {
                cache.splice(cache.begin(), cache, it);
                return it->zone;
            }
    }
    ZoneContents z = makeZone(zx, zy, false);
    const int32_t refreshDay = settings.refreshDay.load();
    if (refreshDay >= 0 && settings.tablesLoaded)
        refreshZoneCreatures(z, settings.world, creatureTables(), refreshDay);
    // The column values were the generator's working state; a kept zone has no use for them.
    z.terrain.field = {};
    auto made = std::make_shared<const ZoneContents>(std::move(z));
    std::lock_guard<std::mutex> hold(cacheLock);
    cache.push_front({zx, zy, version, options, original, made});
    if (cache.size() > CACHED_ZONES)
        cache.pop_back();
    return made;
}

std::vector<TerrainColumn> litZone(int32_t zx, int32_t zy) {
    // The nine zones the light pass reads, held while it runs.
    std::shared_ptr<const ZoneContents> around[3][3];
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            around[i][j] = zone(zx - 1 + i, zy - 1 + j);
    return lightZone(zx, zy, [&](int32_t x, int32_t y) -> const std::vector<TerrainColumn> & {
        return around[x - zx + 1][y - zy + 1]->terrain.columns;
    });
}

std::vector<uint16_t> zoneDraws(int32_t zx, int32_t zy) { return makeZone(zx, zy, true).terrain.draws; }

std::vector<ZoneObject> zoneObjects(int32_t zx, int32_t zy) {
    const std::shared_ptr<const ZoneContents> z = zone(zx, zy);
    std::vector<ZoneObject> out;
    out.reserve(z->objects.size());
    for (size_t i = 0; i < z->objects.size(); i++) {
        ZoneObject o;
        static_cast<StaticObject &>(o) = z->objects[i];
        if (const auto r = z->rewards.find((int)i); r != z->rewards.end())
            o.reward = r->second;
        out.push_back(o);
    }
    return out;
}

bool hooksChange(int32_t zx, int32_t zy) {
    if (!modHooksSet())
        return false;
    // The zone's place cell and its neighbours (whose landmarks and structures may reach into it).
    const int32_t pcx = floorDiv(zx, PLACE_SHIFT - ZONE_SHIFT), pcy = floorDiv(zy, PLACE_SHIFT - ZONE_SHIFT);
    for (int32_t x = pcx - 1; x <= pcx + 1; x++)
        for (int32_t y = pcy - 1; y <= pcy + 1; y++) {
            const bool big = bigCell(x, y);
            const int hooked = placeKind(x, y, big);
            OriginalGeneration game;
            if (placeKind(x, y, big) != hooked)
                return true;
        }
    // Its place's role and role item.
    const bool big = bigCell(pcx, pcy);
    const int32_t px = big ? 2 * floorDiv(pcx, 1) : pcx, py = big ? 2 * floorDiv(pcy, 1) : pcy;
    const Int2 point = cellPoint(px, py, big);
    const Int2 region = regionAt(point.x, point.y);
    const int hookedRole = plannerRole(region, px, py);
    int gameRole;
    {
        OriginalGeneration game;
        gameRole = plannerRole(region, px, py);
    }
    if (hookedRole != gameRole)
        return true;
    if (gameRole >= 6 && gameRole <= 13 && !keepsGeneratedItem(SPECIAL_ITEM, ROLE_ITEM_IDS[gameRole - 6]))
        return true;
    // A region's boat lies on a cell whose coordinates are multiples of 16 (zone coordinates of 4).
    if (zx % 4 == 0 && zy % 4 == 0 && !keepsGeneratedItem(SPECIAL_ITEM, BOAT_ID))
        return true;
    return false;
}

void forgetZones() {
    std::lock_guard<std::mutex> hold(cacheLock);
    cache.clear();
}

std::vector<soundrules::Emitter> soundEmitters(int32_t zx, int32_t zy) {
    std::vector<soundrules::Emitter> out;
    for (const Model &m : zone(zx, zy)->emitters) {
        soundrules::Emitter e;
        e.type = (uint32_t)m.type;
        e.position[0] = m.x, e.position[1] = m.y, e.position[2] = m.z;
        out.push_back(e);
    }
    return out;
}

} // namespace worldgen
