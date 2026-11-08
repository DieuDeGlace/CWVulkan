#include "Extracted.h"
#include "HostSourceLoader.h"
#include "NativeSaves.h"
#include "SourceStates.h"
#include "Spawned.h"
#include <atomic>
#include <bit>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <thread>

namespace {
namespace fs = std::filesystem;
using namespace std::chrono_literals;
bool waitUntil(const auto &ready) {
    const auto deadline = std::chrono::steady_clock::now() + 15s;
    while (!ready()) {
        if (std::chrono::steady_clock::now() >= deadline) return false;
        std::this_thread::sleep_for(1ms);
    }
    return true;
}
struct ReplayClock {
    std::atomic<uint32_t> value{0};
    std::atomic<unsigned> samples{0};
    uint32_t sample() {
        const auto result = value.load();
        samples.fetch_add(1);
        return result;
    }
    bool cycles() {
        const auto target = samples.load() + 3;
        return waitUntil([&] { return samples.load() >= target; });
    }
};
struct CountedMetadata final : sourcestates::Metadata {
    mutable std::atomic<unsigned> captures{0};
    int32_t day = 7; // mutable fields use the owning Store guard
    float timeMs = std::bit_cast<float>(0x4c095441u);
    void capture(nativesaves::WorldState &out) const override {
        out.day = day;
        out.timeMs = timeMs;
        captures.fetch_add(1);
    }
};
void cadence(const auto &check) {
    HostSaveCadence timer(100);
    check(!timer.due(60100, true) && timer.due(60101, true) && !timer.due(60101, false),
          "strict elapsed boundary and running gate");
    timer.completed(60101);
    check(!timer.due(120101, true) && timer.due(120102, true),
          "baseline uses cycle sample rather than writer completion time");
    HostSaveCadence wrap(0xfffffff0u);
    check(!wrap.due(uint32_t(0xfffffff0u + 60000u), true) &&
              wrap.due(uint32_t(0xfffffff0u + 60001u), true),
          "unsigned elapsed time survives the 32-bit wall-clock wrap");
}
void workerScheduling(const fs::path &folder, const auto &check) {
    ExtractedWorld world;
    world.openGenerated(0, 0);
    world.configureSourceStates(nullptr, false);
    auto store = world.sourceStateStore();
    auto metadata = std::make_shared<CountedMetadata>();
    auto clock = std::make_shared<ReplayClock>();
    const auto file = folder / "periodic.db";
    store->setMetadata(metadata);
    store->setPersistence(file, false);
    store->setHostClock([clock] { return clock->sample(); });
    world.setHostStreaming(true); // empty demand: no terrain generation obscures cadence
    check(clock->cycles(), "worker establishes its initial wall-clock sample");
    clock->value = 60000;
    check(clock->cycles() && metadata->captures == 0 && !fs::exists(file), "exactly 60 seconds does not capture");
    clock->value = 60001;
    check(clock->cycles() && metadata->captures == 1 && !fs::exists(file), "gated due cycle captures without writing");
    store->setPersistence(file, true);
    check(clock->cycles() && metadata->captures == 1 && !fs::exists(file), "gated return still resets the cadence baseline");
    {
        auto guard = store->lock();
        metadata->day = 1234567;
        metadata->timeMs = -123.25f;
    }
    clock->value = 120001;
    check(clock->cycles() && metadata->captures == 1 && !fs::exists(file),
          "accelerated World calendar cannot substitute for elapsed wall time");
    clock->value = 120002;
    check(clock->cycles() && metadata->captures == 2 && fs::exists(file), "next strict wall interval writes the current metadata");
    nativesaves::WorldState saved;
    check(nativesaves::readWorldFile(file, saved) && saved.day == 1234567 && saved.timeMs == -123.25f,
          "periodic file contains raw current day/time without normalization");
    clock->value = 1000000;
    check(clock->cycles() && metadata->captures == 3, "large elapsed jump captures once without catch-up writes");
    const auto blocker = folder / "regular-file";
    std::ofstream(blocker) << "not a directory";
    store->setPersistence(blocker / "world.db", true);
    clock->value = 1060001;
    check(clock->cycles() && metadata->captures == 4, "failed writer returns after one due capture");
    check(store->takeSaveErrors().size() == 1 && store->takeSaveErrors().empty(), "writer failure is delivered once to the owner");
    const auto recovered = folder / "recovered.db";
    store->setPersistence(recovered, true);
    check(clock->cycles() && metadata->captures == 4 && !fs::exists(recovered),
          "failed writer still resets baseline instead of retrying every cycle");
    clock->value = 1120001;
    check(clock->cycles() && metadata->captures == 4 && !fs::exists(recovered), "failure baseline keeps the strict equality boundary");
    clock->value = 1120002;
    check(clock->cycles() && metadata->captures == 5 && fs::exists(recovered), "writer recovers at the next admitted interval");
    world.setHostStreaming(false);
    check(metadata->captures == 6 && nativesaves::readWorldFile(recovered, saved),
          "normal worker exit captures once after its last cycle");
    store->setPersistence(folder / "no-save" / "world.db", false);
    store->persist();
    check(metadata->captures == 7 && !fs::exists(folder / "no-save"), "no-save never creates even its target directory");
}
void metadataLifetime(const fs::path &folder, const auto &check) {
    ExtractedWorld world;
    world.openGenerated(0, 0);
    const auto store = world.sourceStateStore();
    nativesaves::WorldState earlier;
    constexpr auto timeBits = 0x4c095441u;
    {
        nativesaves::WorldState input;
        input.dead = {900}; input.rewards = {901};
        input.zones[{8, 9}].placeCounters[{2, 3}] = 11;
        SpawnedCreatures actor;
        actor.setClock(17, std::bit_cast<float>(timeBits));
        actor.setWorldDead({7, 8}); actor.setWorldRewards({19, 20});
        actor.setStoredZones(world, &input, false);
        earlier = store->snapshot();
        check(earlier.dead == std::set<int64_t>{7, 8} && earlier.rewards == std::set<int64_t>{19, 20},
              "source archive binding does not merge unrelated runtime identifier sets");
        actor.setClock(18, -0.0f);
        actor.setWorldDead({21}); actor.setWorldRewards({22});
        input.zones.clear();
        {
            auto guard = store->lock();
            store->retained().zones[{8, 9}].placeCounters[{2, 3}] = 99;
        }
    } // both actor and input die; the Store owns every value used below
    const auto later = store->snapshot();
    check(earlier.day == 17 && std::bit_cast<uint32_t>(earlier.timeMs) == timeBits &&
              earlier.zones.at({8, 9}).placeCounters.at({2, 3}) == 11 &&
              later.day == 18 && std::bit_cast<uint32_t>(later.timeMs) == 0x80000000u &&
              later.dead == std::set<int64_t>{21} && later.rewards == std::set<int64_t>{22} &&
              later.zones.at({8, 9}).placeCounters.at({2, 3}) == 99,
          "metadata outlives its actor while earlier scalar/identifier/map snapshots remain independent");
    const auto file = folder / "owned.db";
    store->setPersistence(file, true);
    store->persist();
    nativesaves::WorldState loaded;
    check(nativesaves::readWorldFile(file, loaded) && loaded.day == later.day &&
              std::bit_cast<uint32_t>(loaded.timeMs) == 0x80000000u && loaded.dead == later.dead &&
              loaded.rewards == later.rewards && loaded.zones.at({8, 9}).placeCounters.at({2, 3}) == 99,
          "real state database round-trips owned metadata and retained values after actor destruction");
    world.openGenerated(0, 0); // reuse must forget the previous session's target/provider/clock
    store->persist();
    const auto reset = store->snapshot();
    check(reset.day == nativesaves::NEW_WORLD_DAY && reset.dead.empty() && reset.rewards.empty() && reset.zones.empty() &&
              !store->hostClock(), "reader reuse resets persistence ownership and its replay clock");
    check(nativesaves::readWorldFile(file, loaded) && loaded.day == later.day && loaded.dead == later.dead,
          "reset reader cannot overwrite a previous session's file");
}
void finalBeforeRetention(const fs::path &folder, const auto &check) {
    auto world = std::make_unique<ExtractedWorld>();
    world->openGenerated(0, 0);
    nativesaves::WorldState initial;
    initial.zones[{90, 91}].deadTracked = 41;
    world->configureSourceStates(&initial, false);
    auto store = world->sourceStateStore();
    auto metadata = std::make_shared<CountedMetadata>();
    auto clock = std::make_shared<ReplayClock>();
    const auto file = folder / "final.db";
    store->setMetadata(metadata);
    store->setHostClock([clock] { return clock->sample(); });
    store->setPersistence(file, true);
    world->setHostSourceCentres({{0, 0}});
    world->setHostStreaming(true);
    check(waitUntil([&] { return bool(store->find({0, 0})); }), "host factory admits a still-required physical source");
    {
        auto guard = store->lock();
        if (const auto source = store->find({0, 0})) source->lists.deadTracked = 77;
    }
    world->setHostStreaming(false); // drain selected prefix, then capture; no guard held while joining
    nativesaves::WorldState saved;
    check(metadata->captures == 1 && store->find({0, 0}) &&
              nativesaves::readWorldFile(file, saved) && saved.zones.contains({90, 91}) && !saved.zones.contains({0, 0}),
          "final capture includes retained state without recording still-loaded source payloads");
    world.reset(); // same reader's destructor now records the loaded sources
    check(store->snapshot().zones.at({0, 0}).deadTracked == 77 &&
              nativesaves::readWorldFile(file, saved) && !saved.zones.contains({0, 0}),
          "later World teardown retention cannot retroactively change the submitted final snapshot");
}
} // namespace

int nativePersistenceTests() {
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        if (!ok) { ++failures; std::printf("FAIL native persistence: %s\n", message); }
    };
    const auto temp = fs::weakly_canonical(fs::temp_directory_path());
    const auto folder = temp / ("CubeVulkan-native-persistence-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directory(folder);
    cadence(check);
    workerScheduling(folder, check);
    metadataLifetime(folder, check);
    finalBeforeRetention(folder, check);
    const auto resolved = fs::weakly_canonical(folder);
    const bool safe = resolved.parent_path() == temp && resolved.filename().string().starts_with("CubeVulkan-native-persistence-");
    check(safe, "temporary cleanup target remains inside the verified test directory");
    if (safe) fs::remove_all(resolved);
    std::printf("Native host state persistence: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
