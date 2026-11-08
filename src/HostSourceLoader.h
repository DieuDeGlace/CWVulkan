#pragma once
// Owned host source worker, independently specified by steam-host-source-cycle-20261107.md.
#include "SteamZoneDemand.h"
#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

// Unsigned wall milliseconds, independent of accelerated World time.
class HostSaveCadence {
  public:
    explicit HostSaveCadence(uint32_t initial) : previous(initial) {}
    bool due(uint32_t sampled, bool running) const { return running && uint32_t(sampled - previous) > 60000; }
    void completed(uint32_t sampled) { previous = sampled; }
  private:
    uint32_t previous;
};
class ExtractedWorld;
class HostSourceLoader {
  public:
    using Clock = std::function<uint32_t()>;
    HostSourceLoader(ExtractedWorld &, std::vector<zonedemand::BlockPoint>, Clock clock = {});
    ~HostSourceLoader();
    void setCentres(std::vector<zonedemand::BlockPoint>);
    size_t pending() const { return selected.load(std::memory_order_acquire); }

  private:
    void run(std::stop_token);
    ExtractedWorld &world;
    std::mutex mutex;
    std::condition_variable_any wake;
    std::vector<zonedemand::BlockPoint> centres;
    std::exception_ptr failure;
    std::atomic<size_t> selected{0}; // selected batch, including already returned calls
    Clock now;
    std::jthread worker; // declared last: state is ready before the worker starts
};
