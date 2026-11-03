#include "alphagen/SurfaceStore.h"

namespace alphagen {
const Column &ColumnLease::raw() const {
    return zone->column(at.x, at.y);
}
Block ColumnLease::block(int32_t z) const {
    return zone->read(at.x, at.y, z);
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
    : world(seed), terrain(world), frame(frame), cache(cachedParents) {
    world.named = named;
}

SurfaceStore::Parent SurfaceStore::parent(int32_t zx, int32_t zy) {
    if (zx < 0 || zy < 0 || zx >= 65536 || zy >= 65536)
        return {};
    return cache.get(zx, zy, [&] { return terrain.generateSurface(zx, zy); });
}

std::optional<ColumnLease> SurfaceStore::column(int32_t x, int32_t y) {
    const auto point = frame.point(x, y);
    if (!point)
        return std::nullopt;
    auto stage = parent(point->x / alphacoords::ALPHA_ZONE, point->y / alphacoords::ALPHA_ZONE);
    if (!stage)
        return std::nullopt;
    return ColumnLease{std::shared_ptr<const Zone>(stage, stage->zone.get()), *point};
}
std::optional<alphacoords::Coverage> SurfaceStore::coverage(int32_t zx, int32_t zy) const {
    return frame.zone(zx, zy);
}
} // namespace alphagen
