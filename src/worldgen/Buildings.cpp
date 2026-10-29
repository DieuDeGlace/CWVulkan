// The building builder, written from docs/specs/worldgen-structures-buildings.md §2-§5, §7, §8
// (sections cited as §n). Room contents (§6) are in Rooms.cpp.
#include "worldgen/BuildingBuilder.h"

#include "worldgen/BlockColours.h"
#include "worldgen/Height.h"

#include <algorithm>
#include <cmath>

namespace worldgen {
namespace {

constexpr int WALL_TURN[4] = {3, 1, 0, 2};
constexpr float WALL_ANGLE[4] = {270.0f, 90.0f, 0.0f, 180.0f};

Colour plus(Colour a, Colour b) { return {a.r + b.r, a.g + b.g, a.b + b.b}; }
Colour half(Colour c) { return {c.r * 0.5f, c.g * 0.5f, c.b * 0.5f}; }
int32_t lo32(int32_t a, int32_t b) { return a < b ? a : b; }
int32_t hi32(int32_t a, int32_t b) { return a > b ? a : b; }
bool oneOf(int t, std::initializer_list<int> list) { return std::find(list.begin(), list.end(), t) != list.end(); }

// The floor border of castle detail 7 and fortress code 7 (§4.1): 0 nothing, 1 or 2 (bright).
int floorBorder(int32_t x, int32_t y, const int code[8]) {
    auto seven = [&](Side s) { return code[s] == 7; };
    const bool w = seven(WEST), e = seven(EAST), s = seven(SOUTH), n = seven(NORTH);
    const bool sw = seven(SOUTH_WEST), se = seven(SOUTH_EAST), nw = seven(NORTH_WEST), ne = seven(NORTH_EAST);
    const bool drawn = (3 <= x && x < 13 && 3 <= y && y < 13) || (x < 3 && w && 3 <= y && y <= 12) ||
                       (x >= 13 && e && 3 <= y && y < 13) || (y < 3 && s && 3 <= x && x <= 12) ||
                       (y >= 13 && n && 3 <= x && x < 13) || (x < 3 && y < 3 && sw && w && s) ||
                       (x >= 13 && y < 3 && se && e && s) || (x < 3 && y >= 13 && nw && w && n) ||
                       (x >= 13 && y >= 13 && ne && e && n);
    if (!drawn)
        return 0;
    // The bright line, as a sequence of steps that answer, jump or fall through.
    int step = 1;
    for (;;) {
        switch (step) {
        case 1:
            if (w && (y >= 3 || code[SOUTH_WEST] == code[WEST])) {
                step = !(y < 13 || nw) ? 2 : 3;
                continue;
            }
            break;
        case 2:
            if (x == 4 && (y >= x || s) && (y < 12 || n))
                return 2;
            break;
        case 3:
            if (e && (y >= 3 || code[SOUTH_EAST] == code[EAST])) {
                step = !(y < 13 || ne) ? 4 : 5;
                continue;
            }
            break;
        case 4:
            if (x == 11) {
                if (y >= 4 || s) {
                    if (y < 12 || n)
                        return 2;
                    step = 5;
                    continue;
                }
                step = 6;
                continue;
            }
            break;
        case 5:
            if (s && (x >= 3 || code[SOUTH_WEST] == code[SOUTH]) && (x < 13 || se)) {
                step = 7;
                continue;
            }
            break;
        case 6:
            if (y == 4 && (x >= y || w) && (x < 12 || e))
                return 2;
            break;
        case 7:
            if (n && (x >= 3 || code[NORTH_WEST] == code[NORTH]) && (x < 13 || ne)) {
                step = 9;
                continue;
            }
            break;
        case 8:
            if (y == 11 && (x >= 4 || w) && (x < 12 || e))
                return 2;
            break;
        case 9:
            if (!sw) {
                if (x == 4) {
                    if (y > x) {
                        step = 10;
                        continue;
                    }
                    if (s)
                        return 2;
                }
                if (y == 4 && x <= y && w)
                    return 2;
            }
            break;
        case 10:
            if (!se) {
                if (x == 11) {
                    if (y > 4) {
                        step = 11;
                        continue;
                    }
                    if (s)
                        return 2;
                }
                if (y == 4 && x >= 11 && e)
                    return 2;
            }
            break;
        case 11:
            if (!nw) {
                if (x == 4) {
                    if (y < 11) {
                        step = 12;
                        continue;
                    }
                    if (n)
                        return 2;
                }
                if (y == 11 && x <= 4 && w)
                    return 2;
            }
            break;
        case 12:
            if (!ne) {
                if (x == 11) {
                    if (y < x)
                        return 1;
                    if (n)
                        return 2;
                }
                if (y == 11 && x >= y && e)
                    return 2;
            }
            break;
        default: return 1;
        }
        step++;
    }
}

} // namespace

// --- §2 ------------------------------------------------------------------------------------------

BuildingBuilder::BuildingBuilder(ZoneBuild &zone, BuildingRules &rules, int i, int j, int32_t cx, int32_t cy, int t, CastleCell k)
    : zone(zone), rules(rules), i(i), j(j), cx(cx), cy(cy), t(t), k(k), m(Mason::atCell(zone, cx, cy)) {}

void buildBuilding(ZoneBuild &zone, BuildingRules &rules, int i, int j, int32_t cx, int32_t cy, int t, CastleCell k) {
    BuildingBuilder(zone, rules, i, j, cx, cy, t, k).build();
}

void BuildingBuilder::build() {
    setUp();
    if (F > 0)
        foundations();
    s3 = m.scheme;
    for (Colour &c : s3.colour)
        c = scaled(c, 0.75f);
    if (roof == 4)
        signDraw = zone.draw();
    frameAndBattlements();
    // A castle cell under no building goes straight to the roof step.
    if (k.kind != 0 || isBuilding(t)) {
        floorLoop();
        cornerPosts();
    }
    roofAndExtras();
}

void BuildingBuilder::setUp() {
    const uint32_t style = zone.place.style;
    base = rules.baseHeight(cx, cy, t);
    height = buildingHeight(t) + (keepsHeight(t, style) ? 0 : 2);
    if (i >= 0 && waterLineTest(t, style))
        m.waterLine = 10;
    colA = wallColour(t, cx, cy, colours());
    colB = trimColour(t, cx, cy, colours());
    roof = roofKind(t);
    sub = roof == 1 ? zone.place.culture : -1;
    switch (roof) {
    case 1:
        switch (sub) {
        case 1: case 3: case 4: overhang = 2; break;
        case 5: overhang = 0; break;
        case 6: overhang = 4; break;
        default: overhang = 1; break;
        }
        break;
    case 3: case 6: case 10: overhang = 4; break;
    case 4: case 9: overhang = 2; break;
    default: overhang = 1; break;
    }
    deepRoof = roof == 1 ? sub == 4 : (roof == 3 || roof == 4 || roof == 6 || roof == 10);
    if (i >= 0 && t == 25)
        m.twoColours(colB, 0, 0, plus(colA, {50, 50, 50}), 0, 10, true);
    else if (i >= 0 && (roof == 7 || sub == 2 || sub == 3))
        m.twoColours(colB, 6, 0, colA, 6, 10, true);
    else if (i >= 0 && sub == 6 && t != 26)
        m.twoColours(colB, 7, 0, colA, 7, 10, true);
    else if (i >= 0 && roof == 8)
        m.twoColours({colB.r * 0.8f, colB.g * 1.0f, colB.b * 0.8f}, 1, 0, colA, 1, 10, true);
    else if (i >= 0 && roof == 5)
        m.twoColours(colB, 1, 0, colA, 1, 25, true);
    else
        m.twoColours(colB, 1, 0, colA, 1, 10, true);
    s1 = m.scheme;
    F = floorHeight(t);
}

void BuildingBuilder::foundations() {
    if (t >= 63 && t <= 65) {
        m.oneColour(colA, pattern::PLAIN);
        const int32_t z = base - 2 + (t == 63 || t == 64 ? 3 : 0);
        const int te = rules.type(cx + 1, cy), tn = rules.type(cx, cy + 1);
        const bool e = te >= 63 && te <= 65, n = tn >= 63 && tn <= 65;
        m.foundation(0, 0, z, 2, 2, 1, 0, 0);
        if (!e)
            m.foundation(14, 0, z, 2, 2, 1, 0, 0);
        if (!n)
            m.foundation(0, 14, z, 2, 2, 1, 0, 0);
        if (!e && !n)
            m.foundation(14, 14, z, 2, 2, 1, 0, 0);
        m.oneColour(colA, zone.draw() % 2 + 4);
        m.type = 23;
        m.box(0, 0, base - 1, 16, 16, 1);
        return;
    }
    if (roof == 3 || roof == 6 || roof == 10 || oneOf(t, {16, 26, 50, 51, 55, 57, 58, 59, 61})) {
        m.foundation(0, 0, base - 1, 16, 16, 1, 0, 0);
        return;
    }
    int32_t top = rules.floorOffset(cx, cy) + base;
    for (int d = WEST; d <= NORTH; d++) {
        const int32_t nx = cx + SIDE_OFFSET[d].x, ny = cy + SIDE_OFFSET[d].y;
        if (isBuilding(rules.type(nx, ny)))
            top = lo32(top, rules.baseHeight(nx, ny) + rules.floorOffset(nx, ny));
    }
    m.foundation(0, 0, top, 16, 16, height - top + base, 0, 0);
}

// --- §3 ------------------------------------------------------------------------------------------

void BuildingBuilder::frameAndBattlements() {
    const uint32_t style = zone.place.style;
    timberFrame = (sub == 0 || sub == 2 || roof == 7 || roof == 8) && keepsHeight(t, style);
    if (timberFrame) {
        const Scheme saved = m.scheme;
        if (sub == 0)
            m.oneColour(timberColour(colours()), pattern::PLAIN);
        else if (sub == 2)
            m.twoColours(plus(scaled({255, 255, 255}, 0.5f), scaled(colB, 0.5f)), 0, 0,
                         plus(scaled({255, 255, 255}, 0.5f), scaled(colA, 0.5f)), 0, 0, false);
        else if (roof == 8)
            m.oneColour({60, 50, 30}, pattern::PLAIN);
        else
            m.oneColour(scaled(colA, 0.85f), pattern::PLAIN);
        s3 = m.scheme;
        if (F > 0)
            for (int32_t kk = 0; kk <= height / F; kk++)
                m.box(0, 0, base - 1 + kk * F, 16, 16, 1);
        const bool jW = attachedHouse(rules.type(cx - 1, cy)), jS = attachedHouse(rules.type(cx, cy - 1));
        const bool jE = attachedHouse(rules.type(cx + 1, cy)), jN = attachedHouse(rules.type(cx, cy + 1));
        const int32_t bW = rules.baseHeight(cx - 1, cy), bS = rules.baseHeight(cx, cy - 1);
        int32_t z = base;
        if (jW)
            z = lo32(bW, base);
        if (jS)
            z = lo32(bS, base);
        m.box(0, 0, z - 1, 1, 1, height - z + base);
        if (!jE) {
            z = jS ? lo32(bS, base) : base;
            m.box(15, 0, z - 1, 1, 1, height - z + base);
        }
        if (!jN) {
            z = jW ? lo32(bW, base) : base;
            m.box(0, 15, z - 1, 1, 1, height - z + base);
        }
        if (!jE && !jN)
            m.box(15, 15, base - 1, 1, 1, height);
        m.scheme = saved;
    }
    if (!hasBattlements(t, style))
        return;
    const int32_t o = overhang, len = 16 + 2 * o, z = base + height - o;
    if (!joinsTower(t, rules.type(cx - 1, cy))) {
        m.box(-o, -o, z, 2, len, 3);
        m.battlements(false, -o, -o, z + 3, len, 2, 2);
    }
    if (!joinsTower(t, rules.type(cx + 1, cy))) {
        m.box(o + 14, -o, z, 2, len, 3);
        m.battlements(false, o + 14, -o, z + 3, len, 2, 2);
    }
    if (!joinsTower(t, rules.type(cx, cy - 1))) {
        m.box(-o, -o, z, len, 2, 3);
        m.battlements(true, -o, -o, z + 3, len, 2, 2);
    }
    if (!joinsTower(t, rules.type(cx, cy + 1))) {
        m.box(-o, o + 14, z, len, 2, 3);
        m.battlements(true, -o, o + 14, z + 3, len, 2, 2);
    }
}

// --- §4 ------------------------------------------------------------------------------------------

void BuildingBuilder::floorLoop() {
    floors = F > 0 ? buildingHeight(t) / F : 0;
    for (int d = 0; d < 8; d++) {
        nT[d] = rules.type(cx + SIDE_OFFSET[d].x, cy + SIDE_OFFSET[d].y);
        nB[d] = rules.baseHeight(cx + SIDE_OFFSET[d].x, cy + SIDE_OFFSET[d].y);
    }
    // The door's side.
    int sides[4], count = 0;
    if (hasDoor(t)) {
        for (int d = WEST; d <= NORTH; d++)
            if (!isBuilding(nT[d]))
                sides[count++] = 1 << d;
        zone.addSpot(cx, cy, 4, {dotsOf(16 * cx + 8), dotsOf(16 * cy + 8), dotsOf(base + rules.floorOffset(cx, cy))});
    }
    if (count > 0) {
        doorSide = doorPick = sides[zone.draw() % count];
    } else {
        doorPick = -1;
        static constexpr int ANY[4] = {1, 2, 4, 8};
        doorSide = ANY[zone.draw() % 4];
    }
    if (k.kind != 0) {
        castleZ = rules.castleFloor(cx, cy);
        castleBottom = rules.castleBlockTop(cx, cy);
    }
    castleTop = castleZ + castleRoomRise(k.style);
    special = t == 39 || t == 86 || t == 94 || t == 95;
    for (int f = k.kind != 0 ? -1 : 0; f < floors; f++) {
        if (f >= 0 && !rules.floorExists(cx, cy, f))
            continue;
        Floor fl;
        fl.f = f;
        prepareFloor(fl);
        floorColumns(fl);
        afterColumns(fl);
        if (!fl.level) {
            wallsDoorsWindows(fl);
            ceilingLamp(fl);
        }
        roomContents(fl);
    }
}

void BuildingBuilder::prepareFloor(Floor &fl) {
    const int f = fl.f;
    fl.level = f < 0;
    fl.stairs = rules.stairs(cx, cy, f);
    fl.z0 = f * F + base + rules.floorOffset(cx, cy);
    if (fl.level) {
        fl.z0 = castleZ;
        fl.depth0 = castleRoomDepth(k.kind);
        fl.floorColour = castleFirstColour(k.style, zone.place.structure);
        fl.floorColour2 = castleSecondColour(k.style, zone.place.structure);
    } else {
        fl.floorColour = fl.floorColour2 = colA;
    }
    // 2. Two colours drawn for carpets and borders.
    {
        const int a = zone.draw() % 150 + 50, b = zone.draw() % 150 + 50, c = zone.draw() % 150 + 50;
        fl.tint = {(float)c, (float)b, (float)a};
    }
    {
        const int a = zone.draw() % 100 + 150, b = zone.draw() % 100 + 150, c = zone.draw() % 100 + 150;
        fl.border = {(float)c, (float)b, (float)a};
    }
    // 3. The carpet.
    if (!fl.level && floors == 1 && (roof == 1 || roof == 8 || roof == 9) && t != 26)
        fl.carpet = zone.draw() % 4 == 0;
    fl.carpetW = zone.draw() % 5 + 8;
    fl.carpetD = zone.draw() % 5 + 8;
    fl.floorPattern = fl.level && zone.place.structure == 42 ? 0 : 1;
    // 5. The neighbours' floors.
    if (!fl.level) {
        for (int d = 0; d < 8; d++) {
            const int32_t nx = cx + SIDE_OFFSET[d].x, ny = cy + SIDE_OFFSET[d].y;
            fl.has[d] = rules.floorExists(nx, ny, f);
            fl.nz[d] = nB[d] + rules.floorOffset(nx, ny) + floorHeight(nT[d]) * f;
            if ((t == 25 || roof == 9) && d <= NORTH) {
                fl.has[d] = true;
                fl.nz[d] = rules.baseHeight(nx, ny, t) + rules.floorOffset(nx, ny) + floorHeight(nT[d]) * f;
            }
            fl.nTop[d] = floorHeight(nT[d]) * (f + 1) + fl.nz[d];
            fl.nBottom[d] = fl.nz[d];
        }
        fl.top0 = base + (f + 1) * F;
        fl.bottom0 = fl.z0;
    } else {
        for (int d = 0; d < 8; d++) {
            const int32_t nx = cx + SIDE_OFFSET[d].x, ny = cy + SIDE_OFFSET[d].y;
            const CastleCell nc = rules.castleCell(nx, ny);
            fl.has[d] = nc.kind != 0;
            if (!fl.has[d])
                continue;
            fl.nz[d] = rules.castleFloor(nx, ny);
            fl.nTop[d] = fl.nz[d] + castleRoomRise(nc.style);
            fl.nDepth[d] = castleRoomDepth(nc.kind);
            fl.nBottom[d] = rules.castleBlockTop(nx, ny) - fl.nDepth[d];
        }
        fl.top0 = castleTop;
        fl.bottom0 = castleBottom - fl.depth0;
    }
    // 6.
    fl.floorMin = fl.bottom0, fl.maxTop = fl.top0;
    for (int d = 0; d < 8; d++)
        if (fl.has[d])
            fl.floorMin = lo32(fl.floorMin, fl.nBottom[d]), fl.maxTop = hi32(fl.maxTop, fl.nTop[d]);
    fl.innerX0 = fl.has[WEST] ? 0 : 1, fl.innerX1 = fl.has[EAST] ? 16 : 15;
    fl.innerY0 = fl.has[SOUTH] ? 0 : 1, fl.innerY1 = fl.has[NORTH] ? 16 : 15;
    // 7.
    if (!fl.level) {
        if (roof == 1 || roof == 2 || roof == 8)
            fl.doorWidth = 10;
        else if (roof == 7 && t != 78 && t != 79)
            fl.doorWidth = 8;
        if (t == 25)
            fl.doorWidth = 14, fl.wall = 1;
        else if (oneOf(t, {13, 16, 26, 49, 50, 51, 53, 57, 60, 61, 63, 64, 65, 86, 94, 95}) || roof == 9)
            fl.doorWidth = 16, fl.wall = 0;
    } else if (k.style == 1) {
        fl.wall = 1, fl.dome = true;
    }
    // 9. A floor border.
    if (fl.level && k.detail == 7) {
        fl.ring = true;
        for (int d = 0; d < 8; d++)
            fl.ringCodes[d] = rules.castleCell(cx + SIDE_OFFSET[d].x, cy + SIDE_OFFSET[d].y).detail;
    } else if (!fl.level && !fl.carpet && t == 45 && rules.fortressFloorCode(cx, cy, f) == 7) {
        fl.ring = true;
        for (int d = 0; d < 8; d++)
            fl.ringCodes[d] = rules.fortressFloorCode(cx + SIDE_OFFSET[d].x, cy + SIDE_OFFSET[d].y, f);
    }
}

void BuildingBuilder::floorColumns(Floor &fl) {
    const uint32_t style = zone.place.style;
    const bool keeps = keepsHeight(t, style);
    const int f = fl.f;
    auto inDoor = [&](int32_t v) { return (16 - fl.doorWidth) / 2 <= v && v < (fl.doorWidth + 16) / 2; };
    // The 3 x 3 cells around, for the castle level's blend: castle cells, floors and tops.
    CastleCell around[9];
    int32_t aroundZ[9] = {}, aroundTop[9] = {};
    if (fl.level) {
        static constexpr int INDEX[8] = {1, 7, 3, 5, 0, 6, 2, 8};
        around[4] = k, aroundZ[4] = fl.z0, aroundTop[4] = fl.top0;
        for (int d = 0; d < 8; d++) {
            around[INDEX[d]] = rules.castleCell(cx + SIDE_OFFSET[d].x, cy + SIDE_OFFSET[d].y);
            aroundZ[INDEX[d]] = fl.nz[d], aroundTop[INDEX[d]] = fl.nTop[d];
        }
    }
    Colour ownLight;
    const bool ownHasLight = castleLightColour(k.kind, ownLight);
    const Colour ownFirst = castleFirstColour(k.style, zone.place.structure);
    const Colour ownSecond = castleSecondColour(k.style, zone.place.structure);
    const uint8_t floorFlags = fl.level ? 0x22 : 2;
    const int32_t wall = fl.wall;

    for (int32_t lx = 0; lx < 16; lx++)
        for (int32_t ly = 0; ly < 16; ly++) {
            // a.
            bool pillar = (lx < wall && ly < wall && !fl.has[SOUTH_WEST]) ||
                          (lx >= 16 - wall && ((ly < wall && !fl.has[SOUTH_EAST]) || (ly >= 16 - wall && !fl.has[NORTH_EAST]))) ||
                          (lx < wall && ly >= 16 - wall && !fl.has[NORTH_WEST]);
            // b.
            int32_t floorZ = fl.z0;
            int32_t ceiling = fl.level ? castleTop - 1 : (f + 1) * F - 1 + base;
            // c. The four sides.
            bool skip = false;
            const struct {
                Side d;
                int32_t along, depth;
                bool far;
            } sides[4] = {{WEST, ly, lx, false}, {EAST, ly, 15 - lx, true}, {SOUTH, lx, ly, false}, {NORTH, lx, 15 - ly, true}};
            for (const auto &s : sides) {
                if (!fl.has[s.d]) {
                    if (s.depth < wall)
                        pillar = true;
                    continue;
                }
                const bool door = inDoor(s.along);
                const int32_t dz = fl.z0 - fl.nz[s.d], kk = dz <= 5 ? 2 : 1;
                if (door && s.depth + (s.far ? 1 : 0) <= dz * kk && fl.depth0 == 0 && fl.nDepth[s.d] == 0 && !special)
                    floorZ = lo32(floorZ, fl.z0 - (dz * kk - s.depth) / kk);
                if (s.depth < wall) {
                    if (!door && (fl.z0 > fl.nz[s.d] || t == 25)) {
                        skip = true;
                        break;
                    }
                    if (floorHeight(nT[s.d]) != 0)
                        ceiling = lo32(ceiling, floorHeight(nT[s.d]) * (f + 1) + nB[s.d] - 1);
                }
            }
            if (skip)
                continue;
            // d. The walls.
            const int32_t wx = 16 * cx + lx, wy = 16 * cy + ly;
            double zz = fl.z0, tt = fl.top0;
            const bool outsideInner = !(lx >= fl.innerX0 && lx < fl.innerX1 && ly >= fl.innerY0 && ly < fl.innerY1);
            if (!fl.level && (!keeps || (roofed(t) && outsideInner))) {
                m.oneColour(colA, fl.floorPattern);
            } else {
                Colour c0 = colA, c1 = colA, c2 = colA;
                if (fl.level) {
                    double L[3] = {}, lw = 0, A[3] = {}, B[3] = {}, cw = 0, zs = 0, ts = 0;
                    for (int32_t gx = floorDiv(wx - 16, 4); gx <= floorDiv(wx + 16, 4); gx++)
                        for (int32_t gy = floorDiv(wy - 16, 4); gy <= floorDiv(wy + 16, 4); gy++) {
                            const int32_t dx = lo32(wx + 16, 16 * gx + 16) - hi32(wx - 16, 16 * gx);
                            const int32_t dy = lo32(wy + 16, 16 * gy + 16) - hi32(wy - 16, 16 * gy);
                            if (!(dx > 0 && dy > 0))
                                continue;
                            const int index = (gx - cx + 1) * 3 + (gy - cy + 1);
                            const CastleCell &g = around[index];
                            if (g.kind == 0)
                                continue;
                            double wgt = (double)(dy * dx) * 0.00390625;
                            wgt = wgt * wgt;
                            Colour light;
                            const Colour first = castleFirstColour(g.style, zone.place.structure);
                            if (castleLightColour(g.kind, light)) {
                                L[0] += light.r * wgt, L[1] += light.g * wgt, L[2] += light.b * wgt;
                                lw += wgt;
                            } else {
                                const double w2 = wgt * 0.01;
                                L[0] += first.r * w2, L[1] += first.g * w2, L[2] += first.b * w2;
                                lw += w2;
                            }
                            cw += wgt;
                            zs += aroundZ[index] * wgt;
                            ts += aroundTop[index] * wgt;
                            const int gt = rules.type(gx, gy);
                            if (gt >= 72 && gt <= 74) {
                                const Colour wc = wallColour(gt, gx, gy, colours());
                                A[0] += wc.r * wgt, A[1] += wc.g * wgt, A[2] += wc.b * wgt;
                                B[0] += wc.r * wgt, B[1] += wc.g * wgt, B[2] += wc.b * wgt;
                            } else {
                                const Colour second = castleSecondColour(g.style, zone.place.structure);
                                A[0] += first.r * wgt, A[1] += first.g * wgt, A[2] += first.b * wgt;
                                B[0] += second.r * wgt, B[1] += second.g * wgt, B[2] += second.b * wgt;
                            }
                        }
                    if (cw > 0.0001) {
                        zz = zs / cw, tt = ts / cw;
                        c1 = {(float)(A[0] / cw), (float)(A[1] / cw), (float)(A[2] / cw)};
                        c2 = {(float)(B[0] / cw), (float)(B[1] / cw), (float)(B[2] / cw)};
                    } else {
                        c1 = ownFirst, c2 = ownSecond;
                    }
                    if (lw > 0)
                        c0 = {(float)(L[0] / lw), (float)(L[1] / lw), (float)(L[2] / lw)};
                    else
                        c0 = ownHasLight ? ownLight : ownFirst;
                }
                m.slots(c0, fl.floorPattern, (float)(zz - 10.0), c1, fl.floorPattern, (float)zz, c2, fl.floorPattern, (float)tt);
                // The wall column (the foundation already fills the columns of the first branch).
                if (fl.level) {
                    m.box(lx, ly, fl.floorMin, 1, 1, fl.maxTop - fl.floorMin + 1);
                } else {
                    m.writeMode = writemode::NOT_PROTECTED;
                    m.box(lx, ly, fl.floorMin, 1, 1, fl.top0 - fl.floorMin + 1);
                    m.writeMode = 0;
                }
            }
            // e.
            if (pillar)
                continue;
            // f. Stairs.
            if (fl.stairs == 1 && f > 0) {
                const int32_t h = (16 - F) / 2;
                if (((doorSide == 2 && lx <= 4) || (doorSide == 1 && lx >= 11)) && h <= ly && ly < 16 - h) {
                    floorZ = F * (f - 1) - h + rules.floorOffset(cx, cy) + ly + base;
                    m.put(lx, ly, floorZ - 2, m.block(lx, ly, floorZ - 2, blocktype::ROCK, 2));
                }
                if (((doorSide == 8 && ly <= 4) || (doorSide == 4 && ly >= 11)) && h <= lx && lx < 16 - h) {
                    floorZ = F * (f - 1) - h + rules.floorOffset(cx, cy) + lx + base;
                    m.put(lx, ly, floorZ - 2, m.block(lx, ly, floorZ - 2, blocktype::ROCK, 2));
                }
            }
            // g.
            const float lightF = maxOf((noiseAt(wx, wy, 0.05, 0, 0.05, 0) * 1.5f + 0.5f) * 127.5f, 0.0f);
            const uint8_t light = (uint8_t)trunc32(lightF);
            // h.
            if (attachedHouse(t) && !fl.level) {
                if (roof != 8)
                    m.oneColour(timberColour(colours()), posMod(cx + cy, 2) + 4);
                else
                    m.oneColour(fl.floorColour, pattern::STONE);
            } else if (!fl.level) {
                m.twoColours(colB, 1, 0, fl.floorColour, 1, 10, true);
            }
            // i. The floor.
            auto lay = [&](int32_t z, Colour c, uint8_t type, uint8_t flags) { m.put(lx, ly, z, packed(c, light, type, flags)); };
            const int32_t fz = fl.depth0 != 0 ? castleBottom - fl.depth0 : floorZ;
            const uint8_t floorType = f > 0 ? (uint8_t)(f + 23) : blocktype::ROCK;
            if (fz < floorZ) {
                lay(fz - 1, m.colourAt(lx, ly, fz - 1), blocktype::ROCK, floorFlags | 8);
            } else if (!(!fl.level && (t == 75 || t == 80 || t == 85))) {
                lay(fz - 1, m.colourAt(lx, ly, fz - 1), floorType, floorFlags);
                if (fl.carpet) {
                    const int32_t x0 = (16 - fl.carpetW) / 2, x1 = (fl.carpetW + 16) / 2;
                    const int32_t y0 = (16 - fl.carpetD) / 2, y1 = (fl.carpetD + 16) / 2;
                    if (lx >= x0 && lx < x1 && ly >= y0 && ly < y1) {
                        const bool edge = ((lx == x0 + 1 || lx == x1 - 2) && y0 + 1 <= ly && ly < y1 - 1) ||
                                          ((ly == y0 + 1 || ly == y1 - 2) && x0 + 1 <= lx && lx < x1 - 1);
                        lay(fz - 1, edge ? fl.border : fl.tint, floorType, floorFlags);
                    }
                } else if (fl.ring) {
                    const int r = floorBorder(lx, ly, fl.ringCodes);
                    if (r == 2)
                        lay(fz - 1, {200, 200, 100}, floorType, floorFlags);
                    else if (r == 1)
                        lay(fz - 1, {200, 50, 100}, floorType, floorFlags);
                }
            }
            // j. A dome.
            if (fl.dome) {
                tt -= (double)((noiseAt(wx, wy, 0.1, 808348, 0.1, 8553) + 1.0f) * 3.0f);
                const float q = 0.125f;
                float e = 1.0f;
                if (!fl.has[WEST] && lx < 8)
                    e = minOf(e, (float)lx * q);
                if (!fl.has[EAST] && lx >= 8)
                    e = minOf(e, (float)(15 - lx) * q);
                if (!fl.has[SOUTH] && ly < 8)
                    e = minOf(e, (float)ly * q);
                if (!fl.has[NORTH] && ly >= 8)
                    e = minOf(e, (float)(15 - ly) * q);
                if (!fl.has[SOUTH_WEST] && lx < 8 && ly < 8)
                    e = minOf(e, maxOf((float)ly * q, (float)lx * q));
                if (!fl.has[SOUTH_EAST] && lx >= 8 && ly < 8)
                    e = minOf(e, maxOf((float)ly * q, (float)(15 - lx) * q));
                if (!fl.has[NORTH_EAST] && lx >= 8 && ly >= 8)
                    e = minOf(e, maxOf((float)(15 - ly) * q, (float)(15 - lx) * q));
                if (!fl.has[NORTH_WEST] && lx < 8 && ly >= 8)
                    e = minOf(e, maxOf((float)(15 - ly) * q, (float)lx * q));
                float tw = minOf(1.0f - e, 1.0f);
                tw = tw * (tw * tw);
                ceiling = (int32_t)std::floor((double)(1.0f - tw) * (tt - 1.0) + (double)tw * zz);
            }
            // k. The room's air.
            for (int32_t z = fz; z <= ceiling - 2; z++)
                m.put(lx, ly, z, Block{0, 0, 0, light, 0, floorFlags});
            // l. Grass.
            const bool grassy = fl.level ? (k.style == 2 || zone.place.structure == 43 || zone.place.structure == 44)
                                         : (f == 0 && grassyFloor(t, style));
            if (grassy && fz == floorZ && noiseAt(wx, wy, 0.1, 89834, 0.1, 12233) > 0.25f)
                m.put(lx, ly, fz, packed(grassColour(wx, wy), light, blocktype::GRASS, floorFlags));
            // m. The ceiling.
            if (fl.level || keeps)
                lay(ceiling - 1, m.colourAt(lx, ly, ceiling - 1), blocktype::ROCK, 2);
        }
}

void BuildingBuilder::afterColumns(Floor &fl) {
    const int f = fl.f;
    // Liquids under the castle level.
    if (fl.level) {
        const int32_t g = castleBottom;
        if (k.kind == 10) {
            m.liquid(0, 0, fl.bottom0, 16, 16, g - fl.bottom0 - 2, 0x22, 1);
        } else if (k.kind == 11) {
            m.foundation(0, 0, g - 5, 16, 16, 1, 0, 0);
            m.liquid(0, 0, g - 4, 16, 16, 1, 0x22, 2);
            for (int32_t x = 0; x < 16; x++)
                for (int32_t y = 0; y < 16; y++)
                    for (int32_t z = g - 3; z <= g - 1; z++) {
                        Block b = m.get(x, y, z);
                        b.glow = 255;
                        m.put(x, y, z, b);
                    }
        } else if (k.kind == 12) {
            m.foundation(0, 0, g - 5, 16, 16, 1, 0, 0);
            m.liquid(0, 0, g - 4, 16, 16, 1, 0x22, 3);
        }
    }
    // Spiral stairs.
    if (fl.stairs == 2) {
        int32_t bottom = (f - 1) * F + base, rise = F;
        int32_t sx = rules.stairs(cx - 1, cy, f) == 2 ? 0 : 8, sy = rules.stairs(cx, cy - 1, f) == 2 ? 0 : 8;
        if (rules.stairs(cx + 1, cy, f) == 2)
            sx += 8;
        if (rules.stairs(cx, cy + 1, f) == 2)
            sy += 8;
        int32_t r = 1, w = 5;
        if (!(sx == 8 && sy == 8))
            r = 5, w = 7;
        if (special && f == 0) {
            m.slots(castleFirstColour(k.style, zone.place.structure), 1, (float)castleZ, castleSecondColour(k.style, zone.place.structure), 1,
                    (float)castleTop, fl.floorColour, 1, (float)(fl.z0 - 1));
            m.box(0, 0, castleTop - 1, 16, 16, fl.z0 - castleTop);
            bottom = castleZ, rise = fl.z0 - castleZ;
        } else {
            m.oneColour(fl.floorColour, pattern::STONE);
        }
        const int32_t steps = rise - 1, turn = 2 * r + 1;
        int32_t z = bottom + 1;
        for (int32_t qq = steps; qq >= 0; qq--, z++) {
            int32_t q = qq / turn, pos = qq % turn;
            if (qq == steps)
                q = (qq - 1) / turn, pos = (qq - 1) % turn + 1;
            int32_t x, y;
            switch (posMod(q, 4)) {
            case 0: x = pos - r - w + sx, y = sy - r - w; break;
            case 1: x = sx + r, y = pos - r - w + sy; break;
            case 2: x = sx - pos + r, y = sy + r; break;
            default: x = sx - r - w, y = sy - pos + r; break;
            }
            m.air(x, y, z, w, w, 10, 0);
            m.box(x, y, z - 2, w, w, 2);
        }
        m.box(sx - r, sy - r, bottom, 2 * r, 2 * r, rise);
    }
    // The beam and its object.
    const int code = fl.level ? k.detail : t == 45 ? rules.fortressFloorCode(cx, cy, f) : 0;
    int kind = -1;
    switch (code) {
    case 2: kind = 8; break;
    case 3: kind = 9; break;
    case 4: kind = 10; break;
    case 5: kind = zone.planner.itemForCell(cx, cy); break;
    default: break;
    }
    if (kind <= 0)
        return;
    int32_t down = 0;
    while (down < 30 && isOpen(zone.read(16 * cx + 8, 16 * cy + 8, fl.z0 - 1 - down)))
        down++;
    m.twoColours(fl.floorColour, 1, (float)fl.z0, fl.floorColour2, 1, (float)fl.top0, false);
    const int32_t run = 16 - 2 * fl.wall, zb = fl.z0 - down + run, hb = fl.top0 - fl.z0 + down - run;
    const bool alongX = fl.has[SOUTH] && fl.has[NORTH];
    if (alongX)
        m.box(fl.wall, 7, zb, run, 2, hb);
    else
        m.box(7, fl.wall, zb, 2, run, hb);
    StaticObject beam = makeObject(kind, dotsOf(16 * cx + 8), dotsOf(16 * cy + 8), dotsOf(fl.z0 - down), alongX ? 0 : 1);
    beam.scale = (float)run;
    beam.swing = 1000.0f;
    zone.keepIfInside(beam);
}

// --- §5 ------------------------------------------------------------------------------------------

void BuildingBuilder::outdoorSpot(int d, float along, float modelAlong, int turnOverride) {
    if (!(zone.draw() & 1))
        return;
    const bool xWall = d < 2;
    const int64_t mid = dotsOf(16 * (xWall ? cy : cx) + 8);
    const int64_t edge = dotsOf(16 * (xWall ? cx : cy) + ((d & 1) ? 16 : 0));
    const int64_t out = (d & 1) ? 1 : -1;
    auto ground = [&](int64_t x, int64_t y) {
        return dotsOf(floorToInt(groundHeight(blocksTowardZero(x), blocksTowardZero(y), 0)) + 1);
    };
    static constexpr int FURNITURE[7] = {38, 39, 37, 40, 74, 28, 30};
    if (zone.draw() & 1) {
        if (roof != 1)
            return;
        int type = FURNITURE[zone.draw() % 7];
        if (type == 74)
            type = 74 + zone.draw() % 9;
        const int64_t across = edge + out * dotsOf(0.5f) + out * dotsOf(objectSize(type).y * 0.5f);
        const int64_t a = mid + dotsOf(along);
        const int64_t x = xWall ? across : a, y = xWall ? a : across;
        StaticObject o = makeObject(type, x, y, ground(x, y), (uint8_t)(turnOverride >= 0 ? turnOverride : WALL_TURN[d]));
        zone.settleAndKeep(o);
        return;
    }
    Model mo;
    mo.type = 0x36 + zone.draw() % 3;
    mo.r = mo.g = mo.b = 1.0f;
    const int64_t across = edge + out * dotsOf(0.5f);
    const int64_t a = mid + dotsOf(modelAlong);
    mo.x = xWall ? across : a, mo.y = xWall ? a : across;
    mo.z = ground(mo.x, mo.y);
    mo.size = zone.fraction(0.05f) + 0.15f;
    mo.yaw = WALL_ANGLE[d];
    zone.keepIfInside(mo);
}

int BuildingBuilder::doorObjectType() const {
    const Int2 c1 = zone.place.cell;
    int type = (sub == 4 || roof == 4) ? 5 : 4;
    if (roof == 5) {
        type = 8;
        if (!(random(cellKey(c1.x, c1.y) + 0x10581) & 1)) {
            // The verified reciprocal quotient has the same low two bits as R.
            if ((cellRandom(c1, 0x29fa4) & 3) != 0 && !zone.planner.items.empty())
                type = zone.planner.items[(size_t)((uint32_t)cellRandom(c1, 0x89d85) % (uint32_t)zone.planner.items.size())];
        }
    } else if (roof == 4) {
        if (signDraw % 4 == 0 && zone.planner.role != 12 && zone.planner.role != 10)
            type = zone.planner.itemForCell(cx, cy);
    } else if (roof == 3 || roof == 6 || roof == 9 || roof == 10) {
        type = -1;
    }
    if (t == 76 || t == 81 || t == 100) {
        type = -1;
        if (t == 100 || !(random(cellKey(c1.x, c1.y) + 0x134cb8) & 1))
            type = zone.planner.itemForCell(cx, cy);
    }
    return type;
}

void BuildingBuilder::wallsDoorsWindows(Floor &fl) {
    const int f = fl.f;
    m.scheme = s3;
    const bool openings = t != 25 && t != 26 && (roof == 1 || roof == 2 || roof == 4 || roof == 5 || roof == 8);
    const int doorType = doorObjectType();
    const int32_t doorH = roof == 5 ? 10 : (roof == 3 || roof == 4 || roof == 6 || roof == 9 || roof == 10 || t == 76 || t == 81 || t == 100) ? 6 : 4;
    const int32_t windowH = roof == 1 ? (sub == 4 ? 4 : 2) : ((roof >= 3 && roof <= 6) || roof == 10) ? 4 : 2;
    const bool flowerBoxes = roof == 1 && sub != 4 && sub != 6;
    const int32_t wall = fl.wall, z0 = fl.z0;
    if (!openings && !hasDoor(t))
        return;
    for (int d = 0; d < 4; d++) {
        if (isBuilding(nT[d]))
            continue;
        const bool front = f == 0 && hasDoor(t) && doorPick == (1 << d);
        int32_t w = zone.draw() % 3;
        bool furnish = false;
        if (front) {
            if (doorH > 4 || w == 0)
                w = 1;
            furnish = true;
        } else if (w > 0) {
            furnish = true;
        } else {
            if (timberFrame) {
                m.scheme = s3;
                static constexpr Int2 POST[4] = {{0, 8}, {15, 8}, {8, 0}, {8, 15}};
                m.box(POST[d].x, POST[d].y, z0, 1, 1, F);
            }
            if (f == 0) {
                outdoorSpot(d, -3.51f, -3.51f, -1);
                outdoorSpot(d, 3.51f, 3.51f, -1);
            }
        }
        if (f == 0 && furnish && openings && !(front && doorH > 4)) {
            if (w == 1) {
                outdoorSpot(d, -3.51f, -3.51f, d == 1 ? 3 : -1);
                outdoorSpot(d, 3.51f, 3.51f, -1);
            } else if (w == 2) {
                outdoorSpot(d, (d & 1) ? 0.01f : -0.01f, 0.01f, -1);
            }
        }
        const bool xWall = d < 2;
        const int64_t edge = dotsOf(16 * (xWall ? cx : cy) + ((d & 1) ? 16 : 0));
        const int64_t inward = (d & 1) ? -dotsOf(0.5f) : dotsOf(0.5f);
        auto inWall = [&](int32_t c, int64_t across, int32_t z, int type) {
            const int64_t along = dotsOf(16 * (xWall ? cy : cx) + c);
            return makeObject(type, xWall ? across : along, xWall ? along : across, dotsOf(z), (uint8_t)WALL_TURN[d]);
        };
        for (int32_t kk = 0; kk < w; kk++) {
            const int32_t c = 16 / w / 2 + kk * 16 / w;
            if (front && kk == 0) {
                const int32_t h = doorH, at = c - h / 2;
                switch (d) {
                case 0:
                    m.box(0, at - 1, z0, wall, h + 2, h + 1);
                    m.air(0, at, z0, wall, h, h, 0);
                    m.ramp(true, -16, at - 1, z0 + 1, 16, h + 2, -16, 0, 0, 0);
                    break;
                case 1:
                    m.box(16 - wall, at - 1, z0, wall, h + 2, h + 1);
                    m.air(16 - wall, at, z0, wall, h, h, 0);
                    m.ramp(true, 16, at - 1, z0, 16, h + 2, 0, -16, 0, 0);
                    break;
                case 2:
                    m.box(at - 1, 0, z0, h + 2, wall, h + 1);
                    m.air(at, 0, z0, h, wall, h, 0);
                    m.ramp(false, at - 1, -16, z0 + 1, h + 2, 16, -16, 0, 0, 0);
                    break;
                default:
                    m.box(at - 1, 16 - wall, z0, h + 2, wall, h + 1);
                    m.air(at, 16 - wall, z0, h, wall, h, 0);
                    m.ramp(false, at - 1, 16, z0, h + 2, 16, 0, -16, 0, 0);
                    break;
                }
                if (doorType >= 0) {
                    StaticObject door = inWall(c, edge + inward, z0, doorType);
                    door.scale = (float)doorH / objectSize(doorType).x;
                    door.swing = 1000.0f;
                    zone.keepIfInside(door);
                }
                // Outside the door: a lamp and a sign.
                const int64_t out = edge + ((d & 1) ? DOTS : -DOTS);
                const int64_t rowStart = dotsOf(16 * (xWall ? cy : cx));
                auto place = [&](Model &mo, int64_t across, int64_t along) {
                    mo.x = xWall ? across : along, mo.y = xWall ? along : across;
                };
                if (attachedHouse(t)) {
                    Model lamp;
                    lamp.type = sub == 4 ? 0x19 : 0x18;
                    place(lamp, out, rowStart + dotsOf(c));
                    lamp.z = dotsOf(z0 + doorH);
                    lamp.yaw = WALL_ANGLE[d];
                    if (lamp.type == 0x19)
                        lamp.r = 0.5f, lamp.g = 1.0f, lamp.b = 0.4f;
                    else
                        lamp.r = 1.0f, lamp.g = 0.9f, lamp.b = 0.4f;
                    lamp.size = 0.1f, lamp.flags = modelflag::LIGHT;
                    zone.keepIfInside(lamp);
                }
                const int block = townBlock(zone.world, cx, cy, zone.place.place.kind);
                if (block >= 2 && block <= 9) {
                    static constexpr int SIGNS[8] = {0x2e, 0x2f, 0x31, 0x30, 0x32, 0x33, 0x34, 0x35};
                    Model sign;
                    sign.type = SIGNS[block - 2];
                    sign.r = 1.0f, sign.g = 0.9f, sign.b = 0.4f, sign.flags = 2;
                    const int64_t along = rowStart + ((d & 1) ? dotsOf(c + doorH / 2) + 98304 : dotsOf(c - doorH / 2) - 98304);
                    int64_t across = out;
                    place(sign, across, along);
                    sign.z = dotsOf(z0 - 1 + doorH);
                    sign.yaw = WALL_ANGLE[d];
                    if (!isOpen(zone.read(blocksOf(sign.x), blocksOf(sign.y), blocksOf(sign.z)))) {
                        across += (d & 1) ? DOTS : -DOTS;
                        place(sign, across, along);
                    }
                    sign.size = 0.06666667f;
                    zone.keepIfInside(sign);
                }
            } else if (openings) {
                switch (d) {
                case 0:
                    m.box(0, c - 2, z0, wall, 4, windowH + 2);
                    m.air(0, c - 1, z0 + 1, wall, 2, windowH, 0);
                    break;
                case 1:
                    m.box(16 - wall, c - 2, z0, wall, 4, windowH + 2);
                    m.air(16 - wall, c - 1, z0 + 1, wall, 2, windowH, 0);
                    break;
                case 2:
                    m.box(c - 2, 0, z0, 4, wall, windowH + 2);
                    m.air(c - 1, 0, z0 + 1, 2, wall, windowH, 0);
                    break;
                default:
                    m.box(c - 2, 16 - wall, z0, 4, wall, windowH + 2);
                    m.air(c - 1, 16 - wall, z0 + 1, 2, wall, windowH, 0);
                    break;
                }
                StaticObject pane = inWall(c, edge + inward, z0 + 1, windowH == 4 ? 7 : 6);
                pane.swing = 1000.0f;
                zone.keepIfInside(pane);
                if (flowerBoxes)
                    zone.keepIfInside(inWall(c, edge - inward, z0, 0x3c + zone.draw() % 3));
            }
        }
    }
}

void BuildingBuilder::ceilingLamp(Floor &fl) {
    if (!insideZone() || !(attachedHouse(t) || roof == 9))
        return;
    const bool drawn = zone.draw() % 4 == 0;
    const bool lamp = drawn || hasDoor(t) || (townBlock(zone.world, cx, cy, zone.place.place.kind) == 2 && fl.f == 0);
    if (!lamp || t == 88)
        return;
    Model mo;
    mo.type = 0x17;
    mo.r = mo.g = mo.b = 1.0f;
    if (zone.place.culture == 4)
        mo.type = 0x1a, mo.r = 0.5f, mo.g = 1.0f, mo.b = 0.4f;
    mo.x = dotsOf(16 * cx + 8) + 32768, mo.y = dotsOf(16 * cy + 8) + 32768;
    mo.z = dotsOf(base + (fl.f + 1) * F - 3);
    for (int n = 0; n < 4; n++) {
        if (!isOpen(zone.read(blocksOf(mo.x), blocksOf(mo.y), blocksOf(mo.z + DOTS)))) {
            mo.size = 0.1f, mo.flags = modelflag::LIGHT, mo.yaw = 0.0f;
            zone.models.push_back(mo);
            return;
        }
        mo.z += DOTS;
    }
}

// --- §7 ------------------------------------------------------------------------------------------

void BuildingBuilder::cornerPosts() {
    if (!((roof == 3 || roof == 4 || roof == 6 || roof == 9 || roof == 10 || t == 16 || sub == 1 || sub == 3 || sub == 4 || sub == 6) &&
          t != 26))
        return;
    const int kind = (roof == 1 || roof == 9 || roof == 10) ? ((sub == 3 || sub == 6) ? 3 : 0)
                                                          : (int)(xorshift(zone.place.style + 0xdaad) % 3);
    m.twoColours(colB, 1, 0, colA, 1, 10, true);
    if (kind == 3)
        m.oneColour(scaled(colA, 0.8f), pattern::PLAIN);
    if (!(oneOf(t, {13, 16, 49, 50, 51, 53, 60, 86, 94, 95, 97}) || roof == 9))
        m.writeMode = writemode::NOT_SOLID | writemode::NOT_PROTECTED;
    static constexpr int CORNERS[4][2] = {{0, 0}, {16, 0}, {0, 16}, {16, 16}};
    for (const auto &c : CORNERS) {
        const int dx = c[0] ? 1 : -1, dy = c[1] ? 1 : -1;
        int32_t lo = base, hi = buildingHeight(t) + base;
        for (Int2 n : {Int2{cx + dx, cy}, Int2{cx, cy + dy}, Int2{cx + dx, cy + dy}}) {
            const int nt = rules.type(n.x, n.y);
            if (!isBuilding(nt))
                continue;
            const int32_t nb = rules.baseHeight(n.x, n.y);
            lo = lo32(lo, nb);
            hi = hi32(hi, buildingHeight(nt) + nb);
        }
        switch (kind) {
        case 0: m.squarePost(c[0], c[1], lo - 1, 1, hi - lo - 1, true); break;
        case 1: m.cappedPost(c[0], c[1], lo - 1, 1, hi - lo - 1, true); break;
        case 2: m.roundPost(c[0], c[1], lo - 1, 1, hi - lo - 1, true); break;
        default: m.foundation(c[0] - 1, c[1] - 1, lo - 1, 2, 2, hi - lo + 1, 0, 0); break;
        }
    }
    m.writeMode = 0;
}

// --- §8 ------------------------------------------------------------------------------------------

void BuildingBuilder::roofAndExtras() {
    const uint32_t style = zone.place.style;
    if (keepsHeight(t, style)) {
        const int joins = rules.joins(cx, cy);
        const int32_t x0 = (joins & 1) ? 0 : -overhang, x1 = (joins & 2) ? 16 : 16 + overhang;
        const int32_t y0 = (joins & 4) ? 0 : -overhang, y1 = (joins & 8) ? 16 : 16 + overhang;
        const int ridge = rules.ridgeSides(cx, cy, t, joins);
        m.oneColour(roofColour(t, cx, cy, colours()), pattern::PLAIN);
        const Scheme s5 = m.scheme;
        m.writeMode = writemode::NOT_SOLID | writemode::NOT_PROTECTED;
        if (sub == 1) {
            m.scheme = s1;
            m.box(-3, -3, base + height - 2, 22, 22, 2);
            m.box(-1, -1, base + height - 3, 18, 18, 1);
        } else if (sub == 3) {
            m.oneColour(scaled(colA, 0.8f), pattern::PLAIN);
            m.box(-1, -1, base + height - 2, 18, 18, 2);
        }
        for (int32_t x = x0; x < x1; x++)
            for (int32_t y = y0; y < y1; y++) {
                const int32_t h = BuildingRules::roofHeight(x, y, t, ridge, sub);
                if ((x >= 0 && x <= 15 && y >= 0 && y <= 15) || deepRoof) {
                    m.scheme = s1;
                    for (int32_t kk = deepRoof ? -2 : 0; kk < h; kk++)
                        m.put(x, y, base + height + kk, m.block(x, y, base + height + kk, blocktype::ROCK, 0));
                }
                m.scheme = s5;
                int32_t top = height + h;
                m.put(x, y, base + top, m.block(x, y, base + top, blocktype::ROCK, 0));
                const int32_t around[4] = {BuildingRules::roofHeight(x + 1, y, t, ridge, sub), BuildingRules::roofHeight(x - 1, y, t, ridge, sub),
                                           BuildingRules::roofHeight(x, y + 1, t, ridge, sub), BuildingRules::roofHeight(x, y - 1, t, ridge, sub)};
                bool step = false;
                for (int32_t a : around)
                    step = step || h - a > 1;
                if (step) {
                    top = h + 1 + height;
                    m.put(x, y, base + top, m.block(x, y, base + top, blocktype::ROCK, 0));
                }
                const Mason::World w = m.world(x, y, 0);
                const Place p = placeAt(w.x, w.y);
                const float fb = (float)base;
                if (snowAt(w.x, w.y, warmth(neighbourhood(w.x, w.y), p.kind, p.weight), {fb, fb, fb, fb}))
                    m.put(x, y, base + 1 + top, Block{100, 140, 200, 0, blocktype::SNOW, 1});
            }
        if (t == 88) {
            m.writeMode = 0;
            m.twoColours(colA, 1, (float)(base + height), scaled(colA, 0.5f), 1, (float)(base + height + 25), false);
            m.box(5, 5, base + height - 3, 6, 6, 28);
            m.air(6, 6, base + height - 3, 4, 4, 28, 0);
        }
        m.writeMode = 0;
    }
    m.twoColours(colB, 1, 0, colA, 1, 10, true);
    if (insideZone() && (t == 53 || t == 14 || t == 16 || t == 60))
        m.box(4, 4, rules.baseHeight(cx, cy, t), 8, 8, 2);
    if (t == 57 || t == 61) {
        Mason posts = Mason::atCell(zone, cx, cy);
        const Colour wall = wallColour(t, cx, cy, colours());
        posts.twoColours(wall, 1, 0, wall, 1, 20, true);
        const int32_t b = rules.baseHeight(cx, cy, t);
        for (Int2 c : {Int2{0, 0}, Int2{15, 0}, Int2{15, 15}, Int2{0, 15}})
            posts.squarePost(c.x, c.y, b, 1, 10, true);
    }
    // Objects on plinths, for a cell inside the zone only.
    const int32_t b = rules.baseHeight(cx, cy, t);
    auto middle = [&](int type, int32_t z) {
        zone.keepIfInside(makeObject(type, dotsOf(16 * cx + 8), dotsOf(16 * cy + 8), dotsOf(z), (uint8_t)(zone.draw() % 4)));
    };
    const bool plinths = insideZone();
    if (plinths && t == 53) {
        middle(0, b + 2);
    } else if (plinths && (t == 14 || t == 16 || t == 60)) {
        GroundItem it;
        it.angle = (float)zone.draw() / 32767.0f * 360.0f;
        it.item.category = itemcategory::GENERATED;
        it.item.id = zone.draw() % 2;
        it.item.rarity = (uint8_t)(zone.draw() % 2 + 3);
        it.item.modifier = random((uint32_t)(cx + 8883) + (uint32_t)(cy + 983049823) * 57u);
        it.flags |= PER_PLAYER;
        it.x = dotsOf(16 * cx + 8), it.y = dotsOf(16 * cy + 8), it.z = dotsOf(b + 2);
        zone.items.push_back(it);
    } else if (plinths && t == 51) {
        middle(1, b);
    } else if (plinths && t == 61) {
        middle(2, b);
    }
    // Town squares, for every cell of the building builder, inside the zone or not.
    const int block = townBlock(zone.world, cx, cy, zone.place.place.kind);
    if (block >= 7 && block <= 9) {
        const int u = cellTurn(cx, cy);
        const int64_t y = dotsOf(16 * cy + 4 + zone.draw() % 8) + dotsOf(0.5f);
        const int64_t x = dotsOf(16 * cx + 4 + zone.draw() % 8) + dotsOf(0.5f);
        const uint8_t turn = (uint8_t)(zone.draw() % 4);
        static constexpr int SQUARES[3][4] = {{0, 0x62, 0x63, 0x68}, {0, 0x66, 0x67, 0}, {0, 0x65, 0x64, 0}};
        const int type = u < 0 ? 0 : SQUARES[block - 7][u];
        if (type != 0) {
            StaticObject o = makeObject(type, x, y, dotsOf(base), turn);
            zone.settle(o, true);
            zone.keepIfInside(o);
        }
    }
}

} // namespace worldgen
