#pragma once
// The ability step (the game's 0x2c5250), written from docs/specs/creature-abilities.md and
// creature-abilities-ranged.md. The world's update runs it for every living creature, after
// its physics and before its interactions. It looks at what the creature is doing (its
// ability, +0x68, and how far into it it is, +0x6c) and does that ability's work for the
// frame: swings that break blocks and strike creatures, the sounds and particles of
// charging and casting, shots, arrows and areas (effect objects, stepped by NativeEffects),
// rays and beams, healing, using items, dashes, blinks, flights, summons and taming. Every
// call then ends the same way: a combo runs out after 4 s without a landed hit, and the
// skills' cooldowns run down.
//
// It works on game-layout creature records (nativepose::CREATURE_SIZE bytes) with the
// containers the game keeps beside them (nativeai::Creature). What it needs of the world
// comes through `World`; what it makes is appended to the world's lists there.
#include "NativeAI.h"
#include "NativeEffects.h"
#include "NativeUpkeep.h"

#include <array>
#include <cstdint>
#include <functional>
#include <list>
#include <set>
#include <utility>
#include <vector>

namespace nativeability {

// The world as the step sees it. Callbacks may be left unset: an unset one answers as an
// empty world would (nobody by id, the default block, rand 0, no hit lands), and the
// branches that need one that is missing do nothing.
struct World {
    bool client = false;                // world+0x358: this world mirrors a server
    const uint8_t *local = nullptr;     // world+0x448: the local player's record
    std::vector<nativeai::Creature *> creatures;          // world+0x40, in the world's order
    std::function<nativeai::Creature *(int64_t id)> byId; // world+0x450
    std::function<int()> rand;                            // the game's shared rand(), 0..32767

    // Blocks: one by world position (0xeab0), one by block coordinates with whether a zone
    // (64 blocks square) is loaded (for sight 0x2b5d90 and the block ray 0x2d1d20), and the
    // two writes (0x2a59f0 removes a block; 0x2e0710 drops what destroyed blocks left floating).
    std::function<nativeai::Block(int64_t x, int64_t y, int64_t z)> blockAt;
    std::function<nativeai::Block(int x, int y, int z)> block;
    std::function<bool(int zx, int zy)> zone;
    std::function<void(int x, int y, int z)> removeBlock;
    std::function<void(int x0, int y0, int x1, int y1)> rebuild;
    // 0x2b77c0: whether a creature of `size` (+0x84..+0x8c) at `pos` would overlap a solid
    // block (`pass`: it may pass through what buff 0x13 lets through). Used by the blink.
    std::function<bool(const int64_t pos[3], const float size[3], bool pass)> overlaps;

    // The world generator: 0x35ea0 (the terrain's height of a block column), 0x8a140 (the
    // land's height with its place, as the eagle flies over it), 0x69c60 (the object of a
    // 16-block cell) and 0x2d87f0 (the height a cell's building stands at).
    std::function<float(int x, int y)> ground;
    std::function<float(int x, int y)> elevation;
    std::function<int(int cx, int cy)> cellObject;
    std::function<int(int cx, int cy)> cellHeight;
    // A loaded zone's objects (zone+0x60), or null.
    std::function<std::vector<nativeai::Object> *(int zx, int zy)> objects;
    // The local player's map of visited cells (its +0x1528 structure, +0xa8): inserts the
    // cell and says whether it was new.
    std::function<bool(int cx, int cy)> discover;

    // 0x29eb60: applies a hit and says whether it landed (every hit asked for is also kept
    // in `hits`); 0x29dcf0: applies a damage number (a heal).
    std::function<bool(const nativeupkeep::Hit &)> hit;
    std::function<void(const std::array<uint8_t, 0x48> &)> heal;

    // Creatures made or changed by summoning and taming. `construct` (0x4ce80) returns a new
    // creature with the constructor's defaults and the id given, kept by the world but not
    // in it yet; `added` (0x29d0d0) then puts it at the end of the world's list and id map.
    std::function<nativeai::Creature *(int64_t id)> construct;
    std::function<void(nativeai::Creature *made)> added;
    std::function<void(int64_t id)> markDead;                            // world+0x20: the dead ids
    std::function<void(int64_t oldId, nativeai::Creature *c)> renumbered; // 0x2ac530, 0x29bf70: the id map
    std::function<void(int64_t ownerId, nativeai::Creature *pet)> adopt;  // 0x2b67b0: becomes a pet

    // What the step tells the world: sounds (world+0xa0), particle bursts (+0xb0), damage
    // numbers (+0x90), buff notices (+0x150), changed blocks (+0xc0), the hits asked for,
    // effect objects (+0x60, and their copies for the network +0xd0) and zones whose
    // objects changed (+0x1a0).
    std::vector<std::array<uint8_t, 0x30>> sounds, buffNotes;
    std::vector<std::array<uint8_t, 0x48>> particles, numbers;
    std::vector<std::array<uint8_t, 0x20>> blockChanges;
    std::vector<nativeupkeep::Hit> hits;
    uint32_t changeCount = 0; // world+0x08, stamped on each changed block
    std::list<nativeeffects::Effect> *effects = nullptr;
    std::list<nativeeffects::Effect> *sentEffects = nullptr;
    std::set<std::pair<int, int>> changedZones;
};

// One frame of the ability step for `c`, `dt` milliseconds long.
void ability(World &world, nativeai::Creature *c, float dt);

// How long the eagle takes from one block to another (0x2adc50): 2.5 ms a block, 10 to 30 s.
int flightTime(int32_t fromX, int32_t fromY, int32_t toX, int32_t toY);

} // namespace nativeability
