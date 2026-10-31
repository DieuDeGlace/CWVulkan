#pragma once
// docs/specs/steam-gameplay-camera.md §2/§7.2: frame coefficients and view-only shake.
// Angles, fixed anchors, obstruction and manual input retain their own contracts.
#include "GameMath.h"
#include "NativeCrt.h"
#include <glm/vec2.hpp>

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
// Native view basis: positive X points left, positive Y points up.
inline glm::vec2 viewShake(float worldClockMs, float magnitude) {
    const float half = magnitude * 0.5f;
    const float y = nativecrt::cosf(worldClockMs * 0.04f) * half;
    const float x = nativecrt::cosf(worldClockMs * 0.03f) * half;
    return {x, y};
}
} // namespace cameramotion
