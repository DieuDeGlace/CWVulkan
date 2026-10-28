#pragma once
// The state of a zone while part B fills it (docs/specs/worldgen-structures.md §2-§5): the
// terrain it continues (blocks, column values and the rand() stream), the zone's place, the
// record lists and the spot map. Every builder of part B works on one of these; part C's
// creature passes get it too.
#include "worldgen/CellObjects.h"
#include "worldgen/Spawns.h"
#include "worldgen/StructureTables.h"
#include "worldgen/ZoneRecords.h"
#include "worldgen/ZoneTerrain.h"

#include <cstdint>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

namespace worldgen {

struct RegionPlan; // part C (Planner.h)

// The zone's place (§5), computed once at the start of the generator.
struct ZonePlace {
    Place place;        // 0x31a5d0 at the place cell; with kind 0 the landmark point is (0, 0)
    Int2 cell;          // C1: the place cell, evened when big
    int structure = 0;  // B: the big structure, 0 for none
    Int2 anchor = NO_ANCHOR; // A
    uint32_t style = 0; // the place's realm (lore-and-artifacts.md §4.3)
    int people = 0;     // the people race of the style (part C uses it)
    int biome = 0;      // the biome of the region of the landmark point
    int culture = 0;    // the building culture (tables T9)
    Int2 region;        // the region of the landmark point (the planner's)
    ColourContext colours() const { return {style, culture}; }
};

// The place's realm (0x2bb060).
uint32_t placeStyle(int32_t px, int32_t py);

// What part B reads from the region planner (part C, worldgen-creatures.md): the role of the zone's
// place cell, the planner's item list and the role's own treasure.
struct PlannerFacts {
    int role = 0;
    std::vector<int> items;
    Item treasure; // category 0 for none
    // Part C's whole plan of the region (null when part C is not run).
    std::shared_ptr<const RegionPlan> plan;
    // The planner's item for a cell (0x12a600): an entry of the item list chosen by
    // R(cx, cy, 0x89d85) mod its size, or -1 when the list is empty.
    int itemForCell(int32_t cx, int32_t cy) const;
};

// A spot (contents file §C1): a position where something may stand.
struct Spot {
    int64_t x = 0, y = 0, z = 0;
};
using SpotKey = std::tuple<int32_t, int32_t, int>; // cell x, cell y, kind
using SpotMap = std::map<SpotKey, std::vector<Spot>>;

class ZoneBuild {
public:
    ZoneBuild(ZoneTerrain &terrain, const WorldValues &world, CellObjects &cells, bool recordDraws);

    ZoneTerrain &terrain;
    const WorldValues &world;
    CellObjects &cells;
    ZonePlace place;
    PlannerFacts planner;
    int32_t zx, zy;
    int32_t x0, y0; // the zone's first block

    // --- The rand() stream ---
    int draw();
    void seed(uint32_t value) { terrain.random.seed(value); }
    // "rand fraction x a": float of rand x a / 32767.
    float fraction(float a) { return (float)draw() * a / 32767.0f; }
    // "rand angle": float of (double of rand x 2 pi / 32767).
    float angle() { return (float)((double)draw() * 6.283185307179586 / 32767.0); }

    // --- Blocks (§3) ---
    bool inZone(int32_t x, int32_t y) const { return terrain.contains(x, y); }
    bool insideDots(int64_t x, int64_t y) const;
    Block read(int32_t x, int32_t y, int32_t z) const { return terrain.read(x, y, z); }
    void store(int32_t x, int32_t y, int32_t z, Block b) { terrain.store(x, y, z, b); }
    // "Put unless protected": read first and skip a block with flag 2.
    void putUnlessProtected(int32_t x, int32_t y, int32_t z, Block b);
    // The column at (x, y) of the zone (nullptr outside).
    const TerrainColumn *column(int32_t x, int32_t y) const;
    // A column's values: the zone's field inside; outside, made as the field makes them (with
    // the zone's place).
    ColumnField columnValuesAt(int32_t x, int32_t y) const;

    int cellType(int32_t cx, int32_t cy) { return cells.type(cx, cy); }

    // --- Records ---
    std::vector<StaticObject> objects;
    std::vector<Model> models;
    std::vector<GroundItem> items;
    std::map<int, Item> rewards; // a reward chest's item by its index in `objects`
    // "Keep if inside" (0x1194b0, 0x2fb7d0): appended when x and y lie in the zone.
    bool keepIfInside(const StaticObject &o);
    bool keepIfInside(const Model &m);
    // Settling (contents file §C8): false when the object can't stand there.
    bool settle(StaticObject &o, bool groundTest) const;
    // "Settled and kept": settle with the ground test, then keep when true and inside.
    bool settleAndKeep(StaticObject o);
    // A chest with a reward: kept if inside, and then the reward stored under its index.
    void keepChest(const StaticObject &o, const Item &reward);

    // --- Spots (contents file §C1) ---
    SpotMap spots;   // zone +0x100
    SpotMap working; // zone +0x110: the copy spots are given out from
    void addSpot(int32_t cx, int32_t cy, int kind, Spot s) { spots[{cx, cy, kind}].push_back(s); }
    int spotCount(int32_t cx, int32_t cy, int kind) const; // 0x1299a0
    Spot takeSpot(int32_t cx, int32_t cy, int kind);       // 0x31bd90
    Spot spotInCell(int32_t cx, int32_t cy);                // 0x31c0e0

    // --- Part C's creatures (worldgen-creatures.md) ---
    ZoneSpawns spawns;

private:
    bool recordDraws_;
};

// Taking spots (contents file §C1, worldgen-creatures.md §5.3, §5.4) from a zone's spot map, its
// working copy and objects: the generator's, and the world's afterwards (its creatures walk to a
// cell spot of a loaded zone, 0x31c0e0, drawing the world's rand()). `middle` is the top of the
// zone's middle column, where a spot comes from when there is none.
struct ZoneSpots {
    const SpotMap &spots;
    SpotMap &working;
    const std::vector<StaticObject> &objects;
    Spot middle;
};
Spot takeSpot(const ZoneSpots &zone, int32_t cx, int32_t cy, int kind, Draw rand); // 0x31bd90
Spot spotInCell(const ZoneSpots &zone, int32_t cx, int32_t cy, Draw rand);         // 0x31c0e0
Spot middleSpot(const ZoneTerrain &terrain);

// Static objects made fresh (0x2f93a0): scale 1, active, nothing taken.
inline StaticObject makeObject(int32_t type, int64_t x, int64_t y, int64_t z, uint8_t turn) {
    StaticObject o;
    o.type = type, o.x = x, o.y = y, o.z = z, o.turn = turn;
    return o;
}

} // namespace worldgen
