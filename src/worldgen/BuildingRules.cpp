// Building, castle and fortress rules, written from docs/specs/worldgen-structures-buildings.md
// §1, §9 and §10.
#include "worldgen/BuildingRules.h"

#include "worldgen/BlockColours.h"
#include "worldgen/Height.h"

#include <cstdlib>

namespace worldgen {
namespace {

int32_t evened(int32_t v) { return 2 * floorDiv(v, 1); }
int32_t placeCellOfCell(int32_t c) { return floorDiv(wrapMul(c, 16), PLACE_SHIFT); }
bool smallStructure(int b) { return b == 41 || b == 43 || b == 44; }

// The corridor of 128-block rooms of a big structure (§9.1).
struct Corridor {
    Int2 c1;          // the place cell, evened when big
    int structure;    // B'
    int dir;          // 0 -x, 1 +x, 2 -y, 3 +y
    int32_t ax, ay;   // the anchor room

    int32_t wobbleX(int32_t t) const { // 0x2aad70
        return t == ax ? ay : trunc32(simplex((double)(t / 3) * 0.5, 10984.0) * 3.0f + (float)ay);
    }
    int32_t wobbleY(int32_t t) const { // 0x2aad10
        return t == ay ? ax : trunc32(simplex((double)(t / 3) * 0.5, 76643.0) * 3.0f + (float)ax);
    }

    bool onCorridor(int32_t x, int32_t y) const { // 0x2b7de0
        int32_t a, b, v;
        switch (dir) {
        case 0: if (x > ax) return false; a = wobbleX(x), b = wobbleX(x - 1), v = y; break;
        case 1: if (x < ax) return false; a = wobbleX(x), b = wobbleX(x + 1), v = y; break;
        case 2: if (y > ay) return false; a = wobbleY(y), b = wobbleY(y - 1), v = x; break;
        default: if (y < ay) return false; a = wobbleY(y), b = wobbleY(y + 1), v = x; break;
        }
        return (v >= a && v <= b) || (v >= b && v <= a);
    }

    int steps(int32_t x, int32_t y) const { // 0x2aa880
        const bool alongX = dir < 2;
        const int32_t p = alongX ? x : y, q = alongX ? y : x, origin = alongX ? ax : ay;
        auto wob = [&](int32_t t) { return alongX ? wobbleX(t) : wobbleY(t); };
        int sum = 0;
        if (dir == 0 || dir == 2) {
            if (p > origin)
                return 0;
            const int32_t n = origin - p;
            for (int32_t i = 0; i <= n; i++) {
                const int32_t t = origin - i;
                const int32_t cur = wob(t), prev = i == n ? q : wob(t - 1);
                sum += std::abs(prev - cur) + 1;
            }
        } else {
            if (p < origin)
                return 0;
            for (int32_t t = origin; t <= p; t++) {
                const int32_t cur = wob(t);
                const int32_t d = t == p ? q - cur : wob(t + 1) - cur;
                sum += std::abs(d) + 1;
            }
        }
        return sum;
    }

    Int2 end(int in) const { // 0x2b8310 (in 0), 0x2b88a0 (in 1)
        switch (dir) {
        case 0: { const int32_t x = floorDiv(wrapMul(c1.x, 1024), 7) + in; return {x, wobbleX(x - 1)}; }
        case 2: { const int32_t y = floorDiv(wrapMul(c1.y, 1024), 7) + in; return {wobbleY(y - 1), y}; }
        case 1: { const int32_t x = floorDiv(wrapAdd(wrapMul(c1.x, 1024), 0x7ff), 7) - in; return {x, wobbleX(x + 1)}; }
        default: { const int32_t y = floorDiv(wrapAdd(wrapMul(c1.y, 1024), 0x7ff), 7) - in; return {wobbleY(y + 1), y}; }
        }
    }

    CastleRoom corridorRoom(int32_t x, int32_t y) const { // 0x2a04e0
        CastleRoom r;
        if (!onCorridor(x, y))
            return r;
        r.type = 1;
        r.b2 = (structure == 42 || structure == 44) ? 1 : 0;
        r.steps = steps(x, y);
        if (r.steps == 1)
            r.type = 2;
        else if (r.steps == 2)
            r.type = 3;
        else if (Int2{x, y} == end(1))
            r.type = 6;
        else if (Int2{x, y} == end(0))
            r.type = 7;
        else if (!(x == ax && y == ay)) {
            const int32_t v = cellRandom(x, y, 0x6c9c4) & 7;
            static constexpr int TYPES[6] = {9, 10, 13, 14, 8, 4};
            if (v <= 5)
                r.type = TYPES[v];
            if (cellRandom(x, y, 0xab97d) & 1)
                r.flags = 4;
        }
        if (cellRandom(x, y, 0x11a0595) & 1)
            r.flags |= 1;
        if (!smallStructure(structure) && (cellRandom(x, y, 0xe0fae) & 1))
            r.flags |= 2;
        return r;
    }

    CastleRoom room(int32_t x, int32_t y) const { // 0x2aa450
        CastleRoom r = corridorRoom(x, y);
        if (r.type != 0)
            return r;
        r = {};
        if (cellRandom(x, y, 0x71504) % 5 != 0)
            return r;
        CastleRoom neighbours[4] = {corridorRoom(x - 1, y), corridorRoom(x + 1, y), corridorRoom(x, y - 1), corridorRoom(x, y + 1)};
        int sides[4], count = 0;
        for (int i = 0; i < 4; i++)
            if (neighbours[i].type != 0)
                sides[count++] = i + 1;
        if (count == 0)
            return r;
        const int pick = sides[cellRandom(x, y, 0x55260) % count];
        r.side = pick;
        r.steps = neighbours[pick - 1].steps;
        r.flags = 1;
        r.type = cellRandom(x, y, 0x57b071) % 3 != 0 ? 5 : 4;
        r.b2 = r.type == 5 ? 2 : (structure == 42 || structure == 44) ? 1 : 0;
        if (!smallStructure(structure) && (cellRandom(x, y, 0xe0fae) & 1))
            r.flags = 3;
        return r;
    }
};

// The corridor's direction (§9 step 3): away from the place cell's farthest edge.
int corridorDirection(Int2 c1, Int2 m) {
    const int32_t west = m.x - (c1.x << 11);
    const int32_t best = west > 0 ? west : 0;
    int dir = 2;
    int32_t b2 = best;
    const int32_t d = m.y - (c1.y << 11);
    if (d > best)
        b2 = d;
    else
        dir = 0;
    const int32_t e = (c1.x + 2) * 1024 - m.x;
    if (e > b2)
        dir = 1, b2 = e;
    const int32_t f = (c1.y + 2) * 1024 - m.y;
    if (f > b2)
        dir = 3;
    return dir;
}

} // namespace

// --- §1 ------------------------------------------------------------------------------------------

int32_t BuildingRules::baseHeight(int32_t cx, int32_t cy, int t) {
    if (t == 0)
        t = type(cx, cy);
    if ((t >= 45 && t <= 47) || t == 52) {
        const int32_t px = placeCellOfCell(cx), py = placeCellOfCell(cy);
        const Int2 p = cellPoint(px, py, bigCell(px, py));
        return trunc32(groundHeight(p.x, p.y, 0) + 1.0f);
    }
    if (t >= 40 && t <= 43) {
        int32_t best = 0;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) {
                const int32_t h = floorToInt(groundHeight(32 * floorDiv(cx, 1) + 16 * i, 32 * floorDiv(cy, 1) + 16 * j, 0));
                if (h > best)
                    best = h;
            }
        return best;
    }
    const int32_t mx = 16 * cx + 8, my = 16 * cy + 8;
    if (t == 60 || t == 13 || t == 14) {
        const float g = groundHeight(mx, my, 0);
        const float lift = t == 13 ? 44.0f : 80.0f;
        return trunc32(maxOf(g, 0.0f) + lift);
    }
    if (t <= 12)
        return trunc32(groundHeight(mx, my, 0));
    if (t == 94 || t == 95) {
        const Place p = placeAt(mx, my);
        const float b = worldgen::baseHeight(neighbourhood(p.point.x, p.point.y), p.kind, 1.0f);
        return t == 95 ? trunc32(b + 100.0f - 24.0f) : trunc32(b + 1.0f);
    }
    if (t == 86) {
        const int32_t x = type(cx + 1, cy) == 86 ? 16 * cx + 16 : 16 * cx;
        const int32_t y = type(cx, cy + 1) == 86 ? 16 * cy + 16 : 16 * cy;
        return trunc32(groundHeight(x, y, 0) + 2.0f);
    }
    const ColumnValues c = columnValues(mx, my, 0);
    if (c.river > 0.5f) {
        const int32_t f = floorToInt(waterLevel(c.base, c.place) + 1.0f);
        const int32_t v = f / 2 * 2 + 1;
        return v > 0 ? v : 0;
    }
    float h = c.height;
    float corner = groundHeight(16 * cx + 16, 16 * cy + 16, 0);
    for (Int2 q : {Int2{16 * cx, 16 * cy + 16}, Int2{16 * cx + 16, 16 * cy}, Int2{16 * cx, 16 * cy}}) {
        const float g = groundHeight(q.x, q.y, 0);
        corner = corner > g ? corner : g;
    }
    if (h < corner)
        h = corner;
    return floorToInt(maxOf(h + 1.0f, 0.0f) * 0.5f * 2.0f + 2.0f + 0.0f);
}

int32_t BuildingRules::floorOffset(int32_t cx, int32_t cy) {
    const int t = type(cx, cy);
    if (t == 45 || t == 46) {
        const int code = fortressCellCode(cx, cy);
        if (code == 0 || code == 1 || code == 4)
            return 0;
        return trunc32((simplex((double)floorDiv(cx, 2) * 1.5, (double)floorDiv(cy, 2) * 1.5) + 1.0f) * 5.0f);
    }
    if (t >= 72 && t <= 75)
        return -10 * (t - 71);
    if (t >= 77 && t <= 80)
        return -10 * (t - 76);
    if (t >= 82 && t <= 85)
        return -10 * (t - 81);
    return 0;
}

bool BuildingRules::floorExists(int32_t cx, int32_t cy, int f) {
    const int t = type(cx, cy);
    if (f >= mostFloors(t))
        return false;
    if (t != 45)
        return true;
    return fortressCellCode(cx, cy) != 0;
}

int BuildingRules::stairs(int32_t cx, int32_t cy, int f) {
    const int t = type(cx, cy);
    if (t == 47)
        return f > 0 ? 2 : 0;
    if (t == 39 || t == 86 || t == 94 || t == 95)
        return f == 0 ? 2 : f > 0 ? 1 : 0;
    return f > 0 ? 1 : 0;
}

bool BuildingRules::sameBuilding(int32_t ax, int32_t ay, int32_t bx, int32_t by) {
    const int ta = type(ax, ay), tb = type(bx, by);
    return ta == tb && baseHeight(ax, ay) + buildingHeight(ta) == baseHeight(bx, by) + buildingHeight(tb);
}

int BuildingRules::joins(int32_t cx, int32_t cy) {
    int j = 0;
    if (sameBuilding(cx, cy, cx - 1, cy))
        j |= 1;
    if (sameBuilding(cx, cy, cx + 1, cy))
        j |= 2;
    if (sameBuilding(cx, cy, cx, cy - 1))
        j |= 4;
    if (sameBuilding(cx, cy, cx, cy + 1))
        j |= 8;
    return j;
}

int BuildingRules::ridgeSides(int32_t cx, int32_t cy, int t, int joins) {
    const int roof = roofKind(t);
    if (!(t == 25 || roof == 1 || roof == 3 || roof == 4 || roof == 7 || roof == 8))
        return joins;
    bool w = joins & 1, e = joins & 2, s = joins & 4, n = joins & 8;
    if (w != e) {
        if (s != n && (cellRandom(cx, cy, 0x44b2d3) & 1)) {
            s = n = true;
        } else {
            w = e = true;
            if (s != n)
                s = n = true;
        }
    } else if (s != n) {
        s = n = true;
    }
    if (!w && !e && !s && !n) {
        if (cellRandom(cx, cy, 0x2f887fb) & 1)
            w = e = true;
        else
            s = n = true;
    }
    return joins | (w ? 1 : 0) | (e ? 2 : 0) | (s ? 4 : 0) | (n ? 8 : 0);
}

int BuildingRules::roofHeight(int32_t x, int32_t y, int t, int ridge, int sub) {
    if (t == 95)
        return 2;
    const int roof = roofKind(t);
    if (roof == 1 && sub == 5)
        return (x >= 2 && x < 14 && y >= 2 && y < 14) ? sub - 3 : 3;
    const int32_t dx = x - 8, dy = y - 8;
    const int32_t ay = trunc32(std::fabs((float)dy + 0.5f)), ax = trunc32(std::fabs((float)dx + 0.5f));
    int32_t c = ax > ay ? ax : ay;
    if (t == 38 || sub == 6) {
    } else if (t == 40 || t == 43 || t == 47) {
        c = -8;
        if (ridge & 1)
            c = c > dx ? c : dx;
        if (ridge & 2)
            c = c > 7 - x ? c : 7 - x;
        if (ridge & 4)
            c = c > dy ? c : dy;
        if (ridge & 8)
            c = c > 7 - y ? c : 7 - y;
    } else {
        if (((ridge & 1) && x < 8) || ((ridge & 2) && x >= 8))
            c = c < ay ? c : ay;
        if (((ridge & 4) && y < 8) || ((ridge & 8) && y >= 8))
            c = c < ax ? c : ax;
    }
    int32_t h = 8 - c;
    if (sub == 4)
        h *= 2;
    if (t == 38 || t == 43 || t == 47 || t == 81 || t == 91 || t == 96 || t == 100 || roof == 8)
        h *= 2;
    else if (t == 25 || t == 76 || roof == 3 || sub == 6)
        h /= 2;
    return h;
}

// --- §9 ------------------------------------------------------------------------------------------

CastleCell BuildingRules::castleCell(int32_t cx, int32_t cy) {
    const int here = type(cx, cy);
    if (here == 39 || here == 86 || here == 94 || here == 95)
        return {1, 0, 0, 0};
    Int2 c1{placeCellOfCell(cx), placeCellOfCell(cy)};
    const bool big = bigCell(c1.x, c1.y);
    if (big)
        c1 = {evened(c1.x), evened(c1.y)};
    const int kind = placeKind(c1.x, c1.y, big);
    const int structure = bigStructure(c1.x, c1.y, kind, big);
    if (structure == 0)
        return {};
    const Int2 m = cellPoint(c1.x, c1.y, big);
    const Int2 a = structureAnchor(c1.x, c1.y, kind, big);
    const Corridor corridor{c1, structure, corridorDirection(c1, m), floorDiv(a.x, 3), floorDiv(a.y, 3)};
    const int32_t rx = floorDiv(cx, 3), ry = floorDiv(cy, 3);
    const CastleRoom rec = corridor.room(rx, ry);
    if (rec.type == 0)
        return {};

    const uint32_t g = cellKey(rx, ry);
    auto r3 = [&](uint32_t k) { return (int32_t)(random(g + k) % 3u); };
    int32_t a0 = r3(0x4e6289), a1 = r3(0x21d4a), a2 = r3(0x11a43f), a3 = r3(0x14f747);
    const int32_t lx = posMod(cx, 8), ly = posMod(cy, 8);
    auto door = [&](int32_t lo, uint32_t o1, int32_t hi, uint32_t o2, uint32_t o3) {
        const int32_t first = (lo > r3(o1) ? lo : r3(o1)) + 1;
        const int32_t last = 7 - (hi > r3(o2) ? hi : r3(o2));
        return first + (int32_t)(random(g + o3) % (uint32_t)(last - first));
    };
    const int32_t doorW = door(a2, 0x11a43e, a3, 0x14f746, 0x7bf18);
    const int32_t doorS = door(a0, 0x4e6250, a1, 0x21d11, 0x14018);
    const int32_t doorE = door(a2, 0x11a440, a3, 0x14f748, 0x7bf19);
    const int32_t doorN = door(a0, 0x4e62c2, a1, 0x21d83, 0x14051);
    if (rx == corridor.ax && ry == corridor.ay) {
        const int32_t bx = posMod(a.x, 8), by = posMod(a.y, 8);
        if (bx <= a0 + 1)
            a0 = 0;
        if (bx >= 6 - a1)
            a1 = 0;
        if (by <= a2 + 1)
            a2 = 0;
        if (by >= 6 - a3)
            a3 = 0;
    }
    // 7. A passage through a room wall.
    auto passage = [&](int32_t nx, int32_t ny, int recSides, int nbSides, int32_t wall, int32_t c1Coord, bool atEdge,
                       CastleCell &out) {
        const CastleRoom nb = corridor.room(nx, ny);
        if (nb.type == 0 || std::abs(nb.steps - rec.steps) >= 2 || !((recSides >> rec.side) & 1) || !((nbSides >> nb.side) & 1) ||
            evened(floorDiv(wall, PLACE_SHIFT)) != c1Coord)
            return false;
        auto cell = [](int detail, const CastleRoom &r) {
            return CastleCell{8, (uint8_t)detail, (uint8_t)r.b2, (uint8_t)r.steps};
        };
        if (!atEdge)
            out = cell(0, rec);
        else if (rec.b2 == 2 && nb.b2 != 2)
            out = cell(5, nb);
        else if ((rec.type == 4 || rec.type == 6 || rec.type == 8) && nb.steps > rec.steps)
            out = cell(2, rec);
        else if (rec.type == 2 && nb.steps > rec.steps)
            out = cell(6, rec);
        else
            out = cell(0, rec);
        return true;
    };
    // 8. The walls.
    CastleCell out;
    if (lx <= a0) {
        if (ly == doorW && passage(rx - 1, ry, 0b11, 0b101, 128 * rx - 128, c1.x, lx == 0, out))
            return out;
        return {};
    }
    if (ly <= a2) {
        if (lx == doorS && passage(rx, ry - 1, 0b1001, 0b10001, 128 * ry - 128, c1.y, ly == 0, out))
            return out;
        return {};
    }
    if (lx >= 7 - a1) {
        if (ly == doorE && passage(rx + 1, ry, 0b101, 0b11, 128 * (rx + 1), c1.x, lx == 7, out))
            return out;
        return {};
    }
    if (ly >= 7 - a3) {
        if (lx == doorN && passage(rx, ry + 1, 0b10001, 0b1001, 128 * (ry + 1), c1.y, ly == 7, out))
            return out;
        return {};
    }
    // 9. Inside the room.
    const int32_t mx = (a0 - a1 + 8) / 2, my = (a2 - a3 + 8) / 2;
    const int floorKind = cellRandom(cx, cy, 0x7dbcb9a) % 3 != 0 ? 1 : 6;
    int deco = 0;
    if (!(here == 39 || here == 75 || here == 80 || here == 85 || here == 86 || here == 94 || here == 95)) {
        const int32_t h = signedRandom(cellKey(cx / 2, cy / 2) + 0x7a667);
        if ((rec.flags & 1) && cellRandom(cx, cy, 0x468acd) % 3 == 0)
            deco = 1;
        else if ((rec.flags & 2) && h % 2 == 0)
            deco = 7;
        else if (structure == 42 && h % 2 == 0)
            deco = 11;
        else if (structure == 44 && h % 2 == 0)
            deco = 12;
    }
    auto make = [&](int kind, int detail) { return CastleCell{(uint8_t)kind, (uint8_t)detail, (uint8_t)rec.b2, (uint8_t)rec.steps}; };
    const CastleCell special = make(floorKind, deco);
    auto hasRoom = [&](int32_t x, int32_t y) { return corridor.room(x, y).type != 0; };
    // 10.
    if ((rec.type == 2 || rec.type == 3) && lx == a0 + 1 && ly == a2 + 1)
        return make(1, 14);
    // 11.
    if (rec.type >= 9 && rec.type <= 12) {
        if ((lx <= mx && ly == doorW && hasRoom(rx - 1, ry)) || (lx >= mx && ly == doorE && hasRoom(rx + 1, ry)) ||
            (ly <= my && lx == doorS && hasRoom(rx, ry - 1)) || (ly >= my && lx == doorN && hasRoom(rx, ry + 1)) ||
            (lx == mx && ly == my))
            return special;
        return make(rec.type, 0);
    }
    // 12.
    if (rec.type >= 13 && rec.type <= 15) {
        if ((lx == a0 + 1 && ly == doorW && hasRoom(rx - 1, ry)) || (lx == 6 - a1 && ly == doorE && hasRoom(rx + 1, ry)) ||
            (lx == doorS && ly == a2 + 1 && hasRoom(rx, ry - 1)) || (lx == doorN && ly == 6 - a3 && hasRoom(rx, ry + 1)))
            return special;
        const int kind = rec.type == 13 ? 9 : rec.type == 14 ? 11 : 12;
        return make(kind, (cellRandom(cx, cy, 0x54bb5) & 3) != 0 ? 8 : 0);
    }
    // 13.
    const bool centre = lx == mx && ly == my;
    const int keep7 = deco == 7 ? 7 : 0;
    switch (rec.type) {
    case 4: return centre ? make(2, keep7) : make(1, deco);
    case 5: return centre ? make(3, keep7) : make(1, deco);
    case 6: return centre ? make(4, keep7) : make(floorKind, deco);
    case 7: return centre ? make(5, keep7) : make(1, deco);
    case 8: return make(6, deco);
    default: return make(floorKind, deco);
    }
}

// --- §10 -----------------------------------------------------------------------------------------

int32_t BuildingRules::castleBlockTop(int32_t cx, int32_t cy) {
    const int32_t px = placeCellOfCell(cx), py = placeCellOfCell(cy);
    const bool big = bigCell(px, py);
    const int kind = placeKind(px, py, big);
    const Int2 a = structureAnchor(px, py, kind, big);
    const int32_t bx = floorDiv(cx, 3), by = floorDiv(cy, 3);
    if (bx == floorDiv(a.x, 3) && by == floorDiv(a.y, 3) && kind != 45 && kind != 46)
        return baseHeight(a.x, a.y, 0) - 40;
    const int32_t x0 = 128 * bx, y0 = 128 * by, x1 = x0 + 128, y1 = y0 + 128;
    constexpr int LOW = heightmode::NO_VARIANT_SHAPES | heightmode::LOW;
    const float h0 = groundHeight((x1 + x0) / 2, (y1 + y0) / 2, LOW);
    const float h1 = groundHeight(x1, y1, LOW), h2 = groundHeight(x0, y1, LOW), h3 = groundHeight(x1, y0, LOW);
    const float h4 = groundHeight(x0, y0, heightmode::LOW);
    float m = h1 <= h0 ? h1 : h0;
    m = h2 > m ? m : h2;
    m = h3 > m ? m : h3;
    return trunc32((m < h4 ? m : h4) - 40.0f);
}

int32_t BuildingRules::castleFloor(int32_t cx, int32_t cy) {
    int32_t t = castleBlockTop(cx, cy);
    const int32_t lx = posMod(cx, 8), ly = posMod(cy, 8);
    bool side = true;
    Int2 n{cx, cy};
    if (lx == 0)
        n = {cx - 1, cy};
    else if (ly == 0)
        n = {cx, cy - 1};
    else if (lx == 7)
        n = {cx + 1, cy};
    else if (ly == 7)
        n = {cx, cy + 1};
    else
        side = false;
    if (side) {
        const int32_t other = castleBlockTop(n.x, n.y);
        t = ((other > t ? other : t) + t) / 2;
    }
    Int2 c1{placeCellOfCell(cx), placeCellOfCell(cy)};
    const bool big = bigCell(c1.x, c1.y);
    if (big)
        c1 = {evened(c1.x), evened(c1.y)};
    const int kind = placeKind(c1.x, c1.y, big);
    const int structure = bigStructure(c1.x, c1.y, kind, big);
    if (structure == 0)
        return t;
    const Int2 m = cellPoint(c1.x, c1.y, big);
    const Corridor corridor{c1, structure, corridorDirection(c1, m), floorDiv(m.x, 7), floorDiv(m.y, 7)};
    if (corridor.room(floorDiv(cx, 3), floorDiv(cy, 3)).flags & 4)
        t += cellRandom(cx / 3, cy / 3, 0x7b7e8) % 6;
    return t;
}

namespace {
struct Fortress {
    Int2 c1;
    uint32_t g;
    FortressSquare square;
};
Fortress fortressOf(int32_t cx, int32_t cy) {
    const Int2 c1{evened(placeCellOfCell(cx)), evened(placeCellOfCell(cy))};
    const Int2 p = cellPoint(c1.x, c1.y, bigCell(c1.x, c1.y));
    return {c1, cellKey(c1.x, c1.y), fortressSquare(c1, p)};
}
} // namespace

int BuildingRules::fortressCellCode(int32_t cx, int32_t cy) {
    const Fortress f = fortressOf(cx, cy);
    const FortressSquare &q = f.square;
    const Int2 t = q.turned(cx, cy);
    const int32_t x = t.x, y = t.y, ax = q.middle.x, ay = q.middle.y;
    const int32_t half = q.size / 2, x0 = ax - half, y0 = ay - half, x1 = x0 + q.size, y1 = y0 + q.size;
    const uint32_t h = cellKey(evened(placeCellOfCell(x)), evened(placeCellOfCell(y)));
    const int32_t r = (int32_t)(random(h + 0x44a753) % (uint32_t)(q.size - 8)) - half + 4;
    int mark = 0;
    switch (random(h + 0x4d9848) & 3) {
    case 0: if (x == ax - r && y >= y0 + 1 && y <= y0 + 3) mark = 2; break;
    case 1: if (x == ax - r && y >= y1 - 4 && y <= y1 - 2) mark = 2; break;
    case 2: if (y == ay - r && x >= x0 + 1 && x <= x0 + 3) mark = 2; break;
    default: if (y == ay - r && x >= x1 - 4 && x <= x1 - 2) mark = 2; break;
    }
    switch (random(h + 0x311e6) & 3) {
    case 0: if (x == ax && y >= y0 + 4 && y <= y0 + 5) mark = 1; break;
    case 1: if (x == ax && y >= y1 - 6 && y <= y1 - 5) mark = 1; break;
    case 2: if (y == ay && x >= x0 + 4 && x <= x0 + 5) mark = 1; break;
    default: if (y == ay && x >= x1 - 6 && x <= x1 - 5) mark = 1; break;
    }
    auto gapped = [&](int v) { return ((3 * x + y) % 5 == 0 && (random(cellKey(x, y)) & 1)) ? 0 : v; };
    if (x0 + 1 < x && x < x1 - 2 && y0 + 1 < y && y < y1 - 2) {
        if (x >= x0 + 3 && x <= x1 - 4 && y >= y0 + 3 && y <= y1 - 4 && !(x > x0 + 4 && y > y0 + 4 && x < x1 - 5 && y < y1 - 5))
            return mark != 0 ? 3 : gapped(3);
        if (x >= x0 + 6 && x <= x1 - 7 && y >= y0 + 6 && y <= y1 - 7)
            return 4;
        return mark;
    }
    return mark != 0 ? 2 : gapped(2);
}

int BuildingRules::fortressStyle(int32_t cx, int32_t cy) {
    switch (random(fortressOf(cx, cy).g + 0x55a05c) % 3) {
    case 1: return 4;
    case 2: return 3;
    default: return 2;
    }
}

int BuildingRules::fortressFloorCode(int32_t cx, int32_t cy, int f) {
    const Fortress fo = fortressOf(cx, cy);
    const int32_t ax = fo.square.middle.x, ay = fo.square.middle.y;
    const int t = type(cx, cy);
    if (t == 47) {
        if (f <= 0 || cx % 2 != 0 || cy % 2 != 0)
            return 0;
        if (fortressStyle(cx, cy) != 3)
            return 13;
        switch (random(fo.g + 0x100d7) & 3) {
        case 0: return cx < ax && cy < ay ? 13 : 0;
        case 1: return cx >= ax && cy < ay ? 13 : 0;
        case 2: return cx >= ax && cy >= ay ? 13 : 0;
        default: return cx < ax && cy >= ay ? 13 : 0;
        }
    }
    if (!floorExists(cx, cy, f))
        return 0;
    const int room = fortressCellCode(cx, cy);
    auto nextTo = [&](int v) {
        return fortressCellCode(cx - 1, cy) == v || fortressCellCode(cx + 1, cy) == v || fortressCellCode(cx, cy - 1) == v ||
               fortressCellCode(cx, cy + 1) == v;
    };
    if (room == 4) {
        if (cx == ax && cy == ay)
            return 0;
        return (cellRandom(cx, cy, 0x4f690) & 7) != 0 ? 1 : 0;
    }
    if (room == 1 && nextTo(4))
        return 5;
    if (room == 2 && nextTo(3))
        return fortressStyle(cx, cy);
    if ((cellRandom(floorDivBy(cx, 3), floorDivBy(cy, 3), 0x559dbf) & 7) == 0)
        return 7;
    if ((cellRandom(cx, cy, 0x4f690) & 7) == 1)
        return 1;
    if ((cellRandom(cx, cy, 0x9b61c) & 7) != 0)
        return 0;
    if (room == 2 && random(fo.g + 0x55a05c) % 3 == 0)
        return 13;
    return 0;
}

} // namespace worldgen
