#pragma once
// Per-object World prelude from steam-world-traps.md and steam-dungeon-live-gate-guard-counts.md.
#include "NativeAI.h"
#include "NativeUpkeep.h"
#include "SoundRules.h"
#include <array>
#include <functional>
#include <optional>
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
    // Snapshot of the admitted players in registration order, resolved at use.
    std::vector<int64_t> gatePlayers;
    std::function<const uint8_t *(int64_t)> actor;
    std::function<std::optional<int32_t>(Cell)> gateThreshold;
    std::function<int32_t(Cell zone, Cell gate)> guardCount;
    std::function<uint8_t(Cell)> castleSteps;
    std::function<nativeai::Block(int, int, int)> block;
};

// Clear0x2002 on all current actors, collect kind0 players, then visit their7x7
// zone lookup keys. The caller does lookup-only collection and unsigned Zone-pointer
// sorting/deduplication while retaining each selected zone's lifetime.
void collectZones(World &world, const std::function<void(Cell)> &lookup);

// Shared clock/personal-state portion, also used by clock-only storage callers.
void advanceClock(nativeai::Object &object, float milliseconds, const std::set<Cell> *usedCells);
// Complete one object's clock, personal state, gate or trap update.
void step(World &world, nativeai::Object &object, float milliseconds);

} // namespace nativeobjects
