#pragma once
// The colours and types of the terrain's blocks (docs/specs/worldgen-terrain-zone.md §4):
// grass, ground, sand, rock, snow, lava, poison, river beds, and the blend to rock on steep
// sides.
#include "worldgen/Height.h"
#include "worldgen/Places.h"
#include "worldgen/Regions.h"

#include <cstdint>

namespace worldgen {

// Block types the terrain makes.
namespace blocktype {
constexpr uint8_t AIR = 0;
constexpr uint8_t SOLID = 1; // what lies outside a zone or below a column
constexpr uint8_t WATER = 2;
constexpr uint8_t WET_GROUND = 3;
constexpr uint8_t GRASS = 4;
constexpr uint8_t GROUND = 5;
constexpr uint8_t ROCK = 6;
constexpr uint8_t SNOW = 10;
constexpr uint8_t ROAD = 13;
constexpr uint8_t LAVA_GROUND = 14;
constexpr uint8_t LAVA = 17;
constexpr uint8_t CLIFF_FACE = 18;
constexpr uint8_t POISON = 19;
constexpr uint8_t LIGHT = 20;
} // namespace blocktype

// A block as a zone stores it: colour bytes (half the working colour), glow, type, flags.
struct Block {
    uint8_t r = 0, g = 0, b = 0, glow = 0, type = 0, flags = 0;
    bool operator==(const Block &o) const {
        return r == o.r && g == o.g && b == o.b && glow == o.glow && type == o.type && flags == o.flags;
    }
    bool operator!=(const Block &o) const { return !(*this == o); }
};
// "Open" blocks (air and water) let light through.
inline bool isOpen(const Block &b) { return (b.type & 0xfd) == 0; }

// A block property as the generator works with it: float colours (0..510 for a full byte).
struct BlockProps {
    float r = 0, g = 0, b = 0;
    uint8_t glow = 0, type = 0, flags = 0;
};
// Stored (0xba820): each colour halved, clamped to 0..255 and truncated.
Block toBlock(const BlockProps &p);

// The fixed blocks (tables §7).
constexpr Block AIR_BLOCK = {255, 255, 255, 0, blocktype::AIR, 0};
constexpr Block SEA_WATER = {255, 255, 255, 0, blocktype::WATER, 0};
constexpr Block SOLID_GREY = {200, 200, 200, 0, blocktype::SOLID, 0};
constexpr Block EMPTY_BLOCK = {0, 0, 0, 0, 0, 0};

// The values the colours of a column depend on.
struct ColourInputs {
    float highlands = 0, warmth = 0, dryness = 0, vegetation = 0, roads = 0;
    float lavaRegion = 0, forestRegion = 0, kind27 = 0, lava = 0, swamp = 0;
};

// Warmth (0x2d9b60): 0..1, up near warm biomes, craters and volcanoes, down near cold ones.
float warmth(const Neighbourhood &n, int kind, float weight);
// The weight of a kind-27 place (0x2abb60), 0 for other kinds.
inline float kind27Weight(int kind, float weight) { return kind == 27 ? weight : 0.0f; }
// A column's colour inputs from its pipeline values (warmth and the kind-27 weight are made
// from its regions and place).
ColourInputs colourInputs(const ColumnValues &c);

// The world's sand-colour noise offsets (world+0x330, +0x334); set once per world.
struct SandOffsets {
    double x = 0, y = 0;
};

BlockProps groundColour(int32_t x, int32_t y, int32_t z, const ColourInputs &c, const SandOffsets &sand); // 0x2ab410
BlockProps rockColour(int32_t x, int32_t y, int32_t z, float warmth, float lavaRegion, float forestRegion); // 0x2d3150
float sandShade(int32_t x, int32_t y, const SandOffsets &sand); // the sand() noise of 0x2ab410
// The colour under the surface (0x2a3140): rock in highlands, else ground.
BlockProps buriedColour(int32_t x, int32_t y, int32_t z, const ColourInputs &c, const SandOffsets &sand);
// The colour of the surface (0x2a3e30): grass tinted by dryness, forest, kind 27 and lava.
BlockProps surfaceColour(int32_t x, int32_t y, int32_t z, const ColourInputs &c);

// The four neighbours' heights (west x-1, east x+1, south y-1, north y+1).
struct NeighbourHeights {
    float west = 0, east = 0, south = 0, north = 0;
};
// The block at height z of a column of height h (0x2d8e00).
BlockProps blockAt(int32_t x, int32_t y, int32_t z, float h, const NeighbourHeights &nb, const ColourInputs &c,
                   const SandOffsets &sand);

// Whether snow lies on a column (0x2b4320), and the snow block (0x2d3d90).
bool snowAt(int32_t x, int32_t y, float warmth, const NeighbourHeights &nb);
constexpr BlockProps SNOW_BLOCK = {200, 280, 400, 0, blocktype::SNOW, 1};

// A river bed's colour, from scratch (0xbd110).
BlockProps riverBedColour(int32_t x, int32_t y, int32_t z, const SandOffsets &sand);
// The water level of a river (0x2e6420): the base height, raised toward a place's own ground.
float waterLevel(float base, const Place &p);

} // namespace worldgen
