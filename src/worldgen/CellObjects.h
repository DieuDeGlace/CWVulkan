#pragma once
// The object of each 16-block cell (docs/specs/worldgen-structures.md §6): trees, rocks, hills,
// houses, towns, castles, dungeons, towers, fields and landmarks. A cell's object depends only on
// the world seed; builders ask for their neighbours' again and again, so the answers are cached.
#include "worldgen/Places.h"
#include "worldgen/StructureTables.h"
#include "worldgen/WorldValues.h"

#include <cstdint>
#include <vector>

namespace worldgen {

// Where in its cell an object stands (0x2d8630).
Int2 cellPoint(int32_t cx, int32_t cy);

// A big place's big structure (0x2aa270): a castle or dungeon kind 40..47, or 0.
int bigStructure(int32_t px, int32_t py, int kind, bool big);
// Its anchor, in cells (0x2a9df0).
Int2 structureAnchor(int32_t px, int32_t py, int kind, bool big);
// The anchor stage A's terrain asks for (TerrainOptions::anchor): NO_ANCHOR without a big
// structure.
Int2 terrainAnchor(Int2 placeCell, int kind, bool big);

// A place's landmark cell object (0x2ba990), 0 for none.
int placeLandmark(int kind, Int2 placeCell);

// --- Town helpers (§6.4) -------------------------------------------------------------------

// The town block of a cell (0x2af7b0): 0 unless kind 5; else the layout's entry (2..11), 13 for a
// farm block, 1 for a plain town block, 0 outside.
int townBlock(const WorldValues &world, int32_t cx, int32_t cy, int kind);
// A town block's corner cell (0x2b5760).
Int2 blockCorner(int32_t bx, int32_t by);
// A cell's turn in its block (0x2b5820), -1 for the third row and column.
int cellTurn(int32_t cx, int32_t cy);
// Whether a block is well inside a place of `kind` (0x2b8e20).
bool innerBlock(int32_t bx, int32_t by, int kind, Int2 anchor);

// The area landmark's cell of the 64 x 64-cell area (qx, qy) (0x2d3b60).
Int2 areaLandmark(int32_t qx, int32_t qy);

// A kind-8 place's fortress square: its size, turn and middle cell (§6.3 rule 17).
struct FortressSquare {
    int32_t size = 0, turn = 0;
    Int2 middle;
    // A cell turned about the middle as the game turns it.
    Int2 turned(int32_t cx, int32_t cy) const;
};
FortressSquare fortressSquare(Int2 placeCell, Int2 landmark);

// --- The cell objects ----------------------------------------------------------------------

class CellObjects {
public:
    explicit CellObjects(const WorldValues &world);
    // The object of cell (cx, cy) (0x69c60), 0 for none.
    int type(int32_t cx, int32_t cy);
    const WorldValues &world() const { return *world_; }

private:
    int compute(int32_t cx, int32_t cy) const;
    int natural(int32_t cx, int32_t cy, Int2 q, const Place &p) const;

    struct Entry {
        int32_t cx = 0, cy = 0;
        int16_t type = -1;
    };
    const WorldValues *world_;
    std::vector<Entry> cache_;
};

} // namespace worldgen
