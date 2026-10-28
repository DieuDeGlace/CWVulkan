// The light pass, written from the summary in docs/specs/worldgen-terrain-zone.md §6.
#include "worldgen/ZoneLight.h"

#include <algorithm>

namespace worldgen {
namespace {

constexpr int MARGIN = 8;                     // columns read into the neighbouring zones
constexpr int AREA = ZONE_SIZE + 2 * MARGIN;  // 80
constexpr int SPREAD_ROUNDS = MARGIN - 3;     // 5
constexpr uint8_t DARK = 0, SKY = 255;
constexpr int SPREAD_FLOOR = 25;              // a neighbour counts at least this bright
constexpr int SPREAD_KEEP = 85;               // percent passed on per block

struct LitColumn {
    const TerrainColumn *column = nullptr;
    std::vector<uint8_t> light; // one per stored block
};

class LightMap {
public:
    LightMap(int32_t zx, int32_t zy, const ZoneColumnsSource &zones) : x0(zx * ZONE_SIZE - MARGIN), y0(zy * ZONE_SIZE - MARGIN) {
        cells.resize(AREA * AREA);
        for (int i = 0; i < AREA; i++)
            for (int j = 0; j < AREA; j++) {
                const int32_t x = x0 + i, y = y0 + j;
                const int32_t czx = floorDiv(x, ZONE_SHIFT), czy = floorDiv(y, ZONE_SHIFT);
                const std::vector<TerrainColumn> &zone = zones(czx, czy);
                LitColumn &c = cells[i * AREA + j];
                c.column = &zone[ZoneTerrain::index(x - czx * ZONE_SIZE, y - czy * ZONE_SIZE)];
                skyLight(c);
            }
    }

    // 2. The light spreads from lit blocks into the dark, one block per round.
    void spread() {
        for (int round = 0; round < SPREAD_ROUNDS; round++) {
            std::vector<std::vector<uint8_t>> next(cells.size());
            for (int i = 1; i < AREA - 1; i++)
                for (int j = 1; j < AREA - 1; j++) {
                    const LitColumn &c = cells[i * AREA + j];
                    std::vector<uint8_t> &out = next[i * AREA + j];
                    out = c.light;
                    for (size_t k = 0; k < c.light.size(); k++) {
                        if (!isOpen(c.column->blocks[k]) || c.light[k] == SKY)
                            continue;
                        const int32_t z = c.column->base + (int32_t)k;
                        int brightest = 0;
                        for (int v : {at(i - 1, j, z), at(i + 1, j, z), at(i, j - 1, z), at(i, j + 1, z), at(i, j, z - 1),
                                      at(i, j, z + 1)})
                            brightest = std::max(brightest, std::max(v, SPREAD_FLOOR));
                        out[k] = (uint8_t)(brightest * SPREAD_KEEP / 100);
                    }
                }
            for (int i = 1; i < AREA - 1; i++)
                for (int j = 1; j < AREA - 1; j++)
                    cells[i * AREA + j].light = std::move(next[i * AREA + j]);
        }
    }

    // 3. The zone's own open blocks take the light as their red byte.
    std::vector<TerrainColumn> lit() const {
        std::vector<TerrainColumn> out(ZONE_COLUMNS);
        for (int lx = 0; lx < ZONE_SIZE; lx++)
            for (int ly = 0; ly < ZONE_SIZE; ly++) {
                const LitColumn &c = cells[(lx + MARGIN) * AREA + ly + MARGIN];
                TerrainColumn &t = out[ZoneTerrain::index(lx, ly)];
                t = *c.column;
                for (size_t k = 0; k < t.blocks.size(); k++)
                    if (isOpen(t.blocks[k]))
                        t.blocks[k].r = c.light[k];
            }
        return out;
    }

private:
    int32_t x0, y0;
    std::vector<LitColumn> cells;

    // 1. Sky light, from the top of each column down.
    static void skyLight(LitColumn &c) {
        const std::vector<Block> &blocks = c.column->blocks;
        c.light.assign(blocks.size(), DARK);
        int light = SKY;
        bool sky = true;
        for (size_t k = blocks.size(); k-- > 0;) {
            const Block &b = blocks[k];
            if (b.type == blocktype::LIGHT) {
                sky = true;
                light = b.glow;
                c.light[k] = (uint8_t)light;
            } else if (!isOpen(b)) {
                c.light[k] = (uint8_t)light;
                sky = false;
                light = std::min<int>(light, b.glow);
            } else if (sky) {
                c.light[k] = SKY;
            } else {
                // A glowing open block (the cliff pass's overhang light) lights the open blocks
                // below it too: the running light keeps its glow. (The spec's summary leaves
                // this open; the saved zones show it.)
                light = std::max<int>(light, b.glow);
                c.light[k] = (uint8_t)light;
            }
        }
    }

    // The light at a block of the area: dark below a column, open sky above it and outside.
    int at(int i, int j, int32_t z) const {
        if (i < 0 || j < 0 || i >= AREA || j >= AREA)
            return SKY;
        const LitColumn &c = cells[i * AREA + j];
        if (c.column->empty() || z >= c.column->top())
            return SKY;
        if (z < c.column->base)
            return DARK;
        return c.light[(size_t)(z - c.column->base)];
    }
};

} // namespace

std::vector<TerrainColumn> lightZone(int32_t zx, int32_t zy, const ZoneColumnsSource &zones) {
    LightMap map(zx, zy, zones);
    map.spread();
    return map.lit();
}

} // namespace worldgen
