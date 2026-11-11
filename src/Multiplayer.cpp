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
    joinHostThreads(); // the threads use everything below
    {
        // Posted results refer to game-thread state that may already be gone; the game
        // runs them through stopHostThread() at its own teardown.
        std::lock_guard hold(postedMutex);
        posted.clear();
    }
    regionLevelsChanged = {}; // caller-owned gameplay objects may already be destroyed
    // Leaving: the host tells its guests (packet 2), the client tells its host.
    if (hostGame)
        hostGame->link().dropAll();
    if (client && client->link().isGuestElsewhere())
        client->link().goHome(clockNow());
}

bool Multiplayer::start(const Setup &setup, std::string &error) {
    worldIdentity = setup.blocks ? setup.blocks->sourceIdentity() : std::string{};
    worldIdentityRequired = setup.blocks && setup.blocks->generated() && worldgen::usingAlphaTerrain();
    worldDescriptor = {"cwv.world-generation.v1", {1, uint8_t(worldIdentityRequired)}};
    worldDescriptor.payload.insert(worldDescriptor.payload.end(), worldIdentity.begin(), worldIdentity.end());
    sentWorldIdentities.clear();
    pendingWorldIdentities.clear();
    requestedWorldEpoch = pendingWorldEpoch = uint64_t(-1);
    how = setup.way;
    steam = setup.steam;
    switch (how) {
    case Way::Steam:
        if (!steam || !steam->networkReady()) {
            error = "Steam networking is not available";
            return false;
        }
        // A host thread may share it with the game thread: one lock around the backend.
        transport = std::make_unique<cwnet::SerializedTransport>(std::make_unique<cwnet::SteamTransport>(*steam));
        break;
    case Way::Address: {
        auto udp = std::make_unique<cwnet::UdpTransport>();
        if (!udp->open(setup.port, error))
            return false;
        udp->loss = setup.loss;
        transport = std::make_unique<cwnet::SerializedTransport>(std::move(udp));
        break;
    }
    case Way::Local:
        loopback = std::make_unique<cwnet::LoopbackNetwork>();
        transport = loopback->endpoint(LOCAL_MACHINE);
        break;
    }
    if (setup.hostWorld && setup.blocks) {
        authorityBlocks = setup.blocks;
        if (setup.clientWorld) {
            ownedHostBlocks = setup.blocks->forkSource();
            authorityBlocks = ownedHostBlocks.get();
        }
        hostWorld = std::make_unique<HostSide>(*setup.hostWorld, *authorityBlocks);
        hostWorld->playerObserved = [this, state = setup.hostWorld](const uint8_t *record) {
            if (!sharedRegions)
                return;
            int64_t id, position[2];
            int32_t level;
            std::memcpy(&id, record + worldgen::creature::ID, sizeof id);
            std::memcpy(position, record + worldgen::creature::POSITION, sizeof position);
            std::memcpy(&level, record + worldgen::creature::LEVEL, sizeof level);
            const auto region = worldgen::regionAt((int32_t)(position[0] >> 16), (int32_t)(position[1] >> 16));
            regionlevels::Levels before, after;
            {
                std::lock_guard hold(occupancyMutex);
                const auto previous = regionOccupancy.regionOf(id);
                if (previous && *previous == regionlevels::Region{region.x, region.y})
                    return;
                before = regionOccupancy.snapshot();
            }
            state->keepHealthShares([&] {
                std::lock_guard hold(occupancyMutex);
                regionOccupancy.update(id, region, level);
                after = regionOccupancy.snapshot();
            });
            if (before != after)
                hostNotify([this] { if (regionLevelsChanged) regionLevelsChanged(); });
        };
        hostWorld->playerLeft = [this, state = setup.hostWorld](int64_t id) {
            regionlevels::Levels before, after;
            {
                std::lock_guard hold(occupancyMutex);
                if (!regionOccupancy.regionOf(id))
                    return;
                before = regionOccupancy.snapshot();
            }
            state->keepHealthShares([&] {
                std::lock_guard hold(occupancyMutex);
                regionOccupancy.leave(id);
                after = regionOccupancy.snapshot();
            });
            if (before != after)
                hostNotify([this] { if (regionLevelsChanged) regionLevelsChanged(); });
        };
        hostGame = std::make_unique<cwnet::HostGame>(*transport, *hostWorld);
        hostGame->link().setWorldHandshake(
            worldIdentityRequired ? std::optional<cwnet::CubeVulkanMessage>{{"cwv.world-generation.ack.v1", worldDescriptor.payload}}
                                  : std::nullopt);
        hostGame->log = [this](const std::string &line) {
            if (log)
                log(line);
        };
    }
    if (hostGame)
        hostGame->extra = [this](cwnet::PeerId from, const cwnet::CubeVulkanMessage &message) {
            if (message.channel == "cwv.world-generation.request.v1" && message.payload.empty())
                sentWorldIdentities.erase(from);
            if (message.channel == "cwv.region-levels.request.v1" && message.payload.empty())
                sentRegionLevels.erase(from); // requests never contain client-authored region levels
        };
    if (setup.clientWorld && setup.blocks) {
        mirrorWorld = std::make_unique<MirrorSide>(*setup.clientWorld, *setup.blocks);
        // Native send/request clocks belong to the process, across client lifetimes.
        static const auto processClocks = std::make_shared<cwnet::GuestLink::Clocks>();
        client = std::make_unique<cwnet::ClientGame>(*transport, *mirrorWorld, [this] {
            auto hold = lockHost();
            if (hostGame)
                hostGame->link().dropAll();
        }, setup.clientTiming.value_or(cwnet::GuestLink::Timing{clockNow, processClocks, {}}));
        client->link().waitForWorldHandshake(worldIdentityRequired);
        client->extra = [this](const cwnet::CubeVulkanMessage &message) {
            if (message.channel == "cwv.world-generation.v1") {
                if (message.payload.size() < 2 || message.payload.size() > 258 || message.payload[0] != 1 || message.payload[1] > 1)
                    return;
                const std::string identity(message.payload.begin() + 2, message.payload.end());
                if ((worldIdentityRequired || message.payload[1]) && identity != worldIdentity) {
                    notice("World generation differs. Select the host's terrain mode, seed and scale before joining.");
                    clearRegionView();
                    client->link().goHome(clockNow());
                    return;
                }
                client->link().acceptWorld();
                client->link().sendExtra({"cwv.world-generation.ack.v1", message.payload});
                return;
            }
            if (message.channel != regionlevels::CHANNEL || !client->link().worldAccepted())
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
        {
            auto host = lockHost();
            std::lock_guard hold(occupancyMutex);
            regionOccupancy.clear();
            sentRegionLevels.clear();
        }
        if (regionLevelsChanged)
            regionLevelsChanged();
    }
}

int Multiplayer::regionLevel(worldgen::Int2 region, int relativeFallback) const {
    if (remoteRegionView) {
        std::lock_guard hold(regionViewMutex);
        return regionView && regionView->enabled ? regionView->basis(region) : relativeFallback;
    }
    if (!sharedRegions)
        return relativeFallback;
    std::lock_guard hold(occupancyMutex);
    return regionOccupancy.basis(region);
}

void Multiplayer::visitLocalRegion(worldgen::Int2 region, int level) {
    if (transport || !sharedRegions)
        return;
    bool changed;
    {
        std::lock_guard hold(occupancyMutex);
        const auto before = regionOccupancy.snapshot();
        regionOccupancy.update(1, region, level);
        changed = before != regionOccupancy.snapshot();
    }
    if (changed && regionLevelsChanged)
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
    regionlevels::Levels levels;
    if (enabled) {
        std::lock_guard hold(occupancyMutex);
        levels = regionOccupancy.snapshot();
    }
    const cwnet::CubeVulkanMessage message{regionlevels::CHANNEL, regionlevels::encode({enabled, levels})};
    const auto &connections = hostGame->link().connections();
    for (auto it = sentRegionLevels.begin(); it != sentRegionLevels.end();)
        if (!connections.count(it->first))
            it = sentRegionLevels.erase(it);
        else
            ++it;
    for (const auto &[peer, connection] : connections) {
        if (!connection.cubeVulkan || !connection.worldAccepted) {
            sentRegionLevels.erase(peer);
            continue;
        }
        const auto previous = sentRegionLevels.find(peer);
        if (previous == sentRegionLevels.end() || previous->second != message.payload)
            if (hostGame->link().sendExtra(peer, message))
                sentRegionLevels[peer] = message.payload;
    }
}

void Multiplayer::exchangeWorldIdentity(uint32_t nowMs) {
    if (hostGame && !hostThreadRunning())
        exchangeHostWorldIdentity(nowMs);
    exchangeClientWorldIdentity(nowMs);
}

void Multiplayer::exchangeHostWorldIdentity(uint32_t nowMs) {
    {
        const auto &connections = hostGame->link().connections();
        for (auto it = sentWorldIdentities.begin(); it != sentWorldIdentities.end();)
            it = connections.count(*it) ? std::next(it) : sentWorldIdentities.erase(it);
        for (auto it = pendingWorldIdentities.begin(); it != pendingWorldIdentities.end();)
            it = connections.count(it->first) ? std::next(it) : pendingWorldIdentities.erase(it);
        std::vector<cwnet::PeerId> expired;
        for (const auto &[peer, connection] : connections) {
            if (connection.cubeVulkan && !sentWorldIdentities.count(peer) && hostGame->link().sendExtra(peer, worldDescriptor))
                sentWorldIdentities.insert(peer);
            if (connection.worldAccepted) {
                pendingWorldIdentities.erase(peer);
                continue;
            }
            const auto [entry, inserted] = pendingWorldIdentities.emplace(peer, nowMs);
            if (nowMs - entry->second > 5000)
                expired.push_back(peer);
        }
        for (auto peer : expired) {
            hostNotify([this] { notice("A guest could not confirm this world's Alpha generation settings."); });
            hostGame->link().kick(peer);
            pendingWorldIdentities.erase(peer);
            sentWorldIdentities.erase(peer);
        }
    }
}

void Multiplayer::exchangeClientWorldIdentity(uint32_t nowMs) {
    if (client && client->link().hostIsCubeVulkan() && requestedWorldEpoch != client->link().connectionEpoch())
        if (client->link().sendExtra({"cwv.world-generation.request.v1", {}}))
            requestedWorldEpoch = client->link().connectionEpoch();
    if (client && client->link().localId() && !client->link().worldAccepted()) {
        if (pendingWorldEpoch != client->link().connectionEpoch()) {
            pendingWorldEpoch = client->link().connectionEpoch();
            pendingWorldSince = nowMs;
        } else if (nowMs - pendingWorldSince > 5000) {
            notice("The host did not confirm compatible Alpha terrain. Steam hosts require Steam generation.");
            clearRegionView();
            client->link().goHome(nowMs);
        }
    }
}

void Multiplayer::beginFrame(uint32_t nowMs) {
    runPosted();
    if (!transport)
        return;
    transport->pump(nowMs);
    // The host idles while its player is a guest elsewhere (netcode-transport.md §2).
    remoteRegionView = guestElsewhere();
    hostIdle = guestElsewhere();
    if (hostThreadRunning())
        return; // the host thread ticks on its own
    if (hostGame && !guestElsewhere()) {
        hostGame->frame(nowMs);
        sendRegionLevels();
    }
}

void Multiplayer::dispatch(uint32_t nowMs) {
    if (!transport) return;
    if (client) {
        client->prepareSourceRequests();
        client->receive(nowMs);
        client->send(nowMs);
        remoteRegionView = guestElsewhere();
        hostIdle = guestElsewhere();
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
    exchangeWorldIdentity(nowMs);
    if (hostThreadRunning())
        return; // the host thread reports joins and leaves
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

void Multiplayer::endFrame() {
    if (client)
        client->collectOwnEvents();
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
    hostIdle = guestElsewhere();
    return true;
}

void Multiplayer::join(uint64_t host) {
    if (client) {
        clearRegionView();
        client->link().connectTo(host, clockNow());
        remoteRegionView = guestElsewhere();
        hostIdle = guestElsewhere();
    }
}

void Multiplayer::leave() {
    if (client) {
        clearRegionView();
        client->link().goHome(clockNow());
        remoteRegionView = false;
        hostIdle = false;
    }
}

bool Multiplayer::guestElsewhere() const {
    return client && client->link().isGuestElsewhere();
}

int64_t Multiplayer::playerId() const {
    return client ? client->link().localId() : 0;
}

size_t Multiplayer::guests() const {
    auto hold = lockHost();
    return hostGame ? const_cast<cwnet::HostGame &>(*hostGame).link().connections().size() : 0;
}

std::vector<Multiplayer::FriendRow> Multiplayer::friends() const {
    std::vector<FriendRow> rows;
    if (!transport)
        return rows;
    auto hold = lockHost();
    auto rowOf = [&](uint64_t id) {
        FriendRow row;
        row.id = id;
        row.name = nameOf(id);
        row.eligible = how != Way::Steam || !checkGame || (steam && steam->friendInThisGame(id));
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
    auto hold = lockHost();
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
    const bool number =
        !friendName.empty() && std::all_of(friendName.begin(), friendName.end(), [](char c) { return std::isdigit((unsigned char)c); });
    for (uint64_t id : steam->friendIds())
        if ((number && std::to_string(id) == friendName) || lower(steam->friendName(id)) == lower(friendName)) {
            {
                auto hold = lockHost();
                hostGame->link().invite(id, clockNow());
            }
            notice("Invited " + steam->friendName(id));
            return true;
        }
    return false;
}

std::vector<cwnet::ChatLine> Multiplayer::takeChat() {
    return client ? std::exchange(client->chat, {}) : std::vector<cwnet::ChatLine>{};
}

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
    case Way::Steam:
        return steam ? steam->friendName(peer) : std::to_string(peer);
    case Way::Address:
        return cwnet::UdpTransport::describe(peer);
    default:
        return std::to_string(peer);
    }
}

// --- The host and AI threads (steam-in-process-host-thread-and-world-step-scheduling-20261111.md) ---

void Multiplayer::post(std::function<void()> action) {
    std::lock_guard hold(postedMutex);
    posted.push_back(std::move(action));
}

void Multiplayer::runPosted() {
    std::vector<std::function<void()>> actions;
    {
        std::lock_guard hold(postedMutex);
        actions.swap(posted);
    }
    for (auto &action : actions)
        action();
}

void Multiplayer::hostNotify(std::function<void()> action) {
    if (hostThreadRunning())
        post(std::move(action));
    else
        action();
}

void Multiplayer::startHostThread() {
    if (!hostGame || !hostWorld || hostThreadRunning())
        return;
    {
        auto hold = lockHost();
        // The host World's results for game-thread state cross as posted actions, as the
        // host's results reach the live World only as queued messages (contract 4.3).
        if (auto deaths = hostWorld->deaths)
            hostWorld->deaths = [this, deaths](const std::vector<SpawnedCreatures::Death> &died) { post([deaths, died] { deaths(died); }); };
        if (auto reset = hostWorld->reset)
            hostWorld->reset = [this, reset] { post(reset); };
        hostWorld->separateAi = true;
        hostIdle = guestElsewhere();
    }
    hostThread = std::jthread([this](std::stop_token stop) { hostLoop(stop); });
    aiThread = std::jthread([this](std::stop_token stop) { aiLoop(stop); });
}

void Multiplayer::joinHostThreads() {
    if (!hostThreadRunning())
        return;
    aiThread.request_stop();
    hostThread.request_stop();
    if (aiThread.joinable())
        aiThread.join();
    hostThread.join();
    hostThread = {};
    aiThread = {};
    if (hostWorld)
        hostWorld->separateAi = false;
}

void Multiplayer::stopHostThread() {
    joinHostThreads();
    runPosted(); // results of the last ticks still reach the game thread
}

// T5 (2.1): a tick every 32 ms measured start to start, no catch-up; while this player is a
// guest elsewhere the host sleeps 33 ms without ticking and keeps its last start.
void Multiplayer::hostLoop(std::stop_token stop) {
    uint32_t previous = clockNow();
    while (!stop.stop_requested()) {
        if (hostIdle) {
            std::this_thread::sleep_for(std::chrono::milliseconds(HOST_IDLE_SLEEP_MS));
            continue;
        }
        const uint32_t start = clockNow();
        hostTick(start, start - previous);
        if (const uint32_t rest = hostSleepAfterTick(clockNow() - start))
            std::this_thread::sleep_for(std::chrono::milliseconds(rest));
        previous = start;
    }
}

void Multiplayer::hostTick(uint32_t nowMs, uint32_t elapsedMs) {
    auto hold = lockHost();
    hostGame->tick(nowMs, elapsedMs); // its world step keeps the 1000 ms cap and 16 ms slices
    sendRegionLevels();
    exchangeHostWorldIdentity(nowMs);
    const size_t now = hostGame->link().connections().size();
    if (now != guestsBefore) {
        guestsBefore = now;
        if (log)
            log("Multiplayer: " + std::to_string(now) + " player(s) on this host");
    }
}

// T7 (3.1): the host World's AI every 40 ms with the real elapsed time, with no pause. It
// shares the host lock so the host tick never reads a creature list the AI is changing.
void Multiplayer::aiLoop(std::stop_token stop) {
    uint32_t previous = clockNow();
    while (!stop.stop_requested()) {
        const uint32_t start = clockNow();
        {
            auto hold = lockHost();
            hostWorld->think(start - previous);
        }
        previous = start;
        std::this_thread::sleep_for(std::chrono::milliseconds(aiSleepAfterTick(clockNow() - start)));
    }
}
