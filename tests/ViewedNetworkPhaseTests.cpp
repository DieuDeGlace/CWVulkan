#include "CombatText.h"
#include "Extracted.h"
#include "Multiplayer.h"
#include "NativeBuffs.h"
#include "NetWorlds.h"
#include "Spawned.h"
#include "net/ClientGame.h"
#include "worldgen/Residents.h"
#include <cstdio>
#include <cstring>
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
template <class T> void setActorField(uint8_t *record, size_t offset, T value) {
    std::memcpy(record + offset, &value, sizeof value);
}

void guestBubbleHealing(const auto &check) {
    using namespace cwnet;
    using namespace worldgen::creature;
    constexpr int64_t caster = 777, recipient = 1, bubble = 101;
    ExtractedWorld blocks;
    blocks.openGenerated(0, 0);
    SpawnedCreatures host;
    bool created = false;
    auto *actor = host.networkCreature(caster, created);
    actor[HOSTILITY] = 0;
    setActorField(actor, POSITION, int64_t(100) * 65536);
    setActorField(actor, HEALTH, 100.f);
    actor = host.networkCreature(recipient, created);
    actor[HOSTILITY] = 0;
    setActorField(actor, HEALTH, 10.f);
    actor = host.networkCreature(bubble, created);
    actor[HOSTILITY] = 7;
    setActorField(actor, RACE, int32_t(0x11d));
    setActorField(actor, OWNER, caster);
    setActorField(actor, HEALTH, 0.f);
    // Unequipped neutral caster with creation roll zero, at expiry on the 563rd 16 ms visit.
    setActorField(actor, BLOCK_POWER, 3.75f);
    setActorField(actor, 0x188, 0.09918756783008575f);

    host.simulate(blocks, 10);
    const auto &notices = host.events().damageNumbers;
    if (notices.size() != 2) {
        check(false, "a dedicated authority emits the guest's two bubble healing notices");
        return;
    }
    const bool critical = notices[0][0x14] != 0;
    const float amount = critical ? 0.7439067363739014f : 0.3719533681869507f;
    float hostHealth;
    std::memcpy(&hostHealth, host.record(recipient) + HEALTH, sizeof hostHealth);
    check(notices[0] == notices[1] && field<int64_t>(notices[0], damage::ATTACKER) == caster &&
              field<int64_t>(notices[0], 8) == recipient && field<float>(notices[0], 0x10) == -amount &&
              hostHealth == 10.f, "the dedicated authority leaves the guest's HP to its receiving client");

    SpawnedCreatures guest;
    guest.setNativePlayer(true);
    guest.setRemoteClient(true);
    MirrorSide mirror(guest, blocks);
    LoopbackNetwork network;
    constexpr PeerId peer = 1;
    auto endpoint = network.endpoint(peer);
    ClientGame client(*endpoint, mirror);
    client.link().goHome(1000);
    endpoint->send(peer, Channel::ToClient, encode(YourId{recipient}, Direction::ToClient));
    client.receive(1000);
    guest.setPlayer(blocks, {}, 0.f, {}, 10.f, 1, true);
    guest.setPlayerHealth(50.f);
    std::vector<DamageRecord> shown;
    guest.setHitVisualObserver([&](const DamageRecord &notice) { shown.push_back(notice); });
    const auto packet = encode(WorldUpdate{host.events()}, Direction::ToClient);
    endpoint->send(peer, Channel::ToClient, packet);
    client.receive(1001);
    check(guest.playerHealth() == 50.f && shown.empty(), "receipt queues the bubble batch until event consumption");
    client.consumeWorldUpdates();
    const float healed = (50.f + amount) + amount; // Two separately rounded HP writes.
    check(guest.playerHealth() == healed && shown == notices && mirror.hitsTaken == 2,
          "the guest applies and presents each of the two healing records in the compressed batch");
    for (const auto &notice : shown) {
        const auto labels = combattext::visit(notice, [&](int64_t id) { return guest.creature(id); }, guest.playerRecord());
        check(labels.size() == 1 && labels[0].text == (critical ? L"0.7" : L"0.3") &&
                  labels[0].colour == glm::vec4(0.4f, 1.f, 0.5f, 1.f) && labels[0].fontSize == (critical ? 60.f : 20.f),
              "the displayed heal uses the recorded magnitude and critical size");
    }
    client.consumeWorldUpdates();
    check(guest.playerHealth() == healed && shown == notices && mirror.hitsTaken == 2 &&
              guest.events().damageNumbers.empty(), "empty consumption neither replays nor echoes guest healing");
    guest.setPlayerHealth(guest.playerMaxHealth() - 0.125f);
    endpoint->send(peer, Channel::ToClient, packet);
    client.receive(1002);
    client.consumeWorldUpdates();
    check(guest.playerHealth() == guest.playerMaxHealth() && mirror.hitsTaken == 4,
          "each delivered healing record respects the guest's maximum HP");
}
// Single player is the client joined to its own host through the loopback. A Water Mage's own
// bubbles heal the caster through the same two records: the host (no local actor) leaves the
// player's HP alone and the local client applies each record, so the caster gains twice the
// burst. The eight bubbles are the strength (+0x184) and remaining lifetime (+0x188) recorded
// from an original Steam cast (run-4, 2026-11-21), where the caster went from 20 to 46.351 HP.
void selfCastBubbleHealing(const auto &check) {
    using namespace cwnet;
    using namespace worldgen::creature;
    constexpr int64_t me = 1;
    constexpr float strengths[8] = {6.702720f, 8.846398f, 5.875595f, 7.285725f, 5.597901f, 8.455851f, 6.656365f, 8.068336f};
    constexpr float lifetimes[8] = {0.231484f, 0.208284f, 0.191784f, 0.175284f, 0.155485f, 0.135685f, 0.119185f, 0.099385f};
    ExtractedWorld blocks;
    blocks.openGenerated(0, 0);
    SpawnedCreatures host;
    bool created = false;
    auto *actor = host.networkCreature(me, created);
    actor[HOSTILITY] = 0;
    setActorField(actor, HEALTH, 20.f);
    for (int i = 0; i < 8; ++i) {
        actor = host.networkCreature(101 + i, created);
        actor[HOSTILITY] = 7;
        setActorField(actor, RACE, int32_t(0x11d));
        setActorField(actor, OWNER, me);
        setActorField(actor, HEALTH, 0.f);
        setActorField(actor, BLOCK_POWER, strengths[i]);
        setActorField(actor, 0x188, lifetimes[i]);
    }
    host.simulate(blocks, 10);
    const auto &notices = host.events().damageNumbers;
    std::vector<DamageRecord> heals;
    for (const auto &notice : notices)
        if (field<int64_t>(notice, 8) == me)
            heals.push_back(notice);
    bool paired = heals.size() == 16;
    for (size_t i = 0; paired && i < heals.size(); i += 2) {
        const float amount = -field<float>(heals[i], 0x10);
        const int bubble = int(i / 2);
        const float single = strengths[bubble] * lifetimes[bubble], doubled = (strengths[bubble] + strengths[bubble]) * lifetimes[bubble];
        paired = heals[i] == heals[i + 1] && field<int64_t>(heals[i], damage::ATTACKER) == me &&
                 amount == (heals[i][0x14] ? doubled : single);
    }
    float hostHealth;
    std::memcpy(&hostHealth, host.record(me) + HEALTH, sizeof hostHealth);
    check(paired && hostHealth == 20.f,
          "each of the caster's eight bubbles publishes two caster-credited heal records and the host leaves the player's HP");
    if (!paired)
        return;

    SpawnedCreatures local;
    local.setNativePlayer(true);
    local.setRemoteClient(true);
    MirrorSide mirror(local, blocks);
    LoopbackNetwork network;
    constexpr PeerId peer = 1;
    auto endpoint = network.endpoint(peer);
    ClientGame client(*endpoint, mirror);
    client.link().goHome(1000);
    endpoint->send(peer, Channel::ToClient, encode(YourId{me}, Direction::ToClient));
    client.receive(1000);
    local.setPlayer(blocks, {}, 0.f, {}, 10.f, 1, true);
    local.setPlayerHealth(20.f);
    float expected = 20.f, once = 20.f;
    for (size_t i = 0; i < heals.size(); ++i) {
        expected = expected - field<float>(heals[i], 0x10); // each record is its own HP write
        if (i % 2 == 0)
            once = once - field<float>(heals[i], 0x10);
    }
    if (!(local.playerMaxHealth() >= expected)) {
        check(false, "the local caster's maximum HP leaves room for the whole self-heal");
        return;
    }
    endpoint->send(peer, Channel::ToClient, encode(WorldUpdate{host.events()}, Direction::ToClient));
    client.receive(1001);
    client.consumeWorldUpdates();
    check(local.playerHealth() == expected && mirror.hitsTaken == 16 && expected - 20.f > 2.f * (once - 20.f) - 1e-3f &&
              expected - 20.f < 2.f * (once - 20.f) + 1e-3f,
          "the single-player caster applies every self-heal record: twice the host's burst, as in the original");
    client.consumeWorldUpdates();
    check(local.playerHealth() == expected && mirror.hitsTaken == 16, "an empty pass does not heal the caster again");
}
enum class BubbleDefense { Guard, FullShield, BrokenShield };

void guestBubbleDefense(BubbleDefense defense, const auto &check) {
    using namespace cwnet;
    using namespace worldgen::creature;
    constexpr int64_t caster = 71, recipient = 1, bubble = 101;
    const bool guarding = defense == BubbleDefense::Guard;
    const bool broken = defense == BubbleDefense::BrokenShield;
    const float secondPool = broken ? 0.75f : 8.f;
    const auto firstShield = nativebuffs::make(6, 0.25f, 2000.f, 17);
    const auto secondShield = nativebuffs::make(6, secondPool, 3000.f, 19);

    ExtractedWorld blocks;
    blocks.openGenerated(0, 0);
    SpawnedCreatures host;
    bool created = false;
    auto *actor = host.networkCreature(caster, created);
    actor[HOSTILITY] = 1;
    actor[CLASS] = 3;
    actor[SPEC] = 1;
    setActorField(actor, POSITION, int64_t(100) * 65536);
    actor = host.networkCreature(recipient, created);
    actor[HOSTILITY] = 0;
    actor[ABILITY] = guarding ? 9 : 0;
    setActorField(actor, POSITION, int64_t(65536));
    setActorField(actor, HEALTH, 20.f);
    setActorField(actor, BLOCK_POWER, 1.f);
    host.giveBuffNotice(nativebuffs::notice(17, recipient, firstShield, 0));
    host.giveBuffNotice(nativebuffs::notice(19, recipient, secondShield, 0));
    actor = host.networkCreature(bubble, created);
    actor[HOSTILITY] = 7;
    setActorField(actor, RACE, int32_t(0x11d));
    setActorField(actor, OWNER, caster);
    setActorField(actor, HEALTH, 0.f);
    setActorField(actor, BLOCK_POWER, 16.f);
    setActorField(actor, 0x188, 0.125f);
    host.simulate(blocks, 0);
    const auto &events = host.events();
    const auto &hits = events.damageNumbers;
    if (hits.size() != (broken ? 2u : 1u)) {
        check(false, "bubble defense produces a special record and only a positive excess produces an ordinary hit");
        return;
    }
    const float incoming = field<float>(hits.front(), 0x10);
    const float residual = broken ? incoming - 1.f : 0.f;
    check((incoming == 2.f || incoming == 4.f) && hits.front()[0x45] == (guarding ? 1 : 5) &&
              !hits.front()[0x14] && !hits.front()[0x44] && field<int64_t>(hits.front(), 0) == caster &&
              field<int64_t>(hits.front(), 8) == recipient && field<float>(hits.front(), 0x38) == 5.f &&
              field<float>(hits.front(), 0x40) == 1.25f,
          "block and absorb records carry the full incoming amount, half impulse and cleared critical and spirit bytes");
    if (broken)
        check(hits.back()[0x45] == 0 && hits.back()[0x44] == 1 && hits.back()[0x14] == (incoming == 4.f) &&
                  field<float>(hits.back(), 0x10) == residual && field<float>(hits.back(), 0x38) == 10.f,
              "the ordinary residual follows absorption and retains the original critical flag and full impulse");
    check(events.buffs.empty(), "a dedicated host leaves the player's shield wear notices to the recipient");

    SpawnedCreatures guest;
    guest.setNativePlayer(true);
    guest.setRemoteClient(true);
    MirrorSide mirror(guest, blocks);
    LoopbackNetwork network;
    auto endpoint = network.endpoint(1);
    ClientGame client(*endpoint, mirror);
    const auto deliver = [&](const Packet &packet) {
        endpoint->send(endpoint->self(), Channel::ToClient, encode(packet, Direction::ToClient));
    };
    client.link().goHome(1000);
    deliver(YourId{recipient});
    client.receive(1000);
    guest.setPlayer(blocks, {}, 0.f, {}, 20.f, 1, true);
    guest.setPlayerHealth(20.f);
    auto &player = *guest.localPlayer();
    player.record[ABILITY] = guarding ? 9 : 0;
    setActorField(player.record, BLOCK_POWER, 1.f);
    client.send(1000);
    deliver(WorldUpdate{events});
    // Both compressed messages reach the same Game frame. Buffs run before hits across that batch.
    WorldEvents granted;
    granted.buffs = {nativebuffs::notice(17, recipient, firstShield, 0), nativebuffs::notice(19, recipient, secondShield, 0)};
    deliver(WorldUpdate{granted});
    client.receive(1035);
    client.consumeWorldUpdates();
    if (player.buffs.size() != 2) {
        check(false, "the recipient installs both distinct-source shields");
        return;
    }
    check(guest.playerHealth() == 20.f - residual && mirror.hitsTaken == hits.size(),
          "only the ordinary residual reduces guest HP");
    const auto &changed = guest.events().buffs;
    if (guarding) {
        float power;
        std::memcpy(&power, player.record + BLOCK_POWER, sizeof power);
        check(power == 1.f - incoming / 5.f && player.buffs[0] == firstShield && player.buffs[1] == secondShield &&
                  changed.empty(), "a successful guard spends block power before either shield can absorb the hit");
        return;
    }
    const auto spentFirst = nativebuffs::make(6, 0.f, 0.f, 17);
    const auto spentSecond = nativebuffs::make(6, broken ? 0.f : secondPool - incoming, broken ? 0.f : 3000.f, 19);
    const std::vector<BuffRecord> expected{nativebuffs::notice(recipient, recipient, spentFirst, 0),
                                           nativebuffs::notice(recipient, recipient, spentSecond, 0)};
    check(player.buffs[0] == spentFirst && player.buffs[1] == spentSecond && changed == expected,
          "each shield loses the full amount, keeps its original source, and reports the recipient as sender");
    client.collectOwnEvents();
    client.send(1070);
    Received message;
    GuestEvents returned;
    while (endpoint->receive(Channel::ToHost, message)) {
        const auto packet = decode(message.bytes, Direction::ToHost);
        if (const auto *notice = std::get_if<BuffNotice>(&packet))
            returned.buffs.push_back(notice->record);
    }
    check(returned.buffs == expected, "the guest returns both shield updates in separate packet-12 messages");
    const auto hitsBeforeRelay = mirror.hitsTaken;
    host.events() = {};
    guest.events() = {};
    host.applyGuestEvents(blocks, returned);
    check(host.events().buffs == expected, "the host relays the returned shield updates in order");
    deliver(WorldUpdate{host.events()});
    client.receive(1105);
    client.consumeWorldUpdates();
    client.collectOwnEvents();
    check(guest.playerHealth() == 20.f - residual && guest.events().buffs.empty() && client.link().outbox.buffs.empty() &&
              player.buffs[0] == spentFirst && player.buffs[1] == spentSecond && mirror.hitsTaken == hitsBeforeRelay,
          "the returning self notices produce no new hit or outgoing shield update");
}

} // namespace
int viewedNetworkPhaseTests() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) { ++failures; std::printf("FAIL viewed network phase: %s\n", message); }
    };
    sessionPhases(check);
    queueAndSendOrder(check);
    guestBubbleHealing(check);
    selfCastBubbleHealing(check);
    for (auto defense : {BubbleDefense::Guard, BubbleDefense::FullShield, BubbleDefense::BrokenShield})
        guestBubbleDefense(defense, check);
    std::printf("Viewed network dispatch phases: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
