// Owned cache policy: reuse immutable values still held by a terrain consumer.
#include "worldgen/Generator.h"
#include <array>
#include <barrier>
#include <cstdio>
#include <future>
#include <vector>

int generatorCacheTests() {
    int failures = 0;
    auto check = [&](bool pass, const char *what) {
        if (!pass) { std::printf("FAIL generator cache: %s\n", what); ++failures; }
    };
    const auto seed = worldgen::currentWorld().seed;
    const auto day = worldgen::worldDay();
    const auto scale = worldgen::worldScale();
    worldgen::setWorldSeed(123);
    worldgen::forgetZones();
    auto retained = worldgen::zone(0, 0);
    std::weak_ptr<const worldgen::ZoneContents> live = retained;
    std::weak_ptr<const worldgen::ZoneContents> unowned = worldgen::zone(1, 0);
    // Exceed the ready LRU without retaining the intervening results.
    for (int x = 100; x < 200; ++x) worldgen::zone(x, 0);
    check(unowned.expired(), "ready eviction releases terrain without a consumer");
    {
        std::barrier begin(4);
        std::array<std::future<std::shared_ptr<const worldgen::ZoneContents>>, 4> readers;
        for (auto &reader : readers)
            reader = std::async(std::launch::async, [&] {
                begin.arrive_and_wait();
                return worldgen::zone(0, 0);
            });
        bool reused = true;
        for (auto &reader : readers) reused &= reader.get() == retained;
        check(reused, "concurrent readers reuse a live generation after ready eviction");
    }
    // The weak lookup must not turn externally held terrain into a permanent keep.
    for (int x = 200; x < 300; ++x) worldgen::zone(x, 0);
    retained.reset();
    check(live.expired(), "reused terrain is released after ready eviction and its last consumer");

    {
        std::vector<std::shared_ptr<const worldgen::ZoneContents>> consumers;
        for (int x = 800; x < 900; ++x) consumers.push_back(worldgen::zone(x, 0));
        std::weak_ptr<const worldgen::ZoneContents> working = worldgen::zone(2000, 0);
        bool same = true;
        for (int x = 800; x < 900; ++x) same &= worldgen::zone(x, 0) == consumers[size_t(x - 800)];
        check(same && !working.expired(),
              "recovering already-owned results preserves unowned ready inputs for neighboring consumers");
    }

    const auto beforeFlush = worldgen::zone(0, 0);
    worldgen::forgetZones();
    const auto afterFlush = worldgen::zone(0, 0);
    check(beforeFlush != afterFlush && beforeFlush->terrain.columns[0].blocks == afterFlush->terrain.columns[0].blocks,
          "flush retires live generations and preserves deterministic terrain");
    worldgen::setWorldDay(day + 1);
    const auto afterDay = worldgen::zone(0, 0);
    check(afterDay != afterFlush, "world settings prevent live-generation reuse");
    worldgen::setWorldScale(scale);
    const auto afterOptions = worldgen::zone(0, 0);
    check(afterOptions != afterDay, "option epochs prevent live-generation reuse");
    worldgen::ModHooks hooks;
    hooks.generatedItem = [](int, int) { return true; };
    worldgen::setModHooks(hooks);
    const auto hooked = worldgen::zone(0, 0);
    {
        worldgen::OriginalGeneration original;
        check(worldgen::zone(0, 0) != hooked, "original generation has a separate immutable result");
    }
    check(worldgen::zone(0, 0) == hooked, "original scope does not replace the hooked generation");
    worldgen::setModHooks({});
    worldgen::setWorldDay(day);
    worldgen::setWorldSeed(seed);
    worldgen::forgetZones();
    std::printf("Generator cache: %d failures\n", failures);
    return failures;
}
