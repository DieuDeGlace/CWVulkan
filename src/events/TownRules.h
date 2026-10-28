#pragma once
#include "EventRules.h"
#include <cmath>
#include <array>

namespace encounters::town {
// Public marker values also serve older nameplate adapters (1/2).
enum QuestMarker { NoQuest = 0, Offer = 1, HandIn = 2, InProgress = 3 };
inline int referralMarker(bool complete, bool claimed, bool failed) {
    return claimed || failed ? NoQuest : complete ? HandIn : InProgress;
}
// Ordinary talking villagers only: merchants/pets and named quest bosses keep
// their native interactions. Humanoid race membership is checked at runtime.
inline bool eligible(int behaviour, float hp, unsigned flags) {
    constexpr unsigned excluded = (1u << 9) | (1u << 13) | (1u << 17) | (1u << 18);
    return behaviour == 3 && std::isfinite(hp) && hp > 0.f && !(flags & excluded);
}
inline bool referralKind(Kind kind) {
    return kind == Kind::Battle || kind == Kind::Rescue || kind == Kind::Ambush;
}
inline Kind kind(uint64_t roll) {
    constexpr Kind choices[] = {Kind::Battle, Kind::Rescue, Kind::Ambush};
    return choices[roll % 3];
}
inline Site candidate(Site origin, int radius, unsigned attempt) {
    constexpr Site directions[] = {{1, 0},  {1, 1},   {0, 1},  {-1, 1},
                                   {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
    auto offset = directions[attempt % 8];
    auto distance = int64_t(std::clamp(radius, 0, 4096)) + 128;
    distance = std::max<int64_t>(256, distance) + 128 * (attempt / 8);
    return {origin.x + offset.x * distance, origin.y + offset.y * distance};
}
inline Site nearby(Site target, unsigned attempt) {
    if (!attempt)
        return target;
    constexpr Site directions[] = {{1, 0},  {1, 1},   {0, 1},  {-1, 1},
                                   {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
    auto direction = directions[(attempt - 1) % 8];
    int distance = 16 * (1 + (attempt - 1) / 8);
    return {target.x + direction.x * distance, target.y + direction.y * distance};
}
// Check a center and its actor footprint before reserving unloaded terrain.
// Native sea level is zero; the margin keeps low, wet shorelines out.
inline bool dryLand(const std::array<float, 5> &heights) {
    for (auto height : heights)
        if (!std::isfinite(height) || height < 4.f || height > 100000.f)
            return false;
    auto bounds = std::minmax_element(heights.begin(), heights.end());
    return *bounds.second - *bounds.first <= 12.f;
}
constexpr Site HeightSamples[] = {{0, 0}, {40, 0}, {-16, 0}, {0, 16}, {0, -16}};
} // namespace encounters::town
