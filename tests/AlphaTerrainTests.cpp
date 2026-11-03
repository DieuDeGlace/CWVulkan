// Independent formula vectors, plus the explicit surface-stage/data-layout boundary.
// Formula checks establish contract conformance; native runtime oracles remain separate.
#include "alphagen/Terrain.h"
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
    std::printf("Alpha surface generation core: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
