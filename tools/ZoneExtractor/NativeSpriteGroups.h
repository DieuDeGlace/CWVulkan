#pragma once
// Independent contract: steam-catalog-indexed-side-resources.md sections1–6.
#include "NativeSpriteCatalog.h"

namespace captureabi {
constexpr std::array<size_t, 3> SPRITE_GROUP_OFFSETS{0x38, 0x50, 0x68};
constexpr size_t MAX_GROUP_SPRITES = 65536; // Capture budget, not a native live-length limit.
struct IndexedSpriteGroups {
    uintptr_t catalog = 0;
    std::array<std::vector<uintptr_t>, 3> entries;
};

// Copy borrowed pointer values only. Preserve side indices, aliases and nulls;
// do not visit the procedural cache or dereference sprite/resource payloads.
// Caller retains catalog/sprite lifetime and an appropriate capture phase.
// Header rechecks detect changed metadata, not concurrent element writes,
// teardown, GPU readiness or an atomic snapshot across resource families.
template <class Read> bool readIndexedSpriteGroups(uintptr_t world, IndexedSpriteGroups &output, Read read) {
    output = {};
    IndexedSpriteGroups result;
    if (!readCatalogAddress(world, result.catalog, read) || !result.catalog)
        return false;
    std::array<std::array<uintptr_t, 3>, 3> bounds;
    for (size_t group = 0; group < bounds.size(); ++group) {
        auto &span = bounds[group];
        if (!readField(result.catalog, SPRITE_GROUP_OFFSETS[group], span.data(), sizeof span, read))
            return false;
        const auto begin = span[0], end = span[1], capacityEnd = span[2];
        if ((!begin && (end || capacityEnd)) || begin > end || end > capacityEnd || ((begin | end | capacityEnd) & 7))
            return false;
        const auto count = (end - begin) / sizeof(uintptr_t);
        if (count > MAX_GROUP_SPRITES)
            return false;
        auto &entries = result.entries[group];
        entries.resize(count);
        if (count && !readField(begin, 0, entries.data(), count * sizeof(uintptr_t), read))
            return false;
    }
    uintptr_t currentCatalog = 0;
    if (!readCatalogAddress(world, currentCatalog, read) || currentCatalog != result.catalog)
        return false;
    for (size_t group = 0; group < bounds.size(); ++group) {
        std::array<uintptr_t, 3> current;
        if (!readField(result.catalog, SPRITE_GROUP_OFFSETS[group], current.data(), sizeof current, read) ||
            current != bounds[group])
            return false;
    }
    output = std::move(result);
    return true;
}
} // namespace captureabi
