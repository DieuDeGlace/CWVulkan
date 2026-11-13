#include "ShrineRespawn.h"

#include "worldgen/Noise.h"

namespace shrinerespawn {
namespace {
int32_t floorDiv(int64_t a, int64_t b) { return (int32_t)(a >= 0 ? a / b : -((-a + b - 1) / b)); }
} // namespace

Cell siteOf(int32_t rx, int32_t ry) {
    const uint32_t base = worldgen::cellKey(rx, ry);
    return {rx * 64 + 1 + (int32_t)(worldgen::random(base + 0x24341u) % 62u), ry * 64 + 1 + (int32_t)(worldgen::random(base + 0x165709u) % 62u)};
}

Cell choose(const World &world, const std::set<Cell> &known, int64_t x, int64_t y, Cell land) {
    const int32_t bx = floorDiv(x, 65536), by = floorDiv(y, 65536);
    Cell best{floorDiv(bx, 16), floorDiv(by, 16)}; // the death cell
    double bestD2 = -1.0;
    // Candidate centres (cell * 16 + 8 blocks) against the fixed position, horizontally.
    const auto consider = [&](Cell c) {
        const int32_t cx = c[0] * 16 + 8, cy = c[1] * 16 + 8;
        if (world.landOf(cx, cy) != land)
            return;
        const double dx = (double)((int64_t)cx * 65536 - x) * (1.0 / 65536.0), dy = (double)((int64_t)cy * 65536 - y) * (1.0 / 65536.0);
        const double d2 = dx * dx + dy * dy;
        if ((bestD2 < 0.0 || d2 < bestD2) && world.isShrine(c[0], c[1]))
            best = c, bestD2 = d2;
    };
    for (const Cell &c : known) // pass 1: known shrines
        consider(c);
    if (bestD2 >= 0.0)
        return best;
    // Pass 2: every site region within about 8192 blocks, x outer.
    const int32_t rx0 = floorDiv(floorDiv(bx - 8192, 16), 64), rx1 = floorDiv(floorDiv(bx + 8192, 16), 64);
    const int32_t ry0 = floorDiv(floorDiv(by - 8192, 16), 64), ry1 = floorDiv(floorDiv(by + 8192, 16), 64);
    for (int32_t rx = rx0; rx <= rx1; ++rx)
        for (int32_t ry = ry0; ry <= ry1; ++ry)
            consider(siteOf(rx, ry));
    return best;
}

} // namespace shrinerespawn
