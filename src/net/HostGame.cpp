#include "net/HostGame.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <set>

namespace cwnet {

namespace {
// The two sounds the game logs as it hands them out.
constexpr int32_t DEATH_SOUND = 0x1b, MISSION_SOUND = 0x1f;
constexpr size_t SOUND_ID = 0x18; // i32 in a sound request

const int64_t *eventPosition(const uint8_t *record) { return reinterpret_cast<const int64_t *>(record); }
} // namespace

uint32_t CreatureView::flags() const {
    uint32_t v;
    std::memcpy(&v, record + creature::FLAGS, sizeof v);
    return v;
}

HostGame::HostGame(Transport &transport, HostWorld &world)
    : transport(transport), world(world),
      hostLink(transport, HostLink::World{[&world](int64_t id) { return world.hasCreature(id); },
                                          [&world](int64_t id) { world.removeCreature(id); }}) {}

bool HostGame::tooFar(const int64_t a[3], const int64_t b[3], double blocks) {
    constexpr double BLOCK = 1.0 / 65536.0;
    const double dx = (double)(a[0] - b[0]) * BLOCK, dy = (double)(a[1] - b[1]) * BLOCK, dz = (double)(a[2] - b[2]) * BLOCK;
    return std::fabs(dx) > blocks || std::fabs(dy) > blocks || dx * dx + dy * dy + dz * dz > blocks * blocks;
}

bool HostGame::frame(uint32_t nowMs) {
    if (lastTick && nowMs - *lastTick < TICK_MS)
        return false;
    const uint32_t elapsed = lastTick ? nowMs - *lastTick : TICK_MS;
    lastTick = nowMs;
    tick(nowMs, elapsed);
    return true;
}

void HostGame::tick(uint32_t nowMs, uint32_t elapsedMs) {
    auto say = [&](const std::string &line) {
        if (log)
            log(line);
    };
    // 1. Receive: each guest's own record goes straight into the world, its interactions onto
    //    its creature's list; the rest waits for step 4.
    HostLink::Inbox inbox;
    hostLink.receive(inbox);
    // (The link has read the whole batch: a guest that left in it, after its last update, has no
    // connection now, and its creature is not made again.)
    std::set<int64_t> connected;
    for (const auto &[peer, guest] : hostLink.connections())
        connected.insert(guest.creatureId);
    for (const CreatureDelta &delta : inbox.creatureUpdates)
        if (connected.count(delta.id))
            world.receiveCreature(delta);
    for (const auto &[creatureId, record] : inbox.interactions) {
        world.queueInteraction(creatureId, record);
        say("Host: interaction of kind " + std::to_string(record[interaction::KIND]) + " from creature " + std::to_string(creatureId));
    }
    for (PeerId peer : inbox.joined)
        say("Host: a player joined (" + std::to_string(peer) + ")");
    for (PeerId peer : inbox.left)
        say("Host: a player left (" + std::to_string(peer) + ")");
    for (const std::string &line : inbox.log)
        say("Host: " + line);
    if (extra)
        for (const auto &[from, message] : inbox.extras)
            extra(from, message);

    // 2. Invitations again for friends who have not joined.
    hostLink.reinvite(nowMs);

    // 3. Each guest's creatures and the events gathered for it last tick.
    world.creatures(list);
    for (auto &[peer, guest] : hostLink.connections())
        sendToGuest(guest);

    // 4-5. What the guests caused, applied to the world.
    GuestEvents caused;
    caused.damageNumbers = std::move(inbox.damageNumbers);
    caused.sounds = std::move(inbox.sounds);
    caused.buffs = std::move(inbox.buffs);
    caused.effects = std::move(inbox.effects);
    world.applyGuestEvents(caused);

    // 6. The world runs.
    world.advanceTimers(elapsedMs);
    world.advance(std::min(elapsedMs, MAX_STEP_MS));

    for (const PickupRecord &p : world.events().pickups) // (for the log: who took what)
        say("Host: creature " + std::to_string(field<int64_t>(p, 0)) + " picked up an item of type " + std::to_string(p[8]));

    // 7. A world reset: everyone gets full updates of everything in view next tick.
    for (int32_t sleep : world.events().sleeps)
        if (sleep == 0) {
            hostLink.forgetKnownCreatures();
            break;
        }

    // 8. This tick's events to each guest.
    world.creatures(list);
    for (auto &[peer, guest] : hostLink.connections())
        distribute(guest);

    // 9. The world's lists start empty next tick.
    world.events() = WorldEvents{};
}

void HostGame::sendToGuest(HostLink::Connection &guest) {
    const CreatureView *self = nullptr;
    for (const CreatureView &c : list)
        if (c.id == guest.creatureId)
            self = &c;
    if (!self)
        return; // a guest whose creature does not exist yet gets nothing
    std::map<int64_t, CreatureSnapshot> visible;
    bool newcomerTaken = false;
    for (const CreatureView &c : list) {
        if (c.id == guest.creatureId)
            continue;
        // Players are always sent; anything else only within 270 blocks.
        if (c.hostility() != 0 && tooFar(c.position(), self->position(), VIEW_BLOCKS))
            continue;
        // One newcomer a tick, the first met; those flagged 0x800 or 0x2000 always (and they use
        // up the tick's place too).
        if (!guest.known.count(c.id)) {
            const bool always = (c.flags() & creature::ALWAYS_SENT) != 0;
            if (newcomerTaken && !always)
                continue;
            newcomerTaken = true;
        }
        visible[c.id] = CreatureSnapshot::of(c.record);
    }
    hostLink.sendTick(guest, visible, world.clock());
}

void HostGame::distribute(HostLink::Connection &guest) {
    const WorldEvents &e = world.events();
    WorldEvents &to = guest.waiting;
    const CreatureView *self = nullptr;
    for (const CreatureView &c : list)
        if (c.id == guest.creatureId)
            self = &c;

    to.blockChanges.insert(to.blockChanges.end(), e.blockChanges.begin(), e.blockChanges.end());
    to.damageNumbers.insert(to.damageNumbers.end(), e.damageNumbers.begin(), e.damageNumbers.end());
    if (self) {
        // Particles and sounds near the guest only; never the sounds its own creature made (it
        // played them already).
        for (const ParticleRecord &p : e.particles)
            if (!tooFar(eventPosition(p.data()), self->position(), EVENT_BLOCKS))
                to.particles.push_back(p);
        for (const SoundRecord &s : e.sounds) {
            const bool sent = !tooFar(eventPosition(s.data()), self->position(), EVENT_BLOCKS) &&
                              field<int64_t>(s, sound::OWNER) != guest.creatureId;
            const int32_t id = field<int32_t>(s, SOUND_ID);
            if (log && (id == DEATH_SOUND || id == MISSION_SOUND))
                log(std::string(id == DEATH_SOUND ? "SoundDie" : "SoundMissionComplete") + (sent ? " SENT!" : " NOT SENT!"));
            if (sent)
                to.sounds.push_back(s);
        }
    }
    to.effects.insert(to.effects.end(), e.effects.begin(), e.effects.end());
    to.groundItems.insert(to.groundItems.end(), e.groundItems.begin(), e.groundItems.end());
    to.staticObjects.insert(to.staticObjects.end(), e.staticObjects.begin(), e.staticObjects.end());
    to.zonesAnswered.insert(to.zonesAnswered.end(), e.zonesAnswered.begin(), e.zonesAnswered.end());
    to.pickups.insert(to.pickups.end(), e.pickups.begin(), e.pickups.end());
    to.kills.insert(to.kills.end(), e.kills.begin(), e.kills.end());
    to.feats.insert(to.feats.end(), e.feats.begin(), e.feats.end());
    to.aggro.insert(to.aggro.end(), e.aggro.begin(), e.aggro.end());
    to.buffs.insert(to.buffs.end(), e.buffs.begin(), e.buffs.end());
    to.sleeps.insert(to.sleeps.end(), e.sleeps.begin(), e.sleeps.end());
    to.calls.insert(to.calls.end(), e.calls.begin(), e.calls.end());

    // The zones it asked for: the change log, the lists ever changed, and the key as answered.
    // A zone not in memory here is not answered; the guest asks again a second later.
    for (const ZoneKey &zone : guest.requestedZones) {
        ZoneAnswer answer;
        if (!world.zoneAnswer(zone, answer))
            continue;
        to.blockChanges.insert(to.blockChanges.end(), answer.blockChanges.begin(), answer.blockChanges.end());
        if (answer.groundItems)
            to.groundItems.push_back({zone, std::move(*answer.groundItems)});
        if (answer.staticObjects)
            to.staticObjects.push_back({zone, std::move(*answer.staticObjects)});
        to.zonesAnswered.push_back(zone);
    }
    guest.requestedZones.clear();
}

} // namespace cwnet
