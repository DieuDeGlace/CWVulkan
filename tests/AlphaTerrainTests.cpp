// Independent formula vectors, plus the explicit surface-stage/data-layout boundary.
// Formula checks establish contract conformance; native runtime oracles remain separate.
#include "alphagen/Terrain.h"
#include "alphagen/Coordinates.h"
#include "alphagen/SurfaceStore.h"
#include "alphagen/SteamBridge.h"
#include <future>
#include <cmath>
#include <cstdio>

int alphaTerrainTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL Alpha terrain: %s\n", what);
            ++failures;
        }
    };
    alphacoords::Mapping mapping{{0, 0}, {235, 235}};
    const auto patches = mapping.zone(0, 0);
    check(patches && patches->count == 4, "Steam zone spans four Alpha parents");
    int covered = 0;
    if (patches)
        for (size_t i = 0; i < patches->count; ++i)
            covered += patches->patches[i].width * patches->patches[i].height;
    check(covered == 4096 && !mapping.point(-1000, -1000), "finite nonwrapping Alpha coverage");
    alphagen::Random random(123);
    check(random.draw() == 440 && random.draw() == 19053 && random.draw() == 23075, "native random stream");
    const auto offsets = alphagen::makeSeedOffsets(123);
    check(offsets.A() == 440 && offsets[0x168] == 19053 && offsets[0x178] == 25656 && offsets[0x274] == 26554,
          "seed offsets preserve non-address draw order");
    check(alphagen::noise(0, 0) == -0.28179097175598145f && alphagen::noise(1.25, 2.75) == 0.14833128452301025f &&
              alphagen::noise(-0.3, 0) == -0.3548884093761444f,
          "float noise words including negative fractional truncation");
    alphagen::World world(123);
    const auto *home = world.makeCell(512, 512);
    check(home && home->x == 8397824 && home->y == 8397824 && home->elevation == 74 && home->moisture == 0.4915097653865814f &&
              home->warmth == 0.5601367354393005f,
          "home region cell and climate");
    alphagen::Terrain terrain(world);
    check(!terrain.generateSurface(-1, 32800) && !terrain.generateSurface(65536, 32800), "finite Alpha zone domain");
    auto stage = terrain.generateSurface(32800, 32800);
    check(stage && stage->zone && stage->paddedHeights.size() == 257 * 257, "complete padded surface stage");
    if (stage && stage->zone) {
        const auto &zone = *stage->zone;
        check(zone.zx == 32800 && zone.zy == 32800 && zone.columns.size() == 256 * 256, "Alpha zone is not a Steam 64-column zone");
        check(stage->paddedHeights[256 * 257] == terrain.height(zone.originX() + 256, zone.originY()) &&
                  stage->paddedHeights[256] == terrain.height(zone.originX(), zone.originY() + 256),
              "padded far row and column use absolute coordinates");
        bool valid = true;
        for (const auto &column : zone.columns)
            valid &= !column.blocks.empty() && std::isfinite(column.moisture) && std::isfinite(column.warmth);
        check(valid, "every surface column retains blocks and climate");
        check(zone.shadedLight.empty() && zone.plants.empty() && zone.statics.empty(),
              "surface stage excludes later lighting and placement");
    }
    // Alpha type13 is glowing, while Steam type13 is road. Flags are different too.
    const auto light = alphagen::steamBlock({40, 50, 60, uint8_t(alphagen::GLOWING | alphagen::KEEP)});
    check(light && light->type == worldgen::blocktype::LIGHT && light->glow == 255 && !worldgen::isOpen(*light),
          "glowing Alpha material is not a Steam road");
    const auto dry = alphagen::steamBlock({255, 255, 255, uint8_t(alphagen::AIR | alphagen::DRY_AIR | alphagen::KEEP)});
    const auto plain = alphagen::steamBlock({255, 255, 255, alphagen::AIR});
    worldgen::ZoneTerrain nativeRead;
    nativeRead.columns.resize(worldgen::ZONE_COLUMNS);
    nativeRead.columns[0].base = -1;
    if (dry && plain)
        nativeRead.columns[0].blocks = {*dry, *plain};
    check(dry && plain && nativeRead.read(0, 0, -1).type == worldgen::blocktype::AIR &&
              nativeRead.read(0, 0, 0).type == worldgen::blocktype::WATER && dry->flags == 2,
          "translated dry-air flag preserves native effective underwater reads");
    check(!alphagen::steamBlock({0, 0, 0, 31}) && !alphagen::steamBlock({0, 0, 0, alphagen::WOOD_TYPE}) &&
              !alphagen::steamBlock({0, 0, 0, alphagen::LEAVES_TYPE}),
          "unknown and unspecified Alpha materials are rejected rather than reinterpreted");
    {
        alphagen::SurfaceStore source(123, {{0, 0}, {alphacoords::HOME + 107, alphacoords::HOME + 107}});
        std::string why = "old diagnostic";
        const auto tile = alphagen::steamTile(source, 0, 0, why);
        check(tile && why.empty(), "four-parent surface crop converts into one Steam-sized tile");
        if (tile) {
            check(tile->covers(0, 0) && tile->covers(63, 63) && !tile->covers(64, 0), "Steam tile coverage uses source coordinates");
            bool matching = true, axisWitness = false;
            for (int x = 0; x < 64; ++x)
                for (int y = 0; y < 64; ++y) {
                    const auto original = source.column(x, y), transposed = source.column(y, x);
                    if (!original || !transposed) {
                        matching = false;
                        continue;
                    }
                    const auto &a = original->raw();
                    const auto &b = tile->columns()[worldgen::ZoneTerrain::index(x, y)];
                    matching &= a.bottom == b.base && a.blocks.size() == b.blocks.size();
                    for (size_t i = 0; i < a.blocks.size() && i < b.blocks.size(); ++i)
                        matching &= a.blocks[i].r == b.blocks[i].r && a.blocks[i].g == b.blocks[i].g && a.blocks[i].b == b.blocks[i].b;
                    axisWitness |= a.bottom != transposed->raw().bottom || a.blocks.size() != transposed->raw().blocks.size();
                }
            check(matching && axisWitness, "all 4096 converted columns preserve geometry and RGB across an asymmetric four-parent crop");
            for (const auto point : {alphacoords::Point{17, 53}, alphacoords::Point{4, 29}, alphacoords::Point{63, 0}}) {
                const auto original = source.column(point.x, point.y);
                if (!original) {
                    check(false, "mapped source column is available");
                    continue;
                }
                const int z = original->raw().top() - 1;
                const auto a = original->block(z);
                const auto b = tile->read(point.x, point.y, z);
                check(a.r == b.r && a.g == b.g && a.b == b.b && alphagen::clear(a.type()) == worldgen::isOpen(b),
                      "Alpha XY-major and Steam X-major column indexing preserve sampled voxel identity");
            }
        }
        check(!alphagen::steamTile(source, INT32_MAX, 0, why) && !why.empty(), "invalid framing cannot publish a partial converted tile");
        alphagen::SteamTile absent(0, 0);
        check(absent.read(0, 0, 100).type == worldgen::blocktype::SOLID,
              "uncovered Alpha columns are missing-domain filler, not implicit sea or air");
    }
    stage.reset();
    std::optional<alphagen::ColumnLease> held;
    std::weak_ptr<const alphagen::SurfaceStage> pinned;
    alphagen::Block retainedBlock;
    float retainedHeight = 0;
    {
        alphagen::SurfaceStore source(123, {}, 1);
        check(!source.column(INT32_MIN, 0), "unmapped coordinates do not wrap into an Alpha parent");
        auto first = std::async(std::launch::async, [&] { return source.parent(32800, 32800); });
        auto second = std::async(std::launch::async, [&] { return source.parent(32800, 32800); });
        const auto a = first.get(), b = second.get();
        check(a && a == b, "concurrent Steam-sized consumers share one Alpha parent stage");
        pinned = a;
        held = source.column(0, 0);
        check(held && held->position().x == alphacoords::HOME, "column lease preserves absolute Alpha coordinates");
        if (held) {
            retainedBlock = held->block(0);
            retainedHeight = held->surface();
        }
        check(source.parent(32801, 32800) != a, "a different parent can replace the bounded ready-cache entry");
        // The sea cell center has an island; sample its subcell boundary farther east.
        const auto ocean = source.parent(32848, 32796);
        bool waterReadChecked = false;
        if (ocean)
            for (int x = 0; x < 256 && !waterReadChecked; ++x)
                for (int y = 0; y < 256 && !waterReadChecked; ++y) {
                    const int ax = ocean->zone->originX() + x, ay = ocean->zone->originY() + y;
                    if (ocean->zone->read(ax, ay, 0).type() != alphagen::WATER)
                        continue;
                    const auto wet = source.column(ax - alphacoords::HOME, ay - alphacoords::HOME);
                    if (wet && wet->surface(false) < 1) {
                        check(wet->surface() == 1, "water surface is the top of the effective z0 water cell");
                        waterReadChecked = true;
                    }
                }
        if (!waterReadChecked && ocean) {
            float minGrid = ocean->paddedHeights.front(), maxGrid = minGrid;
            for (auto value : ocean->paddedHeights) {
                minGrid = std::min(minGrid, value);
                maxGrid = std::max(maxGrid, value);
            }
            std::printf("Alpha water fixture grid range: %.3f .. %.3f; first bottom/top: %d/%d\n", minGrid, maxGrid,
                        ocean->zone->columns.front().bottom, ocean->zone->columns.front().top());
        }
        check(waterReadChecked, "water and solid-surface reads use different effective column products");
    }
    check(held && !pinned.expired(), "column lease survives ready-cache eviction and store destruction");
    if (held)
        check(held->block(0).typeByte == retainedBlock.typeByte && held->block(0).r == retainedBlock.r && held->surface() == retainedHeight,
              "leased block and surface remain readable without the world store");
    held.reset();
    check(pinned.expired(), "the last lease releases its parent stage");
    std::printf("Alpha surface generation core: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
