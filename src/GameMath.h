#pragma once
// Small numeric rules the game shares between its systems.
#include <algorithm>
#include <cstdint>
#include <cstring>

namespace gamemath {

// Ordinary Game frame, steam-scene-lighting §5.1: retain both double operations.
inline double millisecondsFromNanoseconds(int64_t elapsedNanoseconds) {
    return (double(elapsedNanoseconds) / 1000000000.0) * 1000.0;
}
inline double frameMilliseconds(double elapsedMs) {
    return std::min(elapsedMs, 1000.0);
}

// Fixed-coordinate arithmetic preserves native low64-bit operations before conversion.
inline int64_t addFixed(int64_t a, int64_t b) {
    const uint64_t bits = uint64_t(a) + uint64_t(b);
    int64_t result;
    std::memcpy(&result, &bits, sizeof result);
    return result;
}
inline int64_t subtractFixed(int64_t a, int64_t b) {
    const uint64_t bits = uint64_t(a) - uint64_t(b);
    int64_t result;
    std::memcpy(&result, &bits, sizeof result);
    return result;
}
inline float fixedBlocksBetween(int64_t a, int64_t b) {
    return float(subtractFixed(a, b)) * (1.0f / 65536.0f);
}
inline float fixedDistanceSquared(const int64_t a[3], const int64_t b[3]) {
    float result = 0;
    for (int axis = 0; axis < 3; ++axis) {
        const float delta = fixedBlocksBetween(a[axis], b[axis]);
        result += delta * delta;
    }
    return result;
}

// the frame blend's factor (0xa7a0): how far a value moves toward its target over a frame of
// `ms` milliseconds at `rate`. The frame is cut into hundredths of a millisecond, each moving
// the value toward that step's share of the frame by rate * 0.02.
inline float blendFactor(double ms, float rate) {
    const int steps = (int)(ms * 100.0);
    double value = 0.0;
    for (int i = 0; i < steps; i++)
        value += ((i + 1.0) / steps - value) * (rate * 0.02);
    return (float)value;
}

} // namespace gamemath
