#pragma once
// What the host tick and the client's frame need from a world (docs/specs/netcode-roles.md).
//
// Every running game has two worlds: the host world, which decides everything that is not a
// player (it is "authoritative"), and the client world, a mirror of whichever host the player
// is connected to (its own host too, by loopback). The protocol side (net/HostGame.h,
// net/ClientGame.h) drives them through these two interfaces, so it knows nothing of how a
// world keeps its creatures, zones or events, and a test can stand in a small fake world.
//
// Records are the game's own layouts (net/Records.h); creature records are whole
// game-layout records (0x15f8 bytes), of which the network sends the part a CreatureSnapshot
// holds.
#include "net/CreatureDelta.h"
#include "net/Packets.h"

#include <optional>
#include <set>
#include <string>
#include <vector>

namespace cwnet {

// Creature record offsets the network side reads itself.
namespace creature {
constexpr size_t ID = 0x08;        // i64
constexpr size_t POSITION = 0x10;  // i64 x3, 1/65536 block
constexpr size_t HOSTILITY = 0x60; // byte: 0 is a player
constexpr size_t FLAGS = 0x80;     // u32
constexpr size_t HEALTH = 0x180;   // f32
constexpr uint32_t ALWAYS_SENT = 0x800 | 0x2000; // newcomers with these flags skip the one-a-tick queue
} // namespace creature

// One creature of a world, as interest management reads it.
struct CreatureView {
    int64_t id = 0;
    const uint8_t *record = nullptr; // the whole game-layout record

    uint8_t hostility() const { return record[creature::HOSTILITY]; }
    uint32_t flags() const;
    const int64_t *position() const { return reinterpret_cast<const int64_t *>(record + creature::POSITION); }
};

// A zone's state as the host answers a zone request (netcode-roles.md §4.2).
struct ZoneAnswer {
    std::vector<BlockChangeRecord> blockChanges;          // its change log (zone+0xd8)
    std::optional<std::vector<ZoneObjectRecord>> groundItems;   // when they were ever changed (zone+0xf9)
    std::optional<std::vector<ZoneObjectRecord>> staticObjects; // when they were ever changed (zone+0xfa)
};

// What the guests sent in one tick (packets 10-13), for the host world to apply (§2 step 5).
struct GuestEvents {
    std::vector<SoundRecord> sounds;
    std::vector<DamageRecord> damageNumbers;
    std::vector<BuffRecord> buffs;
    std::vector<EffectRecord> effects;
};

// The host world: authoritative, with no local player of its own. Every player in it, the one
// at this machine included, is a guest's creature made from that guest's packets 3.
class HostWorld {
  public:
    virtual ~HostWorld() = default;

    // Every creature in the world's own order (world+0x40).
    virtual void creatures(std::vector<CreatureView> &out) = 0;
    virtual bool hasCreature(int64_t id) = 0;
    // A guest's packet 3 about its own creature: found or made (with that id), the delta written
    // into its record, its drawn position smoothed (netcode-packets.md §2.4).
    virtual void receiveCreature(const CreatureDelta &delta) = 0;
    // A guest left or was kicked: its creature goes (0x2d2260).
    virtual void removeCreature(int64_t id) = 0;
    // Packet 9: appended to the creature's pending interactions, which the world performs.
    virtual void queueInteraction(int64_t creatureId, const InteractionRecord &record) = 0;
    // Step 5 of the tick: sounds into the world's list, damage numbers applied (0x29dcf0) and
    // listed, buffs given and listed, effect objects set going and listed for the guests.
    virtual void applyGuestEvents(const GuestEvents &events) = 0;
    // Step 6: `dtMs` of play (at most 1000), in 16-ms world steps, then the drawn-position
    // smoothing and the zone lists of what changed.
    virtual void advance(uint32_t dtMs) = 0;
    // Optional engine timers take actual elapsed host time, before the simulation cap.
    virtual void advanceTimers(uint32_t elapsedMs) {}
    // The world's event lists of this tick (world+0x90 .. +0x170), which the tick hands out to
    // the guests and then empties.
    virtual WorldEvents &events() = 0;
    // A zone request's answer; false when the zone is not in memory (the guest asks again).
    virtual bool zoneAnswer(const ZoneKey &zone, ZoneAnswer &out) = 0;
    // The world's day and time of day (packet 8).
    virtual Time clock() = 0;
};

// The client world: a mirror of its host. It spawns nothing, runs no AI and performs no
// interactions; its creatures are made, changed and removed by the host's packets, and its
// local player is the player at this machine.
class MirrorWorld {
  public:
    virtual ~MirrorWorld() = default;

    // --- The local player ---
    // Packet 4: the local player takes the creature id the host gave it.
    virtual void setLocalId(int64_t id) = 0;
    // The local player's record as it is sent (false while there is none).
    virtual bool localSnapshot(CreatureSnapshot &out) = 0;
    // Its first pending interaction, taken off its list (one is sent per send tick).
    virtual bool takeInteraction(InteractionRecord &out) = 0;

    // --- Creatures from the host ---
    // Packet 3 about another creature: found or made, the delta applied, the smoothing armed.
    virtual void receiveCreature(const CreatureDelta &delta) = 0;
    // Packet 6: every creature but the local player not in `heard` is removed.
    virtual void keepOnly(const std::set<int64_t> &heard) = 0;
    // Connecting to any host: every creature but the local player goes, every loaded zone's
    // block changes are undone and every loaded zone needs its state again (§7).
    virtual void startOver() = 0;

    // --- Time (packet 8) ---
    virtual int32_t day() = 0;
    // A day other than the world's runs the day change (0x2c34c0) first.
    virtual void dayChange(int32_t newDay) = 0;
    virtual void setClock(int32_t day, int32_t timeOfDayMs) = 0;

    // --- A world update's lists, consumed in the frame routine's order (§8) ---
    virtual void applyBlockChange(const BlockChangeRecord &) {}
    virtual void zoneAnswered(const ZoneKey &) {}
    virtual void replaceGroundItems(const ZoneObjects &) {}
    virtual void replaceStaticObjects(const ZoneObjects &) {}
    virtual void giveBuff(const BuffRecord &) {}
    virtual void setAggro(const AggroRecord &) {}
    virtual void zoneChangeStep() {}
    virtual void markDead(int64_t) {}
    // Item 9: the lists the world's own consumers play, show or act on.
    virtual void keepEvents(const WorldEvents &) {}
    virtual void feat(const FeatRecord &) {}
    virtual bool hasCreature(int64_t) { return false; }
    virtual bool isDead(int64_t) { return false; }
    // A dead target collected for its death effects.
    virtual void deathSeen(int64_t) {}
    // A monster's hit on the local player, applied to its own record.
    virtual void hitLocalPlayer(const DamageRecord &) {}
    // Another creature's hit flash (+0x994 = 1.0).
    virtual void flash(int64_t) {}
    virtual void addEffect(const EffectRecord &) {}
    // A damage number made by someone else, to be shown.
    virtual void showDamageNumber(const DamageRecord &) {}

    // --- What the client sends ---
    // The world's own event lists of this frame (the local player's are sent to the host); the
    // world empties them itself after the frame.
    virtual const WorldEvents &ownEvents() = 0;
    // The loaded zones near the local player still needing their state (zone requests).
    virtual std::vector<ZoneKey> neededZones() = 0;
};

} // namespace cwnet
