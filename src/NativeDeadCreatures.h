#pragma once
// Dead-creature areas, terrain destruction and lifecycle from steam-world-death-area.md.
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
    uint32_t day = 0;
    std::function<nativeai::Block(int32_t, int32_t, int32_t)> block;
    std::function<void(int64_t, int64_t, int64_t)> removeBlock;
    std::function<void(int32_t, int32_t, int32_t, int32_t)> rebuild;
    std::function<void(const std::array<uint8_t, 0x20> &)> blockChange;
    std::function<void(const std::array<uint8_t, 0x30> &)> sound;

    // Append directly to the common lists so nested hit outputs keep native ordering.
    using Record = std::array<uint8_t, 0x48>;
    std::function<void(const Record &)> burst, number, apply;
    std::function<void(const nativeupkeep::Hit &)> hit;
};

// Visits first-clock-zero areas, advances clocks and revives eligible owned kind6 actors.
// An unconfigured terrain adapter preserves race0x90 input until its helpers exist.
// Retirement IDs are returned in registration order after every source was visited
// and its aggro/damage references cleared. The caller then erases those actors and
// rebuilds its borrowed views; no pointer to shifted vector storage is returned.
std::vector<int64_t> step(World &world, float milliseconds);
} // namespace nativedeath
