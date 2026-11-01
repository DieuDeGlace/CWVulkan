#pragma once
// Per-object World prelude from docs/specs/steam-world-traps.md.
// Other object-family callbacks and the separate area pass remain outside this module.
#include "NativeAI.h"
#include "NativeUpkeep.h"
#include "SoundRules.h"
#include <array>
#include <functional>
#include <set>
#include <vector>

namespace nativeobjects {

using Cell = std::array<int32_t, 2>;

struct World {
    bool client = false;
    float clockMs = 0; // World+0x440, already advanced by this substep's time prelude
    // Borrow the current registration-order view; callback tail births remain visible.
    const std::vector<nativeai::Creature *> *creatures = nullptr;
    const std::set<Cell> *usedCells = nullptr; // nonnull means a local player exists
    std::function<int()> rand;
    std::function<float(const nativeai::Creature &)> maxHealth;
    std::function<void(nativeai::Creature &, const nativeupkeep::Hit &)> hit;
    std::function<void(const soundrules::RequestRecord &)> sound;
    std::function<void()> objectsChanged;
};

// Clear0x2002 on all current actors, collect kind0 players, then visit their7x7
// zone lookup keys. The caller does lookup-only collection and unsigned Zone-pointer
// sorting/deduplication while retaining each selected zone's lifetime.
void collectZones(World &world, const std::function<void(Cell)> &lookup);

// Shared clock/personal-state portion, also used by clock-only storage callers.
void advanceClock(nativeai::Object &object, float milliseconds, const std::set<Cell> *usedCells);
// Complete one object's clock/personal-state and specified0x11/0x12/0x13 tail.
void step(World &world, nativeai::Object &object, float milliseconds);

} // namespace nativeobjects
