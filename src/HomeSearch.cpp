#include "HomeSearch.h"
#include "GameMath.h"
#include "worldgen/Generator.h"
#include "worldgen/Noise.h"
#include <bit>
#include <cmath>

namespace homesearch {
namespace {
int32_t blockAt(int32_t cell, int32_t offset) {
    return std::bit_cast<int32_t>(uint32_t(cell) * 16u + uint32_t(offset));
}
} // namespace
bool World::ready() const {
    return areaCell && cellObject && ground && region && regionType && neighbours && place && weight && roads &&
           channel && excludedPlace;
}
World nativeWorld() {
    World world;
    world.areaCell = [](int32_t x, int32_t y) {
        const auto p = worldgen::areaLandmark(x, y);
        return Cell{p.x, p.y};
    };
    world.cellObject = worldgen::cellObject;
    world.ground = [](int32_t x, int32_t y) { return worldgen::groundHeight(x, y, 0); };
    world.region = [](int32_t x, int32_t y) {
        const auto p = worldgen::regionAt(x, y);
        return Cell{p.x, p.y};
    };
    world.regionType = worldgen::regionBiome;
    world.neighbours = worldgen::neighbourhood;
    world.place = worldgen::placeAt;
    world.weight = [](Weight which, const worldgen::Neighbourhood &n, const worldgen::Place &p) {
        switch (which) {
        case Weight::LavaRegion:
            return worldgen::lavaRegion(n, p.kind, p.weight);
        case Weight::ForestRegion:
            return worldgen::forestRegion(n, p.kind, p.weight);
        case Weight::LakeRegion:
            return worldgen::lakeRegion(n);
        case Weight::DryRegion:
            return worldgen::dryRegion(n);
        case Weight::WetRegion:
            return worldgen::wetRegion(n);
        }
        return 0.f;
    };
    world.roads = [](int32_t x, int32_t y, const worldgen::Place &p) {
        return worldgen::roads(x, y, p.kind, p.weight);
    };
    world.channel = [](Channel which, int32_t x, int32_t y, const Terrain &t) {
        switch (which) {
        case Channel::River:
            return worldgen::river(x, y, t.roads, t.lavaRegion, t.forestRegion, t.wet, t.place);
        case Channel::Lava:
            return worldgen::lava(x, y, t.roads, t.lavaRegion, t.lake, t.place);
        case Channel::Swamp:
            return worldgen::swamp(x, y, t.roads, t.forestRegion, t.lake, t.place);
        }
        return 0.f;
    };
    world.excludedPlace = [](int32_t x, int32_t y) {
        const auto plan = worldgen::regionPlan(x, y); // borrow only the needed value; no role/item vector copies
        return Cell{plan->specialTown.x, plan->specialTown.y};
    };
    return world;
}
bool fits(const World &world, Cell home) {
    if (!world.ready() || (world.playable && !world.playable(home)))
        return false;
    if (world.cellObject(home[0], home[1]) != 0x35)
        return false;
    const int32_t x = blockAt(home[0], 8), y = blockAt(home[1], 40);
    if (world.ground(x, y) < 0.f)
        return false;
    const Cell region = world.region(x, y);
    const int type = world.regionType(region[0], region[1]);
    if (type != 2 && type != 7 && type != 0)
        return false;
    const auto neighbours = world.neighbours(x, y);
    Terrain terrain;
    terrain.place = world.place(x, y);
    if (terrain.place.kind != 5)
        return false;
    terrain.lavaRegion = world.weight(Weight::LavaRegion, neighbours, terrain.place);
    if (terrain.lavaRegion > 0.f)
        return false;
    terrain.forestRegion = world.weight(Weight::ForestRegion, neighbours, terrain.place);
    if (terrain.forestRegion > 0.f)
        return false;
    terrain.lake = world.weight(Weight::LakeRegion, neighbours, terrain.place);
    if (terrain.lake > 0.f)
        return false;
    terrain.dryRegion = world.weight(Weight::DryRegion, neighbours, terrain.place);
    if (terrain.dryRegion > 0.f)
        return false;
    terrain.wet = world.weight(Weight::WetRegion, neighbours, terrain.place);
    terrain.roads = world.roads(x, y, terrain.place);
    if (world.channel(Channel::River, x, y, terrain) > .4f)
        return false;
    if (world.channel(Channel::Lava, x, y, terrain) > .4f)
        return false;
    if (world.channel(Channel::Swamp, x, y, terrain) > .4f)
        return false;
    const float left = world.ground(gamemath::add32(x, -8), y);
    const float right = world.ground(gamemath::add32(x, 8), y);
    if (std::abs(left - right) > 4.f)
        return false;
    const float down = world.ground(x, gamemath::add32(y, -8));
    const float up = world.ground(x, gamemath::add32(y, 8));
    if (std::abs(down - up) > 4.f)
        return false;
    Cell cell{worldgen::floorDiv(x, 10), worldgen::floorDiv(y, 10)};
    if (terrain.place.big) {
        cell[0] -= cell[0] & 1;
        cell[1] -= cell[1] & 1;
    }
    const Cell excluded = world.excludedPlace(region[0], region[1]);
    return excluded[0] != cell[0] || excluded[1] != cell[1];
}
bool find(const World &world, const std::function<int()> &random, Cell origin, int radiusFrom, int radiusSpread,
          int maxAttempts, Cell &out) {
    if (!world.ready() || !random)
        return false;
    const double angle = (double(random()) * 6.283185307179586) / 32767.0;
    const int32_t radial = random(); // distinct second direct sample, also for the explicit near-search adapter
    const int32_t radius = gamemath::add32(radiusFrom, radiusSpread > 0 ? radial % radiusSpread : 0);
    int32_t x = gamemath::add32(origin[0], worldgen::trunc32(double(radius) * std::cos(angle)));
    const int32_t y = gamemath::add32(origin[1], worldgen::trunc32(double(radius) * std::sin(angle)));
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        x = gamemath::add32(x, 1);
        out = world.areaCell(x, y); // the caller's output pair is written before each candidate's queries
        if (fits(world, out))
            return true;
    }
    return false; // explicit engine abort bound, not a native termination assertion
}
Cell block(Cell home) { return {blockAt(home[0], 8), blockAt(home[1], 32)}; }
std::array<int64_t, 3> position(Cell home, float height) {
    const Cell at = block(home);
    const float fixedHeight = height * 65536.f;
    return {int64_t(at[0]) * 65536, int64_t(at[1]) * 65536, worldgen::trunc64(fixedHeight)};
}
} // namespace homesearch
