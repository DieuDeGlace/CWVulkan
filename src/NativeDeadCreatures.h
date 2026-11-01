#pragma once
// Ordinary dead-creature lifecycle from steam-world-death-area.md sections1/6.
// The three first-visit area-source races stay deferred until their area pass is wired.
#include "NativeAI.h"
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
};

// Advances ordinary nonplayer corpses and revives eligible owned kind6 actors.
// Retirement IDs are returned in registration order after every source was visited
// and its aggro/damage references cleared. The caller then erases those actors and
// rebuilds its borrowed views; no pointer to shifted vector storage is returned.
std::vector<int64_t> step(World &world, float milliseconds);
} // namespace nativedeath
