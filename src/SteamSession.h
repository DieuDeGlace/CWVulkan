#pragma once
// Uses the original install's Steam runtime to check the active account's
// Cube World license. No Steam binaries or account credentials are bundled.
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class SteamSession {
  public:
    static constexpr unsigned APP_ID = 1128000;
    SteamSession();
    ~SteamSession();
    SteamSession(const SteamSession &) = delete;
    SteamSession &operator=(const SteamSession &) = delete;
    bool start(const std::filesystem::path &gameDir, std::string &error);
    bool check(std::string &error) const;
    void pump();
    // Steam Cloud (the game's own cloud space, ISteamRemoteStorage): files sync to
    // the account and follow it to other PCs. False when the cloud is off or full.
    bool cloudReady() const;
    bool cloudWrite(const std::string &name, const std::string &bytes) const;
    bool cloudRead(const std::string &name, std::string &bytes) const;
    enum class CloudRead { Success, Missing, Unavailable, Failed };
    CloudRead cloudReadState(const std::string &name, std::string &bytes) const;
    bool cloudDelete(const std::string &name) const;
    // Free and total bytes in the game's cloud space (false if unknown).
    bool cloudQuota(unsigned long long &total, unsigned long long &available) const;

    // Steam networking (ISteamNetworking005, ISteamFriends017) as the game uses it
    // (docs/specs/netcode-transport.md §1), for the game-compatible protocol
    // (src/net/SteamTransport.*): players reach each other by Steam account, through Steam's
    // relay when needed, with no addresses or ports; sessions are accepted only from the
    // account's immediate friends.
    bool networkReady() const;
    uint64_t steamId() const;
    std::string personaName() const;
    static constexpr int SEND_RELIABLE = 2; // k_EP2PSendReliable
    bool p2pSendOn(uint64_t to, const void *data, uint32_t size, int sendType, int channel) const;
    bool p2pReadOn(std::vector<uint8_t> &data, uint64_t &from, int channel) const;
    // Answers the session requests of immediate friends (P2PSessionRequest_t, 0x9f6c0).
    void acceptFriendSessions(bool on);
    // The account's immediate friends (GetFriendCount / GetFriendByIndex with flag 4).
    std::vector<uint64_t> friendIds() const;
    std::string friendName(uint64_t id) const;

  private:
    struct State;
    std::unique_ptr<State> state;
};
