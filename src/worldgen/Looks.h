#pragma once
// A creature's looks as generation makes them (0x53e80 with no looks record;
// docs/specs/worldgen-creatures.md §4.2 and tables T7): every race draws five numbers (a colour
// and two picks for head and hair), some draw more, and the rest of the appearance is the race's
// fixed data (CreatureTables::baseAppearance). The race's flags are added to the ones already
// set, and the size grows for the flags 0x20000, 0x200 and 0x40000.
#include "worldgen/CreatureTables.h"

namespace worldgen {

// Fills `a` for `race` from its draws, keeping the flags `a` already has (they can grow the
// size). `wild`: the looks of a wild group's creature, which adds flag 0x8000 for some races.
// Returns false for a race the tables lack (it still makes the five common draws).
bool makeLooks(const CreatureTables &tables, int32_t race, bool wild, Draw rand, Appearance &a);

// The size growth of step 4 for the flags given (exposed for tests).
void growSize(float size[3], uint32_t flags);

} // namespace worldgen
