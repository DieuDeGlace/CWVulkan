#pragma once
// Steam P2P as Steam Cube World uses it (docs/specs/netcode-transport.md §1): the account's
// SteamNetworking005 through the game's own steam_api64.dll (loaded by SteamSession), every
// packet reliable (send type 2), host -> client on channel 0 and client -> host on channel 1,
// sessions accepted only from the account's immediate friends. A packet to one's own SteamID
// stays in the process, as the game's loopback does, so single player runs the same protocol.
#include "net/Transport.h"

#include <deque>
#include <mutex>

class SteamSession;

namespace cwnet {

// The game's SteamID test (0x3be00): universe 1..4; account type 1..10; an individual account
// (type 1) needs a non-zero account number and an instance below 5, type 7 a non-zero account
// number and instance 0, type 3 a non-zero account number.
bool validSteamId(uint64_t id);

class SteamTransport : public Transport {
  public:
    // Turns on friends-only session acceptance for as long as the transport lives. `steam` must
    // have started and must outlive the transport.
    explicit SteamTransport(SteamSession &steam);
    ~SteamTransport() override;
    SteamTransport(const SteamTransport &) = delete;
    SteamTransport &operator=(const SteamTransport &) = delete;

    PeerId self() const override { return me; }
    bool valid(PeerId peer) const override { return validSteamId(peer); }
    bool send(PeerId to, Channel channel, const Bytes &packet) override;
    bool receive(Channel channel, Received &out) override;
    void pump(uint32_t nowMs) override;

  private:
    SteamSession &steam;
    PeerId me = 0;
    std::mutex lock;
    std::deque<Received> local[2]; // packets to ourselves, by channel
};

} // namespace cwnet
