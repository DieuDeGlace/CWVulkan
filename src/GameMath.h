#pragma once
// Small numeric rules the game shares between its systems.
#include <algorithm>
#include <cstdint>

namespace gamemath {

// Ordinary Game frame, steam-scene-lighting §5.1: retain both double operations.
inline double millisecondsFromNanoseconds(int64_t elapsedNanoseconds) {
    return (double(elapsedNanoseconds) / 1000000000.0) * 1000.0;
}
inline double frameMilliseconds(double elapsedMs) {
    return std::min(elapsedMs, 1000.0);
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
