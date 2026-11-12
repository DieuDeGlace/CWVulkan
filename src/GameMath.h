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

// Game update phase 27 (steam-game-update-complete-phase-schedule-20261112 §3.3): after the
// world batch, a maximum health that differs from its value before the batch rescales
// health by new/old in binary32; only a strictly greater result is clamped to the new maximum.
inline float rescaledHealth(float health, float before, float after) {
    if (!(after != before))
        return health;
    const float scaled = health * (after / before);
    return scaled > after ? after : scaled;
}

// Four-byte native counter updates retain the low32 bits before signed use.
inline int32_t add32(int32_t a, int32_t b) {
    const uint32_t bits = uint32_t(a) + uint32_t(b);
    int32_t result;
    std::memcpy(&result, &bits, sizeof result);
    return result;
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
inline int64_t multiplyFixed(int64_t a, int64_t b) {
    const uint64_t bits = uint64_t(a) * uint64_t(b);
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

// Native fixed-to-block conversion narrows before its negative exactness test.
inline int32_t floorFixedBlock(int64_t fixed) {
    uint32_t bits = uint32_t(fixed / 65536);
    int32_t narrowed;
    std::memcpy(&narrowed, &bits, sizeof narrowed);
    if (fixed < 0 && int64_t(narrowed) * 65536 != fixed)
        --bits;
    std::memcpy(&narrowed, &bits, sizeof narrowed);
    return narrowed;
}

// Native137e0: floor the narrowed block to its 16-block personal-state cell.
inline int32_t floorFixedCell(int64_t fixed) {
    const int64_t block = floorFixedBlock(fixed);
    return int32_t(block >= 0 ? block / 16 : (block - 15) / 16);
}

// The same native narrowed-block conversion for 64-block zones.
inline int32_t floorFixedZone(int64_t fixed) {
    const int64_t block = floorFixedBlock(fixed);
    return int32_t(block >= 0 ? block / 64 : (block - 63) / 64);
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
