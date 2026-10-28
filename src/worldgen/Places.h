#pragma once
// Places: the world's 1024-block cells, some holding a landmark (a village, a castle, a lake,
// a volcano...) at a random point (docs/specs/worldgen-terrain.md §3). Some cells are "big":
// then the 2 x 2 cells sharing an even corner form one place of radius 500 instead of 250.
// Landmark kinds are the game's numbers, 1..47.
#include "worldgen/Regions.h"

#include <cstdint>

namespace worldgen {

constexpr int PLACE_SHIFT = 10; // 1024 blocks
inline int32_t placeRadius(bool big) { return big ? 500 : 250; }

// Whether a place cell is big (0x36160, with its "sevenths" test 0x36210).
bool bigCell(int32_t cx, int32_t cy);
// The landmark point of a place cell, in blocks (0x362a0), for the big flag given.
Int2 cellPoint(int32_t cx, int32_t cy, bool big);
// The noise offsets (blocks) by which a point is moved when measuring its distance to a
// landmark (0x364e0 and 0x2baf90).
struct LandmarkWarp {
    float x, y;
};
LandmarkWarp landmarkWarp(int32_t x, int32_t y);
// The squared distance from a point, warped as above, to a landmark point (0x364e0).
float warpedDist2(Int2 landmark, int32_t x, int32_t y);
// The landmark kind of a place cell, 0 for none (0x2b99d0).
int placeKind(int32_t cx, int32_t cy, bool big);
// The place's variant (0x2abb90): 0; 1 raised plateau; 2 spiral ramp up; 4 water or lava ring
// (and part B's "now and then" variants); 5 walled pit. (3, a spiral pit, is never made here.)
int placeVariant(int32_t cx, int32_t cy, int kind, bool big);
// The weight of a place at squared distance d2 from its landmark (0x9bee0): (1 - d2 / r^2)^2
// inside the radius r, else 0; 0 for no place.
float placeWeight(int kind, bool big, float d2);

// A place record (0x35ce0's 0x20-byte record).
struct Place {
    int32_t kind = 0;
    bool big = false;
    int32_t variant = 0;
    int32_t radius = 0; // 500 or 250; 0 when there is no place
    Int2 point;         // the landmark point
    float dist2 = 0;    // warped squared distance from the column to it
    float weight = 0;   // 1 at the landmark, 0 at the radius and beyond
};
// Same place for a column (kind 0 places compare equal whatever their other fields).
bool samePlace(const Place &a, const Place &b);

// The place around a point (0x35ce0): its own cell's landmark, if the point is within reach of
// it (532 blocks for a big place, 282 otherwise, unwarped).
Place placeAt(int32_t x, int32_t y);
// A cell's place with no reach limit, as a zone keeps for all its columns (0x31a5d0): kind,
// big, variant, radius and landmark point; the distance and weight are left 0.
Place cellPlace(int32_t cx, int32_t cy);
// That place's distance and weight at one column (0x31a690).
Place placeForColumn(const Place &cell, int32_t x, int32_t y);

} // namespace worldgen
