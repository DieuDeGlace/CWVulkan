// The game's plant sway (docs/specs/vegetation-wind.md §4). See VegetationWind.h.
#include "VegetationWind.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace vegetationwind {
namespace {

// §4.1: how far the top moves, as a share of the two waves.
constexpr double TOP_REACH = 0.1;     // blocks per unit of a or b
constexpr double PHASE_STEP = 30.0;   // radians added per phase index
// a(t, i) = cos(0.004 t + 30 i + 84) + cos(0.0027 t + 93): the y wave.
constexpr double Y_RATE_OWN = 0.004, Y_OFFSET_OWN = 84.0;
constexpr double Y_RATE_SHARED = 0.0027, Y_OFFSET_SHARED = 93.0;
// b(t, i) = 0.8 (cos(0.005 t + 30 i) + cos(0.003 t)): the x wave.
constexpr double X_RATE_OWN = 0.005, X_RATE_SHARED = 0.003, X_SCALE = 0.8;

// §4.4: Steam's swing from the top, 30 degrees at most, at the y wave's own rate.
constexpr double SWING_DEGREES = 30.0, SWING_RATE = 0.004;
constexpr uint32_t SWING_PHASES = 1000;

// Arguments of the cosines are formed in double precision: the game keeps its clock in a
// float (Steam) or an integer (Alpha) and loses nothing visible in the first hours; an engine may
// keep it exact (§2).
double wave(double rate, double tMs, double offset) { return std::cos(rate * tMs + offset); }

// The per-model phase of the swing (§4.4): a xorshift of the list position, kept under 1000.
uint32_t swingPhase(int listPosition) {
    uint32_t h = (uint32_t)listPosition;
    h ^= h << 13;
    h ^= h >> 17;
    h ^= h << 5;
    return h % SWING_PHASES;
}

} // namespace

int phaseIndex(int listPosition, bool steamLook) {
    return steamLook ? listPosition % STEAM_PHASE_GROUPS : listPosition;
}

glm::vec2 topShift(double tMs, int phase) {
    const double own = PHASE_STEP * phase;
    const double a = wave(Y_RATE_OWN, tMs, own + Y_OFFSET_OWN) + wave(Y_RATE_SHARED, tMs, Y_OFFSET_SHARED);
    const double b = X_SCALE * (wave(X_RATE_OWN, tMs, own) + wave(X_RATE_SHARED, tMs, 0.0));
    return glm::vec2((float)(-TOP_REACH * b), (float)(TOP_REACH * a));
}

float swingDegrees(double tMs, int listPosition) {
    return (float)(SWING_DEGREES * std::cos(SWING_RATE * tMs + swingPhase(listPosition)));
}

glm::mat4 modelMatrix(const Placement &p, double tMs, bool steamLook) {
    // Built right to left, in the order a vertex goes through: the last factor acts first.
    glm::mat4 m = glm::translate(glm::mat4(1), p.position);
    m = glm::scale(m, glm::vec3(p.size));
    const bool sways = (p.flags & modelflag::SWAYS) != 0 && p.heightVoxels > 0 && p.size > 0;
    if (sways) {
        // A shear of x and y by height: at z = H the voxel has moved by the top shift, which is
        // in blocks, so in voxels before the scale it is divided by the size.
        const glm::vec2 top = topShift(tMs, phaseIndex(p.listPosition, steamLook));
        glm::mat4 shear(1);
        shear[2][0] = top.x / (p.heightVoxels * p.size);
        shear[2][1] = top.y / (p.heightVoxels * p.size);
        m = m * shear;
    }
    const bool swings = steamLook && (p.flags & (modelflag::SWINGS_Y | modelflag::SWINGS_X)) != 0 && p.heightVoxels > 0;
    if (swings) {
        // Hung from its top: moved down by H, turned, moved back up.
        const float angle = glm::radians(swingDegrees(tMs, p.listPosition));
        const glm::vec3 axis = (p.flags & modelflag::SWINGS_Y) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        m = glm::translate(m, glm::vec3(0, 0, p.heightVoxels));
        m = glm::rotate(m, angle, axis);
        if ((p.flags & modelflag::SWINGS_Y) && (p.flags & modelflag::SWINGS_X))
            m = glm::rotate(m, angle, glm::vec3(1, 0, 0));
        m = glm::translate(m, glm::vec3(0, 0, -p.heightVoxels));
    }
    return glm::rotate(m, p.yawRadians, glm::vec3(0, 0, 1));
}

} // namespace vegetationwind
