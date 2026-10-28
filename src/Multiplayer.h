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
//   session.beginFrame(now);    // the host's tick (about every 32 ms, unless the player is a guest
//                               // elsewhere), then what the client's host sent
//   session.consume();          // the world updates, before the client world's step
//   ... the client world steps, the player plays ...
//   session.endFrame(now);      // what the player caused, and the client's packets
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

#include <functional>
#include <memory>
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
        uint16_t port = 0;       // Address: the UDP port to open (0: any)
        float loss = 0;          // Address: the fraction of datagrams dropped on purpose (testing)
        SteamSession *steam = nullptr;
        // The worlds: the host world (null: no host, which no game is; kept for tests) and the
        // client world (null: a dedicated server).
        SpawnedCreatures *hostWorld = nullptr;
        SpawnedCreatures *clientWorld = nullptr;
        ExtractedWorld *blocks = nullptr;
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

    void beginFrame(uint32_t nowMs);
    void consume();
    void endFrame(uint32_t nowMs);

    // The pieces, for the game frame's hooks and for mods.
    HostSide *hostSide() { return hostWorld.get(); }
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
        bool hostOfMine = false;  // this player is that friend's guest: " (host)"
        bool joined = false;      // connected to this player's host: " (joined)"
        bool invited = false;     // invited by this player's host: " (invited)"
        bool invitation = false;  // that friend's invitation to this player is live
    };
    // The immediate Steam friends, as the menu lists them when it opens (Steam only).
    std::vector<FriendRow> friends() const;
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
    Way how = Way::Local;
    SteamSession *steam = nullptr;
    std::unique_ptr<cwnet::LoopbackNetwork> loopback;
    std::unique_ptr<cwnet::Transport> transport;
    std::unique_ptr<HostSide> hostWorld;
    std::unique_ptr<MirrorSide> mirrorWorld;
    std::unique_ptr<cwnet::HostGame> hostGame;
    std::unique_ptr<cwnet::ClientGame> client;
    std::vector<Notice> notices;
    size_t guestsBefore = 0;

    void notice(const std::string &line, bool invitation = false);
    std::string nameOf(uint64_t peer) const;
};
