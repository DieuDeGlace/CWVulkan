#include "alphagen/LandscapeStore.h"
#include <stdexcept>
#include <cmath>

namespace alphagen {
LandscapeStore::LandscapeStore(uint32_t seed, alphacoords::Mapping frame, std::size_t cachedParents, bool named,
                               std::shared_ptr<const TreeModels> palms)
    : world(seed), terrain(world, std::move(palms)), frame(frame), cache(cachedParents) {
    world.named = named;
}
LandscapeStore::Parent LandscapeStore::parent(int32_t zx, int32_t zy) {
    if (zx < 0 || zy < 0 || zx >= 65536 || zy >= 65536)
        return {};
    return cache.get(zx, zy, [&] {
        std::string error;
        auto stage = terrain.generateLandscape(zx, zy, error);
        if (!stage)
            throw std::runtime_error(error);
        return stage;
    });
}
std::optional<ColumnLease> LandscapeStore::column(int32_t x, int32_t y) {
    const auto point = frame.point(x, y);
    if (!point)
        return std::nullopt;
    auto stage = parent(point->x / alphacoords::ALPHA_ZONE, point->y / alphacoords::ALPHA_ZONE);
    if (!stage)
        return std::nullopt;
    return ColumnLease{std::shared_ptr<const Zone>(stage, stage->zone.get()), *point};
}
std::optional<TerrainSample> LandscapeStore::sample(int32_t x, int32_t y) {
    const auto point = frame.point(x, y);
    if (!point)
        return std::nullopt;
    const auto ax = point->x, ay = point->y;
    // Match the surface stage's preparation window without allocating voxel zones.
    for (int i = -1; i <= 1; ++i)
        for (int j = -1; j <= 1; ++j)
            terrain.prepareRegion(ax / REGION + i, ay / REGION + j);
    TerrainSample result;
    result.warmth = world.warmth(ax, ay);
    result.moisture = world.moisture(ax, ay);
    result.sea = world.seaShare(ax, ay);
    result.fire = world.fireShare(ax, ay);
    result.height = terrain.height(ax, ay);
    result.mountain = terrain.mountains(ax, ay);
    result.colour = terrain.surface(ax, ay, int32_t(std::floor(result.height)));
    return result;
}
std::optional<alphacoords::Coverage> LandscapeStore::coverage(int32_t zx, int32_t zy) const {
    return frame.zone(zx, zy);
}
} // namespace alphagen
