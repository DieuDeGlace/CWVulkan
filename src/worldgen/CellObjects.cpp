// The cell objects, written from docs/specs/worldgen-structures.md §6 (rules cited by number).
#include "worldgen/CellObjects.h"

#include "worldgen/BlockColours.h"
#include "worldgen/Height.h"
#include "worldgen/ZoneTerrain.h"

#include <cstdlib>

namespace worldgen {
namespace {

int32_t evened(int32_t v) { return 2 * floorDiv(v, 1); }
int32_t cellOfBlock(int32_t v) { return floorDiv(v, 4); } // 16 blocks
int32_t sq(int32_t v) { return v * v; }
float radius2(bool big) { return big ? 250000.0f : 62500.0f; }

// The place cell of a cell, evened when the place there is big.
Int2 placeCellOf(int32_t cx, int32_t cy, bool big) {
    Int2 c{floorDiv(wrapMul(cx, 16), PLACE_SHIFT), floorDiv(wrapMul(cy, 16), PLACE_SHIFT)};
    if (big)
        c = {evened(c.x), evened(c.y)};
    return c;
}

// The side pair test of rules 13-15: one, two or three of the cells beside the anchor.
bool sidePair(int32_t cx, int32_t cy, Int2 c1, int32_t ax, int32_t ay) {
    const int32_t bx = (cellRandom(c1, 0x711ac) & 1) * 2 - 1 + ax;
    const int32_t dy = (cellRandom(c1, 0x11b548) & 1) * 2 - 1;
    const bool a = cellRandom(c1, 0x6b0e8) & 1, b = cellRandom(c1, 0xb2234) & 1;
    if (cx == bx && cy == ay)
        return a;
    if (cx == ax && cy == ay + dy)
        return b;
    if (cx == bx && cy == ay + dy)
        return a || b;
    return false;
}

// The rocky and shrub noises of rule 24 (0x2a0b90, 0x2adb80).
float rockyNoise(const WorldValues &world, int32_t x, int32_t y, float warm) {
    if (warm > 0.75f)
        return 0.0f;
    float v = simplex((double)x * 0.001 + 8473.0 + (double)world.value(0x350), (double)y * 0.001 + 9438.0 + (double)world.value(0x354));
    v = v + simplex((double)wrapMul(x, 65), (double)wrapMul(y, 77)) * 0.05f;
    v = v - 0.2f;
    return v + (1.0f - warm) * 0.1f;
}
float shrubNoise(const WorldValues &world, int32_t x, int32_t y, float warm) {
    float v = simplex((double)x * 0.0015 + (double)world.value(0x350), (double)y * 0.0015 + (double)world.value(0x354));
    v = v + simplex((double)wrapMul(x, 37), (double)wrapMul(y, 37)) * 0.05f;
    v = v - 0.2f;
    return v + (1.0f - warm) * 0.1f;
}

constexpr int BIOMES_3_TO_6[] = {3, 4, 5, 6};

} // namespace

// --- §6.1, §6.2 --------------------------------------------------------------------------------

Int2 cellPoint(int32_t cx, int32_t cy) {
    return {wrapAdd(wrapMul(cx, 16), 4 + (cellRandom(cx, cy, 0x55bda9) & 7)),
            wrapAdd(wrapMul(cy, 16), 4 + (cellRandom(cx, cy, 0x9c373) & 7))};
}

int bigStructure(int32_t px, int32_t py, int kind, bool big) {
    if (!big)
        return 0;
    const int32_t h = cellRandom(evened(px), evened(py), 0x62ad0);
    switch (kind) {
    case 5: return (h & 7) == 0 ? 41 : 0;
    case 9: return (h & 1) == 0 ? 40 : 0;
    case 15: return (h & 1) == 0 ? 43 : 0;
    case 42: case 44: case 45: case 46: case 47: return kind;
    default: return 0;
    }
}

Int2 structureAnchor(int32_t px, int32_t py, int kind, bool big) {
    if (big)
        px = evened(px), py = evened(py);
    const Int2 p = worldgen::cellPoint(px, py, big);
    if (bigStructure(px, py, kind, big) == 0)
        return {cellOfBlock(p.x), cellOfBlock(p.y)};
    Int2 a{8 * floorDiv(p.x, 7) + 4, 8 * floorDiv(p.y, 7) + 4};
    const bool fixed = kind == 9 || kind == 42 || kind == 44 || kind == 45 || kind == 46 || kind == 47;
    if (!fixed) {
        a.x += (cellRandom(px, py, 0x8a4d5) % 5) * 8 - 16;
        a.y += (cellRandom(px, py, 0x47940) % 5) * 8 - 16;
    }
    const int32_t span = big ? 127 : 63;
    auto clampTo = [span](int32_t v, int32_t lo) { return v < lo ? lo : (v > lo + span ? lo + span : v); };
    a.x = clampTo(a.x, wrapMul(px, 64));
    a.y = clampTo(a.y, wrapMul(py, 64));
    return a;
}

Int2 terrainAnchor(Int2 placeCell, int kind, bool big) {
    if (bigStructure(placeCell.x, placeCell.y, kind, big) == 0)
        return NO_ANCHOR;
    return structureAnchor(placeCell.x, placeCell.y, kind, big);
}

int placeLandmark(int kind, Int2 c1) {
    const int32_t v = cellRandom(c1, 0xae13d9) & 3;
    const int pick = (cellRandom(c1, 0x5708d4) & 1) + 69;
    switch (kind) {
    case 10: return v == 1 ? pick : v == 2 ? 65 : 0;
    case 13: return v == 1 ? pick : v == 2 ? 63 : 0;
    case 14: case 19: case 23: case 25: case 28: case 31: return v == 1 ? pick : 0;
    default: return 0;
    }
}

// --- §6.4 --------------------------------------------------------------------------------------

int townBlock(const WorldValues &world, int32_t cx, int32_t cy, int kind) {
    if (kind != 5)
        return 0;
    const int32_t bx = floorDivBy(cx, 3), by = floorDivBy(cy, 3);
    const int32_t px = evened(floorDiv(wrapMul(cx, 16), PLACE_SHIFT)), py = evened(floorDiv(wrapMul(cy, 16), PLACE_SHIFT));
    const bool big = bigCell(px, py);
    const Int2 c = worldgen::cellPoint(px, py, big);
    const int32_t ox = floorDivBy(cellOfBlock(c.x), 3) - 2, oy = floorDivBy(cellOfBlock(c.y), 3) - 2;
    if (ox <= bx && bx < ox + 4 && oy <= by && by < oy + 4) {
        const auto &layout = world.townLayouts[random(cellKey(px, py)) % 64u];
        const int32_t v = layout[(size_t)((by - oy) * 4 + (bx - ox))];
        if (v != 0)
            return v;
    }
    const int32_t x = wrapAdd(wrapMul(bx, 48), 16), y = wrapAdd(wrapMul(by, 48), 16);
    const float d2 = warpedDist2(c, x, y);
    const float r2 = radius2(big);
    if (d2 >= r2)
        return 0;
    float w = 1.0f - d2 / r2;
    w = w * w;
    if (0.5f > w || roads(x, y, 5, w) > 0.4f)
        return 0;
    const int32_t v = cellRandom(bx, by, 0x11b98f) % 1000;
    if ((float)v > w * 800.0f)
        return v % 2 == 0 ? 13 : 0;
    return 1;
}

Int2 blockCorner(int32_t bx, int32_t by) {
    const int32_t v = cellRandom(bx, by, 0x5a44a) & 3;
    return {3 * bx + ((v == 1 || v == 2) ? 1 : 0), 3 * by + ((v == 2 || v == 3) ? 1 : 0)};
}

int cellTurn(int32_t cx, int32_t cy) {
    const int32_t bx = floorDivBy(cx, 3), by = floorDivBy(cy, 3);
    const int32_t v = cellRandom(bx, by, 0x5a44a) & 3;
    const int32_t lx = cx - 3 * bx, ly = cy - 3 * by;
    if (lx == 0 && ly == 0)
        return (4 - v) & 3;
    if (lx == 1 && ly == 0)
        return (5 - v) & 3;
    if (lx == 1 && ly == 1)
        return (6 - v) & 3;
    if (lx == 0 && ly == 1)
        return (7 - v) & 3;
    return -1;
}

bool innerBlock(int32_t bx, int32_t by, int kind, Int2 anchor) {
    if (bx == floorDivBy(anchor.x, 3) && by == floorDivBy(anchor.y, 3))
        return false;
    const int32_t x = wrapAdd(wrapMul(bx, 48), 16), y = wrapAdd(wrapMul(by, 48), 16);
    const int32_t px = floorDiv(x, PLACE_SHIFT), py = floorDiv(y, PLACE_SHIFT);
    const bool big = bigCell(px, py);
    const Int2 p = worldgen::cellPoint(px, py, big);
    const float d2 = warpedDist2(p, x, y);
    const float r2 = radius2(big);
    if (kind == 0 || d2 >= r2)
        return false;
    float w = 1.0f - d2 / r2;
    w = w * w;
    return w >= 0.5f;
}

Int2 areaLandmark(int32_t qx, int32_t qy) {
    return {cellRandom(qx, qy, 0x24341) % 62 + 64 * qx + 1, cellRandom(qx, qy, 0x165709) % 62 + 64 * qy + 1};
}

FortressSquare fortressSquare(Int2 c1, Int2 landmark) {
    FortressSquare f;
    f.size = (cellRandom(c1, 0xc0aa) & 3) + 15;
    f.turn = cellRandom(c1, 0x5d932) & 3;
    f.middle = {cellOfBlock(landmark.x), cellOfBlock(landmark.y)};
    return f;
}

Int2 FortressSquare::turned(int32_t cx, int32_t cy) const {
    const int32_t ax = middle.x, ay = middle.y;
    switch (turn) {
    case 1: return {cy - ay + ax, ax - cx + ay};
    case 2: return {ax - cy + ay, ax - cx + ay}; // the game's formula
    case 3: return {ax - cy + ay, ay + cx - ax};
    default: return {cx, cy};
    }
}

// --- §6.3 --------------------------------------------------------------------------------------

CellObjects::CellObjects(const WorldValues &world) : world_(&world), cache_(1 << 14) {}

int CellObjects::type(int32_t cx, int32_t cy) {
    Entry &e = cache_[(cellKey(cx, cy) * 2654435761u >> 18) & (cache_.size() - 1)];
    if (e.type < 0 || e.cx != cx || e.cy != cy)
        e = {cx, cy, (int16_t)compute(cx, cy)};
    return e.type;
}

int CellObjects::compute(int32_t cx, int32_t cy) const {
    const Int2 q = cellPoint(cx, cy);
    const Place p = placeAt(q.x, q.y);
    const int k = p.kind;
    const float w = p.weight;
    const bool big = p.big;
    const Int2 c1 = placeCellOf(cx, cy, big);

    // 1. Big structure cells.
    Int2 anchor{0, 0};
    if (const int b = bigStructure(c1.x, c1.y, k, big)) {
        anchor = structureAnchor(c1.x, c1.y, k, big);
        const int32_t ax = anchor.x, ay = anchor.y;
        auto row = [&](int first) {
            if (cy == ay && cx >= ax && cx <= ax + 4)
                return first + (4 - (cx - ax));
            return 0;
        };
        const bool square = (cx == ax - 1 || cx == ax) && (cy == ay - 1 || cy == ay);
        switch (b) {
        case 43: if (int t = row(71)) return t; break;
        case 42: if (int t = row(76)) return t; break;
        case 47: if (int t = row(81)) return t; break;
        case 44: case 41: if (square) return 86; break;
        case 45: if (square) return 94; break;
        case 46: if (square) return 95; break;
        default: break;
        }
    }

    // 2. A place landmark.
    if (k != 0 && w > 0.5f) {
        if (const int m = placeLandmark(k, c1)) {
            const int32_t mx = cellRandom(c1, 0x890d5) % 7 - 3 + cellOfBlock(p.point.x);
            const int32_t my = cellRandom(c1, 0x10275) % 7 - 3 + cellOfBlock(p.point.y);
            if (cx == mx && cy == my)
                return m;
            if (sq(cx - mx) + sq(cy - my) < sq(cellReach(m) + 1))
                return 0;
        }
    }

    const int32_t bx = floorDivBy(cx, 3), by = floorDivBy(cy, 3);

    // 3. Towns.
    if (k == 5) {
        const int t = townBlock(*world_, cx, cy, k);
        if (t == 0) {
            if (cx % 2 == 0 && cy % 2 == 0 && cellRandom(cx, cy, 0x723db) % 10 == 0 && w > 0.5f)
                return warmth(neighbourhood(q.x, q.y), k, w) > 0.75f ? 7 : 6;
        } else if (t == 13) {
            if (forestRegion(neighbourhood(q.x, q.y), k, w) > 0.5f)
                return 34;
            return cellRandom(bx, by, 0x4f60c1) % 7 + 27;
        } else if (posMod(cx, 3) != 2 && posMod(cy, 3) != 2) {
            const Int2 g = blockCorner(bx, by);
            const bool corner = cx == g.x && cy == g.y;
            if (t == 2)
                return corner ? 24 : 23;
            if (t == 11)
                return corner ? 91 : 90;
            if (t == 10)
                return corner ? 92 : 0;
            if (t == 7 || t == 8 || t == 9) {
                const int u = cellTurn(cx, cy);
                if (t == 7 && u == 1)
                    return 26;
                if (u < (t == 7 ? 4 : 3) || (cellRandom(cx, cy, 0xfcc8) & 1)) {
                    switch (cellRandom(cx, cy, 0x310c07) % 3) {
                    case 0: return 26;
                    case 1: return corner ? 24 : 22;
                    case 2: return 25;
                    default: break;
                    }
                }
            } else {
                const bool house = corner || (cx == g.x && cellRandom(bx, by, 0x45982d) % 3 == 0) ||
                                   (cy == g.y && cellRandom(bx, by, 0x43dcd) % 3 == 0) || (cellRandom(bx, by, 0xf794) & 7) == 0;
                if (house)
                    return corner ? 24 : cellRandom(cx, cy, 0x12a915) % 10 == 0 ? 23 : 22;
            }
        }
    }

    // 4. Kind 15.
    if (k == 15 && innerBlock(bx, by, 15, anchor)) {
        uint32_t h = random(cellKey(bx, by));
        if (h & 1) {
            h >>= 1;
            if (h % 20 == 0 && posMod(cx, 3) == 1 && posMod(cy, 3) == 1)
                return (int)((h / 20) & 1) + 69;
            if (h % 20 != 0 && posMod(cx, 3) != 2 && posMod(cy, 3) != 2)
                return (cellRandom(cx, cy, 0x4d468e) & 1) + 49;
        }
    }

    const int32_t ax = cellOfBlock(p.point.x), ay = cellOfBlock(p.point.y);
    const int32_t d2 = sq(cx - ax) + sq(cy - ay);
    const int32_t mx = wrapAdd(wrapMul(cx, 16), 8), my = wrapAdd(wrapMul(cy, 16), 8);
    auto cw = [&](int kind) { return placeWeight(kind, big, warpedDist2(p.point, mx, my)); };
    const bool atCentre = cx == ax && cy == ay;
    const bool near = std::abs(cx - ax) < 2 && std::abs(cy - ay) < 2;

    switch (k) {
    case 16: // 5.
        if (atCentre)
            return 55;
        if (cx == ax && cy == ay + 1)
            return 56;
        if ((std::abs(cx - ax) == 4 && cy == ay) || (cx == ax && std::abs(cy - ay) == 4) ||
            (std::abs(cx - ax) == 3 && std::abs(cy - ay) == 3))
            return 54;
        if (cw(16) > 0.5f)
            return 0;
        break;
    case 34: case 35: // 6.
        if (atCentre)
            return 98;
        if (d2 < 9 && (cellRandom(cx, cy, 0x7f395) & 3) == 0)
            return 99;
        break;
    case 45: case 46: // 7.
        if (std::abs(cx - anchor.x) <= 7 && std::abs(cy - anchor.y) < 7)
            return 93;
        break;
    case 24: // 8.
        if (std::abs(cx - ax) == 6 && std::abs(cy - ay) == 6)
            return 96;
        if (cx == ax - 3 && cy == ay)
            return 97;
        if (d2 <= 112)
            return 93;
        break;
    case 17: { // 9.
        if (atCentre)
            return 66;
        const int32_t e = big ? 31 : 15;
        const int32_t dx = std::abs(cx - ax), dy = std::abs(cy - ay);
        if ((cx == ax || cy == ay) && dx <= e - 1 && dy <= e - 1) {
            const int32_t m = big ? 128 : 64;
            const int32_t vx = posMod(cx, m), vy = posMod(cy, m);
            if (dx == e - 1 || dy == e - 1 || vx == 0 || vy == 0 || vx == m - 1 || vy == m - 1)
                return 68;
            return 67;
        }
        if ((dx <= 1 || dy <= 1) && dx <= e && dy <= e)
            return 0;
        if (cw(17) > 0.5f)
            return 0;
        break;
    }
    case 38: // 10.
        if (atCentre)
            return (int)(random(cellKey(c1.x, c1.y)) & 1) + 69;
        if (d2 < 9)
            return 0;
        break;
    case 39: // 11.
        if (atCentre)
            return 55;
        break;
    case 37: // 12.
        if (atCentre)
            return 57;
        if (d2 < 9)
            return 0;
        break;
    case 1: case 3: // 13.
        if (atCentre)
            return 24;
        if (near && sidePair(cx, cy, c1, ax, ay))
            return (cellRandom(cx, cy, 0x23b0d) & 1) + 22;
        break;
    case 4: // 14.
        if (atCentre)
            return 37;
        if (near && sidePair(cx, cy, c1, ax, ay))
            return (cellRandom(cx, cy, 0x23b0d) & 1) == 1 ? 38 : 35;
        break;
    case 2: case 9: { // 15 and 22.
        const int32_t x = wrapAdd(wrapMul(bx, 48), 16), y = wrapAdd(wrapMul(by, 48), 16);
        const float wb = placeWeight(k, big, warpedDist2(p.point, x, y));
        if (!(wb > 0.5f))
            break;
        if (k == 2) {
            if (atCentre)
                return 24;
            if (near)
                return sidePair(cx, cy, c1, ax, ay) ? (cellRandom(cx, cy, 0x23b0d) & 1) + 22 : 0;
            if ((cellRandom(bx, by, 0x49f7f4) & 3) != 0 && roads(x, y, 2, wb) < 0.4f)
                return cellRandom(bx, by, 0x4f60c1) % 7 + 27;
            break;
        }
        int32_t tx = 8 * floorDiv(p.point.x, 7) + 4, ty = 8 * floorDiv(p.point.y, 7) + 4;
        if (std::abs(cx - tx) <= 1 && std::abs(cy - ty) <= 1) {
            if ((cx == tx - 1 || cx == tx) && (cy == ty - 1 || cy == ty) && bigStructure(c1.x, c1.y, 9, big) != 0)
                return 39;
            if ((cellRandom(c1, 0x8ca8d) & 1) == 0)
                tx += 1;
            else
                ty += 1;
            if (cx == tx && cy == ty)
                return 36;
            return (cellRandom(cx, cy, 0x12a915) & 7) == 1 ? 38 : 35;
        }
        if ((cellRandom(bx, by, 0x49f7f4) & 3) != 0 && roads(x, y, 9, wb) < 0.4f)
            return 34;
        break;
    }
    case 7: // 16.
        if (d2 < 16)
            return atCentre ? 52 : 0;
        break;
    case 8: { // 17.
        const FortressSquare f = fortressSquare(c1, p.point);
        const Int2 t = f.turned(cx, cy);
        const int32_t half = f.size / 2, x0 = ax - half, y0 = ay - half, x1 = x0 + f.size, y1 = y0 + f.size;
        if (t.x < x0 || t.x >= x1 || t.y < y0 || t.y >= y1)
            break;
        if ((t.x < x0 + 2 && (t.y < y0 + 2 || t.y >= y1 - 2)) || (t.x >= x1 - 2 && (t.y >= y1 - 2 || t.y < y0 + 2)))
            return 47;
        if (t.x == x0 && t.y == ay)
            return 46;
        return 45;
    }
    case 12: { // 18.
        const Int2 lp = worldgen::cellPoint(c1.x, c1.y, big);
        const float wb = placeWeight(12, big, warpedDist2(lp, mx, my));
        if (!(wb > 0.5f))
            break;
        if (!(cx % 5 == 0 || cy % 5 == 0 || signedRandom(cellKey(cx / 5, cy / 5)) % 5 == 0))
            return 0;
        if (groundHeight(mx, my, 0) < 1.0f || roads(mx, my, 12, wb) > 0.5f)
            return 0;
        return (cx + 3 * cy) % 5 == 0 ? 43 : 41;
    }
    case 22: // 19.
        if (atCentre)
            return 89;
        if ((cx == ax + 1 && cy == ay) || (cx == ax && cy == ay + 1))
            return 87;
        if (cx == ax + 1 && cy == ay + 1)
            return 88;
        break;
    case 20: // 20.
        if (atCentre)
            return 8;
        break;
    case 21: // 21.
        if (std::abs(ax - cx) < 2 && std::abs(ay - cy) < 2)
            return atCentre ? 12 : 11;
        break;
    default: break;
    }

    // 23. The area landmark.
    {
        const int32_t qx = floorDiv(cx, 6), qy = floorDiv(cy, 6);
        const Int2 l = areaLandmark(qx, qy);
        int type = 60, reach = 1;
        if ((qx + qy) & 1)
            type = 53;
        else
            switch (cellRandom(l, 0x8ae21) % 11) {
            case 1: type = 15, reach = 2; break;
            case 2: type = 37, reach = 2; break;
            case 3: type = 55; break;
            case 4: type = 69, reach = 2; break;
            case 5: type = 70, reach = 2; break;
            case 6: type = 13; break;
            case 7: type = 14; break;
            case 8: type = 62; break;
            case 9: type = 21; break;
            case 10: type = 100, reach = 2; break;
            default: break;
            }
        if (cx == l.x && cy == l.y) {
            if (type == 21 && groundHeight(wrapAdd(wrapMul(l.x, 16), 8), wrapAdd(wrapMul(l.y, 16), 8), 0) < 0.0f)
                type = 18;
            return type;
        }
        if (type == 60) {
            int32_t sx = l.x + 4;
            if (floorDiv(sx, 6) != floorDiv(cx, 6))
                sx = l.x - 4;
            if (cx == sx && cy == l.y)
                return 51;
            if (std::abs(cx - sx) < 2 && std::abs(cy - l.y) < 2)
                return 0;
        }
        if (std::abs(cx - l.x) <= reach && std::abs(cy - l.y) <= reach)
            return 0;
    }

    // 24.
    return natural(cx, cy, q, p);
}

int CellObjects::natural(int32_t cx, int32_t cy, Int2 q, const Place &p) const {
    const int k = p.kind;
    const float w = p.weight;
    const bool inPlace = k != 0 && w > 0.5f;
    int rare = 600;
    bool skipRare = false;
    if (k == 28 && w > 0.5f)
        rare = 50;
    else if (k == 29 && w > 0.5f)
        rare = 20;
    else if (inPlace)
        skipRare = true;
    if (!skipRare) {
        if (cellRandom(cx, cy, 0x8af1e) % rare == 0) {
            if (groundHeight(q.x, q.y, 0) < 1.0f)
                return 0;
            const float road = roads(q.x, q.y, k, w);
            if (road > 0.4f || river(q.x, q.y, road, 0, 0, 0, p) > 0.4f)
                return 0;
            if ((k == 28 || k == 29) && w > 0.5f)
                return cellRandom(cx, cy, 0x3005f52) % 3 + 10;
            return (cellRandom(cx, cy, 0x3005f52) & 1) + 10;
        }
        if (cellRandom(cx, cy, 0x1e2d8) % 200 == 0) {
            if (cliffs(q.x, q.y, k, w) > 0.4f || inPlace || groundHeight(q.x, q.y, 0) < 1.0f)
                return 0;
            const float road = roads(q.x, q.y, k, w);
            if (road > 0.5f || river(q.x, q.y, road, 0, 0, 0, p) > 0.5f)
                return 0;
            return 9;
        }
    }
    if ((k == 14 || k == 18 || k == 19) && w > 0.5f)
        return 0;
    if ((cellRandom(cx, cy, 0x8e9a9) & 3) != 0)
        return 0;
    const Neighbourhood n = neighbourhood(q.x, q.y);
    float v = (simplex((double)cx * 0.08 + 8989.0, (double)cy * 0.08 + 60466.0) + 1.0f) * 0.25f;
    v = v + regionWeight(n, BIOMES_3_TO_6, 4, 0.0004f) * 0.35f;
    v = v + simplex((double)wrapMul(cx, 2) + (double)0x182a8, (double)wrapMul(cy, 2) + (double)0x5cb89e) * 0.5f;
    if ((k == 25 || k == 26 || k == 27 || k == 32) && w > 0.5f)
        v = simplex((double)wrapMul(cx, 2) + (double)0x182a8, (double)wrapMul(cy, 2) + (double)0xa52de) * 0.5f + 0.75f;
    else if (dryRegion(n) > 0.5f)
        return 0;
    if (k == 30 && w > 0.5f)
        v = v * 0.7f;
    else if (k == 36 && w > 0.5f)
        v = v * 0.6f;
    else if (k == 33 && w > 0.5f)
        return 0;
    if (!(v > 0.5f))
        return 0;
    const float h = groundHeight(q.x, q.y, 0);
    if (1.0f > h)
        return 0;
    const float road = roads(q.x, q.y, k, w);
    if (road > 0.5f)
        return 0;
    if (river(q.x, q.y, road, 0, 0, wetRegion(n), p) > 0.5f)
        return 0;
    const int32_t r = cellRandom(cx, cy, 0x49ecc);
    if (r % 30 == 0)
        return (cellRandom(cx, cy, 0x17b9f76) & 1) ? 19 : 20;
    const float warm = warmth(n, k, w);
    if (warm > 0.75f && (20.0f > h || (r & 7) == 0))
        return 5;
    if (warm > 0.6f && 20.0f > h && (r & 3) == 0)
        return 5;
    if (warm > 0.8f && dryness(n) > 0.8f)
        return 4;
    const int32_t x = wrapMul(cx, 16), y = wrapMul(cy, 16);
    if (rockyNoise(*world_, x, y, warm) > 0.6f)
        return 3;
    if (0.8f > warm && cellRandom(cx, cy, 0x2f8944d) % 10 == 0)
        return 3;
    if (0.7f > warm) {
        if (shrubNoise(*world_, x, y, warm) > 0.3f)
            return 2;
        if (0.8f > warm && cellRandom(cx, cy, 0x4bf00) % 10 == 0)
            return 2;
    }
    return 1;
}

} // namespace worldgen
