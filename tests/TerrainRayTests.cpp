// Derived native traversal references; no game assets or replacement world required.
#include "TerrainRay.h"
#include <array>
#include <cstdio>
#include <cmath>
#include <vector>

int terrainRayTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL Terrain ray: %s\n", what);
            ++failures;
        }
    };
    using namespace terrainray;
    using Point = std::array<int32_t, 3>;
    World world;
    world.hasZone = [](int32_t, int32_t) { return true; };
    std::vector<Point> visited;
    world.block = [&](int32_t x, int32_t y, int32_t z) {
        visited.push_back({x, y, z});
        return Cell{uint8_t(x == 1 ? 1 : 0), 0};
    };
    check(trace(world, {16384, 32768, 32768}, {1, 0, 0}, 60, Material::HonorFlag40) == .75f,
          "positive boundary reaches obstructed next cell at .75");
    check(visited == std::vector<Point>{{0, 0, 0}, {1, 0, 0}}, "initial effective cell is read before advancement");
    world.block = [](int32_t, int32_t, int32_t) { return Cell{0, 0}; };
    check(trace(world, {16384, 32768, 32768}, {1, 0, 0}, 60, Material::HonorFlag40) == 60, "clear ray returns its positive limit");
    visited.clear();
    world.block = [&](int32_t x, int32_t y, int32_t z) {
        visited.push_back({x, y, z});
        return Cell{uint8_t(x == 0), 0};
    };
    check(trace(world, {65536, 32768, 32768}, {-1, 0, 0}, 60, Material::HonorFlag40) == 0 &&
              visited == std::vector<Point>{{1, 0, 0}, {0, 0, 0}},
          "negative exact boundary advances with zero parameter");
    visited.clear();
    world.block = [&](int32_t x, int32_t y, int32_t z) {
        visited.push_back({x, y, z});
        return Cell{uint8_t(x == 1 && y == 1), 0};
    };
    check(trace(world, {32768, 32768, 32768}, {1, 1, 0}, 60, Material::HonorFlag40) == .5f &&
              visited == std::vector<Point>{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}},
          "strict axis ties visit intermediate X cell then zero-length Y crossing");
    for (uint8_t flags : {uint8_t(0x40), uint8_t(0xc0), uint8_t(0x80)}) {
        world.block = [&](int32_t, int32_t, int32_t) { return Cell{2, flags}; };
        check(trace(world, {32768, 32768, 32768}, {1, 0, 0}, 1, Material::HonorFlag40) == (flags == 0x80 ? 1 : 0),
              "aim honors flag40 independently of flag80");
        check(trace(world, {32768, 32768, 32768}, {1, 0, 0}, 1, Material::TypeOnly) == 1,
              "camera predicate does not inherit aim flag40 obstruction");
    }
    int reads = 0;
    std::vector<std::array<int32_t, 2>> zones;
    world.block = [&](int32_t, int32_t, int32_t) {
        ++reads;
        return Cell{0, 0};
    };
    world.hasZone = [&](int32_t x, int32_t y) {
        zones.push_back({x, y});
        return x == 0;
    };
    check(trace(world, {63 * 65536 + 32768, 32768, 32768}, {1, 0, 0}, .5f, Material::HonorFlag40) == .5f && reads == 1 &&
              zones == std::vector<std::array<int32_t, 2>>{{0, 0}, {1, 0}},
          "crossed-zone lookup precedes range exit but endpoint cell is not read");
    reads = 0;
    zones.clear();
    check(trace(world, {63 * 65536 + 32768, 32768, 32768}, {1, 0, 0}, 60, Material::HonorFlag40) == .5f && reads == 1,
          "missing crossed zone returns already advanced parameter");
    reads = 0;
    world.hasZone = [](int32_t, int32_t) { return false; };
    check(trace(world, {0, 0, 0}, {1, 0, 0}, 60, Material::HonorFlag40) == 0 && reads == 0, "missing initial zone does not read a block");
    world.hasZone = [](int32_t, int32_t) { return true; };
    reads = 0;
    const float budget = trace(world, {0, 0, 0}, {1000, 0, 0}, 60, Material::HonorFlag40);
    check(reads == 502 && budget > 0 && budget < 1, "501 advances still read the final cell before budget termination");
    visited.clear();
    world.block = [&](int32_t x, int32_t y, int32_t z) {
        visited.push_back({x, y, z});
        return Cell{0, 0};
    };
    check(trace(world, {0, 0, 0}, {0, 0, 0}, 15, Material::HonorFlag40) == 15 && visited == std::vector<Point>{{0, 0, 0}, {-1, 0, 0}},
          "default10/X rule is retained when no axis participates");
    check(blockCoordinate(-1) == -1 && blockCoordinate(-65537) == -2, "ordinary negative fixed coordinates floor");
    check(blockCoordinate(-140737488420864ll) == 2147483646, "negative extreme compares reconstruction after signed32 narrowing");
    int queries = 0;
    world.hasZone = [&](int32_t, int32_t) {
        ++queries;
        return false;
    };
    check(visible(world, {0, 0, 0}, {655, 0, 0}, 200, Material::TypeOnly) && queries == 0,
          "near visibility succeeds without querying terrain");
    check(!visible(world, {0, 0, 0}, {656, 0, 0}, 200, Material::TypeOnly) && queries == 1,
          "the first farther axial fixed point takes the ray path");
    queries = 0;
    check(!visible(world, {0, 0, 0}, {7864320, 10485760, 1}, 200, Material::TypeOnly) && queries == 0,
          "double squared precheck rejects a fixed unit beyond the 120/160 boundary");
    check(!visible(world, {0, 0, 0}, {13107201, 0, 0}, 200, Material::TypeOnly) && queries == 0,
          "absolute-axis range rejects before querying terrain");
    // A loaded clear ray proves equality passes the separate double radius check.
    world.hasZone = [&](int32_t, int32_t) {
        ++queries;
        return true;
    };
    check(visible(world, {0, 0, 0}, {7864320, 10485760, 0}, 200, Material::TypeOnly) && queries > 0,
          "exact 120/160 radius equality reaches the full-length ray");
    check(visible(world, {0, 0, 0}, {13107200, 0, 0}, 200, Material::TypeOnly), "exact axial radius equality passes");
    world.block = [](int32_t, int32_t, int32_t) { return Cell{0, 0x40}; };
    check(visible(world, {16384, 32768, 32768}, {147456, 32768, 32768}, 200, Material::TypeOnly) &&
              !visible(world, {16384, 32768, 32768}, {147456, 32768, 32768}, 200, Material::HonorFlag40),
          "camera and ordinary creature LOS ignore the aim-only flag40 obstruction");
    world.block = [](int32_t x, int32_t, int32_t) { return Cell{uint8_t(x == 1), 0}; };
    check(visible(world, {655, 32768, 32768}, {66191, 32768, 32768}, 200, Material::TypeOnly) &&
              !visible(world, {656, 32768, 32768}, {66192, 32768, 32768}, 200, Material::TypeOnly),
          "full ray result is compared against rounded length minus epsilon");
    for (bool loaded : {false, true})
        for (float maximum : {0.0f, -0.0f}) {
            reads = 0;
            zones.clear();
            world.hasZone = [&](int32_t x, int32_t y) {
                zones.push_back({x, y});
                return loaded;
            };
            world.block = [&](int32_t, int32_t, int32_t) {
                ++reads;
                return Cell{1, 0x40};
            };
            const float result = trace(world, {-1, 64 * 65536 + 5, -65537}, {1, 0, 0}, maximum, Material::TypeOnly);
            check(result == 0 && !std::signbit(result) && reads == 0 && zones == std::vector<std::array<int32_t, 2>>{{-1, 1}},
                  "zero maximum resolves the initial zone then returns positive zero without a block read");
        }
    std::printf("Terrain ray: %s\n", failures ? "FAILED" : "passed");
    return failures;
}
