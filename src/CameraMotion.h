#pragma once
// docs/specs/steam-gameplay-camera.md: frame coefficients, unshaken basis and view shake.
// Fixed anchors, obstruction and manual-input producers retain their own contracts.
#include "GameMath.h"
#include "NativeCrt.h"
#include <glm/vec2.hpp>
#include <glm/mat3x3.hpp>

namespace cameramotion {
inline float angleRate(int smoothness) {
    return nativecrt::powf(0.005f, float(smoothness) * 0.01f);
}
inline float distance(float current, float requested, double ms) {
    return current + (requested - current) * gamemath::blendFactor(gamemath::frameMilliseconds(ms), 0.004f);
}
inline float decayShake(float current, double ms) {
    return current + (0.0f - current) * gamemath::blendFactor(gamemath::frameMilliseconds(ms), 0.005f);
}
// Camera-to-world columns A/B/C, T*Rz(yaw)*Ry(roll)*Rx(pitch), before shake.
// Narrow each displayed double angle before multiplying by the native float constant.
inline glm::mat3 basis(double pitchDegrees, double rollDegrees, double yawDegrees) {
    constexpr float RADIANS_PER_DEGREE = 0.01745329238474369f;
    const float p = float(pitchDegrees) * RADIANS_PER_DEGREE;
    const float r = float(rollDegrees) * RADIANS_PER_DEGREE;
    const float y = float(yawDegrees) * RADIANS_PER_DEGREE;
    const float cp = nativecrt::cosf(p), sp = nativecrt::sinf(p);
    const float cr = nativecrt::cosf(r), sr = nativecrt::sinf(r);
    const float cy = nativecrt::cosf(y), sy = nativecrt::sinf(y);
    return {{cy * cr, sy * cr, -sr},
            {(cy * sr) * sp - sy * cp, (sy * sr) * sp + cy * cp, cr * sp},
            {(cy * sr) * cp + sy * sp, (sy * sr) * cp - cy * sp, cr * cp}};
}
// Unshaken native world-to-view rotation. Negate the stored double angles
// before narrowing, and append inverse rotations X, Y, Z in that order.
inline glm::mat3 viewRotation(double pitchDegrees, double rollDegrees, double yawDegrees) {
    constexpr float RADIANS_PER_DEGREE = 0.01745329238474369f;
    const float p = float(-pitchDegrees) * RADIANS_PER_DEGREE;
    const float r = float(-rollDegrees) * RADIANS_PER_DEGREE;
    const float y = float(-yawDegrees) * RADIANS_PER_DEGREE;
    const float cp = nativecrt::cosf(p), sp = nativecrt::sinf(p);
    const float cr = nativecrt::cosf(r), sr = nativecrt::sinf(r);
    const float cy = nativecrt::cosf(y), sy = nativecrt::sinf(y);
    const glm::mat3 x{{1, 0, 0}, {0, cp, sp}, {0, -sp, cp}};
    const glm::mat3 tilt{{cr, 0, -sr}, {0, 1, 0}, {sr, 0, cr}};
    const glm::mat3 z{{cy, sy, 0}, {-sy, cy, 0}, {0, 0, 1}};
    return (x * tilt) * z;
}
// Native view basis: positive X points left, positive Y points up.
inline glm::vec2 viewShake(float worldClockMs, float magnitude) {
    const float half = magnitude * 0.5f;
    const float y = nativecrt::cosf(worldClockMs * 0.04f) * half;
    const float x = nativecrt::cosf(worldClockMs * 0.03f) * half;
    return {x, y};
}
} // namespace cameramotion
