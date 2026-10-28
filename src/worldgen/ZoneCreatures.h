#pragma once
// Part C of the zone generator (docs/specs/worldgen-creatures*.md): the region planner's facts,
// the region role's things, the creatures of the cell pass, the marking pass and the daily
// refresh, plugged into part B's generator through its hooks (Structures.h).
//
//   ZoneContents z = generateZoneWithCreatures(world, tables, day, zx, zy);
//   refreshZoneCreatures(z, world, tables, day); // when the world touches the zone on a day
#include "worldgen/CreatureTables.h"
#include "worldgen/Structures.h"

namespace worldgen {

// The region planner's facts for a zone's place (the generator's planner hook).
PlannerFacts plannerFacts(const ZonePlace &place);

// The hooks that make part C on the zone generator's stream, for the world's day (the place
// encounters depend on it). `tables` must outlive the hooks.
StructureHooks creatureHooks(const CreatureTables &tables, int32_t day);
// The daily refresh's part (worldgen-creatures-zone.md §4.2, §4.3).
RefreshHook creatureRefresh(const CreatureTables &tables);

// A whole zone with its creatures.
ZoneContents generateZoneWithCreatures(const WorldValues &world, const CreatureTables &tables, int32_t day, int32_t zx, int32_t zy,
                                       bool recordDraws = false);
// The daily refresh, once a day.
void refreshZoneCreatures(ZoneContents &zone, const WorldValues &world, const CreatureTables &tables, int32_t day);

// The region role's things (0x11a820; worldgen-creatures-roles.md), RoleThings.cpp.
class CreatureBuilder;
void roleThings(CreatureBuilder &b, int role);

} // namespace worldgen
