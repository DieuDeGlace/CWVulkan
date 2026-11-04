#pragma once
// Independent sequence/lock facts: steam-loaded-zone-container.md §§1–5.
#include "CaptureMemory.h"
#include <array>
#include <utility>
#include <vector>

namespace captureabi {
constexpr std::size_t WORLD_ZONES_SECTION = 0x3f0;
constexpr std::size_t MAX_LOADED_ZONES = 65536; // Capture policy, not a native limit.

// Caller retains a valid client World and its container lock through this read
// AND every subsequent use of these borrowed Zones. No keys or buckets are read.
template <class Read> bool readLoadedZones(std::uintptr_t world, std::vector<std::uintptr_t> &output, Read read) {
    output.clear();
    std::array<std::uintptr_t, 2> header{};
    if (!readField(world, 0x468, header.data(), sizeof header, read) || !header[0] || header[1] > MAX_LOADED_ZONES)
        return false;
    const auto sentinel = header[0], count = header[1];
    std::array<std::uintptr_t, 2> ends{};
    if (!readField(sentinel, 0, ends.data(), sizeof ends, read))
        return false;
    std::vector<std::uintptr_t> result;
    result.reserve(count);
    auto previous = sentinel, node = ends[0];
    std::size_t visited = 0;
    while (node != sentinel) {
        if (!node || visited >= count)
            return false;
        std::array<std::uintptr_t, 2> links{};
        std::uintptr_t zone = 0;
        if (!readField(node, 0, links.data(), sizeof links, read) || links[1] != previous ||
            !readField(node, 0x18, &zone, sizeof zone, read))
            return false;
        if (zone)
            result.push_back(zone);
        previous = node;
        node = links[0];
        ++visited;
    }
    if (visited != count || ends[1] != previous)
        return false;
    std::array<std::uintptr_t, 2> currentHeader{}, currentEnds{};
    if (!readField(world, 0x468, currentHeader.data(), sizeof currentHeader, read) || currentHeader != header ||
        !readField(sentinel, 0, currentEnds.data(), sizeof currentEnds, read) || currentEnds != ends)
        return false;
    output = std::move(result);
    return true;
}
} // namespace captureabi
