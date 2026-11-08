#pragma once
// Independent client/host source footprints: steam-world-zone-window-demand.md §§2–7.
#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace zonedemand {
struct Window {
    std::array<int32_t, 2> center{}, minimum{}, maximum{};
    int32_t radiusSquared = 0;
    bool contains(int64_t x, int64_t y) const;
};
using BlockPoint = std::array<int32_t, 2>;
std::optional<BlockPoint> eagleAnchor(uint8_t ability, const BlockPoint &destination);
// Source loaders sample whole blocks by truncation, unlike mesh retention floors.
BlockPoint sourcePoint(const std::array<int64_t, 3> &physical, const std::optional<BlockPoint> &anchor = {});
// Signed32 squared distance to the zone's closed block rectangle; ordinary inputs.
int32_t generationPriority(BlockPoint zone, BlockPoint sampledBlocks);
// Minimum admitted centre key; -1 when no centre passes the host scoring gate.
int32_t hostGenerationPriority(BlockPoint zone, std::span<const BlockPoint> centres);
Window clientWindow(const std::array<int64_t, 3> &physical, int32_t renderDistance,
                    const std::optional<BlockPoint> &anchor = {});
Window hostWindow(const std::array<int64_t, 3> &physical, const std::optional<BlockPoint> &anchor = {});
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
