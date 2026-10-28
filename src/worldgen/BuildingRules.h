#pragma once
// What buildings, castles and fortresses are made of, before any block is laid
// (docs/specs/worldgen-structures-buildings.md §1, §9, §10): base heights, floors, stairs, joins and
// roofs of buildings; the castle cells of big structures with their rooms and corridor; the
// fortress squares of kind-8 places. Part C reads some of these too (the castle cells, the
// fortress floors).
#include "worldgen/CellObjects.h"

#include <cstdint>

namespace worldgen {

// A castle cell's four bytes (0x2a8290): kind 0 nothing; 8 a passage through a room wall; 9-12 room
// middles; 1-6 floors and room features.
struct CastleCell {
    uint8_t kind = 0, detail = 0, style = 0, steps = 0;
};

// A room of a big structure (0x2aa450, 0x2a04e0).
struct CastleRoom {
    int type = 0, flags = 0, b2 = 0, side = 0, steps = 0;
};

class BuildingRules {
public:
    explicit BuildingRules(CellObjects &cells) : cells(cells) {}
    CellObjects &cells;
    int type(int32_t cx, int32_t cy) { return cells.type(cx, cy); }

    // --- §1 ---
    int32_t baseHeight(int32_t cx, int32_t cy, int t = 0); // 0x2d87f0; t = 0 means the cell's type
    int32_t floorOffset(int32_t cx, int32_t cy);           // 0x2d82c0
    bool floorExists(int32_t cx, int32_t cy, int f);       // 0x2b8b10
    int stairs(int32_t cx, int32_t cy, int f);             // 0x2d8790
    bool sameBuilding(int32_t ax, int32_t ay, int32_t bx, int32_t by); // 0x2b8a60
    int joins(int32_t cx, int32_t cy);                     // 0x2d86e0: 1 west, 2 east, 4 south, 8 north
    int ridgeSides(int32_t cx, int32_t cy, int t, int joins); // 0x29d170
    static int roofHeight(int32_t x, int32_t y, int t, int ridge, int sub); // 0x2d34b0

    // --- §9 ---
    CastleCell castleCell(int32_t cx, int32_t cy); // 0x2a8290
    int32_t castleBlockTop(int32_t cx, int32_t cy); // 0x2aadd0
    int32_t castleFloor(int32_t cx, int32_t cy);    // 0x2ab0a0

    // --- §10 ---
    int fortressCellCode(int32_t cx, int32_t cy);          // 0x2a25a0
    int fortressStyle(int32_t cx, int32_t cy);             // 0x2b6870
    int fortressFloorCode(int32_t cx, int32_t cy, int f);  // 0x2a1d10
};

} // namespace worldgen
