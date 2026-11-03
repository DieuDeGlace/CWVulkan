#include "alphagen/SurfaceStore.h"
#include <algorithm>

namespace alphagen {
const Column &ColumnLease::raw() const {
    return stage->zone->column(at.x, at.y);
}
Block ColumnLease::block(int32_t z) const {
    return stage->zone->read(at.x, at.y, z);
}
float ColumnLease::surface(bool includeWater) const {
    const auto &column = raw();
    float height = float(column.bottom); // The implicit filler immediately below the stored run.
    for (int64_t z = int64_t(column.top()) - 1; z >= column.bottom; --z)
        if (!clear(block(int32_t(z)).type())) {
            height = float(z + 1);
            break;
        }
    if (includeWater && height < 1 && block(0).type() == WATER)
        height = 1;
    return height;
}

SurfaceStore::SurfaceStore(uint32_t seed, alphacoords::Mapping frame, std::size_t cachedParents, bool named)
    : world(seed), terrain(world), frame(frame), capacity(std::max(std::size_t(1), cachedParents)) {
    world.named = named;
}

SurfaceStore::Parent SurfaceStore::parent(int32_t zx, int32_t zy) {
    if (zx < 0 || zy < 0 || zx >= 65536 || zy >= 65536)
        return {};
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
        Parent result(terrain.generateSurface(zx, zy));
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

std::optional<ColumnLease> SurfaceStore::column(int32_t x, int32_t y) {
    const auto point = frame.point(x, y);
    if (!point)
        return std::nullopt;
    auto stage = parent(point->x / alphacoords::ALPHA_ZONE, point->y / alphacoords::ALPHA_ZONE);
    if (!stage)
        return std::nullopt;
    return ColumnLease{std::move(stage), *point};
}
std::optional<alphacoords::Coverage> SurfaceStore::coverage(int32_t zx, int32_t zy) const {
    return frame.zone(zx, zy);
}
} // namespace alphagen
