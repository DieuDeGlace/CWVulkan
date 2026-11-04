#pragma once
// Original project CWZ1 encoding. Native storage and derived biome labels stay
// separate; no effective-reader water/air substitution is applied to stored bytes.
#include "NativeZoneTerrain.h"
#include <type_traits>

namespace zonecapture {
namespace detail {
template <class T> void append(std::vector<std::uint8_t> &out, T value) {
    using Unsigned = typename std::make_unsigned<T>::type;
    const auto word = static_cast<Unsigned>(value);
    for (std::size_t byte = 0; byte < sizeof(T); ++byte)
        out.push_back(static_cast<std::uint8_t>(word >> (byte * 8)));
}
} // namespace detail

inline std::vector<std::uint8_t> encode(const captureabi::ZoneTerrain &terrain, std::int32_t derivedBiome) {
    if (terrain.columns.size() != captureabi::ZONE_COLUMN_COUNT)
        return {};
    // Same file ceiling as the engine reader; not a native terrain limit.
    constexpr std::size_t maximumBytes = 128 * 1024 * 1024;
    std::vector<std::uint8_t> out;
    out.reserve(256 * 1024);
    out.insert(out.end(), {'C', 'W', 'Z', '1'});
    detail::append(out, terrain.coordinates[0]);
    detail::append(out, terrain.coordinates[1]);
    detail::append(out, derivedBiome);
    detail::append(out, std::int32_t(0));
    for (const auto &column : terrain.columns) {
        if (out.size() > maximumBytes - 8)
            return {};
        detail::append(out, column.baseZ);
        const auto runsAt = out.size();
        detail::append(out, std::uint32_t(0));
        std::uint32_t runs = 0;
        for (std::size_t first = 0; first < column.blocks.size();) {
            std::size_t end = first + 1;
            while (end < column.blocks.size() && end - first < 65535 && column.blocks[end] == column.blocks[first])
                ++end;
            if (out.size() > maximumBytes - 8)
                return {};
            detail::append(out, static_cast<std::uint16_t>(end - first));
            const auto &block = column.blocks[first];
            out.insert(out.end(), block.begin(), block.end());
            ++runs;
            first = end;
        }
        for (unsigned byte = 0; byte < 4; ++byte)
            out[runsAt + byte] = static_cast<std::uint8_t>(runs >> (byte * 8));
    }
    return out;
}
} // namespace zonecapture
