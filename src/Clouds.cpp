#include "Clouds.h"
#include "worldgen/Noise.h"
#include <algorithm>
#include <cmath>

namespace clouds {
namespace {
constexpr double PI = 3.14159265358979323846;

// --- Alpha's value noise (§A.5, 0x4c0ef0) ---
// The hash multiplies and adds in 32-bit unsigned arithmetic, wrapping around.
constexpr uint32_t ROW_STRIDE = 57, HASH_SHIFT = 13;
constexpr uint32_t HASH_SQUARE = 60493, HASH_LINEAR = 19990303, HASH_OFFSET = 3524844813u;
constexpr uint32_t LOW_31_BITS = 0x7fffffffu;
constexpr double HASH_SCALE = 1073741824.0; // 2^30: the hash becomes 1 - hash / 2^30

// The noise's value at lattice point (a, b), about -1..1.
double latticeValue(int32_t a, int32_t b) {
    uint32_t k = (uint32_t)a + ROW_STRIDE * (uint32_t)b;
    k = (k << HASH_SHIFT) ^ k;
    const uint32_t hashed = (k * (k * k * HASH_SQUARE + HASH_LINEAR) + HASH_OFFSET) & LOW_31_BITS;
    return 1.0 - hashed / HASH_SCALE;
}

// The cosine blend between two lattice values for a fraction f of the way.
double cosineBlend(double a, double b, double f) {
    const double w = (1 - std::cos(PI * f)) / 2;
    return a * (1 - w) + b * w;
}

// --- Alpha's constants (§A.5) ---
constexpr float GATE = .01f;                // player moisture above, fire share below
constexpr float COVERAGE = .8f;             // a cell is cloudy while (n + 1) / 2 < 0.8 m1
constexpr float WET_ENOUGH = .3f;           // ... and the moisture at the cloud is at least 0.3
constexpr float MOUNTAIN_RISE = 100;        // blocks added over full mountains
constexpr float ALPHA_HEIGHT = 170, ALPHA_HEIGHT_NOISE = 50;
constexpr float SCALE_BASE = 7, SCALE_NOISE = 2; // blocks per voxel, both looks
constexpr float QUARTER_TURN = 90;          // degrees per (cx + cy)

// --- Steam's constants (§B.7) ---
constexpr float STEAM_HEIGHT = 200, STEAM_HEIGHT_NOISE = 30;
constexpr float DARK_FOREST = .3f; // cloud-dark over places with more forest weight than this
constexpr double DOTS = 65536;     // fixed-point units per block
constexpr double FADE_START = 80, FADE_PER_BLOCK = .01, ALWAYS_OPAQUE = 180;
} // namespace

int32_t cellOf(double blocks) { return (int32_t)std::trunc(blocks) / CELL_BLOCKS; }

glm::dvec2 cellBase(int32_t cx, int32_t cy) {
    return {(double)CELL_BLOCKS * cx, (double)CELL_BLOCKS * cy + (CELL_BLOCKS / 2) * (cx % 2)};
}

float alphaNoise(double u, double v) {
    const int32_t a = (int32_t)std::trunc(u), b = (int32_t)std::trunc(v);
    const double fu = u - a, fv = v - b;
    const double nearRow = cosineBlend(latticeValue(a, b), latticeValue(a + 1, b), fu);
    const double farRow = cosineBlend(latticeValue(a, b + 1), latticeValue(a + 1, b + 1), fu);
    return (float)cosineBlend(nearRow, farRow, fv);
}

bool alphaCloudsShown(float moistureAtPlayer, float fireAtPlayer) {
    return moistureAtPlayer > GATE && fireAtPlayer < GATE;
}

std::optional<Cloud> alphaCloud(int32_t cx, int32_t cy, const AlphaWorld &world) {
    // 1-2: the coverage noise against the moisture, read (as the game does) at the column whose
    // coordinates are the cell numbers themselves.
    const float coverage = (alphaNoise(cx, cy) + 1) * .5f;
    if (!(coverage < COVERAGE * world.moisture(cx, cy)))
        return std::nullopt;
    // 3-4: the staggered base point, which must itself be moist.
    const glm::dvec2 at = cellBase(cx, cy);
    const int32_t x = (int32_t)at.x, y = (int32_t)at.y;
    if (world.moisture(x, y) < WET_ENOUGH)
        return std::nullopt;
    // 5: above the ground (and higher over mountains), never below sea level, then 170..220 up.
    float z = world.ground(x, y) + MOUNTAIN_RISE * world.mountain(x, y);
    z = std::max(z, 0.f);
    z += ALPHA_HEIGHT + ALPHA_HEIGHT_NOISE * alphaNoise(17.0 * cx, 19.0 * cy);
    // 6: 5..9 blocks per voxel, a quarter turn per cell step.
    Cloud cloud;
    cloud.base = {at.x, at.y, z};
    cloud.scale = SCALE_BASE + SCALE_NOISE * alphaNoise(15.0 * cx, 23.0 * cy);
    cloud.turnDegrees = (float)(cx + cy) * QUARTER_TURN;
    return cloud;
}

int steamReach(float viewDistance) { return 2 + (int)std::trunc(viewDistance / CELL_BLOCKS); }

std::optional<Cloud> steamCloud(int32_t cx, int32_t cy, const SteamWorld &world) {
    const glm::dvec2 at = cellBase(cx, cy);
    const int32_t x = (int32_t)at.x, y = (int32_t)at.y;
    // 1: coverage: the generator's simplex noise at the base point against half the dryness.
    const float dryness = world.dryness(x, y);
    const float noise = worldgen::simplex(at.x, at.y);
    if (!((noise + 1) * .5f <= dryness * .5f))
        return std::nullopt;
    // 2: the height in fixed point: the ground (at least sea level) plus 170..230 blocks.
    int64_t ground = (int64_t)std::trunc(world.ground(x, y) * DOTS);
    ground = std::max<int64_t>(ground, 0);
    const float rise = STEAM_HEIGHT + STEAM_HEIGHT_NOISE * worldgen::simplex(17.0 * cx, 19.0 * cy);
    const int64_t z = ground + (int64_t)std::trunc(rise * DOTS);
    // 4-5: dark over forests; size and turn as in Alpha, from the same noise as the height.
    Cloud cloud;
    cloud.base = {at.x, at.y, z / DOTS};
    cloud.scale = SCALE_BASE + SCALE_NOISE * worldgen::simplex(15.0 * cx, 23.0 * cy);
    cloud.turnDegrees = (float)(cx + cy) * QUARTER_TURN;
    cloud.dark = world.forest(x, y) > DARK_FOREST;
    return cloud;
}

float steamOpacity(double distance) {
    if (distance >= ALWAYS_OPAQUE)
        return 1;
    return (float)std::clamp((distance - FADE_START) * FADE_PER_BLOCK, 0.0, 1.0);
}

} // namespace clouds
