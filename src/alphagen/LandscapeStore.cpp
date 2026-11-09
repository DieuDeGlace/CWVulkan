#include "alphagen/LandscapeStore.h"
#include <stdexcept>
#include <cmath>

namespace alphagen {
namespace {
// Allocated vector payloads; caller leases, in-flight work and allocator overhead
// are separate from the cache's ready-stage budget.
std::size_t landscapeBytes(const LandscapeStage &stage) {
    const auto allocated = [](const auto &values) { return values.capacity() * sizeof(values[0]); };
    const auto &zone = *stage.zone;
    std::size_t bytes = sizeof(stage) + sizeof(zone) + allocated(zone.columns) + allocated(zone.plants) +
        allocated(zone.trees) + allocated(zone.items) + allocated(zone.statics) + allocated(zone.objectList) +
        allocated(zone.spawns) + allocated(zone.sprites) + allocated(zone.drops) + allocated(zone.markers) +
        allocated(zone.leafSources) + allocated(zone.songPosts) + allocated(zone.houses) +
        allocated(zone.townPlots) + allocated(zone.shadedLight);
    for (const auto &column : zone.columns)
        bytes += allocated(column.blocks);
    for (const auto &spawn : zone.spawns)
        bytes += allocated(spawn.inventory) + allocated(spawn.schedule) + allocated(spawn.route) + allocated(spawn.specials);
    for (const auto &house : zone.houses)
        bytes += allocated(house.storeys) + allocated(house.seats) + allocated(house.beds);
    return bytes;
}
} // namespace
LandscapeStore::LandscapeStore(uint32_t seed, alphacoords::Mapping frame, std::size_t cachedParents, bool named,
                               std::shared_ptr<const TreeModels> palms, std::size_t cachedBytes)
    : world(seed), terrain(world, std::move(palms)), frame(frame), cache(cachedParents, cachedBytes), foliage(cachedParents) {
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
    }, landscapeBytes);
}
std::shared_ptr<const VegetationStage> LandscapeStore::vegetationParent(int32_t zx, int32_t zy) {
    if (zx < 0 || zy < 0 || zx >= 65536 || zy >= 65536)
        return {};
    return foliage.get(zx, zy, [&] {
        const auto landscape = parent(zx, zy);
        return std::make_unique<VegetationStage>(terrain.vegetation(*landscape->zone, landscape->random));
    });
}
std::vector<Plant> LandscapeStore::plantsForTile(int32_t zx, int32_t zy) {
    std::vector<Plant> result;
    const auto covered = coverage(zx, zy);
    if (!covered)
        return result;
    for (size_t i = 0; i < covered->count; ++i) {
        const auto &patch = covered->patches[i];
        const auto plants = vegetationParent(patch.zoneX, patch.zoneY);
        const int32_t ax = patch.zoneX * 256 + patch.alphaX, ay = patch.zoneY * 256 + patch.alphaY;
        for (const auto &plant : plants->plants) {
            if (plant.x < ax || plant.y < ay || plant.x >= ax + patch.width || plant.y >= ay + patch.height)
                continue;
            auto mapped = plant;
            mapped.x = int32_t(int64_t(zx) * 64 + patch.sourceX + plant.x - ax);
            mapped.y = int32_t(int64_t(zy) * 64 + patch.sourceY + plant.y - ay);
            result.push_back(mapped);
        }
    }
    return result;
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
std::optional<alphacoords::Point> LandscapeStore::preparedPoint(int32_t x, int32_t y) const {
    const auto point = frame.point(x, y);
    if (!point)
        return std::nullopt;
    // Match the surface stage's preparation window without allocating voxel zones.
    for (int i = -1; i <= 1; ++i)
        for (int j = -1; j <= 1; ++j)
            terrain.prepareRegion(point->x / REGION + i, point->y / REGION + j);
    return point;
}
std::optional<float> LandscapeStore::height(int32_t x, int32_t y) {
    const auto point = preparedPoint(x, y);
    return point ? std::optional<float>(terrain.height(point->x, point->y)) : std::nullopt;
}
std::optional<TerrainSample> LandscapeStore::sample(int32_t x, int32_t y) {
    const auto point = preparedPoint(x, y);
    if (!point)
        return std::nullopt;
    const auto ax = point->x, ay = point->y;
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
std::optional<glm::vec3> LandscapeStore::rockColour(int32_t x, int32_t y, int32_t z) {
    const auto point = frame.point(x, y);
    if (!point)
        return std::nullopt;
    terrain.prepareRegion(point->x / REGION, point->y / REGION);
    return terrain.rock(point->x, point->y, z);
}
std::optional<Block> LandscapeStore::rock(int32_t x, int32_t y, int32_t z) {
    const auto colour = rockColour(x, y, z);
    if (!colour)
        return std::nullopt;
    return Block{uint8_t(int32_t(colour->r)), uint8_t(int32_t(colour->g)), uint8_t(int32_t(colour->b)), ROCK};
}
std::optional<alphacoords::Coverage> LandscapeStore::coverage(int32_t zx, int32_t zy) const {
    return frame.zone(zx, zy);
}
} // namespace alphagen
