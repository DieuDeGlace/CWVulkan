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
// using a field added after CWV_API_VERSION 1.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CWV_API_VERSION 1
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

typedef struct CwvMod {
    const char *name;
    // Called once per frame with the frame time in seconds.
    void (*on_update)(double seconds);
    // Called when a key is pressed (GLFW key code). Return nonzero to consume it.
    int (*on_key)(int key);
    void (*on_shutdown)(void);
} CwvMod;

typedef int (*CwvModLoad)(const CwvApi *api, CwvMod *mod);

#ifdef __cplusplus
}
#endif
