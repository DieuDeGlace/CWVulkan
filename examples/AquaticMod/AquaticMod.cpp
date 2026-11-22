// Example CubeVulkan world mod (mod API version 2): the aquatic encounters add-on as a mod.
// Built with the engine (target AquaticMod) into build/examples/AquaticMod.dll. It runs the same
// rules as the built-in add-on (src/AquaticPlan.cpp, compiled into this DLL), so install it only
// with the built-in one switched off ([Creatures] AquaticEnemies=0), or both will make schools.
#include <cubevulkan/ModApi.h>

#include "AquaticPlan.h"

#include <cstdio>

static const CwvApi *api;
static aquatic::Planner planner;

static void onWorldStep(const CwvWorld *w, double milliseconds) {
    aquatic::Env env;
    env.zoneLoaded = [w](int32_t zx, int32_t zy) { return w->zone_loaded(zx, zy) != 0; };
    env.blockType = [w](int32_t x, int32_t y, int32_t z, bool &loaded) {
        int ok = 0;
        const uint8_t type = w->block_type(x, y, z, &ok);
        loaded = ok != 0;
        return type;
    };
    env.ground = [w](int32_t x, int32_t y) { return w->ground(x, y); };
    env.ocean = [w](int32_t x, int32_t y) { return w->biome(x, y) == CWV_BIOME_OCEAN; };
    int made = 0;
    env.make = [w, &made](const aquatic::School &school, const glm::dvec3 &at) {
        const double position[3] = {at.x, at.y, at.z};
        const int64_t id = w->spawn_creature(school.race, school.hostility, school.stars, position);
        made += id != 0;
        return id;
    };
    env.addFlags = [w](int64_t id, uint32_t flags) { w->add_flags(id, flags); };
    double players[16][3];
    const int n = w->players(players, 16);
    std::vector<glm::dvec3> at;
    for (int i = 0; i < n; ++i)
        at.push_back(glm::dvec3(players[i][0], players[i][1], players[i][2]));
    planner.update(env, at, w->seed, milliseconds);
    if (made) {
        char line[64];
        std::snprintf(line, sizeof line, "made %d sea creatures", made);
        api->log(line);
    }
}

extern "C" CWV_EXPORT int cwv_mod_load(const CwvApi *a, CwvMod *mod) {
    if (a->version < 2)
        return 0; // needs the world step
    api = a;
    mod->name = "AquaticMod";
    mod->on_world_step = onWorldStep;
    mod->changes_world = 1;
    mod->version = "1";
    api->log("Aquatic encounters running as a mod");
    return 1;
}
