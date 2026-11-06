#pragma once
// CWR1 owner-data chunk for the Zone's retained object-mark coordinates.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <set>
#include <tuple>
#include <vector>

namespace objectoccupancy {
inline constexpr uint32_t Chunk = 0xe8;
inline constexpr uint32_t RecordBytes = 12;
inline constexpr size_t MaxPositions = 262144;
using Positions = std::set<std::tuple<int32_t, int32_t, int32_t>>;

inline std::vector<uint8_t> encode(const Positions &positions) {
    std::vector<uint8_t> bytes;
    bytes.reserve(positions.size() * RecordBytes);
    for (const auto &[x, y, z] : positions) {
        const int32_t point[] = {x, y, z};
        const auto *begin = reinterpret_cast<const uint8_t *>(point);
        bytes.insert(bytes.end(), begin, begin + sizeof point);
    }
    return bytes;
}

inline std::optional<Positions> decode(const uint8_t *bytes, size_t size) {
    if (size % RecordBytes || size / RecordBytes > MaxPositions)
        return std::nullopt;
    Positions positions;
    for (size_t at = 0; at < size; at += RecordBytes) {
        int32_t point[3];
        std::memcpy(point, bytes + at, sizeof point);
        const auto key = std::make_tuple(point[0], point[1], point[2]);
        if (!positions.empty() && !(*positions.rbegin() < key))
            return std::nullopt;
        positions.emplace_hint(positions.end(), key);
    }
    return positions;
}
} // namespace objectoccupancy
