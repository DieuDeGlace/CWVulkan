#pragma once
// Owner-extracted procedural voxel resources. Native facts: steam-procedural-item-resources.md.
#include <array>
#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace proceduralitem {
struct Key {
    int32_t id = 0;
    uint32_t modifier = 0;
    bool operator==(const Key &other) const { return id == other.id && modifier == other.modifier; }
    bool operator<(const Key &other) const;
};
enum class Origin : uint8_t { Generated, NativeLookup, NativeGeneration };
struct Resource {
    Key key;
    std::array<int32_t, 3> dimensions{24, 24, 24};
    uint8_t classification = 1, mode = 1;
    std::array<uint8_t, 3> shade{};
    int32_t radius = 3;
    Origin origin = Origin::Generated;
    uint32_t sourceTimestamp = 0;
    std::vector<uint8_t> rgb; // X fastest, then Y, then Z; full allocation, never cropped.
};
inline constexpr size_t HEADER_SIZE = 44;
inline constexpr size_t MAX_RGB_BYTES = 3 * 256 * 256 * 256;
// Both functions leave their destination unchanged on failure.
bool decode(const std::vector<uint8_t> &, Resource &, std::string *error = nullptr);
bool encode(const Resource &, std::vector<uint8_t> &, std::string *error = nullptr);
std::string fileName(Key);
inline constexpr size_t MAX_REQUEST_KEYS = 512, MAX_REQUEST_BYTES = 65536;
bool decodeRequest(std::string_view, std::vector<Key> &, std::string *error = nullptr);
} // namespace proceduralitem
