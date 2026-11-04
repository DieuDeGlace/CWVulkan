#pragma once
// Primitive stored bytes only: steam-zone-terrain-storage.md §§1–7. The caller
// retains Zone/World lifetime and supplies guarded reads for the supported image.
// Rechecks reject observed changes; they do not freeze in-place writes/lighting.
#include "CaptureMemory.h"
#include <array>
#include <cstring>
#include <utility>
#include <vector>

namespace captureabi {
constexpr std::size_t ZONE_COLUMN_COUNT = 4096;
constexpr std::size_t ZONE_COLUMNS = 0x518, ZONE_COLUMN_STRIDE = 0x58;
constexpr std::uintptr_t ZONE_TABLE_RVA = 0x4738c0;
// Resource ceilings are capture policy, not native world/height limits.
constexpr std::size_t MAX_COLUMN_BLOCKS = 131072;
constexpr std::size_t MAX_ZONE_BLOCK_BYTES = 16 * 1024 * 1024 * 6;
using StoredBlock = std::array<std::uint8_t, 6>;
static_assert(sizeof(StoredBlock) == 6);
struct StoredColumn {
    std::int32_t baseZ = 0;
    std::vector<StoredBlock> blocks;
};
struct ZoneTerrain {
    std::array<std::int32_t, 2> coordinates{};
    std::vector<StoredColumn> columns;
};

template <class Read>
bool readZoneIdentity(std::uintptr_t module, std::uintptr_t world, std::uintptr_t zone,
                      std::array<std::int32_t, 2> &coordinates, Read read) {
    coordinates = {};
    constexpr auto maximum = std::numeric_limits<std::uintptr_t>::max();
    std::array<std::uintptr_t, 3> identity{};
    if (!module || !world || module > maximum - ZONE_TABLE_RVA ||
        !readField(zone, 0, identity.data(), sizeof identity, read) || identity[0] != module + ZONE_TABLE_RVA ||
        identity[1] != world)
        return false;
    std::memcpy(coordinates.data(), &identity[2], sizeof coordinates);
    return true;
}

template <class Read>
bool readStoredColumn(std::uintptr_t zone, std::size_t index, StoredColumn &output, Read read,
                      std::size_t byteBudget = MAX_ZONE_BLOCK_BYTES) {
    output = {};
    if (index >= ZONE_COLUMN_COUNT)
        return false;
    const auto offset = ZONE_COLUMNS + index * ZONE_COLUMN_STRIDE;
    std::int32_t base = 0;
    std::array<std::uintptr_t, 3> span{};
    if (!readField(zone, offset + 0x3c, &base, sizeof base, read) ||
        !readField(zone, offset + 0x40, span.data(), sizeof span, read))
        return false;
    const auto begin = span[0], end = span[1], capacity = span[2];
    if (end < begin || capacity < end || (!begin && (end || capacity)) || (end - begin) % sizeof(StoredBlock) ||
        (capacity - begin) % sizeof(StoredBlock))
        return false;
    const auto bytes = end - begin, count = bytes / sizeof(StoredBlock);
    if (count > MAX_COLUMN_BLOCKS || bytes > byteBudget)
        return false;
    StoredColumn result;
    result.baseZ = base;
    result.blocks.resize(count);
    if (bytes && !readField(begin, 0, result.blocks.data(), bytes, read))
        return false;
    std::int32_t currentBase = 0;
    std::array<std::uintptr_t, 3> currentSpan{};
    if (!readField(zone, offset + 0x3c, &currentBase, sizeof currentBase, read) || currentBase != base ||
        !readField(zone, offset + 0x40, currentSpan.data(), sizeof currentSpan, read) || currentSpan != span)
        return false;
    output = std::move(result);
    return true;
}

template <class Read>
bool readZoneTerrain(std::uintptr_t module, std::uintptr_t world, std::uintptr_t zone, ZoneTerrain &output, Read read,
                     std::size_t byteBudget = MAX_ZONE_BLOCK_BYTES) {
    output = {};
    ZoneTerrain result;
    if (!readZoneIdentity(module, world, zone, result.coordinates, read))
        return false;
    result.columns.resize(ZONE_COLUMN_COUNT);
    for (std::size_t index = 0; index < ZONE_COLUMN_COUNT; ++index) {
        auto &column = result.columns[index];
        if (!readStoredColumn(zone, index, column, read, byteBudget))
            return false;
        byteBudget -= column.blocks.size() * sizeof(StoredBlock);
    }
    std::array<std::int32_t, 2> currentCoordinates{};
    if (!readZoneIdentity(module, world, zone, currentCoordinates, read) || currentCoordinates != result.coordinates)
        return false;
    output = std::move(result);
    return true;
}
} // namespace captureabi
