#pragma once
// The world generator's building blocks (docs/specs/worldgen-terrain.md §0 and §1): the integer
// hash, the 0..100000 random number made from it, the game's 2D simplex noise, the C runtime's
// rand(), and the small float rules every step of the generator follows.
//
// Bit-exactness: the generator's results must equal the game's float for float. Every helper
// here does one rounded single-precision operation at a time, in the order the spec gives;
// "in double" is said where it applies.
#include <cmath>
#include <cstdint>

namespace worldgen {

// --- Integers ------------------------------------------------------------------------------

// The integer hash (§1.1): 0 .. 2^31 - 1.
uint32_t hash(uint32_t n);
// The 0..100000 random number of a hash (§1.2): 100000 - hash / 21474. For the top 0.003 % of
// hashes it wraps below zero (as unsigned: 4294967293..4294967295); callers that read it as a
// signed number cast it.
uint32_t random(uint32_t n);
// Whether hash(n) / 21474 is odd (§1.3) - the same as random(n) being odd.
bool oddHash(uint32_t n);
// The cell key most hashes start from (§1.2): y * 57 + x, wrapping.
inline uint32_t cellKey(int32_t x, int32_t y) { return (uint32_t)y * 57u + (uint32_t)x; }

// floor(a / 2^shift) for a signed 32-bit a (§0 "integer floor division"). An arithmetic right
// shift is exactly the floor.
inline int32_t floorDiv(int32_t a, int shift) { return a >> shift; }
// floor(a / 65536) of a 64-bit dot coordinate, as a 32-bit block coordinate.
inline int32_t floorDots(int64_t dots) { return (int32_t)(dots >> 16); }

// Conversion toward zero as the processor does it (cvttss2si / cvttsd2si): a value out of
// range, or NaN, gives the "integer indefinite" (the lowest integer).
inline int32_t trunc32(double v) {
    return v > -2147483649.0 && v < 2147483648.0 ? (int32_t)v : INT32_MIN;
}
inline int64_t trunc64(double v) {
    return v > -9223372036854775808.0 && v < 9223372036854775808.0 ? (int64_t)v : INT64_MIN;
}
// The game's floor of a float (§0): truncate, then step down for negatives with a fraction.
inline int32_t floorToInt(float v) {
    int32_t i = trunc32(v);
    if (v < 0.0f && (float)i != v)
        i -= 1;
    return i;
}

// --- Float rules (§0) -----------------------------------------------------------------------

// min and max as the processor's minss / maxss: the first operand only when the strict
// comparison holds (so the second on a tie or NaN).
inline float minOf(float a, float b) { return a < b ? a : b; }
inline float maxOf(float a, float b) { return a > b ? a : b; }
// min(1, max(0, v)).
inline float clamp01(float v) { return minOf(1.0f, maxOf(0.0f, v)); }
// The smooth step s*3*s - (s+s)*s*s, in exactly this order.
inline float smooth(float s) { return s * 3.0f * s - (s + s) * s * s; }
// |v| taken in double and rounded back (which is the same as fabsf).
inline float absOf(float v) { return std::fabs(v); }

// --- Noise (§1.4) ----------------------------------------------------------------------------

// The game's 2D simplex noise (0x2e0c0): Gustavson's public-domain formulation with Ken
// Perlin's permutation; inputs scaled by 0.32 first; about -1..1.
float simplex(double x, double y);

// One noise layer at a column (the specs' "N(sx·X + ox, sy·Y + oy)"): each input is the
// integer coordinate in double, times the scale, plus the offset.
inline float noiseAt(int32_t x, int32_t y, double sx, double ox, double sy, double oy) {
    return simplex((double)x * sx + ox, (double)y * sy + oy);
}

// --- The C runtime's rand() (§1.5) -----------------------------------------------------------

struct CrtRandom {
    uint32_t state = 1; // as after srand(1)
    void seed(uint32_t value) { state = value; }
    int next() {
        state = state * 214013u + 2531011u;
        return (int)((state >> 16) & 0x7fff);
    }
};

} // namespace worldgen
