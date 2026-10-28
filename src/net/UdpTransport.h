#pragma once
// CubeVulkan's own transport for dedicated servers reached by address (not part of the game).
//
// The game's protocol assumes Steam's reliable, ordered channels: creature deltas only work if
// nothing is lost or reordered (netcode-compatibility.md §2, "Reliability"). Plain UDP gives
// neither, so this transport adds them: every packet is cut into numbered fragments per peer and
// channel, the receiver acknowledges the next number it expects, the sender resends what is not
// acknowledged, and the receiver delivers whole packets in order. Every peer here is CubeVulkan.
//
// Datagram layout (little-endian):
//   u32 magic "CVU1", u8 kind (0 data, 1 acknowledgement), u8 channel, u8 flags (bit 0: the
//   packet's last fragment), u8 reserved, u32 number; then, for data, the fragment's bytes.
//   An acknowledgement's number is the next fragment number the receiver expects.
#include "net/Transport.h"

#include <map>
#include <mutex>
#include <string>

namespace cwnet {

class UdpTransport : public Transport {
  public:
    UdpTransport();
    ~UdpTransport() override;
    UdpTransport(const UdpTransport &) = delete;
    UdpTransport &operator=(const UdpTransport &) = delete;

    // Opens a socket on `port` (0: any free port), on every interface.
    bool open(uint16_t port, std::string &error);
    uint16_t port() const { return boundPort; }

    // The peer id of an IPv4 address and port (address and port packed: ip << 16 | port), or 0
    // if the name does not resolve. Accepts "a.b.c.d" and host names.
    static PeerId resolve(const std::string &host, uint16_t port);
    static std::string describe(PeerId peer);

    PeerId self() const override { return me; }
    bool valid(PeerId peer) const override { return peer != 0; }
    bool send(PeerId to, Channel channel, const Bytes &packet) override;
    bool receive(Channel channel, Received &out) override;
    // Reads the socket and resends what is not yet acknowledged. Call every frame or tick.
    void pump(uint32_t nowMs) override;
    bool cubeVulkanOnly() const override { return true; }

    // Packets sent but not yet fully acknowledged (all peers).
    size_t unacknowledged() const;
    // Testing: the fraction of outgoing datagrams dropped on purpose.
    float loss = 0;

    static constexpr size_t FRAGMENT_BYTES = 1100;
    static constexpr uint32_t RESEND_MS = 100;

  private:
    struct Sent {
        Bytes datagram;
        uint32_t lastSent = 0;
    };
    struct Fragment {
        Bytes bytes;
        bool last = false;
    };
    // One peer's state on one channel.
    struct Stream {
        uint32_t nextOut = 0;               // the next fragment number we send
        std::map<uint32_t, Sent> unacked;   // ours, waiting for their acknowledgement
        uint32_t nextIn = 0;                // the next fragment number we deliver
        std::map<uint32_t, Fragment> early; // theirs, arrived ahead of nextIn
        Bytes assembling;                   // the packet being put together
    };

    unsigned long long socketHandle;
    uint16_t boundPort = 0;
    PeerId me = 0;
    uint32_t clock = 0;
    uint32_t lossDice = 0x2545f491;
    mutable std::mutex lock;
    std::map<std::pair<PeerId, int>, Stream> streams;
    std::map<int, std::deque<Received>> inbox;

    void readSocket();
    void handle(PeerId from, const uint8_t *data, size_t size);
    void transmit(PeerId to, const Bytes &datagram);
};

} // namespace cwnet
