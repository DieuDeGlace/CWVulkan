#pragma once
// Inventory panel state and region choices (remaining-runtime-gaps.md §4.9, §4.15).
#include "NativeKeyOrder.h"
#include <bit>
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
        auto difference = [](int32_t value, int32_t origin) {
            const uint32_t word = (uint32_t)value - (uint32_t)origin;
            const int32_t delta = std::bit_cast<int32_t>(word);
            return delta < 0 ? std::bit_cast<int32_t>(0u - word) : delta;
        };
        return std::max(difference(region[0], here[0]), difference(region[1], here[1]));
    };
    keyorder::sort(regions, distance);
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
