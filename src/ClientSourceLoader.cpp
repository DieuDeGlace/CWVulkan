#include "ClientSourceLoader.h"
#include "NativeKeyOrder.h"
#include "TaskPool.h"
#include <algorithm>
#include <chrono>
#include <future>
#include <tuple>
#include <utility>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace {
void priority(int value) {
#ifdef _WIN32
    SetThreadPriority(GetCurrentThread(), value);
#else
    (void)value;
#endif
}
void yield() {
#ifdef _WIN32
    Sleep(0);
#else
    std::this_thread::yield();
#endif
}
}
bool ClientSourceLoader::View::sameDemand(const View &other) const {
    return std::tie(window.center, window.minimum, window.maximum, window.radiusSquared,
                    point, retentionCentre, retentionRadius, destination) ==
           std::tie(other.window.center, other.window.minimum, other.window.maximum, other.window.radiusSquared,
                    other.point, other.retentionCentre, other.retentionRadius, other.destination);
}
ClientSourceLoader::ClientSourceLoader(Access access, View initial)
    : access(std::move(access)), view(initial), worker([this](std::stop_token stop) { run(stop); }) {}
ClientSourceLoader::~ClientSourceLoader() { stop(); }

void ClientSourceLoader::setView(View next) {
    {
        std::lock_guard lock(mutex);
        if (failure)
            std::rethrow_exception(failure);
        if (!view.sameDemand(next)) {
            ++revision;
            required.store(true, std::memory_order_release);
        }
        view = next;
    }
    wake.notify_all();
}
void ClientSourceLoader::stop() {
    if (!worker.joinable())
        return;
    worker.request_stop();
    wake.notify_all();
    worker.join();
}

void ClientSourceLoader::run(std::stop_token stop) {
    try {
        // The viewed worker's entry policy is distinct from the chunk worker's
        // unproved initial scheduler. Provider fairness/actual PPL lanes are not
        // reproduced by these bounded engine task lanes.
        priority(-1);
        lanes = std::make_unique<TaskPool>(4);
        int sampledPerformance = 1;
        while (!stop.stop_requested()) {
            View sampled;
            uint64_t epoch;
            {
                std::lock_guard lock(mutex);
                sampled = view;
                epoch = revision;
            }
            if (sampled.cpuPerformance != sampledPerformance) {
                sampledPerformance = sampled.cpuPerformance;
                priority(sampledPerformance == 0 ? -1 : 0);
                lanes = std::make_unique<TaskPool>(sampledPerformance == 0 ? 4 : 8);
            }
            struct Candidate { Key key; int32_t priority; };
            std::vector<Candidate> missing;
            for (int64_t x = sampled.window.minimum[0]; x <= sampled.window.maximum[0]; ++x)
                for (int64_t y = sampled.window.minimum[1]; y <= sampled.window.maximum[1]; ++y) {
                    const Key key{int32_t(x), int32_t(y)};
                    if (sampled.window.contains(x, y) && !access.loaded(key) && access.available(key))
                        missing.push_back({key, zonedemand::generationPriority(key, sampled.point)});
                }
            const bool needed = std::any_of(missing.begin(), missing.end(), [&](const Candidate &candidate) {
                return sampled.destination ||
                       zonedemand::withinRetention(int64_t(candidate.key[0]) - sampled.retentionCentre[0],
                                                   int64_t(candidate.key[1]) - sampled.retentionCentre[1],
                                                   sampled.retentionRadius);
            });
            {
                std::lock_guard lock(mutex);
                // An older pass cannot publish readiness for a newer demand.
                if (epoch == revision)
                    required.store(needed, std::memory_order_release);
            }
            keyorder::sort(missing, [](const Candidate &candidate) { return candidate.priority; });
            if (missing.size() > 4)
                missing.resize(4);
            selected.store(missing.size(), std::memory_order_release);
            if (missing.size() == 1) {
                access.load(missing.front().key);
            } else {
                std::vector<std::future<bool>> jobs;
                for (const auto &candidate : missing)
                    jobs.push_back(lanes->submit([this, key = candidate.key] { return access.load(key); }));
                // Finish the selected prefix before another snapshot or stop
                // test. Completion order is not priority/publication order.
                for (auto &job : jobs)
                    job.get();
            }
            selected.store(0, std::memory_order_release);
            if (missing.empty()) {
                std::unique_lock lock(mutex);
                wake.wait_for(lock, stop, std::chrono::milliseconds(5), [&] { return revision != epoch; });
            }
            yield();
        }
        lanes.reset();
    } catch (...) {
        const auto error = std::current_exception();
        lanes.reset(); // remaining admitted calls finish before error publication
        std::lock_guard lock(mutex);
        failure = error;
        selected.store(0, std::memory_order_release);
        required.store(true, std::memory_order_release);
    }
}
