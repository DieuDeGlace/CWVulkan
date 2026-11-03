#pragma once
// Immutable seed/frame context and shared Alpha parent stages for Steam-sized consumers.
#include "alphagen/Coordinates.h"
#include "alphagen/Terrain.h"
#include <future>
#include <list>
#include <map>
#include <mutex>

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
    ColumnLease(std::shared_ptr<const SurfaceStage> stage, alphacoords::Point point) : stage(std::move(stage)), at(point) {}
    std::shared_ptr<const SurfaceStage> stage; // Keeps the column alive across eviction/store destruction.
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
    using Key = std::pair<int32_t, int32_t>;
    World world;
    Terrain terrain;
    const alphacoords::Mapping frame;
    const std::size_t capacity;
    std::mutex lock;
    std::list<std::pair<Key, Parent>> ready; // MRU first; external leases can outlive cache entries.
    std::map<Key, std::shared_future<Parent>> pending;
};
} // namespace alphagen
