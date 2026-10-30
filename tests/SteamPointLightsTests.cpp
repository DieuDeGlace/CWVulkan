#include "SteamPointLights.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

namespace {
int failures = 0;
#define POINT_CHECK(value) do { if (!(value)) { ++failures; std::cerr << "Steam points line " << __LINE__ << ": " #value "\n"; } } while (0)
using namespace steampoints;
bool near(float a, float b) { return std::abs(a - b) < 1e-6f; }
ModelSource model(int type, Point position = {}) {
    ModelSource source;
    source.record.type = type;
    source.record.x = position[0]; source.record.y = position[1]; source.record.z = position[2];
    source.record.flags = 1;
    source.record.r = .2f; source.record.g = .4f; source.record.b = .6f;
    source.bakedLight = 255;
    return source;
}
worldgen::StaticObject object(int type) {
    worldgen::StaticObject source;
    source.type = type;
    return source;
}
}

int steamPointLightsTests() {
    failures = 0;
    Frame frame;
    frame.range = 10;
    frame.daylight = .4f;
    frame.visible = [](const Point &, float radius, float range) { return radius > 0 && range == 10; };
    ZoneSources zone;
    zone.models.push_back(model(0x17));
    std::vector<Light> lights;
    collect(zone, frame, lights);
    POINT_CHECK(lights.size() == 1);
    if (!lights.empty()) {
        POINT_CHECK(near(lights[0].radius, 15.5f)); // noise(0,0)=0, cos(0)=1
        POINT_CHECK(near(lights[0].colour.r, .12f) && near(lights[0].colour.g, .24f));
    }
    frame.daylight = std::nextafter(.4f, 1.f);
    lights.clear(); collect(zone, frame, lights);
    POINT_CHECK(lights.empty());
    zone.models[0].record.type = 20; // unrelated flag1 types have no .4 gate
    collect(zone, frame, lights);
    POINT_CHECK(lights.size() == 1 && near(lights[0].colour.r, .2f));

    zone.models = {model(0x4c), model(20)};
    zone.models[0].record.flags = 0; // the null marker does not require flag1
    zone.objects = {object(0x70)};
    lights.clear(); collect(zone, frame, lights);
    POINT_CHECK(lights.size() == 2); // marker stops models, but objects still run
    if (lights.size() == 2) {
        POINT_CHECK(near(lights[0].radius, 32));
        POINT_CHECK(near(lights[1].colour.b, 1.3f));
    }
    zone.models[0] = model(0x4c, {42 * 65536, 0, 0});
    lights.clear(); collect(zone, frame, lights);
    POINT_CHECK(lights.size() == 2 && near(lights[0].radius, 15.5f)); // equality excluded, no break

    zone.models.clear(); zone.objects = {object(0x3f)};
    zone.objects[0].scale = 1.5f; // native type height8, not an asset voxel height
    frame.daylight = .4f;
    lights.clear(); collect(zone, frame, lights);
    POINT_CHECK(lights.empty()); // object day gate is strict, unlike the model U gate
    frame.daylight = std::nextafter(.4f, 0.f);
    bool testedTop = false;
    frame.visible = [&](const Point &p, float radius, float) {
        testedTop = p[2] == 12 * 65536 && radius == 50;
        return testedTop;
    };
    collect(zone, frame, lights);
    POINT_CHECK(testedTop && lights.size() == 1);
    if (!lights.empty())
        POINT_CHECK(lights[0].position[2] == 12 * 65536 && lights[0].radius == 32);

    zone.objects = {object(0)};
    zone.objects[0].x = zone.objects[0].y = -1;
    zone.objects[0].active = false;
    frame.visible = [](const Point &, float, float) { return true; };
    lights.clear(); collect(zone, frame, lights);
    POINT_CHECK(lights.empty());
    frame.usedCell = [](int64_t x, int64_t y) { return x == -1 && y == -1; };
    collect(zone, frame, lights);
    POINT_CHECK(lights.size() == 1 && lights[0].position[2] == 5 * 65536);

    POINT_CHECK(distanceKey({200, 200, 0}, {}) == 0); // divide EACH square before summing
    POINT_CHECK(distanceKey({257, 257, 0}, {}) == 2.0f / 65536);
    POINT_CHECK(distanceKey({65536, 0, 0}, {}) == 1);
    POINT_CHECK(distanceKey({std::numeric_limits<int64_t>::min(), 0, 0}, {}) == 0); // machine wrap
    glm::mat4 matrix(1);
    matrix[3][0] = 61.5f;
    POINT_CHECK(bindingZone(matrix, {4,2,3}, {}) == ZoneKey(1,0)); // padded full-header center
    POINT_CHECK(bindingZone(matrix, {.5f,.5f,.5f}, {}) == ZoneKey(0,0)); // wrong occupied AABB
    matrix = glm::mat4(1);
    matrix[3][0] = -1.0f / 131072;
    POINT_CHECK(bindingZone(matrix, {}, {}) == ZoneKey(0,0)); // truncate fixed BEFORE floor
    matrix[3][0] = -1.0f / 65536;
    POINT_CHECK(bindingZone(matrix, {}, {}) == ZoneKey(-1,0));
    matrix[3][0] = 128; matrix[3][3] = 2;
    POINT_CHECK(bindingZone(matrix, {}, {}) == ZoneKey(1,0)); // homogeneous reciprocal
    POINT_CHECK(bindingZone(glm::mat4(1), {}, {64*65536,0,0}) == ZoneKey(1,0));
    lights.clear();
    for (int x = 24; x > 0; --x)
        lights.push_back({{x * 65536, 0, 0}, 1, glm::vec3((float)x)});
    const auto ranked = distribute(lights, {}, {{0,0}, {1,0}});
    const auto &first = ranked.at({0,0});
    POINT_CHECK(first.size() == 16 && ranked.at({1,0}).empty());
    for (size_t i = 0; i < first.size(); ++i)
        POINT_CHECK(first[i].colour.r == (float)i + 1);
    const auto rectangle = distribute({Light{{0,0,0}, 64, glm::vec3(1)}}, {}, {{-1,-1}, {0,0}, {1,1}, {2,0}});
    POINT_CHECK(rectangle.at({-1,-1}).size() == 1 && rectangle.at({1,1}).size() == 1);
    POINT_CHECK(rectangle.at({2,0}).empty()); // inclusive rectangle, no sphere/corner filter
    const auto negative = distribute({Light{{-1,-1,0}, 0, glm::vec3(1)}}, {}, {{-1,-1}, {0,0}});
    POINT_CHECK(negative.at({-1,-1}).size() == 1 && negative.at({0,0}).empty());
    std::cout << "Steam point lights: " << (failures ? "FAIL\n" : "PASS\n");
    return failures;
}
