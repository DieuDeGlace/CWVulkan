#include "ObjectVisual.h"
#include <cmath>
#include <cstdio>

int objectVisualTests() {
    int failures = 0;
    auto check = [&](bool result, const char *message) {
        if (!result) { std::printf("FAIL object geometry: %s\n", message); ++failures; }
    };
    auto near = [](float a, float b) { return std::abs(a - b) < 0.00001f; };
    auto point = [](const glm::mat4 &m, glm::vec3 p) { return glm::vec3(m * glm::vec4(p, 1)); };
    auto same = [](glm::vec3 a, glm::vec3 b) { return glm::length(a - b) < 0.00002f; };
    check(near(objectvisual::voxelScale(3, 1, 6), 1.0f / 3), "small door has two-block width from six voxel columns");
    check(near(objectvisual::voxelScale(5, 1.5f, 16), 0.375f), "opening scale is applied once to the large iron door");
    check(near(objectvisual::voxelScale(65, 1, 6), 0.3f), "lamp visual factor uses the X voxel dimension");
    check(near(objectvisual::voxelScale(66, 1, 6), 2.0f / 3), "neighboring lamp type has its own visual factor");
    check(objectvisual::voxelScale(3, 1, 0) == 0, "missing dimensions cannot cause division by zero");

    objectvisual::State door{3, 0, 1, 1, 1000};
    const glm::vec3 at(10, 20, 30), dimensions(6, 3, 9), hinge(-3, 0, 0);
    const auto shut = objectvisual::matrix(door, at, dimensions);
    check(same(point(shut, {0, 0, 9}), at + glm::vec3(0, 0, 3)), "door stays bottom-centered and uniformly scaled");
    door.closed = 0;
    const auto open = objectvisual::matrix(door, at, dimensions);
    check(same(point(shut, hinge), point(open, hinge)), "opening preserves the left-edge hinge");
    check(same(point(open, {3, 0, 0}), at + glm::vec3(-1, 2, 0)), "door's right edge rotates around its hinge");
    door.turn = 1;
    const auto turnedOpen = objectvisual::matrix(door, at, dimensions);
    check(same(point(turnedOpen, hinge), at + glm::vec3(0, -1, 0)), "static wall rotation also rotates the hinge");

    objectvisual::State window{7, 0, 0, 1, 1000};
    const auto pane = objectvisual::matrix(window, at, {9, 1, 18});
    window.closed = 1;
    check(same(point(pane, {4.5f, 0, 18}), point(objectvisual::matrix(window, at, {9, 1, 18}), {4.5f, 0, 18})),
          "large window type seven remains fixed");
    objectvisual::State gate{8, 0, 0, 2, 1000};
    check(near(point(objectvisual::matrix(gate, at, {10, 1, 10}), {}).z, 31.6f), "sliding gate rises by 0.8 of scaled base height");
    objectvisual::State hatch{0x12, 0, 1, 1, 1000};
    check(near(point(objectvisual::matrix(hatch, at, {10, 1, 10}), {}).z, 29), "closed type eighteen retains its fixed downward offset");
    door.turn = 191;
    check(same(point(objectvisual::matrix(door, at, dimensions), hinge),
               at + glm::vec3(-std::cos(191.0f * 90.0f * 0.01745329238474369f),
                              -std::sin(191.0f * 90.0f * 0.01745329238474369f), 0)), "full unsigned turns remain valid");
    door.elapsedMs = -100;
    check(near(objectvisual::openness(door), -0.1f), "native opening has no extra lower clamp");
    door.closed = 1;
    check(near(objectvisual::openness(door), 1.1f), "native closing has no extra upper clamp");
    std::puts("object geometry: type dimensions, visual factors, hinge pivots and sliding extents checked");
    return failures;
}
