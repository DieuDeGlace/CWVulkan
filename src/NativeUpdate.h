#pragma once
// A creature's physics step (the game's 0x2bb8e0), written from docs/specs/creature-physics.md.
// The world's update (0x2da6c0) runs it each tick for every living creature, before the
// ability step and the interactions. It works out how fast the creature may go (its ground
// grip, sprint, charges and dashes), adds gravity, the glider's lift and wind, pushes it apart
// from other creatures and frees it from blocks it is stuck in, then moves it through the
// blocks and objects in small steps, stepping up ledges and landing. Afterwards it records what
// the creature touches (the contact flags +0x5c and the wall normal +0x9ac), floats it in
// water, burns it in lava, slows it in swamps, and updates its dust, stride, footsteps, lean
// and the way it faces.
//
// It works on a game-layout creature record (nativepose::CREATURE_SIZE bytes); what it needs
// of the world comes through `World`.
#include "NativeBuffs.h"
#include "NativeEffects.h"

#include <array>
#include <cstdint>
#include <functional>
#include <list>
#include <vector>

namespace nativeupdate {

// A block as the world's lookup (0xe9a0) gives it: colour (bytes 0-2), type (+4), flags (+5).
using Block = std::array<uint8_t, 6>;

// The world as the physics step sees it. Every callback is optional: an unset one answers as
// an empty world would (air everywhere, every zone loaded, nobody by id, no buffs, rand 0).
struct World {
    uint8_t *local = nullptr; // world+0x448: the local player's record
    bool client = false;      // world+0x358: this world mirrors another's
    float time = 0;           // world+0x440: the world clock (ms)

    std::function<Block(int x, int y, int z)> block; // 0xe9a0, by block coordinates
    std::function<bool(int x, int y)> loaded;        // 0x935e0: the block column's zone is loaded
    std::vector<uint8_t *> creatures;                // world+0x40, in the world's order
    // world+0x50: elevators, doors, ladders and the like, laid out like creature records.
    std::vector<const uint8_t *> objects;
    std::function<uint8_t *(int64_t id)> byId; // 0xe720
    std::function<int()> rand;                 // the C runtime's rand(), 0..32767

    // The creature's buff list (+0x980), or null for none.
    std::function<const std::vector<nativebuffs::Buff> *(const uint8_t *creature)> buffs;
    // 0x65390: gives the creature a buff (see nativebuffs::give for `refresh`).
    std::function<void(uint8_t *creature, const nativebuffs::Buff &buff, bool refresh)> giveBuff;

    // world+0x60: the effect objects (the fire trail looks for its own and adds one); each one
    // added is also copied to `sentEffects` (world+0xd0, for the network). Null: none.
    std::list<nativeeffects::Effect> *effects = nullptr;
    std::vector<nativeeffects::Effect> sentEffects;

    // The glider's wind at a world position (0x2e65e0), blocks a second; unset: windAt below.
    std::function<std::array<float, 3>(int64_t x, int64_t y)> wind;
    // How warm the column (x, y) is: the generator's warmth (0x2d9b60) with the regions around
    // it (0x358e0) and its reach-gated place (0x35ce0). Below 0.2 water freezes the local
    // player. Unset (no world generator): nothing freezes.
    std::function<float(int x, int y)> warmth;

    // What the step tells the world: sound requests (world+0xa0, 0x30 bytes), particle
    // bursts (+0xb0, 0x48 bytes), damage numbers (+0x90, 0x48 bytes) and buff-change notices
    // (+0x150, 0x30 bytes), in the layouts of docs/specs/creature-hit.md section 0.
    std::vector<std::array<uint8_t, 0x30>> sounds, buffNotes;
    std::vector<std::array<uint8_t, 0x48>> particles, numbers;
};

// One physics step of `creature`, `dt` milliseconds long (0x2bb8e0).
void update(World &world, uint8_t *creature, float dt);

// 0x2b77c0: whether a box of `size` blocks at world position `pos` would overlap a blocked block
// of the world (section 7.1: a shell scan of the box; with `honour`, passages block unless `pass`).
bool boxBlocked(const World &world, const int64_t pos[3], const float size[3], bool pass, bool honour);

// The glider's wind at world position (x, y) in 1/65536 block (0x2e65e0), blocks a second.
std::array<float, 3> windAt(int64_t x, int64_t y);

} // namespace nativeupdate
