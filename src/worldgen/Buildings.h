#pragma once
// Buildings, castles and set pieces (docs/specs/worldgen-structures-buildings.md): the building
// builder (§2-§8) with its rooms (§6), the wizard's tower (§11), the other set pieces (§12), crop
// fields and street lamps (§13).
#include "worldgen/BuildingRules.h"
#include "worldgen/Mason.h"
#include "worldgen/ZoneBuild.h"

namespace worldgen {

// The building builder (§2-§8) for cell (cx, cy) at (i, j) from the zone's first cell, of type t
// with castle cell k.
void buildBuilding(ZoneBuild &zone, BuildingRules &rules, int i, int j, int32_t cx, int32_t cy, int t, CastleCell k);

// Set pieces (§11, §12).
void wizardTower(ZoneBuild &zone, BuildingRules &rules, int32_t cx, int32_t cy);   // 52
// The arena master's dais (97). The master himself is part C's (StructureHooks::arenaMaster, right after it).
void arenaDais(ZoneBuild &zone, BuildingRules &rules, int32_t cx, int32_t cy);     // 97
void waterTower(ZoneBuild &zone, BuildingRules &rules, int32_t cx, int32_t cy);    // 66
void cistern(ZoneBuild &zone, BuildingRules &rules, int32_t cx, int32_t cy);       // 68
void aqueduct(ZoneBuild &zone, BuildingRules &rules, int32_t cx, int32_t cy);      // 67
void towerPlatform(ZoneBuild &zone, BuildingRules &rules, int32_t cx, int32_t cy, int t); // 69, 70

// Crop fields (§13; types 27-34, inside the zone) and street lamps (§13, step 4 of the generator).
void cropField(ZoneBuild &zone, BuildingRules &rules, int32_t cx, int32_t cy, int t);
void streetLamps(ZoneBuild &zone, BuildingRules &rules);

} // namespace worldgen
