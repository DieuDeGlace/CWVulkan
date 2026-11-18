#pragma once
#include "GameMath.h"
#include <cmath>

namespace gamemath {
// Steam's common range test, also used on positions inside packed network records.
inline bool outsideFixedRange(const void *first, const void *second, float radius) {
    const auto delta = [&](size_t axis) {
        int64_t a, b;
        std::memcpy(&a, static_cast<const uint8_t *>(first) + axis * sizeof a, sizeof a);
        std::memcpy(&b, static_cast<const uint8_t *>(second) + axis * sizeof b, sizeof b);
        return double(subtractFixed(a, b)) * (1.0 / 65536.0);
    };
    const double x = delta(0);
    if (std::abs(x) > radius)
        return true;
    const double y = delta(1);
    if (std::abs(y) > radius)
        return true;
    const double z = delta(2);
    return (y * y + x * x) + z * z > double(radius * radius);
}
} // namespace gamemath
