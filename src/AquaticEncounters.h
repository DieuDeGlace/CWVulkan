#pragma once
// CubeVulkan's aquatic encounters ([Creatures] AquaticEnemies, on by default at the owner's request; not the
// game's): the shared rules of AquaticPlan.h run on the engine's host world. The user's add-on, at parity
// with their later Infinite-Cubes Vulkan work (the game's own AI moves them).
#include "AquaticPlan.h"

class ExtractedWorld;
class SpawnedCreatures;

namespace aquatic {

class Manager {
  public:
    // Plans and makes the schools of loaded zones within two zones of each player (absolute blocks).
    void update(SpawnedCreatures &world, ExtractedWorld &terrain, const std::vector<glm::dvec3> &players, int32_t seed, double elapsedMs);

  private:
    Planner planner;
};

} // namespace aquatic
