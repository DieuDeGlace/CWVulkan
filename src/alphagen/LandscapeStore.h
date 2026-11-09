#pragma once
// Cached hybrid Alpha terrain and voxel forests. Steam structure integration
// remains a separate stage; this is not native full-zone generation.
#include "alphagen/SurfaceStore.h"
#include "alphagen/TerrainSample.h"

namespace alphagen {
class LandscapeStore {
  public:
    using Parent = std::shared_ptr<const LandscapeStage>;
    static constexpr std::size_t DEFAULT_CACHE_BYTES = 384 * 1024 * 1024;
    explicit LandscapeStore(uint32_t seed, alphacoords::Mapping frame = {}, std::size_t cachedParents = 4, bool named = true,
                            std::shared_ptr<const TreeModels> palms = {},
                            std::size_t cachedBytes = DEFAULT_CACHE_BYTES);
    // Generation failures reach all waiters and are not retained as completed stages.
    Parent parent(int32_t alphaZoneX, int32_t alphaZoneY);
    std::shared_ptr<const VegetationStage> vegetationParent(int32_t alphaZoneX, int32_t alphaZoneY);
    // Plant column anchors rebased into the requested Steam tile, without changing Z.
    std::vector<Plant> plantsForTile(int32_t steamZoneX, int32_t steamZoneY);
    std::optional<ColumnLease> column(int32_t steamX, int32_t steamY);
    std::optional<TerrainSample> sample(int32_t steamX, int32_t steamY);
    // Same prepared H as sample, without unrelated climate/color queries.
    std::optional<float> height(int32_t steamX, int32_t steamY);
    std::optional<glm::vec3> rockColour(int32_t steamX, int32_t steamY, int32_t sourceZ);
    std::optional<Block> rock(int32_t steamX, int32_t steamY, int32_t sourceZ);
    std::optional<alphacoords::Coverage> coverage(int32_t steamZoneX, int32_t steamZoneY) const;

  private:
    std::optional<alphacoords::Point> preparedPoint(int32_t steamX, int32_t steamY) const;
    World world;
    Terrain terrain;
    const alphacoords::Mapping frame;
    detail::ParentStages<LandscapeStage> cache;
    detail::ParentStages<VegetationStage> foliage;
};
} // namespace alphagen
