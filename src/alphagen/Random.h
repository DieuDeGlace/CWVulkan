// Adapted from AlphaVulkan (DieuDeGlace), GPL-3.0-only; see docs/alpha-generation.md.
#pragma once
// Randomness of Alpha's world generator (docs/specs/alpha-worldgen-random.md): the C runtime's
// linear congruential generator, the 77 per-world seed offsets, and the lattice noise N(x, y)
// every terrain, climate and colour formula is built on. The section numbers point there.
//
// Precision matters for a 1:1 world: wherever the spec says float the code uses float, and
// double where it says double.
#include <cstdint>

namespace alphagen {

// §1.1: state = state * 214013 + 2531011; a draw is bits 16..30 (0..32767).
class Random {
  public:
    explicit Random(uint32_t seed = 1) : state(seed) {}
    void seed(uint32_t s) { state = s; }
    int draw() {
        state = state * 214013u + 2531011u;
        draws++;
        return (int)((state >> 16) & 0x7fff);
    }
    uint32_t count() const { return draws; } // draws made so far (for checks)

  private:
    uint32_t state;
    uint32_t draws = 0;
};

// §2.2: the 77 seed offsets, drawn once per world. The specs name each by its place in the
// world record, 0x800168 .. 0x800298 (4 bytes apart, 77 in all), so they are kept by that
// place: offsets[0x18c] is the spec's O18c.
struct SeedOffsets {
    static constexpr uint32_t FIRST = 0x168, LAST = 0x298;
    int32_t values[(LAST - FIRST) / 4 + 1] = {};

    // The spec's Oxxx, by the low bits of its place (0x168 .. 0x298).
    int32_t operator[](uint32_t place) const { return values[(place - FIRST) / 4]; }
    // The world's base offset A (place 0x188).
    int32_t A() const { return (*this)[0x188]; }
};

// §2.2: srand(seed), then 77 draws in the table's order.
SeedOffsets makeSeedOffsets(uint32_t seed);

// §3: cosine-interpolated lattice value noise, double in, float out.
float noise(double x, double y);
// §3.1: the lattice value at one integer point.
double latticeValue(int32_t n);

// §4: (n + 1) * 0.5 in float.
inline float unit(float n) {
    return (n + 1.f) * 0.5f;
}
// 3t^2 - 2t^3.
inline float smooth(float t) {
    return t * t * 3.f - t * t * t * 2.f;
}
inline float sat(float t) {
    return t < 0 ? 0.f : (t > 1 ? 1.f : t);
}
// max(1 - f, 0)^2, the common falloff of a normalised squared distance.
inline float bump(float f) {
    const float v = 1.f - f;
    return v > 0 ? v * v : 0.f;
}
// Division rounding towards minus infinity.
inline int32_t floorDiv(int32_t a, int32_t n) {
    return a >= 0 ? a / n : -((-a + n - 1) / n);
}

} // namespace alphagen
