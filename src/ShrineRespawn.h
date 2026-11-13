#pragma once
// Where a revived player stands (docs/specs/steam-shrine-of-life-activation-20261112.md §6.2,
// 0x2C2860): the nearest known Shrine of Life in the player's current land, else the nearest
// Shrine-of-Life site of that land within about 8192 blocks, else the death cell.
#include <array>
#include <cstdint>
#include <functional>
#include <set>

namespace shrinerespawn {
using Cell = std::array<int32_t, 2>; // 16-block cells

struct World {
    std::function<Cell(int32_t blockX, int32_t blockY)> landOf; // 0x2ADA20 of a block
    std::function<bool(int32_t cx, int32_t cy)> isShrine;      // the cell's building is 0x35
};

// The site cell of a 64-cell site region (0x2D3B60).
Cell siteOf(int32_t rx, int32_t ry);

// `known`: the character's known cells (+0x1528 +0xa8, in its sort order); `x`, `y`: the
// player's position (1/65536 block); `land`: its current land (+0x150/+0x154).
Cell choose(const World &world, const std::set<Cell> &known, int64_t x, int64_t y, Cell land);

} // namespace shrinerespawn
