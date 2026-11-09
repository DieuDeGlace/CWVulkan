#include "ClientSourceLoader.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <future>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
using Key = ClientSourceLoader::Key;
using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;
ClientSourceLoader::View aroundOrigin() {
    return {zonedemand::clientWindow({0,0,0}, 0), {0,0}, {0,0}, 3, false, 1};
}
ClientSourceLoader::View single(Key key) {
    return {{key,key,key,0}, {key[0]*64,key[1]*64}, key, 3, true, 1};
}
bool eventually(const auto &predicate) {
    const auto deadline = Clock::now() + 5s;
    do {
        if (predicate()) return true;
        std::this_thread::sleep_for(1ms);
    } while (Clock::now() < deadline);
    return false;
}
struct Sources {
    std::mutex mutex;
    std::condition_variable changed;
    std::set<Key> loaded;
    std::vector<Key> entered;
    size_t active = 0, maximum = 0, finished = 0;
    bool blocked = false, fail = false;
    ClientSourceLoader::Access access() {
        return {
            [this](Key key) { std::lock_guard lock(mutex); return loaded.contains(key); },
            [](Key) { return true; },
            [this](Key key) {
                std::unique_lock lock(mutex);
                entered.push_back(key);
                maximum = std::max(maximum, ++active);
                changed.notify_all();
                changed.wait(lock, [&] { return !blocked; });
                --active;
                ++finished;
                if (fail && key == Key{0,0}) {
                    changed.notify_all();
                    throw std::runtime_error("source callback failure");
                }
                loaded.insert(key);
                changed.notify_all();
                return true;
            }};
    }
    bool waitEntered(size_t count) {
        std::unique_lock lock(mutex);
        return changed.wait_for(lock, 5s, [&] { return entered.size() >= count; });
    }
    void release() {
        { std::lock_guard lock(mutex); blocked = false; }
        changed.notify_all();
    }
};
}

int clientSourceLoaderTests() {
    int failures = 0;
    const auto check = [&](bool pass, const char *message) {
        if (!pass) { ++failures; std::printf("FAIL viewed source worker: %s\n", message); }
    };
    {
        Sources sources;
        ClientSourceLoader loader(sources.access(), aroundOrigin());
        check(eventually([&] { std::lock_guard lock(sources.mutex); return sources.loaded.size() == 9; }),
              "all nine demanded sources finish without another frame or view publication");
        check(eventually([&] { return !loader.preparing(); }), "completed required inputs become ready independently");
        loader.stop();
        check(sources.entered.size() == 9 && sources.maximum <= 4 && sources.finished == 9,
              "one source per coordinate, bounded selected batches, complete drain");
    }
    {
        Sources sources;
        sources.blocked = true;
        ClientSourceLoader loader(sources.access(), aroundOrigin());
        const bool admitted = sources.waitEntered(4);
        check(admitted && loader.pending() == 4, "four selected calls reach a controlled source gate");
        std::promise<void> stopping;
        auto stopped = std::async(std::launch::async, [&] { stopping.set_value(); loader.stop(); });
        stopping.get_future().wait();
        check(stopped.wait_for(50ms) == std::future_status::timeout,
              "stop waits for the admitted prefix instead of abandoning borrowed inputs");
        sources.release();
        stopped.get();
        const std::set<Key> first(sources.entered.begin(), sources.entered.end());
        check(first == std::set<Key>{{-1,-1},{-1,0},{0,-1},{0,0}} && sources.finished == 4 && loader.pending() == 0,
              "native tied priority prefix drains once; stop admits no later batch");
    }
    {
        std::mutex mutex;
        std::condition_variable changed;
        std::set<Key> loaded;
        bool oldEntered = false, newEntered = false, releaseOld = false, releaseNew = false;
        std::thread::id scanningThread;
        bool singleOnWorker = true;
        ClientSourceLoader loader({
            [&](Key key) { std::lock_guard lock(mutex); scanningThread = std::this_thread::get_id(); return loaded.contains(key); },
            [](Key) { return true; },
            [&](Key key) {
                std::unique_lock lock(mutex);
                singleOnWorker &= std::this_thread::get_id() == scanningThread;
                bool &entered = key == Key{0,0} ? oldEntered : newEntered;
                entered = true;
                changed.notify_all();
                changed.wait(lock, [&] { return key == Key{0,0} ? releaseOld : releaseNew; });
                loaded.insert(key);
                return true;
            }}, single({0,0}));
        {
            std::unique_lock lock(mutex);
            check(changed.wait_for(lock, 5s, [&] { return oldEntered; }), "old snapshot enters direct single-source call");
        }
        auto next = single({20,20});
        loader.setView(next);
        next = single({99,99}); // publication copies; no borrowed view state
        check(loader.preparing(), "new demand invalidates the old pass immediately");
        { std::lock_guard lock(mutex); releaseOld = true; }
        changed.notify_all();
        {
            std::unique_lock lock(mutex);
            check(changed.wait_for(lock, 5s, [&] { return newEntered; }), "worker samples copied destination after completing the old batch");
        }
        check(loader.preparing(), "old completion cannot clear a newer blocked destination");
        { std::lock_guard lock(mutex); releaseNew = true; }
        changed.notify_all();
        check(eventually([&] { return !loader.preparing(); }), "destination completes without another frame");
        loader.stop();
        check(loaded == std::set<Key>{{0,0},{20,20}} && singleOnWorker,
              "single requests run directly on the viewed worker with immutable published coordinates");
    }
    {
        Sources sources;
        sources.blocked = sources.fail = true;
        ClientSourceLoader loader(sources.access(), aroundOrigin());
        check(sources.waitEntered(4), "failing batch admits all four controlled calls");
        sources.release();
        bool delivered = eventually([&] {
            try { loader.setView(aroundOrigin()); }
            catch (const std::runtime_error &) { return true; }
            return false;
        });
        loader.stop();
        check(delivered && sources.finished == 4 && sources.entered.size() == 4 && loader.preparing(),
              "callback failure drains other selected calls, stays unready and reaches the owning thread");
    }
    std::printf("Viewed source worker: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
