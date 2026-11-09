// User-requested hybrid continuity checks, separate from native Alpha formula vectors.
#include "alphagen/SteamBridge.h"
#include "worldgen/AlphaTerrain.h"
#include "worldgen/Generator.h"
#include "worldgen/MapModel.h"
#include "worldgen/Options.h"
#include <algorithm>

#include <cmath>
#include <cstdio>

namespace {
int ground(const worldgen::TerrainColumn &column) {
    for (int i = int(column.blocks.size()) - 1; i >= 0; --i) {
        const auto &block = column.blocks[size_t(i)];
        if (!worldgen::isOpen(block) && block.type != worldgen::blocktype::WOOD &&
            block.type != worldgen::blocktype::LEAVES && block.type != 11)
            return column.base + i + 1;
    }
    return column.base;
}
int mapTop(const worldgen::MapRegion &map, int x, int y) {
    const int i = worldgen::floorDivBy(x, map.unit) - 16 * map.rx + 3;
    const int j = worldgen::floorDivBy(y, map.unit) - 16 * map.ry + 3;
    for (int k = map.sizeZ - 1; k >= 0; --k) {
        const auto *rgb = map.voxel(i, j, k);
        if (rgb[0] || rgb[1] || rgb[2])
            return (map.base() + k + 1) * map.unit;
    }
    return map.lowest;
}
} // namespace

int alphaTransitionTests() {
    using namespace worldgen;
    int failures = 0;
    auto check = [&](bool pass, const char *message) {
        if (!pass) {
            ++failures;
            std::printf("FAIL Alpha transition: %s\n", message);
        }
    };
    const Block rockA{11, 22, 33, 0, blocktype::ROCK, 0};
    const Block rockB{44, 55, 66, 0, blocktype::GROUND, 0};
    const Block waterA{3, 4, 5, 0, blocktype::WATER, 0};
    const Block waterB{6, 7, 8, 0, blocktype::WATER, 0};
    TerrainColumn sea{-3, {rockA, rockB, waterA, waterB}, true, 7};
    const auto unchanged = alphagen::shiftedTerrainColumn(sea, 0);
    check(unchanged && unchanged->base == sea.base && unchanged->blocks == sea.blocks && unchanged->fullColour &&
              unchanged->sourceShift == 7,
          "zero shift preserves exact block bytes and source metadata");
    const auto raised = alphagen::shiftedTerrainColumn(sea, 6);
    check(raised && raised->base == 3 && raised->sourceShift == 13 && raised->fullColour &&
              raised->blocks.size() == 2 && raised->blocks[0] == rockA && raised->blocks[1] == rockB,
          "raising ocean ground keeps its solid material and removes lifted sea water");
    const auto lowered = alphagen::shiftedTerrainColumn(sea, -3);
    ZoneTerrain reader;
    reader.columns.resize(ZONE_COLUMNS);
    if (lowered)
        reader.columns[0] = *lowered;
    check(lowered && lowered->base == -6 && lowered->sourceShift == 4 &&
              reader.read(0, 0, 0).type == blocktype::WATER && reader.read(0, 0, 1).type == blocktype::AIR,
          "lowered ground retains the fixed effective sea plane");

    TerrainColumn pool{2, {rockA, rockB, waterA, waterB}, true, 0};
    const auto poolUp = alphagen::shiftedTerrainColumn(pool, 1);
    if (poolUp)
        reader.columns[0] = *poolUp;
    check(poolUp && reader.read(0, 0, 4) == rockB && reader.read(0, 0, 5) == waterB &&
              reader.read(0, 0, 6).type == blocktype::AIR,
          "raising a pool floor clips water at its original level");
    const auto poolDown = alphagen::shiftedTerrainColumn(pool, -2);
    if (poolDown)
        reader.columns[0] = *poolDown;
    check(poolDown && reader.read(0, 0, 1) == rockB && reader.read(0, 0, 2).type == blocktype::WATER &&
              reader.read(0, 0, 4) == waterA && reader.read(0, 0, 5) == waterB &&
              reader.read(0, 0, 6).type == blocktype::AIR,
          "lowering a pool floor fills open space while retaining the original surface and stored water colors");
    alphagen::PlantModels models;
    models.environment[31] = 1031;
    const alphagen::Plant lily{0, 0, 6, .09f, 0, 31, 0};
    for (const int shift : {1, -2}) {
        const auto movedPool = alphagen::shiftedTerrainColumn(pool, shift);
        const auto floating = alphagen::steamPlant(lily, models, shift);
        if (movedPool)
            reader.columns[0] = *movedPool;
        const int z = floating ? int(floating->z >> 16) : 0;
        check(movedPool && floating && reader.read(0, 0, z).type == blocktype::WATER &&
                  reader.read(0, 0, z + 1).type == blocktype::AIR,
              "floating plant base remains on the composed pool surface after solid translation");
    }
    TerrainColumn dry{1, {rockA, Block{255, 255, 255, 0, blocktype::AIR, 2}, waterA, waterB}, true, 0};
    const auto dryMoved = alphagen::shiftedTerrainColumn(dry, -1);
    if (dryMoved)
        reader.columns[0] = *dryMoved;
    check(dryMoved && reader.read(0, 0, 1).type == blocktype::AIR && reader.read(0, 0, 1).flags == 2,
          "water filling never overwrites translated protected dry air");
    TerrainColumn wetOnly{-2, {waterA, waterB}, true, 0};
    const auto wetMoved = alphagen::shiftedTerrainColumn(wetOnly, 2);
    if (wetMoved)
        reader.columns[0] = *wetMoved;
    check(wetMoved && !wetMoved->empty() && reader.read(0, 0, -1).type == blocktype::SOLID &&
              reader.read(0, 0, 0).type == blocktype::WATER,
          "a translated water-only run retains its implicit rock boundary");
    TerrainColumn limit{INT32_MAX - 2, {rockA, rockB}, true, 0};
    check(!alphagen::shiftedTerrainColumn(limit, 1), "out-of-range translated storage is rejected");
    limit.base = 0;
    limit.sourceShift = INT32_MAX;
    check(!alphagen::shiftedTerrainColumn(limit, 1), "out-of-range source metadata is rejected");
    check(sea.base == -3 && sea.sourceShift == 7 && sea.blocks == std::vector<Block>{rockA, rockB, waterA, waterB},
          "column translation leaves the immutable original storage unchanged");

    const auto oldSeed = currentWorld().seed;
    const auto oldScale = worldScale();
    setWorldSeed(0);
    setWorldScale(1);
    const auto world = worldValues(0);
    AlphaTerrain alpha(1570745101u, {}, 0);
    std::string error;

    // A preserved forest mound meets Alpha ocean at the shared x=9920 tile edge.
    // The two boundary columns must approach one another without lifting the sea.
    auto outside = generateZone(world, 154, -169);
    auto inside = generateZone(world, 155, -169);
    const auto coreBefore = inside.terrain.columns[ZoneTerrain::index(0, 32)];
    check(alpha.compose(outside, world, error) && alpha.compose(inside, world, error),
          "both sides of the reproduced boundary compose successfully");
    const auto &outer = outside.terrain.columns[ZoneTerrain::index(63, 32)];
    const auto &inner = inside.terrain.columns[ZoneTerrain::index(0, 32)];
    const int step = std::abs(ground(outer) - ground(inner));
    std::printf("Alpha transition boundary: outside %d, inside %d, one-column step %d\n", ground(outer), ground(inner),
                step);
    check(step <= 8, "a coastal construction boundary has a gradual voxel transition");
    check(inner.base == coreBefore.base && inner.blocks == coreBefore.blocks && !inner.fullColour,
          "protected Steam core retains its exact geometry and color encoding");

    auto mound = generateZone(world, 156, -169);
    const auto peakBefore = mound.terrain.columns[ZoneTerrain::index(8, 56)];
    check(alpha.compose(mound, world, error), "mound peak composes successfully");
    const auto &peak = mound.terrain.columns[ZoneTerrain::index(8, 56)];
    check(ground(peak) == 281 && peak.base == peakBefore.base && peak.blocks == peakBefore.blocks,
          "forest mound retains its full original peak and all stored blocks");

    auto map = mapRegion(19, -22, 3);
    const int retainedTop = mapTop(map, 9920, -10784);
    alpha.composeMap(map);
    const int mapStep = std::abs(mapTop(map, 9888, -10784) - mapTop(map, 9920, -10784));
    std::printf("Alpha transition overview: one-sample step %d at unit %d\n", mapStep, map.unit);
    check(mapStep <= 64, "coarse map samples show the blended approach instead of a rectangular pad edge");
    check(mapTop(map, 9920, -10784) == retainedTop, "native map core retains its complete original height");

    // Steam biome membership and stored colors remain authoritative in the
    // hybrid, including where the independent Alpha seed supplies ocean.
    struct BiomeCase { int type, x, y; };
    for (const auto test : {BiomeCase{12, 12288, -11776}, BiomeCase{10, -27136, 10240},
                            BiomeCase{10, 25766, -1334}}) {
        const int zx = floorDivBy(test.x, 64), zy = floorDivBy(test.y, 64);
        auto zone = generateZone(world, zx, zy);
        const auto original = zone.terrain.columns;
        check(alpha.compose(zone, world, error), "representative land biome composes");
        bool dry = true, palette = true, gentle = true;
        int compared = 0;
        for (int lx = 0; lx < 64; ++lx)
            for (int ly = 0; ly < 64; ++ly) {
                const int x = zx * 64 + lx, y = zy * 64 + ly;
                if (biomeAt(x, y) != test.type)
                    continue;
                const auto index = ZoneTerrain::index(lx, ly);
                const auto &before = original[index], &after = zone.terrain.columns[index];
                const int nativeTop = ground(before), hybridTop = ground(after);
                if (nativeTop > 1)
                    dry &= hybridTop > 1;
                const auto at = [](const TerrainColumn &column, int z) {
                    const auto k = int64_t(z) - column.base;
                    return k >= 0 && k < int64_t(column.blocks.size()) ? column.blocks[size_t(k)] : SOLID_GREY;
                };
                const auto a = at(before, nativeTop - 1), b = at(after, hybridTop - 1);
                palette &= !after.fullColour && a.r == b.r && a.g == b.g && a.b == b.b;
                gentle &= before.base == after.base && before.blocks == after.blocks;
                ++compared;
            }
        std::printf("Alpha biome %d: %d compared, dry %d, palette %d, native geometry %d\n",
                    test.type, compared, dry, palette, gentle);
        check(compared > 0 && dry, "Steam land does not become Alpha sea");
        check(palette, "actual hybrid ground retains each Steam biome's stored RGB encoding");
        if (test.type == 10)
            check(gentle, "savannah retains native terrain instead of ocean or excessive Alpha relief");

        const int rx = floorDivBy(test.x, 512), ry = floorDivBy(test.y, 512);
        auto overview = mapRegion(rx, ry, 3);
        const auto nativeMap = overview;
        alpha.composeMap(overview);
        bool mapDry = true, mapGentle = true;
        for (int x = rx * 512; x < (rx + 1) * 512; x += 32)
            for (int y = ry * 512; y < (ry + 1) * 512; y += 32) {
                if (biomeAt(x, y) != test.type)
                    continue;
                const int before = mapTop(nativeMap, x, y), after = mapTop(overview, x, y);
                if (before > 64)
                    mapDry &= after > 32;
                mapGentle &= before == after;
            }
        check(mapDry, "map land coverage agrees with the corrected playable biome");
        if (test.type == 10)
            check(mapGentle, "savannah overview retains native relief");
    }

    setWorldSeed(oldSeed);
    setWorldScale(oldScale);
    std::printf("Alpha hybrid transitions: %d failure(s)\n", failures);
    return failures;
}
