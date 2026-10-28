#include "SteamSession.h"
#include <windows.h>
#include <algorithm>

namespace {
struct EnvironmentValue {
    const wchar_t *name;
    std::wstring previous;
    bool existed = false;
    explicit EnvironmentValue(const wchar_t *key) : name(key) {
        DWORD size = GetEnvironmentVariableW(name, nullptr, 0);
        existed = size != 0 || GetLastError() != ERROR_ENVVAR_NOT_FOUND;
        if (size) {
            previous.resize(size);
            GetEnvironmentVariableW(name, previous.data(), size);
            previous.resize(size - 1);
        }
    }
    void set(const wchar_t *value) { SetEnvironmentVariableW(name, value); }
    ~EnvironmentValue() { SetEnvironmentVariableW(name, existed ? previous.c_str() : nullptr); }
};
// The Steam SDK's CCallbackBase, declared exactly as in steam_api_common.h so its
// method table matches what the Steam runtime calls (no virtual destructor).
class CallbackBase {
  public:
    virtual void Run(void *param) = 0;
    virtual void Run(void *param, bool ioFailure, uint64_t call) = 0;
    virtual int GetCallbackSizeBytes() = 0;
    uint8_t flags = 0;
    int callback = 0;
};
constexpr int P2P_SESSION_REQUEST = 1202; // k_iSteamNetworkingCallbacks + 2
} // namespace

struct SteamSession::State {
    // App context identifies the game; it does not grant a license. The API
    // checks the active Steam account below. These changes are process-local.
    EnvironmentValue appId{L"SteamAppId"}, gameId{L"SteamGameId"};
    HMODULE module = nullptr;
    bool initialized = false;
    bool (*init)() = nullptr;
    void (*shutdown)() = nullptr;
    void (*callbacks)() = nullptr;
    bool (*running)() = nullptr;
    int (*userHandle)() = nullptr;
    void *(*interfaceFor)(int, const char *) = nullptr;
    bool (*loggedOn)(void *) = nullptr;
    bool (*subscribed)(void *, unsigned) = nullptr;
    unsigned (*applicationId)(void *) = nullptr;
    void *user = nullptr, *apps = nullptr, *utils = nullptr;
    // Steam Cloud (optional: CubeVulkan still runs without it).
    bool (*fileWrite)(void *, const char *, const void *, int) = nullptr;
    int (*fileRead)(void *, const char *, void *, int) = nullptr;
    bool (*fileExists)(void *, const char *) = nullptr;
    int (*fileSize)(void *, const char *) = nullptr;
    bool (*cloudAccount)(void *) = nullptr;
    bool (*cloudApp)(void *) = nullptr;
    bool (*quota)(void *, unsigned long long *, unsigned long long *) = nullptr;
    bool (*fileDelete)(void *, const char *) = nullptr;
    void *storage = nullptr;
    // Steam networking and friends (optional: single player runs without them).
    bool (*p2pSend)(void *, uint64_t, const void *, uint32_t, int, int) = nullptr;
    bool (*p2pAvailable)(void *, uint32_t *, int) = nullptr;
    bool (*p2pRead)(void *, void *, uint32_t, uint32_t *, uint64_t *, int) = nullptr;
    bool (*p2pAccept)(void *, uint64_t) = nullptr;
    uint64_t (*mySteamId)(void *) = nullptr;
    const char *(*myName)(void *) = nullptr;
    int (*friendCount)(void *, int) = nullptr;
    uint64_t (*friendAt)(void *, int, int) = nullptr;
    const char *(*friendName)(void *, uint64_t) = nullptr;
    void (*registerCallback)(CallbackBase *, int) = nullptr;
    void (*unregisterCallback)(CallbackBase *) = nullptr;
    void *networking = nullptr, *friendsApi = nullptr;
    // Accepts the Steam sessions of the account's immediate friends (as the game does, 0x9f6c0).
    struct SessionRequests : CallbackBase {
        State *state = nullptr;
        void Run(void *param) override {
            if (!state || !state->networking || !param)
                return;
            const uint64_t remote = *(const uint64_t *)param;
            if (state->isFriend(remote))
                state->p2pAccept(state->networking, remote);
        }
        void Run(void *param, bool, uint64_t) override { Run(param); }
        int GetCallbackSizeBytes() override { return sizeof(uint64_t); }
    } sessionRequests;
    bool accepting = false;
    bool isFriend(uint64_t id) const {
        constexpr int IMMEDIATE = 4; // k_EFriendFlagImmediate
        if (!friendsApi || id == 0)
            return false;
        for (int i = 0, n = friendCount(friendsApi, IMMEDIATE); i < n; i++)
            if (friendAt(friendsApi, i, IMMEDIATE) == id)
                return true;
        return false;
    }
    template<class T> bool bind(T &fn, const char *name) {
        fn = reinterpret_cast<T>(GetProcAddress(module, name));
        return fn != nullptr;
    }
    ~State() {
        if (accepting)
            unregisterCallback(&sessionRequests);
        if (initialized)
            shutdown();
        if (module)
            FreeLibrary(module);
    }
};

SteamSession::SteamSession() = default;
SteamSession::~SteamSession() = default;

bool SteamSession::start(const std::filesystem::path &gameDir, std::string &error) {
    state.reset();
    error.clear();
    std::error_code ec;
    auto runtime = std::filesystem::absolute(gameDir / "steam_api64.dll", ec);
    if (ec || !std::filesystem::is_regular_file(runtime, ec)) {
        error = "steam_api64.dll is missing from the Steam Cube World installation. Verify the game files in Steam.";
        return false;
    }
    auto s = std::make_unique<State>();
    s->appId.set(L"1128000");
    s->gameId.set(L"1128000");
    s->module = LoadLibraryExW(runtime.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!s->module) {
        error = "Cannot load the Steam runtime from the Cube World installation (Windows error " +
                std::to_string(GetLastError()) + "). Verify the game files in Steam.";
        return false;
    }
    if (!s->bind(s->init, "SteamAPI_Init") || !s->bind(s->shutdown, "SteamAPI_Shutdown") ||
        !s->bind(s->callbacks, "SteamAPI_RunCallbacks") || !s->bind(s->running, "SteamAPI_IsSteamRunning") ||
        !s->bind(s->userHandle, "SteamAPI_GetHSteamUser") ||
        !s->bind(s->interfaceFor, "SteamInternal_FindOrCreateUserInterface") ||
        !s->bind(s->loggedOn, "SteamAPI_ISteamUser_BLoggedOn") ||
        !s->bind(s->subscribed, "SteamAPI_ISteamApps_BIsSubscribedApp") ||
        !s->bind(s->applicationId, "SteamAPI_ISteamUtils_GetAppID")) {
        error = "The installed Steam runtime does not provide the required license-check interfaces.";
        return false;
    }
    if (!s->running() || !s->init()) {
        error = "Start Steam and sign in to an account that owns Cube World, then launch CubeVulkan again.";
        return false;
    }
    s->initialized = true;
    int handle = s->userHandle();
    s->user = s->interfaceFor(handle, "SteamUser020");
    s->apps = s->interfaceFor(handle, "STEAMAPPS_INTERFACE_VERSION008");
    s->utils = s->interfaceFor(handle, "SteamUtils009");
    if (s->bind(s->fileWrite, "SteamAPI_ISteamRemoteStorage_FileWrite") &&
        s->bind(s->fileRead, "SteamAPI_ISteamRemoteStorage_FileRead") &&
        s->bind(s->fileExists, "SteamAPI_ISteamRemoteStorage_FileExists") &&
        s->bind(s->fileSize, "SteamAPI_ISteamRemoteStorage_GetFileSize") &&
        s->bind(s->cloudAccount, "SteamAPI_ISteamRemoteStorage_IsCloudEnabledForAccount") &&
        s->bind(s->cloudApp, "SteamAPI_ISteamRemoteStorage_IsCloudEnabledForApp") &&
        s->bind(s->quota, "SteamAPI_ISteamRemoteStorage_GetQuota") &&
        s->bind(s->fileDelete, "SteamAPI_ISteamRemoteStorage_FileDelete"))
        s->storage = s->interfaceFor(handle, "STEAMREMOTESTORAGE_INTERFACE_VERSION014");
    if (s->bind(s->p2pSend, "SteamAPI_ISteamNetworking_SendP2PPacket") &&
        s->bind(s->p2pAvailable, "SteamAPI_ISteamNetworking_IsP2PPacketAvailable") &&
        s->bind(s->p2pRead, "SteamAPI_ISteamNetworking_ReadP2PPacket") &&
        s->bind(s->p2pAccept, "SteamAPI_ISteamNetworking_AcceptP2PSessionWithUser") &&
        s->bind(s->mySteamId, "SteamAPI_ISteamUser_GetSteamID") &&
        s->bind(s->myName, "SteamAPI_ISteamFriends_GetPersonaName") &&
        s->bind(s->friendCount, "SteamAPI_ISteamFriends_GetFriendCount") &&
        s->bind(s->friendAt, "SteamAPI_ISteamFriends_GetFriendByIndex") &&
        s->bind(s->friendName, "SteamAPI_ISteamFriends_GetFriendPersonaName") &&
        s->bind(s->registerCallback, "SteamAPI_RegisterCallback") &&
        s->bind(s->unregisterCallback, "SteamAPI_UnregisterCallback")) {
        s->networking = s->interfaceFor(handle, "SteamNetworking005");
        s->friendsApi = s->interfaceFor(handle, "SteamFriends017");
    }
    if (!s->user || !s->apps || !s->utils) {
        error = "Steam could not provide the account and application interfaces for Cube World.";
        return false;
    }
    state = std::move(s);
    if (!check(error)) {
        state.reset();
        return false;
    }
    return true;
}

bool SteamSession::check(std::string &error) const {
    error.clear();
    if (!state || !state->initialized || !state->running())
        error = "Steam is not running. Start Steam and sign in to the account that owns Cube World.";
    else if (state->applicationId(state->utils) != APP_ID)
        error = "Steam initialized the wrong application. CubeVulkan requires the Steam Cube World license.";
    else if (!state->loggedOn(state->user))
        error = "The Steam account is offline. Reconnect Steam before playing CubeVulkan.";
    else if (!state->subscribed(state->apps, APP_ID))
        error = "The signed-in Steam account does not have a Cube World license.";
    return error.empty();
}

void SteamSession::pump() {
    if (state && state->initialized)
        state->callbacks();
}

bool SteamSession::cloudReady() const {
    return state && state->storage && state->cloudAccount(state->storage) && state->cloudApp(state->storage);
}

bool SteamSession::cloudWrite(const std::string &name, const std::string &bytes) const {
    if (!cloudReady())
        return false;
    // Steam refuses a write now and then while it is still syncing the last one;
    // a short retry after letting it run its callbacks gets through.
    for (int attempt = 0; attempt < 4; attempt++) {
        if (state->fileWrite(state->storage, name.c_str(), bytes.data(), (int)bytes.size()))
            return true;
        state->callbacks();
        Sleep(60);
    }
    return false;
}

bool SteamSession::cloudRead(const std::string &name, std::string &bytes) const {
    return cloudReadState(name, bytes) == CloudRead::Success;
}

SteamSession::CloudRead SteamSession::cloudReadState(const std::string &name, std::string &bytes) const {
    bytes.clear();
    if (!cloudReady()) return CloudRead::Unavailable;
    if (!state->fileExists(state->storage, name.c_str())) return CloudRead::Missing;
    int size = state->fileSize(state->storage, name.c_str());
    if (size <= 0 || size > 64 * 1024 * 1024) return CloudRead::Failed;
    bytes.resize(size);
    int got = state->fileRead(state->storage, name.c_str(), bytes.data(), size);
    if (got != size) { bytes.clear(); return CloudRead::Failed; }
    return CloudRead::Success;
}

bool SteamSession::cloudQuota(unsigned long long &total, unsigned long long &available) const {
    return cloudReady() && state->quota && state->quota(state->storage, &total, &available);
}

bool SteamSession::cloudDelete(const std::string &name) const {
    return cloudReady() && state->fileExists(state->storage, name.c_str()) && state->fileDelete(state->storage, name.c_str());
}

bool SteamSession::networkReady() const { return state && state->networking && state->friendsApi; }

uint64_t SteamSession::steamId() const { return state && state->mySteamId ? state->mySteamId(state->user) : 0; }

std::string SteamSession::personaName() const {
    const char *name = networkReady() ? state->myName(state->friendsApi) : nullptr;
    return name ? name : "";
}

bool SteamSession::p2pSendOn(uint64_t to, const void *data, uint32_t size, int sendType, int channel) const {
    return networkReady() && state->p2pSend(state->networking, to, data, size, sendType, channel);
}

bool SteamSession::p2pReadOn(std::vector<uint8_t> &data, uint64_t &from, int channel) const {
    uint32_t size = 0;
    if (!networkReady() || !state->p2pAvailable(state->networking, &size, channel))
        return false;
    data.resize(size ? size : 1);
    uint32_t got = 0;
    if (!state->p2pRead(state->networking, data.data(), (uint32_t)data.size(), &got, &from, channel))
        return false;
    data.resize(std::min<size_t>(got, data.size()));
    return true;
}

void SteamSession::acceptFriendSessions(bool on) {
    if (!networkReady() || on == state->accepting)
        return;
    state->sessionRequests.state = state.get();
    if (on)
        state->registerCallback(&state->sessionRequests, P2P_SESSION_REQUEST);
    else
        state->unregisterCallback(&state->sessionRequests);
    state->accepting = on;
}

std::vector<uint64_t> SteamSession::friendIds() const {
    std::vector<uint64_t> ids;
    if (!networkReady())
        return ids;
    constexpr int IMMEDIATE = 4;
    for (int i = 0, n = state->friendCount(state->friendsApi, IMMEDIATE); i < n; i++)
        ids.push_back(state->friendAt(state->friendsApi, i, IMMEDIATE));
    return ids;
}

std::string SteamSession::friendName(uint64_t id) const {
    const char *name = networkReady() ? state->friendName(state->friendsApi, id) : nullptr;
    return name ? name : "";
}
