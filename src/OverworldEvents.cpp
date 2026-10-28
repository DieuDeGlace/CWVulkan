#include "OverworldEvents.h"

#include "Extracted.h"
#include "worldgen/Generator.h"
#include "NativeDrop.h"
#include "NativeRules.h"
#include "Spawned.h"
#include "events/DifficultyRules.h"
#include "events/EventHealthRules.h"
#include "events/EventObjectRules.h"
#include "events/InstanceRules.h"
#include "events/TownRules.h"
#include "events/TravelRules.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

using namespace encounters;

namespace {
#include "ItemKeys.inc"
#include "RaceKeys.inc"

// The game's creature record (cube::Creature) where the events read and change it.
constexpr size_t Position = 0x10, Pitch = 0x28, Roll = 0x2c, Yaw = 0x30, Velocity = 0x34, Acceleration = 0x40,
                 Retreat = 0x4c, Hostility = 0x60, Race = 0x64, Ability = 0x68, Flags = 0x80, GraphicsScale = 0x84,
                 HitboxScale = 0x88, PhysicsScale = 0x8c, ChestScale = 0xa4, ChestPositionZ = 0xe8, Toggles = 0x128,
                 Level = 0x144, Health = 0x180, InteractionState = 0x19c, Prisoner = 0x1a8, Home = 0x1b8,
                 Equipment = 0x278, Name = 0x958, Behaviours = 0xbcc, DrawnBefore = 0xaf8, Drawn = 0xb10,
                 Smoothing = 0xb54;
// Hostility (cube::Enums::EntityBehaviour) and appearance flags.
constexpr int HostileBehaviour = 1, VillagerBehaviour = 3;
constexpr uint32_t Fish = 1u << 8, Named = 1u << 9, NoTalk = 1u << 13, CanPickup = 1u << 14, Friendly = 1u << 15,
                   Mini = 1u << 17, Boss = 1u << 18, Petrified = 1u << 22;
constexpr uint16_t MapMarker = 1 << 8; // +0x128: shown on the map

template <class T> T get(const uint8_t *r, size_t at) {
    T v;
    std::memcpy(&v, r + at, sizeof v);
    return v;
}
template <class T> void set(uint8_t *r, size_t at, T v) { std::memcpy(r + at, &v, sizeof v); }

using Dots = std::array<int64_t, 3>;
Dots positionOf(const uint8_t *r) { return {get<int64_t>(r, Position), get<int64_t>(r, Position + 8), get<int64_t>(r, Position + 16)}; }
Site siteOf(const uint8_t *r) { return {floorDivide(get<int64_t>(r, Position), 65536), floorDivide(get<int64_t>(r, Position + 8), 65536)}; }
float hp(const uint8_t *r) { return get<float>(r, Health); }
float fraction(const uint8_t *r) {
    const float maximum = nativerules::maxHealth(r);
    return std::isfinite(maximum) && maximum > 0 ? std::clamp(hp(r) / maximum, 0.f, 1.f) : 1.f;
}
void setHome(uint8_t *r, const Dots &p) {
    for (int i = 0; i < 3; i++)
        set<int64_t>(r, Home + 8 * (size_t)i, p[(size_t)i]);
}
// Anchored objects and bound captives keep their simulated and drawn places together, so
// that smoothing does not pull them away (the creature inspection).
void setStationary(uint8_t *r, const Dots &p) {
    for (size_t at : {Position, DrawnBefore, Drawn})
        for (int i = 0; i < 3; i++)
            set<int64_t>(r, at + 8 * (size_t)i, p[(size_t)i]);
    set<float>(r, Smoothing, 0.f);
}
void stopMoving(uint8_t *r) {
    for (size_t at : {Velocity, Acceleration, Retreat})
        for (int i = 0; i < 3; i++)
            set<float>(r, at + 4 * (size_t)i, 0.f);
}
// the AI's idle step (0x10b90) wanders only with wandering, about the home the spawning sets.
void allowRoaming(uint8_t *r) {
    setHome(r, positionOf(r));
    set<uint32_t>(r, Behaviours, (get<uint32_t>(r, Behaviours) & ~0x80u) | 0x10u);
}
void friendlyActor(uint8_t *r) { set<uint32_t>(r, Flags, (get<uint32_t>(r, Flags) & ~(NoTalk | CanPickup | Petrified)) | Friendly); }
void noncombatObject(uint8_t *r) {
    set<uint8_t>(r, Hostility, (uint8_t)objects::ExamineBehaviour);
    set<float>(r, Health, 1.f); // positive liveness, without health on its plate
    std::memset(r + Equipment, 0, 11 * 0xa0);
    set<uint32_t>(r, Flags, get<uint32_t>(r, Flags) & ~(NoTalk | CanPickup | Petrified | Friendly | Named | Mini | Boss));
    stopMoving(r);
    set<uint8_t>(r, InteractionState, 1);
}
void nameBoss(uint8_t *r, uint64_t roll) {
    static const char *const first[] = {"Vor", "Mor", "Kel", "Zar", "Ash", "Grim", "Bel", "Fen"};
    static const char *const last[] = {"gath", "drak", "thorn", "mire", "gorn", "zul", "bane", "fang"};
    set<uint32_t>(r, Flags, (get<uint32_t>(r, Flags) & ~(Friendly | Mini)) | Named | Boss);
    char name[16];
    std::snprintf(name, sizeof name, "%s%s", first[roll % 8], last[(roll >> 16) % 8]);
    std::memcpy(r + Name, name, sizeof name);
    for (size_t at : {GraphicsScale, HitboxScale, PhysicsScale})
        set<float>(r, at, get<float>(r, at) * 1.3f);
}
void placeContainer(uint8_t *r, const Dots &floor) {
    set<float>(r, Pitch, 0.f), set<float>(r, Roll, 0.f);
    struct Point {
        int64_t x, y, z;
    } p{floor[0], floor[1], floor[2]};
    p = objects::containerPosition(p, (int)get<int32_t>(r, Race), get<float>(r, GraphicsScale), get<float>(r, HitboxScale),
                                   get<float>(r, PhysicsScale), get<float>(r, ChestScale), get<float>(r, ChestPositionZ));
    setStationary(r, {p.x, p.y, p.z});
    setHome(r, positionOf(r));
}

// The world's queries (the game functions the Steam mod calls).
bool coordinate(Site p) { return p.x >= INT32_MIN && p.x <= INT32_MAX && p.y >= INT32_MIN && p.y <= INT32_MAX; }
Region regionOf(Site p) { // 0x2ada20
    const worldgen::Int2 r = coordinate(p) ? worldgen::regionAt((int32_t)p.x, (int32_t)p.y) : worldgen::Int2{};
    return {r.x, r.y};
}
struct Location { // 0x35ce0
    int type = 0, radius = 0;
    bool big = false;
    Site centre;
    float distanceSquared = 0, influence = 0;
};
Location locationAt(Site p) {
    Location l;
    if (!coordinate(p))
        return l;
    const worldgen::Place place = worldgen::placeAt((int32_t)p.x, (int32_t)p.y);
    l.type = place.kind, l.radius = place.radius, l.big = place.big;
    l.centre = {place.point.x, place.point.y};
    l.distanceSquared = place.dist2, l.influence = place.weight;
    return l;
}
float predictedHeight(Site p) { // 0x35ea0: the generator's ground, loaded or not
    return coordinate(p) ? worldgen::groundHeight((int32_t)p.x, (int32_t)p.y, 0) : std::numeric_limits<float>::quiet_NaN();
}
std::array<float, 5> predictedFootprint(Site p) {
    std::array<float, 5> heights;
    for (size_t i = 0; i < heights.size(); i++)
        heights[i] = predictedHeight({p.x + town::HeightSamples[i].x, p.y + town::HeightSamples[i].y});
    return heights;
}
// The loaded block's type (0xe9a0); false where its zone is not in memory.
bool blockType(ExtractedWorld &terrain, Site p, int z, int &type) {
    bool loaded = false;
    const ExtractedWorld::GameBlock b = terrain.gameBlock(p.x, p.y, z, &loaded);
    type = b[4];
    return loaded;
}
// Standing ground with `clearance` blocks of air over it, near the column's top.
bool ground(ExtractedWorld &terrain, Site p, int &height, int clearance = 4) {
    const float top = predictedHeight(p);
    if (!std::isfinite(top) || std::fabs(top) > 100000)
        return false;
    const int start = (int)std::floor(top) + 8;
    for (int z = start; z >= start - 32; --z) {
        int type = 0;
        if (!blockType(terrain, p, z, type))
            return false;
        if (type != 1 && type != 4 && type != 5 && type != 10 && type != 13) // solid, grass, ground, snow, path
            continue;
        bool open = true;
        for (int above = 1; above <= clearance && open; ++above)
            open = blockType(terrain, p, z + above, type) && type == 0;
        if (open) {
            height = z + 1;
            return true;
        }
    }
    return false;
}
// Open overworld of the region: away from every native location's footprint.
bool overworld(Site p, Region owner) {
    const Region actual = regionOf(p);
    if (actual.x != owner.x || actual.y != owner.y)
        return false;
    const Location poi = locationAt(p);
    if (poi.type == 0)
        return true;
    if (!std::isfinite(poi.influence) || !std::isfinite(poi.distanceSquared) || poi.influence > 0)
        return false;
    const int safe = std::max(96, poi.radius + 48);
    return poi.distanceSquared > (float)safe * (float)safe;
}
// Whether the region's planner (0x115df0) put a utility item (its quests 6-13: the glider,
// boat, reins, boots and the other category 24 specials) at the location of `center`.
bool utilitySite(Site center, bool big) {
    int64_t cx = floorDivide(center.x, 1024), cy = floorDivide(center.y, 1024);
    if (big)
        cx = floorDivide(cx, 2) * 2, cy = floorDivide(cy, 2) * 2;
    const Region owner = regionOf(center);
    worldgen::OriginalGeneration original; // the planner's own roles, travel quests included
    for (const auto &role : worldgen::regionRoles({owner.x, owner.y}).roles)
        if (role[0] == cx && role[1] == cy)
            return role[2] >= 6 && role[2] <= 13;
    return false;
}
bool sameRegion(Region a, Region b) { return a.x == b.x && a.y == b.y; }
uint64_t utcNow() {
    return (uint64_t)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}
const Site Offsets[10] = {{-8, -7}, {8, -7}, {-10, 5}, {10, 5}, {-5, 12}, {5, 12}, {0, 0}, {-3, 2}, {0, 0}, {10, -3}};
} // namespace

// --- Setup ---------------------------------------------------------------------------------

void OverworldEvents::replaceTravelLocations(int32_t seed, const std::string &worldName) {
    // The kinds the place names' dispatch (0x16b390) calls Castle, AncientRuins, Crypt and
    // Tower: the mod's dungeonTypes, which it reads from that dispatch's jump table (0x16b7dc).
    static constexpr std::array<int, 4> Dungeons{8, 15, 4, 7};
    const uint64_t identity = travel::worldIdentity((uint32_t)seed, worldName);
    // The planner's travel quests by region (as the game alone plans them), kept for the
    // next lookups as the mod keeps them.
    struct Plans {
        std::mutex lock;
        std::map<std::pair<int32_t, int32_t>, std::set<std::pair<int32_t, int32_t>>> travel;
    };
    auto plans = std::make_shared<Plans>();
    auto travelQuest = [plans](worldgen::Int2 owner, int32_t x, int32_t y) {
        {
            std::lock_guard hold(plans->lock);
            if (auto found = plans->travel.find({owner.x, owner.y}); found != plans->travel.end())
                return found->second.count({x, y}) > 0;
        }
        std::set<std::pair<int32_t, int32_t>> sites;
        for (const auto &role : worldgen::regionRoles(owner).roles) // (in the game's own generation)
            if (travel::isTravelQuest(role[2]))
                sites.insert({role[0], role[1]});
        std::lock_guard hold(plans->lock);
        if (plans->travel.size() >= 128)
            plans->travel.clear();
        return plans->travel.emplace(std::make_pair(owner.x, owner.y), std::move(sites)).first->second.count({x, y}) > 0;
    };
    worldgen::ModHooks hooks;
    hooks.placeKind = [=](int32_t cx, int32_t cy, bool big, int kind) {
        if (!kind)
            return kind;
        const int32_t x = big ? (int32_t)floorDivide(cx, 2) * 2 : cx, y = big ? (int32_t)floorDivide(cy, 2) * 2 : cy;
        const worldgen::Int2 centre = worldgen::cellPoint(x, y, big);
        if (!travelQuest(worldgen::regionAt(centre.x, centre.y), x, y))
            return kind;
        return Dungeons[travel::dungeonIndex(identity, (uint32_t)seed, x, y)];
    };
    hooks.role = [](int32_t, int32_t, int role) { return travel::isTravelQuest(role) ? 0 : role; };
    hooks.generatedItem = [](int category, int id) { return !travel::isTravelItem((unsigned)category, id); };
    worldgen::setModHooks(hooks);
}

OverworldEvents::~OverworldEvents() {
    flush();
    if (callbacks)
        nativerules::removeCallbacks(callbacks);
}

void OverworldEvents::configure(const Settings &next, const std::filesystem::path &file, int32_t seed, const std::string &worldName) {
    settings = next;
    settings.slots = std::clamp(settings.slots, 1u, 8u);
    settings.cap = std::clamp(settings.cap, 1u, 3u);
    settings.cooldown = std::clamp(settings.cooldown, 30u, 1440u);
    settings.landmarkCap = std::min(settings.landmarkCap, 4u);
    stateFile = file;
    // The mod's world identity: its seed, then each character of its name.
    worldId = mix((uint32_t)seed);
    for (unsigned char c : worldName)
        worldId = mix(worldId ^ c);
    resolveAssets();
    usable = assets.cageRace >= 0 || !assets.npcRaces.empty();
    if (!settings.enabled)
        return;
    std::string error;
    if (std::filesystem::exists(stateFile) && !state.load(stateFile, &error)) {
        log("Events: " + error + "; stopped to keep the unreadable state file");
        usable = false;
        return;
    }
    loaded = true;
    // A new session rerolls unfinished encounters; accepted village quests and cooldowns stay.
    if (state.restartUnfinished(worldId)) {
        dirty = true;
        log("Events: unfinished encounters rerolled; completed cooldowns kept");
    }
    for (const auto &[key, saved] : state.entries())
        if (saved.world == worldId && saved.townQuest)
            referrals.push_back({saved.town, key, saved.cycle});
    // The mod loader's callbacks: a guardian's health by the mod's rule, and the encounters'
    // bounded difficulty premium.
    if (callbacks)
        nativerules::removeCallbacks(callbacks);
    callbacks = nativerules::addCallbacks({[this](const uint8_t *c, float &hp) {
                                   const int64_t id = get<int64_t>(c, 8);
                                   hp = health::normalizedMaximumHP(hp, get<uint32_t>(c, Flags), guardians.count(id) != 0);
                                   if (auto it = enemyDifficulty.find(id); it != enemyDifficulty.end())
                                       hp *= difficulty::health(it->second);
                               },
                               [this](const uint8_t *c, float &power) {
                                   if (auto it = enemyDifficulty.find(get<int64_t>(c, 8)); it != enemyDifficulty.end())
                                       power *= difficulty::damage(it->second);
                               }});
    log("Events: on (" + std::to_string(state.size()) + " records)");
}

// The assets by their exact dictionary names (RuntimeAssets.h), from the keys the game's
// dictionary gives races and items.
void OverworldEvents::resolveAssets() {
    auto race = [&](const char *name) {
        int found = -1;
        for (const auto &[id, key] : RACE_KEYS)
            if (std::strcmp(key, name) == 0) {
                if (found >= 0 && found != id)
                    return -1; // ambiguous: unavailable rather than a guess
                found = id;
            }
        return found;
    };
    assets = {};
    for (const char *name : {"Cage1", "Cage2", "Cage3", "Cage4"})
        if ((assets.cageRace = race(name)) >= 0)
            break;
    assets.sackRace = race("Sack");
    for (const char *name : {"SeaCrab", "SapphireFish", "LemonFish", "Seahorse", "Mermaid", "Merman", "Shark", "LanternFish",
                             "MawFish", "Piranha", "Blowfish"})
        if (const int r = race(name); r >= 0)
            assets.aquaticRaces.push_back(r);
    for (const char *name : {"WoodChest", "Chest"})
        if (const int r = race(name); r >= 181 && r <= 184) {
            assets.chestRace = r;
            break;
        }
    for (const char *name : {"HumanMale", "HumanFemale", "ElfMale", "ElfFemale", "DwarfMale", "DwarfFemale", "GoblinMale",
                             "GoblinFemale", "LizardmanMale", "LizardmanFemale", "FrogmanMale", "FrogmanFemale", "OrcMale",
                             "OrcFemale", "UndeadMale", "UndeadFemale"})
        if (const int r = race(name); r >= 0)
            assets.npcRaces.push_back(r);
    for (const char *name : {"Key", "RustyKey", "CopperKey", "BronzeKey", "IronKey", "SilverKey", "GoldKey", "BoneKey",
                             "SkullKey", "ScratchedKey", "AdornedKey", "ObsidianKey", "SteelKey", "TwistedKey", "SmallKey",
                             "HeavyKey", "MassiveKey", "WoodKey", "StoneKey", "TinyKey", "DelicateKey"}) {
        int category = -1, id = -1;
        bool ambiguous = false;
        for (const auto &k : ITEM_KEYS)
            if (std::strcmp(k.key, name) == 0 && k.category == 22 && k.id <= 19) {
                ambiguous = ambiguous || (id >= 0 && id != k.id);
                category = k.category, id = k.id;
            }
        if (id >= 0 && !ambiguous) {
            assets.keyCategory = category, assets.keyId = id;
            break;
        }
    }
}

void OverworldEvents::flush() {
    if (!dirty || !loaded)
        return;
    std::filesystem::create_directories(stateFile.parent_path());
    std::string error;
    if (state.save(stateFile, &error))
        dirty = false;
    else
        log("Events: " + error);
}

// --- Bookkeeping ---------------------------------------------------------------------------

// Only unfinished instances count toward the caps; random and landmark ones apart. An
// accepted village objective not yet loaded reserves its place too.
unsigned OverworldEvents::unfinished(bool landmarks) const {
    unsigned count = 0;
    for (const Active &a : active) {
        const EventRecord *saved = state.find(a.key.world, a.key.region, a.key.slot);
        count += saved && !saved->rewardClaimed && !saved->failed && landmarkEvent(saved->kind) == landmarks;
    }
    if (!landmarks)
        for (const auto &[key, saved] : state.entries()) {
            if (saved.world != worldId || !saved.townQuest || saved.rewardClaimed || saved.failed)
                continue;
            bool loadedNow = false;
            for (const Active &a : active)
                loadedNow = loadedNow || a.key == key;
            count += !loadedNow;
        }
    return count;
}

bool OverworldEvents::owned(int64_t id) const {
    if (!id)
        return false;
    for (const Active &a : active)
        for (int64_t actor : a.actors)
            if (actor == id)
                return true;
    return false;
}

// The region's own dry overworld natives, for an encounter's enemies (wraiths excluded:
// their teleport home is not set by the common factory).
std::vector<const uint8_t *> OverworldEvents::enemyPool(World &world, Region region) const {
    std::vector<const uint8_t *> pool;
    world.creatures.forEach([&](const uint8_t *r) {
        if (pool.size() >= 32 || owned(get<int64_t>(r, 8)))
            return;
        const uint32_t race = get<uint32_t>(r, Race);
        if (r[Hostility] != HostileBehaviour || !(hp(r) > 0) || race == 276 ||
            std::count(assets.aquaticRaces.begin(), assets.aquaticRaces.end(), (int)race) ||
            (get<uint32_t>(r, Flags) & (Named | Friendly | Mini | Boss | Fish)))
            return;
        const Site at = siteOf(r);
        if (!sameRegion(regionOf(at), region))
            return;
        int type = 0, height = 0;
        if (!blockType(world.terrain, at, (int)floorDivide(get<int64_t>(r, Position + 16), 65536), type) || type == 2 ||
            !ground(world.terrain, at, height) || !overworld(at, region))
            return;
        pool.push_back(r);
    });
    return pool;
}

bool OverworldEvents::chooseEnemies(World &world, Region region, uint64_t roll, EventRecord &saved, bool repairAquatic) {
    if (!saved.enemyCount)
        return true;
    const std::vector<const uint8_t *> pool = enemyPool(world, region);
    if (pool.empty())
        return false;
    for (unsigned index = 0; index < saved.enemyCount; ++index) {
        if (repairAquatic && ((saved.defeatedMask & (1u << index)) ||
                              !std::count(assets.aquaticRaces.begin(), assets.aquaticRaces.end(), (int)saved.enemyRaces[index])))
            continue;
        const uint8_t *pick = pool[mix(roll + index) % pool.size()];
        const int stars = (int)get<uint32_t>(pick, Level);
        const int regional = world.creatureLevel ? world.creatureLevel(pick) : 1 + 8 * stars;
        saved.difficultyLevel = std::max(saved.difficultyLevel, (unsigned)std::clamp(std::max(world.playerLevel, regional), 1, 10000000));
        saved.enemyRaces[index] = get<uint32_t>(pick, Race);
        saved.enemyStars[index] = (unsigned)encounters::stars(stars, leader(saved.kind, index));
    }
    return true;
}

EventRecord *OverworldEvents::unusedSlot(Region region) {
    for (unsigned slot = 0; slot < settings.slots; ++slot) {
        if (state.size() >= StateStore::MaxRecords && !state.find(worldId, region, slot))
            continue;
        EventRecord &saved = state.get(worldId, region, slot);
        if (!saved.planned && !saved.rewardClaimed && !saved.failed)
            return &saved;
    }
    return nullptr;
}

bool OverworldEvents::available(Kind kind) const {
    if (!chestEvent(kind) && kind != Kind::Ambush && assets.npcRaces.empty())
        return false;
    if (kind == Kind::Rescue && assets.cageRace < 0)
        return false;
    if (chestEvent(kind) && assets.keyCategory != 22)
        return false;
    return kind != Kind::ChestSack || assets.sackRace >= 0;
}

// --- Placing and making the actors ---------------------------------------------------------

bool OverworldEvents::selectTownSite(EventRecord &candidate, Site village, int radius) {
    const uint64_t key = (uint64_t)(uint32_t)village.x << 32 | (uint32_t)village.y;
    if (townSites.size() > 128)
        townSites.clear();
    TownSite &cached = townSites[key];
    auto free = [&](Site site) {
        return relocating(site, candidate.previousSite, candidate.cycle) &&
               siteAvailable(state, {candidate.world, candidate.region, candidate.slot}, site, utcNow());
    };
    if (now < cached.until) {
        if (!cached.found)
            return false;
        if (free(cached.site)) {
            candidate.site = cached.site;
            return true;
        }
    }
    cached = {{}, false, now + 5.0};
    for (unsigned attempt = 0; attempt < 64; ++attempt) {
        const Site site = town::candidate(village, radius, attempt);
        if (coordinate(site) && free(site) && overworld(site, candidate.region) && town::dryLand(predictedFootprint(site))) {
            candidate.site = cached.site = site;
            cached.found = true;
            return true;
        }
    }
    return false;
}

bool OverworldEvents::sackPoint(World &world, const EventRecord &saved, int center, Site &point, int &height) {
    const uint64_t roll = plan(saved.world, saved.region, saved.slot, saved.cycle).roll;
    for (unsigned attempt = 0; attempt < 8; ++attempt) {
        const double angle = double((roll >> 8) % 360) * 3.14159265358979323846 / 180 + attempt * 3.14159265358979323846 / 4;
        point = {saved.site.x + (int64_t)std::llround(56 * std::cos(angle)), saved.site.y + (int64_t)std::llround(56 * std::sin(angle))};
        bool flat = ground(world.terrain, point, height, 8);
        for (int x = -1; x <= 1 && flat; ++x)
            for (int y = -1; y <= 1 && flat; ++y) {
                int other = 0;
                flat = ground(world.terrain, {point.x + x, point.y + y}, other, 8) && other == height;
            }
        if (flat && std::abs(height - center) <= 24 && overworld(point, saved.region))
            return true;
    }
    return false;
}

bool OverworldEvents::place(World &world, EventRecord &saved, std::array<Dots, ActorCount> &points) {
    // Random encounters avoid native locations; a landmark's stands inside its location.
    const bool landmark = landmarkEvent(saved.kind);
    auto allowed = [&](Site p) { return landmark ? sameRegion(regionOf(p), saved.region) : overworld(p, saved.region); };
    auto footprint = [&](Site site, int &height, int radius, bool flat) {
        if (!ground(world.terrain, site, height, 8))
            return false;
        for (int x = -radius; x <= radius; ++x)
            for (int y = -radius; y <= radius; ++y) {
                int other = 0;
                if (!ground(world.terrain, {site.x + x, site.y + y}, other, 8) || std::abs(other - height) > (flat ? 0 : 1))
                    return false;
            }
        return true;
    };
    int center = 0;
    if (!allowed(saved.site) ||
        !footprint(saved.site, center, saved.kind == Kind::Rescue ? 3 : 1, saved.kind == Kind::Rescue || chestEvent(saved.kind)))
        return false;
    for (unsigned actor = 0; actor < ActorCount; ++actor) {
        bool needed;
        if (actor < Npc)
            needed = actor < saved.enemyCount && !(saved.defeatedMask & (1u << actor));
        else if (actor == Npc)
            needed = !chestEvent(saved.kind) && saved.kind != Kind::Ambush && !landmark;
        else if (actor == Companion)
            needed = saved.kind == Kind::Battle;
        else if (actor == Object) // the chest's point (chest and landmark events), or the cage
            needed = chestEvent(saved.kind) || landmark || (saved.kind == Kind::Rescue && !saved.rewardClaimed);
        else
            needed = saved.kind == Kind::ChestSack; // the sack's own point
        if (!needed)
            continue;
        Site offset = Offsets[actor];
        // Hostiles and townsfolk notice within 20 blocks: a guardian waits farther out.
        if (actor == 0 && guardianEvent(saved.kind))
            offset = {36, 0};
        if (actor == 0 && landmark)
            offset = {0, 0}; // the leader holds the landmark's centre
        Site p{saved.site.x + offset.x, saved.site.y + offset.y};
        int height = 0;
        if (actor == Container && saved.kind == Kind::ChestSack) {
            if (!sackPoint(world, saved, center, p, height))
                return false;
        } else if (!ground(world.terrain, p, height, 5) || std::abs(height - center) > (landmark ? 6 : 3) || !allowed(p))
            return false;
        points[actor] = {p.x * 65536 + 32768, p.y * 65536 + 32768, (int64_t)height * 65536};
    }
    return true;
}

// A creature made for an instance, as the bridge made one: the game's construction, a turn,
// and for an object no gear and no health of its own.
uint8_t *OverworldEvents::make(World &world, Active &instance, unsigned index, const Dots &point, int race, int behaviour, int stars) {
    const int64_t id = world.creatures.make(race, behaviour, stars, point.data(), {instance.key.region.x, instance.key.region.y});
    uint8_t *r = id ? world.creatures.record(id) : nullptr;
    if (!r)
        return nullptr;
    set<float>(r, Yaw, (float)(mix(lineCounter++ ^ (uint64_t)id) % 3600) / 10.0f);
    if (behaviour == HostileBehaviour)
        set<uint32_t>(r, Flags, get<uint32_t>(r, Flags) & ~Friendly);
    if (behaviour == objects::ExamineBehaviour) {
        std::memset(r + Equipment, 0, 11 * 0xa0);
        set<uint32_t>(r, Flags, get<uint32_t>(r, Flags) & ~((1u << 9) | (1u << 14) | (1u << 15) | (1u << 17) | (1u << 18) | (1u << 22)));
        set<float>(r, Health, 1.f);
    }
    setHome(r, point);
    instance.actors[index] = id;
    spawned.insert(id);
    return r;
}

void OverworldEvents::release(World &world, Active &instance, unsigned index) {
    const int64_t id = instance.actors[index];
    enemyDifficulty.erase(id);
    guardians.erase(id);
    instance.actors[index] = 0;
    world.creatures.remove(id);
}

void OverworldEvents::bindRescue(World &world, const Active &instance) {
    uint8_t *cage = world.creatures.record(instance.actors[Object]);
    uint8_t *npc = world.creatures.record(instance.actors[Npc]);
    if (!cage || !npc || !(hp(cage) > 0) || !(hp(npc) > 0))
        return;
    struct Point {
        int64_t x, y, z;
    } p{get<int64_t>(cage, Position), get<int64_t>(cage, Position + 8), get<int64_t>(cage, Position + 16)};
    p = objects::captivePosition(p, get<float>(cage, PhysicsScale), get<float>(npc, PhysicsScale));
    setStationary(npc, {p.x, p.y, p.z});
    set<uint8_t>(npc, Hostility, (uint8_t)objects::ExamineBehaviour);
    set<uint8_t>(npc, Ability, objects::CaptiveAbility);
    for (size_t at : {Yaw, Pitch, Roll})
        set<float>(npc, at, get<float>(cage, at));
    stopMoving(npc);
    set<int64_t>(cage, Prisoner, instance.actors[Npc]); // the cage binder's prisoner (+0x1a8)
}

void OverworldEvents::freeRescue(World &world, const Active &instance) {
    if (uint8_t *npc = world.creatures.record(instance.actors[Npc])) {
        set<uint8_t>(npc, Hostility, (uint8_t)VillagerBehaviour);
        set<uint8_t>(npc, Ability, 0);
        set<uint32_t>(npc, Flags, get<uint32_t>(npc, Flags) | Friendly);
        set<float>(npc, Health, nativerules::maxHealth(npc));
        allowRoaming(npc);
    }
    if (uint8_t *cage = world.creatures.record(instance.actors[Object])) // the opened cage goes
        world.creatures.remove(get<int64_t>(cage, 8));
}

bool OverworldEvents::spawn(World &world, EventRecord &saved, const std::array<Dots, ActorCount> &points, Active *existing) {
    Active instance{{saved.world, saved.region, saved.slot}};
    if (existing)
        instance = *existing;
    const auto before = instance.actors;
    auto missing = [&](unsigned index) { return !world.creatures.record(instance.actors[index]); };
    const uint64_t roll = plan(saved.world, saved.region, saved.slot, saved.cycle).roll;
    const int friendlyRace = assets.npcRaces.empty() ? -1 : assets.npcRaces[roll % assets.npcRaces.size()];
    if (captiveNeedsCage(saved.kind, saved.rewardClaimed) && missing(Object))
        if (uint8_t *cage = make(world, instance, Object, points[Object], assets.cageRace, objects::ExamineBehaviour, 0)) {
            noncombatObject(cage);
            set<float>(cage, Yaw, 0.f), set<float>(cage, Pitch, 0.f), set<float>(cage, Roll, 0.f);
            struct Point {
                int64_t x, y, z;
            } p{points[Object][0], points[Object][1], points[Object][2]};
            p = objects::cagePosition(p, (int)get<int32_t>(cage, Race), get<float>(cage, GraphicsScale), get<float>(cage, HitboxScale),
                                      get<float>(cage, ChestScale));
            setStationary(cage, {p.x, p.y, p.z});
        }
    // A captive is only made inside its cage.
    const bool captiveReady = !captiveNeedsCage(saved.kind, saved.rewardClaimed) || !missing(Object);
    if (!chestEvent(saved.kind) && saved.kind != Kind::Ambush && !landmarkEvent(saved.kind) && missing(Npc) && captiveReady)
        if (uint8_t *npc = make(world, instance, Npc, points[Npc], friendlyRace,
                                saved.kind == Kind::Battle ? VillagerBehaviour : objects::ExamineBehaviour, 2)) {
            friendlyActor(npc);
            if (saved.kind == Kind::Battle) {
                set<float>(npc, Health, nativerules::maxHealth(npc) * std::max(.05f, saved.npcHPFraction));
                allowRoaming(npc);
            } else
                noncombatObject(npc);
            char name[16] = "Adventurer";
            std::memcpy(npc + Name, name, sizeof name);
        }
    if (saved.kind == Kind::Battle && missing(Companion))
        if (uint8_t *ally = make(world, instance, Companion, points[Companion], friendlyRace, VillagerBehaviour, 1)) {
            friendlyActor(ally);
            allowRoaming(ally);
        }
    for (unsigned enemy = 0; enemy < saved.enemyCount; ++enemy) {
        if ((saved.defeatedMask & (1u << enemy)) || !missing(enemy))
            continue;
        guardians.erase(instance.actors[enemy]);
        uint8_t *creature = make(world, instance, enemy, points[enemy], (int)saved.enemyRaces[enemy], HostileBehaviour,
                                 (int)saved.enemyStars[enemy]);
        if (!creature)
            continue;
        const int64_t id = instance.actors[enemy];
        set<uint32_t>(creature, Flags, get<uint32_t>(creature, Flags) & ~(Friendly | Named | Mini | Boss));
        allowRoaming(creature);
        enemyDifficulty[id] = saved.difficultyLevel;
        if (leader(saved.kind, enemy)) {
            guardians.insert(id);
            nameBoss(creature, roll);
        }
        set<float>(creature, Health, nativerules::maxHealth(creature) * std::clamp(saved.enemyHPFraction[enemy], .01f, 1.f));
    }
    if (saved.kind == Kind::ChestSack && missing(Object))
        if (uint8_t *sack = make(world, instance, Object, points[Container], assets.sackRace, objects::ExamineBehaviour, 0)) {
            noncombatObject(sack);
            placeContainer(sack, points[Container]);
        }
    const bool rewardChest = chestEvent(saved.kind) || (landmarkEvent(saved.kind) && saved.objectiveDone);
    if (rewardChest && missing(Container))
        if (uint8_t *chest = make(world, instance, Container, points[Object], assets.chestRace, objects::ExamineBehaviour, 0)) {
            noncombatObject(chest);
            placeContainer(chest, points[Object]);
            set<uint8_t>(chest, InteractionState, saved.rewardClaimed ? 2 : 1);
        }
    bool complete = landmarkEvent(saved.kind)    ? !rewardChest || instance.actors[Container] != 0
                    : saved.kind == Kind::Ambush ? instance.actors[0] != 0
                    : chestEvent(saved.kind)     ? instance.actors[Container] != 0
                                                 : instance.actors[Npc] != 0;
    complete &= saved.kind != Kind::Battle || instance.actors[Companion] != 0;
    complete &= saved.kind != Kind::ChestSack || instance.actors[Object] != 0;
    complete &= saved.kind != Kind::Rescue || saved.rewardClaimed || instance.actors[Object] != 0;
    for (unsigned enemy = 0; enemy < saved.enemyCount; ++enemy)
        complete &= (saved.defeatedMask & (1u << enemy)) || instance.actors[enemy] != 0;
    // Objects made now become anchors.
    for (unsigned index : {Object, Container})
        if (instance.actors[index] && instance.actors[index] != before[index])
            if (const uint8_t *object = world.creatures.record(instance.actors[index])) {
                instance.anchors[index] = positionOf(object);
                instance.anchoredMask |= 1u << index;
            }
    if (captiveNeedsCage(saved.kind, saved.rewardClaimed))
        bindRescue(world, instance);
    else if (saved.kind == Kind::Rescue)
        freeRescue(world, instance); // an adventurer freed before, reloaded
    objectiveMarker(world, saved, instance);
    if (!complete)
        log("Events: an encounter is not complete yet; only its missing actors will be retried");
    if (existing)
        *existing = instance;
    else
        active.push_back(instance);
    return complete;
}

// The objective's place, for the map: the chest, the leader of a group (its first living
// member), or the event's NPC.
void OverworldEvents::objectiveMarker(World &world, const EventRecord &saved, Active &instance) {
    const bool landmark = landmarkEvent(saved.kind);
    unsigned index = chestEvent(saved.kind) || (landmark && saved.objectiveDone) ? Container : Npc;
    if (saved.kind == Kind::Ambush || (landmark && !saved.objectiveDone)) {
        index = 0;
        for (unsigned enemy = 0; enemy < saved.enemyCount; ++enemy)
            if (!(saved.defeatedMask & (1u << enemy)) && world.creatures.record(instance.actors[enemy])) {
                index = enemy;
                break;
            }
    }
    for (int64_t id : instance.actors)
        if (uint8_t *r = world.creatures.record(id))
            set<uint16_t>(r, Toggles, get<uint16_t>(r, Toggles) & ~MapMarker);
    if (const uint8_t *r = world.creatures.record(instance.actors[index])) {
        const Dots p = positionOf(r);
        instance.marker = {floorDivide(p[0], 65536), floorDivide(p[1], 65536), floorDivide(p[2], 65536), false};
        instance.markerKnown = true;
    }
}

// The objectives for the map, including accepted village quests not loaded yet; a finished
// village objective moves its grey pin back to the town.
void OverworldEvents::publishPins() {
    mapPins.clear();
    auto siteMarker = [](Site site) {
        const float h = predictedHeight(site);
        return Pin{site.x, site.y, std::isfinite(h) && std::fabs(h) < 100000 ? (int64_t)std::floor(h) : 0, true};
    };
    for (const Active &a : active) {
        const EventRecord *saved = state.find(a.key.world, a.key.region, a.key.slot);
        if (!a.markerKnown || !saved || saved->rewardClaimed || saved->failed)
            continue;
        Pin pin = a.marker;
        pin.destination = saved->townQuest;
        if (saved->townQuest && saved->objectiveDone)
            pin = siteMarker(saved->town);
        mapPins.push_back(pin);
    }
    for (const auto &[key, saved] : state.entries()) {
        if (saved.world != worldId || !saved.townQuest || saved.rewardClaimed || saved.failed)
            continue;
        bool loadedNow = false;
        for (const Active &a : active)
            loadedNow = loadedNow || a.key == key;
        if (!loadedNow)
            mapPins.push_back(siteMarker(saved.objectiveDone ? saved.town : saved.site));
    }
}

gameshop::Item OverworldEvents::keyItem(const EventRecord &saved) const {
    gameshop::Item key{};
    key[0] = (uint8_t)assets.keyCategory;
    set<int32_t>(key.data(), 4, assets.keyId);
    set<uint32_t>(key.data(), 8, (uint32_t)plan(saved.world, saved.region, saved.slot, saved.cycle).roll | 1u);
    set<int32_t>(key.data(), 0xc, saved.region.x);
    set<int32_t>(key.data(), 0x10, saved.region.y);
    return key;
}

bool OverworldEvents::grantKey(World &world, EventRecord &saved) {
    const gameshop::Item key = keyItem(saved);
    if (world.holds(key) <= 0 && !world.give(key)) // one key at most
        return false;
    saved.keySearchFound = true;
    dirty = true;
    return true;
}

// Exactly one reward per instance: equipment for the player's class from the game's own
// chest generators (0x2d15d0 weapons, 0x2d0680 armour and jewellery), claimed and saved
// before it is given, the claim undone if it cannot be.
bool OverworldEvents::deliver(World &world, EventRecord &saved) {
    if (saved.rewardClaimed)
        return false;
    const uint64_t roll = plan(saved.world, saved.region, saved.slot, saved.cycle).roll;
    const int cls = world.playerClass;
    gameshop::Item reward{};
    bool made = false;
    nativedrop::World generator;
    uint32_t seed = (uint32_t)(roll >> 7);
    generator.rand = [&] {
        seed = seed * 214013u + 2531011u;
        return (int)((seed >> 16) & 0x7fff);
    };
    const uint8_t rarity = (uint8_t)((roll >> 32) % 5);
    for (unsigned attempt = 0; attempt < 8 && !made; ++attempt) {
        gameshop::Item candidate{};
        const bool weapon = attempt != 7 && ((roll >> (attempt % 8)) & 1);
        if (weapon)
            nativedrop::weapon(generator, candidate.data(), rarity, cls);
        else
            nativedrop::armour(generator, candidate.data(), rarity, cls, false);
        if (!candidate[0] || !nativerules::classMayUse(candidate.data(), cls))
            continue;
        set<uint32_t>(candidate.data(), 8, (uint32_t)mix(roll + 0x9e3779b97f4a7c15ull) | 1u);
        set<int32_t>(candidate.data(), 0xc, saved.region.x);
        set<int32_t>(candidate.data(), 0x10, saved.region.y);
        reward = candidate;
        made = true;
    }
    if (!made)
        return false;
    saved.rewardClaimed = true;
    saved.deadlineUTCsecs = cooldownDeadline(utcNow(), settings.cooldown);
    saved.emptySince = 0;
    dirty = true;
    flush();
    if (!world.give(reward)) {
        saved.rewardClaimed = false;
        saved.deadlineUTCsecs = 0;
        dirty = true;
        flush();
        log("Events: the reward could not be given; it can be claimed again");
        return false;
    }
    return true;
}

// --- Progress --------------------------------------------------------------------------------

void OverworldEvents::death(int64_t id) {
    if (!enabled())
        return;
    enemyDifficulty.erase(id);
    for (Active &a : active) {
        EventRecord &saved = record(a.key);
        if (saved.failed || saved.rewardClaimed)
            continue;
        if (a.actors[Npc] && a.actors[Npc] == id) { // the adventurer fell: the event failed
            saved.failed = true;
            saved.deadlineUTCsecs = cooldownDeadline(utcNow(), settings.cooldown);
            saved.emptySince = 0;
            dirty = true;
            continue;
        }
        for (unsigned enemy = 0; enemy < saved.enemyCount; ++enemy) {
            if (a.actors[enemy] != id || (saved.defeatedMask & (1u << enemy)))
                continue;
            saved.defeatedMask |= 1u << enemy;
            if (leader(saved.kind, enemy))
                guardians.erase(id);
            if (objectiveComplete(saved.enemyCount, saved.defeatedMask) && saved.kind != Kind::ChestBoss)
                saved.objectiveDone = true; // (a chest guardian's key is given by progress())
            dirty = true;
        }
    }
}

// Actors no instance owns any more are let go; health is noted; a site the player comes back
// to gets its missing roles made again.
void OverworldEvents::progress(World &world) {
    for (auto it = spawned.begin(); it != spawned.end();) {
        const uint8_t *r = world.creatures.record(*it);
        if (r && hp(r) > 0 && !owned(*it)) {
            world.creatures.remove(*it);
            log("Events: removed an actor no encounter owns");
            it = spawned.erase(it);
        } else
            ++it;
    }
    for (Active &a : active) {
        EventRecord &saved = record(a.key);
        if (!saved.rewardClaimed && !saved.failed)
            for (unsigned index = 0; index <= Npc; ++index)
                if (const uint8_t *r = world.creatures.record(a.actors[index])) {
                    if (!(hp(r) > 0))
                        death(a.actors[index]);
                    else if (index < saved.enemyCount)
                        saved.enemyHPFraction[index] = fraction(r);
                    else if (index == Npc && saved.kind == Kind::Battle)
                        saved.npcHPFraction = fraction(r);
                }
        // The fallen chest guardian's key goes to the player.
        if (saved.kind == Kind::ChestBoss && !saved.rewardClaimed && !saved.keySearchFound &&
            objectiveComplete(saved.enemyCount, saved.defeatedMask))
            grantKey(world, saved);
        if (captiveNeedsCage(saved.kind, saved.rewardClaimed) && !saved.failed) {
            // Without its cage the captive goes too; both are made again together.
            if (!world.creatures.record(a.actors[Object]) && a.actors[Npc])
                release(world, a, Npc);
            bindRescue(world, a);
        }
        if (!saved.failed && within(world.player, saved.site, 192)) {
            bool missing = false;
            for (unsigned index = 0; index < ActorCount; ++index) {
                bool expected;
                if (index < Npc)
                    expected = index < saved.enemyCount && !(saved.defeatedMask & (1u << index));
                else if (index == Npc)
                    expected = !chestEvent(saved.kind) && saved.kind != Kind::Ambush && !landmarkEvent(saved.kind);
                else if (index == Companion)
                    expected = saved.kind == Kind::Battle;
                else if (index == Object)
                    expected = saved.kind == Kind::ChestSack || captiveNeedsCage(saved.kind, saved.rewardClaimed);
                else
                    expected = chestEvent(saved.kind) || (landmarkEvent(saved.kind) && saved.objectiveDone);
                missing = missing || (expected && !world.creatures.record(a.actors[index]));
            }
            std::array<Dots, ActorCount> points{};
            if (missing && place(world, saved, points))
                spawn(world, saved, points, &a);
        }
        objectiveMarker(world, saved, a);
    }
}

void OverworldEvents::testEncounter(World &world, Region region) {
    if (requestedTest == Kind::Count)
        return;
    auto finish = [&](const std::string &reply) {
        said.push_back(reply);
        requestedTest = Kind::Count;
    };
    if (unfinished(false) >= settings.cap)
        return finish("Event limit reached. Finish an active encounter before testing another.");
    if (!available(requestedTest))
        return finish("The native assets needed for that event are unavailable.");
    EventRecord *selected = unusedSlot(region);
    if (!selected)
        return finish("This region has no unused event slot. Try testing in another region.");
    for (unsigned search = 0; search < 2 && testAttempt < 32; ++search) {
        EventRecord candidate = *selected;
        candidate.kind = requestedTest;
        candidate.site = testSite(world.player, testAttempt++);
        const uint64_t roll = plan(worldId, region, candidate.slot, candidate.cycle).roll;
        candidate.enemyCount = enemyCount(candidate.kind, roll);
        if (!relocating(candidate.site, candidate.previousSite, candidate.cycle) ||
            !siteAvailable(state, {candidate.world, candidate.region, candidate.slot}, candidate.site, utcNow()))
            continue;
        std::array<Dots, ActorCount> points{};
        if (!place(world, candidate, points) || !chooseEnemies(world, region, roll, candidate))
            continue;
        candidate.planned = true;
        *selected = candidate;
        dirty = true;
        const bool started = spawn(world, *selected, points);
        char reply[180];
        std::snprintf(reply, sizeof reply, "%s %.0f blocks away. Look for the chest or NPC marker on your map.",
                      started ? "Test encounter started" : "Native spawn needs a retry",
                      std::hypot(double(candidate.site.x - world.player.x), double(candidate.site.y - world.player.y)));
        log("Events: test kind " + std::to_string((unsigned)candidate.kind) + " at " + std::to_string(candidate.site.x) + "," +
            std::to_string(candidate.site.y));
        return finish(reply);
    }
    if (testAttempt >= 32)
        finish("No eligible dry overworld site found nearby. Move away from POIs or find open terrain.");
}

// Event objects stay where they were placed, as village props do; a captive stays bound.
void OverworldEvents::hold(World &world) {
    for (Active &a : active) {
        for (unsigned index : {Object, Container}) {
            if (!(a.anchoredMask & (1u << index)))
                continue;
            if (uint8_t *object = world.creatures.record(a.actors[index]); object && hp(object) > 0) {
                setStationary(object, a.anchors[index]);
                stopMoving(object);
            }
        }
        const EventRecord *saved = state.find(a.key.world, a.key.region, a.key.slot);
        if (saved && !saved->failed && captiveNeedsCage(saved->kind, saved->rewardClaimed))
            bindRescue(world, a);
    }
}

// The landmark record of a native location: the slot already bound to it, or a free one of
// its region's reserved slots (then bound to it through previousSite).
EventRecord *OverworldEvents::landmarkRecord(Region area, Site center) {
    const unsigned start = (unsigned)(mix((uint64_t)center.x * 0x9e3779b1ull ^ (uint64_t)center.y) % LandmarkSlots);
    int freeSlot = -1;
    for (unsigned i = 0; i < LandmarkSlots; ++i) {
        const unsigned slot = FirstLandmarkSlot + (start + i) % LandmarkSlots;
        const EventRecord *existing = state.find(worldId, area, slot);
        if (existing && (within(existing->previousSite, center, 16) || (existing->planned && within(existing->site, center, 16))))
            return &state.get(worldId, area, slot);
        const bool unused = !existing || (!existing->planned && !existing->rewardClaimed && !existing->failed &&
                                          !existing->previousSite.x && !existing->previousSite.y);
        if (unused && freeSlot < 0 && (existing || state.size() < StateStore::MaxRecords))
            freeSlot = (int)slot;
    }
    if (freeSlot < 0)
        return nullptr;
    EventRecord &saved = state.get(worldId, area, (unsigned)freeSlot);
    saved.previousSite = center;
    dirty = true;
    return &saved;
}

// A natural location within 160 blocks that holds no utility item gets an encounter of its
// own, at the location itself. The locations are found on a 128-block lattice around the
// player (each query answers for its own 1024-block cell); the plan generates nothing.
void OverworldEvents::planLandmarks(World &world, uint64_t utc, std::vector<Request> &pending) {
    unsigned running = unfinished(true);
    for (const Request &r : pending)
        running += landmarkEvent(record(r.key).kind);
    std::vector<std::pair<Kind, Site>> found;
    std::vector<std::pair<int, Site>> utility;
    auto seen = [](const auto &list, Site center) {
        for (const auto &entry : list)
            if (entry.second.x == center.x && entry.second.y == center.y)
                return true;
        return false;
    };
    for (int dx = -3; dx <= 3; ++dx)
        for (int dy = -3; dy <= 3; ++dy) {
            const Location location = locationAt({world.player.x + dx * 128, world.player.y + dy * 128});
            Kind kind;
            if (!landmarkKind(location.type, kind) || seen(found, location.centre) || seen(utility, location.centre))
                continue;
            if (utilitySite(location.centre, location.big))
                utility.push_back({location.type, location.centre});
            else
                found.push_back({kind, location.centre});
        }
    if (reportLandmarks) {
        reportLandmarks = false;
        log("Events: landmarks near you: " + std::to_string(found.size()));
        for (const auto &[kind, center] : found)
            log("  kind " + std::to_string((unsigned)kind) + " at " + std::to_string(center.x) + "," + std::to_string(center.y) + " (" +
                std::to_string((int)std::hypot(double(center.x - world.player.x), double(center.y - world.player.y))) + " away)");
        for (const auto &[type, center] : utility)
            log("  skipped: location type " + std::to_string(type) + " holds a utility item, at " + std::to_string(center.x) + "," +
                std::to_string(center.y));
    }
    unsigned searches = 0;
    for (const auto &[kind, center] : found) {
        if (running >= settings.landmarkCap || searches >= 2)
            return;
        if (!within(world.player, center, 160))
            continue;
        const Region area = regionOf(center);
        EventRecord *saved = landmarkRecord(area, center);
        if (!saved)
            continue;
        const EventKey key{worldId, area, saved->slot};
        bool busy = false;
        for (const Active &a : active)
            busy = busy || a.key == key;
        for (const Request &r : pending)
            busy = busy || r.key == key;
        if (busy || (saved->planned && !saved->rewardClaimed && !saved->failed))
            continue;
        if (saved->rewardClaimed || saved->failed) { // back after its cooldown, as a new cycle
            if (utc < saved->deadlineUTCsecs)
                continue;
            const unsigned cycle = saved->cycle + 1;
            *saved = EventRecord{};
            saved->world = key.world, saved->region = area, saved->slot = key.slot;
            saved->cycle = cycle;
            saved->previousSite = center;
            dirty = true;
        }
        ++searches;
        const uint64_t roll = plan(worldId, area, saved->slot, saved->cycle).roll;
        EventRecord candidate = *saved;
        candidate.kind = kind;
        candidate.enemyCount = landmarkEnemies(kind, roll);
        std::array<Dots, ActorCount> points{};
        // A large centrepiece (a mana tree's trunk) holds the exact centre: the outer rings
        // stand the group beside it.
        static const Site nudges[] = {{0, 0},  {6, 0},   {-6, 0},  {0, 6},   {0, -6},   {8, 8},    {-8, 8},   {8, -8},
                                      {16, 0}, {-16, 0}, {0, 16},  {0, -16}, {24, 24}, {-24, 24}, {24, -24}, {-24, -24}};
        bool placed = false;
        for (const Site &nudge : nudges) {
            candidate.site = {center.x + nudge.x, center.y + nudge.y};
            if (siteAvailable(state, key, candidate.site, utc) && place(world, candidate, points) &&
                chooseEnemies(world, area, roll, candidate)) {
                placed = true;
                break;
            }
        }
        if (!placed)
            continue;
        candidate.planned = true;
        *saved = candidate;
        dirty = true;
        pending.push_back({key, points});
        ++running;
    }
}

void OverworldEvents::report(World &world) {
    for (const Active &a : active) {
        const EventRecord *saved = state.find(a.key.world, a.key.region, a.key.slot);
        if (!saved)
            continue;
        log("Events: kind " + std::to_string((unsigned)saved->kind) + " at " + std::to_string(saved->site.x - world.player.x) + "," +
            std::to_string(saved->site.y - world.player.y) + " from the player, defeated mask " + std::to_string(saved->defeatedMask) +
            (saved->objectiveDone ? ", done" : "") + (saved->rewardClaimed ? ", rewarded" : ""));
        for (unsigned index = 0; index < ActorCount; ++index)
            if (const uint8_t *r = world.creatures.record(a.actors[index])) {
                const Site at = siteOf(r);
                log("  actor " + std::to_string(index) + ": race " + std::to_string(get<uint32_t>(r, Race)) + " hostility " +
                    std::to_string(r[Hostility]) + " level " + std::to_string(get<uint32_t>(r, Level)) + " hp " +
                    std::to_string((int)hp(r)) + "/" + std::to_string((int)nativerules::maxHealth(r)) + " at " +
                    std::to_string(at.x - world.player.x) + "," + std::to_string(at.y - world.player.y) + " z " + std::to_string(get<int64_t>(r, Position + 16) / 65536.0 - predictedHeight(at)).substr(0, 5) + " '" +
                    std::string((const char *)r + Name, strnlen((const char *)r + Name, 16)) + "'");
            }
    }
}

void OverworldEvents::tick(World &world) {
    if (reportStatus) {
        reportStatus = false;
        report(world);
    }
    const Region region = regionOf(world.player);
    const uint64_t utc = utcNow();
    progress(world);
    const bool testing = requestedTest != Kind::Count;
    testEncounter(world, region);
    std::vector<Request> pending;
    // Accepted village objectives: their site is checked as the player nears it.
    for (const Referral &referral : referrals) {
        EventRecord &saved = record(referral.objective);
        bool exists = false;
        for (const Active &a : active)
            exists = exists || a.key == referral.objective;
        if (exists || saved.cycle != referral.cycle || saved.failed || saved.rewardClaimed)
            continue;
        if (saved.awaitingSite && !town::dryLand(predictedFootprint(saved.site))) {
            const Location village = locationAt(saved.town);
            if (!selectTownSite(saved, village.centre, village.radius))
                continue;
            saved.attempt = 0;
            dirty = true;
        }
        if (!within(world.player, saved.site, 192))
            continue;
        EventRecord candidate = saved;
        const Site target = saved.site;
        std::array<Dots, ActorCount> points{};
        for (unsigned search = 0; search < (saved.awaitingSite ? 4u : 1u); ++search) {
            const unsigned attempt = saved.awaitingSite ? saved.attempt % 33 : 0;
            saved.attempt = (saved.attempt + 1) % 33;
            candidate.site = town::nearby(target, attempt);
            if (!siteAvailable(state, referral.objective, candidate.site, utc) || !place(world, candidate, points))
                continue;
            bool aquatic = false;
            for (unsigned index = 0; index < saved.enemyCount; ++index)
                aquatic = aquatic || (!(saved.defeatedMask & (1u << index)) &&
                                      std::count(assets.aquaticRaces.begin(), assets.aquaticRaces.end(), (int)saved.enemyRaces[index]));
            if ((saved.awaitingSite || aquatic) &&
                !chooseEnemies(world, saved.region, plan(saved.world, saved.region, saved.slot, saved.cycle).roll, candidate,
                               aquatic && !saved.awaitingSite))
                continue;
            candidate.awaitingSite = false;
            saved = candidate;
            dirty = true;
            pending.push_back({referral.objective, points});
            break;
        }
    }
    // Random encounters in the regions around: at most two placement searches a second.
    unsigned searches = 0;
    for (int rx = -1; rx <= 1; ++rx)
        for (int ry = -1; ry <= 1; ++ry)
            for (unsigned slot = 0; slot < settings.slots; ++slot) {
                const Region area{region.x + rx, region.y + ry};
                if (state.size() >= StateStore::MaxRecords && !state.find(worldId, area, slot))
                    continue;
                EventRecord &saved = state.get(worldId, area, slot);
                bool exists = false;
                for (const Active &a : active)
                    exists = exists || a.key == EventKey{worldId, area, slot};
                if (saved.rewardClaimed || saved.failed) {
                    if (within(world.player, saved.site, 256)) {
                        if (saved.emptySince)
                            saved.emptySince = 0, dirty = true;
                    } else if (!saved.emptySince)
                        saved.emptySince = utc, dirty = true;
                    if (exists || !canRestart(utc, saved.deadlineUTCsecs, saved.emptySince))
                        continue;
                    const Site previous = saved.site;
                    const unsigned cycle = saved.cycle + 1;
                    saved = EventRecord{};
                    saved.world = worldId, saved.region = area, saved.slot = slot;
                    saved.cycle = cycle;
                    saved.previousSite = previous;
                    dirty = true;
                }
                if (testing || unfinished(false) + pending.size() >= settings.cap || exists || saved.townQuest)
                    continue;
                const Plan planned = plan(worldId, area, slot, saved.cycle, saved.attempt);
                const Site site = saved.planned ? saved.site : planned.site;
                if (!coordinate(site) || !within(world.player, site, 192) || (!saved.planned && within(world.player, site, 48))) {
                    if (!saved.planned)
                        saved.attempt = (saved.attempt + 1) & 0xffffu;
                    continue;
                }
                if (searches >= 2)
                    continue;
                if (!siteAvailable(state, {saved.world, saved.region, saved.slot}, site, utc)) {
                    if (!saved.planned)
                        saved.attempt = (saved.attempt + 1) & 0xffffu;
                    continue;
                }
                if (!available(saved.planned ? saved.kind : planned.kind))
                    continue;
                ++searches;
                if (!saved.planned) {
                    if (!relocating(site, saved.previousSite, saved.cycle)) {
                        saved.attempt = (saved.attempt + 1) & 0xffffu;
                        continue;
                    }
                    saved.site = site;
                    saved.kind = planned.kind;
                    saved.enemyCount = planned.enemies;
                }
                std::array<Dots, ActorCount> points{};
                if (!place(world, saved, points)) {
                    if (!saved.planned)
                        saved.attempt = (saved.attempt + 1) & 0xffffu;
                    continue;
                }
                if (!saved.planned && !chooseEnemies(world, area, planned.roll, saved))
                    continue;
                saved.planned = true;
                dirty = true;
                pending.push_back({{saved.world, saved.region, saved.slot}, points});
            }
    if (!testing && settings.landmarkCap)
        planLandmarks(world, utc, pending);
    for (const Request &r : pending)
        spawn(world, record(r.key), r.points);
    publishPins();
    flush();
}

void OverworldEvents::update(World &world, double nowSeconds) {
    if (!enabled())
        return;
    now = nowSeconds;
    hold(world);
    if (now < nextTick)
        return;
    nextTick = now + 1.0;
    tick(world);
}

// --- Talking ---------------------------------------------------------------------------------

// One of five lines for the situation, never the same twice running from one villager.
std::string OverworldEvents::line(int64_t speaker, dialogue::Topic topic, Kind kind) {
    const uint64_t random = mix((uint64_t)(now * 1000.0) ^ (++lineCounter << 32) ^ (uint64_t)speaker);
    const wchar_t *text = dialogue::pool(topic, kind)[picker.pick((uint64_t)speaker, topic, kind, random)];
    std::string narrow; // (the lines are plain ASCII)
    for (; *text; ++text)
        narrow.push_back((char)*text);
    return narrow;
}

std::string OverworldEvents::referralLine(World &world, int64_t speaker, const EventRecord &saved, bool offer) {
    const uint8_t *r = world.creatures.record(speaker);
    const Site origin = r ? siteOf(r) : world.player;
    const double distance = std::hypot(double(saved.site.x - origin.x), double(saved.site.y - origin.y));
    char text[320];
    std::snprintf(text, sizeof text, line(speaker, offer ? dialogue::Topic::Offer : dialogue::Topic::Reminder, saved.kind).c_str(), distance);
    return text;
}

bool OverworldEvents::townOffer(World &world, Site village, int radius, EventRecord &candidate) {
    if (unfinished(false) >= settings.cap)
        return false;
    const Region region = regionOf(village);
    EventRecord *selected = unusedSlot(region);
    if (!selected)
        return false;
    candidate = *selected;
    const uint64_t roll = plan(worldId, region, candidate.slot, candidate.cycle).roll;
    candidate.kind = town::kind(mix(roll ^ mix((uint64_t)village.x) ^ (uint64_t)village.y));
    if (!available(candidate.kind)) // a missing cage must not stop a town offering work
        candidate.kind = Kind::Ambush;
    candidate.enemyCount = enemyCount(candidate.kind, roll);
    if (!selectTownSite(candidate, village, radius))
        return false;
    candidate.attempt = 0;
    candidate.planned = candidate.townQuest = candidate.awaitingSite = true;
    candidate.town = village;
    candidate.difficultyLevel = (unsigned)std::clamp(world.playerLevel, 1, 10000000);
    return true;
}

// Ordinary villagers of a village offer, remind of and reward the town's quests.
bool OverworldEvents::townInteraction(World &world, int64_t id, Said &out) {
    const uint8_t *r = world.creatures.record(id);
    if (!r || !town::eligible(r[Hostility], hp(r), get<uint32_t>(r, Flags)) ||
        !std::count(assets.npcRaces.begin(), assets.npcRaces.end(), (int)get<uint32_t>(r, Race)))
        return false;
    const Site origin = siteOf(r);
    const Location village = locationAt(origin);
    if (village.type != 5 || !std::isfinite(village.influence) || village.influence <= 0)
        return false;
    const Site center = village.centre;
    const Region region = regionOf(origin);
    out.speaker = id;
    for (auto it = referrals.begin(); it != referrals.end(); ++it) {
        if (it->town.x != center.x || it->town.y != center.y)
            continue;
        EventRecord &saved = record(it->objective);
        if (saved.cycle != it->cycle) {
            referrals.erase(it);
            break;
        }
        if (saved.rewardClaimed)
            out.line = line(id, dialogue::Topic::Thanks, saved.kind);
        else if (saved.failed)
            out.line = line(id, dialogue::Topic::Over, saved.kind);
        else if (!saved.objectiveDone)
            out.line = referralLine(world, id, saved, false);
        else {
            auto instance = std::find_if(active.begin(), active.end(), [&](const Active &a) { return a.key == it->objective; });
            if (deliver(world, saved)) {
                if (saved.kind == Kind::Rescue && instance != active.end())
                    freeRescue(world, *instance);
                if (instance != active.end())
                    objectiveMarker(world, saved, *instance);
                out.line = line(id, dialogue::Topic::Reward, saved.kind);
            } else
                out.line = "Your reward is not ready. Please speak to me again in a moment.";
        }
        publishPins();
        return true;
    }
    // A town may send the player to an encounter already in its region.
    for (const Active &a : active) {
        EventRecord &saved = record(a.key);
        if (saved.failed || saved.rewardClaimed || saved.townQuest || !sameRegion(saved.region, region) || !town::referralKind(saved.kind))
            continue;
        saved.townQuest = true;
        saved.town = center;
        dirty = true;
        referrals.push_back({center, a.key, saved.cycle});
        hints.clear();
        publishPins();
        flush();
        out.line = referralLine(world, id, saved, true);
        return true;
    }
    EventRecord candidate;
    if (!townOffer(world, center, village.radius, candidate))
        return false;
    EventRecord &selected = state.get(candidate.world, candidate.region, candidate.slot);
    selected = candidate;
    dirty = true;
    referrals.push_back({center, {candidate.world, candidate.region, candidate.slot}, candidate.cycle});
    hints.clear();
    publishPins();
    out.line = referralLine(world, id, selected, true);
    flush();
    return true;
}

bool OverworldEvents::interact(World &world, int64_t id, Said &out) {
    if (!enabled() || !id)
        return false;
    for (Active &a : active) {
        unsigned actor = ActorCount;
        for (unsigned index = Npc; index < ActorCount; ++index)
            if (a.actors[index] == id)
                actor = index;
        if (actor == ActorCount)
            continue;
        EventRecord &saved = record(a.key);
        out.speaker = id;
        if (saved.kind == Kind::Rescue && actor == Object && world.creatures.record(a.actors[Npc]))
            out.speaker = a.actors[Npc]; // the captive answers for its cage
        if (saved.failed)
            return true;
        if (saved.rewardClaimed) {
            out.line = chestEvent(saved.kind) || landmarkEvent(saved.kind) ? "The chest has been emptied."
                                                                            : "Thank you for your help. Safe travels!";
            return true;
        }
        if (actor == Object && saved.kind == Kind::ChestSack) {
            out.line = !saved.objectiveDone && grantKey(world, saved) ? "You have the key for this chest." : "The sack is empty.";
            flush();
            return true;
        }
        auto open = [&] {
            if (uint8_t *chest = world.creatures.record(a.actors[Container])) {
                set<uint8_t>(chest, InteractionState, 2);
                set<uint16_t>(chest, Toggles, get<uint16_t>(chest, Toggles) & ~MapMarker);
            }
        };
        if (actor == Container && landmarkEvent(saved.kind)) {
            if (!saved.objectiveDone)
                out.line = "Defeat the guardian and its followers first.";
            else if (deliver(world, saved)) {
                open();
                out.line = "You found equipment inside the chest.";
            } else
                out.line = "Please try opening the chest again in a moment.";
            return true;
        }
        if (actor == Container && chestEvent(saved.kind)) {
            if (saved.objectiveDone) {
                if (deliver(world, saved)) {
                    open();
                    out.line = "You found equipment inside the chest.";
                } else
                    out.line = "Please try opening the chest again in a moment.";
                return true;
            }
            if (saved.kind == Kind::ChestBoss && objectiveComplete(saved.enemyCount, saved.defeatedMask))
                grantKey(world, saved);
            if (!world.take(keyItem(saved))) {
                out.line = saved.kind == Kind::ChestSack ? "This chest is locked. Look for its key nearby."
                                                         : "This chest is locked. The guardian holds its key.";
                return true;
            }
            saved.keyOwned = true;
            saved.objectiveDone = true;
            dirty = true;
            if (deliver(world, saved)) {
                open();
                out.line = "You unlocked the chest and found equipment inside.";
            } else
                out.line = "The chest is unlocked. Please try opening it again in a moment.";
            return true;
        }
        if (actor != Npc && !(actor == Object && saved.kind == Kind::Rescue)) {
            out.line = "Help our group, then speak to the adventurer.";
            return true;
        }
        if (!saved.objectiveDone) {
            switch (saved.kind) {
            case Kind::Battle: out.line = "Please help us defeat these enemies. Speak to me afterward for a reward."; break;
            case Kind::Rescue: out.line = "The guardian is keeping me trapped. Defeat it and come back to free me."; break;
            case Kind::Bounty: out.line = "A dangerous enemy is nearby. Defeat it, then return to me for a reward."; break;
            default: out.line = "Unlock the nearby chest, then speak to me for your reward."; break;
            }
            return true;
        }
        if (deliver(world, saved)) {
            if (saved.kind == Kind::Rescue)
                freeRescue(world, a);
            out.line = "Thank you. This equipment is your reward.";
        } else
            out.line = "Your reward is not ready. Please speak to me again in a moment.";
        publishPins();
        return true;
    }
    return townInteraction(world, id, out);
}

int OverworldEvents::questMarker(World &world, int64_t id) {
    if (!enabled())
        return 0;
    const uint8_t *r = world.creatures.record(id);
    if (!r || !town::eligible(r[Hostility], hp(r), get<uint32_t>(r, Flags)) ||
        !std::count(assets.npcRaces.begin(), assets.npcRaces.end(), (int)get<uint32_t>(r, Race)))
        return 0;
    if (auto cached = hints.find(id); cached != hints.end() && now < cached->second.second)
        return cached->second.first;
    int marker = town::NoQuest;
    const Site origin = siteOf(r);
    const Location village = locationAt(origin);
    if (village.type == 5 && std::isfinite(village.influence) && village.influence > 0) {
        bool referred = false;
        for (const Referral &referral : referrals) {
            if (referral.town.x != village.centre.x || referral.town.y != village.centre.y)
                continue;
            referred = true;
            const EventRecord *saved = state.find(referral.objective.world, referral.objective.region, referral.objective.slot);
            if (saved && saved->cycle == referral.cycle)
                marker = town::referralMarker(saved->objectiveDone, saved->rewardClaimed, saved->failed);
        }
        if (!referred) {
            const Region region = regionOf(origin);
            for (const Active &a : active) {
                const EventRecord &saved = record(a.key);
                if (!saved.townQuest && !saved.failed && !saved.rewardClaimed && sameRegion(saved.region, region) &&
                    town::referralKind(saved.kind))
                    marker = town::Offer;
            }
            EventRecord offer;
            if (!marker && townOffer(world, village.centre, village.radius, offer))
                marker = town::Offer;
        }
    }
    if (hints.size() > 512)
        hints.clear();
    hints[id] = {marker, now + 1.0};
    return marker;
}

bool OverworldEvents::command(const std::string &text, std::string &reply) {
    if (text.compare(0, 7, "/events") != 0)
        return false;
    static const char *const tests[] = {"/events test battle", "/events test rescue", "/events test bounty", "/events test chest-sack",
                                        "/events test chest-boss"};
    for (unsigned index = 0; index < 5; ++index)
        if (text == tests[index]) {
            requestedTest = (Kind)index;
            testAttempt = 0;
            reply.clear();
            return true;
        }
    if (text == "/events landmarks") {
        reportLandmarks = true;
        reply = "Nearby landmarks are written to the log.";
        return true;
    }
    if (text == "/events status") {
        reply = std::string("Events: ") + (enabled() ? "on" : "off") + ", " + std::to_string(active.size()) + " active, " +
                std::to_string(state.size()) + " records";
        reportStatus = true;
        return true;
    }
    return false;
}
