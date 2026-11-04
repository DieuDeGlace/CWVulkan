#pragma once
// Project-owned CWA1/CWA2 wire values. Legacy padding is part of the format,
// not a description of the native game's animation-state allocation.
#include <array>
#include <cstdint>
#include <cstring>

namespace recording {
constexpr int SPRITES = 14, MATRICES = 33;
struct Frame {
    uint64_t ms = 0;
    int64_t position[3] = {0, 0, 0}; // dots; zero in CWA1
    int32_t state = 0, timer = 0, flags = 0, ability = 0;
    float sinceAbility = 0, yaw = 0, velocity[3] = {0, 0, 0};
    int32_t slots[SPRITES]{};
    float matrices[MATRICES][16]{};
};

constexpr size_t POSITIONED_FRAME_BYTES = 8 + 24 + 16 + 20 + SPRITES * 4 + MATRICES * 64;
inline std::array<uint8_t, POSITIONED_FRAME_BYTES> encodePositioned(const Frame &frame) {
    std::array<uint8_t, POSITIONED_FRAME_BYTES> bytes{};
    auto *out = bytes.data();
    const auto put = [&](const void *source, size_t count) {
        std::memcpy(out, source, count);
        out += count;
    };
    put(&frame.ms, 8);
    put(frame.position, 24);
    put(&frame.state, 4);
    put(&frame.timer, 4);
    put(&frame.flags, 4);
    put(&frame.ability, 4);
    put(&frame.sinceAbility, 4);
    put(&frame.yaw, 4);
    put(frame.velocity, 12);
    put(frame.slots, sizeof frame.slots);
    put(frame.matrices, sizeof frame.matrices);
    return bytes;
}
} // namespace recording
