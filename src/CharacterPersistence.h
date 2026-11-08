#pragma once
// Character-only saves from steam-character-save-worker-and-snapshots-20261108.md.
// Sources and sinks capture owned state. A source releases its World guard before
// returning this value; serialization/database work belongs to the sink.
#include "NativeSaves.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace characterpersistence {
struct Snapshot {
    int32_t slot = -1;
    nativesaves::Character character;
    std::string notes; // engine extension; never part of the native character blob
};
class Session : public std::enable_shared_from_this<Session> {
  public:
    using Clock = std::function<uint32_t()>;
    struct Access {
        std::function<bool()> named;
        std::function<int32_t(size_t)> visibility; // selection, StartMenu, creation; stop at first nonzero
        std::function<std::optional<Snapshot>()> capture;
        std::function<bool(Snapshot &&)> write;
    };
    explicit Session(Clock clock = {});
    void bind(Access); // quiescent owner setup, before generation cycles can capture
    bool configured() const { return bound.load(std::memory_order_acquire); }
    class Cycle {
      public:
        ~Cycle();
        void complete(); // after all admitted generation work, at most once
      private:
        friend class Session;
        Cycle(std::shared_ptr<Session>, uint32_t);
        std::shared_ptr<Session> owner;
        uint32_t sample;
        std::atomic<bool> done{false};
    };
    std::shared_ptr<Cycle> beginCycle(); // sampled before generation; allocates only when overdue
    bool force(); // explicit writer call; no menu/name/cadence/role gate
    void requestStop() { running.store(false, std::memory_order_release); }
    void finish(); // outside World guards, after admitted work can complete; once, three UI gates only
    std::vector<std::string> takeErrors();
  private:
    bool menusAllow() const;
    bool save();
    void periodic(uint32_t);
    void releaseCycle();
    void report(std::string);
    Clock now;
    std::atomic<uint32_t> baseline;
    std::atomic<bool> bound{false}, running{true}, busy{false};
    Access access;
    std::mutex execution, waiting, errorsMutex;
    std::condition_variable wake;
    bool finished = false;
    std::vector<std::string> errors;
};

// The owner adds jobs before submission and removes its sentinel after submitting
// the batch. Worker completion cannot capture until that sentinel is removed.
class Batch {
  public:
    explicit Batch(std::shared_ptr<Session::Cycle> cycle) : cycle(std::move(cycle)) {}
    void add() { remaining.fetch_add(1, std::memory_order_relaxed); }
    void completed() {
        if (remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) cycle->complete();
    }
    // True: no worker remains; enqueue complete() on a worker, outside the owner's guard.
    bool seal() { return remaining.fetch_sub(1, std::memory_order_acq_rel) == 1; }
    void complete() { cycle->complete(); }
  private:
    std::shared_ptr<Session::Cycle> cycle;
    std::atomic<size_t> remaining{1};
};
struct JobDone {
    std::shared_ptr<Batch> batch;
    ~JobDone() { if (batch) batch->completed(); }
};
} // namespace characterpersistence
