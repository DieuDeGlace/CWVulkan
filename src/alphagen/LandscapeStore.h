#pragma once
// Cached hybrid Alpha terrain and voxel forests. Steam structure integration
// remains a separate stage; this is not native full-zone generation.
#include "alphagen/SurfaceStore.h"
#include "alphagen/TerrainSample.h"

namespace alphagen {
class LandscapeStore {
  public:
    using Parent = std::shared_ptr<const LandscapeStage>;
    explicit LandscapeStore(uint32_t seed, alphacoords::Mapping frame = {}, std::size_t cachedParents = 4, bool named = true,
                            std::shared_ptr<const TreeModels> palms = {});
    // Generation failures reach all waiters and are not retained as completed stages.
    Parent parent(int32_t alphaZoneX, int32_t alphaZoneY);
    std::optional<ColumnLease> column(int32_t steamX, int32_t steamY);
    std::optional<TerrainSample> sample(int32_t steamX, int32_t steamY);
    std::optional<alphacoords::Coverage> coverage(int32_t steamZoneX, int32_t steamZoneY) const;

  private:
    World world;
    Terrain terrain;
    const alphacoords::Mapping frame;
    detail::ParentStages<LandscapeStage> cache;
};
} // namespace alphagen
