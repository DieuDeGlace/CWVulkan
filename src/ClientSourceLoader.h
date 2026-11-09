#pragma once
// Viewed source batches run independently of presentation.
// docs/specs/steam-generation-request-selection-and-worker-drain-20261109.md
#include "SteamZoneDemand.h"
#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

class TaskPool;
class ClientSourceLoader {
  public:
    using Key = zonedemand::BlockPoint;
    struct View {
        zonedemand::Window window;
        Key point{}, retentionCentre{};
        int retentionRadius = 0;
        bool destination = false;
        int cpuPerformance = 1;
        bool sameDemand(const View &) const;
    };
    // Ports must remain valid and support concurrent source reads/publication.
    // The owner stops this loader before releasing those borrowed inputs.
    struct Access {
        std::function<bool(Key)> loaded, available, load;
    };
    ClientSourceLoader(Access, View);
    ~ClientSourceLoader();
    void setView(View);
    void stop(); // selected work completes; no new batch is selected afterward
    bool preparing() const { return required.load(std::memory_order_acquire); }
    size_t pending() const { return selected.load(std::memory_order_acquire); }

  private:
    void run(std::stop_token);
    Access access;
    std::mutex mutex;
    std::condition_variable_any wake;
    View view;
    uint64_t revision = 0;
    std::exception_ptr failure;
    std::atomic<bool> required{true};
    std::atomic<size_t> selected{0};
    std::unique_ptr<TaskPool> lanes;
    std::jthread worker; // last: all state exists before the child starts
};
