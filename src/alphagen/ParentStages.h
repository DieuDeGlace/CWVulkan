#pragma once
// Shared immutable parent-stage cache. Loading runs outside the metadata lock;
// concurrent requests for one parent share a result or exception.
#include <algorithm>
#include <cstdint>
#include <future>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <limits>

namespace alphagen::detail {
template <class Stage> class ParentStages {
  public:
    using Parent = std::shared_ptr<const Stage>;
    struct UnitWeight { std::size_t operator()(const Stage &) const { return 1; } };
    explicit ParentStages(std::size_t entries, std::size_t weightLimit = std::numeric_limits<std::size_t>::max())
        : capacity(std::max(std::size_t(1), entries)), weightLimit(weightLimit) {}
    template <class Loader, class Weight = UnitWeight>
    Parent get(int32_t zx, int32_t zy, Loader load, Weight weight = {}) {
        const Key key{zx, zy};
        std::shared_future<Parent> future;
        std::shared_ptr<std::promise<Parent>> producer;
        {
            std::lock_guard guard(lock);
            const auto found = std::find_if(ready.begin(), ready.end(), [&](const auto &entry) { return entry.key == key; });
            if (found != ready.end()) {
                Parent result = found->stage;
                ready.splice(ready.begin(), ready, found);
                return result;
            }
            const auto loading = pending.find(key);
            if (loading != pending.end())
                future = loading->second;
            else {
                producer = std::make_shared<std::promise<Parent>>();
                future = producer->get_future().share();
                pending.emplace(key, future);
            }
        }
        if (!producer)
            return future.get();
        try {
            Parent result(load());
            const std::size_t charge = result ? weight(*result) : 0;
            std::lock_guard guard(lock);
            // Oversized results still reach their callers, without displacing
            // useful entries or exceeding this ready-cache budget.
            if (charge <= weightLimit) {
                while (!ready.empty() && (ready.size() >= capacity || readyWeight > weightLimit - charge)) {
                    readyWeight -= ready.back().weight;
                    ready.pop_back();
                }
                ready.push_front({key, result, charge});
                readyWeight += charge;
            }
            producer->set_value(result);
            pending.erase(key);
            return result;
        } catch (...) {
            std::lock_guard guard(lock);
            producer->set_exception(std::current_exception());
            pending.erase(key);
            throw;
        }
    }

  private:
    using Key = std::pair<int32_t, int32_t>;
    struct Ready { Key key; Parent stage; std::size_t weight; };
    const std::size_t capacity, weightLimit;
    std::size_t readyWeight = 0;
    std::mutex lock;
    std::list<Ready> ready;
    std::map<Key, std::shared_future<Parent>> pending;
};
} // namespace alphagen::detail
