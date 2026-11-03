#pragma once
// Cached terrain through the rock/road/terrace boundary. Steam structures and
// Alpha dungeon/post-dungeon stages are not included.
#include "alphagen/SurfaceStore.h"

namespace alphagen {
class ReliefStore {
  public:
    using Parent = std::shared_ptr<const ReliefStage>;
    explicit ReliefStore(uint32_t seed, alphacoords::Mapping frame = {}, std::size_t cachedParents = 4, bool named = true,
                         std::shared_ptr<const TreeModels> palms = {});
    // Generation failures reach all waiters and are not retained as completed stages.
    Parent parent(int32_t alphaZoneX, int32_t alphaZoneY);
    std::optional<ColumnLease> column(int32_t steamX, int32_t steamY);
    std::optional<alphacoords::Coverage> coverage(int32_t steamZoneX, int32_t steamZoneY) const;

  private:
    World world;
    Terrain terrain;
    const alphacoords::Mapping frame;
    detail::ParentStages<ReliefStage> cache;
};
} // namespace alphagen
