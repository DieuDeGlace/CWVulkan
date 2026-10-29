#pragma once
// Inventory panel state and region choices (remaining-runtime-gaps.md §4.9, §4.15).
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace inventoryui {
using Region = std::array<int32_t, 2>;

inline std::vector<Region> regionChoices(std::vector<Region> regions, Region here) {
    regions.push_back(here);
    std::sort(regions.begin(), regions.end());
    regions.erase(std::unique(regions.begin(), regions.end()), regions.end());
    auto distance = [&](Region region) {
        const int64_t x = (int64_t)region[0] - here[0], y = (int64_t)region[1] - here[1];
        return std::max(x < 0 ? -x : x, y < 0 ? -y : y);
    };
    // The verified insertion path preserves lexicographic ties through 32 entries.
    // Larger distance ties retain that order until their native permutation is specified.
    std::stable_sort(regions.begin(), regions.end(), [&](Region a, Region b) { return distance(a) < distance(b); });
    return regions;
}

inline void changeRegion(Region &active, const std::vector<Region> &choices, int direction) {
    const auto found = std::find(choices.begin(), choices.end(), active);
    if (found == choices.end())
        return;
    const auto count = (int)choices.size(), index = (int)(found - choices.begin());
    active = choices[(size_t)((index + (direction < 0 ? count - 1 : 1)) % count)];
}

template <size_t Count> struct Tabs {
    int tab = 0, scroll = 0;
    int selected = -1; // Entry index; scrolling leaves it unchanged.
    std::array<int, Count> rowOffsets{};

    void selectTab(int next) {
        rowOffsets[(size_t)tab] = scroll;
        tab = (next % (int)Count + (int)Count) % (int)Count;
        scroll = rowOffsets[(size_t)tab];
    }
};
} // namespace inventoryui
