#include "DrawnState.h"
#include "GameMath.h"
#include "GameAngles.h"
#include <array>
#include <cstring>

namespace drawnstate {
namespace {
template <class T> T read(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
template <class T> void write(uint8_t *record, size_t offset, T value) {
    std::memcpy(record + offset, &value, sizeof value);
}
} // namespace

void update(uint8_t *record, float frameMs) {
    if (read<float>(record, 0xb58) > 0.0f) {
        const float factor = gamemath::blendFactor(double(frameMs), .02f) + 0.0f;
        const int64_t weight = int64_t(factor * 65536.0f);
        const auto physical = read<std::array<int64_t, 3>>(record, 0x10);
        for (int axis = 0; axis < 3; ++axis) {
            const size_t offset = 0xb10 + axis * 8;
            const int64_t old = read<int64_t>(record, offset);
            const int64_t delta = gamemath::subtractFixed(physical[axis], old);
            const int64_t product = gamemath::multiplyFixed(delta, weight);
            write(record, offset, gamemath::addFixed(old, product / 65536));
        }
        // Native Z has a second float residual correction after the quantized XYZ pass.
        const int64_t z = read<int64_t>(record, 0xb20);
        const float residual = float(gamemath::subtractFixed(read<int64_t>(record, 0x20), z));
        write(record, 0xb20, gamemath::addFixed(z, int64_t(residual * factor)));
        for (int axis = 0; axis < 3; ++axis) {
            const float old = read<float>(record, 0xb34 + axis * 4);
            const float target = read<float>(record, 0x28 + axis * 4);
            write(record, 0xb34 + axis * 4, gameangles::turn(old, target, factor));
        }
        const float increment = factor * 32.0f;
        write(record, 0x6c, read<float>(record, 0x6c) + increment);
        if (read<float>(record, 0x990) < 0.0f)
            write(record, 0x990, 0.0f);
    } else {
        std::memcpy(record + 0xb10, record + 0x10, 3 * sizeof(int64_t));
        std::memcpy(record + 0xb34, record + 0x28, 3 * sizeof(float));
    }
    const float rate = record[0x60] == 0 ? .05f : .01f;
    const float lookFactor = gamemath::blendFactor(double(frameMs), rate) + 0.0f;
    for (int axis = 0; axis < 3; ++axis) {
        const float old = read<float>(record, 0xb4c + axis * 4);
        const float delta = read<float>(record, 0x174 + axis * 4) - old;
        write(record, 0xb4c + axis * 4, old + delta * lookFactor);
    }
}
} // namespace drawnstate
