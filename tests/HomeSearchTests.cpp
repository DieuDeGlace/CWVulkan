#include "Frontend.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {
frontend::StartSpotWorld flatWorld() {
    frontend::StartSpotWorld world;
    world.areaCell = [](int32_t, int32_t) { return nativesaves::Cell{0, 0}; };
    world.cellObject = [](int32_t, int32_t) { return 53; };
    world.region = [](int32_t, int32_t) { return nativesaves::Cell{0, 0}; };
    world.regionType = [](int32_t, int32_t) { return 2; };
    world.neighbours = [](int32_t, int32_t) { return worldgen::Neighbourhood{}; };
    world.place = [](int32_t, int32_t) {
        worldgen::Place p;
        p.kind = 5;
        return p;
    };
    world.weight = [](homesearch::Weight, const worldgen::Neighbourhood &, const worldgen::Place &) { return 0.f; };
    world.roads = [](int32_t, int32_t, const worldgen::Place &) { return 0.f; };
    world.channel = [](homesearch::Channel, int32_t, int32_t, const homesearch::Terrain &) { return 0.f; };
    world.excludedPlace = [](int32_t, int32_t) { return nativesaves::Cell{INT32_MAX, INT32_MAX}; };
    world.ground = [](int32_t, int32_t) { return 10.f; };
    return world;
}

struct QueryFixture {
    frontend::StartSpotWorld world = flatWorld();
    std::vector<std::string> calls;
    bool operands = true;
    int object = 53, type = 7, kind = 5;
    std::array<float, 5> weights{0, 0, 0, 0, .7f};
    std::array<float, 3> channels{.4f, .4f, .4f};
    std::array<float, 5> heights{0, -3, -3, -3, -3};
    nativesaves::Cell excluded{INT32_MAX, INT32_MAX};
    static constexpr nativesaves::Cell HOME{-65, -3}, REGIONAL{17, -19};
    static constexpr std::array<nativesaves::Cell, 5> POINTS{
        {{-1032, -8}, {-1040, -8}, {-1024, -8}, {-1032, -16}, {-1032, 0}}};
    QueryFixture() {
        world.cellObject = [this](int32_t x, int32_t y) {
            calls.push_back("object");
            operands &= nativesaves::Cell{x, y} == HOME;
            return object;
        };
        world.ground = [this](int32_t x, int32_t y) {
            const auto found = std::find(POINTS.begin(), POINTS.end(), nativesaves::Cell{x, y});
            operands &= found != POINTS.end();
            if (found == POINTS.end())
                return 0.f;
            const size_t index = size_t(found - POINTS.begin());
            calls.push_back("h" + std::to_string(index));
            return heights[index];
        };
        world.region = [this](int32_t x, int32_t y) {
            calls.push_back("region");
            operands &= nativesaves::Cell{x, y} == POINTS[0];
            return REGIONAL;
        };
        world.regionType = [this](int32_t x, int32_t y) {
            calls.push_back("type");
            operands &= nativesaves::Cell{x, y} == REGIONAL;
            return type;
        };
        world.neighbours = [this](int32_t x, int32_t y) {
            calls.push_back("summary");
            operands &= nativesaves::Cell{x, y} == POINTS[0];
            worldgen::Neighbourhood n{};
            n.x = x;
            n.y = y;
            n.nearestBiome = 73;
            return n;
        };
        world.place = [this](int32_t x, int32_t y) {
            calls.push_back("place");
            operands &= nativesaves::Cell{x, y} == POINTS[0];
            worldgen::Place p;
            p.kind = kind;
            p.big = true;
            p.variant = 4;
            p.radius = 500;
            p.point = {999, 888};
            p.weight = .75f;
            return p;
        };
        world.weight = [this](homesearch::Weight which, const worldgen::Neighbourhood &n, const worldgen::Place &p) {
            static constexpr const char *names[]{"A", "B", "C7", "D", "W8"};
            const size_t index = size_t(which);
            calls.push_back(names[index]);
            operands &= n.x == -1032 && n.y == -8 && n.nearestBiome == 73 && p.kind == 5 && p.big && p.weight == .75f;
            return weights[index];
        };
        world.roads = [this](int32_t x, int32_t y, const worldgen::Place &p) {
            calls.push_back("q");
            operands &= nativesaves::Cell{x, y} == POINTS[0] && p.kind == 5 && p.weight == .75f;
            return .9f;
        };
        world.channel = [this](homesearch::Channel which, int32_t x, int32_t y, const homesearch::Terrain &t) {
            static constexpr const char *names[]{"F", "G", "H"};
            const size_t index = size_t(which);
            calls.push_back(names[index]);
            operands &= nativesaves::Cell{x, y} == POINTS[0] && t.place.point == worldgen::Int2{999, 888} &&
                        t.place.weight == .75f && t.lavaRegion == 0 && t.forestRegion == 0 && t.lake == 0 &&
                        t.dryRegion == 0 && t.wet == .7f && t.roads == .9f;
            return channels[index];
        };
        world.excludedPlace = [this](int32_t x, int32_t y) {
            calls.push_back("K");
            operands &= nativesaves::Cell{x, y} == REGIONAL;
            return excluded;
        };
    }
};
const std::vector<std::string> FULL_ORDER{"object", "h0", "region", "type", "summary", "place", "A",  "B",  "C7", "D",
                                          "W8",     "q",  "F",      "G",    "H",       "h1",    "h2", "h3", "h4", "K"};
void orderedAdmission(const auto &check) {
    QueryFixture full;
    check(frontend::startCellFits(full.world, QueryFixture::HOME) && full.operands && full.calls == FULL_ORDER,
          "all lazy calls retain source order/coordinates/cached operands; wet and q are not direct thresholds and "
          "equality0.4 passes");
    const auto rejection = [&](QueryFixture &fixture, const char *last) {
        const auto end = std::find(FULL_ORDER.begin(), FULL_ORDER.end(), last);
        check(!frontend::startCellFits(fixture.world, QueryFixture::HOME) && fixture.operands &&
                  end != FULL_ORDER.end() && fixture.calls == std::vector<std::string>(FULL_ORDER.begin(), end + 1),
              "a rejected gate performs no later summary/field/height/exclusion work");
    };
    {
        QueryFixture f;
        f.object = 52;
        rejection(f, "object");
    }
    {
        QueryFixture f;
        f.heights[0] = -.01f;
        rejection(f, "h0");
    }
    {
        QueryFixture f;
        f.type = 9;
        rejection(f, "type");
    }
    {
        QueryFixture f;
        f.kind = 0;
        rejection(f, "place");
    }
    static constexpr const char *weightEnd[]{"A", "B", "C7", "D"};
    for (size_t index = 0; index < 4; ++index) {
        QueryFixture f;
        f.weights[index] = .01f;
        rejection(f, weightEnd[index]);
    }
    static constexpr const char *channelEnd[]{"F", "G", "H"};
    for (size_t index = 0; index < 3; ++index) {
        QueryFixture f;
        f.channels[index] = std::nextafter(.4f, 1.f);
        rejection(f, channelEnd[index]);
    }
    {
        QueryFixture f;
        f.heights[1] = 0;
        f.heights[2] = 5;
        rejection(f, "h2");
    }
    {
        QueryFixture f;
        f.heights[3] = 0;
        f.heights[4] = 5;
        rejection(f, "h4");
    }
    {
        QueryFixture f;
        f.excluded = {-2, -2};
        rejection(f, "K");
    }
    {
        QueryFixture f;
        f.heights = {0, 0, 4, -10, -6};
        check(frontend::startCellFits(f.world, QueryFixture::HOME),
              "opposite-height equality4 and negative neighbors are admitted independently of center nonnegativity");
    }
    for (int type : {0, 2, 7}) {
        QueryFixture f;
        f.type = type;
        check(frontend::startCellFits(f.world, QueryFixture::HOME), "all three literal regional codes remain admitted");
    }
    {
        QueryFixture f;
        f.excluded = {-3, -2};
        check(frontend::startCellFits(f.world, QueryFixture::HOME), "different exclusion X returns the original home");
    }
    {
        QueryFixture f;
        f.excluded = {-2, -3};
        check(frontend::startCellFits(f.world, QueryFixture::HOME), "equal exclusion X and different Y still admit");
    }
    {
        QueryFixture f;
        f.world.playable = [](auto) { return false; };
        check(
            !frontend::startCellFits(f.world, QueryFixture::HOME) && f.calls.empty(),
            "explicit captured/Alpha coverage can reject before native work; native factory has no coverage override");
    }
}
void coordinateBoundaries(const auto &check) {
    struct Boundary {
        nativesaves::Cell home, key;
        bool big;
    };
    for (const Boundary entry : {Boundary{{-1, -3}, {-1, -1}, false},
                                 {{-1, -3}, {-2, -2}, true},
                                 {{-65, -67}, {-2, -2}, false},
                                 {{-129, -131}, {-4, -4}, true},
                                 {{INT32_MAX, INT32_MIN}, {-2, 0}, true}}) {
        auto world = flatWorld();
        world.place = [&](int32_t, int32_t) {
            worldgen::Place p;
            p.kind = 5;
            p.big = entry.big;
            return p;
        };
        world.excludedPlace = [&](int32_t, int32_t) { return entry.key; };
        check(!frontend::startCellFits(world, entry.home),
              "final cells use signed block floor and conditional downward-even rounding, including low32 wrap");
    }
    auto world = flatWorld();
    int draws = 0;
    bool outputPublished = true;
    nativesaves::Cell out{91, 93};
    std::vector<int32_t> visited;
    world.areaCell = [&](int32_t x, int32_t) {
        visited.push_back(x);
        return visited.size() == 1 ? nativesaves::Cell{13, 17} : nativesaves::Cell{19, 23};
    };
    world.cellObject = [&](int32_t x, int32_t y) {
        outputPublished &= out == nativesaves::Cell{x, y};
        return x == 13 ? 0 : 53;
    };
    check(frontend::findStartCell(
              world,
              [&] {
                  ++draws;
                  return 0;
              },
              {INT32_MAX, 0}, 0, 0, 2, out) &&
              outputPublished && draws == 2 && visited == std::vector<int32_t>{INT32_MIN, INT32_MIN + 1} &&
              out == nativesaves::Cell{19, 23},
          "coarse increment wraps low32, both prefix draws occur, and every candidate is published before its queries");
    draws = 0;
    visited.clear();
    out = {91, 93};
    check(!frontend::findStartCell(
              world,
              [&] {
                  ++draws;
                  return 0;
              },
              {INT32_MAX, 0}, 0, 0, 1, out) &&
              draws == 2 && out == nativesaves::Cell{13, 17},
          "the explicit finite abort bound retains the last attempted candidate without inventing a native completion "
          "result");
    world.excludedPlace = {};
    draws = 0;
    check(!frontend::findStartCell(
              world,
              [&] {
                  ++draws;
                  return 0;
              },
              {0, 0}, 100000, 100000, 1, out) &&
              draws == 0,
          "incomplete engine query bindings return safely before consuming the search provider");
    const auto precise = homesearch::position({9000001, -9000003}, 1.1f);
    check(precise == std::array<int64_t, 3>{int64_t(144000024) * 65536, int64_t(-144000016) * 65536, 72089},
          "native placement retains large integerXY and the rounded float multiply before truncating Z");
    check(homesearch::position({INT32_MAX, INT32_MAX}, -1.1f) == std::array<int64_t, 3>{-8 * 65536, 16 * 65536, -72089},
          "placement keeps low32 home arithmetic, sign extension and negative truncation without voxel/feet offsets");
}
void nativeBindings(const auto &check) {
    const auto native = homesearch::nativeWorld();
    check(native.ready() && !native.playable,
          "ordinary native bindings are complete without an extension coverage gate");
    const std::array<int, 6> types{13, 12, 5, 7, 9, 8};
    for (int type : types) {
        worldgen::Neighbourhood n{};
        n.nearestDist2 = 0;
        n.nearestBiome = type;
        for (int x = 0; x < 3; ++x)
            for (int y = 0; y < 3; ++y) {
                n.biome[x][y] = type;
                n.dist2[x][y] = 0;
            }
        worldgen::Place p;
        p.kind = 5;
        p.weight = .75f;
        const std::array<float, 5> expected{type == 13 ? 1.f : 0.f, type == 12 || type == 5 ? 1.f : 0.f,
                                            type == 7 ? 1.f : 0.f, type == 9 || type == 13 ? 1.f : 0.f,
                                            type == 8 ? 1.f : 0.f};
        for (size_t index = 0; index < expected.size(); ++index)
            check(native.weight(homesearch::Weight(index), n, p) == expected[index],
                  "native binding maps each nonpositive/wet weight to its independently specified integer-code set");
    }
}

} // namespace
int homeSearchTests() {
    int failures = 0;
    const auto check = [&](bool okay, const char *message) {
        if (!okay) {
            ++failures;
            std::printf("FAIL home search: %s\n", message);
            std::fflush(stdout);
        }
    };
    auto world = flatWorld();
    std::vector<nativesaves::Cell> coarse;
    world.areaCell = [&](int32_t x, int32_t y) {
        coarse.push_back({x, y});
        return nativesaves::Cell{0, 0};
    };
    int draws = 0;
    auto random = [&] {
        ++draws;
        return 0;
    };
    nativesaves::Cell found;
    check(frontend::findStartCell(world, random, {0, 0}, 100000, 100000, 1, found) &&
              coarse == std::vector<nativesaves::Cell>{{100001, 0}} && draws == 2,
          "polar coordinates are coarse cells and increment before the first direct candidate query");
    coarse.clear();
    int candidates = 0;
    world.cellObject = [&](int32_t, int32_t) { return ++candidates == 2 ? 53 : 0; };
    check(frontend::findStartCell(world, random, {0, 0}, 100000, 100000, 2, found) &&
              coarse == std::vector<nativesaves::Cell>{{100001, 0}, {100002, 0}},
          "each rejection advances one coarse cell without a sixty-four-step area cache");
    world.cellObject = [](int32_t, int32_t) { return 53; };
    coarse.clear();
    draws = 0;
    auto negativeRadius = [&] { return ++draws == 1 ? 0 : -1; };
    check(frontend::findStartCell(world, negativeRadius, {0, 0}, 100000, 100000, 1, found) &&
              coarse == std::vector<nativesaves::Cell>{{100000, 0}} && draws == 2,
          "the second signed draw uses signed remainder before radius addition and preincrement");
    world = flatWorld();
    std::vector<nativesaves::Cell> samples;
    world.ground = [&](int32_t x, int32_t y) {
        samples.push_back({x, y});
        return x == 8 && y == 40 ? 100.f : 10.f;
    };
    check(frontend::startCellFits(world, {0, 0}),
          "flatness compares opposite neighbors rather than the high center to a positive offset");
    check(samples == std::vector<nativesaves::Cell>{{8, 40}, {0, 40}, {16, 40}, {8, 32}, {8, 48}},
          "height calls sample center then negative/positive eight in X then Y");
    world = flatWorld();
    world.ground = [](int32_t x, int32_t) { return x == 0 ? 0.f : x == 16 ? 5.f : 10.f; };
    check(!frontend::startCellFits(world, {0, 0}),
          "a five-block difference across the X pair rejects even when center and plus16 match");
    world = flatWorld();
    world.ground = [](int32_t, int32_t y) { return y == 32 ? 0.f : y == 48 ? 5.f : 10.f; };
    check(!frontend::startCellFits(world, {0, 0}), "a five-block difference across the Y pair rejects independently");
    const auto native = homesearch::nativeWorld();
    check(native.areaCell(0, 0) == nativesaves::Cell{22, 47}, "independent unsigned coordinate vector: coarse 0,0");
    check(native.areaCell(-1, -1) == nativesaves::Cell{-16, -52},
          "independent unsigned coordinate vector: coarse -1,-1");
    check(native.areaCell(100001, 0) == nativesaves::Cell{6400081, 50},
          "independent unsigned coordinate vector: coarse 100001,0");
    check(native.areaCell(123456, -98765) == nativesaves::Cell{7901234, -6320948},
          "independent unsigned coordinate vector: coarse 123456,-98765");
    check(native.areaCell(INT32_MAX, INT32_MIN) == nativesaves::Cell{-63, 38},
          "independent unsigned coordinate vector: coarse 2147483647,-2147483648");
    check(native.areaCell(11696, 0) == nativesaves::Cell{748548, 9},
          "independent unsigned coordinate vector: coarse 11696,0");
    orderedAdmission(check);
    coordinateBoundaries(check);
    nativeBindings(check);
    std::printf("Home search: progression, admission and placement: %d failure(s)\n", failures);
    return failures;
}
