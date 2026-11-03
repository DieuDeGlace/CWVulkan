#include "alphagen/ReliefStore.h"
#include <stdexcept>

namespace alphagen {
ReliefStore::ReliefStore(uint32_t seed, alphacoords::Mapping frame, std::size_t cachedParents, bool named)
    : world(seed), terrain(world), frame(frame), cache(cachedParents) {
    world.named = named;
}
ReliefStore::Parent ReliefStore::parent(int32_t zx, int32_t zy) {
    if (zx < 0 || zy < 0 || zx >= 65536 || zy >= 65536)
        return {};
    return cache.get(zx, zy, [&] {
        std::string error;
        auto stage = terrain.generateRelief(zx, zy, error);
        if (!stage)
            throw std::runtime_error(error);
        return stage;
    });
}
std::optional<ColumnLease> ReliefStore::column(int32_t x, int32_t y) {
    const auto point = frame.point(x, y);
    if (!point)
        return std::nullopt;
    auto stage = parent(point->x / alphacoords::ALPHA_ZONE, point->y / alphacoords::ALPHA_ZONE);
    if (!stage)
        return std::nullopt;
    return ColumnLease{std::shared_ptr<const Zone>(stage, stage->zone.get()), *point};
}
std::optional<alphacoords::Coverage> ReliefStore::coverage(int32_t zx, int32_t zy) const {
    return frame.zone(zx, zy);
}
} // namespace alphagen
