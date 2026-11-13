#pragma once
// Combat texts of the Game update (phase 68): steam-combat-floating-texts-20261113.md sections
// 2, 3 and 6.1. The hit lights of the same routine are steampoints::TransientLights::hit; the
// taming branch (section 5) belongs to the ground-item/pet code.
#include "FeatText.h"
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
namespace combattext {
using Hit = std::array<uint8_t, 0x48>;
// The creature record of an identifier (the World's map, the local player included), or null.
using Lookup = std::function<uint8_t *(int64_t)>;
constexpr int32_t LIFETIME = 1000;
constexpr float FONT_SIZE = 20, CRITICAL_FONT_SIZE = 60;
// Section 4: flags 1 for both TextShapes (notices pass 0); plasma-shapes-and-text.md 4.2 makes it
// "centre each line horizontally on the origin", the first baseline staying on the origin.
constexpr int32_t FLAGS = 1;
// Section 3.3: |amount| truncated when at least 1, otherwise in tenths; no sign.
std::wstring number(float amount);
// One hit record visited: the attacker's bookkeeping (section 6.1), then "K.O." and the number or
// label, in that order. `player` is the local creature's record (null when there is none).
std::vector<feattext::Record> visit(const Hit &hit, const Lookup &lookup, const uint8_t *player);
} // namespace combattext
