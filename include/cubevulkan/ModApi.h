#pragma once
// CubeVulkan mod API (C ABI). A mod is a DLL in `mods/` that exports
//
//     CWV_EXPORT int cwv_mod_load(const CwvApi *api, CwvMod *mod);
//
// It fills `mod` and returns nonzero to stay loaded. The engine calls the
// callbacks on the main thread. Asset mods need no code at all: a folder
// `mods/<Name>/sprites/*.cub` (or `music/*.ogg`, `sounds/*.wav`) overrides the
// Steam file of the same name. Mods load in alphabetical order; later ones win.
//
// Compatibility: fields are only ever appended. Check `api->version` before
// using a field added after CWV_API_VERSION 1. Version 2 adds world logic on
// the host (CwvMod.on_world_step with the CwvWorld services); see
// docs/modding-api.md. The engine zero-fills CwvMod, so a version 1 mod simply
// leaves the version 2 callbacks unset.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CWV_API_VERSION 2
// Engine biome numbers returned by CwvWorld.biome.
#define CWV_BIOME_OCEAN 1
// Engine block type byte (CwvWorld.block_type): 0 air, 2 water.
#define CWV_BLOCK_AIR 0
#define CWV_BLOCK_WATER 2
// Creature flag bits (CwvWorld.add_flags): a fish swims; a boss is drawn and fights as one.
#define CWV_FLAG_FISH 0x100u
#define CWV_FLAG_BOSS 0x40000u
// Creature hostility kinds (CwvWorld.spawn_creature).
#define CWV_HOSTILE 1
#define CWV_NEUTRAL 4
#ifdef _WIN32
#define CWV_EXPORT __declspec(dllexport)
#else
#define CWV_EXPORT __attribute__((visibility("default")))
#endif

typedef int32_t CwvModel;    // Loaded .cub model; negative on failure.
typedef int32_t CwvInstance; // Placed model; negative on failure.

typedef struct CwvApi {
    uint32_t version;
    // Writes a line to CubeVulkan.log, prefixed with the mod name.
    void (*log)(const char *message);
    // Reads `[section] key` from CubeVulkan.ini or mods/<Name>.ini (mod first).
    const char *(*config)(const char *section, const char *key, const char *fallback);

    // Loads a sprite by file name without extension ("water-lily01"), using
    // mod overrides. Repeated calls return the same handle.
    CwvModel (*load_model)(const char *name);
    // Places a model. Position is in blocks (z up), yaw in radians; `scale`
    // is blocks per voxel (vegetation uses 1/16). Instances cast shadows.
    CwvInstance (*spawn)(CwvModel model, float x, float y, float z, float yaw, float scale);
    void (*move)(CwvInstance instance, float x, float y, float z, float yaw);
    void (*remove)(CwvInstance instance);

    // Terrain surface height (top of the highest solid block) at a column.
    float (*terrain_height)(float x, float y);
    // Water depth at a column, 0 on land.
    float (*water_depth)(float x, float y);

    // World clock in hours [0, 24).
    float (*get_hour)(void);
    void (*set_hour)(float hour);
    // Camera focus (player) position in blocks.
    void (*get_focus)(float out[3]);
} CwvApi;

// Version 2: the world that decides (the host's), valid only during on_world_step.
// Positions are absolute blocks (z up). Creature ids are the engine's.
typedef struct CwvWorld {
    int32_t seed; // the world's seed
    // Fills up to `max` player positions; returns how many.
    int (*players)(double (*out)[3], int max);
    // Whether the zone (64 x 64 blocks; zone = floor(block / 64)) is loaded.
    int (*zone_loaded)(int32_t zone_x, int32_t zone_y);
    // A block's type byte; *loaded is set to 0 when its zone is not in memory.
    uint8_t (*block_type)(int32_t x, int32_t y, int32_t z, int *loaded);
    // Ground height and biome number at a column.
    float (*ground)(int32_t x, int32_t y);
    int32_t (*biome)(int32_t x, int32_t y);
    // Makes a creature; returns its id, or 0 when it could not be made (zone not loaded).
    int64_t (*spawn_creature)(int32_t race, int32_t hostility, int32_t level, const double position[3]);
    // Sets flag bits on a creature; returns 0 when it does not exist.
    int (*add_flags)(int64_t id, uint32_t flags);
} CwvWorld;

typedef struct CwvMod {
    const char *name;
    // Called once per frame with the frame time in seconds.
    void (*on_update)(double seconds);
    // Called when a key is pressed (GLFW key code). Return nonzero to consume it.
    int (*on_key)(int key);
    void (*on_shutdown)(void);
    // Version 2. Called once per host world step, under the host's lock, with the
    // step's milliseconds. Never called on a guest's mirror world.
    void (*on_world_step)(const CwvWorld *world, double milliseconds);
    // Version 2. Nonzero when the mod changes the world (creatures, blocks):
    // multiplayer peers must then run the same mods (name and `version`).
    int changes_world;
    const char *version;
} CwvMod;

typedef int (*CwvModLoad)(const CwvApi *api, CwvMod *mod);

#ifdef __cplusplus
}
#endif
