#include "ZoneStates.h"
#include "NativeInteract.h"
#include "ObjectVisual.h"
#include "LightBeam.h"
#include "worldgen/ZoneLight.h"
#include "SteamScenery.h"
#include <cmath>
#include <cstdio>
#include <cstring>

int objectClockTests() {
    int failures = 0;
    auto check = [&](bool result, const char *message) {
        if (!result) { std::printf("FAIL object clock/beam: %s\n", message); ++failures; }
    };
    auto near = [](float a, float b) { return std::abs(a - b) < 0.00001f; };
    auto same = [](glm::vec3 a, glm::vec3 b) { return glm::length(a - b) < 0.00001f; };
    worldgen::ZoneObject source;
    source.type = 3;
    source.turn = 191;
    source.scale = 1.5f;
    source.swing = 1000;
    zonestates::ZoneStates states;
    auto &central = states.load({0, 0}, {}, {zonestates::staticObjectRecord(source)});
    states.load({-3, 3}, {}, central.staticObjects);
    states.load({-4, 0}, {}, central.staticObjects);
    std::vector<nativeinteract::Sound> sounds;
    auto &door = central.staticObjects[0];
    check(nativeinteract::openOrShut(door, 0, sounds), "closed door starts opening");
    states.advanceObjects(16, {{0, 0}, {1, 0}});
    auto drawn = objectvisual::read(door.data());
    check(near(drawn.elapsedMs, 16) && near(objectvisual::openness(drawn), 0.016f), "overlapping player rectangles tick once");
    check(drawn.turn == 191 && near(drawn.scale, 1.5f), "clock preserves full turn and placement scale");
    check(near(objectvisual::read(states.find({-3, 3})->staticObjects[0].data()).elapsedMs, 1016), "inclusive three-zone reach");
    check(near(objectvisual::read(states.find({-4, 0})->staticObjects[0].data()).elapsedMs, 1000), "outside reach stays unchanged");
    check(!central.staticObjectsChanged && states.zonesInMemory().size() == 3, "clock neither dirties nor generates zones");
    states.advanceObjects(16, {});
    check(near(objectvisual::read(door.data()).elapsedMs, 16), "no players means no selected zones");
    states.advanceObjects(234, {{0, 0}});
    const auto before = objectvisual::matrix(objectvisual::read(door.data()), {}, {6, 3, 9});
    nativeinteract::openOrShut(door, 1, sounds);
    drawn = objectvisual::read(door.data());
    const auto reversed = objectvisual::matrix(drawn, {}, {6, 3, 9});
    check(near(drawn.elapsedMs, 750) && same(glm::vec3(before * glm::vec4(3, 0, 0, 1)),
                                                          glm::vec3(reversed * glm::vec4(3, 0, 0, 1))),
          "reversing a quarter-open door preserves its visible pose");
    states.advanceObjects(16, {{0, 0}});
    check(near(objectvisual::openness(objectvisual::read(door.data())), 0.234f), "next step closes smoothly");
    states.advanceObjects(300, {{0, 0}});
    check(near(objectvisual::read(door.data()).elapsedMs, 1066), "finished timers keep advancing beyond one second");
    states.advanceObjects(0.25f, {{0, 0}});
    check(near(objectvisual::read(door.data()).elapsedMs, 1066.25f), "timer retains fractional milliseconds");

    source.type = 0x16;
    source.x = -1;
    source.y = -16 * 65536;
    source.active = true;
    auto &chest = states.load({-1, -1}, {}, {zonestates::staticObjectRecord(source)}).staticObjects[0];
    std::set<nativesaves::Cell> used{{-1, -1}};
    states.advanceObjects(16, {{0, 0}}, &used);
    drawn = objectvisual::read(chest.data());
    check(!drawn.closed && drawn.elapsedMs == 0, "negative personal cell floors correctly; changed state resets after addition");
    states.advanceObjects(16, {{0, 0}}, &used);
    check(objectvisual::read(chest.data()).elapsedMs == 16, "unchanged personal state keeps clock running");
    used.clear();
    states.advanceObjects(16, {{0, 0}}, &used);
    check(objectvisual::read(chest.data()).closed && objectvisual::read(chest.data()).elapsedMs == 0,
          "empty local personal set closes the reward object");
    states.advanceObjects(16, {{0, 0}});
    check(objectvisual::read(chest.data()).elapsedMs == 16, "no local player does not synchronize personal state");

    lightbeam::Placement shaft{{147456, 229376, 1622016}, 20, 1}; // (2.25,3.5,24.75)
    auto quad = lightbeam::sample(shaft, {2.25f, 3.5f, 24.75f}, 0, 20, 1);
    check(quad && quad->alpha == 100 && same(quad->corners[0], {1.75f, 3.5f, 24.75f}) &&
          same(quad->corners[3], {7.25f, 3.5f, 4.75f}), "native upper width, expanded lower width and downward extent");
    quad = lightbeam::sample(shaft, {2.25f, 3.5f, 24.75f}, 90, 20, 1);
    check(quad && same(quad->corners[0], {2.25f, 3, 24.75f}) && same(quad->corners[3], {2.25f, 8.5f, 4.75f}),
          "displayed camera yaw rotates the width direction");
    quad = lightbeam::sample(shaft, {21.25f, 3.5f, 24.75f}, 0, 20, 1);
    check(quad && quad->alpha == 50, "fade reaches half opacity at 95 percent of range");
    check(!lightbeam::sample(shaft, {22.25f, 3.5f, 24.75f}, 0, 20, 1), "upper midpoint at exact range is excluded");
    quad = lightbeam::sample(shaft, {2.25f, 3.5f, 24.75f}, 0, 20, 0);
    check(quad && quad->alpha == 0, "CRT daylight zero makes the primitive transparent");
    quad = lightbeam::sample(shaft, {2.25f, 3.5f, 24.75f}, 0, 20, 3);
    check(quad && quad->alpha == 44, "upper alpha is retained as a byte");
    const Mesh mesh = lightbeam::mesh();
    check(mesh.indices == std::vector<uint32_t>({0, 1, 2, 2, 1, 3}) && mesh.vertices.size() == 4 &&
          mesh.vertices[0].ao == 255 && mesh.vertices[2].ao == 0, "quad uses native topology and lower-edge alpha zero");
    // A roofed cavity and a solid sample distinguish native model light from
    // palette red, vertex AO, and a global daylight factor.
    std::vector<worldgen::TerrainColumn> columns(worldgen::ZONE_COLUMNS);
    for (auto &column : columns) {
        column.base = -2;
        column.blocks = {{100, 20, 30, 0, 3, 0}, {0, 0, 0, 0, 0, 0},
                         {100, 20, 30, 0, 3, 0}, {0, 0, 0, 0, 0, 0}, {100, 20, 30, 0, 3, 0}};
    }
    const auto sourceColumns = [&](int32_t, int32_t) -> const auto & { return columns; };
    auto lights = worldgen::sampleZoneLight(0, 0, sourceColumns,
        {{0, 0, -65536}, {0, 0, 0}, {0, 0, 65536}, {0, 0, 3 * 65536}, {-9 * 65536, 0, 0}, {0, 0, -3 * 65536}});
    check(lights == std::vector<uint8_t>({21, 0, 216, 255, 255, 0}),
          "native five-round model samples include solid, below-base, sky and padded-map boundaries");
    for (auto &column : columns)
        column.blocks.back() = {0, 0, 0, 50, 20, 0};
    lights = worldgen::sampleZoneLight(0, 0, sourceColumns, {{0, 0, 0}, {0, 0, -65536}});
    check(lights == std::vector<uint8_t>({255, 216}), "direct-sky open block restores running light before the next solid");
    for (auto &column : columns) {
        column.blocks.clear();
        column.base = 12;
    }
    lights = worldgen::sampleZoneLight(0, 0, sourceColumns, {{0, 0, 11 * 65536}, {0, 0, 12 * 65536}});
    check(lights == std::vector<uint8_t>({0, 255}), "empty stored column still has a dark below-base boundary");
    const auto modelBounds = steamscenery::bounds({0, 0, 0}, {4, 90, 20}, 1, 0);
    check(modelBounds.center[2] == 10 * 65536 && modelBounds.radius == 20,
          "ordinary model culling uses raised center and max voxel X/Z, not Y or half-radius");
    const float raisedDistance = steamscenery::distanceSquared(modelBounds.center, {0, 0, 29});
    check(near(steamscenery::fade(raisedDistance, 20), 0.5f), "ordinary fade samples the raised center without a 64-block extension");
    check(steamscenery::queue(modelBounds, 0) == steamscenery::Queue::Hidden &&
          steamscenery::queue(modelBounds, 1) == steamscenery::Queue::OrdinaryOpaque &&
          steamscenery::queue(modelBounds, 0.5f) == steamscenery::Queue::OrdinaryFaded,
          "ordinary faded descriptors follow water while nonpositive fades are omitted");
    const auto wetBounds = steamscenery::bounds({0, 0, 0}, {4, 9, 3}, 1.5f, 8);
    check(wetBounds.center[2] == 147456 && wetBounds.radius == 6 &&
          steamscenery::queue(wetBounds, 1) == steamscenery::Queue::WaterOpaque &&
          steamscenery::queue(wetBounds, 0.5f) == steamscenery::Queue::WaterFaded,
          "water flag retains its own opaque and faded queues before water");
    std::puts("object clocks and light shafts: selection, reversal, personal state, geometry, fade and native model light checked");
    return failures;
}
