#pragma once
// Part B's small helpers and per-type tables (docs/specs/worldgen-structures.md §0 and
// worldgen-structures-tables.md T1-T15): the cell hash and its random numbers, the xorshifts,
// floor division, and what the builders know about each cell object type.
#include "worldgen/Noise.h"
#include "worldgen/Regions.h"

#include <cstdint>

namespace worldgen {

// --- Hashes and integer helpers (main file §0) ------------------------------------------------

// R(x, y, k) = rnd(H(x, y) + k), read as a signed number (the game's remainders are signed
// unless a step says otherwise).
inline int32_t cellRandom(int32_t x, int32_t y, uint32_t salt) { return (int32_t)random(cellKey(x, y) + salt); }
inline int32_t cellRandom(Int2 c, uint32_t salt) { return cellRandom(c.x, c.y, salt); }
// rnd(n) as a signed number.
inline int32_t signedRandom(uint32_t n) { return (int32_t)random(n); }

// The three-step xorshift X(v) (and xs(n), the same).
inline uint32_t xorshift(uint32_t v) {
    v ^= v << 13;
    v ^= v >> 17;
    v ^= v << 5;
    return v;
}
// The two-step form: v ^= v << 13; v ^= v >> 17.
inline uint32_t xorshift2(uint32_t v) {
    v ^= v << 13;
    v ^= v >> 17;
    return v;
}

// floor(a / n) and the non-negative remainder, for any positive n.
inline int32_t floorDivBy(int32_t a, int32_t n) {
    const int32_t q = a / n;
    return (a % n != 0 && a < 0) ? q - 1 : q;
}
inline int32_t posMod(int32_t a, int32_t n) {
    const int32_t r = a % n;
    return r < 0 ? r + n : r;
}
// 32-bit wrapping arithmetic.
inline int32_t wrapAdd(int32_t a, int32_t b) { return (int32_t)((uint32_t)a + (uint32_t)b); }
inline int32_t wrapMul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }

// Positions in records: 1/65536 of a block ("dots").
constexpr int64_t DOTS = 65536;
inline int64_t dotsOf(int32_t blocks) { return (int64_t)blocks * DOTS; }
// "Dots of a float f": trunc64(f x 65536.0) computed in float.
inline int64_t dotsOf(float f) { return trunc64(f * 65536.0f); }
// Blocks of dots, floor (an arithmetic shift).
inline int32_t blocksOf(int64_t d) { return (int32_t)(d >> 16); }
// Blocks of dots toward zero (division by 65536 truncating).
inline int32_t blocksTowardZero(int64_t d) { return (int32_t)(d / DOTS); }

// --- Colours ----------------------------------------------------------------------------------

struct Colour {
    float r = 0, g = 0, b = 0;
    bool operator==(const Colour &o) const { return r == o.r && g == o.g && b == o.b; }
};
inline Colour scaled(Colour c, float k) { return {c.r * k, c.g * k, c.b * k}; }

// --- Per-type tables (T1-T8) -------------------------------------------------------------------

int roofKind(int type);       // T2 (0x25fff0)
int floorHeight(int type);    // T3 (0x25f540)
int buildingHeight(int type); // T4 (0x25f6a0)
int mostFloors(int type);     // T5 (0x25f5f0)
int cellReach(int type);      // T6 (0x25f790)
// A building is a type whose floor height is above 0 (0x31ab90).
inline bool isBuilding(int type) { return floorHeight(type) > 0; }

// T7, the building tests. `style` is the zone place's style (main file §5).
bool keepsHeight(int type, uint32_t style);     // 0x25e9a0
bool hasBattlements(int type, uint32_t style);  // 0x25e940
bool joinsTower(int a, int b);                  // 0x25eb60
bool roofed(int type);                          // 0x25ea00
bool attachedHouse(int type);                   // 0x31ac20
bool waterLineTest(int type, uint32_t style);   // 0x25ec30
bool grassyFloor(int type, uint32_t style);     // 0x25ebc0
bool hasDoor(int type);                         // 0x31abb0
bool isHouse(int type);                         // 0x2b8030
bool crateCell(int type);                       // 0x2b4410
bool loreCell(int type);                        // 0x2b42e0
bool fenceFreeNeighbour(int type);              // 0x31a7c0
bool lightObject(int objectType);               // 0x31d470's test
bool restampedCategory(int category);           // 0x31ac70

// T8: a static object type's size in blocks at scale 1 (0x13a520).
struct ObjectSize {
    float x = 1, y = 1, z = 1;
};
ObjectSize objectSize(int objectType);

// --- Colours by type (T9-T15) ------------------------------------------------------------------

// T9: the building culture of a style and biome (0x25ea50).
int buildingCulture(uint32_t style, int biome);

// What the colour tables read: the zone place's style and culture, and the cell (patchwork).
struct ColourContext {
    uint32_t style = 0;
    int culture = 0;
};
Colour wallColour(int type, int32_t cx, int32_t cy, const ColourContext &c); // T11 (0x2600f0)
Colour trimColour(int type, int32_t cx, int32_t cy, const ColourContext &c); // T12 (0x25ece0)
Colour timberColour(const ColourContext &c);                                 // T13 (0x260a00)
Colour roofColour(int type, int32_t cx, int32_t cy, const ColourContext &c); // T14 (0x25f840)

// T15: castle colours by the castle cell's style byte and the zone's big structure.
Colour castleFirstColour(int styleByte, int bigStructure);  // 0x2fec70
Colour castleSecondColour(int styleByte, int bigStructure); // 0x2feb30
// The light colour of a castle room kind; false when it has none.
bool castleLightColour(int kind, Colour &out);
int castleRoomDepth(int kind);    // 0x2feaf0
int castleRoomRise(int styleByte); // 0x2aa250

} // namespace worldgen
