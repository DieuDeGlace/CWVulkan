#include "Multiplayer.h"

#include "Extracted.h"
#include "Spawned.h"
#include "SteamSession.h"
#include "net/SteamTransport.h"
#include "net/UdpTransport.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>

namespace {
constexpr cwnet::PeerId LOCAL_MACHINE = 1; // "me" on the loopback-only network
} // namespace

uint32_t Multiplayer::clockNow() {
    return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

Multiplayer::Multiplayer() = default;

Multiplayer::~Multiplayer() {
    regionLevelsChanged = {}; // caller-owned gameplay objects may already be destroyed
    // Leaving: the host tells its guests (packet 2), the client tells its host.
    if (hostGame)
        hostGame->link().dropAll();
    if (client && client->link().isGuestElsewhere())
        client->link().goHome(clockNow());
}

bool Multiplayer::start(const Setup &setup, std::string &error) {
    how = setup.way;
    steam = setup.steam;
    switch (how) {
    case Way::Steam:
        if (!steam || !steam->networkReady()) {
            error = "Steam networking is not available";
            return false;
        }
        transport = std::make_unique<cwnet::SteamTransport>(*steam);
        break;
    case Way::Address: {
        auto udp = std::make_unique<cwnet::UdpTransport>();
        if (!udp->open(setup.port, error))
            return false;
        udp->loss = setup.loss;
        transport = std::move(udp);
        break;
    }
    case Way::Local:
        loopback = std::make_unique<cwnet::LoopbackNetwork>();
        transport = loopback->endpoint(LOCAL_MACHINE);
        break;
    }
    if (setup.hostWorld && setup.blocks) {
        hostWorld = std::make_unique<HostSide>(*setup.hostWorld, *setup.blocks);
        hostWorld->playerObserved = [this, state = setup.hostWorld](const uint8_t *record) {
            if (!sharedRegions)
                return;
            int64_t id, position[2];
            int32_t level;
            std::memcpy(&id, record + worldgen::creature::ID, sizeof id);
            std::memcpy(position, record + worldgen::creature::POSITION, sizeof position);
            std::memcpy(&level, record + worldgen::creature::LEVEL, sizeof level);
            const auto region = worldgen::regionAt((int32_t)(position[0] >> 16), (int32_t)(position[1] >> 16));
            const auto previous = regionOccupancy.regionOf(id);
            if (previous && *previous == regionlevels::Region{region.x, region.y})
                return;
            const auto before = regionOccupancy.snapshot();
            state->keepHealthShares([&] { regionOccupancy.update(id, region, level); });
            if (before != regionOccupancy.snapshot() && regionLevelsChanged)
                regionLevelsChanged();
        };
        hostWorld->playerLeft = [this, state = setup.hostWorld](int64_t id) {
            if (!regionOccupancy.regionOf(id))
                return;
            const auto before = regionOccupancy.snapshot();
            state->keepHealthShares([&] { regionOccupancy.leave(id); });
            if (before != regionOccupancy.snapshot() && regionLevelsChanged)
                regionLevelsChanged();
        };
        hostGame = std::make_unique<cwnet::HostGame>(*transport, *hostWorld);
        hostGame->log = [this](const std::string &line) {
            if (log)
                log(line);
        };
    }
    if (hostGame)
        hostGame->extra = [this](cwnet::PeerId from, const cwnet::CubeVulkanMessage &message) {
            if (message.channel == "cwv.region-levels.request.v1" && message.payload.empty())
                sentRegionLevels.erase(from); // requests never contain client-authored region levels
        };
    if (setup.clientWorld && setup.blocks) {
        mirrorWorld = std::make_unique<MirrorSide>(*setup.clientWorld, *setup.blocks);
        client = std::make_unique<cwnet::ClientGame>(*transport, *mirrorWorld, [this] {
            if (hostGame)
                hostGame->link().dropAll();
        });
        client->extra = [this](const cwnet::CubeVulkanMessage &message) {
            if (message.channel != regionlevels::CHANNEL)
                return;
            const auto decoded = regionlevels::decode(message.payload);
            if (!decoded)
                return;
            bool changed;
            {
                std::lock_guard hold(regionViewMutex);
                changed = !regionView || regionView->enabled != decoded->enabled || regionView->levels != decoded->levels;
                regionView = *decoded;
            }
            if (changed && regionLevelsChanged)
                regionLevelsChanged();
        };
        // Single player: the client joins its own host through the loopback.
        client->link().goHome(clockNow());
    }
    return true;
}

void Multiplayer::setRegionScaling(bool enabled) {
    if (sharedRegions.exchange(enabled) != enabled) {
        regionOccupancy.clear();
        sentRegionLevels.clear();
        if (regionLevelsChanged)
            regionLevelsChanged();
    }
}

int Multiplayer::regionLevel(worldgen::Int2 region, int relativeFallback) const {
    if (remoteRegionView) {
        std::lock_guard hold(regionViewMutex);
        return regionView && regionView->enabled ? regionView->basis(region) : relativeFallback;
    }
    return sharedRegions ? regionOccupancy.basis(region) : relativeFallback;
}

void Multiplayer::visitLocalRegion(worldgen::Int2 region, int level) {
    if (transport || !sharedRegions)
        return;
    const auto before = regionOccupancy.snapshot();
    regionOccupancy.update(1, region, level);
    if (before != regionOccupancy.snapshot() && regionLevelsChanged)
        regionLevelsChanged();
}

void Multiplayer::clearRegionView() {
    requestedRegionHost = 0;
    std::lock_guard hold(regionViewMutex);
    regionView.reset();
}

void Multiplayer::sendRegionLevels() {
    if (!hostGame)
        return;
    const bool enabled = sharedRegions;
    const cwnet::CubeVulkanMessage message{regionlevels::CHANNEL,
        regionlevels::encode({enabled, enabled ? regionOccupancy.snapshot() : regionlevels::Levels{}})};
    const auto &connections = hostGame->link().connections();
    for (auto it = sentRegionLevels.begin(); it != sentRegionLevels.end();)
        if (!connections.count(it->first))
            it = sentRegionLevels.erase(it);
        else
            ++it;
    for (const auto &[peer, connection] : connections) {
        if (!connection.cubeVulkan) {
            sentRegionLevels.erase(peer);
            continue;
        }
        const auto previous = sentRegionLevels.find(peer);
        if (previous == sentRegionLevels.end() || previous->second != message.payload)
            if (hostGame->link().sendExtra(peer, message))
                sentRegionLevels[peer] = message.payload;
    }
}

void Multiplayer::beginFrame(uint32_t nowMs) {
    if (!transport)
        return;
    transport->pump(nowMs);
    // The host idles while its player is a guest elsewhere (netcode-transport.md §2).
    remoteRegionView = guestElsewhere();
    if (hostGame && !guestElsewhere()) {
        hostGame->frame(nowMs);
        sendRegionLevels();
    }
    if (client) {
        client->receive(nowMs);
        remoteRegionView = guestElsewhere();
        if (client->link().hostIsCubeVulkan() && requestedRegionHost != client->link().host()) {
            if (client->link().sendExtra({"cwv.region-levels.request.v1", {}}))
                requestedRegionHost = client->link().host();
        }
        for (cwnet::PeerId from : std::exchange(client->invitations, {}))
            notice("You were invited by " + nameOf(from), true);
        if (std::exchange(client->wentHome, false)) {
            clearRegionView();
            remoteRegionView = false;
            notice("Back in your own world");
        }
    }
    // Joins and leaves, as the host sees them.
    const size_t now = guests();
    if (hostGame && now != guestsBefore) {
        if (log)
            log("Multiplayer: " + std::to_string(now) + " player(s) on this host");
        guestsBefore = now;
    }
}

void Multiplayer::consume() {
    if (client)
        client->consumeWorldUpdates();
}

void Multiplayer::endFrame(uint32_t nowMs) {
    if (client)
        client->send(nowMs);
}

bool Multiplayer::joinAddress(const std::string &address, uint16_t port, std::string &error) {
    if (how != Way::Address || !client) {
        error = "joining by address needs the address transport";
        return false;
    }
    const cwnet::PeerId host = cwnet::UdpTransport::resolve(address, port);
    if (!host) {
        error = "cannot resolve " + address;
        return false;
    }
    clearRegionView();
    client->link().connectTo(host, clockNow());
    remoteRegionView = guestElsewhere();
    return true;
}

void Multiplayer::join(uint64_t host) {
    if (client) {
        clearRegionView();
        client->link().connectTo(host, clockNow());
        remoteRegionView = guestElsewhere();
    }
}

void Multiplayer::leave() {
    if (client) {
        clearRegionView();
        client->link().goHome(clockNow());
        remoteRegionView = false;
    }
}

bool Multiplayer::guestElsewhere() const { return client && client->link().isGuestElsewhere(); }

int64_t Multiplayer::playerId() const { return client ? client->link().localId() : 0; }

size_t Multiplayer::guests() const { return hostGame ? const_cast<cwnet::HostGame &>(*hostGame).link().connections().size() : 0; }

std::vector<Multiplayer::FriendRow> Multiplayer::friends() const {
    std::vector<FriendRow> rows;
    if (!transport)
        return rows;
    auto rowOf = [&](uint64_t id) {
        FriendRow row;
        row.id = id;
        row.name = nameOf(id);
        if (client) {
            row.hostOfMine = client->link().host() == id && id != transport->self();
            row.invitation = client->link().hasInvitation(id);
        }
        if (hostGame) {
            row.joined = hostGame->link().isConnected(id);
            row.invited = hostGame->link().isInvited(id);
        }
        return row;
    };
    if (how == Way::Steam && steam) {
        // The immediate friends with valid individual SteamIDs (0xb33f0).
        for (uint64_t id : steam->friendIds())
            if (cwnet::validSteamId(id))
                rows.push_back(rowOf(id));
        return rows;
    }
    // By address there is no friends list (CubeVulkan's own): the players on this host and the
    // host this player is a guest of, so they can be kicked or left from the same menu.
    if (hostGame)
        for (const auto &[peer, guest] : const_cast<cwnet::HostGame &>(*hostGame).link().connections())
            if (peer != transport->self())
                rows.push_back(rowOf(peer));
    if (guestElsewhere())
        rows.push_back(rowOf(client->link().host()));
    return rows;
}

void Multiplayer::hostButton(uint64_t friendId) {
    if (!hostGame)
        return;
    cwnet::HostLink &link = hostGame->link();
    if (link.isInvited(friendId) || link.isConnected(friendId)) {
        link.kick(friendId);
        notice("Uninvited " + nameOf(friendId));
    } else {
        link.invite(friendId, clockNow());
        notice("Invited " + nameOf(friendId));
    }
}

void Multiplayer::guestButton(uint64_t friendId) {
    if (!client)
        return;
    if (client->link().host() == friendId && friendId != transport->self())
        leave();
    else if (client->link().hasInvitation(friendId)) {
        join(friendId);
        notice("Joining " + nameOf(friendId));
    }
}

bool Multiplayer::invite(const std::string &friendName) {
    if (how != Way::Steam || !steam || !hostGame)
        return false;
    auto lower = [](std::string s) {
        for (char &c : s)
            c = (char)std::tolower((unsigned char)c);
        return s;
    };
    const bool number = !friendName.empty() && std::all_of(friendName.begin(), friendName.end(), [](char c) { return std::isdigit((unsigned char)c); });
    for (uint64_t id : steam->friendIds())
        if ((number && std::to_string(id) == friendName) || lower(steam->friendName(id)) == lower(friendName)) {
            hostGame->link().invite(id, clockNow());
            notice("Invited " + steam->friendName(id));
            return true;
        }
    return false;
}

std::vector<cwnet::ChatLine> Multiplayer::takeChat() { return client ? std::exchange(client->chat, {}) : std::vector<cwnet::ChatLine>{}; }

void Multiplayer::say(const std::u16string &line) {
    if (client && !line.empty())
        client->link().outbox.chat.push_back(line);
}

void Multiplayer::notice(const std::string &line, bool invitation) {
    notices.push_back({line, invitation});
    if (log)
        log("Multiplayer: " + line);
}

std::string Multiplayer::nameOf(uint64_t peer) const {
    switch (how) {
    case Way::Steam: return steam ? steam->friendName(peer) : std::to_string(peer);
    case Way::Address: return cwnet::UdpTransport::describe(peer);
    default: return std::to_string(peer);
    }
}
