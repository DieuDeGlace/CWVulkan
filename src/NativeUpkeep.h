#pragma once
// A creature's upkeep: the last of its steps in each world update (the game's 0x2de4c0),
// run for every creature in the world, alive or dead, after its physics, its abilities and
// its interactions. It keeps the slow-changing state ticking: the group-fight health buff,
// riders and carried creatures following their carrier, the short timers, buffs (their
// countdown, poison and healing ticks, expiry), the stun, the ability clock, mana, block
// power, stealth, stamina (gliding, swimming and drowning, regeneration) and the dodge timer.
//
// It works on a game-layout creature record with the containers the game keeps beside it
// (nativeai::Creature). Behaviour as described in docs/specs/creature-upkeep.md.
#include "NativeAI.h"

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace nativeupkeep {

// A hit as the game's "apply a hit" routine (0x29eb60) takes it: who is hit and by whom,
// how hard and how. The ability, effect and hit modules share this form.
struct Hit {
    int64_t target = 0, attacker = 0; // creature ids (attacker 0: none)
    float damage = 0;
    uint8_t critical = 0, flag = 0;
    float charge = 0;
    int64_t where[3] = {0, 0, 0}; // world position (1/65536 block)
    float push[3] = {0, 0, 0};
    int32_t type = 0, ability = 0;
    int64_t extra = 0;
    uint8_t last = 0;
};

// The world as the upkeep sees it. Every callback is optional: an unset one answers as an
// empty world would (no creature, nobody dead, no zone, the default block, no light, rand 0).
struct World {
    bool client = false;            // this world only mirrors a server (world+0x358)
    const uint8_t *local = nullptr; // the local player's record (world+0x448)
    float time = 0;                 // world time, ms (world+0x440)
    float day = 0;                  // time of day, ms; a day is 86 400 000 (world+0xc)
    std::function<nativeai::Creature *(int64_t id)> byId; // creature by id (world+0x450)
    std::function<bool(int64_t id)> dead;                 // the world's set of dead ids (world+0x20)
    // The zone holding world position (x, y): false when it is not loaded; otherwise its
    // linked zones (zone+0x470), each loaded or not, with the creature ids it lists (+0x480).
    struct Link {
        bool loaded = false;
        std::vector<int64_t> ids;
    };
    std::function<bool(int64_t x, int64_t y, std::vector<Link> &links)> zoneLinks;
    // The block at a world position (0xeab0); a missing zone gives the default block.
    std::function<nativeai::Block(int64_t x, int64_t y, int64_t z)> blockAt;
    // The light at a block (0xdab20 with channel 0), 0..255.
    std::function<int(int x, int y, int z)> light;
    std::function<int()> rand; // the game's shared rand(), 0..32767
    // Applies a hit now, inside the upkeep (poison ticks), returning whether it landed.
    // When unset, the hits are only collected in `hits` for the caller to apply.
    std::function<bool(const Hit &)> hit;

    // What the upkeep tells the world: sound requests (world+0xa0), damage numbers
    // (world+0x90), buff-change notices (world+0x150) and every hit it asked for.
    std::vector<std::array<uint8_t, 0x30>> sounds, buffNotes;
    std::vector<std::array<uint8_t, 0x48>> numbers;
    std::vector<Hit> hits;
};

// Race-table riding offset used by upkeep and the gameplay camera.
std::array<float, 3> ridingOffset(int32_t race);
float ridingOffsetZ(int32_t race);

// One upkeep step of `creature`, `dt` milliseconds long.
void upkeep(World &world, nativeai::Creature *creature, float dt);

} // namespace nativeupkeep
