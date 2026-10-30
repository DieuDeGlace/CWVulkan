#pragma once
// CubeVulkan's first-entrant region difficulty. This is a gameplay option, not
// a claim about vanilla Steam progression. Empty regions retain no discovery lock.
#include "net/Wire.h"
#include "worldgen/Regions.h"
#include <map>
#include <mutex>
#include <optional>
#include <set>

namespace regionlevels {
using Region = std::pair<int32_t, int32_t>;
using Levels = std::map<Region, int>;
constexpr const char *CHANNEL = "cwv.region-levels.v1";

class Occupancy {
  public:
    // Arrival order is authoritative. Level changes while inside do not replace
    // the first entrant's baseline; another occupant keeps the region active.
    void update(int64_t player, worldgen::Int2 region, int level);
    void leave(int64_t player);
    void unload(worldgen::Int2 region);
    void clear();
    int basis(worldgen::Int2 region) const;
    std::optional<Region> regionOf(int64_t player) const;
    Levels snapshot() const;

  private:
    struct Active {
        int level;
        std::set<int64_t> players;
    };
    void leaveLocked(int64_t player);
    mutable std::mutex mutex;
    std::map<int64_t, Region> players;
    std::map<Region, Active> regions;
};

struct Snapshot {
    bool enabled = false;
    Levels levels;
    int basis(worldgen::Int2 region) const;
};
cwnet::Bytes encode(const Snapshot &);
std::optional<Snapshot> decode(const cwnet::Bytes &);
} // namespace regionlevels
