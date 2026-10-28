#pragma once
// Loads code mods (mods/*.dll exporting cwv_mod_load) and gives them the
// CwvApi table from include/cubevulkan/ModApi.h.
#include "Config.h"
#include <cubevulkan/ModApi.h>
#include <glm/glm.hpp>
#include <filesystem>
#include <functional>
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

class ModHost {
  public:
    // Loads every DLL in `dir` in alphabetical order.
    void load(const std::filesystem::path &dir, ModContext &context);
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
};
