#pragma once
#include "ObjectVisual.h"
// Procedural terrain built around the ten region types Steam Cube World's own
// biome function reports (see reference/README.md), with colours measured from
// vanilla reference shots (Palette.h). Relief uses TerrainVariance's rules, or
// Alpha-style mountains. Rivers carry WaterLilies.
//
// Everything is a pure function of the seed and world coordinates, so any zone
// can be generated on any thread in any order and borders always agree.
#include "Mesher.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

class Config;

constexpr int ZONE = 64;          // Blocks per zone side (as in Cube World).
constexpr int WORLD_HEIGHT = 320; // Blocks per column.

enum Block : uint8_t {
    AIR,
    GRASS,
    DIRT,
    STONE,
    SAND,
    SNOW,
    WOOD,
    LEAVES1, // Crown shading from the underside (1) to the top (4).
    LEAVES2,
    LEAVES3,
    LEAVES4,
    WATER,
    CACTUS,
    ACCENT, // Region accent layer (mountain sand bands).
};

// The game's region types, in the order the scout listed them. Comments give
// the game's RegionType value.
enum Biome : uint8_t {
    PLAINS,    // 0
    OCEAN,     // 1
    HILLS,     // 2
    WOODLANDS, // 3
    DARKWOODS, // 5
    MOUNTAINS, // 7
    WETLANDS,  // 8
    DESERT,    // 9
    SNOWLANDS, // 11
    DEADLANDS, // 12
    BIOMES
};
const char *biomeName(int biome);

// Sky and fog of a region at full daylight (colours before vanilla's grade, 0..1).
struct RegionSky {
    float sky[3], fog[3];
    float daylight; // Dark regions dim the sun.
};
const RegionSky &regionSky(int biome);
// Water colour of a region (block colour, 0..255).
glm::vec3 regionWater(int biome);

struct TerrainSettings {
    uint64_t seed = 0x5d93ab0d;
    // "alpha": tall ridged mountains, deep valleys and cliffs, close to Alpha's
    // range. "variance": exactly the TerrainVariance mod's relief.
    bool alphaStyle = true;
    // TerrainVariance ([Terrain] in its ini): Amplitude, Scale, MountainAmplitude.
    double amplitude = 68, scale = 768, mountainAmplitude = 80;
    double alphaMountainHeight = 150;
    // Region size in blocks. Larger regions mean longer stretches of one landscape.
    double regionSize = 2048;
    int seaLevel = 72;
    bool rivers = true;
    double riverScale = 1500, riverWidth = 7;
    // WaterLilies ([WaterLilies] in its ini).
    bool lilies = true;
    double lilyPatchDensity = .75, lilyFlowerPercent = 25;
    int lilyMaxPadsPerZone = 28, lilyMaximumDepth = 10;
    double sceneryDensity = 1;

    void read(const Config &config);
};

// Analytic column description (without trees).
struct ColumnInfo {
    float height;      // Top of the highest solid block.
    int ground;        // Blocks [0, ground) are solid.
    int water;         // Water surface (sea level) if ground < it, else 0.
    float river;       // 0..1, how central in a river channel.
    bool freshwater;   // Inland water (rivers, ponds, lakes), not ocean.
    uint8_t biome;     // Nearest region's type.
    float weights[BIOMES]; // Blended region influence (sums to 1).
    float spire;       // 0..1 inside a rock spire.
};

struct Scenery {
    std::string model; // Sprite name without extension.
    float x, y, z, yaw, scale;
    uint8_t flags = 0;
    bool estimatedScale = false; // extracted object lacks a native size measurement
    // A zone model's record flags (+0x38; vegetationwind::modelflag: 1 shines, 4 sways) and its
    // 1-based place in the zone's model list, which phases its sway (vegetation-wind.md §3, §4).
    int32_t modelFlags = 0;
    int listPosition = 0;
    // The Alpha look: the light where it stands (0..1, alpha-rendering.md 9.2), worked out with
    // the zone; below 0 when not known.
    float light = -1;
    objectvisual::State object; // type<0 for ordinary zone models.
};

struct ZoneData {
    int zx = 0, zy = 0;
    // Zone blocks plus a one-block border (66 x 66 x WORLD_HEIGHT).
    static constexpr int P = ZONE + 2;
    std::vector<uint8_t> blocks;
    std::array<uint8_t, P * P> biome{};
    std::array<uint8_t, P * P> shade{};
    std::array<std::array<uint8_t, 3>, P * P> ground{}; // Blended top colour (grass/snow/sand).
    int maxZ = 0;
    std::vector<Scenery> scenery;
    uint8_t at(int lx, int ly, int z) const {
        if (z < 0)
            return STONE;
        if (z >= WORLD_HEIGHT)
            return AIR;
        return blocks[((size_t)(ly + 1) * P + (lx + 1)) * WORLD_HEIGHT + z];
    }
};

class Terrain {
  public:
    explicit Terrain(TerrainSettings s = {}) : settings(s) {}
    const TerrainSettings &config() const { return settings; }
    ColumnInfo column(double x, double y) const;
    float height(double x, double y) const { return column(x, y).height; }
    ZoneData generate(int zx, int zy) const;
    void mesh(const ZoneData &zone, Mesh &opaque, Mesh &water) const;

  private:
    TerrainSettings settings;
    void regionWeights(double x, double y, float weights[BIOMES], uint8_t &nearest) const;
    void plantTrees(ZoneData &zone, const std::vector<ColumnInfo> &columns, int pad) const;
    void placeScenery(ZoneData &zone, const std::vector<ColumnInfo> &columns, int pad) const;
    VoxelColor blockColor(const ZoneData &zone, int x, int y, int z) const;
};
