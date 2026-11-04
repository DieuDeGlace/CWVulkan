#pragma once
// Independent active-list/lock facts: steam-terrain-buffer-resources.md §§1–5.
#include "NativeSpriteBuffers.h"

namespace captureabi {
constexpr std::size_t WORLD_MESH_SECTION = 0x3c8, WORLD_ZONES_SECTION = 0x3f0;
constexpr std::size_t MAX_TERRAIN_GROUPS = 65536; // Capture policy, not a native limit.

// Caller retains a valid Zone and the native publication lock for the WHOLE
// traversal. Header/link rechecks cannot replace locking or extend lifetimes.
// Pending lists, geometry counts and CPU vertex bytes are deliberately unread.
template <class Read>
bool readTerrainVertexBuffers(std::uintptr_t module, std::uintptr_t zone, std::vector<VertexBufferView> &output,
                              Read read) {
    output.clear();
    if (!vertexBufferModuleFits(module))
        return false;
    std::array<std::uintptr_t, 2> header{};
    if (!readSpriteField(zone, 0x1c8, header.data(), sizeof header, read) || !header[0] ||
        header[1] > MAX_TERRAIN_GROUPS)
        return false;
    const auto sentinel = header[0], count = header[1];
    std::array<std::uintptr_t, 2> ends{};
    if (!readSpriteField(sentinel, 0, ends.data(), sizeof ends, read))
        return false;
    std::vector<VertexBufferView> result;
    result.reserve(count);
    auto previous = sentinel, node = ends[0];
    std::size_t visited = 0;
    while (node != sentinel) {
        if (!node || visited >= count)
            return false;
        std::array<std::uintptr_t, 3> links{};
        if (!readSpriteField(node, 0, links.data(), sizeof links, read) || links[1] != previous || !links[2])
            return false;
        std::uintptr_t wrapper = 0;
        if (!readSpriteField(links[2], 8, &wrapper, sizeof wrapper, read))
            return false;
        VertexBufferView resource;
        if (!readVertexBufferWrapper(module, wrapper, resource, read))
            return false;
        std::uintptr_t currentWrapper = 0;
        if (!readSpriteField(links[2], 8, &currentWrapper, sizeof currentWrapper, read) || currentWrapper != wrapper)
            return false;
        result.push_back(resource);
        previous = node;
        node = links[0];
        ++visited;
    }
    if (visited != count || ends[1] != previous)
        return false;
    std::array<std::uintptr_t, 2> currentHeader{}, currentEnds{};
    if (!readSpriteField(zone, 0x1c8, currentHeader.data(), sizeof currentHeader, read) || currentHeader != header ||
        !readSpriteField(sentinel, 0, currentEnds.data(), sizeof currentEnds, read) || currentEnds != ends)
        return false;
    output = std::move(result);
    return true;
}
} // namespace captureabi
