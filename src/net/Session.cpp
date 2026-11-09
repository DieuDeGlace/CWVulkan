#include "net/Session.h"
#include <utility>
#include "net/Worlds.h"

#include <type_traits>

namespace cwnet {

// --- HostLink ----------------------------------------------------------------------------------

HostLink::HostLink(Transport &transport, World world) : transport(transport), world(std::move(world)) {
}

void HostLink::send(PeerId to, const Packet &packet) {
    transport.send(to, Channel::ToClient, encode(packet, Direction::ToClient));
}

HostLink::Connection *HostLink::connection(PeerId peer) {
    auto it = guests.find(peer);
    return it == guests.end() ? nullptr : &it->second;
}

void HostLink::invite(PeerId friendId, uint32_t nowMs) {
    send(friendId, Invitation{GAME_VERSION});
    invitedAt[friendId] = nowMs;
}

void HostLink::removeGuest(PeerId peer) {
    auto it = guests.find(peer);
    if (it == guests.end())
        return;
    if (world.creatureExists && world.removeCreature && world.creatureExists(it->second.creatureId))
        world.removeCreature(it->second.creatureId);
    guests.erase(it);
}

void HostLink::kick(PeerId peer) {
    invitedAt.erase(peer);
    removeGuest(peer);
    if (peer != transport.self())
        send(peer, Leave{});
}

void HostLink::dropAll() {
    for (const auto &[peer, guest] : guests)
        if (peer != transport.self())
            send(peer, Leave{});
    guests.clear();
    invitedAt.clear();
}

// Packet 1 (netcode-packets.md §2.2): the smallest id 1..10 that no creature of the host world
// has. The id is only taken once the guest's creature exists (its first packet 3), so two joins
// in one tick can get the same id, as in the game.
void HostLink::onJoin(PeerId from, Inbox &inbox) {
    int64_t id = 0;
    for (int64_t k = FIRST_PLAYER_ID; k <= LAST_PLAYER_ID && id == 0; k++)
        if (!world.creatureExists || !world.creatureExists(k)) {
            bool reserved = false;
            if (worldHandshake)
                for (const auto &[peer, pending] : guests)
                    reserved = reserved || (peer != from && pending.creatureId == k);
            if (!reserved)
                id = k;
        }
    if (id == 0)
        return;         // full: the joiner gets no answer
    guests.erase(from); // a second join replaces the old record (its creature stays)
    Connection &guest = guests[from];
    guest.creatureId = id;
    guest.peer = from;
    guest.worldAccepted = !worldHandshake || from == transport.self();
    inbox.joined.push_back(from);
    send(from, YourId{id});
}

// Packet 2 (§2.3): the guest leaves; it is not re-invited.
void HostLink::onLeave(PeerId from, Inbox &inbox) {
    invitedAt.erase(from);
    if (guests.count(from))
        inbox.left.push_back(from);
    removeGuest(from);
}

void HostLink::receive(Inbox &inbox) {
    Received in;
    while (transport.receive(Channel::ToHost, in)) {
        DecodeInfo info;
        Packet packet = decode(in.bytes, Direction::ToHost, &info);
        const uint32_t id = packetId(packet);
        Connection *guest = connection(in.from);
        if (!guest)
            inbox.log.push_back("MESSAGE??? " + std::to_string(id));
        // Join, leave and version mismatch are taken from anyone; everything else needs a connection.
        if (std::holds_alternative<Join>(packet)) {
            onJoin(in.from, inbox);
            continue;
        }
        if (std::holds_alternative<Leave>(packet)) {
            onLeave(in.from, inbox);
            continue;
        }
        if (std::holds_alternative<VersionMismatch>(packet)) {
            invitedAt.erase(in.from);
            continue;
        }
        if (!guest)
            continue;
        if (!guest->worldAccepted && !std::holds_alternative<CubeVulkanHello>(packet) && !std::holds_alternative<CubeVulkanMessage>(packet))
            continue;
        std::visit(
            [&](auto &p) {
                using T = std::decay_t<decltype(p)>;
                if constexpr (std::is_same_v<T, CreatureUpdate>) {
                    // Only the guest's own creature, and never an ignored (empty) packet.
                    if (!info.ignored && p.delta.id == guest->creatureId)
                        inbox.creatureUpdates.push_back(std::move(p.delta));
                } else if constexpr (std::is_same_v<T, Interaction>)
                    inbox.interactions.emplace_back(guest->creatureId, p.record);
                else if constexpr (std::is_same_v<T, DamageNumber>)
                    inbox.damageNumbers.push_back(p.record);
                else if constexpr (std::is_same_v<T, SoundRequest>)
                    inbox.sounds.push_back(p.record);
                else if constexpr (std::is_same_v<T, BuffNotice>)
                    inbox.buffs.push_back(p.record);
                else if constexpr (std::is_same_v<T, EffectObject>)
                    inbox.effects.push_back(p.record);
                else if constexpr (std::is_same_v<T, Chat>) {
                    // Relayed to every guest, the speaker included.
                    if (!p.text.empty())
                        for (auto &[peer, other] : guests)
                            other.chat.push_back({guest->creatureId, p.text});
                } else if constexpr (std::is_same_v<T, ZoneRequest>)
                    guest->requestedZones.insert(p.zone);
                else if constexpr (std::is_same_v<T, CubeVulkanHello>) {
                    if (p.magic == CubeVulkanHello::MAGIC && !guest->cubeVulkan) {
                        guest->cubeVulkan = true;
                        send(in.from, CubeVulkanHello{});
                    }
                } else if constexpr (std::is_same_v<T, CubeVulkanMessage>) {
                    if (guest->cubeVulkan) {
                        if (worldHandshake && p.channel == worldHandshake->channel && p.payload == worldHandshake->payload)
                            guest->worldAccepted = true;
                        inbox.extras.emplace_back(in.from, std::move(p));
                    }
                }
                // Anything else is not for the host and is ignored, as the game does.
            },
            packet);
    }
}

void HostLink::reinvite(uint32_t nowMs) {
    for (auto &[peer, at] : invitedAt)
        if (!guests.count(peer) && nowMs - at > REINVITE_MS) {
            at = nowMs;
            send(peer, Invitation{GAME_VERSION});
        }
}

void HostLink::sendTick(Connection &guest, const std::map<int64_t, CreatureSnapshot> &visible, const Time &time) {
    if (!guest.worldAccepted)
        return;
    for (const auto &[id, now] : visible) {
        auto known = guest.known.find(id);
        send(guest.peer, CreatureUpdate{makeDelta(id, now, known == guest.known.end() ? nullptr : &known->second)});
    }
    guest.known = visible;
    send(guest.peer, EndOfCreatureUpdates{});
    send(guest.peer, time);
    send(guest.peer, WorldUpdate{std::move(guest.waiting)});
    guest.waiting = WorldEvents{};
    if (!guest.chat.empty()) {
        const ChatLine line = std::move(guest.chat.front());
        guest.chat.pop_front();
        send(guest.peer, Chat{line.speaker, line.text});
    }
}

void HostLink::forgetKnownCreatures() {
    for (auto &[peer, guest] : guests)
        guest.known.clear();
}

bool HostLink::sendExtra(PeerId to, const CubeVulkanMessage &message) {
    const Connection *guest = connection(to);
    if (!guest || !guest->cubeVulkan)
        return false;
    send(to, message);
    return true;
}

// --- GuestLink ---------------------------------------------------------------------------------

GuestLink::GuestLink(Transport &transport, World world, Timing timing)
    : transport(transport), world(std::move(world)), timing(std::move(timing)) {
    if (!this->timing.shared) this->timing.shared = std::make_shared<Clocks>();
}
uint32_t GuestLink::sampleTime(uint32_t fallback) const {
    return timing.sample ? timing.sample() : fallback;
}

void GuestLink::send(PeerId to, const Packet &packet) {
    transport.send(to, Channel::ToHost, encode(packet, Direction::ToHost));
}

void GuestLink::connectTo(PeerId host, uint32_t nowMs) {
    drainTime.reset();
    ++connectionVersion;
    worldReady = !requireWorldHandshake || host == transport.self();
    if (requireWorldHandshake)
        myId = 0;
    // 1. Leave the current host.
    if (transport.valid(currentHost)) {
        send(currentHost, Leave{});
        invitedBy.erase(currentHost);
    }
    // 2. Reset what was sent and heard; the next update is a full one.
    lastSent.reset();
    currentHost = 0;
    heard.clear();
    outbox.damageNumbers.clear();
    outbox.sounds.clear();
    outbox.buffs.clear();
    outbox.effects.clear();
    hostSaidHello = false;
    helloSent = false;
    // 3. and 4. The mirror world starts over; this machine's own host drops its guests.
    if (world.resetMirror)
        world.resetMirror();
    if (world.ownHostDropAll)
        world.ownHostDropAll();
    // 5. and 6. Join the new host.
    send(host, Join{});
    fallbackBaseline = sampleTime(nowMs);
    currentHost = host;
    if (world.mirrorConnected)
        world.mirrorConnected();
}

void GuestLink::receive(uint32_t nowMs, Inbox &inbox, MirrorWorld *receiver) {
    drainTime.reset();
    auto batch = transport.receiveBatch(Channel::ToClient);
    for (auto &entry : batch) {
        Received in = std::move(entry); // release this packet after dispatch, before the next entry
        const bool fromHost = in.from == currentHost;
        if (fromHost)
            fallbackBaseline = sampleTime(nowMs);
        Packet packet = decode(in.bytes, Direction::ToClient);
        if (!worldReady && !std::holds_alternative<Invitation>(packet) && !std::holds_alternative<Leave>(packet) &&
            !std::holds_alternative<YourId>(packet) && !std::holds_alternative<CubeVulkanHello>(packet) &&
            !std::holds_alternative<CubeVulkanMessage>(packet))
            continue;
        // Only the current host is listened to, except for invitations and leave notices.
        if (!fromHost && !std::holds_alternative<Invitation>(packet) && !std::holds_alternative<Leave>(packet))
            continue;
        std::visit(
            [&](auto &p) {
                using T = std::decay_t<decltype(p)>;
                if constexpr (std::is_same_v<T, Invitation>) {
                    if (p.version == GAME_VERSION) {
                        auto [invitation, inserted] = invitedBy.try_emplace(in.from, 0);
                        if (inserted) inbox.invitations.push_back(in.from);
                        invitation->second = sampleTime(nowMs); // after insertion/lookup, on every renewal
                    } else
                        send(in.from, VersionMismatch{GAME_VERSION});
                } else if constexpr (std::is_same_v<T, Leave>) {
                    if (in.from == currentHost) {
                        connectTo(transport.self(), nowMs);
                        inbox.wentHome = true;
                    }
                    invitedBy.erase(in.from);
                } else if constexpr (std::is_same_v<T, CreatureUpdate>) {
                    if (p.delta.id == myId)
                        return; // the player's own record never comes from the network
                    heard.insert(p.delta.id);
                    if (receiver)
                        receiver->receiveCreature(p.delta);
                    else {
                        inbox.creatureOrder.push_back({true, inbox.creatureUpdates.size()});
                        inbox.creatureUpdates.push_back(std::move(p.delta));
                    }
                } else if constexpr (std::is_same_v<T, YourId>) {
                    if (receiver) receiver->setLocalId(p.id);
                    myId = p.id;
                    inbox.givenId = p.id;
                    requestFullUpdate();
                    if (!helloSent) {
                        helloSent = true;
                        send(in.from, CubeVulkanHello{});
                    }
                } else if constexpr (std::is_same_v<T, EndOfCreatureUpdates>) {
                    if (receiver)
                        receiver->keepOnly(heard);
                    else {
                        inbox.creatureOrder.push_back({false, inbox.creatureLists.size()});
                        inbox.creatureLists.push_back(std::move(heard));
                    }
                    heard.clear();
                } else if constexpr (std::is_same_v<T, WorldUpdate>) {
                    // A world reset (a sleep notice of 0) makes the next update a full one.
                    for (int32_t sleep : p.events.sleeps)
                        if (sleep == 0)
                            requestFullUpdate();
                    inbox.worldUpdates.push_back(std::move(p.events));
                } else if constexpr (std::is_same_v<T, Time>) {
                    if (receiver) {
                        if (p.day != receiver->day()) receiver->dayChange(p.day);
                        receiver->setClock(p.day, p.timeOfDay);
                    } else
                        inbox.times.push_back(p);
                } else if constexpr (std::is_same_v<T, Chat>) {
                    if (!p.text.empty())
                        inbox.chat.push_back({p.speaker, std::move(p.text)});
                } else if constexpr (std::is_same_v<T, CubeVulkanHello>)
                    hostSaidHello = hostSaidHello || (p.magic == CubeVulkanHello::MAGIC && helloSent);
                else if constexpr (std::is_same_v<T, CubeVulkanMessage>) {
                    if (hostSaidHello)
                        inbox.extras.push_back(std::move(p));
                }
            },
            packet);
    }
}

bool GuestLink::checkTimers(uint32_t nowMs) {
    DrainTime phase{sampleTime(nowMs), 0, false};
    {
        std::lock_guard lock(timing.shared->guard);
        if (!timing.shared->send) timing.shared->send = sampleTime(nowMs);
    }
    phase.comparison = sampleTime(nowMs);
    if (uint32_t(phase.comparison - fallbackBaseline) > HOST_SILENCE_MS) {
        goHome(nowMs); // the connection takes its own fresh sample after the request attempt
        phase.fallback = true;
        drainTime = phase;
        return true; // invitation expiry and ordinary output belong only to nonfallback drains
    }
    for (auto it = invitedBy.begin(); it != invitedBy.end();)
        it = uint32_t(phase.comparison - it->second) > INVITATION_LIFE_MS ? invitedBy.erase(it) : std::next(it);
    drainTime = phase;
    return false;
}

bool GuestLink::sendTick(uint32_t nowMs, const CreatureSnapshot &player) {
    return sendTick(nowMs, [&] { return &player; }, {});
}

bool GuestLink::sendTick(uint32_t nowMs, const SnapshotSource &source, const InteractionSource &interaction) {
    if (!drainTime) checkTimers(nowMs);
    const DrainTime phase = *std::exchange(drainTime, std::nullopt);
    if (phase.fallback) return false;
    const auto finish = [&](bool sent) {
        const uint32_t duration = sampleTime(nowMs) - phase.began;
        if (duration > 8 && timing.slowDrain) timing.slowDrain(duration);
        return sent;
    };
    bool due;
    {
        std::lock_guard lock(timing.shared->guard);
        due = uint32_t(phase.comparison - *timing.shared->send) > SEND_INTERVAL_MS;
        if (due) timing.shared->send = phase.comparison;
    }
    if (!due || !transport.valid(currentHost) || !worldReady) return finish(false);
    const CreatureSnapshot *snapshot = source ? source() : nullptr;
    if (!snapshot) return finish(false); // safe engine handling before a local entity exists
    const auto &player = *snapshot;
    const PeerId to = currentHost;
    // [3] The player's record, against the last copy sent (or whole).
    send(to, CreatureUpdate{makeDelta(myId, player, lastSent ? &*lastSent : nullptr)});
    lastSent = player;
    // [14] One chat line.
    if (!outbox.chat.empty()) {
        if (!outbox.chat.front().empty())
            send(to, Chat{0, outbox.chat.front()});
        outbox.chat.pop_front();
    }
    // [9] One interaction.
    if (outbox.interactions.empty() && interaction) {
        InteractionRecord record;
        if (interaction(record)) outbox.interactions.push_back(record);
    }
    if (!outbox.interactions.empty()) {
        send(to, Interaction{outbox.interactions.front()});
        outbox.interactions.pop_front();
    }
    // [10] [11] [12] [13] Everything the player caused.
    for (const DamageRecord &r : outbox.damageNumbers)
        send(to, DamageNumber{r});
    for (const SoundRecord &r : outbox.sounds)
        send(to, SoundRequest{r});
    for (const BuffRecord &r : outbox.buffs)
        send(to, BuffNotice{r});
    for (const EffectRecord &r : outbox.effects)
        send(to, EffectObject{r});
    outbox.damageNumbers.clear();
    outbox.sounds.clear();
    outbox.buffs.clear();
    outbox.effects.clear();
    // [15] Its separate process clock samples fresh, inside the admitted send path.
    {
        std::lock_guard lock(timing.shared->guard);
        if (!timing.shared->requests) timing.shared->requests = sampleTime(nowMs);
    }
    const uint32_t requestSample = sampleTime(nowMs);
    bool requestDue;
    {
        std::lock_guard lock(timing.shared->guard);
        requestDue = uint32_t(requestSample - *timing.shared->requests) > ZONE_REQUEST_MS;
        if (requestDue) timing.shared->requests = requestSample;
    }
    if (requestDue)
        for (const ZoneKey &zone : outbox.neededZones)
            send(to, ZoneRequest{zone});
    return finish(true);
}

void GuestLink::queueOwnEvents(const WorldEvents &events) {
    if (!transport.valid(currentHost))
        return;
    for (const DamageRecord &r : events.damageNumbers)
        if (field<int64_t>(r, damage::ATTACKER) == myId)
            outbox.damageNumbers.push_back(r);
    for (const SoundRecord &r : events.sounds)
        if (field<int64_t>(r, sound::OWNER) == myId)
            outbox.sounds.push_back(r);
    for (const BuffRecord &r : events.buffs)
        if (field<int64_t>(r, buffnote::SOURCE) == myId)
            outbox.buffs.push_back(r);
    for (const EffectRecord &r : events.effects)
        if (field<int64_t>(r, effect::OWNER) == myId)
            outbox.effects.push_back(r);
}

bool GuestLink::sendExtra(const CubeVulkanMessage &message) {
    if (!hostSaidHello || !transport.valid(currentHost))
        return false;
    send(currentHost, message);
    return true;
}

} // namespace cwnet
