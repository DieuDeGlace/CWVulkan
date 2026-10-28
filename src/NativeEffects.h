#pragma once
// The game's effect objects (world+0x60) and their step (0x2dba40), run once a world update
// after every creature's (docs/specs/effect-objects.md): arrows and bolts (kinds 0-3, 7),
// homing shots (4, 10), a boomerang that returns to its owner (5), lasting areas (6), and
// things that drift (8) or hover over the ground (9). Each moves, meets the creatures in its
// way through the game's hit (0x29eb60), breaks the blocks it strikes that break, bursts,
// and runs out. Creating them is the abilities' work (NativeAbility).
#include "NativeAI.h"
#include "NativeUpkeep.h"

#include <array>
#include <cstdint>
#include <functional>
#include <list>
#include <vector>

namespace nativeeffects {

// An effect object as the game keeps it (0x80 bytes, made by 0x29a0b0): its owner (+0),
// position (+8), target point (+0x20), velocity (+0x44, blocks a second), damage (+0x50),
// contact radius (+0x54), size (+0x58), charge (+0x5c), "no damage on contact" flag (+0x64),
// kind (+0x68), hit type (+0x6c), age (+0x70, ms), lifetime (+0x74, ms) and the freeze time
// it gives what it strikes (+0x78).
using Effect = std::array<uint8_t, 0x80>;
Effect made(); // 0x29a0b0's defaults

// The world as the step sees it. Callbacks other than byId and blockAt may be left unset.
struct World {
    bool client = false;                // world+0x358: this world mirrors a server
    const uint8_t *local = nullptr;     // world+0x448: the local player's record
    std::vector<nativeai::Creature *> creatures;          // world+0x40
    std::function<nativeai::Creature *(int64_t id)> byId; // world+0x450
    std::function<nativeai::Block(int64_t x, int64_t y, int64_t z)> blockAt; // 0xeab0 (world position)
    std::function<nativeai::Block(int x, int y, int z)> block;               // for sight and rays
    std::function<bool(int zx, int zy)> zone;                                // a zone is loaded
    std::function<float(int x, int y)> ground;                               // 0x35ea0 (blocks)
    std::function<void(int x, int y, int z)> removeBlock;                    // 0x2a59f0
    std::function<void(int x0, int y0, int x1, int y1)> rebuild;             // 0x2e0710
    std::function<int()> rand;
    // 0x29eb60 (whether it landed) and 0x29dcf0 (a damage number).
    std::function<bool(const nativeupkeep::Hit &)> hit;
    std::function<void(const std::array<uint8_t, 0x48> &)> apply;
    // What the step tells the world: sounds (world+0xa0), buff notices (+0x150), particle
    // bursts (+0xb0), damage numbers (+0x90) and block changes (+0xc0).
    std::vector<std::array<uint8_t, 0x30>> sounds, buffNotes;
    std::vector<std::array<uint8_t, 0x48>> particles, numbers;
    std::vector<std::array<uint8_t, 0x20>> blockChanges;
    uint32_t changeCount = 0; // world+0x08, copied into every block change
};

// One step of 0x2dba40 over the world's effect objects (dt in ms).
void step(World &world, std::list<Effect> &effects, float dt);

} // namespace nativeeffects
