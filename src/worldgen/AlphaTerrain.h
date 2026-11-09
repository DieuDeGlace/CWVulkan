#pragma once
// Project hybrid policy: Alpha relief with Steam land floors, biome palettes and
// conservative construction cores. Savannah retains its gentler native terrain.
// Steam generation and the raw Alpha generator remain unchanged.
#include "alphagen/LandscapeStore.h"
#include "alphagen/PlantModels.h"
#include "worldgen/Structures.h"
namespace worldgen {
struct MapRegion;
class AlphaTerrain {
  public:
    explicit AlphaTerrain(uint32_t seed, std::shared_ptr<const alphagen::TreeModels> palms = {}, int32_t steamSeed = 0,
                          std::shared_ptr<const alphagen::PlantModels> plants = {});
    std::string identity(double scale = 1) const;
    bool coversZone(int32_t zx, int32_t zy) const;
    std::optional<glm::vec3> implicitColour(const TerrainColumn &, int32_t x, int32_t y, int32_t z);
    std::optional<Block> rock(int32_t x, int32_t y, int32_t sourceZ);
    std::optional<alphagen::TerrainSample> sample(int32_t x, int32_t y) { return source.sample(x, y); }
    // Coarse H-based map overview; exact final voxel geometry remains in compose.
    void composeMap(MapRegion &);
    // Transactional: a failure leaves the native zone unchanged.
    bool compose(ZoneContents &, const WorldValues &, std::string &error);

  private:
    const uint32_t seed;
    const int32_t steamSeed;
    alphagen::LandscapeStore source;
    const std::shared_ptr<const alphagen::PlantModels> plantModels;
};
} // namespace worldgen
