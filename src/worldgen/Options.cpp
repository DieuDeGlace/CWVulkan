// CubeVulkan's own generator options: the world scale and the mods' hooks (Options.h).
#include "worldgen/Options.h"

#include "worldgen/Noise.h"

#include <algorithm>
#include <atomic>

namespace worldgen {
namespace {

double scale = 1.0;
ModHooks hooks;
bool anyHook = false;
std::atomic<uint32_t> epoch{1};
thread_local int originalDepth = 0;

// While a hook runs it sees the game's own generation.
struct InHook {
    InHook() { originalDepth++; }
    ~InHook() { originalDepth--; }
};

// The hooked place kinds of the last cells asked for on this thread. A hook may be slow (it can
// ask the game's planner), and the height of every column asks for its place's kind.
struct HookedKinds {
    struct Entry {
        int32_t cx = 0, cy = 0;
        int8_t big = -1;
        int16_t kind = 0;
    };
    uint32_t epoch = 0;
    Entry entries[512];
};
thread_local HookedKinds hookedKinds;

} // namespace

void setWorldScale(double s) {
    scale = std::clamp(s, 1.0, 4.0);
    epoch++;
}
double worldScale() { return scale; }

void setModHooks(const ModHooks &h) {
    hooks = h;
    anyHook = (bool)h.placeKind || (bool)h.role || (bool)h.generatedItem;
    epoch++;
}
bool modHooksSet() { return anyHook; }

OriginalGeneration::OriginalGeneration() { originalDepth++; }
OriginalGeneration::~OriginalGeneration() { originalDepth--; }
bool generatingOriginal() { return !anyHook || originalDepth > 0; }

int hookedPlaceKind(int32_t cx, int32_t cy, bool big, int kind) {
    if (!hooks.placeKind || originalDepth > 0)
        return kind;
    HookedKinds &cache = hookedKinds;
    if (cache.epoch != epoch.load(std::memory_order_relaxed)) {
        cache = {};
        cache.epoch = epoch.load(std::memory_order_relaxed);
    }
    HookedKinds::Entry &e = cache.entries[(cellKey(cx, cy) * 2u + (big ? 1u : 0u)) & 511u];
    if (e.big != (big ? 1 : 0) || e.cx != cx || e.cy != cy) {
        InHook in;
        e = {cx, cy, (int8_t)(big ? 1 : 0), (int16_t)hooks.placeKind(cx, cy, big, kind)};
    }
    return e.kind;
}

int hookedRole(int32_t px, int32_t py, int role) {
    if (!hooks.role || originalDepth > 0)
        return role;
    InHook in;
    return hooks.role(px, py, role);
}

bool keepsGeneratedItem(int category, int id) {
    if (!hooks.generatedItem || originalDepth > 0)
        return true;
    InHook in;
    return hooks.generatedItem(category, id);
}

uint32_t optionsVersion() { return epoch.load(std::memory_order_relaxed); }

} // namespace worldgen
