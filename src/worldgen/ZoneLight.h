#pragma once
// The light pass (docs/specs/worldgen-terrain-zone.md §6; the game's 0x2a3260). Not part of the
// generator: the game lights a zone later, reading 8 columns into its neighbours. The result is
// the red byte of the zone's open blocks (air and water): 255 under the open sky, less in
// overhangs and caves, spreading 5 blocks from lit ones.
//
// The spec gives this pass as a summary (its light map's layout is not specified); this follows
// that summary.
#include "worldgen/ZoneTerrain.h"

#include <functional>

namespace worldgen {

// The stored columns of zone (zx, zy), unlit (as zoneTerrain makes them).
using ZoneColumnsSource = std::function<const std::vector<TerrainColumn> &(int32_t zx, int32_t zy)>;

// Zone (zx, zy)'s columns, lit: the red byte of every open block set by the light pass.
std::vector<TerrainColumn> lightZone(int32_t zx, int32_t zy, const ZoneColumnsSource &zones);

} // namespace worldgen
