#pragma once
// CubeVulkan's own options for the world generator, which the game doesn't have. Both default to
// the game's own behaviour.
//
// World scale ([World] Scale, 1 to 4, default 1): regions and everything sized by them (biomes,
// landmasses, oceans, their blends and the region-sized noise) grow wider, while places, roads,
// rivers and blocks keep the game's sizes. It plugs in at the start of the region lookups
// (worldgen-terrain.md §2.6): the point is pulled toward the origin before the warp, and the
// distances to region centres are given back in full-size blocks. At 1 nothing changes.
//
// Mod hooks: as the mod loader lets a mod stand in for a game function, a hook answers in the
// game's place, given what the game made. They plug in at the place kind (0x2b99d0), the region
// planner's role lookup (0x93480) and the items generation lays down (role items and region
// boats). A hook replaces an answer, never a random draw (worldgen-creatures.md "Conventions").
// While a hook runs, or while an OriginalGeneration lives on the thread, generation is the
// game's own, so a hook can ask what the game would have made.
#include <cstdint>
#include <functional>

namespace worldgen {

// --- World scale -----------------------------------------------------------------------------

// Set before generating anything; clamped to 1..4.
void setWorldScale(double scale);
double worldScale();

// --- Mod hooks -------------------------------------------------------------------------------

struct ModHooks {
    // The landmark kind of a place cell (0x2b99d0), given the game's.
    std::function<int(int32_t cx, int32_t cy, bool big, int kind)> placeKind;
    // The region planner's role of a place (0x93480), given the game's.
    std::function<int(int32_t px, int32_t py, int role)> role;
    // Whether an item generation lays down (a role item, a region's boat) stays: false leaves it
    // out, after its draws.
    std::function<bool(int category, int id)> generatedItem;
};
// Set before generation begins.
void setModHooks(const ModHooks &hooks);
// Whether any hook is set.
bool modHooksSet();

// The game's own generation on this thread while it lives.
class OriginalGeneration {
public:
    OriginalGeneration();
    ~OriginalGeneration();
    OriginalGeneration(const OriginalGeneration &) = delete;
    OriginalGeneration &operator=(const OriginalGeneration &) = delete;
};
// Whether this thread generates as the game does (no hook set, or an OriginalGeneration alive).
bool generatingOriginal();

// The hooked answers (the game's when the hook is unset or the generation is the original).
int hookedPlaceKind(int32_t cx, int32_t cy, bool big, int kind);
int hookedRole(int32_t px, int32_t py, int role);
bool keepsGeneratedItem(int category, int id);

// A number that changes whenever an option changes: caches of generated answers compare it to
// know when to start over. Caches of answers that depend on the hooks also key on
// generatingOriginal().
uint32_t optionsVersion();

} // namespace worldgen
