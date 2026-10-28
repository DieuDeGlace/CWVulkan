// A zone's terrain, written from docs/specs/worldgen-terrain-zone.md (sections cited as §n).
#include "worldgen/ZoneTerrain.h"

#include "worldgen/Height.h"

#include <cmath>
#include <cstdlib>

namespace worldgen {
namespace {

// River water: the generator builds it as the property 255 per channel, which storing halves
// (§5.4 step 1), so its bytes are 127. Sea water (SEA_WATER) is stored as bytes and stays 255.
constexpr Block RIVER_WATER = {127, 127, 127, 0, blocktype::WATER, 0};
constexpr Block GLOWING_AIR = {0, 0, 0, 255, blocktype::AIR, 0}; // marks an overhang light's spot
constexpr uint8_t FLAG_KEEP_AIR = 2; // air with this flag stays air below sea level

// |v| of a 32-bit value, wrapping like the processor for the lowest value.
int32_t abs32(int32_t v) { return v < 0 ? (int32_t)(0u - (uint32_t)v) : v; }
int32_t wrapSub(int32_t a, int32_t b) { return (int32_t)((uint32_t)a - (uint32_t)b); }
int32_t wrapMul(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }

// The heights of the zone's columns and the ring around them, as the full pipeline gives them
// (0x35ea0, mode 0, with each column's own reach-limited place): the four neighbour heights of
// every column come from here. Inside the zone the pipeline usually sees the same place as the
// column field, and then its height is the field's.
class NeighbourGrid {
public:
    static constexpr int SIDE = ZONE_SIZE + 2;
    void fill(const ZoneTerrain &zone) {
        const int32_t x0 = zone.zx * ZONE_SIZE, y0 = zone.zy * ZONE_SIZE;
        for (int i = 0; i < SIDE; i++)
            for (int j = 0; j < SIDE; j++) {
                const int32_t x = x0 + i - 1, y = y0 + j - 1;
                const Place own = placeAt(x, y);
                const bool inside = i >= 1 && i <= ZONE_SIZE && j >= 1 && j <= ZONE_SIZE;
                if (inside) {
                    const ColumnField &f = zone.field[ZoneTerrain::index(i - 1, j - 1)];
                    if (samePlace(own, f.place)) {
                        heights[i][j] = f.height;
                        continue;
                    }
                }
                heights[i][j] = columnValues(x, y, own, 0).height;
            }
    }
    NeighbourHeights around(int lx, int ly) const {
        return {heights[lx][ly + 1], heights[lx + 2][ly + 1], heights[lx + 1][ly], heights[lx + 1][ly + 2]};
    }

private:
    float heights[SIDE][SIDE];
};

// The column's colours "from scratch" (§4.8): its values through the reach-limited place, which
// are the field's own unless the reach limit changes the place.
ColourInputs scratchColours(int32_t x, int32_t y, const ColumnField &f) {
    const Place own = placeAt(x, y);
    if (samePlace(own, f.place))
        return f.colours;
    return colourInputs(columnValues(x, y, own, 0));
}

} // namespace

// --- Rock spans (§5.8) ---

bool hasRockSpans(int kind) { return kind == 24 || kind == 45 || kind == 46; } // test 0x2b8010

float rockSpanHeight(const Place &p) {
    const float h = placeHeight(p);
    return h <= 10.0f ? 10.0f : h;
}

float spanBaseHeight(const Place &p, float ownHeight, int32_t x, int32_t y) {
    if (p.variant == 2) {
        int32_t cx = floorDiv(x, PLACE_SHIFT), cy = floorDiv(y, PLACE_SHIFT);
        if (p.big)
            cx = 2 * floorDiv(cx, 1), cy = 2 * floorDiv(cy, 1);
        const float turns = (float)(random((uint32_t)cy * 57u + 0x4e3e1cu + (uint32_t)cx) % 20u + 25u);
        return ownHeight + minOf(150.0f, (float)p.radius * 0.6f / turns * 15.0f);
    }
    if (p.variant == 1)
        return ownHeight + ((float)p.radius * 0.7f - (float)p.radius * 0.5f) * 0.75f;
    return ownHeight;
}

// The span's bottom (0x2ba2b0).
int32_t spanBottom(const RockSpan &s, int32_t x, int32_t y, float h) {
    if (s.kind == 45 && abs32(wrapSub(x, wrapMul(s.anchor.x, 16))) < 8 && y < wrapMul(s.anchor.y + 1, 16))
        return trunc32(s.spanBase + 15.0f);
    if (s.kind == 24) {
        const int32_t qx = floorDiv(s.place.point.x, 4), qy = floorDiv(s.place.point.y, 4);
        const int32_t dx = qx * 16 - x + 8, dy = qy * 16 - y + 8;
        const int32_t d = dx * dx + dy * dy;
        if (0xe0f < d && d < 0x4731) {
            if (d > 0x4623)
                return trunc32(s.spanBase + 30.0f);
            if (d > 0x4518)
                return trunc32(s.spanBase + 29.0f);
            if (floorDiv(x, 4) == qx || floorDiv(y, 4) == qy)
                return floorToInt(s.spanBase) + 12;
        }
    }
    return floorToInt(h);
}

// The span's top (0x2bab80).
int32_t spanTop(const RockSpan &s, int32_t x, int32_t y, float h) {
    if (s.kind == 24) {
        const int32_t cx = floorDiv(s.place.point.x, 4) * 16 + 8, cy = floorDiv(s.place.point.y, 4) * 16 + 8;
        const int32_t d = (cx - x) * (cx - x) + (cy - y) * (cy - y);
        if (d >= 0x4731)
            return floorToInt(h) - 1;
        if (d < 0xe10)
            return trunc32(s.spanBase + 1.0f);
        if (d > 0x3fff)
            return trunc32(s.spanBase + 40.0f);
        const int32_t ax = std::abs(x - cx), ay = std::abs(y - cy);
        if (ax > 19 && ay > 19) {
            const float v = std::sqrt((float)d) * 0.5f - 30.0f;
            const float sv = v > 0.0f ? v : 0.0f;
            return trunc32((float)((trunc32(sv) / 2) * 2 + 2) + s.spanBase + 1.0f);
        }
        const float v = (float)(ax > ay ? ax : ay) * 0.5f - 30.0f;
        const float sv = v > 0.0f ? v : 0.0f;
        return trunc32(sv + s.spanBase + 1.0f);
    }
    // Kinds 45 and 46: a square around the anchor, stepped for 46.
    const int32_t dx = abs32(wrapSub(x, wrapMul(s.anchor.x, 16))), dy = abs32(wrapSub(y, wrapMul(s.anchor.y, 16)));
    int32_t m = dx > dy ? dx : dy;
    if (s.kind == 46) {
        m = m > 24 ? m : 24;
        if (dx > 12 && dy > 12)
            m = (m / 6) * 6 - 3;
    }
    const float v = (s.spanBase + 100.0f) - (float)m;
    return floorToInt(v <= h ? h : v);
}

// The span's colour (0x2ba5f0).
BlockProps spanColour(const RockSpan &s, int32_t x, int32_t y, int32_t z, const SandOffsets &sand) {
    if (s.kind == 24) {
        const int32_t qx = floorDiv(s.place.point.x, 4), qy = floorDiv(s.place.point.y, 4);
        const int32_t dx = x - (qx * 16 + 8), dy = y - qy * 16 - 8;
        if (dx * dx + dy * dy < 0xe10) {
            const float q = sandShade(x, y, sand);
            return {q + 340.0f, q + 250.0f, q + 100.0f, 0, blocktype::ROCK, 0};
        }
    }
    float r = 180.0f, g = 170.0f, b = s.kind == 24 ? 120.0f : 90.0f;
    if (s.kind == 45)
        r = 260.0f, g = 250.0f, b = 130.0f;
    // Speckles by block, from integer noise inputs (wrapping like the game's).
    const int32_t nx = (int32_t)((uint32_t)((x + z) / 2) * 154u + (uint32_t)z * 53u);
    const int32_t ny = (int32_t)((uint32_t)((y + z) / 2) * 154u + (uint32_t)z * 53u);
    float n = simplex((double)nx, (double)ny);
    if (n > 0.7f) {
        n = (n - 0.7f) / 0.3f * 0.1f + 1.0f;
        r = r * n, g = g * n, b = b * n;
    }
    return {r, g, b, 0, blocktype::ROCK, 0};
}

// --- A road over a walled pit (§5.4 step 3) ---

bool roadPit(int32_t x, int32_t y, const Place &p, float roads, float &bottom, float &top) {
    if (!(roads >= 0.5f) || p.variant != 5 || !(p.dist2 <= (float)(p.radius * p.radius)))
        return false;
    float t = (std::sqrt(p.dist2) / (float)p.radius - 0.5f) * 5.0f;
    t = t * t;
    if (t > 1.0f)
        t = 1.0f;
    const float k = minOf(1.0f - (1.0f - roads) / (t * 0.75f + 0.25f), 1.0f);
    if (!(0.5f <= k))
        return false;
    top = groundHeight(x, y, heightmode::NO_VARIANT_SHAPES);
    bottom = (top - 1.0f) - (t * 40.0f + 20.0f) * (k - 0.5f);
    return true;
}

namespace {

// --- The generator ---

class Generator {
public:
    Generator(ZoneTerrain &zone, const TerrainOptions &options) : zone(zone), options(options) {}

    void run() {
        prologue();
        fillField();
        neighbours.fill(zone);
        const int32_t x0 = zone.zx * ZONE_SIZE, y0 = zone.zy * ZONE_SIZE;
        for (int lx = 0; lx < ZONE_SIZE; lx++)
            for (int ly = 0; ly < ZONE_SIZE; ly++)
                columnBlocks(x0 + lx, y0 + ly, lx, ly);
    }

private:
    ZoneTerrain &zone;
    const TerrainOptions &options;
    NeighbourGrid neighbours;
    bool spans = false;
    float spanHeight = 0; // SPH: the place's own height for rock spans

    int draw() {
        const int v = zone.random.next();
        if (options.recordDraws)
            zone.draws.push_back((uint16_t)v);
        return v;
    }

    // §3.2: the zone's place, the structure anchor and the rock spans' height.
    void prologue() {
        const int32_t pcx = floorDiv(zone.zx * ZONE_SIZE, PLACE_SHIFT), pcy = floorDiv(zone.zy * ZONE_SIZE, PLACE_SHIFT);
        zone.place = cellPlace(pcx, pcy);
        zone.placeCell = zone.place.big ? Int2{2 * floorDiv(pcx, 1), 2 * floorDiv(pcy, 1)} : Int2{pcx, pcy};
        zone.anchor = options.anchor ? options.anchor(zone.placeCell, zone.place.kind, zone.place.big) : NO_ANCHOR;
        spans = hasRockSpans(zone.place.kind);
        if (spans)
            spanHeight = rockSpanHeight(zone.place);
    }

    // §5.1: every column's values with the zone's place.
    void fillField() {
        const int32_t x0 = zone.zx * ZONE_SIZE, y0 = zone.zy * ZONE_SIZE;
        for (int lx = 0; lx < ZONE_SIZE; lx++)
            for (int ly = 0; ly < ZONE_SIZE; ly++) {
                const int32_t x = x0 + lx, y = y0 + ly;
                const ColumnValues c = columnValues(x, y, placeForColumn(zone.place, x, y), 0);
                ColumnField &f = zone.field[ZoneTerrain::index(lx, ly)];
                f.place = c.place;
                f.base = c.base;
                f.height = c.height;
                f.lake = c.lake;
                f.river = c.river;
                f.colours = colourInputs(c);
            }
    }

    // §5.4: one column's blocks.
    void columnBlocks(int32_t x, int32_t y, int lx, int ly) {
        const ColumnField &f = zone.field[ZoneTerrain::index(lx, ly)];
        const float h = f.height;
        const int32_t fz = floorToInt(h);
        bool snowed = false;
        if (f.river > 0.5f && h > -5.0f) {
            // 1. A river: its bed, and water up to its level.
            zone.store(x, y, fz - 1, riverBedColour(x, y, fz - 1, zone.sand));
            const int32_t top = floorToInt(waterLevel(f.base, f.place)) - 1;
            for (int32_t z = fz; z < top; z++)
                zone.store(x, y, z, RIVER_WATER);
        } else {
            // 2. The ground's top block, snow, and a rock span.
            const NeighbourHeights nb = neighbours.around(lx, ly);
            zone.store(x, y, fz, blockAt(x, y, fz, h, nb, f.colours, zone.sand));
            if (!(f.colours.vegetation > 0.5f) && !(f.colours.roads > 0.5f) && h > 0.0f &&
                snowAt(x, y, f.colours.warmth, nb)) {
                zone.store(x, y, fz + 1, SNOW_BLOCK);
                snowed = true;
            }
            if (spans)
                rockSpan(x, y, f);
        }
        roadOverPit(x, y, f);
        cliffs(x, y, f, fz, snowed);
    }

    void rockSpan(int32_t x, int32_t y, const ColumnField &f) {
        const RockSpan s{zone.place.kind, zone.place, zone.anchor, spanBaseHeight(zone.place, spanHeight, x, y)};
        const int32_t lo = spanBottom(s, x, y, f.height), hi = spanTop(s, x, y, f.height);
        for (int32_t z = lo; z < hi; z++)
            zone.store(x, y, z, spanColour(s, x, y, z, zone.sand));
    }

    // §5.4 step 3: a road crossing a walled pit (variant 5) keeps an earth bridge under it.
    void roadOverPit(int32_t x, int32_t y, const ColumnField &f) {
        // (The field's place is the zone's, with this column's distance: its variant is the zone place's.)
        float bottom = 0, top = 0;
        if (!roadPit(x, y, f.place, f.colours.roads, bottom, top))
            return;
        for (int32_t z = floorToInt(bottom); z <= floorToInt(top); z++)
            zone.store(x, y, z, buriedColour(x, y, z, f.colours, zone.sand));
    }

    // §5.6: overhangs cut into highland cliffs, and their faces coloured.
    void cliffs(int32_t x, int32_t y, const ColumnField &f, int32_t fz, bool snowed) {
        if (!(f.colours.highlands > 0.5f))
            return;
        const float g = worldgen::cliffs(x, y, f.place.kind, f.place.weight);
        if (!(g > 0.0f))
            return;
        const float h = f.height;
        const HeightInputs in{f.base, f.colours.highlands, f.river, f.colours.lava, f.colours.swamp, f.colours.roads,
                              f.colours.vegetation};
        const float low = finalHeight(x, y, in, f.place, heightmode::LOW);
        const float top = (noiseAt(x, y, 0.01, 7474, 0.01, 98984) + 1.0f) * 20.0f + low;
        const int32_t bottom = floorToInt(low);
        const Block old = zone.read(x, y, fz);
        const float oldR = (float)old.r + (float)old.r, oldG = (float)old.g + (float)old.g,
                    oldB = (float)old.b + (float)old.b;
        const float slope = noiseAt(x, y, 0.02, 8744, 0.02, 904) * 0.025f + 0.06f;
        bool solidBelow = true;
        int32_t gapBase = bottom;
        const bool lightSpot = (int32_t)((uint32_t)x + (uint32_t)y * 2u + (uint32_t)y) % 5 == 0;
        for (int32_t z = bottom + 1; z <= fz + 2; z++) {
            const float zf = (float)z;
            const float d = h - zf;
            const float depth = d * 0.1f + 0.5f;
            const float m = minOf(absOf((zf - top) * slope), 1.0f);
            const float t = 1.0f - (1.0f - m * m) * g;
            const float v = smooth(t) * depth;
            if (0.5f < v) {
                // Solid. Now and then a light hangs from the roof of a tall gap.
                if (!solidBelow && lightSpot && draw() % 500 == 0 && fz + 1 - z > 16) {
                    zone.lights.push_back({(int64_t)x * 65536 + 32768, (int64_t)y * 65536 + 32768, (int64_t)(z + 1) * 65536,
                                           (float)(z - gapBase)});
                    zone.store(x, y, z, GLOWING_AIR);
                }
                solidBelow = true;
                continue;
            }
            // Open: cut out, and colour the face of the block below a gap's first block.
            zone.store(x, y, z, EMPTY_BLOCK);
            if (!solidBelow)
                continue;
            solidBelow = false;
            gapBase = z - 1;
            BlockProps c = buriedColour(x, y, z - 1, scratchColours(x, y, f), zone.sand);
            uint8_t type = blocktype::CLIFF_FACE;
            const float a = minOf(1.0f, maxOf(0.0f, 1.0f - d * 0.3f));
            if (a > 0.5f)
                type = old.type;
            c.r = c.r + (oldR - c.r) * a;
            c.g = c.g + (oldG - c.g) * a;
            c.b = c.b + (oldB - c.b) * a;
            const float u = minOf(1.0f, maxOf(0.0f, 1.0f - (zf - low) * 0.3f));
            const ColourInputs scratch = scratchColours(x, y, f);
            const BlockProps e = f.colours.vegetation > 0.5f || lowShore(x, y, zf)
                                     ? buriedColour(x, y, z - 1, scratch, zone.sand)
                                     : surfaceColour(x, y, z - 1, scratch);
            if (u > 0.5f)
                type = e.type;
            c.r = c.r + (e.r - c.r) * u;
            c.g = c.g + (e.g - c.g) * u;
            c.b = c.b + u * (e.b - c.b);
            c.glow = 0, c.type = type, c.flags = 0;
            zone.store(x, y, z - 1, c);
            if (snowed && type != blocktype::CLIFF_FACE)
                zone.store(x, y, z, SNOW_BLOCK);
        }
    }
};

} // namespace

Block ZoneTerrain::read(int32_t x, int32_t y, int32_t z) const {
    if (!contains(x, y))
        return SOLID_GREY;
    const TerrainColumn &c = columns[index(x - zx * ZONE_SIZE, y - zy * ZONE_SIZE)];
    if (!c.empty() && z < c.base)
        return SOLID_GREY;
    if (!c.empty() && z < c.top()) {
        const Block &b = c.blocks[(size_t)(z - c.base)];
        if (b.type == blocktype::AIR && z <= 0 && !(b.flags & FLAG_KEEP_AIR))
            return SEA_WATER;
        return b;
    }
    return z > 0 ? AIR_BLOCK : SEA_WATER;
}

void ZoneTerrain::store(int32_t x, int32_t y, int32_t z, Block b) {
    if (!contains(x, y))
        return;
    const int i = index(x - zx * ZONE_SIZE, y - zy * ZONE_SIZE);
    TerrainColumn &c = columns[i];
    // No empty block is kept above a column (nor starts one).
    if (b.type == 0 && b.flags == 0 && (c.empty() || z >= c.top()))
        return;
    if (c.empty())
        c.base = z;
    if (z < 1 && b.type == blocktype::AIR && !(b.flags & FLAG_KEEP_AIR))
        b = SEA_WATER;
    if (z < c.base) {
        // Grow down: the new block, then the buried colour up to the old base.
        std::vector<Block> below;
        below.reserve((size_t)(c.base - z));
        below.push_back(b);
        for (int32_t k = z + 1; k < c.base; k++)
            below.push_back(toBlock(buriedColour(x, y, k, field[i].colours, sand)));
        c.blocks.insert(c.blocks.begin(), below.begin(), below.end());
        c.base = z;
        return;
    }
    if (z >= c.top())
        c.blocks.resize((size_t)(z - c.base), EMPTY_BLOCK), c.blocks.push_back(b);
    else
        c.blocks[(size_t)(z - c.base)] = b;
}

ZoneTerrain zoneTerrain(const WorldValues &world, int32_t zx, int32_t zy, const TerrainOptions &options) {
    ZoneTerrain zone;
    zone.zx = zx, zone.zy = zy;
    zone.field.resize(ZONE_COLUMNS);
    zone.columns.resize(ZONE_COLUMNS);
    zone.sand = sandOffsets(world);
    zone.random.seed(zoneRandomSeed(world, zx, zy));
    Generator(zone, options).run();
    return zone;
}

} // namespace worldgen
