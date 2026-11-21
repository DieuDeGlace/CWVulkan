#pragma once
// CubeVulkan's aquatic encounters ([Creatures] AquaticEnemies, off by default; not the game's): schools
// of fish, crabs and slimes in deep enough water, now and then a boss, made once per loaded zone near a
// player. The user's add-on, at parity with their later Infinite-Cubes Vulkan work (the game's own AI
// moves them).
#include <glm/glm.hpp>

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <set>
#include <vector>

class ExtractedWorld;
class SpawnedCreatures;

namespace aquatic {

constexpr int NEUTRAL = 4, HOSTILE = 1;
constexpr uint32_t FISH_FLAG = 1u << 8, BOSS_FLAG = 0x40000;

// A school planned for a zone: one race, its members' places (absolute blocks).
struct School {
    int race = 0, hostility = HOSTILE, stars = 0;
    bool boss = false;
    std::vector<glm::dvec3> members;
};
// The water at a column: its floor (first water block) and surface (first block above the water); false
// when the column holds no water or is covered.
using WaterColumn = std::function<bool(int32_t x, int32_t y, int32_t &floor, int32_t &surface)>;
// The zone's school, from its water (`ocean`: the column's land is ocean), or none.
std::optional<School> plan(int32_t zx, int32_t zy, int32_t seed, const WaterColumn &column, const std::function<bool(int32_t, int32_t)> &ocean);
bool passiveFish(int race);
bool bottomDweller(int race);
uint32_t waterHash(int32_t x, int32_t y, int32_t seed);

class Manager {
  public:
    // Plans and makes the schools of loaded zones within two zones of each player (absolute blocks).
    void update(SpawnedCreatures &world, ExtractedWorld &terrain, const std::vector<glm::dvec3> &players, int32_t seed, double elapsedMs);

  private:
    std::set<std::array<int32_t, 2>> done;
    double sinceUpdateMs = 0;
};

} // namespace aquatic
