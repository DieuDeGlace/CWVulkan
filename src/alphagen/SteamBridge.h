#pragma once
// Representation bridge for Alpha terrain snapshots into Steam-sized world consumers.
// This does not run Steam structure placement or select the runtime generator.
#include "alphagen/SurfaceStore.h"
#include "worldgen/ZoneTerrain.h"
#include "worldgen/ZoneRecords.h"
#include "alphagen/PlantModels.h"
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace alphagen {
class ReliefStore;
class LandscapeStore;
// Alpha's packed type/flags byte is not a Steam type byte. Unsupported materials
// fail explicitly until their gameplay/render semantics have a defined bridge.
std::optional<worldgen::Block> steamBlock(Block);
// Hybrid policy: translate solid terrain while keeping liquid levels in world Z.
// Source storage is immutable; invalid height ranges return no column.
std::optional<worldgen::TerrainColumn> shiftedTerrainColumn(const worldgen::TerrainColumn &, int32_t shift);
// Plant XY is already in Steam block coordinates; shift is the composed column's
// solid translation; water lilies keep their fixed world-Z surface. Native
// constructor flags and fixed placement are explicit.
std::optional<worldgen::Model> steamPlant(const Plant &, const PlantModels &, int32_t shift = 0);

class SteamTile {
  public:
    explicit SteamTile(int32_t zx, int32_t zy);
    int32_t zoneX() const { return terrain.zx; }
    int32_t zoneY() const { return terrain.zy; }
    const std::vector<worldgen::TerrainColumn> &columns() const { return terrain.columns; }
    bool covers(int32_t x, int32_t y) const;
    worldgen::Block read(int32_t x, int32_t y, int32_t z) const;

  private:
    friend struct SteamTileBuilder;
    worldgen::ZoneTerrain terrain; // Read-only converted storage, not a prepared Steam structure builder.
    std::array<bool, worldgen::ZONE_COLUMNS> present{};
};
// Uncovered finite-world columns remain absent, not implicit native sea/air.
std::optional<SteamTile> steamTile(SurfaceStore &, int32_t zx, int32_t zy, std::string &error);
std::optional<SteamTile> steamTile(ReliefStore &, int32_t zx, int32_t zy, std::string &error);
std::optional<SteamTile> steamTile(LandscapeStore &, int32_t zx, int32_t zy, std::string &error);
} // namespace alphagen
