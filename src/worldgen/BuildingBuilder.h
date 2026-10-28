#pragma once
// The building builder's state (docs/specs/worldgen-structures-buildings.md §2-§8), shared by
// Buildings.cpp (walls, floors, doors, roofs) and Rooms.cpp (furniture and room objects, §6).
// Internal to the world generator.
#include "worldgen/Buildings.h"

#include <array>
#include <vector>

namespace worldgen {

// The eight neighbours in the game's order: W, E, S, N, SW, SE, NW, NE.
enum Side { WEST, EAST, SOUTH, NORTH, SOUTH_WEST, SOUTH_EAST, NORTH_WEST, NORTH_EAST };
constexpr Int2 SIDE_OFFSET[8] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1}};

// One floor's values (§4 steps 1-9).
struct Floor {
    int f = 0;
    bool level = false; // the castle level below the building (f = -1)
    int stairs = 0;
    int32_t z0 = 0, top0 = 0, bottom0 = 0, depth0 = 0;
    Colour floorColour, floorColour2;
    Colour tint, border;
    bool carpet = false;
    int32_t carpetW = 0, carpetD = 0;
    int floorPattern = 1;
    bool has[8] = {};
    int32_t nz[8] = {}, nTop[8] = {}, nBottom[8] = {}, nDepth[8] = {};
    int32_t floorMin = 0, maxTop = 0;
    int32_t innerX0 = 0, innerX1 = 0, innerY0 = 0, innerY1 = 0;
    int32_t doorWidth = 16, wall = 2;
    bool dome = false;
    bool ring = false; // a floor border (castle detail 7, fortress code 7)
    int ringCodes[8] = {};
};

class BuildingBuilder {
public:
    BuildingBuilder(ZoneBuild &zone, BuildingRules &rules, int i, int j, int32_t cx, int32_t cy, int t, CastleCell k);
    void build();

    ZoneBuild &zone;
    BuildingRules &rules;
    int i, j;
    int32_t cx, cy;
    int t;
    CastleCell k;
    Mason m;

    // §2.
    int32_t base = 0, height = 0;
    Colour colA, colB;
    int roof = 0, sub = -1, overhang = 1;
    bool deepRoof = false;
    Scheme s1, s3;
    int signDraw = 0;
    bool timberFrame = false;
    // §4.
    int floors = 0, F = 0;
    int nT[8] = {};
    int32_t nB[8] = {};
    int doorSide = 0, doorPick = -1;
    int32_t castleZ = 0, castleTop = 0, castleBottom = 0;
    bool special = false;

    bool insideZone() const { return i >= 0 && i < 4 && j >= 0 && j < 4; }
    ColourContext colours() const { return zone.place.colours(); }

    // Steps.
    void setUp();
    void foundations();
    void frameAndBattlements();
    void floorLoop();
    void prepareFloor(Floor &fl);
    void floorColumns(Floor &fl);
    void afterColumns(Floor &fl);
    void wallsDoorsWindows(Floor &fl);
    void ceilingLamp(Floor &fl);
    void cornerPosts();
    void roofAndExtras();

    // §5 helpers.
    void outdoorSpot(int d, float along, float modelAlong, int turnOverride);
    int doorObjectType() const;

    // §6 (Rooms.cpp).
    void roomContents(Floor &fl);
    void roomObjects(Floor &fl);
    int roomCode(const Floor &fl);
};

} // namespace worldgen
