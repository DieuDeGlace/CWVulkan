#pragma once
// Owned host source worker, independently specified by steam-host-source-cycle-20261107.md.
#include "SteamZoneDemand.h"
#include <atomic>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <thread>
#include <vector>

class ExtractedWorld;
class HostSourceLoader {
  public:
    HostSourceLoader(ExtractedWorld &, std::vector<zonedemand::BlockPoint>);
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
    std::jthread worker; // declared last: state is ready before the worker starts
};
