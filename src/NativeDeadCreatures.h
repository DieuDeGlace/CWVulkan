#pragma once
// Dead-creature lifecycle and non-terrain areas from steam-world-death-area.md.
// Race0x90 first visits stay deferred until the shared terrain/scenery helpers are wired.
#include "NativeAI.h"
#include "NativeUpkeep.h"
#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace nativedeath {
struct World {
    bool client = false;
    const std::vector<nativeai::Creature *> *creatures = nullptr;
    std::function<nativeai::Creature *(int64_t)> byId;
    std::function<void(nativeai::Creature &)> markDead;
    std::function<float(const nativeai::Creature &)> maxHealth;
    std::function<bool(const nativeai::Creature &, const nativeai::Creature &)> hostile;
    std::function<int()> rand;
    // Append directly to the common lists so nested hit outputs keep native ordering.
    using Record = std::array<uint8_t, 0x48>;
    std::function<void(const Record &)> burst, number, apply;
    std::function<void(const nativeupkeep::Hit &)> hit;
};

// Visits non-terrain areas, advances corpse clocks and revives eligible owned kind6 actors.
// Retirement IDs are returned in registration order after every source was visited
// and its aggro/damage references cleared. The caller then erases those actors and
// rebuilds its borrowed views; no pointer to shifted vector storage is returned.
std::vector<int64_t> step(World &world, float milliseconds);
} // namespace nativedeath
