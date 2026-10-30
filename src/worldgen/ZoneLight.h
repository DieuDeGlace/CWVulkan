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
#include <array>

namespace worldgen {

// The stored columns of zone (zx, zy), unlit (as zoneTerrain makes them).
using ZoneColumnsSource = std::function<const std::vector<TerrainColumn> &(int32_t zx, int32_t zy)>;

// Zone (zx, zy)'s columns, lit: the red byte of every open block set by the light pass.
std::vector<TerrainColumn> lightZone(int32_t zx, int32_t zy, const ZoneColumnsSource &zones);

// Independent retained result of the completed80x80 mesh-light pass. It owns its
// bytes, so the source terrain snapshots may be released after solving.
struct MeshLightVolume {
    static constexpr int SIZE = 80;
    struct Column {
        int32_t base = 0;
        std::vector<uint8_t> values;
    };
    int64_t x0 = 0, y0 = 0;
    std::vector<Column> columns; // X-major, thenY
    uint8_t at(int64_t x, int64_t y, int64_t z) const;
    uint8_t fixed(const std::array<int64_t, 3> &position) const;
};
MeshLightVolume meshZoneLight(int32_t zx, int32_t zy, const ZoneColumnsSource &zones);

// Sample the completed mesh light volume at fixed model positions, including
// solid entries which cannot be recovered from terrain air-light bytes.
std::vector<uint8_t> sampleZoneLight(int32_t zx, int32_t zy, const ZoneColumnsSource &zones,
                                    const std::vector<std::array<int64_t, 3>> &positions);

} // namespace worldgen
