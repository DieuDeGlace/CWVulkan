#pragma once
// Independent client source footprint: steam-world-zone-window-demand.md §§2–5.
#include <array>
#include <cstdint>
#include <optional>

namespace zonedemand {
struct ClientWindow {
    std::array<int32_t, 2> center{}, minimum{}, maximum{};
    int32_t radiusSquared = 0;
    bool contains(int64_t x, int64_t y) const;
};
using BlockPoint = std::array<int32_t, 2>;
std::optional<BlockPoint> eagleAnchor(uint8_t ability, const BlockPoint &destination);
ClientWindow clientWindow(const std::array<int64_t, 3> &physical, int32_t renderDistance,
                          const std::optional<BlockPoint> &anchor = {});
BlockPoint preparationCenter(const std::array<int64_t, 3> &physical, const std::optional<BlockPoint> &anchor = {});
// Steady retention boundary; the original worker evicts beyond it incrementally.
bool withinRetention(int64_t dx, int64_t dy, int32_t sceneRadius);

template <class Lookup> bool neighborhood(int64_t x, int64_t y, const Lookup &available) {
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            if (!available(x + dx, y + dy))
                return false;
    return true;
}
} // namespace zonedemand
