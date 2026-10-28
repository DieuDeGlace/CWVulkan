#pragma once
// Natural structures (docs/specs/worldgen-structures-nature.md): trees and their leaves, stumps,
// fallen logs, rocks, boulders, hills and canyons, the mound under hilltop buildings and the
// sunken chest. Each builds into the zone only; the rand() draws are the zone's.
#include "worldgen/ZoneBuild.h"

namespace worldgen {

// Trees at (x, y) with their foot at z, size h and trunk height w (§3-§7).
void commonTree(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, int32_t h, int32_t w); // 0x264670
void fir(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, int32_t h, int32_t w);        // 0x263210
void birch(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, int32_t h, int32_t w);      // 0x2626e0
void willow(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, int32_t h, int32_t w);     // 0x263ea0
void palm(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, int32_t h, int32_t w);       // 0x265e70
void stump(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, int32_t h, int32_t w);      // 0x266ad0
// A fallen log of radius r and length len, along x or y (0x2651f0, 0x265840).
void fallenLog(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, float r, int32_t len, bool hollow, bool alongY);

// The mound under a hilltop building (0x146090).
void mound(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, int32_t rx, int32_t ry);
// The sunken chest of cell (cx, cy) (§9).
void sunkenChest(ZoneBuild &zone, int32_t cx, int32_t cy);
// A rock (0x146450) and a boulder (0x146560).
void rock(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, int32_t rx, int32_t ry, int32_t rz, uint8_t flags);
void boulder(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, int32_t rx, int32_t ry, int32_t rz, uint8_t flags);
// A hill of rock or earth with its top at `top` (0x145fa0, 0x145eb0); with a canyon running from
// its centre to `canyonEnd` (0x145b30, 0x145cf0).
struct Canyon {
    Int2 end;
    bool open = false;
};
void hill(ZoneBuild &zone, int32_t x, int32_t y, int32_t top, int32_t rx, int32_t ry, bool rocky, const Canyon *canyon = nullptr);

// Whether block (X, Y, Z) is inside a rock centred at (x, y, z) with radii rx, ry, rz (0x146180).
bool insideRock(int32_t X, int32_t Y, int32_t Z, int32_t x, int32_t y, int32_t z, int32_t rx, int32_t ry, int32_t rz);

// The two leaf colours of a tree standing at (x, y) (§2), given the climate there: the common
// leaves (0x2afbf0), the birch's (0x2ae3a0) or the fir's (0x2ae840).
enum class LeafKind { Common, Birch, Fir };
struct TreeLeaves {
    Colour a, b;
};
TreeLeaves treeLeaves(LeafKind kind, int32_t x, int32_t y, float warmth, float dryness, float forest, float kind27);

} // namespace worldgen
