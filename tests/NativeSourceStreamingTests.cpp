#include "AlphaLook.h"
#include "Assets.h"
#include "Extracted.h"
#include "World.h"
#include "SteamZoneDemand.h"
#include "NativeKeyOrder.h"
#include "NativeSourceMap.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <future>
#include <thread>
#include <vector>

namespace {
void sourceMapExamples(const auto &check) {
    using Map = NativeSourceMap<int>;
    using Key = Map::Key;
    const std::pair<Key,uint64_t> hashes[] = {
        {{0,0},0xa8c7f832281a39c5ULL}, {{1,0},0x89cd31291d2aefa4ULL},
        {{8,0},0xa09e307a7f948acdULL}, {{0,1},0x08cd4c29d1e47d34ULL},
        {{-1,0},0x4ebaad864bc6f2e1ULL}, {{64,0},0x6779ba74e3ecc205ULL},
        {{2,0},0xe6bd86443df8ce07ULL}, {{0,2},0xe8bd5042d485b2e7ULL}
    };
    for (const auto &[key,expected] : hashes) check(Map::hash(key) == expected, "independent eight-byte source hash vector");
    Map::Entries entries;
    Map map(entries);
    auto order = [&] {
        std::vector<Key> result;
        for (const auto &entry : entries) result.push_back(entry.first);
        return result;
    };
    const Key a{0,0}, b{1,0}, c{8,0}, d{0,1};
    for (Key key : {a,b,c,d}) map.insert(key,1);
    check(order() == std::vector<Key>{c,a,d,b}, "empty-bucket tail and occupied-bucket front insertion");
    map.find(a); map.find(d);
    const auto existing = map.insert(c,99);
    map.find(a)->second = 2;
    check(!existing.second && existing.first->second == 1 && order() == std::vector<Key>{c,a,d,b},
          "read, existing-key admission and payload replacement preserve traversal");
    map.erase(map.find(d));
    check(order() == std::vector<Key>{c,a,b}, "first-entry erasure advances its bucket endpoint");
    map.erase(map.find(a));
    check(order() == std::vector<Key>{c,b}, "last-entry erasure backs up its bucket endpoint");
    map.erase(map.find(c));
    map.insert(a,1); map.insert(c,1);
    check(order() == std::vector<Key>{b,c,a}, "emptied bucket rejoins at tail then receives its collision at front");
    map.find(a)->second = 100;
    map.find(b)->second = 200;
    map.insert(d,400);
    int around[3][3]{};
    map.neighborhood({0,0},around);
    check(around[1][1] == 100 && around[1][2] == 200 && around[2][1] == 400 && around[0][0] == 0,
          "one neighborhood capture preserves XY orientation and missing-source values");
    {
        Map::Entries grown;
        Map growth(grown);
        for (int x : {0,1,8,64,2,3,4,5,6}) growth.insert({x,0},x);
        std::vector<Key> actual;
        for (const auto &entry : grown) actual.push_back(entry.first);
        check(growth.bucketCount() == 64 && actual == std::vector<Key>{{0,0},{64,0},{8,0},{1,0},{2,0},{3,0},{4,0},{5,0},{6,0}},
              "ninth admission rehashes existing nodes in the independent native traversal");
        const auto before = actual;
        growth.find({64,0});
        actual.clear(); for (const auto &entry : grown) actual.push_back(entry.first);
        check(actual == before, "read after growth preserves the rebuilt traversal");
    }
    {
        Map::Entries grown;
        Map growth(grown);
        bool boundaries = true;
        for (int x = 0; x < 513; ++x) {
            growth.insert({x,0},x);
            const size_t expected = x < 8 ? 8 : x < 64 ? 64 : x < 512 ? 512 : 1024;
            boundaries &= growth.bucketCount() == expected && growth.find({x,0})->second == x;
        }
        for (int x = 0; x < 513; ++x) {
            const auto found = growth.find({x,0});
            boundaries &= found != grown.end() && found->second == x;
        }
        check(boundaries, "binary32 unit-load growth follows 8,64,512,1024 buckets at the literal boundaries");
        while (!grown.empty()) growth.erase(grown.begin());
        check(growth.bucketCount() == 1024 && growth.find({0,0}) == grown.end(), "entry erasure empties endpoints without shrinking buckets");
    }
}
void priorities(const auto &check) {
    using Point = zonedemand::BlockPoint;
    struct Case { Point zone, point; int32_t expected; };
    const Case cases[] = {
        {{-1,-1},{0,0},0}, {{-1,0},{0,0},0}, {{0,-1},{0,0},0}, {{0,0},{0,0},0},
        {{1,0},{0,0},4096}, {{1,1},{0,0},8192}, {{0,0},{63,63},0}, {{1,0},{63,63},1},
        {{1,1},{63,63},2}, {{-1,0},{63,63},3969}, {{-1,-1},{63,63},7938},
        {{-1,1},{-65,127},1}, {{-1,1},{-64,128},0}
    };
    for (const auto &value : cases)
        check(zonedemand::generationPriority(value.zone, value.point) == value.expected,
              "literal whole-block closed-rectangle generation priority");
    const std::array<int64_t,3> fractional{-32768, -64LL*65536-32768, 0};
    check(zonedemand::sourcePoint(fractional) == Point{0,-64} &&
              zonedemand::preparationCenter(fractional) == Point{-1,-2},
          "source truncation and mesh-retention floors stay distinct for negative fractions");
    struct Candidate { Point zone; int32_t priority; };
    std::vector<Candidate> candidates;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            candidates.push_back({{x,y},zonedemand::generationPriority({x,y},{0,0})});
    keyorder::sort(candidates, [](const Candidate &value) { return value.priority; });
    const Point expected[] = {{-1,-1},{-1,0},{0,-1},{0,0}};
    bool stable = true;
    for (size_t i = 0; i < 4; ++i) stable &= candidates[i].zone == expected[i] && candidates[i].priority == 0;
    check(stable, "small complete candidate vector preserves native tied prefix before four-entry truncation");
}
bool finish(World &world) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
    do {
        world.update({0, 0, 166});
        if (!world.preparingTerrain() && world.zoneCount() == 1) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
}
}
int nativeSourceStreamingTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *message) {
        if (!pass) { ++failures; std::printf("FAIL source streaming: %s\n", message); }
    };
    priorities(check);
    sourceMapExamples(check);
    if (worldgen::modHooksSet() || worldgen::usingAlphaTerrain()) {
        check(false, "fixture requires the default independent Steam generator");
        return failures;
    }
    {
        ExtractedWorld tied;
        tied.openGenerated(0, 0);
        tied.setClientStreaming(true);
        tied.loadSourceZone(2, 0);
        tied.loadSourceZone(0, 2);
        tied.loadSourceZone(2, 0); // Existing source hit must preserve the native map node's position.
        check(tied.retireClientSource({0, 0}, 1) == std::pair<int, int>{0, 2},
              "serialized source installation/read history gives the native tied retirement choice");
        tied.setClientStreaming(false);
    }
    const auto previous = renderStyle();
    setRenderStyle(RenderStyle::Steam);
    worldgen::forgetZones();
    ExtractedWorld blocks;
    blocks.openGenerated(0, 0);
    Renderer renderer;
    RenderSettings settings;
    settings.style = RenderStyle::Steam;
    renderer.initHeadless(settings);
    Assets assets;
    TerrainSettings options;
    options.lilies = false;
    Terrain terrain(options);
    std::promise<void> entered, release;
    const auto enteredFuture = entered.get_future();
    const auto released = release.get_future().share();
    std::atomic<bool> claimed = false;
    const auto caller = std::this_thread::get_id();
    worldgen::ModHooks hooks;
    hooks.placeKind = [&](int32_t, int32_t, bool, int kind) {
        if (std::this_thread::get_id() != caller && !claimed.exchange(true)) {
            entered.set_value();
            released.wait();
        }
        return kind;
    };
    worldgen::setModHooks(hooks);
    std::optional<ExtractedWorld::ZoneToken> boundaryLease;
    std::vector<std::optional<ExtractedWorld::ZoneToken>> retiredLeases;
    {
        World world(terrain, assets, renderer, 3, &blocks);
        world.setSourceRenderDistance(0); // Nine demanded source zones; only the center has all nine neighbors.
        world.update({0, 0, 166});
        check(enteredFuture.wait_for(std::chrono::seconds(10)) == std::future_status::ready,
              "controlled background generation reaches the source gate");
        check(world.pendingCount() == 0,
              "mesh selection waits for loaded source neighbors instead of generating them itself");
        check(world.pendingSourceCount() == 4, "source generation has a four-coordinate batch independent of mesh jobs");
        for (int i = 0; i < 8; ++i) world.update({0, 0, 166});
        check(world.pendingSourceCount() == 4 && world.pendingCount() == 0,
              "a held source batch admits neither another source batch nor a partial neighborhood mesh");
        bool loaded = true;
        check(blocks.gameBlock(64000, 0, 1000, &loaded) == ExtractedWorld::GameBlock{200, 200, 200, 0, 1, 0} &&
                  !loaded && !blocks.gameZoneLoaded(1000, 0), "a missing block lookup returns the solid fallback without loading source");
        release.set_value();
        check(finish(world), "separate source generation completes before settled terrain is reported");
        int demanded = 0;
        for (int x = -1; x <= 1; ++x)
            for (int y = -1; y <= 1; ++y) demanded += blocks.gameZoneLoaded(x, y);
        check(demanded == 9 && !blocks.gameZoneLoaded(2, 0), "source demand fills its own region without padding neighbors");
        blocks.propsAbsolute(4, 0); // Retained boundary, outside the generation region.
        boundaryLease = blocks.gameZoneToken(4, 0);
        for (int x = 20; x < 29; ++x) {
            blocks.propsAbsolute(x, 0);
            retiredLeases.push_back(blocks.gameZoneToken(x, 0));
            blocks.gameBlock(int64_t(x) * 64, 0, 1000); // Populate the native block-query memo before retirement.
        }
        const std::array<int32_t,3> edited{20*64,0,1000};
        const auto originalBlock = blocks.gameBlock(edited[0],edited[1],edited[2]);
        const ExtractedWorld::GameBlock firstEdit{11,22,33,0,6,0}, finalEdit{44,55,66,0,6,0};
        auto change = [&](const auto &was, const auto &now) {
            nativesaves::BlockChange result;
            std::memcpy(result.bytes.data(),edited.data(),12);
            std::memcpy(result.bytes.data()+12,was.data(),6);
            std::memcpy(result.bytes.data()+18,now.data(),6);
            return result;
        };
        blocks.setBlockChanges(20,0,{change(originalBlock,firstEdit),change(firstEdit,finalEdit)});
        check(blocks.gameBlock(edited[0],edited[1],edited[2]) == finalEdit, "local ordered block journal applies through the normal loaded setter");
        world.update({0, 0, 166});
        int remaining = 0;
        for (int x = 20; x < 29; ++x) remaining += blocks.gameZoneLoaded(x, 0);
        check(remaining == 1, "one native mesh cycle retires at most eight loaded source zones");
        bool fallback = true;
        for (int x = 20; x < 29; ++x)
            if (!blocks.gameZoneLoaded(x, 0)) {
                loaded = true;
                fallback &= blocks.gameBlock(int64_t(x) * 64, 0, 1000, &loaded) ==
                    ExtractedWorld::GameBlock{200, 200, 200, 0, 1, 0} && !loaded && retiredLeases[size_t(x - 20)] && bool(retiredLeases[size_t(x - 20)]->keepAlive);
            }
        check(fallback, "retirement invalidates memoized block reads even while old snapshots remain owned");
        check(blocks.gameZoneLoaded(4, 0), "inclusive retained boundary does not imply fresh generation demand");
        world.update({0, 0, 166});
        remaining = 0;
        for (int x = 20; x < 29; ++x) remaining += blocks.gameZoneLoaded(x, 0);
        check(remaining == 0 && blocks.gameZoneLoaded(4, 0), "later cycles finish outside retirement while preserving the boundary");
        check(!blocks.gameZoneLoaded(-4, 0) && !blocks.gameZoneLoaded(0, 4), "retained radius does not prefetch missing boundary zones");
        blocks.loadSourceZone(20,0);
        check(blocks.gameBlock(edited[0],edited[1],edited[2]) == finalEdit,
              "source retirement replays the local block journal in order after fresh procedural generation");
        blocks.undoBlockChanges(20,0);
        world.update({0,0,166});
        blocks.loadSourceZone(20,0);
        check(blocks.gameBlock(edited[0],edited[1],edited[2]) == originalBlock,
              "an emptied local edit journal does not revive earlier changes after another source reload");
        world.update({0,0,166});
        blocks.loadSourceZone(1000, 0);
        blocks.loadSourceZone(8, 8);
        check(blocks.retireClientSource({0, 0}, 4) == std::pair<int, int>{8, 8} && blocks.gameZoneLoaded(1000, 0),
              "native far-distance clamp can rank an unclamped diagonal above a much farther zone");
        int samples = 0;
        world.setScenePlayerProvider([&] {
            scenerange::Player player;
            if (++samples > 1) player.physical[0] = 64000LL * 65536;
            return std::optional(player);
        });
        world.update({0, 0, 166});
        check(samples >= 9 && blocks.loadedSourceCount() == 3,
              "each of eight source retirement calls resamples the physical center independently");
        world.setScenePlayerProvider({});
    }
    check(blocks.loadedSourceCount() == 0 && boundaryLease && boundaryLease->keepAlive,
          "client shutdown clears residency while a retained snapshot survives");
    bool loaded = true;
    blocks.gameBlock(4 * 64, 0, 1000, &loaded);
    check(!loaded, "shutdown invalidates memoized membership without destroying externally held storage");
    worldgen::setModHooks({});
    {
        ExtractedWorld hostBlocks;
        hostBlocks.openGenerated(0,0);
        {
            // A dedicated server owns a World facade but never calls its mesh update.
            World hostOnly(terrain,assets,renderer,0,&hostBlocks,false);
            hostBlocks.keepAround(0,0,0);
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
            while (!hostBlocks.gameZoneLoaded(0,0) && std::chrono::steady_clock::now() < deadline)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            check(hostBlocks.gameZoneLoaded(0,0) && !hostOnly.preparingTerrain(),
                  "host-only terrain admission remains independent of the unused client renderer");
        }
        check(hostBlocks.gameZoneLoaded(0,0), "destroying a host-only renderer does not clear the host source owner");
    }
    {
        setRenderStyle(RenderStyle::Alpha);
        ExtractedWorld alpha;
        alpha.openGenerated(0, 0);
        World world(terrain, assets, renderer, 0, &alpha);
        world.update({0, 0, 166});
        check(world.pendingCount() == 1, "Alpha retains its existing immediate asynchronous terrain route");
        check(finish(world), "Alpha terrain still settles independently");
    }
    renderer.shutdown();
    setRenderStyle(previous);
    worldgen::forgetZones();
    std::printf("Native source streaming: %d failures\n", failures);
    return failures;
}
