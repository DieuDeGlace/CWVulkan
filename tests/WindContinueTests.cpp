// Checks of the game's plant sway (src/VegetationWind.*, docs/specs/vegetation-wind.md) and of
// where Continue and Select put the player (frontend-title.md §6.1: the record rebuilt from the
// save, at the saved position, with no velocity, ability or eagle flight left over).
#include "Extracted.h"
#include "Frontend.h"
#include "NativeProgress.h"
#include "Spawned.h"
#include "VegetationWind.h"
#include "worldgen/Residents.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
int failures = 0;
#define WIND_CHECK(...)                                                                            \
    do {                                                                                           \
        if (!(__VA_ARGS__)) {                                                                      \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #__VA_ARGS__);                    \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

bool near(double a, double b, double tolerance = 1e-4) { return std::abs(a - b) <= tolerance; }

using namespace vegetationwind;

// §4.1: the top's shift, by the formula, at a few times and phases.
void topShiftRule() {
    for (double t : {0.0, 1234.0, 98765.0})
        for (int i : {0, 1, 7, 11}) {
            const double a = std::cos(0.004 * t + 30 * i + 84) + std::cos(0.0027 * t + 93);
            const double b = 0.8 * (std::cos(0.005 * t + 30 * i) + std::cos(0.003 * t));
            const glm::vec2 s = topShift(t, i);
            WIND_CHECK(near(s.x, -0.1 * b) && near(s.y, 0.1 * a));
        }
    // At t = 0 and phase 0 the x wave is at its peak: 0.8 x 2 x 0.1 = 0.16 block.
    WIND_CHECK(near(topShift(0, 0).x, -0.16));
    // Never more than 0.2 block along y, 0.16 along x.
    for (int k = 0; k < 2000; k++) {
        const glm::vec2 s = topShift(k * 37.0, k % 12);
        WIND_CHECK(std::abs(s.y) <= 0.2f + 1e-6f && std::abs(s.x) <= 0.16f + 1e-6f);
    }
    // Steam shares 12 phases by list position, Alpha uses the position itself.
    WIND_CHECK(phaseIndex(13, true) == 1 && phaseIndex(13, false) == 13 && phaseIndex(12, true) == 0);
}

// §4: the base stays put, the top moves by the shift whatever the size, linearly with height;
// flowers (no flag 4) stand still; the yaw turns the model before the sway.
void modelMatrixRule() {
    Placement grass;
    grass.position = {100, 200, 50};
    grass.yawRadians = 1.0f;
    grass.size = 0.0625f;
    grass.heightVoxels = 16;
    grass.flags = modelflag::SWAYS | 2;
    grass.listPosition = 5;
    const double t = 4321;
    const glm::mat4 m = modelMatrix(grass, t, false);
    const glm::vec3 base = glm::vec3(m * glm::vec4(0, 0, 0, 1));
    WIND_CHECK(near(base.x, 100) && near(base.y, 200) && near(base.z, 50));
    const glm::vec2 shift = topShift(t, 5);
    const glm::vec3 top = glm::vec3(m * glm::vec4(0, 0, 16, 1));
    WIND_CHECK(near(top.x, 100 + shift.x) && near(top.y, 200 + shift.y) && near(top.z, 51));
    const glm::vec3 middle = glm::vec3(m * glm::vec4(0, 0, 8, 1));
    WIND_CHECK(near(middle.x, 100 + shift.x / 2) && near(middle.y, 200 + shift.y / 2));
    // A voxel off the axis is turned by the yaw, then sheared like the axis.
    const glm::vec3 side = glm::vec3(m * glm::vec4(4, 0, 16, 1));
    WIND_CHECK(near(side.x, 100 + 0.25 * std::cos(1.0) + shift.x) && near(side.y, 200 + 0.25 * std::sin(1.0) + shift.y));
    // The same top shift for a plant four times the size: the reach is in blocks.
    Placement big = grass;
    big.size = 0.25f;
    const glm::vec3 bigTop = glm::vec3(modelMatrix(big, t, false) * glm::vec4(0, 0, 16, 1));
    WIND_CHECK(near(bigTop.x, 100 + shift.x) && near(bigTop.y, 200 + shift.y) && near(bigTop.z, 54));
    // A second later the top is elsewhere (the capture's check, by the rule).
    const glm::vec3 later = glm::vec3(modelMatrix(grass, t + 1000, false) * glm::vec4(0, 0, 16, 1));
    WIND_CHECK(glm::length(later - top) > 0.01f);
    // Flowers (flags 2) never move.
    Placement flowers = grass;
    flowers.flags = 2;
    const glm::vec3 still = glm::vec3(modelMatrix(flowers, t, false) * glm::vec4(0, 0, 16, 1));
    const glm::vec3 still2 = glm::vec3(modelMatrix(flowers, t + 777, true) * glm::vec4(0, 0, 16, 1));
    WIND_CHECK(near(still.x, 100) && near(still.y, 200) && still == still2);
    // Steam's swing (flag 0x10) turns about y from the top: the top stays, the base swings.
    Placement lantern = flowers;
    lantern.flags = modelflag::SWINGS_Y;
    const glm::mat4 hung = modelMatrix(lantern, t, true);
    const glm::vec3 hook = glm::vec3(hung * glm::vec4(0, 0, 16, 1)), foot = glm::vec3(hung * glm::vec4(0, 0, 0, 1));
    const double angle = glm::radians((double)swingDegrees(t, 5));
    WIND_CHECK(near(hook.x, 100) && near(hook.z, 51) && near(foot.x, 100 - std::sin(angle), 1e-3) && near(foot.y, 200));
    WIND_CHECK(std::abs(swingDegrees(t, 5)) <= 30.0f);
    // The Alpha look has no swing.
    const glm::vec3 alphaFoot = glm::vec3(modelMatrix(lantern, t, false) * glm::vec4(0, 0, 0, 1));
    WIND_CHECK(near(alphaFoot.x, 100) && near(alphaFoot.z, 50));
}

// §6.1: Continue rebuilds the player from the save. The flyover left it high in the air,
// moving, on the eagle; after Continue it stands where it was saved, still, with no ability and
// no eagle flight, facing yaw 0.
void continuePlacement() {
    constexpr size_t POSITION = worldgen::creature::POSITION, YAW = worldgen::creature::YAW, VELOCITY = 0x34, ABILITY = 0x68, ABILITY_TIME = 0x6c;
    constexpr size_t FLIGHT_FIELDS = 0x1b0, FLIGHT_FIELDS_END = 0x1c8;
    ExtractedWorld ew;
    SpawnedCreatures world;
    world.setNativePlayer(true);
    // The flyover: placed, then carried 300 blocks up with the eagle's speed.
    world.setPlayer(ew, {10, 20, 140}, 2.0f, glm::vec3(0), 100, 5, false);
    uint8_t *r = world.localPlayer()->record;
    const int64_t high[3] = {50 * 65536, 60 * 65536, 400 * 65536};
    std::memcpy(r + POSITION, high, sizeof high);
    const float moving[6] = {10, 5, 0, 10, 5, -30};
    std::memcpy(r + VELOCITY, moving, sizeof moving);
    r[ABILITY] = 0x9d;
    const float abilityTime = 5000;
    std::memcpy(r + ABILITY_TIME, &abilityTime, 4);
    std::memset(r + FLIGHT_FIELDS, 0x5a, FLIGHT_FIELDS_END - FLIGHT_FIELDS); // (the title rewrote them every frame)

    // The save: a character standing at block (1000.5, -2000.25, 70) of the game's world.
    nativesaves::Character saved = frontend::newCharacter({0, 0}, {0, 0, 0});
    saved.position = {(int64_t)(1000.5 * 65536), (int64_t)(-2000.25 * 65536), (int64_t)(70 * 65536)};
    const nativeprogress::Frame frame{0, 0};
    const glm::vec3 feet = frontend::enteredFeet(saved, frame, glm::vec3(5, 5, 5));
    WIND_CHECK(near(feet.x, 1000.5) && near(feet.y, -2000.25) && near(feet.z, 70 + ExtractedWorld::Z_OFFSET - SpawnedCreatures::NATIVE_FEET));
    // Never placed: the world's home stands in.
    const nativesaves::Character fresh = frontend::newCharacter({0, 0}, {0, 0, 0});
    WIND_CHECK(frontend::enteredFeet(fresh, frame, glm::vec3(5, 6, 7)) == glm::vec3(5, 6, 7));

    world.resetPlayer();
    world.setPlayer(ew, feet, 0, glm::vec3(0), 100, 5, false);
    r = world.localPlayer()->record;
    int64_t at[3];
    std::memcpy(at, r + POSITION, sizeof at);
    WIND_CHECK(at[0] == saved.position[0] && at[1] == saved.position[1] && at[2] == saved.position[2]);
    float velocity[6], yaw, time;
    std::memcpy(velocity, r + VELOCITY, sizeof velocity);
    std::memcpy(&yaw, r + YAW, 4);
    std::memcpy(&time, r + ABILITY_TIME, 4);
    WIND_CHECK(velocity[0] == 0 && velocity[1] == 0 && velocity[2] == 0 && velocity[3] == 0 && velocity[4] == 0 && velocity[5] == 0);
    WIND_CHECK(r[ABILITY] == 0 && time == 0 && yaw == 0);
    // The eagle's flight fields are a new creature's again.
    SpawnedCreatures made;
    made.setNativePlayer(true);
    made.setPlayer(ew, feet, 0, glm::vec3(0), 100, 5, false);
    WIND_CHECK(std::memcmp(r + FLIGHT_FIELDS, made.localPlayer()->record + FLIGHT_FIELDS, FLIGHT_FIELDS_END - FLIGHT_FIELDS) == 0);
    // The game's own frame then leaves it there: the frame's setPlayer at the same feet does
    // not move it.
    world.setPlayer(ew, feet, 1.0f, glm::vec3(3, 0, 0), 100, 5, false);
    std::memcpy(at, world.localPlayer()->record + POSITION, sizeof at);
    WIND_CHECK(at[2] == saved.position[2]);
}
// Imported survey health must not exceed native health after level/gear are set.
// Repeated frame placement must neither heal nor overwrite an existing wound.
void startingHealth() {
    ExtractedWorld blocks;
    SpawnedCreatures player;
    player.setNativePlayer(true);
    player.setPlayer(blocks, {10, 20, 140}, 0, glm::vec3(0), 10000, 1, false);
    const float maximum = player.playerMaxHealth();
    WIND_CHECK(maximum > 0 && player.playerHealth() == maximum);
    player.setPlayerHealth(maximum * .4f);
    player.setPlayer(blocks, {10, 20, 140}, 0, glm::vec3(0), 10000, 1, false);
    WIND_CHECK(near(player.playerHealth(), maximum * .4f));
}
} // namespace

int windContinueTests() {
    topShiftRule();
    modelMatrixRule();
    continuePlacement();
    startingHealth();
    return failures;
}
