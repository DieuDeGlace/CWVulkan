#include "Spawned.h"
#include "SteamCreatureLights.h"
#include "worldgen/BlockColours.h"
#include "worldgen/Generator.h"
#include "NativeItems.h"
#include "ProceduralItems.h"
#include "NativeRules.h"
#include "NativeUpdate.h"
#include "NativeInteract.h"
#include "GroundItems.h"
#include "NativePlayer.h"
#include "NativePlayerHit.h"
#include "NativePet.h"
#include "World.h"
#include "GameMath.h"
#include "NativeBuffs.h"

#include <cmath>
#include <bit>
#include <cstring>
#include <fstream>
#include <random>
#include <set>
#include <sstream>
#include <unordered_map>
#include <glm/gtc/matrix_transform.hpp>

namespace {
constexpr int NEAR_ZONES = 4; // the game makes creatures within 256 blocks of a player (0x2e17d0)

// Creature record offsets of the drawn place and its smoothing (netcode-packets.md §2.4,
// creature-pose.md).
constexpr size_t ORIENTATION = 0x28;                // f32 x3: pitch, roll, yaw
constexpr size_t HIT_FLASH = 0x994;                 // f32: 1.0 when struck
constexpr size_t PREVIOUS_DRAWN_POSITION = 0xaf8;   // i64 x3
constexpr size_t DRAWN_POSITION = 0xb10;            // i64 x3
constexpr size_t PREVIOUS_DRAWN_ORIENTATION = 0xb28; // f32 x3
constexpr size_t DRAWN_ORIENTATION = 0xb34;         // f32 x3
constexpr size_t PREVIOUS_AIM = 0xb40;              // f32 x3
constexpr size_t AIM = 0xb4c;                       // f32 x3
constexpr int SPIRIT_CUBE_MODEL = 0xa46;            // cube.cub, one white voxel (creature-pose-items.md §9.1)
constexpr size_t SMOOTHING = 0xb58;                 // f32: ms of easing left
constexpr float SMOOTHING_MS = 60.0f;
// How fast the drawn place eases (the frame blend's rates; netcode-compatibility.md open
// question 13 asks which constant goes where and what counts the 60 ms down: here the
// position eases at 0.02, the orientation at 0.03, and the frame's milliseconds count it down).
constexpr float POSITION_EASE = 0.02f, ORIENTATION_EASE = 0.03f;
constexpr int32_t RACE_WITHOUT_DEATH = 0x11d; // a death the frame routine does not collect
constexpr size_t ABILITY_TIME = 0x6c;         // f32: ms into the current ability
// How far a mirror draws a creature's ability on past its host's last update (CubeVulkan's own:
// a few host ticks; the game draws the times as they arrive, about every 32 ms).
constexpr float ABILITY_LEAD_MS = 200.0f;

template <class T> void put(worldgen::CreatureRecord &r, size_t at, T v) { std::memcpy(&r[at], &v, sizeof v); }
template <class T> T get(const worldgen::CreatureRecord &r, size_t at) {
    T v;
    std::memcpy(&v, &r[at], sizeof v);
    return v;
}
int floorDiv(int v, int d) { return v >= 0 ? v / d : -((-v + d - 1) / d); }
} // namespace

bool SpawnedCreatures::init(const std::filesystem::path &extractDir, std::string *why) {
    std::ifstream in(extractDir / "sprites.txt");
    if (!in) {
        if (why)
            *why = "no sprites.txt in " + extractDir.string();
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream l(line);
        int id;
        std::string path;
        Sprite s;
        if (!(l >> id >> path >> s.size[0] >> s.size[1] >> s.size[2]) || id < 0)
            continue;
        if (path.rfind("sprites/", 0) == 0)
            path = path.substr(8);
        if (path.size() > 4 && path.substr(path.size() - 4) == ".cub")
            path.resize(path.size() - 4);
        s.name = path;
        if ((size_t)id >= sprites.size())
            sprites.resize(id + 1);
        sprites[id] = s;
    }
    nativeitems::load(extractDir / "oracle"); // worn items' models (optional: without it none are drawn)
    for (size_t id = 0; id < sprites.size(); id++) {
        nativepose::Model m;
        std::memcpy(m.size, sprites[id].size, sizeof m.size);
        m.user = reinterpret_cast<const void *>((intptr_t)id);
        models[(int)id] = m;
    }
    return !sprites.empty();
}

void SpawnedCreatures::setEnemyRespawn(enemyrespawn::Settings settings) {
    respawnSettings = settings;
    enemyDeaths = enemyrespawn::Deaths(settings);
}

void SpawnedCreatures::advanceRespawns(double elapsedMs) {
    if (!client)
        releaseDeaths(enemyDeaths.advance(elapsedMs / 1000.0));
}

std::set<int64_t> SpawnedCreatures::persistentDeadSet() const {
    std::set<int64_t> saved;
    for (int64_t id : worldDead)
        if (!enemyDeaths.contains(id))
            saved.insert(id);
    return saved;
}

void SpawnedCreatures::releaseDeaths(const std::vector<int64_t> &ids) {
    for (int64_t id : ids) {
        remove(id);
        made.erase(id);
        worldDead.erase(id);
        dead.erase(id);
        seenAlive.erase(id);
        toBurst.erase(id);
        lastAbility.erase(id);
        trips.erase(id);
        // A resident is rebuilt from its original spawn point on the next update.
    }
}

void SpawnedCreatures::update(ExtractedWorld &ew, glm::vec3 playerLocal) {
    if (!worldgen::creatureTablesLoaded())
        return;
    lastWorld = &ew;
    // The players this world keeps its zones around: the local player and every player the
    // network brought (a host world's guests). Before there is any, where the local player is
    // about to be.
    std::vector<const worldgen::CreatureRecord *> players;
    if (hasPlayer)
        players.push_back(&player.record);
    if (const auto network = zones.find(NETWORK_ZONE); network != zones.end())
        for (const Creature &cr : network->second)
            if (cr.record[worldgen::creature::HOSTILITY] == 0)
                players.push_back(&cr.record);
    std::vector<worldgen::ResidentWorld::Player> at;
    for (const worldgen::CreatureRecord *r : players)
        at.push_back({get<int64_t>(*r, worldgen::creature::POSITION), get<int64_t>(*r, worldgen::creature::POSITION + 8),
                      get<int64_t>(*r, worldgen::creature::POSITION + 16)});
    if (at.empty())
        at.push_back({(int64_t)std::floor((double)playerLocal.x * 65536.0) + ew.originDotsX(),
                      (int64_t)std::floor((double)playerLocal.y * 65536.0) + ew.originDotsY(),
                      (int64_t)std::floor(((double)playerLocal.z - ExtractedWorld::Z_OFFSET) * 65536.0)});
    std::vector<std::pair<int, int>> centres;
    for (const auto &p : at)
        centres.push_back({floorDiv((int)(p.x >> 16), 64), floorDiv((int)(p.y >> 16), 64)});
    auto near = [&](int zx, int zy, int reach) {
        for (const auto &[cx, cy] : centres)
            if (std::abs(zx - cx) <= reach && std::abs(zy - cy) <= reach)
                return true;
        return false;
    };
    ew.keepAround(this, centres, NEAR_ZONES + 1);
    // A zone that leaves memory has its state recorded (0x2d3e20) and its lists let go.
    for (const nativesaves::Cell &zone : zoneStates.zonesInMemory())
        if (!near(zone[0], zone[1], NEAR_ZONES + 1)) {
            releaseDeaths(enemyDeaths.unload({zone[0], zone[1]}));
            zoneStates.unload(zone, deadTrackedIn({zone[0], zone[1]}));
            stateNeeded.erase({zone[0], zone[1]});
            unloadedZones.push_back({zone[0], zone[1]});
        }
    for (auto it = zones.begin(); it != zones.end();)
        if (it->first != NETWORK_ZONE && !near(it->first.first, it->first.second, NEAR_ZONES + 1)) {
            releaseDeaths(enemyDeaths.unload(it->first));
            for (const Creature &cr : it->second)
                made.erase(get<int64_t>(cr.record, worldgen::creature::ID));
            if (const auto residents = zoneResidents.find(it->first); residents != zoneResidents.end())
                for (const auto &[id, sp] : residents->second->spawns.points) // (a zone let go takes its creatures)
                    made.erase(id);
            zoneMissions.erase(it->first), zoneResidents.erase(it->first), zoneRand.erase(it->first);
            zoneSpots.erase(it->first), zoneChests.erase(it->first);
            it = zones.erase(it);
        } else
            ++it;
    if (client) {
        // A mirror makes no creatures; it only has the near zones' lists ready (their objects and
        // ground items, its own generation's until the host sends its state).
        for (const auto &[cx, cy] : centres)
            for (int zx = cx - NEAR_ZONES; zx <= cx + NEAR_ZONES; zx++)
                for (int zy = cy - NEAR_ZONES; zy <= cy + NEAR_ZONES; zy++)
                    zoneLists(ew, zx, zy);
        return;
    }
    // A zone that comes near brings its residents (zone+0x460), its mission creatures
    // (+0x498) and the creatures it counts the deaths of (+0x480).
    std::vector<std::pair<int, int>> fresh;
    for (const auto &[cx, cy] : centres)
        for (int zx = cx - NEAR_ZONES; zx <= cx + NEAR_ZONES; zx++)
            for (int zy = cy - NEAR_ZONES; zy <= cy + NEAR_ZONES; zy++) {
                if (zones.count({zx, zy}))
                    continue;
                zones[{zx, zy}];
                const std::shared_ptr<const worldgen::ZoneContents> zone = zoneResidents[{zx, zy}] = worldgen::zone(zx, zy);
                // Old saves have no respawn timestamps. In a respawning world, an enemy
                // killed before this instance starts is available again when its zone loads.
                if (respawnSettings.mode != enemyrespawn::Mode::Permanent)
                    for (const auto &[id, point] : zone->spawns.points)
                        if (enemyrespawn::isEnemy(point.hostility) && !enemyDeaths.contains(id) && !protectedDead.count(id))
                            worldDead.erase(id);
                zoneRand.emplace(std::make_pair(zx, zy), std::mt19937((uint32_t)(zx * 73856093) ^ (uint32_t)(zy * 19349663)));
                zoneTracked[{zx, zy}] = zone->spawns.tracked;
                if (!zone->spawns.mission.empty())
                    zoneMissions[{zx, zy}] = zone->spawns.mission;
                fresh.push_back({zx, zy});
            }
    // A visited zone refreshes its daily spawn points and base objects before its
    // residents are scanned. Refreshing alone does not remove live shoppers.
    // See remaining-runtime-gaps.md §6; a midnight reset is a separate operation.
    for (auto &[key, zone] : zoneResidents)
        if (near(key.first, key.second, NEAR_ZONES) && zone->spawns.day != dayCount) {
            auto refreshed = std::make_shared<worldgen::ZoneContents>(*zone);
            worldgen::refreshZoneCreatures(*refreshed, worldgen::currentWorld(), worldgen::creatureTables(), dayCount);
            zone = refreshed;
            for (const auto &[id, point] : refreshed->spawns.points)
                if (enemyDeaths.contains(id) && !enemyrespawn::isEnemy(point.hostility)) {
                    // Daily points can reuse an id for a friendly resident.
                    enemyDeaths.forget(id);
                    worldDead.erase(id);
                }
            if (auto *lists = zoneStates.find({key.first, key.second})) {
                std::vector<nativesaves::Record> objects;
                for (const auto &source : refreshed->objects) {
                    worldgen::ZoneObject object;
                    static_cast<worldgen::StaticObject &>(object) = source;
                    objects.push_back(zonestates::staticObjectRecord(object));
                }
                if (objects != lists->staticObjects) {
                    lists->staticObjects = std::move(objects);
                    zoneStates.objectsChanged({key.first, key.second});
                    objectsChanged.insert(key);
                }
            }
        }
    // the world's resident update (0x2e17d0): the residents not in the world yet, not dead for good,
    // within 256 blocks of a player, whose conditions are met, are made.
    // The regions where a player lifted the demon portals' promotion (+0x128 bit 0x800).
    auto regionOf = [](const worldgen::CreatureRecord &r) {
        return std::array<int32_t, 2>{get<int32_t>(r, worldgen::creature::REGION), get<int32_t>(r, worldgen::creature::REGION + 4)};
    };
    std::set<std::array<int32_t, 2>> spared;
    for (const worldgen::CreatureRecord *r : players)
        if (get<uint16_t>(*r, 0x128) & 0x800)
            spared.insert(regionOf(*r));
    const worldgen::ResidentWorld resident = residentWorld(at);
    const auto maxHealth = [](const uint8_t *record) { return nativerules::maxHealth(record); };
    for (auto &[key, zone] : zoneResidents) {
        auto &list = zones[key];
        auto rand = [&, &worldRand = zoneRand.at(key)] { return (int)(worldRand() & 0x7fff); };
        for (const auto &[id, sp] : zone->spawns.points) {
            if (!worldgen::residentReady(sp, key.first, key.second, resident))
                continue;
            made.insert(id);
            const worldgen::Int2 region = worldgen::regionAt((int32_t)(sp.x >> 16), (int32_t)(sp.y >> 16));
            worldgen::MadeCreature m = worldgen::makeResident(sp, worldgen::Draw(rand), spared.count({region.x, region.y}) != 0, maxHealth);
            // What 0x2e17d0 copies beside the record: who it keeps with (+0xc08), its special
            // moves (+0xbd0), the races it may call up (+0xbf0) and its inventory (+0x9f0).
            Creature cr{std::move(m.record), {}};
            cr.side.group = m.extras.group;
            cr.side.skills = m.extras.abilities;
            cr.side.summonRaces = m.extras.abilities2;
            cr.side.inventory = worldgen::inventoryRecords(m.extras.inventory);
            list.push_back(std::move(cr));
        }
    }
    if (roadTravel)
        for (const auto &key : fresh)
            startTrips(zones[key]);
    // The arena masters' stages (0x2e3510) and the petrified freed where the witch's curse
    // was lifted (0x2e17d0, +0x128 bit 0x8000 of a player in the region).
    std::set<std::array<int32_t, 2>> unpetrified;
    for (const worldgen::CreatureRecord *r : players)
        if (get<uint16_t>(*r, 0x128) & 0x8000)
            unpetrified.insert(regionOf(*r));
    for (auto &[k, list] : zones)
        for (Creature &cr : list) {
            if (k == NETWORK_ZONE)
                continue;
            worldgen::CreatureRecord &r = cr.record;
            if (r[worldgen::creature::CLASS] == worldgen::ARENA_MASTER_CLASS && r[worldgen::creature::STAGE] != worldgen::ARENA_FINISHED)
                arenaStage(r);
            if ((get<uint32_t>(r, worldgen::creature::FLAGS) & 0x400000) &&
                unpetrified.count({get<int32_t>(r, worldgen::creature::REGION), get<int32_t>(r, worldgen::creature::REGION + 4)}))
                put<uint32_t>(r, worldgen::creature::FLAGS, get<uint32_t>(r, worldgen::creature::FLAGS) & ~0x400000u);
        }
}

worldgen::ResidentWorld SpawnedCreatures::residentWorld(const std::vector<worldgen::ResidentWorld::Player> &players) const {
    worldgen::ResidentWorld w;
    w.players = players;
    w.alive = [this](int64_t id) { return made.count(id) != 0; };
    w.dead = [this](int64_t id) { return worldDead.count(id) != 0; };
    // The dead tracked creatures (zone+0x480) of a zone. The game counts its loaded zones' and the
    // saved counts of zones it had loaded; CubeVulkan, the zones it has made the creatures of
    // this session.
    w.deadTracked = [this](int32_t zx, int32_t zy) {
        if (const std::optional<int32_t> count = deadTrackedIn({zx, zy}))
            return *count;
        // A zone not made this session: the count its stored state kept.
        if (storedZones)
            if (auto stored = storedZones->zones.find({zx, zy}); stored != storedZones->zones.end())
                return stored->second.deadTracked;
        return 0;
    };
    // An arena master at the stage.
    w.arenaAtStage = [this](int stage) {
        for (const auto &[k, list] : zones)
            for (const Creature &cr : list)
                if (cr.record[worldgen::creature::CLASS] == worldgen::ARENA_MASTER_CLASS && cr.record[worldgen::creature::STAGE] == stage)
                    return true;
        return false;
    };
    return w;
}

void SpawnedCreatures::arenaStage(worldgen::CreatureRecord &master) {
    // The zones within 64 blocks of where it lives (+0x1b8).
    const int64_t hx = get<int64_t>(master, worldgen::creature::HOME), hy = get<int64_t>(master, worldgen::creature::HOME + 8);
    auto zoneOf = [](int64_t dots) { return floorDiv((int)(dots >> 16), 64); };
    auto dead = [this](int64_t id) { return worldDead.count(id) != 0; };
    for (int zx = zoneOf(hx - 0x400000); zx <= zoneOf(hx + 0x400000); zx++)
        for (int zy = zoneOf(hy - 0x400000); zy <= zoneOf(hy + 0x400000); zy++) {
            auto it = zoneResidents.find({zx, zy});
            if (it == zoneResidents.end())
                continue;
            std::vector<const worldgen::SpawnPoint *> points;
            for (const auto &[id, sp] : it->second->spawns.points)
                points.push_back(&sp);
            uint8_t &stage = master[worldgen::creature::STAGE];
            stage = worldgen::arenaStage(stage, points, dead);
        }
}

size_t SpawnedCreatures::count() const {
    size_t n = 0;
    for (auto &[k, list] : zones)
        n += list.size();
    return n;
}

void SpawnedCreatures::contacts(size_t &ground, size_t &water, size_t &all) const {
    ground = water = all = 0;
    for (auto &[k, list] : zones)
        for (const Creature &cr : list) {
            const uint32_t f = get<uint32_t>(cr.record, 0x5c);
            ground += f & 1, water += (f >> 1) & 1, all++;
        }
}

bool SpawnedCreatures::nearestPerson(const ExtractedWorld &ew, glm::vec3 from, float range, int64_t &id, int &race,
                                     glm::vec3 &at) const {
    float best = range;
    bool found = false;
    for (auto &[k, list] : zones)
        for (const Creature &cr : list) {
            const uint8_t h = cr.record[worldgen::creature::HOSTILITY];
            if ((h != 3 && h != 4 && h != 8) || !(get<float>(cr.record, worldgen::creature::HEALTH) > 0.0f))
                continue;
            const glm::vec3 p = ew.toLocal(reinterpret_cast<const int64_t *>(&cr.record[worldgen::creature::POSITION]));
            const float d = glm::length(p - from);
            if (d < best) {
                best = d, found = true, at = p;
                id = get<int64_t>(cr.record, worldgen::creature::ID);
                race = get<int32_t>(cr.record, worldgen::creature::RACE);
            }
        }
    return found;
}

int64_t SpawnedCreatures::make(int race, int hostility, int level, const int64_t dots[3], worldgen::Int2 region) {
    const int zx = (int)(dots[0] >> 22), zy = (int)(dots[1] >> 22); // (64 blocks of 65536 dots)
    auto zone = zones.find({zx, zy});
    if (zone == zones.end())
        return 0;
    const int64_t id = nextMade++;
    const auto maxHealth = [](const uint8_t *record) { return nativerules::maxHealth(record); };
    worldgen::MadeCreature m = worldgen::makeInPlay(id, race, hostility, level, dots, region, 0, worldgen::creatureTables(), maxHealth);
    Creature cr{std::move(m.record), {}};
    cr.side.skills = m.extras.abilities;
    cr.side.summonRaces = m.extras.abilities2;
    cr.side.inventory = worldgen::inventoryRecords(m.extras.inventory);
    zone->second.push_back(std::move(cr));
    return id;
}

uint8_t *SpawnedCreatures::record(int64_t id) {
    for (auto &[k, list] : zones)
        for (Creature &cr : list)
            if (get<int64_t>(cr.record, worldgen::creature::ID) == id)
                return cr.record.data();
    return nullptr;
}

const uint8_t *SpawnedCreatures::record(int64_t id) const {
    return const_cast<SpawnedCreatures *>(this)->record(id);
}

void SpawnedCreatures::remove(int64_t id) {
    for (auto &[k, list] : zones)
        for (auto it = list.begin(); it != list.end(); ++it)
            if (get<int64_t>(it->record, worldgen::creature::ID) == id) {
                list.erase(it);
                ai.erase(id);
                return;
            }
}

void SpawnedCreatures::forEach(const std::function<void(const uint8_t *)> &f) const {
    for (auto &[k, list] : zones)
        for (const Creature &cr : list)
            f(cr.record.data());
}

void SpawnedCreatures::keepHealthShares(const std::function<void()> &change) {
    auto maxHealth = [](const Creature &cr) {
        std::vector<nativerules::Buff> buffs;
        for (auto &b : cr.side.buffs) {
            float v;
            std::memcpy(&v, b.data() + 4, 4);
            buffs.push_back({b[0], v});
        }
        return nativerules::maxHealth(cr.record.data(), buffs.data(), buffs.size());
    };
    std::vector<std::pair<Creature *, float>> shares;
    for (auto &[k, list] : zones)
        for (Creature &cr : list)
            if (const float hp = get<float>(cr.record, worldgen::creature::HEALTH), most = maxHealth(cr); hp > 0 && most > 0)
                shares.push_back({&cr, std::clamp(hp / most, 0.f, 1.f)});
    change();
    for (auto &[cr, share] : shares)
        put<float>(cr->record, worldgen::creature::HEALTH, maxHealth(*cr) * share);
}

bool SpawnedCreatures::positionOf(const ExtractedWorld &ew, int64_t id, glm::vec3 &at) const {
    for (auto &[k, list] : zones)
        for (const Creature &cr : list)
            if (get<int64_t>(cr.record, worldgen::creature::ID) == id) {
                at = ew.toLocal(reinterpret_cast<const int64_t *>(&cr.record[worldgen::creature::POSITION]));
                return true;
            }
    return false;
}

int SpawnedCreatures::classOf(int64_t id) const {
    for (auto &[k, list] : zones)
        for (const Creature &cr : list)
            if (get<int64_t>(cr.record, worldgen::creature::ID) == id)
                return cr.record[worldgen::creature::CLASS];
    return -1;
}

bool SpawnedCreatures::homeOf(int64_t id, int32_t &x, int32_t &y) const {
    for (auto &[k, list] : zones)
        for (const Creature &cr : list)
            if (get<int64_t>(cr.record, worldgen::creature::ID) == id) {
                const int64_t hx = get<int64_t>(cr.record, worldgen::creature::HOME), hy = get<int64_t>(cr.record, worldgen::creature::HOME + 8);
                x = (int32_t)(hx >= 0 ? hx / 65536 : -((-hx + 65535) / 65536));
                y = (int32_t)(hy >= 0 ? hy / 65536 : -((-hy + 65535) / 65536));
                return true;
            }
    return false;
}

std::string SpawnedCreatures::angriest(const ExtractedWorld &ew, glm::vec3 playerLocal) const {
    const Creature *best = nullptr;
    float bestD = 1e9f;
    for (auto &[k, list] : zones)
        for (const Creature &cr : list)
            if (auto f = cr.side.aggro.find(localId); f != cr.side.aggro.end() && f->second > 0.0f &&
                                               get<float>(cr.record, worldgen::creature::HEALTH) > 0.0f) {
                const glm::vec3 at = ew.toLocal(reinterpret_cast<const int64_t *>(&cr.record[worldgen::creature::POSITION]));
                const float d = glm::length(at - playerLocal);
                if (d < bestD)
                    bestD = d, best = &cr;
            }
    if (!best)
        return "";
    const glm::vec3 at = ew.toLocal(reinterpret_cast<const int64_t *>(&best->record[worldgen::creature::POSITION]));
    const float bearing = std::atan2(playerLocal.y - at.y, playerLocal.x - at.x) * 57.2957795f;
    char s[200];
    std::snprintf(s, sizeof s, "race %d ability 0x%x t %.0f yaw %.1f bearing %.1f distance %.1f aim %.2f %.2f",
                  get<int32_t>(best->record, worldgen::creature::RACE), best->record[worldgen::creature::ABILITY],
                  get<float>(best->record, 0x6c), get<float>(best->record, worldgen::creature::YAW), bearing, bestD,
                  get<float>(best->record, 0x174), get<float>(best->record, 0x178));
    return s;
}

size_t SpawnedCreatures::afterPlayer() const {
    size_t n = 0;
    for (auto &[k, list] : zones)
        for (const Creature &cr : list)
            if (auto f = cr.side.aggro.find(localId); f != cr.side.aggro.end() && f->second > 0.0f)
                n++;
    return n;
}

bool SpawnedCreatures::bossAfterPlayer() const {
    constexpr uint8_t MONSTER = 1;
    constexpr uint32_t BOSS_FLAGS = 0x40000 | 0x200; // a boss (0x5eee0) or an elite (0x5eda0)
    for (auto &[k, list] : zones)
        for (const Creature &cr : list)
            if (cr.record[worldgen::creature::HOSTILITY] == MONSTER && cr.side.aggro.count(localId) &&
                (get<uint32_t>(cr.record, worldgen::creature::FLAGS) & BOSS_FLAGS))
                return true;
    return false;
}

std::vector<nativeai::Creature *> SpawnedCreatures::worldList() {
    std::vector<nativeai::Creature *> list;
    if (hasPlayer) {
        player.side.record = player.record.data();
        list.push_back(&player.side);
    }
    for (auto &[zone, creatures] : zones)
        for (Creature &cr : creatures) {
            cr.side.record = cr.record.data();
            list.push_back(&cr.side);
        }
    return list;
}

std::vector<SpawnedCreatures::Target> SpawnedCreatures::targets(const ExtractedWorld &ew, glm::vec3 near, float range) const {
    std::vector<Target> out;
    for (auto &[zone, list] : zones)
        for (const Creature &cr : list) {
            if (!(get<float>(cr.record, worldgen::creature::HEALTH) > 0.0f))
                continue;
            const glm::vec3 at = ew.toLocal(reinterpret_cast<const int64_t *>(&cr.record[worldgen::creature::POSITION]));
            if (glm::length(at - near) > range)
                continue;
            std::vector<nativerules::Buff> buffs;
            for (auto &b : cr.side.buffs) {
                float v;
                std::memcpy(&v, b.data() + 4, 4);
                buffs.push_back({b[0], v});
            }
            out.push_back({get<int64_t>(cr.record, worldgen::creature::ID), at,
                           nativerules::armor(cr.record.data(), buffs.data(), buffs.size()),
                           nativerules::resistance(cr.record.data(), buffs.data(), buffs.size()),
                           cr.record[worldgen::creature::HOSTILITY] == 1});
        }
    return out;
}

std::vector<SpawnedCreatures::Label> SpawnedCreatures::labels(const ExtractedWorld &ew, glm::vec3 near, float range) const {
    std::vector<Label> out;
    for (auto &[zone, list] : zones)
        for (const Creature &cr : list) {
            const worldgen::CreatureRecord &r = cr.record;
            if (!(get<float>(r, worldgen::creature::HEALTH) > 0.0f))
                continue;
            Label l;
            l.at = ew.toLocal(reinterpret_cast<const int64_t *>(&r[worldgen::creature::POSITION]));
            l.distance2 = glm::dot(l.at - near, l.at - near);
            if (l.distance2 > range * range)
                continue;
            std::vector<nativerules::Buff> buffs;
            for (auto &b : cr.side.buffs) {
                float v;
                std::memcpy(&v, b.data() + 4, 4);
                buffs.push_back({b[0], v});
            }
            l.id = get<int64_t>(r, worldgen::creature::ID);
            l.height = get<float>(r, 0x8c);
            l.hp = get<float>(r, worldgen::creature::HEALTH);
            l.maxHp = std::max(nativerules::maxHealth(r.data(), buffs.data(), buffs.size()), 1e-6f);
            l.hostility = r[worldgen::creature::HOSTILITY];
            l.race = get<int32_t>(r, worldgen::creature::RACE);
            l.level = get<int32_t>(r, worldgen::creature::LEVEL);
            l.cls = r[worldgen::creature::CLASS];
            l.flags = get<uint32_t>(r, worldgen::creature::FLAGS);
            l.faction = r[0x970];
            const char *name = reinterpret_cast<const char *>(&r[0x958]);
            l.name.assign(name, strnlen(name, 16));
            out.push_back(l);
        }
    return out;
}

float SpawnedCreatures::strike(int64_t id, float damage, bool critical, float charge, int ability, int type, glm::vec2 dir) {
    if (!hasPlayer)
        return -1;
    const std::vector<nativeai::Creature *> list = worldList();
    std::map<int64_t, nativeai::Creature *> byId;
    for (nativeai::Creature *c : list) {
        int64_t cid;
        std::memcpy(&cid, c->record + worldgen::creature::ID, 8);
        byId[cid] = c;
    }
    byId[localId] = &player.side;
    auto target = byId.find(id);
    if (target == byId.end() || id == localId)
        return -1;
    nativehit::World hw;
    hw.client = client;
    hw.local = player.record.data();
    hw.creatures = list;
    hw.byId = [&](int64_t i) -> nativeai::Creature * {
        auto f = byId.find(i);
        return f == byId.end() ? nullptr : f->second;
    };
    hw.rand = [this] {
        randState ^= randState << 13, randState ^= randState >> 17, randState ^= randState << 5;
        return (int)(randState & 0x7fff);
    };
    hw.died = [&](nativeai::Creature *c) {
        if (lastWorld && !client)
            dropFrom(*lastWorld, c);
    };
    nativeupkeep::Hit h;
    h.target = id;
    h.attacker = localId;
    h.damage = damage;
    h.critical = critical;
    h.charge = charge;
    std::memcpy(h.where, &player.record[worldgen::creature::POSITION], 24);
    // The push the game's melee step gives (as for a creature of attack speed 1, 0x5f180's
    // default): 2 along the way to the target, a quarter of it up.
    h.push[0] = dir.x * 2.0f, h.push[1] = dir.y * 2.0f, h.push[2] = 0.5f;
    h.type = type;
    h.ability = ability;
    h.last = 1;
    uint8_t *r = target->second->record;
    float before;
    std::memcpy(&before, r + worldgen::creature::HEALTH, 4);
    const bool landed = nativehit::hit(hw, target->second, &player.side, h);
    sounds.insert(sounds.end(), hw.sounds.begin(), hw.sounds.end()); // the impact's sound
    keepRecords(hw.sounds, hw.particles, hw.numbers, hw.buffNotes);
    for (const auto &a : hw.aggroNotes)
        worldEvents.aggro.push_back(a);
    if (!landed)
        return -1;
    // A mirror applies nothing: the hit is the host's to apply, and what it is worth is its
    // damage number (the last one made, the main number).
    if (client) {
        float worth = 0;
        if (!hw.numbers.empty())
            std::memcpy(&worth, hw.numbers.back().data() + 0x10, 4);
        return worth;
    }
    float after;
    std::memcpy(&after, r + worldgen::creature::HEALTH, 4);
    return before - after;
}

void SpawnedCreatures::keepRecords(const std::vector<std::array<uint8_t, 0x30>> &soundList,
                                   const std::vector<std::array<uint8_t, 0x48>> &particleList,
                                   const std::vector<std::array<uint8_t, 0x48>> &numberList,
                                   const std::vector<std::array<uint8_t, 0x30>> &buffList) {
    worldEvents.sounds.insert(worldEvents.sounds.end(), soundList.begin(), soundList.end());
    worldEvents.particles.insert(worldEvents.particles.end(), particleList.begin(), particleList.end());
    worldEvents.damageNumbers.insert(worldEvents.damageNumbers.end(), numberList.begin(), numberList.end());
    for (const auto &number : numberList) notifyHitVisual(number);
    worldEvents.buffs.insert(worldEvents.buffs.end(), buffList.begin(), buffList.end());
}

void SpawnedCreatures::keepBlockChanges(const std::vector<std::array<uint8_t, 0x20>> &changes) {
    for (const auto &change : changes) {
        cwnet::BlockChangeRecord r;
        std::memcpy(r.data(), change.data(), r.size()); // (the network sends the first 28 bytes)
        worldEvents.blockChanges.push_back(r);
    }
}

nativedrop::World SpawnedCreatures::dropWorld(ExtractedWorld &ew) {
    nativedrop::World dw;
    dw.rand = [this] {
        randState ^= randState << 13, randState ^= randState >> 17, randState ^= randState << 5;
        return (int)(randState & 0x7fff);
    };
    dw.creatures = worldList();
    auto byId = std::make_shared<std::map<int64_t, nativeai::Creature *>>();
    for (nativeai::Creature *c : dw.creatures) {
        int64_t cid;
        std::memcpy(&cid, c->record + worldgen::creature::ID, 8);
        (*byId)[cid] = c;
    }
    if (hasPlayer)
        (*byId)[localId] = &player.side;
    dw.byId = [byId](int64_t id) -> nativeai::Creature * {
        auto f = byId->find(id);
        return f == byId->end() ? nullptr : f->second;
    };
    dw.day = (int32_t)aiClock;
    dw.petList = specials;
    dw.petListsByKey = specialsBy;
    dw.block = [&ew](int x, int y, int z) {
        const auto b = ew.gameBlock(x, y, z);
        nativeai::Block o;
        std::memcpy(o.data(), b.data(), o.size());
        return o;
    };
    dw.zone = [&ew](int zx, int zy) {
        bool loaded = false;
        ew.gameBlock((int64_t)zx * 64, (int64_t)zy * 64, 0, &loaded);
        return loaded;
    };
    return dw;
}

void SpawnedCreatures::keepDrops(ExtractedWorld &ew, const nativedrop::World &dw, int64_t from, int32_t level, bool reward) {
    for (const nativedrop::Drop &d : dw.drops) {
        drops.push_back({from, level, d, ew.toLocal(reinterpret_cast<const int64_t *>(d.data() + 0xa0)), reward});
        // The ground item joins its zone's list (0x2a5b70), which the zone keeps, saves and,
        // having changed, sends to every guest.
        int64_t at[2];
        std::memcpy(at, d.data() + grounditems::ground::POSITION, sizeof at);
        const nativesaves::Cell zone = zonestates::zoneOfBlock(at[0] >> 16, at[1] >> 16);
        if (nativesaves::ZoneLists *lists = zoneLists(ew, zone[0], zone[1])) {
            lists->groundItems.push_back(d);
            zoneStates.groundItemsChanged(zone);
            groundChanged.insert({zone[0], zone[1]});
        }
    }
}

void SpawnedCreatures::dropFrom(ExtractedWorld &ew, nativeai::Creature *dead) {
    nativedrop::World dw = dropWorld(ew);
    nativedrop::drop(dw, dead);
    int64_t id;
    int32_t level;
    std::memcpy(&id, dead->record + worldgen::creature::ID, 8);
    std::memcpy(&level, dead->record + worldgen::creature::LEVEL, 4);
    uint16_t toggles;
    std::memcpy(&toggles, dead->record + 0x128, 2);
    if (toggles & 0x100) // (0x29dcf0: a resident with a task done also gives its reward)
        nativedrop::reward(dw, reinterpret_cast<const int64_t *>(dead->record + worldgen::creature::POSITION), std::min(level + 1, 4));
    keepDrops(ew, dw, id, level, false);
}

bool SpawnedCreatures::loadSpecials(const std::filesystem::path &file) {
    std::ifstream in(file, std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    if (b.size() < 12 || std::memcmp(b.data(), "CIG1", 4) != 0)
        return false;
    size_t k = 4;
    auto u32 = [&] {
        uint32_t v = 0;
        if (k + 4 <= b.size())
            std::memcpy(&v, &b[k], 4);
        k += 4;
        return v;
    };
    specials.resize(u32());
    for (auto &v : specials)
        v = (int32_t)u32();
    const uint32_t n = u32();
    for (uint32_t i = 0; i < n && k < b.size(); i++) {
        const int32_t key = (int32_t)u32();
        auto &v = specialsBy[key];
        v.resize(u32());
        for (auto &e : v)
            e = (int32_t)u32();
    }
    return k <= b.size() && !specials.empty();
}

std::string SpawnedCreatures::itemSprite(const uint8_t *item) const {
    if (item && item[0] == nativeitems::CATEGORY_ON_DEMAND) {
        const auto fields = nativeitems::fieldsOf(item);
        return proceduralitem::spriteKey({std::bit_cast<int32_t>(fields.id), fields.modifier});
    }
    const int id = nativeitems::modelId(item);
    return id >= 0 && (size_t)id < sprites.size() ? sprites[id].name : std::string();
}

void SpawnedCreatures::setPlayerClass(int cls) {
    if (hasPlayer)
        put<uint8_t>(player.record, worldgen::creature::CLASS, (uint8_t)cls);
}

std::vector<SpawnedCreatures::Death> SpawnedCreatures::takeDeaths(const ExtractedWorld &ew) {
    std::vector<Death> out;
    for (auto &[zone, list] : zones)
        for (const Creature &cr : list) {
            const int64_t id = get<int64_t>(cr.record, worldgen::creature::ID);
            if (get<float>(cr.record, worldgen::creature::HEALTH) > 0.0f) {
                seenAlive.insert(id);
                continue;
            }
            if (dead.count(id) || !seenAlive.count(id)) // (already dead when its zone came)
                continue;
            dead.insert(id);
            worldDead.insert(id); // (the world's: its zone does not make it again)
            if (!client && enemyrespawn::isEnemy(cr.record[worldgen::creature::HOSTILITY]))
                if (const auto source = zoneResidents.find(zone); source != zoneResidents.end() && source->second->spawns.points.count(id))
                    enemyDeaths.killed(id, zone);
            toBurst.insert(id);
            out.push_back({id, get<int64_t>(cr.record, 0x9e0),
                           ew.toLocal(reinterpret_cast<const int64_t *>(&cr.record[worldgen::creature::POSITION])),
                           get<int32_t>(cr.record, worldgen::creature::RACE), get<int32_t>(cr.record, worldgen::creature::LEVEL), cr.record,
                           reinterpret_cast<uintptr_t>(&cr)});
        }
    return out;
}

void SpawnedCreatures::abilities(size_t &using_, size_t &started) {
    for (auto &[k, list] : zones)
        for (const Creature &cr : list)
            if (const uint8_t a = cr.record[0x68]) {
                using_++;
                const int64_t id = get<int64_t>(cr.record, worldgen::creature::ID);
                if (lastAbility[id] != a)
                    started++;
                lastAbility[id] = a;
            } else
                lastAbility.erase(get<int64_t>(cr.record, worldgen::creature::ID));
}

void SpawnedCreatures::walking(size_t &moving, size_t &pathing) const {
    moving = pathing = 0;
    for (auto &[k, list] : zones)
        for (const Creature &cr : list) {
            const float x = get<float>(cr.record, 0x40), y = get<float>(cr.record, 0x44);
            moving += x * x + y * y > 0.0f;
        }
    for (auto &[id, e] : ai)
        pathing += !e.path.empty();
}

bool SpawnedCreatures::nearest(const ExtractedWorld &ew, glm::vec3 from, int &race, glm::vec3 &at, int hostility) const {
    float best = 1e30f;
    for (auto &[zone, list] : zones)
        for (const Creature &cr : list) {
            if (hostility >= 0 && cr.record[worldgen::creature::HOSTILITY] != hostility)
                continue;
            const glm::vec3 p = ew.toLocal(reinterpret_cast<const int64_t *>(&cr.record[worldgen::creature::POSITION]));
            const float d = glm::length(p - from);
            if (d < best)
                best = d, at = p, race = get<int32_t>(cr.record, worldgen::creature::RACE);
        }
    return best < 1e30f;
}

std::vector<std::pair<int, glm::vec3>> SpawnedCreatures::list(const ExtractedWorld &ew, int hostility) const {
    std::vector<std::pair<int, glm::vec3>> out;
    for (auto &[zone, creatures] : zones)
        for (const Creature &cr : creatures)
            if (cr.record[worldgen::creature::HOSTILITY] == hostility)
                out.push_back({get<int32_t>(cr.record, worldgen::creature::RACE),
                               ew.toLocal(reinterpret_cast<const int64_t *>(&cr.record[worldgen::creature::POSITION]))});
    return out;
}

void SpawnedCreatures::simulate(ExtractedWorld &ew, double dtMs) {
    lastWorld = &ew;
    // Gameplay-camera §7.2.3: ordinary frames retain double milliseconds. The host
    // supplies integer timeGetTime differences to its separate world through this same
    // splitter. Both consume the final float residual, including zero, without carry.
    const double ms = gamemath::frameMilliseconds(dtMs);
    nativeupdate::World w;
    w.client = client;
    std::vector<nativeai::Creature *> sides;
    std::map<int64_t, nativeai::Creature *> byId;
    if (hasPlayer) {
        player.side.record = player.record.data();
        byId[localId] = &player.side;
    }
    const bool playerMoves = hasPlayer && nativePlayer; // the world's update moves the player too, first
    if (playerMoves) {
        w.local = player.record.data();
        w.creatures.push_back(player.record.data());
        sides.push_back(&player.side);
    }
    for (auto &[zone, list] : zones)
        for (Creature &cr : list) {
            w.creatures.push_back(cr.record.data());
            cr.side.record = cr.record.data();
            sides.push_back(&cr.side);
            byId[get<int64_t>(cr.record, worldgen::creature::ID)] = &cr.side;
        }
    // Their upkeep (0x2de4c0), as the server's world runs it. The light (0xda830) is not
    // ported: sneaking creatures see the dark as at no light.
    nativeupkeep::World u;
    u.client = client;
    u.local = hasPlayer ? player.record.data() : nullptr;
    u.time = elapsedWorldMs;
    u.day = dayMs;
    u.byId = [&](int64_t id) -> nativeai::Creature * {
        auto f = byId.find(id);
        return f == byId.end() ? nullptr : f->second;
    };
    u.blockAt = [&](int64_t x, int64_t y, int64_t z) {
        const auto b = ew.gameBlock(x >> 16, y >> 16, (int)(z >> 16));
        nativeai::Block o;
        std::memcpy(o.data(), b.data(), o.size());
        return o;
    };
    w.block = [&](int x, int y, int z) { return ew.gameBlock(x, y, z); };
    w.loaded = [&](int x, int y) {
        bool loaded = false;
        ew.gameBlock(x, y, 0, &loaded);
        return loaded;
    };
    w.rand = [this] {
        randState ^= randState << 13, randState ^= randState >> 17, randState ^= randState << 5;
        return (int)(randState & 0x7fff);
    };
    u.rand = w.rand;
    // The physics reads and gives buffs, finds mounts, owners and riders by id, and lays the
    // fire trail's areas among the effect objects.
    std::unordered_map<const uint8_t *, nativeai::Creature *> sideIndex; // (rebuilt as `sides` grows)
    auto sideOf = [&](const uint8_t *record) -> nativeai::Creature * {
        if (sideIndex.size() != sides.size()) {
            sideIndex.clear();
            for (nativeai::Creature *s : sides)
                sideIndex[s->record] = s;
        }
        auto f = sideIndex.find(record);
        return f == sideIndex.end() ? nullptr : f->second;
    };
    w.byId = [&](int64_t id) -> uint8_t * {
        auto f = byId.find(id);
        return f == byId.end() ? nullptr : f->second->record;
    };
    w.buffs = [&](const uint8_t *record) -> const std::vector<nativebuffs::Buff> * {
        nativeai::Creature *s = sideOf(record);
        return s ? &s->buffs : nullptr;
    };
    w.giveBuff = [&](uint8_t *record, const nativebuffs::Buff &buff, bool refresh) {
        if (nativeai::Creature *s = sideOf(record))
            nativebuffs::give(*s, buff, refresh);
    };
    w.effects = &effects;
    // Water freezes the player where the generator's warmth is low.
    w.warmth = [](int x, int y) {
        const worldgen::Place p = worldgen::placeAt(x, y);
        return worldgen::warmth(worldgen::neighbourhood(x, y), p.kind, p.weight);
    };
    // Their abilities (0x2c5250): the world's list with the player first, as the AI's.
    nativeability::World a;
    a.client = client;
    a.local = u.local;
    if (hasPlayer)
        a.creatures.push_back(&player.side);
    a.creatures.insert(a.creatures.end(), sides.begin() + (playerMoves ? 1 : 0), sides.end());
    a.byId = u.byId;
    a.blockAt = u.blockAt;
    a.block = [&](int x, int y, int z) {
        const auto b = ew.gameBlock(x, y, z);
        nativeai::Block o;
        std::memcpy(o.data(), b.data(), o.size());
        return o;
    };
    a.zone = [&](int zx, int zy) {
        bool loaded = false;
        ew.gameBlock((int64_t)zx * 64, (int64_t)zy * 64, 0, &loaded);
        return loaded;
    };
    a.rand = w.rand;
    a.ground = [](int x, int y) { return worldgen::groundHeight(x, y, 0); };
    a.elevation = [](int x, int y) { return worldgen::baseHeightWithPlace(x, y); };
    a.cellHeight = [](int cx, int cy) { return (int)worldgen::cellHeight(cx, cy); };
    // 0x2b77c0 for the blink: the physics' own box test, passages honoured.
    a.overlaps = [&](const int64_t pos[3], const float size[3], bool pass) { return nativeupdate::boxBlocked(w, pos, size, pass, true); };
    // A hit on the player goes to CubeVulkan's player; one on another creature is the
    // game's (0x29eb60 with 0x29dcf0, on the records).
    nativehit::World hw;
    hw.client = client;
    hw.local = u.local;
    hw.creatures = a.creatures;
    hw.byId = u.byId;
    hw.rand = w.rand;
    hw.died = [&](nativeai::Creature *c) {
        if (c == &player.side || client) // (the player's death is CubeVulkan's to handle, main.cpp;
            return;                      // a mirror's creatures die on its host)
        kills++;
        dropFrom(ew, c);
    };
    a.hit = [&](const nativeupkeep::Hit &h) {
        nativeai::Creature *from = u.byId(h.attacker);
        if (hasPlayer && h.target == localId && !nativePlayer)
            return playerHit ? playerHit(h, from ? from->record : nullptr) : false;
        nativeai::Creature *to = u.byId(h.target);
        const bool landed = to ? nativehit::hit(hw, to, from, h) : false;
        strikesLanded += landed && h.attacker == localId;
        return landed;
    };
    u.hit = a.hit;
    a.effects = &effects;
    std::list<nativeeffects::Effect> sentEffects; // world+0xd0: the effect objects made, for the network
    a.sentEffects = &sentEffects;
    // The effect objects (0x2dba40) after every creature's step.
    nativeeffects::World fw;
    fw.client = client;
    fw.local = u.local;
    fw.creatures = a.creatures;
    fw.byId = u.byId;
    fw.blockAt = a.blockAt;
    fw.block = a.block;
    fw.zone = a.zone;
    fw.ground = [](int x, int y) { return worldgen::groundHeight(x, y, 0); };
    fw.rand = w.rand;
    fw.hit = a.hit;
    fw.apply = [&](const std::array<uint8_t, 0x48> &number) { nativehit::apply(hw, number); };
    // What each creature does with its interactions (0x2c3710): seats and doors, and what the
    // special skills and bombs make or do.
    nativeinteract::World iw;
    iw.client = client;
    iw.day = dayCount;
    iw.timeOfDay = dayMs;
    iw.byId = u.byId;
    iw.creatures = a.creatures;
    // Creatures made this frame (shadow clones, water bubbles, bombs) are kept here, where
    // they stay put while the zones' lists are walked, and join their zone after the frame.
    std::list<Creature> born;
    iw.construct = [&](int64_t id) -> nativeai::Creature * {
        born.push_back({worldgen::newCreature(id), {}});
        Creature &made = born.back();
        made.side.record = made.record.data();
        return &made.side;
    };
    std::vector<nativeai::Creature *> *alsoListing[] = {&a.creatures, &hw.creatures, &fw.creatures};
    iw.added = [&](nativeai::Creature *made) { // at the end of every list, so it takes its turns this frame
        w.creatures.push_back(made->record);
        sides.push_back(made);
        for (std::vector<nativeai::Creature *> *list : alsoListing)
            list->push_back(made);
        int64_t id;
        std::memcpy(&id, made->record + worldgen::creature::ID, 8);
        byId[id] = made;
    };
    // A freed resident's thanks (0x2a7ca0) and what it carried (0x2a5b70), a chest's reward
    // and dropped items, as drops (kept with whose they were: none for an object's).
    iw.rand = w.rand;
    iw.rewarded = &rewarded;
    auto idLevel = [](const uint8_t *r, int64_t &id, int32_t &level) {
        id = 0, level = 0;
        if (r)
            std::memcpy(&id, r + worldgen::creature::ID, 8), std::memcpy(&level, r + worldgen::creature::LEVEL, 4);
    };
    iw.reward = [&](const int64_t pos[3], int rarity, const uint8_t *from) {
        nativedrop::World dw = dropWorld(ew);
        nativedrop::reward(dw, pos, rarity);
        int64_t id;
        int32_t level;
        idLevel(from, id, level);
        keepDrops(ew, dw, id, level, true);
    };
    iw.lay = [&](const uint8_t *item, const int64_t pos[3], float angle, int count, nativeai::Creature *by,
                 const uint8_t *from) {
        nativedrop::World dw = dropWorld(ew);
        nativedrop::Place p;
        std::memcpy(p.item.data(), item, p.item.size());
        std::copy(pos, pos + 3, p.pos);
        p.angle = angle, p.count = count;
        nativedrop::place(dw, p, by);
        int64_t id;
        int32_t level;
        idLevel(from, id, level);
        keepDrops(ew, dw, id, level, true);
    };
    iw.objects = [&](int zx, int zy) -> std::vector<nativeai::Object> * {
        nativesaves::ZoneLists *lists = zoneLists(ew, zx, zy);
        return lists ? &lists->staticObjects : nullptr;
    };
    iw.groundItems = [&](int zx, int zy) -> std::vector<nativeinteract::GroundItem> * {
        nativesaves::ZoneLists *lists = zoneLists(ew, zx, zy);
        return lists ? &lists->groundItems : nullptr;
    };
    // The ability step's summons join the world as the interactions' creatures do, and its
    // special items read the same zone objects and the generator's cell objects.
    a.construct = iw.construct;
    a.added = iw.added;
    a.objects = iw.objects;
    a.cellObject = [](int cx, int cy) { return worldgen::cellObject(cx, cy); };
    // Taming renames the creature: its old id joins the dead set, which play never empties (so
    // its spawn point is not made again and prerequisites count it), and the id map moves it
    // to its new id. It keeps its place in the creature lists.
    a.markDead = [&](int64_t id) {
        protectedDead.insert(id);
        enemyDeaths.forget(id);
        if (worldDead.insert(id).second)
            markedDead.push_back(id);
    };
    a.renumbered = [&](int64_t oldId, nativeai::Creature *c) {
        byId.erase(oldId);
        int64_t id;
        std::memcpy(&id, c->record + worldgen::creature::ID, 8);
        byId[id] = c;
    };
    // Taming makes a creature a pet (0x2b67b0): its race's looks and gear, its owner.
    a.adopt = [&](int64_t owner, nativeai::Creature *pet) {
        const worldgen::CreatureExtras extras = worldgen::makePet(pet->record, owner, worldgen::creatureTables(), [&](int64_t id) {
            nativeai::Creature *c = u.byId(id);
            return c ? c->record : nullptr;
        });
        pet->skills = extras.abilities;
        pet->summonRaces = extras.abilities2;
        pet->inventory = worldgen::inventoryRecords(extras.inventory);
    };
    // What a chest of a near zone holds (zone+0x450), by its index in the zone's objects.
    iw.contents = [&](int zx, int zy, int index) -> const uint8_t * {
        const auto zone = zoneResidents.find({zx, zy});
        if (zone == zoneResidents.end())
            return nullptr;
        auto [chests, fresh] = zoneChests.try_emplace({zx, zy});
        if (fresh)
            for (const auto &[i, item] : zone->second->rewards)
                worldgen::writeItem(chests->second[i].data(), item);
        const auto held = chests->second.find(index);
        return held == chests->second.end() ? nullptr : held->second.data();
    };
    // The sound requests of a step join the world's list, to be played by Audio; every list a
    // step makes joins the world's event lists (world+0x90 ..), for the network.
    auto keepSounds = [&](std::vector<std::array<uint8_t, 0x30>> &made) {
        sounds.insert(sounds.end(), made.begin(), made.end());
        made.clear();
    };
    auto keep = [&](auto &made) {
        keepRecords(made.sounds, made.particles, made.numbers, made.buffNotes);
        made.particles.clear(), made.numbers.clear(), made.buffNotes.clear();
        keepSounds(made.sounds);
    };
    nativepet::World pets;
    pets.client = client;
    pets.local = hasPlayer ? &player.side : nullptr;
    pets.construct = iw.construct;
    pets.added = [&](nativeai::Creature *made) {
        iw.creatures.push_back(made);
        iw.added(made);
    };
    pets.setup = a.adopt;
    auto step = [&](float dt) {
        elapsedWorldMs += dt; // native world time advances before the near-player pass
        u.time = elapsedWorldMs;
        std::vector<nativesaves::Cell> playerZones;
        auto includePlayer = [&](const uint8_t *record) {
            if (record[0x60] == 0) {
                int64_t position[2];
                std::memcpy(position, record + worldgen::creature::POSITION, sizeof position);
                playerZones.push_back(zonestates::zoneOfBlock(position[0] >> 16, position[1] >> 16));
            }
        };
        for (const uint8_t *record : w.creatures)
            includePlayer(record);
        if (hasPlayer && !playerMoves)
            includePlayer(player.record.data());
        const std::set<nativesaves::Cell> noUsedCells;
        zoneStates.advanceObjects(dt, playerZones, hasPlayer ? (usedCells ? usedCells : &noUsedCells) : nullptr);
        pets.creatures = sides;
        nativepet::step(pets);
        worldEvents.buffs.insert(worldEvents.buffs.end(), pets.buffNotes.begin(), pets.buffNotes.end());
        pets.buffNotes.clear();
        for (size_t i = 0; i < w.creatures.size(); i++) {
            uint8_t *c = w.creatures[i];
            float health;
            int64_t at[2];
            std::memcpy(&health, c + 0x180, 4);
            std::memcpy(at, c + worldgen::creature::POSITION, sizeof at);
            // Creatures whose ground is not in memory wait, as the game's do outside its loaded zones.
            if (!w.loaded((int)(at[0] >> 16), (int)(at[1] >> 16)))
                continue;
            if (health > 0.0f) {
                nativeupdate::update(w, c, dt);
                nativeability::ability(a, sides[i], dt);
                nativeinteract::step(iw, sides[i]);
            }
            nativeupkeep::upkeep(u, sides[i], dt); // (poison's ticks land inside it, through u.hit)
            u.hits.clear();
        }
        if (hasPlayer) {
            const int64_t x = get<int64_t>(player.record, worldgen::creature::POSITION);
            const int64_t y = get<int64_t>(player.record, worldgen::creature::POSITION + 8);
            const int64_t z = get<int64_t>(player.record, worldgen::creature::POSITION + 16);
            const auto block = ew.gameBlock(x >> 16, y >> 16, (int)(z >> 16));
            const auto region = worldgen::regionAtFixed(x, y, (block[5] & 0x20) != 0);
            put<int32_t>(player.record, worldgen::creature::REGION, region.x);
            put<int32_t>(player.record, worldgen::creature::REGION + 4, region.y);
            regionHistory.observe({region.x, region.y}, get<uint8_t>(player.record, worldgen::creature::ABILITY));
        }
        // What the physics and the effect objects made.
        for (const nativeeffects::Effect &e : w.sentEffects)
            worldEvents.effects.push_back(e);
        w.sentEffects.clear();
        keep(w);
        nativeeffects::step(fw, effects, dt);
        keep(fw);
        // The client side of taming (0xa0750, loot-pickup.md §5.2): a tame's damage number to
        // the local player fills its pet slot; the inventory part is the caller's (takeTames).
        if (hasPlayer)
            for (const auto &number : a.numbers) {
                const grounditems::Tamed t = grounditems::tame(number.data(), player.record.data(), [&](int64_t id) -> const uint8_t * {
                    nativeai::Creature *c = a.byId(id);
                    return c ? c->record : nullptr;
                });
                if (t.happened)
                    tames.push_back(t);
            }
        // What the abilities, the upkeep and the hits made.
        keep(a);
        for (const nativeeffects::Effect &e : sentEffects)
            worldEvents.effects.push_back(e);
        sentEffects.clear();
        a.hits.clear();
        keepRecords(u.sounds, {}, u.numbers, u.buffNotes);
        keepSounds(u.sounds);
        u.numbers.clear(), u.buffNotes.clear(), u.hits.clear();
        keep(hw);
        worldEvents.aggro.insert(worldEvents.aggro.end(), hw.aggroNotes.begin(), hw.aggroNotes.end());
        hw.aggroNotes.clear();
        for (const auto &[credited, dead] : hw.kills) { // (a kill credit: who is credited, then the dead)
            cwnet::KillRecord k;
            std::memcpy(k.data(), &credited, 8);
            std::memcpy(k.data() + 8, &dead, 8);
            worldEvents.kills.push_back(k);
        }
        hw.kills.clear();
        for (const auto &sound : iw.sounds) { // a door's swing, the heroic shout or a water bubble
            int32_t id;
            std::memcpy(&id, sound.data() + 0x18, 4);
            if (id == nativeinteract::HEROIC_SHOUT_SOUND)
                shouts++;
            else if (id != nativeinteract::BUBBLE_SOUND)
                swings++;
        }
        objectChanges += iw.changedZones.size();
        for (const auto &[zx, zy] : iw.changedZones) {
            zoneStates.objectsChanged({zx, zy});
            objectsChanged.insert({zx, zy});
        }
        for (const auto &[zx, zy] : iw.changedGroundZones) {
            zoneStates.groundItemsChanged({zx, zy});
            groundChanged.insert({zx, zy});
        }
        // The blocks the abilities and effects broke (+0xc0) join their zones' change lists.
        for (auto *changes : {&a.blockChanges, &fw.blockChanges})
            for (const auto &change : *changes) {
                nativesaves::BlockChange kept;
                std::memcpy(kept.bytes.data(), change.data(), kept.bytes.size());
                zoneStates.addBlockChange(kept);
            }
        keepBlockChanges(a.blockChanges);
        keepBlockChanges(fw.blockChanges);
        if (iw.day != dayCount) { // a night at the inn: the next day, at 7:00
            innNights += iw.day - dayCount;
            dayCount = iw.day;
            dayMs = iw.timeOfDay;
        }
        worldEvents.sounds.insert(worldEvents.sounds.end(), iw.sounds.begin(), iw.sounds.end());
        keepSounds(iw.sounds);
        worldEvents.buffs.insert(worldEvents.buffs.end(), iw.buffNotes.begin(), iw.buffNotes.end());
        worldEvents.pickups.insert(worldEvents.pickups.end(), iw.pickUps.begin(), iw.pickUps.end());
        worldEvents.sleeps.insert(worldEvents.sleeps.end(), iw.sleepNotices.begin(), iw.sleepNotices.end());
        a.blockChanges.clear(), fw.blockChanges.clear();
        iw.changedZones.clear(), iw.buffNotes.clear();
        iw.sleepNotices.clear(), iw.pickUps.clear(), iw.changedGroundZones.clear();
    };
    double left = ms;
    while (left > 16) {
        step(16.0f);
        left -= 16;
    }
    step((float)left);
    settle(born);
}

void SpawnedCreatures::settle(std::list<Creature> &born) {
    for (Creature &made : born) {
        const int64_t *at = reinterpret_cast<const int64_t *>(&made.record[worldgen::creature::POSITION]);
        const std::pair<int, int> zone{(int)std::floor((double)(at[0] >> 16) / 64.0), (int)std::floor((double)(at[1] >> 16) / 64.0)};
        seenAlive.insert(get<int64_t>(made.record, worldgen::creature::ID));
        summoned++;
        zones[zone].push_back(std::move(made));
    }
    born.clear();
}

// --- Road travel ([Creatures] RoadTravel; not the game's) ----------------------------------

namespace {
bool onRoad(double x, double y) { return worldgen::columnValues((int32_t)std::floor(x), (int32_t)std::floor(y)).roads > 0.5f; }
constexpr double STEP = 12; // blocks between waypoints
// The next point along the road from (x, y) going `heading`: the straightest of a few
// turns that is still on the road (and, with `open`, where it may go), else back the way
// it came.
template <class Open> bool nextOnRoad(double &x, double &y, float &heading, Open open) {
    static const float turns[] = {0.0f, 0.35f, -0.35f, 0.7f, -0.7f, 1.05f, -1.05f, 1.4f, -1.4f, 1.75f, -1.75f, 2.1f, -2.1f};
    for (float back : {0.0f, 3.14159265f})
        for (float t : turns) {
            const float h = heading + back + t;
            const double nx = x + std::cos(h) * STEP, ny = y + std::sin(h) * STEP;
            if (onRoad(nx, ny) && open(nx, ny)) {
                x = nx, y = ny, heading = h;
                return true;
            }
        }
    return false;
}
} // namespace

void SpawnedCreatures::startTrips(std::vector<Creature> &list) {
    // Packs: wild creatures (+0xbcc 0x10 or 0x30) made one after another within 8 blocks of
    // the first, of the same hostility; travellers already know each other (+0xc08).
    for (size_t i = 0; i < list.size();) {
        worldgen::CreatureRecord &first = list[i].record;
        const uint32_t roam = get<uint32_t>(first, 0xbcc);
        const uint8_t kind = first[worldgen::creature::HOSTILITY];
        size_t end = i + 1;
        if ((roam == 0x10 || roam == 0x30) && (kind == 1 || kind == 2 || kind == 3) && list[i].side.group.empty())
            while (end < list.size()) {
                const worldgen::CreatureRecord &o = list[end].record;
                const double dx = (double)(get<int64_t>(o, worldgen::creature::HOME) - get<int64_t>(first, worldgen::creature::HOME)) / 65536,
                             dy = (double)(get<int64_t>(o, worldgen::creature::HOME + 8) - get<int64_t>(first, worldgen::creature::HOME + 8)) / 65536;
                if (get<uint32_t>(o, 0xbcc) != roam || o[worldgen::creature::HOSTILITY] != kind || dx * dx + dy * dy > 64 ||
                    !list[end].side.group.empty())
                    break;
                end++;
            }
        if (end - i >= 2) {
            std::set<int64_t> pack;
            for (size_t k = i; k < end; k++)
                pack.insert(get<int64_t>(list[k].record, worldgen::creature::ID));
            for (size_t k = i; k < end; k++)
                list[k].side.group = pack;
        }
        i = end;
    }
    // A trip for each group whose home is near a road.
    for (Creature &cr : list) {
        if (cr.side.group.size() < 2 || trips.count(*cr.side.group.begin()) ||
            get<int64_t>(cr.record, worldgen::creature::ID) != *cr.side.group.begin())
            continue;
        const double hx = (double)get<int64_t>(cr.record, worldgen::creature::HOME) / 65536,
                     hy = (double)get<int64_t>(cr.record, worldgen::creature::HOME + 8) / 65536;
        bool found = false;
        RoadTrip trip;
        for (int r = 0; r <= 96 && !found; r += 4)
            for (int a = 0; a < 16 && !found; a++) {
                const double px = hx + std::cos(a * 0.3927) * r, py = hy + std::sin(a * 0.3927) * r;
                if (onRoad(px, py))
                    trip.x = px, trip.y = py, found = true;
            }
        if (!found)
            continue;
        const int start = (int)(get<int64_t>(cr.record, worldgen::creature::ID) & 7);
        for (int k = 0; k < 8; k++) {
            trip.heading = (float)((start + k) % 8) * 0.785398f;
            double x = trip.x, y = trip.y;
            float h = trip.heading;
            if (nextOnRoad(x, y, h, [](double, double) { return true; }) && std::fabs(h - trip.heading) < 1.5f)
                break;
        }
        trips[*cr.side.group.begin()] = trip;
    }
}

void SpawnedCreatures::advanceTrips(ExtractedWorld &ew, double dtMs) {
    std::map<int64_t, worldgen::CreatureRecord *> byId;
    std::map<int64_t, const std::set<int64_t> *> groupOf;
    for (auto &[zone, list] : zones)
        for (Creature &cr : list) {
            const int64_t id = get<int64_t>(cr.record, worldgen::creature::ID);
            byId[id] = &cr.record;
            if (!cr.side.group.empty())
                groupOf[*cr.side.group.begin()] = &cr.side.group;
        }
    for (auto it = trips.begin(); it != trips.end();) {
        auto g = groupOf.find(it->first);
        if (g == groupOf.end()) { // its zone was let go
            it = trips.erase(it);
            continue;
        }
        // The pack follows its first living member (0x103f0), so that one leads.
        worldgen::CreatureRecord *leader = nullptr;
        for (int64_t id : *g->second)
            if (auto f = byId.find(id); f != byId.end() && get<float>(*f->second, 0x180) > 0.0f) {
                leader = f->second;
                break;
            }
        RoadTrip &t = it->second;
        ++it;
        if (!leader)
            continue;
        const double lx = (double)get<int64_t>(*leader, worldgen::creature::POSITION) / 65536,
                     ly = (double)get<int64_t>(*leader, worldgen::creature::POSITION + 8) / 65536;
        // Only into zones in memory: at the edge of what is loaded the pack turns back.
        auto inMemory = [&](double x, double y) {
            bool in = false;
            ew.gameBlock((int64_t)std::floor(x), (int64_t)std::floor(y), 0, &in);
            return in;
        };
        // On when the leader is near the waypoint, or after 20 s without reaching it (a wall,
        // a river, a house in the way).
        t.waitedMs += dtMs;
        if ((lx - t.x) * (lx - t.x) + (ly - t.y) * (ly - t.y) < 36.0 || t.waitedMs > 20000.0) {
            if (!nextOnRoad(t.x, t.y, t.heading, inMemory))
                continue;
            t.waitedMs = 0;
        }
        // Home on the ground at the waypoint (the top of its column, as spawn points stand).
        const int64_t bx = (int64_t)std::floor(t.x), by = (int64_t)std::floor(t.y);
        int z = (int)std::floor(worldgen::groundHeight((int32_t)bx, (int32_t)by)) + 4;
        bool loaded = false;
        ew.gameBlock(bx, by, z, &loaded);
        if (loaded)
            while (z > -64 && (ew.gameBlock(bx, by, z - 1)[4] & 0xfd) == 0)
                z--;
        put<int64_t>(*leader, worldgen::creature::HOME, (int64_t)(t.x * 65536));
        put<int64_t>(*leader, worldgen::creature::HOME + 8, (int64_t)(t.y * 65536));
        put<int64_t>(*leader, worldgen::creature::HOME + 16, (int64_t)z << 16);
        put<uint32_t>(*leader, 0xbcc, get<uint32_t>(*leader, 0xbcc) | 2);
    }
}

void SpawnedCreatures::setPlayer(const ExtractedWorld &ew, glm::vec3 local, float headingRad, glm::vec3 velocity,
                                 float health, int level, bool onGround) {
    worldgen::CreatureRecord &r = player.record;
    if (!hasPlayer) {
        worldgen::SpawnPoint sp; // the constructor's defaults, as the game makes a player
        sp.id = localId;
        auto zero = [] { return 0; };
        r = worldgen::makeResident(sp, worldgen::Draw(zero), false, [](const uint8_t *record) { return nativerules::maxHealth(record); }).record;
        put<uint8_t>(r, worldgen::creature::HOSTILITY, 0);
        put<int32_t>(r, worldgen::creature::RACE, 2);
        put<uint32_t>(r, worldgen::creature::FLAGS, 0);
        // A human's size (the game's for the TEST character: 0.96 by 0.96 by 2.16 blocks).
        put<float>(r, 0x84, 0.96f), put<float>(r, 0x88, 0.96f), put<float>(r, 0x8c, 2.16f);
        put<uint32_t>(r, 0xbcc, 0);
        hasPlayer = true;
        if (!gearRecord.empty())
            setPlayerGear(gearRecord);
    }
    put<int64_t>(r, worldgen::creature::ID, localId);
    put<int32_t>(r, worldgen::creature::LEVEL, level);
    if (!nativePlayer || !placed) {
        // The startup bridge can supply legacy/survey health. Establish level and
        // equipment first, then respect the native maximum (upkeep spec step 1b).
        // Later native frames retain the record's health, including its wounds.
        const float maximum = playerMaxHealth();
        put<float>(r, 0x180, nativePlayer && maximum > 0 ? std::min(health, maximum) : health);
    }
    // The native player moves itself; it is placed only at first and when CubeVulkan puts
    // it elsewhere (half a block or more from where its record is). Its record stands a
    // block above where CubeVulkan has its feet, as every creature's does (NATIVE_FEET).
    glm::vec3 game = local;
    if (nativePlayer)
        game.z += NATIVE_FEET;
    if (nativePlayer && placed &&
        glm::length(game - ew.toLocal(reinterpret_cast<const int64_t *>(&r[worldgen::creature::POSITION]))) < 0.5f)
        return;
    placed = true;
    const int64_t at[3] = {(int64_t)std::llround((double)game.x * 65536) + ew.originDotsX(),
                           (int64_t)std::llround((double)game.y * 65536) + ew.originDotsY(),
                           (int64_t)std::llround(((double)game.z - ExtractedWorld::Z_OFFSET) * 65536)};
    std::memcpy(&r[worldgen::creature::POSITION], at, sizeof at);
    std::memcpy(&r[0xb10], at, sizeof at); // where it is drawn (what the AI aims at)
    put<float>(r, worldgen::creature::YAW, headingRad * 57.2957795f);
    if (nativePlayer)
        return;
    put<float>(r, 0x40, velocity.x), put<float>(r, 0x44, velocity.y), put<float>(r, 0x48, velocity.z);
    put<uint32_t>(r, 0x5c, (get<uint32_t>(r, 0x5c) & ~1u) | (onGround ? 1u : 0u));
}

nativecontrol::World SpawnedCreatures::controlWorld() {
    nativecontrol::World world;
    world.byId = [this](int64_t id) -> const uint8_t * {
        const Creature *creature = find(id);
        return creature ? creature->record.data() : nullptr;
    };
    world.hasBuff = [this](int type) {
        for (const auto &entry : player.side.buffs)
            if (entry[0] == (uint8_t)type)
                return true;
        return false;
    };
    world.ownsSpecialItem = controlOwnsSpecialItem;
    world.playSound = controlPlaySound;
    return world;
}

void SpawnedCreatures::noteControlMove() {
    if (player.record[0x68] != lastPlayerMove && player.record[0x68])
        movesStarted++;
    lastPlayerMove = player.record[0x68];
}

bool SpawnedCreatures::prepareControl(const nativecontrol::Input &in, double dtMs) {
    if (!nativePlayer || !hasPlayer)
        return false;
    // An ability needing a special item ends without it, as in the existing bridge.
    worldgen::CreatureRecord &record = player.record;
    if ((record[0x68] == 0x78 && (get<uint32_t>(record, 0x5c) & 1) && !hasGlider) || (record[0x68] == 0x79 && !hasBoat))
        record[0x68] = 0;
    player.side.record = record.data();
    auto world = controlWorld();
    const bool admitted = nativecontrol::beginFrame(in, controlState, player.side, dtMs, world);
    worldEvents.buffs.insert(worldEvents.buffs.end(), world.buffNotes.begin(), world.buffNotes.end());
    noteControlMove();
    return admitted;
}

void SpawnedCreatures::controlOwnerLook() {
    if (!nativePlayer || !hasPlayer)
        return;
    auto world = controlWorld();
    nativecontrol::ownerLook(player.side, world);
    noteControlMove();
}

void SpawnedCreatures::controlPet(const nativecontrol::Input &in) {
    if (!nativePlayer || !hasPlayer)
        return;
    auto world = controlWorld();
    nativecontrol::pet(in, controlState, player.side, world);
    noteControlMove();
}

void SpawnedCreatures::finishControl(const nativecontrol::Input &in) {
    if (!nativePlayer || !hasPlayer)
        return;
    nativecontrol::attack(in, controlState, player.side);
    noteControlMove();
}

void SpawnedCreatures::control(const nativecontrol::Input &in, double dtMs) {
    if (!prepareControl(in, dtMs))
        return;
    controlOwnerLook();
    controlPet(in);
    finishControl(in);
}

float SpawnedCreatures::playerMana() const { return hasPlayer ? get<float>(player.record, 0x188) : 0.0f; }
uint32_t SpawnedCreatures::playerToggles() const { return hasPlayer ? get<uint16_t>(player.record, 0x128) : 0u; }

float SpawnedCreatures::playerStamina() const { return hasPlayer ? get<float>(player.record, 0x9a0) : 0.0f; }

float SpawnedCreatures::playerHealth() const { return hasPlayer ? get<float>(player.record, worldgen::creature::HEALTH) : 0.0f; }

float SpawnedCreatures::playerMaxHealth() const {
    if (!hasPlayer)
        return 0.0f;
    std::vector<nativerules::Buff> buffs;
    for (auto &b : player.side.buffs) {
        float v;
        std::memcpy(&v, b.data() + 4, 4);
        buffs.push_back({b[0], v});
    }
    return nativerules::maxHealth(player.record.data(), buffs.data(), buffs.size());
}

void SpawnedCreatures::setPlayerHealth(float hp) {
    if (hasPlayer)
        put<float>(player.record, worldgen::creature::HEALTH, hp);
}

nativeai::Creature *SpawnedCreatures::localPlayer() {
    if (!nativePlayer || !hasPlayer)
        return nullptr;
    player.side.record = player.record.data();
    return &player.side;
}

float SpawnedCreatures::playerAbilityDuration() const {
    if (!hasPlayer)
        return 0.0f;
    const nativerules::HasBuff has = [&](int type) {
        for (const auto &b : player.side.buffs)
            if (b[0] == (uint8_t)type)
                return true;
        return false;
    };
    return (float)nativerules::abilityDuration(player.record.data(), has);
}

const std::vector<nativeai::Object> *SpawnedCreatures::zoneObjects(int zx, int zy) const {
    const nativesaves::ZoneLists *lists = zoneStates.find({zx, zy});
    return lists ? &lists->staticObjects : nullptr;
}

std::optional<objectvisual::State> SpawnedCreatures::objectVisual(int zx, int zy, const objectvisual::Key &key) const {
    const auto *objects = zoneObjects(zx, zy);
    if (!objects)
        return std::nullopt;
    auto matches = [&](const nativeai::Object &object) {
        int type;
        std::array<int64_t, 3> position;
        std::memcpy(&type, object.data(), 4);
        std::memcpy(position.data(), object.data() + 8, sizeof position);
        return type == key.type && position == key.position;
    };
    // Captured and regenerated lists can differ after daily refresh. Never animate
    // an unrelated object merely because it occupies the same array slot.
    if (key.index >= 0 && (size_t)key.index < objects->size() && matches((*objects)[key.index]))
        return objectvisual::read((*objects)[key.index].data());
    for (const auto &object : *objects)
        if (matches(object))
            return objectvisual::read(object.data());
    return std::nullopt;
}

void SpawnedCreatures::setStoredZones(ExtractedWorld &ew, nativesaves::WorldState *world, bool serverMode) {
    storedZones = world;
    if (world && respawnSettings.mode != enemyrespawn::Mode::Permanent)
        for (auto &[zone, state] : world->zones)
            state.deadTracked = 0; // temporary deaths from an earlier instance have ended
    zoneStates = zonestates::ZoneStates(world, serverMode, [&ew](nativesaves::Cell zone, const std::vector<nativesaves::BlockChange> &changes) {
        ew.setBlockChanges(zone[0], zone[1], changes);
    });
}

nativesaves::ZoneLists *SpawnedCreatures::zoneLists(ExtractedWorld &ew, int zx, int zy) {
    if (nativesaves::ZoneLists *lists = zoneStates.find({zx, zy}))
        return lists;
    std::vector<worldgen::ZoneObject> objects;
    if (!ew.gameObjects(zx, zy, objects))
        return nullptr; // (its blocks are not in memory yet)
    std::vector<nativesaves::Record> objectRecords, itemRecords;
    for (const worldgen::ZoneObject &o : objects)
        objectRecords.push_back(zonestates::staticObjectRecord(o));
    for (const worldgen::GroundItem &g : worldgen::zone(zx, zy)->items)
        itemRecords.push_back(zonestates::groundItemRecord(g, worldgen::worldDay()));
    // A mirror's new zone shows its own generation until the host has sent the zone's state.
    // (Which routine flags a mirror's new zones is netcode-compatibility.md's open question 7;
    // every new zone is flagged here, which the recorded zone requests bear out.)
    if (client)
        stateNeeded.insert({zx, zy});
    return &zoneStates.load({zx, zy}, std::move(itemRecords), std::move(objectRecords));
}

std::optional<int32_t> SpawnedCreatures::deadTrackedIn(std::pair<int, int> zone) const {
    auto it = zoneTracked.find(zone);
    if (it == zoneTracked.end())
        return std::nullopt;
    int32_t count = 0;
    for (int64_t id : it->second)
        count += worldDead.count(id) != 0;
    return count;
}

const std::vector<nativesaves::Record> *SpawnedCreatures::groundItems(ExtractedWorld &ew, int zx, int zy) {
    nativesaves::ZoneLists *lists = zoneLists(ew, zx, zy);
    return lists ? &lists->groundItems : nullptr;
}

const nativesaves::Record *SpawnedCreatures::findGroundItem(const std::array<int32_t, 3> &key) const {
    const auto *lists = zoneStates.find({key[0], key[1]});
    if (!lists || key[2] < 0 || (size_t)key[2] >= lists->groundItems.size())
        return nullptr;
    return &lists->groundItems[(size_t)key[2]];
}

void SpawnedCreatures::takeGroundItem(int zx, int zy, size_t index) {
    nativesaves::ZoneLists *lists = zoneStates.find({zx, zy});
    if (!lists || index >= lists->groundItems.size())
        return;
    nativeinteract::emptyGroundItem(lists->groundItems[index]);
    zoneStates.groundItemsChanged({zx, zy});
}

void SpawnedCreatures::setObjectActive(int zx, int zy, size_t index, bool active) {
    nativesaves::ZoneLists *lists = zoneStates.find({zx, zy});
    if (!lists || index >= lists->staticObjects.size())
        return;
    if (nativeinteract::openOrShut(lists->staticObjects[index], active ? 1 : 0, worldEvents.sounds)) {
        zoneStates.objectsChanged({zx, zy});
        objectsChanged.insert({zx, zy});
    }
}

void SpawnedCreatures::changeBlock(ExtractedWorld &ew, int32_t x, int32_t y, int32_t z, const std::array<uint8_t, 6> &block) {
    const ExtractedWorld::GameBlock was = ew.gameBlock(x, y, z);
    zoneStates.addBlockChange(zonestates::blockChange(x, y, z, was, block));
}

void SpawnedCreatures::recordAllZones() {
    for (const nativesaves::Cell &zone : zoneStates.zonesInMemory()) {
        zoneStates.unload(zone, deadTrackedIn({zone[0], zone[1]}));
        unloadedZones.push_back({zone[0], zone[1]});
    }
}

SpawnedCreatures::CastBar SpawnedCreatures::castBar() const {
    CastBar bar;
    if (!nativePlayer || !hasPlayer)
        return bar;
    const worldgen::CreatureRecord &r = player.record;
    const nativerules::HasBuff has = [&](int t) {
        for (const auto &b : player.side.buffs)
            if (b[0] == (uint8_t)t)
                return true;
        return false;
    };
    static const std::set<int> casts = {150, 151, 117, 133, 144, 48, 46, 39, 40, 41, 38, 47, 136, 98, 105, 104, 106,
                                        107, 96, 101, 45, 42, 43, 44, 31, 32, 33, 34, 35, 134, 135, 97, 51};
    const int a = r[0x68];
    const float t = get<float>(r, 0x6c);
    const float anticipation = (float)nativerules::abilityEffectTime(r.data(), -1, has);
    const float strike = (float)nativerules::abilityMiddleTime(r.data(), -1, has);
    const float stealth = get<float>(r, 0x18c), block = get<float>(r, 0x184);
    if (!((casts.count(a) && t <= anticipation) || stealth > 0.0f || block < 1.0f ||
          ((a == 95 || a == 140) && t >= anticipation && t < anticipation + strike)))
        return bar;
    bar.shown = true;
    const bool caster = r[0x14c] == 3; // a mage (the caster check's classes; the others are monsters')
    if (!caster && stealth > 0.0f)
        bar.fraction = stealth, bar.label = "Stealth";
    else if (a == 140)
        bar.fraction = std::max(0.0f, (t - anticipation) / strike), bar.label = "Assassinate";
    else if (!(block < 1.0f)) // (and not blocking, which a cast is not)
        bar.fraction = t / anticipation, bar.ability = a;
    else
        bar.fraction = std::max(0.0f, block), bar.label = "Block Power";
    return bar;
}

SpawnedCreatures::Sheet SpawnedCreatures::sheet() const {
    Sheet s;
    if (!nativePlayer || !hasPlayer)
        return s;
    const worldgen::CreatureRecord &r = player.record;
    const uint8_t *c = r.data();
    const int32_t *region = reinterpret_cast<const int32_t *>(&r[worldgen::creature::REGION]);
    // Equipment (+0x278, 0xa0 each): head, neck, chest, feet, hands, shoulder, the weapons, the rings, the pet.
    auto slot = [&](int i) { return c + worldgen::creature::EQUIPMENT + (size_t)i * 0xa0; };
    auto twoHanded = [](const uint8_t *item) {
        uint32_t id;
        std::memcpy(&id, item + 4, 4);
        return item[0] == 3 && id <= 0x17 && ((0x878de0u >> id) & 1);
    };
    auto rarity = [&](int i, uint8_t category) {
        return slot(i)[0] == category ? (float)nativerules::itemLevel(slot(i), region) : 0.0f;
    };
    s.weaponLevel = rarity(6, 3) + rarity(7, 3);
    if (!(twoHanded(slot(6)) || twoHanded(slot(7))))
        s.weaponLevel *= 0.5f;
    s.armorLevel = (rarity(5, 7) + 2.0f * rarity(2, 4) + rarity(4, 5) + rarity(3, 6)) / 5.0f;
    std::vector<nativerules::Buff> buffs;
    for (auto &b : player.side.buffs) {
        float v;
        std::memcpy(&v, b.data() + 4, 4);
        buffs.push_back({b[0], v});
    }
    s.resistance = nativerules::resistance(c, buffs.data(), buffs.size());
    for (int i = 1; i <= 9; i++) { // (the slots' own categories; the head and the pet add nothing)
        s.tempo += nativerules::itemHaste(slot(i), c);
        s.regeneration += nativerules::itemStaminaRegen(slot(i), c);
    }
    s.valid = true;
    return s;
}

int SpawnedCreatures::specialSkill(float &cooldownMs) const {
    cooldownMs = 0;
    if (!nativePlayer || !hasPlayer)
        return 0;
    const int skill = nativecontrol::specialSkill(player.record.data());
    if (const auto it = player.side.cooldowns.find(skill); it != player.side.cooldowns.end())
        cooldownMs = it->second;
    return skill;
}

void SpawnedCreatures::setPlayerArtifacts(const std::array<int32_t, 7> &skills) {
    if (!hasPlayer)
        return;
    for (size_t k = 0; k < skills.size(); k++)
        put<int32_t>(player.record, 0xb5c + 4 * k, skills[k]);
}

void SpawnedCreatures::activate(int64_t id) {
    if (!hasPlayer)
        return;
    nativeai::Interaction in = nativeinteract::fresh(nativeinteract::ACTIVATE);
    std::memcpy(&in[nativeinteract::TARGET], &id, 8);
    player.side.interactions.push_back(in);
}

bool SpawnedCreatures::useItem(const uint8_t *item) {
    worldgen::CreatureRecord &r = player.record;
    if (!nativePlayer || !hasPlayer || !(get<float>(r, 0x130) <= 0.0f))
        return false;
    const uint8_t category = item[0];
    int32_t id;
    std::memcpy(&id, item + 4, 4);
    if (category != 1 && category != 0x1c) // consumables and elixirs
        return false;
    if (category == 1 && id == 9) { // a bomb: the world makes it where the player stands (kind 1)
        nativeai::Interaction in = nativeinteract::fresh(nativeinteract::USE_ITEM);
        std::memcpy(&in[nativeinteract::ITEM], item, 0xa0);
        player.side.interactions.push_back(in);
        return true; // (one is used up)
    }
    if (category == 1 && id == 13) // a portal stone (not ported)
        return false;
    std::memcpy(&r[0x1d8], item, 0xa0);
    const bool drink = category == 0x1c || (id >= 1 && id <= 3) || id == 12; // potions
    r[0x68] = drink ? 0x54 : 0x55;
    put<float>(r, 0x6c, 0.0f);
    put<float>(r, 0x12c, 0.0f);
    return true;
}

int SpawnedCreatures::playerAbility(float &ms) const {
    ms = 0;
    if (!hasPlayer)
        return 0;
    ms = get<float>(player.record, 0x6c);
    return player.record[0x68];
}

std::vector<int64_t> SpawnedCreatures::missionIds(int zx, int zy) const {
    if (auto it = zoneMissions.find({zx, zy}); it != zoneMissions.end())
        return it->second;
    // A mirror makes no residents: the zone's mission creatures are its generator's all the same.
    if (client && worldgen::creatureTablesLoaded())
        return worldgen::zone(zx, zy)->spawns.mission;
    return {};
}

void SpawnedCreatures::setPlayerToggle(uint16_t bit, bool on) {
    if (!hasPlayer)
        return;
    const uint16_t t = get<uint16_t>(player.record, 0x128);
    put<uint16_t>(player.record, 0x128, on ? (uint16_t)(t | bit) : (uint16_t)(t & ~bit));
}

bool SpawnedCreatures::playerBuff(int type, float &strength, float &ms) const {
    for (const auto &b : player.side.buffs)
        if (b[0] == (uint8_t)type) {
            std::memcpy(&strength, b.data() + 4, 4);
            std::memcpy(&ms, b.data() + 8, 4);
            return true;
        }
    return false;
}

void SpawnedCreatures::givePlayerBuff(int type, float strength, float ms) {
    if (!hasPlayer)
        return;
    endPlayerBuff(type);
    std::array<uint8_t, 0x18> b{};
    b[0] = (uint8_t)type;
    std::memcpy(b.data() + 4, &strength, 4);
    std::memcpy(b.data() + 8, &ms, 4);
    const int64_t self = get<int64_t>(player.record, worldgen::creature::ID);
    std::memcpy(b.data() + 0x10, &self, 8);
    player.side.buffs.push_back(b);
}

void SpawnedCreatures::endPlayerBuff(int type) {
    auto &buffs = player.side.buffs;
    for (auto it = buffs.begin(); it != buffs.end(); ++it)
        if ((*it)[0] == (uint8_t)type) {
            buffs.erase(it);
            return;
        }
}

bool SpawnedCreatures::startFlight(int32_t x, int32_t y) {
    if (!nativePlayer || !hasPlayer)
        return false;
    put<int32_t>(player.record, 0x1b0, x);
    put<int32_t>(player.record, 0x1b4, y);
    std::memcpy(&player.record[0x1b8], &player.record[worldgen::creature::POSITION], 24);
    player.record[0x68] = 0x9c;
    put<float>(player.record, 0x6c, 0.0f);
    return true;
}

bool SpawnedCreatures::startShrineReturn(int32_t cellX, int32_t cellY) {
    if (!nativePlayer || !hasPlayer)
        return false;
    put<int32_t>(player.record, 0x1d0, cellX);
    put<int32_t>(player.record, 0x1d4, cellY);
    player.record[0x68] = 0x75;
    put<float>(player.record, 0x6c, 0.0f);
    return true;
}

void SpawnedCreatures::setPlayerGear(const worldgen::CreatureRecord &from, bool clampHealth) {
    gearRecord = from;
    if (!hasPlayer || from.empty())
        return;
    worldgen::CreatureRecord &r = player.record;
    put<int32_t>(r, worldgen::creature::RACE, get<int32_t>(from, worldgen::creature::RACE));
    put<int32_t>(r, worldgen::creature::LEVEL, get<int32_t>(from, worldgen::creature::LEVEL));
    r[0x14c] = from[0x14c], r[0x14d] = from[0x14d];
    std::memcpy(&r[0x150], &from[0x150], 8);         // the region it stands in
    std::memcpy(&r[0x278], &from[0x278], 11 * 0xa0); // what it wears
    std::memcpy(&r[worldgen::creature::APPEARANCE], &from[worldgen::creature::APPEARANCE], 0xb0); // and looks like
    if (clampHealth) {
        const float maximum = playerMaxHealth();
        if (nativePlayer && maximum > 0 && playerHealth() > maximum)
            put<float>(r, worldgen::creature::HEALTH, maximum);
    }
}

bool SpawnedCreatures::playerState(const ExtractedWorld &ew, glm::vec3 &local, float &headingRad, glm::vec3 &velocity,
                                   bool &onGround) const {
    if (!hasPlayer)
        return false;
    const worldgen::CreatureRecord &r = player.record;
    local = ew.toLocal(reinterpret_cast<const int64_t *>(&r[worldgen::creature::POSITION]));
    local.z -= NATIVE_FEET;
    headingRad = get<float>(r, worldgen::creature::YAW) / 57.2957795f;
    velocity = {get<float>(r, 0x34), get<float>(r, 0x38), get<float>(r, 0x3c)};
    onGround = get<uint32_t>(r, 0x5c) & 1;
    return true;
}

void SpawnedCreatures::think(ExtractedWorld &ew, double dtMs, float hour) {
    aiCarried += dtMs;
    if (aiCarried < 40.0)
        return;
    const uint32_t dt = (uint32_t)aiCarried; // the ms since its last tick, as the game passes it
    aiCarried -= dt;
    aiClock += dt;
    nativeai::World w;
    w.client = client;
    w.time = elapsedWorldMs;
    // A world that keeps its own clock (a host's, or a mirror's set by its host) goes by it;
    // otherwise by the caller's hour.
    if (clockRate <= 0 && !client)
        dayMs = std::fmod(hour, 24.0f) * 3600000.0f;
    w.day = dayMs;
    if (hasPlayer) { // the local player comes first in the world's list
        player.side.record = player.record.data();
        w.creatures.push_back(&player.side);
        w.byId[localId] = &player.side;
        w.local = &player.side;
    }
    for (auto &[zone, list] : zones)
        for (Creature &cr : list) {
            cr.side.record = cr.record.data();
            w.creatures.push_back(&cr.side);
            w.byId[get<int64_t>(cr.record, worldgen::creature::ID)] = &cr.side;
        }
    w.block = [&](int x, int y, int z) { return ew.gameBlock(x, y, z); };
    w.zone = [&](int zx, int zy) {
        bool loaded = false;
        ew.gameBlock((int64_t)zx * 64, (int64_t)zy * 64, 0, &loaded);
        return loaded;
    };
    // A zone's static objects as the game keeps them (0xe0 bytes): kind, place, quarter
    // turns (+0x20), scale (+0x24), shut (+0x28, for doors; waiting, for those that take an
    // item), the swing time (+0x2c, a float), whoever uses it (+0x30) and the item it takes
    // (+0x38, written as the use prompt writes the item handed in, so the two compare equal).
    w.objects = [&](int zx, int zy) -> const std::vector<nativeai::Object> * {
        const nativesaves::ZoneLists *lists = zoneLists(ew, zx, zy);
        return lists ? &lists->staticObjects : nullptr;
    };
    // A cell spot of a near zone to walk to (0x31c0e0), taken from the zone's working spot map
    // with the AI's rand().
    w.spot = [&](int cellX, int cellY, int64_t out[3]) {
        const std::pair<int, int> key{floorDiv(cellX, 4), floorDiv(cellY, 4)};
        const auto zone = zoneResidents.find(key);
        if (zone == zoneResidents.end())
            return false;
        const worldgen::ZoneContents &z = *zone->second;
        worldgen::SpotMap &working = zoneSpots.try_emplace(key, z.working).first->second;
        const worldgen::Spot s = worldgen::spotInCell({z.spots, working, z.objects, worldgen::middleSpot(z.terrain)}, cellX, cellY, worldgen::Draw(w.rand));
        out[0] = s.x, out[1] = s.y, out[2] = s.z;
        return true;
    };
    w.objectKey = [](int zx, int zy, int i) {
        return ((uint64_t)(uint32_t)zx << 40) ^ ((uint64_t)(uint32_t)zy << 16) ^ (uint64_t)(uint32_t)i;
    };
    w.rand = [this] { // MSVC's rand()
        aiRand = aiRand * 214013u + 2531011u;
        return (int)((aiRand >> 16) & 0x7fff);
    };
    w.clock = [this] { return aiClock; };
    if (roadTravel)
        advanceTrips(ew, dt);
    nativeai::tick(w, ai, dt);
    // What the AI hands each creature (seats, doors) waits for its next step (0x2c3710).
    for (nativeai::Creature *c : w.creatures)
        handed += c->interactions.size();
    // What the tick tells the world: aggro, buff notices and calls for help.
    worldEvents.aggro.insert(worldEvents.aggro.end(), w.aggroNotes.begin(), w.aggroNotes.end());
    worldEvents.buffs.insert(worldEvents.buffs.end(), w.buffNotes.begin(), w.buffNotes.end());
    worldEvents.calls.insert(worldEvents.calls.end(), w.calls.begin(), w.calls.end());
}

std::optional<SpawnedCreatures::SpriteAsset> SpawnedCreatures::spriteAsset(World &world, int catalogId) {
    const float white[4] = {1,1,1,1};
    const int model = worldModel(world, catalogId, white, false);
    if (model < 0 || !world.modelMesh(model)) return {};
    return SpriteAsset{model, glm::ivec3(world.modelSize(model))};
}

int SpawnedCreatures::worldModel(World &world, int id, const float *tint, bool recoloured, const hair::Style *look) {
    if (id < 0 || (size_t)id >= sprites.size() || sprites[id].name.empty())
        return -1;
    // The hair takes the appearance's colour (the draw's tint) and, like every hair
    // sprite, does not show its red, green and blue marker voxels (the model loading).
    const int r = recoloured ? (int)std::lround(tint[0] * 255) : 255, g = recoloured ? (int)std::lround(tint[1] * 255) : 255,
              b = recoloured ? (int)std::lround(tint[2] * 255) : 255;
    auto key = std::make_tuple(id, r, g, b, recoloured, look ? hair::cacheKey(*look) : std::array<uint32_t, 3>{});
    if (auto it = tinted.find(key); it != tinted.end())
        return it->second;
    const int m = world.loadModel(sprites[id].name, glm::u8vec3(r, g, b), recoloured, look);
    tinted[key] = m;
    return m;
}

void SpawnedCreatures::draw(const ExtractedWorld &ew, World &world, double dtMs, uint32_t nowMs, glm::vec3 playerLocal,
                            float range, std::vector<DrawItem> &out) {
    nativepose::Models lookup;
    lookup.byId = [&](int id) -> const nativepose::Model * {
        auto it = models.find(id);
        return it == models.end() ? nullptr : &it->second;
    };
    lookup.forItem = [&](const uint8_t *item) -> const nativepose::Model * { // 0x173b10 (NativeItems)
        auto it = models.find(nativeitems::modelId(item));
        return it == models.end() ? nullptr : &it->second;
    };
    std::vector<nativepose::Draw> draws;
    static std::mt19937 poseRand(99);
    drawn = 0;
    auto drawOne = [&](Creature &cr, bool isPlayer) {
            worldgen::CreatureRecord &r = cr.record;
            const glm::vec3 at = ew.toLocal(reinterpret_cast<const int64_t *>(&r[worldgen::creature::POSITION]));
            if (glm::length(glm::vec2(at - playerLocal)) > range)
                return;
            // A dead creature is not drawn; as it dies its parts burst into voxels (main.cpp).
            const int64_t id = get<int64_t>(r, worldgen::creature::ID);
            const bool bursting = !isPlayer && toBurst.erase(id) > 0;
            if (!bursting && !(get<float>(r, worldgen::creature::HEALTH) > 0.0f))
                return;
            // Where it is drawn (+0xb10) and which way it faces (+0xb34..+0xb3c): its place, as
            // the game keeps them while nothing else moves it; a creature a host's update moved
            // eases there (smooth).
            if (!(get<float>(r, SMOOTHING) > 0.0f)) {
                std::memcpy(&r[DRAWN_POSITION], &r[worldgen::creature::POSITION], 24);
                std::memcpy(&r[DRAWN_ORIENTATION], &r[ORIENTATION], 12);
            }

            nativepose::Call call;
            call.dt = (float)dtMs;
            call.cameraX = -ew.originDotsX();
            call.cameraY = -ew.originDotsY();
            call.nowMs = nowMs;
            call.rand = [] { return (int)(poseRand() & 0x7fff); };
            const nativerules::HasBuff has = [&](int t) {
                for (const auto &b : cr.side.buffs)
                    if (b[0] == (uint8_t)t)
                        return true;
                return false;
            };
            call.hasBuff = has;
            call.sparks = &cr.sparks;
            draws.clear();
            // The world's draw sets the pose state and its clock from the ability right before
            // posing a creature it draws (creature-pose.md §12); one culled above keeps them.
            if (get<float>(r, worldgen::creature::HEALTH) > 0.0f)
                nativepose::setPoseState(r.data());
            nativepose::pose(r.data(), call, lookup, &draws);
            // A weapon's effect (0xeec50) stands for the spirit cubes set into the weapon
            // (creature-pose-items.md §9.1): plain cube draws, none for a weapon without spirits.
            const size_t posed = draws.size();
            for (size_t k = 0; k < posed; k++)
                if (draws[k].kind == nativepose::WEAPON_EFFECT_DRAW)
                    nativepose::spiritDraws(draws[k], draws);
            const int hairModel = get<int16_t>(r, 0x92); // the appearance's hair (+0x1a)
            for (const nativepose::Draw &d : draws) {
                if (!d.model || d.kind == nativepose::WEAPON_EFFECT_DRAW)
                    continue;
                const int id = (int)reinterpret_cast<intptr_t>(d.model->user);
                // The hair takes the appearance's colour, a spirit cube (the white cube.cub) its
                // material's: both are the model recoloured by the draw's tint.
                const bool recoloured = id == hairModel || id == SPIRIT_CUBE_MODEL;
                const auto *look = isPlayer && id == hairModel && playerHairStyle.dual ? &playerHairStyle : nullptr;
                const int m = worldModel(world, id, d.tint, recoloured, look);
                const uint32_t mesh = world.modelMesh(m);
                if (!mesh)
                    continue;
                glm::mat4 game;
                std::memcpy(&game[0][0], d.matrix, sizeof d.matrix); // the game's row vectors are glm's columns
                const glm::vec3 size = world.modelSize(m);
                glm::mat4 place = glm::translate(glm::mat4(1), glm::vec3(0, 0, (float)ExtractedWorld::Z_OFFSET)) * game *
                                  glm::translate(glm::mat4(1), glm::vec3(size.x * .5f, size.y * .5f, 0));
                if (bursting)
                    bursts.push_back({m, place});
                else
                    out.push_back({mesh, place, 0, 0, true});
                playerPartsDrawn += isPlayer;
            }
            drawn += !isPlayer;
    };
    for (auto &[zone, list] : zones)
        for (Creature &cr : list)
            drawOne(cr, false);
    playerPartsDrawn = 0;
    if (nativePlayer && hasPlayer) // the native player, by the same pose code
        drawOne(player, true);
}

// --- Multiplayer: the world's side (docs/specs/netcode-roles.md) ----------------------------

SpawnedCreatures::Creature *SpawnedCreatures::find(int64_t id) {
    if (hasPlayer && id == localId)
        return &player;
    for (auto &[zone, list] : zones)
        for (Creature &cr : list)
            if (get<int64_t>(cr.record, worldgen::creature::ID) == id)
                return &cr;
    return nullptr;
}

void SpawnedCreatures::setPlayerId(int64_t id) {
    localId = id;
    if (hasPlayer)
        put<int64_t>(player.record, worldgen::creature::ID, id);
}

uint8_t *SpawnedCreatures::networkCreature(int64_t id, bool &made) {
    made = false;
    if (hasPlayer && id == localId)
        return nullptr; // (the local player's record never comes from the network)
    if (Creature *cr = find(id)) {
        // The update applies to what the host sent, not to the swing drawn on since.
        if (client)
            put<float>(cr->record, ABILITY_TIME, cr->sentAbilityTime);
        return cr->record.data();
    }
    // 0x4ce80 with the id, then into the world (0x29d0d0).
    std::vector<Creature> &list = zones[NETWORK_ZONE];
    list.push_back({worldgen::newCreature(id), {}});
    made = true;
    return list.back().record.data();
}

void SpawnedCreatures::receivedUpdate(int64_t id, bool fresh) {
    Creature *cr = find(id);
    if (!cr)
        return;
    worldgen::CreatureRecord &r = cr->record;
    if (get<float>(r, worldgen::creature::HEALTH) > 0 && (dead.count(id) || worldDead.count(id))) {
        // The host has brought this id back. Its next death must produce effects and
        // kill credit again, and it must not ease from its old corpse's position.
        fresh = true;
        dead.erase(id);
        worldDead.erase(id);
        toBurst.erase(id);
        seenAlive.insert(id);
        cr->side.buffs.clear();
        cr->sparks = {};
    }
    // The ability time shown: the host's when the swing is new (another ability, or the same one
    // started again: the host's time went back), otherwise never behind what is shown already.
    const float sent = get<float>(r, ABILITY_TIME);
    const uint8_t ability = r[worldgen::creature::ABILITY];
    const bool restarted = sent < cr->sentAbilityTime;
    cr->sentAbilityTime = sent;
    if (fresh || !client || ability != cr->shownAbility || restarted)
        cr->shownAbilityTime = sent;
    else
        cr->shownAbilityTime = std::max(cr->shownAbilityTime, sent);
    cr->shownAbility = ability;
    if (client)
        put<float>(r, ABILITY_TIME, cr->shownAbilityTime);
    if (fresh || !(get<float>(r, worldgen::creature::HEALTH) > 0.0f)) {
        // Drawn where it is, with nothing to ease from.
        put<float>(r, SMOOTHING, 0.0f);
        cr->side.inventory.clear();
        std::memcpy(&r[DRAWN_POSITION], &r[worldgen::creature::POSITION], 24);
        std::memcpy(&r[DRAWN_ORIENTATION], &r[ORIENTATION], 12);
        std::memcpy(&r[PREVIOUS_DRAWN_POSITION], &r[worldgen::creature::POSITION], 24);
        std::memcpy(&r[PREVIOUS_DRAWN_ORIENTATION], &r[ORIENTATION], 12);
        return;
    }
    // Eases from where it is drawn now.
    put<float>(r, SMOOTHING, SMOOTHING_MS);
    std::memcpy(&r[PREVIOUS_DRAWN_POSITION], &r[DRAWN_POSITION], 24);
    std::memcpy(&r[PREVIOUS_DRAWN_ORIENTATION], &r[DRAWN_ORIENTATION], 12);
    std::memcpy(&r[PREVIOUS_AIM], &r[AIM], 12);
}

void SpawnedCreatures::smooth(float dtMs) {
    const float kPosition = gamemath::blendFactor(dtMs, POSITION_EASE);
    const float kOrientation = gamemath::blendFactor(dtMs, ORIENTATION_EASE);
    for (auto &[zone, list] : zones)
        for (Creature &cr : list) {
            worldgen::CreatureRecord &r = cr.record;
            // A mirror's creature goes on with its ability between its host's updates, so its
            // swing is drawn as it plays rather than in the updates' steps. It runs at most a
            // little way past the host's last time and then waits for the host to catch up
            // (it never steps back).
            if (client && zone == NETWORK_ZONE && r[worldgen::creature::ABILITY] != 0 &&
                get<float>(r, worldgen::creature::HEALTH) > 0.0f) {
                cr.shownAbilityTime = std::max(cr.shownAbilityTime, std::min(cr.shownAbilityTime + dtMs, cr.sentAbilityTime + ABILITY_LEAD_MS));
                put<float>(r, ABILITY_TIME, cr.shownAbilityTime);
            }
            const float left = get<float>(r, SMOOTHING);
            if (!(left > 0.0f))
                continue;
            for (size_t k = 0; k < 3; k++) {
                const int64_t at = get<int64_t>(r, worldgen::creature::POSITION + 8 * k);
                const int64_t drawn = get<int64_t>(r, DRAWN_POSITION + 8 * k);
                put<int64_t>(r, DRAWN_POSITION + 8 * k, drawn + (int64_t)std::llround((double)(at - drawn) * kPosition));
                // Angles in degrees, the short way round.
                const float to = get<float>(r, ORIENTATION + 4 * k), from = get<float>(r, DRAWN_ORIENTATION + 4 * k);
                const float turn = std::remainder(to - from, 360.0f);
                put<float>(r, DRAWN_ORIENTATION + 4 * k, from + turn * kOrientation);
            }
            put<float>(r, SMOOTHING, left - dtMs);
        }
}

void SpawnedCreatures::keepOnly(const std::set<int64_t> *keep) {
    for (auto &[zone, list] : zones)
        for (auto it = list.begin(); it != list.end();) {
            const int64_t id = get<int64_t>(it->record, worldgen::creature::ID);
            if (!keep || !keep->count(id)) {
                ai.erase(id);
                it = list.erase(it);
            } else
                ++it;
        }
    if (!keep) {
        // A world started over makes its residents again as its zones come near.
        zoneResidents.clear(), zoneRand.clear(), zoneMissions.clear(), zoneSpots.clear(), zoneChests.clear();
        made.clear();
        for (auto it = zones.begin(); it != zones.end();)
            it = it->first == NETWORK_ZONE ? std::next(it) : zones.erase(it);
        effects.clear();
    }
}

void SpawnedCreatures::queueInteraction(int64_t id, const nativeai::Interaction &in) {
    if (Creature *cr = find(id))
        cr->side.interactions.push_back(in);
}

bool SpawnedCreatures::takePlayerInteraction(nativeai::Interaction &out) {
    if (!hasPlayer || player.side.interactions.empty())
        return false;
    out = player.side.interactions.front();
    player.side.interactions.pop_front();
    return true;
}

nativehit::World SpawnedCreatures::hitWorld(ExtractedWorld &ew, std::map<int64_t, nativeai::Creature *> &byId) {
    nativehit::World hw;
    hw.client = client;
    hw.local = hasPlayer ? player.record.data() : nullptr;
    hw.creatures = worldList();
    for (nativeai::Creature *c : hw.creatures) {
        int64_t id;
        std::memcpy(&id, c->record + worldgen::creature::ID, 8);
        byId[id] = c;
    }
    hw.byId = [&byId](int64_t id) -> nativeai::Creature * {
        auto f = byId.find(id);
        return f == byId.end() ? nullptr : f->second;
    };
    hw.rand = [this] {
        randState ^= randState << 13, randState ^= randState >> 17, randState ^= randState << 5;
        return (int)(randState & 0x7fff);
    };
    hw.died = [this, &ew](nativeai::Creature *c) {
        if (c == &player.side || client)
            return;
        kills++;
        dropFrom(ew, c);
    };
    return hw;
}

void SpawnedCreatures::keepHitRecords(nativehit::World &hw) {
    keepRecords(hw.sounds, hw.particles, hw.numbers, hw.buffNotes);
    sounds.insert(sounds.end(), hw.sounds.begin(), hw.sounds.end());
    worldEvents.aggro.insert(worldEvents.aggro.end(), hw.aggroNotes.begin(), hw.aggroNotes.end());
    for (const auto &[credited, dead] : hw.kills) {
        cwnet::KillRecord k;
        std::memcpy(k.data(), &credited, 8);
        std::memcpy(k.data() + 8, &dead, 8);
        worldEvents.kills.push_back(k);
    }
}

void SpawnedCreatures::applyGuestEvents(ExtractedWorld &ew, const cwnet::GuestEvents &caused) {
    lastWorld = &ew;
    // 1. Their sounds join the world's list.
    worldEvents.sounds.insert(worldEvents.sounds.end(), caused.sounds.begin(), caused.sounds.end());
    // 2. Their hits: listed, then each applied (0x29dcf0); a hit that hurts makes its target flash.
    std::map<int64_t, nativeai::Creature *> byId;
    nativehit::World hw = hitWorld(ew, byId);
    for (const cwnet::DamageRecord &number : caused.damageNumbers) {
        worldEvents.damageNumbers.push_back(number);
        notifyHitVisual(number);
        nativehit::apply(hw, number);
        int64_t target;
        float damage;
        std::memcpy(&target, number.data() + 8, 8);
        std::memcpy(&damage, number.data() + 0x10, 4);
        if (Creature *t = find(target); t && damage > 0.0f)
            put<float>(t->record, HIT_FLASH, 1.0f);
    }
    keepHitRecords(hw);
    // 3. Their buffs, given to their targets (flag 0) and listed.
    for (const cwnet::BuffRecord &notice : caused.buffs) {
        int64_t target;
        std::memcpy(&target, notice.data() + 8, 8);
        if (Creature *t = find(target)) {
            nativebuffs::Buff b;
            std::memcpy(b.data(), notice.data() + 0x10, b.size());
            nativebuffs::give(t->side, b, false);
        }
        worldEvents.buffs.push_back(notice);
    }
    // 4. Their effect objects go on in this world (whose hits they now make) and to the others.
    for (const cwnet::EffectRecord &e : caused.effects) {
        effects.push_back(e);
        worldEvents.effects.push_back(e);
    }
}

void SpawnedCreatures::giveBuffNotice(const cwnet::BuffRecord &notice) {
    int64_t target;
    std::memcpy(&target, notice.data() + 8, 8);
    if (Creature *t = find(target)) {
        nativebuffs::Buff b;
        std::memcpy(b.data(), notice.data() + 0x10, b.size());
        nativebuffs::give(t->side, b, notice[0x28] != 0);
    }
}

void SpawnedCreatures::setAggro(const cwnet::AggroRecord &notice) {
    int64_t angering, angered;
    float value;
    std::memcpy(&angering, notice.data(), 8);
    std::memcpy(&angered, notice.data() + 8, 8);
    std::memcpy(&value, notice.data() + 0x10, 4);
    if (Creature *c = find(angered))
        c->side.aggro[angering] = value;
}

void SpawnedCreatures::hitPlayer(const std::array<uint8_t, 0x48> &number) {
    if (!hasPlayer)
        return;
    if (!nativePlayer) { // CubeVulkan's own player takes it its own way
        if (playerNumber)
            playerNumber(number);
        return;
    }
    player.side.record = player.record.data();
    const auto applied = nativeplayerhit::apply(player.side, number);
    worldEvents.buffs.insert(worldEvents.buffs.end(), applied.buffNotices.begin(), applied.buffNotices.end());
    worldEvents.sounds.insert(worldEvents.sounds.end(), applied.sounds.begin(), applied.sounds.end());
    sounds.insert(sounds.end(), applied.sounds.begin(), applied.sounds.end());
}

void SpawnedCreatures::flash(int64_t id) {
    if (Creature *c = find(id))
        put<float>(c->record, HIT_FLASH, 1.0f);
}

steampoints::GroundSources SpawnedCreatures::groundLights(ExtractedWorld &ew, int zx, int zy) {
    steampoints::GroundSources sources;
    if (record(localId)) sources.player = localId;
    const auto *items = groundItems(ew, zx, zy);
    if (!items) return sources;
    sources.items.reserve(items->size());
    for (const auto &record : *items) {
        auto &item = sources.items.emplace_back();
        item.category = record[nativesaves::item::TYPE];
        std::memcpy(&item.id, record.data() + nativesaves::item::SUBTYPE, sizeof item.id);
        std::memcpy(item.position.data(), record.data() + grounditems::ground::POSITION, sizeof item.position);
        std::memcpy(&item.timerMs, record.data() + grounditems::ground::TIMER, sizeof item.timerMs);
        std::memcpy(&item.quantity, record.data() + grounditems::ground::COUNT, sizeof item.quantity);
        std::memcpy(&item.owner, record.data() + grounditems::ground::OWNER, sizeof item.owner);
    }
    return sources;
}

void SpawnedCreatures::collectLights(const steampoints::Frame &frame, std::vector<steampoints::Light> &out) const {
    auto collect = [&](const Creature &creature) {
        const nativerules::HasBuff has = [&](int type) {
            for (const auto &buff : creature.side.buffs) if (buff[0] == type) return true;
            return false;
        };
        steampoints::collectCreature(creature.record.data(), has, frame, out);
    };
    if (hasPlayer) collect(player);
    for (const auto &[zone, list] : zones)
        for (const auto &creature : list) collect(creature);
}

std::vector<steampoints::EffectSource> SpawnedCreatures::effectLights() const {
    std::vector<steampoints::EffectSource> sources;
    sources.reserve(effects.size());
    for (const auto &record : effects) {
        auto &source = sources.emplace_back();
        std::memcpy(&source.kind, record.data() + 0x68, sizeof source.kind);
        std::memcpy(source.position.data(), record.data() + 8, sizeof source.position);
        std::memcpy(&source.strength, record.data() + 0x54, sizeof source.strength);
    }
    return sources;
}

void SpawnedCreatures::addEffect(const cwnet::EffectRecord &effect) { effects.push_back(effect); }

bool SpawnedCreatures::isDead(int64_t id) const {
    const uint8_t *r = record(id);
    if (!r)
        return false;
    float health;
    int32_t race;
    std::memcpy(&health, r + worldgen::creature::HEALTH, 4);
    std::memcpy(&race, r + worldgen::creature::RACE, 4);
    return !(health > 0.0f) && race != RACE_WITHOUT_DEATH;
}

bool SpawnedCreatures::zoneAnswer(ExtractedWorld &ew, const cwnet::ZoneKey &zone, cwnet::ZoneAnswer &out) {
    const nativesaves::ZoneLists *lists = zoneLists(ew, zone.x, zone.y);
    if (!lists)
        return false;
    for (const nativesaves::BlockChange &change : lists->blockChanges) {
        cwnet::BlockChangeRecord r;
        std::memcpy(r.data(), change.bytes.data(), r.size());
        out.blockChanges.push_back(r);
    }
    if (lists->groundItemsChanged)
        out.groundItems = std::vector<cwnet::ZoneObjectRecord>(lists->groundItems.begin(), lists->groundItems.end());
    if (lists->staticObjectsChanged)
        out.staticObjects = std::vector<cwnet::ZoneObjectRecord>(lists->staticObjects.begin(), lists->staticObjects.end());
    return true;
}

void SpawnedCreatures::replaceGroundItems(ExtractedWorld &ew, const cwnet::ZoneObjects &zone) {
    nativesaves::ZoneLists *lists = zoneLists(ew, zone.key.x, zone.key.y);
    if (!lists)
        return; // (only a zone in memory takes them)
    lists->groundItems.assign(zone.records.begin(), zone.records.end());
    replacedGround.insert({zone.key.x, zone.key.y});
}

void SpawnedCreatures::replaceStaticObjects(ExtractedWorld &ew, const cwnet::ZoneObjects &zone) {
    nativesaves::ZoneLists *lists = zoneLists(ew, zone.key.x, zone.key.y);
    if (!lists)
        return;
    lists->staticObjects.assign(zone.records.begin(), zone.records.end());
    objectsChanged.insert({zone.key.x, zone.key.y}); // (queued for the object rebuild, world+0x1a0)
}

void SpawnedCreatures::applyBlockChange(const cwnet::BlockChangeRecord &change) {
    // The change goes into its zone's change log, which puts its "now" block in (an empty "now"
    // block, type and flags 0, is a block removed).
    nativesaves::BlockChange kept;
    std::memcpy(kept.bytes.data(), change.data(), kept.bytes.size());
    zoneStates.addBlockChange(kept);
}

std::vector<cwnet::ZoneKey> SpawnedCreatures::zonesNeedingState(int64_t blockX, int64_t blockY) const {
    constexpr int64_t REACH = 256; // blocks either way of the player
    auto zoneOf = [](int64_t block) { return (int32_t)std::floor((double)block / 64.0); };
    std::vector<cwnet::ZoneKey> out;
    for (const auto &[zx, zy] : stateNeeded)
        if (zx >= zoneOf(blockX - REACH) && zx <= zoneOf(blockX + REACH) && zy >= zoneOf(blockY - REACH) &&
            zy <= zoneOf(blockY + REACH))
            out.push_back({zx, zy});
    return out;
}

void SpawnedCreatures::undoBlockChanges(ExtractedWorld &ew) {
    for (const nativesaves::Cell &zone : zoneStates.zonesInMemory()) {
        if (nativesaves::ZoneLists *lists = zoneStates.find(zone))
            lists->blockChanges.clear();
        ew.undoBlockChanges(zone[0], zone[1]);
        stateNeeded.insert({zone[0], zone[1]});
    }
}

void SpawnedCreatures::zoneChangeLists() {
    // (A mirror's own step makes no lists to send: its zones change by the host's.)
    if (!client) {
        for (const auto &[zx, zy] : groundChanged)
            if (const nativesaves::ZoneLists *lists = zoneStates.find({zx, zy}))
                worldEvents.groundItems.push_back({{zx, zy}, {lists->groundItems.begin(), lists->groundItems.end()}});
        for (const auto &[zx, zy] : objectsChanged)
            if (const nativesaves::ZoneLists *lists = zoneStates.find({zx, zy}))
                worldEvents.staticObjects.push_back({{zx, zy}, {lists->staticObjects.begin(), lists->staticObjects.end()}});
    }
    groundChanged.clear();
    objectsChanged.clear();
}

std::set<std::pair<int, int>> SpawnedCreatures::takeReplacedGround() { return std::exchange(replacedGround, {}); }

void SpawnedCreatures::advanceClock(double dtMs) {
    if (clockRate <= 0)
        return;
    constexpr double DAY_MS = 86400000.0;
    double t = (double)dayMs + dtMs * clockRate;
    while (t > DAY_MS) {
        t -= DAY_MS;
        dayCount++;
        worldgen::setWorldDay(dayCount);
        worldgen::setRefreshDay(dayCount);
        worldReset();
        worldEvents.sleeps.push_back(0);
    }
    dayMs = (float)t;
}

void SpawnedCreatures::worldReset() {
    // Every creature that is not a player goes; the zones make them again, the dead included.
    for (auto &[zone, list] : zones)
        for (auto it = list.begin(); it != list.end();)
            if (it->record[worldgen::creature::HOSTILITY] != 0) {
                ai.erase(get<int64_t>(it->record, worldgen::creature::ID));
                it = list.erase(it);
            } else
                ++it;
    made.clear();
    // Loaded zones and their previous refresh day survive the native reset. Keeping
    // that day lets the next visited-zone pass restore the new day's base objects.
    worldDead.clear();
    if (respawnSettings.mode != enemyrespawn::Mode::Permanent) {
        // Native resets must not bypass either configured enemy lifecycle.
        const auto pending = enemyDeaths.ids();
        worldDead.insert(pending.begin(), pending.end());
    } else
        enemyDeaths.clear();
    protectedDead.clear();
    dead.clear();
    seenAlive.clear();
    if (storedZones)
        nativesaves::resetWorld(*storedZones);
}

void SpawnedCreatures::keepReceived(const cwnet::WorldEvents &received) {
    cwnet::WorldEvents &e = worldEvents;
    e.sounds.insert(e.sounds.end(), received.sounds.begin(), received.sounds.end());
    sounds.insert(sounds.end(), received.sounds.begin(), received.sounds.end());
    e.particles.insert(e.particles.end(), received.particles.begin(), received.particles.end());
    e.blockChanges.insert(e.blockChanges.end(), received.blockChanges.begin(), received.blockChanges.end());
    e.pickups.insert(e.pickups.end(), received.pickups.begin(), received.pickups.end());
    e.kills.insert(e.kills.end(), received.kills.begin(), received.kills.end());
    e.feats.insert(e.feats.end(), received.feats.begin(), received.feats.end());
    e.calls.insert(e.calls.end(), received.calls.begin(), received.calls.end());
    e.sleeps.insert(e.sleeps.end(), received.sleeps.begin(), received.sleeps.end());
}

void SpawnedCreatures::layItem(ExtractedWorld &ew, const uint8_t *item, const int64_t pos[3], int count) {
    nativedrop::World dw = dropWorld(ew);
    nativedrop::Place p;
    std::memcpy(p.item.data(), item, p.item.size());
    std::copy(pos, pos + 3, p.pos);
    p.count = count;
    nativedrop::place(dw, p, nullptr);
    keepDrops(ew, dw, 0, 0, false);
}
