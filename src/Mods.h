#pragma once
// Loads code mods (mods/*.dll exporting cwv_mod_load) and gives them the
// CwvApi table from include/cubevulkan/ModApi.h.
#include "Config.h"
#include "LuaMods.h"
#include <cubevulkan/ModApi.h>
#include <glm/glm.hpp>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class World;

struct ModContext {
    World *world = nullptr;
    const Config *config = nullptr;
    float *hour = nullptr;
    glm::vec3 *focus = nullptr;
    std::function<void(const std::string &)> log;
};

// Version 2: what the host world offers mods during its step (CwvWorld in ModApi.h).
struct ModWorld {
    int32_t seed = 0;
    std::function<std::vector<glm::dvec3>()> players;
    std::function<bool(int32_t zx, int32_t zy)> zoneLoaded;
    std::function<uint8_t(int32_t x, int32_t y, int32_t z, bool &loaded)> blockType;
    std::function<float(int32_t x, int32_t y)> ground;
    std::function<int32_t(int32_t x, int32_t y)> biome;
    std::function<int64_t(int32_t race, int32_t hostility, int32_t level, const glm::dvec3 &at)> spawnCreature;
    std::function<bool(int64_t id, uint32_t flags)> addFlags;
};

class ModHost {
  public:
    // Loads every DLL in `dir` in alphabetical order.
    void load(const std::filesystem::path &dir, ModContext &context);
    // Version 2: one host world step for the mods that take part (call under the host's lock).
    void worldStep(const ModWorld &world, double milliseconds);
    // Whether any loaded mod takes part in the world step.
    bool stepsWorld() const;
    // "name version" of each loaded mod that changes the world, for multiplayer matching.
    std::vector<std::string> worldChangingMods() const;
    // Loads a single DLL (also used by tests); false when it is not a CubeVulkan mod or declines.
    bool loadOne(const std::filesystem::path &dll, ModContext &context);
    // Loads a Lua script mod folder (mods/<Name>/main.lua; also used by tests).
    bool loadScript(const std::filesystem::path &folder, ModContext &context);
    void update(double seconds);
    bool key(int key);
    void shutdown();
    std::vector<std::string> names() const;

  private:
    struct Loaded {
        void *module = nullptr;
        CwvMod mod{};
        std::string name;
        Config config; // mods/<Name>.ini
    };
    std::vector<Loaded> mods;
    std::vector<std::unique_ptr<LuaScript>> scripts;
};
