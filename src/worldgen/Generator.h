#pragma once
// The world generator as the engine uses it: one world at a time (its seed, its day, the day
// zones are refreshed for and the creature tables), the zones it makes, kept in a small cache, and
// the questions the engine asks of it about cells, places and regions.
//
// The generator itself is the rest of src/worldgen (docs/specs/worldgen-*.md); CubeVulkan's own
// options (world scale, mod hooks) are in Options.h.
//
//   worldgen::setWorldSeed(123);
//   worldgen::loadCreatureTables(gameDir / "Extract" / "oracle");
//   auto zone = worldgen::zone(zx, zy);            // blocks, objects, models, items, spawn points
//   auto lit = worldgen::litZone(zx, zy);          // its columns lit as the game lights them
#include "worldgen/BuildingRules.h"
#include "worldgen/CellObjects.h"
#include "worldgen/CreatureTables.h"
#include "worldgen/Height.h"
#include "worldgen/Options.h"
#include "worldgen/Planner.h"
#include "worldgen/Residents.h"
#include "worldgen/Structures.h"
#include "worldgen/ZoneCreatures.h"
#include "worldgen/ZoneLight.h"

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace worldgen {

// --- The world -----------------------------------------------------------------------------------

// The world seed (world+0x244 comes from it); 0 by default. Set before generating.
void setWorldSeed(int32_t seed);
const WorldValues &currentWorld();
// The world's day (world+8): some places' encounters depend on it.
void setWorldDay(int32_t day);
int32_t worldDay();
// The day zones are refreshed for when they are made (the daily refresh, 0x31c540: its shoppers
// and travellers' camp); -1 (the default) leaves zones as the generator makes them.
void setRefreshDay(int32_t day);

// The creature tables (Extract/oracle); without them no creatures are generated.
bool loadCreatureTables(const std::filesystem::path &oracleDir, std::string *why = nullptr);
bool creatureTablesLoaded();
const CreatureTables &creatureTables();

// --- Cells and columns of the current world ----------------------------------------------------

int cellObject(int32_t cx, int32_t cy);                     // 0x69c60
CastleCell castleCell(int32_t cx, int32_t cy);              // 0x2a8290
int32_t cellHeight(int32_t cx, int32_t cy, int type = 0);   // 0x2d87f0: the height a cell's building stands at
// A column's values as its zone keeps them (the zone's place, with no reach limit).
ColumnField columnField(int32_t x, int32_t y);

// --- Regions and places, with the mods' hooks -----------------------------------------------------

// A region's plan as the engine reads it: each place's role (with the role hook) and level, the
// special town, whether it marks spawn points, and its items.
struct RegionRoles {
    std::vector<std::array<int32_t, 3>> roles, levels; // place x, y, value
    Int2 special;
    bool marked = false;
    std::vector<int32_t> items;
};
RegionRoles regionRoles(Int2 region);
// One place of a region's plan: its role (0x93480), level (0x12a200) and encounter on `day`
// (0x119a00); 0 (-1 for the encounter) for none.
int plannerRole(Int2 region, int32_t px, int32_t py);
int plannerLevel(Int2 region, int32_t px, int32_t py);
int plannerEncounter(Int2 region, int32_t px, int32_t py, int32_t day);

// A place cell's landmark, role and encounter on the world's day (-1 for none, 100 + role for the
// set pieces of roles 19-23), and the zone of its landmark point: which zones to compare.
struct PlaceEncounter {
    int kind = 0, role = 0, encounter = -1;
    bool big = false;
    int32_t zx = 0, zy = 0;
    Int2 point;
};
PlaceEncounter placeEncounter(int32_t px, int32_t py);

// --- Zones ---------------------------------------------------------------------------------------

// Zone (zx, zy) as the world makes it: generated with its creatures on the world's day, then
// refreshed for the refresh day. Unlit. Kept in a small cache shared by the threads.
std::shared_ptr<const ZoneContents> zone(int32_t zx, int32_t zy);
// Its columns lit as the game lights a loaded zone, reading into its eight neighbours.
std::vector<TerrainColumn> litZone(int32_t zx, int32_t zy);
// Every rand() value generating the zone draws, in order (the refresh's not included).
std::vector<uint16_t> zoneDraws(int32_t zx, int32_t zy);

// A zone's static object with what a reward chest holds (category 0 for none).
struct ZoneObject : StaticObject {
    Item reward;
};
std::vector<ZoneObject> zoneObjects(int32_t zx, int32_t zy);

// Whether the mods' hooks change what zone (zx, zy) is made with: its place or its neighbours',
// its place's role or role item, or a region's boat on it. (A world kept as extracted zones makes
// these again.)
bool hooksChange(int32_t zx, int32_t zy);

// Lets go of the cached zones (after the world or its options change, they are made again anyway).
void forgetZones();

} // namespace worldgen
