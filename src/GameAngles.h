#pragma once
// Shared finite0x17840 contract, steam-creature-drawn-interpolation.md §6.
#include "NativeCrt.h"
#include <algorithm>

namespace gameangles {
inline float turn(float oldDegrees, float targetDegrees, float factor) {
    constexpr double PI = 3.141592653589793;
    const float oldRadians = float(double(oldDegrees / 180.0f) * PI);
    const float targetRadians = float(double(targetDegrees / 180.0f) * PI);
    const double oldCos = nativecrt::cosf(oldRadians), oldSin = nativecrt::sinf(oldRadians);
    const double targetCos = nativecrt::cosf(targetRadians), targetSin = nativecrt::sinf(targetRadians);
    const double dot = targetCos * oldCos + targetSin * oldSin;
    if (dot < double(-.9f))
        return targetDegrees;
    const float cross = float(targetSin * oldCos - targetCos * oldSin);
    const float sine = std::max(-1.0f, std::min(1.0f, cross));
    const double angle = nativecrt::asinf(sine);
    const double gain = double(factor) * 180.0 / PI;
    return oldDegrees + float(angle * gain);
}
} // namespace gameangles
