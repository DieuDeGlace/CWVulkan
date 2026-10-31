// Independent gameplay-camera §§5–6; derived finite references, no game assets.
#include "CameraPosition.h"
#include <cstdio>
#include <vector>

int cameraPositionTests() {
    using namespace cameraposition;
    constexpr int64_t BLOCK = 65536;
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) {
            std::printf("FAIL Camera position: %s\n", what);
            ++failures;
        }
    };
    terrainray::World world;
    world.hasZone = [](int32_t, int32_t) { return true; };
    std::vector<std::array<int32_t, 3>> queries;
    world.block = [&](int32_t x, int32_t y, int32_t z) {
        queries.push_back({x, y, z});
        return terrainray::Cell{0, 0};
    };
    Subject subject;
    subject.physical = {10 * BLOCK, 20 * BLOCK, 30 * BLOCK};
    subject.stepOffset = .1f;
    check(candidate(subject, world) == Point{10 * BLOCK, 20 * BLOCK, 2057831},
          "ordinary anchor separately truncates step offset and half-height plus .5");
    check(queries.size() == 10 && queries[0] == queries[1] && queries[0] == std::array<int32_t, 3>{10, 20, 31},
          "candidate starts with two effective reads of the same floored cell");
    subject.ability = 0x57;
    subject.size[0] = 1.2f;
    check(candidate(subject, world)[2] == 30 * BLOCK + 32768, "posture57 uses half width rather than body height");
    subject.ability = 0;
    subject.creationLayerShown = true;
    check(candidate(subject, world)[2] == 30 * BLOCK - 6553, "creation layer removes only the ordinary height offset");
    subject.ability = 0x78;
    check(candidate(subject, world)[2] == 2057831, "gliding still adds half-height while creation layer is shown");
    subject.creationLayerShown = false;
    subject.ability = 0x76;
    check(candidate(subject, world)[2] == 2057831 && subject.ability == 0,
          "missing mount cancels ability without restarting the already built candidate");
    subject.ability = 0x76;
    subject.mount = Mount{2, .1f, true};
    check(candidate(subject, world)[2] == 33 * BLOCK && subject.ability == 0x76, "living mount uses .75 height and race offset");
    subject.ability = 0x79;
    check(candidate(subject, world)[2] == 2057831 + 2 * BLOCK, "boat adds two fixed blocks after ordinary posture height");
    subject.ability = 0;
    subject.stepOffset = 0;
    queries.clear();
    world.block = [&](int32_t x, int32_t y, int32_t z) {
        queries.push_back({x, y, z});
        return terrainray::Cell{uint8_t(queries.size() == 3), 0};
    };
    check(candidate(subject, world) == subject.physical && queries.size() == 9 && queries[3] == std::array<int32_t, 3>{9, 20, 29},
          "failed candidate corner resets to raw P and breaks only its inner Z loop");
    int reads = 0;
    world.block = [&](int32_t, int32_t, int32_t) {
        ++reads;
        return terrainray::Cell{0, 0x40};
    };
    candidate(subject, world);
    check(reads == 9, "flag40 without80 stops the first probe without a duplicate read");
    reads = 0;
    world.block = [&](int32_t, int32_t, int32_t) {
        ++reads;
        return terrainray::Cell{0, 0xc0};
    };
    candidate(subject, world);
    check(reads == 10, "flag80 keeps the duplicate initial probe despite flag40");

    Subject origin;
    State state;
    const Point oneBlock{BLOCK, BLOCK, BLOCK};
    follow(state, origin, oneBlock, .01);
    check(state.lookAt == Point{32, 32, 4}, "ordinary XY quantized weight differs from direct float Z smoothing");
    state = {};
    origin.velocity[0] = 1;
    follow(state, origin, oneBlock, .01);
    check(state.lookAt == Point{447, 447, 4}, "physical velocity changes only the ordinary XY coefficient");
    state = {};
    origin.ability = 0x98;
    follow(state, origin, oneBlock, .01);
    check(state.lookAt == Point{327, 327, 327}, "ability98 uses float .25 factor and quantized fixed weight");
    state = {};
    origin.ability = 0x79;
    follow(state, origin, oneBlock, .01);
    check(state.lookAt == Point{32, 32, 32}, "boat uses its distinct double-return factor and rate/100 multiplier");
    origin.ability = 0;
    state.lookAt = {50 * BLOCK, 0, 0};
    follow(state, origin, oneBlock, 0);
    check(state.lookAt == Point{50 * BLOCK, 0, 0}, "exact fifty-block anchor separation does not snap");
    ++state.lookAt[0];
    follow(state, origin, oneBlock, 0);
    check(state.lookAt == origin.physical, "anchor beyond fifty blocks snaps to raw P without a head offset");
    origin.ability = 0x9d;
    follow(state, origin, oneBlock, 16);
    check(state.lookAt == Point{0, 0, BLOCK}, "eagle9d anchor is raw P plus one block");

    int zones = 0;
    reads = 0;
    world.hasZone = [&](int32_t, int32_t) {
        ++zones;
        return true;
    };
    world.block = [&](int32_t, int32_t, int32_t) {
        ++reads;
        return terrainray::Cell{1, 0};
    };
    state = {};
    update(state, origin, {1, 0, 0}, 16, world);
    check(state.distance == 0 && state.eye == origin.physical && state.lookAt == origin.physical,
          "final solid eye resets eye/anchor to raw P and shortens current distance to zero");
    check(zones == 16 && reads == 15, "all eight guards and eight shortening calls run, including seven zero-limit calls");
    origin.ability = 0x9c;
    zones = reads = 0;
    state = {};
    update(state, origin, {1, 0, 0}, 16, world);
    check(zones == 0 && reads == 5 && state.eye == Point{-8 * BLOCK, 0, 0} && state.distance == 8,
          "eagle9c skips eye guards/final obstruction after ordinary candidate probes");
    origin.ability = 0x9d;
    world.block = [](int32_t x, int32_t, int32_t) { return terrainray::Cell{uint8_t(x < -2), 0}; };
    state = {};
    update(state, origin, {1, 0, 0}, 16, world);
    check(state.distance == 1.75f && state.eye == Point{-114688, 0, BLOCK},
          "shortening retains the tightest quarter-corner distance without an extra safety skin");
    world.block = [](int32_t, int32_t, int32_t) { return terrainray::Cell{2, 0x40}; };
    state = {};
    update(state, origin, {1, 0, 0}, 16, world);
    check(state.distance == 8 && state.eye == Point{-8 * BLOCK, 0, BLOCK}, "camera ignores water/flag40 aim-only obstruction");
    std::printf("Camera position: %s\n", failures ? "FAILED" : "passed");
    return failures;
}
