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

namespace alphagen::detail {
template <class Stage> class ParentStages {
  public:
    using Parent = std::shared_ptr<const Stage>;
    explicit ParentStages(std::size_t entries) : capacity(std::max(std::size_t(1), entries)) {}
    template <class Loader> Parent get(int32_t zx, int32_t zy, Loader load) {
        const Key key{zx, zy};
        std::shared_future<Parent> future;
        std::shared_ptr<std::promise<Parent>> producer;
        {
            std::lock_guard guard(lock);
            const auto found = std::find_if(ready.begin(), ready.end(), [&](const auto &entry) { return entry.first == key; });
            if (found != ready.end()) {
                Parent result = found->second;
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
            std::lock_guard guard(lock);
            ready.emplace_front(key, result);
            while (ready.size() > capacity)
                ready.pop_back();
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
    const std::size_t capacity;
    std::mutex lock;
    std::list<std::pair<Key, Parent>> ready;
    std::map<Key, std::shared_future<Parent>> pending;
};
} // namespace alphagen::detail
