#pragma once
// The two ends of a Steam Cube World session, as protocol state machines over any Transport:
//
//   * HostLink: the game's built-in host (docs/specs/netcode-transport.md §4, netcode-roles.md
//     §2-§4): invitations and re-invitations, joins and creature ids 1..10, leaves and kicks,
//     the per-guest record of what it knows, and the per-tick packets to each guest in the
//     game's order.
//   * GuestLink: the game's network client (netcode-transport.md §4.2-§4.6, netcode-packets.md
//     §2, §4): invitations and their expiry, connecting (join, leave, back to one's own host),
//     the host-silence timeout, the full-update flag, the creatures heard between two packet 6s,
//     and the per-send-tick packets in the game's order.
//
// Neither touches a world. What the world must do (apply a creature update, remove a creature,
// show a chat line, ...) comes out in an Inbox; what it wants sent goes in through the links'
// queues and send calls. Wiring them to CubeVulkan's worlds (mirror mode, host mode) is the
// integration's job.
//
// Times are milliseconds from any steady clock (the game uses timeGetTime), passed in so the
// links can be tested.
//
// CubeVulkan extras: after a guest gets its id, it says hello (packet 25); a CubeVulkan host
// answers in kind. Packet 26 messages then pass between those two only (sendExtra refuses
// otherwise), so an original-game peer never sees anything but its own protocol.
#include "net/Packets.h"
#include "net/Transport.h"

#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace cwnet {

struct ChatLine {
    int64_t speaker = 0;
    std::u16string text;
};

// --- Host ------------------------------------------------------------------------------------------

class HostLink {
  public:
    static constexpr int64_t FIRST_PLAYER_ID = 1;
    static constexpr int64_t LAST_PLAYER_ID = 10; // at most ten players share a host
    static constexpr uint32_t REINVITE_MS = 5000; // an invited friend who has not joined is re-invited after this

    // One guest (the game's 0x150-byte connection record).
    struct Connection {
        int64_t creatureId = 0;                      // +0x00
        PeerId peer = 0;                             // +0x08
        WorldEvents waiting;                         // +0x10: this tick's events for it
        std::set<ZoneKey> requestedZones;            // +0x110: zones it asked for (packet 15)
        std::deque<ChatLine> chat;                   // +0x130: lines waiting for it
        std::map<int64_t, CreatureSnapshot> known;   // +0x140: the creatures it has, as last sent
        bool cubeVulkan = false;                     // it said hello (packet 25)
    };

    // What the link needs to ask the host world.
    struct World {
        std::function<bool(int64_t id)> creatureExists; // a creature with this id is in the host world
        std::function<void(int64_t id)> removeCreature; // a leaving or kicked guest's creature goes
    };

    // What the guests sent, for the host world to apply (netcode-roles.md §2 steps 4-5).
    struct Inbox {
        // Packet 3 from a guest, already checked to be about that guest's own creature.
        std::vector<CreatureDelta> creatureUpdates;
        // Packet 9: {the sender's creature id, the interaction}, in arrival order.
        std::vector<std::pair<int64_t, InteractionRecord>> interactions;
        // Packets 10-13: the four queues of events the guests caused.
        std::vector<DamageRecord> damageNumbers;
        std::vector<SoundRecord> sounds;
        std::vector<BuffRecord> buffs;
        std::vector<EffectRecord> effects;
        // Joins and leaves, for the host's log or menu.
        std::vector<PeerId> joined, left;
        // CubeVulkan extras (packet 26) from identified CubeVulkan guests.
        std::vector<std::pair<PeerId, CubeVulkanMessage>> extras;
        // Lines the game would log ("MESSAGE??? <id>").
        std::vector<std::string> log;
    };

    HostLink(Transport &transport, World world);

    // The multiplayer menu's left button (§3.3, §4.1, §4.5).
    void invite(PeerId friendId, uint32_t nowMs);
    void kick(PeerId peer); // "uninvite": also removes a connected guest
    // Drops every guest (the host stopping, or its player joining someone else: §4.4).
    void dropAll();

    bool isInvited(PeerId peer) const { return invitedAt.count(peer) != 0; }
    bool isConnected(PeerId peer) const { return guests.count(peer) != 0; }
    Connection *connection(PeerId peer);
    // Guests in ascending peer order, as the host tick walks them.
    std::map<PeerId, Connection> &connections() { return guests; }

    // Host tick step 1: everything waiting on channel 1.
    void receive(Inbox &inbox);
    // Host tick step 2: re-invites invited friends who have not joined.
    void reinvite(uint32_t nowMs);
    // Host tick step 3, for one guest whose creature exists in the host world: one packet 3 per
    // creature of `visible` (its interest set, ascending id; deltas against what the guest knows,
    // full for newcomers), then 6, 8, 7 (the guest's waiting events, which are then emptied) and
    // one chat line if any. `visible` becomes what the guest knows.
    void sendTick(Connection &guest, const std::map<int64_t, CreatureSnapshot> &visible, const Time &time);
    // Host tick step 7: after a world reset every guest forgets its creatures, so the next tick
    // sends full updates of everything in view.
    void forgetKnownCreatures();

    // A CubeVulkan extra to an identified CubeVulkan guest; false otherwise.
    bool sendExtra(PeerId to, const CubeVulkanMessage &message);

  private:
    Transport &transport;
    World world;
    std::map<PeerId, uint32_t> invitedAt;  // host+0x578: friend -> time of the last invitation
    std::map<PeerId, Connection> guests;   // host+0x568

    void send(PeerId to, const Packet &packet);
    void onJoin(PeerId from, Inbox &inbox);
    void onLeave(PeerId from, Inbox &inbox);
    void removeGuest(PeerId peer);
};

// --- Guest -----------------------------------------------------------------------------------------

class GuestLink {
  public:
    static constexpr uint32_t HOST_SILENCE_MS = 5000;      // longer than this without a packet: back home
    static constexpr uint32_t INVITATION_LIFE_MS = 6000;   // an invitation not renewed for this long expires
    static constexpr uint32_t SEND_INTERVAL_MS = 32;       // a send tick at most this often
    static constexpr uint32_t ZONE_REQUEST_MS = 1000;      // zone requests at most once a second

    // What the link needs from the client's game.
    struct World {
        // Connecting anywhere (also home): remove every creature but the local player, undo the
        // block changes of loaded zones and mark them as needing their state (§4.3 step 3).
        std::function<void()> resetMirror;
        // Connecting anywhere: this machine's own host drops its guests (§4.3 step 4).
        std::function<void()> ownHostDropAll;
    };

    // What the host sent, for the client world to apply (netcode-packets.md §2).
    struct Inbox {
        std::vector<CreatureDelta> creatureUpdates; // never the local player's own
        // Each packet 6: the ids heard since the previous one; every other creature but the
        // local player is to be removed.
        std::vector<std::set<int64_t>> creatureLists;
        // The two above in the order they came: an update (true) or a packet 6 (false), with its
        // index in its list. A world applies them in this order, so that a creature updated after
        // one packet 6 is not removed by it.
        std::vector<std::pair<bool, size_t>> creatureOrder;
        std::optional<int64_t> givenId;             // packet 4 (the local player's new id)
        std::vector<WorldEvents> worldUpdates;      // packet 7
        std::vector<Time> times;                    // packet 8
        std::vector<ChatLine> chat;                 // packet 14
        std::vector<PeerId> invitations;            // new invitations to show ("You were invited by ...")
        bool wentHome = false;                      // the host left or fell silent: connected back home
        std::vector<CubeVulkanMessage> extras;      // packet 26 from a CubeVulkan host
    };

    // What the client's game queues for the host; sent and emptied by sendTick.
    struct Outbox {
        std::deque<std::u16string> chat;            // one line per send tick
        std::deque<InteractionRecord> interactions; // one per send tick
        std::vector<DamageRecord> damageNumbers;    // all of these every send tick
        std::vector<SoundRecord> sounds;
        std::vector<BuffRecord> buffs;
        std::vector<EffectRecord> effects;
        std::vector<ZoneKey> neededZones;           // the zones still needing their state
    };
    Outbox outbox;

    GuestLink(Transport &transport, World world);

    // §4.3: leaves the current host (packet 2), resets, and joins `host` (packet 1). Joining a
    // friend, leaving (to one's own host) and the timeouts all come through here.
    void connectTo(PeerId host, uint32_t nowMs);
    void goHome(uint32_t nowMs) { connectTo(transport.self(), nowMs); }

    PeerId host() const { return currentHost; }
    bool hasHost() const { return transport.valid(currentHost); }
    bool isGuestElsewhere() const { return hasHost() && currentHost != transport.self(); }
    int64_t localId() const { return myId; }
    bool hostIsCubeVulkan() const { return hostSaidHello; }
    // Live invitations: friend -> when it was last renewed.
    const std::map<PeerId, uint32_t> &invitations() const { return invitedBy; }
    bool hasInvitation(PeerId from) const { return invitedBy.count(from) != 0; }

    // The next packet 3 carries the whole record (after packet 4, connecting, a world reset).
    void requestFullUpdate() { lastSent.reset(); }

    // Start of the client tick: everything waiting on channel 0.
    void receive(uint32_t nowMs, Inbox &inbox);
    // Then the timers: host silence and invitation expiry. Returns true if it went home.
    bool checkTimers(uint32_t nowMs);
    // Then, at most every 32 ms with a valid host: [3] -> [14] -> [9] -> [10]s -> [11]s -> [12]s ->
    // [13]s -> and once a second one [15] per needed zone. `player` is the local player's record.
    // Returns whether a send tick happened.
    bool sendTick(uint32_t nowMs, const CreatureSnapshot &player);

    // End of the client frame (0x3cc40): takes from the client world's event lists the ones the
    // local player caused into the outbox (damage numbers by attacker, sounds by owner, buff
    // notices by source, effects by owner), while there is a valid host.
    void queueOwnEvents(const WorldEvents &clientWorldEvents);

    // A CubeVulkan extra to a CubeVulkan host; false otherwise.
    bool sendExtra(const CubeVulkanMessage &message);

  private:
    Transport &transport;
    World world;
    PeerId currentHost = 0;                    // client+0x70
    uint32_t lastHeard = 0;                    // client+0x9ec
    std::optional<CreatureSnapshot> lastSent;  // client+0x78; empty: the full-update flag is set
    std::set<int64_t> heard;                   // client+0x9f0
    std::map<PeerId, uint32_t> invitedBy;      // client+0xa00
    int64_t myId = 0;
    bool hostSaidHello = false;
    bool helloSent = false;
    std::optional<uint32_t> lastSendTick, lastZoneRound;

    void send(PeerId to, const Packet &packet);
};

} // namespace cwnet
