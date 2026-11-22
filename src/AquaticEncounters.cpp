#include "AquaticEncounters.h"

#include "Extracted.h"
#include "Spawned.h"
#include "worldgen/Height.h"
#include "worldgen/Regions.h"
#include "worldgen/Residents.h"

#include <cstring>

namespace aquatic {

void Manager::update(SpawnedCreatures &world, ExtractedWorld &terrain, const std::vector<glm::dvec3> &players, int32_t seed, double elapsedMs) {
    Env env;
    env.zoneLoaded = [&](int32_t zx, int32_t zy) {
        bool loaded = false;
        terrain.gameBlock(int64_t(zx) * 64 + 32, int64_t(zy) * 64 + 32, 0, &loaded);
        return loaded;
    };
    env.blockType = [&](int32_t x, int32_t y, int32_t z, bool &loaded) { return terrain.gameBlock(x, y, z, &loaded)[4]; };
    env.ground = [](int32_t x, int32_t y) { return worldgen::groundHeight(x, y); };
    env.ocean = [](int32_t x, int32_t y) { return worldgen::biomeAt(x, y) == OCEAN_BIOME; };
    env.make = [&](const School &school, const glm::dvec3 &at) -> int64_t {
        const int64_t dots[3] = {int64_t(at.x * 65536), int64_t(at.y * 65536), int64_t(at.z * 65536)};
        const int64_t id = world.make(school.race, school.hostility, school.stars, dots, worldgen::regionOf(int32_t(at.x), int32_t(at.y)));
        return id && world.record(id) ? id : 0;
    };
    env.addFlags = [&](int64_t id, uint32_t add) {
        if (uint8_t *r = world.record(id)) {
            uint32_t flags;
            std::memcpy(&flags, r + worldgen::creature::FLAGS, sizeof flags);
            flags |= add;
            std::memcpy(r + worldgen::creature::FLAGS, &flags, sizeof flags);
        }
    };
    planner.update(env, players, seed, elapsedMs);
}

} // namespace aquatic
