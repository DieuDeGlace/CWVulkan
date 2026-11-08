#include "Extracted.h"
#include "SourceStates.h"
#include "ZoneStates.h"
#include <algorithm>
#include <chrono>
#include <string>
#include <cstdio>
#include <future>
#include <thread>

namespace {
using nativesaves::Cell;
using nativesaves::Record;
Record marker(uint8_t value) { Record out{}; out[0] = value; return out; }
void publication(const auto &check) {
    sourcestates::Store store;
    nativesaves::WorldState initial;
    initial.zones[{7, 8}].deadTracked = 71;
    initial.zones[{7, 8}].placeCounters[{2, 3}] = 9;
    store.configure(&initial, true);
    auto first = store.make({7, 8}, 900, {marker(1)}, {}, {});
    auto second = store.make({-2, -3}, 50, {}, {marker(2)}, {});
    store.install(first); store.install(second);
    check(store.scalar({7, 8}) == 0 && store.counter({7, 8}, {2, 3}) == 0,
          "loaded empty/zero summaries override older retained values");
    store.queueGround(first->key); store.queueGround(first->key);
    store.queueObjects(first->key);
    store.queueGround(second->key); store.queueObjects(second->key);
    check(!first->lists.groundItemsChanged && !first->lists.staticObjectsChanged,
          "pending membership is distinct from sticky array override flags");
    first->lists.groundItems = {marker(3)};
    second->lists.staticObjects.clear();
    std::vector<std::string> order;
    store.publish(
        [&](Cell key, const auto &records) {
            order.push_back(key == first->key ? "g900" : "g50");
            if (key == first->key) check(records == std::vector<Record>{marker(3)}, "publication reads the current array after admission");
        },
        [&](Cell key, const auto &records) {
            order.push_back(key == first->key ? "o900" : "o50");
            check(records.empty(), "explicit empty object arrays still publish");
        },
        [&](Cell key, const auto &) { order.push_back(key == first->key ? "r900" : "r50"); });
    check(order == std::vector<std::string>{"g50", "g900", "o50", "o900", "r50", "r900"},
          "publisher uses unsigned source order, array-group order and the final object pass");
    check(first->lists.groundItemsChanged && first->lists.staticObjectsChanged && second->lists.staticObjectsChanged,
          "publication sets sticky flags even for empty arrays");
    order.clear();
    store.publish([&](Cell, const auto &) { order.push_back("again"); }, {}, {});
    check(order.empty() && first->lists.groundItemsChanged, "publication clears memberships without clearing source flags");
    store.queueGround(first->key);
    store.unlink(first);
    check(store.scalar({7, 8}) == 71 && store.counter({7, 8}, {2, 3}) == 9,
          "absent physical source uses retained summaries even while its old lease survives");
    auto replacement = store.make(first->key, first->order, {}, {}, {});
    store.install(replacement);
    store.publish([&](Cell, const auto &) { order.push_back("stale"); }, {}, {});
    check(order.empty() && !replacement->lists.groundItemsChanged,
          "retired pending identity cannot target a replacement at the same coordinate or address order");
    check(store.keys().size() == 2 && !store.find({100, 200}), "summary lookup creates no missing source");
}
void physicalLifecycle(const auto &check) {
    const Cell key{0, 0}, gate{13, -7};
    const auto raw = worldgen::zone(0, 0);
    const auto &column = raw->terrain.columns[5 * 64 + 6];
    check(!column.blocks.empty(), "generated test column has stored cells");
    if (column.blocks.empty()) return;
    const int32_t z = column.base + int32_t(column.blocks.size()) - 1;
    const auto &base = column.blocks.back();
    const ExtractedWorld::GameBlock original{base.r, base.g, base.b, base.glow, base.type, base.flags};
    const ExtractedWorld::GameBlock changed{20, 30, 40, 0, 1, 0}, last{50, 60, 70, 0, 1, 0};
    nativesaves::WorldState initial;
    auto &saved = initial.zones[key];
    saved.flags = 3;
    saved.groundItems = {marker(0x44)};
    saved.staticObjects = {marker(0x55)};
    saved.blockChanges = {zonestates::blockChange(5, 6, z, original, changed),
                          zonestates::blockChange(5, 6, z, changed, last)};
    saved.placeCounters[gate] = 4;
    saved.deadTracked = 42;
    ExtractedWorld world;
    world.openGenerated(0, 0);
    world.setClientStreaming(true);
    world.configureSourceStates(&initial, false);
    const auto store = world.sourceStateStore();
    check(!store->find(key) && world.loadedSourceCount() == 0, "configuration alone publishes no physical source");
    check(world.loadSourceZone(0, 0), "physical factory loads generated source");
    auto first = store->find(key);
    auto terrainLease = world.gameZoneToken(0, 0);
    check(first && terrainLease, "factory publishes the matching gameplay payload and terrain identity");
    if (!first) return;
    check(first->lists.groundItems == saved.groundItems && first->lists.staticObjects == saved.staticObjects &&
              first->lists.groundItemsChanged && first->lists.staticObjectsChanged && first->lists.blockChanges.size() == saved.blockChanges.size() &&
              std::equal(first->lists.blockChanges.begin(), first->lists.blockChanges.end(), saved.blockChanges.begin(),
                         [](const auto &a, const auto &b) { return a.bytes == b.bytes; }) &&
              first->lists.placeCounters == saved.placeCounters && first->lists.deadTracked == 42,
          "all five retained fields and both flags are restored at factory publication");
    check(world.gameBlock(5, 6, z) == last, "ordered repeated-coordinate edits replay before physical publication");
    int notices = 0;
    store->publish([&](Cell, const auto &) { ++notices; }, [&](Cell, const auto &) { ++notices; }, {});
    check(notices == 0, "restoration does not invent dirty memberships");
    {
        auto guard = world.lockSourceState();
        store->retained().zones[key].groundItems = {marker(0x99)};
    }
    world.loadSourceZone(0, 0);
    check(store->find(key) == first && first->lists.groundItems == std::vector<Record>{marker(0x44)},
          "an existing physical hit preserves identity and does not reapply retained state");
    world.configureSourceStates(nullptr, false);
    check(store->find(key) == first && first->lists.groundItemsChanged && first->lists.staticObjectsChanged &&
              first->lists.blockChanges.size() == 2 && world.gameBlock(5, 6, z) == last,
          "retained-map rebinding preserves an existing source's flags, payload, edits and identity");
    world.setGameBlock(5, 6, z, changed);
    check(store->find(key) == first && world.gameZoneToken(0, 0)->order == terrainLease->order,
          "COW block replacement shares the same source payload and identity");
    {
        auto guard = world.lockSourceState();
        first->lists.groundItems.clear(); first->lists.staticObjects.clear();
        first->lists.blockChanges.clear(); first->lists.placeCounters.clear(); first->lists.deadTracked = 0;
    }
    const auto retired = world.retireClientSource({1000, 1000}, 1);
    check(retired && !store->find(key) && world.loadedSourceCount() == 0,
          "physical retirement ends loaded membership while old leases remain alive");
    const auto snapshot = store->snapshot();
    const auto &recorded = snapshot.zones.at(key);
    check(recorded.flags == 3 && recorded.groundItems.empty() && recorded.staticObjects.empty() &&
              recorded.blockChanges.empty() && recorded.placeCounters.empty() && recorded.deadTracked == 0,
          "actual physical retirement records authoritative empty/zero values for all five fields");
    world.loadSourceZone(0, 0);
    check(store->find(key) != first && world.gameZoneToken(0, 0)->order != terrainLease->order &&
              world.gameBlock(5, 6, z) == original && store->find(key)->lists.blockChanges.empty(),
          "fresh reload uses a fresh identity and raw generation after old edits were cleared");
    auto independent = world.forkSource();
    independent->setClientStreaming(true);
    independent->configureSourceStates(nullptr, false);
    independent->loadSourceZone(0, 0);
    check(independent->sourceStateStore() != store && independent->sourceStateStore()->find(key) != store->find(key) &&
              independent->sourceStateStore()->snapshot().zones.empty(),
          "independent Worlds do not share mutable physical payloads or retained maps");
    std::promise<void> entered;
    auto signaled = entered.get_future();
    auto guard = world.lockSourceState();
    auto removal = std::async(std::launch::async, [&] {
        entered.set_value();
        return world.retireClientSource({1000, 1000}, 1);
    });
    signaled.wait();
    check(removal.wait_for(std::chrono::milliseconds(20)) == std::future_status::timeout && store->find(key),
          "owner World guard protects the complete lifetime of borrowed mutable views");
    guard.unlock();
    check(removal.get().has_value() && !store->find(key), "retirement proceeds after the owner scope releases its guard");
}
void hostAndTeardown(const auto &check) {
    std::shared_ptr<sourcestates::Store> held;
    {
        ExtractedWorld world;
        world.openGenerated(0, 0);
        {
            nativesaves::WorldState shortLived;
            shortLived.zones[{0, 0}].flags = 1;
            shortLived.zones[{0, 0}].groundItems = {marker(0xfe)};
            shortLived.zones[{0, 0}].deadTracked = 999;
            world.configureSourceStates(&shortLived, true);
        }
        held = world.sourceStateStore();
        world.loadSourceZone(0, 0);
        auto live = held->find({0, 0});
        check(live && !live->lists.groundItemsChanged && live->lists.deadTracked == 0 && live->lists.blockChanges.empty(),
              "nonzero restore gate leaves generated state unchanged after the input archive dies");
        if (!live) return;
        {
            auto guard = world.lockSourceState();
            live->lists.groundItems.clear(); live->lists.deadTracked = 7; live->lists.placeCounters[{1, 2}] = 12;
            held->queueGround({0, 0}); held->publish({}, {}, {});
        }
        world.setHostStreaming(true); // empty native demand retires the already loaded source
        for (int tries = 0; tries < 2000 && world.gameZoneLoaded(0, 0); ++tries)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        world.setHostStreaming(false); // join outside the World guard
        check(!world.gameZoneLoaded(0, 0) && !held->find({0, 0}), "host worker removes the actual outside-demand payload");
        const auto stored = held->snapshot().zones.at({0, 0});
        check(stored.groundItems.empty() && (stored.flags & 1) && stored.deadTracked == 7 && stored.placeCounters.at({1, 2}) == 12,
              "host retirement records current state despite restoration being suppressed");
        world.loadSourceZone(1, 0);
        auto tail = held->find({1, 0});
        check(bool(tail), "teardown fixture has a final loaded source");
        if (tail) tail->lists.deadTracked = 13;
    }
    check(held->keys().empty() && held->snapshot().zones.at({1, 0}).deadTracked == 13,
          "reader teardown records remaining physical payloads without a dangling save-file owner");
}
} // namespace
int physicalSourceStateTests() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) { ++failures; std::printf("FAIL physical source state: %s\n", message); }
    };
    if (worldgen::modHooksSet() || worldgen::usingAlphaTerrain()) {
        check(false, "controlled lifecycle fixture requires the unmodified Steam generator");
        return failures;
    }
    publication(check);
    physicalLifecycle(check);
    hostAndTeardown(check);
    std::printf("Physical source state: %s\n", failures ? "FAILED" : "passed");
    return failures;
}
