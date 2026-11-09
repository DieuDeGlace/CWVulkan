#pragma once
// Creation mutates the retained actor. Disk data and list snapshots have different owners.
#include "CharacterState.h"
#include <functional>
#include <string_view>

namespace charactercreation {
struct Placement {
    nativesaves::Cell home{};
    std::array<int64_t,3> position{};
};
using FindHome = std::function<Placement()>;
void begin(worldgen::CreatureRecord &, nativeai::Creature &, nativesaves::Character &, const FindHome &);
void rebuild(worldgen::CreatureRecord &, nativeai::Creature &, nativesaves::Character &,
             nativesaves::Looks, uint8_t characterClass);
void publishAccount(worldgen::CreatureRecord &, uint64_t account);
std::string nameBytes(std::string_view utf8);
// No UI name gate or HP/position reset here. Malformed/oversized names are bounded safely.
void confirm(worldgen::CreatureRecord &, nativeai::Creature &, nativesaves::Character &, std::string_view name);
struct SelectionSnapshot {
    nativesaves::Character character;
    worldgen::CreatureRecord record;
};
SelectionSnapshot selectionSnapshot(const worldgen::CreatureRecord &, const nativeai::Creature &, const nativesaves::Character &);
}
