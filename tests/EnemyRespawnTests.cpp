#include "EnemyRespawn.h"
#include "Assets.h"
#include "Config.h"
#include "Spawned.h"
#include "NativeProgress.h"
#include "worldgen/Generator.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
int failures = 0;
#define CHECK_RESPAWN(condition) do { if (!(condition)) { \
    std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); ++failures; } } while (0)

using namespace enemyrespawn;

void configuration() {
    Config config;
    auto settings = Settings::read(config);
    CHECK_RESPAWN(settings.mode == Mode::Timed && settings.seconds == 300);
    config.set("Creatures", "DeathMode", "iNsTaNcE");
    CHECK_RESPAWN(Settings::read(config).mode == Mode::Instance);
    config.set("Creatures", "DeathMode", "Permanent");
    CHECK_RESPAWN(Settings::read(config).mode == Mode::Permanent);
    config.set("Creatures", "DeathMode", "mistyped");
    config.set("Creatures", "RespawnSeconds", "nan");
    CHECK_RESPAWN(Settings::read(config).mode == Mode::Timed && Settings::read(config).seconds == 300);
    config.set("Creatures", "RespawnSeconds", "-4");
    CHECK_RESPAWN(Settings::read(config).seconds == 1);
    CHECK_RESPAWN(isEnemy(1) && isEnemy(7) && !isEnemy(0) && !isEnemy(3) && !isEnemy(6) && !isEnemy(8));
}

void lifecycle() {
    Deaths timed({Mode::Timed, 300});
    timed.killed(11, {2, 3});
    CHECK_RESPAWN(timed.unload({2, 3}).empty()); // walking away does not bypass the timer
    CHECK_RESPAWN(timed.advance(299).empty());
    timed.killed(11, {2, 3}); // a repeated observation must not restart it
    CHECK_RESPAWN(timed.advance(-1).empty());
    CHECK_RESPAWN(timed.advance(1) == std::vector<int64_t>{11});
    CHECK_RESPAWN(!timed.contains(11) && timed.advance(1000).empty());
    timed.killed(11, {2, 3}); // the same enemy can be killed and respawn again
    CHECK_RESPAWN(timed.advance(300) == std::vector<int64_t>{11});

    Deaths instance({Mode::Instance, 300});
    instance.killed(12, {-2, 3});
    instance.killed(13, {2, 3});
    CHECK_RESPAWN(instance.advance(100000).empty());
    CHECK_RESPAWN(instance.unload({-2, 3}) == std::vector<int64_t>{12});
    CHECK_RESPAWN(instance.contains(13) && !instance.contains(12));
    instance.forget(13); // a tamed creature's old id is excluded
    CHECK_RESPAWN(instance.unload({2, 3}).empty());
    Deaths permanent({Mode::Permanent, 300});
    permanent.killed(14, {2, 3});
    CHECK_RESPAWN(!permanent.contains(14) && permanent.advance(100000).empty() && permanent.unload({2, 3}).empty());
}

float health(const uint8_t *record) {
    float value;
    std::memcpy(&value, record + worldgen::creature::HEALTH, sizeof value);
    return value;
}
void setHealth(uint8_t *record, float value) {
    std::memcpy(record + worldgen::creature::HEALTH, &value, sizeof value);
}

void mirrorRepeatedDeaths() {
    SpawnedCreatures mirror;
    mirror.setRemoteClient(true);
    ExtractedWorld blocks;
    bool made = false;
    uint8_t *record = mirror.networkCreature(5000, made);
    setHealth(record, 100);
    mirror.receivedUpdate(5000, true);
    CHECK_RESPAWN(mirror.takeDeaths(blocks).empty());
    for (int life = 0; life < 2; ++life) {
        setHealth(record, 0);
        mirror.receivedUpdate(5000, false);
        CHECK_RESPAWN(mirror.takeDeaths(blocks).size() == 1);
        CHECK_RESPAWN(mirror.takeDeaths(blocks).empty());
        setHealth(record, 100);
        mirror.receivedUpdate(5000, false);
        CHECK_RESPAWN(!mirror.isWorldDead(5000));
    }
}

void dailyReset() {
    SaveGame progress;
    progress.perPlayerTaken.insert({1, 2, 3});
    progress.finishedMissions.insert({1, 2});
    progress.solvedMissions.insert({3, 4});
    progress.regionCounters[{5, 6}] = 9;
    progress.purchases[7].push_back({});
    SaveGame::Bagged temporary, retained;
    temporary.category = 0x16, retained.category = 3;
    progress.bag = {temporary, retained};
    nativeprogress::resetDay(progress);
    CHECK_RESPAWN(progress.perPlayerTaken.empty() && progress.finishedMissions.empty() && progress.purchases.empty());
    CHECK_RESPAWN(progress.bag.size() == 1 && progress.bag[0].category == 3);
    CHECK_RESPAWN(progress.solvedMissions.count({3, 4}) && progress.regionCounters.at({5, 6}) == 9);
    SpawnedCreatures host;
    bool made = false;
    uint8_t *player = host.networkCreature(7001, made);
    player[worldgen::creature::HOSTILITY] = 0;
    setHealth(player, 100);
    uint8_t *npc = host.networkCreature(7002, made);
    npc[worldgen::creature::HOSTILITY] = 3;
    setHealth(npc, 100);
    host.setClock(4, 86400000.0f);
    host.setClockRate(1);
    host.advanceClock(0);
    CHECK_RESPAWN(host.day() == 4 && host.record(7002) && host.events().sleeps.empty());
    host.advanceClock(1);
    CHECK_RESPAWN(host.day() == 5 && host.timeOfDay() == 1);
    CHECK_RESPAWN(host.record(7001) && !host.record(7002));
    CHECK_RESPAWN(host.events().sleeps == std::vector<int32_t>{0});
}

// Optional owner-data integration: the same generated enemy must actually return,
// with health and its original spawn point, in both lifecycle modes.
void generatedEnemy() {
    Assets assets;
    ExtractedWorld blocks;
    std::string why;
    if (!assets.locate("") || !blocks.open(ExtractedWorld::find(assets.gameDir()), why) ||
        !worldgen::loadCreatureTables(assets.gameDir() / "Extract" / "oracle", &why)) {
        std::puts("enemy respawn: generated-world check skipped (owner exports unavailable)");
        return;
    }
    worldgen::setWorldSeed(blocks.seed());
    SpawnedCreatures host;
    host.setEnemyRespawn({Mode::Timed, 300});
    const glm::vec3 home = blocks.home();
    host.update(blocks, home);
    int64_t id = 0;
    host.forEach([&](const uint8_t *record) {
        if (id || !isEnemy(record[worldgen::creature::HOSTILITY]))
            return;
        int64_t candidate, position[2];
        std::memcpy(&candidate, record + worldgen::creature::ID, sizeof candidate);
        std::memcpy(position, record + worldgen::creature::POSITION, sizeof position);
        const auto zone = worldgen::zone((int32_t)std::floor(double(position[0]) / 65536 / 64),
                                        (int32_t)std::floor(double(position[1]) / 65536 / 64));
        const auto point = zone->spawns.points.find(candidate);
        if (point != zone->spawns.points.end() && !point->second.requiredDefeated && !point->second.requiredStage &&
            point->second.prerequisites.empty())
            id = candidate;
    });
    CHECK_RESPAWN(id != 0);
    if (!id)
        return;
    host.takeDeaths(blocks);
    setHealth(host.record(id), 0);
    CHECK_RESPAWN(!host.takeDeaths(blocks).empty());
    CHECK_RESPAWN(host.isWorldDead(id) && !host.persistentDeadSet().count(id));
    host.advanceRespawns(299999);
    CHECK_RESPAWN(host.isWorldDead(id));
    host.worldReset(); // Daily resets must not bypass an elapsed-time respawn.
    CHECK_RESPAWN(host.isWorldDead(id));
    host.advanceRespawns(1);
    CHECK_RESPAWN(!host.isWorldDead(id) && !host.record(id));
    host.update(blocks, home);
    CHECK_RESPAWN(host.record(id) && health(host.record(id)) > 0);
    if (!host.record(id))
        return;
    host.setEnemyRespawn({Mode::Instance, 300});
    host.takeDeaths(blocks);
    setHealth(host.record(id), 0);
    host.takeDeaths(blocks);
    host.advanceRespawns(1000000);
    CHECK_RESPAWN(host.isWorldDead(id));
    host.worldReset();
    host.update(blocks, home);
    CHECK_RESPAWN(host.isWorldDead(id) && !host.record(id));
    host.update(blocks, home + glm::vec3(64 * 20, 0, 0));
    CHECK_RESPAWN(!host.isWorldDead(id));
    host.update(blocks, home);
    CHECK_RESPAWN(host.record(id) && health(host.record(id)) > 0);
    // A permanent old death is respected in native mode, recovered in respawn
    // modes, and unrelated saved ids are retained.
    SpawnedCreatures permanentSave;
    permanentSave.setEnemyRespawn({Mode::Permanent, 300});
    permanentSave.setWorldDead({id, -123456789});
    permanentSave.update(blocks, home);
    CHECK_RESPAWN(!permanentSave.record(id) && permanentSave.isWorldDead(id));
    SpawnedCreatures recoveredSave;
    recoveredSave.setEnemyRespawn({Mode::Timed, 300});
    recoveredSave.setWorldDead({id, -123456789});
    recoveredSave.update(blocks, home);
    CHECK_RESPAWN(recoveredSave.record(id) && health(recoveredSave.record(id)) > 0 && !recoveredSave.isWorldDead(id));
    CHECK_RESPAWN(recoveredSave.persistentDeadSet().count(-123456789));

    // Refreshing an already loaded zone restores its static objects without
    // clearing ground items or directly removing its live friendly residents.
    const int zx = (int)std::floor(((double)blocks.originDotsX() / 65536.0 + home.x) / 64.0);
    const int zy = (int)std::floor(((double)blocks.originDotsY() / 65536.0 + home.y) / 64.0);
    blocks.propsAbsolute(zx, zy); // Load this owner's home zone without a renderer.
    CHECK_RESPAWN(recoveredSave.groundItems(blocks, zx, zy));
    if (recoveredSave.groundItems(blocks, zx, zy)) {
        cwnet::ZoneObjects ground{{zx, zy}, {cwnet::ZoneObjectRecord{}}};
        ground.records[0][0] = 123;
        recoveredSave.replaceGroundItems(blocks, ground);
        cwnet::ZoneObjects objects = ground;
        recoveredSave.replaceStaticObjects(blocks, objects);
        bool madeShopper = false;
        uint8_t *shopper = recoveredSave.networkCreature(7003, madeShopper);
        shopper[worldgen::creature::HOSTILITY] = 3;
        setHealth(shopper, 100);
        recoveredSave.setClock(1, 100);
        recoveredSave.update(blocks, home);
        CHECK_RESPAWN(recoveredSave.record(7003));
        CHECK_RESPAWN(*recoveredSave.groundItems(blocks, zx, zy) == ground.records);
        CHECK_RESPAWN(recoveredSave.zoneObjects(zx, zy) && *recoveredSave.zoneObjects(zx, zy) != objects.records);
        recoveredSave.worldReset();
        CHECK_RESPAWN(!recoveredSave.record(7003));
        CHECK_RESPAWN(*recoveredSave.groundItems(blocks, zx, zy) == ground.records);
    }
}
} // namespace

int enemyRespawnTests() {
    failures = 0;
    configuration();
    lifecycle();
    mirrorRepeatedDeaths();
    dailyReset();
    generatedEnemy();
    std::puts("enemy respawn: configuration, lifecycle, mirror and generated-world checks complete");
    return failures;
}
