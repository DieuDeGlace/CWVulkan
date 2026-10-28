#pragma once
// The game's host: one tick of its loop, about every 32 ms (docs/specs/netcode-roles.md §2,
// §4; netcode-transport.md §2).
//
// A tick, in the game's order:
//   1. everything the guests sent is received (their own creatures' records and interactions);
//   2. invited friends who have not joined are invited again every 5 s;
//   3. each guest whose creature exists gets the creatures it can see, then 6, 8, 7 (the events
//      gathered for it last tick) and one chat line;
//   4-5. the damage numbers, sounds, buffs and effect objects the guests caused are applied to
//      the host world;
//   6. the world runs for the time since the last tick, in 16-ms steps;
//   7. after a world reset (a sleep notice of 0) every guest forgets the creatures it knows;
//   8. this tick's events go into each guest's waiting lists (sounds and particles only near it),
//      and the zones it asked for are answered;
//   9. the world's event lists are emptied.
//
// What a guest sees (§4.1): every player, and every other creature within 270 blocks of its
// own; creatures new to it arrive one a tick (with a full record), and one that leaves its view
// is removed by its absence from the tick's updates.
#include "net/Session.h"
#include "net/Worlds.h"

#include <functional>
#include <string>

namespace cwnet {

class HostGame {
  public:
    static constexpr uint32_t TICK_MS = 32;       // a tick at most this often
    static constexpr uint32_t MAX_STEP_MS = 1000; // a tick never runs the world longer
    static constexpr double VIEW_BLOCKS = 270.0;  // non-players farther than this are not sent (0x42e7dc)
    static constexpr double EVENT_BLOCKS = 256.0; // sounds and particles farther than this are not sent (0x42e5c4)

    HostGame(Transport &transport, HostWorld &world);

    HostLink &link() { return hostLink; }

    // One tick if one is due (`nowMs` from any steady millisecond clock). Returns whether one ran.
    bool frame(uint32_t nowMs);
    // One tick now, with `elapsedMs` since the last.
    void tick(uint32_t nowMs, uint32_t elapsedMs);

    // Lines for the log (joins, leaves, "MESSAGE???", the death and mission sounds sent).
    std::function<void(const std::string &)> log;
    // CubeVulkan extras (packet 26) from CubeVulkan guests, for mods and engine features.
    std::function<void(PeerId from, const CubeVulkanMessage &message)> extra;

    // The test of 0xf3e20: whether two positions (1/65536 block) are farther apart than `blocks`,
    // in double precision: |dx| or |dy| over it, or the 3-D distance over it.
    static bool tooFar(const int64_t a[3], const int64_t b[3], double blocks);

  private:
    Transport &transport;
    HostWorld &world;
    HostLink hostLink;
    std::optional<uint32_t> lastTick;
    std::vector<CreatureView> list; // reused each tick

    void sendToGuest(HostLink::Connection &guest);
    void distribute(HostLink::Connection &guest);
};

} // namespace cwnet
