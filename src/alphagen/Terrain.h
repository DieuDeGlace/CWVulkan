#pragma once
// Alpha terrain stages, adapted from AlphaVulkan's GPL clean-room implementation.
// Surface and pre-dungeon relief are separate boundaries; later generation is not implied.
#include "alphagen/Regions.h"
#include "alphagen/Zone.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

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
    std::unique_ptr<const Zone> zone;
    std::vector<float> paddedHeights;
    std::vector<BoulderSeed> ravineBoulders;
    Random random;
};

// Initial terrain followed by the rock, road and terrace passes; stops before
// Alpha dungeon generation. The retained stream/seeds belong to this boundary.
struct ReliefStage {
    std::unique_ptr<const Zone> zone;
    Random random;
    std::vector<BoulderSeed> terraceBoulders, ravineBoulders;
};

enum class FormationTest { Mound, MesaGuard, MesaFill };
int32_t formationTop(int32_t x, int32_t y, int32_t z, int32_t rz, bool mesa);
glm::vec2 formationUV(FormationTest, int32_t X, int32_t Y, int32_t Z, int32_t x, int32_t y, int32_t rx, int32_t ry);
float roadShade(int32_t x, int32_t y);
struct Terrace {
    int32_t zb = 0;
    float t = 0, a = 0;
    int32_t zlow = 0, zw = 0;
};
Terrace terraceAt(float height);

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
    std::unique_ptr<ReliefStage> generateRelief(int32_t zx, int32_t zy, std::string &error) const;
    // Native formations and boulders use positive radii and operate only inside the supplied zone.
    void formation(Zone &, int32_t x, int32_t y, int32_t z, int32_t rx, int32_t ry, int32_t rz, bool mesa) const;
    void boulder(Zone &, const BoulderSeed &, int32_t ax, int32_t ay, int32_t az, bool fromRavine) const;

  private:
    struct SurfaceWork {
        std::unique_ptr<Zone> zone;
        std::vector<float> paddedHeights;
        std::vector<BoulderSeed> ravineBoulders;
        Random random;
    };
    std::unique_ptr<SurfaceWork> buildSurface(int32_t zx, int32_t zy) const;
    bool reliefPasses(Zone &, const std::vector<float> &, Random &, std::vector<BoulderSeed> &, std::string &error) const;
    void surfacePass(Zone &, const std::vector<float> &, Random &, std::vector<BoulderSeed> &) const;
    Block rockBlock(int32_t x, int32_t y, int32_t z, const Zone &) const;
    float noiseAt(uint32_t ox, uint32_t oy, double k, int32_t x, int32_t y) const;
    const World &w;
};
} // namespace alphagen
