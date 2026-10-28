#pragma once
// Where the clouds are, in both looks (docs/specs/sky-and-clouds.md §A.5 for Alpha, §B.7 for
// Steam). Both games lay a grid of 200-block cells around the camera, stagger every odd column
// by half a cell, and decide from the cell numbers alone whether a cell holds a cloud, how high
// it hangs, how big it is and which way it is turned. Nothing depends on the time, so clouds
// never move: they only appear and disappear as the camera crosses cell boundaries.
//
// Positions here are in the game's block coordinates (z = 0 at sea level); the renderer moves
// them to its own origin. The world is asked about columns through small callbacks, so these
// rules can be tested without a world.
#include <glm/glm.hpp>
#include <cstdint>
#include <functional>
#include <optional>

namespace clouds {

constexpr int32_t CELL_BLOCKS = 200;

// The cell a block coordinate falls in. Both games divide with C's integer division, which
// rounds towards zero rather than down, so cell 0 spans blocks -199..199 and every other cell
// 200 blocks.
int32_t cellOf(double blocks);

// The base point of cell (cx, cy) in blocks: x = 200 cx, y = 200 cy, and odd columns (C's
// remainder, so -1 for odd negative cx) shifted by half a cell in y.
glm::dvec2 cellBase(int32_t cx, int32_t cy);

// One cloud: the centre of its base, how many blocks one voxel of the model spans, and its turn
// about the vertical axis. The model is drawn centred over the base point.
struct Cloud {
    glm::dvec3 base{0};    // game blocks
    float scale = 1;       // blocks per voxel
    float turnDegrees = 0; // about +z
    bool dark = false;     // Steam: cloud-dark.cub instead of cloud02.cub
};

// --- Alpha (§A.5) -----------------------------------------------------------------------------

// The cells reach six either way: a 13 x 13 grid.
constexpr int ALPHA_REACH = 6;

// Alpha's lattice value noise N(u, v) (0x4c0ef0): a hashed value at each integer lattice point,
// blended with cosine weights; about -1..1.
float alphaNoise(double u, double v);

// Clouds are drawn at all only where the player's column is a little moist and not on fire.
bool alphaCloudsShown(float moistureAtPlayer, float fireAtPlayer);

// What the Alpha clouds ask of the world at a block column.
struct AlphaWorld {
    std::function<float(int32_t x, int32_t y)> moisture; // the zone moisture factor Mo (0x5c4800)
    std::function<float(int32_t x, int32_t y)> mountain; // a 0..1 field that rises in mountains (0x5effa0)
    std::function<float(int32_t x, int32_t y)> ground;   // the ground height in blocks (0x5c5e20)
};

// The cloud of cell (cx, cy), or none.
std::optional<Cloud> alphaCloud(int32_t cx, int32_t cy, const AlphaWorld &world);

// --- Steam (§B.7) -----------------------------------------------------------------------------

// The cells reach 2 + trunc(V / 200) either way, V the view distance in blocks.
int steamReach(float viewDistance);

// What the Steam clouds ask of the world at a block column.
struct SteamWorld {
    std::function<float(int32_t x, int32_t y)> dryness; // the region's dryness (0x2b5960)
    std::function<float(int32_t x, int32_t y)> ground;  // the ground height, mode 1 (0x35ea0)
    std::function<float(int32_t x, int32_t y)> forest;  // the forest weight of the place there (0x2d9f30)
};

// The cloud of cell (cx, cy), or none (before the view test, which is the renderer's).
std::optional<Cloud> steamCloud(int32_t cx, int32_t cy, const SteamWorld &world);

// The view test's sphere radius and how far past the view distance a cloud may stand.
constexpr float STEAM_CULL_RADIUS = 250, STEAM_CULL_BEYOND = 200;

// Near the camera Steam clouds fade: the opacity for a cloud `distance` blocks from the camera
// (1 from 180 blocks out, 0 within 81). Below 0.01 the cloud is not drawn.
float steamOpacity(double distance);
constexpr float STEAM_HIDDEN_BELOW = .01f;

} // namespace clouds
