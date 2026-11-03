// Adapted from AlphaVulkan (DieuDeGlace), GPL-3.0-only; see docs/alpha-generation.md.
#include "alphagen/Random.h"
#include <cmath>

namespace alphagen {

SeedOffsets makeSeedOffsets(uint32_t seed) {
    // §2.2: the groups in draw order, as (first place, count). Group 14 lies earlier in the
    // record but is drawn after group 13.
    static constexpr struct {
        uint32_t first, count;
    } GROUPS[] = {
        {0x188, 1}, {0x168, 4}, {0x18c, 20}, {0x1dc, 2}, {0x1e4, 2}, {0x1ec, 2}, {0x1f4, 2}, {0x1fc, 8},
        {0x21c, 6}, {0x234, 6}, {0x24c, 10}, {0x274, 4}, {0x284, 2}, {0x178, 4}, {0x28c, 2}, {0x294, 2},
    };
    SeedOffsets offsets;
    Random random(seed);
    for (const auto &group : GROUPS)
        for (uint32_t i = 0; i < group.count; i++)
            offsets.values[(group.first + 4 * i - SeedOffsets::FIRST) / 4] = random.draw() % 100000;
    return offsets;
}

double latticeValue(int32_t n) {
    uint32_t u = (uint32_t)n;
    u ^= u << 13;
    const uint32_t h = ((u * u * 60493u + 19990303u) * u + 0xD208DD0Du) & 0x7fffffffu;
    return 1.0 - (double)h * (1.0 / 1073741824.0);
}

float noise(double x, double y) {
    // §3.2. Truncation towards zero, not floor; pi is the game's double 3.1415927.
    constexpr double PI = 3.1415927;
    const int32_t ix = (int32_t)x, iy = (int32_t)y;
    const uint32_t ux = (uint32_t)ix, uy = (uint32_t)iy;
    const int32_t a = (int32_t)(ux + 57u * uy), b = (int32_t)(ux + 1u + 57u * uy);
    const int32_t c = (int32_t)(ux + 57u * (uy + 1u)), d = (int32_t)(ux + 1u + 57u * (uy + 1u));
    const double wx = (1.0 - std::cos((x - ix) * PI)) * 0.5;
    const double wy = (1.0 - std::cos((y - iy) * PI)) * 0.5;
    const double near = latticeValue(a) * (1.0 - wx) + latticeValue(b) * wx;
    const double far = latticeValue(c) * (1.0 - wx) + latticeValue(d) * wx;
    return (float)(near * (1.0 - wy) + far * wy);
}

} // namespace alphagen
