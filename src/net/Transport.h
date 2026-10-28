#pragma once
// How packets move between machines (docs/specs/netcode-transport.md §1.4, §1.5).
//
// The game sends every packet reliably and in order: host -> client on channel 0, client -> host
// on channel 1. A machine runs a host and a client side by side; the host reads channel 1 and the
// client reads channel 0, so both share one transport. A packet a machine sends to itself (its
// own id) never leaves the process: that is single player, the client talking to its own host.
//
// Three transports implement this:
//   * SteamTransport (net/SteamTransport.h): Steam P2P, as the game does; talks to original-game
//     players.
//   * LoopbackNetwork (below): in-process, for single player and for tests.
//   * UdpTransport (net/UdpTransport.h): CubeVulkan only, by address, for dedicated servers.
#include "net/Wire.h"

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <utility>

namespace cwnet {

// Who a packet comes from or goes to: a SteamID over Steam, an address over UDP, any non-zero
// number in a loopback network. 0 means nobody.
using PeerId = uint64_t;

enum class Channel : int {
    ToClient = 0, // host -> client
    ToHost = 1,   // client -> host
};

struct Received {
    PeerId from = 0;
    Bytes bytes;
};

class Transport {
  public:
    virtual ~Transport() = default;

    // This machine's own id ("me").
    virtual PeerId self() const = 0;
    // Whether `peer` names someone a packet can be sent to (the game's SteamID test over Steam).
    virtual bool valid(PeerId peer) const = 0;
    // Sends one packet, reliably and in order on its channel. False if it could not be queued.
    virtual bool send(PeerId to, Channel channel, const Bytes &packet) = 0;
    // Takes the next packet waiting on `channel`; false when none is.
    virtual bool receive(Channel channel, Received &out) = 0;
    // Lets the transport do its own work (Steam callbacks, UDP resends). Call once a frame.
    virtual void pump(uint32_t nowMs) { (void)nowMs; }
    // Whether every peer on this transport is CubeVulkan (UDP): extras need no hello there.
    virtual bool cubeVulkanOnly() const { return false; }
};

// An in-process network. Each endpoint() is a transport with its own id; packets go into the
// receiving endpoint's queue for that channel. Thread-safe, so a host thread and a game thread
// can share it, as the game's host and client do.
class LoopbackNetwork {
  public:
    // A transport for the peer `id` (non-zero). The network must outlive it.
    std::unique_ptr<Transport> endpoint(PeerId id);

    // Packets waiting for `id` on `channel`.
    size_t waiting(PeerId id, Channel channel) const;

  private:
    class Endpoint;
    mutable std::mutex lock;
    std::map<std::pair<PeerId, int>, std::deque<Received>> queues;
};

} // namespace cwnet
