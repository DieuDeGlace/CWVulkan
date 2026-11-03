// Independent formula vectors, plus the explicit surface-stage/data-layout boundary.
// Formula checks establish contract conformance; native runtime oracles remain separate.
#include "alphagen/Terrain.h"
#include "alphagen/Coordinates.h"
#include "alphagen/SurfaceStore.h"
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
