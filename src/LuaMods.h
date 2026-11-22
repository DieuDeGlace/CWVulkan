#pragma once
// Lua script mods (docs/modding-api.md): a folder mods/<Name>/ holding main.lua. The script sees the
// same services as code mods (ModApi.h), as a `cwv` table and, during the world step, a `world` table.
// Sandboxed: text chunks only, no io/os/package/debug or file loading, an instruction budget per call;
// an error disables that script and is logged.
#include "Config.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

struct lua_State;
struct ModWorld;

class LuaScript {
  public:
    struct Host {
        std::function<void(const std::string &)> log;
        const Config *config = nullptr; // CubeVulkan.ini
        float *hour = nullptr;
        const float *focus = nullptr; // x, y, z
    };
    LuaScript() = default;
    LuaScript(const LuaScript &) = delete;
    LuaScript &operator=(const LuaScript &) = delete;
    ~LuaScript();

    // Loads mods/<Name>/main.lua (and mods/<Name>.ini); false when the script fails or declines.
    bool load(const std::filesystem::path &folder, const Host &host);
    void update(double seconds);
    bool key(int key);
    void worldStep(const ModWorld &world, double milliseconds);
    void shutdown();

    const std::string &name() const { return modName; }
    bool stepsWorld() const { return hasWorldStep && !failed; }
    bool changesWorld() const { return changes; }
    // "name version hash": what multiplayer peers compare for a world-changing script.
    std::string identity() const;

    // For the script's `cwv` functions.
    void hostLog(const std::string &line);
    const std::string *findConfig(const std::string &section, const std::string &key) const;
    float *hostHour() const { return host.hour; }
    const float *hostFocus() const { return host.focus; }

  private:
    bool call(const char *function, int args, int results);
    void fail(const std::string &why);

    lua_State *L = nullptr;
    Host host;
    Config config;
    std::string modName, modVersion;
    uint64_t textHash = 0;
    bool changes = false, hasWorldStep = false, failed = false;
};
