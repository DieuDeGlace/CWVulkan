#include "NativeDeadCreatures.h"
#include "Extracted.h"
#include "NativePose.h"
#include "Spawned.h"
#include "worldgen/Residents.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
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
        check(nativedeath::step(scene.world, 5000) == std::vector<int64_t>{3, 4} && scene.dead.size() == 3,
              "non-terrain area sources complete lifecycle; destructive source stays deferred");
        check(get<float>(bomb.record.data(), 0x6c) == 0 && get<float>(blast.record.data(), 0x6c) == 5000 &&
                  get<float>(bubble.record.data(), 0x6c) == 5000,
              "only pending terrain-source first-clock-zero input is preserved");
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
    {
        Actor source(1, 1, 0), edge(2, 3, 1), near(3, 3, 1), high(4, 3, 1), rejected(5, 8, 1), faded(6, 3, 1);
        put(source.record.data(), 0x64, int32_t(0x119));
        put(source.record.data(), 0x184, -5.0f);
        put(edge.record.data(), 0x10, int64_t(8 * 65536));
        put(near.record.data(), 0x10, int64_t(3277));
        put(high.record.data(), 0x20, int64_t(8 * 65536));
        put(faded.record.data(), 0x12c, std::numeric_limits<float>::quiet_NaN());
        Scene scene;
        scene.actors = {&source.side, &edge.side, &near.side, &high.side, &rejected.side, &faded.side};
        scene.world.hostile = [](const auto &, const auto &) { return true; };
        int draws = 0;
        scene.world.rand = [&] {
            ++draws;
            return 0;
        };
        std::vector<nativeupkeep::Hit> hits;
        std::vector<nativedeath::World::Record> bursts;
        scene.world.hit = [&](const auto &hit) { hits.push_back(hit); };
        scene.world.burst = [&](const auto &burst) { bursts.push_back(burst); };
        nativedeath::step(scene.world, 0);
        check(hits.size() == 2 && hits[0].target == 2 && hits[1].target == 3 && draws == 3,
              "horizontal equality admitted, vertical equality/NaN fade excluded; kind8 draws then rejects");
        check(source.side.counted == std::set<int64_t>{2, 3, 5}, "remembered targets include late-rejected kind8");
        if (hits.size() == 2) {
            check(hits[0].attacker == 1 && hits[0].damage == -10 && hits[0].critical == 1 && hits[0].type == 1 && hits[0].flag == 1 &&
                      hits[0].last == 1 && hits[0].charge == 0 && hits[0].ability == 0,
                  "blast passes critical self-add, negative damage and exact shared-hit arguments");
            check(hits[0].push[0] == 10 && hits[0].push[2] == 2.5f && hits[1].push[0] == (3277.0f / 65536.0f) * 10.0f,
                  "large push normalized; small nonzero push deliberately retains its magnitude");
        }
        check(bursts.size() == 1 && get<int32_t>(bursts[0].data(), 0x38) == 20 && get<int32_t>(bursts[0].data(), 0x3c) == 1 &&
                  get<float>(bursts[0].data(), 0x20) == 20 && get<float>(bursts[0].data(), 0x24) == 1 &&
                  get<float>(bursts[0].data(), 0x28) == 0.2f && get<float>(bursts[0].data(), 0x34) == 0.25f &&
                  get<float>(bursts[0].data(), 0x40) == 3,
              "first blast burst carries specified initialized fields; native alpha/padding not oracle data");
        nativedeath::step(scene.world, 0);
        check(bursts.size() == 2 && hits.size() == 2 && draws == 3, "zero step repeats burst while remembered targets suppress redraw/hit");
    }
    {
        Actor source(1, 1, 0), target(2, 3, 1);
        put(source.record.data(), 0x64, int32_t(0x119));
        put(source.record.data(), 0x10, std::numeric_limits<int64_t>::max() - 32768);
        put(target.record.data(), 0x10, std::numeric_limits<int64_t>::min() + 32767);
        Scene scene;
        scene.actors = {&source.side, &target.side};
        scene.world.hostile = [](const auto &, const auto &) { return true; };
        bool hit = false;
        scene.world.hit = [&](const auto &h) { hit = h.target == 2 && h.push[0] == 10; };
        nativedeath::step(scene.world, 1);
        check(hit, "area distances preserve wrapped low64 subtraction before widening");
    }
    {
        Actor source(1, 1, 0), victim(2, 3, 1), chain(3, 1, 1), born(4, 3, 1);
        put(source.record.data(), 0x64, int32_t(0x119));
        put(chain.record.data(), 0x64, int32_t(0x119));
        Scene scene;
        scene.actors = {&source.side, &victim.side, &chain.side};
        scene.world.hostile = [](const auto &, const auto &) { return true; };
        std::vector<std::pair<int64_t, int64_t>> hits;
        int bursts = 0;
        scene.world.burst = [&](const auto &) { ++bursts; };
        scene.world.hit = [&](const auto &h) {
            hits.emplace_back(h.attacker, h.target);
            if (h.target == 2) {
                put(victim.record.data(), 0x180, 0.0f);
                put(chain.record.data(), 0x180, 0.0f);
                scene.actors.push_back(&born.side);
            }
        };
        nativedeath::step(scene.world, 1);
        check(hits == std::vector<std::pair<int64_t, int64_t>>{{1, 2}, {1, 4}, {3, 4}} && bursts == 2 && scene.dead.count(3),
              "live source/target walks include callback births and later actors killed earlier in this pass");
    }
    {
        Actor owner(1, 0, 100), source(2, 1, 0), friendActor(3, 6, 10), kind7(4, 7, 10), kind8(5, 8, 10), orphanTarget(6, 6, 10);
        put(owner.record.data(), 0x10, int64_t(100 * 65536));
        put(source.record.data(), 0x64, int32_t(0x11d));
        put(source.record.data(), 0x184, 10.0f);
        put(source.record.data(), 0x188, 2.0f);
        put(source.record.data(), 0x1a0, int64_t(1));
        Scene scene;
        scene.actors = {&owner.side, &source.side, &friendActor.side, &kind7.side, &kind8.side};
        scene.world.hostile = [&](const auto &a, const auto &) { return &a == &source.side; };
        int draws = 0, applies = 0, bursts = 0;
        scene.world.rand = [&] {
            ++draws;
            return 0;
        };
        std::vector<nativedeath::World::Record> numbers;
        scene.world.number = [&](const auto &number) { numbers.push_back(number); };
        scene.world.burst = [&](const auto &burst) {
            ++bursts;
            check(get<int32_t>(burst.data(), 0x3c) == 2 && get<float>(burst.data(), 0x28) == 0.5f, "water burst style/RGB");
        };
        scene.world.apply = [&](const auto &number) {
            ++applies;
            check(numbers.size() == 2 && numbers[0] == number && numbers[1] == number,
                  "both identical healing appends precede the single apply");
            check(get<int64_t>(number.data(), 0) == 1 && get<int64_t>(number.data(), 8) == 3 && get<float>(number.data(), 0x10) == -40 &&
                      number[0x14] == 1 && number[0x44] == 0 && number[0x45] == 0,
                  "healing notice uses owner, critical-adjusted negative damage and ordinary number bytes");
            put(friendActor.record.data(), 0x180, get<float>(friendActor.record.data(), 0x180) - get<float>(number.data(), 0x10));
        };
        nativedeath::step(scene.world, 0);
        check(applies == 1 && draws == 3 && source.side.counted == std::set<int64_t>{3, 4, 5},
              "friendly kind7 and kind8 consume critical/remember before rejecting");
        nativedeath::step(scene.world, 0);
        check(applies == 1 && draws == 3 && bursts == 2 && get<float>(friendActor.record.data(), 0x180) == 50,
              "repeat zero-time water burst does not heal remembered targets twice");
        put(source.record.data(), 0x1a0, int64_t(99));
        scene.actors.push_back(&orphanTarget.side);
        nativedeath::step(scene.world, 0);
        check(source.side.counted.contains(6) && draws == 4 && applies == 1, "missing water owner rejects only after draw/remember");
        put(source.record.data(), 0x1a0, int64_t(1));
        source.side.counted.erase(6);
        scene.world.hostile = [](const auto &, const auto &) { return true; };
        put(owner.record.data(), 0x180, -1.0f); // Area owner lookup has no lifecycle-style HP gate.
        nativeupkeep::Hit hit;
        scene.world.hit = [&](const auto &h) { hit = h; };
        nativedeath::step(scene.world, 0);
        check(hit.target == 6 && hit.attacker == 1 && hit.type == 2 && hit.damage == 40 && hit.flag == 1 && hit.last == 1,
              "hostile water target uses resolved negative-HP owner and hit type2");
        hit = {};
        source.side.counted.erase(6);
        put(source.record.data(), 0x188, 0.0f);
        const int before = draws;
        nativedeath::step(scene.world, 0);
        check(hit.target == 0 && source.side.counted.contains(6) && draws == before + 1,
              "nonpositive water damage rejects after critical draw and remembered insertion");
    }
    {
        ExtractedWorld terrain;
        SpawnedCreatures actors;
        bool made = false;
        uint8_t *source = actors.networkCreature(101, made);
        source[0x60] = 1;
        put(source, 0x64, int32_t(0x119));
        put(source, 0x180, 0.0f);
        put(source, 0x6c, 0.0f);
        put(source, 0x184, 20.0f);
        uint8_t *target = actors.networkCreature(102, made);
        target[0x60] = 3;
        put(target, 0x180, 100.0f);
        put(target, 0x12c, 0.0f);
        actors.simulate(terrain, 0);
        const uint8_t *after = actors.record(102);
        check(after && get<float>(after, 0x180) < 100 && !actors.events().damageNumbers.empty() && !actors.events().particles.empty(),
              "actual facade area uses shared hit health and outgoing number/burst queues");
        if (!actors.events().particles.empty())
            check(get<int32_t>(actors.events().particles.front().data(), 0x38) == 20 &&
                      get<int32_t>(actors.events().particles.front().data(), 0x3c) == 1,
                  "area burst precedes nested shared-hit particle output in common queue");
    }
    {
        ExtractedWorld terrain;
        SpawnedCreatures actors;
        bool made = false;
        uint8_t *owner = actors.networkCreature(200, made);
        owner[0x60] = 0;
        put(owner, 0x180, 100.0f);
        put(owner, 0x10, int64_t(100 * 65536));
        uint8_t *source = actors.networkCreature(201, made);
        source[0x60] = 1;
        put(source, 0x64, int32_t(0x11d));
        put(source, 0x180, 0.0f);
        put(source, 0x6c, 0.0f);
        put(source, 0x184, 10.0f);
        put(source, 0x188, 1.0f);
        put(source, 0x1a0, int64_t(200));
        uint8_t *target = actors.networkCreature(202, made);
        target[0x60] = 3;
        put(target, 0x180, 10.0f);
        put(target, 0x12c, 0.0f);
        const float maximum = nativerules::maxHealth(target);
        actors.simulate(terrain, 0);
        const auto &numbers = actors.events().damageNumbers;
        const uint8_t *after = actors.record(202);
        check(numbers.size() == 2 && numbers[0] == numbers[1], "facade retains both native water healing notices");
        if (numbers.size() == 2 && after) {
            const float healing = -get<float>(numbers[0].data(), 0x10);
            check(healing > 0 && get<float>(after, 0x180) == std::min(10.0f + healing, maximum),
                  "real shared apply heals once, capped by native maximum health");
        }
    }
    std::printf("Dead-creature lifecycle and non-terrain areas: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
