#pragma once
// The region planner (0x115df0; docs/specs/worldgen-creatures.md §2, conversation-script.md §1.5,
// §5.5, lore-and-artifacts.md §5.1): for each 8192-block region, which of its places gets which
// role (a missing gnome, a crafting book, an artifact, a witch, a demon portal, a monster...) and
// at what level, the region's special town, its items and artifacts. It uses only hashes, never
// rand(), so a region's plan is always the same.
#include "worldgen/Regions.h"
#include "worldgen/ZoneRecords.h"

#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace worldgen {

using PlaceCell = std::pair<int32_t, int32_t>; // compared as signed ints, x first

struct RegionPlan {
    Int2 region;
    std::map<PlaceCell, int> roles;     // +0x10: a big place under its even corner
    std::map<PlaceCell, Item> artifacts; // +0x20
    std::vector<PlaceCell> helpTargets;  // +0x30
    std::vector<PlaceCell> artifactPlaces; // +0x48
    std::map<PlaceCell, int> levels;    // +0x60
    Int2 specialTown{0x7fffffff, 0x7fffffff}; // +0x80
    bool marked = false;                // +0x88: a demon portal is in the region
    uint32_t itemBits = 0;              // +0x8c
    std::vector<int32_t> items;         // +0x90

    // 0x93480 and 0x12a200: 0 when the place has none.
    int roleAt(int32_t x, int32_t y) const;
    int levelAt(int32_t x, int32_t y) const;
    // The place's own treasure (0x131540): a crafting book for roles 2-5, the artifact for 14,
    // else none (category 0).
    Item roleTreasure(int32_t x, int32_t y) const;
    // The region item of a place (0x119800).
    int32_t regionItem(int32_t px, int32_t py) const;
    // The door item of a cell (0x12a600): -1 for an empty list.
    int32_t doorItem(int32_t cx, int32_t cy) const;
    // The encounter of a place on the world's day (0x119a00; conversation-script.md §5.5).
    int encounter(int32_t x, int32_t y, int32_t day) const;
};

// Region roles: the ones the zone builders and the talk read.
namespace role {
constexpr int GNOME = 1;
constexpr int ARTIFACT = 14;
constexpr int MONSTER = 16;
constexpr int ARENA = 17;
constexpr int WITCH = 19;
constexpr int DEMON_PORTAL = 20;
constexpr int TOWER = 21;
constexpr int BRAZIERS = 22;
constexpr int SPIDERS = 23;
} // namespace role

// The plan of region (rx, ry).
RegionPlan makeRegionPlan(int32_t rx, int32_t ry);
// The same, kept in a small per-thread cache.
std::shared_ptr<const RegionPlan> regionPlan(int32_t rx, int32_t ry);

// The plan of the region a place belongs to (the region of its landmark point), and the artifact
// hidden at a place (0x119660; an empty item for a place that hides none). A big place answers for
// its even corner.
std::shared_ptr<const RegionPlan> placePlan(int32_t px, int32_t py);
Item placeArtifact(int32_t px, int32_t py);

// An artifact as the planner makes it for place (x, y) at counter c (0x11d7b0); it uses X(c) and
// X(c + 1).
Item makeArtifact(int32_t x, int32_t y, uint32_t c);

} // namespace worldgen
