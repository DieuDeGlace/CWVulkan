#pragma once
// Alpha surface generation, adapted from AlphaVulkan's GPL clean-room implementation.
// The named boundary precedes rock/road/structure/vegetation passes and block lighting.
#include "alphagen/Regions.h"
#include "alphagen/Zone.h"
#include <glm/glm.hpp>
#include <memory>

namespace alphagen {
struct Climate {
    float moisture, warmth, fire;
};
struct BoulderSeed {
    int32_t x, y, z;
};

// Retain the exact intermediate products and stream for later terrain passes.
// This must not be labeled as a complete generated Alpha world.
struct SurfaceStage {
    std::unique_ptr<Zone> zone;
    std::vector<float> paddedHeights;
    std::vector<BoulderSeed> ravineBoulders;
    Random random;
};

class Terrain {
  public:
    explicit Terrain(const World &world) : w(world) {}
    float townness(int32_t x, int32_t y) const;
    float flatness(int32_t x, int32_t y) const;
    float roughness(int32_t x, int32_t y) const;
    float mountains(int32_t x, int32_t y) const;
    float height(int32_t x, int32_t y) const;
    Climate climate(int32_t x, int32_t y, const Zone *zone = nullptr) const;
    glm::vec3 grass(int32_t x, int32_t y) const;
    glm::vec3 sand(int32_t x, int32_t y) const;
    glm::vec3 beach(int32_t x, int32_t y) const;
    glm::vec3 snow(int32_t x, int32_t y) const;
    glm::vec3 rock(int32_t x, int32_t y, int32_t z, const Zone *zone = nullptr) const;
    glm::vec3 cliff(int32_t x, int32_t y, int32_t z, const Zone *zone = nullptr) const;
    glm::vec3 stone(int32_t x, int32_t y, int32_t z) const;
    Block surface(int32_t x, int32_t y, int32_t z, const Zone *zone = nullptr) const;
    Block surface(int32_t x, int32_t y, int32_t z, const Climate &, const Zone *) const;
    void prepareRegion(int32_t rx, int32_t ry) const;
    std::unique_ptr<SurfaceStage> generateSurface(int32_t zx, int32_t zy) const;

  private:
    void surfacePass(Zone &, const std::vector<float> &, Random &, std::vector<BoulderSeed> &) const;
    Block rockBlock(int32_t x, int32_t y, int32_t z, const Zone &) const;
    float noiseAt(uint32_t ox, uint32_t oy, double k, int32_t x, int32_t y) const;
    const World &w;
};
} // namespace alphagen
