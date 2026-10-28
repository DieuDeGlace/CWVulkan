#pragma once
// A creature's class, worn items, abilities and the consumables it carries (0x511e0;
// docs/specs/worldgen-creatures.md §4.3).
#include "worldgen/Spawns.h"

namespace worldgen {

struct GearInput {
    int32_t hostility = 1;
    int32_t race = 0;
    int32_t level = 0;
    uint32_t flags = 0; // the appearance flags
    int32_t people = 0;
};

// Fills `g` (its class byte is the caller's choice on the way in, 0 to pick one). Draws in the
// game's order.
void makeGear(const GearInput &in, Draw rand, Gear &g);


} // namespace worldgen
