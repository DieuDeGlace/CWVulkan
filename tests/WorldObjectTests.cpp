#include "NativeWorldObjects.h"
#include "NativeInteract.h"
#include "NativePose.h"
#include "Spawned.h"
#include "Extracted.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <utility>

namespace {
constexpr int64_t BLOCK = 65536;
template <class T> void put(uint8_t *record, size_t offset, T value) {
    std::memcpy(record + offset, &value, sizeof value);
}
template <class T> T get(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
struct Actor {
    std::array<uint8_t, nativepose::CREATURE_SIZE> record{};
    nativeai::Creature side;
    Actor(int64_t id, uint8_t kind, int64_t x = 0, int64_t y = 0, int64_t z = 0) {
        side.record = record.data();
        put(record.data(), worldgen::creature::ID, id);
        record[0x60] = kind;
        put(record.data(), 0x10, x);
        put(record.data(), 0x18, y);
        put(record.data(), 0x20, z);
        put(record.data(), 0x180, 100.0f);
    }
};
nativeai::Object object(int type, uint8_t state, float timer) {
    nativeai::Object result{};
    put(result.data(), 0, int32_t(type));
    result[nativeinteract::OBJECT_STATE] = state;
    put(result.data(), nativeinteract::OBJECT_SWING, timer);
    return result;
}
struct Scene {
    nativeobjects::World world;
    std::vector<nativeai::Creature *> actors;
    std::vector<nativeupkeep::Hit> hits;
    std::vector<soundrules::Request> sounds;
    std::vector<std::string> order;
    std::vector<int> draws;
    size_t randomCalls = 0;
    int changes = 0;
    Scene() {
        world.creatures = &actors;
        world.maxHealth = [](const nativeai::Creature &) { return 100.0f; };
        world.hit = [&](nativeai::Creature &, const nativeupkeep::Hit &hit) {
            hits.push_back(hit);
            order.push_back("hit");
        };
        world.rand = [&] {
            order.push_back("rand");
            const size_t index = randomCalls++;
            return index < draws.size() ? draws[index] : 0;
        };
        world.sound = [&](const soundrules::RequestRecord &record) {
            sounds.push_back(soundrules::readRequest(record));
            order.push_back("sound" + std::to_string(sounds.back().number));
        };
        world.objectsChanged = [&] {
            ++changes;
            order.push_back("changed");
        };
    }
};

void writeFixture(const std::filesystem::path &root, uint8_t type) {
    std::filesystem::create_directories(root);
    std::ofstream(root / "manifest.txt") << "seed=123\nhomeX=0\nhomeY=0\nhomeZ=1\n";
    std::ofstream out(root / "z_0_0.cwz", std::ios::binary);
    out.write("CWZ1", 4);
    const int32_t header[] = {0, 0, 0, 0};
    out.write(reinterpret_cast<const char *>(header), sizeof header);
    for (int i = 0; i < 4096; ++i) {
        const int32_t base = 0;
        const uint32_t runs = 1;
        const uint16_t count = 1;
        const uint8_t block[] = {50, 60, 70, 0, type, 0};
        out.write(reinterpret_cast<const char *>(&base), sizeof base);
        out.write(reinterpret_cast<const char *>(&runs), sizeof runs);
        out.write(reinterpret_cast<const char *>(&count), sizeof count);
        out.write(reinterpret_cast<const char *>(block), sizeof block);
    }
}
} // namespace

int worldObjectTests() {
    int failures = 0;
    auto check = [&](bool result, const char *message) {
        if (!result) {
            std::printf("FAIL World objects: %s\n", message);
            ++failures;
        }
    };
    {
        Scene scene;
        Actor npc(2, 1), deadPlayer(3, 0, -281474976710656LL, -281474976710656LL), other(4, 2);
        scene.actors = {&npc.side, &deadPlayer.side, &other.side};
        put(deadPlayer.record.data(), 0x180, 0.0f);
        for (auto *actor : scene.actors)
            put(actor->record, 0x128, uint16_t(0xffff));
        std::vector<nativeobjects::Cell> lookups;
        nativeobjects::collectZones(scene.world, [&](auto key) {
            check(get<uint16_t>(other.record.data(), 0x128) == 0xdffd, "all flags clear before zone lookup begins");
            lookups.push_back(key);
        });
        check(lookups.size() == 49 && lookups.front() == nativeobjects::Cell{-4, -4} && lookups[1] == nativeobjects::Cell{-4, -3} &&
                  lookups.back() == nativeobjects::Cell{2, 2},
              "dead kind0 player selects native narrowed7x7 keys in X/Y lookup order");
        check(get<uint16_t>(npc.record.data(), 0x128) == 0xdffd, "nonplayers also clear0x2002");
        scene.actors = {&npc.side};
        lookups.clear();
        nativeobjects::collectZones(scene.world, [&](auto key) { lookups.push_back(key); });
        check(lookups.empty(), "nonplayers do not select zones");
    }
    {
        Scene scene;
        Actor victim(2, 2, 3 * BLOCK);
        scene.actors = {&victim.side};
        scene.world.clockMs = 199.75f;
        scene.draws = {0, 32767};
        scene.world.hit = [&](nativeai::Creature &, const nativeupkeep::Hit &hit) {
            scene.hits.push_back(hit);
            scene.order.push_back("hit");
            scene.world.rand(); // Shared hit callbacks can consume their own random draws.
            soundrules::Request nested;
            nested.number = 0x1b;
            scene.world.sound(soundrules::makeRequest(nested));
        };
        auto trap = object(0x12, 1, 1000);
        nativeobjects::step(scene.world, trap, 0.5f);
        check(scene.hits.size() == 1 && scene.randomCalls == 2 && scene.sounds.size() == 2,
              "fractional forward clock crossing invokes hit and then direct sound draw");
        check(scene.order == std::vector<std::string>{"hit", "rand", "sound27", "rand", "sound1"},
              "nested hit/death outputs precede direct impact sound without replay");
        if (!scene.hits.empty()) {
            const auto &hit = scene.hits[0];
            check(hit.target == 2 && hit.attacker == 0 && hit.type == 0 && hit.last == 1 && hit.damage == 10 && hit.push[0] == 20 &&
                      hit.push[1] == 0 && hit.push[2] == 5 && hit.critical == 0 && hit.flag == 0 && hit.charge == 0 && hit.ability == 0 &&
                      hit.extra == 0,
                  "trap uses specified null-attacker hit parameters and horizontal push");
        }
        check(scene.sounds.size() == 2 && scene.sounds.back().number == 1 && scene.sounds.back().pitch == 1.3f &&
                  scene.sounds.back().volume == 1 && scene.sounds.back().owner == 0,
              "impact pitch uses post-hit draw and native0.3 multiply/divide/add");
    }
    {
        Scene scene;
        Actor edge(2, 2, 5 * BLOCK), inside(3, 3, 5 * BLOCK - 1), diagonal(4, 2, 3 * BLOCK, 4 * BLOCK);
        scene.actors = {&edge.side, &inside.side, &diagonal.side};
        scene.world.clockMs = 199;
        auto trap = object(0x11, 1, 1000);
        nativeobjects::step(scene.world, trap, 1);
        check(scene.hits.size() == 1 && scene.hits[0].target == 3 && scene.hits[0].type == 1,
              "strict nominal5-block test rejects axis and diagonal equality but accepts one dot inside");
        check(scene.sounds.empty() && scene.randomCalls == 0, "type11 has no direct impact sound/random draw");
    }
    {
        Scene scene;
        Actor activator(2, 0, 19 * BLOCK), dead(3, 2, BLOCK), friendly(4, 1), appended(5, 3, -BLOCK);
        put(activator.record.data(), 0x180, 0.0f);
        put(dead.record.data(), 0x180, 0.0f);
        scene.actors = {&activator.side, &dead.side, &friendly.side};
        scene.world.hit = [&](nativeai::Creature &target, const nativeupkeep::Hit &hit) {
            scene.hits.push_back(hit);
            scene.order.push_back("hit");
            if (&target == &dead.side)
                scene.actors.push_back(&appended.side);
            put(target.record, 0x10, int64_t(7 * BLOCK));
        };
        auto trap = object(0x13, 0, 2001);
        nativeobjects::step(scene.world, trap, 0);
        check(scene.changes == 1 && trap[nativeinteract::OBJECT_STATE] == 1 && get<float>(trap.data(), nativeinteract::OBJECT_SWING) == 0,
              "dead kind0 actor can activate a trap after strict2000ms");
        check(scene.hits.size() == 2 && scene.hits[0].target == 3 && scene.hits[1].target == 5,
              "hit tail skips only kind1; includes dead targets and callback tail births in registration order");
        check(!scene.hits.empty() && scene.hits[0].damage == 50 && scene.hits[0].push[0] == 40 && scene.hits[0].push[2] == 10,
              "type13 applies sequential maxHP fraction and doubled push");
        check(scene.order.size() >= 2 && scene.order[0] == "sound57" && scene.order[1] == "changed",
              "state-change sound and dirty notice precede trap hits");
        check(scene.sounds.size() == 3 && scene.sounds[1].position[0] == 7 * BLOCK && scene.sounds[2].position[0] == 7 * BLOCK,
              "impact sounds copy each target position after its hit callback");
    }
    {
        Scene scene;
        Actor target(2, 2);
        scene.actors = {&target.side};
        auto trap = object(0x13, 1, 0);
        nativeobjects::step(scene.world, trap, 0);
        nativeobjects::step(scene.world, trap, 0);
        check(scene.hits.size() == 2, "active type13 zero-time visits have no synthetic consumed-hit latch");
        nativeobjects::step(scene.world, trap, 0.25f);
        check(scene.hits.size() == 2, "positive timer advance precedes type13 exact-zero test");
        auto periodic = object(0x12, 1, 1000);
        scene.world.clockMs = 200;
        nativeobjects::step(scene.world, periodic, 0);
        check(scene.hits.size() == 2, "periodic traps do not fire at zero interval");
    }
    {
        Scene scene;
        Actor player(2, 0, 20 * BLOCK);
        scene.actors = {&player.side};
        auto trap = object(0x13, 0, 2001);
        nativeobjects::step(scene.world, trap, 0);
        check(scene.changes == 0, "activation rejects exact20-block distance");
        auto threshold = object(0x13, 1, 2000);
        nativeobjects::step(scene.world, threshold, 0);
        check(scene.changes == 0, "automatic transition rejects exact2000ms");
        nativeobjects::step(scene.world, threshold, 0.25f);
        check(scene.changes == 1 && threshold[nativeinteract::OBJECT_STATE] == 0, "nonzero state returns to state0 after strict threshold");
        put(player.record.data(), 0x10, int64_t(1) << 40);
        nativeobjects::step(scene.world, trap, 0);
        check(scene.changes == 2 && scene.hits.size() == 1,
              "distance keeps low64 squares and per-axis division rather than substituting a float sphere");
    }
    {
        Scene scene;
        Actor actor(2, 0);
        scene.actors = {&actor.side};
        scene.world.client = true;
        auto trap = object(0x13, 1, 2001);
        nativeobjects::step(scene.world, trap, 1);
        check(scene.hits.empty() && scene.changes == 0 && get<float>(trap.data(), nativeinteract::OBJECT_SWING) == 2002,
              "mirror advances clocks but does not cycle or hit traps");
        std::set<nativeobjects::Cell> used{{-1, 0}};
        scene.world.usedCells = &used;
        auto personal = object(0x16, 1, 1000);
        put(personal.data(), 8, -281474976710656LL);
        nativeobjects::step(scene.world, personal, 0);
        check(personal[nativeinteract::OBJECT_STATE] == 0 && get<float>(personal.data(), nativeinteract::OBJECT_SWING) == 0 &&
                  scene.sounds.empty() && scene.changes == 0,
              "mirror personal-state reset is silent and does not mark dirty");
    }
    {
        Scene scene;
        Actor actor(2, 2);
        scene.actors = {&actor.side};
        auto trap = object(0x12, 1, 1000);
        scene.world.clockMs = std::numeric_limits<float>::quiet_NaN();
        nativeobjects::step(scene.world, trap, 16);
        check(scene.hits.empty(), "invalid SSE clock conversions share the indefinite bucket");
        scene.world.clockMs = 2147483520.0f;
        nativeobjects::step(scene.world, trap, 128);
        check(scene.hits.size() == 1, "future clock conversion uses signed32 indefinite instead of saturation");
    }
    {
        Scene scene;
        Actor actor(2, 2);
        scene.actors = {&actor.side};
        const uint32_t healthBits = 0x3fa0013e;
        float health;
        std::memcpy(&health, &healthBits, sizeof health);
        scene.world.maxHealth = [=](const nativeai::Creature &) { return health; };
        auto trap = object(0x13, 1, 0);
        nativeobjects::step(scene.world, trap, 0);
        uint32_t damageBits = 0;
        if (!scene.hits.empty())
            std::memcpy(&damageBits, &scene.hits[0].damage, sizeof damageBits);
        check(damageBits == 0x3f20013f, "type13 retains separate0.1 then5 rounding instead of folding to0.5");
    }
    {
        Scene scene;
        Actor player(2, 0);
        scene.actors = {&player.side};
        auto first = object(0x13, 0, 2001);
        auto second = object(3, 1, 100);
        bool beforeNextClock = false;
        scene.world.hit = [&](nativeai::Creature &, const nativeupkeep::Hit &) {
            beforeNextClock = get<float>(second.data(), nativeinteract::OBJECT_SWING) == 100;
        };
        nativeobjects::step(scene.world, first, 16);
        nativeobjects::step(scene.world, second, 16);
        check(beforeNextClock && get<float>(second.data(), nativeinteract::OBJECT_SWING) == 116,
              "each object's state/hit callbacks finish before the next object's clock advances");
    }
    {
        // Actual facade: a loaded zone's native object list, local actor, hit and event queues.
        const auto root = std::filesystem::temp_directory_path() /
                          ("CubeVulkan-world-objects-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        writeFixture(root, 1);
        {
            ExtractedWorld terrain;
            std::string why;
            check(terrain.open(root, why), "facade fixture opens");
            SpawnedCreatures actors;
            actors.setNativePlayer(true);
            actors.setPlayer(terrain, {1, 1, ExtractedWorld::Z_OFFSET + 1}, 0, {}, 100, 1, true);
            actors.setPlayerHealth(actors.playerMaxHealth());
            const float before = actors.playerHealth();
            auto trap = object(0x13, 1, 0);
            std::memcpy(trap.data() + 8, actors.localRecord() + 0x10, 24);
            cwnet::ZoneObjects objects;
            objects.key = {0, 0};
            objects.records.push_back(trap);
            actors.replaceStaticObjects(terrain, objects);
            actors.simulate(terrain, 0);
            check(actors.playerHealth() < before && !actors.events().damageNumbers.empty(),
                  "facade zero-time trap reaches actual local-player hit/health/event path");
            const auto first = actors.takeSounds();
            check(!first.empty() && actors.takeSounds().empty(), "facade delivers trap sounds once to local audio");
            check(first == actors.events().sounds, "local audio and outgoing world retain the same trap/hit sound order");
            hudcontext::Tabs inventory(3);
            const auto requirement = nativesaves::makeItem(22, 7);
            inventory[1].push_back({3, requirement});
            actors.setHudContextSource(true, &inventory);
            auto required = object(0xfe, 1, 0);
            std::copy(requirement.begin(), requirement.end(), required.begin() + 0x38);
            objects.records = {required};
            actors.replaceStaticObjects(terrain, objects);
            actors.simulate(terrain, 0);
            check(actors.hudContext().item == requirement && actors.hudContext().count == 3,
                  "actual zero-time World facade derives the context from loaded requirements and canonical inventory");
            inventory[1][0].count = 0;
            check(actors.hudContext().count == 3, "inventory edits retain the prior snapshot until a World rebuild");
            actors.simulate(terrain, 0);
            check(actors.hudContext().item[0] == 0 && actors.hudContext().count == 0,
                  "the next zero-time World pass clears the unavailable selected requirement");
        }
        {
            ExtractedWorld terrain;
            std::string why;
            check(terrain.open(root, why), "legacy facade fixture opens");
            SpawnedCreatures actors;
            actors.setNativePlayer(false);
            actors.setPlayerId(77);
            actors.setPlayer(terrain, {1, 1, ExtractedWorld::Z_OFFSET + 1}, 0, {}, 100, 1, true);
            int64_t hitTarget = -1;
            actors.setPlayerHit([&](const nativeupkeep::Hit &hit, const uint8_t *attacker) {
                hitTarget = hit.target;
                check(attacker == nullptr, "legacy trap callback keeps null attacker");
                return false;
            });
            auto trap = object(0x13, 1, 0);
            std::memcpy(trap.data() + 8, actors.localRecord() + 0x10, 24);
            cwnet::ZoneObjects objects;
            objects.key = {0, 0};
            objects.records.push_back(trap);
            actors.replaceStaticObjects(terrain, objects);
            actors.simulate(terrain, 0);
            check(hitTarget == 77 && !actors.takeSounds().empty(),
                  "actual legacy bridge receives native record ID and emits impact even for a rejected hit");
        }
        writeFixture(root, 17);
        {
            ExtractedWorld terrain;
            std::string why;
            check(terrain.open(root, why), "lava fixture opens");
            SpawnedCreatures actors;
            actors.setNativePlayer(true);
            actors.setPlayer(terrain, {1, 1, ExtractedWorld::Z_OFFSET + 1}, 0, {}, 100, 1, true);
            actors.setPlayerHealth(actors.playerMaxHealth());
            const float before = actors.playerHealth();
            actors.simulate(terrain, 200);
            check(actors.elapsedWorldTime() == 200 && actors.playerHealth() < before && !actors.events().damageNumbers.empty(),
                  "facade physics receives the advanced world clock for200ms lava damage");
        }
        std::filesystem::remove_all(root);
    }
    std::printf("World object clocks, selection, trap state/hits/sounds and facade timing: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
