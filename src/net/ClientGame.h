#pragma once
// The game's network client, as each frame runs it (docs/specs/netcode-roles.md §5, §8;
// netcode-packets.md §2, §4).
//
// The client world is always a mirror of a host: of a friend's when this player joined one,
// otherwise of this machine's own host through the loopback (that is single player). Each frame:
//
//   receive()             the host's packets: creatures made, changed and removed (3, 6), the
//                         player's id (4), the clock (8, a new day running the day change), chat;
//                         the world updates (7) are kept for...
//   consumeWorldUpdates() ...the frame routine's pass over them, in the game's order, before the
//                         client world's step: blocks, zone states, buffs, aggro, kill credits,
//                         the lists the world's consumers play, damage numbers (a monster's hit
//                         on the local player is applied here, by its own client), effects;
//   send()                at the end of the frame: what the local player caused this frame is
//                         queued (its hits, sounds, buffs and effects), and at most every 32 ms
//                         its record, one chat line, one interaction, those events and, once a
//                         second, the zones still needing their state go to the host.
#include "net/Session.h"
#include "net/Worlds.h"

#include <functional>

namespace cwnet {

class ClientGame {
  public:
    // `ownHostDropAll`: connecting anywhere makes this machine's own host drop its guests.
    ClientGame(Transport &transport, MirrorWorld &world, std::function<void()> ownHostDropAll = {});

    GuestLink &link() { return guestLink; }
    const GuestLink &link() const { return guestLink; }

    void receive(uint32_t nowMs);
    void consumeWorldUpdates();
    void send(uint32_t nowMs);

    // What happened for the player to see, taken by the game.
    std::vector<ChatLine> chat;         // lines received, with the speaker's creature id
    std::vector<PeerId> invitations;    // new invitations ("You were invited by ...")
    bool wentHome = false;              // the host left or fell silent: back in one's own world
    std::function<void(const CubeVulkanMessage &)> extra; // CubeVulkan extras from a CubeVulkan host

  private:
    MirrorWorld &world;
    GuestLink guestLink;
    std::vector<WorldEvents> received; // packets 7 not yet consumed
};

} // namespace cwnet
