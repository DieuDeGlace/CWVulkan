#pragma once
// Independent layout facts: docs/specs/steam-native-sprite-catalog.md §§2–5.
#include "CaptureMemory.h"
#include <array>
#include <utility>
#include <vector>

namespace captureabi {
struct SpriteDimensions {
    std::int32_t x = 0, y = 0, z = 0;
};
struct PrimarySprite {
    std::uintptr_t address = 0;
    SpriteDimensions dimensions;
};

// Exporter allocation ceiling, not a native count or an exhaustive resource list.
constexpr std::size_t MAX_PRIMARY_SPRITES = 65536;

// The caller validates the supported image and copies through a guarded reader.
// Only borrowed addresses and raw dimensions are captured: no RGB/GPU readiness,
// scale, resource ownership, or complete SDK replacement is implied. Rechecking
// the vector catches a changed range; it does not synchronize concurrent teardown.
template <class Read> bool readPrimarySprites(std::uintptr_t world, std::vector<PrimarySprite> &output, Read read) {
    static_assert(sizeof(std::uintptr_t) == 8, "The supported native image is AMD64");
    static_assert(sizeof(SpriteDimensions) == 12);
    output.clear();
    std::uintptr_t catalog = 0;
    if (!readField(world, 0x70, &catalog, sizeof catalog, read) || !catalog)
        return false;
    std::array<std::uintptr_t, 3> bounds{};
    if (!readField(catalog, 0x10, bounds.data(), sizeof bounds, read))
        return false;
    const auto begin = bounds[0], end = bounds[1], capacityEnd = bounds[2];
    if ((!begin && (end || capacityEnd)) || begin > end || end > capacityEnd || ((begin | end | capacityEnd) & 7))
        return false;
    const auto count = (end - begin) / sizeof(std::uintptr_t);
    if (count > MAX_PRIMARY_SPRITES)
        return false;
    std::vector<std::uintptr_t> pointers(count);
    if (count && !readField(begin, 0, pointers.data(), count * sizeof(std::uintptr_t), read))
        return false;
    std::vector<PrimarySprite> result(count);
    for (std::size_t i = 0; i < count; ++i) {
        result[i].address = pointers[i];
        if (pointers[i] && !readField(pointers[i], 0x88, &result[i].dimensions, sizeof(SpriteDimensions), read))
            return false;
    }
    std::uintptr_t currentCatalog = 0;
    std::array<std::uintptr_t, 3> currentBounds{};
    if (!readField(world, 0x70, &currentCatalog, sizeof currentCatalog, read) || currentCatalog != catalog ||
        !readField(catalog, 0x10, currentBounds.data(), sizeof currentBounds, read) || currentBounds != bounds)
        return false;
    output = std::move(result);
    return true;
}
} // namespace captureabi
