#include "HostSourceLoader.h"
#include "Extracted.h"
#include "NativeKeyOrder.h"
#include <chrono>
#include <set>

namespace {
uint32_t milliseconds() {
    return uint32_t(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
}
HostSourceLoader::HostSourceLoader(ExtractedWorld &world, std::vector<zonedemand::BlockPoint> initial, Clock clock)
    : world(world), centres(std::move(initial)), now(clock ? std::move(clock) : Clock(milliseconds)),
      worker([this](std::stop_token stop) { run(stop); }) {}

HostSourceLoader::~HostSourceLoader() {
    worker.request_stop();
    wake.notify_all();
    worker.join();
}

void HostSourceLoader::setCentres(std::vector<zonedemand::BlockPoint> next) {
    std::lock_guard lock(mutex);
    if (failure)
        std::rethrow_exception(failure); // deliver worker failures on the owning engine thread
    centres = std::move(next);
}

void HostSourceLoader::run(std::stop_token stop) {
    try {
        HostSaveCadence saves(now());
        while (!stop.stop_requested()) {
            std::vector<zonedemand::BlockPoint> sampled;
            {
                std::lock_guard lock(mutex);
                sampled = centres;
            }
            const uint32_t cycleTime = now(); // after centre snapshot, before retirement/generation
            std::set<std::pair<int, int>> required;
            for (const auto &point : sampled) {
                const auto window = zonedemand::hostWindow({int64_t(point[0]) * 65536, int64_t(point[1]) * 65536, 0});
                for (int64_t x = window.minimum[0]; x <= window.maximum[0]; ++x)
                    for (int64_t y = window.minimum[1]; y <= window.maximum[1]; ++y)
                        if (window.contains(x, y))
                            required.emplace(int(x), int(y));
            }
            world.retireHostSources(required);
            struct Candidate {
                std::pair<int, int> key;
                int32_t priority;
            };
            std::vector<Candidate> missing;
            for (const auto &key : required)
                // Alpha's finite extension domain keeps its existing admission policy.
                if (world.has(key.first, key.second) && !world.gameZoneLoaded(key.first, key.second)) {
                    const int32_t priority = zonedemand::hostGenerationPriority({key.first, key.second}, sampled);
                    if (priority >= 0)
                        missing.push_back({key, priority});
                }
            keyorder::sort(missing, [](const Candidate &candidate) { return candidate.priority; });
            if (missing.size() > 4)
                missing.resize(4);
            selected.store(missing.size(), std::memory_order_release);
            // Finish the admitted prefix before the next centre snapshot or stop
            // check. Calls are serial; an existing-source hit consumes its slot.
            for (const auto &candidate : missing)
                world.loadSourceZone(candidate.key.first, candidate.key.second);
            selected.store(0, std::memory_order_release);
            if (saves.due(cycleTime, !stop.stop_requested())) {
                world.sourceStateStore()->persist();
                saves.completed(cycleTime); // reset even when the writer is gated or returns failure
            }
            if (missing.empty()) {
                std::unique_lock lock(mutex);
                wake.wait_for(lock, stop, std::chrono::milliseconds(5), [] { return false; });
            }
            std::this_thread::yield();
        }
        // Successful stop drains the admitted prefix, captures already retained
        // values once, and returns before reader destruction records loaded sources.
        world.sourceStateStore()->persist();
    } catch (...) {
        std::lock_guard lock(mutex);
        failure = std::current_exception();
        selected.store(0, std::memory_order_release);
    }
}
