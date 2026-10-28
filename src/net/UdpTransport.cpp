#include "net/UdpTransport.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <cstring>

namespace cwnet {

namespace {

constexpr uint32_t MAGIC = 0x31555643; // "CVU1"
constexpr uint8_t KIND_DATA = 0, KIND_ACK = 1;
constexpr uint8_t LAST_FRAGMENT = 1;
constexpr size_t HEADER_BYTES = 12;
constexpr unsigned long long NO_SOCKET = ~0ull;

struct Header {
    uint32_t magic = MAGIC;
    uint8_t kind = KIND_DATA;
    uint8_t channel = 0;
    uint8_t flags = 0;
    uint8_t reserved = 0;
    uint32_t number = 0;
};
static_assert(sizeof(Header) == HEADER_BYTES);

PeerId peerOf(const sockaddr_in &a) { return ((PeerId)ntohl(a.sin_addr.s_addr) << 16) | ntohs(a.sin_port); }

sockaddr_in addressOf(PeerId peer) {
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl((uint32_t)(peer >> 16));
    a.sin_port = htons((uint16_t)(peer & 0xffff));
    return a;
}

Bytes datagram(const Header &h, const uint8_t *data, size_t size) {
    Bytes out(HEADER_BYTES + size);
    std::memcpy(out.data(), &h, HEADER_BYTES);
    if (size)
        std::memcpy(out.data() + HEADER_BYTES, data, size);
    return out;
}

// Winsock is started once per process and left running.
bool startWinsock() {
    static const bool started = [] {
        WSADATA data;
        return WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }();
    return started;
}

} // namespace

UdpTransport::UdpTransport() : socketHandle(NO_SOCKET) {}

UdpTransport::~UdpTransport() {
    if (socketHandle != NO_SOCKET)
        closesocket((SOCKET)socketHandle);
}

bool UdpTransport::open(uint16_t port, std::string &error) {
    if (!startWinsock()) {
        error = "Windows sockets could not start";
        return false;
    }
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) {
        error = "cannot make a UDP socket";
        return false;
    }
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = htons(port);
    if (bind(s, (sockaddr *)&a, sizeof a) != 0) {
        closesocket(s);
        error = "UDP port " + std::to_string(port) + " is in use";
        return false;
    }
    u_long nonBlocking = 1;
    ioctlsocket(s, FIONBIO, &nonBlocking);
    int length = sizeof a;
    getsockname(s, (sockaddr *)&a, &length);
    boundPort = ntohs(a.sin_port);
    socketHandle = (unsigned long long)s;
    me = ((PeerId)INADDR_LOOPBACK << 16) | boundPort;
    return true;
}

PeerId UdpTransport::resolve(const std::string &host, uint16_t port) {
    if (!startWinsock())
        return 0;
    addrinfo hints{}, *found = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(host.c_str(), nullptr, &hints, &found) != 0 || !found)
        return 0;
    sockaddr_in a = *(const sockaddr_in *)found->ai_addr;
    freeaddrinfo(found);
    a.sin_port = htons(port);
    return peerOf(a);
}

std::string UdpTransport::describe(PeerId peer) {
    const uint32_t ip = (uint32_t)(peer >> 16);
    return std::to_string(ip >> 24) + "." + std::to_string((ip >> 16) & 255) + "." + std::to_string((ip >> 8) & 255) +
           "." + std::to_string(ip & 255) + ":" + std::to_string(peer & 0xffff);
}

void UdpTransport::transmit(PeerId to, const Bytes &bytes) {
    if (socketHandle == NO_SOCKET)
        return;
    if (loss > 0) {
        lossDice = lossDice * 1103515245u + 12345u;
        if ((float)((lossDice >> 8) & 0xffff) / 65536.0f < loss)
            return;
    }
    const sockaddr_in a = addressOf(to);
    sendto((SOCKET)socketHandle, (const char *)bytes.data(), (int)bytes.size(), 0, (const sockaddr *)&a, sizeof a);
}

bool UdpTransport::send(PeerId to, Channel channel, const Bytes &packet) {
    std::lock_guard<std::mutex> hold(lock);
    if (to == me) {
        inbox[(int)channel].push_back({me, packet});
        return true;
    }
    if (socketHandle == NO_SOCKET || to == 0)
        return false;
    Stream &stream = streams[{to, (int)channel}];
    // Cut into fragments; an empty packet is one empty last fragment.
    size_t at = 0;
    do {
        const size_t n = std::min(FRAGMENT_BYTES, packet.size() - at);
        Header h;
        h.channel = (uint8_t)channel;
        h.flags = at + n == packet.size() ? LAST_FRAGMENT : 0;
        h.number = stream.nextOut++;
        Sent sent{datagram(h, packet.data() + at, n), clock};
        transmit(to, sent.datagram);
        stream.unacked.emplace(h.number, std::move(sent));
        at += n;
    } while (at < packet.size());
    return true;
}

void UdpTransport::handle(PeerId from, const uint8_t *data, size_t size) {
    if (size < HEADER_BYTES)
        return;
    Header h;
    std::memcpy(&h, data, HEADER_BYTES);
    if (h.magic != MAGIC || h.channel > 1)
        return;
    Stream &stream = streams[{from, (int)h.channel}];
    if (h.kind == KIND_ACK) {
        stream.unacked.erase(stream.unacked.begin(), stream.unacked.lower_bound(h.number));
        return;
    }
    if (h.kind != KIND_DATA)
        return;
    // Keep it if it is new (numbers wrap after 4 billion fragments; a session never gets there).
    if (h.number >= stream.nextIn && !stream.early.count(h.number))
        stream.early[h.number] = {Bytes(data + HEADER_BYTES, data + size), (h.flags & LAST_FRAGMENT) != 0};
    // Deliver what is now in order.
    for (auto it = stream.early.find(stream.nextIn); it != stream.early.end(); it = stream.early.find(stream.nextIn)) {
        stream.assembling.insert(stream.assembling.end(), it->second.bytes.begin(), it->second.bytes.end());
        if (it->second.last) {
            inbox[h.channel].push_back({from, std::move(stream.assembling)});
            stream.assembling.clear();
        }
        stream.early.erase(it);
        stream.nextIn++;
    }
    // Acknowledge, also for a repeat (our last acknowledgement may have been lost).
    Header ack;
    ack.kind = KIND_ACK;
    ack.channel = h.channel;
    ack.number = stream.nextIn;
    transmit(from, datagram(ack, nullptr, 0));
}

void UdpTransport::readSocket() {
    if (socketHandle == NO_SOCKET)
        return;
    uint8_t buffer[65536];
    for (int n = 0; n < 4096; n++) {
        sockaddr_in a{};
        int length = sizeof a;
        const int got = recvfrom((SOCKET)socketHandle, (char *)buffer, sizeof buffer, 0, (sockaddr *)&a, &length);
        if (got < 0)
            break; // nothing more (or an ICMP "port unreachable" from a peer that left)
        handle(peerOf(a), buffer, (size_t)got);
    }
}

bool UdpTransport::receive(Channel channel, Received &out) {
    std::lock_guard<std::mutex> hold(lock);
    readSocket();
    auto &queue = inbox[(int)channel];
    if (queue.empty())
        return false;
    out = std::move(queue.front());
    queue.pop_front();
    return true;
}

void UdpTransport::pump(uint32_t nowMs) {
    std::lock_guard<std::mutex> hold(lock);
    clock = nowMs;
    readSocket();
    for (auto &[key, stream] : streams)
        for (auto &[number, sent] : stream.unacked)
            if (nowMs - sent.lastSent >= RESEND_MS) {
                sent.lastSent = nowMs;
                transmit(key.first, sent.datagram);
            }
}

size_t UdpTransport::unacknowledged() const {
    std::lock_guard<std::mutex> hold(lock);
    size_t n = 0;
    for (const auto &[key, stream] : streams)
        n += stream.unacked.size();
    return n;
}

} // namespace cwnet
