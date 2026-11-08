#include "Extracted.h"
#include "SteamZoneDemand.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <future>
#include <set>
#include <stdexcept>
#include <thread>

int hostSourceTests() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) {
            ++failures;
            std::printf("FAIL host source: %s\n", message);
        }
    };
    using Point = zonedemand::BlockPoint;
    struct PriorityCase { Point zone, point; int32_t expected; };
    const PriorityCase cases[]{
        {{0,0},{64,64},0}, {{-1,-1},{0,0},0}, {{0,0},{128,0},4096},
        {{0,0},{384,0},102400}, {{0,0},{385,0},-1}, {{0,0},{-384,0},147456},
        {{0,0},{-385,0},-1}, {{1,0},{448,0},102400}, {{1,0},{449,0},-1}};
    for (const auto &c : cases) {
        const std::array<Point, 1> centres{c.point};
        check(zonedemand::hostGenerationPriority(c.zone, centres) == c.expected,
              "literal host closed-edge and 384-block scoring-gate vector");
    }
    const std::array<Point, 3> multiple{{{1000000000,1000000000},{384,0},{128,0}}};
    check(zonedemand::hostGenerationPriority({0,0}, multiple) == 4096 &&
              zonedemand::hostGenerationPriority({0,0}, {}) == -1,
          "host priority takes the minimum admitted centre and rejects an empty centre list");
    if (worldgen::modHooksSet() || worldgen::usingAlphaTerrain()) {
        check(false, "fixture requires the independent Steam generator without another owner's hooks");
        return failures;
    }
    worldgen::forgetZones();
    ExtractedWorld blocks;
    blocks.openGenerated(0, 0);
    const auto heldBatch = [&](const char *message) {
        struct Gate {
            std::promise<void> entered, release;
            std::shared_future<void> released = release.get_future().share();
            std::atomic<bool> claimed{false};
        };
        const auto gate = std::make_shared<Gate>();
        auto enteredFuture = gate->entered.get_future();
        worldgen::ModHooks hooks;
        hooks.placeKind = [gate](int32_t, int32_t, bool, int kind) {
            if (!gate->claimed.exchange(true)) {
                gate->entered.set_value();
                gate->released.wait();
            }
            return kind;
        };
        worldgen::setModHooks(hooks);
        blocks.setHostSourceCentres({{0,0}});
        blocks.setHostStreaming(true);
        check(enteredFuture.wait_for(std::chrono::seconds(15)) == std::future_status::ready, message);
        check(blocks.pendingHostSourceCount() == 4, "one serial host batch selects four missing requests");
        blocks.setHostSourceCentres({{6400,6400}});
        check(blocks.pendingHostSourceCount() == 4, "moving centres cannot replace an admitted prefix");
        auto stopped = std::async(std::launch::async, [&] { blocks.setHostStreaming(false); });
        check(stopped.wait_for(std::chrono::milliseconds(20)) == std::future_status::timeout,
              "shutdown joins the held batch rather than abandoning its requests");
        gate->release.set_value();
        check(stopped.wait_for(std::chrono::seconds(30)) == std::future_status::ready, "admitted serial batch drains during shutdown");
        stopped.get();
    };
    heldBatch("first uncached host factory reaches the controlled gate");
    const std::set<std::pair<int,int>> zero{{-1,-1},{-1,0},{0,-1},{0,0}};
    bool firstPrefix = blocks.loadedSourceCount() == 4;
    for (const auto &key : zero)
        firstPrefix &= blocks.gameZoneLoaded(key.first, key.second);
    check(firstPrefix, "the complete first prefix contains all four zero-distance rectangles from the old snapshot");
    check(!blocks.hostSourceStreaming() && blocks.pendingHostSourceCount() == 0,
          "worker exit preserves existing sources and clears active worker state");
    const auto lease = blocks.gameZoneToken(0, 0);
    const auto canonical = worldgen::zone(0, 0);
    size_t compared = 0, mismatches = 0;
    for (int x = 0; x < 64; ++x)
        for (int y = 0; y < 64; ++y) {
            const auto &column = canonical->terrain.columns[size_t(x) * 64 + y];
            for (size_t at = 0; at < column.blocks.size(); ++at) {
                const auto &b = column.blocks[at];
                const int32_t z = int32_t(int64_t(column.base) + int64_t(at));
                ExtractedWorld::GameBlock expected{b.r,b.g,b.b,b.glow,b.type,b.flags};
                if (b.type == 0 && z <= 0 && !(b.flags & 2))
                    expected = {255,255,255,0,2,0};
                mismatches += blocks.gameBlock(x,y,z) != expected;
                ++compared;
            }
        }
    check(compared && !mismatches, "host source creation preserves raw canonical bytes instead of eagerly lighting neighbors");
    heldBatch("a second batch skips loaded hits before reaching its first missing factory");
    const std::set<std::pair<int,int>> next{{-2,-1},{-2,0},{1,-1},{1,0},{-1,-2},{0,-2},{-1,1},{0,1}};
    bool secondPrefix = blocks.loadedSourceCount() == 8;
    for (const auto &key : blocks.loadedLightZones())
        secondPrefix &= zero.contains(key) || next.contains(key);
    const auto retained = blocks.gameZoneToken(0, 0);
    check(secondPrefix && lease && retained && lease->order == retained->order,
          "loaded hits consume no cap and remain the same source while four nearest missing requests finish");
    blocks.gameBlock(0,0,1000); // populate the loaded-block memo before empty-demand retirement
    blocks.setHostStreaming(true); // stop cleared the previous centres; there is no stand-in
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (blocks.loadedSourceCount() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    bool loaded = true;
    check(blocks.loadedSourceCount() == 0 && !blocks.gameZoneToken(0,0) && lease && lease->keepAlive &&
              blocks.gameBlock(0,0,1000,&loaded) == ExtractedWorld::GameBlock{200,200,200,0,1,0} && !loaded,
          "empty required set retires all sources while held leases survive and missing queries use the fallback");
    blocks.setHostStreaming(false);
    worldgen::ModHooks throwing;
    std::promise<void> failed;
    auto failedFuture = failed.get_future();
    std::atomic<bool> failedOnce{false};
    throwing.placeKind = [&](int32_t, int32_t, bool, int) -> int {
        if (!failedOnce.exchange(true))
            failed.set_value();
        throw std::runtime_error("controlled host source failure");
    };
    worldgen::setModHooks(throwing);
    blocks.setHostSourceCentres({{0,0}});
    blocks.setHostStreaming(true);
    check(failedFuture.wait_for(std::chrono::seconds(15)) == std::future_status::ready, "controlled factory failure reaches the worker");
    bool delivered = false;
    const auto errorDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!delivered && std::chrono::steady_clock::now() < errorDeadline) {
        try { blocks.setHostSourceCentres({{0,0}}); }
        catch (const std::runtime_error &error) { delivered = std::string(error.what()) == "controlled host source failure"; }
        if (!delivered)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    check(delivered, "worker errors return to the engine owner instead of terminating or silently disappearing");
    blocks.setHostStreaming(false);
    worldgen::setModHooks({});
    worldgen::forgetZones();
    std::printf("Native host source cycle: %s (%zu stored records; %zu mismatches)\n", failures ? "FAIL" : "PASS", compared, mismatches);
    return failures;
}
