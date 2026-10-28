// The zone's state while part B fills it, written from docs/specs/worldgen-structures.md §3-§5 and
// worldgen-structures-contents.md §C1, §C8.
#include "worldgen/ZoneBuild.h"

#include "worldgen/Height.h"

namespace worldgen {

uint32_t placeStyle(int32_t px, int32_t py) {
    if (bigCell(px, py))
        px = 2 * floorDiv(px, 1), py = 2 * floorDiv(py, 1);
    const int32_t bx = floorDiv(px, 4), by = floorDiv(py, 4);
    uint32_t salt = 0x1e44622; // the block's main realm, 22 times in 30
    switch (cellRandom(px, py, 0x4f7cff) % 30) {
    case 1: salt = 0x1e44621; break;
    case 2: salt = 0x1e44623; break;
    case 3: salt = 0x1e445e9; break;
    case 4: salt = 0x1e4465b; break;
    case 5: salt = 0x1e445e8; break;
    case 6: salt = 0x1e445ea; break;
    case 7: salt = 0x1e4465a; break;
    case 8: salt = 0x1e4465c; break;
    default: break;
    }
    return random(cellKey(bx, by) + salt);
}

int PlannerFacts::itemForCell(int32_t cx, int32_t cy) const {
    if (items.empty())
        return -1;
    return items[(size_t)((uint32_t)cellRandom(cx, cy, 0x89d85) % (uint32_t)items.size())];
}

ZoneBuild::ZoneBuild(ZoneTerrain &terrain, const WorldValues &world, CellObjects &cells, bool recordDraws)
    : terrain(terrain), world(world), cells(cells), zx(terrain.zx), zy(terrain.zy), x0(terrain.zx * ZONE_SIZE),
      y0(terrain.zy * ZONE_SIZE), recordDraws_(recordDraws) {}

int ZoneBuild::draw() {
    const int v = terrain.random.next();
    if (recordDraws_)
        terrain.draws.push_back((uint16_t)v);
    return v;
}

bool ZoneBuild::insideDots(int64_t x, int64_t y) const {
    return x >= dotsOf(x0) && x < dotsOf(x0 + ZONE_SIZE) && y >= dotsOf(y0) && y < dotsOf(y0 + ZONE_SIZE);
}

void ZoneBuild::putUnlessProtected(int32_t x, int32_t y, int32_t z, Block b) {
    if (!inZone(x, y) || (read(x, y, z).flags & 2))
        return;
    store(x, y, z, b);
}

const TerrainColumn *ZoneBuild::column(int32_t x, int32_t y) const {
    if (!inZone(x, y))
        return nullptr;
    return &terrain.columns[(size_t)ZoneTerrain::index(x - x0, y - y0)];
}

ColumnField ZoneBuild::columnValuesAt(int32_t x, int32_t y) const {
    if (inZone(x, y))
        return terrain.field[(size_t)ZoneTerrain::index(x - x0, y - y0)];
    const ColumnValues c = columnValues(x, y, placeForColumn(terrain.place, x, y), 0);
    ColumnField f;
    f.place = c.place;
    f.base = c.base;
    f.height = c.height;
    f.lake = c.lake;
    f.river = c.river;
    f.colours = colourInputs(c);
    return f;
}

// --- Records ---------------------------------------------------------------------------------

bool ZoneBuild::keepIfInside(const StaticObject &o) {
    if (!insideDots(o.x, o.y))
        return false;
    objects.push_back(o);
    return true;
}

bool ZoneBuild::keepIfInside(const Model &m) {
    if (!insideDots(m.x, m.y))
        return false;
    models.push_back(m);
    return true;
}

bool ZoneBuild::settle(StaticObject &o, bool groundTest) const {
    if (!inZone(blocksOf(o.x), blocksOf(o.y)))
        return false;
    const ObjectSize size = objectSize(o.type);
    float sx = size.x * o.scale, sy = size.y * o.scale;
    if (o.turn & 1)
        std::swap(sx, sy);
    constexpr int64_t EDGE = 655; // trunc64(655.3599853515625)
    const int64_t hx = trunc64(sx * 0.5f * 65536.0f), hy = trunc64(sy * 0.5f * 65536.0f);
    const int32_t bx0 = blocksOf(EDGE - hx + o.x), bx1 = blocksOf(hx - EDGE + o.x);
    const int32_t by0 = blocksOf(EDGE - hy + o.y), by1 = blocksOf(hy - EDGE + o.y);
    auto any = [&](int32_t z, bool wantSolid) {
        for (int32_t x = bx0; x <= bx1; x++)
            for (int32_t y = by0; y <= by1; y++)
                if (isOpen(read(x, y, z)) != wantSolid)
                    return true;
        return false;
    };
    for (int i = 0; i < 20 && !any(blocksTowardZero(o.z), true); i++)
        o.z -= DOTS;
    for (int i = 0; i < 20 && any(blocksTowardZero(o.z), true); i++)
        o.z += DOTS;
    if (groundTest && any(blocksTowardZero(o.z - DOTS), false))
        return false;
    const int32_t x = blocksOf(o.x), y = blocksOf(o.y);
    if (read(x, y, blocksOf(o.z)).type == blocktype::WATER)
        return false;
    const uint8_t below = read(x, y, blocksOf(o.z - DOTS)).type;
    if (below == 3 || below == 17 || below == 19)
        return false;
    o.z = dotsOf(blocksOf(o.z));
    return true;
}

bool ZoneBuild::settleAndKeep(StaticObject o) {
    if (!settle(o, true))
        return false;
    return keepIfInside(o);
}

void ZoneBuild::keepChest(const StaticObject &o, const Item &reward) {
    if (keepIfInside(o))
        rewards[(int)objects.size() - 1] = reward;
}

// --- Spots -----------------------------------------------------------------------------------

int ZoneBuild::spotCount(int32_t cx, int32_t cy, int kind) const {
    const auto it = spots.find({cx, cy, kind});
    return it == spots.end() ? 0 : (int)it->second.size();
}

Spot ZoneBuild::takeSpot(int32_t cx, int32_t cy, int kind) {
    auto next = [this] { return draw(); };
    return worldgen::takeSpot({spots, working, objects, middleSpot(terrain)}, cx, cy, kind, Draw(next));
}

Spot ZoneBuild::spotInCell(int32_t cx, int32_t cy) {
    auto next = [this] { return draw(); };
    return worldgen::spotInCell({spots, working, objects, middleSpot(terrain)}, cx, cy, Draw(next));
}

// --- Spots outside the generator -------------------------------------------------------------

Spot middleSpot(const ZoneTerrain &terrain) {
    const TerrainColumn &c = terrain.columns[(size_t)ZoneTerrain::index(32, 32)];
    return {dotsOf(terrain.zx * ZONE_SIZE + 32) + 32768, dotsOf(terrain.zy * ZONE_SIZE + 32) + 32768, dotsOf(c.top())};
}

Spot takeSpot(const ZoneSpots &z, int32_t cx, int32_t cy, int kind, Draw rand) {
    if (kind == 3) {
        for (const StaticObject &o : z.objects)
            if (o.type == objecttype::CELL_92_OBJECT)
                return {o.x, o.y, o.z + trunc64(objectSize(o.type).z * o.scale * 65536.0f)};
    }
    const auto it = z.working.find({cx, cy, kind});
    if (it != z.working.end()) {
        std::vector<Spot> &list = it->second;
        if (list.empty()) {
            const auto from = z.spots.find({cx, cy, kind});
            if (from != z.spots.end())
                list = from->second;
        }
        if (!list.empty()) {
            const size_t i = (size_t)(rand() % (int)list.size());
            const Spot s = list[i];
            list.erase(list.begin() + (std::ptrdiff_t)i);
            return s;
        }
    }
    return z.middle; // the top of the zone's middle column
}

Spot spotInCell(const ZoneSpots &z, int32_t cx, int32_t cy, Draw rand) {
    for (int kind : {2, 7, 16, 1}) {
        const auto it = z.spots.find({cx, cy, kind});
        if (it != z.spots.end() && !it->second.empty())
            return takeSpot(z, cx, cy, kind, rand);
    }
    return takeSpot(z, cx, cy, 0, rand);
}

} // namespace worldgen
