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
} // namespace

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
    for (auto &path : dlls) {
        std::string stem = utf8(path.stem());
        HMODULE module = LoadLibraryW(path.wstring().c_str());
        if (!module) {
            context.log("Mod " + stem + ": cannot load DLL (error " + std::to_string(GetLastError()) + ")");
            continue;
        }
        auto entry = (CwvModLoad)GetProcAddress(module, "cwv_mod_load");
        if (!entry) {
            // Probably a CubeModLoader mod for cubeworld.exe: those patch the
            // original executable and cannot run here.
            context.log("Mod " + stem + ": no cwv_mod_load export (not a CubeVulkan mod), skipped");
            FreeLibrary(module);
            continue;
        }
        Loaded l;
        l.module = module;
        l.name = stem;
        l.config.load(dir / (stem + ".ini"));
        mods.push_back(std::move(l));
        auto &m = mods.back();
        Scope scope(m.name, &m.config);
        if (!entry(&API, &m.mod)) {
            context.log("Mod " + stem + ": declined to load");
            FreeLibrary(module);
            mods.pop_back();
            continue;
        }
        if (m.mod.name)
            m.name = m.mod.name;
        context.log("Mod loaded: " + m.name);
    }
}

void ModHost::update(double seconds) {
    for (auto &m : mods)
        if (m.mod.on_update) {
            Scope scope(m.name, &m.config);
            m.mod.on_update(seconds);
        }
}

bool ModHost::key(int key) {
    for (auto &m : mods)
        if (m.mod.on_key) {
            Scope scope(m.name, &m.config);
            if (m.mod.on_key(key))
                return true;
        }
    return false;
}

void ModHost::shutdown() {
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
    return n;
}
