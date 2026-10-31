#pragma once
// Independent steam-authoritative-residents.md: membership/region snapshots,
// current player positions, nonnull actor admission and final flag reconciliation.
#include <array>
#include <cstdint>
#include <functional>
#include <set>
#include <vector>
#include <utility>

namespace residentpass {
using Region = std::array<int32_t, 2>;
class Collection {
  public:
    // Engine storage may move a record while constructing residents. Resolve the
    // same registered actor by ID in that case; standalone callers can borrow it.
    using Resolve = std::function<const uint8_t *(int64_t)>;
    explicit Collection(Resolve resolve = {}) : resolve_(std::move(resolve)) {}
    void collect(const uint8_t *record);
    void added(int64_t id) { ids_.insert(id); }
    bool contains(int64_t id) const { return ids_.count(id) != 0; }
    std::vector<const uint8_t *> players() const;
    bool withinRange(int64_t x, int64_t y, int64_t z) const;
    bool portalCleared(Region region) const { return portals_.count(region) != 0; }
    void releasePetrified(uint8_t *record) const;

  private:
    struct Player {
        int64_t id;
        const uint8_t *record;
    };
    const uint8_t *recordOf(const Player &player) const;
    Resolve resolve_;
    std::vector<Player> players_;
    std::set<int64_t> ids_;
    std::set<Region> portals_, released_;
};
} // namespace residentpass
