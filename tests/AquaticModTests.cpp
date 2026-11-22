// Mod API version 2 (include/cubevulkan/ModApi.h): the AquaticMod example DLL, driven through ModHost
// and the CwvWorld services, makes exactly the creatures the built-in add-on's rules make.
#include "AquaticPlan.h"
#include "Mods.h"
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
#include <windows.h>

#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++aquaticModFailures; } } while (0)
static int aquaticModFailures = 0;

int aquaticModTests() {
    aquaticModFailures = 0;
    using namespace aquatic;
    // Mod API version 2: the AquaticMod example DLL, driven through ModHost and the CwvWorld services,
    // makes exactly the creatures the built-in add-on's rules make in the same world.
    struct Made {
        int race, hostility, level;
        glm::dvec3 at;
        uint32_t flags;
        bool operator==(const Made &) const = default;
    };
    // A sea floor at -20 under water up to z 0 west of x = 0, a pond (floor 10, surface 12) to the east.
    const auto ground = [](int32_t x, int32_t) { return x < 0 ? -21.0f : 9.0f; };
    const auto type = [](int32_t x, int32_t, int32_t z) -> uint8_t {
        if (x < 0)
            return z <= -21 ? 1 : z <= 0 ? 2 : 0;
        return z <= 9 ? 1 : z <= 11 ? 2 : 0;
    };
    const std::vector<glm::dvec3> players = {{0.5, 0.5, 1}};
    auto run = [&](auto &&step) {
        std::vector<Made> made;
        auto spawn = [&](int race, int hostility, int level, const glm::dvec3 &at) -> int64_t {
            made.push_back({race, hostility, level, at, 0});
            return int64_t(made.size());
        };
        auto flag = [&](int64_t id, uint32_t f) {
            if (id < 1 || id > int64_t(made.size()))
                return false;
            made[size_t(id - 1)].flags |= f;
            return true;
        };
        for (int i = 0; i < 3; ++i)
            step(spawn, flag);
        return made;
    };
    Planner reference;
    const auto builtIn = run([&](auto &spawn, auto &flag) {
        Env env;
        env.zoneLoaded = [](int32_t, int32_t) { return true; };
        env.blockType = [&](int32_t x, int32_t y, int32_t z, bool &loaded) { loaded = true; return type(x, y, z); };
        env.ground = ground;
        env.ocean = [](int32_t x, int32_t) { return x < 0; };
        env.make = [&](const School &s, const glm::dvec3 &at) { return spawn(s.race, s.hostility, s.stars, at); };
        env.addFlags = [&](int64_t id, uint32_t f) { flag(id, f); };
        reference.update(env, players, 123, 1000);
    });
    wchar_t exe[MAX_PATH];
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    const std::filesystem::path dll = std::filesystem::path(exe).parent_path() / "examples" / "AquaticMod.dll";
    ModHost host;
    ModContext context;
    std::vector<std::string> logged;
    context.log = [&](const std::string &line) { logged.push_back(line); };
    CHECK(host.loadOne(dll, context) && host.stepsWorld());
    CHECK(host.worldChangingMods() == std::vector<std::string>{"AquaticMod 1"});
    const auto viaMod = run([&](auto &spawn, auto &flag) {
        ModWorld w;
        w.seed = 123;
        w.players = [&] { return players; };
        w.zoneLoaded = [](int32_t, int32_t) { return true; };
        w.blockType = [&](int32_t x, int32_t y, int32_t z, bool &loaded) { loaded = true; return type(x, y, z); };
        w.ground = ground;
        w.biome = [](int32_t x, int32_t) { return int32_t(x < 0 ? OCEAN_BIOME : 3); };
        w.spawnCreature = [&](int32_t race, int32_t hostility, int32_t level, const glm::dvec3 &at) { return spawn(race, hostility, level, at); };
        w.addFlags = [&](int64_t id, uint32_t f) { return flag(id, f); };
        host.worldStep(w, 1000);
    });
    host.shutdown();
    size_t bossesMade = 0, fish = 0;
    for (const Made &m : builtIn)
        bossesMade += (m.flags & BOSS_FLAG) != 0, fish += (m.flags & FISH_FLAG) != 0;
    CHECK(!builtIn.empty() && fish == builtIn.size());
    CHECK(viaMod == builtIn);
    std::printf("Aquatic mod: %zu creatures (%zu bosses), identical to the built-in add-on: %s\n", viaMod.size(), bossesMade,
                viaMod == builtIn ? "yes" : "NO");
    return aquaticModFailures;
}
