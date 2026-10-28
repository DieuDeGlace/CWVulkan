#include "Terrain.h"
#include "AlphaLight.h"
#include "AlphaLook.h"
#include "Config.h"
#include "Palette.h"
#include "TerrainRules.h"
#include <algorithm>
#include <cmath>
#include <memory>

using terrain::hash;
using terrain::noise;
using terrain::smooth;

const char *biomeName(int biome) {
    static const char *names[] = {"Plains",    "Ocean",    "Hills",  "Woodlands", "Darkwoods",
                                  "Mountains", "Wetlands", "Desert", "Snowlands", "Deadlands"};
    return biome >= 0 && biome < BIOMES ? names[biome] : "?";
}

void TerrainSettings::read(const Config &c) {
    seed = (uint64_t)c.number("World", "Seed", (double)seed);
    std::string style = c.text("Terrain", "Style", alphaStyle ? "Alpha" : "Variance");
    alphaStyle = !(style == "Variance" || style == "variance" || style == "VARIANCE");
    // Same keys and limits as Mods/TerrainVariance.ini.
    amplitude = c.number("Terrain", "Amplitude", amplitude, 0, 96);
    scale = c.number("Terrain", "Scale", scale, 256, 2048);
    mountainAmplitude = c.number("Terrain", "MountainAmplitude", mountainAmplitude, amplitude, 96);
    alphaMountainHeight = c.number("Terrain", "AlphaMountainHeight", alphaMountainHeight, 0, 200);
    regionSize = c.number("Terrain", "RegionSize", regionSize, 512, 8192);
    rivers = c.flag("Terrain", "Rivers", rivers);
    riverScale = c.number("Terrain", "RiverScale", riverScale, 300, 8000);
    riverWidth = c.number("Terrain", "RiverWidth", riverWidth, 2, 24);
    sceneryDensity = c.number("Terrain", "SceneryPercent", sceneryDensity * 100, 0, 400) / 100;
    // Same keys as Mods/WaterLilies.ini.
    lilies = c.flag("WaterLilies", "Enabled", lilies);
    lilyPatchDensity = c.number("WaterLilies", "PatchDensity", lilyPatchDensity * 100, 0, 100) / 100;
    lilyFlowerPercent = c.number("WaterLilies", "FlowerPercent", lilyFlowerPercent, 0, 100);
    lilyMaxPadsPerZone = (int)c.number("WaterLilies", "MaxPadsPerChunk", lilyMaxPadsPerZone, 0, 256);
    lilyMaximumDepth = (int)c.number("WaterLilies", "MaximumDepth", lilyMaximumDepth, 1, 40);
}

namespace {
enum TreeKind : uint8_t { NO_TREES, BLOB, GIANT, FIR, CACTUS_TREE, DEAD };

// Shape and look of each region. Colours are as measured on sunlit top faces
// in the reference shots (0..255, after vanilla's grade); Palette.h turns
// them into block colours.
struct Style {
    float base, relief, mountain, trees, rivers;
    TreeKind tree;
    Block top;             // Surface material on gentle ground.
    float ground[3];       // Grass / snow / sand top.
    float under[3];        // Material just below snow (grass) or sand (sand).
    float stone[3], band[3]; // Cliff stone and its strata band.
    float accent[3];       // Mountain sand bands, desert sandstone...
    float crownLight[3], crownDark[3], trunk[3];
    float water[3];
    bool spires;           // Rock spires as in Vartar Hills.
};
constexpr Style STYLES[BIOMES] = {
    // Plains: bright lime grass, blob trees, white/lilac banded rock spires.
    {10, .70f, .05f, .22f, 1.f, BLOB, GRASS, {135, 228, 66}, {60, 160, 40},
     {200, 215, 245}, {120, 160, 205}, {253, 253, 182},
     {110, 210, 60}, {6, 90, 16}, {110, 70, 34}, {11, 109, 211}, true},
    // Ocean: open sea with occasional islands.
    {-30, .35f, 0.f, .20f, 0.f, BLOB, GRASS, {120, 220, 70}, {60, 160, 40},
     {200, 215, 245}, {120, 160, 205}, {253, 253, 182},
     {100, 205, 60}, {6, 90, 16}, {110, 70, 34}, {11, 109, 211}, false},
    // Hills: blue-green grass, rolling, spires.
    {14, 1.0f, .12f, .20f, 1.f, BLOB, GRASS, {28, 199, 87}, {20, 140, 50},
     {200, 215, 245}, {120, 160, 205}, {253, 253, 182},
     {70, 200, 70}, {4, 86, 24}, {110, 70, 34}, {11, 109, 211}, true},
    // Woodlands: darker olive grass, giant trees, brown cliffs, reeds.
    {12, .85f, .15f, 1.0f, 1.f, GIANT, GRASS, {72, 150, 22}, {40, 100, 14},
     {150, 74, 22}, {110, 50, 14}, {253, 253, 182},
     {60, 160, 34}, {3, 56, 6}, {96, 56, 26}, {11, 109, 211}, false},
    // Darkwoods: navy ground, purple/blue crowns, teal sky.
    {10, .80f, .10f, .80f, 1.f, BLOB, GRASS, {34, 52, 120}, {22, 32, 84},
     {110, 80, 165}, {80, 56, 130}, {150, 140, 120},
     {96, 72, 190}, {10, 10, 64}, {84, 48, 36}, {40, 120, 150}, false},
    // Mountains: green terraces with orange sand bands higher up.
    {26, 1.0f, 1.0f, .15f, .8f, BLOB, GRASS, {19, 196, 91}, {10, 130, 40},
     {200, 215, 245}, {120, 160, 205}, {214, 110, 24},
     {60, 190, 80}, {4, 80, 26}, {110, 70, 34}, {11, 109, 211}, true},
    // Wetlands: emerald grass, many pools, mushrooms.
    {2, .20f, 0.f, .30f, 1.3f, BLOB, GRASS, {0, 190, 117}, {0, 130, 80},
     {170, 190, 210}, {110, 140, 170}, {220, 220, 170},
     {40, 180, 100}, {4, 80, 44}, {100, 64, 34}, {20, 120, 170}, false},
    // Desert: vivid yellow-orange sand, sandstone mesas, pale shallow water.
    {9, .65f, .12f, .05f, .35f, CACTUS_TREE, SAND, {252, 194, 44}, {240, 170, 50},
     {238, 150, 60}, {205, 110, 40}, {230, 139, 64},
     {70, 150, 60}, {20, 90, 30}, {110, 70, 34}, {120, 220, 225}, false},
    // Snowlands: cyan-white snow over green, grey stone, blue crowns.
    {16, 1.0f, .35f, .35f, .8f, BLOB, SNOW, {190, 250, 255}, {30, 110, 30},
     {150, 152, 152}, {112, 114, 114}, {200, 240, 250},
     {40, 100, 230}, {2, 24, 150}, {100, 50, 18}, {30, 110, 200}, false},
    // Deadlands: violet ground, navy crowns, dark trunks, green-teal sky.
    {8, .60f, .10f, .30f, .8f, DEAD, GRASS, {95, 85, 156}, {70, 60, 120},
     {80, 70, 120}, {60, 50, 96}, {120, 100, 150},
     {48, 40, 130}, {8, 5, 52}, {60, 26, 58}, {30, 110, 90}, false},
};

// Relative frequency of each region type, guided by the scout's sample counts
// around the reference save (Hills most common, Deadlands rarest).
constexpr uint8_t REGION_TABLE[] = {
    PLAINS,    PLAINS,    PLAINS,    PLAINS,    OCEAN,     OCEAN,    OCEAN,    HILLS,
    HILLS,     HILLS,     HILLS,     HILLS,     HILLS,     WOODLANDS, WOODLANDS, WOODLANDS,
    WOODLANDS, DARKWOODS, DARKWOODS, DARKWOODS, MOUNTAINS, MOUNTAINS, MOUNTAINS, WETLANDS,
    WETLANDS,  DESERT,    DESERT,    DESERT,    SNOWLANDS, SNOWLANDS, DEADLANDS};

glm::vec3 albedoOf(const float c[3]) { return palette::albedo({c[0], c[1], c[2]}); }

// Block colours per region, converted once.
struct Colors {
    glm::vec3 ground[BIOMES], under[BIOMES], stone[BIOMES], band[BIOMES], accent[BIOMES],
        crownLight[BIOMES], crownDark[BIOMES], trunk[BIOMES], water[BIOMES], dirt, cactus;
    Colors() {
        for (int b = 0; b < BIOMES; b++) {
            const Style &s = STYLES[b];
            ground[b] = albedoOf(s.ground);
            under[b] = albedoOf(s.under);
            stone[b] = albedoOf(s.stone);
            band[b] = albedoOf(s.band);
            accent[b] = albedoOf(s.accent);
            crownLight[b] = albedoOf(s.crownLight);
            crownDark[b] = albedoOf(s.crownDark);
            trunk[b] = albedoOf(s.trunk);
            water[b] = palette::albedo({s.water[0], s.water[1], s.water[2]}, false);
        }
        const float d[3] = {120, 76, 40}, c[3] = {60, 160, 50};
        dirt = albedoOf(d);
        cactus = albedoOf(c);
    }
};
const Colors &colors() {
    static const Colors c;
    return c;
}

uint8_t regionType(int64_t rx, int64_t ry, uint64_t seed) {
    // The spawn region is always Plains.
    if (rx == 0 && ry == 0)
        return PLAINS;
    return REGION_TABLE[hash(rx, ry, seed ^ 0xb10e) % sizeof(REGION_TABLE)];
}
double unit(int64_t x, int64_t y, uint64_t seed) {
    return (hash(x, y, seed) >> 11) * (1.0 / 9007199254740992.0);
}
double fbm(double x, double y, uint64_t seed, int octaves) {
    double sum = 0, amp = 1, norm = 0;
    for (int i = 0; i < octaves; i++) {
        sum += noise(x, y, seed + i * 0x9e37) * amp;
        norm += amp;
        amp *= .5;
        x *= 2.03;
        y *= 2.03;
    }
    return sum / norm;
}
template <class F> float mixed(const ColumnInfo &c, F field) {
    float v = 0;
    for (int b = 0; b < BIOMES; b++)
        v += c.weights[b] * field(STYLES[b]);
    return v;
}
} // namespace

const RegionSky &regionSky(int biome) {
    // Measured skies: blue by default, teal in Darkwoods, green-teal in Deadlands.
    static const auto table = [] {
        struct Observed {
            float sky[3], fog[3], daylight;
        };
        const Observed observed[BIOMES] = {
            {{0, 128, 206}, {150, 210, 240}, 1},   {{0, 120, 210}, {150, 205, 240}, 1},
            {{0, 128, 206}, {150, 210, 240}, 1},   {{0, 120, 200}, {140, 200, 230}, .95f},
            {{79, 170, 164}, {79, 170, 164}, .72f}, {{0, 128, 206}, {160, 215, 245}, 1},
            {{20, 140, 200}, {130, 200, 220}, .95f}, {{40, 150, 220}, {200, 220, 230}, 1},
            {{60, 150, 230}, {200, 230, 250}, 1},  {{20, 129, 87}, {40, 140, 100}, .7f}};
        std::array<RegionSky, BIOMES> out{};
        for (int b = 0; b < BIOMES; b++) {
            auto s = palette::albedo({observed[b].sky[0], observed[b].sky[1], observed[b].sky[2]}, false);
            auto f = palette::albedo({observed[b].fog[0], observed[b].fog[1], observed[b].fog[2]}, false);
            out[b] = {{s.x / 255, s.y / 255, s.z / 255}, {f.x / 255, f.y / 255, f.z / 255}, observed[b].daylight};
        }
        return out;
    }();
    return table[std::clamp(biome, 0, BIOMES - 1)];
}

void Terrain::regionWeights(double x, double y, float weights[BIOMES], uint8_t &nearest) const {
    const double R = settings.regionSize;
    const uint64_t seed = settings.seed;
    double wx = x + fbm(x / (R * .45), y / (R * .45), seed ^ 0x77, 3) * R * .22;
    double wy = y + fbm(x / (R * .45), y / (R * .45), seed ^ 0x78, 3) * R * .22;
    int64_t rx = (int64_t)std::floor(wx / R), ry = (int64_t)std::floor(wy / R);
    double dist[9];
    uint8_t type[9];
    double best = 1e300;
    for (int i = 0; i < 9; i++) {
        int64_t cx = rx + i % 3 - 1, cy = ry + i / 3 - 1;
        double px = (cx + .2 + .6 * unit(cx, cy, seed ^ 0xc1)) * R;
        double py = (cy + .2 + .6 * unit(cx, cy, seed ^ 0xc2)) * R;
        dist[i] = std::hypot(wx - px, wy - py);
        type[i] = regionType(cx, cy, seed);
        if (dist[i] < best) {
            best = dist[i];
            nearest = type[i];
        }
    }
    std::fill(weights, weights + BIOMES, 0.f);
    // Regions blend over 15% of their size, so no cliffs at borders.
    double blend = R * .15, total = 0;
    for (int i = 0; i < 9; i++) {
        double w = smooth(1 - (dist[i] - best) / blend);
        weights[type[i]] += (float)w;
        total += w;
    }
    for (int b = 0; b < BIOMES; b++)
        weights[b] = (float)(weights[b] / total);
}

ColumnInfo Terrain::column(double x, double y) const {
    const auto &s = settings;
    const uint64_t seed = s.seed;
    ColumnInfo c{};
    regionWeights(x, y, c.weights, c.biome);
    // Steam-like gentle ground for each region; oceans are their own regions.
    double native = mixed(c, [](const Style &t) { return t.base; }) + 5 * noise(x / 96, y / 96, seed ^ 1) +
                    2.5 * noise(x / 28, y / 28, seed ^ 2);
    // Islands in the ocean.
    if (c.weights[OCEAN] > 0) {
        double island = std::max(0., fbm(x / 260, y / 260, seed ^ 0x15, 3) - .25) * 110;
        native += island * c.weights[OCEAN];
    }
    // TerrainVariance: keep coasts at their native height, full relief inland.
    double shore = smooth(native / 24);
    double relief = mixed(c, [](const Style &t) { return t.relief; });
    double mountainShare = mixed(c, [](const Style &t) { return t.mountain; });
    double h;
    if (!s.alphaStyle) {
        double limit = s.amplitude + (s.mountainAmplitude - s.amplitude) * c.weights[MOUNTAINS];
        h = native + terrain::variation((int64_t)std::floor(x), (int64_t)std::floor(y), seed,
                                        limit * relief, s.scale) *
                         shore;
    } else {
        double broad = terrain::variation((int64_t)std::floor(x), (int64_t)std::floor(y), seed,
                                          std::min(96., s.amplitude * 1.15) * relief, s.scale);
        double r1 = 1 - std::abs(fbm(x / 520, y / 520, seed ^ 0x51, 3));
        double r2 = 1 - std::abs(noise(x / 150, y / 150, seed ^ 0x52));
        double ridged = r1 * r1 * (.78 + .22 * r2 * r2);
        double mountain = smooth(mountainShare) * s.alphaMountainHeight;
        double detail = 4 * fbm(x / 22, y / 22, seed ^ 0x53, 2) * smooth(mountainShare * 2);
        h = native + (broad + ridged * ridged * mountain + detail) * shore;
        // Desert mesas: steep terraces 9 blocks apart.
        float desert = c.weights[DESERT];
        if (desert > 0 && h > 4) {
            double step = 9, t = h / step, f = t - std::floor(t);
            double terraced = (std::floor(t) + smooth(smooth((f - .55) / .45))) * step;
            h += (terraced - h) * desert * .85;
        }
    }
    // Rock spires (Plains, Hills, Mountains): flat-topped banded pillars.
    float spireShare = 0;
    for (int b = 0; b < BIOMES; b++)
        spireShare += STYLES[b].spires ? c.weights[b] : 0;
    if (spireShare > .5f && h > 4) {
        constexpr double CELL = 150;
        int64_t cx = (int64_t)std::floor(x / CELL), cy = (int64_t)std::floor(y / CELL);
        for (int64_t j = cy - 1; j <= cy + 1; j++)
            for (int64_t i = cx - 1; i <= cx + 1; i++) {
                uint64_t r = hash(i, j, seed ^ 0x5b1e);
                if ((r & 255) > 70)
                    continue;
                double px = (i + .2 + .6 * ((r >> 8) & 255) / 255.) * CELL;
                double py = (j + .2 + .6 * ((r >> 16) & 255) / 255.) * CELL;
                double radius = 4.0 + (double)((r >> 24) & 7), height = 16.0 + (double)((r >> 32) % 30);
                // Slightly lumpy outline.
                double d = std::hypot(x - px, y - py) * (1 + .18 * noise(x / 5, y / 5, seed ^ (r & 0xfff)));
                if (d < radius) {
                    double profile = 1 - std::pow(d / radius, 6);
                    h += height * profile * smooth((spireShare - .5f) * 4);
                    c.spire = std::max(c.spire, (float)profile);
                }
            }
    }
    // Wetlands: low ground just around sea level, full of ponds.
    if (c.weights[WETLANDS] > 0) {
        double wet = .9 + 2.6 * noise(x / 30, y / 30, seed ^ 0x5a) + 1.2 * noise(x / 9, y / 9, seed ^ 0x5c);
        h += (wet - h) * c.weights[WETLANDS] * .9;
    }
    // Rivers.
    if (s.rivers) {
        double wx = x + 60 * noise(x / 240, y / 240, seed ^ 0x3a);
        double wy = y + 60 * noise(x / 240, y / 240, seed ^ 0x3b);
        double n = .82 * noise(wx / s.riverScale, wy / s.riverScale, seed ^ 0x31) +
                   .18 * noise(wx / (s.riverScale * .3), wy / (s.riverScale * .3), seed ^ 0x32);
        double distance = std::abs(n) * s.riverScale * .55;
        double width = s.riverWidth * (.7 + .6 * (.5 + .5 * noise(x / 300, y / 300, seed ^ 0x33))) *
                       mixed(c, [](const Style &t) { return t.rivers; });
        double valley = width + 6 + std::max(0., h) * 1.1;
        double channel = width > .5 ? smooth(1 - distance / width) : 0;
        double inValley = smooth(1 - distance / valley) * (width > .5 ? 1 : 0);
        double floor = 1.5 + (-4 - 2 * channel - 1.5) * channel;
        bool ocean = c.weights[OCEAN] > .5f;
        if (h > floor && !ocean)
            h += (floor - h) * inValley;
        c.river = ocean ? 0.f : (float)channel;
    }
    h += s.seaLevel;
    constexpr double knee = WORLD_HEIGHT - 70.0;
    if (h > knee)
        h = knee + 50 * std::tanh((h - knee) / 50);
    h = std::clamp(h, 3.0, WORLD_HEIGHT - 16.0);
    c.ground = (int)std::floor(h);
    c.height = (float)c.ground;
    c.water = c.ground < s.seaLevel ? s.seaLevel : 0;
    c.freshwater = c.water && c.weights[OCEAN] < .5f && (c.river > .05f || native > 0);
    return c;
}

VoxelColor Terrain::blockColor(const ZoneData &zone, int x, int y, int z) const {
    const Colors &k = colors();
    size_t ci = (size_t)(y + 1) * ZoneData::P + (x + 1);
    int b = zone.biome[ci];
    // Very gentle per-column variation; vanilla surfaces are nearly flat colours.
    float v = .97f + .06f * (zone.shade[ci] / 255.f);
    auto out = [&](glm::vec3 c, uint8_t flags = 0) {
        c = glm::clamp(c * v, 0.f, 255.f);
        return VoxelColor{(uint8_t)c.r, (uint8_t)c.g, (uint8_t)c.b, flags};
    };
    switch (zone.at(x, y, z)) {
    case GRASS:
    case SNOW:
    case SAND: {
        auto &g = zone.ground[ci];
        uint8_t block = zone.at(x, y, z);
        // Sand on beaches and river beds keeps its own pale colour.
        if (block == SAND && STYLES[b].top != SAND)
            return out(k.accent[OCEAN]);
        // Snow is only the top layers; grass shows below it.
        if (block == GRASS && STYLES[b].top == SNOW)
            return out(k.under[b]);
        return out({g[0], g[1], g[2]});
    }
    case DIRT: return out(k.dirt);
    case STONE: {
        // Strata: alternating bands a few blocks thick, as on vanilla cliffs.
        bool band = ((z + (zone.shade[ci] & 1)) / 3) % 2 == 0;
        return out(band ? k.band[b] : k.stone[b]);
    }
    case ACCENT: return out(k.accent[b]);
    case WOOD: return out(k.trunk[b]);
    case CACTUS: return out(k.cactus);
    case LEAVES1:
    case LEAVES2:
    case LEAVES3:
    case LEAVES4: {
        float t = (zone.at(x, y, z) - LEAVES1) / 3.f;
        return out(glm::mix(k.crownDark[b], k.crownLight[b], t), SWAY);
    }
    default: return out({255, 0, 255});
    }
}

ZoneData Terrain::generate(int zx, int zy) const {
    constexpr int P = ZoneData::P;
    // Columns beyond the zone for trees that reach into it: the widest crown
    // (giant woodland trees, 9 * 1.8 blocks) plus the trunk's own cell.
    constexpr int pad = 24;
    constexpr int C = ZONE + 2 * pad;
    ZoneData zone;
    zone.zx = zx;
    zone.zy = zy;
    zone.blocks.assign((size_t)P * P * WORLD_HEIGHT, AIR);
    const double ox = (double)zx * ZONE, oy = (double)zy * ZONE;
    std::vector<ColumnInfo> columns((size_t)C * C);
    for (int y = 0; y < C; y++)
        for (int x = 0; x < C; x++)
            columns[(size_t)y * C + x] = column(ox + x - pad, oy + y - pad);
    auto col = [&](int lx, int ly) -> const ColumnInfo & {
        return columns[(size_t)(ly + pad) * C + (lx + pad)];
    };
    const Colors &k = colors();
    const int sea = settings.seaLevel;
    for (int ly = -1; ly <= ZONE; ly++)
        for (int lx = -1; lx <= ZONE; lx++) {
            const auto &c = col(lx, ly);
            size_t ci = (size_t)(ly + 1) * P + (lx + 1);
            int64_t gx = (int64_t)ox + lx, gy = (int64_t)oy + ly;
            zone.biome[ci] = c.biome;
            zone.shade[ci] = (uint8_t)(hash(gx, gy, settings.seed ^ 0x5ade) & 255);
            // Top colour blended across region borders, with broad soft patches
            // (vanilla grass shifts gently over large areas).
            glm::vec3 g(0);
            for (int b = 0; b < BIOMES; b++)
                g += c.weights[b] * k.ground[b];
            float patch = (float)noise(gx / 70., gy / 70., settings.seed ^ 0x9a7c);
            g *= 1 + .07f * patch;
            for (int i = 0; i < 3; i++)
                zone.ground[ci][i] = (uint8_t)std::clamp(g[i], 0.f, 255.f);

            int slope = 0;
            for (auto [dx, dy] : {std::pair{1, 0}, {-1, 0}, {0, 1}, {0, -1}})
                slope = std::max(slope, std::abs(col(lx + dx, ly + dy).ground - c.ground));
            const Style &st = STYLES[c.biome];
            bool cliff = slope >= 3 || c.spire > .2f;
            bool beach = c.ground <= sea + 1 && st.top != SNOW;
            // Mountains: orange sand bands on high terraces.
            bool accent = c.biome == MOUNTAINS && c.ground > sea + 30 &&
                          ((c.ground + (int)(4 * noise(gx / 60., gy / 60., settings.seed ^ 0xac))) / 6) % 3 == 0;
            size_t base = ci * WORLD_HEIGHT;
            for (int z = 0; z < c.ground && z < WORLD_HEIGHT; z++) {
                int depth = c.ground - 1 - z;
                uint8_t b;
                if (cliff)
                    b = depth < 2 && !c.spire && slope < 5 ? st.top == SAND ? SAND : GRASS : STONE;
                else if (beach)
                    b = depth < 4 ? SAND : STONE;
                else if (accent)
                    b = depth < 3 ? ACCENT : STONE;
                else if (st.top == SNOW)
                    b = depth < 2 ? SNOW : depth < 4 ? GRASS : STONE;
                else if (st.top == SAND)
                    b = depth < 8 ? SAND : STONE;
                else
                    // Vanilla grass columns are green several blocks down.
                    b = depth < 4 ? GRASS : depth < 7 ? DIRT : STONE;
                zone.blocks[base + z] = b;
            }
            for (int z = c.ground; z < sea; z++)
                zone.blocks[base + z] = WATER;
            zone.maxZ = std::max(zone.maxZ, std::max(c.ground, sea));
        }
    plantTrees(zone, columns, pad);
    placeScenery(zone, columns, pad);
    return zone;
}

void Terrain::plantTrees(ZoneData &zone, const std::vector<ColumnInfo> &columns, int pad) const {
    constexpr int P = ZoneData::P;
    const int C = ZONE + 2 * pad;
    const int64_t ox = (int64_t)zone.zx * ZONE, oy = (int64_t)zone.zy * ZONE;
    const uint64_t seed = settings.seed ^ 0x7ae3;
    auto set = [&](int lx, int ly, int z, uint8_t b, bool overwrite = false) {
        if (lx < -1 || ly < -1 || lx > ZONE || ly > ZONE || z < 0 || z >= WORLD_HEIGHT)
            return;
        auto &cell = zone.blocks[((size_t)(ly + 1) * P + (lx + 1)) * WORLD_HEIGHT + z];
        if (cell == AIR || overwrite)
            cell = b;
        zone.maxZ = std::max(zone.maxZ, z + 1);
    };
    // Crown of overlapping ellipsoids, shaded by height (LEAVES1 dark underside
    // to LEAVES4 light top), with a slightly ragged outline.
    auto crown = [&](int lx, int ly, float cz, float rx, float rz, uint64_t r, int blobs) {
        struct Blob {
            float x, y, z, rx, rz;
        };
        std::vector<Blob> list{{0, 0, 0, rx, rz}};
        for (int i = 0; i < blobs; i++) {
            float a = ((r >> (i * 7)) & 127) / 127.f * 6.2831853f;
            float d = rx * (.45f + .25f * (((r >> (i * 5 + 3)) & 15) / 15.f));
            list.push_back({std::cos(a) * d, std::sin(a) * d, -rz * .25f + rz * .5f * ((r >> (i * 3)) & 1),
                            rx * .62f, rz * .7f});
        }
        float top = cz + rz, bottom = cz - rz * 1.1f;
        int reach = (int)std::ceil(rx * 1.8f), up = (int)std::ceil(rz * 1.4f);
        for (int dz = -up; dz <= up; dz++)
            for (int dy = -reach; dy <= reach; dy++)
                for (int dx = -reach; dx <= reach; dx++) {
                    bool inside = false;
                    for (auto &b : list) {
                        float ex = (dx - b.x) / b.rx, ey = (dy - b.y) / b.rx, ez = (dz - b.z) / b.rz;
                        float d = ex * ex + ey * ey + ez * ez;
                        // World coordinates, so neighbouring zones agree.
                        if (d <= 1 && !(d > .8f && (hash(dx * 31 + ox + lx, dy * 17 + oy + ly, r ^ (dz * 13)) & 7) == 0)) {
                            inside = true;
                            break;
                        }
                    }
                    if (!inside)
                        continue;
                    float z = cz + dz;
                    float t = std::clamp((z - bottom) / std::max(1.f, top - bottom), 0.f, 1.f);
                    uint8_t level = (uint8_t)(LEAVES1 + std::min(3, (int)(t * 4)));
                    set(lx + dx, ly + dy, (int)std::round(z), level);
                }
    };
    auto trunk = [&](int lx, int ly, int z0, int width, int height) {
        int lo = -(width - 1) / 2;
        for (int z = 0; z < height; z++)
            for (int dy = lo; dy < lo + width; dy++)
                for (int dx = lo; dx < lo + width; dx++)
                    set(lx + dx, ly + dy, z0 + z, WOOD, true);
        // Root flare on wide trunks.
        if (width >= 3)
            for (auto [dx, dy] : {std::pair{-2, 0}, {2, 0}, {0, -2}, {0, 2}})
                set(lx + dx, ly + dy, z0, WOOD, true);
    };
    constexpr int CELL = 7;
    int64_t c0x = (int64_t)std::floor(double(ox - pad) / CELL), c1x = (ox + ZONE + pad) / CELL;
    int64_t c0y = (int64_t)std::floor(double(oy - pad) / CELL), c1y = (oy + ZONE + pad) / CELL;
    for (int64_t cy = c0y; cy <= c1y; cy++)
        for (int64_t cx = c0x; cx <= c1x; cx++) {
            uint64_t h = hash(cx, cy, seed);
            int64_t gx = cx * CELL + (int64_t)(h % CELL), gy = cy * CELL + (int64_t)((h >> 8) % CELL);
            int lx = (int)(gx - ox), ly = (int)(gy - oy);
            if (lx < -pad + 1 || ly < -pad + 1 || lx >= ZONE + pad - 1 || ly >= ZONE + pad - 1)
                continue;
            const auto &c = columns[(size_t)(ly + pad) * C + (lx + pad)];
            const Style &st = STYLES[c.biome];
            float density = 0;
            for (int b = 0; b < BIOMES; b++)
                density += c.weights[b] * STYLES[b].trees;
            if (((h >> 16) & 0xffff) / 65536.f >= density * .5f)
                continue;
            if (c.water || c.ground <= settings.seaLevel + 1 || c.river > .02f || c.spire > 0)
                continue;
            int lo = c.ground, hi = c.ground;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    int g = columns[(size_t)(ly + pad + dy) * C + (lx + pad + dx)].ground;
                    lo = std::min(lo, g);
                    hi = std::max(hi, g);
                }
            if (hi - lo > 2)
                continue;
            int z0 = c.ground;
            uint64_t r = hash(gx, gy, seed ^ 0x51ce);
            TreeKind kind = st.tree;
            // Firs high in the mountains.
            if (c.biome == MOUNTAINS && c.ground > settings.seaLevel + 70)
                kind = FIR;
            switch (kind) {
            case CACTUS_TREE: {
                int height = 3 + (int)(r % 4);
                for (int z = 0; z < height; z++)
                    set(lx, ly, z0 + z, CACTUS);
                if (r & 0x100) {
                    set(lx + 1, ly, z0 + 1, CACTUS);
                    set(lx + 2, ly, z0 + 1, CACTUS);
                    set(lx + 2, ly, z0 + 2, CACTUS);
                }
                if (r & 0x200) {
                    set(lx - 1, ly, z0 + 2, CACTUS);
                    set(lx - 2, ly, z0 + 2, CACTUS);
                    set(lx - 2, ly, z0 + 3, CACTUS);
                }
                break;
            }
            case FIR: {
                int height = 9 + (int)(r % 6);
                trunk(lx, ly, z0, 1, height);
                for (int z = 2; z <= height + 1; z++) {
                    float radius = 4.2f * (1 - float(z - 2) / height) + ((z & 1) ? 0 : .7f);
                    int ri = (int)std::ceil(radius);
                    for (int dy = -ri; dy <= ri; dy++)
                        for (int dx = -ri; dx <= ri; dx++)
                            if (dx * dx + dy * dy <= radius * radius)
                                set(lx + dx, ly + dy, z0 + z, (uint8_t)(LEAVES1 + std::min(3, (z * 4) / (height + 2))));
                }
                break;
            }
            case GIANT: {
                // Woodlands: huge trees with thick trunks and wide crowns.
                int height = 10 + (int)(r % 7);
                trunk(lx, ly, z0, 3, height + 3);
                crown(lx, ly, z0 + height + 3.5f, 6.f + (r >> 20) % 4, 3.8f, r, 4);
                break;
            }
            case DEAD: {
                // Deadlands: sparse dark crowns on thin trunks.
                int height = 5 + (int)(r % 4);
                trunk(lx, ly, z0, (r & 0x40) ? 2 : 1, height + 1);
                crown(lx, ly, (float)(z0 + height + 2), 3.f + (float)((r >> 20) % 2), 2.2f, r, 2);
                break;
            }
            default: {
                // Rounded blob tree, as in Plains, Hills and Snowlands.
                bool large = (r >> 12) % 3 == 0;
                int height = large ? 5 + (int)(r % 3) : 3 + (int)(r % 3);
                float rx = large ? 4.2f + ((r >> 20) % 3) * .6f : 2.6f + ((r >> 20) % 3) * .4f;
                trunk(lx, ly, z0, large ? 2 : 1, height + 1);
                crown(lx, ly, z0 + height + rx * .55f, rx, rx * .78f, r, large ? 3 : 1);
                break;
            }
            }
        }
}

void Terrain::placeScenery(ZoneData &zone, const std::vector<ColumnInfo> &columns, int pad) const {
    const int C = ZONE + 2 * pad;
    const int64_t ox = (int64_t)zone.zx * ZONE, oy = (int64_t)zone.zy * ZONE;
    const uint64_t seed = settings.seed ^ 0x5ce;
    const float voxel = 1.f / 16; // Vegetation sprites are 16 voxels per block.
    const int sea = settings.seaLevel;
    auto col = [&](int lx, int ly) -> const ColumnInfo & {
        return columns[(size_t)(ly + pad) * C + (lx + pad)];
    };
    auto topBlock = [&](int lx, int ly) {
        int g = col(lx, ly).ground;
        return zone.at(lx, ly, g - 1) != AIR && zone.at(lx, ly, g) == AIR ? zone.at(lx, ly, g - 1)
                                                                          : (uint8_t)AIR;
    };
    auto pick = [](uint64_t h, std::initializer_list<const char *> names) {
        return std::string(names.begin()[h % names.size()]);
    };
    const double density = settings.sceneryDensity;
    for (int ly = 0; ly < ZONE; ly++)
        for (int lx = 0; lx < ZONE; lx++) {
            const auto &c = col(lx, ly);
            int64_t gx = ox + lx, gy = oy + ly;
            uint64_t h = hash(gx, gy, seed);
            double roll = (h >> 40) / double(1 << 24);
            float x = gx + .2f + .6f * ((h >> 8) & 255) / 255.f;
            float y = gy + .2f + .6f * ((h >> 16) & 255) / 255.f;
            float yaw = ((h >> 24) & 3) * 1.5707963f;
            float z = (float)c.ground;
            uint8_t top = topBlock(lx, ly);
            auto add = [&](const std::string &m, uint8_t flags = SWAY, float scale = 1.f / 16) {
                zone.scenery.push_back({m, x, y, z, yaw, scale, flags});
            };
            if (top == GRASS) {
                switch (c.biome) {
                case PLAINS:
                case HILLS:
                case OCEAN:
                    // Tall grass and the blue flowers seen across vanilla plains.
                    if (roll < .07 * density)
                        add(pick(h, {"grass", "grass2", "grass3"}));
                    else if (roll < .10 * density)
                        add(pick(h >> 3, {"flower-blue", "flower-blue", "flower1", "flower2", "flowers"}));
                    else if (roll < .105 * density)
                        add("bush");
                    break;
                case WOODLANDS:
                    // Dense cattail reeds and dark grass under giant trees.
                    if (roll < .10 * density)
                        add("reed");
                    else if (roll < .17 * density)
                        add(pick(h, {"grass-dark1", "grass-dark2"}));
                    else if (roll < .175 * density)
                        add("mushroom", 0);
                    break;
                case DARKWOODS:
                    if (roll < .06 * density)
                        add(pick(h, {"grass-dark1", "grass-dark2", "scrub-dark"}));
                    else if (roll < .07 * density)
                        add(pick(h >> 2, {"shimmer-mushroom", "mushroom"}), 0);
                    break;
                case MOUNTAINS:
                    if (roll < .05 * density)
                        add(pick(h, {"grass", "grass2"}));
                    else if (roll < .055 * density)
                        add("flower-blue");
                    break;
                case WETLANDS:
                    // Purple mushrooms and red flowers as in Anlur Wetlands.
                    if (roll < .04 * density)
                        add(pick(h, {"grass", "grass2"}));
                    else if (roll < .05 * density)
                        add("mushroom", 0);
                    else if (roll < .056 * density)
                        add(pick(h >> 4, {"flower1", "flower3"}));
                    break;
                case DEADLANDS:
                    if (roll < .03 * density)
                        add(pick(h, {"grass-dry1", "grass-dry2", "cobwebscrub"}));
                    else if (roll < .033 * density)
                        add("thorn-tree", 0, 1.f / 5);
                    break;
                default: break;
                }
            } else if (top == SNOW) {
                if (roll < .012 * density)
                    add("snow-bush");
            } else if (top == SAND && c.biome == DESERT) {
                if (roll < .004 * density)
                    add("cactus1", 0);
                else if (roll < .006 * density)
                    add(pick(h, {"grass-dry1", "grass-dry2"}));
                else if (roll < .0065 * density)
                    add("desert-trunk", 0, 1.f / 5);
            }
            if (top == STONE && roll > .998)
                add(pick(h >> 5, {"stone", "stone2"}), 0);
            // Reeds on banks next to fresh water.
            if (!c.water && c.ground <= sea + 2 && roll > .55 && roll < .55 + .25 * density &&
                (top == GRASS || top == SAND)) {
                bool bank = false;
                for (int dy = -2; dy <= 2 && !bank; dy++)
                    for (int dx = -2; dx <= 2 && !bank; dx++) {
                        int nx = lx + dx, ny = ly + dy;
                        if (nx < -pad || ny < -pad || nx >= ZONE + pad || ny >= ZONE + pad)
                            continue;
                        bank = col(nx, ny).freshwater;
                    }
                if (bank)
                    add("reed");
            }
        }
    // WaterLilies: patches on shallow fresh water.
    if (!settings.lilies)
        return;
    int pads = 0;
    constexpr int PATCH = 16;
    for (int py = 0; py < ZONE / PATCH; py++)
        for (int px = 0; px < ZONE / PATCH; px++) {
            int64_t cx = ox / PATCH + px, cy = oy / PATCH + py;
            uint64_t h = hash(cx, cy, seed ^ 0x111e);
            if ((h & 0xffff) / 65536.0 >= settings.lilyPatchDensity * .6)
                continue;
            int count = 3 + (int)((h >> 16) % 8);
            float centerX = px * PATCH + 2 + (float)((h >> 24) % (PATCH - 4));
            float centerY = py * PATCH + 2 + (float)((h >> 32) % (PATCH - 4));
            for (int i = 0; i < count && pads < settings.lilyMaxPadsPerZone; i++) {
                uint64_t r = hash(cx * 16 + i, cy * 16 - i, seed ^ 0x9ad);
                float angle = (r & 0xffff) / 65536.f * 6.2831853f;
                float dist = ((r >> 16) & 0xffff) / 65536.f * 4.5f;
                float lx = centerX + std::cos(angle) * dist, ly = centerY + std::sin(angle) * dist;
                int ix = (int)std::floor(lx), iy = (int)std::floor(ly);
                if (ix < 0 || iy < 0 || ix >= ZONE || iy >= ZONE)
                    continue;
                const auto &c = col(ix, iy);
                int depth = c.water - c.ground;
                if (!c.water || !c.freshwater || depth < 1 || depth > settings.lilyMaximumDepth ||
                    zone.at(ix, iy, sea - 1) != WATER || zone.at(ix, iy, sea) != AIR)
                    continue;
                float x = float(ox + lx), y = float(oy + ly), yaw = ((r >> 32) & 0xffff) / 65536.f * 6.2831853f;
                float surface = sea - .12f;
                zone.scenery.push_back({(r >> 48) & 1 ? "water-lily01" : "water-lily02", x, y, surface, yaw, voxel, 0});
                pads++;
                if (((r >> 50) % 100) < settings.lilyFlowerPercent)
                    zone.scenery.push_back({"lily", x, y, surface + 2 * voxel, yaw, voxel, SWAY});
            }
        }
}

void Terrain::mesh(const ZoneData &zone, Mesh &opaque, Mesh &water) const {
    auto opaqueAt = [&](int x, int y, int z) {
        uint8_t b = zone.at(x, y, z);
        return b != AIR && b != WATER;
    };
    auto colour = [&](int x, int y, int z) { return blockColor(zone, x, y, z); };
    const glm::ivec3 hi(ZONE, ZONE, std::min(zone.maxZ + 1, WORLD_HEIGHT));
    // The Alpha look bakes the block light (docs/specs/alpha-rendering.md 9.2-9.4), here over
    // the zone and its one-block border.
    std::unique_ptr<alphalight::LightVolume> light;
    if (renderStyle() == RenderStyle::Alpha) {
        const int top = std::min(zone.maxZ + 2, WORLD_HEIGHT);
        light = std::make_unique<alphalight::LightVolume>(glm::ivec3(-1, -1, 0), glm::ivec3(ZONE + 2, ZONE + 2, top));
        for (int z = 0; z < top; z++)
            for (int y = -1; y <= ZONE; y++)
                for (int x = -1; x <= ZONE; x++)
                    light->setKind({x, y, z}, opaqueAt(x, y, z) ? alphalight::BlockKind::Solid : alphalight::BlockKind::Open);
        light->flood();
        auto shade = [&](int x, int y, int z, int face, int k, VoxelColor c) {
            const int *corner = FACE_CORNERS[face][k];
            return alphalight::shadeCorner(*light, {x + corner[0], y + corner[1], z + corner[2]}, face, c);
        };
        meshFaces({0, 0, 0}, hi, opaqueAt, colour, shade, opaque);
    } else
        meshVoxels({0, 0, 0}, hi, opaqueAt, colour, opaque);
    // Water: a surface quad on each open water column, just below the block top.
    // Its colour is the region's water; the AO byte carries the depth.
    const Colors &k = colors();
    const int sea = settings.seaLevel;
    for (int y = 0; y < ZONE; y++)
        for (int x = 0; x < ZONE; x++) {
            if (zone.at(x, y, sea - 1) != WATER || zone.at(x, y, sea) != AIR)
                continue;
            float z = sea - .1f;
            uint32_t base = (uint32_t)water.vertices.size();
            int depth = 0;
            while (depth < 20 && zone.at(x, y, sea - 1 - depth) == WATER)
                depth++;
            uint8_t shallow = (uint8_t)(255 - std::min(depth, 12) * 21);
            glm::vec3 wc = k.water[zone.biome[(size_t)(y + 1) * ZoneData::P + (x + 1)]];
            for (auto &corner : FACE_CORNERS[4]) {
                glm::vec3 p(x + corner[0], y + corner[1], z);
                CornerShade s{(uint8_t)wc.r, (uint8_t)wc.g, (uint8_t)wc.b, shallow};
                if (light) { // Alpha: water's colour by its own light, shaded by the light above (9.5)
                    glm::vec3 w = alphalight::waterColour(light->light({x, y, sea - 1}) / 255.f);
                    VoxelColor c{alphalight::toByte(w.r), alphalight::toByte(w.g), alphalight::toByte(w.b), 0};
                    s = alphalight::shadeCorner(*light, {x + corner[0], y + corner[1], sea}, 4, c);
                }
                water.vertices.push_back({p.x, p.y, p.z, s.r, s.g, s.b, s.a, 4, WATER_SURFACE, 0, 0});
                water.min = glm::min(water.min, p);
                water.max = glm::max(water.max, p);
            }
            water.indices.insert(water.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
        }
}

glm::vec3 regionWater(int biome) { return colors().water[std::clamp(biome, 0, BIOMES - 1)]; }
