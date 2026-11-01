#include "NativeDeadCreatures.h"
#include "NativePose.h"
#include "Spawned.h"
#include "Extracted.h"
#include "worldgen/Residents.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>

namespace {
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
    Actor(int64_t id, uint8_t kind, float health, float clock = 0, int64_t owner = 0) {
        side.record = record.data();
        put(record.data(), worldgen::creature::ID, id);
        record[0x60] = kind;
        put(record.data(), 0x180, health);
        put(record.data(), 0x6c, clock);
        put(record.data(), 0x1a0, owner);
    }
};
struct Scene {
    nativedeath::World world;
    std::vector<nativeai::Creature *> actors;
    std::set<int64_t> dead;
    Scene() {
        world.creatures = &actors;
        world.maxHealth = [](const nativeai::Creature &) { return 123.25f; };
        world.byId = [&](int64_t id) -> nativeai::Creature * {
            for (auto *actor : actors)
                if (get<int64_t>(actor->record, worldgen::creature::ID) == id)
                    return actor;
            return nullptr;
        };
        world.markDead = [&](nativeai::Creature &actor) { dead.insert(get<int64_t>(actor.record, worldgen::creature::ID)); };
    }
};
} // namespace

int deadCreatureTests() {
    int failures = 0;
    auto check = [&](bool result, const char *message) {
        if (!result) {
            std::printf("FAIL dead creatures: %s\n", message);
            ++failures;
        }
    };
    {
        Actor first(90, 2, 0, 999.75f), second(3, 1, -5, 1000), player(1, 0, 0, 30000), alive(7, 2, 1, 5000);
        Scene scene;
        scene.actors = {&first.side, &second.side, &player.side, &alive.side};
        for (auto *actor : scene.actors) {
            actor->aggro = {{90, 2.0f}, {3, 4.0f}, {123, 6.0f}};
            actor->damage = actor->aggro;
            actor->counted = {90, 3};
            actor->struck = {90, 3};
        }
        const auto retired = nativedeath::step(scene.world, 0.25f);
        check(retired == std::vector<int64_t>{90, 3}, "retirement follows registration order rather than sorted IDs");
        check(scene.actors.size() == 4 && scene.dead == std::set<int64_t>{3, 90},
              "all sources visited before caller deletion; players/alive entries excluded");
        check(get<float>(first.record.data(), 0x6c) == 1000 && get<float>(second.record.data(), 0x6c) == 1000.25f,
              "dead clock advances in float milliseconds before inclusive1000ms test");
        check(get<float>(player.record.data(), 0x6c) == 30000 && get<float>(alive.record.data(), 0x6c) == 5000,
              "dead player and living actor clocks remain outside this pass");
        for (auto *actor : scene.actors)
            check(actor->aggro == std::map<int64_t, float>{{123, 6.0f}} && actor->damage == actor->aggro &&
                      actor->counted == std::set<int64_t>{3, 90} && actor->struck == actor->counted,
                  "retirement clears only aggro/damage references on every current actor");
    }
    {
        Actor corpse(2, 2, 0, 999.5f);
        Scene scene;
        scene.actors = {&corpse.side};
        check(nativedeath::step(scene.world, 0.25f).empty(), "fractional clock below1000ms stays registered");
        check(nativedeath::step(scene.world, 0.25f) == std::vector<int64_t>{2}, "exact1000ms queues retirement");
        put(corpse.record.data(), 0x6c, 1000.0f);
        check(nativedeath::step(scene.world, 0) == std::vector<int64_t>{2}, "admitted zero interval still retires an old corpse");
    }
    {
        Actor owner(1, 0, 0), pet(2, 6, 0, 19999.75f, 1);
        const std::array<int64_t, 3> position{123, -456, 789};
        std::memcpy(owner.record.data() + worldgen::creature::POSITION, position.data(), sizeof position);
        put(pet.record.data(), 0x9e0, int64_t(999));
        put(pet.record.data(), 0x30, 17.0f);
        pet.side.aggro = {{9, 1.0f}};
        pet.side.damage = {{9, 2.0f}};
        pet.side.counted = {9};
        Scene scene;
        scene.actors = {&pet.side, &owner.side};
        check(nativedeath::step(scene.world, 0.25f).empty(), "owned kind6 revives instead of entering ordinary retirement");
        check(get<float>(pet.record.data(), 0x180) == 123.25f && get<float>(pet.record.data(), 0x6c) == 20000,
              "exact20000ms and owner HP zero admit max-health revival without clock reset");
        check(std::memcmp(pet.record.data() + worldgen::creature::POSITION, position.data(), sizeof position) == 0 &&
                  get<int64_t>(pet.record.data(), 0x9e0) == 0 && get<float>(pet.record.data(), 0x30) == 17,
              "revival copies physical XYZ and clears last foe without copying owner yaw");
        check(pet.side.aggro.empty() && pet.side.damage == std::map<int64_t, float>{{9, 2.0f}} &&
                  pet.side.counted == std::set<int64_t>{9} && scene.dead.count(2),
              "revival keeps dead ID, damage and remembered targets while clearing only aggro");
    }
    {
        Actor orphan(2, 6, 0, 30000, 77), owner(77, 0, -1);
        Scene scene;
        scene.actors = {&orphan.side};
        check(nativedeath::step(scene.world, 0).empty() && get<float>(orphan.record.data(), 0x180) == 0,
              "missing owner does not fall back to one-second retirement");
        scene.actors.push_back(&owner.side);
        check(nativedeath::step(scene.world, 0).empty() && get<float>(orphan.record.data(), 0x180) == 0,
              "negative owner health prevents revival without deleting owned corpse");
        put(orphan.record.data(), 0x1a0, int64_t(0));
        check(nativedeath::step(scene.world, 0) == std::vector<int64_t>{2}, "ownerless kind6 uses ordinary retirement");
    }
    {
        Scene scene;
        Actor bomb(2, 8, 0), blast(3, 2, 0), bubble(4, 8, 0);
        scene.actors = {&bomb.side, &blast.side, &bubble.side};
        put(bomb.record.data(), 0x64, int32_t(0x90));
        put(blast.record.data(), 0x64, int32_t(0x119));
        put(bubble.record.data(), 0x64, int32_t(0x11d));
        check(nativedeath::step(scene.world, 5000).empty() && scene.dead.size() == 3,
              "special area sources enter dead IDs but await their separate burst/area implementation");
        for (auto *actor : scene.actors)
            check(get<float>(actor->record, 0x6c) == 0, "pending first-clock-zero area input is not consumed by lifecycle-only pass");
    }
    {
        Scene scene;
        Actor corpse(2, 2, 0, 1000);
        scene.actors = {&corpse.side};
        scene.world.client = true;
        check(nativedeath::step(scene.world, 16).empty() && scene.dead.empty() && get<float>(corpse.record.data(), 0x6c) == 1000,
              "mirror does not advance authoritative death lifecycle");
    }
    {
        // Real vector-backed facade: deleting several sources must refresh surviving views.
        ExtractedWorld terrain;
        SpawnedCreatures actors;
        struct Entry {
            int64_t id;
            float hp, clock;
        };
        const std::array<Entry, 4> entries{{{90, 0.0f, 1000.0f}, {3, 0.0f, 1000.0f}, {7, 1.0f, 0.0f}, {8, 0.0f, 999.75f}}};
        for (const auto &[id, hp, clock] : entries) {
            bool made = false;
            uint8_t *record = actors.networkCreature(id, made);
            check(record != nullptr && made, "facade actor registered");
            if (!record)
                continue;
            record[0x60] = 2;
            put(record, 0x180, hp);
            put(record, 0x6c, clock);
        }
        actors.simulate(terrain, 0);
        check(actors.record(90) == nullptr && actors.record(3) == nullptr && actors.record(7) && actors.record(8),
              "facade defers multiple erasures and retains shifted surviving actors");
        actors.simulate(terrain, 0.25);
        check(actors.record(8) == nullptr && actors.record(7), "next substep uses refreshed actor pointers after retirement");
        std::vector<int64_t> ids;
        actors.forEachWorldCreature([&](const uint8_t *record) { ids.push_back(get<int64_t>(record, worldgen::creature::ID)); });
        check(ids == std::vector<int64_t>{7}, "remaining World registration order and membership are coherent");
    }
    {
        ExtractedWorld terrain;
        SpawnedCreatures actors;
        bool made = false;
        uint8_t *pet = actors.networkCreature(10, made);
        check(pet != nullptr && made, "facade owned actor registered");
        if (pet) {
            pet[0x60] = 6;
            // This owned actor keeps independent equipment (pet contract section7.5).
            // Otherwise the later equipment pass correctly detaches an unlinked fixture.
            put(pet, 0x80, get<uint32_t>(pet, 0x80) | uint32_t(0x800));
            put(pet, 0x180, 0.0f);
            put(pet, 0x6c, 20000.0f);
            put(pet, 0x1a0, int64_t(20));
        }
        uint8_t *owner = actors.networkCreature(20, made);
        check(owner != nullptr && made, "facade owner registered");
        if (owner) {
            owner[0x60] = 0;
            put(owner, 0x180, 0.0f);
        }
        actors.simulate(terrain, 0);
        const uint8_t *revived = actors.record(10);
        check(revived && get<float>(revived, 0x180) > 0 && get<float>(revived, 0x6c) == 20000,
              "actual facade revives owned actor with dead-but-nonnegative owner at zero interval");
        check(actors.persistentDeadSet().count(10) != 0, "facade revival retains native dead ID");
    }
    std::printf("Ordinary corpse retirement and owned-kind6 revival: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
