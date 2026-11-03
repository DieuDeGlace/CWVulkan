// Adapted from AlphaVulkan (DieuDeGlace), GPL-3.0-only; see docs/alpha-generation.md.
#pragma once
// The large-scale layer of Alpha's world (docs/specs/alpha-worldgen-regions.md): one jittered
// region cell per 16384-block region carrying moisture, warmth, elevation and a fire flag; the
// noise-displaced nearest-cell search and the distance-weighted climate blends built on it;
// the "link channels" between neighbouring cells; and the location records inside regions.
#include "alphagen/Random.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace alphagen {

// §2.1: one region cell.
struct RegionCell {
    int32_t x = 0, y = 0; // the control point, blocks (the centre of a location sub-cell)
    bool fire = false;
    float moisture = 0, warmth = 0;
    int32_t id = 0;
    int32_t elevation = 0; // negative: sea
};

// Locations spec §1.2: a location record (type 0: none).
struct Location {
    int64_t x = 0, y = 0; // centre, 1/65536 block
    float radius = 0;
    float height = 0; // ground height at the centre
    int32_t type = 0;
    int32_t variant = 0;
    int32_t serial = 0; // a running number, or an id (town: cell id; type 10: random id)
    int32_t level = 1;
    int32_t subLevel = 0;
};

// Locations spec §1.1: one zone's entry in its region's table, copied into the zone.
struct ZoneEntry {
    uint8_t kind = 0;    // 0 none, 1 town quarter, 3 dungeon entrance, 4 type-10 site
    uint8_t variant = 0; // town quarter 1..4, dungeon style 0..5
    int32_t id = 0;
    int32_t level = 1;
    uint8_t subLevel = 0;
};

// Locations spec §1: what a region holds once prepared.
struct Region {
    int32_t tier = 1, h10 = 0, style = 0;
    std::array<Location, 64> records{};     // record 8 i + j for slot (i, j)
    std::array<ZoneEntry, 64 * 64> zones{}; // entry 64 zxl + zyl
    uint32_t draws = 0;                     // draws made while preparing (for checks)
};

// The world's home point (spawn), blocks (overview §2).
constexpr int32_t HOME = 8396928;
constexpr int32_t REGION = 16384, ZONE = 256, SUBCELL = 2048;

class World {
  public:
    explicit World(uint32_t seed);

    uint32_t seed() const { return worldSeed; }
    const SeedOffsets &offsets() const { return o; }
    int32_t A() const { return o.A(); }

    // Locations spec §2 step 1: makes the region cells (rx + i, ry + j), i, j = -2..2, if missing.
    // Safe to call from several threads.
    void makeCells(int32_t rx, int32_t ry) const;
    // Locations spec §2: prepares region (rx, ry) once (its cells, then its location records and
    // zone table); `height` is the terrain's base height H. Other threads asking for the same
    // region wait for it. The region becomes visible to lookups only when it is finished.
    void prepareRegion(int32_t rx, int32_t ry, const std::function<float(int32_t, int32_t)> &height) const;
    // The prepared region, or null (also while it is being prepared).
    const Region *region(int32_t rx, int32_t ry) const;
    // Whether the world has a name (locations spec §0); every world in play has one.
    bool named = true;
    // §2: the cell of region (rx, ry) if it has been made, else null.
    const RegionCell *cell(int32_t rx, int32_t ry) const;
    // §2.2: makes it now if missing.
    const RegionCell *makeCell(int32_t rx, int32_t ry) const;

    // §3.1: the sample displacement of a block column.
    glm::vec2 displacement(int32_t x, int32_t y) const;
    // §3.4: the owning cell's index, false if none.
    bool ownerIndex(int32_t x, int32_t y, int32_t &i, int32_t &j) const;
    const RegionCell *owner(int32_t x, int32_t y) const;
    // ui-screens §5.1.6: whether zone (zx, zy) belongs to land (region cell) (lx, ly) for its
    // map picture. Only the 8 neighbour cells compete, and ties belong to both.
    bool landOwnsZone(int32_t lx, int32_t ly, int32_t zx, int32_t zy) const;

    // §4: the climate blends of a block column.
    float warmth(int32_t x, int32_t y) const;
    float moisture(int32_t x, int32_t y) const;
    float seaShare(int32_t x, int32_t y) const;
    float fireShare(int32_t x, int32_t y) const;

    // §5.2: the link-channel distance B (0 on the lines between cell points, 1 far away).
    float channel(int32_t x, int32_t y) const;
    // §5.3: grid coordinates G (for town streets).
    glm::dvec2 grid(int32_t x, int32_t y) const;

    // §6.3: the location of a column's 2048-block sub-cell, null if none (or type 0).
    const Location *location(int32_t x, int32_t y) const;
    // A zone's entry in its region's table (locations spec §1.1), null if the region is not ready.
    const ZoneEntry *zoneEntry(int32_t zx, int32_t zy) const;
    // §7: the normalised distance of block column (x, y) from location L (0 at the centre,
    // 1 on its rim).
    float locationDistance(const Location &L, int32_t x, int32_t y) const;
    // The same at a point in 1/65536 blocks.
    float locationDistanceFx(const Location &L, int64_t X, int64_t Y) const;

  private:
    // §3.3: the integer squared distance from the displaced column to a cell.
    static int32_t squaredDistance(const RegionCell &c, int32_t px, int32_t py);
    // §4: the shared two-pass blend; `value` reads one cell.
    template <class Value> float blend(int32_t x, int32_t y, float fallback, Value value) const;
    // The displaced column point of §3.3.
    void displaced(int32_t x, int32_t y, int32_t &px, int32_t &py) const;
    // The two noise sums that move a column for the link channels and the grid (§5.2 step 2):
    // P = (x, y) + 500 * sums.
    glm::dvec2 channelNoise(int32_t x, int32_t y) const;

    std::unique_ptr<Region> buildRegion(int32_t rx, int32_t ry, const std::function<float(int32_t, int32_t)> &height) const;

    uint32_t worldSeed;
    SeedOffsets o;
    mutable std::shared_mutex cellsLock;
    // (Caches of what the seed decides: filling them does not change the world.)
    mutable std::unordered_map<uint32_t, std::unique_ptr<RegionCell>> cells; // key rx * 1024 + ry
    // A region being or having been prepared: built once, published when done.
    struct RegionSlot {
        std::once_flag once;
        std::atomic<const Region *> ready{nullptr};
        std::unique_ptr<Region> region;
    };
    mutable std::shared_mutex regionsLock;
    mutable std::unordered_map<uint32_t, std::shared_ptr<RegionSlot>> regions;
};

} // namespace alphagen
