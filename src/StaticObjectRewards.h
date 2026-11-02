#pragma once
// Project-owned CWR1 reward-map chunk. Native ABI is confined to the capture adapter.
#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <optional>
#include <vector>

namespace objectrewards {
constexpr uint32_t CHUNK = 0xffff0002u;
constexpr uint32_t RECORD_BYTES = 4 + 0xa0;
using Item = std::array<uint8_t, 0xa0>;
using Map = std::map<int32_t, Item>;

inline std::vector<uint8_t> encode(const Map &rewards) {
    std::vector<uint8_t> bytes;
    bytes.reserve(rewards.size() * RECORD_BYTES);
    for (const auto &[index, item] : rewards) {
        uint32_t key;
        std::memcpy(&key, &index, sizeof key);
        for (int byte = 0; byte < 4; ++byte)
            bytes.push_back(uint8_t(key >> (8 * byte)));
        bytes.insert(bytes.end(), item.begin(), item.end());
    }
    return bytes;
}

inline std::optional<Map> decode(const uint8_t *bytes, size_t size) {
    if (size % RECORD_BYTES || (size && !bytes))
        return {};
    Map rewards;
    std::optional<int32_t> previous;
    for (size_t at = 0; at < size; at += RECORD_BYTES) {
        uint32_t key = 0;
        for (int byte = 0; byte < 4; ++byte)
            key |= uint32_t(bytes[at + byte]) << (8 * byte);
        int32_t index;
        std::memcpy(&index, &key, sizeof index);
        if (previous && index <= *previous)
            return {}; // duplicate or out-of-order keys cannot represent this map
        Item item;
        std::memcpy(item.data(), bytes + at + 4, item.size());
        rewards.emplace(index, item);
        previous = index;
    }
    return rewards; // an explicit zero-entry chunk is known empty, not missing data
}
inline std::optional<Map> decode(const std::vector<uint8_t> &bytes) {
    return decode(bytes.data(), bytes.size());
}
} // namespace objectrewards
