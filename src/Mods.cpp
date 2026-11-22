#include "Mods.h"
#include "World.h"
#include "Assets.h"
#include <algorithm>
#include <cmath>
#include <deque>
#include <windows.h>

namespace {
ModContext *ctx = nullptr;
const Config *currentModConfig = nullptr; // Config of the mod being called.
std::string currentModName;
std::deque<std::string> strings; // Stable storage for config() results.

struct Scope { // Marks which mod is running, for log prefixes and config.
    std::string savedName;
    const Config *savedConfig;
    Scope(const std::string &name, const Config *config)
        : savedName(currentModName), savedConfig(currentModConfig) {
        currentModName = name;
        currentModConfig = config;
    }
    ~Scope() {
        currentModName = savedName;
        currentModConfig = savedConfig;
    }
};

void apiLog(const char *message) {
    if (ctx && ctx->log && message)
        ctx->log("[" + currentModName + "] " + message);
}
const char *apiConfig(const char *section, const char *key, const char *fallback) {
    if (!section || !key)
        return fallback;
    const std::string *v = currentModConfig ? currentModConfig->find(section, key) : nullptr;
    if (!v && ctx && ctx->config)
        v = ctx->config->find(section, key);
    if (!v)
        return fallback;
    strings.push_back(*v);
    if (strings.size() > 4096)
        strings.pop_front();
    return strings.back().c_str();
}
CwvModel apiLoadModel(const char *name) { return ctx && name ? ctx->world->loadModel(name) : -1; }
CwvInstance apiSpawn(CwvModel m, float x, float y, float z, float yaw, float scale) {
    return ctx ? ctx->world->spawn(m, {x, y, z}, yaw, scale) : -1;
}
void apiMove(CwvInstance i, float x, float y, float z, float yaw) {
    if (ctx)
        ctx->world->move(i, {x, y, z}, yaw);
}
void apiRemove(CwvInstance i) {
    if (ctx)
        ctx->world->remove(i);
}
float apiHeight(float x, float y) { return ctx ? ctx->world->terrain().height(x, y) : 0; }
float apiWaterDepth(float x, float y) {
    if (!ctx)
        return 0;
    auto c = ctx->world->terrain().column(x, y);
    return c.water ? float(c.water - c.ground) : 0.f;
}
float apiGetHour() { return ctx ? *ctx->hour : 12; }
void apiSetHour(float h) {
    if (ctx && std::isfinite(h))
        *ctx->hour = std::fmod(std::fmod(h, 24.f) + 24.f, 24.f);
}
void apiFocus(float out[3]) {
    if (ctx && out) {
        out[0] = ctx->focus->x;
        out[1] = ctx->focus->y;
        out[2] = ctx->focus->z;
    }
}
const CwvApi API = {CWV_API_VERSION, apiLog,      apiConfig,     apiLoadModel, apiSpawn,
                    apiMove,         apiRemove,   apiHeight,     apiWaterDepth, apiGetHour,
                    apiSetHour,      apiFocus};

// Version 2: the world of the step in progress (null outside ModHost::worldStep).
const ModWorld *stepWorld = nullptr;
int worldPlayers(double (*out)[3], int max) {
    if (!stepWorld || !stepWorld->players || !out || max <= 0)
        return 0;
    const auto players = stepWorld->players();
    const int n = std::min(max, int(players.size()));
    for (int i = 0; i < n; ++i)
        out[i][0] = players[i].x, out[i][1] = players[i].y, out[i][2] = players[i].z;
    return n;
}
int worldZoneLoaded(int32_t zx, int32_t zy) { return stepWorld && stepWorld->zoneLoaded && stepWorld->zoneLoaded(zx, zy); }
uint8_t worldBlockType(int32_t x, int32_t y, int32_t z, int *loaded) {
    bool ok = false;
    const uint8_t type = stepWorld && stepWorld->blockType ? stepWorld->blockType(x, y, z, ok) : 0;
    if (loaded)
        *loaded = ok;
    return type;
}
float worldGround(int32_t x, int32_t y) { return stepWorld && stepWorld->ground ? stepWorld->ground(x, y) : 0.f; }
int32_t worldBiome(int32_t x, int32_t y) { return stepWorld && stepWorld->biome ? stepWorld->biome(x, y) : 0; }
int64_t worldSpawn(int32_t race, int32_t hostility, int32_t level, const double position[3]) {
    if (!stepWorld || !stepWorld->spawnCreature || !position)
        return 0;
    return stepWorld->spawnCreature(race, hostility, level, glm::dvec3(position[0], position[1], position[2]));
}
int worldAddFlags(int64_t id, uint32_t flags) { return stepWorld && stepWorld->addFlags && stepWorld->addFlags(id, flags); }
} // namespace

void ModHost::worldStep(const ModWorld &world, double milliseconds) {
    CwvWorld w{world.seed, worldPlayers, worldZoneLoaded, worldBlockType, worldGround, worldBiome, worldSpawn, worldAddFlags};
    stepWorld = &world;
    for (auto &m : mods)
        if (m.mod.on_world_step) {
            Scope scope(m.name, &m.config);
            m.mod.on_world_step(&w, milliseconds);
        }
    stepWorld = nullptr;
    for (auto &s : scripts)
        s->worldStep(world, milliseconds);
}

bool ModHost::stepsWorld() const {
    return std::any_of(mods.begin(), mods.end(), [](const Loaded &m) { return m.mod.on_world_step != nullptr; }) ||
           std::any_of(scripts.begin(), scripts.end(), [](const auto &s) { return s->stepsWorld(); });
}

std::vector<std::string> ModHost::worldChangingMods() const {
    std::vector<std::string> list;
    for (auto &m : mods)
        if (m.mod.changes_world)
            list.push_back(m.name + " " + (m.mod.version ? m.mod.version : ""));
    for (auto &s : scripts)
        if (s->changesWorld())
            list.push_back(s->identity());
    return list;
}

void ModHost::load(const std::filesystem::path &dir, ModContext &context) {
    ctx = &context;
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec))
        return;
    std::vector<std::filesystem::path> dlls;
    for (auto &e : std::filesystem::directory_iterator(dir, ec))
        if (e.is_regular_file() && e.path().extension() == ".dll")
            dlls.push_back(e.path());
    std::sort(dlls.begin(), dlls.end());
    for (auto &path : dlls)
        loadOne(path, context);
    // Then Lua script mods: folders holding main.lua.
    std::vector<std::filesystem::path> folders;
    for (auto &e : std::filesystem::directory_iterator(dir, ec))
        if (e.is_directory() && std::filesystem::is_regular_file(e.path() / "main.lua", ec))
            folders.push_back(e.path());
    std::sort(folders.begin(), folders.end());
    for (auto &folder : folders)
        loadScript(folder, context);
}

bool ModHost::loadScript(const std::filesystem::path &folder, ModContext &context) {
    LuaScript::Host host;
    host.log = context.log;
    host.config = context.config;
    host.hour = context.hour;
    host.focus = context.focus ? &context.focus->x : nullptr;
    auto script = std::make_unique<LuaScript>();
    if (!script->load(folder, host)) {
        if (context.log)
            context.log("Lua mod " + utf8(folder.filename()) + ": not loaded");
        return false;
    }
    if (context.log)
        context.log("Lua mod loaded: " + script->name());
    scripts.push_back(std::move(script));
    return true;
}

bool ModHost::loadOne(const std::filesystem::path &path, ModContext &context) {
    ctx = &context;
    std::string stem = utf8(path.stem());
    HMODULE module = LoadLibraryW(path.wstring().c_str());
    if (!module) {
        context.log("Mod " + stem + ": cannot load DLL (error " + std::to_string(GetLastError()) + ")");
        return false;
    }
    auto entry = (CwvModLoad)GetProcAddress(module, "cwv_mod_load");
    if (!entry) {
        // Probably a CubeModLoader mod for cubeworld.exe: those patch the
        // original executable and cannot run here.
        context.log("Mod " + stem + ": no cwv_mod_load export (not a CubeVulkan mod), skipped");
        FreeLibrary(module);
        return false;
    }
    Loaded l;
    l.module = module;
    l.name = stem;
    l.config.load(path.parent_path() / (stem + ".ini"));
    mods.push_back(std::move(l));
    auto &m = mods.back();
    Scope scope(m.name, &m.config);
    if (!entry(&API, &m.mod)) {
        context.log("Mod " + stem + ": declined to load");
        FreeLibrary(module);
        mods.pop_back();
        return false;
    }
    if (m.mod.name)
        m.name = m.mod.name;
    context.log("Mod loaded: " + m.name);
    return true;
}

void ModHost::update(double seconds) {
    for (auto &m : mods)
        if (m.mod.on_update) {
            Scope scope(m.name, &m.config);
            m.mod.on_update(seconds);
        }
    for (auto &s : scripts)
        s->update(seconds);
}

bool ModHost::key(int key) {
    for (auto &m : mods)
        if (m.mod.on_key) {
            Scope scope(m.name, &m.config);
            if (m.mod.on_key(key))
                return true;
        }
    for (auto &s : scripts)
        if (s->key(key))
            return true;
    return false;
}

void ModHost::shutdown() {
    for (auto it = scripts.rbegin(); it != scripts.rend(); ++it)
        (*it)->shutdown();
    scripts.clear();
    for (auto it = mods.rbegin(); it != mods.rend(); ++it) {
        if (it->mod.on_shutdown) {
            Scope scope(it->name, &it->config);
            it->mod.on_shutdown();
        }
        FreeLibrary((HMODULE)it->module);
    }
    mods.clear();
}

std::vector<std::string> ModHost::names() const {
    std::vector<std::string> n;
    for (auto &m : mods)
        n.push_back(m.name);
    for (auto &s : scripts)
        n.push_back(s->name());
    return n;
}
