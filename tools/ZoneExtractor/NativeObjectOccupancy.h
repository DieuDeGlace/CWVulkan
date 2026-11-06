#pragma once
// Scope and node layout from steam-zone-restored-object-occupancy.md.
#include "../../src/ObjectOccupancyCapture.h"
#include "NativeSpawnCellTrees.h"

namespace captureabi {
// Requires a retained fresh Zone at the synchronous generator boundary.
// The ordinary live-zone/mesh lease does not establish mutation exclusion here.
template <class Read>
std::optional<objectoccupancy::Positions> readFreshObjectMarks(uintptr_t zone, Read read) {
    objectoccupancy::Positions positions;
    if (!spawn_tree::visit(zone, 0xe8, objectoccupancy::MaxPositions, read, [&](uintptr_t node) {
            std::array<int32_t, 3> point{};
            if (!readField(node, 0x1c, point.data(), sizeof point, read))
                return false;
            const auto key = std::make_tuple(point[0], point[1], point[2]);
            if (!positions.empty() && !(*positions.rbegin() < key))
                return false;
            positions.emplace_hint(positions.end(), key);
            return true;
        }))
        return std::nullopt;
    return positions;
}
} // namespace captureabi
