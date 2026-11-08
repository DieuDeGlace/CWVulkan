#include "Extracted.h"
#include "Multiplayer.h"
#include "NetWorlds.h"
#include "Spawned.h"
#include "net/ClientGame.h"
#include <cstdio>
#include <string>
#include <vector>

namespace {
struct ObservedMirror : MirrorSide {
    using MirrorSide::MirrorSide;
    std::vector<std::string> visits;
    std::vector<cwnet::ZoneKey> requested{{7, 8}};
    unsigned requestReads = 0;
    std::vector<cwnet::ZoneKey> neededZones() override { ++requestReads; return requested; }
    void setLocalId(int64_t id) override {
        visits.push_back("i" + std::to_string(id));
        MirrorSide::setLocalId(id);
    }
    void receiveCreature(const cwnet::CreatureDelta &delta) override {
        visits.push_back("c" + std::to_string(delta.id));
        MirrorSide::receiveCreature(delta);
    }
    void keepOnly(const std::set<int64_t> &ids) override {
        visits.push_back("k");
        MirrorSide::keepOnly(ids);
    }
    void setClock(int32_t day, int32_t time) override {
        visits.push_back("t" + std::to_string(day));
        MirrorSide::setClock(day, time);
    }
    // A well-formed all-zero selected record is sufficient for these wire-timing checks.
    bool localSnapshot(cwnet::CreatureSnapshot &out) override { out = {}; return true; }
};
void sessionPhases(const auto &check) {
    ExtractedWorld blocks;
    blocks.openGenerated(0, 0);
    blocks.setClientStreaming(true);
    SpawnedCreatures actor;
    actor.setStoredZones(blocks, nullptr, true);
    actor.setRemoteClient(true);
    Multiplayer session;
    Multiplayer::Setup setup;
    setup.clientWorld = &actor;
    setup.blocks = &blocks; // actual session, intentionally no authority in this fixture
    std::string why;
    check(session.start(setup, why), "client-only fixture starts the real local session driver");
    if (!session.started()) return;
    actor.setClock(1, 100.f);
    auto *transport = session.net();
    const auto peer = transport->self();
    transport->send(peer, cwnet::Channel::ToClient, cwnet::encode(cwnet::Time{2, 200}, cwnet::Direction::ToClient));
    cwnet::WorldEvents events;
    cwnet::KillRecord death{};
    cwnet::setField(death, 8, int64_t(99));
    events.kills.push_back(death);
    transport->send(peer, cwnet::Channel::ToClient, cwnet::encode(cwnet::WorldUpdate{events}, cwnet::Direction::ToClient));
    session.beginFrame(Multiplayer::clockNow());
    check(actor.day() == 1 && actor.timeOfDay() == 100.f,
          "early host/transport phase leaves current incoming clock state unapplied during viewed simulation/input");
    session.consume();
    check(!actor.isWorldDead(99), "current late-arriving packet7 cannot enter an earlier pending-event pass");
    const auto late = Multiplayer::clockNow();
    session.dispatch(late);
    check(actor.day() == 2 && actor.timeOfDay() == 200.f && !actor.isWorldDead(99),
          "late dispatch applies the clock now while retaining packet7 for a later application pass");
    session.endFrame();
    actor.events() = {};
    transport->send(peer, cwnet::Channel::ToClient, cwnet::encode(cwnet::Time{3, 300}, cwnet::Direction::ToClient));
    session.beginFrame(late + 1);
    session.consume();
    check(actor.day() == 2 && actor.isWorldDead(99), "next pending-event phase applies the prior victim before current clock dispatch");
    session.dispatch(late + 1);
    check(actor.day() == 3 && !actor.isWorldDead(99), "later different-day clock reset follows the earlier death-ID merge");
    session.endFrame();
}
void queueAndSendOrder(const auto &check) {
    using namespace cwnet;
    ExtractedWorld blocks;
    blocks.openGenerated(0, 0);
    SpawnedCreatures actor;
    actor.setStoredZones(blocks, nullptr, true);
    actor.setRemoteClient(true);
    ObservedMirror mirror(actor, blocks);
    LoopbackNetwork network;
    constexpr PeerId peer = 1;
    auto endpoint = network.endpoint(peer);
    ClientGame client(*endpoint, mirror);
    client.link().goHome(1000);
    const auto enqueue = [&](const Packet &packet) {
        endpoint->send(peer, Channel::ToClient, encode(packet, Direction::ToClient));
    };
    enqueue(YourId{1});
    client.receive(1000);
    mirror.visits.clear();
    CreatureDelta creature;
    creature.id = 55; // mask0 is a complete no-field delta, not a truncated payload
    enqueue(Time{2, 200});
    enqueue(CreatureUpdate{creature});
    enqueue(YourId{2});
    auto local = creature; local.id = 2;
    enqueue(CreatureUpdate{local});
    enqueue(Time{3, 300});
    enqueue(EndOfCreatureUpdates{});
    enqueue(Time{4, 400});
    mirror.dayChanged = [&](int32_t day) {
        if (day == 3) { mirror.requested = {{9, 10}}; enqueue(Time{5, 500}); }
    };
    client.prepareSourceRequests();
    client.receive(1001);
    check(mirror.visits == std::vector<std::string>{"t2", "c55", "i2", "t3", "k", "t4"},
          "immediate3/4/6/8 effects retain queue order and current identity filters later creature packets");
    check(actor.day() == 4 && network.waiting(peer, Channel::ToClient) == 1,
          "packet queued by an applying callback waits outside the completed receive snapshot");
    check(client.link().outbox.neededZones == std::vector<ZoneKey>{{7, 8}} && mirror.requestReads == 1,
          "source requests retain the pre-dispatch sample when a callback changes current demand");
    DamageRecord hit{};
    setField(hit, damage::ATTACKER, int64_t(2));
    setField(hit, 0x08, int64_t(55)); // target record field
    actor.events().damageNumbers.push_back(hit);
    client.send(1003);
    const auto damagePackets = [&] {
        int count = 0;
        Received incoming;
        while (endpoint->receive(Channel::ToHost, incoming))
            count += std::holds_alternative<DamageNumber>(decode(incoming.bytes, Direction::ToHost));
        return count;
    };
    check(damagePackets() == 0, "late dispatch/send does not collect this frame's own event group before its end stage");
    client.collectOwnEvents();
    check(actor.events().damageNumbers.size() == 1 && client.link().outbox.damageNumbers.size() == 1,
          "end collection owns outgoing copies without clearing the live World event group");
    actor.events() = {};
    client.prepareSourceRequests();
    client.receive(1036);
    check(actor.day() == 5 && client.link().outbox.neededZones == std::vector<ZoneKey>{{9, 10}},
          "next snapshot applies the callback's packet with a new pre-dispatch request sample");
    client.send(1036);
    check(damagePackets() == 1, "a later admitted send emits exactly the event collected at frame end");
    // Peer transition during a fixed snapshot changes the admission of later entries.
    auto remote = network.endpoint(2);
    client.link().connectTo(2, 1100);
    mirror.dayChanged = {};
    remote->send(peer, Channel::ToClient, encode(Time{6, 600}, Direction::ToClient));
    remote->send(peer, Channel::ToClient, encode(Leave{}, Direction::ToClient));
    remote->send(peer, Channel::ToClient, encode(Time{9, 900}, Direction::ToClient));
    enqueue(Time{7, 700});
    mirror.visits.clear();
    client.receive(1101);
    check(mirror.visits == std::vector<std::string>{"t6", "t7"} && client.link().host() == peer,
          "one snapshot applies pre-transition state then rechecks each later sender against the newly selected peer");
}
} // namespace
int viewedNetworkPhaseTests() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) { ++failures; std::printf("FAIL viewed network phase: %s\n", message); }
    };
    sessionPhases(check);
    queueAndSendOrder(check);
    std::printf("Viewed network dispatch phases: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
