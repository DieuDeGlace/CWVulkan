#include "LuaMods.h"

#include "Mods.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace {
// The instruction budget of one call into a script (a runaway loop errors instead of hanging the game).
constexpr int HOOK_EVERY = 100000;
constexpr int BUDGET_HOOKS = 2000; // 200 million instructions

LuaScript *self(lua_State *L) {
    lua_getfield(L, LUA_REGISTRYINDEX, "cwv.script");
    auto *s = static_cast<LuaScript *>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return s;
}
int hooksLeft = 0;
void budgetHook(lua_State *L, lua_Debug *) {
    if (--hooksLeft < 0)
        luaL_error(L, "script ran too long (instruction budget exceeded)");
}

const ModWorld *world(lua_State *L) { return static_cast<const ModWorld *>(lua_touserdata(L, lua_upvalueindex(1))); }
int wPlayers(lua_State *L) {
    const ModWorld *w = world(L);
    lua_newtable(L);
    if (w && w->players) {
        int i = 1;
        for (const auto &p : w->players()) {
            lua_createtable(L, 3, 0);
            lua_pushnumber(L, p.x), lua_rawseti(L, -2, 1);
            lua_pushnumber(L, p.y), lua_rawseti(L, -2, 2);
            lua_pushnumber(L, p.z), lua_rawseti(L, -2, 3);
            lua_rawseti(L, -2, i++);
        }
    }
    return 1;
}
int32_t int32Arg(lua_State *L, int i) { return int32_t(luaL_checkinteger(L, i)); }
int wZoneLoaded(lua_State *L) {
    const ModWorld *w = world(L);
    lua_pushboolean(L, w && w->zoneLoaded && w->zoneLoaded(int32Arg(L, 1), int32Arg(L, 2)));
    return 1;
}
int wBlockType(lua_State *L) {
    const ModWorld *w = world(L);
    bool loaded = false;
    const uint8_t type = w && w->blockType ? w->blockType(int32Arg(L, 1), int32Arg(L, 2), int32Arg(L, 3), loaded) : 0;
    lua_pushinteger(L, type);
    lua_pushboolean(L, loaded);
    return 2;
}
int wGround(lua_State *L) {
    const ModWorld *w = world(L);
    lua_pushnumber(L, w && w->ground ? w->ground(int32Arg(L, 1), int32Arg(L, 2)) : 0.0);
    return 1;
}
int wBiome(lua_State *L) {
    const ModWorld *w = world(L);
    lua_pushinteger(L, w && w->biome ? w->biome(int32Arg(L, 1), int32Arg(L, 2)) : 0);
    return 1;
}
int wSpawn(lua_State *L) {
    const ModWorld *w = world(L);
    const glm::dvec3 at(luaL_checknumber(L, 4), luaL_checknumber(L, 5), luaL_checknumber(L, 6));
    lua_pushinteger(L, w && w->spawnCreature ? w->spawnCreature(int32Arg(L, 1), int32Arg(L, 2), int32Arg(L, 3), at) : 0);
    return 1;
}
int wAddFlags(lua_State *L) {
    const ModWorld *w = world(L);
    lua_pushboolean(L, w && w->addFlags && w->addFlags(luaL_checkinteger(L, 1), uint32_t(luaL_checkinteger(L, 2))));
    return 1;
}

int cLog(lua_State *L) {
    LuaScript *s = self(L);
    std::string line;
    for (int i = 1, n = lua_gettop(L); i <= n; ++i) {
        size_t len = 0;
        const char *text = luaL_tolstring(L, i, &len);
        line += (i > 1 ? "\t" : "") + std::string(text, len);
        lua_pop(L, 1);
    }
    if (s)
        s->hostLog(line);
    return 0;
}
int cConfig(lua_State *L) {
    LuaScript *s = self(L);
    const char *section = luaL_checkstring(L, 1), *key = luaL_checkstring(L, 2);
    const std::string *v = s ? s->findConfig(section, key) : nullptr;
    if (v)
        lua_pushlstring(L, v->data(), v->size());
    else
        lua_pushvalue(L, 3);
    return 1;
}
int cGetHour(lua_State *L) {
    LuaScript *s = self(L);
    lua_pushnumber(L, s && s->hostHour() ? *s->hostHour() : 12.0);
    return 1;
}
int cSetHour(lua_State *L) {
    LuaScript *s = self(L);
    const double h = luaL_checknumber(L, 1);
    if (s && s->hostHour() && std::isfinite(h))
        *s->hostHour() = float(std::fmod(std::fmod(h, 24.0) + 24.0, 24.0));
    return 0;
}
int cFocus(lua_State *L) {
    LuaScript *s = self(L);
    const float *f = s ? s->hostFocus() : nullptr;
    for (int i = 0; i < 3; ++i)
        lua_pushnumber(L, f ? f[i] : 0.0);
    return 3;
}

uint64_t fnv(const std::string &text) {
    uint64_t h = 1469598103934665603ull;
    for (unsigned char c : text)
        h = (h ^ c) * 1099511628211ull;
    return h;
}
} // namespace

LuaScript::~LuaScript() {
    if (L)
        lua_close(L);
}

void LuaScript::hostLog(const std::string &line) {
    if (host.log)
        host.log("[" + modName + "] " + line);
}
const std::string *LuaScript::findConfig(const std::string &section, const std::string &key) const {
    const std::string *v = config.find(section, key);
    return v ? v : host.config ? host.config->find(section, key) : nullptr;
}

void LuaScript::fail(const std::string &why) {
    failed = true;
    hostLog("error, script disabled: " + why);
}

bool LuaScript::call(const char *function, int args, int results) {
    // The function was pushed below its `args` arguments.
    hooksLeft = BUDGET_HOOKS;
    if (lua_pcall(L, args, results, 0) != LUA_OK) {
        fail(std::string(function) + ": " + (lua_tostring(L, -1) ? lua_tostring(L, -1) : "?"));
        lua_pop(L, 1);
        return false;
    }
    return true;
}

bool LuaScript::load(const std::filesystem::path &folder, const Host &h) {
    host = h;
    modName = folder.filename().string();
    config.load(folder.parent_path() / (modName + ".ini"));
    std::ifstream in(folder / "main.lua", std::ios::binary);
    if (!in) {
        hostLog("no main.lua");
        return false;
    }
    std::ostringstream text;
    text << in.rdbuf();
    const std::string source = text.str();
    textHash = fnv(source);

    L = luaL_newstate();
    if (!L)
        return false;
    // The safe standard libraries only.
    static const luaL_Reg libs[] = {{LUA_GNAME, luaopen_base},   {LUA_TABLIBNAME, luaopen_table}, {LUA_STRLIBNAME, luaopen_string},
                                    {LUA_MATHLIBNAME, luaopen_math}, {LUA_UTF8LIBNAME, luaopen_utf8}, {LUA_COLIBNAME, luaopen_coroutine}};
    for (const auto &lib : libs) {
        luaL_requiref(L, lib.name, lib.func, 1);
        lua_pop(L, 1);
    }
    for (const char *unsafe : {"dofile", "loadfile", "load", "require", "collectgarbage"}) {
        lua_pushnil(L);
        lua_setglobal(L, unsafe);
    }
    lua_pushlightuserdata(L, this);
    lua_setfield(L, LUA_REGISTRYINDEX, "cwv.script");
    lua_sethook(L, budgetHook, LUA_MASKCOUNT, HOOK_EVERY);
    // print goes to the log; the cwv table.
    lua_pushcfunction(L, cLog);
    lua_setglobal(L, "print");
    static const luaL_Reg cwv[] = {{"log", cLog},         {"config", cConfig}, {"get_hour", cGetHour},
                                   {"set_hour", cSetHour}, {"focus", cFocus},   {nullptr, nullptr}};
    luaL_newlib(L, cwv);
    lua_pushinteger(L, 2);
    lua_setfield(L, -2, "version");
    lua_setglobal(L, "cwv");

    // Text only: precompiled bytecode is refused.
    if (luaL_loadbufferx(L, source.data(), source.size(), ("@" + modName + "/main.lua").c_str(), "t") != LUA_OK) {
        fail(lua_tostring(L, -1) ? lua_tostring(L, -1) : "cannot compile");
        return false;
    }
    if (!call("main.lua", 0, 0))
        return false;
    // The optional `mod` table: name, version, changes_world.
    if (lua_getglobal(L, "mod") == LUA_TTABLE) {
        if (lua_getfield(L, -1, "name") == LUA_TSTRING)
            modName = lua_tostring(L, -1);
        lua_pop(L, 1);
        if (lua_getfield(L, -1, "version") == LUA_TSTRING || lua_isnumber(L, -1))
            modVersion = lua_tostring(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, -1, "changes_world");
        changes = lua_toboolean(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    hasWorldStep = lua_getglobal(L, "on_world_step") == LUA_TFUNCTION;
    lua_pop(L, 1);
    return true;
}

void LuaScript::update(double seconds) {
    if (failed || lua_getglobal(L, "on_update") != LUA_TFUNCTION) {
        if (!failed)
            lua_pop(L, 1);
        return;
    }
    lua_pushnumber(L, seconds);
    call("on_update", 1, 0);
}

bool LuaScript::key(int key) {
    if (failed || lua_getglobal(L, "on_key") != LUA_TFUNCTION) {
        if (!failed)
            lua_pop(L, 1);
        return false;
    }
    lua_pushinteger(L, key);
    if (!call("on_key", 1, 1))
        return false;
    const bool used = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return used;
}

void LuaScript::worldStep(const ModWorld &w, double milliseconds) {
    if (!stepsWorld())
        return;
    lua_getglobal(L, "on_world_step");
    lua_createtable(L, 0, 8);
    lua_pushinteger(L, w.seed);
    lua_setfield(L, -2, "seed");
    static const luaL_Reg services[] = {{"players", wPlayers},         {"zone_loaded", wZoneLoaded}, {"block_type", wBlockType},
                                        {"ground", wGround},           {"biome", wBiome},            {"spawn_creature", wSpawn},
                                        {"add_flags", wAddFlags},      {nullptr, nullptr}};
    lua_pushlightuserdata(L, const_cast<ModWorld *>(&w));
    luaL_setfuncs(L, services, 1);
    lua_pushnumber(L, milliseconds);
    call("on_world_step", 2, 0);
}

void LuaScript::shutdown() {
    if (!L || failed)
        return;
    if (lua_getglobal(L, "on_shutdown") == LUA_TFUNCTION)
        call("on_shutdown", 0, 0);
    else
        lua_pop(L, 1);
}

std::string LuaScript::identity() const {
    char hash[17];
    std::snprintf(hash, sizeof hash, "%016llx", (unsigned long long)textHash);
    return modName + " " + modVersion + " " + hash;
}
