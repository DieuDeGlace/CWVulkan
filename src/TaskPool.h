#pragma once
// Bounded worker lanes reused across mesh batches. Jobs own their futures and
// immutable inputs; destruction drains queued jobs before joining the workers.
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class TaskPool {
  public:
    explicit TaskPool(unsigned count) {
        count = std::max(1u, count);
        workers.reserve(count);
        for (unsigned i = 0; i < count; ++i)
            workers.emplace_back([this](std::stop_token stop) {
                for (;;) {
                    std::packaged_task<void()> task;
                    {
                        std::unique_lock lock(mutex);
                        if (!ready.wait(lock, stop, [&] { return !queue.empty(); }))
                            return;
                        task = std::move(queue.front());
                        queue.pop_front();
                    }
                    task();
                    outstanding.fetch_sub(1, std::memory_order_release);
                }
            });
    }
    ~TaskPool() {
        for (auto &worker : workers) worker.request_stop();
        ready.notify_all();
        workers.clear();
    }
    unsigned size() const { return unsigned(workers.size()); }
    bool idle() const { return outstanding.load(std::memory_order_acquire) == 0; }
    template <class F> auto submit(F &&function) {
        std::packaged_task<std::invoke_result_t<F &>()> task(std::forward<F>(function));
        auto result = task.get_future();
        {
            std::lock_guard lock(mutex);
            queue.emplace_back([task = std::move(task)]() mutable { task(); });
            outstanding.fetch_add(1, std::memory_order_relaxed);
        }
        ready.notify_one();
        return result;
    }
  private:
    std::mutex mutex;
    std::condition_variable_any ready;
    std::deque<std::packaged_task<void()>> queue;
    std::atomic<size_t> outstanding = 0;
    std::vector<std::jthread> workers;
};
