#include "PanelScroll.h"
#include <bit>
#include <limits>

namespace panelscroll {
namespace {
int32_t word(uint32_t value) {
    return std::bit_cast<int32_t>(value);
}
int32_t add(int32_t a, int32_t b) {
    return word(uint32_t(a) + uint32_t(b));
}
int32_t subtract(int32_t a, int32_t b) {
    return word(uint32_t(a) - uint32_t(b));
}
int32_t multiply(int32_t a, int32_t b) {
    return word(uint32_t(a) * uint32_t(b));
}
int32_t truncate(float value) {
    if (!(value >= -2147483648.f && value < 2147483648.f))
        return std::numeric_limits<int32_t>::min();
    return int32_t(value);
}
uint64_t widen(int32_t value) {
    return uint64_t(int64_t(value));
}
float unsignedFloat(uint64_t value) {
    float converted = float(std::bit_cast<int64_t>(value));
    if (value >> 63)
        converted += 18446744073709551616.f;
    return converted;
}
int32_t track(float height, float inset) {
    const float body = height - 150.f;
    return truncate(body - inset);
}
std::optional<int32_t> divide(int32_t numerator, int32_t denominator) {
    if (!denominator || (numerator == std::numeric_limits<int32_t>::min() && denominator == -1))
        return std::nullopt;
    return numerator / denominator;
}
} // namespace

Change controls(int32_t first, int wheelDelta, float localHeight) {
    if (!wheelDelta)
        return {first, false};
    if (wheelDelta > 0) {
        first = subtract(first, 1);
        return {first < 0 ? 0 : first, true};
    }
    const int32_t rows = track(localHeight, 35) / 30;
    first = add(first, 1);
    if (first >= subtract(23, rows))
        first = subtract(22, rows);
    return {first, true};
}

Change friends(int32_t first, int wheelDelta, float localHeight, uint64_t storedRows) {
    if (!wheelDelta)
        return {first, false};
    if (wheelDelta > 0) {
        first = subtract(first, 1);
        return {first < 0 ? 0 : first, true};
    }
    const int32_t rows = track(localHeight, 60) / 30;
    if (storedRows <= widen(rows))
        return {0, false};
    first = add(first, 1);
    const uint64_t last = storedRows - widen(rows) - 1;
    if (widen(first) >= last)
        first = word(uint32_t(last));
    return {first < 0 ? 0 : first, true};
}

std::optional<Thumb> controlsThumb(int32_t first, float width, float height, float thumbWidth) {
    const int32_t distance = track(height, 35), rows = distance / 30;
    const int32_t thumbHeight = multiply(rows, distance) / 23;
    const auto offset = divide(multiply(subtract(distance, thumbHeight), first), subtract(22, rows));
    if (!offset)
        return std::nullopt;
    return Thumb{width - 28.f, float(add(35, *offset)), thumbWidth, float(thumbHeight)};
}

std::optional<Thumb> friendsThumb(int32_t first, float width, float height, float thumbWidth, uint64_t storedRows) {
    const int32_t distance = track(height, 60), rows = distance / 30;
    if (storedRows <= widen(add(rows, 1)))
        return Thumb{width - 28.f, 60, thumbWidth, float(distance)};
    const int32_t thumbHeight = word(uint32_t(widen(multiply(rows, distance)) / storedRows));
    const uint64_t denominator = storedRows - widen(rows) - 1;
    if (!denominator)
        return std::nullopt;
    const uint64_t y = widen(multiply(subtract(distance, thumbHeight), first)) / denominator + 60;
    return Thumb{width - 28.f, unsignedFloat(y), thumbWidth, float(thumbHeight)};
}
} // namespace panelscroll
