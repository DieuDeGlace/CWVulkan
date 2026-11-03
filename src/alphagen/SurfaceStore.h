#pragma once
// Immutable seed/frame context and shared Alpha parent stages for Steam-sized consumers.
#include "alphagen/Coordinates.h"
#include "alphagen/Terrain.h"
#include "alphagen/ParentStages.h"

namespace alphagen {
class SurfaceStore;
class ColumnLease {
  public:
    const Column &raw() const;
    Block block(int32_t z) const;
    float surface(bool includeWater = true) const;
    alphacoords::Point position() const { return at; }

  private:
    friend class SurfaceStore;
    friend class ReliefStore;
    ColumnLease(std::shared_ptr<const Zone> zone, alphacoords::Point point) : zone(std::move(zone)), at(point) {}
    std::shared_ptr<const Zone> zone; // Aliases the parent stage, retaining its ownership across eviction.
    alphacoords::Point at;
};

// Only the initial surface stage. Later terrain/Steam placement integration must
// retain its own stage/version identity rather than rebrand this as full generation.
class SurfaceStore {
  public:
    using Parent = std::shared_ptr<const SurfaceStage>;
    explicit SurfaceStore(uint32_t seed, alphacoords::Mapping frame = {}, std::size_t cachedParents = 4, bool named = true);
    Parent parent(int32_t alphaZoneX, int32_t alphaZoneY);
    std::optional<ColumnLease> column(int32_t steamX, int32_t steamY);
    std::optional<alphacoords::Coverage> coverage(int32_t steamZoneX, int32_t steamZoneY) const;

  private:
    World world;
    Terrain terrain;
    const alphacoords::Mapping frame;
    detail::ParentStages<SurfaceStage> cache;
};
} // namespace alphagen
