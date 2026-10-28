// The world map's model of a map region and its markers, written from docs/specs/worldmap-model.md
// (sections cited as §n; "zone §n" is worldgen-terrain-zone.md).
#include "worldgen/MapModel.h"

#include "worldgen/BlockColours.h"
#include "worldgen/BuildingRules.h"
#include "worldgen/Generator.h"
#include "worldgen/Nature.h"
#include "worldgen/ZoneBuild.h"
#include "worldgen/ZoneTerrain.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <set>

namespace worldgen {
namespace {

constexpr int32_t CELL_SHIFT = 4; // 16-block cells

// Cell objects the model knows by number (structures §6).
namespace object {
constexpr int FIR = 2, BIRCH = 3, SPLIT_CROWN = 4, PALM_LIKE = 5, BUSH = 7, BIG_TREE = 8;
constexpr int BOULDER = 9;
constexpr int SHRINE = 53;
constexpr int ROUND_TOWER = 52;
constexpr int WATER_TOWER = 66, AQUEDUCT = 67, CISTERN = 68, PLATFORM = 69, HIGH_PLATFORM = 70;
constexpr int GARDEN = 62, STONES = 54, CRAG = 55, NO_ROOF = 60, PATH_STONES = 34;
} // namespace object

// Place list kinds (§6.4-6.6).
constexpr int SHRINE_PLACE = 12, LORE_PLACE = 14, TREASURE_PLACE = 15;
constexpr int FARM_BLOCK = 13;

constexpr Colour TRUNK{200, 150, 100}, BIRCH_TRUNK{255, 255, 255};
constexpr Colour FIELD_STONE{100, 200, 100}, PATH_STONE{100, 100, 100}, TOWER_STONE{140, 150, 180};
constexpr int MAP_LOW_ZOOM = 2; // levels up to here have no haze

Colour colourOf(const BlockProps &p) { return {p.r, p.g, p.b}; }
Colour snowColour() { return colourOf(SNOW_BLOCK); }

// The cell hashes the model reads (the cell's tower, tree and size numbers).
uint32_t cellHash(int32_t cx, int32_t cy, uint32_t salt) { return random(cellKey(cx, cy) + salt); }
int32_t towerExtra(int32_t cx, int32_t cy) { return (int32_t)(cellHash(cx, cy, 0x5d0ba) % 40u + 100u); }   // 0x2d9db0
int32_t treeHeight(int32_t cx, int32_t cy) { return (int32_t)(cellHash(cx, cy, 0x12ce32c) % 10u + 12u); } // 0x2d9e70
int32_t treeRadius(int32_t cx, int32_t cy) { return (int32_t)(cellHash(cx, cy, 0x2fbcc94) % 10u + 20u); } // 0x2d9ed0
int32_t towerRadius(int32_t cx, int32_t cy) { return (int32_t)(cellHash(cx, cy, 0x255509) % 10u + 20u); } // 0x2d9e10
int32_t platformRadius(uint32_t style) { return (int32_t)(xorshift(style + 0x17f55) % 13u) + 20; }       // 0x25eca0

bool oneOf(int v, std::initializer_list<int> set) { return std::find(set.begin(), set.end(), v) != set.end(); }
bool castleStructure(int t) { return t >= 45 && t <= 47; } // 0x25eb30
bool fortStructure(int t) { return t >= 40 && t <= 43; }   // 0x25eb40
bool isEdge(int32_t v, int32_t m) { return v == 0 || v == m - 1; }

// A model column's values (§3.2): the height pipeline at (X, Y) with its reach-gated place.
struct Column {
    int32_t X = 0, Y = 0;
    ColumnValues v;
    ColourInputs ci;
    float h0 = 0; // height mode 0
};

// A cell's object and its point (structures §6.1), and the pipeline at the point its tree or
// rock stands on, made when first asked for.
struct Cell {
    int type = 0;
    Int2 point;
    bool pointMade = false;
    Int2 standsAt;
    ColumnValues at;
    ColourInputs atColours;
};

class ModelBuilder {
public:
    ModelBuilder(MapRegion &m)
        : m(m), world(currentWorld()), cells(cellObjectsFor(world)), rules(cells), sand(sandOffsets(world)), u(m.unit),
          r(m.reach), B(MAP_REGION_UNITS * m.unit) {}

    void build() {
        columnTops();
        columnsPass();
        shade();
        markers();
    }

private:
    MapRegion &m;
    const WorldValues &world;
    CellObjects &cells;
    BuildingRules rules;
    SandOffsets sand;
    const int u, r;
    const int32_t B;
    int32_t lowU = 0; // lowest / u
    float hazeWeight = 0;
    std::vector<Column> columns;  // MAP_MODEL_SIDE^2, index (a + 3) * side + (b + 3)
    std::vector<int32_t> tops;    // G(a, b), same index
    std::map<std::pair<int32_t, int32_t>, Cell> cellCache;

    static size_t index(int a, int b) { return (size_t)(a + MAP_BORDER) * MAP_MODEL_SIDE + (size_t)(b + MAP_BORDER); }
    int32_t G(int a, int b) const { return tops[index(a, b)]; }

    // --- Voxels ---

    // Storing a colour (§0): each channel halved, clamped to 0..255, truncated.
    void store(int i, int j, int k, Colour c) {
        if (i < 0 || j < 0 || k < 0 || i >= m.side || j >= m.side || k >= m.sizeZ)
            return;
        const Block b = toBlock(BlockProps{c.r, c.g, c.b, 0, 0, 0});
        uint8_t *v = &m.rgb[(((size_t)k * m.side + j) * m.side + i) * 3];
        v[0] = b.r, v[1] = b.g, v[2] = b.b;
    }
    void store(int i, int j, int k, const BlockProps &p) { store(i, j, k, colourOf(p)); }
    void fill(int i, int j, int k1, int k2, Colour c) {
        for (int k = k1; k <= k2; k++)
            store(i, j, k, c);
    }
    // A block height to a voxel index (§3.6 "zv").
    int32_t zv(float f) const { return trunc32(f / (float)u - (float)lowU); }
    int32_t zv(int32_t n) const { return n / u - lowU; }

    // --- Cells ---

    Cell &cell(int32_t cx, int32_t cy) {
        auto [it, fresh] = cellCache.try_emplace({cx, cy});
        if (fresh) {
            it->second.type = cells.type(cx, cy);
            it->second.point = cellPoint(cx, cy);
        }
        return it->second;
    }
    // The pipeline where the cell's tree or rock stands: its point, or the cell's centre for the
    // big tree and the big rocks.
    Cell &standing(int32_t cx, int32_t cy) {
        Cell &c = cell(cx, cy);
        if (!c.pointMade) {
            const bool centred = c.type == object::BIG_TREE || (c.type >= 11 && c.type <= 14);
            c.standsAt = centred ? Int2{16 * cx + 8, 16 * cy + 8} : c.point;
            c.at = columnValues(c.standsAt.x, c.standsAt.y);
            c.atColours = colourInputs(c.at);
            c.pointMade = true;
        }
        return c;
    }
    // The height there with lava and swamp left out.
    static float dryHeight(const Cell &c, int mode) {
        HeightInputs in = c.at.heightInputs();
        in.lava = 0.0f, in.swamp = 0.0f;
        return finalHeight(c.standsAt.x, c.standsAt.y, in, c.at.place, mode);
    }
    bool inReach(int type, int dx, int dy) const { return !(cellReach(type) < std::abs(dx) || cellReach(type) < std::abs(dy)); }

    // --- §2 Pass 1: column tops ---

    // How far above its top a column's objects can reach (pass 1 step 4).
    int32_t objectRise(int t, int32_t cx, int32_t cy) const {
        if (t == object::BOULDER || t < 1 || t > 15) {
            int32_t e = buildingHeight(t);
            if (t == 43 || t == 47)
                e += 16;
            else if (t == object::ROUND_TOWER)
                e += towerExtra(cx, cy);
            else if (t == object::WATER_TOWER)
                e += 15;
            else if (t == object::CISTERN)
                e += 10;
            else if (t == object::AQUEDUCT)
                e += 8;
            return e;
        }
        int32_t e = treeHeight(cx, cy);
        if (t == object::FIR)
            e = trunc32((float)e * 1.5f);
        else if (t == object::BIG_TREE)
            e *= 3;
        else if (t == 11 || t == 13)
            e = 40;
        else if (t == 12 || t == 14)
            e = 80;
        return e;
    }

    // The rock span of a column's place (pass 1 step 5, §3.5 step 1): the anchor without the
    // zone generator's big-structure gate.
    RockSpan rockSpan(const Column &c) const {
        const Place &p = c.v.place;
        const Int2 anchor = structureAnchor(floorDiv(c.X, PLACE_SHIFT), floorDiv(c.Y, PLACE_SHIFT), p.kind, p.big);
        return {p.kind, p, anchor, spanBaseHeight(p, rockSpanHeight(p), c.X, c.Y)};
    }

    void columnTops() {
        columns.resize((size_t)MAP_MODEL_SIDE * MAP_MODEL_SIDE);
        tops.resize(columns.size());
        int32_t lowest = 0, highest = 0;
        bool first = true;
        for (int a = -MAP_BORDER; a < MAP_REGION_UNITS + MAP_BORDER; a++)
            for (int b = -MAP_BORDER; b < MAP_REGION_UNITS + MAP_BORDER; b++) {
                Column &c = columns[index(a, b)];
                c.X = u * a + B * m.rx, c.Y = u * b + B * m.ry;
                c.v = columnValues(c.X, c.Y);
                c.ci = colourInputs(c.v);
                c.h0 = c.v.height;
                // 1-3. The top: the ground, or a river's water.
                float h4 = finalHeight(c.X, c.Y, c.v.heightInputs(), c.v.place, heightmode::CLIFF_STEPS);
                if (h4 > 0.0f && c.v.river > 0.5f)
                    h4 = waterLevel(c.v.base, c.v.place);
                const int32_t top = h4 > 0.0f ? floorToInt(h4 + 0.999f) : 0;
                // 4. What the objects of the cells in reach add.
                int32_t e = 2;
                for (int dx = -r; dx <= r; dx++)
                    for (int dy = -r; dy <= r; dy++) {
                        const int32_t cx = floorDiv(c.X, CELL_SHIFT) + dx, cy = floorDiv(c.Y, CELL_SHIFT) + dy;
                        const int t = cell(cx, cy).type;
                        if (t == 0 || !inReach(t, dx, dy))
                            continue;
                        e = std::max(e, objectRise(t, cx, cy));
                    }
                // 5. Rock spans.
                if (hasRockSpans(c.v.place.kind))
                    e = std::max(e, spanTop(rockSpan(c), c.X, c.Y, h4) - floorToInt(h4));
                // 6-8.
                const float h1 = finalHeight(c.X, c.Y, c.v.heightInputs(), c.v.place, heightmode::NO_VARIANT_SHAPES);
                const int32_t peak = std::max(trunc32(h1), top) + e;
                if (first || top < lowest)
                    lowest = top;
                if (first || peak > highest)
                    highest = peak;
                first = false;
                tops[index(a, b)] = top;
            }
        m.lowest = lowest;
        lowU = lowest / u;
        m.sizeZ = std::min(highest / u - lowest / u + 8, MAP_MOST_VOXELS);
        m.rgb.assign((size_t)m.side * m.side * (size_t)std::max(m.sizeZ, 0) * 3, 0);
        float w = (float)(m.level - MAP_LOW_ZOOM) * 0.2f;
        if (w >= 1.0f)
            w = 1.0f;
        hazeWeight = w > 0.0f ? w : 0.0f;
    }

    // --- §3 Pass 2: the columns ---

    // The surroundings a column's colours read (§3.2).
    struct Around {
        NeighbourHeights nb;
        int biome = 0;
        bool water = false;
        int32_t t = 0;
    };

    void columnsPass() {
        for (int a = -MAP_BORDER; a < MAP_REGION_UNITS + MAP_BORDER; a++)
            for (int b = -MAP_BORDER; b < MAP_REGION_UNITS + MAP_BORDER; b++)
                column(a, b);
    }

    void column(int a, int b) {
        const Column &c = columns[index(a, b)];
        const int i = a + MAP_BORDER, j = b + MAP_BORDER;
        const int32_t X = c.X, Y = c.Y;
        Around ar;
        ar.t = G(a, b);
        const int32_t t = ar.t;
        int32_t z0 = t / u - lowU;
        const Int2 region = regionAt(X, Y);
        ar.biome = regionBiome(region.x, region.y);
        // 3.1 Region borders.
        if (a >= 0 && a < MAP_REGION_UNITS && b >= 0 && b < MAP_REGION_UNITS) {
            const Int2 others[6] = {regionAt(X + u, Y), regionAt(X, Y + u), regionAt(X + u, Y + u),
                                    regionAt(X - u, Y), regionAt(X, Y - u), regionAt(X - u, Y - u)};
            if (std::any_of(std::begin(others), std::end(others), [&](Int2 o) { return o != region; }))
                m.borders[{region.x, region.y}].push_back({X, Y, t});
        }
        // 3.2 The neighbours' heights, and the lowest voxel to fill.
        const float h0 = c.h0;
        ar.nb = {groundHeight(X - 1, Y, 0), groundHeight(X + 1, Y, 0), groundHeight(X, Y - 1, 0), groundHeight(X, Y + 1, 0)};
        int32_t lo = z0;
        auto lower = [&](int na, int nb) { lo = std::min(lo, G(na, nb) / u - lowU + 1); };
        if (a > -2)
            lower(a - 1, b);
        if (b > -2)
            lower(a, b - 1);
        if (a < 17)
            lower(a + 1, b);
        if (b < 17)
            lower(a, b + 1);
        // 3.3 A water column's colour.
        ar.water = t < 1 || c.v.river > 0.5f;
        const Colour water = ar.water ? waterColour(c, ar) : Colour{};
        // 3.4 The fill.
        if (!ar.water || (t > 0 && u >= 128))
            z0 += 1;
        for (int32_t k = lo; k <= z0; k++) {
            const int32_t zb = t - u * (z0 - k);
            store(i, j, k, ar.water ? water : landColour(c, ar, zb));
        }
        // 3.5 Rock spans, snow, road pits.
        if (hasRockSpans(c.v.place.kind)) {
            const RockSpan s = rockSpan(c);
            const int32_t bottom = spanBottom(s, X, Y, h0), top = spanTop(s, X, Y, h0);
            if ((float)top > h0) {
                const BlockProps rock = spanColour(s, X, Y, t, sand);
                for (int32_t k = bottom / u - lowU + 1; k <= top / u - lowU + 1; k++)
                    store(i, j, k, rock);
            }
        }
        if (c.ci.roads < 0.5f && !ar.water && c.ci.vegetation < 0.5f && snowAt(X, Y, c.ci.warmth, ar.nb))
            store(i, j, z0, SNOW_BLOCK);
        float pitBottom = 0, pitTop = 0;
        if (u < 2048 && roadPit(X, Y, c.v.place, c.ci.roads, pitBottom, pitTop)) {
            const BlockProps earth = buriedColour(X, Y, t, c.ci, sand);
            for (int32_t k = floorToInt(pitBottom) / u - lowU + 1; k < floorToInt(pitTop) / u - lowU + 2; k++)
                store(i, j, k, earth);
        }
        // 3.6 The objects near the column.
        for (int dx = -r; dx <= r; dx++)
            for (int dy = -r; dy <= r; dy++)
                nearObject(c, ar, i, j, dx, dy, z0);
        // 3.7 Haze.
        if (t > 0)
            haze(i, j, t / u - lowU + (ar.water ? 1 : 0), h0, c.ci);
    }

    // §3.3: the water's colour, the same for the whole column.
    Colour waterColour(const Column &c, const Around &ar) const {
        const float h0 = c.h0;
        const float d = h0 > 200.0f ? 200.0f : h0;
        float q = (float)std::fabs((double)d) * 0.005f + 1.0f;
        if (!(q > 0.0f))
            q = 0.0f;
        float k = (float)std::fabs((double)(1.0f - q));
        if (k > 1.0f)
            k = 1.0f;
        if (ar.t > 0) {
            float s = (c.v.river - 0.5f) * 3.0f;
            if (s >= 1.0f)
                s = 1.0f;
            k = k * smooth(s);
        }
        const float g0 = 180.0f - k * 100.0f;
        Colour w{0.0f, g0, 255.0f};
        if (d > 0.0f) {
            float f = d * 0.01f;
            if (!(f > 0.0f))
                f = 0.0f;
            if (!(f < 1.0f))
                f = 1.0f;
            w.g = (220.0f - g0) * c.ci.forestRegion * f + g0;
            w.b = 255.0f - c.ci.forestRegion * f * 80.0f;
            if (u > 32) {
                // Far out, toward the ground's own colour.
                const Colour p = snowAt(c.X, c.Y, c.ci.warmth, ar.nb) ? snowColour()
                                                                     : colourOf(blockAt(c.X, c.Y, ar.t, h0, ar.nb, c.ci, sand));
                float mix = (float)(u - 32) * 0.0025f;
                if (mix >= 1.0f)
                    mix = 1.0f;
                w.r = w.r + mix * (p.r - w.r);
                w.g = w.g + mix * (p.g - w.g);
                w.b = w.b + mix * (p.b - w.b);
            }
        }
        return w;
    }

    // §3.4: a land voxel's colour at block height zb, blended toward the cliff colours on
    // highland cliffs (no carving).
    Colour landColour(const Column &c, const Around &ar, int32_t zb) const {
        const int32_t X = c.X, Y = c.Y;
        const float h0 = c.h0;
        Colour col = colourOf(blockAt(X, Y, zb, h0, ar.nb, c.ci, sand));
        if (!(c.ci.highlands > 0.5f) || u >= 128)
            return col;
        const float gc = cliffs(X, Y, c.v.place.kind, c.v.place.weight);
        if (!(gc > 0.0f))
            return col;
        const float low = finalHeight(X, Y, c.v.heightInputs(), c.v.place, heightmode::LOW);
        const float zf = (float)zb;
        const Colour face = colourOf(blockAt(X, Y, floorToInt(h0), h0, ar.nb, c.ci, sand));
        const int32_t zc = trunc32(zf - 1.0f);
        const Colour buried = colourOf(buriedColour(X, Y, zc, c.ci, sand));
        const float alpha = clampUnit(1.0f - ((h0 - zf) - 1.0f) * 0.3f);
        const Colour mid{buried.r + alpha * (face.r - buried.r), buried.g + alpha * (face.g - buried.g), buried.b + alpha * (face.b - buried.b)};
        const float beta = clampUnit(1.0f - (zf - low) * 0.3f);
        const Colour e = c.ci.vegetation > 0.5f || lowShore(X, Y, zf) ? buried : colourOf(surfaceColour(X, Y, zc, c.ci));
        col.b = mid.b + (e.b - mid.b) * beta;
        col.r = mid.r + beta * (e.r - mid.r);
        col.g = mid.g + (e.g - mid.g) * beta;
        return col;
    }
    // 0 unless v >= 0, then 1 unless v <= 1.
    static float clampUnit(float v) {
        v = v >= 0.0f ? v : 0.0f;
        return v <= 1.0f ? v : 1.0f;
    }

    // --- §3.6 Objects near the column ---

    // The water line of mason §3: N(0.05 X + 9076, 0.05 Y + 8998) x b + a.
    static float waterLine(int32_t X, int32_t Y, float a, float b) { return noiseAt(X, Y, 0.05, 9076, 0.05, 8998) * b + a; }

    void nearObject(const Column &c, const Around &ar, int i, int j, int dx, int dy, int32_t &z0) {
        const int32_t X = c.X, Y = c.Y;
        const int32_t cx = floorDiv(X, CELL_SHIFT) + dx, cy = floorDiv(Y, CELL_SHIFT) + dy;
        const Cell &here = cell(cx, cy);
        const int T = here.type;
        if (T == 0 || !inReach(T, dx, dy))
            return;
        if (T >= 1 && T <= 8)
            tree(c, i, j, cx, cy, T, z0);
        if (u >= 256)
            return;
        // The realm of the column's place cell, for the building tests and colours.
        uint32_t realm = 0;
        if (T >= 13)
            realm = placeStyle(floorDiv(X, PLACE_SHIFT), floorDiv(Y, PLACE_SHIFT));
        const ColourContext ctx{realm, buildingCulture(realm, ar.biome)};
        const bool own = dx == 0 && dy == 0;
        const int32_t cellX = 16 * cx, cellY = 16 * cy;

        // 1. The round tower.
        if (T == object::ROUND_TOWER) {
            roundTower(c, i, j, cx, cy, realm, ctx, z0);
            return;
        }
        if (T == object::WATER_TOWER) {
            // 2. The water tower.
            const float ex = (float)(cellX - X + 8), ey = (float)(cellY - Y + 8);
            const int32_t d = trunc32(ey * ey + ex * ex);
            if (d <= 1024) {
                int32_t kt = std::max(zv(rules.baseHeight(cx, cy, T) + 10), z0);
                Colour col = wallColour(T, cx, cy, ctx);
                if (d <= 64)
                    kt += 1, col = TOWER_STONE;
                fill(i, j, z0, kt, col);
            }
        } else {
            // 3. The tower platforms.
            if (T == object::PLATFORM || T == object::HIGH_PLATFORM)
                platform(c, i, j, cx, cy, T, realm, ctx, z0);
            if (T == object::AQUEDUCT) {
                // 4. The aqueduct's channel.
                if (u < 32) {
                    const int32_t m = 16 / u;
                    const int32_t ix = posMod(floorDivBy(X, u), m), iy = posMod(floorDivBy(Y, u), m);
                    if ((iy == m / 2 && (cells.type(cx - 1, cy) == T || cells.type(cx + 1, cy) == T)) ||
                        (ix == m / 2 && (cells.type(cx, cy - 1) == T || cells.type(cx, cy + 1) == T)))
                        store(i, j, z0 + 1, scaled(wallColour(T, cx, cy, ctx), (float)((ix + iy) % 2) * 0.1f + 0.9f));
                }
            } else {
                otherTypes(c, ar, i, j, cx, cy, T, own, realm, ctx, z0);
            }
        }
        // 6. Walls and roofs.
        if ((keepsHeight(T, realm) || castleStructure(T) || fortStructure(T)) && own && T != object::NO_ROOF)
            wallsAndRoof(c, ar, i, j, cx, cy, T, realm, ctx, z0);
        // 7. Rocks and boulders.
        if (T >= 9 && T <= 15) {
            rocks(c, ar, i, j, cx, cy, T, z0);
            return;
        }
        // 8. Stepping stones on a path.
        if (T == object::PATH_STONES && own && c.ci.roads < 0.45f && posMod(floorDivBy(X, u), 4) == 2 &&
            posMod(floorDivBy(Y, u), 4) == 2 && random(cellKey(X, Y)) % 10u != 0) {
            z0 += 1;
            store(i, j, z0, PATH_STONE);
        }
    }

    // §3.6.1: a tree's trunk and leaves.
    void tree(const Column &c, int i, int j, int32_t cx, int32_t cy, int T, int32_t z0) {
        const int32_t X = c.X, Y = c.Y;
        float rad = (float)treeRadius(cx, cy) * 0.75f;
        if (T == object::BIRCH)
            rad = rad * 0.75f;
        else if (T == object::BIG_TREE)
            rad = rad + rad;
        const Cell &p = standing(cx, cy);
        const int32_t px = p.standsAt.x, py = p.standsAt.y;
        const float ox = (float)(px - X), oy = (float)(py - Y);
        const float d2 = ox * ox + oy * oy;
        if (rad * rad < d2)
            return;
        const float hp = dryHeight(p, heightmode::CLIFF_STEPS);
        const ColourInputs &k = p.atColours;
        const float f = (float)treeHeight(cx, cy);
        int32_t th = trunc32(f + f);
        if (T == object::BIG_TREE)
            th *= 2;
        float q = d2 / (rad * rad);
        if (k.warmth >= 0.7f && k.dryness < 0.3f)
            q = 1.0f - (1.0f - q) * 0.5f;
        // The leaves' span.
        int32_t kb = 0, topVoxel = 0;
        if (T == object::FIR) {
            th = trunc32((float)th * 1.5f);
            kb = zv((float)(th / 3) + hp);
            topVoxel = zv((hp + (float)th) - (std::sqrt(q) * 2.0f * (float)th) / 3.0f);
        } else {
            const float mid = q * (float)th * 0.25f;
            kb = zv(((float)(th / 2) + hp) + mid);
            topVoxel = zv((hp + (float)th) - mid);
        }
        kb += 1;
        int32_t kt = topVoxel + 1;
        if (T == object::PALM_LIKE || T == object::BUSH) {
            kb = topVoxel;
            const int32_t ex = floorDivBy(px - X, u), ey = floorDivBy(py - Y, u);
            if (std::abs(ex) != std::abs(ey) && ex != 0 && ey != 0)
                kb = topVoxel + kt; // no leaves in this column
        } else if (T == object::SPLIT_CROWN) {
            const uint32_t n = random(cellKey(X, Y) + 0x714ef);
            if (n % 4u == 0) {
                int32_t s = (th / u) / 2 + 1;
                if (s < 2)
                    s = 1;
                kb -= (int32_t)((n / 4u) % (uint32_t)s);
            }
        }
        if (u > 16)
            kb = std::max(kb, z0 + 1), kt = std::max(kt, z0 + 1);
        // The trunk.
        float rt = rad * 0.2f;
        if (rt <= 1.0f)
            rt = 1.0f;
        if (d2 < rt * rt || (X / u == px / u && Y / u == py / u))
            fill(i, j, z0 + 1, kb - 1, T == object::BIRCH ? BIRCH_TRUNK : TRUNK);
        // The leaves, the mean of the tree's two colours; snow on them where it is cold.
        const LeafKind kind = T == object::FIR ? LeafKind::Fir : T == object::BIRCH ? LeafKind::Birch : LeafKind::Common;
        const TreeLeaves l = treeLeaves(kind, px, py, k.warmth, k.dryness, k.forestRegion, k.kind27);
        const Colour leaf{l.a.r * 0.5f + l.b.r * 0.5f, l.a.g * 0.5f + l.b.g * 0.5f, l.a.b * 0.5f + l.b.b * 0.5f};
        if (kt >= kb)
            for (int32_t z = kb; z <= kt; z++) {
                store(i, j, z, leaf);
                if (k.warmth < 0.15f)
                    store(i, j, z, snowColour());
            }
    }

    // §3.6.2 rule 1: the round tower (52), its wall, rim and roof.
    void roundTower(const Column &c, int i, int j, int32_t cx, int32_t cy, uint32_t realm, const ColourContext &ctx, int32_t z0) {
        const int T = object::ROUND_TOWER;
        const int32_t rr = towerRadius(cx, cy);
        const double dX = (double)(16 * cx) + 0.5 + 8.0 - (double)c.X, dY = (double)(16 * cy) + 0.5 + 8.0 - (double)c.Y;
        const int32_t d2 = trunc32(dY * dY + dX * dX);
        if (rr * rr < d2)
            return;
        const int32_t top = rules.baseHeight(cx, cy, T) + towerExtra(cx, cy) - 2 * rr;
        int32_t kt = zv(top);
        if ((rr - u) * (rr - u) <= d2 && !(hasBattlements(T, realm) && (floorDivBy(c.X, u) + floorDivBy(c.Y, u)) % 2 != 0))
            kt += 1;
        fill(i, j, z0 + 1, kt, wallColour(T, cx, cy, ctx));
        if (keepsHeight(T, realm)) {
            const int32_t kr = zv((1.0f - std::sqrt((float)d2) / (float)rr) * (float)(2 * rr) + (float)top);
            fill(i, j, kt, kr, roofColour(T, cx, cy, ctx));
        }
    }

    // §3.6.2 rule 3: the tower platforms (69, 70), with their posts at fine zoom levels.
    void platform(const Column &c, int i, int j, int32_t cx, int32_t cy, int T, uint32_t realm, const ColourContext &ctx, int32_t z0) {
        const int32_t X = c.X, Y = c.Y;
        // (The spec gives 28 for the high platform; worldmap-model.md §3.6.2 rule 3.)
        const int32_t rad = T == object::HIGH_PLATFORM ? 28 : platformRadius(realm);
        const float ex = (float)(16 * cx - X + 8), ey = (float)(16 * cy - Y + 8);
        if (trunc32(ey * ey + ex * ex) > rad * rad)
            return;
        const int32_t lift = T == object::PLATFORM ? 3 : 10;
        int32_t top = rules.baseHeight(cx, cy, T) + lift;
        bool posts = false;
        if (u < 16) {
            const int32_t ax = std::abs(floorDivBy(X, u) - floorDivBy(16 * cx + 8, u));
            const int32_t ay = std::abs(floorDivBy(Y, u) - floorDivBy(16 * cy + 8, u));
            const int32_t k1 = trunc32((float)rad * 0.7f) / u, k2 = (rad - 1) / u;
            if ((ax == k1 && ay == k1) || (ax == k2 && ay == 0) || (ax == 0 && ay == k2)) {
                top += 15;
                if (waterLineTest(T, realm)) {
                    const int32_t v = trunc32(waterLine(X, Y, (float)(lift + 10), 10.0f) + c.h0);
                    top = std::max(top - 15, std::min(top, v));
                }
                posts = true;
            }
        }
        const int32_t kt = zv(top) + 1;
        const Colour col = wallColour(T, cx, cy, ctx);
        const bool grassTop = !posts && grassyFloor(T, realm) && simplex((double)X * 0.06 + 7631, (double)Y * 0.06 + 983) > 0.0f;
        for (int32_t k = z0; k <= kt; k++)
            store(i, j, k, k == kt && grassTop ? colourOf(surfaceColour(X, Y, k, c.ci)) : col);
    }

    // §3.6.2 rule 5: fields, the garden, walls and the crags.
    void otherTypes(const Column &c, const Around &ar, int i, int j, int32_t cx, int32_t cy, int T, bool own, uint32_t realm,
                    const ColourContext &ctx, int32_t &z0) {
        const int32_t X = c.X, Y = c.Y;
        // a. Fields.
        if (T >= 27 && T <= 33 && c.ci.roads <= 0.5f && c.v.river <= 0.5f) {
            const int32_t v = (floorDivBy(cx, 3) + floorDivBy(cy, 3)) % 2 != 0 ? X : Y;
            const int32_t s = (v / u) % 2;
            store(i, j, z0, scaled(colourOf(riverBedColour(X, Y, ar.t, sand)), (float)s * 0.05f + 0.75f));
            if (posMod(floorDivBy(X, u), 3) == 1 && posMod(floorDivBy(Y, u), 3) == 1 && random(cellKey(X, Y)) % 4u != 0) {
                z0 += 1;
                store(i, j, z0, FIELD_STONE);
            }
        }
        // b. The garden.
        if (T == object::GARDEN) {
            // (The buried colour "from scratch" reads the column's own reach-gated values.)
            const int32_t ox = X - 16 * cx - 8, oy = Y - 16 * cy - 8;
            if (ar.t > 0 && ox * ox + oy * oy < 256) {
                const float n = simplex((double)X * 0.25, (double)Y * 0.25);
                store(i, j, z0, scaled(colourOf(buriedColour(X, Y, ar.t, c.ci, sand)), (float)std::fabs(n) * 0.3f + 0.7f));
            }
            return;
        }
        // c. Walls on their own cell.
        if (own && oneOf(T, {16, 26, 50, 51, 55, 57, 61, 63, 64, 65})) {
            const int32_t base = rules.baseHeight(cx, cy, T);
            const Colour col = wallColour(T, cx, cy, ctx);
            int32_t kt = zv(base + 1);
            z0 -= 1;
            bool corner = false;
            if (u < 17) {
                const int32_t m = 16 / u;
                const int32_t ix = posMod(floorDivBy(X, u), m), iy = posMod(floorDivBy(Y, u), m);
                if (oneOf(T, {16, 50, 51, 57, 61}) && isEdge(ix, m) && isEdge(iy, m)) {
                    int32_t top = base + 10;
                    if (waterLineTest(T, realm))
                        top = std::min(top, trunc32(waterLine(X, Y, 10.0f, 10.0f) + c.h0));
                    kt = zv(top) + 1;
                    corner = true;
                } else if (T >= 63 && T <= 65 && (!isEdge(ix, m) || !isEdge(iy, m))) {
                    z0 = kt - 1;
                }
            }
            while (z0 < kt) {
                z0 += 1;
                store(i, j, z0, col);
            }
            if (!corner && grassyFloor(T, realm) && simplex((double)X * 0.1 + 89834, (double)Y * 0.1 + 12233) > 0.25f)
                store(i, j, kt, surfaceColour(X, Y, base, c.ci));
        }
        // d. The crags (55, on any cell in reach) and the stones (54).
        if (T == object::CRAG) {
            const float ex = (float)trunc32((float)(16 * cx + 8 - X)) - 0.5f, ey = (float)(16 * cy + 8 - Y) - 0.5f;
            const float d = ey * ey + ex * ex;
            if (!(d > 25.0f && u < 9)) {
                const float q = d / 25.0f;
                rockFill(c, i, j, z0, zv((1.0f - q * q) * 20.0f + (float)rules.baseHeight(cx, cy, T)) + 2);
            }
        } else if (T == object::STONES) {
            const Int2 p = cell(cx, cy).point;
            const float ex = (float)(p.x - X) - 0.5f, ey = (float)(p.y - Y) - 0.5f;
            const float d = ey * ey + ex * ex;
            if (d <= 64.0f || u > 8) {
                const float q = d * 0.015625f;
                rockFill(c, i, j, z0, zv((1.0f - q * q) * 25.0f + (float)rules.baseHeight(cx, cy, T)) + 2);
            }
        }
    }

    void rockFill(const Column &c, int i, int j, int32_t z0, int32_t kt) {
        for (int32_t k = z0; k <= kt; k++)
            store(i, j, k, rockColour(c.X, c.Y, u * k + m.lowest, c.ci.warmth, c.ci.lavaRegion, c.ci.forestRegion));
    }

    // §3.6.2 rule 6: a building's walls, battlements and roof on its own cell.
    void wallsAndRoof(const Column &c, const Around &ar, int i, int j, int32_t cx, int32_t cy, int T, uint32_t realm,
                      const ColourContext &ctx, int32_t &z0) {
        const int32_t X = c.X, Y = c.Y;
        const int32_t base = rules.baseHeight(cx, cy, T), bh = buildingHeight(T);
        const Colour col = wallColour(T, cx, cy, ctx);
        const bool waterline = waterLineTest(T, realm);
        const int32_t waterTop = waterline ? trunc32(waterLine(X, Y, 10.0f, 10.0f) + c.h0) : 0;
        int32_t top = bh + base;
        if (waterline)
            top = std::min(top, waterTop);
        int32_t kt = zv(top);
        if (u < 17 && (T == 13 || oneOf(T, {49, 53, 86, 95}))) {
            const int32_t m = 16 / u;
            if (!isEdge(posMod(floorDivBy(X, u), m), m) || !isEdge(posMod(floorDivBy(Y, u), m), m))
                kt = zv(base) + 1; // inside the walls only the floor shows
        }
        while (z0 <= kt) {
            z0 += 1;
            store(i, j, z0, col);
        }
        z0 = zv(bh + base);
        if (u < 16 && hasBattlements(T, realm) && crenel(X, Y, cx, cy, T)) {
            z0 += 1;
            store(i, j, z0, col);
        }
        if (keepsHeight(T, realm)) {
            const int ridge = rules.ridgeSides(cx, cy, T, rules.joins(cx, cy));
            const int culture = buildingCulture(realm, ar.biome);
            const int32_t rh = BuildingRules::roofHeight(X + u / 2 - 16 * cx, Y + u / 2 - 16 * cy, T, ridge, culture);
            int32_t rt = rh + bh + base;
            if (waterline)
                rt = std::min(rt, waterTop);
            const int32_t kr = zv(rt) + 1;
            const Colour roof = roofColour(T, cx, cy, ctx);
            while (z0 < kr) {
                z0 += 1;
                store(i, j, z0, roof);
            }
        }
    }
    // A battlement's tooth on the wall's edge, unless the neighbour joins as a tower.
    bool crenel(int32_t X, int32_t Y, int32_t cx, int32_t cy, int T) const {
        const int32_t m = 16 / u;
        const int32_t ix = posMod(floorDivBy(X, u), m);
        if (floorDivBy(Y, u) % 2 == 0 && ((ix == 0 && !joinsTower(T, cells.type(cx - 1, cy))) ||
                                           (ix == m - 1 && !joinsTower(T, cells.type(cx + 1, cy)))))
            return true;
        const int32_t iy = posMod(Y / u, m); // (C division here)
        return floorDivBy(X, u) % 2 == 0 && ((iy == 0 && !joinsTower(T, cells.type(cx, cy - 1))) ||
                                             (iy == m - 1 && !joinsTower(T, cells.type(cx, cy + 1))));
    }

    // §3.6.2 rule 7: rocks (9) and boulders and big rocks (10-15).
    void rocks(const Column &c, const Around &ar, int i, int j, int32_t cx, int32_t cy, int T, int32_t z0) {
        const int32_t X = c.X, Y = c.Y;
        float rr = (float)treeRadius(cx, cy), rh = (float)treeHeight(cx, cy);
        if (T == 11 || T == 13)
            rh = 40.0f, rr = 32.0f;
        else if (T == 12 || T == 14)
            rh = 80.0f, rr = 32.0f;
        const Cell &p = standing(cx, cy);
        const int32_t px = p.standsAt.x, py = p.standsAt.y;
        const float ox = (float)(px - X), oy = (float)(py - Y);
        if (rr * rr < ox * ox + oy * oy)
            return;
        const float hp = std::max(dryHeight(p, 0), 0.0f);
        if (T == object::BOULDER) {
            const int32_t kt = zv(hp + rr) + 1;
            if (kt < z0)
                return;
            for (int32_t k = z0; k <= kt; k++) {
                const int32_t zb = u * k + m.lowest;
                if (insideRock(X, Y, zb, px, py, trunc32(hp), trunc32(rr), trunc32(rr), trunc32(rh)))
                    store(i, j, k, rockColour(X, Y, zb, c.ci.warmth, c.ci.lavaRegion, c.ci.forestRegion));
            }
            return;
        }
        const float n = simplex((double)X * 0.01 + 4394, (double)Y * 0.01 + 8974);
        const int32_t kt = zv(n * 10.0f + hp + rh) + 1;
        const bool rocky = p.atColours.highlands > 0.5f || (T >= 11 && T <= 14);
        for (int32_t k = z0; k < kt; k++) {
            const int32_t zb = u * k + m.lowest;
            store(i, j, k, rocky ? rockColour(X, Y, zb, c.ci.warmth, c.ci.lavaRegion, c.ci.forestRegion)
                                 : groundColour(X, Y, zb, c.ci, sand));
        }
        if (snowAt(X, Y, c.ci.warmth, NeighbourHeights{}))
            store(i, j, kt, SNOW_BLOCK);
        else if (c.ci.vegetation < 0.5f)
            store(i, j, kt, surfaceColour(X, Y, ar.t, c.ci));
    }

    // --- §3.7 Haze ---

    void haze(int i, int j, int32_t from, float h0, const ColourInputs &ci) {
        const float w = hazeWeight;
        const int32_t h = trunc32(h0);
        float f = (float)(h - 0) / (float)(500 - 0);
        if (!(f >= 0.0f))
            f = 0.0f;
        if (!(f <= 1.0f))
            f = 1.0f;
        const float s = 1.0f - f;
        float a = (ci.warmth - 0.8f) * (1.0f - ci.dryness) * 8.0f;
        if (a >= 1.0f)
            a = 1.0f;
        if (a <= 0.0f)
            a = 0.0f;
        const float ia = 1.0f - a;
        const float kk = s * s * s * 0.8f;
        for (int32_t k = std::max(from, 0); k < m.sizeZ; k++) {
            uint8_t *v = &m.rgb[(((size_t)k * m.side + j) * m.side + i) * 3];
            if (!v[0] && !v[1] && !v[2])
                continue;
            const float r = v[0], g = v[1], b = v[2];
            const float lum = r * 0.0078125f * 0.299f + g * 0.0078125f * 0.587f + b * 0.0078125f * 0.114f;
            const float ob = ((a * 0.0f + ia * 140.0f) * lum - b) * kk + b;
            const float orr = ((a * 130.0f + ia * 0.0f) * lum - r) * kk + r;
            const float og = ((a * 60.0f + ia * 40.0f) * lum - g) * kk + g;
            v[0] = (uint8_t)trunc64(w * orr + r * (1.0f - w));
            v[1] = (uint8_t)trunc64(w * og + g * (1.0f - w));
            v[2] = (uint8_t)trunc64(w * ob + b * (1.0f - w));
        }
    }

    // --- §4 Pass 3: shade under overhangs ---

    void shade() {
        for (int i = 0; i < m.side; i++)
            for (int j = 0; j < m.side; j++) {
                bool seen = false, gap = false;
                for (int k = m.sizeZ - 1; k >= 0; k--) {
                    uint8_t *v = &m.rgb[(((size_t)k * m.side + j) * m.side + i) * 3];
                    if (v[0] || v[1] || v[2]) {
                        if (gap)
                            v[0] >>= 1, v[1] >>= 1, v[2] >>= 1;
                        seen = true;
                    } else if (seen) {
                        gap = true;
                    }
                }
            }
    }

    // --- §6 Markers ---

    bool inside(int32_t x, int32_t y) const {
        const int64_t x0 = (int64_t)B * m.rx, y0 = (int64_t)B * m.ry;
        return x >= x0 && x < x0 + B && y >= y0 && y < y0 + B;
    }
    static int32_t groundAt(int32_t x, int32_t y) { return trunc32(groundHeight(x, y, 0)); }
    // The place cell of a block position, evened when big (map-markers.md §0.5).
    static void placeCellOf(MapRegion::Marker &k) {
        int32_t px = floorDiv(k.x, PLACE_SHIFT), py = floorDiv(k.y, PLACE_SHIFT);
        k.big = bigCell(px, py);
        if (k.big)
            px = 2 * floorDiv(px, 1), py = 2 * floorDiv(py, 1);
        k.px = px, k.py = py;
    }
    static MapRegion::Marker place(int kind, int32_t x, int32_t y, int32_t z) {
        MapRegion::Marker k;
        k.list = 0, k.type = kind, k.x = x, k.y = y, k.z = z;
        placeCellOf(k);
        return k;
    }

    void markers() {
        std::vector<MapRegion::Marker> places, missions, lands, flights;
        const int32_t x0 = B * m.rx, y0 = B * m.ry, x1 = B * (m.rx + 1), y1 = B * (m.ry + 1);
        // 6.1 Missions.
        if (u <= 256) {
            std::set<std::pair<int32_t, int32_t>> seen;
            for (int32_t px = floorDivBy(x0, 1024); px <= floorDivBy(x1, 1024); px++)
                for (int32_t py = floorDivBy(y0, 1024); py <= floorDivBy(y1, 1024); py++) {
                    const bool big = bigCell(px, py);
                    const int32_t qx = big ? 2 * floorDiv(px, 1) : px, qy = big ? 2 * floorDiv(py, 1) : py;
                    const int sel = placeKind(qx, qy, big);
                    if (sel == 0)
                        continue;
                    const Int2 at = cellPoint(qx, qy, big);
                    if (!inside(at.x, at.y) || seen.count({qx, qy}))
                        continue;
                    const Int2 region = regionAt(at.x, at.y);
                    MapRegion::Marker k;
                    k.list = 1, k.type = sel, k.x = at.x, k.y = at.y, k.z = groundAt(at.x, at.y);
                    k.kind = plannerRole(region, qx, qy);
                    k.level = plannerLevel(region, qx, qy);
                    const std::shared_ptr<const RegionPlan> plan = regionPlan(region.x, region.y);
                    if (const auto it = plan->artifacts.find({qx, qy}); it != plan->artifacts.end())
                        k.item = it->second;
                    k.px = qx, k.py = qy, k.big = big;
                    if (k.kind == role::ARTIFACT || u <= 64) {
                        missions.push_back(k);
                        seen.insert({qx, qy});
                    }
                }
        }
        // 6.2 The land's name. (With CubeVulkan's world scale the regions are wider; at 1 this is
        // the game's.)
        if (u >= 128) {
            const double scale = worldScale();
            const int32_t lx = (int32_t)std::floor((double)x0 / (8192.0 * scale)), ly = (int32_t)std::floor((double)y0 / (8192.0 * scale));
            Int2 centre = regionCentre(lx, ly);
            if (scale != 1.0)
                centre = {(int32_t)std::floor(centre.x * scale), (int32_t)std::floor(centre.y * scale)};
            if (inside(centre.x, centre.y)) {
                MapRegion::Marker k;
                k.list = 2, k.type = regionBiome(lx, ly), k.x = centre.x, k.y = centre.y, k.z = groundAt(centre.x, centre.y);
                k.px = lx, k.py = ly;
                lands.push_back(k);
            }
        }
        // 6.3 The flight point of the 2048-block square.
        if (u <= 256) {
            const int32_t fx = floorDivBy(x0, 2048), fy = floorDivBy(y0, 2048);
            const uint32_t key = cellKey(fx, fy);
            const int32_t x = fx * 2048 + (int32_t)(random(key + 0x474d6) & 2047u);
            const int32_t y = fy * 2048 + (int32_t)(random(key + 0x181f0) & 2047u);
            if (inside(x, y)) {
                MapRegion::Marker k;
                k.list = 3, k.x = x, k.y = y, k.z = groundAt(x, y);
                placeCellOf(k);
                flights.push_back(k);
            }
        }
        // 6.4 Shrines.
        if (u <= 512)
            for (int32_t qx = floorDivBy(x0, 1024); qx <= floorDivBy(x1, 1024); qx++)
                for (int32_t qy = floorDivBy(y0, 1024); qy <= floorDivBy(y1, 1024); qy++) {
                    const Int2 s = areaLandmark(qx, qy);
                    const int32_t x = 16 * s.x + 8, y = 16 * s.y + 8;
                    if (inside(x, y) && cells.type(s.x, s.y) == object::SHRINE)
                        places.push_back(place(SHRINE_PLACE, x, y, rules.baseHeight(s.x, s.y, 0)));
                }
        // 6.5 Lore buildings and treasure cells.
        if (u <= 64)
            for (int32_t cx = floorDivBy(x0, 16); cx <= floorDivBy(x1, 16); cx++)
                for (int32_t cy = floorDivBy(y0, 16); cy <= floorDivBy(y1, 16); cy++) {
                    const int32_t x = 16 * cx + 8, y = 16 * cy + 8;
                    if (!inside(x, y))
                        continue;
                    const int t = cells.type(cx, cy);
                    if (loreCell(t))
                        places.push_back(place(LORE_PLACE, x, y, rules.baseHeight(cx, cy, 0)));
                    else if (crateCell(t))
                        places.push_back(place(TREASURE_PLACE, x, y, rules.baseHeight(cx, cy, 0)));
                }
        // 6.6 Castle cells and a town's buildings.
        if (u < 64) {
            const int32_t n = B / 16;
            for (int32_t di = 0; di < n; di++)
                for (int32_t dj = 0; dj < n; dj++) {
                    const int32_t cx = floorDivBy(x0, 16) + di, cy = floorDivBy(y0, 16) + dj;
                    const CastleCell w = rules.castleCell(cx, cy);
                    if (w.kind != 0)
                        m.castles.push_back({{w.kind, w.detail, w.style, w.steps}, 16 * cx, 16 * cy, rules.castleFloor(cx, cy)});
                    if (u > 16 || !oneOf(cells.type(cx, cy), {22, 23, 24, 25, 26, 91, 92}))
                        continue;
                    if (blockCorner(floorDivBy(cx, 3), floorDivBy(cy, 3)) != Int2{cx, cy})
                        continue;
                    const int32_t zx = floorDivBy(16 * cx, 1024), zy = floorDivBy(16 * cy, 1024);
                    const int kind = placeKind(zx, zy, bigCell(zx, zy));
                    const int plot = townBlock(world, cx, cy, kind);
                    if (plot > 1 && plot != FARM_BLOCK) {
                        const int32_t x = 16 * cx + 8, y = 16 * cy + 8;
                        places.push_back(place(plot, x, y, trunc32(groundHeight(x, y, 0) + 20.0f)));
                    }
                }
        }
        for (const auto *list : {&places, &missions, &lands, &flights})
            m.markers.insert(m.markers.end(), list->begin(), list->end());
    }
};

} // namespace

int mapUnit(int level) { return std::max(1, trunc32(std::pow(2.0f, (float)level) * 4.0f)); }

int mapReach(int unit) {
    if (unit >= 32)
        return 1;
    const int k = 32 / unit;
    return k > 1 ? 2 : k;
}

MapRegion mapRegion(int32_t rx, int32_t ry, int level) {
    MapRegion m;
    m.rx = rx, m.ry = ry, m.level = level;
    m.unit = mapUnit(level);
    m.reach = mapReach(m.unit);
    ModelBuilder(m).build();
    return m;
}

} // namespace worldgen
