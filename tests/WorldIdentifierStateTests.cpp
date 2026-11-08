#include "Extracted.h"
#include "NetWorlds.h"
#include "Spawned.h"
#include "net/ClientGame.h"
#include "net/Transport.h"
#include <cstring>
#include <cstdio>
#include <limits>

int worldIdentifierStateTests() {
    using namespace cwnet;
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) { ++failures; std::printf("FAIL World identifiers: %s\n", message); }
    };
    ExtractedWorld blocks;
    blocks.openGenerated(0, 0);
    blocks.setClientStreaming(true);
    SpawnedCreatures actor;
    actor.setStoredZones(blocks, nullptr, true);
    actor.setRemoteClient(true);
    MirrorSide mirror(actor, blocks);
    constexpr PeerId machine = 1;
    LoopbackNetwork network;
    auto endpoint = network.endpoint(machine);
    ClientGame client(*endpoint, mirror);
    client.link().goHome(1000);
    actor.setClock(11, 100.f);
    actor.setWorldDead({17});
    actor.setWorldRewards({19, -21});
    check(blocks.loadSourceZone(0, 0), "fixture loads a real physical viewed source");
    const auto store = blocks.sourceStateStore();
    const auto source = store->find({0, 0});
    if (!source) return failures + 1;
    {
        auto guard = store->lock();
        store->retained().dead = {77};
        store->retained().zones[{8, 9}].deadTracked = 23;
        nativesaves::Record marker{}; marker[0] = 0x65;
        source->lists.groundItems = {marker};
        source->lists.groundItemsChanged = true;
        source->lists.deadTracked = 31;
    }
    bool made = false;
    auto *resident = actor.networkCreature(55, made);
    check(made && resident, "fixture registers an actual viewed resident");
    if (resident) {
        resident[worldgen::creature::HOSTILITY] = 1;
        const float health = 100;
        std::memcpy(resident + worldgen::creature::HEALTH, &health, sizeof health);
    }
    const auto send = [&](const Packet &packet, uint32_t now) {
        endpoint->send(machine, Channel::ToClient, encode(packet, Direction::ToClient));
        client.receive(now);
    };
    WorldEvents deaths;
    for (int64_t victim : {int64_t(0), int64_t(-99), std::numeric_limits<int64_t>::min(), int64_t(-99)}) {
        KillRecord record{};
        setField(record, 0, int64_t(1234)); // a distinct credit identifier, not the death-set key
        setField(record, 8, victim);
        deaths.kills.push_back(record);
    }
    send(WorldUpdate{deaths}, 1001);
    check(actor.worldDeadSet() == std::set<int64_t>{17}, "packet7 queues identifiers without applying them during receive");
    client.consumeWorldUpdates();
    const std::set<int64_t> expected{17, 0, -99, std::numeric_limits<int64_t>::min()};
    check(actor.worldDeadSet() == expected && actor.events().kills.size() == 4 && !actor.isWorldDead(1234),
          "application merges every signed victim ID and deduplicates membership without losing event records");
    client.consumeWorldUpdates();
    check(actor.worldDeadSet() == expected, "clearing consumed input lists does not erase already applied death memberships");
    int changes = 0;
    bool resetBeforeClock = false;
    mirror.dayChanged = [&](int32_t next) {
        ++changes;
        const auto state = store->snapshot();
        resetBeforeClock = actor.day() == 11 && actor.timeOfDay() == 200.f && next == -3 &&
                           state.dead.empty() && state.zones.empty();
    };
    send(Time{11, 200}, 1002);
    check(changes == 0 && actor.worldDeadSet() == expected && store->snapshot().zones.contains({8, 9}) &&
              actor.timeOfDay() == 200.f, "same-day packet8 updates its scalar without resetting metadata");
    send(Time{-3, -123}, 1003);
    check(changes == 1 && resetBeforeClock && actor.day() == -3 && actor.timeOfDay() == -123.f &&
              actor.worldDeadSet().empty() && store->snapshot().zones.empty(),
          "backward day change clears retained map and death IDs before storing the new day/time");
    {
        auto guard = store->lock();
        check(store->retained().dead.empty() && store->retained().zones.empty() &&
                  store->snapshot().rewards == std::set<int64_t>{19, -21},
              "reset clears retained metadata while preserving the separate reward-marker set");
        check(store->find({0, 0}) == source && source->lists.groundItems.size() == 1 &&
                  source->lists.groundItems[0][0] == 0x65 && source->lists.groundItemsChanged && source->lists.deadTracked == 31 &&
                  actor.hasCreature(55),
              "metadata reset preserves physical source identity, live payload/flags and resident membership");
    }
    actor.markDead(81);
    {
        auto guard = store->lock();
        store->retained().zones[{4, 5}].deadTracked = 9;
    }
    auto otherPeer = network.endpoint(99);
    otherPeer->send(machine, Channel::ToClient, encode(Time{44, 900}, Direction::ToClient));
    client.receive(1004);
    check(changes == 1 && actor.day() == -3 && actor.isWorldDead(81) && store->snapshot().zones.contains({4, 5}),
          "an unrelated peer cannot trigger the selected clock/reset route");
    std::printf("World identifier runtime state: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
