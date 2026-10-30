#include "AlphaScenery.h"
#include "NativeKeyOrder.h"
#include <cmath>
#include <cstdio>
#include <vector>

int alphaSceneryTests() {
    int failures = 0;
    auto check = [&](bool passed, const char *what) {
        if (!passed) {
            std::printf("FAIL Alpha scenery: %s\n", what);
            ++failures;
        }
    };
    using namespace alphascenery;
    auto model = sample({Kind::Model, {0, 0, 0}}, {}, 640, 200);
    check(model.end == 128.f && model.full == 115.19999694824219f, "separate model range stages");
    const float boundary = model.full * model.full;
    check(boundary == 13271.0390625f, "native threshold reference");
    model.squared = std::nextafter(boundary, 0.f);
    check(!model.afterWater(), "previous float stays near");
    model.squared = boundary;
    check(!model.afterWater() && model.opacity() == 1, "equality stays before water");
    model.squared = std::nextafter(boundary, INFINITY);
    check(model.afterWater(), "following float defers");
    model.squared = 120.f * 120.f;
    check(std::abs(model.opacity() - .625f) < .000001f, "captured squared-distance fade");
    model.squared = 130.f * 130.f;
    check(model.opacity() < 0, "no invented final clamp or admission");
    const auto object = sample({Kind::Object, {3 * 65536, 4 * 65536, 12 * 65536}}, {}, 640, 200);
    check(object.squared == 169, "full XYZ fixed distance");
    check(object.end == (200.f + 22.4f) * .3f && object.end != model.end, "object uses fog range");
    struct Entry {
        int id;
        float squared;
        Kind kind;
    };
    std::vector<Entry> mixed{{1, 10, Kind::Model}, {2, 30, Kind::Object}, {3, 20, Kind::Model}, {4, 30, Kind::Model}};
    keyorder::sort(mixed, [](const Entry &entry) { return -entry.squared; });
    check(mixed[0].id == 2 && mixed[1].id == 4 && mixed[2].id == 3 && mixed[3].id == 1,
          "one descending mixed vector, small finite ties retain source order");
    std::printf("Alpha scenery staging: %s\n", failures ? "FAILED" : "PASS");
    return failures;
}
