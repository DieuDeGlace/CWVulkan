#pragma once
// Multiplayer as Steam Cube World plays it (docs/specs/netcode-*.md, docs/multiplayer.md).
//
// Every running game has a host (the authority over its world) and a client (the world the
// player sees, a mirror of a host). Single player is the client connected to its own host by
// loopback; hosting is the same with friends connected too; being a guest is the client connected
// to a friend's host, while one's own host idles. A dedicated server (CubeVulkan's own extra) is a
// host with no client at all.
//
// The session owns the transport and the two protocol ends, and drives them in the game's order
// each frame:
//
//   session.beginFrame(now);    // transport ingestion and the separate host tick
//   ... the client World steps and smooths its drawn positions ...
//   session.consume();          // category lists pending from earlier dispatches
//   ... local player input/control ...
//   session.dispatch(now);      // source-request snapshot, incoming batch and send
//   ... camera/interface/renderer ...
//   session.endFrame();         // collect current local events for a later send
//
// How players reach each other:
//   * Steam (the game's way): immediate Steam friends only; the host invites, the guest joins from
//     the multiplayer menu (the J key). Talks to players of the original game.
//   * By address over UDP (CubeVulkan only): --host [port], --join address[:port], --server [port].
//   * Local: no network at all, the loopback only (single player without Steam networking).
#include "NetWorlds.h"
#include "net/ClientGame.h"
#include "net/HostGame.h"
#include "net/Transport.h"
#include "RegionLevels.h"

#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class SteamSession;
class SpawnedCreatures;
class ExtractedWorld;

class Multiplayer {
  public:
    enum class Way { Local, Steam, Address };
    static constexpr uint16_t DEFAULT_PORT = 27016;

    struct Setup {
        Way way = Way::Local;
        uint16_t port = 0; // Address: the UDP port to open (0: any)
        float loss = 0;    // Address: the fraction of datagrams dropped on purpose (testing)
        SteamSession *steam = nullptr;
        // The worlds: the host world (null: no host, which no game is; kept for tests) and the
        // client world (null: a dedicated server).
        SpawnedCreatures *hostWorld = nullptr;
        SpawnedCreatures *clientWorld = nullptr;
        // Client terrain/source; a combined session creates an independent host reader.
        // A dedicated host uses this reader directly.
        ExtractedWorld *blocks = nullptr;
        // Optional deterministic clock/storage injection; production uses fresh process clocks.
        std::optional<cwnet::GuestLink::Timing> clientTiming;
    };

    Multiplayer();
    ~Multiplayer();
    Multiplayer(const Multiplayer &) = delete;
    Multiplayer &operator=(const Multiplayer &) = delete;

    // Opens the transport and connects the client to its own host. False with a reason.
    bool start(const Setup &setup, std::string &error);
    bool started() const { return transport != nullptr; }
    Way way() const { return how; }
    bool dedicated() const { return !client; }

    // The milliseconds the frame calls take (any steady clock; this is the one the menu's buttons
    // and the command line's joins use).
    static uint32_t clockNow();

    // Steam's host and AI threads (steam-in-process-host-thread-and-world-step-scheduling-20261111.md).
    // Once started, the host ticks every 32 ms on its own thread (idling in 33 ms sleeps while
    // this player is a guest elsewhere) and the host World's AI runs on a 40 ms thread; beginFrame
    // then only pumps the transport and runs posted results. Start them after the host World is
    // set up. Every other access to the host World, its terrain or the host link must hold
    // lockHost(), the analogue of the host's connection lock H+0x5F8.
    static constexpr uint32_t HOST_PERIOD_MS = 32, HOST_IDLE_SLEEP_MS = 33, AI_PERIOD_MS = 40;
    static uint32_t hostSleepAfterTick(uint32_t usedMs) { return usedMs < HOST_PERIOD_MS ? HOST_PERIOD_MS - usedMs : 0; }
    static uint32_t aiSleepAfterTick(uint32_t usedMs) { return usedMs < AI_PERIOD_MS ? AI_PERIOD_MS - usedMs : 1; }
    void startHostThread();
    void stopHostThread();
    bool hostThreadRunning() const { return hostThread.joinable(); }
    std::unique_lock<std::recursive_mutex> lockHost() const { return std::unique_lock(hostMutex); }
    // Host results for game-thread state, run by the next beginFrame in posting order.
    void post(std::function<void()> action);
    void beginFrame(uint32_t nowMs);
    void consume();
    void dispatch(uint32_t nowMs);
    void endFrame();
    // Pyro's world-shared baseline lasts only while a region has occupants.
    // The host decides; negotiated CubeVulkan extras mirror it to guests.
    void setRegionScaling(bool enabled);
    int regionLevel(worldgen::Int2 region, int relativeFallback) const;
    void visitLocalRegion(worldgen::Int2 region, int level); // no transport/recorded-world fallback
    std::function<void()> regionLevelsChanged;

    // The pieces, for the game frame's hooks and for mods.
    HostSide *hostSide() { return hostWorld.get(); }
    ExtractedWorld *hostBlocks() { return hostWorld ? authorityBlocks : nullptr; }
    MirrorSide *mirrorSide() { return mirrorWorld.get(); }
    cwnet::HostGame *host() { return hostGame.get(); }
    cwnet::ClientGame *clientGame() { return client.get(); }
    cwnet::Transport *net() { return transport.get(); }

    // --- Joining and leaving ---
    // Joins the host at a UDP address (Address only). False with a reason.
    bool joinAddress(const std::string &address, uint16_t port, std::string &error);
    // Joins a Steam friend's game (the menu's "join"; the command line's --join-steam).
    void join(uint64_t host);
    // Back to one's own world (the menu's "leave").
    void leave();
    bool guestElsewhere() const;
    int64_t playerId() const;
    size_t guests() const; // players connected to this machine's host, its own player included

    // --- The multiplayer menu (netcode-transport.md §3) ---
    struct FriendRow {
        uint64_t id = 0;
        std::string name;
        bool hostOfMine = false; // this player is that friend's guest: " (host)"
        bool joined = false;     // connected to this player's host: " (joined)"
        bool invited = false;    // invited by this player's host: " (invited)"
        bool invitation = false; // that friend's invitation to this player is live
        bool eligible = true;
    };
    // The immediate Steam friends, as the menu lists them when it opens (Steam only).
    std::vector<FriendRow> friends() const;
    bool toggleGameCheck() { return checkGame = !checkGame; }
    bool gameCheckEnabled() const { return checkGame; }
    // The menu's two buttons on a friend's row.
    void hostButton(uint64_t friendId);  // invite, or uninvite / kick
    void guestButton(uint64_t friendId); // join, or leave
    // Invites a friend by Steam name or SteamID (the command line's --invite).
    bool invite(const std::string &friendName);

    // Notices for the player ("You were invited by ...", joins and leaves), taken by the game.
    struct Notice {
        std::string text;
        bool invitation = false; // "You were invited by": the game shows it in orange (1, 0.5, 0.2)
    };
    std::vector<Notice> takeNotices() { return std::exchange(notices, {}); }
    std::function<void(const std::string &)> log;
    // Chat (packet 14): lines received, with the speaker's creature id; one line sent per tick.
    std::vector<cwnet::ChatLine> takeChat();
    void say(const std::u16string &line);

  private:
    mutable std::recursive_mutex hostMutex;
    std::mutex postedMutex;
    std::vector<std::function<void()>> posted;
    std::atomic<bool> hostIdle{false};
    std::jthread hostThread, aiThread;
    mutable std::recursive_mutex occupancyMutex; // region occupancy, shared by both threads
    void hostLoop(std::stop_token stop);
    void aiLoop(std::stop_token stop);
    void hostTick(uint32_t nowMs, uint32_t elapsedMs);
    void runPosted();
    void joinHostThreads();
    void resetOwnHost();
    void hostNotify(std::function<void()> action); // posted while threaded, else run now
    void exchangeHostWorldIdentity(uint32_t nowMs);
    void exchangeClientWorldIdentity(uint32_t nowMs);
    bool checkGame = true; // native MultiplayerWidget constructor; /check toggles it
    Way how = Way::Local;
    SteamSession *steam = nullptr;
    std::unique_ptr<cwnet::LoopbackNetwork> loopback;
    std::unique_ptr<cwnet::Transport> transport;
    std::unique_ptr<ExtractedWorld> ownedHostBlocks; // outlives the host adapter and protocol end
    ExtractedWorld *authorityBlocks = nullptr;
    std::unique_ptr<HostSide> hostWorld;
    std::unique_ptr<MirrorSide> mirrorWorld;
    std::unique_ptr<cwnet::HostGame> hostGame;
    std::unique_ptr<cwnet::ClientGame> client;
    std::vector<Notice> notices;
    size_t guestsBefore = 0;
    regionlevels::Occupancy regionOccupancy;
    std::atomic<bool> sharedRegions{false}, remoteRegionView{false};
    mutable std::mutex regionViewMutex;
    std::optional<regionlevels::Snapshot> regionView;
    cwnet::PeerId requestedRegionHost = 0;
    std::map<cwnet::PeerId, cwnet::Bytes> sentRegionLevels;

    void clearRegionView();
    void sendRegionLevels();
    void exchangeWorldIdentity(uint32_t nowMs);
    std::string worldIdentity;
    cwnet::CubeVulkanMessage worldDescriptor;
    bool worldIdentityRequired = false;
    std::set<cwnet::PeerId> sentWorldIdentities;
    std::map<cwnet::PeerId, uint32_t> pendingWorldIdentities;
    uint64_t requestedWorldEpoch = uint64_t(-1), pendingWorldEpoch = uint64_t(-1);
    uint32_t pendingWorldSince = 0;

    void notice(const std::string &line, bool invitation = false);
    std::string nameOf(uint64_t peer) const;
};
