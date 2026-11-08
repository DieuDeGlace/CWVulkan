#pragma once
// The game's network client, as each frame runs it (docs/specs/netcode-roles.md §5, §8;
// netcode-packets.md §2, §4).
//
// The client world is always a mirror of a host: of a friend's when this player joined one,
// otherwise of this machine's own host through the loopback (that is single player). Each frame:
//
//   ... the client World steps, smooths and consumes previously pending lists ...
//   ... local player input/control ...
//   prepareSourceRequests()  snapshots the already built request coordinates;
//   receive()               dispatches one batch in queue order:3/4/6/8 are
//                           immediate,7 appends lists for a later consume pass;
//   send()                  connection maintenance and the outgoing send tick;
//   ... camera/interface/renderer ...
//   collectOwnEvents()      queues this frame's local events for a later send.
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
    void prepareSourceRequests();
    void send(uint32_t nowMs);
    void collectOwnEvents();

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
