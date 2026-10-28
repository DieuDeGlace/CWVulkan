#pragma once
// A zone, made whole (docs/specs/worldgen-structures.md §2): part A's terrain, then part B's
// structures and contents in the game's twelve steps on one rand() stream: the cell objects,
// street lamps, spots, the cells' chests and crates, plants, the region role's items, the cells'
// items, the copies, the lists and the marking of object blocks.
//
// Part C (worldgen-creatures*.md) owns the creatures, spawn points and the region planner. Where
// its passes draw from the same stream, the generator calls the hooks below at exactly those
// points; an unset hook draws nothing, so the stream then differs from the game's from the first
// creature draw on.
#include "worldgen/BuildingRules.h"
#include "worldgen/ZoneBuild.h"
#include "worldgen/ZoneContents.h"

#include <functional>

namespace worldgen {

struct StructureHooks {
    // The region planner's facts for the zone's place (part C's 0x115df0, 0x93480, 0x12a600,
    // 0x131540). Unset: role 0, no items, no treasure.
    std::function<PlannerFacts(const ZonePlace &)> planner;
    // The arena master on his dais (cell object 97), right after the dais, on the cell's stream
    // (worldgen-creatures-zone.md §5): `base` is the dais's base height.
    std::function<void(ZoneBuild &, int32_t cx, int32_t cy, int32_t base)> arenaMaster;
    // Step 8, the region role's things for roles other than 6-13 (0x11a820's part C: roles 19-23
    // and the encounters, with their campfires, chests and braziers). C draws from the stream where
    // the plants left it, until it reseeds.
    std::function<void(ZoneBuild &, int role)> roleThings;
    // Step 9, inside the cell pass (contents file §C5, worldgen-creatures-zone.md §2).
    CellPassHooks cells;
    // Step 10, the marked-region pass (0x318e28): one rand() % 50 per eligible spawn point.
    std::function<void(ZoneBuild &)> markedRegion;
};

struct StructureOptions {
    StructureHooks hooks;
    bool recordDraws = false; // keep every rand() value drawn, in order
};

// Everything a zone holds after the generator.
struct ZoneContents {
    ZoneTerrain terrain; // the blocks (and the column values, the draws)
    ZonePlace place;
    PlannerFacts planner;
    std::vector<StaticObject> objects; // +0x60
    std::vector<Model> models;         // +0x18
    std::vector<Model> modelLights;    // +0x90: the models that give light, as the plant pass ends (C2.1)
    std::vector<Model> emitters;       // +0xa8: the models that sound (C2.1)
    std::vector<GroundItem> items;     // +0x30
    std::map<int, Item> rewards;       // +0x450: a reward chest's item by its index
    SpotMap spots;                     // +0x100
    SpotMap working;                   // +0x110, as the generator leaves it
    ZoneLists lists;                   // +0x78, +0x48, +0xc0, +0x4b8, +0xe8
    ZoneSpawns spawns;                 // +0x460 ..: part C's spawn points and lists
};

// Zone (zx, zy) of the world given.
ZoneContents generateZone(const WorldValues &world, int32_t zx, int32_t zy, const StructureOptions &options = {});

// The daily refresh (0x31c540; contents file §C9), when the world's day differs from the zone's:
// the last refresh's spawn points are removed and the objects come back from the generator's copy,
// then part C's refresh (its seed, shoppers and travellers) runs on the zone; it places the camp's
// objects with campObjects (ZoneContents.h). Unset, only the undoing is done.
using RefreshHook = std::function<void(ZoneBuild &, int day)>;
void refreshZone(ZoneContents &zone, const WorldValues &world, int day, const RefreshHook &partC = {});

// The zone's place (§5).
ZonePlace zonePlace(const ZoneTerrain &terrain);

// The cell objects' cache of the calling thread for a world.
CellObjects &cellObjectsFor(const WorldValues &world);

} // namespace worldgen
