// Independent steam-gameplay-frame-order.md §§4/6, without game assets.
#include "NativeControl.h"
#include "NativeRules.h"
#include "Spawned.h"
#include "Extracted.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
using Record = std::array<uint8_t, 0x2000>;
template <class T> void put(Record &record, size_t offset, T value) {
    std::memcpy(record.data() + offset, &value, sizeof value);
}
template <class T> T get(const Record &record, size_t offset) {
    T value;
    std::memcpy(&value, record.data() + offset, sizeof value);
    return value;
}
void setup(Record &record, nativeai::Creature &actor) {
    record.fill(0);
    actor.record = record.data();
    put<float>(record, 0x180, 100);
    put<float>(record, 0x9a0, 1);
    put<float>(record, 0x6c, 100000);
    put<uint32_t>(record, 0x5c, 1);
    put<float>(record, 0x84, 1);
    put<float>(record, 0x88, 1);
    put<float>(record, 0x8c, 2);
}
} // namespace

int controlPhaseTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL Control phases: %s\n", what);
            ++failures;
        }
    };
    {
        Record record;
        nativeai::Creature actor;
        setup(record, actor);
        nativecontrol::Input input;
        input.lamp = input.pet = input.ultimate = input.typing = true;
        nativecontrol::State state;
        nativecontrol::World world;
        check(!nativecontrol::beginFrame(input, state, actor, 0, world), "active chat stops the post-movement input stages");
        check(state.lastLamp == 0 && state.lastPet == 0 && state.lastUltimate == 0, "skipped stages retain their edge bytes");
    }
    {
        Record record;
        nativeai::Creature actor;
        setup(record, actor);
        nativecontrol::Input input;
        input.lamp = input.pet = input.ordinaryUi = true;
        nativecontrol::State state;
        nativecontrol::World world;
        int sounds = 0;
        world.playSound = [&](int id) {
            check(id == 0x7c, "pet call uses its native sound ID");
            ++sounds;
        };
        check(nativecontrol::beginFrame(input, state, actor, 0, world),
              "ordinary UI does not skip early movement/lamp/ultimate opportunity");
        check(state.lastLamp == 1 && state.lastPet == 0 && actor.interactions.empty(), "begin does not run the later pet stage");
        // A direct Use between early input and pet replaces the current ability/timer.
        record[0x68] = 0x80;
        put<float>(record, 0x6c, 0);
        nativecontrol::pet(input, state, actor, world);
        check(record[0x68] == 0x80 && actor.interactions.empty() && state.lastPet == 1,
              "pet re-reads current post-Use ability rather than early snapshot");
        put<float>(record, 0x6c, 100000);
        nativecontrol::pet(input, state, actor, world);
        check(actor.interactions.empty(), "held pet does not retry when the current ability later finishes");
        input.pet = false;
        nativecontrol::pet(input, state, actor, world);
        input.pet = true;
        nativecontrol::pet(input, state, actor, world);
        check(record[0x68] == 0x9b && get<float>(record, 0x6c) == 0 && record[0x1d8] == 0x21 && actor.interactions.size() == 1 &&
                  sounds == 1,
              "fresh post-Use pet edge calls pet even under ordinary UI");
        input.attack = true;
        nativecontrol::attack(input, state, actor);
        check(state.lastAttack == 0, "late ordinary-UI gate preserves previous attack byte");
        input.ordinaryUi = false;
        nativecontrol::attack(input, state, actor);
        check(state.lastAttack == 1, "late attack edge updates when its own gate is reached");
    }
    {
        Record record, owner;
        nativeai::Creature actor, ownerActor;
        setup(record, actor);
        setup(owner, ownerActor);
        record[0x68] = 0x9a;
        put<int64_t>(record, 0x1a0, 7);
        put<float>(owner, 0x180, -1); // This branch does not filter a resolved owner's health.
        put<int64_t>(owner, 0xb10, 5 * 65536);
        put<int64_t>(owner, 0xb18, 4 * 65536);
        put<int64_t>(owner, 0xb20, 3 * 65536);
        nativecontrol::World world;
        world.byId = [&](int64_t id) -> const uint8_t * { return id == 7 ? owner.data() : nullptr; };
        const float duration = float(nativerules::abilityDuration(record.data(), [](int) { return false; }));
        put<float>(record, 0x6c, duration - 1);
        nativecontrol::ownerLook(actor, world);
        check(get<float>(record, 0x174) == 5 && get<float>(record, 0x178) == 4 && get<float>(record, 0x17c) == 3 &&
                  (get<uint16_t>(record, 0x128) & 4),
              "owner look reads drawn owner minus physical player without health filter");
        put<float>(record, 0x6c, duration);
        put<float>(record, 0x174, 9);
        nativecontrol::ownerLook(actor, world);
        check(!(get<uint16_t>(record, 0x128) & 4) && get<float>(record, 0x174) == 9,
              "owner duration equality clears mask but preserves old vector");
    }
    for (int distance : {5, 6}) {
        Record record, pet;
        nativeai::Creature actor, petActor;
        setup(record, actor);
        setup(pet, petActor);
        put<int64_t>(record, 0x9d8, 2);
        put<int64_t>(pet, 0x10, int64_t(distance) * 65536);
        nativecontrol::Input input;
        input.pet = true;
        nativecontrol::State state;
        nativecontrol::World world;
        world.byId = [&](int64_t id) -> const uint8_t * { return id == 2 ? pet.data() : nullptr; };
        world.ownsSpecialItem = [](int id) { return id == 3; };
        nativecontrol::pet(input, state, actor, world);
        check(record[0x68] == (distance == 5 ? 0x76 : 0x9b), "pet mounting uses strict squared-distance below 36");
        if (distance == 5)
            check(get<float>(record, 0x6c) == 100000, "mount does not reset ability timer");
    }
    {
        Record record;
        nativeai::Creature actor;
        setup(record, actor);
        record[0x68] = 0x76;
        const float duration = float(nativerules::abilityDuration(record.data(), [](int) { return false; }));
        put<float>(record, 0x6c, duration);
        nativecontrol::Input input;
        input.pet = true;
        nativecontrol::State state;
        nativecontrol::World world;
        nativecontrol::pet(input, state, actor, world);
        check(record[0x68] == 0x76 && state.lastPet == 1, "duration equality rejects dismount but stores pet edge");
        put<float>(record, 0x6c, duration + 100);
        nativecontrol::pet(input, state, actor, world);
        check(record[0x68] == 0x76, "held edge does not dismount after equality rejection");
        input.pet = false;
        nativecontrol::pet(input, state, actor, world);
        input.pet = true;
        nativecontrol::pet(input, state, actor, world);
        check(record[0x68] == 0 && get<float>(record, 0x6c) == duration + 100, "new finished edge dismounts without timer reset");
    }
    { // Cached ray inputs are evaluated only after the movement/early-return region.
        Record record;
        nativeai::Creature actor;
        setup(record, actor);
        put<int64_t>(record, 0x20, 20 * 65536);
        nativecontrol::Input input;
        input.nativeAim.emplace();
        input.nativeAim->rayOrigin[2] = 8 * 65536;
        input.nativeAim->forward[2] = 1;
        input.nativeAim->wallDistance = 60;
        input.nativeAim->aimDepth = 20; // player-relative dot0 rejects this snap
        input.hasAim = true;
        input.aimAt[2] = -999; // an old prepared point must not win
        nativecontrol::State state;
        nativecontrol::World world;
        check(nativecontrol::beginFrame(input, state, actor, 0, world), "cached native aim reaches its named stage");
        check(get<float>(record, 0x17c) == 48, "working aim uses current physical player instead of stale precomputed point");
        input.typing = true;
        put<float>(record, 0x17c, 9);
        check(!nativecontrol::beginFrame(input, state, actor, 0, world) && get<float>(record, 0x17c) == 9,
              "early chat return does not construct or apply cached aim");
    }
    { // Production facade: local lookup, current ownership/sound hooks and buff transport.
        ExtractedWorld blocks;
        SpawnedCreatures creatures;
        creatures.setNativePlayer(true);
        creatures.setPlayer(blocks, {0, 0, 130}, 0, {0, 0, 0}, 100, 1, true);
        auto *actor = creatures.localPlayer();
        check(actor != nullptr, "facade creates a native local player without assets");
        if (actor) {
            actor->record[0x14c] = 4;
            actor->record[0x14d] = 1; // rogue Ninjutsu
            const float ready = 100000, health = 100;
            std::memcpy(actor->record + 0x6c, &ready, sizeof ready);
            std::memcpy(actor->record + 0x180, &health, sizeof health);
            const int64_t localPet = creatures.playerId();
            std::memcpy(actor->record + 0x9d8, &localPet, sizeof localPet);
            int ownershipQuery = 0, sound = 0;
            creatures.setControlHooks(
                [&](int id) {
                    ownershipQuery = id;
                    return false;
                },
                [&](int id) { sound = id; });
            nativecontrol::Input input;
            input.ultimate = input.ordinaryUi = true;
            check(creatures.prepareControl(input, 0), "facade admits early ultimate under ordinary UI");
            check(!creatures.events().buffs.empty(), "early ultimate buff notices reach outgoing world events");
            input.pet = true;
            creatures.controlPet(input);
            check(ownershipQuery == 3 && sound == 0x7c && actor->record[0x68] == 0x9b,
                  "facade resolves local ID and invokes current ownership/sound hooks");
        }
    }
    { // World registration is independent of zone storage and signed actor IDs.
        SpawnedCreatures creatures;
        ExtractedWorld blocks;
        creatures.setNativePlayer(true);
        bool made = false;
        creatures.networkCreature(42, made);
        creatures.setPlayer(blocks, {0, 0, 130}, 0, {0, 0, 0}, 100, 1, true);
        creatures.networkCreature(-7, made);
        auto order = [&] {
            std::vector<int64_t> ids;
            creatures.forEachWorldCreature([&](const uint8_t *record) {
                int64_t id;
                std::memcpy(&id, record + 8, sizeof id);
                ids.push_back(id);
            });
            return ids;
        };
        check(order() == std::vector<int64_t>{42, 1, -7}, "world visit order follows registration rather than ID or local-first grouping");
        creatures.networkCreature(42, made);
        check(!made && order() == std::vector<int64_t>{42, 1, -7}, "updating a registered creature preserves its list position");
        creatures.remove(42);
        creatures.networkCreature(42, made);
        check(made && order() == std::vector<int64_t>{1, -7, 42}, "removed and recreated creature rejoins at the tail");
        creatures.resetPlayer();
        creatures.setPlayer(blocks, {0, 0, 130}, 0, {0, 0, 0}, 100, 1, true);
        check(order() == std::vector<int64_t>{-7, 42, 1}, "recreated local player also rejoins at the tail");
    }
    { // The zero residual still runs the global clear, without an outer loaded-zone gate.
        SpawnedCreatures creatures;
        ExtractedWorld unloaded;
        creatures.setRemoteClient(true);
        creatures.setNativePlayer(true);
        creatures.setPlayer(unloaded, {0, 0, 130}, 0, {0, 0, 0}, 0, 1, true);
        bool made = false;
        auto *remote = creatures.networkCreature(42, made);
        const float dead = 0;
        const uint16_t flags = 0x10;
        std::memcpy(remote + 0x180, &dead, sizeof dead);
        std::memcpy(remote + 0x128, &flags, sizeof flags);
        auto *local = creatures.localPlayer()->record;
        std::memcpy(local + 0x180, &dead, sizeof dead);
        std::memcpy(local + 0x128, &flags, sizeof flags);
        creatures.simulate(unloaded, 0);
        uint16_t localFlags, remoteFlags;
        std::memcpy(&localFlags, local + 0x128, sizeof localFlags);
        std::memcpy(&remoteFlags, remote + 0x128, sizeof remoteFlags);
        check(!(localFlags & 0x10) && !(remoteFlags & 0x10),
              "zero-time World pass clears shared state on dead local/remote records without loaded terrain");
    }
    { // World interpolation admits local and dead remote records before visible drawing.
        SpawnedCreatures creatures;
        ExtractedWorld blocks;
        creatures.setNativePlayer(true);
        creatures.setPlayer(blocks, {0, 0, 130}, 0, {0, 0, 0}, 100, 1, true);
        bool made;
        auto *remote = creatures.networkCreature(42, made);
        auto *local = creatures.localPlayer()->record;
        for (auto *record : {local, remote}) {
            const std::array<int64_t, 3> raw{65536, 65536, 65536}, drawn{};
            const float positive = 1, zero = 0;
            std::memcpy(record + 0x10, raw.data(), sizeof raw);
            std::memcpy(record + 0xb10, drawn.data(), sizeof drawn);
            std::memcpy(record + 0xb58, &positive, sizeof positive);
            std::memcpy(record + 0x180, &zero, sizeof zero);
            std::memcpy(record + 0x6c, &zero, sizeof zero);
        }
        creatures.smooth(16);
        for (auto *record : {local, remote}) {
            int64_t x, z;
            float timer, control;
            std::memcpy(&x, record + 0xb10, sizeof x);
            std::memcpy(&z, record + 0xb20, sizeof z);
            std::memcpy(&timer, record + 0x6c, sizeof timer);
            std::memcpy(&control, record + 0xb58, sizeof control);
            check(x == 17143 && z == 29801 && timer == 8.37070083618164f && control == 1,
                  "actual World facade applies native interpolation to local and dead entries without visibility or countdown gates");
        }
    }
    { // All interpolation visits must finish before the first linked-placement visit.
        SpawnedCreatures creatures;
        ExtractedWorld blocks;
        creatures.setNativePlayer(true);
        creatures.setPlayer(blocks, {0, 0, 130}, 0, {0, 0, 0}, 100, 1, true);
        bool made;
        auto *target = creatures.networkCreature(42, made);
        auto *carrier = creatures.localPlayer()->record;
        carrier[0x68] = 0x80;
        const int64_t owner = 42;
        const float negativeStep = -1, positive = 1, zero = 0, health = 100;
        const uint32_t carriable = 0x4000;
        std::memcpy(carrier + 0x1a0, &owner, sizeof owner);
        std::memcpy(carrier + 0x990, &negativeStep, sizeof negativeStep);
        std::memcpy(carrier + 0xb58, &zero, sizeof zero);
        std::memcpy(carrier + 0x180, &zero, sizeof zero); // no outer carrier HP gate
        std::memcpy(target + 0x180, &health, sizeof health);
        std::memcpy(target + 0x80, &carriable, sizeof carriable);
        std::memcpy(target + 0xb58, &positive, sizeof positive);
        std::memcpy(target + 0x990, &zero, sizeof zero);
        creatures.smooth(16);
        float step;
        std::memcpy(&step, target + 0x990, sizeof step);
        check(step == -1, "later target interpolation cannot clamp the negative step copied by the already completed linked-pose pass");
    }
    std::printf("Control phases: %s\n", failures ? "FAILED" : "passed");
    return failures;
}
