#include "net/SteamTransport.h"

#include "SteamSession.h"

namespace cwnet {

bool validSteamId(uint64_t id) {
    const uint32_t high = (uint32_t)(id >> 32), account = (uint32_t)id;
    const uint32_t universe = high >> 24, type = (high >> 20) & 0xf, instance = high & 0xfffff;
    if (universe < 1 || universe > 4 || type < 1 || type > 10)
        return false;
    constexpr uint32_t INDIVIDUAL = 1, CLAN = 3, CHAT = 7;
    constexpr uint32_t MAX_INDIVIDUAL_INSTANCE = 4;
    switch (type) {
    case INDIVIDUAL: return account != 0 && instance <= MAX_INDIVIDUAL_INSTANCE;
    case CHAT: return account != 0 && instance == 0;
    case CLAN: return account != 0;
    default: return true;
    }
}

SteamTransport::SteamTransport(SteamSession &steam) : steam(steam), me(steam.steamId()) {
    steam.acceptFriendSessions(true);
}

SteamTransport::~SteamTransport() { steam.acceptFriendSessions(false); }

bool SteamTransport::send(PeerId to, Channel channel, const Bytes &packet) {
    if (to == me) {
        std::lock_guard<std::mutex> hold(lock);
        local[(int)channel].push_back({me, packet});
        return true;
    }
    return steam.p2pSendOn(to, packet.data(), (uint32_t)packet.size(), SteamSession::SEND_RELIABLE, (int)channel);
}

bool SteamTransport::receive(Channel channel, Received &out) {
    // What Steam has first, then the packets to ourselves, as the host tick reads them (§1.5).
    out.from = 0;
    if (steam.p2pReadOn(out.bytes, out.from, (int)channel))
        return true;
    std::lock_guard<std::mutex> hold(lock);
    auto &queue = local[(int)channel];
    if (queue.empty())
        return false;
    out = std::move(queue.front());
    queue.pop_front();
    return true;
}

void SteamTransport::pump(uint32_t) { steam.pump(); }

} // namespace cwnet
