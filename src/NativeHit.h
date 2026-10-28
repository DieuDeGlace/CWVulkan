#pragma once
// The game's hit (0x29eb60) and what applies it (0x29dcf0), on game-layout records.
// 0x29eb60 decides how a hit lands: an untouchable target, a block (+0x184), a mana
// shield (buff 6), armor or resistance against the attacker's combo, the runes of the
// attacker's weapons, its frenzy (buff 2), poison (ability 0x12), the hit's sound, a
// critical's cry and the stun. It makes a damage number (world+0x90) and, on the server,
// applies it through 0x29dcf0: sneaking lost, aggro (+0xb88, +0xb98) and its spread
// (0x2c3200), knockback (+0x4c), stun (+0x130), health, heals, the shield's drain,
// and a death: its sound, its kill credits (world+0x120), and every creature forgetting
// it. What a death drops (0x2a6860, 0x2a7ca0) is the world's (`died`).
#include "NativeAI.h"
#include "NativeUpkeep.h"

#include <array>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace nativehit {

using Number = std::array<uint8_t, 0x48>; // a damage number (world+0x90)

struct World {
    bool client = false;                // world+0x358
    const uint8_t *local = nullptr;     // world+0x448
    std::vector<nativeai::Creature *> creatures;          // world+0x40
    std::function<nativeai::Creature *(int64_t id)> byId; // world+0x450
    std::function<int()> rand;
    std::function<void(nativeai::Creature *dead)> died; // 0x2a6860 (and 0x2a7ca0 for +0x128 bit 0x100)
    // What the hit tells the world: sounds (world+0xa0), particles (+0xb0), damage
    // numbers (+0x90), buff notes (+0x150), aggro notes (+0x140) and kill credits
    // (+0x120: whose aggro, the dead).
    std::vector<std::array<uint8_t, 0x30>> sounds, buffNotes;
    std::vector<std::array<uint8_t, 0x48>> particles;
    std::vector<Number> numbers;
    std::vector<std::array<uint8_t, 0x18>> aggroNotes;
    std::vector<std::pair<int64_t, int64_t>> kills;
};

// 0x29eb60: `attacker` (may be null) hits `target`; whether it landed.
bool hit(World &world, nativeai::Creature *target, nativeai::Creature *attacker, const nativeupkeep::Hit &h);
// 0x29dcf0: applies a damage number.
void apply(World &world, const Number &number);

} // namespace nativehit
