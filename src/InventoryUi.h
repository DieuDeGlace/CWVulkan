#pragma once
// Inventory panel state and region choices (remaining-runtime-gaps.md §4.9, §4.15).
#include "NativeKeyOrder.h"
#include "worldgen/Noise.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

namespace inventoryui {
using Region = std::array<int32_t, 2>;
enum class Panel { Inventory = 0, Shop = 1, Crafting = 2 };

// Explicit class setup replaces the initial blank controls. It is independent of
// the carried/crafting group count and does not repair any selected tab or row.
inline std::vector<std::string> craftingTabIcons(int playerClass) {
    std::vector<std::string> icons;
    if (playerClass >= 1 && playerClass <= 4) {
        static const char *const classes[] = {"warrior", "ranger", "mage", "rogue"};
        const std::string prefix = std::string("craft-") + classes[playerClass - 1];
        icons.push_back(prefix + "-weapons");
        icons.push_back(prefix + "-armor");
    }
    for (const char *name : {"craft-jewels", "craft-cooking", "craft-alchemy", "craft-ingredients"})
        icons.emplace_back(name);
    return icons;
}

// Each wheel message supplies one row attempt; opposing messages remain separate.
struct WheelEvent {
    double x = 0, y = 0; // window coordinates at the event
    int rowStep = 0;
};
inline int wheelRowStep(double delta) { return delta < 0 ? 1 : delta > 0 ? -1 : 0; }
template <class Hit> int wheelTarget(const Hit &hit) {
    // Native priority differs from the controller panel-registration order.
    for (int which : {0, 1, 2}) // inventory, shop, crafting
        if (hit(which))
            return which;
    return -1;
}

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

// Native2806a0 reads the player's canonical tab, not the bound collection.
// Missing engine source headers defer the update; no native success is claimed
// for an invalid carried-tab access. Binding/mode skips preserve cached choices.
template <class Tabs>
bool refreshRegionCache(bool bound, Panel panel, std::vector<Region> &cache, const Tabs &carried, int tab,
                        Region playerRegion) {
    if (!bound || panel != Panel::Inventory || tab < 0 || size_t(tab) >= carried.size())
        return false;
    std::vector<Region> regions;
    for (const auto &stack : carried[size_t(tab)]) {
        if (stack.count == 0)
            continue;
        Region region;
        std::memcpy(region.data(), stack.item.data() + 0x0c, sizeof region);
        regions.push_back(region);
    }
    cache = regionChoices(std::move(regions), playerRegion);
    return true;
}

inline void changeRegion(Region &active, const std::vector<Region> &choices, int direction) {
    const auto found = std::find(choices.begin(), choices.end(), active);
    if (found == choices.end())
        return;
    const auto count = (int)choices.size(), index = (int)(found - choices.begin());
    active = choices[(size_t)((index + (direction < 0 ? count - 1 : 1)) % count)];
}

// Region arrows request sound before searching, then refresh even with no matching
// choice. Next-region adds an explicit canonical sort before refresh's own sort.
// steam-inventory-refresh-events.md section 4.2.
template <class Sound, class Sort, class Refresh, class Thumb>
void requestRegion(Region &active, const std::vector<Region> &choices, int direction, Sound &&sound, Sort &&sort,
                   Refresh &&refresh, Thumb &&thumb) {
    if (!direction)
        return;
    sound();
    changeRegion(active, choices, direction);
    if (direction > 0)
        sort();
    refresh();
    thumb();
}

// Reached row handlers refresh even when already at a boundary. The callback
// maintains the current row and returns the available first-row count, not a
// maximum offset. A clamp performed by refresh is not a requested row movement.
// steam-inventory-refresh-events.md section 4.2 / steam-inventory-grid-layout.md section 4.
template <class Refresh> bool requestRow(int &row, int direction, Refresh &&refresh) {
    if (!direction)
        return false;
    const int available = refresh();
    const int before = row;
    if (direction < 0) {
        if (row > 0)
            --row;
    } else if (row < available - 1)
        ++row;
    return row != before;
}

struct ThumbPlacement {
    int height;
    float y;
};
inline std::optional<ThumbPlacement> thumbPlacement(int32_t track, int32_t available, int32_t row) {
    if (!available || (track == INT32_MIN && available == -1))
        return std::nullopt; // A0 skips; native IDIV overflow has no qualified successful result.
    const int32_t height = track / available;
    int32_t y = 35;
    if (available > 1) {
        const uint32_t product = ((uint32_t)track - (uint32_t)height) * (uint32_t)row;
        const int32_t distance = std::bit_cast<int32_t>(product) / (available - 1);
        y = std::bit_cast<int32_t>((uint32_t)distance + 35u);
    }
    return ThumbPlacement{height, (float)y};
}

struct PanelState {
    int tab = 0, scroll = 0;
    int selected = -1; // Presentation index; scrolling leaves it unchanged.
    int availableRows = 1;
    std::vector<int> rowOffsets; // Empty until an admitted refresh supplies physical tab headers.

    void retainScroll() {
        if (tab >= 0 && (size_t)tab < rowOffsets.size())
            rowOffsets[(size_t)tab] = scroll;
    }
};

// Inventory thumb event 12. It changes the cached row and continuous position
// without refreshing the collection or using the discrete placement formula.
inline std::optional<float> dragThumb(PanelState &state, int32_t track, float y, float delta) {
    const int32_t available = state.availableRows;
    if (available <= 0 || state.tab < 0 || (size_t)state.tab >= state.rowOffsets.size() || !std::isfinite(y + delta))
        return std::nullopt;
    const auto travel = std::bit_cast<int32_t>(uint32_t(track) - uint32_t(track / available));
    const auto end = std::bit_cast<int32_t>(uint32_t(travel) + 35u);
    const float next = std::min(std::max(y + delta, 35.0f), float(end));
    int32_t row = state.scroll;
    if (travel > 0)
        row = worldgen::trunc32((float(available - 1) * (next - 35.0f)) / float(travel));
    state.scroll = std::clamp(row, 0, available - 1);
    state.retainScroll();
    return next;
}

// The caller has already performed the separate region/sort stages. Binding and
// nonpositive-column skips preserve A, row-vector size and offsets. Invalid engine
// tab indices use a safe fallback; native successful negative indexing is unproved.
template <class Collection>
bool refreshRange(PanelState &state, const Collection *source, int columns, int visibleRows,
                  const Region *filter = nullptr) {
    if (!source || columns <= 0)
        return false;
    state.retainScroll();
    state.rowOffsets.resize(source->size(), 0);
    if (state.tab < 0 || (size_t)state.tab >= source->size()) {
        state.availableRows = 1;
        return true;
    }
    const auto &slots = (*source)[(size_t)state.tab];
    if (slots.empty()) {
        state.availableRows = 0;
        state.scroll = 0;
    } else {
        int32_t lastRow;
        if (filter) {
            uint32_t matching = 0;
            for (const auto &slot : slots) {
                Region region;
                std::memcpy(region.data(), slot.item.data() + 0x0c, sizeof region);
                if (slot.count != 0 && region == *filter)
                    ++matching;
            }
            lastRow = std::bit_cast<int32_t>(matching - 1u) / columns;
        } else {
            lastRow = std::bit_cast<int32_t>((uint32_t)((slots.size() - 1) / (uint32_t)columns));
        }
        // Native scalar arithmetic keeps the low32 bits after the length division.
        const uint32_t positions = (uint32_t)lastRow - (uint32_t)visibleRows + 2u;
        state.availableRows = std::max(1, std::bit_cast<int32_t>(positions));
        if (state.scroll > state.availableRows - 1)
            state.scroll = state.availableRows - 1;
    }
    state.retainScroll();
    return true;
}

template <size_t Count> struct Tabs : PanelState {
    void selectTab(int next, bool canonical = false) {
        if (!canonical && rowOffsets.size() < Count)
            rowOffsets.resize(Count, 0); // Recording-only panels have no canonical binding.
        retainScroll();
        tab = (next % (int)Count + (int)Count) % (int)Count;
        scroll = (size_t)tab < rowOffsets.size() ? rowOffsets[(size_t)tab] : 0;
    }
};
} // namespace inventoryui
