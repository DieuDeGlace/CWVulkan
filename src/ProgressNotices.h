#pragma once
// The Game update's progress notices (steam-progress-notices-and-floating-text-20261112.md):
// phases 17 and 75 take snapshots; phases 89-92 compare and produce "+N XP", "+N GOLD",
// "+N PLATINUM" and "LEVEL UP!" floating texts, chat lines and the level sound.
#include "FeatText.h"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace progressnotices {
using Zone = std::array<int32_t, 2>;
// The compared values of the local creature P.
struct Progress {
    int32_t gold = 0;     // P+0xAAC
    int32_t platinum = 0; // 0x60EC0(P): the platinum-coin counter of P's current zone (0 if none)
    Zone zone{};          // P+0x150/+0x154
    int32_t level = 1;    // P+0x144
    int32_t experience = 0; // P+0x148
};
struct BeforeWorld { int32_t gold = 0, platinum = 0; Zone zone{}; };  // phase 17
struct AfterCamera { int32_t totalExperience = 0, level = 1; };       // phase 75
BeforeWorld beforeWorld(const Progress &);
AfterCamera afterCamera(const Progress &);
// Total XP 0x666A0 (section 7.2): experience plus trunc(10 * 2^((k-1)/2)) for k = 1..level-1.
int32_t totalExperience(int32_t level, int32_t experience);
// The anchor of phases 89-92 (section 3.2): P's position plus a fixed offset from three draws
// r1 (Y), r2 (X), r3 (Z, upward), each 0..32767.
feattext::Point anchor(const feattext::Point &player, int r1, int r2, int r3);
struct Notice {
    feattext::Record record;
    std::u16string chat;         // empty: no chat line
    std::array<uint8_t, 3> chatColour{};
    int sound = 0;               // played before the record is appended (phase 92 only)
};
// Phases 89, 90, 91 and 92 in order; each that fires takes its three draws from `rand`.
std::vector<Notice> compare(const BeforeWorld &, const AfterCamera &, const Progress &now, const feattext::Point &player,
                            const std::function<int()> &rand);
} // namespace progressnotices
