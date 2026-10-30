#pragma once
// Project-owned CWR1 extension: coherent native mesh-cache evidence, not generated assets.
// Format: docs/extracted-scene-metadata.md. Native field facts: steam-scene-lighting.md §1.
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>

namespace scenecapture {
constexpr uint32_t MESH_CACHE_RECORD = 0xffff0001u;
constexpr size_t MESH_CACHE_BYTES = 112;
struct MeshCache {
    std::array<int64_t, 3> minimum{}, maximum{}, center{};
    uint64_t activeGroups = 0, pendingGroups = 0;
    float radius = 0;
    uint8_t dirty = 0;
    int32_t triangleCost = 0; // native weighted mesh budget, not a vertex count
};
using MeshBytes = std::array<uint8_t, MESH_CACHE_BYTES>;

namespace detail {
inline void put(uint8_t *out, uint64_t value, size_t bytes) {
    for (size_t i = 0; i < bytes; ++i)
        out[i] = (uint8_t)(value >> (8 * i));
}
inline uint64_t get(const uint8_t *in, size_t bytes) {
    uint64_t value = 0;
    for (size_t i = 0; i < bytes; ++i)
        value |= (uint64_t)in[i] << (8 * i);
    return value;
}
}

inline MeshBytes encode(const MeshCache &cache) {
    MeshBytes bytes{};
    detail::put(bytes.data(), 1, 4); // format version
    detail::put(bytes.data() + 4, 1, 4); // read under the native mesh-cache lock
    detail::put(bytes.data() + 8, cache.activeGroups, 8);
    detail::put(bytes.data() + 16, cache.pendingGroups, 8);
    const std::array<int64_t, 3> *vectors[] = {&cache.minimum, &cache.maximum, &cache.center};
    for (size_t vector = 0; vector < 3; ++vector)
        for (size_t axis = 0; axis < 3; ++axis)
            detail::put(bytes.data() + 24 + 24 * vector + 8 * axis, (uint64_t)(*vectors[vector])[axis], 8);
    uint32_t radius;
    std::memcpy(&radius, &cache.radius, sizeof radius);
    detail::put(bytes.data() + 96, radius, 4);
    bytes[100] = cache.dirty;
    detail::put(bytes.data() + 104, (uint32_t)cache.triangleCost, 4);
    return bytes;
}

inline std::optional<MeshCache> decode(const uint8_t *bytes, size_t size) {
    if (!bytes || size != MESH_CACHE_BYTES || detail::get(bytes, 4) != 1 || detail::get(bytes + 4, 4) != 1)
        return std::nullopt;
    MeshCache cache;
    cache.activeGroups = detail::get(bytes + 8, 8);
    cache.pendingGroups = detail::get(bytes + 16, 8);
    if (!cache.activeGroups && !cache.pendingGroups)
        return std::nullopt; // zero counts cannot distinguish an unwritten cache from a completed empty mesh
    std::array<int64_t, 3> *vectors[] = {&cache.minimum, &cache.maximum, &cache.center};
    for (size_t vector = 0; vector < 3; ++vector)
        for (size_t axis = 0; axis < 3; ++axis) {
            const uint64_t value = detail::get(bytes + 24 + 24 * vector + 8 * axis, 8);
            std::memcpy(&(*vectors[vector])[axis], &value, sizeof value);
        }
    const uint32_t radius = (uint32_t)detail::get(bytes + 96, 4);
    std::memcpy(&cache.radius, &radius, sizeof radius);
    cache.dirty = bytes[100];
    const uint32_t cost = (uint32_t)detail::get(bytes + 104, 4);
    std::memcpy(&cache.triangleCost, &cost, sizeof cost);
    for (int axis = 0; axis < 3; ++axis)
        if (cache.minimum[axis] > cache.maximum[axis])
            return std::nullopt;
    if (!std::isfinite(cache.radius) || cache.radius < 0)
        return std::nullopt;
    return cache;
}
} // namespace scenecapture
